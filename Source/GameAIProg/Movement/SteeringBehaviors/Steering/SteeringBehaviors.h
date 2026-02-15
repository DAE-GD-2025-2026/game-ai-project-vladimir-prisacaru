#pragma once

#include <Movement/SteeringBehaviors/SteeringHelpers.h>
#include "Kismet/KismetMathLibrary.h"

class ASteeringAgent;

// SteeringBehavior base, all steering behaviors should derive from this.
class ISteeringBehavior
{
    public:
    ISteeringBehavior() = default;
    virtual ~ISteeringBehavior() = default;

    // Override to implement your own behavior
    virtual SteeringOutput CalculateSteering(float DeltaT, ASteeringAgent& Agent) = 0;

    void SetTarget(const FTargetData& NewTarget) { Target = NewTarget; }

    template<class T, std::enable_if_t<std::is_base_of_v<ISteeringBehavior, T>>* = nullptr>
    T* As()
    {
        return static_cast<T*>(this);
    }

    protected:
    FTargetData Target;
};

// Your own SteeringBehaviors should follow here...

class Seek : public ISteeringBehavior
{
    public:

    Seek() = default;

    virtual ~Seek() override = default;

    virtual SteeringOutput CalculateSteering(float DeltaT,
        ASteeringAgent& Agent) override;
};

class Wander : public Seek
{
    public:

    Wander() = default;

    virtual ~Wander() override = default;

    virtual SteeringOutput CalculateSteering(float DeltaT,
        ASteeringAgent& Agent) override;

    protected:

    float m_OffsetDistance { 300.0f };
    float m_Radius { 100.0f };
    float m_MaxAngleChange { 0.785398f };

    // How often does the target change
    float m_TargetChangeInterval { 2.0f };
    // How far away from the target is considered "reached"
    float m_TargetDistance { 20.0f };

    // Internal
    float m_WanderAngle { 0.0f };
    float m_TargetChangeTimer { 0.0f };
};

class Flee : public ISteeringBehavior
{
    public:

    Flee() = default;

    virtual ~Flee() override = default;

    virtual SteeringOutput CalculateSteering(float DeltaT,
        ASteeringAgent& Agent) override;
};

class Arrive : public ISteeringBehavior
{
    public:

    Arrive() = default;

    virtual ~Arrive() override = default;

    virtual SteeringOutput CalculateSteering(float DeltaT,
        ASteeringAgent& Agent) override;
};

class Pursuit : public ISteeringBehavior
{
    public:

    Pursuit() = default;

    virtual ~Pursuit() override = default;

    virtual SteeringOutput CalculateSteering(float DeltaT,
        ASteeringAgent& Agent) override;
};

class Evade : public ISteeringBehavior
{
    public:

    Evade() = default;

    virtual ~Evade() override = default;

    virtual SteeringOutput CalculateSteering(float DeltaT,
        ASteeringAgent& Agent) override;
};