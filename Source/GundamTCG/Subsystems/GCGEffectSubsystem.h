// GCGEffectSubsystem.h - Gundam Card Game Effect Processing Subsystem
// Handles all card effects: timing, conditions, costs, operations, and modifiers

#pragma once

#include "CoreMinimal.h"
#include "Subsystems/GameInstanceSubsystem.h"
#include "OnePieceTCG_V2/GCGTypes.h"
#include "GCGEffectSubsystem.generated.h"

// Forward declarations
class AGCGPlayerState;
class AGCGGameState;

/**
 * Effect Execution Result
 * Returned by effect processing functions
 */
USTRUCT(BlueprintType)
struct FGCGEffectResult
{
	GENERATED_BODY()

	UPROPERTY(BlueprintReadOnly)
	bool bSuccess = false;

	UPROPERTY(BlueprintReadOnly)
	FText Message;

	// Result data
	UPROPERTY(BlueprintReadOnly)
	int32 CardsDrawn = 0;

	UPROPERTY(BlueprintReadOnly)
	int32 DamageDealt = 0;

	UPROPERTY(BlueprintReadOnly)
	int32 UnitsDestroyed = 0;

	UPROPERTY(BlueprintReadOnly)
	int32 APGranted = 0;

	UPROPERTY(BlueprintReadOnly)
	TArray<int32> AffectedCardIDs;

	FGCGEffectResult()
		: bSuccess(false), Message(FText::GetEmpty()), CardsDrawn(0), DamageDealt(0),
		  UnitsDestroyed(0), APGranted(0)
	{
	}

	explicit FGCGEffectResult(bool bInSuccess, const FText& InMessage = FText::GetEmpty())
		: bSuccess(bInSuccess), Message(InMessage), CardsDrawn(0), DamageDealt(0),
		  UnitsDestroyed(0), APGranted(0)
	{
	}
};

/**
 * Effect Context
 * Provides context for effect execution (who triggered it, targets, etc.)
 */
USTRUCT(BlueprintType)
struct FGCGEffectContext
{
	GENERATED_BODY()

	// Source card that triggered the effect
	UPROPERTY(BlueprintReadWrite)
	int32 SourceCardInstanceID = 0;

	// Player who owns the source card
	UPROPERTY(BlueprintReadWrite)
	int32 SourcePlayerID = 0;

	// Target card (if any)
	UPROPERTY(BlueprintReadWrite)
	int32 TargetCardInstanceID = 0;

	// Target player (if any)
	UPROPERTY(BlueprintReadWrite)
	int32 TargetPlayerID = 0;

	// Current turn number
	UPROPERTY(BlueprintReadWrite)
	int32 TurnNumber = 0;

	// Additional context data
	UPROPERTY(BlueprintReadWrite)
	TMap<FName, int32> AdditionalData;
};

/**
 * UGCGEffectSubsystem
 *
 * Game Instance Subsystem that handles all card effect processing.
 *
 * Responsibilities:
 * - Trigger effects at the right timing (OnDeploy, OnAttack, Burst, etc.)
 * - Validate effect conditions
 * - Pay effect costs
 * - Execute effect operations (Draw, Damage, Buff, etc.)
 * - Manage active modifiers (AP/HP buffs with durations)
 * - Clean up expired modifiers
 *
 * Phase 8 Implementation:
 * Complete data-driven effect system - no hardcoded card effects.
 */
UCLASS()
class GUNDAMTCG_API UGCGEffectSubsystem : public UGameInstanceSubsystem
{
	GENERATED_BODY()

public:
	// ===========================================================================================
	// SUBSYSTEM LIFECYCLE
	// ===========================================================================================

	virtual void Initialize(FSubsystemCollectionBase& Collection) override;
	virtual void Deinitialize() override;

	// ===========================================================================================
	// EFFECT TRIGGERING
	// ===========================================================================================

	/**
	 * Trigger all effects with a specific timing
	 * @param Timing - Effect timing to trigger
	 * @param Context - Effect context (source, target, etc.)
	 * @param GameState - Current game state
	 * @return Array of effect results
	 */
	UFUNCTION(BlueprintCallable, Category = "GCG|Effects")
	TArray<FGCGEffectResult> TriggerEffects(EGCGEffectTiming Timing, const FGCGEffectContext& Context, AGCGGameState* GameState);

