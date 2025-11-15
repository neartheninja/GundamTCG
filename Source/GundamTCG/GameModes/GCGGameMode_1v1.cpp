// GCGGameMode_1v1.cpp - 1v1 Match Game Mode Implementation
// Unreal Engine 5.6 - Gundam TCG Implementation

#include "GCGGameMode_1v1.h"
#include "GundamTCG/GameState/GCGGameState.h"
#include "TimerManager.h"
#include "Engine/World.h"

AGCGGameMode_1v1::AGCGGameMode_1v1()
{
	// Set default phase advance delay (2 seconds)
	PhaseAdvanceDelay = 2.0f;
}

void AGCGGameMode_1v1::BeginPlay()
{
	Super::BeginPlay();

	UE_LOG(LogTemp, Log, TEXT("AGCGGameMode_1v1::BeginPlay - 1v1 Match Mode initialized"));
}

// ===== GAME INITIALIZATION =====

bool AGCGGameMode_1v1::CanStartGame() const
{
	// Need at least 2 players for 1v1
	TArray<AGCGPlayerState*> PlayerStates = GetAllPlayerStates();
	return PlayerStates.Num() >= 2;
}

void AGCGGameMode_1v1::InitializeGame()
{
	if (!CanStartGame())
	{
		UE_LOG(LogTemp, Warning, TEXT("AGCGGameMode_1v1::InitializeGame - Cannot start game, not enough players"));
		return;
	}

	UE_LOG(LogTemp, Log, TEXT("AGCGGameMode_1v1::InitializeGame - Initializing 1v1 match"));

	AGCGGameState* GCGGameState = GetGCGGameState();
	if (!GCGGameState)
	{
		UE_LOG(LogTemp, Error, TEXT("AGCGGameMode_1v1::InitializeGame - Game state is null"));
		return;
	}

	// Setup game state
	GCGGameState->bGameInProgress = false;
	GCGGameState->bGameOver = false;
	GCGGameState->WinnerPlayerID = -1;
	GCGGameState->TurnNumber = 0;
	GCGGameState->CurrentPhase = EGCGTurnPhase::NotStarted;
	GCGGameState->bIsTeamBattle = false;
	GCGGameState->ActivePlayerID = 0; // Player 1 goes first by default

	// TODO: Setup decks for both players (requires deck lists)
	// For now, this would be called externally after deck selection
	// SetupPlayerDecks(0, MainDeck1, ResourceDeck1);
	// SetupPlayerDecks(1, MainDeck2, ResourceDeck2);

	// TODO: Setup shields and EX Base for both players
	// SetupPlayerShields(0);
	// SetupPlayerShields(1);
	// SetupEXBase(0);
	// SetupEXBase(1);

	// TODO: Setup EX Resource for Player 2
	// SetupEXResource(1);

	UE_LOG(LogTemp, Log, TEXT("AGCGGameMode_1v1::InitializeGame - Game initialized, ready to start first turn"));

	// Start the first turn
	StartNewTurn();
}

// ===== TURN MANAGEMENT =====

void AGCGGameMode_1v1::StartNewTurn()
{
	AGCGGameState* GCGGameState = GetGCGGameState();
	if (!GCGGameState)
	{
		UE_LOG(LogTemp, Error, TEXT("AGCGGameMode_1v1::StartNewTurn - Game state is null"));
		return;
	}

	// Increment turn number
	GCGGameState->TurnNumber++;

	// If this is turn 1, active player is already set (Player 1)
	// Otherwise, switch to next player
	if (GCGGameState->TurnNumber > 1)
	{
		GCGGameState->ActivePlayerID = GetNextPlayerID(GCGGameState->ActivePlayerID);
	}

	UE_LOG(LogTemp, Log, TEXT("AGCGGameMode_1v1::StartNewTurn - Turn %d started (Active Player: %d)"),
		GCGGameState->TurnNumber, GCGGameState->ActivePlayerID);

	// Mark game as in progress
	GCGGameState->bGameInProgress = true;

	// Enter Start Phase
	GCGGameState->CurrentPhase = EGCGTurnPhase::StartPhase;
	ExecuteStartPhase();

	// Call Blueprint event
	OnTurnStarted(GCGGameState->TurnNumber, GCGGameState->ActivePlayerID);
}

