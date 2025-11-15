// GCGGameMode_2v2.cpp - 2v2 Team Battle Game Mode Implementation
// Unreal Engine 5.6 - Gundam TCG Implementation

#include "GCGGameMode_2v2.h"
#include "GundamTCG/GameState/GCGGameState.h"
#include "GundamTCG/PlayerState/GCGPlayerState.h"
#include "GundamTCG/Subsystems/GCGZoneSubsystem.h"
#include "GundamTCG/Subsystems/GCGCombatSubsystem.h"
#include "TimerManager.h"
#include "Engine/World.h"

AGCGGameMode_2v2::AGCGGameMode_2v2()
{
	// 2v2 specific settings
	MaxUnitsPerTeam = 6;
	ShieldsPerTeam = 8;
	ShieldsPerPlayer = 4;
}

// ===========================================================================================
// GAME INITIALIZATION
// ===========================================================================================

void AGCGGameMode_2v2::InitializeGame()
{
	UE_LOG(LogTemp, Log, TEXT("AGCGGameMode_2v2::InitializeGame - Starting 2v2 Team Battle initialization"));

	if (!CanStartGame())
	{
		UE_LOG(LogTemp, Error, TEXT("AGCGGameMode_2v2::InitializeGame - Cannot start game (need 4 players)"));
		return;
	}

	// Setup teams
	SetupTeams();

	// Get game state
	AGCGGameState* GCGGameState = GetGameState<AGCGGameState>();
	if (!GCGGameState)
	{
		UE_LOG(LogTemp, Error, TEXT("AGCGGameMode_2v2::InitializeGame - Game state not found"));
		return;
	}

	// Mark as team battle
	GCGGameState->bIsTeamBattle = true;
	GCGGameState->TeamA = TeamA;
	GCGGameState->TeamB = TeamB;

	// Setup decks for all 4 players (would be called from Blueprint with actual deck lists)
	UE_LOG(LogTemp, Log, TEXT("AGCGGameMode_2v2::InitializeGame - Decks should be setup via Blueprint before calling InitializeGame"));

	// Setup Team A shared shields (8 shields: 4 from Player 0, 4 from Player 2, alternating)
	SetupTeamShields(0);

	// Setup Team B shared shields (8 shields: 4 from Player 1, 4 from Player 3, alternating)
	SetupTeamShields(1);

	// Setup Team A shared EX Base
	SetupTeamEXBase(0);

	// Setup Team B shared EX Base
	SetupTeamEXBase(1);

	// Setup EX Resources for Team B (going second advantage)
	SetupTeamEXResources(1);

	// Draw initial 5-card hands for all players
	UGCGZoneSubsystem* ZoneSubsystem = GetGameInstance()->GetSubsystem<UGCGZoneSubsystem>();
	if (ZoneSubsystem)
	{
		for (int32 PlayerID = 0; PlayerID < 4; PlayerID++)
		{
			AGCGPlayerState* PlayerState = GetPlayerStateByID(PlayerID);
			if (PlayerState)
			{
				// Draw 5 cards
				TArray<FGCGCardInstance> DrawnCards = ZoneSubsystem->DrawTopCards(PlayerState, EGCGCardZone::Deck, 5);
				for (const FGCGCardInstance& Card : DrawnCards)
				{
					ZoneSubsystem->MoveCard(PlayerState, Card.InstanceID, EGCGCardZone::Deck, EGCGCardZone::Hand);
				}

				UE_LOG(LogTemp, Log, TEXT("AGCGGameMode_2v2::InitializeGame - Player %d drew %d starting cards"),
					PlayerID, DrawnCards.Num());
			}
		}
	}

	// Start first turn (Team A goes first)
	GCGGameState->bGameInProgress = true;
	StartNewTurn();

	UE_LOG(LogTemp, Log, TEXT("AGCGGameMode_2v2::InitializeGame - 2v2 Team Battle initialized successfully"));
}

