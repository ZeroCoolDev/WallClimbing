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

	UFUNCTION(BlueprintPure)
	bool IsClimbing() const;

	UFUNCTION(BlueprintPure)
	FVector GetClimbSurfaceNormal() const;

	void WantsClimbing();
	void CancelClimbing();

private:
	virtual void BeginPlay() override;
	virtual void TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction) override;
	virtual void OnMovementUpdated(float DeltaSeconds, const FVector& OldLocation, const FVector& OldVelocity) override;
	virtual void PhysCustom(float deltaTime, int32 Iterations) override;
	virtual void OnMovementModeChanged(EMovementMode PreviousMovementMode, uint8 PreviousCustomMode) override;
	virtual float GetMaxSpeed() const override;
	virtual float GetMaxAcceleration() const override;

	void SweepAndStoreWallHits();
	bool CanStartClimbing() const;
	bool HorizontalClimbCheck(const FHitResult& WallHit) const;
	bool VerticalClimbCheck(const FHitResult& WallHit) const;
	bool EyeHeightTrace(const float TraceDistance) const;

	void PhysClimbing(float DeltaTime, int32 Iterations);
	void ComputeSurfaceInfo();
	void ComputeClimbingVelocity(float DeltaTime);
	bool ShouldStopClimbing();
	void StopClimbing(float DeltaTime, int32 Iterations);
	void MoveAlongClimbingSurface(float DeltaTime);
	FQuat GetSmoothClimbingRotation(float DeltaTime) const;
	void SnapToClimbingSurface(float DeltaTime) const;

	UPROPERTY(Category = "Character Movement: Climbing", EditAnywhere)
	int CollisionCapsulRadius = 50;
	UPROPERTY(Category = "Character Movement: Climbing", EditAnywhere)
	int CollisionCapsulHalfHeight = 72;
	UPROPERTY(Category = "Character Movement: Climbing", EditAnywhere)
	int CollisionCapsulForwardOffset = 20;
	UPROPERTY(Category = "Character Movement: Climbing", EditAnywhere, meta=(ClampMin="0.0", ClampMax = "72.0"))
	int CollisionCapsulClimbingShinkAmount = 30;

	UPROPERTY(Category = "Character Movement: Climbing", EditAnywhere)
	float MinHorizontalDegreesToStartClimbing = 25;
	UPROPERTY(Category = "Character Movement: Climbing", EditAnywhere)
	float MinVerticalDegreesToStartClimbing = 45; // Based off UE5's default CharacterMovementComponent::WalkableFloorAngle

	UPROPERTY(Category = "Character Movement: Climbing", EditAnywhere, meta = (ClampMin = "10.0", ClampMax = "500.0"))
	float MaxClimbingSpeed = 120.f;
	UPROPERTY(Category = "Character Movement: Climbing", EditAnywhere, meta = (ClampMin = "10.0", ClampMax = "2000.0"))
	float MaxClimbingAcceleration = 380.f;
	UPROPERTY(Category = "Character Movement: Climbing", EditAnywhere, meta = (ClampMin = "0.0", ClampMax = "3000.0"))
	float BrakingDecelerationClimbing = 550.f;
	UPROPERTY(Category = "Character Movement: Climbing", EditAnywhere, meta = (ClampMin = "1.0", ClampMax = "12.0"))
	float ClimbingRotationSpeed = 6.f;
	UPROPERTY(Category = "Character Movement: Climbing", EditAnywhere, meta = (ClampMin = "0.0", ClampMax = "60.0"))
	float ClimbingSnapSpeed = 4.f;
	UPROPERTY(Category = "Character Movement: Climbing", EditAnywhere, meta = (ClampMin = "0.0", ClampMax = "90.0"))
	float ClimbingDistanceFromSurface = 45.f;

	TArray<FHitResult> CurrentWallHits;
	FCollisionQueryParams ClimbQueryParams;

	FVector CurrentClimbingNormal;
	FVector CurrentClimbingPosition;

	bool bWantsToClimb = false;

private:
	void DrawEyeTraceDebug(const FVector& Start, const FVector& End) const;
	void DrawDebug(FVector SweepLocation) const;
	bool bIsDebugEnabled = false;
};
