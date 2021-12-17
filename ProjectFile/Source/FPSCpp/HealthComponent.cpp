// Fill out your copyright notice in the Description page of Project Settings.


#include "HealthComponent.h"

#include "GameFramework/Character.h"
#include "Net/UnrealNetwork.h"

// Sets default values for this component's properties
UHealthComponent::UHealthComponent()
{
	// Set this component to be initialized when the game starts, and to be ticked every frame.  You can turn these features
	// off to improve performance if you don't need them.
	PrimaryComponentTick.bCanEverTick = true;
	
	FullHealth=100;
	FullShield=100;
	CurrentHealth=100;
	CurrentShield=100;
	bShieldActive=false;

}


// Called when the game starts
void UHealthComponent::BeginPlay()
{
	Super::BeginPlay();
}


// Called every frame
void UHealthComponent::TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction)
{
	Super::TickComponent(DeltaTime, TickType, ThisTickFunction);

	// ...
}

void UHealthComponent::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);

	DOREPLIFETIME(UHealthComponent,FullHealth);
	DOREPLIFETIME(UHealthComponent,CurrentHealth);
	DOREPLIFETIME(UHealthComponent,FullShield);
	DOREPLIFETIME(UHealthComponent,CurrentHealth);
	DOREPLIFETIME(UHealthComponent,bShieldActive);

}



void UHealthComponent::ChangeHealth_Implementation(float ChangeCount)
{
	CurrentHealth-=ChangeCount;
	if(CurrentHealth<=0)
	{
		Die();
	}
}


void UHealthComponent::Die_Implementation()
{
	GetOwner()->Destroy();
}