	/**
	 * Trigger effects for a specific card
	 * @param CardInstance - Card whose effects to trigger
	 * @param Timing - Effect timing to trigger
	 * @param Context - Effect context
	 * @param SourcePlayer - Player who owns the card
	 * @param GameState - Current game state
	 * @return Array of effect results
	 */
	UFUNCTION(BlueprintCallable, Category = "GCG|Effects")
	TArray<FGCGEffectResult> TriggerCardEffects(const FGCGCardInstance& CardInstance, EGCGEffectTiming Timing,
		const FGCGEffectContext& Context, AGCGPlayerState* SourcePlayer, AGCGGameState* GameState);

	/**
	 * Execute a single effect
	 * @param Effect - Effect data to execute
	 * @param Context - Effect context
	 * @param SourcePlayer - Player who owns the source card
	 * @param GameState - Current game state
	 * @return Effect result
	 */
	UFUNCTION(BlueprintCallable, Category = "GCG|Effects")
	FGCGEffectResult ExecuteEffect(const FGCGEffectData& Effect, const FGCGEffectContext& Context,
		AGCGPlayerState* SourcePlayer, AGCGGameState* GameState);

	// ===========================================================================================
	// EFFECT VALIDATION
	// ===========================================================================================

	/**
	 * Check if all conditions for an effect are met
	 * @param Conditions - Conditions to check
	 * @param Context - Effect context
	 * @param SourcePlayer - Player who owns the source card
	 * @param GameState - Current game state
	 * @return True if all conditions met
	 */
	UFUNCTION(BlueprintCallable, Category = "GCG|Effects")
	bool CheckConditions(const TArray<FGCGEffectCondition>& Conditions, const FGCGEffectContext& Context,
		AGCGPlayerState* SourcePlayer, AGCGGameState* GameState);

	/**
	 * Check a single condition
	 * @param Condition - Condition to check
	 * @param Context - Effect context
	 * @param SourcePlayer - Player who owns the source card
	 * @param GameState - Current game state
	 * @return True if condition met
	 */
	UFUNCTION(BlueprintCallable, Category = "GCG|Effects")
	bool CheckCondition(const FGCGEffectCondition& Condition, const FGCGEffectContext& Context,
		AGCGPlayerState* SourcePlayer, AGCGGameState* GameState);

	// ===========================================================================================
	// EFFECT COSTS
	// ===========================================================================================

	/**
	 * Check if player can pay all costs
	 * @param Costs - Costs to pay
	 * @param Context - Effect context
	 * @param SourcePlayer - Player who will pay costs
	 * @param GameState - Current game state
	 * @return True if can pay all costs
	 */
	UFUNCTION(BlueprintCallable, Category = "GCG|Effects")
	bool CanPayCosts(const TArray<FGCGEffectCost>& Costs, const FGCGEffectContext& Context,
		AGCGPlayerState* SourcePlayer, AGCGGameState* GameState);

	/**
	 * Pay all costs for an effect
	 * @param Costs - Costs to pay
	 * @param Context - Effect context
	 * @param SourcePlayer - Player who will pay costs
	 * @param GameState - Current game state
	 * @return True if all costs paid successfully
	 */
	UFUNCTION(BlueprintCallable, Category = "GCG|Effects")
	bool PayCosts(const TArray<FGCGEffectCost>& Costs, const FGCGEffectContext& Context,
		AGCGPlayerState* SourcePlayer, AGCGGameState* GameState);

	/**
	 * Pay a single cost
	 * @param Cost - Cost to pay
	 * @param Context - Effect context
	 * @param SourcePlayer - Player who will pay cost
	 * @param GameState - Current game state
	 * @return True if cost paid successfully
	 */
	UFUNCTION(BlueprintCallable, Category = "GCG|Effects")
	bool PayCost(const FGCGEffectCost& Cost, const FGCGEffectContext& Context,
		AGCGPlayerState* SourcePlayer, AGCGGameState* GameState);

	// ===========================================================================================
	// EFFECT OPERATIONS
	// ===========================================================================================

	/**
	 * Execute all operations for an effect
	 * @param Operations - Operations to execute
	 * @param Context - Effect context
	 * @param SourcePlayer - Player who owns the source card
	 * @param GameState - Current game state
	 * @return Combined effect result
	 */
	UFUNCTION(BlueprintCallable, Category = "GCG|Effects")
	FGCGEffectResult ExecuteOperations(const TArray<FGCGEffectOperation>& Operations, const FGCGEffectContext& Context,
		AGCGPlayerState* SourcePlayer, AGCGGameState* GameState);

