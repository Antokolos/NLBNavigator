// Предмет, который можно взять в руку и рассмотреть (из NLBVR: AInteractiveCube). В VR — кнопкой
// Grip рядом с предметом; отпущенный падает с физикой.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "NLBGrabbableActor.generated.h"

class UStaticMeshComponent;
class UBoxComponent;

UCLASS()
class NLBNAVIGATOR_API ANLBGrabbableActor : public AActor
{
	GENERATED_BODY()

public:
	ANLBGrabbableActor();

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Components")
	TObjectPtr<UStaticMeshComponent> Mesh;

	/** Область, в которой рука может взять предмет */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Components")
	TObjectPtr<UBoxComponent> GrabZone;

	UFUNCTION(BlueprintCallable, Category = "Interaction")
	void Grab(USceneComponent* AttachTo);

	UFUNCTION(BlueprintCallable, Category = "Interaction")
	void Release();

	UPROPERTY(BlueprintReadOnly, Category = "Interaction")
	bool bIsGrabbed = false;
};
