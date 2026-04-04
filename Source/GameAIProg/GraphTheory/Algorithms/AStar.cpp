#include "AStar.h"

#include <algorithm>

using namespace GameAI;

AStar::AStar(Graph* const pGraph, HeuristicFunctions::Heuristic hFunction)
	: pGraph(pGraph)
	, HeuristicFunction(hFunction)
{
}

std::vector<Node*> AStar::FindPath(Node* const pStartNode, Node* const pGoalNode)
{
	std::vector<Node*> path{};

	// Check trivial / invalid cases
	if (!pStartNode || !pGoalNode) return path;
	if (pStartNode == pGoalNode)
	{
		path.push_back(pStartNode);
		return path;
	}

	// Create the open and closed lists
	std::vector<NodeRecord> openList;
	std::vector<NodeRecord> closedList;

	// Create the start record and push it to the open list
	NodeRecord startRecord;
	startRecord.pNode       = pStartNode;
	startRecord.pConnection = nullptr;
	startRecord.costSoFar   = 0.f;
	startRecord.estimatedTotalCost = GetHeuristicCost(pStartNode, pGoalNode);
	openList.push_back(startRecord);

	NodeRecord currentRecord;

	// Main loop
	while (!openList.empty())
	{
		// Pick the record with the lowest f-cost
		currentRecord = *std::min_element(openList.begin(), openList.end());

		// Check if we have reached the goal node
		if (currentRecord.pNode->GetId() == pGoalNode->GetId())
			break;

		// Get all outgoing connections of the current node
		std::vector<Connection*> connections =
			pGraph->FindConnectionsFrom(currentRecord.pNode->GetId());

		for (Connection* pConnection : connections)
		{
			// Get the node this connection leads to
			Node* pNextNode = pGraph->GetNode(pConnection->GetToId()).get();

			// Compute new g-cost to reach pNextNode
			float newCostSoFar = currentRecord.costSoFar + pConnection->GetWeight();
			
			// Check if pNextNode is already in the closed list
			auto closedIt = std::find_if(closedList.begin(), closedList.end(),
				[pNextNode](const NodeRecord& r){ return r.pNode == pNextNode; });

			if (closedIt != closedList.end())
			{
				// Skip if the existing closed record is already cheaper
				if (closedIt->costSoFar <= newCostSoFar)
					continue;

				// Otherwise remove from closed list
				closedList.erase(closedIt);
			}
			else
			{
				// Check if pNextNode is already in the open list
				auto openIt = std::find_if(openList.begin(), openList.end(),
					[pNextNode](const NodeRecord& r){ return r.pNode == pNextNode; });

				if (openIt != openList.end())
				{
					// Skip if the existing open record is already cheaper
					if (openIt->costSoFar <= newCostSoFar)
						continue;

					// Otherwise replace by the new cheaper record
					openList.erase(openIt);
				}
			}
			
			// Create a new NodeRecord for pNextNode and add to open list
			NodeRecord newRecord;
			newRecord.pNode               = pNextNode;
			newRecord.pConnection         = pConnection;
			newRecord.costSoFar           = newCostSoFar;
			newRecord.estimatedTotalCost  = newCostSoFar + GetHeuristicCost(pNextNode, pGoalNode);
			openList.push_back(newRecord);
		}

		// Move the current record from the open list to the closed list.
		openList.erase(std::find_if(openList.begin(), openList.end(),
			[&currentRecord](const NodeRecord& r){ return r.pNode == currentRecord.pNode; }));
		
		closedList.push_back(currentRecord);
	}

	// If we exited without finding the goal, return an empty path
	if (currentRecord.pNode->GetId() != pGoalNode->GetId())
		return path;

	// Backtrack to reconstruct the path
	while (currentRecord.pNode->GetId() != pStartNode->GetId())
	{
		path.push_back(currentRecord.pNode);
		
		int fromId = currentRecord.pConnection->GetFromId();
		
		auto parentIt = std::find_if(closedList.begin(), closedList.end(),
			[fromId](const NodeRecord& r){ return r.pNode->GetId() == fromId; });
		
		currentRecord = *parentIt;
	}

	// Add the start node itself
	path.push_back(pStartNode);

	// Reverse path to get start to end
	std::reverse(path.begin(), path.end());
	return path;
}

float AStar::GetHeuristicCost(Node* const pStartNode, Node* const pEndNode) const
{
	FVector2D toDestination = pGraph->GetNode(pEndNode->GetId())->GetPosition() -
		pGraph->GetNode(pStartNode->GetId())->GetPosition();
	
	return HeuristicFunction(abs(toDestination.X), abs(toDestination.Y));
}