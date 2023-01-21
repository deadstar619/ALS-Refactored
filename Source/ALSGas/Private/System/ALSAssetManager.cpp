// Fill out your copyright notice in the Description page of Project Settings.


#include "System/ALSAssetManager.h"

#include "AbilitySystemGlobals.h"

UALSAssetManager& UALSAssetManager::Get()
{
	check(GEngine);

	if (UALSAssetManager* Singleton = Cast<UALSAssetManager>(GEngine->AssetManager))
	{
		return *Singleton;
	}

	UE_LOG(LogTemp, Fatal, TEXT("Invalid AssetManagerClassName in DefaultEngine.ini.  It must be set to ALSAssetManager!"));

	// Fatal error above prevents this from being called.
	return *NewObject<UALSAssetManager>();
}

void UALSAssetManager::StartInitialLoading()
{
	SCOPED_BOOT_TIMING("UALSAssetManager::StartInitialLoading");

	// This does all of the scanning, need to do this now even if loads are deferred
	Super::StartInitialLoading();

	UAbilitySystemGlobals::Get().InitGlobalData();
}