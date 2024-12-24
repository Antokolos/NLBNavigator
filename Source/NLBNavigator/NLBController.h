// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "NLBController.generated.h"

UCLASS()
class NLBNAVIGATOR_API ANLBController : public AActor
{
	GENERATED_BODY()

private:
	UPROPERTY()
	class UUserWidget* NLBWidget;

	UPROPERTY()
	class ACameraActor* GameplayCamera;

	bool bIsImageVisible;

public:	
	// Sets default values for this actor's properties
	ANLBController();

	UFUNCTION()
	void ToggleView();

protected:
	// Called when the game starts or when spawned
	virtual void BeginPlay() override;

public:	
	// Called every frame
	virtual void Tick(float DeltaTime) override;

	
	
};
