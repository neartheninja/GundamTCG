// GCGComprehensiveRulesSubsystem.h - Comprehensive Rules Validation
// Unreal Engine 5.6 - Gundam TCG Implementation
// Modular subsystem for validating comprehensive rules without modifying core types

#pragma once

#include "CoreMinimal.h"
#include "Subsystems/GameInstanceSubsystem.h"
#include "GundamTCG/GCGTypes.h"
#include "GCGComprehensiveRulesSubsystem.generated.h"

// Forward declarations
class AGCGPlayerState;
class AGCGGameState;
class UGCGCardDatabase;

/**
 * Result of a comprehensive rules validation check
 */
USTRUCT(BlueprintType)
struct FGCGRulesValidationResult
{
	GENERATED_BODY()

	/** Was the rule check successful? */
	UPROPERTY(BlueprintReadOnly, Category = "Validation")
	bool bIsValid;

	/** Rule number that was checked (e.g., "2-4-3" for deck color restriction) */
	UPROPERTY(BlueprintReadOnly, Category = "Validation")
	FString RuleNumber;

	/** Human-readable error message if validation failed */
	UPROPERTY(BlueprintReadOnly, Category = "Validation")
	FString ErrorMessage;

	/** Additional context or warnings */
	UPROPERTY(BlueprintReadOnly, Category = "Validation")
	TArray<FString> Warnings;

	FGCGRulesValidationResult()
		: bIsValid(true)
		, RuleNumber(TEXT(""))
		, ErrorMessage(TEXT(""))
	{}

	FGCGRulesValidationResult(bool bValid, const FString& InRuleNumber, const FString& InError = TEXT(""))
		: bIsValid(bValid)
		, RuleNumber(InRuleNumber)
		, ErrorMessage(InError)
	{}

	void AddWarning(const FString& Warning)
	{
		Warnings.Add(Warning);
	}
};

/**
 * Comprehensive Rules Subsystem
 *
 * PURPOSE:
 * Provides modular validation for Gundam TCG Comprehensive Rules Sections 1-13.
 * This subsystem allows rules to be validated WITHOUT modifying core types in GCGTypes.h.
 *
 * DESIGN PHILOSOPHY:
 * - Comprehensive rules are documented in Documentation/ComprehensiveRules/
 * - Validation logic lives in this subsystem (not scattered across multiple files)
 * - Core types (GCGTypes.h) remain stable - no breaking changes
 * - Each rule has a dedicated validation method with clear rule number references
 *
 * USAGE:
 * Other subsystems can call validation methods to check specific rules:
 *
 * Example:
 *   UGCGComprehensiveRulesSubsystem* RulesSubsystem = GetGameInstance()->GetSubsystem<UGCGComprehensiveRulesSubsystem>();
 *   FGCGRulesValidationResult Result = RulesSubsystem->ValidateRule_2_4_3_DeckColors(DeckList);
 *   if (!Result.bIsValid)
 *   {
 *       UE_LOG(LogTemp, Error, TEXT("Rule %s violation: %s"), *Result.RuleNumber, *Result.ErrorMessage);
 *   }
 *
 * INTEGRATION STATUS:
 * - Section 1 (Game Overview): Integrated directly into GameMode/Combat subsystems
 * - Section 2 (Card Information): Integrated into GCGTypes.h (with breaking color change)
 * - Sections 3-13: Use this subsystem for modular validation
 */
UCLASS()
class GUNDAMTCG_API UGCGComprehensiveRulesSubsystem : public UGameInstanceSubsystem
{
	GENERATED_BODY()

public:
	// ===== SUBSYSTEM LIFECYCLE =====

	virtual void Initialize(FSubsystemCollectionBase& Collection) override;
	virtual void Deinitialize() override;

	// ===== SECTION 2: CARD INFORMATION =====
	// Note: Most of Section 2 is enforced structurally in GCGTypes.h
	// These methods provide additional runtime validation

