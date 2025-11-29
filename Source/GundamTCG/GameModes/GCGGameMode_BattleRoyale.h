// GCGGameMode_BattleRoyale.h - Battle Royale Game Mode (3+ players)
// Unreal Engine 5.6 - Gundam TCG Implementation
// Implements Section 12-2: Battle Royale Rules

#pragma once

#include "CoreMinimal.h"
#include "GCGGameMode_1v1.h"
#include "GundamTCG/GCGTypes.h"
#include "GCGGameMode_BattleRoyale.generated.h"

/**
 * Victory condition mode for Battle Royale
 */
UENUM(BlueprintType)
enum class EGCGBattleRoyaleVictoryMode : uint8
{
	WinnerTakesAll    UMETA(DisplayName = "Winner Takes All"),    // Rule 12-2-5-1: Only attacker wins when defeating a player
	LastPlayerStanding UMETA(DisplayName = "Last Player Standing") // Rule 12-2-5-2: Last player alive wins
};

/**
 * Turn direction for Battle Royale
 */
UENUM(BlueprintType)
enum class EGCGTurnDirection : uint8
{
	Clockwise        UMETA(DisplayName = "Clockwise"),
	Counterclockwise UMETA(DisplayName = "Counterclockwise")
};

/**
 * Game Mode for Battle Royale (3+ players free-for-all)
 *
 * Section 12-2: Battle Royale Rules
 * - Rule 12-2-1: Clockwise or counterclockwise turn order
 * - Rule 12-2-2: All players except Player One place EX Resources
 * - Rule 12-2-3: Mulligan in order starting with Player One
 * - Rule 12-2-4: Simultaneous actions resolved in turn order
 * - Rule 12-2-5: Victory conditions (winner-takes-all or last-player-standing)
 * - Rule 12-2-6: "Enemy" or "opponent" refers to all other players
 * - Rule 12-2-7: Attack any player or any rested Unit
 * - Rule 12-2-8: Action steps in order, all players must pass
 *
 * Extends 1v1 mode with multiplayer rules.
 */
UCLASS()
class GUNDAMTCG_API AGCGGameMode_BattleRoyale : public AGCGGameMode_1v1
{
	GENERATED_BODY()

public:
	AGCGGameMode_BattleRoyale();

	// ===== BATTLE ROYALE CONFIGURATION =====

	/**
	 * Rule 12-2-5: Victory condition mode
	 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Battle Royale")
	EGCGBattleRoyaleVictoryMode VictoryMode;

	/**
	 * Rule 12-2-1: Turn direction (clockwise or counterclockwise)
	 * Set by Player One at game start
	 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Battle Royale")
	EGCGTurnDirection TurnDirection;

	/**
	 * Minimum number of players for Battle Royale
	 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Battle Royale")
	int32 MinPlayers;

	/**
	 * Maximum number of players for Battle Royale
	 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Battle Royale")
	int32 MaxPlayers;

	/**
	 * Active player IDs (players still in the game)
	 * Players who are defeated are removed from this list
	 */
	UPROPERTY(BlueprintReadOnly, Category = "Battle Royale")
	TArray<int32> ActivePlayerIDs;

	// ===== GAME INITIALIZATION (OVERRIDE) =====

	/**
	 * Override: Initialize game with Battle Royale rules
	 * - Rule 12-2-2: EX Resources for all except Player One
	 * - Rule 12-2-3: Mulligan in order starting with Player One
	 */
	virtual void InitializeGame() override;

	/**
	 * Rule 12-2-2: Place EX Resources for all players except Player One
	 */
	UFUNCTION(BlueprintCallable, Category = "Battle Royale|Setup")
	void SetupBattleRoyaleEXResources();

	/**
	 * Rule 12-2-3: Perform mulligan in order starting with Player One
	 */
	UFUNCTION(BlueprintCallable, Category = "Battle Royale|Setup")
	void PerformOrderedMulligan();

	/**
	 * Rule 12-2-1: Player One chooses turn direction
	 * @param bClockwise True for clockwise, false for counterclockwise
	 */
	UFUNCTION(BlueprintCallable, Category = "Battle Royale|Setup")
	void SetTurnDirection(bool bClockwise);

	// ===== TURN MANAGEMENT (OVERRIDE) =====

	/**
	 * Override: Start new turn with Battle Royale turn order
	 * Rule 12-2-1: Turn moves clockwise or counterclockwise
	 */
	virtual void StartNewTurn() override;

	/**
	 * Rule 12-2-1: Get next player ID in turn order
	 * @param CurrentPlayerID Current active player
	 * @return Next player ID
	 */
	UFUNCTION(BlueprintPure, Category = "Battle Royale|Turn Flow")
	int32 GetNextPlayerID(int32 CurrentPlayerID) const;

	/**
	 * Rule 12-2-1: Get previous player ID in turn order
	 * @param CurrentPlayerID Current active player
	 * @return Previous player ID
	 */
	UFUNCTION(BlueprintPure, Category = "Battle Royale|Turn Flow")
	int32 GetPreviousPlayerID(int32 CurrentPlayerID) const;

