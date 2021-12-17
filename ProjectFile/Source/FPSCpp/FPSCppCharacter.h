// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "FPSCppProjectile.h"
#include "Grenade.h"
#include "Components/SpotLightComponent.h"
#include "GameFramework/Character.h"
#include "GameFramework/SpringArmComponent.h"
#include "FPSCppCharacter.generated.h"

class UInputComponent;
class USkeletalMeshComponent;
class USceneComponent;
class UCameraComponent;
class UMotionControllerComponent;
class UAnimMontage;
class USoundBase;

UCLASS(config=Game)
class AFPSCppCharacter : public ACharacter
{
	GENERATED_BODY()
public:

	UPROPERTY(VisibleAnywhere, BlueprintReadWrite, Category=Mesh)
	USkeletalMeshComponent* Mesh1P;
	
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = Mesh)
	USkeletalMeshComponent* Gun;
	
	UPROPERTY(VisibleDefaultsOnly, BlueprintReadOnly, Category = Location)
	USceneComponent* MuzzleLocation;

	UPROPERTY(VisibleDefaultsOnly, BlueprintReadOnly, Category=Location)
	USceneComponent* GrenadeLocation;

	UPROPERTY(VisibleDefaultsOnly, BlueprintReadOnly, Category=Location)
	USceneComponent* ZoomCameraLocation;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = Camera)
	USpringArmComponent* CameraSpringArm;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = Camera)
	UCameraComponent* TPSCameraComponent;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = Camera)
	UCameraComponent* ZoomInCamera;

	UPROPERTY(Replicated,VisibleAnywhere, BlueprintReadOnly, Category = Camera)
	UCameraComponent* MainCamera;

	UPROPERTY(Replicated,EditDefaultsOnly, BlueprintReadOnly, Category=Animation)
	UAnimMontage* ReloadMontage;

	UPROPERTY(Replicated,EditDefaultsOnly, BlueprintReadOnly, Category=Animation)
	UAnimMontage* Hip_FireMontage;

	UPROPERTY(Replicated,EditDefaultsOnly, BlueprintReadOnly, Category=Animation)
	UAnimMontage* Ironsights_FireMontage;


	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category=Camera)
	float BaseTurnRate;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category=Camera)
	float BaseLookUpRate;


	UPROPERTY(EditDefaultsOnly, Category= Asset)
	TSubclassOf<AFPSCppProjectile> ProjectileClass;

	UPROPERTY(Replicated,EditDefaultsOnly, Category= Asset)
	TSubclassOf<AGrenade> GrenadeClass;

	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category= Asset)
	TSubclassOf<UUserWidget> PlayerStateWidget;

	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category= Asset)
	TSubclassOf<UUserWidget> AimHUDWidget;
	
	UPROPERTY(Replicated,EditDefaultsOnly, Category= Asset)
	UParticleSystem* ShootParticle;

	UPROPERTY(Replicated,EditDefaultsOnly, Category= Asset)
	UParticleSystem* HittedParticle;

	UPROPERTY(EditDefaultsOnly, Category= Asset)
	TSubclassOf<UCameraShakeBase> CameraShake;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category= GameSetting)
	FVector GunOffset;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category= GameSetting)
	USoundBase* FireSound;

	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category= GameSetting)
	float ReloadTimer;

	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category= GameSetting)
	float ReloadTime;

	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category= GameSetting)
	int FullAmmo;

	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category= GameSetting)
	int CurrentAmmo;

	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category= GameSetting)
	int PerAmmo;

	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category= GameSetting)
	int GrenadeCount;

	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category= GameSetting)
	float HitImpulse;

	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category= GameSetting)
	float ShootingDistance;

	UPROPERTY(Replicated,EditDefaultsOnly, BlueprintReadWrite, Category= Gameplay)
	bool bAbleToFire;

	UPROPERTY(Replicated,BlueprintReadWrite, Category= GamePlay)
	bool bIsFiring = false;

	UPROPERTY(Replicated,BlueprintReadWrite, Category=GamePlay)
	bool bIsReloading = false;;

	UPROPERTY(Replicated,EditAnywhere, BlueprintReadWrite, Category=GamePlay)
	bool bIsCrouching = false;

	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category=GamePlay)
	bool bAbleToCrouch;

	UPROPERTY(BlueprintReadWrite, Category=GamePlay)
	bool bIsZooming = false;;

	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category=GamePlay)
	bool bAbleToZoomIn;

	UPROPERTY(BlueprintReadWrite, Category=GamePlay)
	bool bIsJumping = false;;

	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category=GamePlay)
	bool bAbleToJump;

	UPROPERTY(BlueprintReadWrite, Category=GamePlay)
	bool bIsRunning = false;;

	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category=GamePlay)
	bool bAbleToRun;

	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category=GamePlay)
	bool bAbleToUseGrenade;

	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category=GamePlay)
	int DamageValue;

	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;

	FTimerHandle ReloadTimerHandle;
	FTimerHandle GrenadeCoolDownTimerHandle;

