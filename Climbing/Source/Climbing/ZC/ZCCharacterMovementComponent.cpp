// Fill out your copyright notice in the Description page of Project Settings.


#include "Climbing/ZC/ZCCharacterMovementComponent.h"

void UZCCharacterMovementComponent::DrawDebug()
{
	if (GEngine)
		GEngine->AddOnScreenDebugMessage(-1, 10, FColor::Yellow, TEXT("Custom Character Movement Component being used"));
}
