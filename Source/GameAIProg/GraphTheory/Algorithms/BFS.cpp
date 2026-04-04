#include "BFS.h"

#include <map>
#include <queue>

#include "Shared/Graph/Graph.h"

using namespace GameAI;

BFS::BFS(Graph* const pGraph)
	: pGraph(pGraph)
{
}

std::vector<Node*> BFS::FindPath(Node* const pStartNode, Node* const pDestinationNode) const
{
	std::vector<Node*> path;
 
	// Check trivial cases
	if (!pStartNode || !pDestinationNode) return path;
	if (pStartNode == pDestinationNode)
	{
		path.push_back(pStartNode);
		return path;
	}
	
	std::queue<Node*> openList;
	std::map<int, int> closedList;
	
	openList.push(pStartNode);
	closedList[pStartNode->GetId()] = Graphs::InvalidNodeId;
 
	bool bFound = false;
 
	// Explore while there are nodes left to check
	while (!openList.empty())
	{
		// Pop the front node
		Node* pCurrentNode = openList.front();
		openList.pop();
 
		// Check if destination reached
		if (pCurrentNode->GetId() == pDestinationNode->GetId())
		{
			bFound = true;
			break;
		}
 
		// Iterate over every outgoing connection from the current node
		for (Connection* pConnection : pGraph->FindConnectionsFrom(pCurrentNode->GetId()))
		{
			int neighbourId = pConnection->GetToId();
 
			// Skip already visited nodes
			if (closedList.find(neighbourId) != closedList.end())
				continue;
 
			// Record this neighbour with the current node as its parent
			closedList[neighbourId] = pCurrentNode->GetId();
 
			// Add to the back of the queue to be explored later
			openList.push(pGraph->GetNode(neighbourId).get());
		}
	}
 
	// No path exists
	if (!bFound) return path;
 
	// Backtrack to reconstruct the path
	int currentId = pDestinationNode->GetId();
	while (currentId != Graphs::InvalidNodeId)
	{
		path.push_back(pGraph->GetNode(currentId).get());
		currentId = closedList[currentId];
	}
 
	// Reverse path to get start to end
	std::reverse(path.begin(), path.end());
	return path;
}
