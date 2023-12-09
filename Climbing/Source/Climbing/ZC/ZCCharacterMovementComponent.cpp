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

void UZCCharacterMovementComponent::TryClimbDashing()
{
	if (!IsClimbing())
		return;

	if (ClimbDashCurve && !bWantsToClimbDash)
	{
		bWantsToClimbDash = true;
		CurrentClimbDashTime = 0.f;

		CacheClimbDashDirection();
	}
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

	AnimInstance = GetCharacterOwner()->GetMesh()->GetAnimInstance();

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

	const bool bWasClimbing = PreviousMovementMode == EMovementMode::MOVE_Custom && PreviousCustomMode == ECustomMovementMode::CMOVE_Climbing;
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
		if (HorizontalClimbCheck(WallHit) && VerticalClimbCheck(WallHit))
			return true;

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
	const float BaseEyeHeight = Owner ? Owner->BaseEyeHeight + 30 : 1;
	const float EyeHeightOffset = IsClimbing() ? BaseEyeHeight + CollisionCapsulClimbingShinkAmount : BaseEyeHeight;

	const FVector EyeHeight = UpdatedComponent->GetComponentLocation() + (UpdatedComponent->GetUpVector() * EyeHeightOffset);
	const FVector End = EyeHeight + (UpdatedComponent->GetForwardVector() * TraceDistance);

	DrawEyeTraceDebug(EyeHeight, End);

	return GetWorld()->LineTraceSingleByChannel(UpperEdgeHit, EyeHeight, End, ECC_WorldStatic, ClimbQueryParams);
}

void UZCCharacterMovementComponent::PhysClimbing(float DeltaTime, int32 Iterations)
{
	// Note: Taken from UCharacterMovementComponent::PhysFlying
	if (DeltaTime < MIN_TICK_TIME)
		return;

	ComputeSurfaceInfo();

	if (ShouldStopClimbing() || ClimbDownToFloor())
	{
		// Don't exit climbing if the montage is done otherwise the capsule returns to full height and regular physics takes over too early resulting in falling off the ledge
		if (AnimInstance && !AnimInstance->Montage_IsPlaying(LedgeClimbMontage))
		{
			StopClimbing(DeltaTime, Iterations);
			return;
		}
	}

	UpdateClimbDashState(DeltaTime);
	ComputeClimbingVelocity(DeltaTime);

	const FVector OldLocation = UpdatedComponent->GetComponentLocation();

	MoveAlongClimbingSurface(DeltaTime);
	TryClimbUpLedge();

	if (!HasAnimRootMotion() && !CurrentRootMotion.HasOverrideVelocity())
		Velocity = (UpdatedComponent->GetComponentLocation() - OldLocation) / DeltaTime;

	SnapToClimbingSurface(DeltaTime);
}

