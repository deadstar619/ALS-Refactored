// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Abilities/Tasks/AbilityTask.h"
#include "Settings/AlsMantlingSettings.h"
#include "AlsAbilityTask_Mantle.generated.h"

DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FAlsMantleDelegate, const FAlsMantlingParameters&, MantlingParameters);

UCLASS()
class ALSGAS_API UAlsAbilityTask_Mantle : public UAbilityTask
{
	GENERATED_UCLASS_BODY()

	UPROPERTY(BlueprintAssignable)
	FAlsMantleDelegate OnMantleStarted;
	
	UPROPERTY(BlueprintAssignable)
	FAlsMantleDelegate OnMantleCompleted;

	virtual void InitSimulatedTask(UGameplayTasksComponent& InGameplayTasksComponent) override;

	/** Mantles with the given parameters */
	UFUNCTION(BlueprintCallable, Category = "Ability|Tasks", meta = (HidePin = "OwningAbility", DefaultToSelf = "OwningAbility", BlueprintInternalUseOnly = "TRUE"))
	static UAlsAbilityTask_Mantle* Mantle(UGameplayAbility* OwningAbility, FName TaskInstanceName, const FAlsMantlingParameters& Parameters, UAlsMantlingSettings* Settings);

	virtual void Activate() override;

	/** Tick function for this task, if bTickingTask == true */
	virtual void TickTask(float DeltaTime) override;

	virtual void OnDestroy(bool AbilityIsEnding) override;

protected:
	bool bIsFinished;

	UPROPERTY(Replicated)
	FAlsMantlingParameters Parameters;

	UPROPERTY(Replicated, Transient)
	TWeakObjectPtr<UAlsMantlingSettings> Settings;

	UPROPERTY(Transient)
	int32 MantlingRootMotionSourceId;
};
