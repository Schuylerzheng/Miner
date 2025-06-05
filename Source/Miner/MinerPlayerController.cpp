// Copyright Epic Games, Inc. All Rights Reserved.


#include "MinerPlayerController.h"
#include "EnhancedInputSubsystems.h"
#include "Engine/LocalPlayer.h"
#include "InputMappingContext.h"
#include "MinerCameraManager.h"

AMinerPlayerController::AMinerPlayerController()
{
	// set the player camera manager class
	PlayerCameraManagerClass = AMinerCameraManager::StaticClass();
}

void AMinerPlayerController::SetupInputComponent()
{
	Super::SetupInputComponent();

	// Add Input Mapping Context
	if (UEnhancedInputLocalPlayerSubsystem* Subsystem = ULocalPlayer::GetSubsystem<UEnhancedInputLocalPlayerSubsystem>(GetLocalPlayer()))
	{
		for (UInputMappingContext* CurrentContext : DefaultMappingContexts)
		{
			Subsystem->AddMappingContext(CurrentContext, 0);
		}
	}
}
