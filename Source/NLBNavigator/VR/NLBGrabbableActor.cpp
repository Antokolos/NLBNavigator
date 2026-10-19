#include "NLBGrabbableActor.h"

#include "Components/BoxComponent.h"
#include "Components/StaticMeshComponent.h"
#include "UObject/ConstructorHelpers.h"

ANLBGrabbableActor::ANLBGrabbableActor()
{
	PrimaryActorTick.bCanEverTick = false;
	Mesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("Mesh"));
	RootComponent = Mesh;
	static ConstructorHelpers::FObjectFinder<UStaticMesh> Cube(TEXT("/Engine/BasicShapes/Cube.Cube"));
	if (Cube.Succeeded())
	{
		Mesh->SetStaticMesh(Cube.Object);
		Mesh->SetRelativeScale3D(FVector(0.2f));
	}
	Mesh->SetSimulatePhysics(true);
	Mesh->SetEnableGravity(true);
	Mesh->SetCollisionProfileName(TEXT("PhysicsActor"));

	GrabZone = CreateDefaultSubobject<UBoxComponent>(TEXT("GrabZone"));
	GrabZone->SetupAttachment(Mesh);
	GrabZone->SetBoxExtent(FVector(70.0f));  // в единицах меша (масштаб 0.2) — около 14 см
	GrabZone->SetCollisionProfileName(TEXT("OverlapAll"));
}

void ANLBGrabbableActor::Grab(USceneComponent* AttachTo)
{
	if (!AttachTo || bIsGrabbed)
	{
		return;
	}
	Mesh->SetSimulatePhysics(false);
	Mesh->AttachToComponent(AttachTo, FAttachmentTransformRules::KeepWorldTransform);
	bIsGrabbed = true;
}

void ANLBGrabbableActor::Release()
{
	if (!bIsGrabbed)
	{
		return;
	}
	Mesh->DetachFromComponent(FDetachmentTransformRules::KeepWorldTransform);
	Mesh->SetSimulatePhysics(true);
	bIsGrabbed = false;
}
