// Copyright 1998-2019 Epic Games, Inc. All Rights Reserved.

#include "BVCharacter.h"
#include "HeadMountedDisplayFunctionLibrary.h"
#include "Camera/CameraComponent.h"
#include "Components/CapsuleComponent.h"
#include "Components/InputComponent.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "GameFramework/Controller.h"
#include "GameFramework/PlayerController.h"
#include "GameFramework/SpringArmComponent.h"

ABVCharacter::ABVCharacter()
{
	GetCapsuleComponent()->InitCapsuleSize(42.f, 96.0f);

	BaseTurnRate = 45.f;
	BaseLookUpRate = 45.f;

	bUseControllerRotationPitch = false;
	bUseControllerRotationYaw = false;
	bUseControllerRotationRoll = false;

	GetCharacterMovement()->bOrientRotationToMovement = true; 	
	GetCharacterMovement()->RotationRate = FRotator(0.0f, 540.0f, 0.0f);
	GetCharacterMovement()->JumpZVelocity = 600.f;
	GetCharacterMovement()->AirControl = 0.2f;

	CameraBoom = CreateDefaultSubobject<USpringArmComponent>(TEXT("CameraBoom"));
	CameraBoom->SetupAttachment(RootComponent);
	CameraBoom->TargetArmLength = 300.0f; 
	CameraBoom->bUsePawnControlRotation = true; 

	FollowCamera = CreateDefaultSubobject<UCameraComponent>(TEXT("FollowCamera"));
	FollowCamera->SetupAttachment(CameraBoom, USpringArmComponent::SocketName);
	FollowCamera->bUsePawnControlRotation = false; 
}

void ABVCharacter::BeginPlay()
{
	Super::BeginPlay();
	OriginRunSpeed = GetCharacterMovement()->MaxWalkSpeed;
	RelativeFollowCameraLocation = FollowCamera->RelativeLocation;
	RelativeZoomOutFollowCameraLocation = RelativeFollowCameraLocation - FollowCamera->RelativeRotation.Vector().GetSafeNormal() * ZoomOutLenght;
	PlayerController = CastChecked<APlayerController>(Controller);
	ControllerOriginRotation = Controller->GetControlRotation();
	ControllerTargetRotation = ControllerOriginRotation;
}

void ABVCharacter::SetupPlayerInputComponent(class UInputComponent* PlayerInputComponent)
{
	check(PlayerInputComponent);
	PlayerInputComponent->BindAction("Jump", IE_Pressed, this, &ACharacter::Jump);
	PlayerInputComponent->BindAction("Jump", IE_Released, this, &ACharacter::StopJumping);
	PlayerInputComponent->BindAction("ZoomOut", IE_Pressed, this, &ABVCharacter::ZoomOut);
	PlayerInputComponent->BindAction("ZoomOut", IE_Released, this, &ABVCharacter::ZoomIn);
	PlayerInputComponent->BindAction("Run", IE_Pressed, this, &ABVCharacter::RunOn);
	PlayerInputComponent->BindAction("Run", IE_Released, this, &ABVCharacter::RunOff);
	PlayerInputComponent->BindAction("ResetCameraRotation", IE_Pressed, this, &ABVCharacter::ResetCameraRotation);

	PlayerInputComponent->BindAxis("MoveForward", this, &ABVCharacter::MoveForward);
	PlayerInputComponent->BindAxis("MoveRight", this, &ABVCharacter::MoveRight);

	PlayerInputComponent->BindAxis("Turn", this, &ABVCharacter::AddControllerYawInput);
	PlayerInputComponent->BindAxis("TurnRate", this, &ABVCharacter::TurnAtRate);
	PlayerInputComponent->BindAxis("LookUp", this, &ABVCharacter::AddControllerPitchInput);
	PlayerInputComponent->BindAxis("LookUpRate", this, &ABVCharacter::LookUpAtRate);

	PlayerInputComponent->BindTouch(IE_Pressed, this, &ABVCharacter::TouchStarted);
	PlayerInputComponent->BindTouch(IE_Released, this, &ABVCharacter::TouchStopped);
	PlayerInputComponent->BindAction("ResetVR", IE_Pressed, this, &ABVCharacter::OnResetVR);
}

