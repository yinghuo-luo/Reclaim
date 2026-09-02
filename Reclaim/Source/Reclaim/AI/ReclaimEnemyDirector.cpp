// Copyright Epic Games, Inc. All Rights Reserved.

#include "AI/ReclaimEnemyDirector.h"

#include "AI/ReclaimEnemyDefinition.h"
#include "AI/ReclaimEnemySpawnPoint.h"
#include "Characters/ReclaimEnemyCharacter.h"
#include "Core/ReclaimDeterministicRandom.h"
#include "Core/ReclaimGameplayTags.h"
#include "EngineUtils.h"
#include "GameFramework/GameStateBase.h"
#include "GameModes/ReclaimMissionGameState.h"
#include "Net/UnrealNetwork.h"
#include "Player/ReclaimPlayerState.h"
#include "TimerManager.h"

AReclaimEnemyDirector::AReclaimEnemyDirector()
{
	bReplicates = true;
	PrimaryActorTick.bCanEverTick = false;
}

FReclaimDirectorScalingResult AReclaimEnemyDirector::ResolveScaling(const UReclaimAIConfig* Config, int32 InEffectivePlayerCount)
{
	if (Config)
	{
		return Config->ResolveScaling(InEffectivePlayerCount);
	}

	const UReclaimAIConfig* DefaultConfig = GetDefault<UReclaimAIConfig>();
	return DefaultConfig ? DefaultConfig->ResolveScaling(InEffectivePlayerCount) : FReclaimDirectorScalingResult();
}

int32 AReclaimEnemyDirector::ComputeThreatBudget(int32 BaseBudget, float InThreatBudgetScalar)
{
	return FMath::Max(0, FMath::RoundToInt(static_cast<float>(FMath::Max(0, BaseBudget)) * FMath::Max(0.0f, InThreatBudgetScalar)));
}

FReclaimThreatBudgetSolution AReclaimEnemyDirector::SolveThreatBudget(int32 Budget, int32 InSpecialUnitCap, int32 CompositionSeed, const TArray<FReclaimEnemyBudgetEntry>& Entries)
{
	FReclaimThreatBudgetSolution Solution;
	Solution.Budget = FMath::Max(0, Budget);

	if (Solution.Budget <= 0)
	{
		return Solution;
	}

	TArray<FReclaimEnemyBudgetEntry> LegalEntries;
	for (const FReclaimEnemyBudgetEntry& Entry : Entries)
	{
		if (!Entry.bLegal || Entry.ThreatCost <= 0 || Entry.ThreatCost > Solution.Budget || Entry.SpawnWeight <= 0 || Entry.EnemyId.IsNone())
		{
			continue;
		}

		LegalEntries.Add(Entry);
	}

	LegalEntries.Sort([](const FReclaimEnemyBudgetEntry& Left, const FReclaimEnemyBudgetEntry& Right)
	{
		const int32 CostCompare = Left.ThreatCost - Right.ThreatCost;
		if (CostCompare != 0)
		{
			return CostCompare < 0;
		}

		return Left.EnemyId.LexicalLess(Right.EnemyId);
	});

	FRandomStream Stream(CompositionSeed);
	const int32 SpecialCap = FMath::Max(0, InSpecialUnitCap);
	int32 Guard = 0;
	while (Guard++ < 64)
	{
		TArray<const FReclaimEnemyBudgetEntry*> Candidates;
		int32 TotalWeight = 0;
		for (const FReclaimEnemyBudgetEntry& Entry : LegalEntries)
		{
			if (Solution.ThreatSpent + Entry.ThreatCost > Solution.Budget)
			{
				continue;
			}

			if (Entry.bSpecialUnit && Solution.SpecialUnits >= SpecialCap)
			{
				continue;
			}

			Candidates.Add(&Entry);
			TotalWeight += FMath::Max(0, Entry.SpawnWeight);
		}

		if (Candidates.Num() <= 0 || TotalWeight <= 0)
		{
			break;
		}

		const int32 Roll = Stream.RandRange(1, TotalWeight);
		int32 RunningWeight = 0;
		const FReclaimEnemyBudgetEntry* SelectedEntry = Candidates[0];
		for (const FReclaimEnemyBudgetEntry* Candidate : Candidates)
		{
			RunningWeight += FMath::Max(0, Candidate->SpawnWeight);
			if (Roll <= RunningWeight)
			{
				SelectedEntry = Candidate;
				break;
			}
		}

		FReclaimEnemySpawnRequest& Request = Solution.Requests.AddDefaulted_GetRef();
		Request.EnemyId = SelectedEntry->EnemyId;
		Request.EnemyDefinition = SelectedEntry->EnemyDefinition;
		Request.ThreatCost = SelectedEntry->ThreatCost;
		Request.bSpecialUnit = SelectedEntry->bSpecialUnit;
		Solution.ThreatSpent += SelectedEntry->ThreatCost;
		if (SelectedEntry->bSpecialUnit)
		{
			++Solution.SpecialUnits;
		}
	}

	TMap<FName, int32> CountsById;
	for (const FReclaimEnemySpawnRequest& Request : Solution.Requests)
	{
		CountsById.FindOrAdd(Request.EnemyId)++;
	}

	TArray<FName> SortedIds;
	CountsById.GetKeys(SortedIds);
	SortedIds.Sort([](const FName& Left, const FName& Right)
	{
		return Left.LexicalLess(Right);
	});

	TArray<FString> Parts;
	for (const FName& EnemyId : SortedIds)
	{
		Parts.Add(FString::Printf(TEXT("%sx%d"), *EnemyId.ToString(), CountsById.FindChecked(EnemyId)));
	}
	Solution.CompositionSummary = FString::Join(Parts, TEXT(", "));
	return Solution;
}

