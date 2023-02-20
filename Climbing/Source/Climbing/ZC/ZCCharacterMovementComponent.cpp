// Fill out your copyright notice in the Description page of Project Settings.


#include "Climbing/ZC/ZCCharacterMovementComponent.h"

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
}

void UZCCharacterMovementComponent::SweepAndStoreWallHits()
{
	const FCollisionShape CollisionShape = FCollisionShape::MakeCapsule(CollisionCapsulRadius, CollisionCapsulHalfHeight);

	const FVector StartOffset = UpdatedComponent->GetForwardVector() * 20;

	const FVector Start = UpdatedComponent->GetComponentLocation() + StartOffset;	// 20 units in front
	const FVector End = Start + UpdatedComponent->GetForwardVector();				// using the same start/end location for a sweep doesn't trigger hits on Landscapes

	TArray<FHitResult> Hits;
	check(GetWorld());
	const bool bHitWall = GetWorld()->SweepMultiByChannel(Hits, Start, End, FQuat::Identity, ECC_WorldStatic, CollisionShape, ClimbQueryParams);

	bHitWall ? CurrentWallHits = Hits : CurrentWallHits.Reset();

	DrawDebug(Start);
}

bool UZCCharacterMovementComponent::CanStartClimbing()
{
	for (const FHitResult& WallHit : CurrentWallHits)
	{

		const FVector WallHorizontalNormal = WallHit.Normal.GetSafeNormal2D();

		const FVector PlayerForward = UpdatedComponent->GetForwardVector();
		const float HorizontalAngleCos = FVector::DotProduct(PlayerForward, -WallHorizontalNormal);// inverse wall normal so they vectors are pointing the "same" direction
		const bool bIsLookingCloseEnough = FMath::RadiansToDegrees(FMath::Acos(HorizontalAngleCos)) <= MinHorizontalDegreesToStartClimbing;
		
		const float VerticalAngleCos = FVector::DotProduct(WallHit.Normal, WallHorizontalNormal);
		const bool bIsCeiling = FMath::IsNearlyZero(VerticalAngleCos);

		if (bIsLookingCloseEnough && !bIsCeiling)
			return true;
	}
	return false;
}

void UZCCharacterMovementComponent::DrawDebug(FVector SweepLocation)
{
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
