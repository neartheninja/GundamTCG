// GCGKeywordSubsystem.h - Gundam Card Game Keyword Processing Subsystem
// Handles all keyword mechanics (Repair, Breach, Support, FirstStrike, etc.)

#pragma once

#include "CoreMinimal.h"
#include "Subsystems/GameInstanceSubsystem.h"
#include "OnePieceTCG_V2/GCGTypes.h"
#include "GCGKeywordSubsystem.generated.h"

// Forward declarations
class AGCGPlayerState;
class AGCGGameState;

/**
 * Keyword Processing Result
 * Returned by keyword processing functions to indicate success/failure and provide data
 */
USTRUCT(BlueprintType)
struct FGCGKeywordResult
{
	GENERATED_BODY()

	UPROPERTY(BlueprintReadOnly)
	bool bSuccess = false;

	UPROPERTY(BlueprintReadOnly)
	FText Message;

	// Result data
	UPROPERTY(BlueprintReadOnly)
	int32 DamageDealt = 0;

	UPROPERTY(BlueprintReadOnly)
	int32 HealingDone = 0;

	UPROPERTY(BlueprintReadOnly)
	int32 ShieldsBroken = 0;

	UPROPERTY(BlueprintReadOnly)
	int32 APBuff = 0;

	UPROPERTY(BlueprintReadOnly)
	bool bEvaded = false;

	UPROPERTY(BlueprintReadOnly)
	bool bFirstStrikeDamage = false;

	FGCGKeywordResult()
		: bSuccess(false), Message(FText::GetEmpty()), DamageDealt(0), HealingDone(0),
		  ShieldsBroken(0), APBuff(0), bEvaded(false), bFirstStrikeDamage(false)
	{
	}
};

/**
 * UGCGKeywordSubsystem
 *
 * Game Instance Subsystem that handles all keyword processing for Gundam TCG.
 *
 * Keyword Categories:
 * - Combat Keywords: FirstStrike, HighManeuver, Blocker, Suppression, Breach
 * - Stat Keywords: Support (buff allies), Repair (healing)
 * - Special: Burst (shield triggers), LinkUnit (bypass summoning sickness)
 *
 * Phase 7 Implementation:
 * All keywords implemented with proper stacking rules and integration with combat/turn systems.
 */
UCLASS()
class GUNDAMTCG_API UGCGKeywordSubsystem : public UGameInstanceSubsystem
{
	GENERATED_BODY()

public:
	// ===========================================================================================
	// SUBSYSTEM LIFECYCLE
	// ===========================================================================================

	virtual void Initialize(FSubsystemCollectionBase& Collection) override;
	virtual void Deinitialize() override;

	// ===========================================================================================
	// KEYWORD CHECKS
	// ===========================================================================================

	/**
	 * Check if a card has a specific keyword
	 * @param Card - Card instance to check
	 * @param Keyword - Keyword to check for
	 * @return True if card has the keyword
	 */
	UFUNCTION(BlueprintCallable, Category = "GCG|Keywords")
	bool HasKeyword(const FGCGCardInstance& Card, EGCGKeyword Keyword) const;

	/**
	 * Get the total value of a keyword on a card (e.g., Repair 2 + Repair 1 = 3)
	 * @param Card - Card instance to check
	 * @param Keyword - Keyword to get value for
	 * @return Total keyword value (sum of all instances)
	 */
	UFUNCTION(BlueprintCallable, Category = "GCG|Keywords")
	int32 GetKeywordValue(const FGCGCardInstance& Card, EGCGKeyword Keyword) const;

	/**
	 * Check if keyword stacks (Repair, Breach, Support stack; FirstStrike, Blocker don't)
	 * @param Keyword - Keyword to check
	 * @return True if keyword can have multiple instances
	 */
	UFUNCTION(BlueprintPure, Category = "GCG|Keywords")
	bool DoesKeywordStack(EGCGKeyword Keyword) const;

	// ===========================================================================================
	// REPAIR KEYWORD (Heal at end of turn)
	// ===========================================================================================

	/**
	 * Process Repair keyword for a single card
	 * Recovers X damage at end of turn
	 * @param Card - Card instance with Repair keyword
	 * @return Keyword result with healing done
	 */
	UFUNCTION(BlueprintCallable, Category = "GCG|Keywords|Repair")
	FGCGKeywordResult ProcessRepair(UPARAM(ref) FGCGCardInstance& Card);

	/**
	 * Process Repair for all Units in Battle Area at end of turn
	 * @param PlayerState - Player whose units to process
	 * @return Total healing done across all units
	 */
	UFUNCTION(BlueprintCallable, Category = "GCG|Keywords|Repair")
	int32 ProcessRepairForPlayer(AGCGPlayerState* PlayerState);

