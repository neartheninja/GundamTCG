// GCGCombatSubsystem.h - Combat System Subsystem
// Unreal Engine 5.6 - Gundam TCG Implementation
// Handles combat flow: attack declaration, blocking, damage calculation

#pragma once

#include "CoreMinimal.h"
#include "Subsystems/GameInstanceSubsystem.h"
#include "GundamTCG/GCGTypes.h"
#include "GCGCombatSubsystem.generated.h"

// Forward declarations
class AGCGPlayerState;
class AGCGGameState;
class UGCGZoneSubsystem;

/**
 * Combat Attack Declaration
 * Contains information about a declared attack
 */
USTRUCT(BlueprintType)
struct FGCGAttackDeclaration
{
	GENERATED_BODY()

	/** The attacking unit instance ID */
	UPROPERTY(BlueprintReadWrite)
	int32 AttackerInstanceID;

	/** The player declaring the attack */
	UPROPERTY(BlueprintReadWrite)
	int32 AttackingPlayerID;

	/** The target player (defender) */
	UPROPERTY(BlueprintReadWrite)
	int32 DefendingPlayerID;

	/** Is this attack targeting the Base directly? */
	UPROPERTY(BlueprintReadWrite)
	bool bTargetingBase;

	/** Blocker instance ID (0 if unblocked) */
	UPROPERTY(BlueprintReadWrite)
	int32 BlockerInstanceID;

	/** Has this attack been resolved? */
	UPROPERTY(BlueprintReadWrite)
	bool bResolved;

	FGCGAttackDeclaration()
		: AttackerInstanceID(0)
		, AttackingPlayerID(-1)
		, DefendingPlayerID(-1)
		, bTargetingBase(true)
		, BlockerInstanceID(0)
		, bResolved(false)
	{}
};

/**
 * Combat Result
 * Contains information about combat resolution
 */
USTRUCT(BlueprintType)
struct FGCGCombatResult
{
	GENERATED_BODY()

	/** Did the combat succeed? */
	UPROPERTY(BlueprintReadOnly)
	bool bSuccess;

	/** Error message if combat failed */
	UPROPERTY(BlueprintReadOnly)
	FString ErrorMessage;

	/** Damage dealt to defender */
	UPROPERTY(BlueprintReadOnly)
	int32 DamageDealt;

	/** Shields broken */
	UPROPERTY(BlueprintReadOnly)
	int32 ShieldsBroken;

	/** Was attacker destroyed? */
	UPROPERTY(BlueprintReadOnly)
	bool bAttackerDestroyed;

	/** Was blocker destroyed? */
	UPROPERTY(BlueprintReadOnly)
	bool bBlockerDestroyed;

	FGCGCombatResult()
		: bSuccess(false)
		, ErrorMessage(TEXT(""))
		, DamageDealt(0)
		, ShieldsBroken(0)
		, bAttackerDestroyed(false)
		, bBlockerDestroyed(false)
	{}

	FGCGCombatResult(bool bInSuccess, const FString& InErrorMessage = TEXT(""))
		: bSuccess(bInSuccess)
		, ErrorMessage(InErrorMessage)
		, DamageDealt(0)
		, ShieldsBroken(0)
		, bAttackerDestroyed(false)
		, bBlockerDestroyed(false)
	{}
};

/**
 * Combat Subsystem
 *
 * This subsystem handles all combat-related operations:
 * - Attack declaration
 * - Blocker declaration
 * - Damage calculation
 * - Shield breaking
 * - Base damage
 * - Combat resolution
 *
 * Combat Flow:
 * 1. Attack Step: Declare attackers
 * 2. Block Step: Declare blockers
 * 3. Action Step: Play Action timing cards/abilities
 * 4. Damage Step: Calculate and deal damage
 * 5. Battle End Step: Trigger end-of-battle effects
 */
UCLASS()
class GUNDAMTCG_API UGCGCombatSubsystem : public UGameInstanceSubsystem
{
	GENERATED_BODY()

public:
	// ===== SUBSYSTEM LIFECYCLE =====

	virtual void Initialize(FSubsystemCollectionBase& Collection) override;
	virtual void Deinitialize() override;

	// ===== ATTACK DECLARATION =====

	/**
	 * Declare an attack
	 * @param AttackerInstanceID The attacking unit
	 * @param AttackingPlayer The player declaring the attack
	 * @param DefendingPlayer The defending player
	 * @param GameState The current game state
	 * @return Combat result
	 */
	UFUNCTION(BlueprintCallable, Category = "Combat")
	FGCGCombatResult DeclareAttack(int32 AttackerInstanceID,
		AGCGPlayerState* AttackingPlayer, AGCGPlayerState* DefendingPlayer,
		AGCGGameState* GameState);

	/**
	 * Can this unit attack?
	 * @param AttackerInstance The unit to check
	 * @param AttackingPlayer The player who would attack
	 * @param GameState The current game state
	 * @return Combat result with validation
	 */
	UFUNCTION(BlueprintPure, Category = "Combat")
	FGCGCombatResult CanAttack(const FGCGCardInstance& AttackerInstance,
		AGCGPlayerState* AttackingPlayer, AGCGGameState* GameState) const;

	// ===== BLOCKER DECLARATION =====

