#include "CombinedSteeringBehaviors.h"
#include <algorithm>
#include "../SteeringAgent.h"

BlendedSteering::BlendedSteering(const std::vector<WeightedBehavior>& WeightedBehaviors)
	:WeightedBehaviors(WeightedBehaviors)
{};

//****************
//BLENDED STEERING
SteeringOutput BlendedSteering::CalculateSteering(float DeltaT, ASteeringAgent& Agent)
{
	SteeringOutput BlendedSteering = {};
	// TODO: Calculate the weighted average steeringbehavior
	
	float totalWeight { 0.0f };
	
	for (const auto& behaviour : WeightedBehaviors)
	{
		if (behaviour.Weight <= 0.0f || behaviour.pBehavior == nullptr)
			continue;

		auto output = behaviour.pBehavior->CalculateSteering(DeltaT, Agent);

		// Skip invalid outputs (e.g. flocking behaviors with no neighbors)
		if (!output.IsValid)
			continue;

		// Proper weighted accumulation - multiply BEFORE summing
		BlendedSteering.LinearVelocity  += output.LinearVelocity  * behaviour.Weight;
		BlendedSteering.AngularVelocity += output.AngularVelocity * behaviour.Weight;
		totalWeight += behaviour.Weight;
	}

	if (totalWeight > 0.0f)
	{
		BlendedSteering.LinearVelocity  /= totalWeight;
		BlendedSteering.AngularVelocity /= totalWeight;
	}

	BlendedSteering.IsValid = true;
	return BlendedSteering;
}

float* BlendedSteering::GetWeight(ISteeringBehavior* const SteeringBehavior)
{
	auto it = find_if(WeightedBehaviors.begin(),
		WeightedBehaviors.end(),
		[SteeringBehavior](const WeightedBehavior& Elem)
		{
			return Elem.pBehavior == SteeringBehavior;
		}
	);

	if(it!= WeightedBehaviors.end())
		return &it->Weight;
	
	return nullptr;
}

//*****************
//PRIORITY STEERING
SteeringOutput PrioritySteering::CalculateSteering(float DeltaT, ASteeringAgent& Agent)
{
	SteeringOutput Steering = {};

	for (ISteeringBehavior* const pBehavior : m_PriorityBehaviors)
	{
		Steering = pBehavior->CalculateSteering(DeltaT, Agent);

		if (Steering.IsValid)
			break;
	}

	//If none of the behavior return a valid output, last behavior is returned
	return Steering;
}