
#include "Climbing/ZC/ZCClimbingCharacter.h"
#include "Climbing/ZC/ZCCharacterMovementComponent.h"

AZCClimbingCharacter::AZCClimbingCharacter(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer.SetDefaultSubobjectClass<UZCCharacterMovementComponent>(ACharacter::CharacterMovementComponentName))
{
	PrimaryActorTick.bCanEverTick = true;
	MovementComponent = Cast<UZCCharacterMovementComponent>(GetCharacterMovement());
}

void AZCClimbingCharacter::BeginPlay()
{
	Super::BeginPlay();

	if (GEngine)
		GEngine->AddOnScreenDebugMessage(-1, 10, FColor::Green, TEXT("Confirming custom character class is being used"));

	if (UZCCharacterMovementComponent* ZCMoveComponent = GetZCMovementComponent())
		ZCMoveComponent->DrawDebug();
}

void AZCClimbingCharacter::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);
}

void AZCClimbingCharacter::SetupPlayerInputComponent(UInputComponent* PlayerInputComponent)
{
	Super::SetupPlayerInputComponent(PlayerInputComponent);
}
