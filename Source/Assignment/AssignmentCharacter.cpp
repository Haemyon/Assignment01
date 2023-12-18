// Copyright Epic Games, Inc. All Rights Reserved.

#include "AssignmentCharacter.h"
#include "Camera/CameraComponent.h"
#include "Components/CapsuleComponent.h"
#include "Components/InputComponent.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "GameFramework/Controller.h"
#include "GameFramework/SpringArmComponent.h"
#include "EnhancedInputComponent.h"
#include "DrawDebugHelpers.h"
#include "EnhancedInputSubsystems.h"


//////////////////////////////////////////////////////////////////////////
// AAssignmentCharacter

AAssignmentCharacter::AAssignmentCharacter()
{
	// Set size for collision capsule
	GetCapsuleComponent()->InitCapsuleSize(42.f, 96.0f);
		
	// Don't rotate when the controller rotates. Let that just affect the camera.
	bUseControllerRotationPitch = false;
	bUseControllerRotationYaw = false;
	bUseControllerRotationRoll = false;

	// Configure character movement
	GetCharacterMovement()->bOrientRotationToMovement = true; // Character moves in the direction of input...	
	GetCharacterMovement()->RotationRate = FRotator(0.0f, 500.0f, 0.0f); // ...at this rotation rate

	// Note: For faster iteration times these variables, and many more, can be tweaked in the Character Blueprint
	// instead of recompiling to adjust them
	GetCharacterMovement()->JumpZVelocity = 700.f;
	GetCharacterMovement()->AirControl = 0.35f;
	GetCharacterMovement()->MaxWalkSpeed = 500.f;
	GetCharacterMovement()->MinAnalogWalkSpeed = 20.f;
	GetCharacterMovement()->BrakingDecelerationWalking = 2000.f;

	auto movement = GetCharacterMovement();

	// Create a camera boom (pulls in towards the player if there is a collision)
	CameraBoom = CreateDefaultSubobject<USpringArmComponent>(TEXT("CameraBoom"));
	CameraBoom->SetupAttachment(RootComponent);
	CameraBoom->TargetArmLength = 400.0f; // The camera follows at this distance behind the character	
	CameraBoom->bUsePawnControlRotation = true; // Rotate the arm based on the controller

	// Create a follow camera
	FollowCamera = CreateDefaultSubobject<UCameraComponent>(TEXT("FollowCamera"));
	FollowCamera->SetupAttachment(CameraBoom, USpringArmComponent::SocketName); // Attach the camera to the end of the boom and let the boom adjust to match the controller orientation
	FollowCamera->bUsePawnControlRotation = false; // Camera does not rotate relative to arm

	// Note: The skeletal mesh and anim blueprint references on the Mesh component (inherited from Character) 
	// are set in the derived blueprint asset named ThirdPersonCharacter (to avoid direct content references in C++)
}

void AAssignmentCharacter::BeginPlay()
{
	// Call the base class  
	Super::BeginPlay();

	//Add Input Mapping Context
	if (APlayerController* PlayerController = Cast<APlayerController>(Controller))
	{
		if (UEnhancedInputLocalPlayerSubsystem* Subsystem = ULocalPlayer::GetSubsystem<UEnhancedInputLocalPlayerSubsystem>(PlayerController->GetLocalPlayer()))
		{
			Subsystem->AddMappingContext(DefaultMappingContext, 0);
		}
	}
}

void AAssignmentCharacter::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

	const FVector foot = GetActorLocation() - FVector(0, 0, 90.f);
	const FVector start = GetActorLocation();
	const FVector end = foot + (GetActorForwardVector() * 45.f);
	const FVector up = GetActorLocation() + (GetActorUpVector() * 100.f);

	FHitResult HitResult;
	FCollisionQueryParams Params;
	Params.AddIgnoredActor(this);

	if (GetWorld()->LineTraceSingleByChannel(HitResult, start, end, ECollisionChannel::ECC_GameTraceChannel1, Params))
	{
		//UE_LOG(LogTemp, Warning, TEXT("Wall"));
		DrawDebugLine(GetWorld(), foot, end, FColor::Green, false, 1.0f, 0, 1.0f);
		detectedWall = true;
	}
	else
	{
		DrawDebugLine(GetWorld(), foot, end, FColor::Red, false, 1.0f, 0, 1.0f);
		detectedWall = false;
		//DrawDebugLine(GetWorld(), start, up, FColor::Orange, false, 1.0f, 0, 1.0f);
		if (ClimbMod) {
			ClimbMod = false;
			GetCharacterMovement()->SetMovementMode(MOVE_Walking);
			GetCharacterMovement()->bOrientRotationToMovement = true;
			UE_LOG(LogTemp, Warning, TEXT("FlimbMod Off"));
		}
	}
}



