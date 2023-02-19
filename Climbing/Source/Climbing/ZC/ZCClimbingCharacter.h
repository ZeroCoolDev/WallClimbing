// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Climbing/ClimbingCharacter.h"
#include "ZCClimbingCharacter.generated.h"

/**
 * 
 */
UCLASS()
class CLIMBING_API AZCClimbingCharacter : public AClimbingCharacter
{
	GENERATED_BODY()

public:
	AZCClimbingCharacter();
	virtual ~AZCClimbingCharacter(){}

protected:
	// Called when the game starts or when spawned
	virtual void BeginPlay() override;

public:
	// Called every frame
	virtual void Tick(float DeltaTime) override;

	// Called to bind functionality to input
	virtual void SetupPlayerInputComponent(class UInputComponent* PlayerInputComponent) override;
};
