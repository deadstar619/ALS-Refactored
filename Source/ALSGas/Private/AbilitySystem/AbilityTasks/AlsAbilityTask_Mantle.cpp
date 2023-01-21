// Fill out your copyright notice in the Description page of Project Settings.


#include "AbilitySystem/AbilityTasks/AlsAbilityTask_Mantle.h"

#include "AlsCharacter.h"
#include "AlsCharacterMovementComponent.h"
#include "Abilities/Tasks/AbilityTask_PlayMontageAndWait.h"
#include "Curves/CurveVector.h"
#include "GameFramework/Character.h"
#include "Net/UnrealNetwork.h"
#include "RootMotionSources/AlsRootMotionSource_Mantling.h"
#include "Utility/AlsMacros.h"

UAlsAbilityTask_Mantle::UAlsAbilityTask_Mantle(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer), AlsCharacter(nullptr), Settings(nullptr), MantlingRootMotionSourceId(0)
{
	bTickingTask = true;
	bSimulatedTask = false;
}

/*
void UAlsAbilityTask_Mantle::GetLifetimeReplicatedProps(TArray< FLifetimeProperty > & OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);
	
	DOREPLIFETIME(ThisClass, Parameters);
}
*/

UAlsAbilityTask_Mantle* UAlsAbilityTask_Mantle::Mantle(UGameplayAbility* OwningAbility, FName TaskInstanceName, const FAlsMantlingParameters& Parameters, UAlsMantlingSettings* Settings)
{
	if (!IsValid(Settings) || !IsValid(OwningAbility) || !IsValid(OwningAbility->GetAvatarActorFromActorInfo()))
	{
		return nullptr;
	}
	
	UAlsAbilityTask_Mantle* MyObj = NewAbilityTask<UAlsAbilityTask_Mantle>(OwningAbility, TaskInstanceName);
	MyObj->Parameters = Parameters;
	MyObj->Settings = Settings;

	return MyObj;
}

void UAlsAbilityTask_Mantle::TickTask(float DeltaTime)
{
	Super::TickTask(DeltaTime);
	
	RefreshMantling();
}

void UAlsAbilityTask_Mantle::Activate()
{
	AlsCharacter = Cast<AAlsCharacter>(GetAvatarActor());
	if (!AlsCharacter.IsValid())
	{
		EndTask();
		return;
	}
	
	UAlsCharacterMovementComponent* CharMoveComp = Cast<UAlsCharacterMovementComponent>(AlsCharacter->GetMovementComponent());
	if (!CharMoveComp)
	{
		EndTask();
		return;
	}
	
	// SelectMantling Settings comes from the ability now

	if (!ALS_ENSURE(IsValid(Settings.Get())) ||
	    !ALS_ENSURE(IsValid(Settings->BlendInCurve)) ||
	    !ALS_ENSURE(IsValid(Settings.Get()->InterpolationAndCorrectionAmountsCurve)))
	{
		return;
	}

	const auto StartTime{Settings->CalculateStartTime(Parameters.MantlingHeight)};
	const auto PlayRate{Settings->CalculatePlayRate(Parameters.MantlingHeight)};

	// Calculate mantling duration.

	auto MinTime{0.0f};
	auto MaxTime{0.0f};
	Settings->InterpolationAndCorrectionAmountsCurve->GetTimeRange(MinTime, MaxTime);

	const auto Duration{MaxTime - StartTime};

	// Calculate actor offsets (offsets between actor and target transform).

	const auto bUseRelativeLocation{MovementBaseUtility::UseRelativeLocation(Parameters.TargetPrimitive.Get())};
	const auto TargetRelativeRotation{Parameters.TargetRelativeRotation.GetNormalized()};

	const auto TargetTransform{
		bUseRelativeLocation
			? FTransform{
				TargetRelativeRotation, Parameters.TargetRelativeLocation,
				Parameters.TargetPrimitive->GetComponentScale()
			}.GetRelativeTransformReverse(Parameters.TargetPrimitive->GetComponentTransform())
			: FTransform{TargetRelativeRotation, Parameters.TargetRelativeLocation}
	};

	const auto ActorFeetLocationOffset{CharMoveComp->GetActorFeetLocation() - TargetTransform.GetLocation()};
	const auto ActorRotationOffset{TargetTransform.GetRotation().Inverse() * AlsCharacter->GetActorQuat()};

	// Clear the character movement mode and set the locomotion action to mantling.
	CharMoveComp->SetMovementMode(MOVE_Custom);
	CharMoveComp->SetBase(Parameters.TargetPrimitive.Get());
	CharMoveComp->SetMovementModeLocked(true);

	if (AlsCharacter->GetLocalRole() >= ROLE_Authority)
	{
		CharMoveComp->NetworkSmoothingMode = ENetworkSmoothingMode::Disabled;
		AlsCharacter->GetMesh()->SetRelativeLocationAndRotation(AlsCharacter->GetBaseTranslationOffset(), AlsCharacter->GetBaseRotationOffset());
	}

	// Apply mantling root motion.
	const auto Mantling{MakeShared<FAlsRootMotionSource_Mantling>()};
	Mantling->InstanceName = ANSI_TO_TCHAR(__FUNCTION__);
	Mantling->Duration = Duration / PlayRate;
	Mantling->MantlingSettings = Settings.Get();
	Mantling->TargetPrimitive = bUseRelativeLocation ? Parameters.TargetPrimitive : nullptr;
	Mantling->TargetRelativeLocation = Parameters.TargetRelativeLocation;
	Mantling->TargetRelativeRotation = TargetRelativeRotation;
	Mantling->ActorFeetLocationOffset = ActorFeetLocationOffset;
	Mantling->ActorRotationOffset = ActorRotationOffset.Rotator();
	Mantling->MantlingHeight = Parameters.MantlingHeight;

	MantlingRootMotionSourceId = CharMoveComp->ApplyRootMotionSource(Mantling);

	if (Ability)
	{
		Ability->SetMovementSyncPoint(Mantling->InstanceName);
	}
	
	OnMantleStarted.Broadcast(Parameters, Settings.Get());
}

