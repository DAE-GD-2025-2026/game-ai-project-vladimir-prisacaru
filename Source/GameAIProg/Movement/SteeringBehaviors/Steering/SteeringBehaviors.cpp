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

SteeringOutput Wander::CalculateSteering(float DeltaT, ASteeringAgent& Agent)
{
    auto getNewTargetPosition = [this](ASteeringAgent& Agent) -> FVector2D
    {
        const FVector2D circleCenter { Agent.GetPosition() +
            FVector2D(Agent.GetActorForwardVector()) * m_OffsetDistance };

        const float angle { m_WanderAngle +
            FMath::FRandRange(-m_MaxAngleChange, m_MaxAngleChange) };

        m_WanderAngle = angle;

        const FVector2D offset { FVector2D(FMath::Cos(angle),
            FMath::Sin(angle)) * m_Radius };

        return FVector2D { circleCenter + offset };
    };

    bool timerReached { m_TargetChangeTimer >= m_TargetChangeInterval };

    bool targetReached { (Target.Position - Agent.GetPosition()).Length() <=
        m_TargetDistance };

    if (timerReached || targetReached)
    {
        FTargetData newTarget { };

        newTarget.Position = getNewTargetPosition(Agent);

        SetTarget(newTarget);

        m_TargetChangeTimer = 0.0f;
    }
    else
    {
        m_TargetChangeTimer += DeltaT;
    }

    return Seek::CalculateSteering(DeltaT, Agent);
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