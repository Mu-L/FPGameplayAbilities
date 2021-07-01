// Fill out your copyright notice in the Description page of Project Settings.

#include "AbilitySystem/Tasks/FPGAAbilityTask_PlayMontageNotifyAccurate.h"
#include "GameFramework/Character.h"
#include "AbilitySystemComponent.h"
#include "AbilitySystemGlobals.h"

UFPGAAbilityTask_PlayMontageNotifyAccurate::UFPGAAbilityTask_PlayMontageNotifyAccurate(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
	Rate = 1.f;
	bStopWhenAbilityEnds = true;
}

void UFPGAAbilityTask_PlayMontageNotifyAccurate::OnMontageBlendingOut(UAnimMontage* Montage, bool bInterrupted)
{
	if (Ability && Ability->GetCurrentMontage() == MontageToPlay)
	{
		if (Montage == MontageToPlay)
		{
			AbilitySystemComponent->ClearAnimatingAbility(Ability);

			// Reset AnimRootMotionTranslationScale
			ACharacter* Character = Cast<ACharacter>(GetAvatarActor());
			if (Character && (Character->GetLocalRole() == ROLE_Authority ||
				(Character->GetLocalRole() == ROLE_AutonomousProxy && Ability->GetNetExecutionPolicy() == EGameplayAbilityNetExecutionPolicy::LocalPredicted)))
			{
				Character->SetAnimRootMotionTranslationScale(1.f);
			}
		}
	}

	if (bInterrupted)
	{
		if (ShouldBroadcastAbilityTaskDelegates())
		{
			OnInterrupted.Broadcast(NAME_None);
		}
	}
	else
	{
		if (ShouldBroadcastAbilityTaskDelegates())
		{
			OnBlendOut.Broadcast(NAME_None);
		}
	}
}

void UFPGAAbilityTask_PlayMontageNotifyAccurate::OnMontageInterrupted()
{
	if (StopPlayingMontage())
	{
		// Let the BP handle the interrupt as well
		if (ShouldBroadcastAbilityTaskDelegates())
		{
			OnInterrupted.Broadcast(NAME_None);
		}
	}
}

void UFPGAAbilityTask_PlayMontageNotifyAccurate::OnMontageEnded(UAnimMontage* Montage, bool bInterrupted)
{
	UnbindDelegates();
	
	if (!bInterrupted)
	{
		if (ShouldBroadcastAbilityTaskDelegates())
		{
			OnCompleted.Broadcast(NAME_None);
		}
	}

	EndTask();
}

UFPGAAbilityTask_PlayMontageNotifyAccurate* UFPGAAbilityTask_PlayMontageNotifyAccurate::CreatePlayMontageAndWaitProxy(
	UGameplayAbility* OwningAbility,
	FName TaskInstanceName,
	UAnimMontage* MontageToPlay,
	float Rate,
	FName StartSection,
	bool bStopWhenAbilityEnds,
	bool bEndOnCancelInput,
	float AnimRootMotionTranslationScale)
{
	UAbilitySystemGlobals::NonShipping_ApplyGlobalAbilityScaler_Rate(Rate);

	UFPGAAbilityTask_PlayMontageNotifyAccurate* MyObj = NewAbilityTask<UFPGAAbilityTask_PlayMontageNotifyAccurate>(OwningAbility, TaskInstanceName);
	MyObj->MontageToPlay = MontageToPlay;
	MyObj->Rate = Rate;
	MyObj->StartSection = StartSection;
	MyObj->AnimRootMotionTranslationScale = AnimRootMotionTranslationScale;
	// MyObj->bEndOnCancelInput = bEndOnCancelInput;
	MyObj->bStopWhenAbilityEnds = bStopWhenAbilityEnds;

	MyObj->NotifyDelay = (MontageToPlay->Notifies.Num() > 0 ? MontageToPlay->Notifies[0].GetTriggerTime() : 1.f) / Rate;
	MyObj->MontageToPlay->GetPlayLength();

	return MyObj;
}

void UFPGAAbilityTask_PlayMontageNotifyAccurate::TickTask(float DeltaTime)
{
	Super::TickTask(DeltaTime);
}

