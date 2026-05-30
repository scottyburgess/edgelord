#include "UELGrabComponent.h"
#include "IELGrabbable.h"
#include "ELCharacter.h"
#include "Engine/OverlapResult.h"
#include "Components/SkeletalMeshComponent.h"

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

AActor* UELGrabComponent::GetGrabbedActor(EELHandSide Hand) const
{
    UPrimitiveComponent* Comp = (Hand == EELHandSide::Left)
        ? LeftGrabbedComponent.Get()
        : RightGrabbedComponent.Get();
    return Comp ? Comp->GetOwner() : nullptr;
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

        // Resolve target body
        FBodyInstance* TargetBody = nullptr;
        FName TargetBone = IIELGrabbable::Execute_GetGrabbedBoneName(HitActor);

        if (USkeletalMeshComponent* SkelMesh = Cast<USkeletalMeshComponent>(GrabComp))
        {
            // If the interface didn't pick a bone, find the nearest simulated body to the hand
            if (TargetBone == NAME_None)
            {
                float BestDistSq = TNumericLimits<float>::Max();
                for (FBodyInstance* Body : SkelMesh->Bodies)
                {
                    if (!Body || !Body->bSimulatePhysics) continue;
                    const float DistSq = FVector::DistSquared(
                        Body->GetUnrealWorldTransform().GetLocation(), HandLocation);
                    if (DistSq < BestDistSq)
                    {
                        BestDistSq = DistSq;
                        TargetBody = Body;
                        const int32 BodyIndex = SkelMesh->Bodies.IndexOfByKey(Body);
                        TargetBone = SkelMesh->GetBoneName(BodyIndex);
                    }
                }
            }
            else
            {
                TargetBody = SkelMesh->GetBodyInstance(TargetBone);
            }
        }

        if (!TargetBody)
            TargetBody = GrabComp->GetBodyInstance();
        if (!TargetBody) continue;

        FConstraintInstance* CI = new FConstraintInstance();

        // Limited linear motion with some slack — lets the arm stretch instead of fighting
        // the PAC angular drives that hold the rest pose.
        CI->SetLinearXMotion(ELinearConstraintMotion::LCM_Limited);
        CI->SetLinearYMotion(ELinearConstraintMotion::LCM_Limited);
        CI->SetLinearZMotion(ELinearConstraintMotion::LCM_Limited);
        CI->SetLinearLimitSize(20.f);  // 20cm of slack in the grab

        // Soft linear limit so it stretches like an elastic tether rather than a hard wall
        CI->ProfileInstance.LinearLimit.bSoftConstraint = true;
        CI->ProfileInstance.LinearLimit.Stiffness = 500.f;
        CI->ProfileInstance.LinearLimit.Damping = 50.f;

        // Free rotation — Gang Beasts style, object can spin in hand
        CI->SetAngularSwing1Motion(EAngularConstraintMotion::ACM_Free);
        CI->SetAngularSwing2Motion(EAngularConstraintMotion::ACM_Free);
        CI->SetAngularTwistMotion(EAngularConstraintMotion::ACM_Free);

        CI->InitConstraint(HandBody, TargetBody, 1.0f, this);

        OutConstraint = CI;
        OutGrabbedComp = GrabComp;

        UE_LOG(LogTemp, Log, TEXT("UELGrabComponent: Grabbed %s (bone %s) on hand bone %s"),
            *HitActor->GetName(),
            TargetBone == NAME_None ? TEXT("root") : *TargetBone.ToString(),
            *BoneName.ToString());
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