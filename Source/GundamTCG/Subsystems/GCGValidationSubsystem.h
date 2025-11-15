// GCGValidationSubsystem.h - Game State Validation & Rule Enforcement
// Unreal Engine 5.6 - Gundam TCG Implementation
// Validates game state, detects illegal states, and enforces game rules

#pragma once

#include "CoreMinimal.h"
#include "Subsystems/GameInstanceSubsystem.h"
#include "GundamTCG/GCGTypes.h"
#include "GCGValidationSubsystem.generated.h"

// Forward declarations
class AGCGPlayerState;
class AGCGGameState;
class UGCGCardDatabase;

/**
 * Validation Result
 */
USTRUCT(BlueprintType)
struct FGCGValidationResult
{
	GENERATED_BODY()

	UPROPERTY(BlueprintReadOnly, Category = "Validation")
	bool bIsValid = true;

	UPROPERTY(BlueprintReadOnly, Category = "Validation")
	TArray<FString> Errors;

	UPROPERTY(BlueprintReadOnly, Category = "Validation")
	TArray<FString> Warnings;

	void AddError(const FString& Error)
	{
		bIsValid = false;
		Errors.Add(Error);
		UE_LOG(LogTemp, Error, TEXT("VALIDATION ERROR: %s"), *Error);
	}

	void AddWarning(const FString& Warning)
	{
		Warnings.Add(Warning);
		UE_LOG(LogTemp, Warning, TEXT("VALIDATION WARNING: %s"), *Warning);
	}

	FString ToString() const
	{
		FString Result = bIsValid ? TEXT("VALID") : TEXT("INVALID");
		if (Errors.Num() > 0)
		{
			Result += FString::Printf(TEXT("\nErrors (%d):"), Errors.Num());
			for (const FString& Error : Errors)
			{
				Result += TEXT("\n  - ") + Error;
			}
		}
		if (Warnings.Num() > 0)
		{
			Result += FString::Printf(TEXT("\nWarnings (%d):"), Warnings.Num());
			for (const FString& Warning : Warnings)
			{
				Result += TEXT("\n  - ") + Warning;
			}
		}
		return Result;
	}
};

/**
 * Validation Subsystem
 *
 * Validates game state, player actions, and enforces game rules.
 * Used for debugging, testing, and ensuring game integrity.
 */
UCLASS()
class GUNDAMTCG_API UGCGValidationSubsystem : public UGameInstanceSubsystem
{
	GENERATED_BODY()

public:
	// ===========================================================================================
	// INITIALIZATION
	// ===========================================================================================

	virtual void Initialize(FSubsystemCollectionBase& Collection) override;
	virtual void Deinitialize() override;

	// ===========================================================================================
	// FULL GAME STATE VALIDATION
	// ===========================================================================================

	/**
	 * Validate entire game state
	 * Checks all players, zones, game rules
	 * @param GameState The game state to validate
	 * @return Validation result
	 */
	UFUNCTION(BlueprintCallable, Category = "Validation")
	FGCGValidationResult ValidateGameState(AGCGGameState* GameState);

	/**
	 * Validate a single player's state
	 * Checks zones, hand limit, deck counts, etc.
	 * @param PlayerState The player state to validate
	 * @return Validation result
	 */
	UFUNCTION(BlueprintCallable, Category = "Validation")
	FGCGValidationResult ValidatePlayerState(AGCGPlayerState* PlayerState);

	// ===========================================================================================
	// ZONE VALIDATION
	// ===========================================================================================

	/**
	 * Validate zone counts and limits
	 * @param PlayerState The player to validate
	 * @return Validation result
	 */
	UFUNCTION(BlueprintCallable, Category = "Validation")
	FGCGValidationResult ValidateZoneLimits(AGCGPlayerState* PlayerState);

	/**
	 * Validate Battle Area (max 6 Units)
	 * @param PlayerState The player to validate
	 * @return Validation result
	 */
	UFUNCTION(BlueprintCallable, Category = "Validation")
	FGCGValidationResult ValidateBattleArea(AGCGPlayerState* PlayerState);

	/**
	 * Validate Resource Area (max 15 Resources)
	 * @param PlayerState The player to validate
	 * @return Validation result
	 */
	UFUNCTION(BlueprintCallable, Category = "Validation")
	FGCGValidationResult ValidateResourceArea(AGCGPlayerState* PlayerState);

	/**
	 * Validate Shield Stack (max 6 in 1v1, 8 in 2v2)
	 * @param PlayerState The player to validate
	 * @param MaxShields Maximum shields allowed
	 * @return Validation result
	 */
	UFUNCTION(BlueprintCallable, Category = "Validation")
	FGCGValidationResult ValidateShieldStack(AGCGPlayerState* PlayerState, int32 MaxShields = 6);

