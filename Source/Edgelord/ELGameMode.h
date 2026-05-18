// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/GameModeBase.h"
#include "ELGameMode.generated.h"

/**
 *  Simple GameMode for a third person game
 */
UCLASS(abstract)
class EDGELORD_API AELGameMode : public AGameModeBase
{
	GENERATED_BODY()

public:
	
	/** Constructor */
	AELGameMode();
};



