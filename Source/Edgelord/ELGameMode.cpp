// Copyright Epic Games, Inc. All Rights Reserved.

#include "ELGameMode.h"
#include "ELGameState.h"

AELGameMode::AELGameMode()
{
	GameStateClass = AELGameState::StaticClass();
}