	/**
	 * Execute a single operation
	 * @param Operation - Operation to execute
	 * @param Context - Effect context
	 * @param SourcePlayer - Player who owns the source card
	 * @param GameState - Current game state
	 * @return Effect result
	 */
	UFUNCTION(BlueprintCallable, Category = "GCG|Effects")
	FGCGEffectResult ExecuteOperation(const FGCGEffectOperation& Operation, const FGCGEffectContext& Context,
		AGCGPlayerState* SourcePlayer, AGCGGameState* GameState);

	// ===========================================================================================
	// SPECIFIC OPERATIONS
	// ===========================================================================================

	/**
	 * Draw cards operation
	 * @param Amount - Number of cards to draw
	 * @param TargetPlayer - Player who draws
	 * @param GameState - Current game state
	 * @return Effect result with cards drawn
	 */
	UFUNCTION(BlueprintCallable, Category = "GCG|Effects|Operations")
	FGCGEffectResult OP_DrawCards(int32 Amount, AGCGPlayerState* TargetPlayer, AGCGGameState* GameState);

	/**
	 * Deal damage to Unit operation
	 * @param Amount - Amount of damage
	 * @param TargetInstanceID - Target Unit instance ID
	 * @param TargetPlayer - Player who owns target
	 * @param GameState - Current game state
	 * @return Effect result with damage dealt
	 */
	UFUNCTION(BlueprintCallable, Category = "GCG|Effects|Operations")
	FGCGEffectResult OP_DealDamageToUnit(int32 Amount, int32 TargetInstanceID, AGCGPlayerState* TargetPlayer, AGCGGameState* GameState);

	/**
	 * Deal damage to player operation
	 * @param Amount - Amount of damage
	 * @param TargetPlayer - Target player
	 * @param GameState - Current game state
	 * @return Effect result with shields broken
	 */
	UFUNCTION(BlueprintCallable, Category = "GCG|Effects|Operations")
	FGCGEffectResult OP_DealDamageToPlayer(int32 Amount, AGCGPlayerState* TargetPlayer, AGCGGameState* GameState);

	/**
	 * Destroy Unit operation
	 * @param TargetInstanceID - Target Unit instance ID
	 * @param TargetPlayer - Player who owns target
	 * @param GameState - Current game state
	 * @return Effect result with units destroyed
	 */
	UFUNCTION(BlueprintCallable, Category = "GCG|Effects|Operations")
	FGCGEffectResult OP_DestroyUnit(int32 TargetInstanceID, AGCGPlayerState* TargetPlayer, AGCGGameState* GameState);

	/**
	 * Give AP buff operation
	 * @param Amount - AP to grant
	 * @param Duration - How long the buff lasts
	 * @param TargetInstanceID - Target card instance ID
	 * @param TargetPlayer - Player who owns target
	 * @param SourceInstanceID - Source card instance ID
	 * @param GameState - Current game state
	 * @return Effect result with AP granted
	 */
	UFUNCTION(BlueprintCallable, Category = "GCG|Effects|Operations")
	FGCGEffectResult OP_GiveAP(int32 Amount, EGCGModifierDuration Duration, int32 TargetInstanceID,
		AGCGPlayerState* TargetPlayer, int32 SourceInstanceID, AGCGGameState* GameState);

	/**
	 * Give HP buff operation
	 * @param Amount - HP to grant
	 * @param Duration - How long the buff lasts
	 * @param TargetInstanceID - Target card instance ID
	 * @param TargetPlayer - Player who owns target
	 * @param SourceInstanceID - Source card instance ID
	 * @param GameState - Current game state
	 * @return Effect result
	 */
	UFUNCTION(BlueprintCallable, Category = "GCG|Effects|Operations")
	FGCGEffectResult OP_GiveHP(int32 Amount, EGCGModifierDuration Duration, int32 TargetInstanceID,
		AGCGPlayerState* TargetPlayer, int32 SourceInstanceID, AGCGGameState* GameState);

