// GCGGameMode_BattleRoyale.cpp - Battle Royale Game Mode Implementation
// Unreal Engine 5.6 - Gundam TCG Implementation
// Section 12-2: Battle Royale Rules

#include "GCGGameMode_BattleRoyale.h"
#include "GundamTCG/GameState/GCGGameState.h"
#include "GundamTCG/PlayerState/GCGPlayerState.h"
#include "GundamTCG/Subsystems/GCGZoneSubsystem.h"
#include "GundamTCG/Subsystems/GCGCombatSubsystem.h"

AGCGGameMode_BattleRoyale::AGCGGameMode_BattleRoyale()
{
	// Rule 12-2-5: Default to Last Player Standing mode
	VictoryMode = EGCGBattleRoyaleVictoryMode::LastPlayerStanding;

	// Rule 12-2-1: Default to clockwise (Player One chooses)
	TurnDirection = EGCGTurnDirection::Clockwise;

	// Battle Royale player count
	MinPlayers = 3;
	MaxPlayers = 8; // Reasonable upper limit

	LastPlayerToActInActionStep = -1;
}

// ===========================================================================================
// GAME INITIALIZATION
// ===========================================================================================

void AGCGGameMode_BattleRoyale::InitializeGame()
{
	UE_LOG(LogTemp, Log, TEXT("[GCGGameMode_BattleRoyale] Initializing Battle Royale game..."));

	// Get game state
	AGCGGameState* GCGGameState = GetGameState<AGCGGameState>();
	if (!GCGGameState)
	{
		UE_LOG(LogTemp, Error, TEXT("[GCGGameMode_BattleRoyale] Failed to get game state"));
		return;
	}

	// Validate player count
	int32 PlayerCount = GCGGameState->PlayerStates.Num();
	if (PlayerCount < MinPlayers)
	{
		UE_LOG(LogTemp, Error, TEXT("[GCGGameMode_BattleRoyale] Not enough players (%d, need at least %d)"),
			PlayerCount, MinPlayers);
		return;
	}

	if (PlayerCount > MaxPlayers)
	{
		UE_LOG(LogTemp, Error, TEXT("[GCGGameMode_BattleRoyale] Too many players (%d, max is %d)"),
			PlayerCount, MaxPlayers);
		return;
	}

	// Initialize active player list
	ActivePlayerIDs.Empty();
	for (int32 i = 0; i < PlayerCount; i++)
	{
		ActivePlayerIDs.Add(i);
	}

	UE_LOG(LogTemp, Log, TEXT("[GCGGameMode_BattleRoyale] Battle Royale with %d players"), PlayerCount);

	// Call base 1v1 initialization (handles decks, shields, etc.)
	// Note: Will need to override some parts for multiplayer
	Super::InitializeGame();

	// Rule 12-2-2: EX Resources for all players except Player One
	SetupBattleRoyaleEXResources();

	// Rule 12-2-3: Mulligan in order
	PerformOrderedMulligan();

	UE_LOG(LogTemp, Log, TEXT("[GCGGameMode_BattleRoyale] Battle Royale initialization complete"));
}

void AGCGGameMode_BattleRoyale::SetupBattleRoyaleEXResources()
{
	// Rule 12-2-2: All players except Player One place EX Resources

	AGCGGameState* GCGGameState = GetGameState<AGCGGameState>();
	if (!GCGGameState)
	{
		return;
	}

	// Player 0 (Player One) does NOT get EX Resource
	// All other players get 1 EX Resource each
	for (int32 PlayerID : ActivePlayerIDs)
	{
		if (PlayerID == 0)
		{
			// Player One does not place EX Resource
			continue;
		}

		AGCGPlayerState* PlayerState = GetPlayerStateByID(PlayerID);
		if (!PlayerState)
		{
			continue;
		}

		// Create EX Resource token
		FGCGCardInstance EXResource;
		EXResource.bIsToken = true;
		EXResource.TokenType = FName("EXResource");
		EXResource.InstanceID = GCGGameState->GetNextCardInstanceID();
		EXResource.OwnerPlayerID = PlayerID;
		EXResource.ControllerPlayerID = PlayerID;
		EXResource.CurrentZone = EGCGCardZone::ResourceArea;
		EXResource.bIsActive = true; // EX Resources start active

		// Add to Resource Area
		PlayerState->ResourceArea.Add(EXResource);

		UE_LOG(LogTemp, Log, TEXT("[GCGGameMode_BattleRoyale] Player %d placed EX Resource"), PlayerID);
	}
}

