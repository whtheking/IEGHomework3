// Copyright Epic Games, Inc. All Rights Reserved.

#include "FPSCppCharacter.h"
#include "FPSCppProjectile.h"
#include "Target.h"
#include "Grenade.h"
#include "Animation/AnimInstance.h"
#include "Camera/CameraComponent.h"
#include "Components/CapsuleComponent.h"
#include "Components/InputComponent.h"
#include "GameFramework/SpringArmComponent.h"
#include "GameFramework/InputSettings.h"
#include "HeadMountedDisplayFunctionLibrary.h"
#include "HealthComponent.h"
#include "Kismet/GameplayStatics.h"
#include "MotionControllerComponent.h"
#include "XRMotionControllerBase.h" // for FXRMotionControllerBase::RightHandSourceId
#include "Blueprint/UserWidget.h"
#include "Net/UnrealNetwork.h"
#include "Components/SpotLightComponent.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "GameFramework/PawnMovementComponent.h"

DEFINE_LOG_CATEGORY_STATIC(LogFPChar, Warning, All);

//////////////////////////////////////////////////////////////////////////
// AFPSCppCharacter

AFPSCppCharacter::AFPSCppCharacter()
{
	GetCapsuleComponent()->InitCapsuleSize(55.f, 96.0f);
	
	BaseTurnRate = 45.f;
	BaseLookUpRate = 45.f;
	Mesh1P = this->GetMesh();
	Mesh1P->SetOnlyOwnerSee(true);
	Mesh1P->SetupAttachment(GetCapsuleComponent());
	Mesh1P->bCastDynamicShadow = false;
	Mesh1P->CastShadow = false;
	Mesh1P->SetRelativeRotation(FRotator(0.f, -90.f, 0.f));
	Mesh1P->SetRelativeLocation(FVector(-0.f, -0.f, -90.f));

	Gun = CreateDefaultSubobject<USkeletalMeshComponent>(TEXT("GunMesh"));
	Gun->SetupAttachment(Mesh1P, TEXT("Gun"));

	TPSCameraComponent = CreateDefaultSubobject<UCameraComponent>(TEXT("FirstPersonCamera"));
	CameraSpringArm = CreateDefaultSubobject<USpringArmComponent>(TEXT("CameraSpringArm"));
	CameraSpringArm->SetupAttachment(Mesh1P);
	TPSCameraComponent->SetupAttachment(CameraSpringArm);
	TPSCameraComponent->AttachToComponent(CameraSpringArm, FAttachmentTransformRules::KeepRelativeTransform);
	CameraSpringArm->bUsePawnControlRotation = true;
	CameraSpringArm->bEnableCameraLag = true;
	CameraSpringArm->TargetArmLength = 300.f;
	CameraSpringArm->SetRelativeLocation(FVector(60.f, 0.f, 160.f));

	ZoomCameraLocation = CreateDefaultSubobject<USceneComponent>(TEXT("ZoomCameraLocation"));
	ZoomCameraLocation->SetupAttachment(Mesh1P, TEXT("head"));

	ZoomInCamera = CreateDefaultSubobject<UCameraComponent>(TEXT("ZoomInCamera"));
	ZoomInCamera->SetupAttachment(ZoomCameraLocation);
	ZoomInCamera->bUsePawnControlRotation = true;
	ZoomInCamera->SetRelativeLocation(FVector(13.f, 5.f, -5.f));
	ZoomInCamera->SetRelativeRotation(FRotator(-30.f, 75.f, -110.f));

	MainCamera = TPSCameraComponent;

	MuzzleLocation = CreateDefaultSubobject<USceneComponent>(TEXT("MuzzleLocation"));
	MuzzleLocation->SetupAttachment(Gun);
	MuzzleLocation->SetRelativeLocation(FVector(0.f, 50.f, 0.f));

	GrenadeLocation = CreateDefaultSubobject<USceneComponent>(TEXT("GrenadeLocation"));
	GrenadeLocation->SetupAttachment(Gun);
	GrenadeLocation->SetRelativeLocation(FVector(10.f, 30.f, 10.f));
	
	GunOffset = FVector(100.0f, 0.0f, 10.0f);

	CurrentAmmo = 30;
	PerAmmo = 30;
	FullAmmo = 120;
	GrenadeCount = 5;
	ReloadTime = 2;
	ReloadTimer = 0;
	
	bAbleToFire = true;
	bAbleToZoomIn = true;
	bAbleToJump = true;
	bAbleToCrouch = true;
	bAbleToRun=true;
	bAbleToUseGrenade=true;
	HitImpulse = 100000.0f;
	ShootingDistance=10000.0f;

	bReplicates=true;
}

