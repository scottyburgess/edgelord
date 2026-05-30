#pragma once
#include "CoreMinimal.h"
#include "GameFramework/Character.h"
#include "Logging/LogMacros.h"
#include "IELGrabbable.h"
#include "UELGrabComponent.h"
#include "UELCarryComponent.h"
#include "ELCharacter.generated.h"

class USpringArmComponent;
class UCameraComponent;
class UInputAction;
class UPhysicalAnimationComponent;
class UELGrabComponent;
class UELCarryComponent;
class UELGetUpComponent;
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
class EDGELORD_API AELCharacter : public ACharacter, public IIELGrabbable
{
    GENERATED_BODY()

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components", meta = (AllowPrivateAccess = "true"))
    USpringArmComponent* CameraBoom;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components", meta = (AllowPrivateAccess = "true"))
    UCameraComponent* FollowCamera;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components", meta = (AllowPrivateAccess = "true"))
    UPhysicalAnimationComponent* PhysicalAnimation;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components", meta = (AllowPrivateAccess = "true"))
    UELGrabComponent* GrabComponent;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components", meta = (AllowPrivateAccess = "true"))
    UELCarryComponent* CarryComponent;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components", meta = (AllowPrivateAccess = "true"))
    UELGetUpComponent* GetUpComponent;

public:
    AELCharacter();

    // --- Physics profile ---
    UPROPERTY(ReplicatedUsing = OnRep_PhysicsProfile, BlueprintReadOnly, Category = "Physics")
    EELPhysicsProfile CurrentPhysicsProfile = EELPhysicsProfile::Standing;

    UFUNCTION()
    void OnRep_PhysicsProfile();

    UFUNCTION(BlueprintCallable, Category = "Physics")
    void SetPhysicsProfile(EELPhysicsProfile NewProfile);

    // --- Grab state (replicated for cosmetics; physics is local) ---
    UPROPERTY(ReplicatedUsing = OnRep_GrabState, BlueprintReadOnly, Category = "Grab")
    EELGrabState CurrentGrabState = EELGrabState::None;

    UFUNCTION()
    void OnRep_GrabState();

    UFUNCTION(BlueprintCallable, Category = "Grab")
    void SetGrabState(EELGrabState NewState);

    UFUNCTION(Server, Reliable)
    void ServerSetGrabState(EELGrabState NewState);

    UFUNCTION(BlueprintCallable, Category = "Carry")
    AELCharacter* GetCarrier() const { return CarriedByCharacter; }

    // --- Carry state ---
    UPROPERTY(ReplicatedUsing = OnRep_CarryState, BlueprintReadOnly, Category = "Carry")
    EELCarryState CurrentCarryState = EELCarryState::None;

    UPROPERTY(ReplicatedUsing = OnRep_CarriedBy, BlueprintReadOnly, Category = "Carry")
    AELCharacter* CarriedByCharacter = nullptr;

    UFUNCTION()
    void OnRep_CarryState();

    UFUNCTION()
    void OnRep_CarriedBy();

    UFUNCTION(Server, Reliable)
    void ServerSetCarryState(EELCarryState NewState, AELCharacter* NewCarrier);

    UFUNCTION(BlueprintCallable, Category = "Carry")
    bool IsCarried() const { return CurrentCarryState != EELCarryState::None; }

    UFUNCTION(BlueprintCallable, Category = "Carry")
    bool IsDragged() const { return CurrentCarryState == EELCarryState::Dragged; }

    // --- IIELGrabbable ---
    virtual UPrimitiveComponent* GetGrabbedComponent_Implementation() override;
    virtual FName GetGrabbedBoneName_Implementation() override;

    virtual void GetLifetimeReplicatedProps(
        TArray<FLifetimeProperty>& OutLifetimeProps) const override;

protected:
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

    UPROPERTY(EditAnywhere, Category = "Input")
    UInputAction* GrabLeftAction;

    UPROPERTY(EditAnywhere, Category = "Input")
    UInputAction* GrabRightAction;

    virtual void BeginPlay() override;
    virtual void SetupPlayerInputComponent(UInputComponent* PlayerInputComponent) override;

    void Move(const FInputActionValue& Value);
    void Look(const FInputActionValue& Value);
    void Input_GrabLeft_Started(const FInputActionValue& Value);
    void Input_GrabLeft_Completed(const FInputActionValue& Value);
    void Input_GrabRight_Started(const FInputActionValue& Value);
    void Input_GrabRight_Completed(const FInputActionValue& Value);
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
    FORCEINLINE UELGrabComponent* GetGrabComponent() const { return GrabComponent; }
    FORCEINLINE UELCarryComponent* GetCarryComponent() const { return CarryComponent; }
    FORCEINLINE UELGetUpComponent* GetGetUpComponent() const { return GetUpComponent; }
};