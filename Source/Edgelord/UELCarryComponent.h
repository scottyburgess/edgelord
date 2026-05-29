#pragma once
#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "UELCarryComponent.generated.h"

class AELCharacter;

UCLASS(ClassGroup = (Edgelord), meta = (BlueprintSpawnableComponent))
class EDGELORD_API UELCarryComponent : public UActorComponent
{
    GENERATED_BODY()

public:
    UELCarryComponent();

    // Walk-speed multiplier while carrying another character.
    // TODO: derive from carried pawn mass ratio once physics is tuned. 0.5 is a placeholder.
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Carry")
    float CarryingSpeedScale = 0.5f;

    // Called by AELCharacter when its grab state changes.
    void OnGrabStateChanged();

    UFUNCTION(BlueprintCallable, Category = "Carry")
    AELCharacter* GetCarriedCharacter() const { return CurrentlyCarried.Get(); }

protected:
    virtual void BeginPlay() override;
    virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;

private:
    TWeakObjectPtr<AELCharacter> CurrentlyCarried;
    float OriginalMaxWalkSpeed = 0.f;

    void StartCarrying(AELCharacter* Target);
    void StopCarrying();
};