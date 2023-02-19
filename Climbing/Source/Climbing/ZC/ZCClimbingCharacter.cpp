// Fill out your copyright notice in the Description page of Project Settings.


#include "Climbing/ZC/ZCClimbingCharacter.h"

AZCClimbingCharacter::AZCClimbingCharacter()
{
	PrimaryActorTick.bCanEverTick = true;
}

void AZCClimbingCharacter::BeginPlay()
{
	Super::BeginPlay();

	if (GEngine)
		GEngine->AddOnScreenDebugMessage(-1, 10, FColor::Green, TEXT("Confirming custom character class is being used"));
}

void AZCClimbingCharacter::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);
}

void AZCClimbingCharacter::SetupPlayerInputComponent(UInputComponent* PlayerInputComponent)
{
	Super::SetupPlayerInputComponent(PlayerInputComponent);
}
