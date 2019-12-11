// Copyright 1998-2019 Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Character.h"
#include "BVCharacter.generated.h"

UCLASS(config=Game)
class ABVCharacter : public ACharacter
{
	GENERATED_BODY()

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = Camera, meta = (AllowPrivateAccess = "true"))
	class USpringArmComponent* CameraBoom;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = Camera, meta = (AllowPrivateAccess = "true"))
	class UCameraComponent* FollowCamera;
public:
	ABVCharacter();

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = Camera)
	float RunSpeedMultiplier = 1.5f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = Camera)
	float ZoomInterpSpeed = 1.0f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = Camera)
	float ZoomOutLenght = 5.0f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = Camera)
	float MaxPitchAngle = 30.0f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category=Camera)
	float BaseTurnRate;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category=Camera)
	float BaseLookUpRate;

protected:

	void OnResetVR();

	void MoveForward(float Value);

	void MoveRight(float Value);

	void TurnAtRate(float Rate);

	void LookUpAtRate(float Rate);

	void ResetCameraRotation();

	void ZoomOut();

	void ZoomIn();

	void RunOn();

	void RunOff();

	void ExecuteZoom(float DeltaTime);

	void TouchStarted(ETouchIndex::Type FingerIndex, FVector Location);

	void TouchStopped(ETouchIndex::Type FingerIndex, FVector Location);

protected:
	virtual void SetupPlayerInputComponent(class UInputComponent* PlayerInputComponent) override;

	virtual void BeginPlay() override;
	
	virtual void AddControllerPitchInput(float InValue) override;

	virtual void AddControllerYawInput(float InValue) override;

	virtual void Tick(float DeltaTime) override;

	uint32 bIsWalkState : 1;

private:
	FVector RelativeFollowCameraLocation;

	FRotator ControllerOriginRotation;
	
	FRotator ControllerTargetRotation;

	FVector RelativeZoomOutFollowCameraLocation;

	APlayerController* PlayerController;

	float OriginRunSpeed;

	bool bIsZoomOut;

public:
	FORCEINLINE class USpringArmComponent* GetCameraBoom() const { return CameraBoom; }

	FORCEINLINE class UCameraComponent* GetFollowCamera() const { return FollowCamera; }
};