	/**
	 * Rule 2-1-2: Validate deck has max 4 copies of each card
	 * @param CardNumbers Array of card numbers in deck
	 * @return Validation result
	 */
	UFUNCTION(BlueprintCallable, Category = "Comprehensive Rules|Section 2")
	FGCGRulesValidationResult ValidateRule_2_1_2_MaxCopies(const TArray<FName>& CardNumbers) const;

	/**
	 * Rule 2-4-3: Validate deck has 1-2 colors maximum (excluding Colorless)
	 * @param DeckColors Array of all colors in the deck
	 * @return Validation result
	 */
	UFUNCTION(BlueprintCallable, Category = "Comprehensive Rules|Section 2")
	FGCGRulesValidationResult ValidateRule_2_4_3_DeckColors(const TArray<EGCGCardColor>& DeckColors) const;

	/**
	 * Rule 2-8-2: Check if card should be destroyed (HP <= 0)
	 * @param Card Card instance to check
	 * @return True if card should be destroyed
	 */
	UFUNCTION(BlueprintPure, Category = "Comprehensive Rules|Section 2")
	bool ValidateRule_2_8_2_CardDestruction(const FGCGCardInstance& Card) const;

	/**
	 * Rule 2-9-2: Calculate player's Lv (Active Resources + EX Resources)
	 * @param PlayerState Player to calculate Lv for
	 * @return Player's current Lv
	 */
	UFUNCTION(BlueprintPure, Category = "Comprehensive Rules|Section 2")
	int32 ValidateRule_2_9_2_PlayerLv(const AGCGPlayerState* PlayerState) const;

	// ===== SECTION 3: CARD TYPES =====

	/**
	 * Rule 3-2-3: Only Units can attack
	 * @param Card Card attempting to attack
	 * @return Validation result
	 */
	UFUNCTION(BlueprintCallable, Category = "Comprehensive Rules|Section 3")
	FGCGRulesValidationResult ValidateRule_3_2_3_OnlyUnitsAttack(const FGCGCardInstance& Card) const;

	/**
	 * Rule 3-2-4: Check if Unit has summoning sickness
	 * @param Unit Unit attempting to attack
	 * @param bDeployedThisTurn Was this Unit deployed this turn?
	 * @param bIsLinkUnit Is this a Link Unit?
	 * @return Validation result
	 */
	UFUNCTION(BlueprintCallable, Category = "Comprehensive Rules|Section 3")
	FGCGRulesValidationResult ValidateRule_3_2_4_SummoningSickness(
		const FGCGCardInstance& Unit,
		bool bDeployedThisTurn,
		bool bIsLinkUnit) const;

	/**
	 * Rule 3-2-6-4: Check if Pilot satisfies Unit's link condition
	 * Supports partial name matching with [xyz] syntax
	 * @param UnitLinkCondition Link condition from Unit card
	 * @param PilotName Name of Pilot card
	 * @param PilotTraits Traits of Pilot card
	 * @return True if link condition satisfied
	 */
	UFUNCTION(BlueprintCallable, Category = "Comprehensive Rules|Section 3")
	bool ValidateRule_3_2_6_4_LinkCondition(
		const FString& UnitLinkCondition,
		const FString& PilotName,
		const TArray<FName>& PilotTraits) const;

	/**
	 * Rule 3-3-3: Pilot can only exist in battle area if paired with Unit
	 * @param PlayerState Player attempting to play Pilot
	 * @param Pilot Pilot card being played
	 * @return Validation result
	 */
	UFUNCTION(BlueprintCallable, Category = "Comprehensive Rules|Section 3")
	FGCGRulesValidationResult ValidateRule_3_3_3_PilotRequiresUnit(
		const AGCGPlayerState* PlayerState,
		const FGCGCardInstance& Pilot) const;

	/**
	 * Rule 3-3-4: Unit can have at most one Pilot
	 * @param Unit Unit being paired with
	 * @param bAlreadyHasPilot Does this Unit already have a Pilot?
	 * @return Validation result
	 */
	UFUNCTION(BlueprintCallable, Category = "Comprehensive Rules|Section 3")
	FGCGRulesValidationResult ValidateRule_3_3_4_OnePilotPerUnit(
		const FGCGCardInstance& Unit,
		bool bAlreadyHasPilot) const;