bool AGCGGameMode_2v2::CanStartGame() const
{
	// Need exactly 4 players for 2v2
	TArray<AGCGPlayerState*> PlayerStates = GetAllPlayerStates();
	return PlayerStates.Num() == 4;
}

// ===========================================================================================
// TEAM MANAGEMENT
// ===========================================================================================

void AGCGGameMode_2v2::SetupTeams()
{
	UE_LOG(LogTemp, Log, TEXT("AGCGGameMode_2v2::SetupTeams - Setting up teams"));

	// Team A: Players 0 and 2
	TeamA.TeamID = 0;
	TeamA.PlayerIDs.Empty();
	TeamA.PlayerIDs.Add(0);
	TeamA.PlayerIDs.Add(2);
	TeamA.TeamLeaderID = 0; // Player 0 is team leader by default
	TeamA.TotalUnitsOnField = 0;

	// Team B: Players 1 and 3
	TeamB.TeamID = 1;
	TeamB.PlayerIDs.Empty();
	TeamB.PlayerIDs.Add(1);
	TeamB.PlayerIDs.Add(3);
	TeamB.TeamLeaderID = 1; // Player 1 is team leader by default
	TeamB.TotalUnitsOnField = 0;

	UE_LOG(LogTemp, Log, TEXT("AGCGGameMode_2v2::SetupTeams - Team A: Players %d, %d (Leader: %d)"),
		TeamA.PlayerIDs[0], TeamA.PlayerIDs[1], TeamA.TeamLeaderID);
	UE_LOG(LogTemp, Log, TEXT("AGCGGameMode_2v2::SetupTeams - Team B: Players %d, %d (Leader: %d)"),
		TeamB.PlayerIDs[0], TeamB.PlayerIDs[1], TeamB.TeamLeaderID);
}

FGCGTeamInfo* AGCGGameMode_2v2::GetTeamForPlayer(int32 PlayerID)
{
	if (TeamA.PlayerIDs.Contains(PlayerID))
	{
		return &TeamA;
	}
	else if (TeamB.PlayerIDs.Contains(PlayerID))
	{
		return &TeamB;
	}
	return nullptr;
}

int32 AGCGGameMode_2v2::GetTeammateID(int32 PlayerID) const
{
	if (TeamA.PlayerIDs.Contains(PlayerID))
	{
		// Find the other player on Team A
		for (int32 ID : TeamA.PlayerIDs)
		{
			if (ID != PlayerID)
			{
				return ID;
			}
		}
	}
	else if (TeamB.PlayerIDs.Contains(PlayerID))
	{
		// Find the other player on Team B
		for (int32 ID : TeamB.PlayerIDs)
		{
			if (ID != PlayerID)
			{
				return ID;
			}
		}
	}
	return -1;
}

bool AGCGGameMode_2v2::AreTeammates(int32 PlayerID1, int32 PlayerID2) const
{
	if (TeamA.PlayerIDs.Contains(PlayerID1) && TeamA.PlayerIDs.Contains(PlayerID2))
	{
		return true;
	}
	if (TeamB.PlayerIDs.Contains(PlayerID1) && TeamB.PlayerIDs.Contains(PlayerID2))
	{
		return true;
	}
	return false;
}

int32 AGCGGameMode_2v2::GetTeamUnitCount(int32 TeamID) const
{
	int32 TotalUnits = 0;

	const FGCGTeamInfo* Team = (TeamID == 0) ? &TeamA : &TeamB;
	if (!Team)
	{
		return 0;
	}

	// Count Units in Battle Area for all players on the team
	for (int32 PlayerID : Team->PlayerIDs)
	{
		AGCGPlayerState* PlayerState = GetPlayerStateByID(PlayerID);
		if (PlayerState)
		{
			TotalUnits += PlayerState->GetUnitCount();
		}
	}

	return TotalUnits;
}

