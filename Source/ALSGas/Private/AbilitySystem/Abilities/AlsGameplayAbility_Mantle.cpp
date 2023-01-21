// Fill out your copyright notice in the Description page of Project Settings.


#include "AbilitySystem/Abilities/AlsGameplayAbility_Mantle.h"

#include "AlsCharacter.h"
#include "Abilities/Tasks/AbilityTask_PlayMontageAndWait.h"
#include "AbilitySystem/ALSGameplayAbilityTargetTypes.h"
#include "AbilitySystem/AbilityTasks/AlsAbilityTask_Mantle.h"
#include "Components/CapsuleComponent.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "Utility/AlsConstants.h"
#include "Utility/AlsMacros.h"
#include "Utility/AlsMath.h"
#include "Utility/AlsUtility.h"


void UAlsGameplayAbility_Mantle::ActivateLocalPlayerAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilityActivationInfo ActivationInfo, const FGameplayEventData* TriggerEventData)
{
	constexpr bool bReplicateEndAbility = false;
	constexpr bool bWasCancelled = true;

	if (!ActorInfo->AvatarActor.IsValid())
	{
		return;
	}
	
	AlsCharacter = Cast<AAlsCharacter>(ActorInfo->AvatarActor);
	if (!AlsCharacter.IsValid())
	{
		UE_LOG(LogTemp, Warning, TEXT("Ability %s expects an AlsCharacter as the AvatarActor to activate."), *GetName());
		EndAbility(Handle, ActorInfo, ActivationInfo, bReplicateEndAbility, bWasCancelled);
	}
	
	FAlsMantlingParameters MantlingParameters;
	if (MakeMantlingParams(MantlingParameters))
	{
		// Make the target data and send it to the server
		FGameplayAbilityTargetDataHandle TargetDataHandle;
		FALSGameplayAbilityTargetData_Mantle* MantleTargetData = new FALSGameplayAbilityTargetData_Mantle();
		MantleTargetData->TargetPrimitive = MantlingParameters.TargetPrimitive;
		MantleTargetData->TargetRelativeLocation = MantlingParameters.TargetRelativeLocation;
		MantleTargetData->TargetRelativeRotation = MantlingParameters.TargetRelativeRotation;
		MantleTargetData->MantlingHeight = MantlingParameters.MantlingHeight;
		MantleTargetData->MantlingType = MantlingParameters.MantlingType;

		TargetDataHandle.Add(MantleTargetData);

		// Notify self (local client) *AND* server that TargetData is ready to be processed
		NotifyTargetDataReady(TargetDataHandle, FGameplayTag()); // send with a gameplay tag, or empty*/
		return;
	}

	UE_LOG(LogTemp, Warning, TEXT("Ability %s did not get valid target data locally and could not activate."), *GetName());
	EndAbility(Handle, ActorInfo, ActivationInfo, bReplicateEndAbility, bWasCancelled);
}

