// Copyright Epic Games, Inc. All Rights Reserved.

#include "ELCharacter.h"
#include "Engine/LocalPlayer.h"
#include "Camera/CameraComponent.h"
#include "Components/CapsuleComponent.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "GameFramework/SpringArmComponent.h"
#include "GameFramework/Controller.h"
#include "EnhancedInputComponent.h"
#include "EnhancedInputSubsystems.h"
#include "InputActionValue.h"
#include "PhysicsEngine/PhysicalAnimationComponent.h"
#include "Net/UnrealNetwork.h"
#include "Edgelord.h"

AELCharacter::AELCharacter()
{
    GetCapsuleComponent()->InitCapsuleSize(42.f, 96.0f);

    bUseControllerRotationPitch = false;
    bUseControllerRotationYaw = false;
    bUseControllerRotationRoll = false;

    GetCharacterMovement()->bOrientRotationToMovement = true;
    GetCharacterMovement()->RotationRate = FRotator(0.0f, 500.0f, 0.0f);
    GetCharacterMovement()->JumpZVelocity = 500.f;
    GetCharacterMovement()->AirControl = 0.35f;
    GetCharacterMovement()->MaxWalkSpeed = 500.f;
    GetCharacterMovement()->MinAnalogWalkSpeed = 20.f;
    GetCharacterMovement()->BrakingDecelerationWalking = 2000.f;
    GetCharacterMovement()->BrakingDecelerationFalling = 1500.0f;

    CameraBoom = CreateDefaultSubobject<USpringArmComponent>(TEXT("CameraBoom"));
    CameraBoom->SetupAttachment(RootComponent);
    CameraBoom->TargetArmLength = 400.0f;
    CameraBoom->bUsePawnControlRotation = true;

    FollowCamera = CreateDefaultSubobject<UCameraComponent>(TEXT("FollowCamera"));
    FollowCamera->SetupAttachment(CameraBoom, USpringArmComponent::SocketName);
    FollowCamera->bUsePawnControlRotation = false;

    PhysicalAnimation = CreateDefaultSubobject<UPhysicalAnimationComponent>(TEXT("PhysicalAnimation"));

    // TODO: tune these values empirically - hard cap 1 week total tuning

    StandingProfile.OrientationStrength = 1000.f;
    StandingProfile.AngularVelocityStrength = 100.f;
    StandingProfile.PositionStrength = 1000.f;
    StandingProfile.VelocityStrength = 100.f;
    StandingProfile.MaxLinearForce = 100.f;
    StandingProfile.MaxAngularForce = 100.f;

    WalkingProfile.OrientationStrength = 500.f;
    WalkingProfile.AngularVelocityStrength = 50.f;
    WalkingProfile.PositionStrength = 0.f;
    WalkingProfile.VelocityStrength = 0.f;
    WalkingProfile.MaxLinearForce = 0.f;
    WalkingProfile.MaxAngularForce = 0.f;

    GrabbingProfile.OrientationStrength = 1000.f;
    GrabbingProfile.AngularVelocityStrength = 100.f;
    GrabbingProfile.PositionStrength = 0.f;
    GrabbingProfile.VelocityStrength = 0.f;
    GrabbingProfile.MaxLinearForce = 0.f;
    GrabbingProfile.MaxAngularForce = 0.f;

    RagdollProfile.OrientationStrength = 0.f;
    RagdollProfile.AngularVelocityStrength = 0.f;
    RagdollProfile.PositionStrength = 0.f;
    RagdollProfile.VelocityStrength = 0.f;
    RagdollProfile.MaxLinearForce = 0.f;
    RagdollProfile.MaxAngularForce = 0.f;

    GettingUpProfile.OrientationStrength = 200.f;
    GettingUpProfile.AngularVelocityStrength = 20.f;
    GettingUpProfile.PositionStrength = 0.f;
    GettingUpProfile.VelocityStrength = 0.f;
    GettingUpProfile.MaxLinearForce = 0.f;
    GettingUpProfile.MaxAngularForce = 0.f;
}

void AELCharacter::BeginPlay()
{
    Super::BeginPlay();

    UE_LOG(LogTemp, Warning, TEXT("AELCharacter::BeginPlay called. PhysAnim=%s Mesh=%s"),
        PhysicalAnimation ? TEXT("VALID") : TEXT("NULL"),
        GetMesh() ? TEXT("VALID") : TEXT("NULL"));

    if (PhysicalAnimation && GetMesh())
    {
        PhysicalAnimation->SetSkeletalMeshComponent(GetMesh());

        // Defer physics setup by one frame to ensure mesh is fully initialized
        FTimerHandle TimerHandle;
        GetWorld()->GetTimerManager().SetTimer(TimerHandle, [this]()
            {
                if (GetMesh())
                {
                    // Reset everything to kinematic first
                    GetMesh()->SetAllBodiesSimulatePhysics(false);

                    // Simulate everything below pelvis - pelvis stays kinematic as anchor
                    // IncludeSelf=false is the critical change from the old spine_03 setup
                    GetMesh()->SetAllBodiesBelowSimulatePhysics(FName("pelvis"), true, false);
                    GetMesh()->SetAllBodiesBelowPhysicsBlendWeight(FName("pelvis"), 1.0f);

                    SetPhysicsProfile(EELPhysicsProfile::Standing);

                    int32 Simulating = 0;
                    for (FBodyInstance* Body : GetMesh()->Bodies)
                    {
                        if (Body && Body->bSimulatePhysics) Simulating++;
                    }
                    UE_LOG(LogTemp, Warning, TEXT("Deferred setup: %d/%d bodies simulating"),
                        Simulating, GetMesh()->Bodies.Num());
                }
            }, 0.1f, false);

        UE_LOG(LogTemp, Warning, TEXT("PAC setup deferred."));
    }
}

