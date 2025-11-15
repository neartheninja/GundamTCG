// GCGGameMode_2v2.h - 2v2 Team Battle Game Mode
// Unreal Engine 5.6 - Gundam TCG Implementation
// This class implements 2v2 Team Battle with shared shields and team-wide Unit limits

#pragma once

#include "CoreMinimal.h"
#include "GCGGameMode_1v1.h"
#include "GundamTCG/GCGTypes.h"
#include "GCGGameMode_2v2.generated.h"

/**
 * Game Mode for 2v2 Team Battle Gundam Card Game Matches
 *
 * Team Battle Rules:
 * - 4 players total (2 teams of 2)
 * - Shared Shield Stack: 8 shields per team (4 from each player, alternating)
 * - Shared Base: 1 Base per team
 * - Team-wide Unit limit: 6 Units max per team total
 * - Simultaneous turns: Both teammates act during their team's turn
 * - Team leader makes final decisions on conflicts
 * - "Friendly" includes teammate's Units
 * - Can attack either opponent
 *
 * The game is server-authoritative. All game logic runs on the server,
 * and state is replicated to clients via AGCGGameState.
 */
UCLASS()
class GUNDAMTCG_API AGCGGameMode_2v2 : public AGCGGameMode_1v1
{
	GENERATED_BODY()

public:
	AGCGGameMode_2v2();

	// ===========================================================================================
	// GAME INITIALIZATION
	// ===========================================================================================

	/**
	 * Initialize the game (setup teams, decks, shared shields, etc.)
	 * Called after all 4 players have joined
	 */
	virtual void InitializeGame() override;

	/**
	 * Check if the game can start (minimum 4 players)
	 * @return True if game can start
	 */
	virtual bool CanStartGame() const override;

	// ===========================================================================================
	// TEAM MANAGEMENT
	// ===========================================================================================

	/**
	 * Setup teams for 2v2 battle
	 * Team A: Players 0 and 2
	 * Team B: Players 1 and 3
	 */
	UFUNCTION(BlueprintCallable, Category = "Team Battle")
	void SetupTeams();

	/**
	 * Get team info for a player
	 * @param PlayerID The player ID
	 * @return Team info, or nullptr if not found
	 */
	UFUNCTION(BlueprintPure, Category = "Team Battle")
	FGCGTeamInfo* GetTeamForPlayer(int32 PlayerID);

	/**
	 * Get the other player on the same team
	 * @param PlayerID The player ID
	 * @return Teammate's player ID, or -1 if not found
	 */
	UFUNCTION(BlueprintPure, Category = "Team Battle")
	int32 GetTeammateID(int32 PlayerID) const;

	/**
	 * Check if two players are on the same team
	 * @param PlayerID1 First player ID
	 * @param PlayerID2 Second player ID
	 * @return True if teammates
	 */
	UFUNCTION(BlueprintPure, Category = "Team Battle")
	bool AreTeammates(int32 PlayerID1, int32 PlayerID2) const;

	/**
	 * Get the total Units on field for a team
	 * @param TeamID The team ID (0 or 1)
	 * @return Total Units on field
	 */
	UFUNCTION(BlueprintPure, Category = "Team Battle")
	int32 GetTeamUnitCount(int32 TeamID) const;

	/**
	 * Check if a team can add more Units (max 6 per team)
	 * @param TeamID The team ID
	 * @return True if can add more Units
	 */
	UFUNCTION(BlueprintPure, Category = "Team Battle")
	bool CanTeamAddUnit(int32 TeamID) const;

	// ===========================================================================================
	// TURN MANAGEMENT (OVERRIDES)
	// ===========================================================================================

	/**
	 * Start a new turn (team turn)
	 * Both players on the active team act simultaneously
	 */
	virtual void StartNewTurn() override;

	/**
	 * End the current turn and pass to the other team
	 */
	virtual void EndTurn() override;

	// ===========================================================================================
	// SETUP HELPERS (OVERRIDES)
	// ===========================================================================================

	/**
	 * Setup shared shield stack for a team
	 * 8 shields total: 4 from each player, alternating
	 * Order: P1, P2, P1, P2, P1, P2, P1, P2 (from top to bottom)
	 * @param TeamID The team ID (0 = Team A, 1 = Team B)
	 */
	UFUNCTION(BlueprintCallable, Category = "Setup")
	void SetupTeamShields(int32 TeamID);

