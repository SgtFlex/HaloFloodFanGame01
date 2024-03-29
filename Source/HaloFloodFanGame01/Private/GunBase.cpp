// Fill out your copyright notice in the Description page of Project Settings.

#include "GunBase.h"

#include "MyCustomBlueprintFunctionLibrary.h"
#include "NiagaraComponent.h"
#include "NiagaraFunctionLibrary.h"
#include "VectorTypes.h"
#include "Components/Image.h"
#include "Engine/DamageEvents.h"
#include "HaloFloodFanGame01/PlayerCharacter.h"
#include "Perception/AISense_Hearing.h"
#include "Perception/AISense_Sight.h"


// Sets default values
AGunBase::AGunBase()
{
 	// Set this actor to call Tick() every frame.  You can turn this off to improve performance if you don't need it.
	PrimaryActorTick.bCanEverTick = true;

	
	Mesh = CreateDefaultSubobject<USkeletalMeshComponent>(TEXT("CharacterMesh1P"));
	Mesh->SetSimulatePhysics(true);
	RootComponent = Mesh;

	bReplicates = true;
	
}

// Called when the game starts or when spawned
void AGunBase::BeginPlay()
{
	Super::BeginPlay();
	CurMagazine = MaxMagazine;
	CurReserve = MaxReserve;
	InteractIcon = WeaponIcon;
}

// Called every frame
void AGunBase::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);
}

void AGunBase::Pickup(ACharacterBase* Char)
{
	SetOwner(Char);
}

void AGunBase::Equip()
{
}

void AGunBase::Drop()
{
	GetWorldTimerManager().ClearTimer(ReloadTimer);
	bReloading = false;
	SetOwner(nullptr);
	if (CurMagazine + CurReserve <= 0)
	{
		//Disable collision query responses to prevent being picked up.
		Mesh->SetCollisionEnabled(ECollisionEnabled::PhysicsOnly);
	}
}

void AGunBase::StartReload()
{
	Server_StartReload();
}

void AGunBase::Server_StartReload_Implementation()
{
	Multi_StartReload();
}

void AGunBase::Multi_StartReload_Implementation()
{
	if (bReloading || CurReserve <= 0 || CurMagazine == MaxMagazine) return;
	bReloading = true;
	if (BurstAmount > 0) GetWorldTimerManager().ClearTimer(FireHandle);
	GetWorld()->GetTimerManager().SetTimer(ReloadTimer, this, &AGunBase::FinishReload, ReloadSpeed, false);
	if (ReloadSound) UGameplayStatics::SpawnSoundAttached(ReloadSound, GetRootComponent());
}

void AGunBase::PullTrigger_Implementation()
{
	Server_PullTrigger();
}

void AGunBase::Server_PullTrigger_Implementation()
{
	Multi_PullTrigger();
}

bool AGunBase::Server_PullTrigger_Validate()
{
	return true;
}

void AGunBase::Multi_PullTrigger_Implementation()
{
	if (GetWorldTimerManager().TimerExists(BurstRetriggerHandle) || CurMagazine <= 0 || bReloading)
		return;
	BulletsFired = 0;
	GetWorldTimerManager().SetTimer(FireHandle, this, &AGunBase::Fire, 60/FireRate, true, 0);
	GetWorldTimerManager().SetTimer(BurstRetriggerHandle, BurstRetriggerDelay, false);
}

bool AGunBase::Multi_PullTrigger_Validate()
{
	return true;
}

void AGunBase::Server_ReleaseTrigger_Implementation()
{
	Multi_ReleaseTrigger();
}

bool AGunBase::Server_ReleaseTrigger_Validate()
{
	return true;
}

void AGunBase::Multi_ReleaseTrigger_Implementation()
{
	if (BulletsFired >= BurstAmount)
		GetWorldTimerManager().ClearTimer(FireHandle);
}

bool AGunBase::Multi_ReleaseTrigger_Validate()
{
	return true;
}

void AGunBase::ReleaseTrigger_Implementation()
{
	Server_ReleaseTrigger();
}

void AGunBase::OnInteract_Implementation(APlayerCharacter* Character)
{
	IInteractableInterface::OnInteract_Implementation(Character);
	Character->PickupWeapon(this);
}

void AGunBase::GetInteractInfo_Implementation(FText& Text, UTexture2D*& Icon)
{
	IInteractableInterface::GetInteractInfo_Implementation(Text, Icon);

	Text = InteractText;
	Icon = InteractIcon;
	
}