bool AGCGGameMode_2v2::CanTeamAddUnit(int32 TeamID) const
{
	return GetTeamUnitCount(TeamID) < MaxUnitsPerTeam;
}

// ===========================================================================================
// TURN MANAGEMENT
// ===========================================================================================

void AGCGGameMode_2v2::StartNewTurn()
{
	UE_LOG(LogTemp, Log, TEXT("AGCGGameMode_2v2::StartNewTurn - Starting new team turn"));

	AGCGGameState* GCGGameState = GetGameState<AGCGGameState>();
	if (!GCGGameState)
	{
		UE_LOG(LogTemp, Error, TEXT("AGCGGameMode_2v2::StartNewTurn - Game state not found"));
		return;
	}

	// Increment turn number
	GCGGameState->TurnNumber++;

	// Switch active team
	int32 NewTeamID = GetNextTeamID(GCGGameState->ActivePlayerID); // Reuse ActivePlayerID for team tracking
	GCGGameState->ActivePlayerID = NewTeamID;

	// Get team
	const FGCGTeamInfo* ActiveTeam = (NewTeamID == 0) ? &TeamA : &TeamB;
	if (!ActiveTeam)
	{
		UE_LOG(LogTemp, Error, TEXT("AGCGGameMode_2v2::StartNewTurn - Active team not found"));
		return;
	}

	UE_LOG(LogTemp, Log, TEXT("AGCGGameMode_2v2::StartNewTurn - Turn %d, Team %d (Players %d, %d)"),
		GCGGameState->TurnNumber, NewTeamID, ActiveTeam->PlayerIDs[0], ActiveTeam->PlayerIDs[1]);

	// Enter Start Phase
	GCGGameState->CurrentPhase = EGCGTurnPhase::StartPhase;
	ExecuteStartPhase();
}

void AGCGGameMode_2v2::EndTurn()
{
	UE_LOG(LogTemp, Log, TEXT("AGCGGameMode_2v2::EndTurn - Ending team turn"));

	AGCGGameState* GCGGameState = GetGameState<AGCGGameState>();
	if (!GCGGameState)
	{
		return;
	}

	// Enter End Phase
	GCGGameState->CurrentPhase = EGCGTurnPhase::EndPhase;
	ExecuteEndPhase();

	// Start next team's turn
	StartNewTurn();
}

// ===========================================================================================
// SETUP HELPERS
// ===========================================================================================

void AGCGGameMode_2v2::SetupTeamShields(int32 TeamID)
{
	UE_LOG(LogTemp, Log, TEXT("AGCGGameMode_2v2::SetupTeamShields - Setting up shields for Team %d"), TeamID);

	FGCGTeamInfo* Team = (TeamID == 0) ? &TeamA : &TeamB;
	if (!Team)
	{
		UE_LOG(LogTemp, Error, TEXT("AGCGGameMode_2v2::SetupTeamShields - Team not found"));
		return;
	}

	if (Team->PlayerIDs.Num() != 2)
	{
		UE_LOG(LogTemp, Error, TEXT("AGCGGameMode_2v2::SetupTeamShields - Team does not have 2 players"));
		return;
	}

	int32 Player1ID = Team->PlayerIDs[0];
	int32 Player2ID = Team->PlayerIDs[1];

	AGCGPlayerState* Player1State = GetPlayerStateByID(Player1ID);
	AGCGPlayerState* Player2State = GetPlayerStateByID(Player2ID);

	if (!Player1State || !Player2State)
	{
		UE_LOG(LogTemp, Error, TEXT("AGCGGameMode_2v2::SetupTeamShields - Player states not found"));
		return;
	}

	UGCGZoneSubsystem* ZoneSubsystem = GetGameInstance()->GetSubsystem<UGCGZoneSubsystem>();
	if (!ZoneSubsystem)
	{
		UE_LOG(LogTemp, Error, TEXT("AGCGGameMode_2v2::SetupTeamShields - Zone subsystem not found"));
		return;
	}

	// Setup shared shield stack (8 shields: 4 from each player, alternating)
	// Order: P1, P2, P1, P2, P1, P2, P1, P2 (from top to bottom)
	Team->SharedShieldStack.Empty();

	for (int32 i = 0; i < ShieldsPerPlayer; i++)
	{
		// Draw from Player 1's deck
		TArray<FGCGCardInstance> P1Cards = ZoneSubsystem->DrawTopCards(Player1State, EGCGCardZone::Deck, 1);
		if (P1Cards.Num() > 0)
		{
			Team->SharedShieldStack.Add(P1Cards[0]);
		}

		// Draw from Player 2's deck
		TArray<FGCGCardInstance> P2Cards = ZoneSubsystem->DrawTopCards(Player2State, EGCGCardZone::Deck, 1);
		if (P2Cards.Num() > 0)
		{
			Team->SharedShieldStack.Add(P2Cards[0]);
		}
	}

	UE_LOG(LogTemp, Log, TEXT("AGCGGameMode_2v2::SetupTeamShields - Team %d shield stack setup with %d shields"),
		TeamID, Team->SharedShieldStack.Num());
}