void AGCGGameMode_1v1::AdvancePhase()
{
	AGCGGameState* GCGGameState = GetGCGGameState();
	if (!GCGGameState)
	{
		UE_LOG(LogTemp, Error, TEXT("AGCGGameMode_1v1::AdvancePhase - Game state is null"));
		return;
	}

	// Determine next phase
	EGCGTurnPhase NextPhase = EGCGTurnPhase::NotStarted;

	switch (GCGGameState->CurrentPhase)
	{
	case EGCGTurnPhase::NotStarted:
		NextPhase = EGCGTurnPhase::StartPhase;
		break;

	case EGCGTurnPhase::StartPhase:
		NextPhase = EGCGTurnPhase::DrawPhase;
		break;

	case EGCGTurnPhase::DrawPhase:
		NextPhase = EGCGTurnPhase::ResourcePhase;
		break;

	case EGCGTurnPhase::ResourcePhase:
		NextPhase = EGCGTurnPhase::MainPhase;
		break;

	case EGCGTurnPhase::MainPhase:
		NextPhase = EGCGTurnPhase::EndPhase;
		break;

	case EGCGTurnPhase::EndPhase:
		// End of turn, start next turn
		EndTurn();
		return;

	default:
		UE_LOG(LogTemp, Warning, TEXT("AGCGGameMode_1v1::AdvancePhase - Unknown current phase"));
		return;
	}

	// Update phase
	GCGGameState->CurrentPhase = NextPhase;

	UE_LOG(LogTemp, Log, TEXT("AGCGGameMode_1v1::AdvancePhase - Advancing to %s"), *GCGGameState->GetPhaseName());

	// Execute the new phase
	switch (NextPhase)
	{
	case EGCGTurnPhase::StartPhase:
		ExecuteStartPhase();
		break;

	case EGCGTurnPhase::DrawPhase:
		ExecuteDrawPhase();
		break;

	case EGCGTurnPhase::ResourcePhase:
		ExecuteResourcePhase();
		break;

	case EGCGTurnPhase::MainPhase:
		ExecuteMainPhase();
		break;

	case EGCGTurnPhase::EndPhase:
		ExecuteEndPhase();
		break;

	default:
		break;
	}
}

void AGCGGameMode_1v1::EndTurn()
{
	AGCGGameState* GCGGameState = GetGCGGameState();
	if (!GCGGameState)
	{
		UE_LOG(LogTemp, Error, TEXT("AGCGGameMode_1v1::EndTurn - Game state is null"));
		return;
	}

	UE_LOG(LogTemp, Log, TEXT("AGCGGameMode_1v1::EndTurn - Turn %d ended"), GCGGameState->TurnNumber);

	// Call Blueprint event
	OnTurnEnded(GCGGameState->TurnNumber);

	// Start next turn
	StartNewTurn();
}

// ===== PHASE HANDLERS =====

void AGCGGameMode_1v1::ExecuteStartPhase()
{
	AGCGGameState* GCGGameState = GetGCGGameState();
	if (!GCGGameState)
	{
		return;
	}

	UE_LOG(LogTemp, Log, TEXT("AGCGGameMode_1v1::ExecuteStartPhase - Executing Start Phase"));

	// Active Step: Set all rested cards to active
	GCGGameState->CurrentStartPhaseStep = EGCGStartPhaseStep::ActiveStep;
	ActivateAllCardsForPlayer(GCGGameState->ActivePlayerID);

	// Start Step: Trigger "at start of turn" effects
	GCGGameState->CurrentStartPhaseStep = EGCGStartPhaseStep::StartStep;
	// TODO: Trigger "at start of turn" effects (Phase 8: Effect System)

	// Reset step
	GCGGameState->CurrentStartPhaseStep = EGCGStartPhaseStep::None;

	// Call Blueprint event
	OnPhaseExecuted(EGCGTurnPhase::StartPhase);

	// Auto-advance to Draw Phase
	if (ShouldPhaseAutoAdvance(EGCGTurnPhase::StartPhase))
	{
		GetWorldTimerManager().SetTimer(PhaseAdvanceTimerHandle, this, &AGCGGameMode_1v1::AdvancePhase, PhaseAdvanceDelay, false);
	}
}

