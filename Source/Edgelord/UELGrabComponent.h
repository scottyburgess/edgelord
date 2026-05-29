#pragma once
#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "PhysicsEngine/ConstraintInstance.h"
#include "UELGrabComponent.generated.h"

UENUM(BlueprintType)
enum class EELHandSide : uint8
{
    Left    UMETA(DisplayName = "Left"),
    Right   UMETA(DisplayName = "Right")
};

UENUM(BlueprintType)
enum class EELGrabState : uint8
{
    None    UMETA(DisplayName = "None"),
    Left    UMETA(DisplayName = "Left"),
    Right   UMETA(DisplayName = "Right"),
    Both    UMETA(DisplayName = "Both")
};

UCLASS(ClassGroup = (Edgelord), meta = (BlueprintSpawnableComponent))
class EDGELORD_API UELGrabComponent : public UActorComponent
{
    GENERATED_BODY()

public:
    UELGrabComponent();

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Grab")
    float GrabRadius = 50.f;
    // TODO: tune grab radius per play-test

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Grab")
    FName LeftHandBone = FName("hand_l");

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Grab")
    FName RightHandBone = FName("hand_r");

    void TryGrab(EELHandSide Hand);
    void ReleaseGrab(EELHandSide Hand);
    bool IsGrabbing(EELHandSide Hand) const;
    EELGrabState GetGrabState() const;

    UFUNCTION(BlueprintCallable, Category = "Grab")
    AActor* GetGrabbedActor(EELHandSide Hand) const;

protected:
    virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;

private:
    FConstraintInstance* LeftConstraint = nullptr;
    FConstraintInstance* RightConstraint = nullptr;

    TWeakObjectPtr<UPrimitiveComponent> LeftGrabbedComponent;
    TWeakObjectPtr<UPrimitiveComponent> RightGrabbedComponent;

    void ExecuteGrab(FName BoneName, FConstraintInstance*& OutConstraint,
        TWeakObjectPtr<UPrimitiveComponent>& OutGrabbedComp);
    void ExecuteRelease(FConstraintInstance*& Constraint,
        TWeakObjectPtr<UPrimitiveComponent>& GrabbedComp);

    void BroadcastGrabStateChange();
};