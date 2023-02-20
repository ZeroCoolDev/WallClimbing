// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "ZCCharacterMovementComponent.generated.h"

/**
 * 
 */
UCLASS()
class CLIMBING_API UZCCharacterMovementComponent : public UCharacterMovementComponent
{
	GENERATED_BODY()

public:
	UZCCharacterMovementComponent(){}
	virtual ~UZCCharacterMovementComponent(){}

	void DrawDebug();
};