void UFPGAAbilityTask_PlayMontageNotifyAccurate::Activate()
{
	Super::Activate();

	GetWorld()->GetTimerManager().SetTimer(TimerHandle, this, &UFPGAAbilityTask_PlayMontageNotifyAccurate::SendNotify, NotifyDelay);

	if (Ability == nullptr)
	{
		return;
	}

	UE_LOG(LogTemp, Warning, TEXT("Notify delay %f"), NotifyDelay);

	bool bPlayedMontage = false;

	if (AbilitySystemComponent && MontageToPlay)
	{
		const FGameplayAbilityActorInfo* ActorInfo = Ability->GetCurrentActorInfo();
		UAnimInstance* AnimInstance = ActorInfo->GetAnimInstance();
		if (AnimInstance != nullptr)
		{
			const float NewSequenceTime = MontageToPlay->SequenceLength / Rate;

			if (AbilitySystemComponent->PlayMontage(Ability, Ability->GetCurrentActivationInfo(), MontageToPlay, NewSequenceTime, StartSection) > 0.f)
			{
				// Playing a montage could potentially fire off a callback into game code which could kill this ability! Early out if we are  pending kill.
				if (ShouldBroadcastAbilityTaskDelegates() == false)
				{
					return;
				}

				AnimInstancePtr = AnimInstance;

				InterruptedHandle = Ability->OnGameplayAbilityCancelled.AddUObject(this, &UFPGAAbilityTask_PlayMontageNotifyAccurate::OnMontageInterrupted);

				BlendingOutDelegate.BindUObject(this, &UFPGAAbilityTask_PlayMontageNotifyAccurate::OnMontageBlendingOut);
				AnimInstance->Montage_SetBlendingOutDelegate(BlendingOutDelegate, MontageToPlay);

				MontageEndedDelegate.BindUObject(this, &UFPGAAbilityTask_PlayMontageNotifyAccurate::OnMontageEnded);
				AnimInstance->Montage_SetEndDelegate(MontageEndedDelegate, MontageToPlay);

				AnimInstance->OnPlayMontageNotifyBegin.AddDynamic(this, &UFPGAAbilityTask_PlayMontageNotifyAccurate::OnNotifyBeginReceived);
				AnimInstance->OnPlayMontageNotifyEnd.AddDynamic(this, &UFPGAAbilityTask_PlayMontageNotifyAccurate::OnNotifyEndReceived);

				ACharacter* Character = Cast<ACharacter>(GetAvatarActor());
				if (Character && (Character->GetLocalRole() == ROLE_Authority ||
					(Character->GetLocalRole() == ROLE_AutonomousProxy && Ability->GetNetExecutionPolicy() == EGameplayAbilityNetExecutionPolicy::LocalPredicted)))
				{
					Character->SetAnimRootMotionTranslationScale(AnimRootMotionTranslationScale);
				}

				bPlayedMontage = true;
			}
		}
		else
		{
			ABILITY_LOG(Warning, TEXT("UFPGAAbilityTask_PlayMontageNotifyAccurate call to PlayMontage failed!"));
		}
	}
	else
	{
		ABILITY_LOG(Warning, TEXT("UFPGAAbilityTask_PlayMontageNotifyAccurate called on invalid AbilitySystemComponent or Montage"));
	}

	if (!bPlayedMontage)
	{
		ABILITY_LOG(Warning, TEXT("UFPGAAbilityTask_PlayMontageNotifyAccurate called in Ability %s failed to play montage %s; Task Instance Name %s."), *Ability->GetName(), *GetNameSafe(MontageToPlay),
		            *InstanceName.ToString());
		if (ShouldBroadcastAbilityTaskDelegates())
		{
			OnCancelled.Broadcast(NAME_None);
		}
	}

	SetWaitingOnAvatar();
}

void UFPGAAbilityTask_PlayMontageNotifyAccurate::OnNotifyBeginReceived(FName NotifyName, const FBranchingPointNotifyPayload& BranchingPointNotifyPayload)
{
	// OnNotifyBegin.Broadcast(NotifyName);
}

void UFPGAAbilityTask_PlayMontageNotifyAccurate::OnNotifyEndReceived(FName NotifyName, const FBranchingPointNotifyPayload& BranchingPointNotifyPayload)
{
	OnNotifyEnd.Broadcast(NotifyName);
}

void UFPGAAbilityTask_PlayMontageNotifyAccurate::ExternalCancel()
{
	check(AbilitySystemComponent);

	if (ShouldBroadcastAbilityTaskDelegates())
	{
		OnCancelled.Broadcast(NAME_None);
	}
	Super::ExternalCancel();
}