int32 AReclaimEnemyDirector::CompareSpawnCandidatesDeterministic(const FVector& LeftLocation, FName LeftStableId, const FVector& RightLocation, FName RightStableId)
{
	if (!FMath::IsNearlyEqual(LeftLocation.X, RightLocation.X))
	{
		return LeftLocation.X < RightLocation.X ? -1 : 1;
	}

	if (!FMath::IsNearlyEqual(LeftLocation.Y, RightLocation.Y))
	{
		return LeftLocation.Y < RightLocation.Y ? -1 : 1;
	}

	if (!FMath::IsNearlyEqual(LeftLocation.Z, RightLocation.Z))
	{
		return LeftLocation.Z < RightLocation.Z ? -1 : 1;
	}

	if (LeftStableId == RightStableId)
	{
		return 0;
	}

	return LeftStableId.LexicalLess(RightStableId) ? -1 : 1;
}

float AReclaimEnemyDirector::ComputeSpawnVisibilityPreferenceScore(EReclaimSpawnVisibilityRule Rule, bool bOccludedFromActivePlayers)
{
	if (Rule == EReclaimSpawnVisibilityRule::PreferOccluded)
	{
		return bOccludedFromActivePlayers ? 3.0f : 1.0f;
	}

	return 1.0f;
}

bool AReclaimEnemyDirector::DoesPlayerStateCountForEffectivePlayers(const AReclaimPlayerState* PlayerState)
{
	return PlayerState != nullptr && !PlayerState->IsOnlyASpectator();
}

void AReclaimEnemyDirector::RefreshThreatBudget(int32 InEffectivePlayerCount)
{
	if (!HasAuthority())
	{
		return;
	}

	const FReclaimDirectorScalingResult Scaling = ResolveScaling(AIConfig, InEffectivePlayerCount);
	EffectivePlayerCount = FMath::Max(0, InEffectivePlayerCount);
	ThreatBudgetScalar = Scaling.ThreatBudgetScalar;
	EnemyHealthScalar = Scaling.EnemyHealthScalar;
	SpecialUnitCap = Scaling.SpecialUnitCap;
	const int32 BaseBudget = AIConfig ? AIConfig->BaseThreatBudget : GetDefault<UReclaimAIConfig>()->BaseThreatBudget;
	CurrentBudget = ComputeThreatBudget(BaseBudget, ThreatBudgetScalar);
	CurrentThreat = CalculateAliveThreat_Server();
}