	// ===========================================================================================
	// CARD VALIDATION
	// ===========================================================================================

	/**
	 * Validate a card instance
	 * Checks stats, modifiers, zone consistency
	 * @param CardInstance The card to validate
	 * @param PlayerState The player who owns the card
	 * @return Validation result
	 */
	UFUNCTION(BlueprintCallable, Category = "Validation")
	FGCGValidationResult ValidateCardInstance(const FGCGCardInstance& CardInstance, AGCGPlayerState* PlayerState);

	/**
	 * Validate card stats (AP, HP, Cost)
	 * @param CardInstance The card to validate
	 * @return Validation result
	 */
	UFUNCTION(BlueprintCallable, Category = "Validation")
	FGCGValidationResult ValidateCardStats(const FGCGCardInstance& CardInstance);

	/**
	 * Validate card modifiers
	 * @param CardInstance The card to validate
	 * @return Validation result
	 */
	UFUNCTION(BlueprintCallable, Category = "Validation")
	FGCGValidationResult ValidateCardModifiers(const FGCGCardInstance& CardInstance);

	// ===========================================================================================
	// DECK VALIDATION
	// ===========================================================================================

	/**
	 * Validate a deck list (pre-game)
	 * Checks 50-card main deck, 10-card resource deck, max 4 copies
	 * @param DeckList The deck to validate
	 * @return Validation result
	 */
	UFUNCTION(BlueprintCallable, Category = "Validation")
	FGCGValidationResult ValidateDeckList(const FGCGDeckList& DeckList);

	/**
	 * Validate deck during game (check for duplicates, illegal cards)
	 * @param PlayerState The player to validate
	 * @return Validation result
	 */
	UFUNCTION(BlueprintCallable, Category = "Validation")
	FGCGValidationResult ValidateDeckDuringGame(AGCGPlayerState* PlayerState);

	// ===========================================================================================
	// COMBAT VALIDATION
	// ===========================================================================================

	/**
	 * Validate an attack declaration
	 * @param AttackerInstance The attacker
	 * @param AttackingPlayer The attacking player
	 * @param GameState The game state
	 * @return Validation result
	 */
	UFUNCTION(BlueprintCallable, Category = "Validation")
	FGCGValidationResult ValidateAttackDeclaration(
		const FGCGCardInstance& AttackerInstance,
		AGCGPlayerState* AttackingPlayer,
		AGCGGameState* GameState);

	/**
	 * Validate a blocker declaration
	 * @param BlockerInstance The blocker
	 * @param DefendingPlayer The defending player
	 * @return Validation result
	 */
	UFUNCTION(BlueprintCallable, Category = "Validation")
	FGCGValidationResult ValidateBlockerDeclaration(
		const FGCGCardInstance& BlockerInstance,
		AGCGPlayerState* DefendingPlayer);

	// ===========================================================================================
	// RULE ENFORCEMENT
	// ===========================================================================================

	/**
	 * Check for duplicate instance IDs across all players
	 * @param GameState The game state
	 * @return Validation result
	 */
	UFUNCTION(BlueprintCallable, Category = "Validation")
	FGCGValidationResult CheckForDuplicateInstanceIDs(AGCGGameState* GameState);

	/**
	 * Check for orphaned cards (cards in wrong zones)
	 * @param PlayerState The player to check
	 * @return Validation result
	 */
	UFUNCTION(BlueprintCallable, Category = "Validation")
	FGCGValidationResult CheckForOrphanedCards(AGCGPlayerState* PlayerState);

	/**
	 * Check for negative stats (AP, HP, Cost can't be negative)
	 * @param PlayerState The player to check
	 * @return Validation result
	 */
	UFUNCTION(BlueprintCallable, Category = "Validation")
	FGCGValidationResult CheckForNegativeStats(AGCGPlayerState* PlayerState);

	// ===========================================================================================
	// LOGGING
	// ===========================================================================================

	/**
	 * Log validation result to console
	 * @param Result The validation result
	 * @param Context Context description
	 */
	UFUNCTION(BlueprintCallable, Category = "Validation")
	void LogValidationResult(const FGCGValidationResult& Result, const FString& Context);

	/**
	 * Enable/disable validation logging
	 * @param bEnabled Enable logging
	 */
	UFUNCTION(BlueprintCallable, Category = "Validation")
	void SetValidationLogging(bool bEnabled);

	// ===========================================================================================
	// PROPERTIES
	// ===========================================================================================

private:
	// Cached reference to Card Database
	UPROPERTY()
	UGCGCardDatabase* CardDatabase = nullptr;

	// Enable validation logging
	UPROPERTY()
	bool bValidationLoggingEnabled = true;
};
