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

private:
	virtual void BeginPlay() override;
	virtual void TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction) override;

	void SweepAndStoreWallHits();
	bool CanStartClimbing();

	UPROPERTY(Category = "Character Movement: Climbing", EditAnywhere)
	int CollisionCapsulRadius = 50;
	UPROPERTY(Category = "Character Movement: Climbing", EditAnywhere)
	int CollisionCapsulHalfHeight = 72;

	UPROPERTY(Category = "Character Movement: Climbing", EditAnywhere)
	float MinHorizontalDegreesToStartClimbing = 25;

	TArray<FHitResult> CurrentWallHits;
	FCollisionQueryParams ClimbQueryParams;

public:
	void DrawDebug(FVector SweepLocation);
};
