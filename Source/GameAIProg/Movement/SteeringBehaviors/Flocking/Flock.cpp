#include "Flock.h"
#include "FlockingSteeringBehaviors.h"
#include "DrawDebugHelpers.h"
#include "GameAIProg/Shared/ImGuiHelpers.h"



Flock::Flock(
    UWorld* pWorld,
    TSubclassOf<ASteeringAgent> AgentClass,
    int FlockSize,
    float WorldSize,
    ASteeringAgent* const pAgentToEvade,
    bool bTrimWorld)
    : pWorld        { pWorld        }
    , WorldSize     { WorldSize     }
    , FlockSize     { FlockSize     }
    , AgentClass    { AgentClass    }
    , pAgentToEvade { pAgentToEvade }
{
    PendingFlockSize = FlockSize;

    if (!AgentClass)
    {
        UE_LOG(LogTemp, Error, TEXT("Flock: AgentClass is null! Set it in the editor."));
        return;
    }

    SpawnAgents();
    CreateBehaviors();
    AssignBehaviors();

#ifdef GAMEAI_USE_SPACE_PARTITIONING
    InitPartitioning();
#endif
}

Flock::~Flock()
{
    DestroyAgents();
}



// -----------------------
// --- Private helpers ---
// -----------------------

void Flock::SpawnAgents()
{
    Agents.Reserve(FlockSize);
    for (int i = 0; i < FlockSize; ++i)
    {
        FVector spawnLoc{
            FMath::RandRange(-WorldSize * 0.5f, WorldSize * 0.5f),
            FMath::RandRange(-WorldSize * 0.5f, WorldSize * 0.5f),
            90.f
        };

        ASteeringAgent* agent = pWorld->SpawnActor<ASteeringAgent>(
            AgentClass, spawnLoc, FRotator::ZeroRotator);

        if (!IsValid(agent))
        {
            UE_LOG(LogTemp, Error, TEXT("Flock: Failed to spawn agent %d"), i);
            continue;
        }
        Agents.Add(agent);
    }
}

void Flock::CreateBehaviors()
{
    pSeparationBehavior = std::make_unique<Separation>(this);
    pCohesionBehavior   = std::make_unique<Cohesion>(this);
    pVelMatchBehavior   = std::make_unique<VelocityMatch>(this);
    pSeekBehavior       = std::make_unique<Seek>();
    pWanderBehavior     = std::make_unique<Wander>();
    pEvadeBehavior      = std::make_unique<Evade>();

    pBlendedSteering = std::make_unique<BlendedSteering>(
        std::vector<BlendedSteering::WeightedBehavior>{
            { pSeparationBehavior.get(), 2.0f },
            { pCohesionBehavior.get(),   1.5f },
            { pVelMatchBehavior.get(),   1.5f },
            { pSeekBehavior.get(),       1.0f },
            { pWanderBehavior.get(),     0.5f },
        }
    );
    
    pPrioritySteering = std::make_unique<PrioritySteering>(
        std::vector<ISteeringBehavior*>{
            pEvadeBehavior.get(),
            pBlendedSteering.get()
        }
    );
    
    if (IsValid(pAgentToEvade))
    {
        FTargetData evadeTarget;
        evadeTarget.Position       = pAgentToEvade->GetPosition();
        evadeTarget.LinearVelocity = pAgentToEvade->GetLinearVelocity();
        pEvadeBehavior->SetTarget(evadeTarget);
    }
}

void Flock::AssignBehaviors()
{
    for (auto& agent : Agents)
        if (IsValid(agent))
            agent->SetSteeringBehavior(pPrioritySteering.get());
}

void Flock::DestroyAgents()
{
    for (auto& agent : Agents)
        if (IsValid(agent))
            agent->Destroy();
    Agents.Empty();
}

#ifdef GAMEAI_USE_SPACE_PARTITIONING
void Flock::InitPartitioning()
{
    const float fullWidth  = WorldSize * 2.f;
    const float fullHeight = WorldSize * 2.f;

    pPartitionedSpace = std::make_unique<CellSpace>(
        pWorld,
        fullWidth, fullHeight,
        NrOfCellsY, NrOfCellsX,
        Agents.Num()
    );

    OldPositions.SetNum(Agents.Num());
    for (int i = 0; i < Agents.Num(); ++i)
    {
        if (IsValid(Agents[i]))
        {
            OldPositions[i] = Agents[i]->GetPosition();
            pPartitionedSpace->AddAgent(*Agents[i]);
        }
    }
}
#endif

void Flock::Reset()
{
    DestroyAgents();

    FlockSize = PendingFlockSize;

    SpawnAgents();
    CreateBehaviors();
    AssignBehaviors();

#ifdef GAMEAI_USE_SPACE_PARTITIONING
    InitPartitioning();
#endif

    // Re-apply seek target if one was set
    if (bHasSeekTarget)
        pSeekBehavior->SetTarget(SeekTarget);
}



