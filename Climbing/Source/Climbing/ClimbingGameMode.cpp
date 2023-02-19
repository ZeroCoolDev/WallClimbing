// Copyright Epic Games, Inc. All Rights Reserved.

#include "ClimbingGameMode.h"
#include "ClimbingCharacter.h"
#include "UObject/ConstructorHelpers.h"

AClimbingGameMode::AClimbingGameMode()
{
	// ZC: Set the game mode to look for our custom character
	static ConstructorHelpers::FClassFinder<APawn> PlayerPawnBPClass(TEXT("/Game/ZC/Blueprints/BP_ZCCharacter"));
	if (PlayerPawnBPClass.Class != NULL)
	{
		DefaultPawnClass = PlayerPawnBPClass.Class;
	}
}
