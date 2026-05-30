#include "UELGetUpComponent.h"
#include "ELCharacter.h"
#include "Components/SkeletalMeshComponent.h"
#include "Animation/AnimInstance.h"
#include "GameFramework/CharacterMovementComponent.h"

UELGetUpComponent::UELGetUpComponent()
{
    PrimaryComponentTick.bCanEverTick = true;
}

void UELGetUpComponent::BeginPlay()
{
    Super::BeginPlay();
}

void UELGetUpComponent::TickComponent(float DeltaTime, ELevelTick TickType,
    FActorComponentTickFunction* ThisTickFunction)
    
{
    Super::TickComponent(DeltaTime, TickType, ThisTickFunction);

    if (bGettingUp) return;

    AELCharacter* Owner = Cast<AELCharacter>(GetOwner());
    if (!Owner) return;

    if (Owner->IsCarried()) { HorizontalAccumulator = 0.f; return; }
    if (!Owner->HasAuthority()) return;

    USkeletalMeshComponent* Mesh = Owner->GetMesh();
    if (!Mesh) return;

    // Compare head Z to pelvis Z. Standing ~ 120cm gap, fallen ~ small/zero.
    const FVector PelvisLoc = Mesh->GetBoneLocation(PelvisBoneName);
    const FVector HeadLoc = Mesh->GetBoneLocation(HeadBoneName);
    const float VerticalGap = HeadLoc.Z - PelvisLoc.Z;

    if (VerticalGap < UprightHeightThreshold)
    {
        HorizontalAccumulator += DeltaTime;
        if (HorizontalAccumulator >= HorizontalDuration)
            StartGetUp();
    }
    else
    {
        HorizontalAccumulator = 0.f;
    }
}

void UELGetUpComponent::StartGetUp()
{
    AELCharacter* Owner = Cast<AELCharacter>(GetOwner());
    if (!Owner) return;

    bGettingUp = true;
    HorizontalAccumulator = 0.f;

    Owner->SetPhysicsProfile(EELPhysicsProfile::GettingUp);

    if (GetUpMontage)
    {
        if (USkeletalMeshComponent* Mesh = Owner->GetMesh())
        {
            if (UAnimInstance* Anim = Mesh->GetAnimInstance())
                Anim->Montage_Play(GetUpMontage);
        }
    }

    GetWorld()->GetTimerManager().SetTimer(GetUpTimerHandle, this,
        &UELGetUpComponent::FinishGetUp, GetUpDuration, false);

    UE_LOG(LogTemp, Log, TEXT("UELGetUpComponent: Starting get-up on %s"), *Owner->GetName());
}

void UELGetUpComponent::FinishGetUp()
{
    AELCharacter* Owner = Cast<AELCharacter>(GetOwner());
    if (!Owner)
    {
        bGettingUp = false;
        return;
    }

    if (USkeletalMeshComponent* Mesh = Owner->GetMesh())
        Mesh->SetBodySimulatePhysics(PelvisBoneName, false);

    if (UCharacterMovementComponent* MoveComp = Owner->GetCharacterMovement())
        MoveComp->SetMovementMode(MOVE_Walking);

    Owner->SetPhysicsProfile(EELPhysicsProfile::Standing);

    // If we recovered while still tethered to a carrier, tell their carry component
    // so they flip us back to Stable rather than leaving us in a half-dragged limbo.
    if (Owner->IsDragged())
    {
        if (AELCharacter* Carrier = Owner->GetCarrier())
        {
            if (UELCarryComponent* CarrierCarry = Carrier->GetCarryComponent())
                CarrierCarry->OnGrabbeeRecovered(Owner);
        }
    }

    // Cooldown before we can detect again
    GetWorld()->GetTimerManager().SetTimer(GetUpTimerHandle,
        [this]() { bGettingUp = false; },
        PostGetUpCooldown, false);

    UE_LOG(LogTemp, Log, TEXT("UELGetUpComponent: Finished get-up on %s"), *Owner->GetName());
}