bool AReclaimEnemyDirector::SpawnNextWave_Server()
{
	if (!HasAuthority())
	{
		return false;
	}

	RefreshDebugState_Server();
	if (EffectivePlayerCount <= 0 || IsCurrentIntensityTooHigh_Server())
	{
		ScheduleSpawnRetry_Server();
		return false;
	}

	const int32 AliveSpecialUnits = CalculateAliveSpecialUnits_Server();
	const int32 RemainingSpecialCap = FMath::Max(0, SpecialUnitCap - AliveSpecialUnits);
	const int32 AvailableBudget = FMath::Max(0, CurrentBudget - CurrentThreat);
	const int32 CompositionSeed = UReclaimDeterministicRandom::MakeSubSeed(GetRunSeed_Server(), EReclaimRandomDomain::EncounterComposition, WaveIndex, AvailableBudget, RemainingSpecialCap);
	const FReclaimThreatBudgetSolution Solution = SolveThreatBudget(AvailableBudget, RemainingSpecialCap, CompositionSeed, BuildBudgetEntries());
	LastSpawnComposition = Solution.CompositionSummary;

	if (Solution.Requests.Num() <= 0)
	{
		ScheduleSpawnRetry_Server();
		return false;
	}

	bool bSpawnedAny = false;
	for (int32 RequestIndex = 0; RequestIndex < Solution.Requests.Num(); ++RequestIndex)
	{
		const FReclaimEnemySpawnRequest& Request = Solution.Requests[RequestIndex];
		UReclaimEnemyDefinition* Definition = Request.EnemyDefinition;
		if (!Definition)
		{
			continue;
		}

		AReclaimEnemySpawnPoint* SpawnPoint = ChooseSpawnPoint_Server(RequestIndex, CompositionSeed);
		if (!SpawnPoint)
		{
			continue;
		}

		TSubclassOf<AReclaimEnemyCharacter> EnemyClass = Definition->PawnClass ? Definition->PawnClass : DefaultEnemyClass;
		if (!EnemyClass)
		{
			EnemyClass = AReclaimEnemyCharacter::StaticClass();
		}

		FActorSpawnParameters SpawnParameters;
		SpawnParameters.Owner = this;
		SpawnParameters.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AdjustIfPossibleButAlwaysSpawn;
		AReclaimEnemyCharacter* Enemy = GetWorld()->SpawnActor<AReclaimEnemyCharacter>(EnemyClass, SpawnPoint->GetActorTransform(), SpawnParameters);
		if (!Enemy)
		{
			continue;
		}

		const int32 EnemySeed = UReclaimDeterministicRandom::MakeSubSeed(GetRunSeed_Server(), EReclaimRandomDomain::AICombatDecision, WaveIndex, RequestIndex);
		Enemy->InitializeEnemy_Server(Definition, AIConfig ? AIConfig : Definition->AIConfig, EnemyHealthScalar, EnemySeed, this);
		RegisterEnemy(Enemy);
		bSpawnedAny = true;
	}

	if (!bSpawnedAny)
	{
		ScheduleSpawnRetry_Server();
		RefreshDebugState_Server();
		return false;
	}

	++WaveIndex;
	RefreshDebugState_Server();
	return bSpawnedAny;
}

void AReclaimEnemyDirector::RegisterEnemy(AReclaimEnemyCharacter* EnemyCharacter)
{
	if (!HasAuthority() || !EnemyCharacter)
	{
		return;
	}

	AliveEnemies.RemoveAll([](const TWeakObjectPtr<AReclaimEnemyCharacter>& Candidate)
	{
		return !Candidate.IsValid();
	});

	if (!AliveEnemies.Contains(EnemyCharacter))
	{
		AliveEnemies.Add(EnemyCharacter);
	}

	RefreshDebugState_Server();
}

void AReclaimEnemyDirector::UnregisterEnemy(AReclaimEnemyCharacter* EnemyCharacter)
{
	if (!HasAuthority())
	{
		return;
	}

	AliveEnemies.RemoveAll([EnemyCharacter](const TWeakObjectPtr<AReclaimEnemyCharacter>& Candidate)
	{
		return !Candidate.IsValid() || Candidate.Get() == EnemyCharacter;
	});
	RefreshDebugState_Server();
}

void AReclaimEnemyDirector::NotifyEnemyDied(AReclaimEnemyCharacter* EnemyCharacter)
{
	UnregisterEnemy(EnemyCharacter);
}

FString AReclaimEnemyDirector::GetThreatDebugString() const
{
#if UE_BUILD_SHIPPING
	return FString();
#else
	return FString::Printf(
		TEXT("Authority=%s EffectivePlayerCount=%d ThreatBudgetScalar=%.2f EnemyHealthScalar=%.2f SpecialUnitCap=%d CurrentBudget=%d CurrentThreat=%d LastSpawn=%s"),
		HasAuthority() ? TEXT("true") : TEXT("false"),
		EffectivePlayerCount,
		ThreatBudgetScalar,
		EnemyHealthScalar,
		SpecialUnitCap,
		CurrentBudget,
		CurrentThreat,
		*LastSpawnComposition);
#endif
}

