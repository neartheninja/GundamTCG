// GCGEffectStackSubsystem.h - Effect Stack & Priority Resolution System
// Unreal Engine 5.6 - Gundam TCG Implementation
// Implements FAQ Q105-Q112: Effect resolution order and priority

#pragma once

#include "CoreMinimal.h"
#include "Subsystems/GameInstanceSubsystem.h"
#include "OnePieceTCG_V2/GCGTypes.h"
#include "GCGEffectStackSubsystem.generated.h"

// Forward declarations
class AGCGGameState;
class AGCGPlayerState;

/**
 * Effect Priority Level (FAQ Q110, Q112)
 */
UENUM(BlueprintType)
enum class EGCGEffectPriority : uint8
{
	Normal          = 0     UMETA(DisplayName = "Normal"),          // Standard effects
	Trigger         = 10    UMETA(DisplayName = "Trigger"),         // Triggered effects (OnDeploy, OnAttack, etc.)
	Burst           = 20    UMETA(DisplayName = "Burst"),           // Burst effects from shields (Q110)
	Negation        = 30    UMETA(DisplayName = "Negation")         // Negation effects (Q112)
};

/**
 * Effect Stack Entry
 * Represents a single effect waiting to resolve
 */
USTRUCT(BlueprintType)
struct FGCGEffectStackEntry
{
	GENERATED_BODY()

	// Source card instance ID
	UPROPERTY(BlueprintReadOnly, Category = "Effect Stack")
	int32 SourceCardInstanceID;

	// Owner player ID
	UPROPERTY(BlueprintReadOnly, Category = "Effect Stack")
	int32 OwnerPlayerID;

	// Effect data
	UPROPERTY(BlueprintReadOnly, Category = "Effect Stack")
	FGCGEffectData EffectData;

	// Priority level (higher = resolves first)
	UPROPERTY(BlueprintReadOnly, Category = "Effect Stack")
	EGCGEffectPriority Priority;

	// Stack index (for ordering within same priority)
	UPROPERTY(BlueprintReadOnly, Category = "Effect Stack")
	int32 StackIndex;

	// Has this effect been resolved?
	UPROPERTY(BlueprintReadOnly, Category = "Effect Stack")
	bool bResolved;

	// Snapshot of Units affected (FAQ Q105: continuous effects only affect Units in play at activation)
	UPROPERTY(BlueprintReadOnly, Category = "Effect Stack")
	TArray<int32> AffectedUnitInstanceIDs;

	// Timestamp when added to stack
	UPROPERTY(BlueprintReadOnly, Category = "Effect Stack")
	float Timestamp;

	// Constructor
	FGCGEffectStackEntry()
	{
		SourceCardInstanceID = -1;
		OwnerPlayerID = -1;
		Priority = EGCGEffectPriority::Normal;
		StackIndex = 0;
		bResolved = false;
		Timestamp = 0.0f;
	}
};

/**
 * Effect Stack Subsystem
 *
 * Manages effect resolution order and priority according to FAQ Q105-Q112:
 * - Q105: Continuous effects only affect Units in play at activation
 * - Q106: "During this turn" effects persist even if source destroyed
 * - Q107-Q108: Active player resolves effects first, chooses order
 * - Q109: New effects interrupt and resolve first
 * - Q110: Burst effects get priority
 * - Q111: Effects resolve even if source leaves field
 * - Q112: Negation effects have priority
 */
UCLASS()
class GUNDAMTCG_API UGCGEffectStackSubsystem : public UGameInstanceSubsystem
{
	GENERATED_BODY()

public:
	// ===========================================================================================
	// INITIALIZATION
	// ===========================================================================================

	virtual void Initialize(FSubsystemCollectionBase& Collection) override;
	virtual void Deinitialize() override;

	// ===========================================================================================
	// STACK MANAGEMENT
	// ===========================================================================================

	/**
	 * Push effect onto stack
	 * @param SourceCardInstanceID - Card that triggered the effect
	 * @param OwnerPlayerID - Player who owns the effect
	 * @param EffectData - Effect data to execute
	 * @param Priority - Priority level (Normal, Trigger, Burst, Negation)
	 * @param AffectedUnits - Snapshot of affected Units (for Q105)
	 * @return Stack entry that was added
	 */
	UFUNCTION(BlueprintCallable, Category = "Effect Stack")
	FGCGEffectStackEntry PushEffect(
		int32 SourceCardInstanceID,
		int32 OwnerPlayerID,
		const FGCGEffectData& EffectData,
		EGCGEffectPriority Priority = EGCGEffectPriority::Normal,
		const TArray<int32>& AffectedUnits = TArray<int32>()
	);

	/**
	 * Pop top effect from stack (LIFO: Last In, First Out)
	 * @return Top effect entry, or invalid entry if stack empty
	 */
	UFUNCTION(BlueprintCallable, Category = "Effect Stack")
	FGCGEffectStackEntry PopEffect();

	/**
	 * Peek at top effect without removing it
	 */
	UFUNCTION(BlueprintPure, Category = "Effect Stack")
	FGCGEffectStackEntry PeekTopEffect() const;

	/**
	 * Check if stack is empty
	 */
	UFUNCTION(BlueprintPure, Category = "Effect Stack")
	bool IsStackEmpty() const;

	/**
	 * Get stack size
	 */
	UFUNCTION(BlueprintPure, Category = "Effect Stack")
	int32 GetStackSize() const;

	/**
	 * Clear all effects from stack
	 */
	UFUNCTION(BlueprintCallable, Category = "Effect Stack")
	void ClearStack();

	// ===========================================================================================
	// RESOLUTION
	// ===========================================================================================