// ------------
// --- Tick ---
// ------------

void Flock::Tick(float DeltaTime)
{
    // Check if reset is pending
    if (bPendingReset)
    {
        bPendingReset = false;
        Reset();
        return;
    }

    // Refresh evade target position/velocity every frame
    if (IsValid(pAgentToEvade))
    {
        FTargetData evadeTarget;
        evadeTarget.Position       = pAgentToEvade->GetPosition();
        evadeTarget.LinearVelocity = pAgentToEvade->GetLinearVelocity();
        pEvadeBehavior->SetTarget(evadeTarget);
    }

#ifdef GAMEAI_USE_SPACE_PARTITIONING
    // Refresh every agent's cell using last-frame positions
    for (int i = 0; i < Agents.Num(); ++i)
    {
        ASteeringAgent* agent = Agents[i];
        if (!IsValid(agent)) continue;
        pPartitionedSpace->UpdateAgentCell(*agent, OldPositions[i]);
        OldPositions[i] = agent->GetPosition();
    }
#endif

    // Register neighbors then tick each agent
    for (int i = 0; i < Agents.Num(); ++i)
    {
        ASteeringAgent* agent = Agents[i];
        if (!IsValid(agent)) continue;

#ifdef GAMEAI_USE_SPACE_PARTITIONING
        pPartitionedSpace->RegisterNeighbors(*agent, NeighborhoodRadius);
#else
        RegisterNeighbors(agent);
#endif

        agent->Tick(DeltaTime);
    }
}



// -----------------------
// --- Debug rendering ---
// -----------------------

void Flock::RenderDebug()
{
    if (bDebugRenderNeighborhood)
        RenderNeighborhood();

#ifdef GAMEAI_USE_SPACE_PARTITIONING
    if (bDebugRenderPartitions)
        pPartitionedSpace->RenderCells();
#endif
}

void Flock::RenderNeighborhood()
{
    if (Agents.Num() == 0 || !pWorld) return;

    ASteeringAgent* focus = nullptr;
    for (auto& a : Agents)
        if (IsValid(a)) { focus = a; break; }

    if (!focus) return;

    const FVector agentLoc3D{ focus->GetActorLocation() };

    DrawDebugCircle(pWorld, agentLoc3D, NeighborhoodRadius,
        32, FColor::Cyan, false, -1.f, 0, 2.f,
        FVector::ForwardVector, FVector::RightVector);

    const auto& neighbours = GetNeighbors();
    const int   count      = GetNrOfNeighbors();
    for (int i = 0; i < count; ++i)
    {
        if (!IsValid(neighbours[i])) continue;
        DrawDebugSphere(pWorld, neighbours[i]->GetActorLocation(),
            20.f, 8, FColor::Green, false, -1.f, 0, 2.f);
        DrawDebugLine(pWorld, agentLoc3D, neighbours[i]->GetActorLocation(),
            FColor::Green, false, -1.f, 0, 1.f);
    }

    DrawDebugSphere(pWorld, agentLoc3D, 25.f, 8, FColor::Orange, false, -1.f, 0, 3.f);
}


// ----------------
// --- ImGui UI ---
// ----------------

void Flock::ImGuiRender(ImVec2 const& WindowPos, ImVec2 const& WindowSize)
{
#ifdef PLATFORM_WINDOWS
    bool bWindowActive = true;
    ImGui::SetNextWindowPos(WindowPos);
    ImGui::SetNextWindowSize(WindowSize);
    ImGui::Begin("Gameplay Programming", &bWindowActive,
        ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoCollapse);

    // Controls
    ImGui::TextColored(ImVec4(1,1,0,1), "CONTROLS");
    ImGui::Indent();
    ImGui::Text("LMB: place seek target");
    ImGui::Text("RMB: move camera");
    ImGui::Text("Scroll: zoom camera");
    ImGui::Unindent();

    ImGui::Spacing();
    ImGui::Separator();
    ImGui::Spacing();

    // Stats
    ImGui::TextColored(ImVec4(1,1,0,1), "STATS");
    ImGui::Indent();
    ImGui::Text("%.3f ms/frame", 1000.0f / ImGui::GetIO().Framerate);
    ImGui::Text("%.1f FPS",      ImGui::GetIO().Framerate);
    ImGui::Text("Agents: %d",    Agents.Num());
    ImGui::Text("Neighbors (agent[0]): %d", GetNrOfNeighbors());
    ImGui::Unindent();

    ImGui::Spacing();
    ImGui::Separator();
    ImGui::Spacing();

    // Reset
    ImGui::TextColored(ImVec4(1,1,0,1), "Simulation");
    ImGui::Spacing();

    ImGui::Text("Agent count:");
    ImGui::SameLine();
    ImGui::SetNextItemWidth(80.f);
    ImGui::InputInt("##agentcount", &PendingFlockSize);
    PendingFlockSize = FMath::Clamp(PendingFlockSize, 1, 500);

    ImGui::Spacing();

    ImGui::PushStyleColor(ImGuiCol_Button,        ImVec4(0.6f, 0.1f, 0.1f, 1.f));
    ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImVec4(0.8f, 0.2f, 0.2f, 1.f));
    ImGui::PushStyleColor(ImGuiCol_ButtonActive,  ImVec4(1.0f, 0.3f, 0.3f, 1.f));
    if (ImGui::Button("Reset Simulation", ImVec2(-1.f, 0.f)))
        bPendingReset = true;
    ImGui::PopStyleColor(3);

    ImGui::Spacing();
    ImGui::Separator();
    ImGui::Spacing();

    // Debug rendering
    ImGui::TextColored(ImVec4(1,1,0,1), "Debug Rendering");
    ImGui::Spacing();

    ImGui::Checkbox("Show Neighborhood",   &bDebugRenderNeighborhood);
