// GCGDeckValidationSubsystem.h - Deck Construction Validation Subsystem
// Unreal Engine 5.6 - Gundam TCG Implementation
// Validates deck construction rules from Comprehensive Rules Section 6-1

#pragma once

#include "CoreMinimal.h"
#include "Subsystems/GameInstanceSubsystem.h"
#include "GundamTCG/GCGTypes.h"
#include "GCGDeckValidationSubsystem.generated.h"

// Forward declarations
class UGCGCardDatabase;

/**
 * Deck Validation Result
 * Contains validation status and error messages
 */
USTRUCT(BlueprintType)
struct FGCGDeckValidationResult
{
	GENERATED_BODY()

	/** Did the deck pass validation? */
	UPROPERTY(BlueprintReadOnly)
	bool bIsValid;

	/** List of validation errors (empty if valid) */
	UPROPERTY(BlueprintReadOnly)
	TArray<FString> Errors;

	/** List of validation warnings (deck is valid but has issues) */
	UPROPERTY(BlueprintReadOnly)
	TArray<FString> Warnings;

	FGCGDeckValidationResult()
		: bIsValid(true)
	{}

	FGCGDeckValidationResult(bool bInIsValid, const FString& ErrorMessage = TEXT(""))
		: bIsValid(bInIsValid)
	{
		if (!ErrorMessage.IsEmpty())
		{
			Errors.Add(ErrorMessage);
		}
	}

	void AddError(const FString& ErrorMessage)
	{
		bIsValid = false;
		Errors.Add(ErrorMessage);
	}

	void AddWarning(const FString& WarningMessage)
	{
		Warnings.Add(WarningMessage);
	}
};

/**
 * Deck Validation Subsystem
 *
 * This subsystem validates deck construction according to Comprehensive Rules Section 6-1:
 * - Deck size (50 main deck + 10 resource deck)
 * - Color restrictions (1-2 colors only)
 * - Copy limits (max 4 copies per card in main deck)
 * - Card type restrictions (main deck types, resource deck types)
 *
 * Validation should be performed:
 * 1. In deck builder when saving a deck
 * 2. In lobby when selecting a deck for a match
 * 3. (Optional) At game start as final check
 */
UCLASS()
class GUNDAMTCG_API UGCGDeckValidationSubsystem : public UGameInstanceSubsystem
{
	GENERATED_BODY()

public:
	// ===== SUBSYSTEM LIFECYCLE =====

	virtual void Initialize(FSubsystemCollectionBase& Collection) override;
	virtual void Deinitialize() override;

	// ===== COMPLETE DECK VALIDATION =====

	/**
	 * Validate a complete deck (main + resource)
	 * Checks all rules from Section 6-1
	 * @param MainDeckList Array of card numbers in main deck
	 * @param ResourceDeckList Array of card numbers in resource deck
	 * @return Validation result with all errors
	 */
	UFUNCTION(BlueprintCallable, Category = "Deck Validation")
	FGCGDeckValidationResult ValidateCompleteDeck(const TArray<FName>& MainDeckList, const TArray<FName>& ResourceDeckList);

	// ===== INDIVIDUAL RULE VALIDATION =====

	/**
	 * Rule 6-1-1: Validate deck sizes (50 main + 10 resource)
	 * @param MainDeckList Main deck cards
	 * @param ResourceDeckList Resource deck cards
	 * @return True if sizes are correct
	 */
	UFUNCTION(BlueprintPure, Category = "Deck Validation|Size")
	bool ValidateDeckSize(const TArray<FName>& MainDeckList, const TArray<FName>& ResourceDeckList) const;

	/**
	 * Rule 6-1-1-1: Validate main deck card types (Unit, Pilot, Command, Base only)
	 * @param MainDeckList Main deck cards
	 * @param OutInvalidCards List of invalid card numbers found
	 * @return True if all cards are valid types
	 */
	UFUNCTION(BlueprintCallable, Category = "Deck Validation|Types")
	bool ValidateMainDeckCardTypes(const TArray<FName>& MainDeckList, TArray<FName>& OutInvalidCards);

	/**
	 * Rule 6-1-1-2: Validate deck colors (1 or 2 colors maximum)
	 * @param MainDeckList Main deck cards
	 * @param OutColorsFound Set of colors found in deck
	 * @return True if deck uses 1-2 colors only
	 */
	UFUNCTION(BlueprintCallable, Category = "Deck Validation|Colors")
	bool ValidateDeckColors(const TArray<FName>& MainDeckList, TSet<EGCGCardColor>& OutColorsFound);

	/**
	 * Rule 6-1-1-3: Validate copy limits (max 4 copies per card in main deck)
	 * @param MainDeckList Main deck cards
	 * @param OutOverLimitCards Map of card numbers exceeding limit with their counts
	 * @return True if no card exceeds 4 copies
	 */
	UFUNCTION(BlueprintCallable, Category = "Deck Validation|Copies")
	bool ValidateCopyLimits(const TArray<FName>& MainDeckList, TMap<FName, int32>& OutOverLimitCards);

	/**
	 * Rule 6-1-1-4: Validate resource deck card types (Resource cards only)
	 * @param ResourceDeckList Resource deck cards
	 * @param OutInvalidCards List of invalid card numbers found
	 * @return True if all cards are Resource type
	 */
	UFUNCTION(BlueprintCallable, Category = "Deck Validation|Types")
	bool ValidateResourceDeckCardTypes(const TArray<FName>& ResourceDeckList, TArray<FName>& OutInvalidCards);

	/**
	 * Rule 6-1-1-5: Resource deck has no copy limits (any number allowed)
	 * This is always valid, provided for completeness
	 * @return Always true
	 */
	UFUNCTION(BlueprintPure, Category = "Deck Validation|Copies")
	bool ValidateResourceCopyLimits() const { return true; }

	// ===== HELPER FUNCTIONS =====

	/**
	 * Count card copies in a deck
	 * @param DeckList Deck to analyze
	 * @return Map of card number to count
	 */
	UFUNCTION(BlueprintPure, Category = "Deck Validation|Helpers")
	TMap<FName, int32> CountCardCopies(const TArray<FName>& DeckList) const;

	/**
	 * Get unique colors in a deck
	 * @param DeckList Deck to analyze
	 * @return Set of colors found (excludes Colorless)
	 */
	UFUNCTION(BlueprintCallable, Category = "Deck Validation|Helpers")
	TSet<EGCGCardColor> GetDeckColors(const TArray<FName>& DeckList);

	/**
	 * Check if a card exists in database
	 * @param CardNumber Card to check
	 * @return True if card exists
	 */
	UFUNCTION(BlueprintPure, Category = "Deck Validation|Helpers")
	bool CardExists(FName CardNumber) const;

private:
	// ===== INTERNAL HELPERS =====

	/**
	 * Get card database subsystem
	 */
	UGCGCardDatabase* GetCardDatabase() const;

	/**
	 * Cache card database for faster lookups
	 */
	UPROPERTY()
	UGCGCardDatabase* CachedCardDatabase;
};