	/**
	 * Resolve the entire stack
	 * FAQ Q107-Q109: Active player resolves first, new effects interrupt
	 * @param GameState - Current game state
	 * @return True if all effects resolved successfully
	 */
	UFUNCTION(BlueprintCallable, Category = "Effect Stack")
	bool ResolveStack(AGCGGameState* GameState);

	/**
	 * Resolve single effect from top of stack
	 * FAQ Q111: Effects resolve even if source leaves field
	 * @param GameState - Current game state
	 * @return True if effect resolved successfully
	 */
	UFUNCTION(BlueprintCallable, Category = "Effect Stack")
	bool ResolveSingleEffect(AGCGGameState* GameState);

	/**
	 * Sort stack by priority (FAQ Q110, Q112: Burst and Negation get priority)
	 * Higher priority effects move to top of stack
	 */
	UFUNCTION(BlueprintCallable, Category = "Effect Stack")
	void SortStackByPriority();

	/**
	 * Group effects by player for resolution order (FAQ Q107-Q108)
	 * @param ActivePlayerID - Currently active player
	 * @return Effects grouped by player, active player first
	 */
	UFUNCTION(BlueprintCallable, Category = "Effect Stack")
	TMap<int32, TArray<FGCGEffectStackEntry>> GroupEffectsByPlayer(int32 ActivePlayerID);

	// ===========================================================================================
	// PRIORITY HANDLING
	// ===========================================================================================

	/**
	 * Get priority for effect based on timing
	 * FAQ Q110: Burst effects get high priority
	 * @param EffectData - Effect to check
	 * @return Priority level
	 */
	UFUNCTION(BlueprintPure, Category = "Effect Stack")
	static EGCGEffectPriority GetEffectPriority(const FGCGEffectData& EffectData);

	/**
	 * Check if effect is a negation effect (FAQ Q112)
	 * @param EffectData - Effect to check
	 * @return True if this is a negation effect
	 */
	UFUNCTION(BlueprintPure, Category = "Effect Stack")
	static bool IsNegationEffect(const FGCGEffectData& EffectData);

	/**
	 * Check if effect is a continuous effect (FAQ Q105)
	 * Continuous effects only affect Units in play at time of activation
	 * @param EffectData - Effect to check
	 * @return True if this is a continuous effect
	 */
	UFUNCTION(BlueprintPure, Category = "Effect Stack")
	static bool IsContinuousEffect(const FGCGEffectData& EffectData);

	// ===========================================================================================
	// SNAPSHOT MANAGEMENT (FAQ Q105)
	// ===========================================================================================

	/**
	 * Take snapshot of affected Units (FAQ Q105)
	 * Continuous effects only affect Units that were in play at activation
	 * @param EffectData - Effect being activated
	 * @param GameState - Current game state
	 * @return Array of affected Unit instance IDs
	 */
	UFUNCTION(BlueprintCallable, Category = "Effect Stack")
	TArray<int32> TakeUnitSnapshot(const FGCGEffectData& EffectData, AGCGGameState* GameState);

	/**
	 * Check if Unit is in snapshot (still valid target)
	 * @param UnitInstanceID - Unit to check
	 * @param EffectEntry - Effect entry with snapshot
	 * @return True if Unit is in snapshot
	 */
	UFUNCTION(BlueprintPure, Category = "Effect Stack")
	static bool IsUnitInSnapshot(int32 UnitInstanceID, const FGCGEffectStackEntry& EffectEntry);

	// ===========================================================================================
	// DURATION TRACKING (FAQ Q106)
	// ===========================================================================================

	/**
	 * Track "during this turn" effect (FAQ Q106)
	 * These effects persist even if source is destroyed
	 * @param EffectEntry - Effect entry
	 * @param TurnNumber - Current turn number
	 */
	UFUNCTION(BlueprintCallable, Category = "Effect Stack")
	void TrackDuringThisTurnEffect(const FGCGEffectStackEntry& EffectEntry, int32 TurnNumber);

	/**
	 * Clean up expired "during this turn" effects
	 * @param TurnNumber - Current turn number
	 */
	UFUNCTION(BlueprintCallable, Category = "Effect Stack")
	void CleanupExpiredTurnEffects(int32 TurnNumber);

	// ===========================================================================================
	// DEBUG
	// ===========================================================================================

	/**
	 * Print stack contents (for debugging)
	 */
	UFUNCTION(BlueprintCallable, Category = "Effect Stack|Debug")
	void PrintStack() const;

	/**
	 * Get stack as array (for UI display)
	 */
	UFUNCTION(BlueprintPure, Category = "Effect Stack")
	TArray<FGCGEffectStackEntry> GetStackAsArray() const;

private:
	// ===========================================================================================
	// INTERNAL DATA
	// ===========================================================================================

	// Effect stack (LIFO: Last In, First Out)
	UPROPERTY()
	TArray<FGCGEffectStackEntry> EffectStack;

	// Stack index counter (for ordering effects added at same time)
	int32 StackIndexCounter;

	// "During this turn" effects that persist (FAQ Q106)
	// Map: TurnNumber -> Array of effect entries
	UPROPERTY()
	TMap<int32, TArray<FGCGEffectStackEntry>> DuringThisTurnEffects;

	// ===========================================================================================
	// INTERNAL HELPERS
	// ===========================================================================================

	/**
	 * Compare two effect entries for sorting
	 * @return True if A should resolve before B
	 */
	static bool CompareEffectPriority(const FGCGEffectStackEntry& A, const FGCGEffectStackEntry& B);

	/**
	 * Execute effect operation
	 * @param EffectEntry - Effect to execute
	 * @param GameState - Game state
	 * @return True if executed successfully
	 */
	bool ExecuteEffectInternal(const FGCGEffectStackEntry& EffectEntry, AGCGGameState* GameState);
};
