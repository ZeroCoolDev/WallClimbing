// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Climbing/ClimbingCharacter.h"
#include "ZCClimbingCharacter.generated.h"

/**
 * 
 */
UCLASS(config=Game)
class CLIMBING_API AZCClimbingCharacter : public AClimbingCharacter
{
	GENERATED_BODY()

public:
	AZCClimbingCharacter(const FObjectInitializer&);
	virtual ~AZCClimbingCharacter(){}
	virtual void Tick(float DeltaTime) override;
	virtual void SetupPlayerInputComponent(class UInputComponent* PlayerInputComponent) override;

	UFUNCTION(BlueprintPure)
	class UZCCharacterMovementComponent* GetZCMovementComponent() const { return MovementComponent; }

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = Input, meta = (AllowPrivateAccess = "true"))
	class UInputAction* ClimbAction;
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = Input, meta = (AllowPrivateAccess = "true"))
	class UInputAction* CancelClimbAction;
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = Input, meta = (AllowPrivateAccess = "true"))
	class UInputAction* ClimbDashAction;

protected:
	virtual void BeginPlay() override;
	virtual void Move(const FInputActionValue& Value) override;

	// Process raw input for velocity for climbing
	void Climb(const FInputActionValue& Value);
	void CancelClimb(const FInputActionValue& Value);
	void ClimbDash(const FInputActionValue& Value);

	UPROPERTY(Category = Character, VisibleAnywhere, BlueprintReadOnly)
	class UZCCharacterMovementComponent* MovementComponent;
};
