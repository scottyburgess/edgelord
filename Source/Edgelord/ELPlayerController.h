// Copyright Epic Games, Inc. All Rights Reserved.
#pragma once
#include "CoreMinimal.h"
#include "GameFramework/PlayerController.h"
#include "ELPlayerController.generated.h"

class UInputMappingContext;
class UUserWidget;

UCLASS(abstract)
class AELPlayerController : public APlayerController
{
    GENERATED_BODY()

public:
    // Call from lobby UI Ready button
    UFUNCTION(Server, Reliable, BlueprintCallable, Category = "Lobby")
    void ServerSetReady();

protected:
    UPROPERTY(EditAnywhere, Category = "Input|Input Mappings")
    TArray<UInputMappingContext*> DefaultMappingContexts;

    UPROPERTY(EditAnywhere, Category = "Input|Input Mappings")
    TArray<UInputMappingContext*> MobileExcludedMappingContexts;

    UPROPERTY(EditAnywhere, Category = "Input|Touch Controls")
    TSubclassOf<UUserWidget> MobileControlsWidgetClass;

    UPROPERTY()
    TObjectPtr<UUserWidget> MobileControlsWidget;

    UPROPERTY(EditAnywhere, Config, Category = "Input|Touch Controls")
    bool bForceTouchControls = false;

    virtual void BeginPlay() override;
    virtual void SetupInputComponent() override;
    bool ShouldUseTouchControls() const;
};