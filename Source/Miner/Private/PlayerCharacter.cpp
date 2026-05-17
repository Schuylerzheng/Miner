// Copyright Schuyler Zheng. All Rights Reserved.

#include "PlayerCharacter.h"
#include "AbilitySystemComponent.h"
#include "EnhancedInputComponent.h"
#include "EnhancedInputSubsystems.h"
#include "Components/CapsuleComponent.h"

APlayerCharacter::APlayerCharacter()
{
	const FText& Adj = Adjectives[FMath::RandRange(0, Adjectives.Num() - 1)];
	const FText& Noun = Nouns[FMath::RandRange(0, Nouns.Num() - 1)];

	DisplayName = FText::FromString(FString::Printf(TEXT("%s%s"), *Adj.ToString(), *Noun.ToString()));
}

void APlayerCharacter::BeginPlay()
{
	Super::BeginPlay();
}

void APlayerCharacter::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);
}

void APlayerCharacter::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	Super::EndPlay(EndPlayReason);
}

void APlayerCharacter::SetupPlayerInputComponent(UInputComponent* PlayerInputComponent)
{
	Super::SetupPlayerInputComponent(PlayerInputComponent);

	// Set up action bindings
	if (UEnhancedInputComponent* EnhancedInputComponent = Cast<UEnhancedInputComponent>(PlayerInputComponent))	{
		// Jumping
		EnhancedInputComponent->BindAction(JumpAction, ETriggerEvent::Started, this, &APlayerCharacter::DoJumpStart);
		EnhancedInputComponent->BindAction(JumpAction, ETriggerEvent::Completed, this, &APlayerCharacter::DoJumpEnd);

		// Moving
		EnhancedInputComponent->BindAction(MoveAction, ETriggerEvent::Triggered, this, &APlayerCharacter::MoveInput);

		// Looking/Aiming
		EnhancedInputComponent->BindAction(LookAction, ETriggerEvent::Triggered, this, &APlayerCharacter::LookInput);
		EnhancedInputComponent->BindAction(MouseLookAction, ETriggerEvent::Triggered, this, &APlayerCharacter::LookInput);

		// Left click
		EnhancedInputComponent->BindAction(LeftClickAction, ETriggerEvent::Started, this, &APlayerCharacter::DoStartLeftClick);
		EnhancedInputComponent->BindAction(LeftClickAction, ETriggerEvent::Completed, this, &APlayerCharacter::DoStopLeftClick);

		// Right click
		EnhancedInputComponent->BindAction(RightClickAction, ETriggerEvent::Started, this, &APlayerCharacter::DoStartRightClick);
		EnhancedInputComponent->BindAction(RightClickAction, ETriggerEvent::Completed, this, &APlayerCharacter::DoStopRightClick);

		// Sprint
		EnhancedInputComponent->BindAction(SprintAction, ETriggerEvent::Started, this, &APlayerCharacter::DoStartSprint);
		EnhancedInputComponent->BindAction(SprintAction, ETriggerEvent::Completed, this, &APlayerCharacter::DoEndSprint);

		// Crouch
		EnhancedInputComponent->BindAction(CrouchAction, ETriggerEvent::Started, this, &APlayerCharacter::DoStartCrouch);
		EnhancedInputComponent->BindAction(CrouchAction, ETriggerEvent::Completed, this, &APlayerCharacter::DoEndCrouch);

		// Switch item
		EnhancedInputComponent->BindAction(SwitchItemAction, ETriggerEvent::Triggered, this, &APlayerCharacter::DoSwitchItem);
	}
	else {
		UE_LOG(LogTemplateCharacter, Error, TEXT("'%s' Failed to find an Enhanced Input Component! This template is built to use the Enhanced Input system. If you intend to use the legacy system, then you will need to update this C++ file."), *GetNameSafe(this));
	}

	// Set up GAS action bindings currently not doing
	if (IsValid(PlayerInputComponent) && IsValid(AbilitySystemComponent))
	{
		// Bind the ability system component to the input component
		AbilitySystemComponent->BindToInputComponent(PlayerInputComponent);
	}
}
