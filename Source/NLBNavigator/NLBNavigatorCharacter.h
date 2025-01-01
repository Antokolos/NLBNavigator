// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Character.h"
#include "Logging/LogMacros.h"
#include "NLBNavigatorCharacter.generated.h"

class UInputComponent;
class USkeletalMeshComponent;
class UCameraComponent;
class UInputAction;
class UInputMappingContext;
class UEnhancedInputLocalPlayerSubsystem;
class ANLBController;
struct FInputActionValue;

DECLARE_LOG_CATEGORY_EXTERN(LogTemplateCharacter, Log, All);

UCLASS(config=Game)
class ANLBNavigatorCharacter : public ACharacter
{
	GENERATED_BODY()

	/** Pawn mesh: 1st person view (arms; seen only by self) */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category=Mesh, meta = (AllowPrivateAccess = "true"))
	USkeletalMeshComponent* Mesh1P;

	/** First person camera */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = Camera, meta = (AllowPrivateAccess = "true"))
	UCameraComponent* FirstPersonCameraComponent;

	/** MappingContext */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = Input, meta = (AllowPrivateAccess = "true"))
	UInputMappingContext* DefaultMappingContext;

	UInputMappingContext* NLBMappingContext;

	/** Jump Input Action */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category=Input, meta=(AllowPrivateAccess = "true"))
	UInputAction* JumpAction;

	/** Move Input Action */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category=Input, meta=(AllowPrivateAccess = "true"))
	UInputAction* MoveAction;

	/** Look Input Action */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = Input, meta = (AllowPrivateAccess = "true"))
	class UInputAction* LookAction;
	
public:
	ANLBNavigatorCharacter();

protected:
	/** Called for movement input */
	void Move(const FInputActionValue& Value);

	/** Called for looking input */
	void Look(const FInputActionValue& Value);

protected:
	// APawn interface
	virtual void NotifyControllerChanged() override;
	virtual void SetupPlayerInputComponent(UInputComponent* InputComponent) override;
	// End of APawn interface

private:
	// Declare the InputAction as private, but accessible via a getter function
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Input", meta = (AllowPrivateAccess = "true"))
	UInputAction* SwitchToVisualNovelAction; // Use a raw pointer here

	// Declare the InputAction as private, but accessible via a getter function
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Input", meta = (AllowPrivateAccess = "true"))
	UInputAction* NextPageAction; // Use a raw pointer here

	ANLBController* VisualNovelController; // Pointer to NLBController

	//void LoadSwitchToVisualNovelAction();
	void ActivateVisualNovelInput();
	void DeactivateVisualNovelInput();
	void DisableCharacterInput();
	void EnableCharacterInput();
	UEnhancedInputLocalPlayerSubsystem* GetEnhancedInputSubsystem() const;
	void SwitchToVisualNovel(); // Function to switch to visual novel mode
	void ReturnToShooter();     // Function to return to shooter mode
	void NextPage(); // Turn the page

	bool bIsInVisualNovelMode; // Flag to track the current mode

public:
	/** Returns Mesh1P subobject **/
	USkeletalMeshComponent* GetMesh1P() const { return Mesh1P; }
	/** Returns FirstPersonCameraComponent subobject **/
	UCameraComponent* GetFirstPersonCameraComponent() const { return FirstPersonCameraComponent; }
	// Getter for SwitchToVisualNovelAction to make it accessible in Blueprints
	UFUNCTION(BlueprintCallable, Category = "Input")
	UInputAction* GetSwitchToVisualNovelAction() const { return SwitchToVisualNovelAction; }
	// Getter for NextPageAction to make it accessible in Blueprints
    UFUNCTION(BlueprintCallable, Category = "VisualNovel")
    bool GetIsInVisualNovelMode() const { return bIsInVisualNovelMode; }
};