//////////////////////////////////////////////////////////////////////////
// Input

void AAssignmentCharacter::SetupPlayerInputComponent(class UInputComponent* PlayerInputComponent)
{
	// Set up action bindings
	if (UEnhancedInputComponent* EnhancedInputComponent = CastChecked<UEnhancedInputComponent>(PlayerInputComponent)) {
		
		//Jumping
		EnhancedInputComponent->BindAction(JumpAction, ETriggerEvent::Triggered, this, &ACharacter::Jump);
		EnhancedInputComponent->BindAction(JumpAction, ETriggerEvent::Completed, this, &ACharacter::StopJumping);

		//Moving
		EnhancedInputComponent->BindAction(MoveAction, ETriggerEvent::Triggered, this, &AAssignmentCharacter::Move);

		//Looking
		EnhancedInputComponent->BindAction(LookAction, ETriggerEvent::Triggered, this, &AAssignmentCharacter::Look);

		//Climbing
		EnhancedInputComponent->BindAction(ClimbAction, ETriggerEvent::Started, this, &AAssignmentCharacter::Climb);

		//WallJump
		EnhancedInputComponent->BindAction(WallJumpAction, ETriggerEvent::Started, this, &AAssignmentCharacter::WallJump);
	}

}

void AAssignmentCharacter::Move(const FInputActionValue& Value)
{
	// input is a Vector2D
	FVector2D MovementVector = Value.Get<FVector2D>();

	if (Controller != nullptr)
	{
			// find out which way is forward
			const FRotator Rotation = Controller->GetControlRotation();
			const FRotator YawRotation(0, Rotation.Yaw, 0);
		if (!ClimbMod)
		{
			// get forward vector
			const FVector ForwardDirection = FRotationMatrix(YawRotation).GetUnitAxis(EAxis::X);

			// get right vector 
			const FVector RightDirection = FRotationMatrix(YawRotation).GetUnitAxis(EAxis::Y);

			// add movement 
			AddMovementInput(ForwardDirection, MovementVector.Y);
			AddMovementInput(RightDirection, MovementVector.X);
		}
		else if (ClimbMod)
		{
			// get forward vector
			const FVector UpDirection = FRotationMatrix(YawRotation).GetUnitAxis(EAxis::Z);

			// get right vector 
			const FVector RightDirection = FRotationMatrix(YawRotation).GetUnitAxis(EAxis::Y);

			// add movement 
			AddMovementInput(UpDirection, (MovementVector.Y)*0.3);
			AddMovementInput(RightDirection, (MovementVector.X)*0.3);
		}
	}
}

void AAssignmentCharacter::Look(const FInputActionValue& Value)
{
	// input is a Vector2D
	FVector2D LookAxisVector = Value.Get<FVector2D>();

	if (Controller != nullptr)
	{
		// add yaw and pitch input to controller
		AddControllerYawInput(LookAxisVector.X);
		AddControllerPitchInput(LookAxisVector.Y);
	}
}

void AAssignmentCharacter::Climb(const FInputActionValue& Value)
{
	if (detectedWall)
	{
		if (ClimbMod) //벽타기 해제
		{
			ClimbMod = false;
			GetCharacterMovement()->SetMovementMode(MOVE_Walking);
			GetCharacterMovement()->bOrientRotationToMovement = true;
			UE_LOG(LogTemp, Warning, TEXT("FlimbMod Off"));
		}
		else //벽타기 시작
		{
			ClimbMod = true;
			GetCharacterMovement()->SetMovementMode(MOVE_Flying);
			GetCharacterMovement()->bOrientRotationToMovement = false;
			UE_LOG(LogTemp, Warning, TEXT("FlimbMod On"));
		}
	}
}

void AAssignmentCharacter::WallJump(const FInputActionValue& Value)
{
	if (ClimbMod) //벽타기 해제
	{
		UE_LOG(LogTemp, Warning, TEXT("Wall Jump!"));
		FVector JumpVector = FVector(-250.f, 0, 650.f);
		LaunchCharacter(JumpVector, 0, 0);
		ClimbMod = false;
		GetCharacterMovement()->SetMovementMode(MOVE_Walking);
		GetCharacterMovement()->bOrientRotationToMovement = true;
		UE_LOG(LogTemp, Warning, TEXT("FlimbMod Off"));
	}
}
