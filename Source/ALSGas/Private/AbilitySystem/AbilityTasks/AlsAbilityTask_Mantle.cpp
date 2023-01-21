// Fill out your copyright notice in the Description page of Project Settings.


#include "AbilitySystem/AbilityTasks/AlsAbilityTask_Mantle.h"

#include "AlsCharacter.h"
#include "AlsCharacterMovementComponent.h"
#include "Curves/CurveVector.h"
#include "GameFramework/Character.h"
#include "Net/UnrealNetwork.h"
#include "RootMotionSources/AlsRootMotionSource_Mantling.h"
#include "Utility/AlsMacros.h"

UAlsAbilityTask_Mantle::UAlsAbilityTask_Mantle(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer), Settings(nullptr), MantlingRootMotionSourceId(0)
{
	bTickingTask = true;
	bSimulatedTask = true;
	bIsFinished = false;
}

void UAlsAbilityTask_Mantle::InitSimulatedTask(UGameplayTasksComponent& InGameplayTasksComponent)
{
	Super::InitSimulatedTask(InGameplayTasksComponent);
}

void UAlsAbilityTask_Mantle::GetLifetimeReplicatedProps(TArray< FLifetimeProperty > & OutLifetimeProps) const
{
	DOREPLIFETIME(ThisClass, Parameters);
	DOREPLIFETIME(ThisClass, Settings);
}

UAlsAbilityTask_Mantle* UAlsAbilityTask_Mantle::Mantle(UGameplayAbility* OwningAbility, FName TaskInstanceName, const FAlsMantlingParameters& Parameters, UAlsMantlingSettings* Settings)
{
	if (!IsValid(Settings))
	{
		return nullptr;
	}
	
	UAlsAbilityTask_Mantle* MyObj = NewAbilityTask<UAlsAbilityTask_Mantle>(OwningAbility, TaskInstanceName);
	MyObj->Parameters = Parameters;
	MyObj->Settings = Settings;

	return MyObj;
}

void UAlsAbilityTask_Mantle::Activate()
{
	AActor* MyActor = GetAvatarActor();
	if (MyActor)
	{
		return;
	}

	AAlsCharacter* AlsCharacter = Cast<AAlsCharacter>(MyActor);
	if (AlsCharacter)
	{
		return;
	}
	
	UAlsCharacterMovementComponent* CharMoveComp = Cast<UAlsCharacterMovementComponent>(AlsCharacter->GetMovementComponent());
	if (CharMoveComp)
	{
		return;
	}
	// SelectMantling Settings will be in the ability
//	auto* MantlingSettings{SelectMantlingSettings(Parameters.MantlingType)};

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

	// Play the animation montage if valid.

	if (ALS_ENSURE(IsValid(Settings->Montage)))
	{
		// TODO Magic. I can't explain why, but this code fixes animation and root motion source desynchronization.

		const auto MontageStartTime{
			Parameters.MantlingType == EAlsMantlingType::InAir && IsLocallyControlled()
				? StartTime - FMath::GetMappedRangeValueClamped(
					  FVector2f{Settings->ReferenceHeight}, {GetWorld()->GetDeltaSeconds(), 0.0f}, Parameters.MantlingHeight)
				: StartTime
		};

		if (AlsCharacter->GetMesh()->GetAnimInstance()->Montage_Play(Settings->Montage, PlayRate,
		                                               EMontagePlayReturnType::MontageLength,
		                                               MontageStartTime, false))
		{
			AlsCharacter->SetLocomotionAction(AlsLocomotionActionTags::Mantling);
		}
	}

	OnMantleStarted.Broadcast(Parameters);
}

void UAlsAbilityTask_Mantle::TickTask(float DeltaTime)
{
	Super::TickTask(DeltaTime);
}

void UAlsAbilityTask_Mantle::OnDestroy(bool AbilityIsEnding)
{
	Super::OnDestroy(AbilityIsEnding);
}
