// Copyright Epic Games, Inc. All Rights Reserved.
#include "ELPlayerController.h"
#include "EnhancedInputSubsystems.h"
#include "Engine/LocalPlayer.h"
#include "InputMappingContext.h"
#include "Blueprint/UserWidget.h"
#include "Edgelord.h"
#include "Widgets/Input/SVirtualJoystick.h"
#include "ELGameState.h"
#include "ELGameMode.h"

void AELPlayerController::BeginPlay()
{
    Super::BeginPlay();
    if (ShouldUseTouchControls() && IsLocalPlayerController())
    {
        MobileControlsWidget = CreateWidget<UUserWidget>(this, MobileControlsWidgetClass);
        if (MobileControlsWidget)
        {
            MobileControlsWidget->AddToPlayerScreen(0);
        }
        else
        {
            UE_LOG(LogEdgelord, Error, TEXT("Could not spawn mobile controls widget."));
        }
    }
}

void AELPlayerController::SetupInputComponent()
{
    Super::SetupInputComponent();
    if (IsLocalPlayerController())
    {
        if (UEnhancedInputLocalPlayerSubsystem* Subsystem =
            ULocalPlayer::GetSubsystem<UEnhancedInputLocalPlayerSubsystem>(GetLocalPlayer()))
        {
            for (UInputMappingContext* CurrentContext : DefaultMappingContexts)
            {
                Subsystem->AddMappingContext(CurrentContext, 0);
            }
            if (!ShouldUseTouchControls())
            {
                for (UInputMappingContext* CurrentContext : MobileExcludedMappingContexts)
                {
                    Subsystem->AddMappingContext(CurrentContext, 0);
                }
            }
        }
    }
}

bool AELPlayerController::ShouldUseTouchControls() const
{
    return SVirtualJoystick::ShouldDisplayTouchInterface() || bForceTouchControls;
}

void AELPlayerController::ServerSetReady_Implementation()
{
    // Runs on server only
    if (AELGameState* GS = GetWorld()->GetGameState<AELGameState>())
    {
        GS->IncrementReadyCount();

        // Check if all players are ready
        int32 NumPlayers = GetWorld()->GetNumPlayerControllers();
        if (GS->ReadyCount >= NumPlayers)
        {
            // Travel to gameplay map
            GetWorld()->ServerTravel("/Game/Maps/L_GameLevel", false);
        }
    }
}