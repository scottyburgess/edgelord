#pragma once
#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "UELCarryComponent.generated.h"

class AELCharacter;

UENUM(BlueprintType)
enum class EELCarryState : uint8
{
    None     UMETA(DisplayName = "None"),
    Stable   UMETA(DisplayName = "Stable"),    // Grabbed but on feet, retains input
    Dragged  UMETA(DisplayName = "Dragged")    // Lost footing, full ragdoll, no input
};

UCLASS(ClassGroup = (Edgelord), meta = (BlueprintSpawnableComponent))
class EDGELORD_API UELCarryComponent : public UActorComponent
{
    GENERATED_BODY()

public:
    UELCarryComponent();

    // Carrier walk-speed multiplier while grabbing another standing character.
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Carry")
    float StableSpeedScale = 0.65f;

    // Carrier walk-speed multiplier while dragging a ragdolled character.
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Carry")
    float DraggedSpeedScale = 0.4f;

    // Carrier-to-grabbee distance at which the grabbee loses their footing.
    // TODO: tune empirically. 200cm is a starting guess.
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Carry")
    float TetherBreakDistance = 200.f;

    // How long the tether must be over-stretched before they fall.
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Carry")
    float TetherGracePeriod = 0.3f;

    void OnGrabStateChanged();

    UFUNCTION(BlueprintCallable, Category = "Carry")
    AELCharacter* GetCarriedCharacter() const { return CurrentlyCarried.Get(); }

    // Called by the grabbee's GetUp component when it recovers while still tethered.
    // Asks us to flip them back from Dragged to Stable.
    void OnGrabbeeRecovered(AELCharacter* Grabbee);

protected:
    virtual void BeginPlay() override;
    virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;
    virtual void TickComponent(float DeltaTime, ELevelTick TickType,
        FActorComponentTickFunction* ThisTickFunction) override;

private:
    TWeakObjectPtr<AELCharacter> CurrentlyCarried;
    EELCarryState CurrentState = EELCarryState::None;
    float OverStretchAccumulator = 0.f;
    float OriginalMaxWalkSpeed = 0.f;

    void StartCarrying(AELCharacter* Target);
    void StopCarrying();
    void TransitionCarriedTo(EELCarryState NewState);
    void ApplyCarrierSpeed();
};