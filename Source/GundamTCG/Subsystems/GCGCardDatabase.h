// GCGCardDatabase.h - Card Database Subsystem
// Unreal Engine 5.6 - Gundam TCG Implementation
// This subsystem manages all card data and provides card lookup functionality

#pragma once

#include "CoreMinimal.h"
#include "Subsystems/GameInstanceSubsystem.h"
#include "Engine/DataTable.h"
#include "GundamTCG/GCGTypes.h"
#include "GCGCardDatabase.generated.h"

/**
 * Card Database Subsystem
 *
 * This subsystem provides centralized access to all card data:
 * - Card data lookup by card number
 * - Token definitions (EX Base, EX Resource)
 * - Card validation
 * - DataTable management
 *
 * Card data is stored in a DataTable asset (assigned in Project Settings or GameInstance Blueprint).
 * The DataTable uses FGCGCardData as its row structure.
 */
UCLASS()
class GUNDAMTCG_API UGCGCardDatabase : public UGameInstanceSubsystem
{
	GENERATED_BODY()

public:
	// ===== SUBSYSTEM LIFECYCLE =====

	/**
	 * Initialize the subsystem
	 */
	virtual void Initialize(FSubsystemCollectionBase& Collection) override;

	/**
	 * Deinitialize the subsystem
	 */
	virtual void Deinitialize() override;

	// ===== CARD DATA LOOKUP =====

	/**
	 * Get card data by card number
	 * @param CardNumber The card number (e.g., "GU-001", "GU-002")
	 * @return Pointer to card data, or nullptr if not found
	 */
	UFUNCTION(BlueprintPure, Category = "Card Database")
	const FGCGCardData* GetCardData(FName CardNumber) const;

	/**
	 * Check if a card exists in the database
	 * @param CardNumber The card number to check
	 * @return True if card exists
	 */
	UFUNCTION(BlueprintPure, Category = "Card Database")
	bool CardExists(FName CardNumber) const;

	/**
	 * Get all cards in the database
	 * @return Array of all card data entries
	 */
	UFUNCTION(BlueprintPure, Category = "Card Database")
	TArray<FGCGCardData> GetAllCards() const;

	/**
	 * Get all cards of a specific type
	 * @param CardType The type to filter by
	 * @return Array of card data entries matching the type
	 */
	UFUNCTION(BlueprintPure, Category = "Card Database")
	TArray<FGCGCardData> GetCardsByType(EGCGCardType CardType) const;

	/**
	 * Get all cards with a specific color
	 * @param Color The color to filter by
	 * @return Array of card data entries containing the color
	 */
	UFUNCTION(BlueprintPure, Category = "Card Database")
	TArray<FGCGCardData> GetCardsByColor(EGCGCardColor Color) const;

	// ===== TOKEN DEFINITIONS =====

	/**
	 * Get token data by token type
	 * @param TokenType The token type (e.g., "EXBase", "EXResource")
	 * @return Token data
	 */
	UFUNCTION(BlueprintPure, Category = "Card Database")
	FGCGCardData GetTokenData(FName TokenType) const;

	/**
	 * Check if a card number is a token
	 * @param CardNumber The card number to check
	 * @return True if this is a token
	 */
	UFUNCTION(BlueprintPure, Category = "Card Database")
	bool IsToken(FName CardNumber) const;

	// ===== CARD VALIDATION =====

	/**
	 * Validate a deck list
	 * @param DeckList Array of card numbers (should be 50 cards)
	 * @param OutErrors Array of validation error messages
	 * @return True if deck is valid
	 */
	UFUNCTION(BlueprintCallable, Category = "Card Database")
	bool ValidateDeck(const TArray<FName>& DeckList, TArray<FString>& OutErrors) const;

	/**
	 * Validate a resource deck list
	 * @param ResourceDeckList Array of card numbers (should be 10 cards)
	 * @param OutErrors Array of validation error messages
	 * @return True if resource deck is valid
	 */
	UFUNCTION(BlueprintCallable, Category = "Card Database")
	bool ValidateResourceDeck(const TArray<FName>& ResourceDeckList, TArray<FString>& OutErrors) const;

	// ===== DATATABLE MANAGEMENT =====

	/**
	 * Set the card data table
	 * @param NewDataTable The data table to use for card data
	 */
	UFUNCTION(BlueprintCallable, Category = "Card Database")
	void SetCardDataTable(UDataTable* NewDataTable);

	/**
	 * Get the current card data table
	 * @return The card data table
	 */
	UFUNCTION(BlueprintPure, Category = "Card Database")
	UDataTable* GetCardDataTable() const { return CardDataTable; }

	/**
	 * Reload card data from the data table
	 */
	UFUNCTION(BlueprintCallable, Category = "Card Database")
	void ReloadCardData();

	// ===== STATISTICS =====

	/**
	 * Get total number of cards in database
	 * @return Card count
	 */
	UFUNCTION(BlueprintPure, Category = "Card Database")
	int32 GetCardCount() const;

	/**
	 * Get database statistics (for debugging/UI)
	 * @return Human-readable statistics string
	 */
	UFUNCTION(BlueprintPure, Category = "Card Database")
	FString GetDatabaseStats() const;

protected:
	// ===== INTERNAL HELPERS =====

	/**
	 * Initialize token definitions
	 */
	void InitializeTokenDefinitions();

	/**
	 * Create EX Base token data
	 * @return EX Base token data
	 */
	FGCGCardData CreateEXBaseTokenData() const;

	/**
	 * Create EX Resource token data
	 * @return EX Resource token data
	 */
	FGCGCardData CreateEXResourceTokenData() const;

private:
	// ===== DATA STORAGE =====

	/**
	 * The DataTable containing all card data
	 * This should be set in the GameInstance Blueprint or via project settings
	 */
	UPROPERTY()
	UDataTable* CardDataTable;

	/**
	 * Token definitions (EX Base, EX Resource, etc.)
	 * These are hard-coded since they're game rules, not card data
	 */
	UPROPERTY()
	TMap<FName, FGCGCardData> TokenDefinitions;

	/**
	 * Cache of card data for faster lookups
	 * Populated on initialization from DataTable
	 */
	TMap<FName, FGCGCardData*> CardDataCache;
};