void AGCGGameMode_2v2::SetupTeamEXBase(int32 TeamID)
{
	UE_LOG(LogTemp, Log, TEXT("AGCGGameMode_2v2::SetupTeamEXBase - Setting up EX Base for Team %d"), TeamID);

	FGCGTeamInfo* Team = (TeamID == 0) ? &TeamA : &TeamB;
	if (!Team)
	{
		UE_LOG(LogTemp, Error, TEXT("AGCGGameMode_2v2::SetupTeamEXBase - Team not found"));
		return;
	}

	// Create EX Base token (0 AP, 3 HP)
	FGCGCardInstance EXBase = CreateTokenInstance(TEXT("EXBase"), Team->TeamLeaderID);
	EXBase.CardType = EGCGCardType::Base;
	EXBase.AP = 0;
	EXBase.HP = 3;
	EXBase.bIsActive = true;
	EXBase.CardName = FText::FromString("EX Base");
	EXBase.CurrentZone = EGCGCardZone::BaseSection;

	Team->SharedBase = EXBase;

	UE_LOG(LogTemp, Log, TEXT("AGCGGameMode_2v2::SetupTeamEXBase - Team %d EX Base created (0 AP, 3 HP)"), TeamID);
}

void AGCGGameMode_2v2::SetupTeamEXResources(int32 TeamID)
{
	UE_LOG(LogTemp, Log, TEXT("AGCGGameMode_2v2::SetupTeamEXResources - Setting up EX Resources for Team %d"), TeamID);

	FGCGTeamInfo* Team = (TeamID == 0) ? &TeamA : &TeamB;
	if (!Team)
	{
		UE_LOG(LogTemp, Error, TEXT("AGCGGameMode_2v2::SetupTeamEXResources - Team not found"));
		return;
	}

	// Give each player on Team B 1 EX Resource (going second advantage)
	for (int32 PlayerID : Team->PlayerIDs)
	{
		AGCGPlayerState* PlayerState = GetPlayerStateByID(PlayerID);
		if (!PlayerState)
		{
			continue;
		}

		// Create EX Resource token
		FGCGCardInstance EXResource = CreateTokenInstance(TEXT("EXResource"), PlayerID);
		EXResource.CardType = EGCGCardType::Resource;
		EXResource.AP = 0;
		EXResource.HP = 0;
		EXResource.bIsActive = true;
		EXResource.CardName = FText::FromString("EX Resource");
		EXResource.CurrentZone = EGCGCardZone::ResourceArea;

		// Add to player's Resource Area
		PlayerState->ResourceArea.Add(EXResource);

		UE_LOG(LogTemp, Log, TEXT("AGCGGameMode_2v2::SetupTeamEXResources - Player %d received EX Resource"), PlayerID);
	}
}

