// GCGGameMode_1v1.h - 1v1 Match Game Mode with Turn/Phase State Machine
// Unreal Engine 5.6 - Gundam TCG Implementation
// This class implements the complete turn/phase system for 1v1 matches

#pragma once

#include "CoreMinimal.h"
#include "GCGGameModeBase.h"
#include "GundamTCG/GCGTypes.h"
#include "GCGGameMode_1v1.generated.h"

/**
 * Game Mode for 1v1 Gundam Card Game Matches
 *
 * This class implements the complete turn/phase state machine:
 * - Turn Structure: Start → Draw → Resource → Main → End
 * - Phase Handlers: Execute each phase's rules
 * - Game Setup: Initial decks, shields, EX Base, EX Resource for Player 2
 * - Victory Conditions: No shields when taking damage, cannot draw from empty deck
 *
 * The game is server-authoritative. All game logic runs on the server,
 * and state is replicated to clients via AGCGGameState.
 */
UCLASS()
class GUNDAMTCG_API AGCGGameMode_1v1 : public AGCGGameModeBase
{
	GENERATED_BODY()

public:
	AGCGGameMode_1v1();

	// ===== GAME INITIALIZATION =====

	/**
	 * Called when the game starts
	 */
	virtual void BeginPlay() override;

	/**
	 * Initialize the game (setup decks, shields, etc.)
	 * Called after both players have joined
	 */
	UFUNCTION(BlueprintCallable, Category = "Game Flow")
	void InitializeGame();

	/**
	 * Check if the game can start (minimum 2 players)
	 * @return True if game can start
	 */
	UFUNCTION(BlueprintPure, Category = "Game Flow")
	bool CanStartGame() const;

	// ===== TURN MANAGEMENT =====

	/**
	 * Start a new turn
	 * Increments turn number, switches active player, enters Start Phase
	 */
	UFUNCTION(BlueprintCallable, Category = "Turn Flow")
	void StartNewTurn();

	/**
	 * Advance to the next phase
	 * Automatically progresses through turn phases
	 */
	UFUNCTION(BlueprintCallable, Category = "Turn Flow")
	void AdvancePhase();

	/**
	 * End the current turn and pass to the other player
	 */
	UFUNCTION(BlueprintCallable, Category = "Turn Flow")
	void EndTurn();

	// ===== PHASE HANDLERS =====

	/**
	 * Execute Start Phase
	 * - Active Step: Set all rested cards to active
	 * - Start Step: Trigger "at start of turn" effects
	 */
	UFUNCTION(BlueprintCallable, Category = "Phase Handlers")
	void ExecuteStartPhase();

	/**
	 * Execute Draw Phase
	 * - Player draws 1 card (mandatory)
	 * - If deck is empty after draw → player loses
	 */
	UFUNCTION(BlueprintCallable, Category = "Phase Handlers")
	void ExecuteDrawPhase();

	/**
	 * Execute Resource Phase
	 * - Player places 1 card from Resource Deck to Resource Area (mandatory)
	 * - Card is placed face up and active
	 * - If Resource Deck is empty, phase still passes but no placement
	 */
	UFUNCTION(BlueprintCallable, Category = "Phase Handlers")
	void ExecuteResourcePhase();

	/**
	 * Execute Main Phase
	 * - Player can play cards, activate abilities, declare attacks
	 * - Phase waits for player input (does not auto-advance)
	 */
	UFUNCTION(BlueprintCallable, Category = "Phase Handlers")
	void ExecuteMainPhase();

	/**
	 * Execute End Phase
	 * - Action Step: Action timing (Commands, Activate・Action abilities)
	 * - End Step: "At end of turn" effects trigger
	 * - Hand Step: If hand ≥ 11, discard to 10
	 * - Cleanup Step: "During this turn" effects expire
	 */
	UFUNCTION(BlueprintCallable, Category = "Phase Handlers")
	void ExecuteEndPhase();

	// ===== GAME FLOW CONTROL =====

	/**
	 * Check victory conditions for all players
	 * Called after certain actions (damage dealt, draw attempted)
	 */
	UFUNCTION(BlueprintCallable, Category = "Game Flow")
	void CheckVictoryConditions();

	/**
	 * End the game with a winner
	 * @param WinnerPlayerID The winning player ID (-1 for no winner, -2 for draw)
	 */
	UFUNCTION(BlueprintCallable, Category = "Game Flow")
	void EndGame(int32 WinnerPlayerID);

	// ===== AUTOMATIC PHASE PROGRESSION =====

	/**
	 * Should this phase auto-advance after execution?
	 * Some phases (Start, Draw, Resource) auto-advance, others (Main) wait for player
	 * @param Phase The phase to check
	 * @return True if phase should auto-advance
	 */
	UFUNCTION(BlueprintPure, Category = "Phase Handlers")
	bool ShouldPhaseAutoAdvance(EGCGTurnPhase Phase) const;

	// ===== PLAYER ACTIONS (called from PlayerController RPCs) =====

	/**
	 * Player requests to pass priority (advance to next phase/step)
	 * @param PlayerID The player requesting to pass
	 */
	UFUNCTION(BlueprintCallable, Category = "Player Actions")
	void RequestPassPriority(int32 PlayerID);

