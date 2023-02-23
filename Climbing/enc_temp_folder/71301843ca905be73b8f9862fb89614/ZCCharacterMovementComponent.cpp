// Fill out your copyright notice in the Description page of Project Settings.


#include "Climbing/ZC/ZCCharacterMovementComponent.h"
#include "Climbing/ZC/ZCTypes.h"

#include "GameFramework/Character.h"
#include "Components/CapsuleComponent.h"

static TAutoConsoleVariable<bool> CVarDebugToggle(
	TEXT("DebugToggle"),
	true,
	TEXT("Turns on debug information\n")
	TEXT("0: off\n")
	TEXT("1: on"),
	ECVF_Default);

bool UZCCharacterMovementComponent::IsClimbing() const
{
	return MovementMode == EMovementMode::MOVE_Custom && CustomMovementMode == ECustomMovementMode::CMOVE_Climbing;
}

FVector UZCCharacterMovementComponent::GetClimbSurfaceNormal() const
{
	return CurrentClimbingNormal;
}

void UZCCharacterMovementComponent::WantsClimbing()
{
	if (!bWantsToClimb)
		bWantsToClimb = CanStartClimbing();
}

void UZCCharacterMovementComponent::CancelClimbing()
{
	bWantsToClimb = false;
}

void UZCCharacterMovementComponent::BeginPlay()
{
	Super::BeginPlay();

	// Don't want to sweep ourselves
	ClimbQueryParams.AddIgnoredActor(GetOwner());
}

void UZCCharacterMovementComponent::TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction)
{
	Super::TickComponent(DeltaTime, TickType, ThisTickFunction);

	SweepAndStoreWallHits();

	if (bIsDebugEnabled != CVarDebugToggle.GetValueOnAnyThread())
		bIsDebugEnabled = CVarDebugToggle.GetValueOnAnyThread();

}

void UZCCharacterMovementComponent::OnMovementUpdated(float DeltaSeconds, const FVector& OldLocation, const FVector& OldVelocity)
{
	if (bWantsToClimb)
		SetMovementMode(EMovementMode::MOVE_Custom, ECustomMovementMode::CMOVE_Climbing);

	Super::OnMovementUpdated(DeltaSeconds, OldLocation, OldVelocity);
}

void UZCCharacterMovementComponent::PhysCustom(float deltaTime, int32 Iterations)
{
	if (CustomMovementMode == ECustomMovementMode::CMOVE_Climbing)
		PhysClimbing(deltaTime, Iterations);

	Super::PhysCustom(deltaTime, Iterations);
}

void UZCCharacterMovementComponent::OnMovementModeChanged(EMovementMode PreviousMovementMode, uint8 PreviousCustomMode)
{
	if (IsClimbing())
	{
		bOrientRotationToMovement = false;

		// Shrink down
		UCapsuleComponent* Capsule = CharacterOwner->GetCapsuleComponent();
		if (Capsule)
			Capsule->SetCapsuleHalfHeight(Capsule->GetUnscaledCapsuleHalfHeight() - CollisionCapsulClimbingShinkAmount);
	}

	const bool bWasClimbing = PreviousCustomMode == EMovementMode::MOVE_Custom && PreviousCustomMode == ECustomMovementMode::CMOVE_Climbing;
	if (bWasClimbing)
	{
		bOrientRotationToMovement = true;

		// Reset pitch so we end standing straight up
		const FRotator StandRotation = FRotator(0, UpdatedComponent->GetComponentRotation().Yaw, 0);
		UpdatedComponent->SetRelativeRotation(StandRotation);

		UCapsuleComponent* Capsule = CharacterOwner->GetCapsuleComponent();
		if (Capsule)
			Capsule->SetCapsuleHalfHeight(Capsule->GetUnscaledCapsuleHalfHeight() + CollisionCapsulClimbingShinkAmount);

		// After exiting climbing mode, reset velocity and acceleration
		StopMovementImmediately();
	}

	Super::OnMovementModeChanged(PreviousMovementMode, PreviousCustomMode);
}

