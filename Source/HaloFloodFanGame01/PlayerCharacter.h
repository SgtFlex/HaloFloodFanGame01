// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "GenericTeamAgentInterface.h"
#include "GameFramework/Character.h"
#include "InputActionValue.h"
#include "Components/TimelineComponent.h"
#include "Core/CharacterBase.h"
#include "Kismet/GameplayStatics.h"
#include "PlayerCharacter.generated.h"

class IInteractableInterface;
class USphereComponent;
class UBoxComponent;
class UPlayerHUD;
class AGrenadeBase;
class UInputComponent;
class USkeletalMeshComponent;
class USceneComponent;
class UCameraComponent;
class UAnimMontage;
class USoundBase;

DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FWeaponsUpdated, AGunBase*, NewGun, AGunBase*, OldGun);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnInteractableChanged, AActor*, Interactable);


UCLASS(config=Game)
class APlayerCharacter : public ACharacterBase
{
	GENERATED_BODY()

	/** Pawn mesh: 1st person view (arms; seen only by self) */
	UPROPERTY(EditAnywhere, Category=Mesh)
	USkeletalMeshComponent* Mesh1P;

	/** First person camera */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = Camera, meta = (AllowPrivateAccess = "true"))
	UCameraComponent* FirstPersonCameraComponent;

	UPROPERTY(EditAnywhere)
	USphereComponent* InteractionSphere;

	/** MappingContext */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category=Input, meta=(AllowPrivateAccess = "true"))
	class UInputMappingContext* DefaultMappingContext;

	/** Jump Input Action */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category=Input, meta=(AllowPrivateAccess = "true"))
	class UInputAction* JumpAction;

	/** Move Input Action */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category=Input, meta=(AllowPrivateAccess = "true"))
	class UInputAction* MoveAction;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category=Input, meta = (AllowPrivateAccess = "true"))
	class UInputAction* InteractAction;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category=Input, meta = (AllowPrivateAccess = "true"))
	class UInputAction* SwitchWeaponAction;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category=Input, meta = (AllowPrivateAccess = "true"))
	class UInputAction* PrimaryAttackAction;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category=Input, meta = (AllowPrivateAccess = "true"))
	class UInputAction* MeleeAction;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category=Input, meta = (AllowPrivateAccess = "true"))
	class UInputAction* ReloadAction;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category=Input, meta = (AllowPrivateAccess = "true"))
	class UInputAction* SwitchGrenadeAction;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category=Input, meta = (AllowPrivateAccess = "true"))
	class UInputAction* ThrowGrenadeAction;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category=Input, meta = (AllowPrivateAccess = "true"))
	class UInputAction* UseEquipmentAction;

	UPROPERTY()
	UEnhancedInputComponent* EnhancedInputComponent;

	UPROPERTY()
	FTimerHandle PossessionDelay;
	
public:
	APlayerCharacter();

	/** Property replication */
	void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;

	UFUNCTION()
	virtual void OnInteractionSphereBeginOverlap(UPrimitiveComponent* OverlappedComponent, AActor* OtherActor, UPrimitiveComponent* OtherComp, int32 OtherBodyIndex, bool bFromSweep, const FHitResult & SweepResult);

	UFUNCTION()
	virtual void OnInteractionSphereEndOverlap(UPrimitiveComponent* OverlappedComponent, AActor* OtherActor, UPrimitiveComponent* OtherComp, int32 OtherBodyIndex);

protected:
	virtual void BeginPlay();

	virtual void Tick(float DeltaSeconds) override;

public:	
	UFUNCTION(BlueprintGetter)
	FHitResult GetPlayerAim();
	
	/** Look Input Action */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = Input, meta = (AllowPrivateAccess = "true"))
	class UInputAction* LookAction;
	
	UFUNCTION()
	virtual void ThrowEquippedGrenade_Implementation() override;

	UFUNCTION(BlueprintCallable)
	void SwitchWeapon();

	UFUNCTION(BlueprintCallable)
	void Interact();

	UFUNCTION(BlueprintCallable)
	void SwitchGrenadeType();

	void SwitchGrenadeType(int Index);

	virtual void PossessedBy(AController* NewController) override;

	
	virtual void UnPossessed() override;

	void ControllerChanged(AController* OldController, AController* NewController);
	
	virtual void PickupWeapon(AGunBase* Gun) override;
	
	virtual void DropWeapon() override;

	UFUNCTION(BlueprintCallable)
	void SetFragCount(int32 NewFragCount);

	/** Returns Mesh1P subobject **/
	UFUNCTION(BlueprintCallable)
	USkeletalMeshComponent* GetMesh1P() const { return Mesh1P; }
	/** Returns FirstPersonCameraComponent subobject **/
	UFUNCTION(BlueprintCallable)
	UCameraComponent* GetFirstPersonCameraComponent() const { return FirstPersonCameraComponent; }

	UUserWidget* GetPlayerHUD() const { return PlayerHUD; }
protected:
	/** Called for movement input */
	void Move(const FInputActionValue& Value);

	/** Called for looking input */
	void Look(const FInputActionValue& Value);

	UFUNCTION(Server, Unreliable)
	void Server_Look(float Pitch);

	UFUNCTION(NetMulticast, Unreliable)
	void Multi_Look(float Pitch);

	UFUNCTION(BlueprintCallable)
	virtual void Melee_Implementation() override;

	UFUNCTION()
	virtual void MeleeDamageCode();

	UFUNCTION()
	void MeleeUpdate(float Alpha);

	void OnHealthDepleted_Implementation(float Damage, FVector Force, FVector HitLocation, FName HitBoneName, AController* EventInstigator = nullptr, AActor* DamageCauser = nullptr);

	// APawn interface
	virtual void SetupPlayerInputComponent(UInputComponent* InputComponent) override;
	// End of APawn interface
	
public:
	UPROPERTY()
	APlayerController* PlayerController;
	
	UPROPERTY()
	FWeaponsUpdated WeaponsUpdated;

	UPROPERTY()
	FOnInteractableChanged OnInteractableChanged;

	UPROPERTY(EditAnywhere)
	TSubclassOf<class UUserWidget> PlayerHUDClass;


	UPROPERTY()
	class UUserWidget* PlayerHUD;

	UPROPERTY(EditAnywhere, Category="Loadout")
	int32 FragCount = 0;
	UPROPERTY(EditAnywhere, Category="Loadout")
	int32 PlasmaCount = 0;
	UPROPERTY(EditAnywhere, Category="Loadout")
	int32 SpikeCount = 0;
	UPROPERTY(EditAnywhere, Category="Loadout")
	int32 IncenCount = 0;

private:
	UPROPERTY()
	AActor* InteractableActor;
	
	FTimerHandle ShieldDelayTimerHandle;

	FTimeline MeleeTimeline;

	UPROPERTY(EditDefaultsOnly)
	UCurveFloat* MeleeCurve;

	FHitResult PlayerAim;

	
	

protected:
	UPROPERTY(EditAnywhere, Category="Collision")
	TEnumAsByte<ECollisionChannel> TraceChannelProperty = ECC_Pawn;
};

