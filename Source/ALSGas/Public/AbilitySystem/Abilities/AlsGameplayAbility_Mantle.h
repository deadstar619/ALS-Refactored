// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "AbilitySystem/AlsGameplayAbility_Base.h"
#include "Settings/AlsMantlingSettings.h"
#include "AlsGameplayAbility_Mantle.generated.h"

class AAlsCharacter;
class UAbilityTask_PlayMontageAndWait;

/**
 * 
 */
UCLASS()
class ALSGAS_API UAlsGameplayAbility_Mantle : public UAlsGameplayAbility_Base
{
	GENERATED_BODY()

public:
	UFUNCTION(BlueprintImplementableEvent, Category = "Events|Mantling", meta = (DisplayName = "OnMantleStart"))
	void K2_OnMantleStart(const FAlsMantlingParameters& MantlingParams, const UAlsMantlingSettings* MantlingSettings);

	UFUNCTION(BlueprintImplementableEvent, Category = "Events|Mantling", meta = (DisplayName = "OnMantleEnd"))
	void K2_OnMantleEnd(const FAlsMantlingParameters& MantlingParams, const UAlsMantlingSettings* MantlingSettings);
	
	UFUNCTION(BlueprintNativeEvent, Category = "Als Character")
	UAlsMantlingSettings* SelectMantlingSettings(EAlsMantlingType MantlingType);
	
protected:
	virtual void ActivateLocalPlayerAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilityActivationInfo ActivationInfo, const FGameplayEventData* TriggerEventData) override;

	virtual void ActivateAbilityWithTargetData(const FGameplayAbilityTargetDataHandle& TargetDataHandle, FGameplayTag ApplicationTag) override;

	virtual bool MakeMantlingParams(FAlsMantlingParameters& OutMantlingParams);
	
	UFUNCTION()
	void OnMantleStart(const FAlsMantlingParameters& MantlingParameters, const UAlsMantlingSettings* MantlingSettings);

	UFUNCTION()
	void OnMantleEnd(const FAlsMantlingParameters& MantlingParameters, const UAlsMantlingSettings* MantlingSettings);

public:
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Settings")
	FAlsGeneralMantlingSettings Settings;
	
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Settings", meta=(Categories = "Als.OverlayMode"))
	TMap<FGameplayTag, TSoftObjectPtr<UAlsMantlingSettings>> LowMantleSettings;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Settings")
	TObjectPtr<UAlsMantlingSettings> HighMantleSettings;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Settings")
	TObjectPtr<UAlsMantlingSettings> InAirMantleSettings;

private:
	UPROPERTY(Transient)
	TWeakObjectPtr<AAlsCharacter> AlsCharacter = nullptr; 
};
