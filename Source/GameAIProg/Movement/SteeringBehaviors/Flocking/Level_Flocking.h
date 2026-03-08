#pragma once

#include "CoreMinimal.h"
#include "Flock.h"
#include "Shared/Level_Base.h"
#include "Level_Flocking.generated.h"

UCLASS()
class GAMEAIPROG_API ALevel_Flocking : public ALevel_Base
{
	GENERATED_BODY()

public:
	ALevel_Flocking();
	virtual void Tick(float DeltaTime) override;

protected:
	virtual void BeginPlay() override;

	bool bUseMouseTarget{ true };

	UPROPERTY(EditAnywhere, Category = "Flocking")
	int FlockSize{ 50 };

	TUniquePtr<Flock> pFlock{};

	//Evade target agent
	UPROPERTY(EditAnywhere, Category = "Flocking|EvadeTarget",
		meta = (DisplayName = "Evade Agent Class"))
	TSubclassOf<ASteeringAgent> EvadeAgentClass{};

	UPROPERTY(Transient)
	ASteeringAgent* pEvadeAgent{ nullptr };

	std::unique_ptr<Wander> pEvadeWander{};

private:
	void SpawnEvadeAgent();
	void TickEvadeAgent(float DeltaTime);
	void DebugDrawEvadeAgent() const;
};