void UZCCharacterMovementComponent::ComputeSurfaceInfo()
{
	CurrentClimbingNormal = FVector::ZeroVector;
	CurrentClimbingPosition = FVector::ZeroVector;

	if (CurrentWallHits.IsEmpty())
		return;

	const FVector Start = UpdatedComponent->GetComponentLocation();
	const FCollisionShape CollisionShape = FCollisionShape::MakeSphere(6);//TODO: magic number just for a reasonably smol sphere

	for (const FHitResult& WallHit : CurrentWallHits)
	{
		// Using an additional raycast from the character to the point of impact makes sure if the sweep was _under_ or _inside_ geometry we only take the normal of the first face we encounter
		const FVector End = Start + (WallHit.ImpactPoint - Start).GetSafeNormal() * 120; //TODO: magic number here is just making sure we make it to the surface
		FHitResult AssistHit;
		GetWorld()->SweepSingleByChannel(AssistHit, Start, End, FQuat::Identity, ECC_WorldStatic, CollisionShape, ClimbQueryParams);

		CurrentClimbingPosition += AssistHit.ImpactPoint;
		CurrentClimbingNormal += AssistHit.Normal;
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
		if (bWantsToClimbDash)
		{
			AlignClimbDashDirection();

			const float CurrentCurveSpeed = ClimbDashCurve->GetFloatValue(CurrentClimbDashTime);
			Velocity = ClimbDashDirection * CurrentCurveSpeed;
		}
		else
		{
			constexpr float Friction = 0.f;
			constexpr bool bFluid = false;
			CalcVelocity(DeltaTime, Friction, bFluid, BrakingDecelerationClimbing);
		}
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
	StopClimbDashing();

	bWantsToClimb = false;
	bIsInLedgeClimb = false;
	SetMovementMode(EMovementMode::MOVE_Falling);
	StartNewPhysics(DeltaTime, Iterations);
}

void UZCCharacterMovementComponent::MoveAlongClimbingSurface(float DeltaTime)
{
	// Note: Taken from UCharacterMovementComponent::PhysFlying
	const FVector Adjusted = Velocity * DeltaTime;

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
	if (HasAnimRootMotion() || CurrentRootMotion.HasOverrideVelocity())
		return Current;

	const FQuat Target = FRotationMatrix::MakeFromX(-CurrentClimbingNormal).ToQuat();
	const float RotationSpeed = ClimbingRotationSpeed * FMath::Max(1, Velocity.Length() / MaxClimbingSpeed);// TODO: investigate an alternate way to do this

	return FMath::QInterpTo(Current, Target, DeltaTime, RotationSpeed);
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
	const float SnapSpeed = ClimbingSnapSpeed * FMath::Max(1, Velocity.Length() / MaxClimbingSpeed);
	UpdatedComponent->MoveComponent(Offset * SnapSpeed * DeltaTime, Rotation, bSweep);
}

void UZCCharacterMovementComponent::CacheClimbDashDirection()
{
	ClimbDashDirection = UpdatedComponent->GetUpVector();

	const float AccelerationThreshold = MaxClimbingAcceleration / 10;// magic number here just for testing
	if (Acceleration.Length() > AccelerationThreshold)
		ClimbDashDirection = Acceleration.GetSafeNormal();
}

void UZCCharacterMovementComponent::UpdateClimbDashState(float DeltaTime)
{
	if (!bWantsToClimbDash)
		return;

	CurrentClimbDashTime += DeltaTime;

	// TODO: Better to cache it when dash starts
	float MinTime, MaxTime;
	ClimbDashCurve->GetTimeRange(MinTime, MaxTime);

	if (CurrentClimbDashTime >= MaxTime)
		StopClimbDashing();
}

void UZCCharacterMovementComponent::AlignClimbDashDirection()
{
	const FVector HorizontalSurfaceNormal = GetClimbSurfaceNormal().GetSafeNormal2D();// gets the X and Y of the surface we're essentially prone against so up/down left/right
	ClimbDashDirection = FVector::VectorPlaneProject(ClimbDashDirection, HorizontalSurfaceNormal);// TODO: investigate an alternate way to do this
}

void UZCCharacterMovementComponent::StopClimbDashing()
{
	bWantsToClimbDash = false;
	CurrentClimbDashTime = 0.f;
}

bool UZCCharacterMovementComponent::ClimbDownToFloor() const
{
	FHitResult FloorHit;
	if (!CheckFloor(FloorHit))
		return false;

	// GetWalkableFloor is a unit vector describing the steepest walkable surface (~45 degrees, with Z of ~0.7)
	// A hit normal with Z of 1 indicates the surface is perpendicular with respect to Z up.
	// As the hit normal's Z goes from 1 -> 0 that indicates the surface is angled more and more until
	// A hit normal with Z of (near)0 indicates the surface is (near)parallel 
	const bool bOnWalkableFloor = FloorHit.Normal.Z > GetWalkableFloorZ();

	// Upwards velocity will be opposite the -normal resulting in negatives
	// Downwards velocity will be positive
	const float DownSpeed = FVector::DotProduct(Velocity.GetSafeNormal(), -FloorHit.Normal);
	const bool bIsMovingTowardsFloor = DownSpeed > 0 && bOnWalkableFloor;

	// Just a double check to make sure the raycast didn't hit a tiny steep slope, but the actual surface we're climbing on is floor
	const bool bIsClimbingFloor = CurrentClimbingNormal.Z > GetWalkableFloorZ();

	return bIsMovingTowardsFloor || (bIsClimbingFloor && bOnWalkableFloor);
}

bool UZCCharacterMovementComponent::CheckFloor(FHitResult& OutFloorHit) const
{
	const FVector Start = UpdatedComponent->GetComponentLocation();
	const FVector End = Start + FVector::DownVector * FloorCheckDistance;

	DrawClimbDownDebug(Start, End);

	return GetWorld()->LineTraceSingleByChannel(OutFloorHit, Start, End, ECC_WorldStatic, ClimbQueryParams);
}

bool UZCCharacterMovementComponent::TryClimbUpLedge()
{
	if (!AnimInstance || !LedgeClimbMontage)
		return false;
	if (AnimInstance->Montage_IsPlaying(LedgeClimbMontage))
		return false;

	const float UpSpeed = FVector::DotProduct(Velocity.GetSafeNormal(), UpdatedComponent->GetUpVector());
	const bool bIsMovingUp = UpSpeed > 0;

	if (bIsMovingUp && HasReachedLedge() && CanMoveToLedgeClimbLocation())
	{
		const FRotator StandRotation = FRotator(0, UpdatedComponent->GetComponentRotation().Yaw, 0);
		UpdatedComponent->SetRelativeRotation(StandRotation);
		AnimInstance->Montage_Play(LedgeClimbMontage);
		bIsInLedgeClimb = true;

		return true;
	}

	return false;
}

bool UZCCharacterMovementComponent::HasReachedLedge() const
{
	//const UCapsuleComponent* Capsule = CharacterOwner->GetCapsuleComponent();
	const float TraceDistance = CollisionCapsulRadius + CollisionCapsulForwardOffset;
	
	return !EyeHeightTrace(TraceDistance);
}

bool UZCCharacterMovementComponent::IsLedgeWalkable(const FVector& LocationToCheck) const
{
	const FVector CheckEnd = LocationToCheck + (FVector::DownVector * 250.f);

	FHitResult LedgeHit;
	const bool dHitLedgeGround = GetWorld()->LineTraceSingleByChannel(LedgeHit, LocationToCheck, CheckEnd, ECC_WorldStatic, ClimbQueryParams);

	return dHitLedgeGround && LedgeHit.Normal.Z >= GetWalkableFloorZ();
}

bool UZCCharacterMovementComponent::CanMoveToLedgeClimbLocation() const
{
	// TODO: Change magic number into property
	const FVector VerticalOffset = FVector::UpVector * 200.f;
	const FVector HorizontalOffset = UpdatedComponent->GetForwardVector() * 120.f;

	const FVector LocationToCheck = UpdatedComponent->GetComponentLocation() + HorizontalOffset + VerticalOffset;

	if (!IsLedgeWalkable(LocationToCheck))
		return false;

	FHitResult CapsulHit;
	const FVector CapsulStartCheck = LocationToCheck - HorizontalOffset;
	const UCapsuleComponent* Capsule = CharacterOwner->GetCapsuleComponent();

	const bool bBlocked = GetWorld()->SweepSingleByChannel(CapsulHit, CapsulStartCheck, LocationToCheck, FQuat::Identity, ECC_WorldStatic, Capsule->GetCollisionShape(), ClimbQueryParams);
	return !bBlocked;
}

void UZCCharacterMovementComponent::DrawClimbDownDebug(const FVector& Start, const FVector& End) const
{
	if (!bIsDebugEnabled)
		return;

	DrawDebugLine(GetWorld(), Start, End, FColor::Yellow);
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
		if (IsClimbDashing())
			SweepColor = FColor::Magenta;
		else if (IsClimbing())
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