	/**
	 * Rule 3-4-3: Commands have no location during effect resolution
	 * @return Special zone for Commands during resolution (EffectStack)
	 */
	UFUNCTION(BlueprintPure, Category = "Comprehensive Rules|Section 3")
	static EGCGCardZone GetCommandResolutionZone()
	{
		// Rule 3-4-3: Commands not in any specific location during resolution
		// Use EffectStack if it exists, otherwise use a placeholder
		return EGCGCardZone::Graveyard; // TODO: Add EffectStack zone
	}

	// ===== SECTION 4: GAME LOCATIONS =====

	/**
	 * Rule 4-1-1-2: Check if zone is part of "the field"
	 * Field = Resource Area + Battle Area + Shield Area
	 * @param Zone Zone to check
	 * @return True if zone is part of the field
	 */
	UFUNCTION(BlueprintPure, Category = "Comprehensive Rules|Section 4")
	static bool IsFieldZone(EGCGCardZone Zone)
	{
		// Rule 4-1-1-2: Field = Resource Area + Battle Area + Shield Area
		return Zone == EGCGCardZone::ResourceArea ||
		       Zone == EGCGCardZone::BattleArea ||
		       Zone == EGCGCardZone::BaseSection ||
		       Zone == EGCGCardZone::ShieldStack;
	}

	/**
	 * Rule 4-1-4: Check if zone is public or private
	 * @param Zone Zone to check
	 * @return True if public, false if private
	 */
	UFUNCTION(BlueprintPure, Category = "Comprehensive Rules|Section 4")
	static bool IsPublicZone(EGCGCardZone Zone)
	{
		// Rule 4-1-4: Public zones vs private zones
		switch (Zone)
		{
			// Public zones
			case EGCGCardZone::ResourceArea:
			case EGCGCardZone::BattleArea:
			case EGCGCardZone::BaseSection:
			case EGCGCardZone::RemovalArea:
			case EGCGCardZone::Graveyard:
				return true;

			// Private zones
			case EGCGCardZone::Deck:
			case EGCGCardZone::ResourceDeck:
			case EGCGCardZone::ShieldStack:
			case EGCGCardZone::Hand:
				return false;

			default:
				return false;
		}
	}

	/**
	 * Rule 4-4-2: Validate resource area capacity (max 15 resources, max 5 EX Resources)
	 * @param PlayerState Player to check
	 * @return Validation result
	 */
	UFUNCTION(BlueprintCallable, Category = "Comprehensive Rules|Section 4")
	FGCGRulesValidationResult ValidateRule_4_4_2_ResourceAreaLimit(const AGCGPlayerState* PlayerState) const;

	/**
	 * Rule 4-5-4: Validate battle area capacity (max 6 Units)
	 * @param PlayerState Player to check
	 * @return Validation result
	 */
	UFUNCTION(BlueprintCallable, Category = "Comprehensive Rules|Section 4")
	FGCGRulesValidationResult ValidateRule_4_5_4_BattleAreaLimit(const AGCGPlayerState* PlayerState) const;

	/**
	 * Rule 4-6-3: Validate base section capacity (max 1 Base)
	 * @param PlayerState Player to check
	 * @return Validation result
	 */
	UFUNCTION(BlueprintCallable, Category = "Comprehensive Rules|Section 4")
	FGCGRulesValidationResult ValidateRule_4_6_3_BaseSectionLimit(const AGCGPlayerState* PlayerState) const;

	/**
	 * Rule 4-8-4: Validate hand size limit (max 10 during end phase)
	 * @param PlayerState Player to check
	 * @param bIsEndPhase Is it currently the end phase?
	 * @return Validation result
	 */
	UFUNCTION(BlueprintCallable, Category = "Comprehensive Rules|Section 4")
	FGCGRulesValidationResult ValidateRule_4_8_4_HandSizeLimit(
		const AGCGPlayerState* PlayerState,
		bool bIsEndPhase) const;

