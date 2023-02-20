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
	AZCClimbingCharacter(const FObjectInitializer&);
	virtual ~AZCClimbingCharacter(){}
	virtual void Tick(float DeltaTime) override;
	virtual void SetupPlayerInputComponent(class UInputComponent* PlayerInputComponent) override;

	UFUNCTION(BlueprintPure)
	FORCEINLINE class UZCCharacterMovementComponent* GetZCMovementComponent() const { return MovementComponent; }

protected:
	virtual void BeginPlay() override;

	UPROPERTY(Category=Character, VisibleAnywhere, BlueprintReadOnly)
	class UZCCharacterMovementComponent* MovementComponent;
};