void UAlsGameplayAbility_Mantle::ActivateAbilityWithTargetData(const FGameplayAbilityTargetDataHandle& TargetDataHandle, FGameplayTag ApplicationTag)
{
	// retrieve data
	const FGameplayAbilityTargetData* TargetData = TargetDataHandle.Get(0);
	if (!TargetData)
	{
		// client sent us bad data
		CancelAbility(CurrentSpecHandle, CurrentActorInfo, CurrentActivationInfo, true);
		return;
	}

	// Cast to our custom target data type
	const FALSGameplayAbilityTargetData_Mantle* MantleTargetData = static_cast<const FALSGameplayAbilityTargetData_Mantle*>(TargetData);
	check(MantleTargetData);

	// Server: Validate data
	const bool bIsServer = CurrentActorInfo->IsNetAuthority();
	if (bIsServer)
	{
		if (!MantleTargetData->IsValid())
		{
			CancelAbility(CurrentSpecHandle, CurrentActorInfo, CurrentActivationInfo, true);
			return;
		}

		// Do the check on the server to make sure the mantle is valid
		FAlsMantlingParameters MantlingParameters;
		if (MakeMantlingParams(MantlingParameters))
		{
			constexpr float ClientServerTollerance = 0.01f;
						
			const bool bTargetPrimitiveMatch = MantleTargetData->TargetPrimitive == MantlingParameters.TargetPrimitive;
			const bool bTargetRelativeLocationMatch = MantleTargetData->TargetRelativeLocation.Equals(MantlingParameters.TargetRelativeLocation, ClientServerTollerance);
			const bool bTargetRelativeRotationMatch = MantleTargetData->TargetRelativeRotation.Equals(MantlingParameters.TargetRelativeRotation, ClientServerTollerance);
			const bool bMantlingHeightMatch = FMath::IsNearlyEqual(MantleTargetData->MantlingHeight ,MantlingParameters.MantlingHeight, ClientServerTollerance);
			const bool bMantlingTypeMatch = MantleTargetData->MantlingType == MantlingParameters.MantlingType;
			
			UE_LOG(LogTemp, Warning, TEXT("TargetPrimitive mismatch: %s vs %s"), *MantleTargetData->TargetPrimitive->GetName(), *MantlingParameters.TargetPrimitive->GetName());
			UE_LOG(LogTemp, Warning, TEXT("TargetRelativeLocation mismatch: %s vs %s"), *MantleTargetData->TargetRelativeLocation.ToString(), *MantlingParameters.TargetRelativeLocation.ToString());
			UE_LOG(LogTemp, Warning, TEXT("TargetRelativeRotation mismatch: %s vs %s"), *MantleTargetData->TargetRelativeRotation.ToString(), *MantlingParameters.TargetRelativeRotation.ToString());
			UE_LOG(LogTemp, Warning, TEXT("MantlingHeight mismatch: %f vs %f"), MantleTargetData->MantlingHeight, MantlingParameters.MantlingHeight);
			UE_LOG(LogTemp, Warning, TEXT("MantlingType mismatch: %d vs %d"), MantleTargetData->MantlingType, MantlingParameters.MantlingType);

			// Check if all the values match			
			const bool bCanStartMantle = bTargetPrimitiveMatch && bTargetRelativeLocationMatch && bTargetRelativeRotationMatch && bMantlingHeightMatch && bMantlingTypeMatch;

			if(!bCanStartMantle)
			{
				CancelAbility(CurrentSpecHandle, CurrentActorInfo, CurrentActivationInfo, true);
				return;
			}
		}
	}

	//////////////////////////////////////////////////////////////////////
	// Client & Server both -- data is valid, activate the ability with it
	//////////////////////////////////////////////////////////////////////

	// Activate task
	UE_LOG(LogTemp, Warning, TEXT("Mantle data validated, activating task"));
		
	UE_LOG(LogTemp, Display, TEXT("Mantle Data for %s: %s"), bIsServer ? TEXT("server") : TEXT("client"), *MantleTargetData->ToString());
	
	UAlsMantlingSettings* LocMantlingSettings = SelectMantlingSettings(MantleTargetData->MantlingType);
	if (!LocMantlingSettings)
	{
		UE_LOG(LogTemp, Warning, TEXT("Ability %s could not find a valid MantlingSettings for MantlingType %d."), *GetName(), MantleTargetData->MantlingType);
		CancelAbility(CurrentSpecHandle, CurrentActorInfo, CurrentActivationInfo, true);
		return;
	}

	if (AlsCharacter->GetLocalRole() < ROLE_Authority)
	{
		AlsCharacter->GetCharacterMovement()->FlushServerMoves();
	}
	
	UAlsAbilityTask_Mantle* Task = UAlsAbilityTask_Mantle::Mantle(this, "MantleTask", MantleTargetData->GetMantlingParameters(), LocMantlingSettings);
	Task->OnMantleStarted.AddDynamic(this, &UAlsGameplayAbility_Mantle::OnMantleStart);
	Task->OnMantleCompleted.AddDynamic(this, &UAlsGameplayAbility_Mantle::OnMantleEnd);
	
	UAbilityTask_PlayMontageAndWait* MontageTask = nullptr;
	
	if (ALS_ENSURE(IsValid(LocMantlingSettings->Montage)))
	{
		const auto StartTime{LocMantlingSettings->CalculateStartTime(MantleTargetData->GetMantlingParameters().MantlingHeight)};
		const auto PlayRate{LocMantlingSettings->CalculatePlayRate(MantleTargetData->GetMantlingParameters().MantlingHeight)};

		const auto MontageStartTime{
			MantleTargetData->GetMantlingParameters().MantlingType == EAlsMantlingType::InAir && IsLocallyControlled()
				? StartTime - FMath::GetMappedRangeValueClamped(
					  FVector2f{LocMantlingSettings->ReferenceHeight}, {GetWorld()->GetDeltaSeconds(), 0.0f}, MantleTargetData->GetMantlingParameters().MantlingHeight)
				: StartTime
		};
		
		// Spawn and activate the AbilityTask_PlayMontageAndWaitForEvent
		MontageTask = UAbilityTask_PlayMontageAndWait::CreatePlayMontageAndWaitProxy(this, "MontageTask",
			LocMantlingSettings->Montage, PlayRate,NAME_None, true, 1, MontageStartTime);
		
		AlsCharacter->SetLocomotionAction(AlsLocomotionActionTags::Mantling);
	}

	Task->ReadyForActivation();
	
	if (IsValid(MontageTask))
	{
		MontageTask->ReadyForActivation();
	}
}