	// ===== VICTORY CONDITIONS (OVERRIDE) =====

	/**
	 * Override: Check for Battle Royale victory conditions
	 * - Rule 12-2-5-1: Winner-takes-all (attacker wins when defeating player)
	 * - Rule 12-2-5-2: Last-player-standing (last player alive wins)
	 */
	virtual void CheckVictoryConditions() override;

	/**
	 * Rule 12-2-5-1: Winner-takes-all victory
	 * Only the attacking player wins when defeating another player
	 * @param AttackingPlayerID Player who dealt the killing blow
	 * @param DefeatedPlayerID Player who was defeated
	 */
	UFUNCTION(BlueprintCallable, Category = "Battle Royale|Victory")
	void ProcessWinnerTakesAllVictory(int32 AttackingPlayerID, int32 DefeatedPlayerID);

	/**
	 * Rule 12-2-5-2: Last-player-standing victory
	 * Check if only one player remains
	 */
	UFUNCTION(BlueprintCallable, Category = "Battle Royale|Victory")
	void ProcessLastPlayerStandingVictory();

	/**
	 * Remove player from game
	 * Rule 12-2-5: Player and all their cards/effects are removed
	 * @param PlayerID Player to remove
	 */
	UFUNCTION(BlueprintCallable, Category = "Battle Royale|Victory")
	void RemovePlayerFromGame(int32 PlayerID);

	// ===== SIMULTANEOUS ACTIONS =====

	/**
	 * Rule 12-2-4: Get action resolution order
	 * Active player first, then next player in turn order, etc.
	 * @return Ordered list of player IDs for simultaneous actions
	 */
	UFUNCTION(BlueprintPure, Category = "Battle Royale|Actions")
	TArray<int32> GetSimultaneousActionOrder() const;

	// ===== ACTION STEP (OVERRIDE) =====

	/**
	 * Override: Execute Action Step with all players in sequence
	 * Rule 12-2-8: Action steps performed in order, all players must pass
	 */
	virtual void ExecuteActionStep() override;

	/**
	 * Rule 12-2-8: Process action for Battle Royale (all players in sequence)
	 * @param PlayerID Player taking action
	 * @param ActionType Type of action
	 * @param CardInstanceID Card being used (if applicable)
	 * @return True if action was successful
	 */
	virtual bool ProcessActionStepAction(int32 PlayerID, EGCGPlayerActionType ActionType, int32 CardInstanceID = 0) override;

	/**
	 * Check if all active players have passed in Action Step
	 * @return True if all players passed consecutively
	 */
	UFUNCTION(BlueprintPure, Category = "Battle Royale|Action Step")
	bool HaveAllPlayersPassedActionStep() const;

	// ===== ATTACK TARGETS =====

	/**
	 * Rule 12-2-7: Get valid attack targets for a Unit
	 * Can attack any player or any rested Unit from any player
	 * @param AttackingPlayerID Player declaring attack
	 * @return List of valid target player IDs
	 */
	UFUNCTION(BlueprintPure, Category = "Battle Royale|Combat")
	TArray<int32> GetValidAttackTargets(int32 AttackingPlayerID) const;

	// ===== ENEMY/OPPONENT REFERENCE =====

	/**
	 * Rule 12-2-6: Get all enemy players
	 * "Enemy" or "opponent" refers to all other players
	 * @param PlayerID Player to get enemies for
	 * @return List of enemy player IDs
	 */
	UFUNCTION(BlueprintPure, Category = "Battle Royale|Players")
	TArray<int32> GetEnemyPlayers(int32 PlayerID) const;

	/**
	 * Rule 12-2-6: Check if player is an enemy
	 * @param PlayerID Player to check from
	 * @param OtherPlayerID Player to check against
	 * @return True if OtherPlayerID is an enemy of PlayerID
	 */
	UFUNCTION(BlueprintPure, Category = "Battle Royale|Players")
	bool IsEnemyPlayer(int32 PlayerID, int32 OtherPlayerID) const;

	// ===== UTILITY =====

	/**
	 * Get number of active players (not defeated)
	 * @return Number of active players
	 */
	UFUNCTION(BlueprintPure, Category = "Battle Royale|Players")
	int32 GetActivePlayerCount() const;

	/**
	 * Check if player is still active (not defeated)
	 * @param PlayerID Player to check
	 * @return True if player is active
	 */
	UFUNCTION(BlueprintPure, Category = "Battle Royale|Players")
	bool IsPlayerActive(int32 PlayerID) const;

private:
	// ===== INTERNAL TRACKING =====

	/**
	 * Track which players have passed in current Action Step
	 * Used for Rule 12-2-8: Action Step ends when all players pass
	 */
	TArray<int32> PlayersWhoPassedActionStep;

	/**
	 * Last player to take an action (reset pass counter)
	 */
	int32 LastPlayerToActInActionStep;
};