void AFPSCppCharacter::BeginPlay()
{
	// Call the base class  
	Super::BeginPlay();
	if (PlayerStateWidget)
	{
		CreateWidget<UUserWidget>(GetWorld(), PlayerStateWidget)->AddToViewport();
	}
	if (AimHUDWidget)
	{
		CreateWidget<UUserWidget>(GetWorld(), AimHUDWidget)->AddToViewport();
	}
}

void AFPSCppCharacter::GetLifetimeReplicatedProps(TArray< FLifetimeProperty > & OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);

	DOREPLIFETIME(AFPSCppCharacter,HittedParticle);
	DOREPLIFETIME(AFPSCppCharacter,ShootParticle);
	DOREPLIFETIME(AFPSCppCharacter,ReloadMontage);
	DOREPLIFETIME(AFPSCppCharacter,Hip_FireMontage);
	DOREPLIFETIME(AFPSCppCharacter,Ironsights_FireMontage);
	DOREPLIFETIME(AFPSCppCharacter,GrenadeClass);
	DOREPLIFETIME(AFPSCppCharacter,bIsReloading);
	DOREPLIFETIME(AFPSCppCharacter,bIsCrouching);
	DOREPLIFETIME(AFPSCppCharacter,MainCamera);
}

void AFPSCppCharacter::SetupPlayerInputComponent(class UInputComponent* PlayerInputComponent)
{

	check(PlayerInputComponent);
	
	PlayerInputComponent->BindAction("Jump", IE_Pressed, this, &ACharacter::Jump);
	PlayerInputComponent->BindAction("Jump", IE_Released, this, &ACharacter::StopJumping);
	
	PlayerInputComponent->BindAction("Fire", IE_Pressed, this, &AFPSCppCharacter::OnFire);
	PlayerInputComponent->BindAction("Fire", IE_Released, this, &AFPSCppCharacter::StopFire);

	PlayerInputComponent->BindAction("Reload", IE_Pressed, this, &AFPSCppCharacter::Reload);

	PlayerInputComponent->BindAction("Grenade", IE_Pressed, this, &AFPSCppCharacter::Grenade);

	PlayerInputComponent->BindAction("Walk", IE_Pressed, this, &AFPSCppCharacter::Walk);
	PlayerInputComponent->BindAction("Walk", IE_Released, this, &AFPSCppCharacter::StopWalk);

	PlayerInputComponent->BindAction("Crouch", IE_Pressed, this, &AFPSCppCharacter::OnCrouch);
	PlayerInputComponent->BindAction("Crouch", IE_Released, this, &AFPSCppCharacter::StopCrouch);

	PlayerInputComponent->BindAction("ZoomIn", IE_Pressed, this, &AFPSCppCharacter::OnZoom);
	PlayerInputComponent->BindAction("ZoomIn", IE_Released, this, &AFPSCppCharacter::StopZoom);

	// PlayerInputComponent->BindAction("Run", IE_Pressed, this, &AFPSCppCharacter::Run);
	// PlayerInputComponent->BindAction("Run", IE_Released, this, &AFPSCppCharacter::StopRun);


	PlayerInputComponent->BindAxis("MoveForward", this, &AFPSCppCharacter::MoveForward);
	PlayerInputComponent->BindAxis("MoveRight", this, &AFPSCppCharacter::MoveRight);
	
	PlayerInputComponent->BindAxis("Turn", this, &APawn::AddControllerYawInput);
	PlayerInputComponent->BindAxis("TurnRate", this, &AFPSCppCharacter::TurnAtRate);
	PlayerInputComponent->BindAxis("LookUp", this, &APawn::AddControllerPitchInput);
	PlayerInputComponent->BindAxis("LookUpRate", this, &AFPSCppCharacter::LookUpAtRate);
}

void AFPSCppCharacter::OnFire()
{

	if (!bAbleToFire)
		return;
	
	//服务器RPC
	Server_OnFire();
	
	//后座
	if(bIsZooming)
	{
		AddControllerPitchInput(FMath::RandRange(-0.2f,0.f));
		AddControllerYawInput(FMath::RandRange(-0.1f,0.1f));
		
	}
	else
	{
		AddControllerPitchInput(FMath::RandRange(-0.5f,0.f));
		AddControllerYawInput(FMath::RandRange(-0.3f,0.3f));
	}
	
	
	Cast<APlayerController>(GetController())->ClientStartCameraShake(CameraShake);

	CurrentAmmo -= 1;

	if (CurrentAmmo == 0)
	{
		Reload();
		/*此处执行Reload，Montage不播放*/
		MontageEvent();
	}
}


