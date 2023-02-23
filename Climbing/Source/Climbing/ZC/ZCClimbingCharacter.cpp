
#include "Climbing/ZC/ZCClimbingCharacter.h"
#include "Climbing/ZC/ZCCharacterMovementComponent.h"

#include "EnhancedInputComponent.h"

AZCClimbingCharacter::AZCClimbingCharacter(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer.SetDefaultSubobjectClass<UZCCharacterMovementComponent>(ACharacter::CharacterMovementComponentName))
{
	PrimaryActorTick.bCanEverTick = true;
}

void AZCClimbingCharacter::BeginPlay()
{
	Super::BeginPlay();

	MovementComponent = Cast<UZCCharacterMovementComponent>(GetCharacterMovement());
}

void AZCClimbingCharacter::Move(const FInputActionValue& Value)
{
	// input is a Vector2D
	FVector2D MovementVector = Value.Get<FVector2D>();

	if (Controller != nullptr)
	{
		if (MovementComponent && MovementComponent->IsClimbing())
		{
			FVector SurfaceUpDirection = FVector::CrossProduct(MovementComponent->GetClimbSurfaceNormal(), -GetActorRightVector());
			FVector SurfaceRightDirection = FVector::CrossProduct(MovementComponent->GetClimbSurfaceNormal(), GetActorUpVector());

			AddMovementInput(SurfaceUpDirection, MovementVector.Y);
			AddMovementInput(SurfaceRightDirection, MovementVector.X);
		}
		else
		{
			// find out which way is forward
			const FRotator Rotation = Controller->GetControlRotation();
			const FRotator YawRotation(0, Rotation.Yaw, 0);

			// get forward vector
			const FVector ForwardDirection = FRotationMatrix(YawRotation).GetUnitAxis(EAxis::X);

			// get right vector 
			const FVector RightDirection = FRotationMatrix(YawRotation).GetUnitAxis(EAxis::Y);

			// add movement 
			AddMovementInput(ForwardDirection, MovementVector.Y);
			AddMovementInput(RightDirection, MovementVector.X);
		}
	}
}

void AZCClimbingCharacter::Climb(const FInputActionValue& Value)
{
	if (MovementComponent)
		MovementComponent->WantsClimbing();
}

void AZCClimbingCharacter::CancelClimb(const FInputActionValue& Value)
{
	if (MovementComponent)
		MovementComponent->CancelClimbing();
}

void AZCClimbingCharacter::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);
}

void AZCClimbingCharacter::SetupPlayerInputComponent(UInputComponent* PlayerInputComponent)
{
	Super::SetupPlayerInputComponent(PlayerInputComponent);

	// Set up action bindings
	if (UEnhancedInputComponent* EnhancedInputComponent = CastChecked<UEnhancedInputComponent>(PlayerInputComponent)) 
	{
		// Climbing
		EnhancedInputComponent->BindAction(ClimbAction, ETriggerEvent::Triggered, this, &AZCClimbingCharacter::Climb);
		EnhancedInputComponent->BindAction(CancelClimbAction, ETriggerEvent::Triggered, this, &AZCClimbingCharacter::CancelClimb);
	}
}