	/**
	 * Setup shared EX Base for a team
	 * @param TeamID The team ID
	 */
	UFUNCTION(BlueprintCallable, Category = "Setup")
	void SetupTeamEXBase(int32 TeamID);

	/**
	 * Setup EX Resource for Team B (going second advantage)
	 * Each player on Team B gets 1 EX Resource
	 * @param TeamID Should be Team B (ID = 1)
	 */
	UFUNCTION(BlueprintCallable, Category = "Setup")
	void SetupTeamEXResources(int32 TeamID);

	// ===========================================================================================
	// PLAYER ACTIONS (OVERRIDES)
	// ===========================================================================================

	/**
	 * Validate that a player can take actions
	 * In 2v2, both teammates can act during their team's turn
	 * @param PlayerID The player making the request
	 * @return True if player can act
	 */
	UFUNCTION(BlueprintPure, Category = "Team Battle")
	bool CanPlayerAct(int32 PlayerID) const;

	// ===========================================================================================
	// COMBAT (OVERRIDES)
	// ===========================================================================================

	/**
	 * Player requests to declare an attack
	 * In 2v2, can attack either opponent
	 * @param PlayerID The player making the request
	 * @param AttackerInstanceID The attacking unit
	 * @param TargetPlayerID The target player (either opponent)
	 * @return True if attack was successfully declared
	 */
	UFUNCTION(BlueprintCallable, Category = "Team Battle|Combat")
	bool RequestDeclareAttack_2v2(int32 PlayerID, int32 AttackerInstanceID, int32 TargetPlayerID);

	/**
	 * Player requests to declare a blocker
	 * In 2v2, can block for teammate
	 * @param PlayerID The player making the request
	 * @param AttackIndex The index of the attack to block
	 * @param BlockerInstanceID The blocking unit
	 * @return True if blocker was successfully declared
	 */
	UFUNCTION(BlueprintCallable, Category = "Team Battle|Combat")
	bool RequestDeclareBlocker_2v2(int32 PlayerID, int32 AttackIndex, int32 BlockerInstanceID);

	// ===========================================================================================
	// VICTORY CONDITIONS (OVERRIDES)
	// ===========================================================================================

	/**
	 * Check victory conditions for 2v2
	 * Team loses when shared Base is destroyed
	 * @param TeamID The team to check
	 */
	UFUNCTION(BlueprintCallable, Category = "Team Battle")
	void CheckTeamVictoryCondition(int32 TeamID);

	/**
	 * End the game with a winning team
	 * @param WinningTeamID The team that won (0 or 1)
	 */
	UFUNCTION(BlueprintCallable, Category = "Team Battle")
	void EndGameTeamVictory(int32 WinningTeamID);

protected:
	// ===========================================================================================
	// INTERNAL HELPERS
	// ===========================================================================================

	/**
	 * Get the next team ID (for turn switching)
	 * In 2v2, this alternates between 0 and 1
	 * @param CurrentTeamID Current active team ID
	 * @return Next team ID
	 */
	int32 GetNextTeamID(int32 CurrentTeamID) const;

	/**
	 * Activate all cards for both players on a team
	 * @param TeamID The team ID
	 */
	void ActivateAllCardsForTeam(int32 TeamID);

	/**
	 * Process hand limit for both players on a team
	 * @param TeamID The team ID
	 */
	void ProcessHandLimitForTeam(int32 TeamID);

	/**
	 * Get all players on a team
	 * @param TeamID The team ID
	 * @return Array of player IDs
	 */
	TArray<int32> GetPlayersOnTeam(int32 TeamID) const;

	// ===========================================================================================
	// PROPERTIES
	// ===========================================================================================

	// Team A info (Players 0 and 2)
	UPROPERTY(BlueprintReadOnly, Category = "Team Battle")
	FGCGTeamInfo TeamA;

	// Team B info (Players 1 and 3)
	UPROPERTY(BlueprintReadOnly, Category = "Team Battle")
	FGCGTeamInfo TeamB;

	// Maximum Units per team
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Team Battle")
	int32 MaxUnitsPerTeam = 6;

	// Shields per team
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Team Battle")
	int32 ShieldsPerTeam = 8;

	// Shields contributed per player
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Team Battle")
	int32 ShieldsPerPlayer = 4;
};
