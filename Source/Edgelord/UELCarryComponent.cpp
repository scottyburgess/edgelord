#include "UELCarryComponent.h"
#include "ELCharacter.h"
#include "UELGrabComponent.h"
#include "GameFramework/CharacterMovementComponent.h"

UELCarryComponent::UELCarryComponent()
{
    PrimaryComponentTick.bCanEverTick = true;
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

void UELCarryComponent::TickComponent(float DeltaTime, ELevelTick TickType,
    FActorComponentTickFunction* ThisTickFunction)
{
    Super::TickComponent(DeltaTime, TickType, ThisTickFunction);

    if (CurrentState != EELCarryState::Stable) return;

    AELCharacter* Owner = Cast<AELCharacter>(GetOwner());
    AELCharacter* Grabbee = CurrentlyCarried.Get();
    if (!Owner || !Grabbee) return;

    const float Dist = FVector::Dist(Owner->GetActorLocation(), Grabbee->GetActorLocation());
    if (Dist > TetherBreakDistance)
    {
        OverStretchAccumulator += DeltaTime;
        if (OverStretchAccumulator >= TetherGracePeriod)
        {
            TransitionCarriedTo(EELCarryState::Dragged);
            OverStretchAccumulator = 0.f;
        }
    }
    else
    {
        OverStretchAccumulator = 0.f;
    }
}

void UELCarryComponent::OnGrabStateChanged()
{
    AELCharacter* Owner = Cast<AELCharacter>(GetOwner());
    if (!Owner || !Owner->IsLocallyControlled()) return;

    UELGrabComponent* Grab = Owner->GetGrabComponent();
    if (!Grab) return;

    AELCharacter* GrabbedChar = nullptr;
    if (AELCharacter* LC = Cast<AELCharacter>(Grab->GetGrabbedActor(EELHandSide::Left)))
        GrabbedChar = LC;
    else if (AELCharacter* RC = Cast<AELCharacter>(Grab->GetGrabbedActor(EELHandSide::Right)))
        GrabbedChar = RC;

    AELCharacter* Current = CurrentlyCarried.Get();
    if (GrabbedChar == Current) return;

    if (Current)     StopCarrying();
    if (GrabbedChar) StartCarrying(GrabbedChar);
}

void UELCarryComponent::OnGrabbeeRecovered(AELCharacter* Grabbee)
{
    if (Grabbee != CurrentlyCarried.Get()) return;
    if (CurrentState != EELCarryState::Dragged) return;

    TransitionCarriedTo(EELCarryState::Stable);
    OverStretchAccumulator = 0.f;
}

void UELCarryComponent::StartCarrying(AELCharacter* Target)
{
    if (!Target) return;
    CurrentlyCarried = Target;
    TransitionCarriedTo(EELCarryState::Stable);

    UE_LOG(LogTemp, Log, TEXT("UELCarryComponent: Started carrying %s (Stable)"), *Target->GetName());
}

void UELCarryComponent::StopCarrying()
{
    AELCharacter* Target = CurrentlyCarried.Get();
    if (Target)
        Target->ServerSetCarryState(EELCarryState::None, nullptr);

    CurrentState = EELCarryState::None;
    CurrentlyCarried.Reset();
    OverStretchAccumulator = 0.f;
    ApplyCarrierSpeed();

    UE_LOG(LogTemp, Log, TEXT("UELCarryComponent: Stopped carrying %s"),
        Target ? *Target->GetName() : TEXT("(invalid)"));
}

void UELCarryComponent::TransitionCarriedTo(EELCarryState NewState)
{
    AELCharacter* Target = CurrentlyCarried.Get();
    AELCharacter* Carrier = Cast<AELCharacter>(GetOwner());
    if (!Target || !Carrier) return;

    CurrentState = NewState;
    Target->ServerSetCarryState(NewState, Carrier);
    ApplyCarrierSpeed();

    UE_LOG(LogTemp, Log, TEXT("UELCarryComponent: Transitioned %s to %s"),
        *Target->GetName(),
        NewState == EELCarryState::Stable ? TEXT("Stable") :
        NewState == EELCarryState::Dragged ? TEXT("Dragged") : TEXT("None"));
}

void UELCarryComponent::ApplyCarrierSpeed()
{
    AELCharacter* Owner = Cast<AELCharacter>(GetOwner());
    if (!Owner) return;
    UCharacterMovementComponent* MoveComp = Owner->GetCharacterMovement();
    if (!MoveComp) return;

    switch (CurrentState)
    {
    case EELCarryState::Stable:  MoveComp->MaxWalkSpeed = OriginalMaxWalkSpeed * StableSpeedScale;  break;
    case EELCarryState::Dragged: MoveComp->MaxWalkSpeed = OriginalMaxWalkSpeed * DraggedSpeedScale; break;
    case EELCarryState::None:    MoveComp->MaxWalkSpeed = OriginalMaxWalkSpeed;                     break;
    }
}