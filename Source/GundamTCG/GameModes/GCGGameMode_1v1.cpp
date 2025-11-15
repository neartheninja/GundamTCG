// GCGGameMode_1v1.cpp - 1v1 Match Game Mode Implementation
// Unreal Engine 5.6 - Gundam TCG Implementation

#include "GCGGameMode_1v1.h"
#include "GundamTCG/GameState/GCGGameState.h"
#include "GundamTCG/PlayerState/GCGPlayerState.h"
#include "GundamTCG/Subsystems/GCGZoneSubsystem.h"
#include "GundamTCG/Subsystems/GCGPlayerActionSubsystem.h"
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

	// NOTE: Deck setup must be called externally after deck selection
	// Once decks are set up, the following initialization sequence applies:

	// Get zone subsystem for drawing initial hands
	UGCGZoneSubsystem* ZoneSubsystem = GetGameInstance()->GetSubsystem<UGCGZoneSubsystem>();
	if (ZoneSubsystem)
	{
		// Draw initial 5-card hands for both players
		TArray<AGCGPlayerState*> AllPlayerStates = GetAllPlayerStates();
		for (AGCGPlayerState* PlayerState : AllPlayerStates)
		{
			if (PlayerState && PlayerState->GetDeckSize() >= 5)
			{
				TArray<FGCGCardInstance> InitialHand;
				int32 CardsDrawn = ZoneSubsystem->DrawTopCards(EGCGCardZone::Deck, PlayerState, 5, InitialHand);

				// Move cards to hand
				for (FGCGCardInstance& Card : InitialHand)
				{
					Card.CurrentZone = EGCGCardZone::Hand;
					PlayerState->Hand.Add(Card);
				}

				UE_LOG(LogTemp, Log, TEXT("AGCGGameMode_1v1::InitializeGame - Player %d drew initial hand (%d cards)"),
					PlayerState->GetPlayerID(), CardsDrawn);
			}
		}
	}

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

	// Get zone subsystem
	UGCGZoneSubsystem* ZoneSubsystem = GetGameInstance()->GetSubsystem<UGCGZoneSubsystem>();
	if (!ZoneSubsystem)
	{
		UE_LOG(LogTemp, Error, TEXT("AGCGGameMode_1v1::ExecuteDrawPhase - Zone subsystem not found"));
		return;
	}

	// Get active player state
	AGCGPlayerState* ActivePlayerState = GetPlayerStateByID(GCGGameState->ActivePlayerID);
	if (!ActivePlayerState)
	{
		UE_LOG(LogTemp, Error, TEXT("AGCGGameMode_1v1::ExecuteDrawPhase - Active player state not found"));
		return;
	}

	// Check if deck is empty BEFORE drawing
	if (ActivePlayerState->GetDeckSize() == 0)
	{
		UE_LOG(LogTemp, Warning, TEXT("AGCGGameMode_1v1::ExecuteDrawPhase - Player %d cannot draw (deck empty) - LOSES THE GAME"),
			GCGGameState->ActivePlayerID);
		// Player loses if they must draw but deck is empty
		int32 OpponentID = GetNextPlayerID(GCGGameState->ActivePlayerID);
		EndGame(OpponentID);
		return;
	}

	// Player draws 1 card (mandatory)
	FGCGCardInstance DrawnCard;
	if (ZoneSubsystem->DrawTopCard(EGCGCardZone::Deck, ActivePlayerState, DrawnCard))
	{
		// Move card to hand
		if (ZoneSubsystem->MoveCard(DrawnCard, EGCGCardZone::Deck, EGCGCardZone::Hand,
			ActivePlayerState, GCGGameState, false))
		{
			UE_LOG(LogTemp, Log, TEXT("AGCGGameMode_1v1::ExecuteDrawPhase - Player %d drew card: %s (ID: %d)"),
				GCGGameState->ActivePlayerID, *DrawnCard.CardName.ToString(), DrawnCard.InstanceID);

			// Mark that player has drawn this turn
			ActivePlayerState->bHasDrawnThisTurn = true;
		}
	}

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

	// Get zone subsystem
	UGCGZoneSubsystem* ZoneSubsystem = GetGameInstance()->GetSubsystem<UGCGZoneSubsystem>();
	if (!ZoneSubsystem)
	{
		UE_LOG(LogTemp, Error, TEXT("AGCGGameMode_1v1::ExecuteResourcePhase - Zone subsystem not found"));
		return;
	}

	// Get active player state
	AGCGPlayerState* ActivePlayerState = GetPlayerStateByID(GCGGameState->ActivePlayerID);
	if (!ActivePlayerState)
	{
		UE_LOG(LogTemp, Error, TEXT("AGCGGameMode_1v1::ExecuteResourcePhase - Active player state not found"));
		return;
	}

	// Player places 1 card from Resource Deck to Resource Area (mandatory)
	// If Resource Deck is empty, phase still passes but no placement
	if (ActivePlayerState->GetResourceDeckSize() == 0)
	{
		UE_LOG(LogTemp, Warning, TEXT("AGCGGameMode_1v1::ExecuteResourcePhase - Player %d has no cards in Resource Deck"),
			GCGGameState->ActivePlayerID);
	}
	else
	{
		FGCGCardInstance ResourceCard;
		if (ZoneSubsystem->DrawTopCard(EGCGCardZone::ResourceDeck, ActivePlayerState, ResourceCard))
		{
			// Move card to Resource Area
			if (ZoneSubsystem->MoveCard(ResourceCard, EGCGCardZone::ResourceDeck, EGCGCardZone::ResourceArea,
				ActivePlayerState, GCGGameState, true))
			{
				UE_LOG(LogTemp, Log, TEXT("AGCGGameMode_1v1::ExecuteResourcePhase - Player %d placed resource: %s (ID: %d)"),
					GCGGameState->ActivePlayerID, *ResourceCard.CardName.ToString(), ResourceCard.InstanceID);

				// Mark that player has placed resource this turn
				ActivePlayerState->bHasPlacedResourceThisTurn = true;
			}
		}
	}

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

