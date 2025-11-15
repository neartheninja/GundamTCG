// GCGZoneSubsystem.h - Zone Management Subsystem
// Unreal Engine 5.6 - Gundam TCG Implementation
// This subsystem handles all card movement between zones and zone validation

#pragma once

#include "CoreMinimal.h"
#include "Subsystems/GameInstanceSubsystem.h"
#include "GundamTCG/GCGTypes.h"
#include "GCGZoneSubsystem.generated.h"

// Forward declarations
class AGCGPlayerState;
class AGCGGameState;

/**
 * Zone Management Subsystem
 *
 * This subsystem provides centralized zone management for the Gundam Card Game:
 * - Card movement between zones with validation
 * - Zone limit enforcement (6 Units, 15 Resources, etc.)
 * - Zone shuffling (Deck, Resource Deck)
 * - Zone queries (get cards in zone, count, etc.)
 *
 * All zone operations go through this subsystem to ensure consistency and proper replication.
 */
UCLASS()
class GUNDAMTCG_API UGCGZoneSubsystem : public UGameInstanceSubsystem
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

	// ===== CARD MOVEMENT =====

	/**
	 * Move a card from one zone to another
	 * @param Card The card instance to move
	 * @param FromZone The zone the card is currently in
	 * @param ToZone The zone to move the card to
	 * @param PlayerState The player state that owns this card
	 * @param GameState The current game state (for team battle checks)
	 * @param bValidateLimits Should zone limits be validated? (default true)
	 * @return True if move was successful
	 */
	UFUNCTION(BlueprintCallable, Category = "Zone Management")
	bool MoveCard(UPARAM(ref) FGCGCardInstance& Card, EGCGCardZone FromZone, EGCGCardZone ToZone,
		AGCGPlayerState* PlayerState, AGCGGameState* GameState, bool bValidateLimits = true);

	/**
	 * Move multiple cards from one zone to another
	 * @param Cards Array of card instances to move
	 * @param FromZone The zone the cards are currently in
	 * @param ToZone The zone to move the cards to
	 * @param PlayerState The player state that owns these cards
	 * @param GameState The current game state
	 * @param bValidateLimits Should zone limits be validated?
	 * @return Number of cards successfully moved
	 */
	UFUNCTION(BlueprintCallable, Category = "Zone Management")
	int32 MoveCards(UPARAM(ref) TArray<FGCGCardInstance>& Cards, EGCGCardZone FromZone, EGCGCardZone ToZone,
		AGCGPlayerState* PlayerState, AGCGGameState* GameState, bool bValidateLimits = true);

	// ===== ZONE VALIDATION =====

	/**
	 * Check if a card can be added to a zone
	 * @param Zone The zone to check
	 * @param PlayerState The player state
	 * @param GameState The game state (for team battle checks)
	 * @param CardType The type of card being added (for specific zone checks)
	 * @return True if card can be added to the zone
	 */
	UFUNCTION(BlueprintPure, Category = "Zone Management")
	bool CanAddToZone(EGCGCardZone Zone, AGCGPlayerState* PlayerState, AGCGGameState* GameState, EGCGCardType CardType = EGCGCardType::Unit) const;

	/**
	 * Get the current count of cards in a zone
	 * @param Zone The zone to count
	 * @param PlayerState The player state
	 * @return Number of cards in the zone
	 */
	UFUNCTION(BlueprintPure, Category = "Zone Management")
	int32 GetZoneCount(EGCGCardZone Zone, AGCGPlayerState* PlayerState) const;

	/**
	 * Get the maximum capacity for a zone
	 * @param Zone The zone to check
	 * @param CardType The type of card (for specific zone checks)
	 * @return Maximum capacity (-1 for unlimited)
	 */
	UFUNCTION(BlueprintPure, Category = "Zone Management")
	int32 GetZoneMaxCapacity(EGCGCardZone Zone, EGCGCardType CardType = EGCGCardType::Unit) const;

	/**
	 * Check if a zone is at capacity
	 * @param Zone The zone to check
	 * @param PlayerState The player state
	 * @param GameState The game state
	 * @param CardType The type of card
	 * @return True if zone is at maximum capacity
	 */
	UFUNCTION(BlueprintPure, Category = "Zone Management")
	bool IsZoneAtCapacity(EGCGCardZone Zone, AGCGPlayerState* PlayerState, AGCGGameState* GameState, EGCGCardType CardType = EGCGCardType::Unit) const;

	// ===== ZONE QUERIES =====

	/**
	 * Get all cards in a specific zone
	 * @param Zone The zone to query
	 * @param PlayerState The player state
	 * @return Array of card instances in the zone
	 */
	UFUNCTION(BlueprintPure, Category = "Zone Management")
	TArray<FGCGCardInstance> GetCardsInZone(EGCGCardZone Zone, AGCGPlayerState* PlayerState) const;

	/**
	 * Find a card in a zone by instance ID
	 * @param Zone The zone to search
	 * @param PlayerState The player state
	 * @param InstanceID The instance ID to find
	 * @param OutCard The found card (if any)
	 * @return True if card was found
	 */
	UFUNCTION(BlueprintPure, Category = "Zone Management")
	bool FindCardInZone(EGCGCardZone Zone, AGCGPlayerState* PlayerState, int32 InstanceID, FGCGCardInstance& OutCard) const;

	// ===== ZONE MANIPULATION =====

	/**
	 * Shuffle a zone (Deck or Resource Deck)
	 * @param Zone The zone to shuffle (must be Deck or ResourceDeck)
	 * @param PlayerState The player state
	 * @return True if shuffle was successful
	 */
	UFUNCTION(BlueprintCallable, Category = "Zone Management")
	bool ShuffleZone(EGCGCardZone Zone, AGCGPlayerState* PlayerState);

	/**
	 * Draw the top card from a zone
	 * @param Zone The zone to draw from (usually Deck)
	 * @param PlayerState The player state
	 * @param OutCard The drawn card
	 * @return True if draw was successful
	 */
	UFUNCTION(BlueprintCallable, Category = "Zone Management")
	bool DrawTopCard(EGCGCardZone Zone, AGCGPlayerState* PlayerState, FGCGCardInstance& OutCard);

	/**
	 * Draw multiple cards from the top of a zone
	 * @param Zone The zone to draw from
	 * @param PlayerState The player state
	 * @param Count Number of cards to draw
	 * @param OutCards The drawn cards
	 * @return Number of cards successfully drawn
	 */
	UFUNCTION(BlueprintCallable, Category = "Zone Management")
	int32 DrawTopCards(EGCGCardZone Zone, AGCGPlayerState* PlayerState, int32 Count, TArray<FGCGCardInstance>& OutCards);

	/**
	 * Peek at the top card of a zone without removing it
	 * @param Zone The zone to peek at
	 * @param PlayerState The player state
	 * @param OutCard The top card
	 * @return True if peek was successful
	 */
	UFUNCTION(BlueprintPure, Category = "Zone Management")
	bool PeekTopCard(EGCGCardZone Zone, AGCGPlayerState* PlayerState, FGCGCardInstance& OutCard) const;

	// ===== SPECIAL ZONE OPERATIONS =====

	/**
	 * Activate all cards in a zone (set bIsActive = true)
	 * @param PlayerState The player state
	 * @param Zone The zone to activate (default: all relevant zones)
	 * @return Number of cards activated
	 */
	UFUNCTION(BlueprintCallable, Category = "Zone Management")
	int32 ActivateAllCards(AGCGPlayerState* PlayerState, EGCGCardZone Zone = EGCGCardZone::None);

	/**
	 * Rest all cards in a zone (set bIsActive = false)
	 * @param PlayerState The player state
	 * @param Zone The zone to rest
	 * @return Number of cards rested
	 */
	UFUNCTION(BlueprintCallable, Category = "Zone Management")
	int32 RestAllCards(AGCGPlayerState* PlayerState, EGCGCardZone Zone);

	/**
	 * Clear damage from all cards in a zone
	 * @param PlayerState The player state
	 * @param Zone The zone to clear damage from
	 * @return Number of cards with damage cleared
	 */
	UFUNCTION(BlueprintCallable, Category = "Zone Management")
	int32 ClearAllDamage(AGCGPlayerState* PlayerState, EGCGCardZone Zone);

	// ===== HELPER FUNCTIONS =====

	/**
	 * Get zone name as string (for logging/debugging)
	 * @param Zone The zone
	 * @return Human-readable zone name
	 */
	UFUNCTION(BlueprintPure, Category = "Zone Management")
	static FString GetZoneName(EGCGCardZone Zone);

	/**
	 * Check if a zone is public (visible to all players)
	 * @param Zone The zone to check
	 * @return True if zone is public
	 */
	UFUNCTION(BlueprintPure, Category = "Zone Management")
	static bool IsZonePublic(EGCGCardZone Zone);

	/**
	 * Check if a zone is ordered (cards have specific positions)
	 * @param Zone The zone to check
	 * @return True if zone is ordered (Deck, ResourceDeck, ShieldStack)
	 */
	UFUNCTION(BlueprintPure, Category = "Zone Management")
	static bool IsZoneOrdered(EGCGCardZone Zone);

private:
	// ===== INTERNAL HELPERS =====

	/**
	 * Get pointer to the zone array in player state
	 * @param Zone The zone to get
	 * @param PlayerState The player state
	 * @return Pointer to zone array, or nullptr if invalid
	 */
	TArray<FGCGCardInstance>* GetZoneArray(EGCGCardZone Zone, AGCGPlayerState* PlayerState) const;

	/**
	 * Validate zone transition (some zones have special rules)
	 * @param FromZone Source zone
	 * @param ToZone Destination zone
	 * @param Card The card being moved
	 * @return True if transition is valid
	 */
	bool ValidateZoneTransition(EGCGCardZone FromZone, EGCGCardZone ToZone, const FGCGCardInstance& Card) const;

	/**
	 * Apply zone-specific rules when card enters a zone
	 * @param Card The card entering the zone
	 * @param Zone The zone being entered
	 */
	void ApplyZoneEntryRules(FGCGCardInstance& Card, EGCGCardZone Zone);

	/**
	 * Apply zone-specific rules when card leaves a zone
	 * @param Card The card leaving the zone
	 * @param Zone The zone being left
	 */
	void ApplyZoneExitRules(FGCGCardInstance& Card, EGCGCardZone Zone);
};