void UAlsAbilityTask_Mantle::StopMantling()
{
	if (MantlingRootMotionSourceId <= 0)
	{
		return;
	}

	const auto RootMotionSource{AlsCharacter->GetCharacterMovement()->GetRootMotionSourceByID(MantlingRootMotionSourceId)};

	if (RootMotionSource.IsValid() &&
		!RootMotionSource->Status.HasFlag(ERootMotionSourceStatusFlags::Finished) &&
		!RootMotionSource->Status.HasFlag(ERootMotionSourceStatusFlags::MarkedForRemoval))
	{
		RootMotionSource->Status.SetFlag(ERootMotionSourceStatusFlags::MarkedForRemoval);
	}

	MantlingRootMotionSourceId = 0;

	if (AlsCharacter->GetLocalRole() >= ROLE_Authority)
	{
		AlsCharacter->GetCharacterMovement()->NetworkSmoothingMode = ENetworkSmoothingMode::Exponential;
	}

	UAlsCharacterMovementComponent* CharMoveComp = Cast<UAlsCharacterMovementComponent>(AlsCharacter->GetMovementComponent());
	ensure(CharMoveComp);
	CharMoveComp->SetMovementModeLocked(false);
	CharMoveComp->SetMovementMode(MOVE_Walking);

	OnMantleCompleted.Broadcast(Parameters, Settings.Get());
	EndTask();
}

void UAlsAbilityTask_Mantle::OnDestroy(bool AbilityIsEnding)
{
	StopMantling();
	
	Super::OnDestroy(AbilityIsEnding);
}

void UAlsAbilityTask_Mantle::RefreshMantling()
{
	if (!AlsCharacter.IsValid())
	{
		EndTask();
		return;
	}

	if (MantlingRootMotionSourceId <= 0)
	{
		EndTask();
		return;
	}

	const auto RootMotionSource{AlsCharacter->GetCharacterMovement()->GetRootMotionSourceByID(MantlingRootMotionSourceId)};

	if (!RootMotionSource.IsValid() ||
		RootMotionSource->Status.HasFlag(ERootMotionSourceStatusFlags::Finished) ||
		RootMotionSource->Status.HasFlag(ERootMotionSourceStatusFlags::MarkedForRemoval) ||
		// ReSharper disable once CppRedundantParentheses
		(AlsCharacter->GetLocomotionAction().IsValid() && AlsCharacter->GetLocomotionAction() != AlsLocomotionActionTags::Mantling) ||
		AlsCharacter->GetCharacterMovement()->MovementMode != MOVE_Custom)
	{
		StopMantling();
		AlsCharacter->ForceNetUpdate();
	}
}
