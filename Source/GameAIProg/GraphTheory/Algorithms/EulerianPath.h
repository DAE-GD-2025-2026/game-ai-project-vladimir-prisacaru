#pragma once
#include <stack>
#include "Shared/Graph/Graph.h"

namespace GameAI
{
	enum class Eulerianity
	{
		notEulerian,
		semiEulerian,
		eulerian,
	};

	class EulerianPath final
	{
	public:
		EulerianPath(Graph* const pGraph);

		Eulerianity IsEulerian() const;
		std::vector<Node*> FindPath(Eulerianity& eulerianity) const;

	private:
		void VisitAllNodesDFS(const std::vector<Node*>& pNodes, std::vector<bool>& visited, int startIndex) const;
		bool IsConnected() const;

		Graph* m_pGraph;
	};

	inline EulerianPath::EulerianPath(Graph* const pGraph)
		: m_pGraph(pGraph)
	{
	}

	inline Eulerianity EulerianPath::IsEulerian() const
	{
		// If the graph is not connected, there can be no Eulerian Trail
		if (!IsConnected())
			return Eulerianity::notEulerian;
		
		// Count nodes with odd degree
		int oddDegreeCount{ 0 };
		for (Node* pNode : m_pGraph->GetActiveNodes())
		{
			int degree = static_cast<int>(m_pGraph->FindConnectionsFrom(pNode->GetId()).size());
			if (degree % 2 != 0)
				++oddDegreeCount;
		}

		// A connected graph with more than 2 nodes with an odd degree is not Eulerian
		if (oddDegreeCount > 2)
			return Eulerianity::notEulerian;
		
		// A connected graph with exactly 2 nodes with an odd degree is Semi-Eulerian,
		// unless there are only 2 nodes
		if (oddDegreeCount == 2 && m_pGraph->GetNodeCount() > 2)
			return Eulerianity::semiEulerian;

		// A connected graph with no odd nodes is Eulerian
		// (a valid graph with one odd degree node is impossible, so no more checks required)
		return Eulerianity::notEulerian;
	}

	inline std::vector<Node*> EulerianPath::FindPath(Eulerianity& eulerianity) const
	{
		// Get a copy of the graph because this algorithm involves removing edges
		Graph graphCopy = m_pGraph->Clone();
		std::vector<Node*> Path = {};
		std::vector<Node*> Nodes = graphCopy.GetActiveNodes();
		int currentNodeId{ Graphs::InvalidNodeId };
		
		// Check if there can be an Euler path
		eulerianity = IsEulerian();
		
		// Helper lambda
		auto findOddDegreeNodeId = [graphCopy] (const std::vector<Node*>& Nodes) -> int
		{
			for (Node* pNode : Nodes)
			{
				int degree = static_cast<int>(graphCopy.FindConnectionsFrom(pNode->GetId()).size());
				if (degree % 2 != 0)
				{
					return pNode->GetId();
				}
			}
			
			return Graphs::InvalidNodeId;
		};
		
		switch (eulerianity)
		{
			case Eulerianity::notEulerian:
				// If this graph is not eulerian, return the empty path
				return Path;
			case Eulerianity::eulerian:
				// Any active node works as a start
				currentNodeId = Nodes[0]->GetId();
				break;
			case Eulerianity::semiEulerian:
				// Find a node with odd degree to start from
				currentNodeId = findOddDegreeNodeId(Nodes);
				if (currentNodeId == Graphs::InvalidNodeId)
					return Path;
				break;
		}
		
		// Start algorithm loop
		std::stack<int> nodeStack;
		nodeStack.push(currentNodeId);
 
		while (!nodeStack.empty())
		{
			int topId = nodeStack.top();
 
			// Check if this node still has outgoing connections in the copy
			std::vector<Connection*> connections = graphCopy.FindConnectionsFrom(topId);
 
			if (!connections.empty())
			{
				// Pick any neighbour (we take the first available connection)
				Connection* pConn = connections[0];
				int neighbourId = pConn->GetToId();
 
				// Push current node onto the stack (we'll come back to it)
				nodeStack.push(neighbourId);
 
				// Remove the edge (and its inverse, since the graph is undirected)
				// RemoveConnection handles both directions automatically for undirected graphs
				graphCopy.RemoveConnection(topId, neighbourId);
			}
			else
			{
				// No more edges from this node — it belongs in the path
				// IMPORTANT: get the node pointer from the ORIGINAL graph, not the copy
				Path.push_back(m_pGraph->GetNode(topId).get());
				nodeStack.pop();
			}
		}

		std::reverse(Path.begin(), Path.end());
		return Path;
	}

	inline void EulerianPath::VisitAllNodesDFS(const std::vector<Node*>& Nodes, std::vector<bool>& visited, int startIndex) const
	{
		// Mark the current node as visited
		visited[startIndex] = true;

		// Ask the graph for the connections from that node
		int nodeId = Nodes[startIndex]->GetId();
		std::vector<Connection*> connections = m_pGraph->FindConnectionsFrom(nodeId);
		
		// Recursively visit any valid connected nodes that were not visited before
		for (const Connection* pConn : connections)
		{
			int neighbourId = pConn->GetToId();
			
			for (int i = 0; i < static_cast<int>(Nodes.size()); ++i)
			{
				if (Nodes[i]->GetId() == neighbourId)
				{
					if (!visited[i])
					{
						VisitAllNodesDFS(Nodes, visited, i);
					}
					
					break;
				}
			}
		}
	}

	inline bool EulerianPath::IsConnected() const
	{
		std::vector<Node*> Nodes = m_pGraph->GetActiveNodes();
		if (Nodes.size() == 0)
			return false;
		
		// Choose a starting node (with at least one connection)
		int startIndex = 0;
		for (int i = 0; i < static_cast<int>(Nodes.size()); ++i)
		{
			if (!m_pGraph->FindConnectionsFrom(Nodes[i]->GetId()).empty())
			{
				startIndex = i;
				break;
			}
		}
		
		// Start a depth-first-search traversal from the chosen node
		std::vector<bool> visited(Nodes.size(), false);
		VisitAllNodesDFS(Nodes, visited, startIndex);
		
		// If a node was never visited, this graph is not connected
		for (int i = 0; i < static_cast<int>(Nodes.size()); ++i)
		{
			if (!visited[i])
				return false;
		}
 
		return true;
	}
}