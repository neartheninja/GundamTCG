// GCGGameState.h - Replicated Game State for Gundam Card Game
// Unreal Engine 5.6 - Gundam TCG Implementation
// This class holds all replicated game state (turn, phase, active player, etc.)

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/GameState.h"
#include "GundamTCG/GCGTypes.h"
#include "Net/UnrealNetwork.h"
#include "GCGGameState.generated.h"

/**
 * Game State for Gundam Card Game
 *
 * This class holds all replicated game state that all clients need to know:
 * - Current turn number
 * - Current phase and step
 * - Active player ID
 * - Game status (not started, in progress, game over)
 * - Team information (for 2v2 mode)
 *
 * All properties are replicated so clients stay in sync with the server.
 */
UCLASS()
class GUNDAMTCG_API AGCGGameState : public AGameState
{
	GENERATED_BODY()

public:
	AGCGGameState();

	// ===== REPLICATION SETUP =====

	/**
	 * Configure which properties replicate
	 */
	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;

	// ===== GAME STATUS =====

	/**
	 * Is the game currently in progress?
	 */
	UPROPERTY(Replicated, BlueprintReadOnly, Category = "Game Status")
	bool bGameInProgress;

	/**
	 * Has the game ended?
	 */
	UPROPERTY(Replicated, BlueprintReadOnly, Category = "Game Status")
	bool bGameOver;

	/**
	 * Winner player ID (-1 if no winner yet, -2 if draw)
	 */
	UPROPERTY(Replicated, BlueprintReadOnly, Category = "Game Status")
	int32 WinnerPlayerID;

	// ===== TURN TRACKING =====

	/**
	 * Current turn number (starts at 1)
	 */
	UPROPERTY(ReplicatedUsing = OnRep_TurnNumber, BlueprintReadOnly, Category = "Turn")
	int32 TurnNumber;

	/**
	 * Current turn phase
	 */
	UPROPERTY(ReplicatedUsing = OnRep_CurrentPhase, BlueprintReadOnly, Category = "Turn")
	EGCGTurnPhase CurrentPhase;

	/**
	 * Current Start Phase step (if in Start Phase)
	 */
	UPROPERTY(Replicated, BlueprintReadOnly, Category = "Turn")
	EGCGStartPhaseStep CurrentStartPhaseStep;

	/**
	 * Current End Phase step (if in End Phase)
	 */
	UPROPERTY(Replicated, BlueprintReadOnly, Category = "Turn")
	EGCGEndPhaseStep CurrentEndPhaseStep;

	/**
	 * Active player ID (whose turn it is)
	 */
	UPROPERTY(ReplicatedUsing = OnRep_ActivePlayerID, BlueprintReadOnly, Category = "Turn")
	int32 ActivePlayerID;

	// ===== COMBAT TRACKING =====

	/**
	 * Is there currently an attack in progress?
	 */
	UPROPERTY(Replicated, BlueprintReadOnly, Category = "Combat")
	bool bAttackInProgress;

	/**
	 * Current attack data (if attack in progress)
	 */
	UPROPERTY(Replicated, BlueprintReadOnly, Category = "Combat")
	FGCGAttackData CurrentAttack;

	// ===== TEAM BATTLE (2v2) =====

	/**
	 * Is this a team battle game? (2v2 mode)
	 */
	UPROPERTY(Replicated, BlueprintReadOnly, Category = "Team Battle")
	bool bIsTeamBattle;

	/**
	 * Team A information (Player 1 & 2)
	 */
	UPROPERTY(Replicated, BlueprintReadOnly, Category = "Team Battle")
	FGCGTeamInfo TeamA;

	/**
	 * Team B information (Player 3 & 4)
	 */
	UPROPERTY(Replicated, BlueprintReadOnly, Category = "Team Battle")
	FGCGTeamInfo TeamB;

	// ===== REPLICATION CALLBACKS =====

	/**
	 * Called when turn number is replicated
	 */
	UFUNCTION()
	void OnRep_TurnNumber();

	/**
	 * Called when current phase is replicated
	 */
	UFUNCTION()
	void OnRep_CurrentPhase();

	/**
	 * Called when active player ID is replicated
	 */
	UFUNCTION()
	void OnRep_ActivePlayerID();

	// ===== HELPER FUNCTIONS =====

	/**
	 * Get the team info for a player
	 * @param PlayerID The player ID to check
	 * @return Pointer to team info, or nullptr if not in team battle or player not found
	 */
	UFUNCTION(BlueprintPure, Category = "Team Battle")
	const FGCGTeamInfo* GetTeamForPlayer(int32 PlayerID) const;

	/**
	 * Check if a player is the active player
	 * @param PlayerID The player ID to check
	 * @return True if this player is currently active
	 */
	UFUNCTION(BlueprintPure, Category = "Turn")
	bool IsPlayerActive(int32 PlayerID) const;

	/**
	 * Check if a player is on the active team (for team battle)
	 * @param PlayerID The player ID to check
	 * @return True if this player's team is currently active
	 */
	UFUNCTION(BlueprintPure, Category = "Team Battle")
	bool IsPlayerTeamActive(int32 PlayerID) const;

	/**
	 * Get the team ID for a player (0 = Team A, 1 = Team B, -1 = not in team)
	 * @param PlayerID The player ID to check
	 * @return Team ID or -1 if not in a team
	 */
	UFUNCTION(BlueprintPure, Category = "Team Battle")
	int32 GetPlayerTeamID(int32 PlayerID) const;

	/**
	 * Check if two players are teammates
	 * @param PlayerID1 First player ID
	 * @param PlayerID2 Second player ID
	 * @return True if both players are on the same team
	 */
	UFUNCTION(BlueprintPure, Category = "Team Battle")
	bool ArePlayersTeammates(int32 PlayerID1, int32 PlayerID2) const;

	/**
	 * Get phase name as string (for UI display)
	 * @return Human-readable phase name
	 */
	UFUNCTION(BlueprintPure, Category = "Turn")
	FString GetPhaseName() const;

	/**
	 * Get step name as string (for UI display)
	 * @return Human-readable step name
	 */
	UFUNCTION(BlueprintPure, Category = "Turn")
	FString GetStepName() const;

	// ===== BLUEPRINT EVENTS =====

	/**
	 * Called when turn number changes
	 * @param NewTurnNumber The new turn number
	 */
	UFUNCTION(BlueprintImplementableEvent, Category = "Events")
	void OnTurnNumberChanged(int32 NewTurnNumber);

	/**
	 * Called when phase changes
	 * @param NewPhase The new phase
	 */
	UFUNCTION(BlueprintImplementableEvent, Category = "Events")
	void OnPhaseChanged(EGCGTurnPhase NewPhase);

	/**
	 * Called when active player changes
	 * @param NewActivePlayerID The new active player ID
	 */
	UFUNCTION(BlueprintImplementableEvent, Category = "Events")
	void OnActivePlayerChanged(int32 NewActivePlayerID);

	/**
	 * Called when game starts
	 */
	UFUNCTION(BlueprintImplementableEvent, Category = "Events")
	void OnGameStarted();

	/**
	 * Called when game ends
	 * @param WinnerID The winner player ID (-1 = no winner, -2 = draw)
	 */
	UFUNCTION(BlueprintImplementableEvent, Category = "Events")
	void OnGameEnded(int32 WinnerID);
};
