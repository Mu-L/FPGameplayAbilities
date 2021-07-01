// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Animation/AnimInstance.h"
#include "Abilities/Tasks/AbilityTask.h"
#include "FPGAAbilityTask_PlayMontageNotifyAccurate.generated.h"

DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FMontageNotifyAccurateDelegate, FName, NotifyName);

UCLASS()
class FPGAMEPLAYABILITIES_API UFPGAAbilityTask_PlayMontageNotifyAccurate : public UAbilityTask//UFPGAAbilityTask_WaitCancel
{
	GENERATED_UCLASS_BODY()

	UPROPERTY(BlueprintAssignable)
	FMontageNotifyAccurateDelegate OnCompleted;

	UPROPERTY(BlueprintAssignable)
	FMontageNotifyAccurateDelegate OnBlendOut;

	UPROPERTY(BlueprintAssignable)
	FMontageNotifyAccurateDelegate OnInterrupted;

	UPROPERTY(BlueprintAssignable)
	FMontageNotifyAccurateDelegate OnCancelled;

	UPROPERTY(BlueprintAssignable)
	FMontageNotifyAccurateDelegate OnNotifyBegin;

	UPROPERTY(BlueprintAssignable)
	FMontageNotifyAccurateDelegate OnNotifyEnd;

	UFUNCTION()
	void OnMontageBlendingOut(UAnimMontage* Montage, bool bInterrupted);

	UFUNCTION()
	void OnMontageInterrupted();

	UFUNCTION()
	void OnMontageEnded(UAnimMontage* Montage, bool bInterrupted);

	UFUNCTION()
	void OnNotifyBeginReceived(FName NotifyName, const FBranchingPointNotifyPayload& BranchingPointNotifyPayload);

	UFUNCTION()
	void OnNotifyEndReceived(FName NotifyName, const FBranchingPointNotifyPayload& BranchingPointNotifyPayload);

	UFUNCTION(BlueprintCallable, Category = "Ability|Tasks", meta = (DisplayName = "FPGAPlayMontageNotifyAccurate",
		HidePin = "OwningAbility", DefaultToSelf = "OwningAbility", BlueprintInternalUseOnly = "TRUE"))
	static UFPGAAbilityTask_PlayMontageNotifyAccurate* CreatePlayMontageAndWaitProxy(
		UGameplayAbility* OwningAbility = nullptr,
		FName TaskInstanceName = FName("PlayMontageNotifyAccurate"), 
		UAnimMontage* MontageToPlay = nullptr, 
		float Rate = 1.f, 
		FName StartSection = NAME_None, 
		bool bStopWhenAbilityEnds = true,
		bool bEndOnCancelInput = false,
		float AnimRootMotionTranslationScale = 1.f);

	virtual void TickTask(float DeltaTime) override;

	virtual void Activate() override;

	/** Called when the ability is asked to cancel from an outside node. What this means depends on the individual task. By default, this does nothing other than ending the task. */
	virtual void ExternalCancel() override;

	virtual FString GetDebugString() const override;

public:
	//~ Begin UObject Interface
	virtual void BeginDestroy() override;
	//~ End UObject Interface

private:
	TWeakObjectPtr<UAnimInstance> AnimInstancePtr;

	virtual void OnDestroy(bool AbilityEnded) override;

	UFUNCTION()
	void SendNotify();

	FTimerHandle TimerHandle;

	/** Checks if the ability is playing a montage and stops that montage, returns true if a montage was stopped, false if not. */
	bool StopPlayingMontage();

	FOnMontageBlendingOutStarted BlendingOutDelegate;
	FOnMontageEnded MontageEndedDelegate;
	FDelegateHandle InterruptedHandle;

	float NotifyDelay;

	UPROPERTY()
	UAnimMontage* MontageToPlay;

	UPROPERTY()
	float Rate;

	UPROPERTY()
	FName StartSection;

	UPROPERTY()
	float AnimRootMotionTranslationScale;

	UPROPERTY()
	bool bStopWhenAbilityEnds;

	void UnbindDelegates();
};