// Fill out your copyright notice in the Description page of Project Settings.


#include "Climbing/ZC/ZCCharacterMovementComponent.h"
#include "Climbing/ZC/ZCTypes.h"

#include "GameFramework/Character.h"

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
	//TODO: Figure out a better way to do this. Average them all out probably
	return CurrentWallHits.Num() > 0 ? CurrentWallHits[0].Normal : FVector::Zero();
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
	const FVector EyeHeight = UpdatedComponent->GetComponentLocation() + (UpdatedComponent->GetUpVector() * (Owner ? Owner->BaseEyeHeight : 1));
	const FVector End = EyeHeight + (UpdatedComponent->GetForwardVector() * TraceDistance);

	DrawEyeTraceDebug(EyeHeight, End);

	return GetWorld()->LineTraceSingleByChannel(UpperEdgeHit, EyeHeight, End, ECC_WorldStatic, ClimbQueryParams);
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
	FColor SweepColor = CurrentWallHits.Num() == 0 ? FColor::White : CanStartClimbing() ? FColor::Yellow : FColor::Red;
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