void AFPSCppCharacter::Server_OnFire_Implementation()
{
	
	Multicast_Fire();
	
	// 移动时弹道偏移
	float AimOffSet = FireOffset();
	
	FVector HitLocationOffset = 0.02 * FVector(FMath::RandRange(-AimOffSet, AimOffSet),
											   FMath::RandRange(-AimOffSet, AimOffSet),
											   FMath::RandRange(-AimOffSet, AimOffSet));
	
	FHitResult HitResult;
	FVector Start = MainCamera->GetComponentLocation();
	FVector End = MainCamera->GetComponentLocation() + (Cast<AActor>(GetController())->GetActorForwardVector()+ HitLocationOffset).
		GetSafeNormal() * ShootingDistance;
	TArray<AActor*> TraceIgnore;
	TraceIgnore.Add(Cast<AActor>(this));
    UKismetSystemLibrary::LineTraceSingle(GetWorld(),Start,End,ETraceTypeQuery::TraceTypeQuery1,true,TraceIgnore,EDrawDebugTrace::ForDuration,HitResult,true);
	// GetWorld()->LineTraceSingleByChannel(HitResult, Start, End, ECollisionChannel::ECC_Camera);
	if (HitResult.GetComponent())
	{
		AActor* HittedActor=HitResult.GetActor();
		UPrimitiveComponent* HittedComponent=HitResult.GetComponent();
		float HitDistence;
		HitDistence=(HitResult.ImpactPoint-GetActorLocation()).Size();
		
		//距离衰减
		float DamageReduction;
		if(HitDistence<2000)
		{
			DamageReduction=1.f;
		}
		else
		{
			DamageReduction=(10000.f-HitDistence)/10000.f;
		}
		
		if (HittedComponent->IsSimulatingPhysics())
		{
			float PointImpulse;
			PointImpulse = HitImpulse * (ShootingDistance - (HitResult.ImpactPoint - GetActorLocation()).Size()) / ShootingDistance;
			HittedComponent->AddImpulseAtLocation((End - Start).GetSafeNormal() * PointImpulse,
														   GetActorLocation());
		}
		
		//存在生命组件
		TSubclassOf<class UHealthComponent> HealthClass=UHealthComponent::StaticClass();
		UActorComponent* GettedComponent;
		GettedComponent=HittedActor->GetComponentByClass(HealthClass);
		
		if(GettedComponent)
		{
			UHealthComponent* HealthComponent=Cast<UHealthComponent>(GettedComponent);
			if(HitResult.BoneName=="head")
			{
				DamageValue=50.f*DamageReduction;
				HealthComponent->ChangeHealth(DamageValue);
			}
			else
			{
				DamageValue=10.f*DamageReduction;
				HealthComponent->ChangeHealth(DamageValue);
			}
			ReturnDamage(DamageValue);
		}
		Multicast_HitParticle(HitResult.ImpactPoint);
	}
}


void AFPSCppCharacter::Multicast_HitParticle_Implementation(const FVector &HitLocation)
{
	
	UGameplayStatics::SpawnEmitterAtLocation(GetWorld(), HittedParticle, HitLocation,
												 FRotator::ZeroRotator, FVector(.2f));
}


void AFPSCppCharacter::Multicast_Fire_Implementation()
{
	if (GetCharacterMovement()->Velocity.Size() <= 300)
	{
		if (Ironsights_FireMontage)
		{
			PlayAnimMontage(Ironsights_FireMontage);
		}
	}
	else if (GetCharacterMovement()->Velocity.Size() > 300)
	{
		if (Hip_FireMontage)
		{
			PlayAnimMontage(Hip_FireMontage);
		}
	}
	
	if (FireSound != nullptr)
	{
		UGameplayStatics::PlaySoundAtLocation(this, FireSound, GetActorLocation());
	}

	if(ShootParticle!=nullptr)
	{
		UGameplayStatics::SpawnEmitterAttached(ShootParticle, MuzzleLocation, "Mozzle", FVector(0.f),
											   FRotator::ZeroRotator, FVector(.1f));
	}
}


void AFPSCppCharacter::StopFire()
{
	bIsFiring = false;
}


