// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "NLBController.generated.h"

class UCameraComponent;

UCLASS()
class NLBNAVIGATOR_API ANLBController : public AActor
{
	GENERATED_BODY()
	
public:	
	// Sets default values for this actor's properties
	ANLBController();

protected:
	// Called when the game starts or when spawned
	virtual void BeginPlay() override;

public:	
	// Called every frame
	virtual void Tick(float DeltaTime) override;
	
	/** Loads the current set of text and image files */
	void LoadCurrentSet();

	/** Loads the next set of text and image files */
	void LoadNextSet();

	UFUNCTION(BlueprintCallable, Category = "Camera")
	UCameraComponent* GetCameraComponent() const { return CameraComponent; }
	
	void AddToViewport();
	void RemoveFromViewport();

private:
	/** Utility function to load a texture from a file */
	UTexture2D* LoadTextureFromFile(const FString& FilePath);

	UPROPERTY()
	class UNLBWidget* NLBWidget;

	UPROPERTY()
	class UTexture2D* CurrentTexture;


	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Camera", meta = (AllowPrivateAccess = "true"))
	UCameraComponent* CameraComponent;

	int32 CurrentIndex;
	TArray<FString> TextFiles;
	TArray<FString> ImageFiles;
	
};
