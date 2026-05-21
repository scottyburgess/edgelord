// Copyright Epic Games, Inc. All Rights Reserved.
#pragma once
#include "CoreMinimal.h"
#include "GameFramework/GameStateBase.h"
#include "ELGameState.generated.h"

UCLASS()
class EDGELORD_API AELGameState : public AGameStateBase
{
    GENERATED_BODY()

public:
    AELGameState();

    // Number of players who have pressed Ready
    UPROPERTY(ReplicatedUsing = OnRep_ReadyCount, BlueprintReadOnly, Category = "Lobby")
    int32 ReadyCount = 0;

    // Called on clients when ReadyCount changes
    UFUNCTION()
    void OnRep_ReadyCount();

    // Increment ready count — call this from GameMode on server only
    void IncrementReadyCount();

    virtual void GetLifetimeReplicatedProps(
        TArray<FLifetimeProperty>& OutLifetimeProps) const override;
};