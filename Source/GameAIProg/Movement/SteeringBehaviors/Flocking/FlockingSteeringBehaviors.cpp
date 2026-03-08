#include "FlockingSteeringBehaviors.h"
#include "Flock.h"
#include "../SteeringAgent.h"
#include "../SteeringHelpers.h"



SteeringOutput Cohesion::CalculateSteering(float deltaT, ASteeringAgent& pAgent)
{
	if (pFlock->GetNrOfNeighbors() == 0)
	{
		SteeringOutput output{};
		output.IsValid = false;
		return output;
	}

	FTargetData target;
	target.Position = pFlock->GetAverageNeighborPos();
	SetTarget(target);
	
	return Seek::CalculateSteering(deltaT, pAgent);
}



SteeringOutput Separation::CalculateSteering(float deltaT, ASteeringAgent& pAgent)
{
	SteeringOutput output{};

	int count = pFlock->GetNrOfNeighbors();
	if (count == 0)
	{
		output.IsValid = false;
		return output;
	}

	const auto& neighbors = pFlock->GetNeighbors();
	FVector2D agentPos = pAgent.GetPosition();

	for (int i = 0; i < count; ++i)
	{
		if (!IsValid(neighbors[i])) continue;

		FVector2D toAgent = agentPos - neighbors[i]->GetPosition();
		float dist = static_cast<float>(toAgent.Length());

		if (dist > SMALL_NUMBER)
			output.LinearVelocity += toAgent.GetSafeNormal() / dist;
	}
	
	output.LinearVelocity = output.LinearVelocity.GetSafeNormal();
	output.IsValid = !output.LinearVelocity.IsNearlyZero();
	return output;
}



SteeringOutput VelocityMatch::CalculateSteering(float deltaT, ASteeringAgent& pAgent)
{
	SteeringOutput output{};

	if (pFlock->GetNrOfNeighbors() == 0)
	{
		output.IsValid = false;
		return output;
	}

	FVector2D avgVel = pFlock->GetAverageNeighborVelocity();
	
	output.LinearVelocity = avgVel.GetSafeNormal();
	output.IsValid = !output.LinearVelocity.IsNearlyZero();
	return output;
}