bool AGCGGameMode_1v1::RequestPlayCard(int32 PlayerID, int32 CardInstanceID)
{
	AGCGGameState* GCGGameState = GetGCGGameState();
	if (!GCGGameState)
	{
		UE_LOG(LogTemp, Error, TEXT("AGCGGameMode_1v1::RequestPlayCard - Game state not found"));
		return false;
	}

	// Get player state
	AGCGPlayerState* PlayerState = GetPlayerStateByID(PlayerID);
	if (!PlayerState)
	{
		UE_LOG(LogTemp, Error, TEXT("AGCGGameMode_1v1::RequestPlayCard - Player state not found for ID %d"), PlayerID);
		return false;
	}

	// Get action subsystem
	UGCGPlayerActionSubsystem* ActionSubsystem = GetGameInstance()->GetSubsystem<UGCGPlayerActionSubsystem>();
	if (!ActionSubsystem)
	{
		UE_LOG(LogTemp, Error, TEXT("AGCGGameMode_1v1::RequestPlayCard - Action subsystem not found"));
		return false;
	}

	// Execute play card action
	FGCGPlayerActionResult Result = ActionSubsystem->PlayCardFromHand(CardInstanceID, PlayerState, GCGGameState);

	if (!Result.bSuccess)
	{
		UE_LOG(LogTemp, Warning, TEXT("AGCGGameMode_1v1::RequestPlayCard - Player %d failed to play card: %s"),
			PlayerID, *Result.ErrorMessage);
	}

	return Result.bSuccess;
}

