// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Components/ProgressBar.h"
#include "AttributeSet.h"
#include "FPGAAttributeBar.generated.h"

DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FOnFPGAAttributeBarChanged, float, OldValue, float, NewValue);

struct FOnAttributeChangeData;
class UAbilitySystemComponent;

UCLASS()
class FPGAMEPLAYABILITIES_API UFPGAAttributeBar : public UProgressBar
{
	GENERATED_BODY()

public:
	bool bNeedToUnbind;

	float LastCurrentValue;
	float LastMaxValue;

	FDelegateHandle CurrentDelegateHandle;
	FDelegateHandle MaxDelegateHandle;

	UAbilitySystemComponent* AbilitySystemPtr;

	UPROPERTY(EditAnywhere, BlueprintReadOnly)
	FGameplayAttribute CurrentAttribute;

	UPROPERTY(EditAnywhere, BlueprintReadOnly)
	FGameplayAttribute MaxAttribute;

	UPROPERTY(BlueprintAssignable)
	FOnFPGAAttributeBarChanged OnFPGAAttributeBarChanged;

	UFUNCTION(BlueprintCallable)
	void BindAttributes(UAbilitySystemComponent* AbilitySystem);

	UFUNCTION(BlueprintCallable)
	void BindAndSetAttributes(UAbilitySystemComponent* AbilitySystem, FGameplayAttribute NewCurrent, FGameplayAttribute NewMax);
	
	UFUNCTION(BlueprintCallable)
	float GetCurrentValue() const { return LastCurrentValue; }

	void UnbindAttributes();

	void OnCurrentAttributeChanged(const FOnAttributeChangeData& ChangeData);
	void OnMaxAttributeChanged(const FOnAttributeChangeData& ChangeData);

	void UpdatePercent();

	void BeginDestroy() override;

#if WITH_EDITOR
	virtual const FText GetPaletteCategory() override;
#endif
};