protected:

	virtual void BeginPlay();

	virtual void Tick(float DeltaSeconds) override;

	virtual void SetupPlayerInputComponent(UInputComponent* InputComponent) override;

	void MoveForward(float Val);

	void MoveRight(float Val);

	void TurnAtRate(float Rate);

	void LookUpAtRate(float Rate);

    UFUNCTION(BlueprintCallable)
	void OnFire();

	UFUNCTION(Server,Reliable,BlueprintCallable)
	void Server_OnFire();

	UFUNCTION(NetMulticast,Unreliable)
	void Multicast_HitParticle(const FVector &HitLocation);

	UFUNCTION(NetMulticast,Unreliable)
	void Multicast_Fire();

	void StopFire();

	UFUNCTION(BlueprintCallable)
	void Reload();
	
	void ReloadFinish();

	UFUNCTION(Server,Reliable,BlueprintCallable)
	void Server_Reload();

	UFUNCTION(NetMulticast,Reliable,BlueprintCallable)
	void Multicast_Reload();
	
	UFUNCTION(Server,Reliable,BlueprintCallable)
	void Server_ReloadFinish();

	UFUNCTION(BlueprintCallable)
	void Grenade();

	UFUNCTION(Server,Reliable,BlueprintCallable)
	void Server_Grenade();

	void GrenadeCoolDown();

	UFUNCTION(BlueprintCallable)
	void Walk();

	UFUNCTION(BlueprintCallable)
	void StopWalk();
	
	UFUNCTION(Server,Reliable,BlueprintCallable)
	void Server_Walk();

	UFUNCTION(Server,Reliable,BlueprintCallable)
	void Server_StopWalk();
	
	UFUNCTION(BlueprintCallable)
	void OnCrouch();

	UFUNCTION(Server,Reliable,BlueprintCallable)
	void Server_OnCrouch();

	UFUNCTION(BlueprintCallable)
	void StopCrouch();
	
	UFUNCTION(Server,Reliable,BlueprintCallable)
	void Server_StopCrouch();

	UFUNCTION(BlueprintCallable)
	void OnZoom();

	UFUNCTION(BlueprintCallable)
	void StopZoom();

	UFUNCTION(Server,Reliable,BlueprintCallable)
	void Server_OnZoom();

	UFUNCTION(Server,Reliable,BlueprintCallable)
	void Server_StopZoom();

	UFUNCTION(BlueprintCallable)
	void Run();

	UFUNCTION(BlueprintCallable)
	void StopRun();

	virtual float TakeDamage(float DamageAmount, FDamageEvent const& DamageEvent, AController* EventInstigator, AActor* DamageCauser) override;

public:
	
	AFPSCppCharacter();

	UFUNCTION(BlueprintCallable)
	float FireOffset();

	UFUNCTION(BlueprintNativeEvent)
	void ReturnDamage(float Damage);
	
	UFUNCTION(BlueprintNativeEvent)
	void MontageEvent();
};