	// ===========================================================================================
	// BREACH KEYWORD (Extra shield damage when destroying Unit)
	// ===========================================================================================

	/**
	 * Process Breach keyword when a Unit destroys another Unit
	 * Deals X damage to defending player's shields
	 * @param Attacker - Attacking card with Breach keyword
	 * @param DefendingPlayer - Player whose shields to damage
	 * @param GameState - Current game state
	 * @return Keyword result with shields broken
	 */
	UFUNCTION(BlueprintCallable, Category = "GCG|Keywords|Breach")
	FGCGKeywordResult ProcessBreach(const FGCGCardInstance& Attacker, AGCGPlayerState* DefendingPlayer, AGCGGameState* GameState);

	// ===========================================================================================
	// SUPPORT KEYWORD (Buff allies)
	// ===========================================================================================

	/**
	 * Calculate total Support buff for a Unit from allies
	 * Support X grants +X AP to all friendly Units
	 * @param Unit - Unit to calculate buff for
	 * @param PlayerState - Player who owns the Unit
	 * @return Total AP buff from Support
	 */
	UFUNCTION(BlueprintCallable, Category = "GCG|Keywords|Support")
	int32 CalculateSupportBuff(const FGCGCardInstance& Unit, AGCGPlayerState* PlayerState);

	/**
	 * Get all Units providing Support in Battle Area
	 * @param PlayerState - Player whose units to check
	 * @return Array of units with Support keyword
	 */
	UFUNCTION(BlueprintCallable, Category = "GCG|Keywords|Support")
	TArray<FGCGCardInstance> GetUnitsWithSupport(AGCGPlayerState* PlayerState);

	// ===========================================================================================
	// FIRST STRIKE KEYWORD (Deal damage first in combat)
	// ===========================================================================================

	/**
	 * Check if attacker has First Strike advantage
	 * @param Attacker - Attacking card
	 * @param Defender - Defending card
	 * @return True if attacker deals damage first (has FirstStrike and defender doesn't)
	 */
	UFUNCTION(BlueprintCallable, Category = "GCG|Keywords|FirstStrike")
	bool HasFirstStrikeAdvantage(const FGCGCardInstance& Attacker, const FGCGCardInstance& Defender) const;

	/**
	 * Process First Strike damage in combat
	 * If attacker has FirstStrike advantage, deals damage first and may prevent retaliation
	 * @param Attacker - Attacking card
	 * @param Defender - Defending card
	 * @param OutDefenderDestroyed - True if defender was destroyed
	 * @return Keyword result with damage dealt
	 */
	UFUNCTION(BlueprintCallable, Category = "GCG|Keywords|FirstStrike")
	FGCGKeywordResult ProcessFirstStrike(const FGCGCardInstance& Attacker, UPARAM(ref) FGCGCardInstance& Defender, bool& OutDefenderDestroyed);

	// ===========================================================================================
	// HIGH-MANEUVER KEYWORD (Evasion mechanic)
	// ===========================================================================================

	/**
	 * Check if Unit evades attack using High-Maneuver
	 * Requires payment of 1 active resource to evade
	 * @param Defender - Defending Unit with High-Maneuver
	 * @param PlayerState - Player who owns the Unit
	 * @return True if evasion successful (has keyword and paid cost)
	 */
	UFUNCTION(BlueprintCallable, Category = "GCG|Keywords|HighManeuver")
	bool CanEvadeWithHighManeuver(const FGCGCardInstance& Defender, AGCGPlayerState* PlayerState) const;

	/**
	 * Process High-Maneuver evasion
	 * Rests 1 resource to evade attack
	 * @param Defender - Defending Unit
	 * @param PlayerState - Player who owns the Unit
	 * @return Keyword result with evasion success
	 */
	UFUNCTION(BlueprintCallable, Category = "GCG|Keywords|HighManeuver")
	FGCGKeywordResult ProcessHighManeuver(const FGCGCardInstance& Defender, AGCGPlayerState* PlayerState);

	// ===========================================================================================
	// SUPPRESSION KEYWORD (Destroy multiple shields simultaneously)
	// ===========================================================================================

	/**
	 * Process Suppression keyword when dealing player damage
	 * Destroys all shields in a single instance instead of one at a time
	 * @param Attacker - Attacking card with Suppression
	 * @param DefendingPlayer - Player whose shields to destroy
	 * @param GameState - Current game state
	 * @return Keyword result with shields broken and damage to base
	 */
	UFUNCTION(BlueprintCallable, Category = "GCG|Keywords|Suppression")
	FGCGKeywordResult ProcessSuppression(const FGCGCardInstance& Attacker, AGCGPlayerState* DefendingPlayer, AGCGGameState* GameState);