#ifdef GAMEAI_USE_SPACE_PARTITIONING
    ImGui::Checkbox("Show Partitions",     &bDebugRenderPartitions);
#else
    ImGui::BeginDisabled();
    bool dummy = false;
    ImGui::Checkbox("Show Partitions (disabled - define GAMEAI_USE_SPACE_PARTITIONING)", &dummy);
    ImGui::EndDisabled();
#endif

    ImGui::Spacing();
    ImGui::Separator();
    ImGui::Spacing();

    // Neighborhood radius
    ImGui::TextColored(ImVec4(1,1,0,1), "Neighborhood");
    ImGui::Spacing();
    ImGui::SliderFloat("Radius", &NeighborhoodRadius, 50.f, 800.f, "%.0f");

    ImGui::Spacing();
    ImGui::Separator();
    ImGui::Spacing();

    // Behavior weights
    ImGui::TextColored(ImVec4(1,1,0,1), "Behavior Weights");
    ImGui::Spacing();

    auto weightSlider = [&](const char* label, ISteeringBehavior* pBehavior)
    {
        float* w = pBlendedSteering->GetWeight(pBehavior);
        if (w) ImGui::SliderFloat(label, w, 0.f, 5.f, "%.2f");
    };

    weightSlider("Separation",     pSeparationBehavior.get());
    weightSlider("Cohesion",       pCohesionBehavior.get());
    weightSlider("Velocity Match", pVelMatchBehavior.get());
    weightSlider("Seek",           pSeekBehavior.get());
    weightSlider("Wander",         pWanderBehavior.get());

    if (IsValid(pAgentToEvade))
    {
        ImGui::Spacing();
        ImGui::SliderFloat("Evade Radius", &pEvadeBehavior->EvadeRadius, 100.f, 2000.f, "%.0f");
    }

    ImGui::Spacing();
    if (ImGui::Button("Reset Weights"))
    {
        auto& wb = pBlendedSteering->GetWeightedBehaviorsRef();
        for (auto& entry : wb) entry.Weight = 1.0f;
    }

    ImGui::End();
#endif
}



// ----------------------------------------------------
// --- Neighbor registration (non-partitioned path) ---
// ----------------------------------------------------

#ifndef GAMEAI_USE_SPACE_PARTITIONING
void Flock::RegisterNeighbors(ASteeringAgent* const pAgent)
{
    NrOfNeighbors = 0;
    Neighbors.Empty();

    const FVector2D agentPos = pAgent->GetPosition();

    for (auto& agent : Agents)
    {
        if (agent == pAgent || !IsValid(agent)) continue;

        float dist = FVector2D::Distance(agentPos, agent->GetPosition());
        if (dist < NeighborhoodRadius)
            Neighbors.Add(agent);
    }

    NrOfNeighbors = Neighbors.Num();
}
#endif



// --------------------------------
// --- Average neighbor queries ---
// --------------------------------

FVector2D Flock::GetAverageNeighborPos() const
{
    int count = GetNrOfNeighbors();
    if (count == 0) return FVector2D::ZeroVector;

    const auto& neighbours = GetNeighbors();
    FVector2D avg = FVector2D::ZeroVector;
    for (int i = 0; i < count; ++i)
        avg += neighbours[i]->GetPosition();

    return avg / static_cast<float>(count);
}

FVector2D Flock::GetAverageNeighborVelocity() const
{
    int count = GetNrOfNeighbors();
    if (count == 0) return FVector2D::ZeroVector;

    const auto& neighbours = GetNeighbors();
    FVector2D avg = FVector2D::ZeroVector;
    for (int i = 0; i < count; ++i)
        avg += neighbours[i]->GetLinearVelocity();

    return avg / static_cast<float>(count);
}



// -------------------
// --- Seek target ---
// -------------------

void Flock::SetTarget_Seek(FSteeringParams const& Target)
{
    pSeekBehavior->SetTarget(Target);
    SeekTarget     = Target;
    bHasSeekTarget = true;
}