// ===========================================================================================
// PLAYER ACTIONS
// ===========================================================================================

bool AGCGGameMode_2v2::CanPlayerAct(int32 PlayerID) const
{
	AGCGGameState* GCGGameState = GetGameState<AGCGGameState>();
	if (!GCGGameState || !GCGGameState->bGameInProgress)
	{
		return false;
	}

	// In 2v2, both players on the active team can act
	int32 ActiveTeamID = GCGGameState->ActivePlayerID; // ActivePlayerID stores team ID in 2v2
	const FGCGTeamInfo* ActiveTeam = (ActiveTeamID == 0) ? &TeamA : &TeamB;

	if (!ActiveTeam)
	{
		return false;
	}

	return ActiveTeam->PlayerIDs.Contains(PlayerID);
}

// ===========================================================================================
// COMBAT
// ===========================================================================================

bool AGCGGameMode_2v2::RequestDeclareAttack_2v2(int32 PlayerID, int32 AttackerInstanceID, int32 TargetPlayerID)
{
	UE_LOG(LogTemp, Log, TEXT("AGCGGameMode_2v2::RequestDeclareAttack_2v2 - Player %d attacking Player %d with Unit %d"),
		PlayerID, TargetPlayerID, AttackerInstanceID);

	// Validate player can act
	if (!CanPlayerAct(PlayerID))
	{
		UE_LOG(LogTemp, Warning, TEXT("AGCGGameMode_2v2::RequestDeclareAttack_2v2 - Player %d cannot act"), PlayerID);
		return false;
	}

	// Validate target is an opponent
	if (AreTeammates(PlayerID, TargetPlayerID))
	{
		UE_LOG(LogTemp, Warning, TEXT("AGCGGameMode_2v2::RequestDeclareAttack_2v2 - Cannot attack teammate"));
		return false;
	}

	// Use base implementation (1v1 attack logic still works)
	return RequestDeclareAttack(PlayerID, AttackerInstanceID);
}

bool AGCGGameMode_2v2::RequestDeclareBlocker_2v2(int32 PlayerID, int32 AttackIndex, int32 BlockerInstanceID)
{
	UE_LOG(LogTemp, Log, TEXT("AGCGGameMode_2v2::RequestDeclareBlocker_2v2 - Player %d blocking attack %d with Unit %d"),
		PlayerID, AttackIndex, BlockerInstanceID);

	// Validate player can act (teammates can block for each other)
	AGCGGameState* GCGGameState = GetGameState<AGCGGameState>();
	if (!GCGGameState || !GCGGameState->CurrentAttacks.IsValidIndex(AttackIndex))
	{
		return false;
	}

	FGCGAttackDeclaration& Attack = GCGGameState->CurrentAttacks[AttackIndex];

	// Check if player is on the defending team
	int32 DefendingPlayerID = Attack.DefendingPlayerID;
	if (!AreTeammates(PlayerID, DefendingPlayerID))
	{
		UE_LOG(LogTemp, Warning, TEXT("AGCGGameMode_2v2::RequestDeclareBlocker_2v2 - Cannot block for opponent"));
		return false;
	}

	// Use base implementation (blocker logic works with current player)
	return RequestDeclareBlocker(PlayerID, AttackIndex, BlockerInstanceID);
}

// ===========================================================================================
// VICTORY CONDITIONS
// ===========================================================================================