void UFPGAAbilityTask_PlayMontageNotifyAccurate::OnDestroy(bool AbilityEnded)
{	
	// Note: Clearing montage end delegate isn't necessary since its not a multicast and will be cleared when the next montage plays.
	// (If we are destroyed, it will detect this and not do anything)

	// This delegate, however, should be cleared as it is a multicast
	if (Ability)
	{
		Ability->OnGameplayAbilityCancelled.Remove(InterruptedHandle);
		if (AbilityEnded && bStopWhenAbilityEnds)
		{
			StopPlayingMontage();
		}
	}
	else
	{
		UE_LOG(LogTemp, Warning, TEXT("Ability invalid"));
	}

	GetWorld()->GetTimerManager().ClearAllTimersForObject(this);

	Super::OnDestroy(AbilityEnded);
}

void UFPGAAbilityTask_PlayMontageNotifyAccurate::SendNotify()
{
	OnNotifyBegin.Broadcast(FName());
}

bool UFPGAAbilityTask_PlayMontageNotifyAccurate::StopPlayingMontage()
{
	const FGameplayAbilityActorInfo* ActorInfo = Ability->GetCurrentActorInfo();
	if (!ActorInfo)
	{
		UE_LOG(LogTemp, Warning, TEXT("Actor info invalid"));
		return false;
	}

	UAnimInstance* AnimInstance = ActorInfo->GetAnimInstance();
	if (AnimInstance == nullptr)
	{
		UE_LOG(LogTemp, Warning, TEXT("Anim instance invalid"));
		return false;
	}

	// Check if the montage is still playing
	// The ability would have been interrupted, in which case we should automatically stop the montage
	if (AbilitySystemComponent && Ability)
	{
		if (AbilitySystemComponent->GetAnimatingAbility() == Ability
			&& AbilitySystemComponent->GetCurrentMontage() == MontageToPlay)
		{
			// Unbind delegates so they don't get called as well
			FAnimMontageInstance* MontageInstance = AnimInstance->GetActiveInstanceForMontage(MontageToPlay);
			if (MontageInstance)
			{
				MontageInstance->OnMontageBlendingOutStarted.Unbind();
				MontageInstance->OnMontageEnded.Unbind();
			}
			else
			{
				UE_LOG(LogTemp, Warning, TEXT("Failed to unbind?"));
			}

			// UE_LOG(LogTemp, Warning, TEXT("Stopped playing montage! %s | %s"), *ActorInfo->OwnerActor->GetName(), *AbilitySystemComponent->GetOwner()->GetName());
			AbilitySystemComponent->CurrentMontageStop();
			return true;
		}
	}

	return false;
}

FString UFPGAAbilityTask_PlayMontageNotifyAccurate::GetDebugString() const
{
	UAnimMontage* PlayingMontage = nullptr;
	if (Ability)
	{
		const FGameplayAbilityActorInfo* ActorInfo = Ability->GetCurrentActorInfo();
		UAnimInstance* AnimInstance = ActorInfo->GetAnimInstance();

		if (AnimInstance != nullptr)
		{
			PlayingMontage = AnimInstance->Montage_IsActive(MontageToPlay) ? MontageToPlay : AnimInstance->GetCurrentActiveMontage();
		}
	}

	return FString::Printf(TEXT("PlayMontageAndWait. MontageToPlay: %s  (Currently Playing): %s"), *GetNameSafe(MontageToPlay), *GetNameSafe(PlayingMontage));
}

void UFPGAAbilityTask_PlayMontageNotifyAccurate::UnbindDelegates()
{
	if (UAnimInstance* AnimInstance = AnimInstancePtr.Get())
	{
		AnimInstance->OnPlayMontageNotifyBegin.RemoveDynamic(this, &UFPGAAbilityTask_PlayMontageNotifyAccurate::OnNotifyBeginReceived);
		AnimInstance->OnPlayMontageNotifyEnd.RemoveDynamic(this, &UFPGAAbilityTask_PlayMontageNotifyAccurate::OnNotifyEndReceived);
	}
}

void UFPGAAbilityTask_PlayMontageNotifyAccurate::BeginDestroy()
{
	UnbindDelegates();
	Super::BeginDestroy();
}
