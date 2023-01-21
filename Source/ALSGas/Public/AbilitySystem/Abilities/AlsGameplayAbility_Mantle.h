// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "AbilitySystem/AlsGameplayAbility_Base.h"
#include "Settings/AlsMantlingSettings.h"
#include "AlsGameplayAbility_Mantle.generated.h"

class AAlsCharacter;

/**
 * 
 */
UCLASS()
class ALSGAS_API UAlsGameplayAbility_Mantle : public UAlsGameplayAbility_Base
{
	GENERATED_BODY()

protected:	
	virtual void ActivateLocalPlayerAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilityActivationInfo ActivationInfo, const FGameplayEventData* TriggerEventData) override;
	virtual void ActivateAbilityWithTargetData(const FGameplayAbilityTargetDataHandle& TargetDataHandle, FGameplayTag ApplicationTag) override;
	
	bool MakeMantlingTargetData(FAlsMantlingParameters& OutMantlingParams);

public:
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Settings")
	FAlsGeneralMantlingSettings Settings;

private:
	UPROPERTY(Transient)
	TWeakObjectPtr<AAlsCharacter> AlsCharacter = nullptr; 
};
