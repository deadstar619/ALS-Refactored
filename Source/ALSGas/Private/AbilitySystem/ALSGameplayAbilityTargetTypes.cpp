// Fill out your copyright notice in the Description page of Project Settings.


#include "AbilitySystem/ALSGameplayAbilityTargetTypes.h"
#include "Engine/NetSerialization.h"

bool FALSGameplayAbilityTargetData_Mantle::NetSerialize(FArchive& Ar, UPackageMap* Map, bool& bOutSuccess)
{
	Ar << TargetPrimitive;
	TargetRelativeLocation.NetSerialize(Ar, Map, bOutSuccess);
	TargetRelativeRotation.NetSerialize(Ar, Map, bOutSuccess);
	Ar << MantlingHeight;
	Ar << MantlingType;
	
	return true;
}
