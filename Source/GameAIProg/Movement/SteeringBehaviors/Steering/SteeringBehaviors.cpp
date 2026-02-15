#include "SteeringBehaviors.h"
#include "GameAIProg/Movement/SteeringBehaviors/SteeringAgent.h"

//SEEK
//*******
// TODO: Do the Week01 assignment :^)

SteeringOutput Seek::CalculateSteering(float DeltaT, ASteeringAgent& Agent)
{
    FVector2D targetInput { (Target.Position - Agent.GetPosition()).GetSafeNormal() };

    return SteeringOutput { targetInput, 0.0f };
}

SteeringOutput Flee::CalculateSteering(float DeltaT, ASteeringAgent& Agent)
{
    FVector2D targetInput { (Agent.GetPosition() - Target.Position).GetSafeNormal() };

    return SteeringOutput { targetInput, 0.0f };
}

SteeringOutput Arrive::CalculateSteering(float DeltaT, ASteeringAgent& Agent)
{
    FVector2D targetInput { (Target.Position - Agent.GetPosition()) };

    const float distance { static_cast<float>(targetInput.Length()) };

    const float slowRadius { 300.0f };
    const float targetRadius { 100.0f };

    // Already within arrival radius — full stop
    if (distance < targetRadius)
        return SteeringOutput { FVector2D::ZeroVector, 0.f };

    // Inverse lerp: TargetRadius -> 0.0, SlowRadius -> 1.0
    float speedScalar { FMath::Clamp(
        (distance - targetRadius) / (slowRadius - targetRadius),
        0.f, 1.f
    ) };

    return SteeringOutput { targetInput.GetSafeNormal() * speedScalar, 0.f };
}

SteeringOutput Pursuit::CalculateSteering(float DeltaT, ASteeringAgent& Agent)
{
    const float agentSpeed { Agent.GetMaxLinearSpeed() };

    const float distance { static_cast<float>(
        (Target.Position - Agent.GetPosition()).Length()) };

    const float travelTime { distance / agentSpeed };

    const FVector2D targetPredictedPos { Target.Position +
        Target.LinearVelocity * travelTime };

    const FVector2D targetInput { (targetPredictedPos -
        Agent.GetPosition()).GetSafeNormal() };

    return SteeringOutput { targetInput, 0.0f };
}

SteeringOutput Evade::CalculateSteering(float DeltaT, ASteeringAgent& Agent)
{
    const float agentSpeed { Agent.GetMaxLinearSpeed() };

    const float distance { static_cast<float>(
        (Target.Position - Agent.GetPosition()).Length()) };

    const float travelTime { distance / agentSpeed };

    const FVector2D targetPredictedPos { Target.Position +
        Target.LinearVelocity * travelTime };

    const FVector2D targetInput { (Agent.GetPosition() -
        targetPredictedPos).GetSafeNormal() };

    return SteeringOutput { targetInput, 0.0f };
}