void AGunBase::Fire_Implementation()
{
	if (bReloading || CurMagazine <= 0)
	{
		ReleaseTrigger();
		return;
	}

	if (APlayerCharacter* Char = Cast<APlayerCharacter>(GetOwner()))
		if (FiringCameraShake && Char->IsLocallyControlled())
			Cast<APlayerController>(Char->GetController())->PlayerCameraManager->StartCameraShake(FiringCameraShake, 1, ECameraShakePlaySpace::CameraLocal);
	if (FiringSound)
		UGameplayStatics::SpawnSoundAttached(FiringSound, GetRootComponent());
	if (Mesh->DoesSocketExist("Muzzle") && MuzzlePFX)
		UNiagaraFunctionLibrary::SpawnSystemAttached(MuzzlePFX, Mesh, "Muzzle", FVector(0,0,0), FRotator(0,0,0), EAttachLocation::SnapToTarget, true);
	CurMagazine--;
	ACharacterBase* OwningChar = Cast<ACharacterBase>(GetOwner());
	if (!OwningChar) return;
	UAISense_Hearing::ReportNoiseEvent(GetWorld(), GetActorLocation(), 1.0f, OwningChar, 0.0f);
	if (OwningChar->FiringAnim) OwningChar->GetMesh()->GetAnimInstance()->Montage_Play(OwningChar->FiringAnim);
	for (int i = 0; i < Multishot; ++i)
	{
		if (ProjectileClass)
		{
			FVector Location = Mesh->DoesSocketExist("Muzzle") ? Mesh->GetSocketLocation("Muzzle") : GetActorLocation() + GetActorForwardVector()*50000.0f;
			FRotator Rotation = OwningChar->GetBaseAimRotation() + FRotator(FMath::RandRange(-VerticalSpread, VerticalSpread), FMath::RandRange(-HorizontalSpread, HorizontalSpread),0);
			FActorSpawnParameters ActorSpawnParameters;
			ActorSpawnParameters.Owner = this;
			ActorSpawnParameters.Instigator = Cast<ACharacterBase>(this->GetOwner());
			GetWorld()->SpawnActor(ProjectileClass, &Location, &Rotation, ActorSpawnParameters);
		} else
		{
			FHitResult Hit;
			FVector TraceStart;
			FRotator EyeRotation;
			OwningChar->GetActorEyesViewPoint(TraceStart, EyeRotation);
			EyeRotation = OwningChar->GetBaseAimRotation() + FRotator(FMath::RandRange(-VerticalSpread, VerticalSpread), FMath::RandRange(-HorizontalSpread, HorizontalSpread),0);;
			TArray<AActor*> ActorsToIgnore;
			ActorsToIgnore.Add(this);
			ActorsToIgnore.Add(GetOwner());
			// The actual bullet trace, with a width for accuracy forgiveness
			//UKismetSystemLibrary::SphereTraceSingle(GetWorld(), TraceStart, TraceEnd, 20, UEngineTypes::ConvertToTraceType(ECollisionChannel::ECC_Camera), false, ActorsToIgnore, EDrawDebugTrace::None, Hit, true, FLinearColor::Red, FLinearColor::Green, 5);
			AController* EventInstigator = Cast<APawn>(GetOwner())->GetController();
			UMyCustomBlueprintFunctionLibrary::FireHitScanBullet(Hit, GetWorld(), ActorsToIgnore, TraceStart, EyeRotation.Vector(), Range, FalloffCurve, Damage, Force, this, EventInstigator);
			if (Mesh->DoesSocketExist("Muzzle") && TrailPFX)
			{
				FVector TrailEnd = (Hit.bBlockingHit) ? Hit.ImpactPoint : Hit.TraceEnd;
				UNiagaraComponent* TrailPFXComponent = UNiagaraFunctionLibrary::SpawnSystemAttached(TrailPFX, Mesh, "Muzzle", FVector(0,0,0), FRotator(0,0,0), EAttachLocation::SnapToTarget, true);
				TrailPFXComponent->SetVectorParameter("BeamEnd", TrailEnd);
			}
			if (Hit.bBlockingHit)
			{
				if (HitSound) UGameplayStatics::PlaySoundAtLocation(GetWorld(), HitSound, Hit.Location);
				if (ImpactDecal && !Cast<IDamageableInterface>(Hit.GetActor()))
				{
					FVector Location = Hit.ImpactPoint;
					FRotator Rotation = Hit.Normal.Rotation() + FRotator(-90, 0, 0);
					AActor* Decal = GetWorld()->SpawnActor(ImpactDecal, &Location, &Rotation);
					Decal->AttachToComponent(Hit.GetComponent(), FAttachmentTransformRules::KeepWorldTransform);
				}
			}
		}
	}
	BulletsFired++;
	if (BulletsFired==BurstAmount)
	{
		ReleaseTrigger();
	}
	OnFire.Broadcast();
}

void AGunBase::FinishReload_Implementation()
{
	bReloading = false;
	int32 AmountNeed = MaxMagazine - CurMagazine; //32 - 27 gives 5 for example
	int32 AmountGrabbed = FMath::Min(AmountNeed, CurReserve);
	CurMagazine = CurMagazine + AmountGrabbed;
	CurReserve = CurReserve - AmountGrabbed;
	OnReload.Broadcast();
}