bool AGCGGameMode_1v1::RequestPlaceResource(int32 PlayerID, int32 CardInstanceID, bool bFaceUp)
{
	AGCGGameState* GCGGameState = GetGCGGameState();
	if (!GCGGameState)
	{
		UE_LOG(LogTemp, Error, TEXT("AGCGGameMode_1v1::RequestPlaceResource - Game state not found"));
		return false;
	}

	// Get player state
	AGCGPlayerState* PlayerState = GetPlayerStateByID(PlayerID);
	if (!PlayerState)
	{
		UE_LOG(LogTemp, Error, TEXT("AGCGGameMode_1v1::RequestPlaceResource - Player state not found for ID %d"), PlayerID);
		return false;
	}

	// Get action subsystem
	UGCGPlayerActionSubsystem* ActionSubsystem = GetGameInstance()->GetSubsystem<UGCGPlayerActionSubsystem>();
	if (!ActionSubsystem)
	{
		UE_LOG(LogTemp, Error, TEXT("AGCGGameMode_1v1::RequestPlaceResource - Action subsystem not found"));
		return false;
	}

	// Execute place resource action
	FGCGPlayerActionResult Result = ActionSubsystem->PlaceCardAsResource(CardInstanceID, PlayerState, GCGGameState, bFaceUp);

	if (!Result.bSuccess)
	{
		UE_LOG(LogTemp, Warning, TEXT("AGCGGameMode_1v1::RequestPlaceResource - Player %d failed to place resource: %s"),
			PlayerID, *Result.ErrorMessage);
	}

	return Result.bSuccess;
}

int32 AGCGGameMode_1v1::RequestDiscardCards(int32 PlayerID, const TArray<int32>& CardInstanceIDs)
{
	// Get player state
	AGCGPlayerState* PlayerState = GetPlayerStateByID(PlayerID);
	if (!PlayerState)
	{
		UE_LOG(LogTemp, Error, TEXT("AGCGGameMode_1v1::RequestDiscardCards - Player state not found for ID %d"), PlayerID);
		return 0;
	}

	// Get action subsystem
	UGCGPlayerActionSubsystem* ActionSubsystem = GetGameInstance()->GetSubsystem<UGCGPlayerActionSubsystem>();
	if (!ActionSubsystem)
	{
		UE_LOG(LogTemp, Error, TEXT("AGCGGameMode_1v1::RequestDiscardCards - Action subsystem not found"));
		return 0;
	}

	// Execute discard action
	int32 DiscardedCount = ActionSubsystem->DiscardToHandLimit(CardInstanceIDs, PlayerState, 10);

	UE_LOG(LogTemp, Log, TEXT("AGCGGameMode_1v1::RequestDiscardCards - Player %d discarded %d cards"),
		PlayerID, DiscardedCount);

	return DiscardedCount;
}

// ===== SETUP HELPERS =====

void AGCGGameMode_1v1::SetupPlayerDecks(int32 PlayerID, const TArray<FName>& MainDeckList, const TArray<FName>& ResourceDeckList)
{
	UE_LOG(LogTemp, Log, TEXT("AGCGGameMode_1v1::SetupPlayerDecks - Setting up decks for Player %d (Main: %d cards, Resource: %d cards)"),
		PlayerID, MainDeckList.Num(), ResourceDeckList.Num());

	// Get zone subsystem
	UGCGZoneSubsystem* ZoneSubsystem = GetGameInstance()->GetSubsystem<UGCGZoneSubsystem>();
	if (!ZoneSubsystem)
	{
		UE_LOG(LogTemp, Error, TEXT("AGCGGameMode_1v1::SetupPlayerDecks - Zone subsystem not found"));
		return;
	}

	// Get player state
	AGCGPlayerState* PlayerState = GetPlayerStateByID(PlayerID);
	if (!PlayerState)
	{
		UE_LOG(LogTemp, Error, TEXT("AGCGGameMode_1v1::SetupPlayerDecks - Player state not found for ID %d"), PlayerID);
		return;
	}

	// Store deck lists in player state
	PlayerState->MainDeckList = MainDeckList;
	PlayerState->ResourceDeckList = ResourceDeckList;

	// Clear existing deck zones
	PlayerState->Deck.Empty();
	PlayerState->ResourceDeck.Empty();

	// Create and add Main Deck cards
	for (const FName& CardNumber : MainDeckList)
	{
		FGCGCardInstance CardInstance = CreateCardInstance(CardNumber, PlayerID);
		CardInstance.CurrentZone = EGCGCardZone::Deck;
		PlayerState->Deck.Add(CardInstance);
	}

	// Create and add Resource Deck cards
	for (const FName& CardNumber : ResourceDeckList)
	{
		FGCGCardInstance CardInstance = CreateCardInstance(CardNumber, PlayerID);
		CardInstance.CurrentZone = EGCGCardZone::ResourceDeck;
		PlayerState->ResourceDeck.Add(CardInstance);
	}

	// Shuffle both decks
	ZoneSubsystem->ShuffleZone(EGCGCardZone::Deck, PlayerState);
	ZoneSubsystem->ShuffleZone(EGCGCardZone::ResourceDeck, PlayerState);

	UE_LOG(LogTemp, Log, TEXT("AGCGGameMode_1v1::SetupPlayerDecks - Player %d decks created and shuffled (Deck: %d, Resource: %d)"),
		PlayerID, PlayerState->Deck.Num(), PlayerState->ResourceDeck.Num());
}