float UZCCharacterMovementComponent::GetMaxSpeed() const
{
	return IsClimbing() ? MaxClimbingSpeed : Super::GetMaxSpeed();
}

float UZCCharacterMovementComponent::GetMaxAcceleration() const
{
	return IsClimbing() ? MaxClimbingAcceleration : Super::GetMaxAcceleration();
}

void UZCCharacterMovementComponent::SweepAndStoreWallHits()
{
	const FCollisionShape CollisionShape = FCollisionShape::MakeCapsule(CollisionCapsulRadius, CollisionCapsulHalfHeight);

	const FVector StartOffset = UpdatedComponent->GetForwardVector() * CollisionCapsulForwardOffset;

	const FVector Start = UpdatedComponent->GetComponentLocation() + StartOffset;	// a bit in front
	const FVector End = Start + UpdatedComponent->GetForwardVector();				// using the same start/end location for a sweep doesn't trigger hits on Landscapes

	TArray<FHitResult> Hits;
	check(GetWorld());
	const bool bHitWall = GetWorld()->SweepMultiByChannel(Hits, Start, End, FQuat::Identity, ECC_WorldStatic, CollisionShape, ClimbQueryParams);

	bHitWall ? CurrentWallHits = Hits : CurrentWallHits.Reset();

	DrawDebug(Start);
}

bool UZCCharacterMovementComponent::CanStartClimbing() const
{
	for (const FHitResult& WallHit : CurrentWallHits)
	{
		const bool horCheck = HorizontalClimbCheck(WallHit);
		const bool vertCheck = VerticalClimbCheck(WallHit);
		if (GEngine)
			GEngine->AddOnScreenDebugMessage(-1, 0.1, FColor::White, FString::Printf(TEXT("horCheck %i, vertCheck %i"), horCheck, vertCheck));
			if (horCheck && vertCheck)
				return true;
	}

	return false;
}

bool UZCCharacterMovementComponent::HorizontalClimbCheck(const FHitResult& WallHit) const
{
	const FVector WallHorizontalNormal = WallHit.Normal.GetSafeNormal2D();
	const FVector PlayerForward = UpdatedComponent->GetForwardVector();
	const float LookAngleCos = PlayerForward.Dot(-WallHorizontalNormal);// inverse wall normal so the vectors are pointing the "same" direction
	const float LookAngleDiff = FMath::RadiansToDegrees(FMath::Acos(LookAngleCos));
	
	return LookAngleDiff <= MinHorizontalDegreesToStartClimbing;
}

bool UZCCharacterMovementComponent::VerticalClimbCheck(const FHitResult& WallHit) const
{
	const FVector WallHorizontalNormal = WallHit.Normal.GetSafeNormal2D();
	const float VerticalAngleCos = FVector::DotProduct(WallHit.Normal, WallHorizontalNormal);

	// Check if the surface is too flat
	const bool bIsCeilingOrFloor = FMath::IsNearlyZero(VerticalAngleCos);

	// Check if surface is high enough
	const float CollisionEdge = CollisionCapsulRadius + CollisionCapsulForwardOffset;
	const float SteepnessMultiplier = 1 + (1 - VerticalAngleCos) * 6; // magic number here extends it _just_ a bit further so it works on all the angles we need.
	const float TraceLength = CollisionEdge * SteepnessMultiplier;
	const bool bIsHighEnough = EyeHeightTrace(TraceLength);


	// TODO: minimum steepness requirement otherwise its possible to allow climbing on a very long, not so steep surface that we can otherwise walk up if it _just_ hits the bottom of the collider
	// TODO: perhaps use GetWalkableFloorAngle() to check

	return bIsHighEnough && !bIsCeilingOrFloor;
}

