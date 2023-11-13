﻿// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameplayEffectUIData.h"
#include "UObject/Object.h"
#include "FPGameplayEffectUIData.generated.h"

UINTERFACE(BlueprintType, Blueprintable)
class UFPUIInterface : public UInterface
{
	GENERATED_BODY()
};

class IFPUIInterface
{
	GENERATED_BODY()

public:
	UFUNCTION(BlueprintNativeEvent, BlueprintCallable)
	FText GetFPDisplayName();

	UFUNCTION(BlueprintNativeEvent, BlueprintCallable)
	FText GetFPDisplayDescription();

	UFUNCTION(BlueprintNativeEvent, BlueprintCallable)
	TSoftObjectPtr<UTexture2D> GetFPDisplayTexture();

	UFUNCTION(BlueprintNativeEvent, BlueprintCallable)
	FSlateColor GetFPDisplayColor();
};

USTRUCT(BlueprintType)
struct FPGAMEPLAYABILITIES_API FFPUIData
{
	GENERATED_BODY()

	UPROPERTY(BlueprintReadWrite, EditAnywhere)
	FText Name;

	UPROPERTY(BlueprintReadWrite, EditAnywhere)
	FText Description;

	UPROPERTY(BlueprintReadWrite, EditAnywhere)
	TSoftObjectPtr<UTexture2D> Texture;

	UPROPERTY(BlueprintReadWrite, EditAnywhere)
	FSlateColor Color;
};

UCLASS(Blueprintable, EditInlineNew, CollapseCategories)
class FPGAMEPLAYABILITIES_API UFPGameplayEffectUIData : public UGameplayEffectUIData
{
	GENERATED_BODY()

public:
	UPROPERTY(BlueprintReadOnly, EditDefaultsOnly)
	FFPUIData Data;
};