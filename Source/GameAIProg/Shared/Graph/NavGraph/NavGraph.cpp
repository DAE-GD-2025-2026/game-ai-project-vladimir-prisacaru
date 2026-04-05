#include "NavGraph.h"
#include "NavGraphNode.h"

GameAI::NavGraph::NavGraph(std::unique_ptr<TriPolygon> && NavPoly)
	: Graph{false}
	, pNavPoly{std::move(NavPoly)}
{
	CreateNavigationGraph();
}

GameAI::NavGraph::NavGraph(const NavGraph& Other)
	: Graph(false)
{
	Nodes.reserve(Other.Nodes.size());
	for (std::unique_ptr<Node> const & OtherNode : Other.Nodes)
	{
		Nodes.push_back(std::make_unique<NavGraphNode>(*static_cast<NavGraphNode*>(OtherNode.get())));
	}
        
	Connections.reserve(Other.Connections.size());
	for (std::unique_ptr<Connection> const & OtherConnection : Other.Connections)
	{
		Connections.push_back(std::make_unique<Connection>(*OtherConnection.get()));
	}
}

std::unique_ptr<GameAI::NavGraph> GameAI::NavGraph::Clone() const
{
	return std::make_unique<NavGraph>(*this);
}

int GameAI::NavGraph::GetNodeIdFromEdgeIndex(int EdgeIdx) const
{
	if (EdgeIdx >= 0)
	{
		for (auto const & pNode : Nodes)
		{
			if (reinterpret_cast<NavGraphNode*>(pNode.get())->GetEdgeIdx() == EdgeIdx)
			{
				return pNode->GetId();
			}
		}
	}
	
	return Graphs::InvalidNodeId;
}

void GameAI::NavGraph::CreateNavigationGraph()
{
	// Create one node per shared edge
	std::vector<TriPolygon::Edge> const& Edges     = pNavPoly->GetEdges();
	std::vector<TriPolygon::Triangle> const& Triangles = pNavPoly->GetTriangles();

	for (int EdgeIdx = 0; EdgeIdx < static_cast<int>(Edges.size()); ++EdgeIdx)
	{
		// Count how many triangles share this edge
		int SharedCount = 0;
		for (auto const& Tri : Triangles)
		{
			if (Tri.HasEdge(Edges[EdgeIdx]))
				++SharedCount;
		}

		// Only create a node for edges shared between exactly 2 triangles
		if (SharedCount >= 2)
		{
			TriPolygon::Edge const& Edge = Edges[EdgeIdx];

			// Midpoint of the edge
			FVector P1 = Edge.GetP1(*pNavPoly);
			FVector P2 = Edge.GetP2(*pNavPoly);
			FVector2D MidPoint = FVector2D{ (P1.X + P2.X) * 0.5f, (P1.Y + P2.Y) * 0.5f };

			// Create and add the node, storing which edge it belongs to
			auto NewNode = std::make_unique<NavGraphNode>(MidPoint, EdgeIdx);
			AddNode(std::move(NewNode));
		}
	}
	
	// Connect nodes that share the same triangle
	for (auto const& Triangle : Triangles)
	{
		// Get the 3 edges of this triangle and find the node on each shared edge
		std::array<TriPolygon::Edge, 3> TriEdges = Triangle.GetEdges();
		std::vector<int> ValidNodeIds{};

		for (auto const& Edge : TriEdges)
		{
			// Look up the edge index in the polygon
			auto EdgeIdxOpt = pNavPoly->FindEdgeIndex(Edge);
			if (!EdgeIdxOpt.has_value()) continue;

			// Check if a NavGraphNode exists for this edge
			int NodeId = GetNodeIdFromEdgeIndex(EdgeIdxOpt.value());
			if (NodeId != Graphs::InvalidNodeId)
			{
				ValidNodeIds.push_back(NodeId);
			}
		}

		// Connect all pairs of valid nodes found in this triangle
		for (int i = 0; i < static_cast<int>(ValidNodeIds.size()); ++i)
		{
			for (int j = i + 1; j < static_cast<int>(ValidNodeIds.size()); ++j)
			{
				AddConnection(ValidNodeIds[i], ValidNodeIds[j]);
			}
		}
	}
	
	SetConnectionCostsToDistances();
}