#pragma once
#include "Movement/SteeringBehaviors/Steering/SteeringBehaviors.h"
class Flock;



class Cohesion final : public Seek
{
public:
	Cohesion(Flock* const pFlock) :pFlock(pFlock) {};
	SteeringOutput CalculateSteering(float deltaT, ASteeringAgent& pAgent) override;

private:
	Flock* pFlock = nullptr;
};



class Separation final : public ISteeringBehavior
{
public:
	Separation(Flock* const pFlock) : pFlock(pFlock) {}
	SteeringOutput CalculateSteering(float deltaT, ASteeringAgent& pAgent) override;
	
private:
	Flock* pFlock = nullptr;
};



class VelocityMatch final : public ISteeringBehavior
{
public:
	VelocityMatch(Flock* const pFlock) : pFlock(pFlock) {}
	SteeringOutput CalculateSteering(float deltaT, ASteeringAgent& pAgent) override;
	
private:
	Flock* pFlock = nullptr;
};