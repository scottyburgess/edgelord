// Copyright Epic Games, Inc. All Rights Reserved.
#pragma once
#include "CoreMinimal.h"
#include "GameFramework/Character.h"
#include "Logging/LogMacros.h"
#include "ELCharacter.generated.h"

class USpringArmComponent;
class UCameraComponent;
class UInputAction;
class UPhysicalAnimationComponent;
struct FInputActionValue;

DECLARE_LOG_CATEGORY_EXTERN(LogTemplateCharacter, Log, All);

UENUM(BlueprintType)
enum class EELPhysicsProfile : uint8
{
    Standing    UMETA(DisplayName = "Standing"),
    Walking     UMETA(DisplayName = "Walking"),
    Grabbing    UMETA(DisplayName = "Grabbing"),
    Ragdoll     UMETA(DisplayName = "Ragdoll"),
    GettingUp   UMETA(DisplayName = "GettingUp")
};

USTRUCT(BlueprintType)
struct FELPhysicsProfileSettings
{
    GENERATED_BODY()

    // TODO: tune these values empirically — ship defaults, cap tuning at 1 week total
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Physics")
    float OrientationStrength = 1000.f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Physics")
    float AngularVelocityStrength = 100.f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Physics")
    float PositionStrength = 0.f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Physics")
    float VelocityStrength = 0.f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Physics")
    float MaxLinearForce = 0.f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Physics")
    float MaxAngularForce = 0.f;
};

UCLASS()
class EDGELORD_API AELCharacter : public ACharacter
{
    GENERATED_BODY()

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components", meta = (AllowPrivateAccess = "true"))
    USpringArmComponent* CameraBoom;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components", meta = (AllowPrivateAccess = "true"))
    UCameraComponent* FollowCamera;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components", meta = (AllowPrivateAccess = "true"))
    UPhysicalAnimationComponent* PhysicalAnimation;

public:
    AELCharacter();

    UPROPERTY(ReplicatedUsing = OnRep_PhysicsProfile, BlueprintReadOnly, Category = "Physics")
    EELPhysicsProfile CurrentPhysicsProfile = EELPhysicsProfile::Standing;

    UFUNCTION()
    void OnRep_PhysicsProfile();

    UFUNCTION(BlueprintCallable, Category = "Physics")
    void SetPhysicsProfile(EELPhysicsProfile NewProfile);

    virtual void GetLifetimeReplicatedProps(
        TArray<FLifetimeProperty>& OutLifetimeProps) const override;

protected:
    // TODO: tune these values — ship with defaults, cap tuning at 1 week total
    UPROPERTY(EditAnywhere, Category = "Physics|Profiles")
    FELPhysicsProfileSettings StandingProfile;

    UPROPERTY(EditAnywhere, Category = "Physics|Profiles")
    FELPhysicsProfileSettings WalkingProfile;

    UPROPERTY(EditAnywhere, Category = "Physics|Profiles")
    FELPhysicsProfileSettings GrabbingProfile;

    UPROPERTY(EditAnywhere, Category = "Physics|Profiles")
    FELPhysicsProfileSettings RagdollProfile;

    UPROPERTY(EditAnywhere, Category = "Physics|Profiles")
    FELPhysicsProfileSettings GettingUpProfile;

    UPROPERTY(EditAnywhere, Category = "Input")
    UInputAction* JumpAction;

    UPROPERTY(EditAnywhere, Category = "Input")
    UInputAction* MoveAction;

    UPROPERTY(EditAnywhere, Category = "Input")
    UInputAction* LookAction;

    UPROPERTY(EditAnywhere, Category = "Input")
    UInputAction* MouseLookAction;

    virtual void BeginPlay() override;
    virtual void SetupPlayerInputComponent(UInputComponent* PlayerInputComponent) override;

    void Move(const FInputActionValue& Value);
    void Look(const FInputActionValue& Value);

    void ApplyPhysicsProfile(const FELPhysicsProfileSettings& Settings);

public:
    UFUNCTION(BlueprintCallable, Category = "Input")
    virtual void DoMove(float Right, float Forward);

    UFUNCTION(BlueprintCallable, Category = "Input")
    virtual void DoLook(float Yaw, float Pitch);

    UFUNCTION(BlueprintCallable, Category = "Input")
    virtual void DoJumpStart();

    UFUNCTION(BlueprintCallable, Category = "Input")
    virtual void DoJumpEnd();

    FORCEINLINE USpringArmComponent* GetCameraBoom() const { return CameraBoom; }
    FORCEINLINE UCameraComponent* GetFollowCamera() const { return FollowCamera; }
    FORCEINLINE UPhysicalAnimationComponent* GetPhysicalAnimation() const { return PhysicalAnimation; }
};