void AGCGGameMode_BattleRoyale::PerformOrderedMulligan()
{
	// Rule 12-2-3: Redrawing of hands is performed in order starting with Player One

	AGCGGameState* GCGGameState = GetGameState<AGCGGameState>();
	if (!GCGGameState)
	{
		return;
	}

	// Process mulligan for each player in order (Player 0, 1, 2, ...)
	for (int32 PlayerID : ActivePlayerIDs)
	{
		AGCGPlayerState* PlayerState = GetPlayerStateByID(PlayerID);
		if (!PlayerState)
		{
			continue;
		}

		// TODO: Trigger mulligan UI for this player
		// For now, auto-skip mulligan
		UE_LOG(LogTemp, Log, TEXT("[GCGGameMode_BattleRoyale] Player %d mulligan check (auto-skipped)"), PlayerID);
	}
}

void AGCGGameMode_BattleRoyale::SetTurnDirection(bool bClockwise)
{
	// Rule 12-2-1: Player One chooses clockwise or counterclockwise

	TurnDirection = bClockwise ? EGCGTurnDirection::Clockwise : EGCGTurnDirection::Counterclockwise;

	UE_LOG(LogTemp, Log, TEXT("[GCGGameMode_BattleRoyale] Turn direction set to %s"),
		bClockwise ? TEXT("Clockwise") : TEXT("Counterclockwise"));
}

// ===========================================================================================
// TURN MANAGEMENT
// ===========================================================================================

void AGCGGameMode_BattleRoyale::StartNewTurn()
{
	// Rule 12-2-1: Turn moves to next player in chosen direction

	AGCGGameState* GCGGameState = GetGameState<AGCGGameState>();
	if (!GCGGameState)
	{
		return;
	}

	// Increment turn number
	GCGGameState->TurnNumber++;

	// Get next active player
	int32 NextPlayerID = GetNextPlayerID(GCGGameState->ActivePlayerID);
	GCGGameState->ActivePlayerID = NextPlayerID;

	// Reset phase
	GCGGameState->CurrentPhase = EGCGTurnPhase::StartPhase;

	UE_LOG(LogTemp, Log, TEXT("[GCGGameMode_BattleRoyale] Turn %d: Player %d's turn"),
		GCGGameState->TurnNumber, NextPlayerID);

	// Execute Start Phase
	ExecuteStartPhase();
}

int32 AGCGGameMode_BattleRoyale::GetNextPlayerID(int32 CurrentPlayerID) const
{
	// Rule 12-2-1: Get next player in turn order (clockwise or counterclockwise)

	if (ActivePlayerIDs.Num() == 0)
	{
		return -1;
	}

	// Find current player index in active list
	int32 CurrentIndex = ActivePlayerIDs.IndexOfByKey(CurrentPlayerID);
	if (CurrentIndex == INDEX_NONE)
	{
		// Current player not found, return first active player
		return ActivePlayerIDs[0];
	}

	// Get next index based on direction
	int32 NextIndex;
	if (TurnDirection == EGCGTurnDirection::Clockwise)
	{
		NextIndex = (CurrentIndex + 1) % ActivePlayerIDs.Num();
	}
	else // Counterclockwise
	{
		NextIndex = (CurrentIndex - 1 + ActivePlayerIDs.Num()) % ActivePlayerIDs.Num();
	}

	return ActivePlayerIDs[NextIndex];
}

int32 AGCGGameMode_BattleRoyale::GetPreviousPlayerID(int32 CurrentPlayerID) const
{
	// Get previous player in turn order (opposite of GetNextPlayerID)

	if (ActivePlayerIDs.Num() == 0)
	{
		return -1;
	}

	// Find current player index
	int32 CurrentIndex = ActivePlayerIDs.IndexOfByKey(CurrentPlayerID);
	if (CurrentIndex == INDEX_NONE)
	{
		return ActivePlayerIDs[ActivePlayerIDs.Num() - 1];
	}

	// Get previous index (opposite direction)
	int32 PrevIndex;
	if (TurnDirection == EGCGTurnDirection::Clockwise)
	{
		PrevIndex = (CurrentIndex - 1 + ActivePlayerIDs.Num()) % ActivePlayerIDs.Num();
	}
	else // Counterclockwise
	{
		PrevIndex = (CurrentIndex + 1) % ActivePlayerIDs.Num();
	}

	return ActivePlayerIDs[PrevIndex];
}

// ===========================================================================================
// VICTORY CONDITIONS
// ===========================================================================================

void AGCGGameMode_BattleRoyale::CheckVictoryConditions()
{
	// Rule 12-2-5: Check for Battle Royale victory

	if (VictoryMode == EGCGBattleRoyaleVictoryMode::LastPlayerStanding)
	{
		ProcessLastPlayerStandingVictory();
	}
	// Winner-takes-all is handled in ProcessWinnerTakesAllVictory (called when player defeated)
}