void AELCharacter::GetLifetimeReplicatedProps(
    TArray<FLifetimeProperty>& OutLifetimeProps) const
{
    Super::GetLifetimeReplicatedProps(OutLifetimeProps);
    DOREPLIFETIME(AELCharacter, CurrentPhysicsProfile);
}

void AELCharacter::OnRep_PhysicsProfile()
{
    switch (CurrentPhysicsProfile)
    {
    case EELPhysicsProfile::Standing:   ApplyPhysicsProfile(StandingProfile);  break;
    case EELPhysicsProfile::Walking:    ApplyPhysicsProfile(WalkingProfile);   break;
    case EELPhysicsProfile::Grabbing:   ApplyPhysicsProfile(GrabbingProfile);  break;
    case EELPhysicsProfile::Ragdoll:    ApplyPhysicsProfile(RagdollProfile);   break;
    case EELPhysicsProfile::GettingUp:  ApplyPhysicsProfile(GettingUpProfile); break;
    }
}

void AELCharacter::SetPhysicsProfile(EELPhysicsProfile NewProfile)
{
    CurrentPhysicsProfile = NewProfile;

    switch (NewProfile)
    {
    case EELPhysicsProfile::Standing:   ApplyPhysicsProfile(StandingProfile);  break;
    case EELPhysicsProfile::Walking:    ApplyPhysicsProfile(WalkingProfile);   break;
    case EELPhysicsProfile::Grabbing:   ApplyPhysicsProfile(GrabbingProfile);  break;
    case EELPhysicsProfile::Ragdoll:    ApplyPhysicsProfile(RagdollProfile);   break;
    case EELPhysicsProfile::GettingUp:  ApplyPhysicsProfile(GettingUpProfile); break;
    }
}

void AELCharacter::ApplyPhysicsProfile(const FELPhysicsProfileSettings& Settings)
{
    if (!PhysicalAnimation) return;

    FPhysicalAnimationData Data;
    Data.bIsLocalSimulation = false;
    Data.OrientationStrength = Settings.OrientationStrength;
    Data.AngularVelocityStrength = Settings.AngularVelocityStrength;
    Data.PositionStrength = Settings.PositionStrength;
    Data.VelocityStrength = Settings.VelocityStrength;
    Data.MaxLinearForce = Settings.MaxLinearForce;
    Data.MaxAngularForce = Settings.MaxAngularForce;

    // Apply below pelvis - full body simulation with pelvis as kinematic anchor
    PhysicalAnimation->ApplyPhysicalAnimationSettingsBelow(FName("pelvis"), Data, true);
}

void AELCharacter::SetupPlayerInputComponent(UInputComponent* PlayerInputComponent)
{
    if (UEnhancedInputComponent* EnhancedInputComponent =
        Cast<UEnhancedInputComponent>(PlayerInputComponent))
    {
        EnhancedInputComponent->BindAction(JumpAction, ETriggerEvent::Started, this, &ACharacter::Jump);
        EnhancedInputComponent->BindAction(JumpAction, ETriggerEvent::Completed, this, &ACharacter::StopJumping);
        EnhancedInputComponent->BindAction(MoveAction, ETriggerEvent::Triggered, this, &AELCharacter::Move);
        EnhancedInputComponent->BindAction(MouseLookAction, ETriggerEvent::Triggered, this, &AELCharacter::Look);
        EnhancedInputComponent->BindAction(LookAction, ETriggerEvent::Triggered, this, &AELCharacter::Look);
    }
    else
    {
        UE_LOG(LogEdgelord, Error, TEXT("'%s' Failed to find an Enhanced Input component!"), *GetNameSafe(this));
    }
}

void AELCharacter::Move(const FInputActionValue& Value)
{
    FVector2D MovementVector = Value.Get<FVector2D>();
    DoMove(MovementVector.X, MovementVector.Y);
}

void AELCharacter::Look(const FInputActionValue& Value)
{
    FVector2D LookAxisVector = Value.Get<FVector2D>();
    DoLook(LookAxisVector.X, LookAxisVector.Y);
}

void AELCharacter::DoMove(float Right, float Forward)
{
    if (GetController() != nullptr)
    {
        const FRotator Rotation = GetController()->GetControlRotation();
        const FRotator YawRotation(0, Rotation.Yaw, 0);
        const FVector ForwardDirection = FRotationMatrix(YawRotation).GetUnitAxis(EAxis::X);
        const FVector RightDirection = FRotationMatrix(YawRotation).GetUnitAxis(EAxis::Y);
        AddMovementInput(ForwardDirection, Forward);
        AddMovementInput(RightDirection, Right);
    }
}

void AELCharacter::DoLook(float Yaw, float Pitch)
{
    if (GetController() != nullptr)
    {
        AddControllerYawInput(Yaw);
        AddControllerPitchInput(Pitch);
    }
}

void AELCharacter::DoJumpStart()
{
    Jump();
}

void AELCharacter::DoJumpEnd()
{
    StopJumping();
}