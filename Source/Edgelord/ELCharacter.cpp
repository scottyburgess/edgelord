#include "ELCharacter.h"
#include "Engine/LocalPlayer.h"
#include "Camera/CameraComponent.h"
#include "Components/CapsuleComponent.h"
#include "Components/SkeletalMeshComponent.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "GameFramework/SpringArmComponent.h"
#include "GameFramework/Controller.h"
#include "EnhancedInputComponent.h"
#include "EnhancedInputSubsystems.h"
#include "InputActionValue.h"
#include "PhysicsEngine/PhysicalAnimationComponent.h"
#include "Net/UnrealNetwork.h"
#include "UELGrabComponent.h"
#include "UELCarryComponent.h"
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
    GrabComponent = CreateDefaultSubobject<UELGrabComponent>(TEXT("GrabComponent"));
    CarryComponent = CreateDefaultSubobject<UELCarryComponent>(TEXT("CarryComponent"));

    // TODO: tune all profile values empirically - hard cap 1 week total

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

        FTimerHandle TimerHandle;
        GetWorld()->GetTimerManager().SetTimer(TimerHandle, [this]()
            {
                if (GetMesh())
                {
                    GetMesh()->SetAllBodiesSimulatePhysics(false);
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
    }
}

void AELCharacter::GetLifetimeReplicatedProps(
    TArray<FLifetimeProperty>& OutLifetimeProps) const
{
    Super::GetLifetimeReplicatedProps(OutLifetimeProps);
    DOREPLIFETIME(AELCharacter, CurrentPhysicsProfile);
    DOREPLIFETIME(AELCharacter, CurrentGrabState);
    DOREPLIFETIME(AELCharacter, CarriedByCharacter);
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

void AELCharacter::OnRep_GrabState()
{
    // TODO: drive grab animation state from CurrentGrabState when AnimBP supports it
}

void AELCharacter::SetGrabState(EELGrabState NewState)
{
    if (!HasAuthority())
        ServerSetGrabState(NewState);
    else
        CurrentGrabState = NewState;

    // Notify carry component locally so it can react to grab changes
    if (CarryComponent)
        CarryComponent->OnGrabStateChanged();
}

void AELCharacter::ServerSetGrabState_Implementation(EELGrabState NewState)
{
    CurrentGrabState = NewState;
}

void AELCharacter::OnRep_CarriedBy()
{
    UCharacterMovementComponent* MoveComp = GetCharacterMovement();
    USkeletalMeshComponent* SkelMesh = GetMesh();

    if (CarriedByCharacter)
    {
        // Go limp: full ragdoll, pelvis simulates so the constraint can drag us
        SetPhysicsProfile(EELPhysicsProfile::Ragdoll);
        if (SkelMesh)
            SkelMesh->SetBodySimulatePhysics(FName("pelvis"), true);
        if (MoveComp)
            MoveComp->DisableMovement();

        // Attach our whole actor to the carrier so the capsule actually follows them
        AttachToActor(CarriedByCharacter, FAttachmentTransformRules::KeepWorldTransform);
    }
    else
    {
        // Detach, restore physics state
        DetachFromActor(FDetachmentTransformRules::KeepWorldTransform);

        if (SkelMesh)
            SkelMesh->SetBodySimulatePhysics(FName("pelvis"), false);
        SetPhysicsProfile(EELPhysicsProfile::Standing);
        if (MoveComp)
            MoveComp->SetMovementMode(MOVE_Walking);
    }
}

void AELCharacter::ServerSetCarriedBy_Implementation(AELCharacter* NewCarrier)
{
    CarriedByCharacter = NewCarrier;
    OnRep_CarriedBy(); // run effects on server too
}

UPrimitiveComponent* AELCharacter::GetGrabbedComponent_Implementation()
{
    return GetMesh();
}

FName AELCharacter::GetGrabbedBoneName_Implementation()
{
    return NAME_None;  // let GrabComponent pick the nearest simulated body
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

    PhysicalAnimation->ApplyPhysicalAnimationSettingsBelow(FName("pelvis"), Data, true);
}

void AELCharacter::SetupPlayerInputComponent(UInputComponent* PlayerInputComponent)
{
    if (UEnhancedInputComponent* EIC = Cast<UEnhancedInputComponent>(PlayerInputComponent))
    {
        EIC->BindAction(JumpAction, ETriggerEvent::Started, this, &ACharacter::Jump);
        EIC->BindAction(JumpAction, ETriggerEvent::Completed, this, &ACharacter::StopJumping);
        EIC->BindAction(MoveAction, ETriggerEvent::Triggered, this, &AELCharacter::Move);
        EIC->BindAction(MouseLookAction, ETriggerEvent::Triggered, this, &AELCharacter::Look);
        EIC->BindAction(LookAction, ETriggerEvent::Triggered, this, &AELCharacter::Look);

        if (GrabLeftAction)
        {
            EIC->BindAction(GrabLeftAction, ETriggerEvent::Started, this, &AELCharacter::Input_GrabLeft_Started);
            EIC->BindAction(GrabLeftAction, ETriggerEvent::Completed, this, &AELCharacter::Input_GrabLeft_Completed);
        }
        if (GrabRightAction)
        {
            EIC->BindAction(GrabRightAction, ETriggerEvent::Started, this, &AELCharacter::Input_GrabRight_Started);
            EIC->BindAction(GrabRightAction, ETriggerEvent::Completed, this, &AELCharacter::Input_GrabRight_Completed);
        }
    }
    else
    {
        UE_LOG(LogEdgelord, Error, TEXT("'%s' Failed to find an Enhanced Input component!"),
            *GetNameSafe(this));
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

void AELCharacter::Input_GrabLeft_Started(const FInputActionValue& Value)
{
    GrabComponent->TryGrab(EELHandSide::Left);
}
void AELCharacter::Input_GrabLeft_Completed(const FInputActionValue& Value)
{
    GrabComponent->ReleaseGrab(EELHandSide::Left);
}
void AELCharacter::Input_GrabRight_Started(const FInputActionValue& Value)
{
    GrabComponent->TryGrab(EELHandSide::Right);
}
void AELCharacter::Input_GrabRight_Completed(const FInputActionValue& Value)
{
    GrabComponent->ReleaseGrab(EELHandSide::Right);
}

void AELCharacter::DoMove(float Right, float Forward)
{
    if (CarriedByCharacter) return;  // carry disables locomotion input

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

void AELCharacter::DoJumpStart() { if (!CarriedByCharacter) Jump(); }
void AELCharacter::DoJumpEnd() { StopJumping(); }