	/**
	 * Declare a blocker for an attack
	 * @param AttackIndex The index of the attack in CurrentAttacks
	 * @param BlockerInstanceID The blocking unit
	 * @param DefendingPlayer The defending player
	 * @param GameState The current game state
	 * @return Combat result
	 */
	UFUNCTION(BlueprintCallable, Category = "Combat")
	FGCGCombatResult DeclareBlocker(int32 AttackIndex, int32 BlockerInstanceID,
		AGCGPlayerState* DefendingPlayer, AGCGGameState* GameState);

	/**
	 * Can this unit block the given attack?
	 * @param BlockerInstance The unit to check
	 * @param Attack The attack to block
	 * @param DefendingPlayer The defending player
	 * @return Combat result with validation
	 */
	UFUNCTION(BlueprintPure, Category = "Combat")
	FGCGCombatResult CanBlock(const FGCGCardInstance& BlockerInstance,
		const FGCGAttackDeclaration& Attack, AGCGPlayerState* DefendingPlayer) const;

	// ===== DAMAGE CALCULATION =====

	/**
	 * Resolve a single attack (calculate and deal damage)
	 * @param Attack The attack to resolve
	 * @param AttackingPlayer The attacking player
	 * @param DefendingPlayer The defending player
	 * @param GameState The current game state
	 * @return Combat result
	 */
	UFUNCTION(BlueprintCallable, Category = "Combat")
	FGCGCombatResult ResolveAttack(FGCGAttackDeclaration& Attack,
		AGCGPlayerState* AttackingPlayer, AGCGPlayerState* DefendingPlayer,
		AGCGGameState* GameState);

	/**
	 * Deal damage to a unit
	 * @param TargetInstanceID The unit to damage
	 * @param Damage Amount of damage
	 * @param PlayerState The player who owns the unit
	 * @return True if unit was destroyed
	 */
	UFUNCTION(BlueprintCallable, Category = "Combat")
	bool DealDamageToUnit(int32 TargetInstanceID, int32 Damage, AGCGPlayerState* PlayerState);

	/**
	 * Deal damage to a player (breaks shields then damages base)
	 * @param Damage Amount of damage
	 * @param DefendingPlayer The player taking damage
	 * @param GameState The current game state
	 * @param OutShieldsBroken Number of shields broken
	 * @return True if player lost the game
	 */
	UFUNCTION(BlueprintCallable, Category = "Combat")
	bool DealDamageToPlayer(int32 Damage, AGCGPlayerState* DefendingPlayer,
		AGCGGameState* GameState, int32& OutShieldsBroken);

	// ===== SHIELD SYSTEM =====

	/**
	 * Break shields (remove from shield stack)
	 * @param Count Number of shields to break
	 * @param DefendingPlayer The player losing shields
	 * @return Number of shields actually broken
	 */
	UFUNCTION(BlueprintCallable, Category = "Combat")
	int32 BreakShields(int32 Count, AGCGPlayerState* DefendingPlayer);

	/**
	 * Check if player has shields remaining
	 * @param PlayerState The player to check
	 * @return True if player has shields
	 */
	UFUNCTION(BlueprintPure, Category = "Combat")
	bool HasShields(AGCGPlayerState* PlayerState) const;

	// ===== COMBAT RESOLUTION =====

	/**
	 * Resolve all declared attacks
	 * @param AttackingPlayer The attacking player
	 * @param DefendingPlayer The defending player
	 * @param GameState The current game state
	 * @return Total combat results
	 */
	UFUNCTION(BlueprintCallable, Category = "Combat")
	FGCGCombatResult ResolveAllAttacks(AGCGPlayerState* AttackingPlayer,
		AGCGPlayerState* DefendingPlayer, AGCGGameState* GameState);

	/**
	 * Clear all declared attacks (end of combat)
	 * @param GameState The current game state
	 */
	UFUNCTION(BlueprintCallable, Category = "Combat")
	void ClearAttacks(AGCGGameState* GameState);

protected:
	// ===== INTERNAL HELPERS =====

	/**
	 * Check if unit has summoning sickness
	 * @param CardInstance The unit to check
	 * @param GameState The current game state
	 * @return True if unit was deployed this turn
	 */
	bool HasSummoningSickness(const FGCGCardInstance& CardInstance, AGCGGameState* GameState) const;

	/**
	 * Check if unit has a keyword
	 * @param CardInstance The unit to check
	 * @param Keyword The keyword to look for
	 * @return True if unit has the keyword
	 */
	bool HasKeyword(const FGCGCardInstance& CardInstance, EGCGKeyword Keyword) const;

	/**
	 * Get keyword value (for keywords with values like Breach(2))
	 * @param CardInstance The unit to check
	 * @param Keyword The keyword to get value for
	 * @return Keyword value (0 if not found)
	 */
	int32 GetKeywordValue(const FGCGCardInstance& CardInstance, EGCGKeyword Keyword) const;

	/**
	 * Destroy a unit (move to trash)
	 * @param TargetInstanceID The unit to destroy
	 * @param PlayerState The player who owns the unit
	 * @return True if unit was destroyed
	 */
	bool DestroyUnit(int32 TargetInstanceID, AGCGPlayerState* PlayerState);

private:
	/**
	 * Get zone subsystem
	 */
	UGCGZoneSubsystem* GetZoneSubsystem() const;
};
