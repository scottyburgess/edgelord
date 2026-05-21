// Copyright Epic Games, Inc. All Rights Reserved.
#include "ELGameState.h"
#include "Net/UnrealNetwork.h"

AELGameState::AELGameState()
{
    ReadyCount = 0;
}

void AELGameState::GetLifetimeReplicatedProps(
    TArray<FLifetimeProperty>& OutLifetimeProps) const
{
    Super::GetLifetimeReplicatedProps(OutLifetimeProps);
    DOREPLIFETIME(AELGameState, ReadyCount);
}

void AELGameState::OnRep_ReadyCount()
{
    // Clients land here when ReadyCount changes
    // Blueprint can bind to this via the replicated property
    UE_LOG(LogTemp, Warning, TEXT("ReadyCount updated on client: %d"), ReadyCount);
}

void AELGameState::IncrementReadyCount()
{
    // Server only
    ReadyCount++;
    UE_LOG(LogTemp, Warning, TEXT("ReadyCount is now: %d"), ReadyCount);
}