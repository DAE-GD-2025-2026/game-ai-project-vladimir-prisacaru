#include "SpacePartitioning.h"
#include "DrawDebugHelpers.h"

// ------------
// --- Cell ---
// ------------
Cell::Cell(float Left, float Bottom, float Width, float Height)
{
	BoundingBox.Min = { Left, Bottom };
	BoundingBox.Max = { BoundingBox.Min.X + Width, BoundingBox.Min.Y + Height };
}

std::vector<FVector2D> Cell::GetRectPoints() const
{
	const float left   = BoundingBox.Min.X;
	const float bottom = BoundingBox.Min.Y;
	const float width  = BoundingBox.Max.X - BoundingBox.Min.X;
	const float height = BoundingBox.Max.Y - BoundingBox.Min.Y;

	return {
		{ left,         bottom          },
		{ left,         bottom + height },
		{ left + width, bottom + height },
		{ left + width, bottom          },
	};
}

// -------------------------
// --- Partitioned Space ---
// -------------------------
CellSpace::CellSpace(UWorld* pWorld, float Width, float Height, int Rows, int Cols, int MaxEntities)
	: pWorld     { pWorld  }
	, SpaceWidth { Width   }
	, SpaceHeight{ Height  }
	, NrOfRows   { Rows    }
	, NrOfCols   { Cols    }
	, NrOfNeighbors{ 0     }
{
	Neighbors.SetNum(MaxEntities);

	CellWidth  = Width  / Cols;
	CellHeight = Height / Rows;
	
	const float originX = -Width  * 0.5f;
	const float originY = -Height * 0.5f;

	Cells.reserve(Rows * Cols);
	for (int row = 0; row < Rows; ++row)
	{
		for (int col = 0; col < Cols; ++col)
		{
			float left   = originX + col * CellWidth;
			float bottom = originY + row * CellHeight;
			Cells.emplace_back(left, bottom, CellWidth, CellHeight);
		}
	}
	
	CellOrigin = FVector2D(originX, originY);
}

void CellSpace::AddAgent(ASteeringAgent& Agent)
{
	int idx = PositionToIndex(Agent.GetPosition());
	if (idx >= 0 && idx < static_cast<int>(Cells.size()))
		Cells[idx].Agents.push_back(&Agent);
}

void CellSpace::UpdateAgentCell(ASteeringAgent& Agent, const FVector2D& OldPos)
{
	int oldIdx = PositionToIndex(OldPos);
	int newIdx = PositionToIndex(Agent.GetPosition());

	if (oldIdx == newIdx) return;

	// Remove from old cell
	if (oldIdx >= 0 && oldIdx < static_cast<int>(Cells.size()))
	{
		auto& list = Cells[oldIdx].Agents;
		list.remove(&Agent);
	}

	// Insert into new cell
	if (newIdx >= 0 && newIdx < static_cast<int>(Cells.size()))
		Cells[newIdx].Agents.push_back(&Agent);
}

void CellSpace::RegisterNeighbors(ASteeringAgent& Agent, float QueryRadius)
{
	NrOfNeighbors = 0;

	const FVector2D agentPos = Agent.GetPosition();

	// Build a query rectangle around the agent
	FRect queryRect;
	queryRect.Min = agentPos - FVector2D(QueryRadius, QueryRadius);
	queryRect.Max = agentPos + FVector2D(QueryRadius, QueryRadius);

	const float radiusSq = QueryRadius * QueryRadius;

	for (Cell& cell : Cells)
	{
		if (!DoRectsOverlap(queryRect, cell.BoundingBox))
			continue;

		for (ASteeringAgent* pOther : cell.Agents)
		{
			if (pOther == &Agent) continue;

			float distSq = static_cast<float>(
				FVector2D::DistSquared(agentPos, pOther->GetPosition()));

			if (distSq <= radiusSq)
			{
				if (NrOfNeighbors < Neighbors.Num())
					Neighbors[NrOfNeighbors] = pOther;
				++NrOfNeighbors;
			}
		}
	}
}

void CellSpace::EmptyCells()
{
	for (Cell& c : Cells)
		c.Agents.clear();
}

void CellSpace::RenderCells() const
{
	if (!pWorld) return;

	const float Z = 100.f; // draw slightly above ground

	for (const Cell& cell : Cells)
	{
		// Corner points of this cell
		const FVector2D& mn = cell.BoundingBox.Min;
		const FVector2D& mx = cell.BoundingBox.Max;

		FVector corners[4] = {
			{ mn.X, mn.Y, Z },
			{ mx.X, mn.Y, Z },
			{ mx.X, mx.Y, Z },
			{ mn.X, mx.Y, Z },
		};

		// Choose colour based on occupancy
		int agentCount = static_cast<int>(cell.Agents.size());
		FColor color   = agentCount > 0 ? FColor::Yellow : FColor(60, 60, 60, 120);

		// Draw cell outline
		for (int i = 0; i < 4; ++i)
			DrawDebugLine(pWorld, corners[i], corners[(i + 1) % 4], color, false, -1.f, 0, 2.f);

		// Draw agent count as a debug string at cell centre
		if (agentCount > 0)
		{
			FVector centre{
				(mn.X + mx.X) * 0.5f,
				(mn.Y + mx.Y) * 0.5f,
				Z + 20.f
			};
			DrawDebugString(pWorld, centre, FString::FromInt(agentCount),
				nullptr, FColor::White, 0.f);
		}
	}
}

int CellSpace::PositionToIndex(FVector2D const& Pos) const
{
	// Shift position relative to grid origin, then floor-divide by cell size
	int col = static_cast<int>((Pos.X - CellOrigin.X) / CellWidth);
	int row = static_cast<int>((Pos.Y - CellOrigin.Y) / CellHeight);

	// Clamp to valid range to avoid out-of-bounds on border positions
	col = FMath::Clamp(col, 0, NrOfCols - 1);
	row = FMath::Clamp(row, 0, NrOfRows - 1);

	return row * NrOfCols + col;
}

bool CellSpace::DoRectsOverlap(FRect const& RectA, FRect const& RectB)
{
	if (RectA.Max.X < RectB.Min.X || RectA.Min.X > RectB.Max.X) return false;
	if (RectA.Max.Y < RectB.Min.Y || RectA.Min.Y > RectB.Max.Y) return false;
	return true;
}