void AGCGGameMode_BattleRoyale::ProcessWinnerTakesAllVictory(int32 AttackingPlayerID, int32 DefeatedPlayerID)
{
	// Rule 12-2-5-1: Winner-takes-all - only the attacking player wins

	AGCGGameState* GCGGameState = GetGameState<AGCGGameState>();
	if (!GCGGameState)
	{
		return;
	}

	// Remove defeated player
	RemovePlayerFromGame(DefeatedPlayerID);

	// Attacking player wins immediately
	GCGGameState->bGameEnded = true;
	GCGGameState->WinnerPlayerID = AttackingPlayerID;

	UE_LOG(LogTemp, Warning, TEXT("[GCGGameMode_BattleRoyale] WINNER-TAKES-ALL: Player %d wins by defeating Player %d!"),
		AttackingPlayerID, DefeatedPlayerID);
}

void AGCGGameMode_BattleRoyale::ProcessLastPlayerStandingVictory()
{
	// Rule 12-2-5-2: Last-player-standing - last player alive wins

	AGCGGameState* GCGGameState = GetGameState<AGCGGameState>();
	if (!GCGGameState)
	{
		return;
	}

	// Check if only one player remains
	if (ActivePlayerIDs.Num() == 1)
	{
		int32 WinnerID = ActivePlayerIDs[0];
		GCGGameState->bGameEnded = true;
		GCGGameState->WinnerPlayerID = WinnerID;

		UE_LOG(LogTemp, Warning, TEXT("[GCGGameMode_BattleRoyale] LAST PLAYER STANDING: Player %d wins!"),
			WinnerID);
	}
	else if (ActivePlayerIDs.Num() == 0)
	{
		// Draw (all players defeated simultaneously)
		GCGGameState->bGameEnded = true;
		GCGGameState->WinnerPlayerID = -1;

		UE_LOG(LogTemp, Warning, TEXT("[GCGGameMode_BattleRoyale] Game ended in a draw (all players defeated)"));
	}
}

void AGCGGameMode_BattleRoyale::RemovePlayerFromGame(int32 PlayerID)
{
	// Rule 12-2-5: Player and all their cards/effects are removed from game

	AGCGPlayerState* PlayerState = GetPlayerStateByID(PlayerID);
	if (!PlayerState)
	{
		return;
	}

	// Mark player as defeated
	PlayerState->bHasLost = true;

	// Remove from active player list
	ActivePlayerIDs.Remove(PlayerID);

	UE_LOG(LogTemp, Warning, TEXT("[GCGGameMode_BattleRoyale] Player %d removed from game (%d players remaining)"),
		PlayerID, ActivePlayerIDs.Num());

	// TODO: Remove all cards and effects belonging to this player
	// For now, leaving cards in zones (they'll be ignored due to bHasLost flag)
}

// ===========================================================================================
// SIMULTANEOUS ACTIONS
// ===========================================================================================

TArray<int32> AGCGGameMode_BattleRoyale::GetSimultaneousActionOrder() const
{
	// Rule 12-2-4: Active player first, then next in turn order

	AGCGGameState* GCGGameState = GetGameState<AGCGGameState>();
	if (!GCGGameState)
	{
		return TArray<int32>();
	}

	TArray<int32> Order;
	int32 CurrentPlayerID = GCGGameState->ActivePlayerID;

	// Start with active player
	if (IsPlayerActive(CurrentPlayerID))
	{
		Order.Add(CurrentPlayerID);
	}

	// Add next players in turn order
	int32 NextPlayerID = GetNextPlayerID(CurrentPlayerID);
	while (NextPlayerID != CurrentPlayerID && Order.Num() < ActivePlayerIDs.Num())
	{
		if (IsPlayerActive(NextPlayerID))
		{
			Order.Add(NextPlayerID);
		}
		NextPlayerID = GetNextPlayerID(NextPlayerID);
	}

	return Order;
}

// ===========================================================================================
// ACTION STEP
// ===========================================================================================

void AGCGGameMode_BattleRoyale::ExecuteActionStep()
{
	// Rule 12-2-8: Action steps in order starting with next player after active

	AGCGGameState* GCGGameState = GetGameState<AGCGGameState>();
	if (!GCGGameState)
	{
		return;
	}

	GCGGameState->bInActionStep = true;

	// Get player order for Action Step (starts with NEXT player after active)
	int32 FirstPlayerID = GetNextPlayerID(GCGGameState->ActivePlayerID);
	GCGGameState->PriorityPlayerID = FirstPlayerID;

	// Reset pass tracking
	PlayersWhoPassedActionStep.Empty();
	LastPlayerToActInActionStep = -1;

	UE_LOG(LogTemp, Log, TEXT("[GCGGameMode_BattleRoyale] Action Step started (first player: %d)"),
		FirstPlayerID);

	// TODO: Trigger UI for first player's action
}

