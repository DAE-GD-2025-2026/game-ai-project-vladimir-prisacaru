// Fill out your copyright notice in the Description page of Project Settings.

#include "SteeringAgent.h"
#include "AIController.h"


// Sets default values
ASteeringAgent::ASteeringAgent()
{
    // Set this character to call Tick() every frame.  You can turn this off to improve performance if you don't need it.
    PrimaryActorTick.bCanEverTick = true;
}

// Called when the game starts or when spawned
void ASteeringAgent::BeginPlay()
{
    Super::BeginPlay();
}

void ASteeringAgent::BeginDestroy()
{
    Super::BeginDestroy();
}

// Called every frame
void ASteeringAgent::Tick(float DeltaTime)
{
    Super::Tick(DeltaTime);

    if (!SteeringBehavior)
        return;

    SteeringOutput output = SteeringBehavior->CalculateSteering(DeltaTime, *this);

    if (!output.IsValid)
        return;

    AddMovementInput(FVector { output.LinearVelocity, 0.f });

    if (IsAutoOrienting())
        return;

    if (AAIController* AIController = Cast<AAIController>(GetController()))
    {
        const float deltaYaw { FMath::Clamp(output.AngularVelocity, -1.0f, 1.0f) *
            GetMaxAngularSpeed() * DeltaTime };

        FRotator currentRotation { GetActorForwardVector().ToOrientationRotator() };
        FRotator deltaRotation { 0, deltaYaw, 0 };
        FRotator desiredRotation { currentRotation + deltaRotation };

        if (FMath::IsNearlyEqual(currentRotation.Yaw, desiredRotation.Yaw))
            return;

        AIController->SetControlRotation(desiredRotation);
        FaceRotation(desiredRotation);
    }
}

// Called to bind functionality to input
void ASteeringAgent::SetupPlayerInputComponent(UInputComponent* PlayerInputComponent)
{
    Super::SetupPlayerInputComponent(PlayerInputComponent);
}

void ASteeringAgent::SetSteeringBehavior(ISteeringBehavior* NewSteeringBehavior)
{
    SteeringBehavior = NewSteeringBehavior;
}