void AFPSCppCharacter::MoveForward(float Value)
{
	if (Value != 0.0f)
	{
		AddMovementInput(GetActorForwardVector(), Value);
	}
}

void AFPSCppCharacter::MoveRight(float Value)
{
	if (Value != 0.0f)
	{
		AddMovementInput(GetActorRightVector(), Value);
	}
}

void AFPSCppCharacter::TurnAtRate(float Rate)
{
	// calculate delta for this frame from the rate information
	AddControllerYawInput(Rate * BaseTurnRate * GetWorld()->GetDeltaSeconds());
}

void AFPSCppCharacter::LookUpAtRate(float Rate)
{
	// calculate delta for this frame from the rate information
	AddControllerPitchInput(Rate * BaseLookUpRate * GetWorld()->GetDeltaSeconds());
}

void AFPSCppCharacter::Tick(float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);
}


/*换弹*/
void AFPSCppCharacter::Reload()
{
	if (FullAmmo == 0)
	{
		bAbleToFire = false;
	}
	else if (FullAmmo != 0 && !bIsReloading && PerAmmo != CurrentAmmo)
	{
		if (ReloadMontage)
		{
			PlayAnimMontage(ReloadMontage);
		}
		Server_Reload();
		

		bAbleToFire = false;
		bIsReloading = true;

		GetCharacterMovement()->MaxWalkSpeed = 270;
		GetWorldTimerManager().SetTimer(ReloadTimerHandle, this, &AFPSCppCharacter::ReloadFinish, 2.f, false);

		if (CurrentAmmo + FullAmmo < PerAmmo)
		{
			CurrentAmmo += FullAmmo;
			FullAmmo = 0;
		}
		else
		{
			FullAmmo = FullAmmo - PerAmmo + CurrentAmmo;
			CurrentAmmo = PerAmmo;
		}
	}
}


void AFPSCppCharacter::ReloadFinish()
{
	Server_ReloadFinish();
	if (!bIsCrouching&&!bIsZooming)
	{
		GetCharacterMovement()->MaxWalkSpeed = 600;
	}
	bAbleToFire = true;
	bIsReloading = false;
}

void AFPSCppCharacter::Server_Reload_Implementation()
{
	Multicast_Reload();

	bAbleToFire = false;
	bIsReloading = true;

	GetCharacterMovement()->MaxWalkSpeed = 270;
	GetWorldTimerManager().SetTimer(ReloadTimerHandle, this, &AFPSCppCharacter::ReloadFinish, 2.f, false);
	bIsReloading=true;
	
}

void AFPSCppCharacter::Multicast_Reload_Implementation()
{
	if (ReloadMontage)
	{
		PlayAnimMontage(ReloadMontage);
	}
}

void AFPSCppCharacter::Server_ReloadFinish_Implementation()
{
	if (!bIsCrouching&&!bIsZooming)
	{
		GetCharacterMovement()->MaxWalkSpeed = 600;
	}
	bAbleToFire = true;
	bIsReloading = false;
}

/*手雷*/
void AFPSCppCharacter::Grenade()
{
	if (!bAbleToUseGrenade||GrenadeCount == 0)
	{
		return;
	}
	
	Server_Grenade();
	
	GrenadeCount--;
	bAbleToUseGrenade=false;
	GetWorldTimerManager().SetTimer(GrenadeCoolDownTimerHandle,this,&AFPSCppCharacter::GrenadeCoolDown,5.f,false);
	
}

void AFPSCppCharacter::Server_Grenade_Implementation()
{
	if (GrenadeClass != nullptr)
	{
		UWorld* const World = GetWorld();
		if (World != nullptr)
		{
			const FRotator SpawnRotation = GetControlRotation();
			const FVector SpawnLocation = GrenadeLocation->GetComponentLocation();
			
			FActorSpawnParameters ActorSpawnParams;
			ActorSpawnParams.SpawnCollisionHandlingOverride =
				ESpawnActorCollisionHandlingMethod::AdjustIfPossibleButDontSpawnIfColliding;
			
			AGrenade* Grenade = World->SpawnActor<AGrenade>(GrenadeClass, SpawnLocation, SpawnRotation,
															ActorSpawnParams);
			
			if (Grenade)
			{
				Grenade->GetSphereComponent()->AddImpulse(MainCamera->GetForwardVector() * 30000.f);
			}
		}
	}
	
	// Multicast_Grenade();
	
}


void AFPSCppCharacter::GrenadeCoolDown()
{
	bAbleToUseGrenade=true;
}

