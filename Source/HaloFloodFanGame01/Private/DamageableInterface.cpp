// Fill out your copyright notice in the Description page of Project Settings.


#include "DamageableInterface.h"

#include "HealthComponent.h"
#include "Core/BaseCharacter.h"
#include "Engine/DamageEvents.h"
#include "Kismet/GameplayStatics.h"

// Add default functionality here for any IDamageableInterface functions that are not pure virtual.

float IDamageableInterface::TakeDamage(float DamageAmount, FVector Force, FDamageEvent const& DamageEvent,
	AController* EventInstigator, AActor* DamageCauser)
{
	if (GetHealthComponent())
	{
		GetHealthComponent()->TakeDamage(DamageAmount, Force);
	}
		
	return 0;
}

float IDamageableInterface::TakePointDamage(float DamageAmount, FVector Force, FPointDamageEvent const& PointDamageEvent,
	AController* EventInstigator, AActor* DamageCauser)
{
	FDamageEvent DamageEvent;
	TakeDamage(DamageAmount, Force, DamageEvent, EventInstigator, DamageCauser);
	return 0;
}

float IDamageableInterface::TakeRadialDamage(float DamageAmount, FVector Force,
	FRadialDamageEvent const& RadialDamageEvent, AController* EventInstigator, AActor* DamageCauser)
{
	FDamageEvent DamageEvent;
	TakeDamage(DamageAmount, Force, DamageEvent, EventInstigator, DamageCauser);
	return 0;
}

void IDamageableInterface::HealthDepleted(float Damage, FVector Force, FVector HitLocation, FName HitBoneName)
{
}

UHealthComponent* IDamageableInterface::GetHealthComponent()
{
	return 0;
}
