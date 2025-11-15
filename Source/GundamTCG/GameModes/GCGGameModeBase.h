// GCGGameModeBase.h - Base Game Mode for Gundam Card Game
// Unreal Engine 5.6 - Gundam TCG Implementation
// This is the base class for all game modes (Lobby, 1v1, 2v2)

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/GameMode.h"
#include "GundamTCG/GCGTypes.h"
#include "GCGGameModeBase.generated.h"

// Forward declarations
class UDataTable;
class AGCGGameState;
class AGCGPlayerState;
class AGCGPlayerController;

/**
 * Base Game Mode for Gundam Card Game
 *
 * This is the base class that provides common functionality for all game modes:
 * - Card database management
 * - Player management
 * - Game initialization
 *
 * Derived classes:
 * - AGCGGameMode_1v1: Standard 1v1 match
 * - AGCGGameMode_2v2: Team Battle mode
 */
UCLASS()
class GUNDAMTCG_API AGCGGameModeBase : public AGameMode
{
	GENERATED_BODY()

public:
	AGCGGameModeBase();

	// ===== CARD DATABASE =====

	/**
	 * Reference to the main card database DataTable
	 * This should be set to DT_AllCards in the editor
	 */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Card Database")
	UDataTable* CardDatabase;

	/**
	 * Lookup card data by card number
	 * @param CardNumber The card number to look up (e.g., "GCG-001")
	 * @return Pointer to card data, or nullptr if not found
	 */
	UFUNCTION(BlueprintPure, Category = "Cards")
	const FGCGCardData* GetCardData(FName CardNumber) const;

	/**
	 * Check if a card exists in the database
	 * @param CardNumber The card number to check
	 * @return True if card exists, false otherwise
	 */
	UFUNCTION(BlueprintPure, Category = "Cards")
	bool CardExists(FName CardNumber) const;

	// ===== PLAYER MANAGEMENT =====

	/**
	 * Get player state by player ID
	 * @param PlayerID The player ID (0-based)
	 * @return Pointer to player state, or nullptr if not found
	 */
	UFUNCTION(BlueprintPure, Category = "Players")
	AGCGPlayerState* GetPlayerStateByID(int32 PlayerID) const;

	/**
	 * Get player controller by player ID
	 * @param PlayerID The player ID (0-based)
	 * @return Pointer to player controller, or nullptr if not found
	 */
	UFUNCTION(BlueprintPure, Category = "Players")
	AGCGPlayerController* GetPlayerControllerByID(int32 PlayerID) const;

	/**
	 * Get all player states in the game
	 * @return Array of all player states
	 */
	UFUNCTION(BlueprintPure, Category = "Players")
	TArray<AGCGPlayerState*> GetAllPlayerStates() const;

	// ===== GAME STATE ACCESS =====

	/**
	 * Get the GCG game state
	 * @return Pointer to GCG game state, or nullptr if not available
	 */
	UFUNCTION(BlueprintPure, Category = "Game State")
	AGCGGameState* GetGCGGameState() const;

	// ===== UTILITY FUNCTIONS =====

	/**
	 * Create a card instance from a card number
	 * @param CardNumber The card number to create
	 * @param OwnerPlayerID The player who owns this card
	 * @param bIsToken Is this a token card?
	 * @return New card instance
	 */
	UFUNCTION(BlueprintCallable, Category = "Cards")
	FGCGCardInstance CreateCardInstance(FName CardNumber, int32 OwnerPlayerID, bool bIsToken = false);

	/**
	 * Create a token card instance
	 * @param TokenType The type of token (e.g., "EXBase", "EXResource")
	 * @param OwnerPlayerID The player who owns this token
	 * @return New token card instance
	 */
	UFUNCTION(BlueprintCallable, Category = "Cards")
	FGCGCardInstance CreateTokenInstance(FName TokenType, int32 OwnerPlayerID);

protected:
	/**
	 * Called when the game starts or when spawned
	 */
	virtual void BeginPlay() override;

	/**
	 * Called when a player logs in
	 * @param NewPlayer The new player controller
	 */
	virtual void PostLogin(APlayerController* NewPlayer) override;

	/**
	 * Called when a player logs out
	 * @param Exiting The exiting player controller
	 */
	virtual void Logout(AController* Exiting) override;

	// ===== INSTANCE ID GENERATION =====

	/**
	 * Next available card instance ID
	 * This is incremented each time a new card instance is created
	 */
	UPROPERTY()
	int32 NextInstanceID;

	/**
	 * Generate a unique instance ID
	 * @return New unique instance ID
	 */
	int32 GenerateInstanceID();

	// ===== BLUEPRINT EVENTS =====

	/**
	 * Called when the game is initialized
	 */
	UFUNCTION(BlueprintImplementableEvent, Category = "Game Events")
	void OnGameInitialized();

	/**
	 * Called when a player joins the game
	 * @param PlayerID The player who joined
	 */
	UFUNCTION(BlueprintImplementableEvent, Category = "Game Events")
	void OnPlayerJoined(int32 PlayerID);

	/**
	 * Called when a player leaves the game
	 * @param PlayerID The player who left
	 */
	UFUNCTION(BlueprintImplementableEvent, Category = "Game Events")
	void OnPlayerLeft(int32 PlayerID);
};