bool UZCCharacterMovementComponent::EyeHeightTrace(const float TraceDistance) const
{
	FHitResult UpperEdgeHit;

	const ACharacter* Owner = GetCharacterOwner();
	const FVector EyeHeight = UpdatedComponent->GetComponentLocation() + (UpdatedComponent->GetUpVector() * (Owner ? Owner->BaseEyeHeight : 1));
	const FVector End = EyeHeight + (UpdatedComponent->GetForwardVector() * TraceDistance);

	DrawEyeTraceDebug(EyeHeight, End);

	return GetWorld()->LineTraceSingleByChannel(UpperEdgeHit, EyeHeight, End, ECC_WorldStatic, ClimbQueryParams);
}

void UZCCharacterMovementComponent::PhysClimbing(float DeltaTime, int32 Iterations)
{
	// Note: Taken from UCharacterMovementComponent::PhysFlying
	if (DeltaTime > MIN_TICK_TIME)
		return;

	ComputeSurfaceInfo();

	if (ShouldStopClimbing())
	{
		StopClimbing(DeltaTime, Iterations);
		return;
	}

	ComputeClimbingVelocity(DeltaTime);

	const FVector OldLocation = UpdatedComponent->GetComponentLocation();

	MoveAlongClimbingSurface(DeltaTime);

	if (!HasAnimRootMotion() && !CurrentRootMotion.HasOverrideVelocity())
		Velocity = (UpdatedComponent->GetComponentLocation() - OldLocation) / DeltaTime;

	SnapToClimbingSurface(DeltaTime);
}

void UZCCharacterMovementComponent::ComputeSurfaceInfo()
{
	// TODO: Take an average of all the hits
	if (CurrentWallHits.Num() > 0)
	{
		CurrentClimbingNormal = CurrentWallHits[0].Normal;
		CurrentClimbingPosition = CurrentWallHits[0].ImpactPoint;
	}

	CurrentClimbingNormal = FVector::ZeroVector;
	CurrentClimbingPosition = FVector::ZeroVector;

	if (CurrentWallHits.IsEmpty())
		return;

	for (const FHitResult& WallHit : CurrentWallHits)
	{
		CurrentClimbingPosition += WallHit.ImpactPoint;
		CurrentClimbingNormal += WallHit.Normal;
	}

	// Store position as the mean of all the surface impacts
	CurrentClimbingPosition /= CurrentWallHits.Num();
	CurrentClimbingNormal = CurrentClimbingNormal.GetSafeNormal();
}

void UZCCharacterMovementComponent::ComputeClimbingVelocity(float DeltaTime)
{
	// Note: Taken from UCharacterMovementComponent::PhysFlying
	RestorePreAdditiveRootMotionVelocity();

	if (!HasAnimRootMotion() && !CurrentRootMotion.HasOverrideVelocity())
	{
		constexpr float Friction = 0.f;
		constexpr bool bFluid = false;
		CalcVelocity(DeltaTime, Friction, bFluid, BrakingDecelerationClimbing);
	}

	ApplyRootMotionToVelocity(DeltaTime);
}

bool UZCCharacterMovementComponent::ShouldStopClimbing()
{
	const bool bIsOnCeiling = FVector::Parallel(CurrentClimbingNormal, FVector::UpVector);
	return !bWantsToClimb || CurrentClimbingNormal.IsZero() || bIsOnCeiling;
}

void UZCCharacterMovementComponent::StopClimbing(float DeltaTime, int32 Iterations)
{
	bWantsToClimb = false;
	SetMovementMode(EMovementMode::MOVE_Falling);
	StartNewPhysics(DeltaTime, Iterations);
}