	// ===== SECTION 5: ESSENTIAL GAME TERMINOLOGY =====

	/**
	 * Rule 5-4-2: Cards enter field zones as active
	 * @param Zone Zone the card is entering
	 * @return True if card should be set to active
	 */
	UFUNCTION(BlueprintPure, Category = "Comprehensive Rules|Section 5")
	static bool ShouldEnterZoneActive(EGCGCardZone Zone)
	{
		// Rule 5-4-2: Cards entering these zones are set active
		return Zone == EGCGCardZone::BattleArea ||
		       Zone == EGCGCardZone::ResourceArea ||
		       Zone == EGCGCardZone::BaseSection;
	}

	/**
	 * Rule 5-5-5: Check if damage should be dealt (zero damage is not dealt)
	 * @param Damage Amount of damage
	 * @return True if damage should be dealt
	 */
	UFUNCTION(BlueprintPure, Category = "Comprehensive Rules|Section 5")
	static bool ShouldDealDamage(int32 Damage)
	{
		// Rule 5-5-5: Damage is not dealt when amount would be zero
		return Damage > 0;
	}

	/**
	 * Rule 5-6: Recover HP (remove damage counters)
	 * @param Card Card to heal
	 * @param RecoveryAmount Amount of HP to recover
	 * @return Actual amount recovered
	 */
	UFUNCTION(BlueprintCallable, Category = "Comprehensive Rules|Section 5")
	int32 RecoverHP(UPARAM(ref) FGCGCardInstance& Card, int32 RecoveryAmount) const;

	/**
	 * Rule 5-17-2-3: Check if card is colorless (tokens are colorless)
	 * @param Card Card to check
	 * @return True if colorless
	 */
	UFUNCTION(BlueprintPure, Category = "Comprehensive Rules|Section 5")
	bool IsCardColorless(const FGCGCardInstance& Card) const;

	/**
	 * Rule 5-17-2-4: Get card cost (tokens have 0 cost)
	 * @param Card Card to check
	 * @return Effective cost
	 */
	UFUNCTION(BlueprintPure, Category = "Comprehensive Rules|Section 5")
	int32 GetEffectiveCardCost(const FGCGCardInstance& Card) const;

	/**
	 * Rule 5-17-2-4: Get card level (tokens have 0 level)
	 * @param Card Card to check
	 * @return Effective level
	 */
	UFUNCTION(BlueprintPure, Category = "Comprehensive Rules|Section 5")
	int32 GetEffectiveCardLevel(const FGCGCardInstance& Card) const;

	/**
	 * Rule 5-17-2-5: Check if token should be removed when moving to zone
	 * @param Card Token card
	 * @param ToZone Zone being moved to
	 * @return True if token should be removed from game
	 */
	UFUNCTION(BlueprintPure, Category = "Comprehensive Rules|Section 5")
	static bool ShouldRemoveToken(const FGCGCardInstance& Card, EGCGCardZone ToZone)
	{
		// Rule 5-17-2-5: Tokens removed when leaving field zones
		if (!Card.bIsToken)
		{
			return false;
		}

		bool bIsFieldZone = (ToZone == EGCGCardZone::BattleArea ||
		                     ToZone == EGCGCardZone::ResourceArea ||
		                     ToZone == EGCGCardZone::ShieldStack ||
		                     ToZone == EGCGCardZone::BaseSection);

		return !bIsFieldZone; // Remove if not going to field
	}

	/**
	 * Rule 5-17-3-1: Create EX Base token (0 AP / 3 HP)
	 * @param OwnerPlayerID Player who owns this token
	 * @return EX Base token instance
	 */
	UFUNCTION(BlueprintCallable, Category = "Comprehensive Rules|Section 5")
	FGCGCardInstance CreateEXBaseToken(int32 OwnerPlayerID) const;

