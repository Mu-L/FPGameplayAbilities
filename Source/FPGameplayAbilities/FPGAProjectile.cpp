﻿// Fill out your copyright notice in the Description page of Project Settings.

#include "FPGAProjectile.h"

#include "AbilitySystem/FPGAGameplayAbilitiesLibrary.h"
#include "GameFramework/ProjectileMovementComponent.h"
#include "Net/UnrealNetwork.h"

// Sets default values
AFPGAProjectile::AFPGAProjectile()
{
	// Set this actor to call Tick() every frame.  You can turn this off to improve performance if you don't need it.
	PrimaryActorTick.bCanEverTick = true;

	ProjectileMovementComponent = CreateDefaultSubobject<UProjectileMovementComponent>("ProjectileMovement");
	ProjectileMovementComponent->bRotationFollowsVelocity = true;
	ProjectileMovementComponent->bRotationRemainsVertical = true;
	ProjectileMovementComponent->bInitialVelocityInLocalSpace = false;

	ProjectileMovementComponent->ProjectileGravityScale = 0.0f;
	ProjectileMovementComponent->Friction = 0.0f;

	ProjectileMovementComponent->MaxSpeed = 500.0f;
	ProjectileMovementComponent->InitialSpeed = 500.0f;
	ProjectileMovementComponent->HomingAccelerationMagnitude = 250.0f;

	ProjectileMovementComponent->bInterpMovement = true;
	ProjectileMovementComponent->bInterpRotation = true;

	ProjectileMovementComponent->bIsHomingProjectile = true;

	SetReplicates(true);
	SetReplicatingMovement(true);
}

void AFPGAProjectile::InitProjectile(const FGameplayAbilityTargetDataHandle& InTargetData)
{
	TargetData = InTargetData;

	TargetSceneComponent = NewObject<USceneComponent>(this);
	TargetSceneComponent->RegisterComponent();

	if (ProjectileMovementComponent->bIsHomingProjectile)
	{
		ProjectileMovementComponent->bIsHomingProjectile = true;
		ProjectileMovementComponent->HomingTargetComponent = TargetSceneComponent;

		if (AActor* TargetActor = UFPGAGameplayAbilitiesLibrary::GetFirstActorFromTargetData(TargetData))
		{
			TargetActor->OnDestroyed.AddUniqueDynamic(this, &AFPGAProjectile::OnTargetDestroyed);
			TargetSceneComponent->AttachToComponent(TargetActor->GetRootComponent(), FAttachmentTransformRules::SnapToTargetNotIncludingScale);
		}
		else
		{
			FVector TargetLocation;
			if (UFPGAGameplayAbilitiesLibrary::GetLocationFromTargetData(TargetData, 0, TargetLocation))
			{
				// const FVector ToTarget = TargetLocation - GetActorLocation();
				// FVector ProjectileVelocity = ToTarget.GetSafeNormal2D() * ProjectileMovementComponent->MaxSpeed;
				// ProjectileMovementComponent->Velocity = ProjectileVelocity;

				TargetSceneComponent->SetWorldLocation(TargetLocation);
			}
		}
	}
	else
	{
		// just fly forwards until we hit something?
		FVector TargetLocation;
		if (UFPGAGameplayAbilitiesLibrary::GetLocationFromTargetData(TargetData, 0, TargetLocation))
		{
			const FVector ToTarget = TargetLocation - GetActorLocation();
			ProjectileMovementComponent->Velocity = ToTarget.GetSafeNormal2D() * ProjectileMovementComponent->InitialSpeed;
		}
	}
}

// Called when the game starts or when spawned
void AFPGAProjectile::BeginPlay()
{
	Super::BeginPlay();
}

void AFPGAProjectile::NotifyActorBeginOverlap(AActor* OtherActor)
{
	Super::NotifyActorBeginOverlap(OtherActor);
	// UE_LOG(LogTemp, Warning, TEXT("Overlap actor?"));
}

void AFPGAProjectile::Destroyed()
{
	Super::Destroyed();

	if (AActor* TargetActor = UFPGAGameplayAbilitiesLibrary::GetFirstActorFromTargetData(TargetData))
	{
		TargetActor->OnDestroyed.RemoveAll(this);
	}
}

// Called every frame
void AFPGAProjectile::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

	if (GetLocalRole() != ROLE_Authority)
	{
		return;
	}

	FVector TargetLocation;
	if (!UFPGAGameplayAbilitiesLibrary::GetLocationFromTargetData(TargetData, 0, TargetLocation))
	{
		return;
	}

	const FVector ToTarget = TargetLocation - GetActorLocation();
	if (ToTarget.SizeSquared2D() < 25 * 25)
	{
		UE_LOG(LogTemp, Warning, TEXT("Hit target!"));
		Destroy();
	}
	// else
	// {
	// 	UE_LOG(LogTemp, Warning, TEXT("Updating velocity"));
	// 	ProjectileMovementComponent->Velocity = ToTarget.GetSafeNormal2D() * 1000.0f;
	// }
}

void AFPGAProjectile::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);

	DOREPLIFETIME(AFPGAProjectile, TargetData);
}

void AFPGAProjectile::OnTargetDestroyed(AActor* Actor)
{
	TargetSceneComponent->DetachFromComponent(FDetachmentTransformRules::KeepWorldTransform);
}