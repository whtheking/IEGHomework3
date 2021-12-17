// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "HealthComponent.generated.h"


UCLASS( ClassGroup=(Custom), meta=(BlueprintSpawnableComponent) )
class FPSCPP_API UHealthComponent : public UActorComponent
{
	GENERATED_BODY()

public:	
	// Sets default values for this component's properties
	UHealthComponent();

	UPROPERTY(Replicated,EditAnywhere,BlueprintReadWrite,Category=Heaalth)
	float FullHealth;
	
	UPROPERTY(Replicated,EditAnywhere,BlueprintReadWrite,Category=Heaalth)
	float CurrentHealth;
	
	UPROPERTY(Replicated,EditAnywhere,BlueprintReadWrite,Category=Heaalth)
	float FullShield;

	UPROPERTY(Replicated,EditAnywhere,BlueprintReadWrite,Category=Heaalth)
	float CurrentShield;

	UPROPERTY(Replicated,EditAnywhere,BlueprintReadWrite,Category=Heaalth)
	bool bShieldActive;

protected:
	// Called when the game starts
	virtual void BeginPlay() override;


public:	
	// Called every frame
	virtual void TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction) override;

	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;

	
	UFUNCTION(Server,Reliable,BlueprintCallable)
	void ChangeHealth(float ChangeCount);
	
	UFUNCTION(Server,Reliable,BlueprintCallable)
	void Die();
	
};