void UAlsGameplayAbility_Mantle::OnMantleStart(const FAlsMantlingParameters& MantlingParameters, const UAlsMantlingSettings* MantlingSettings)
{	
	K2_OnMantleStart(MantlingParameters, MantlingSettings);
}

void UAlsGameplayAbility_Mantle::OnMantleEnd(const FAlsMantlingParameters& MantlingParameters, const UAlsMantlingSettings* MantlingSettings)
{
	K2_OnMantleEnd(MantlingParameters, MantlingSettings);
	EndAbility(CurrentSpecHandle, CurrentActorInfo, CurrentActivationInfo, false, false);
}

bool UAlsGameplayAbility_Mantle::MakeMantlingParams(FAlsMantlingParameters& OutMantlingParams)
{
	if (!AlsCharacter.IsValid())
	{
		return false;
	}
	
	// Find the state we're currently in and try start the mantling process
	FAlsMantlingTraceSettings TraceSettings;

	if (AlsCharacter->GetLocomotionMode() == AlsLocomotionModeTags::InAir)
	{
		TraceSettings = Settings.InAirTrace;
	
	}
	else if (AlsCharacter->GetLocomotionMode() == AlsLocomotionModeTags::Grounded)
	{
		TraceSettings = Settings.GroundedTrace;
	}

	if (!Settings.bAllowMantling || AlsCharacter->GetLocalRole() <= ROLE_SimulatedProxy/* || !IsMantlingAllowedToStart()*/)
	{
		return false;
	}

	const auto ActorLocation{AlsCharacter->GetActorLocation()};
	const auto ActorYawAngle{UE_REAL_TO_FLOAT(FRotator::NormalizeAxis(AlsCharacter->GetActorRotation().Yaw))};

	float ForwardTraceAngle;
	if (AlsCharacter->GetLocomotionState().bHasSpeed)
	{
		ForwardTraceAngle = AlsCharacter->GetLocomotionState().bHasInput
			                    ? AlsCharacter->GetLocomotionState().VelocityYawAngle +
			                      FMath::ClampAngle(AlsCharacter->GetLocomotionState().InputYawAngle - AlsCharacter->GetLocomotionState().VelocityYawAngle,
			                                        -Settings.MaxReachAngle, Settings.MaxReachAngle)
			                    : AlsCharacter->GetLocomotionState().VelocityYawAngle;
	}
	else
	{
		ForwardTraceAngle = AlsCharacter->GetLocomotionState().bHasInput ? AlsCharacter->GetLocomotionState().InputYawAngle : ActorYawAngle;
	}

	const auto ForwardTraceDeltaAngle{FRotator3f::NormalizeAxis(ForwardTraceAngle - ActorYawAngle)};
	if (FMath::Abs(ForwardTraceDeltaAngle) > Settings.TraceAngleThreshold)
	{
		return false;
	}

	FCollisionObjectQueryParams ObjectQueryParameters;
	for (const auto ObjectType : Settings.MantlingTraceObjectTypes)
	{
		ObjectQueryParameters.AddObjectTypesToQuery(UCollisionProfile::Get()->ConvertToCollisionChannel(false, ObjectType));
	}

	const auto ForwardTraceDirection{
		UAlsMath::AngleToDirectionXY(
			ActorYawAngle + FMath::ClampAngle(ForwardTraceDeltaAngle, -Settings.MaxReachAngle, Settings.MaxReachAngle))
	};

#if ENABLE_DRAW_DEBUG
	const auto bDisplayDebug{UAlsUtility::ShouldDisplayDebug(AlsCharacter.Get(), UAlsConstants::MantlingDisplayName())};
#endif

	const auto* Capsule{AlsCharacter->GetCapsuleComponent()};

	const auto CapsuleScale{Capsule->GetComponentScale().Z};
	const auto CapsuleRadius{Capsule->GetScaledCapsuleRadius()};
	const auto CapsuleHalfHeight{Capsule->GetScaledCapsuleHalfHeight()};

	const FVector CapsuleBottomLocation{ActorLocation.X, ActorLocation.Y, ActorLocation.Z - CapsuleHalfHeight};

	const auto TraceCapsuleRadius{CapsuleRadius - 1.0f};

	const auto LedgeHeightDelta{UE_REAL_TO_FLOAT((TraceSettings.LedgeHeight.GetMax() - TraceSettings.LedgeHeight.GetMin()) * CapsuleScale)};

	// Trace forward to find an object the character cannot walk on.

	static const FName ForwardTraceTag{FString::Format(TEXT("{0} (Forward Trace)"), {ANSI_TO_TCHAR(__FUNCTION__)})};

	auto ForwardTraceStart{CapsuleBottomLocation - ForwardTraceDirection * CapsuleRadius};
	ForwardTraceStart.Z += (TraceSettings.LedgeHeight.X + TraceSettings.LedgeHeight.Y) *
		0.5f * CapsuleScale - UCharacterMovementComponent::MAX_FLOOR_DIST;

	auto ForwardTraceEnd{ForwardTraceStart + ForwardTraceDirection * (CapsuleRadius + (TraceSettings.ReachDistance + 1.0f) * CapsuleScale)};

	const auto ForwardTraceCapsuleHalfHeight{LedgeHeightDelta * 0.5f};

	FHitResult ForwardTraceHit;
	GetWorld()->SweepSingleByObjectType(ForwardTraceHit, ForwardTraceStart, ForwardTraceEnd, FQuat::Identity, ObjectQueryParameters,
	                                    FCollisionShape::MakeCapsule(TraceCapsuleRadius, ForwardTraceCapsuleHalfHeight),
	                                    {ForwardTraceTag, false, AlsCharacter.Get()});

	auto* TargetPrimitive{ForwardTraceHit.GetComponent()};

	if (!ForwardTraceHit.IsValidBlockingHit() ||
	    !IsValid(TargetPrimitive) ||
	    TargetPrimitive->GetComponentVelocity().SizeSquared() > FMath::Square(Settings.TargetPrimitiveSpeedThreshold) ||
	    !TargetPrimitive->CanCharacterStepUp(AlsCharacter.Get()) ||
	    AlsCharacter->GetCharacterMovement()->IsWalkable(ForwardTraceHit))
	{
#if ENABLE_DRAW_DEBUG
		if (bDisplayDebug)
		{
			UAlsUtility::DrawDebugSweepSingleCapsuleAlternative(GetWorld(), ForwardTraceStart, ForwardTraceEnd, TraceCapsuleRadius,
			                                                    ForwardTraceCapsuleHalfHeight, false, ForwardTraceHit, {0.0f, 0.25f, 1.0f},
			                                                    {0.0f, 0.75f, 1.0f}, TraceSettings.bDrawFailedTraces ? 5.0f : 0.0f);
		}
#endif

		return false;
	}

	// Trace downward from the first trace's impact point and determine if the hit location is walkable.

	static const FName DownwardTraceTag{FString::Format(TEXT("{0} (Downward Trace)"), {ANSI_TO_TCHAR(__FUNCTION__)})};

	const auto TargetLocationOffset{
		FVector2D{ForwardTraceHit.ImpactNormal.GetSafeNormal2D()} * (TraceSettings.TargetLocationOffset * CapsuleScale)
	};

	const FVector DownwardTraceStart{
		ForwardTraceHit.ImpactPoint.X - TargetLocationOffset.X,
		ForwardTraceHit.ImpactPoint.Y - TargetLocationOffset.Y,
		CapsuleBottomLocation.Z + LedgeHeightDelta + 2.5f * TraceCapsuleRadius + UCharacterMovementComponent::MIN_FLOOR_DIST
	};

	const FVector DownwardTraceEnd{
		DownwardTraceStart.X,
		DownwardTraceStart.Y,
		CapsuleBottomLocation.Z +
		TraceSettings.LedgeHeight.GetMin() * CapsuleScale + TraceCapsuleRadius - UCharacterMovementComponent::MAX_FLOOR_DIST
	};

	FHitResult DownwardTraceHit;
	GetWorld()->SweepSingleByObjectType(DownwardTraceHit, DownwardTraceStart, DownwardTraceEnd, FQuat::Identity,
	                                    ObjectQueryParameters, FCollisionShape::MakeSphere(TraceCapsuleRadius),
	                                    {DownwardTraceTag, false, AlsCharacter.Get()});

	if (!AlsCharacter->GetCharacterMovement()->IsWalkable(DownwardTraceHit))
	{
#if ENABLE_DRAW_DEBUG
		if (bDisplayDebug)
		{
			UAlsUtility::DrawDebugSweepSingleCapsuleAlternative(GetWorld(), ForwardTraceStart, ForwardTraceEnd, TraceCapsuleRadius,
			                                                    ForwardTraceCapsuleHalfHeight, true, ForwardTraceHit, {0.0f, 0.25f, 1.0f},
			                                                    {0.0f, 0.75f, 1.0f}, TraceSettings.bDrawFailedTraces ? 5.0f : 0.0f);

			UAlsUtility::DrawDebugSweepSingleSphere(GetWorld(), DownwardTraceStart, DownwardTraceEnd, TraceCapsuleRadius,
			                                        false, DownwardTraceHit, {0.25f, 0.0f, 1.0f}, {0.75f, 0.0f, 1.0f},
			                                        TraceSettings.bDrawFailedTraces ? 7.5f : 0.0f);
		}
#endif

		return false;
	}

	// Check if the capsule has room to stand at the downward trace's location. If so,
	// set that location as the target transform and calculate the mantling height.

	static const FName FreeSpaceTraceTag{FString::Format(TEXT("{0} (Free Space Overlap)"), {ANSI_TO_TCHAR(__FUNCTION__)})};

	const FVector TargetLocation{
		DownwardTraceHit.ImpactPoint.X,
		DownwardTraceHit.ImpactPoint.Y,
		DownwardTraceHit.ImpactPoint.Z + UCharacterMovementComponent::MIN_FLOOR_DIST
	};

	const FVector TargetCapsuleLocation{TargetLocation.X, TargetLocation.Y, TargetLocation.Z + CapsuleHalfHeight};

	if (GetWorld()->OverlapAnyTestByObjectType(TargetCapsuleLocation, FQuat::Identity, ObjectQueryParameters,
	                                           FCollisionShape::MakeCapsule(CapsuleRadius, CapsuleHalfHeight),
	                                           {FreeSpaceTraceTag, false, AlsCharacter.Get()}))
	{
#if ENABLE_DRAW_DEBUG
		if (bDisplayDebug)
		{
			UAlsUtility::DrawDebugSweepSingleCapsuleAlternative(GetWorld(), ForwardTraceStart, ForwardTraceEnd, TraceCapsuleRadius,
			                                                    ForwardTraceCapsuleHalfHeight, true, ForwardTraceHit, {0.0f, 0.25f, 1.0f},
			                                                    {0.0f, 0.75f, 1.0f}, TraceSettings.bDrawFailedTraces ? 5.0f : 0.0f);

			UAlsUtility::DrawDebugSweepSingleSphere(GetWorld(), DownwardTraceStart, DownwardTraceEnd, TraceCapsuleRadius,
			                                        false, DownwardTraceHit, {0.25f, 0.0f, 1.0f}, {0.75f, 0.0f, 1.0f},
			                                        TraceSettings.bDrawFailedTraces ? 7.5f : 0.0f);

			DrawDebugCapsule(GetWorld(), TargetCapsuleLocation, CapsuleHalfHeight, CapsuleRadius, FQuat::Identity,
			                 FColor::Red, false, TraceSettings.bDrawFailedTraces ? 10.0f : 0.0f);
		}
#endif

		return false;
	}

#if ENABLE_DRAW_DEBUG
	if (bDisplayDebug)
	{
		UAlsUtility::DrawDebugSweepSingleCapsuleAlternative(GetWorld(), ForwardTraceStart, ForwardTraceEnd, TraceCapsuleRadius,
		                                                    ForwardTraceCapsuleHalfHeight, true, ForwardTraceHit,
		                                                    {0.0f, 0.25f, 1.0f}, {0.0f, 0.75f, 1.0f}, 5.0f);

		UAlsUtility::DrawDebugSweepSingleSphere(GetWorld(), DownwardTraceStart, DownwardTraceEnd,
		                                        TraceCapsuleRadius, true, DownwardTraceHit,
		                                        {0.25f, 0.0f, 1.0f}, {0.75f, 0.0f, 1.0f}, 7.5f);
	}
#endif

	const auto TargetRotation{(-ForwardTraceHit.ImpactNormal.GetSafeNormal2D()).ToOrientationQuat()};

	FAlsMantlingParameters Parameters;

	Parameters.TargetPrimitive = TargetPrimitive;
	Parameters.MantlingHeight = UE_REAL_TO_FLOAT((TargetLocation.Z - CapsuleBottomLocation.Z) / CapsuleScale);

	// Determine the mantling type by checking the movement mode and mantling height.

	Parameters.MantlingType = AlsCharacter->GetLocomotionMode() != AlsLocomotionModeTags::Grounded
		                          ? EAlsMantlingType::InAir
		                          : Parameters.MantlingHeight > Settings.MantlingHighHeightThreshold
		                          ? EAlsMantlingType::High
		                          : EAlsMantlingType::Low;

	// If the target primitive can't move, then use world coordinates to save
	// some performance by skipping some coordinate space transformations later.

	if (MovementBaseUtility::UseRelativeLocation(TargetPrimitive))
	{
		const auto TargetRelativeTransform{
			TargetPrimitive->GetComponentTransform().GetRelativeTransform({TargetRotation, TargetLocation})
		};

		Parameters.TargetRelativeLocation = TargetRelativeTransform.GetLocation();
		Parameters.TargetRelativeRotation = TargetRelativeTransform.Rotator();
	}
	else
	{
		Parameters.TargetRelativeLocation = TargetLocation;
		Parameters.TargetRelativeRotation = TargetRotation.Rotator();
	}

	// We finally have our data.
	OutMantlingParams = Parameters;

	return true;
}

UAlsMantlingSettings* UAlsGameplayAbility_Mantle::SelectMantlingSettings_Implementation(EAlsMantlingType MantlingType)
{
	// Select the appropriate mantling settings based on the mantling type.
	if (!AlsCharacter.IsValid())
	{
		return nullptr;
	}

	switch (MantlingType)
	{
	case EAlsMantlingType::Low:
		{
			if (LowMantleSettings.Find(AlsCharacter->GetOverlayMode()) && AlsCharacter->GetOverlayMode().IsValid())
			{
				return LowMantleSettings[AlsCharacter->GetOverlayMode()];
			}
			return nullptr;
		}
	case EAlsMantlingType::High:
		return HighMantleSettings;
	case EAlsMantlingType::InAir:
		return InAirMantleSettings;
	default:
		return nullptr;
	}
}