void AGCGGameMode_1v1::ExecuteDrawPhase()
{
	AGCGGameState* GCGGameState = GetGCGGameState();
	if (!GCGGameState)
	{
		return;
	}

	UE_LOG(LogTemp, Log, TEXT("AGCGGameMode_1v1::ExecuteDrawPhase - Executing Draw Phase"));

	// Player draws 1 card (mandatory)
	// TODO: Implement actual card drawing (Phase 3: Zone Management)
	// For now, just log
	UE_LOG(LogTemp, Log, TEXT("AGCGGameMode_1v1::ExecuteDrawPhase - Player %d draws 1 card"), GCGGameState->ActivePlayerID);

	// TODO: Check if deck is empty after draw → player loses
	// CheckVictoryConditions();

	// Call Blueprint event
	OnPhaseExecuted(EGCGTurnPhase::DrawPhase);

	// Auto-advance to Resource Phase
	if (ShouldPhaseAutoAdvance(EGCGTurnPhase::DrawPhase))
	{
		GetWorldTimerManager().SetTimer(PhaseAdvanceTimerHandle, this, &AGCGGameMode_1v1::AdvancePhase, PhaseAdvanceDelay, false);
	}
}

void AGCGGameMode_1v1::ExecuteResourcePhase()
{
	AGCGGameState* GCGGameState = GetGCGGameState();
	if (!GCGGameState)
	{
		return;
	}

	UE_LOG(LogTemp, Log, TEXT("AGCGGameMode_1v1::ExecuteResourcePhase - Executing Resource Phase"));

	// Player places 1 card from Resource Deck to Resource Area (mandatory)
	// TODO: Implement actual resource placement (Phase 3: Zone Management)
	// For now, just log
	UE_LOG(LogTemp, Log, TEXT("AGCGGameMode_1v1::ExecuteResourcePhase - Player %d places 1 resource"), GCGGameState->ActivePlayerID);

	// Call Blueprint event
	OnPhaseExecuted(EGCGTurnPhase::ResourcePhase);

	// Auto-advance to Main Phase
	if (ShouldPhaseAutoAdvance(EGCGTurnPhase::ResourcePhase))
	{
		GetWorldTimerManager().SetTimer(PhaseAdvanceTimerHandle, this, &AGCGGameMode_1v1::AdvancePhase, PhaseAdvanceDelay, false);
	}
}

void AGCGGameMode_1v1::ExecuteMainPhase()
{
	AGCGGameState* GCGGameState = GetGCGGameState();
	if (!GCGGameState)
	{
		return;
	}

	UE_LOG(LogTemp, Log, TEXT("AGCGGameMode_1v1::ExecuteMainPhase - Executing Main Phase"));

	// Main Phase: Player can play cards, activate abilities, declare attacks
	// This phase waits for player input and does NOT auto-advance
	// Player must explicitly pass priority to advance to End Phase

	// Call Blueprint event
	OnPhaseExecuted(EGCGTurnPhase::MainPhase);

	// Main Phase does NOT auto-advance - player must request to pass
	UE_LOG(LogTemp, Log, TEXT("AGCGGameMode_1v1::ExecuteMainPhase - Waiting for player input..."));
}

