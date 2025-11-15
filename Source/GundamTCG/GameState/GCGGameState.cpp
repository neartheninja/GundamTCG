// GCGGameState.cpp - Replicated Game State Implementation
// Unreal Engine 5.6 - Gundam TCG Implementation

#include "GCGGameState.h"
#include "Net/UnrealNetwork.h"

AGCGGameState::AGCGGameState()
{
	// Initialize game status
	bGameInProgress = false;
	bGameOver = false;
	WinnerPlayerID = -1;

	// Initialize turn tracking
	TurnNumber = 0;
	CurrentPhase = EGCGTurnPhase::NotStarted;
	CurrentStartPhaseStep = EGCGStartPhaseStep::None;
	CurrentEndPhaseStep = EGCGEndPhaseStep::None;
	ActivePlayerID = 0;

	// Initialize combat tracking
	bAttackInProgress = false;

	// Initialize team battle
	bIsTeamBattle = false;
	TeamA.TeamID = 0;
	TeamB.TeamID = 1;

	// Enable replication
	bReplicates = true;
	bAlwaysRelevant = true;
}

void AGCGGameState::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);

	// Replicate game status
	DOREPLIFETIME(AGCGGameState, bGameInProgress);
	DOREPLIFETIME(AGCGGameState, bGameOver);
	DOREPLIFETIME(AGCGGameState, WinnerPlayerID);

	// Replicate turn tracking
	DOREPLIFETIME(AGCGGameState, TurnNumber);
	DOREPLIFETIME(AGCGGameState, CurrentPhase);
	DOREPLIFETIME(AGCGGameState, CurrentStartPhaseStep);
	DOREPLIFETIME(AGCGGameState, CurrentEndPhaseStep);
	DOREPLIFETIME(AGCGGameState, ActivePlayerID);

	// Replicate combat tracking
	DOREPLIFETIME(AGCGGameState, bAttackInProgress);
	DOREPLIFETIME(AGCGGameState, CurrentAttack);

	// Replicate team battle
	DOREPLIFETIME(AGCGGameState, bIsTeamBattle);
	DOREPLIFETIME(AGCGGameState, TeamA);
	DOREPLIFETIME(AGCGGameState, TeamB);
}

// ===== REPLICATION CALLBACKS =====

void AGCGGameState::OnRep_TurnNumber()
{
	UE_LOG(LogTemp, Log, TEXT("AGCGGameState::OnRep_TurnNumber: Turn %d"), TurnNumber);

	// Call Blueprint event
	OnTurnNumberChanged(TurnNumber);
}

void AGCGGameState::OnRep_CurrentPhase()
{
	UE_LOG(LogTemp, Log, TEXT("AGCGGameState::OnRep_CurrentPhase: Phase changed to %s"), *GetPhaseName());

	// Call Blueprint event
	OnPhaseChanged(CurrentPhase);
}

void AGCGGameState::OnRep_ActivePlayerID()
{
	UE_LOG(LogTemp, Log, TEXT("AGCGGameState::OnRep_ActivePlayerID: Active player is now %d"), ActivePlayerID);

	// Call Blueprint event
	OnActivePlayerChanged(ActivePlayerID);
}

// ===== HELPER FUNCTIONS =====

const FGCGTeamInfo* AGCGGameState::GetTeamForPlayer(int32 PlayerID) const
{
	if (!bIsTeamBattle)
	{
		return nullptr;
	}

	// Check Team A
	if (TeamA.PlayerIDs.Contains(PlayerID))
	{
		return &TeamA;
	}

	// Check Team B
	if (TeamB.PlayerIDs.Contains(PlayerID))
	{
		return &TeamB;
	}

	return nullptr;
}

bool AGCGGameState::IsPlayerActive(int32 PlayerID) const
{
	if (bIsTeamBattle)
	{
		// In team battle, check if player's team is active
		return IsPlayerTeamActive(PlayerID);
	}
	else
	{
		// In 1v1, check if this specific player is active
		return ActivePlayerID == PlayerID;
	}
}

bool AGCGGameState::IsPlayerTeamActive(int32 PlayerID) const
{
	if (!bIsTeamBattle)
	{
		return IsPlayerActive(PlayerID);
	}

	// Get player's team
	const FGCGTeamInfo* PlayerTeam = GetTeamForPlayer(PlayerID);
	if (!PlayerTeam)
	{
		return false;
	}

	// Get active player's team
	const FGCGTeamInfo* ActiveTeam = GetTeamForPlayer(ActivePlayerID);
	if (!ActiveTeam)
	{
		return false;
	}

	// Check if both teams are the same
	return PlayerTeam->TeamID == ActiveTeam->TeamID;
}

int32 AGCGGameState::GetPlayerTeamID(int32 PlayerID) const
{
	const FGCGTeamInfo* Team = GetTeamForPlayer(PlayerID);
	return Team ? Team->TeamID : -1;
}

bool AGCGGameState::ArePlayersTeammates(int32 PlayerID1, int32 PlayerID2) const
{
	if (!bIsTeamBattle)
	{
		return false;
	}

	int32 Team1 = GetPlayerTeamID(PlayerID1);
	int32 Team2 = GetPlayerTeamID(PlayerID2);

	return (Team1 != -1 && Team1 == Team2);
}

FString AGCGGameState::GetPhaseName() const
{
	switch (CurrentPhase)
	{
	case EGCGTurnPhase::NotStarted:
		return TEXT("Not Started");
	case EGCGTurnPhase::StartPhase:
		return TEXT("Start Phase");
	case EGCGTurnPhase::DrawPhase:
		return TEXT("Draw Phase");
	case EGCGTurnPhase::ResourcePhase:
		return TEXT("Resource Phase");
	case EGCGTurnPhase::MainPhase:
		return TEXT("Main Phase");
	case EGCGTurnPhase::EndPhase:
		return TEXT("End Phase");
	case EGCGTurnPhase::GameOver:
		return TEXT("Game Over");
	default:
		return TEXT("Unknown Phase");
	}
}

FString AGCGGameState::GetStepName() const
{
	if (CurrentPhase == EGCGTurnPhase::StartPhase)
	{
		switch (CurrentStartPhaseStep)
		{
		case EGCGStartPhaseStep::ActiveStep:
			return TEXT("Active Step");
		case EGCGStartPhaseStep::StartStep:
			return TEXT("Start Step");
		default:
			return TEXT("");
		}
	}
	else if (CurrentPhase == EGCGTurnPhase::EndPhase)
	{
		switch (CurrentEndPhaseStep)
		{
		case EGCGEndPhaseStep::ActionStep:
			return TEXT("Action Step");
		case EGCGEndPhaseStep::EndStep:
			return TEXT("End Step");
		case EGCGEndPhaseStep::HandStep:
			return TEXT("Hand Step");
		case EGCGEndPhaseStep::CleanupStep:
			return TEXT("Cleanup Step");
		default:
			return TEXT("");
		}
	}

	return TEXT("");
}
