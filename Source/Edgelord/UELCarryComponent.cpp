#include "UELCarryComponent.h"
#include "ELCharacter.h"
#include "UELGrabComponent.h"
#include "GameFramework/CharacterMovementComponent.h"

UELCarryComponent::UELCarryComponent()
{
    PrimaryComponentTick.bCanEverTick = false;
}

void UELCarryComponent::BeginPlay()
{
    Super::BeginPlay();

    if (AELCharacter* Owner = Cast<AELCharacter>(GetOwner()))
    {
        if (UCharacterMovementComponent* MoveComp = Owner->GetCharacterMovement())
            OriginalMaxWalkSpeed = MoveComp->MaxWalkSpeed;
    }
}

void UELCarryComponent::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
    if (CurrentlyCarried.IsValid())
        StopCarrying();
    Super::EndPlay(EndPlayReason);
}

void UELCarryComponent::OnGrabStateChanged()
{
    AELCharacter* Owner = Cast<AELCharacter>(GetOwner());
    if (!Owner || !Owner->IsLocallyControlled()) return;

    UELGrabComponent* Grab = Owner->GetGrabComponent();
    if (!Grab) return;

    // Find any AELCharacter we might be grabbing (either hand — first one wins)
    AELCharacter* GrabbedChar = nullptr;
    if (AELCharacter* LC = Cast<AELCharacter>(Grab->GetGrabbedActor(EELHandSide::Left)))
        GrabbedChar = LC;
    else if (AELCharacter* RC = Cast<AELCharacter>(Grab->GetGrabbedActor(EELHandSide::Right)))
        GrabbedChar = RC;

    AELCharacter* Current = CurrentlyCarried.Get();
    if (GrabbedChar == Current) return;

    if (Current)      StopCarrying();
    if (GrabbedChar)  StartCarrying(GrabbedChar);
}

void UELCarryComponent::StartCarrying(AELCharacter* Target)
{
    if (!Target) return;
    CurrentlyCarried = Target;

    if (AELCharacter* Owner = Cast<AELCharacter>(GetOwner()))
    {
        if (UCharacterMovementComponent* MoveComp = Owner->GetCharacterMovement())
            MoveComp->MaxWalkSpeed = OriginalMaxWalkSpeed * CarryingSpeedScale;
    }

    Target->ServerSetCarriedBy(Cast<AELCharacter>(GetOwner()));

    UE_LOG(LogTemp, Log, TEXT("UELCarryComponent: Started carrying %s"), *Target->GetName());
}

void UELCarryComponent::StopCarrying()
{
    AELCharacter* Target = CurrentlyCarried.Get();
    if (Target)
        Target->ServerSetCarriedBy(nullptr);

    if (AELCharacter* Owner = Cast<AELCharacter>(GetOwner()))
    {
        if (UCharacterMovementComponent* MoveComp = Owner->GetCharacterMovement())
            MoveComp->MaxWalkSpeed = OriginalMaxWalkSpeed;
    }

    UE_LOG(LogTemp, Log, TEXT("UELCarryComponent: Stopped carrying %s"),
        Target ? *Target->GetName() : TEXT("(invalid)"));

    CurrentlyCarried.Reset();
}