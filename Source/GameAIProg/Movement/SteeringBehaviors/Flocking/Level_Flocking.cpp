#include "Level_Flocking.h"
#include "DrawDebugHelpers.h"

ALevel_Flocking::ALevel_Flocking()
{
	PrimaryActorTick.bCanEverTick = true;
}

void ALevel_Flocking::BeginPlay()
{
	Super::BeginPlay();

	TrimWorld->SetTrimWorldSize(3000.f);
	TrimWorld->bShouldTrimWorld = true;

	SpawnEvadeAgent();

	pFlock = TUniquePtr<Flock>(
		new Flock(
			GetWorld(),
			SteeringAgentClass,
			FlockSize,
			TrimWorld->GetTrimWorldSize(),
			pEvadeAgent,
			true)
	);
}

void ALevel_Flocking::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

	TickEvadeAgent(DeltaTime);

	pFlock->ImGuiRender(WindowPos, WindowSize);
	pFlock->Tick(DeltaTime);
	pFlock->RenderDebug();

	if (bUseMouseTarget)
		pFlock->SetTarget_Seek(MouseTarget);

	DebugDrawEvadeAgent();
}

// ─────────────────────────────────────────────────────────────────────────────

void ALevel_Flocking::SpawnEvadeAgent()
{
	if (!EvadeAgentClass)
	{
		UE_LOG(LogTemp, Warning,
			TEXT("ALevel_Flocking: EvadeAgentClass not set — no evade agent spawned. "
			     "Create a Blueprint child of ASteeringAgent with a distinct mesh/colour."));
		return;
	}

	FVector spawnLoc{ 500.f, 500.f, 90.f };

	pEvadeAgent = GetWorld()->SpawnActor<ASteeringAgent>(
		EvadeAgentClass, spawnLoc, FRotator::ZeroRotator);

	if (!IsValid(pEvadeAgent))
	{
		UE_LOG(LogTemp, Error, TEXT("ALevel_Flocking: Failed to spawn evade agent."));
		return;
	}

	pEvadeWander = std::make_unique<Wander>();
	pEvadeAgent->SetSteeringBehavior(pEvadeWander.get());

	UE_LOG(LogTemp, Log, TEXT("ALevel_Flocking: Evade agent spawned."));
}

void ALevel_Flocking::TickEvadeAgent(float DeltaTime)
{
	if (!IsValid(pEvadeAgent)) return;

	pEvadeAgent->Tick(DeltaTime);
}

void ALevel_Flocking::DebugDrawEvadeAgent() const
{
	if (!IsValid(pEvadeAgent) || !GetWorld()) return;

	const FVector loc = pEvadeAgent->GetActorLocation();

	DrawDebugSphere(GetWorld(), loc, 60.f, 16, FColor::Red,    false, -1.f, 0, 4.f);
	DrawDebugCircle(GetWorld(), loc, 90.f, 32, FColor::Orange, false, -1.f, 0, 3.f,
		FVector::ForwardVector, FVector::RightVector);
	DrawDebugString(GetWorld(), loc + FVector(0, 0, 120.f),
		TEXT("EVADE TARGET"), nullptr, FColor::Red, 0.f);
}