void AGCGGameMode_1v1::ExecuteEndPhase()
{
	AGCGGameState* GCGGameState = GetGCGGameState();
	if (!GCGGameState)
	{
		return;
	}

	UE_LOG(LogTemp, Log, TEXT("AGCGGameMode_1v1::ExecuteEndPhase - Executing End Phase"));

	// Action Step: Action timing (Commands, Activate・Action abilities)
	GCGGameState->CurrentEndPhaseStep = EGCGEndPhaseStep::ActionStep;
	// TODO: Allow Action timing cards/abilities (Phase 8: Effect System)

	// End Step: "At end of turn" effects trigger
	GCGGameState->CurrentEndPhaseStep = EGCGEndPhaseStep::EndStep;
	// TODO: Trigger "at end of turn" effects (Phase 8: Effect System)
	// TODO: Process Repair keyword here

	// Hand Step: If hand ≥ 11, discard to 10
	GCGGameState->CurrentEndPhaseStep = EGCGEndPhaseStep::HandStep;
	ProcessHandLimit(GCGGameState->ActivePlayerID);

	// Cleanup Step: "During this turn" effects expire
	GCGGameState->CurrentEndPhaseStep = EGCGEndPhaseStep::CleanupStep;
	CleanupTurnEffects();

	// Reset step
	GCGGameState->CurrentEndPhaseStep = EGCGEndPhaseStep::None;

	// Call Blueprint event
	OnPhaseExecuted(EGCGTurnPhase::EndPhase);

	// Auto-advance to next turn
	if (ShouldPhaseAutoAdvance(EGCGTurnPhase::EndPhase))
	{
		GetWorldTimerManager().SetTimer(PhaseAdvanceTimerHandle, this, &AGCGGameMode_1v1::AdvancePhase, PhaseAdvanceDelay, false);
	}
}

// ===== GAME FLOW CONTROL =====

void AGCGGameMode_1v1::CheckVictoryConditions()
{
	// TODO: Implement victory condition checks (Phase 3: Zone Management)
	// Victory conditions:
	// 1. Player takes damage when they have no shields → they lose
	// 2. Player must draw but deck is empty → they lose

	UE_LOG(LogTemp, Verbose, TEXT("AGCGGameMode_1v1::CheckVictoryConditions - Checking victory conditions"));
}

void AGCGGameMode_1v1::EndGame(int32 WinnerPlayerID)
{
	AGCGGameState* GCGGameState = GetGCGGameState();
	if (!GCGGameState)
	{
		return;
	}

	UE_LOG(LogTemp, Log, TEXT("AGCGGameMode_1v1::EndGame - Game ended, winner: %d"), WinnerPlayerID);

	// Update game state
	GCGGameState->bGameInProgress = false;
	GCGGameState->bGameOver = true;
	GCGGameState->WinnerPlayerID = WinnerPlayerID;
	GCGGameState->CurrentPhase = EGCGTurnPhase::GameOver;

	// Call Blueprint event
	GCGGameState->OnGameEnded(WinnerPlayerID);
}

// ===== AUTOMATIC PHASE PROGRESSION =====

bool AGCGGameMode_1v1::ShouldPhaseAutoAdvance(EGCGTurnPhase Phase) const
{
	switch (Phase)
	{
	case EGCGTurnPhase::StartPhase:
	case EGCGTurnPhase::DrawPhase:
	case EGCGTurnPhase::ResourcePhase:
	case EGCGTurnPhase::EndPhase:
		// These phases auto-advance
		return true;

	case EGCGTurnPhase::MainPhase:
		// Main Phase waits for player input
		return false;

	default:
		return false;
	}
}

// ===== PLAYER ACTIONS =====

void AGCGGameMode_1v1::RequestPassPriority(int32 PlayerID)
{
	AGCGGameState* GCGGameState = GetGCGGameState();
	if (!GCGGameState)
	{
		return;
	}

	// Only the active player can pass priority
	if (PlayerID != GCGGameState->ActivePlayerID)
	{
		UE_LOG(LogTemp, Warning, TEXT("AGCGGameMode_1v1::RequestPassPriority - Player %d cannot pass priority (not active)"), PlayerID);
		return;
	}

	// Only certain phases allow passing
	if (GCGGameState->CurrentPhase == EGCGTurnPhase::MainPhase)
	{
		UE_LOG(LogTemp, Log, TEXT("AGCGGameMode_1v1::RequestPassPriority - Player %d passes priority in Main Phase"), PlayerID);
		AdvancePhase();
	}
	else
	{
		UE_LOG(LogTemp, Warning, TEXT("AGCGGameMode_1v1::RequestPassPriority - Cannot pass priority in current phase"));
	}
}