	// ===========================================================================================
	// BURST KEYWORD (Shield trigger effects)
	// ===========================================================================================

	/**
	 * Check if a shield card has Burst keyword
	 * @param Card - Shield card to check
	 * @return True if card has Burst
	 */
	UFUNCTION(BlueprintCallable, Category = "GCG|Keywords|Burst")
	bool HasBurst(const FGCGCardInstance& Card) const;

	/**
	 * Process Burst trigger when shield is broken
	 * Returns card to hand or triggers effect (Phase 8 integration)
	 * @param ShieldCard - Shield card with Burst
	 * @param PlayerState - Player who owns the shield
	 * @param GameState - Current game state
	 * @return Keyword result with Burst effect data
	 */
	UFUNCTION(BlueprintCallable, Category = "GCG|Keywords|Burst")
	FGCGKeywordResult ProcessBurst(const FGCGCardInstance& ShieldCard, AGCGPlayerState* PlayerState, AGCGGameState* GameState);

	// ===========================================================================================
	// LINK UNIT KEYWORD (Bypass summoning sickness when paired with Pilot)
	// ===========================================================================================

	/**
	 * Check if a Unit is a Link Unit
	 * @param Card - Card to check
	 * @return True if card has LinkUnit keyword
	 */
	UFUNCTION(BlueprintCallable, Category = "GCG|Keywords|LinkUnit")
	bool IsLinkUnit(const FGCGCardInstance& Card) const;

	/**
	 * Check if a Link Unit is currently paired with a Pilot
	 * @param LinkUnit - Link Unit card
	 * @param PlayerState - Player who owns the card
	 * @return True if paired with a Pilot
	 */
	UFUNCTION(BlueprintCallable, Category = "GCG|Keywords|LinkUnit")
	bool IsPairedWithPilot(const FGCGCardInstance& LinkUnit, AGCGPlayerState* PlayerState) const;

	/**
	 * Check if Link Unit can attack this turn (paired with Pilot = bypass summoning sickness)
	 * @param LinkUnit - Link Unit card
	 * @param PlayerState - Player who owns the card
	 * @param GameState - Current game state
	 * @return True if can attack (paired or not deployed this turn)
	 */
	UFUNCTION(BlueprintCallable, Category = "GCG|Keywords|LinkUnit")
	bool CanLinkUnitAttack(const FGCGCardInstance& LinkUnit, AGCGPlayerState* PlayerState, AGCGGameState* GameState) const;

	// ===========================================================================================
	// KEYWORD UTILITY
	// ===========================================================================================

	/**
	 * Get human-readable name for keyword
	 * @param Keyword - Keyword enum
	 * @return Display name
	 */
	UFUNCTION(BlueprintPure, Category = "GCG|Keywords")
	FString GetKeywordName(EGCGKeyword Keyword) const;

	/**
	 * Get description for keyword with value
	 * @param Keyword - Keyword enum
	 * @param Value - Keyword value (X in "Repair X")
	 * @return Formatted description (e.g., "Repair 2: Recover 2 damage at end of turn")
	 */
	UFUNCTION(BlueprintPure, Category = "GCG|Keywords")
	FString GetKeywordDescription(EGCGKeyword Keyword, int32 Value) const;

	// ===========================================================================================
	// BLOCKER KEYWORD (Handled in Combat Subsystem - Helper here)
	// ===========================================================================================

	/**
	 * Check if a Unit has Blocker keyword
	 * Note: Blocker mechanics are implemented in GCGCombatSubsystem
	 * @param Card - Card to check
	 * @return True if card has Blocker
	 */
	UFUNCTION(BlueprintCallable, Category = "GCG|Keywords|Blocker")
	bool HasBlocker(const FGCGCardInstance& Card) const;

private:
	// ===========================================================================================
	// INTERNAL HELPERS
	// ===========================================================================================

	/**
	 * Apply healing to a card (reduce damage)
	 * @param Card - Card to heal
	 * @param Amount - Amount of healing
	 * @return Actual healing done
	 */
	int32 ApplyHealing(FGCGCardInstance& Card, int32 Amount);

	/**
	 * Break shields using Combat Subsystem
	 * @param Count - Number of shields to break
	 * @param PlayerState - Player whose shields to break
	 * @return Actual shields broken
	 */
	int32 BreakShields(int32 Count, AGCGPlayerState* PlayerState);

	/**
	 * Log keyword processing
	 */
	void LogKeyword(const FString& KeywordName, const FString& Message) const;
};