void AReclaimEnemyDirector::BeginPlay()
{
	Super::BeginPlay();

	if (!HasAuthority())
	{
		return;
	}

	RefreshDebugState_Server();
	if (bAutoSpawnCombatSandboxWave)
	{
		if (UWorld* World = GetWorld())
		{
			World->GetTimerManager().SetTimer(SpawnRetryTimerHandle, this, &AReclaimEnemyDirector::SpawnNextWaveTimer_Server, FMath::Max(0.0f, AutoSpawnDelaySeconds), false);
		}
	}
}

TArray<FReclaimEnemyBudgetEntry> AReclaimEnemyDirector::BuildBudgetEntries() const
{
	TArray<FReclaimEnemyBudgetEntry> Entries;
	for (UReclaimEnemyDefinition* Definition : EnemyRoster)
	{
		if (!Definition)
		{
			continue;
		}

		FReclaimEnemyBudgetEntry& Entry = Entries.AddDefaulted_GetRef();
		Entry.EnemyId = Definition->EnemyId;
		Entry.EnemyDefinition = Definition;
		Entry.ThreatCost = FMath::Max(1, Definition->ThreatCost);
		Entry.SpawnWeight = FMath::Max(0, Definition->SpawnWeight);
		Entry.bSpecialUnit = Definition->bCountsTowardSpecialUnitCap;
		Entry.bLegal = true;
	}

	return Entries;
}

AReclaimEnemySpawnPoint* AReclaimEnemyDirector::ChooseSpawnPoint_Server(int32 RequestIndex, int32 CompositionSeed) const
{
	struct FScoredSpawnPoint
	{
		AReclaimEnemySpawnPoint* SpawnPoint = nullptr;
		float Score = 1.0f;
	};

	TArray<FScoredSpawnPoint> Candidates;
	for (AReclaimEnemySpawnPoint* ExplicitPoint : ExplicitSpawnPoints)
	{
		if (ExplicitPoint && ExplicitPoint->PassesHardSpawnRules_Server(AIConfig))
		{
			FScoredSpawnPoint& Candidate = Candidates.AddDefaulted_GetRef();
			Candidate.SpawnPoint = ExplicitPoint;
			Candidate.Score = ExplicitPoint->ComputeSpawnPreferenceScore_Server(AIConfig);
		}
	}

	if (Candidates.Num() <= 0)
	{
		UWorld* World = GetWorld();
		if (!World)
		{
			return nullptr;
		}

		for (TActorIterator<AReclaimEnemySpawnPoint> It(World); It; ++It)
		{
			AReclaimEnemySpawnPoint* SpawnPoint = *It;
			if (SpawnPoint && SpawnPoint->PassesHardSpawnRules_Server(AIConfig))
			{
				FScoredSpawnPoint& Candidate = Candidates.AddDefaulted_GetRef();
				Candidate.SpawnPoint = SpawnPoint;
				Candidate.Score = SpawnPoint->ComputeSpawnPreferenceScore_Server(AIConfig);
			}
		}
	}

	if (Candidates.Num() <= 0)
	{
		return nullptr;
	}

	Candidates.Sort([](const FScoredSpawnPoint& Left, const FScoredSpawnPoint& Right)
	{
		if (!FMath::IsNearlyEqual(Left.Score, Right.Score))
		{
			return Left.Score > Right.Score;
		}

		return Left.SpawnPoint
			&& Right.SpawnPoint
			&& AReclaimEnemyDirector::CompareSpawnCandidatesDeterministic(Left.SpawnPoint->GetActorLocation(), Left.SpawnPoint->GetStableSpawnId(), Right.SpawnPoint->GetActorLocation(), Right.SpawnPoint->GetStableSpawnId()) < 0;
	});

	const int32 PointSeed = UReclaimDeterministicRandom::MakeSubSeed(CompositionSeed, EReclaimRandomDomain::EncounterComposition, RequestIndex);
	FRandomStream Stream(PointSeed);
	int32 TotalWeight = 0;
	for (const FScoredSpawnPoint& Candidate : Candidates)
	{
		TotalWeight += FMath::Max(1, FMath::RoundToInt(Candidate.Score * 100.0f));
	}

	int32 Roll = Stream.RandRange(1, FMath::Max(1, TotalWeight));
	for (const FScoredSpawnPoint& Candidate : Candidates)
	{
		Roll -= FMath::Max(1, FMath::RoundToInt(Candidate.Score * 100.0f));
		if (Roll <= 0)
		{
			return Candidate.SpawnPoint;
		}
	}

	return Candidates.Last().SpawnPoint;
}