	/**
	 * Rule 5-19-1: Check if trait matches condition with "/" (or) operator
	 * @param Condition Condition string (e.g., "Zeon/Neo Zeon")
	 * @param CardTraits Card's traits
	 * @return True if any trait matches
	 */
	UFUNCTION(BlueprintPure, Category = "Comprehensive Rules|Section 5")
	bool MatchesTraitCondition(const FString& Condition, const TArray<FName>& CardTraits) const;

	// ===== SECTION 6: PREPARING TO PLAY =====
	// To be implemented when Section 6 is provided

	/**
	 * Rule 6-X: [Placeholder for Section 6 rules]
	 * @return Validation result
	 */
	UFUNCTION(BlueprintCallable, Category = "Comprehensive Rules|Section 6")
	FGCGRulesValidationResult ValidateSection6_Placeholder() const;

	// ===== SECTION 7: GAME PROGRESSION =====
	// To be implemented when Section 7 is provided

	/**
	 * Rule 7-X: [Placeholder for Section 7 rules]
	 * @return Validation result
	 */
	UFUNCTION(BlueprintCallable, Category = "Comprehensive Rules|Section 7")
	FGCGRulesValidationResult ValidateSection7_Placeholder() const;

	// ===== SECTION 8: ATTACKING AND BATTLES =====
	// To be implemented when Section 8 is provided

	/**
	 * Rule 8-X: [Placeholder for Section 8 rules]
	 * @return Validation result
	 */
	UFUNCTION(BlueprintCallable, Category = "Comprehensive Rules|Section 8")
	FGCGRulesValidationResult ValidateSection8_Placeholder() const;

	// ===== SECTION 9: ACTION STEPS =====
	// To be implemented when Section 9 is provided

	/**
	 * Rule 9-X: [Placeholder for Section 9 rules]
	 * @return Validation result
	 */
	UFUNCTION(BlueprintCallable, Category = "Comprehensive Rules|Section 9")
	FGCGRulesValidationResult ValidateSection9_Placeholder() const;

	// ===== SECTION 10: EFFECT ACTIVATION AND RESOLUTION =====
	// To be implemented when Section 10 is provided

	/**
	 * Rule 10-X: [Placeholder for Section 10 rules]
	 * @return Validation result
	 */
	UFUNCTION(BlueprintCallable, Category = "Comprehensive Rules|Section 10")
	FGCGRulesValidationResult ValidateSection10_Placeholder() const;

	// ===== SECTION 11: RULES MANAGEMENT =====
	// Note: Section 1 includes rules management (1-2-3), already implemented in AGCGGameMode_1v1

	/**
	 * Rule 11-X: [Additional rules management validation if needed]
	 * @return Validation result
	 */
	UFUNCTION(BlueprintCallable, Category = "Comprehensive Rules|Section 11")
	FGCGRulesValidationResult ValidateSection11_Placeholder() const;

	// ===== SECTION 12: MULTIPLAYER BATTLE =====
	// To be implemented when Section 12 is provided

	/**
	 * Rule 12-X: [Placeholder for Section 12 rules]
	 * @return Validation result
	 */
	UFUNCTION(BlueprintCallable, Category = "Comprehensive Rules|Section 12")
	FGCGRulesValidationResult ValidateSection12_Placeholder() const;

	// ===== SECTION 13: KEYWORD EFFECTS AND KEYWORDS =====
	// To be implemented when Section 13 is provided

	/**
	 * Rule 13-X: [Placeholder for Section 13 rules]
	 * @return Validation result
	 */
	UFUNCTION(BlueprintCallable, Category = "Comprehensive Rules|Section 13")
	FGCGRulesValidationResult ValidateSection13_Placeholder() const;

	// ===== HELPER FUNCTIONS =====

	/**
	 * Get reference to card database (cached for performance)
	 * @return Card database subsystem
	 */
	UGCGCardDatabase* GetCardDatabase() const;

private:
	/** Cached reference to card database */
	UPROPERTY()
	mutable UGCGCardDatabase* CachedCardDatabase;
};