// ===== SETUP HELPERS =====

void AGCGGameMode_1v1::SetupPlayerDecks(int32 PlayerID, const TArray<FName>& MainDeckList, const TArray<FName>& ResourceDeckList)
{
	// TODO: Implement deck setup (Phase 3: Zone Management)
	// This will create card instances and place them in the Deck and ResourceDeck zones

	UE_LOG(LogTemp, Log, TEXT("AGCGGameMode_1v1::SetupPlayerDecks - Setting up decks for Player %d (Main: %d cards, Resource: %d cards)"),
		PlayerID, MainDeckList.Num(), ResourceDeckList.Num());
}

void AGCGGameMode_1v1::SetupPlayerShields(int32 PlayerID)
{
	// TODO: Implement shield setup (Phase 3: Zone Management)
	// This will take 6 cards from top of deck and place them in ShieldStack zone

	UE_LOG(LogTemp, Log, TEXT("AGCGGameMode_1v1::SetupPlayerShields - Setting up 6 shields for Player %d"), PlayerID);
}

void AGCGGameMode_1v1::SetupEXBase(int32 PlayerID)
{
	// Create EX Base token
	FGCGCardInstance EXBaseToken = CreateTokenInstance(FName("EXBase"), PlayerID);

	// TODO: Place EX Base in Base section (Phase 3: Zone Management)

	UE_LOG(LogTemp, Log, TEXT("AGCGGameMode_1v1::SetupEXBase - Created EX Base token for Player %d (ID: %d)"),
		PlayerID, EXBaseToken.InstanceID);
}

void AGCGGameMode_1v1::SetupEXResource(int32 PlayerID)
{
	// Create EX Resource token
	FGCGCardInstance EXResourceToken = CreateTokenInstance(FName("EXResource"), PlayerID);

	// TODO: Place EX Resource in Resource Area (Phase 3: Zone Management)

	UE_LOG(LogTemp, Log, TEXT("AGCGGameMode_1v1::SetupEXResource - Created EX Resource token for Player %d (ID: %d)"),
		PlayerID, EXResourceToken.InstanceID);
}

// ===== INTERNAL HELPERS =====

int32 AGCGGameMode_1v1::GetNextPlayerID(int32 CurrentPlayerID) const
{
	// In 1v1, alternate between 0 and 1
	return (CurrentPlayerID == 0) ? 1 : 0;
}

void AGCGGameMode_1v1::ActivateAllCardsForPlayer(int32 PlayerID)
{
	// TODO: Implement card activation (Phase 3: Zone Management)
	// This will set all rested cards in Battle Area, Resource Area, and Base to active

	UE_LOG(LogTemp, Log, TEXT("AGCGGameMode_1v1::ActivateAllCardsForPlayer - Activating all cards for Player %d"), PlayerID);
}

void AGCGGameMode_1v1::ProcessHandLimit(int32 PlayerID)
{
	// TODO: Implement hand limit check (Phase 3: Zone Management)
	// If hand size ≥ 11, player must discard down to 10

	UE_LOG(LogTemp, Log, TEXT("AGCGGameMode_1v1::ProcessHandLimit - Checking hand limit for Player %d"), PlayerID);
}

void AGCGGameMode_1v1::CleanupTurnEffects()
{
	// TODO: Implement effect cleanup (Phase 8: Effect System)
	// This will remove all "UntilEndOfTurn" modifiers and temporary effects

	UE_LOG(LogTemp, Log, TEXT("AGCGGameMode_1v1::CleanupTurnEffects - Cleaning up turn effects"));
}
