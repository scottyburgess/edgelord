#pragma once
#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "UELGetUpComponent.generated.h"

class UAnimMontage;
class AELCharacter;

UCLASS(ClassGroup = (Edgelord), meta = (BlueprintSpawnableComponent))
class EDGELORD_API UELGetUpComponent : public UActorComponent
{
    GENERATED_BODY()

public:
    UELGetUpComponent();

    // How long pelvis must stay horizontal before we trigger get-up.
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "GetUp")
    float HorizontalDuration = 0.8f;

    // Head-Z minus pelvis-Z. Below this we consider the character down.
    // Standing Manny ~ 120, fallen ~ 0-30. 60 is a safe middle.
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "GetUp")
    float UprightHeightThreshold = 60.f;

    // How long to stay in GettingUp profile before returning to Standing.
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "GetUp")
    float GetUpDuration = 1.5f;

    // Cooldown after finishing get-up before we check again — gives physics
    // time to settle before re-triggering.
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "GetUp")
    float PostGetUpCooldown = 1.0f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "GetUp")
    UAnimMontage* GetUpMontage = nullptr;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "GetUp")
    FName PelvisBoneName = FName("pelvis");

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "GetUp")
    FName HeadBoneName = FName("head");

protected:
    virtual void BeginPlay() override;
    virtual void TickComponent(float DeltaTime, ELevelTick TickType,
        FActorComponentTickFunction* ThisTickFunction) override;

private:
    float HorizontalAccumulator = 0.f;
    bool bGettingUp = false;
    FTimerHandle GetUpTimerHandle;

    void StartGetUp();
    void FinishGetUp();
};