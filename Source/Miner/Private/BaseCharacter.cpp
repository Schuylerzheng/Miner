// Copyright Schuyler Zheng. All Rights Reserved.

#include "BaseCharacter.h"
#include "BaseCharacterAttributeSet.h"
#include "GameplayAbilitySystem/CharacterAbilities.h"
#include "AbilitySystemComponent.h"
#include <typeinfo>

DEFINE_LOG_CATEGORY(LogBaseCharacter);

ABaseCharacter::ABaseCharacter()
{
	// configure movement (bugged + useless)
	//GetCharacterMovement()->RotationRate = FRotator(0.0f, 600.0f, 0.0f);

	// create the ability system component 
	AbilitySystemComponent = CreateDefaultSubobject<UAbilitySystemComponent>(TEXT("AbilitySystemComponent"));
}

void ABaseCharacter::BeginPlay()
{
	Super::BeginPlay();

	// Do not overwrite the member created in the constructor.
	// If for some reason it wasn't created, try to find a component on the actor.
	// Also make sure it is good
	if (!IsValid(AbilitySystemComponent)) {	AbilitySystemComponent = FindComponentByClass<UAbilitySystemComponent>(); }
	checkf(IsValid(AbilitySystemComponent), TEXT("Ability System Component was Invalid on BaseCharacter.cpp"));

	// Get attribute set and validate it.
	AttributeSet = AbilitySystemComponent->GetSet<UBaseCharacterAttributeSet>();
	checkf(AttributeSet != nullptr, TEXT("UBaseCharacterAttributeSet was null on BaseCharacter.cpp"));

	// Grant abilities to the character
	GrantAbilities();

	// Make stamina regen (validate the effect class and the spec before applying)
	checkf(IsValid(StaminaRegenClass), TEXT("Needs a valid staminaregenclass"));

	FGameplayEffectContextHandle ContextHandle = AbilitySystemComponent->MakeEffectContext();
	ContextHandle.AddSourceObject(this);

	FGameplayEffectSpecHandle SpecHandle = AbilitySystemComponent->MakeOutgoingSpec(StaminaRegenClass, 1.0f, ContextHandle);
	if (!SpecHandle.Data.IsValid()) { AbilitySystemComponent->ApplyGameplayEffectSpecToSelf(*SpecHandle.Data.Get(), AbilitySystemComponent->ScopedPredictionKey); }
}

void ABaseCharacter::DoJumpStart()
{
	UE_LOG(LogBaseCharacter, Log, TEXT("Jump ability requested for character %s"), *DisplayName.ToString());
	AbilitySystemComponent->AbilityLocalInputPressed(static_cast<int32>(EAbilitiesIndex::JumpAbility));
}

void ABaseCharacter::DoJumpEnd()
{
	UE_LOG(LogBaseCharacter, Log, TEXT("End jump ability requested for character %s"), *DisplayName.ToString());
	AbilitySystemComponent->AbilityLocalInputReleased(static_cast<int32>(EAbilitiesIndex::JumpAbility));
}

void ABaseCharacter::DoStartRightClick()
{
	
}

void ABaseCharacter::DoStopRightClick()
{
}

void ABaseCharacter::DoStartLeftClick()
{
}

void ABaseCharacter::DoStopLeftClick()
{
}

void ABaseCharacter::DoStartSprint()
{
	UE_LOG(LogBaseCharacter, Log, TEXT("Sprint ability requested for character %s"), *DisplayName.ToString());
	AbilitySystemComponent->AbilityLocalInputPressed(static_cast<int32>(EAbilitiesIndex::SprintAbility));
}

void ABaseCharacter::DoEndSprint()
{
	UE_LOG(LogBaseCharacter, Log, TEXT("End sprint ability requested for character %s"), *DisplayName.ToString());
	AbilitySystemComponent->AbilityLocalInputReleased(static_cast<int32>(EAbilitiesIndex::SprintAbility));
}

void ABaseCharacter::DoStartCrouch()
{
	UE_LOG(LogBaseCharacter, Log, TEXT("Crouch ability requested for character %s"), *DisplayName.ToString());
	AbilitySystemComponent->AbilityLocalInputPressed(static_cast<int32>(EAbilitiesIndex::CrouchAbility));
}

void ABaseCharacter::DoEndCrouch()
{
	UE_LOG(LogBaseCharacter, Log, TEXT("End crouch ability requested for character %s"), *DisplayName.ToString());
	AbilitySystemComponent->AbilityLocalInputReleased(static_cast<int32>(EAbilitiesIndex::CrouchAbility));
}

void ABaseCharacter::DoSwitchItem()
{
}

void ABaseCharacter::GrantAbilities()
{
	if (!HasAuthority()) { return; }

	int CurrentAbilityIndex = 0;

	check(IsValid(AbilitySystemComponent));

	for (TSubclassOf<UGameplayAbility> AbilityClass : DefaultAbilities)
	{
		checkf(IsValid(AbilityClass), TEXT("Ability Class invalid while trying to grant abilities. Ability type: %s"), *GetNameSafe(AbilityClass));

		AbilitySystemComponent->GiveAbility(FGameplayAbilitySpec(AbilityClass, 1, CurrentAbilityIndex, this));

		CurrentAbilityIndex++;
	}
}
