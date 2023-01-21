// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Engine/AssetManager.h"
#include "ALSAssetManager.generated.h"

/**
 * 
 */
UCLASS()
class ALSGAS_API UALSAssetManager : public UAssetManager
{
	GENERATED_BODY()

public:
	// Returns the AssetManager singleton object.
	static UALSAssetManager& Get();

	virtual void StartInitialLoading() override;
};