	/**
	 * Player requests to play a card from hand
	 * @param PlayerID The player making the request
	 * @param CardInstanceID The card to play
	 * @return True if action was successful
	 */
	UFUNCTION(BlueprintCallable, Category = "Player Actions")
	bool RequestPlayCard(int32 PlayerID, int32 CardInstanceID);

	/**
	 * Player requests to place a card from hand as a resource
	 * @param PlayerID The player making the request
	 * @param CardInstanceID The card to place as resource
	 * @param bFaceUp Should the resource be face-up?
	 * @return True if action was successful
	 */
	UFUNCTION(BlueprintCallable, Category = "Player Actions")
	bool RequestPlaceResource(int32 PlayerID, int32 CardInstanceID, bool bFaceUp = false);

	/**
	 * Player requests to discard cards (for hand limit)
	 * @param PlayerID The player making the request
	 * @param CardInstanceIDs The cards to discard
	 * @return Number of cards successfully discarded
	 */
	UFUNCTION(BlueprintCallable, Category = "Player Actions")
	int32 RequestDiscardCards(int32 PlayerID, const TArray<int32>& CardInstanceIDs);

	/**
	 * Player requests to declare an attack
	 * @param PlayerID The player making the request
	 * @param AttackerInstanceID The attacking unit
	 * @return True if attack was successfully declared
	 */
	UFUNCTION(BlueprintCallable, Category = "Player Actions|Combat")
	bool RequestDeclareAttack(int32 PlayerID, int32 AttackerInstanceID);

	/**
	 * Player requests to declare a blocker
	 * @param PlayerID The player making the request
	 * @param AttackIndex The index of the attack to block
	 * @param BlockerInstanceID The blocking unit
	 * @return True if blocker was successfully declared
	 */
	UFUNCTION(BlueprintCallable, Category = "Player Actions|Combat")
	bool RequestDeclareBlocker(int32 PlayerID, int32 AttackIndex, int32 BlockerInstanceID);

	/**
	 * Resolve all declared attacks
	 * Called after attack/block phase is complete
	 */
	UFUNCTION(BlueprintCallable, Category = "Player Actions|Combat")
	void ResolveCombat();

	// ===== SETUP HELPERS =====

	/**
	 * Setup initial decks for a player
	 * @param PlayerID The player to setup
	 * @param MainDeckList Array of card numbers for main deck (50 cards)
	 * @param ResourceDeckList Array of card numbers for resource deck (10 cards)
	 */
	UFUNCTION(BlueprintCallable, Category = "Setup")
	void SetupPlayerDecks(int32 PlayerID, const TArray<FName>& MainDeckList, const TArray<FName>& ResourceDeckList);

	/**
	 * Setup initial shields for a player
	 * Takes 6 cards from top of deck and places them in Shield Stack
	 * @param PlayerID The player to setup shields for
	 */
	UFUNCTION(BlueprintCallable, Category = "Setup")
	void SetupPlayerShields(int32 PlayerID);

	/**
	 * Setup EX Base for a player
	 * @param PlayerID The player to setup EX Base for
	 */
	UFUNCTION(BlueprintCallable, Category = "Setup")
	void SetupEXBase(int32 PlayerID);

	/**
	 * Setup EX Resource for Player 2
	 * @param PlayerID Should be player 2 (ID = 1)
	 */
	UFUNCTION(BlueprintCallable, Category = "Setup")
	void SetupEXResource(int32 PlayerID);

protected:
	// ===== INTERNAL HELPERS =====

	/**
	 * Get the next player ID (for turn switching)
	 * In 1v1, this alternates between 0 and 1
	 * @param CurrentPlayerID Current active player ID
	 * @return Next player ID
	 */
	int32 GetNextPlayerID(int32 CurrentPlayerID) const;

	/**
	 * Set all rested cards in a zone to active
	 * @param PlayerID The player whose cards to activate
	 */
	void ActivateAllCardsForPlayer(int32 PlayerID);

	/**
	 * Process hand limit (discard to 10 if hand > 10)
	 * @param PlayerID The player to check
	 */
	void ProcessHandLimit(int32 PlayerID);

	/**
	 * Cleanup "during this turn" effects
	 */
	void CleanupTurnEffects();

	// ===== TIMERS =====

	/**
	 * Timer handle for auto-advancing phases
	 */
	FTimerHandle PhaseAdvanceTimerHandle;

	/**
	 * Delay before auto-advancing to next phase (in seconds)
	 */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Timing")
	float PhaseAdvanceDelay;

	// ===== BLUEPRINT EVENTS =====

	/**
	 * Called when a phase is executed
	 * @param Phase The phase that was executed
	 */
	UFUNCTION(BlueprintImplementableEvent, Category = "Events")
	void OnPhaseExecuted(EGCGTurnPhase Phase);

	/**
	 * Called when a turn starts
	 * @param TurnNumber The new turn number
	 * @param ActivePlayerID The active player for this turn
	 */
	UFUNCTION(BlueprintImplementableEvent, Category = "Events")
	void OnTurnStarted(int32 TurnNumber, int32 ActivePlayerID);

	/**
	 * Called when a turn ends
	 * @param TurnNumber The turn that ended
	 */
	UFUNCTION(BlueprintImplementableEvent, Category = "Events")
	void OnTurnEnded(int32 TurnNumber);
};