	/**
	 * Grant keyword operation
	 * @param Keyword - Keyword to grant
	 * @param Value - Keyword value (for stacking keywords)
	 * @param TargetInstanceID - Target card instance ID
	 * @param TargetPlayer - Player who owns target
	 * @param SourceInstanceID - Source card instance ID
	 * @return Effect result
	 */
	UFUNCTION(BlueprintCallable, Category = "GCG|Effects|Operations")
	FGCGEffectResult OP_GrantKeyword(EGCGKeyword Keyword, int32 Value, int32 TargetInstanceID,
		AGCGPlayerState* TargetPlayer, int32 SourceInstanceID);

	// ===========================================================================================
	// MODIFIER MANAGEMENT
	// ===========================================================================================

	/**
	 * Add a modifier to a card
	 * @param Card - Card to modify
	 * @param ModifierType - Type of modifier (AP, HP, Cost)
	 * @param Amount - Modifier amount
	 * @param Duration - How long the modifier lasts
	 * @param SourceInstanceID - Card that applied the modifier
	 * @param GameState - Current game state
	 */
	UFUNCTION(BlueprintCallable, Category = "GCG|Effects|Modifiers")
	void AddModifier(UPARAM(ref) FGCGCardInstance& Card, FName ModifierType, int32 Amount,
		EGCGModifierDuration Duration, int32 SourceInstanceID, AGCGGameState* GameState);

	/**
	 * Remove modifiers from a card by source
	 * @param Card - Card to modify
	 * @param SourceInstanceID - Source card instance ID
	 */
	UFUNCTION(BlueprintCallable, Category = "GCG|Effects|Modifiers")
	void RemoveModifiersBySource(UPARAM(ref) FGCGCardInstance& Card, int32 SourceInstanceID);

	/**
	 * Clean up expired modifiers
	 * @param Card - Card to clean
	 * @param GameState - Current game state
	 * @param bEndOfTurn - True if cleaning at end of turn
	 * @param bEndOfBattle - True if cleaning at end of battle
	 */
	UFUNCTION(BlueprintCallable, Category = "GCG|Effects|Modifiers")
	void CleanupExpiredModifiers(UPARAM(ref) FGCGCardInstance& Card, AGCGGameState* GameState,
		bool bEndOfTurn = false, bool bEndOfBattle = false);

	/**
	 * Clean up all modifiers for a player
	 * @param PlayerState - Player whose cards to clean
	 * @param GameState - Current game state
	 * @param bEndOfTurn - True if cleaning at end of turn
	 * @param bEndOfBattle - True if cleaning at end of battle
	 */
	UFUNCTION(BlueprintCallable, Category = "GCG|Effects|Modifiers")
	void CleanupAllModifiers(AGCGPlayerState* PlayerState, AGCGGameState* GameState,
		bool bEndOfTurn = false, bool bEndOfBattle = false);

	// ===========================================================================================
	// UTILITY
	// ===========================================================================================

	/**
	 * Get player by ID
	 * @param PlayerID - Player ID
	 * @param GameState - Current game state
	 * @return Player state
	 */
	UFUNCTION(BlueprintCallable, Category = "GCG|Effects")
	AGCGPlayerState* GetPlayerByID(int32 PlayerID, AGCGGameState* GameState);

	/**
	 * Get opponent player
	 * @param CurrentPlayerID - Current player ID
	 * @param GameState - Current game state
	 * @return Opponent player state
	 */
	UFUNCTION(BlueprintCallable, Category = "GCG|Effects")
	AGCGPlayerState* GetOpponentPlayer(int32 CurrentPlayerID, AGCGGameState* GameState);

	/**
	 * Log effect execution
	 */
	void LogEffect(const FString& EffectName, const FString& Message) const;

private:
	// ===========================================================================================
	// INTERNAL HELPERS
	// ===========================================================================================

	/**
	 * Resolve target for an operation
	 * @param TargetName - Target name ("Self", "TargetUnit", "OpponentPlayer", etc.)
	 * @param Context - Effect context
	 * @param SourcePlayer - Source player
	 * @param GameState - Game state
	 * @param OutPlayerState - Output player state
	 * @param OutCardInstanceID - Output card instance ID
	 * @return True if target resolved
	 */
	bool ResolveTarget(FName TargetName, const FGCGEffectContext& Context, AGCGPlayerState* SourcePlayer,
		AGCGGameState* GameState, AGCGPlayerState*& OutPlayerState, int32& OutCardInstanceID);
};