/*静步*/
void AFPSCppCharacter::Walk()
{
	Server_Walk();
	GetCharacterMovement()->MaxWalkSpeed = 270;
}

void AFPSCppCharacter::StopWalk()
{
	Server_StopWalk();
	GetCharacterMovement()->MaxWalkSpeed = 600;
}

void AFPSCppCharacter::Server_Walk_Implementation()
{
	GetCharacterMovement()->MaxWalkSpeed = 270;
}

void AFPSCppCharacter::Server_StopWalk_Implementation()
{
	GetCharacterMovement()->MaxWalkSpeed = 600;
}

/*蹲*/
void AFPSCppCharacter::OnCrouch()
{
	Server_OnCrouch();
	if (bAbleToCrouch)
	{
		GetCharacterMovement()->MaxWalkSpeed = 270;
		CameraSpringArm->AddLocalOffset(FVector(0.f, 0.f, -40.f));
		bIsCrouching = true;
	}
}

void AFPSCppCharacter::Server_OnCrouch_Implementation()
{
	if (bAbleToCrouch)
	{
		GetCharacterMovement()->MaxWalkSpeed = 270;
		CameraSpringArm->AddLocalOffset(FVector(0.f, 0.f, -40.f));
		bIsCrouching = true;
	}
}

void AFPSCppCharacter::StopCrouch()
{
	Server_StopCrouch();
	if (!bIsZooming)
	{
		GetCharacterMovement()->MaxWalkSpeed = 600;
	}
	CameraSpringArm->AddLocalOffset(FVector(0.f, 0.f, 40.f));
	bIsCrouching = false;
}

void AFPSCppCharacter::Server_StopCrouch_Implementation()
{
	if (!bIsZooming)
	{
		GetCharacterMovement()->MaxWalkSpeed = 600;
	}
	CameraSpringArm->AddLocalOffset(FVector(0.f, 0.f, 40.f));
	bIsCrouching = false;
}

/*ADS*/
void AFPSCppCharacter::OnZoom()
{
	Server_OnZoom();
	if (bAbleToZoomIn && ZoomInCamera != nullptr)
	{
		MainCamera = ZoomInCamera;
		bIsZooming = true;
		GetCharacterMovement()->MaxWalkSpeed = 270;
		ZoomInCamera->Activate();
		TPSCameraComponent->Deactivate();
	}
}

void AFPSCppCharacter::StopZoom()
{
	Server_StopZoom();
	if (bAbleToZoomIn && TPSCameraComponent != nullptr)
	{
		
		MainCamera = TPSCameraComponent;
		bIsZooming = false;
		if(!bIsReloading&&!bIsCrouching)
		{
			GetCharacterMovement()->MaxWalkSpeed = 600;
		}
		TPSCameraComponent->Activate();
	}
}

void AFPSCppCharacter::Server_OnZoom_Implementation()
{
	if (ZoomInCamera != nullptr)
	{
		MainCamera = ZoomInCamera;
		bIsZooming = true;
		GetCharacterMovement()->MaxWalkSpeed = 270;
	}
}

void AFPSCppCharacter::Server_StopZoom_Implementation()
{
	if (TPSCameraComponent != nullptr)
	{
		MainCamera = TPSCameraComponent;
		bIsZooming = false;
		if(!bIsReloading&&!bIsCrouching)
		{
			GetCharacterMovement()->MaxWalkSpeed = 600;
		}
	}
}

/*冲刺*/
void AFPSCppCharacter::Run()
{
	if (bAbleToRun)
	{
		bAbleToCrouch = false;
		bAbleToFire = false;
		bIsRunning = true;
		GetCharacterMovement()->MaxWalkSpeed = 900;
	}
}

void AFPSCppCharacter::StopRun()
{
	bAbleToCrouch = true;
	bAbleToFire = true;
	bIsRunning = false;
	GetCharacterMovement()->MaxWalkSpeed = 600;
}

//准星偏移
float AFPSCppCharacter::FireOffset()
{
	return 0.2f+GetVelocity().Size() / 300.f;
}

void AFPSCppCharacter::MontageEvent_Implementation()
{
}

void AFPSCppCharacter::ReturnDamage_Implementation(float Damage)
{
	
}


float AFPSCppCharacter::TakeDamage(float DamageAmount, FDamageEvent const& DamageEvent, AController* EventInstigator,
                                   AActor* DamageCauser)
{
	return Super::TakeDamage(DamageAmount, DamageEvent, EventInstigator, DamageCauser);
}