void AGCGGameMode_1v1::SetupPlayerShields(int32 PlayerID)
{
	UE_LOG(LogTemp, Log, TEXT("AGCGGameMode_1v1::SetupPlayerShields - Setting up 6 shields for Player %d"), PlayerID);

	// Get zone subsystem
	UGCGZoneSubsystem* ZoneSubsystem = GetGameInstance()->GetSubsystem<UGCGZoneSubsystem>();
	if (!ZoneSubsystem)
	{
		UE_LOG(LogTemp, Error, TEXT("AGCGGameMode_1v1::SetupPlayerShields - Zone subsystem not found"));
		return;
	}

	// Get player state
	AGCGPlayerState* PlayerState = GetPlayerStateByID(PlayerID);
	if (!PlayerState)
	{
		UE_LOG(LogTemp, Error, TEXT("AGCGGameMode_1v1::SetupPlayerShields - Player state not found for ID %d"), PlayerID);
		return;
	}

	AGCGGameState* GCGGameState = GetGCGGameState();
	if (!GCGGameState)
	{
		UE_LOG(LogTemp, Error, TEXT("AGCGGameMode_1v1::SetupPlayerShields - Game state not found"));
		return;
	}

	// Take 6 cards from top of deck and place them in Shield Stack
	TArray<FGCGCardInstance> ShieldCards;
	int32 CardsDrawn = ZoneSubsystem->DrawTopCards(EGCGCardZone::Deck, PlayerState, 6, ShieldCards);

	if (CardsDrawn != 6)
	{
		UE_LOG(LogTemp, Warning, TEXT("AGCGGameMode_1v1::SetupPlayerShields - Could only draw %d shields (expected 6)"), CardsDrawn);
	}

	// Move cards to Shield Stack
	for (FGCGCardInstance& ShieldCard : ShieldCards)
	{
		ShieldCard.CurrentZone = EGCGCardZone::ShieldStack;
		PlayerState->ShieldStack.Add(ShieldCard);
	}

	UE_LOG(LogTemp, Log, TEXT("AGCGGameMode_1v1::SetupPlayerShields - Player %d now has %d shields"),
		PlayerID, PlayerState->ShieldStack.Num());
}

void AGCGGameMode_1v1::SetupEXBase(int32 PlayerID)
{
	// Get player state
	AGCGPlayerState* PlayerState = GetPlayerStateByID(PlayerID);
	if (!PlayerState)
	{
		UE_LOG(LogTemp, Error, TEXT("AGCGGameMode_1v1::SetupEXBase - Player state not found for ID %d"), PlayerID);
		return;
	}

	// Create EX Base token
	FGCGCardInstance EXBaseToken = CreateTokenInstance(FName("EXBase"), PlayerID);
	EXBaseToken.CurrentZone = EGCGCardZone::BaseSection;
	EXBaseToken.bIsActive = true;

	// Place EX Base in Base section
	PlayerState->BaseSection.Add(EXBaseToken);

	UE_LOG(LogTemp, Log, TEXT("AGCGGameMode_1v1::SetupEXBase - Created EX Base token for Player %d (ID: %d)"),
		PlayerID, EXBaseToken.InstanceID);
}

