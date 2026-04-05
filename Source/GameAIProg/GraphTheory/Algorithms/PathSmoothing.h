#pragma once
#include <vector>

#include "NavGraphPathfinding.h"
#include "Movement/Pathfinding/Navmesh/TriPolygon.h"
#include "Shared/Graph/Graph.h"
#include "Shared/Graph/NavGraph/NavGraphNode.h"

namespace GameAI
{
	inline float Cross2D(FVector2D const& A, FVector2D const& B)
	{
		return A.X * B.Y - A.Y * B.X;
	}
	
	inline float TriArea2(FVector2D const& A, FVector2D const& B, FVector2D const& C)
	{
		const FVector2D AB = B - A;
		const FVector2D AC = C - A;
		return AC.X * AB.Y - AB.X * AC.Y;
	}

	class SSFA final
	{
	public:
		
		static std::vector<NavLine> FindPortals(std::vector<Node*> const& Path, TriPolygon const& NavPoly)
		{
			std::vector<NavLine> Portals{};

			// At least 3 nodes needed (start, at least one middle, end)
			if (Path.size() < 2)
				return Portals;
			
			// Degenerate start portal
			FVector2D startPos = Path.front()->GetPosition();
			Portals.push_back(NavLine{ startPos, startPos });

			for (int i = 0; i < static_cast<int>(Path.size()); ++i)
			{
				NavGraphNode const* pNavNode = static_cast<NavGraphNode const*>(Path[i]);
				if (!pNavNode) continue;

				int EdgeIdx = pNavNode->GetEdgeIdx();
				
				// Skip the temporary start/end nodes (EdgeIdx == -1)
				if (EdgeIdx < 0) continue;

				auto const& Edges = NavPoly.GetEdges();
				if (EdgeIdx >= static_cast<int>(Edges.size())) continue;

				TriPolygon::Edge const& Edge = Edges[EdgeIdx];
				FVector2D P1_2D = FVector2D{ Edge.GetP1(NavPoly) };
				FVector2D P2_2D = FVector2D{ Edge.GetP2(NavPoly) };

				// Determine travel direction
				FVector2D prevPos = (i > 0)
					? Path[i - 1]->GetPosition()
					: Path[i]->GetPosition();
				
				FVector2D travelDir = Path[i]->GetPosition() - prevPos;

				// Determine left-right using cross product
				FVector2D toP1 = P1_2D - prevPos;
				float crossVal = Cross2D(travelDir, toP1);

				NavLine Portal{};
				if (crossVal < 0.f)
				{
					Portal.P1 = P2_2D;
					Portal.P2 = P1_2D;
				}
				else
				{
					Portal.P1 = P1_2D;
					Portal.P2 = P2_2D;
				}

				Portals.push_back(Portal);
			}

			// Add a degenerate portal at the end position
			FVector2D endPos = Path.back()->GetPosition();
			Portals.push_back(NavLine{ endPos, endPos });

			return Portals;
		}
		
		static std::vector<FVector2D> OptimizePortals(std::vector<NavLine> const& Portals, TriPolygon const&)
		{
			std::vector<FVector2D> Path;

			if (Portals.empty())
				return Path;

			int n = static_cast<int>(Portals.size());

			// Funnel state
			FVector2D portalApex  = Portals[0].P1;
			FVector2D portalLeft  = Portals[0].P1;
			FVector2D portalRight = Portals[0].P2;

			int apexIndex = 0;
			int leftIndex = 0;
			int rightIndex = 0;

			Path.push_back(portalApex);

			for (int i = 1; i < n; ++i)
			{
				const FVector2D& left  = Portals[i].P1;
				const FVector2D& right = Portals[i].P2;
				
				// Right side
				if (TriArea2(portalApex, portalRight, right) <= 0.0f)
				{
					if (portalApex == portalRight ||
						TriArea2(portalApex, portalLeft, right) > 0.0f)
					{
						// Tighten funnel
						portalRight = right;
						rightIndex = i;
					}
					else
					{
						// Right over left: emit left
						Path.push_back(portalLeft);

						// New apex
						portalApex = portalLeft;
						apexIndex = leftIndex;

						// Reset funnel
						portalLeft = portalApex;
						portalRight = portalApex;
						leftIndex = apexIndex;
						rightIndex = apexIndex;

						// Restart scan
						i = apexIndex;
						continue;
					}
				}
				
				// Left side
				if (TriArea2(portalApex, portalLeft, left) >= 0.0f)
				{
					if (portalApex == portalLeft ||
						TriArea2(portalApex, portalRight, left) < 0.0f)
					{
						// Tighten funnel
						portalLeft = left;
						leftIndex = i;
					}
					else
					{
						// Left over right: emit right
						Path.push_back(portalRight);

						// New apex
						portalApex = portalRight;
						apexIndex = rightIndex;

						// Reset funnel
						portalLeft = portalApex;
						portalRight = portalApex;
						leftIndex = apexIndex;
						rightIndex = apexIndex;

						// Restart
						i = apexIndex;
						continue;
					}
				}
			}

			// Add goal
			Path.push_back(Portals.back().P1);

			return Path;
		}

	private:
		SSFA()  {}
		~SSFA() {}
	};
}