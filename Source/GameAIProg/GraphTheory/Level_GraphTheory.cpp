// Fill out your copyright notice in the Description page of Project Settings.


#include "Level_GraphTheory.h"

#include "Algorithms/EulerianPath.h"
#include "Shared/GameAISpectator.h"

using namespace GameAI;

// Sets default values
ALevel_GraphTheory::ALevel_GraphTheory()
{
	// Set this actor to call Tick() every frame.  You can turn this off to improve performance if you don't need it.
	PrimaryActorTick.bCanEverTick = true;
}

// Called when the game starts or when spawned
void ALevel_GraphTheory::BeginPlay()
{
	Super::BeginPlay();
	
	// Add the graph editor to our player
	if (PlayerController = Cast<APlayerController>(GetWorld()->GetFirstLocalPlayerFromController()->PlayerController); 
		GraphEditorClass && PlayerController)
	{
		PlayerGraphEditor = NewObject<UGraphEditorComponent>(PlayerController->GetPawn(), GraphEditorClass);
		PlayerGraphEditor->RegisterComponent();
		PlayerGraphEditor->SetEditedGraph(&Graph);
		PlayerGraphEditor->SetNodeFactory(&NodeFactory);
	}
	else
	{
		UE_LOG(LogTemp, Error, TEXT("Unable to get PlayerController from LocalPlayer or GraphEditorClass is null"))
		return;
	}
	
	// Set the world of graph renderer (needs to be done in BeginPlay)
	Renderer.SetWorld(GetWorld());
	
	// Make the view orthogonal for less perspective issues
	if (AGameAISpectator* Player = Cast<AGameAISpectator>(PlayerController->GetPawnOrSpectator()); Player)
	{
		Player->SetCameraProjection(ECameraProjectionMode::Orthographic);
	}
	
	// Create a few starter nodes
	Graph.AddNode(NodeFactory.CreateNode(FVector2D{ -200.f,  200.f })); // node 0  (top-left)
	Graph.AddNode(NodeFactory.CreateNode(FVector2D{  200.f,  200.f })); // node 1  (top-right)
	Graph.AddNode(NodeFactory.CreateNode(FVector2D{  200.f, -200.f })); // node 2  (bottom-right)
	Graph.AddNode(NodeFactory.CreateNode(FVector2D{ -200.f, -200.f })); // node 3  (bottom-left)
 
	// Four sides of the square
	Graph.AddConnection(0, 1);
	Graph.AddConnection(1, 2);
	Graph.AddConnection(2, 3);
	Graph.AddConnection(3, 0);
	// Two diagonals
	Graph.AddConnection(0, 2);
	Graph.AddConnection(1, 3);
	
	// Spawn the Agent
	Agent = GetWorld()->SpawnActor<ASteeringAgent>(SteeringAgentClass, 
	FVector{0,0,90}, FRotator::ZeroRotator);
	Agent->SetSteeringBehavior(&PathFollow);
}

void ALevel_GraphTheory::BeginDestroy()
{
	Super::BeginDestroy();
}

void ALevel_GraphTheory::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);
	
#pragma region UI
	{
		//Setup
		bool windowActive = true;
		ImGui::SetNextWindowPos(WindowPos);
		ImGui::SetNextWindowSize(WindowSize);
		ImGui::Begin("Gameplay Programming", &windowActive, ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoCollapse);
		ImGui::SetWindowFocus();
		ImGui::PushItemWidth(70);
		//Elements
		ImGui::Text("CONTROLS");
		ImGui::Indent();
		ImGui::Unindent();

		ImGui::Spacing();
		ImGui::Separator();
		ImGui::Spacing();
		ImGui::Spacing();

		ImGui::Text("STATS");
		ImGui::Indent();
		ImGui::Text("%.3f ms/frame", 1000.0f / ImGui::GetIO().Framerate);
		ImGui::Text("%.1f FPS", ImGui::GetIO().Framerate);
		ImGui::Unindent();

		ImGui::Spacing();
		ImGui::Separator();
		ImGui::Spacing();
		ImGui::Spacing();

		ImGui::Text("Graph Theory");
		if (ImGui::Button("Reset Path"))
		{
			ResetPath();
		}
		ImGui::Spacing();
		ImGui::Spacing();

		//End
		ImGui::End();
	}
#pragma endregion UI
	
	Renderer.RenderGraph(Graph);
	
	// Check if the graph has updated
	if (PlayerGraphEditor && PlayerGraphEditor->HasGraphUpdated())
	{
		ResetPath();
	}
}

void ALevel_GraphTheory::ResetPath()
{
	// Run the EulerianPath algorithm on the (possibly changed) graph
	EulerianPath eulerianPath(&Graph);
	Eulerianity eulerianity;
	std::vector<Node*> trail = eulerianPath.FindPath(eulerianity);
 
	// If a valid path was found, update the agent's target path
	if (!trail.empty())
	{
		UpdateAgentPath(trail);
	}
}

void ALevel_GraphTheory::UpdateAgentPath(std::vector<Node*> const& Trail)
{
	std::vector<FVector2D> path(Trail.size());
	
	// Fill the path vector with node positions
	std::ranges::transform(Trail, path.begin(),
	                       [](const Node* node)
	                       {
		                       return node->GetPosition();
	                       });
	
	// Set agent path
	PathFollow.SetPath(path);
	if (path.size() > 0)
	{
		Agent->SetPosition(path[0]);
	}
}