bool AGCGGameMode_BattleRoyale::ProcessActionStepAction(int32 PlayerID, EGCGPlayerActionType ActionType, int32 CardInstanceID)
{
	// Rule 12-2-8: Process action in Battle Royale Action Step

	AGCGGameState* GCGGameState = GetGameState<AGCGGameState>();
	if (!GCGGameState)
	{
		return false;
	}

	if (!GCGGameState->bInActionStep)
	{
		UE_LOG(LogTemp, Warning, TEXT("[GCGGameMode_BattleRoyale] Not in Action Step"));
		return false;
	}

	if (PlayerID != GCGGameState->PriorityPlayerID)
	{
		UE_LOG(LogTemp, Warning, TEXT("[GCGGameMode_BattleRoyale] Player %d does not have priority (priority: %d)"),
			PlayerID, GCGGameState->PriorityPlayerID);
		return false;
	}

	if (ActionType == EGCGPlayerActionType::PassPriority)
	{
		// Mark this player as passed
		PlayersWhoPassedActionStep.AddUnique(PlayerID);

		UE_LOG(LogTemp, Log, TEXT("[GCGGameMode_BattleRoyale] Player %d passed (%d/%d players passed)"),
			PlayerID, PlayersWhoPassedActionStep.Num(), ActivePlayerIDs.Num());

		// Check if all players have passed
		if (HaveAllPlayersPassedActionStep())
		{
			EndActionStep();
			return true;
		}

		// Move priority to next player
		GCGGameState->PriorityPlayerID = GetNextPlayerID(PlayerID);

		UE_LOG(LogTemp, Log, TEXT("[GCGGameMode_BattleRoyale] Priority moved to Player %d"),
			GCGGameState->PriorityPlayerID);
	}
	else if (ActionType == EGCGPlayerActionType::ActivateAbility)
	{
		// Player took an action - reset pass counter
		PlayersWhoPassedActionStep.Empty();
		LastPlayerToActInActionStep = PlayerID;

		UE_LOG(LogTemp, Log, TEXT("[GCGGameMode_BattleRoyale] Player %d activated ability (pass counter reset)"),
			PlayerID);

		// TODO: Process ability activation
		// For now, just move priority to next player
		GCGGameState->PriorityPlayerID = GetNextPlayerID(PlayerID);
	}

	return true;
}

bool AGCGGameMode_BattleRoyale::HaveAllPlayersPassedActionStep() const
{
	// Rule 12-2-8: Action Step ends when all players have passed

	// Check if all active players have passed
	for (int32 PlayerID : ActivePlayerIDs)
	{
		if (!PlayersWhoPassedActionStep.Contains(PlayerID))
		{
			return false;
		}
	}

	return true;
}

// ===========================================================================================
// ATTACK TARGETS
// ===========================================================================================

TArray<int32> AGCGGameMode_BattleRoyale::GetValidAttackTargets(int32 AttackingPlayerID) const
{
	// Rule 12-2-7: Can attack any other player

	TArray<int32> ValidTargets;

	for (int32 PlayerID : ActivePlayerIDs)
	{
		// Cannot attack yourself
		if (PlayerID == AttackingPlayerID)
		{
			continue;
		}

		ValidTargets.Add(PlayerID);
	}

	return ValidTargets;
}

// ===========================================================================================
// ENEMY/OPPONENT REFERENCE
// ===========================================================================================

TArray<int32> AGCGGameMode_BattleRoyale::GetEnemyPlayers(int32 PlayerID) const
{
	// Rule 12-2-6: "Enemy" or "opponent" refers to all other players

	TArray<int32> Enemies;

	for (int32 OtherPlayerID : ActivePlayerIDs)
	{
		if (OtherPlayerID != PlayerID)
		{
			Enemies.Add(OtherPlayerID);
		}
	}

	return Enemies;
}

bool AGCGGameMode_BattleRoyale::IsEnemyPlayer(int32 PlayerID, int32 OtherPlayerID) const
{
	// Rule 12-2-6: All other players are enemies

	// Cannot be your own enemy
	if (PlayerID == OtherPlayerID)
	{
		return false;
	}

	// Both must be active
	if (!IsPlayerActive(PlayerID) || !IsPlayerActive(OtherPlayerID))
	{
		return false;
	}

	return true;
}

// ===========================================================================================
// UTILITY
// ===========================================================================================

int32 AGCGGameMode_BattleRoyale::GetActivePlayerCount() const
{
	return ActivePlayerIDs.Num();
}

bool AGCGGameMode_BattleRoyale::IsPlayerActive(int32 PlayerID) const
{
	return ActivePlayerIDs.Contains(PlayerID);
}