void ABVCharacter::OnResetVR()
{
	UHeadMountedDisplayFunctionLibrary::ResetOrientationAndPosition();
}

void ABVCharacter::ResetCameraRotation()
{
	ControllerTargetRotation = ControllerOriginRotation;
}

void ABVCharacter::TouchStarted(ETouchIndex::Type FingerIndex, FVector Location)
{
	Jump();
}

void ABVCharacter::TouchStopped(ETouchIndex::Type FingerIndex, FVector Location)
{
	StopJumping();
}

void ABVCharacter::AddControllerYawInput(float InValue)
{
	ControllerTargetRotation.Yaw += InValue * PlayerController->InputYawScale;
	Super::AddControllerYawInput(InValue);
}

void ABVCharacter::AddControllerPitchInput(float InValue)
{
	FRotator NewRotator = ControllerTargetRotation;
	NewRotator.Pitch += InValue * PlayerController->InputPitchScale;
	float CurrentCameraZValue = FMath::Abs(NewRotator.Vector().GetSafeNormal().Z);

	if (CurrentCameraZValue < MaxPitchAngle / 90.f) 
	{
		ControllerTargetRotation.Pitch = NewRotator.Pitch;
		Super::AddControllerPitchInput(InValue);
	}
}

void ABVCharacter::TurnAtRate(float Rate)
{
	AddControllerYawInput(Rate * BaseTurnRate * GetWorld()->GetDeltaSeconds());
}

void ABVCharacter::LookUpAtRate(float Rate)
{
	AddControllerPitchInput(Rate * BaseLookUpRate * GetWorld()->GetDeltaSeconds());
}

void ABVCharacter::MoveForward(float Value)
{
	if ((Controller != NULL) && (Value != 0.0f))
	{
		const FRotator Rotation = Controller->GetControlRotation();
		const FRotator YawRotation(0, Rotation.Yaw, 0);

		const FVector Direction = FRotationMatrix(YawRotation).GetUnitAxis(EAxis::X);
		AddMovementInput(Direction, Value);
	}
}

void ABVCharacter::MoveRight(float Value)
{
	if ( (Controller != NULL) && (Value != 0.0f) )
	{
		const FRotator Rotation = Controller->GetControlRotation();
		const FRotator YawRotation(0, Rotation.Yaw, 0);
	
		const FVector Direction = FRotationMatrix(YawRotation).GetUnitAxis(EAxis::Y);
		AddMovementInput(Direction, Value);
	}
}

void ABVCharacter::RunOn()
{
	bIsWalkState = true;
	GetCharacterMovement()->MaxWalkSpeed = OriginRunSpeed * RunSpeedMultiplier;
}

void ABVCharacter::RunOff()
{
	bIsWalkState = false;
	GetCharacterMovement()->MaxWalkSpeed = OriginRunSpeed;
}

void ABVCharacter::ZoomIn()
{
	bIsZoomOut = false;
}

void ABVCharacter::ZoomOut() 
{
	bIsZoomOut = true;
}

void ABVCharacter::ExecuteZoom(float DeltaTime)
{
	FVector AdditiveVector = Controller->GetControlRotation().Vector().GetSafeNormal() * ZoomOutLenght;
	FVector TargetCameraLocation = bIsZoomOut ? RelativeZoomOutFollowCameraLocation : RelativeFollowCameraLocation;
	FVector InterpedLocation = FMath::VInterpTo(FollowCamera->RelativeLocation, TargetCameraLocation, DeltaTime, ZoomInterpSpeed);
	FollowCamera->RelativeLocation = InterpedLocation;
}

void ABVCharacter::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);
	ExecuteZoom(DeltaTime);

	FRotator InterpedLocation = FMath::RInterpTo(Controller->GetControlRotation(), ControllerTargetRotation, DeltaTime, ZoomInterpSpeed);
	Controller->SetControlRotation(InterpedLocation);
}
