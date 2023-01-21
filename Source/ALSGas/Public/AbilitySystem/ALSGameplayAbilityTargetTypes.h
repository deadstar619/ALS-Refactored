// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Abilities/GameplayAbilityTargetTypes.h"
#include "Settings/AlsMantlingSettings.h"
#include "ALSGameplayAbilityTargetTypes.generated.h"

class FArchive;
struct FGameplayEffectContextHandle;


USTRUCT(BlueprintType)
struct  FALSGameplayAbilityTargetData_Mantle : public FGameplayAbilityTargetData
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "ALS")
	TWeakObjectPtr<UPrimitiveComponent> TargetPrimitive;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "ALS")
	FVector_NetQuantize100 TargetRelativeLocation{ForceInit};

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "ALS")
	FRotator TargetRelativeRotation{ForceInit};

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "ALS")
	float MantlingHeight{0.0f};

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "ALS")
	EAlsMantlingType MantlingType{EAlsMantlingType::High};
	
	bool IsValid() const
	{
		return TargetPrimitive.IsValid();
	}

	// Serialized for replication
	bool NetSerialize(FArchive& Ar, class UPackageMap* Map, bool& bOutSuccess);

	virtual UScriptStruct* GetScriptStruct() const override
	{
		return FALSGameplayAbilityTargetData_Mantle::StaticStruct();
	}
	
	virtual FString ToString() const override
	{
		return FString::Printf(TEXT("TargetPrimitive: %s, TargetRelativeLocation: %s, TargetRelativeRotation: %s, MantlingHeight: %f, MantlingType: %s"),
			*TargetPrimitive->GetName(), *TargetRelativeLocation.ToString(), *TargetRelativeRotation.ToString(), MantlingHeight, *UEnum::GetValueAsString(MantlingType));
	}

};

template<>
struct TStructOpsTypeTraits<FALSGameplayAbilityTargetData_Mantle> : public TStructOpsTypeTraitsBase2<FALSGameplayAbilityTargetData_Mantle>
{
	enum
	{
		WithNetSerializer = true	// For now this is REQUIRED for FGameplayAbilityTargetDataHandle net serialization to work
	};
};