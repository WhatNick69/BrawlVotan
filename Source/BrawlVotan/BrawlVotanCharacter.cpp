// Copyright 1998-2019 Epic Games, Inc. All Rights Reserved.

#include "BrawlVotanCharacter.h"
#include "HeadMountedDisplayFunctionLibrary.h"
#include "Camera/CameraComponent.h"
#include "Components/CapsuleComponent.h"
#include "Components/InputComponent.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "GameFramework/Controller.h"
#include "GameFramework/PlayerController.h"
#include "GameFramework/SpringArmComponent.h"

ABrawlVotanCharacter::ABrawlVotanCharacter()
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

void ABrawlVotanCharacter::BeginPlay()
{
	Super::BeginPlay();
	RelativeFollowCameraLocation = FollowCamera->RelativeLocation;
	RelativeZoomOutFollowCameraLocation = RelativeFollowCameraLocation - FollowCamera->RelativeRotation.Vector().GetSafeNormal() * ZoomOutLenght;
	PlayerController = CastChecked<APlayerController>(Controller);
	ControllerOriginRotation = Controller->GetControlRotation();
	ControllerTargetRotation = ControllerOriginRotation;
}

void ABrawlVotanCharacter::SetupPlayerInputComponent(class UInputComponent* PlayerInputComponent)
{
	check(PlayerInputComponent);
	PlayerInputComponent->BindAction("Jump", IE_Pressed, this, &ACharacter::Jump);
	PlayerInputComponent->BindAction("Jump", IE_Released, this, &ACharacter::StopJumping);
	PlayerInputComponent->BindAction("ZoomOut", IE_Pressed, this, &ABrawlVotanCharacter::ZoomOut);
	PlayerInputComponent->BindAction("ZoomOut", IE_Released, this, &ABrawlVotanCharacter::ZoomIn);
	PlayerInputComponent->BindAction("ResetCameraRotation", IE_Pressed, this, &ABrawlVotanCharacter::ResetCameraRotation);

	PlayerInputComponent->BindAxis("MoveForward", this, &ABrawlVotanCharacter::MoveForward);
	PlayerInputComponent->BindAxis("MoveRight", this, &ABrawlVotanCharacter::MoveRight);

	PlayerInputComponent->BindAxis("Turn", this, &ABrawlVotanCharacter::AddControllerYawInput);
	PlayerInputComponent->BindAxis("TurnRate", this, &ABrawlVotanCharacter::TurnAtRate);
	PlayerInputComponent->BindAxis("LookUp", this, &ABrawlVotanCharacter::AddControllerPitchInput);
	PlayerInputComponent->BindAxis("LookUpRate", this, &ABrawlVotanCharacter::LookUpAtRate);

	PlayerInputComponent->BindTouch(IE_Pressed, this, &ABrawlVotanCharacter::TouchStarted);
	PlayerInputComponent->BindTouch(IE_Released, this, &ABrawlVotanCharacter::TouchStopped);
	PlayerInputComponent->BindAction("ResetVR", IE_Pressed, this, &ABrawlVotanCharacter::OnResetVR);
}

void ABrawlVotanCharacter::OnResetVR()
{
	UHeadMountedDisplayFunctionLibrary::ResetOrientationAndPosition();
}

void ABrawlVotanCharacter::ResetCameraRotation()
{
	ControllerTargetRotation = ControllerOriginRotation;
}

void ABrawlVotanCharacter::TouchStarted(ETouchIndex::Type FingerIndex, FVector Location)
{
	Jump();
}

void ABrawlVotanCharacter::TouchStopped(ETouchIndex::Type FingerIndex, FVector Location)
{
	StopJumping();
}

void ABrawlVotanCharacter::AddControllerYawInput(float InValue)
{
	ControllerTargetRotation.Yaw += InValue * PlayerController->InputYawScale;
	Super::AddControllerYawInput(InValue);
}

void ABrawlVotanCharacter::AddControllerPitchInput(float InValue)
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

void ABrawlVotanCharacter::TurnAtRate(float Rate)
{
	AddControllerYawInput(Rate * BaseTurnRate * GetWorld()->GetDeltaSeconds());
}

void ABrawlVotanCharacter::LookUpAtRate(float Rate)
{
	AddControllerPitchInput(Rate * BaseLookUpRate * GetWorld()->GetDeltaSeconds());
}

void ABrawlVotanCharacter::MoveForward(float Value)
{
	if ((Controller != NULL) && (Value != 0.0f))
	{
		const FRotator Rotation = Controller->GetControlRotation();
		const FRotator YawRotation(0, Rotation.Yaw, 0);

		const FVector Direction = FRotationMatrix(YawRotation).GetUnitAxis(EAxis::X);
		AddMovementInput(Direction, Value);
	}
}

void ABrawlVotanCharacter::MoveRight(float Value)
{
	if ( (Controller != NULL) && (Value != 0.0f) )
	{
		const FRotator Rotation = Controller->GetControlRotation();
		const FRotator YawRotation(0, Rotation.Yaw, 0);
	
		const FVector Direction = FRotationMatrix(YawRotation).GetUnitAxis(EAxis::Y);
		AddMovementInput(Direction, Value);
	}
}

void ABrawlVotanCharacter::ZoomIn()
{
	bIsZoomOut = false;
}

void ABrawlVotanCharacter::ZoomOut() 
{
	bIsZoomOut = true;
}

void ABrawlVotanCharacter::ExecuteZoom(float DeltaTime)
{
	FVector AdditiveVector = Controller->GetControlRotation().Vector().GetSafeNormal() * ZoomOutLenght;
	FVector TargetCameraLocation = bIsZoomOut ? RelativeZoomOutFollowCameraLocation : RelativeFollowCameraLocation;
	FVector InterpedLocation = FMath::VInterpTo(FollowCamera->RelativeLocation, TargetCameraLocation, DeltaTime, ZoomInterpSpeed);
	FollowCamera->RelativeLocation = InterpedLocation;
}

void ABrawlVotanCharacter::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);
	ExecuteZoom(DeltaTime);

	FRotator InterpedLocation = FMath::RInterpTo(Controller->GetControlRotation(), ControllerTargetRotation, DeltaTime, ZoomInterpSpeed);
	Controller->SetControlRotation(InterpedLocation);
}
