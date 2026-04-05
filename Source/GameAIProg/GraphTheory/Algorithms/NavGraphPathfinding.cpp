#include "NavGraphPathfinding.h"

#include "AStar.h"
#include "PathSmoothing.h"
#include "Shared/Graph/NavGraph/NavGraph.h"
#include "Shared/Graph/NavGraph/NavGraphNode.h"

using namespace GameAI;

std::vector<FVector2D> NavMeshPathfinding::FindPath(const FVector2D& startPos, const FVector2D& endPos,
	NavGraph* const pNavGraph, std::vector<FVector2D>& debugNodePositions, std::vector<NavLine>& debugPortals)
{
	std::vector<FVector2D> finalPath{};
	
	// Locate the start and end triangles
	TriPolygon const* pNavPoly = pNavGraph->GetNavPolygon();

	FVector2D clampedStart = startPos;
	FVector2D clampedEnd   = endPos;

	TriPolygon::Triangle const* pStartTri = pNavPoly->GetClosestTriangleToPosition(startPos, clampedStart);
	TriPolygon::Triangle const* pEndTri   = pNavPoly->GetClosestTriangleToPosition(endPos,   clampedEnd);

	// Can't find path if either triangle is invalid
	if (!pStartTri || !pEndTri)
		return finalPath;

	// Straight line trivial case, if start and end are in the same triangle
	if (*pStartTri == *pEndTri)
	{
		finalPath.push_back(clampedStart);
		finalPath.push_back(clampedEnd);
		return finalPath;
	}

	// Clone the graph for safe editing
	std::unique_ptr<NavGraph> pWorkingGraph = pNavGraph->Clone();
	
	// Create a temporary start node at the agent's exact position
	// Connect it to all nodes whose edges belong to the start triangle
	auto pStartNode = std::make_unique<NavGraphNode>(clampedStart, Graphs::InvalidNodeId);
	int startNodeId = pWorkingGraph->AddNode(std::move(pStartNode));

	for (auto const& Edge : pStartTri->GetEdges())
	{
		auto EdgeIdxOpt = pNavPoly->FindEdgeIndex(Edge);
		if (!EdgeIdxOpt.has_value()) continue;

		int neighbourId = pWorkingGraph->GetNodeIdFromEdgeIndex(EdgeIdxOpt.value());
		if (neighbourId == Graphs::InvalidNodeId) continue;

		// Cost = Euclidean distance between start pos and that edge midpoint
		FVector2D neighbourPos = pWorkingGraph->GetNode(neighbourId)->GetPosition();
		float cost = FVector2D::Distance(clampedStart, neighbourPos);

		auto conn = std::make_unique<Connection>(startNodeId, neighbourId);
		conn->SetWeight(cost);
		pWorkingGraph->AddConnection(std::move(conn));
	}
	
	// Create a temporary end node at the target position
	// Connect every node whose edge belongs to the end triangle to the end node
	auto pEndNode = std::make_unique<NavGraphNode>(clampedEnd, Graphs::InvalidNodeId);
	int endNodeId = pWorkingGraph->AddNode(std::move(pEndNode));

	for (auto const& Edge : pEndTri->GetEdges())
	{
		auto EdgeIdxOpt = pNavPoly->FindEdgeIndex(Edge);
		if (!EdgeIdxOpt.has_value()) continue;

		int neighbourId = pWorkingGraph->GetNodeIdFromEdgeIndex(EdgeIdxOpt.value());
		if (neighbourId == Graphs::InvalidNodeId) continue;

		FVector2D neighbourPos = pWorkingGraph->GetNode(neighbourId)->GetPosition();
		float cost = FVector2D::Distance(clampedEnd, neighbourPos);

		auto conn = std::make_unique<Connection>(neighbourId, endNodeId);
		conn->SetWeight(cost);
		pWorkingGraph->AddConnection(std::move(conn));
	}
	
	// Run A* on the augmented working graph
	AStar astar(pWorkingGraph.get(), HeuristicFunctions::Euclidean);

	Node* pAStarStart = pWorkingGraph->GetNode(startNodeId).get();
	Node* pAStarEnd   = pWorkingGraph->GetNode(endNodeId).get();

	std::vector<Node*> nodePath = astar.FindPath(pAStarStart, pAStarEnd);

	if (nodePath.empty())
		return finalPath;

	// Collect node positions for debug rendering
	for (Node* pNode : nodePath)
	{
		debugNodePositions.push_back(pNode->GetPosition());
	}
	
	// Convert the node path to positions (non-smoothed path)
	//for (Node* pNode : nodePath)
	//{
	//	finalPath.push_back(pNode->GetPosition());
	//}
	
	// Run SSFA (smoothed path)
	debugPortals = SSFA::FindPortals(nodePath, *pNavPoly);
	finalPath = SSFA::OptimizePortals(debugPortals, *pNavPoly);

	return finalPath;
}

std::vector<FVector2D> NavMeshPathfinding::FindPath(const FVector2D& startPos, const FVector2D& endPos, NavGraph* const pNavGraph)
{
	std::vector<FVector2D> debugNodePositions{};
	std::vector<NavLine> debugPortals{};
	return FindPath(startPos, endPos, pNavGraph, debugNodePositions, debugPortals);
}