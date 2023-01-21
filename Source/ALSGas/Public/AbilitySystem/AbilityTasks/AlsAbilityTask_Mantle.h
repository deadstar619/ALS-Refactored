// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Abilities/Tasks/AbilityTask.h"
#include "Settings/AlsMantlingSettings.h"
#include "AlsAbilityTask_Mantle.generated.h"

class AAlsCharacter;

DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FAlsMantleDelegate, const FAlsMantlingParameters&, MantlingParameters, const UAlsMantlingSettings*, MantlingSettings);

UCLASS()
class ALSGAS_API UAlsAbilityTask_Mantle : public UAbilityTask
{
	GENERATED_UCLASS_BODY()

	UPROPERTY(BlueprintAssignable)
	FAlsMantleDelegate OnMantleStarted;
	
	UPROPERTY(BlueprintAssignable)
	FAlsMantleDelegate OnMantleCompleted;

	/** Mantles with the given parameters */
	UFUNCTION(BlueprintCallable, Category = "Ability|Tasks", meta = (HidePin = "OwningAbility", DefaultToSelf = "OwningAbility", BlueprintInternalUseOnly = "TRUE"))
	static UAlsAbilityTask_Mantle* Mantle(UGameplayAbility* OwningAbility, FName TaskInstanceName, const FAlsMantlingParameters& Parameters, UAlsMantlingSettings* Settings);

	// ~ Begin UAbilityTask Interface
	virtual void Activate() override;
	virtual void TickTask(float DeltaTime) override;
	virtual void OnDestroy(bool AbilityIsEnding) override;
	virtual void InitSimulatedTask(UGameplayTasksComponent& InGameplayTasksComponent) override;
	// ~ End UAbilityTask Interface
	
private:
	virtual void RefreshMantling();
	virtual void StopMantling();

protected:
	UPROPERTY(Transient)
	TWeakObjectPtr<AAlsCharacter> AlsCharacter;

	UPROPERTY(Replicated, Transient)
	FAlsMantlingParameters Parameters;
	
	UPROPERTY(Transient)
	TWeakObjectPtr<UAlsMantlingSettings> Settings;

	UPROPERTY(Transient)
	int32 MantlingRootMotionSourceId;
};