bool AReclaimEnemyDirector::IsCurrentIntensityTooHigh_Server() const
{
	const UReclaimAIConfig* Config = AIConfig ? AIConfig : GetDefault<UReclaimAIConfig>();
	if (!Config || CurrentBudget <= 0)
	{
		return false;
	}

	const int32 MaxAllowedThreat = FMath::RoundToInt(static_cast<float>(CurrentBudget) * FMath::Max(0.0f, Config->MaxAliveThreatRatio));
	return CurrentThreat >= MaxAllowedThreat;
}

void AReclaimEnemyDirector::RefreshDebugState_Server()
{
	RefreshThreatBudget(CalculateEffectivePlayerCount_Server());
	LastSpawnComposition = LastSpawnComposition.IsEmpty() ? TEXT("None") : LastSpawnComposition;
}

int32 AReclaimEnemyDirector::CalculateEffectivePlayerCount_Server() const
{
	const UWorld* World = GetWorld();
	const AGameStateBase* GameState = World ? World->GetGameState() : nullptr;
	if (!GameState)
	{
		return 0;
	}

	int32 Count = 0;
	for (APlayerState* PlayerState : GameState->PlayerArray)
	{
		if (DoesPlayerStateCountForEffectivePlayers(Cast<AReclaimPlayerState>(PlayerState)))
		{
			++Count;
		}
	}

	return FMath::Clamp(Count, 0, 4);
}

int32 AReclaimEnemyDirector::CalculateAliveThreat_Server() const
{
	int32 Threat = 0;
	for (const TWeakObjectPtr<AReclaimEnemyCharacter>& EnemyPtr : AliveEnemies)
	{
		const AReclaimEnemyCharacter* Enemy = EnemyPtr.Get();
		if (Enemy && !Enemy->IsDead())
		{
			Threat += Enemy->GetThreatCost();
		}
	}

	return Threat;
}

int32 AReclaimEnemyDirector::CalculateAliveSpecialUnits_Server() const
{
	int32 Count = 0;
	for (const TWeakObjectPtr<AReclaimEnemyCharacter>& EnemyPtr : AliveEnemies)
	{
		const AReclaimEnemyCharacter* Enemy = EnemyPtr.Get();
		if (Enemy && !Enemy->IsDead() && Enemy->CountsTowardSpecialUnitCap())
		{
			++Count;
		}
	}

	return Count;
}

int32 AReclaimEnemyDirector::GetRunSeed_Server() const
{
	const UWorld* World = GetWorld();
	const AReclaimMissionGameState* MissionGameState = World ? World->GetGameState<AReclaimMissionGameState>() : nullptr;
	return MissionGameState ? MissionGameState->GetRunSeed() : 0;
}

void AReclaimEnemyDirector::ScheduleSpawnRetry_Server()
{
	if (!HasAuthority())
	{
		return;
	}

	if (UWorld* World = GetWorld())
	{
		const UReclaimAIConfig* Config = AIConfig ? AIConfig : GetDefault<UReclaimAIConfig>();
		const float RetryDelay = Config ? FMath::Max(0.0f, Config->SpawnRetryDelaySeconds) : 2.0f;
		if (!World->GetTimerManager().IsTimerActive(SpawnRetryTimerHandle))
		{
			World->GetTimerManager().SetTimer(SpawnRetryTimerHandle, this, &AReclaimEnemyDirector::SpawnNextWaveTimer_Server, RetryDelay, false);
		}
	}
}

void AReclaimEnemyDirector::SpawnNextWaveTimer_Server()
{
	SpawnNextWave_Server();
}

void AReclaimEnemyDirector::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);
	DOREPLIFETIME(AReclaimEnemyDirector, EffectivePlayerCount);
	DOREPLIFETIME(AReclaimEnemyDirector, ThreatBudgetScalar);
	DOREPLIFETIME(AReclaimEnemyDirector, EnemyHealthScalar);
	DOREPLIFETIME(AReclaimEnemyDirector, SpecialUnitCap);
	DOREPLIFETIME(AReclaimEnemyDirector, CurrentBudget);
	DOREPLIFETIME(AReclaimEnemyDirector, CurrentThreat);
	DOREPLIFETIME(AReclaimEnemyDirector, LastSpawnComposition);
}
