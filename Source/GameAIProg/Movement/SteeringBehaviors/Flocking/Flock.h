#pragma once

// Toggle this define to enable/disable spatial partitioning
#define GAMEAI_USE_SPACE_PARTITIONING

#include "FlockingSteeringBehaviors.h"
#include "Movement/SteeringBehaviors/SteeringAgent.h"
#include "Movement/SteeringBehaviors/SteeringHelpers.h"
#include "Movement/SteeringBehaviors/CombinedSteering/CombinedSteeringBehaviors.h"
#include <memory>
#include "imgui.h"
#ifdef GAMEAI_USE_SPACE_PARTITIONING
#include "GameAIProg/Movement/SteeringBehaviors/SpacePartitioning/SpacePartitioning.h"
#endif

class Flock final
{
public:
    Flock(
        UWorld* pWorld,
        TSubclassOf<ASteeringAgent> AgentClass,
        int FlockSize    = 10,
        float WorldSize  = 100.f,
        ASteeringAgent* const pAgentToEvade = nullptr,
        bool bTrimWorld  = false);

    ~Flock();

    void Tick(float DeltaTime);
    void RenderDebug();
    void ImGuiRender(ImVec2 const& WindowPos, ImVec2 const& WindowSize);

    // Neighbor queries
#ifdef GAMEAI_USE_SPACE_PARTITIONING
    int GetNrOfNeighbors() const { return pPartitionedSpace->GetNrOfNeighbors(); }
    const TArray<ASteeringAgent*>& GetNeighbors() const { return pPartitionedSpace->GetNeighbors(); }
#else
    void RegisterNeighbors(ASteeringAgent* const Agent);
    int GetNrOfNeighbors() const { return NrOfNeighbors; }
    const TArray<ASteeringAgent*>& GetNeighbors() const { return Neighbors; }
#endif

    FVector2D GetAverageNeighborPos()      const;
    FVector2D GetAverageNeighborVelocity() const;

    void SetTarget_Seek(FSteeringParams const& Target);

private:
    // Helpers
    void SpawnAgents();
    void CreateBehaviors();
    void AssignBehaviors();
    void DestroyAgents();
    void Reset();
#ifdef GAMEAI_USE_SPACE_PARTITIONING
    void InitPartitioning();
#endif

    // World
    UWorld* pWorld { nullptr };
    float   WorldSize { 100.f };

    // Agents
    int FlockSize { 0 };
    TSubclassOf<ASteeringAgent> AgentClass { nullptr };
    TArray<ASteeringAgent*> Agents {};

    // Spatial partitioning
#ifdef GAMEAI_USE_SPACE_PARTITIONING
    std::unique_ptr<CellSpace> pPartitionedSpace {};
    TArray<FVector2D>          OldPositions {};
    int NrOfCellsX { 10 };
    int NrOfCellsY { 10 };
#else
    TArray<ASteeringAgent*> Neighbors {};
    int NrOfNeighbors { 0 };
#endif

    float NeighborhoodRadius { 200.f };

    // Evade target
    ASteeringAgent* pAgentToEvade { nullptr };

    // Steering behaviors
    std::unique_ptr<Separation>    pSeparationBehavior {};
    std::unique_ptr<Cohesion>      pCohesionBehavior   {};
    std::unique_ptr<VelocityMatch> pVelMatchBehavior   {};
    std::unique_ptr<Seek>          pSeekBehavior       {};
    std::unique_ptr<Wander>        pWanderBehavior     {};
    std::unique_ptr<Evade>         pEvadeBehavior      {};

    std::unique_ptr<BlendedSteering>  pBlendedSteering  {};
    std::unique_ptr<PrioritySteering> pPrioritySteering {};

    // Seek target persistence (for reset)
    FSteeringParams SeekTarget     {};
    bool            bHasSeekTarget { false };

    // UI state
    int  PendingFlockSize       { 0     };
    bool bPendingReset          { false };
    bool bDebugRenderNeighborhood { false };
    bool bDebugRenderPartitions   { false };

    void RenderNeighborhood();
};