void AGCGGameMode_2v2::CheckTeamVictoryCondition(int32 TeamID)
{
	FGCGTeamInfo* Team = (TeamID == 0) ? &TeamA : &TeamB;
	if (!Team)
	{
		return;
	}

	// Check if shared Base is destroyed
	if (Team->SharedBase.IsDestroyed())
	{
		UE_LOG(LogTemp, Log, TEXT("AGCGGameMode_2v2::CheckTeamVictoryCondition - Team %d Base destroyed"), TeamID);

		// Other team wins
		int32 WinningTeamID = (TeamID == 0) ? 1 : 0;
		EndGameTeamVictory(WinningTeamID);
	}

	// Check if both players on the team have lost (empty decks)
	bool bAllPlayersLost = true;
	for (int32 PlayerID : Team->PlayerIDs)
	{
		AGCGPlayerState* PlayerState = GetPlayerStateByID(PlayerID);
		if (PlayerState && !PlayerState->bHasLost)
		{
			bAllPlayersLost = false;
			break;
		}
	}

	if (bAllPlayersLost)
	{
		UE_LOG(LogTemp, Log, TEXT("AGCGGameMode_2v2::CheckTeamVictoryCondition - Team %d all players lost"), TeamID);

		// Other team wins
		int32 WinningTeamID = (TeamID == 0) ? 1 : 0;
		EndGameTeamVictory(WinningTeamID);
	}
}

void AGCGGameMode_2v2::EndGameTeamVictory(int32 WinningTeamID)
{
	UE_LOG(LogTemp, Log, TEXT("AGCGGameMode_2v2::EndGameTeamVictory - Team %d wins!"), WinningTeamID);

	AGCGGameState* GCGGameState = GetGameState<AGCGGameState>();
	if (!GCGGameState)
	{
		return;
	}

	GCGGameState->bGameInProgress = false;
	GCGGameState->CurrentPhase = EGCGTurnPhase::GameOver;
	GCGGameState->WinnerPlayerID = WinningTeamID; // Store winning team ID in WinnerPlayerID

	// Mark all players on losing team as lost
	int32 LosingTeamID = (WinningTeamID == 0) ? 1 : 0;
	const FGCGTeamInfo* LosingTeam = (LosingTeamID == 0) ? &TeamA : &TeamB;

	if (LosingTeam)
	{
		for (int32 PlayerID : LosingTeam->PlayerIDs)
		{
			AGCGPlayerState* PlayerState = GetPlayerStateByID(PlayerID);
			if (PlayerState)
			{
				PlayerState->bHasLost = true;
			}
		}
	}

	UE_LOG(LogTemp, Log, TEXT("AGCGGameMode_2v2::EndGameTeamVictory - Game Over - Team %d victory!"), WinningTeamID);

	// TODO: Trigger Blueprint event for victory screen
}

// ===========================================================================================
// INTERNAL HELPERS
// ===========================================================================================

int32 AGCGGameMode_2v2::GetNextTeamID(int32 CurrentTeamID) const
{
	// Alternate between Team 0 and Team 1
	return (CurrentTeamID == 0) ? 1 : 0;
}

void AGCGGameMode_2v2::ActivateAllCardsForTeam(int32 TeamID)
{
	const FGCGTeamInfo* Team = (TeamID == 0) ? &TeamA : &TeamB;
	if (!Team)
	{
		return;
	}

	// Activate all cards for both players on the team
	for (int32 PlayerID : Team->PlayerIDs)
	{
		ActivateAllCardsForPlayer(PlayerID);
	}
}

void AGCGGameMode_2v2::ProcessHandLimitForTeam(int32 TeamID)
{
	const FGCGTeamInfo* Team = (TeamID == 0) ? &TeamA : &TeamB;
	if (!Team)
	{
		return;
	}

	// Process hand limit for both players on the team
	for (int32 PlayerID : Team->PlayerIDs)
	{
		ProcessHandLimit(PlayerID);
	}
}

TArray<int32> AGCGGameMode_2v2::GetPlayersOnTeam(int32 TeamID) const
{
	const FGCGTeamInfo* Team = (TeamID == 0) ? &TeamA : &TeamB;
	if (Team)
	{
		return Team->PlayerIDs;
	}
	return TArray<int32>();
}
