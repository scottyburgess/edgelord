#include "UELGrabComponent.h"
#include "IELGrabbable.h"
#include "ELCharacter.h"
#include "Engine/OverlapResult.h"

UELGrabComponent::UELGrabComponent()
{
    PrimaryComponentTick.bCanEverTick = false;
}

void UELGrabComponent::TryGrab(EELHandSide Hand)
{
    AELCharacter* Owner = Cast<AELCharacter>(GetOwner());
    if (!Owner || !Owner->IsLocallyControlled()) return;

    if (Hand == EELHandSide::Left)
        ExecuteGrab(LeftHandBone, LeftConstraint, LeftGrabbedComponent);
    else
        ExecuteGrab(RightHandBone, RightConstraint, RightGrabbedComponent);

    BroadcastGrabStateChange();
}

void UELGrabComponent::ReleaseGrab(EELHandSide Hand)
{
    AELCharacter* Owner = Cast<AELCharacter>(GetOwner());
    if (!Owner || !Owner->IsLocallyControlled()) return;

    if (Hand == EELHandSide::Left)
        ExecuteRelease(LeftConstraint, LeftGrabbedComponent);
    else
        ExecuteRelease(RightConstraint, RightGrabbedComponent);

    BroadcastGrabStateChange();
}

bool UELGrabComponent::IsGrabbing(EELHandSide Hand) const
{
    return Hand == EELHandSide::Left
        ? (LeftConstraint != nullptr)
        : (RightConstraint != nullptr);
}

EELGrabState UELGrabComponent::GetGrabState() const
{
    const bool bL = LeftConstraint != nullptr;
    const bool bR = RightConstraint != nullptr;
    if (bL && bR) return EELGrabState::Both;
    if (bL)       return EELGrabState::Left;
    if (bR)       return EELGrabState::Right;
    return EELGrabState::None;
}

void UELGrabComponent::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
    ExecuteRelease(LeftConstraint, LeftGrabbedComponent);
    ExecuteRelease(RightConstraint, RightGrabbedComponent);
    Super::EndPlay(EndPlayReason);
}

void UELGrabComponent::ExecuteGrab(FName BoneName, FConstraintInstance*& OutConstraint,
    TWeakObjectPtr<UPrimitiveComponent>& OutGrabbedComp)
{
    if (OutConstraint) return;

    AELCharacter* Owner = Cast<AELCharacter>(GetOwner());
    if (!Owner) return;

    USkeletalMeshComponent* Mesh = Owner->GetMesh();
    if (!Mesh) return;

    FBodyInstance* HandBody = Mesh->GetBodyInstance(BoneName);
    if (!HandBody)
    {
        UE_LOG(LogTemp, Warning, TEXT("UELGrabComponent: No body for bone %s"), *BoneName.ToString());
        return;
    }

    const FVector HandLocation = Mesh->GetBoneLocation(BoneName);

    TArray<FOverlapResult> Overlaps;
    FCollisionQueryParams Params;
    Params.AddIgnoredActor(Owner);
    GetWorld()->OverlapMultiByChannel(
        Overlaps, HandLocation, FQuat::Identity,
        ECC_WorldDynamic,
        FCollisionShape::MakeSphere(GrabRadius),
        Params);

    for (const FOverlapResult& Overlap : Overlaps)
    {
        AActor* HitActor = Overlap.GetActor();
        if (!HitActor || !HitActor->Implements<UIELGrabbable>()) continue;

        UPrimitiveComponent* GrabComp = IIELGrabbable::Execute_GetGrabbedComponent(HitActor);
        if (!GrabComp) continue;

        FBodyInstance* TargetBody = GrabComp->GetBodyInstance();
        if (!TargetBody) continue;

        FConstraintInstance* CI = new FConstraintInstance();
        CI->SetLinearXMotion(ELinearConstraintMotion::LCM_Locked);
        CI->SetLinearYMotion(ELinearConstraintMotion::LCM_Locked);
        CI->SetLinearZMotion(ELinearConstraintMotion::LCM_Locked);
        // TODO: tune — consider limited angular motion for more control
        CI->SetAngularSwing1Motion(EAngularConstraintMotion::ACM_Free);
        CI->SetAngularSwing2Motion(EAngularConstraintMotion::ACM_Free);
        CI->SetAngularTwistMotion(EAngularConstraintMotion::ACM_Free);
        CI->InitConstraint(HandBody, TargetBody, 1.0f, this);

        OutConstraint = CI;
        OutGrabbedComp = GrabComp;

        UE_LOG(LogTemp, Log, TEXT("UELGrabComponent: Grabbed %s on bone %s"),
            *HitActor->GetName(), *BoneName.ToString());
        return;
    }
}

void UELGrabComponent::ExecuteRelease(FConstraintInstance*& Constraint,
    TWeakObjectPtr<UPrimitiveComponent>& GrabbedComp)
{
    if (Constraint)
    {
        Constraint->TermConstraint();
        delete Constraint;
        Constraint = nullptr;
    }
    GrabbedComp.Reset();
}

void UELGrabComponent::BroadcastGrabStateChange()
{
    AELCharacter* Owner = Cast<AELCharacter>(GetOwner());
    if (Owner)
        Owner->SetGrabState(GetGrabState());
}