void UZCCharacterMovementComponent::MoveAlongClimbingSurface(float DeltaTime)
{
	// Note: Taken from UCharacterMovementComponent::PhysFlying
	const FVector Adjusted = Velocity;

	FHitResult Hit(1.f);

	SafeMoveUpdatedComponent(Adjusted, GetSmoothClimbingRotation(DeltaTime), true, Hit);

	if (Hit.Time < 1.f)
	{
		HandleImpact(Hit, DeltaTime, Adjusted);
		SlideAlongSurface(Adjusted, (1.f - Hit.Time), Hit.Normal, Hit, true);
	}
}

FQuat UZCCharacterMovementComponent::GetSmoothClimbingRotation(float DeltaTime) const
{
	// Smoothly rotate towards the surface (opposite surface normal)
	const FQuat Current = UpdatedComponent->GetComponentQuat();
	const FQuat Target = FRotationMatrix::MakeFromX(-CurrentClimbingNormal).ToQuat();

	return FMath::QInterpTo(Current, Target, DeltaTime, ClimbingRotationSpeed);
}

void UZCCharacterMovementComponent::SnapToClimbingSurface(float DeltaTime) const
{
	// TODO: Maybe change to a threshold later on.
	// If within the threshold move smoothly
	// If not we can teleport instantly to the correct distance decreasing the change the character loses grip at high velocities
	const FVector Forward = UpdatedComponent->GetForwardVector();
	const FVector Location = UpdatedComponent->GetComponentLocation();
	const FQuat Rotation = UpdatedComponent->GetComponentQuat();

	const FVector ForwardDifference = (CurrentClimbingPosition - Location).ProjectOnTo(Forward);
	const FVector Offset = -CurrentClimbingNormal * (ForwardDifference.Length() - ClimbingDistanceFromSurface);

	const bool bSweep = true;
	UpdatedComponent->MoveComponent(Offset * ClimbingSnapSpeed * DeltaTime, Rotation, bSweep);
}

void UZCCharacterMovementComponent::DrawEyeTraceDebug(const FVector& Start, const FVector& End) const
{
	if (!bIsDebugEnabled)
		return;

	DrawDebugLine(GetWorld(), Start, End, FColor::Yellow);
}

void UZCCharacterMovementComponent::DrawDebug(FVector SweepLocation) const
{
	if (!bIsDebugEnabled)
		return;

	// collider sweep
	FColor SweepColor = FColor::White;
	if (CurrentWallHits.Num() > 0)
	{
		if (IsClimbing())
			SweepColor = FColor::Green;
		else if (CanStartClimbing() && !bWantsToClimb)
			SweepColor = FColor::Yellow;
		else
			SweepColor = FColor::Red;
	}
	DrawDebugCapsule(GetWorld(), SweepLocation, CollisionCapsulHalfHeight, CollisionCapsulRadius, FQuat::Identity, SweepColor);

	for (const FHitResult& WallHit : CurrentWallHits)
	{
		// hit
		DrawDebugSphere(GetWorld(), WallHit.ImpactPoint, 5, 26, FColor::Blue);

		// hit surface
		const FVector Normal = WallHit.Normal;
		const FVector Right = UpdatedComponent->GetRightVector();
		const FVector Up = FVector::CrossProduct(Right, WallHit.Normal);
		DrawDebugLine(GetWorld(), WallHit.ImpactPoint, WallHit.ImpactPoint + (Normal * 100), FColor::Cyan);
		//DrawDebugLine(GetWorld(), WallHit.ImpactPoint, WallHit.ImpactPoint + (Right * 100), FColor::Red); // Showing the players right doesn't serve a purpose
		DrawDebugLine(GetWorld(), WallHit.ImpactPoint, WallHit.ImpactPoint + (Up * 100), FColor::Green);
	
		// hit 2D surface normal
		const FVector Normal2DDiff = Normal - WallHit.Normal.GetSafeNormal2D();
		if (!Normal2DDiff.IsNearlyZero())
			DrawDebugLine(GetWorld(), WallHit.ImpactPoint, WallHit.ImpactPoint + (WallHit.Normal.GetSafeNormal2D() * 100), FColor::Magenta);
	}
}