void AGCGGameMode_1v1::SetupEXResource(int32 PlayerID)
{
	// Get player state
	AGCGPlayerState* PlayerState = GetPlayerStateByID(PlayerID);
	if (!PlayerState)
	{
		UE_LOG(LogTemp, Error, TEXT("AGCGGameMode_1v1::SetupEXResource - Player state not found for ID %d"), PlayerID);
		return;
	}

	// Create EX Resource token
	FGCGCardInstance EXResourceToken = CreateTokenInstance(FName("EXResource"), PlayerID);
	EXResourceToken.CurrentZone = EGCGCardZone::ResourceArea;
	EXResourceToken.bIsActive = true;

	// Place EX Resource in Resource Area
	PlayerState->ResourceArea.Add(EXResourceToken);

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
	// Get zone subsystem
	UGCGZoneSubsystem* ZoneSubsystem = GetGameInstance()->GetSubsystem<UGCGZoneSubsystem>();
	if (!ZoneSubsystem)
	{
		UE_LOG(LogTemp, Error, TEXT("AGCGGameMode_1v1::ActivateAllCardsForPlayer - Zone subsystem not found"));
		return;
	}

	// Get player state
	AGCGPlayerState* PlayerState = GetPlayerStateByID(PlayerID);
	if (!PlayerState)
	{
		UE_LOG(LogTemp, Error, TEXT("AGCGGameMode_1v1::ActivateAllCardsForPlayer - Player state not found for ID %d"), PlayerID);
		return;
	}

	// Activate all cards (Zone::None means all relevant zones)
	int32 ActivatedCount = ZoneSubsystem->ActivateAllCards(PlayerState, EGCGCardZone::None);

	UE_LOG(LogTemp, Log, TEXT("AGCGGameMode_1v1::ActivateAllCardsForPlayer - Activated %d cards for Player %d"),
		ActivatedCount, PlayerID);

	// Reset turn flags for new turn
	PlayerState->ResetTurnFlags();
}

void AGCGGameMode_1v1::ProcessHandLimit(int32 PlayerID)
{
	// Get player state
	AGCGPlayerState* PlayerState = GetPlayerStateByID(PlayerID);
	if (!PlayerState)
	{
		UE_LOG(LogTemp, Error, TEXT("AGCGGameMode_1v1::ProcessHandLimit - Player state not found for ID %d"), PlayerID);
		return;
	}

	int32 HandSize = PlayerState->GetHandSize();

	// If hand size ≥ 11, player must discard down to 10
	if (HandSize >= 11)
	{
		int32 CardsToDiscard = HandSize - 10;
		UE_LOG(LogTemp, Warning, TEXT("AGCGGameMode_1v1::ProcessHandLimit - Player %d has %d cards in hand, must discard %d"),
			PlayerID, HandSize, CardsToDiscard);

		// TODO: Implement player choice for which cards to discard (Phase 5: Player Actions)
		// For now, just log the requirement
		// In a real implementation, this would:
		// 1. Pause the game
		// 2. Request player to select cards to discard
		// 3. Move selected cards from Hand to Trash
		// 4. Resume the game
	}
	else
	{
		UE_LOG(LogTemp, Log, TEXT("AGCGGameMode_1v1::ProcessHandLimit - Player %d has %d cards in hand (within limit)"),
			PlayerID, HandSize);
	}
}

void AGCGGameMode_1v1::CleanupTurnEffects()
{
	// TODO: Implement effect cleanup (Phase 8: Effect System)
	// This will remove all "UntilEndOfTurn" modifiers and temporary effects

	UE_LOG(LogTemp, Log, TEXT("AGCGGameMode_1v1::CleanupTurnEffects - Cleaning up turn effects"));
}
