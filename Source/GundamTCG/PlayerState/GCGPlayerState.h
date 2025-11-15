// GCGPlayerState.h - Player State for Gundam Card Game
// Unreal Engine 5.6 - Gundam TCG Implementation
// This class holds all replicated player-specific state (zones, deck lists, etc.)

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/PlayerState.h"
#include "GundamTCG/GCGTypes.h"
#include "Net/UnrealNetwork.h"
#include "GCGPlayerState.generated.h"

/**
 * Player State for Gundam Card Game
 *
 * This class holds all replicated player-specific state:
 * - All card zones (Deck, Hand, Battle Area, Resource Area, etc.)
 * - Deck lists (for game setup and validation)
 * - Player ID (for turn tracking and team battle)
 * - Player-specific counters and flags
 *
 * All zone arrays are replicated so clients stay in sync with the server.
 * The UGCGZoneSubsystem operates on these arrays for all card movements.
 */
UCLASS()
class GUNDAMTCG_API AGCGPlayerState : public APlayerState
{
	GENERATED_BODY()

public:
	AGCGPlayerState();

	// ===== REPLICATION SETUP =====

	/**
	 * Configure which properties replicate
	 */
	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;

	// ===== PLAYER IDENTIFICATION =====

	/**
	 * Get player ID (0-based index)
	 * @return Player ID
	 */
	UFUNCTION(BlueprintPure, Category = "Player")
	int32 GetPlayerID() const { return PlayerID; }

	/**
	 * Set player ID (called by GameMode during setup)
	 * @param NewPlayerID The player ID to assign
	 */
	UFUNCTION(BlueprintCallable, Category = "Player")
	void SetPlayerID(int32 NewPlayerID);

	// ===== CARD ZONES =====
	// NOTE: These are public so UGCGZoneSubsystem can access them directly
	// All zones are replicated to keep clients in sync

	/**
	 * Main Deck (50 cards at start, ordered)
	 * Top card is at index 0
	 */
	UPROPERTY(Replicated, BlueprintReadWrite, Category = "Zones")
	TArray<FGCGCardInstance> Deck;

	/**
	 * Resource Deck (10 cards at start, ordered)
	 * Top card is at index 0
	 */
	UPROPERTY(Replicated, BlueprintReadWrite, Category = "Zones")
	TArray<FGCGCardInstance> ResourceDeck;

	/**
	 * Hand (cards drawn from deck)
	 * Starting hand: 5 cards
	 * Max hand size: 10 (must discard to 10 at end of turn)
	 */
	UPROPERTY(Replicated, BlueprintReadWrite, Category = "Zones")
	TArray<FGCGCardInstance> Hand;

	/**
	 * Resource Area (max 15 resources)
	 * Resources can be face-up or face-down
	 */
	UPROPERTY(Replicated, BlueprintReadWrite, Category = "Zones")
	TArray<FGCGCardInstance> ResourceArea;

	/**
	 * Battle Area (max 6 units, or 6 total for team in 2v2)
	 */
	UPROPERTY(Replicated, BlueprintReadWrite, Category = "Zones")
	TArray<FGCGCardInstance> BattleArea;

	/**
	 * Shield Stack (6 shields in 1v1, 8 shields total in 2v2, ordered)
	 * Top shield is at index 0
	 * When taking damage, shields are removed from top
	 */
	UPROPERTY(Replicated, BlueprintReadWrite, Category = "Zones")
	TArray<FGCGCardInstance> ShieldStack;

	/**
	 * Base Section (max 1 Base card)
	 * Contains either a Base card or EX Base token
	 */
	UPROPERTY(Replicated, BlueprintReadWrite, Category = "Zones")
	TArray<FGCGCardInstance> BaseSection;

	/**
	 * Trash (discard pile, public zone, unordered)
	 * Cards go here when destroyed, discarded, or used
	 */
	UPROPERTY(Replicated, BlueprintReadWrite, Category = "Zones")
	TArray<FGCGCardInstance> Trash;

	/**
	 * Removal (removed from game, private zone, unordered)
	 * Cards here cannot normally return to the game
	 */
	UPROPERTY(Replicated, BlueprintReadWrite, Category = "Zones")
	TArray<FGCGCardInstance> Removal;

	// ===== DECK LISTS (for setup and validation) =====

	/**
	 * Main deck list (card numbers)
	 * Used for deck construction and validation
	 * Must contain exactly 50 cards
	 */
	UPROPERTY(Replicated, BlueprintReadWrite, Category = "Deck Lists")
	TArray<FName> MainDeckList;

	/**
	 * Resource deck list (card numbers)
	 * Used for deck construction and validation
	 * Must contain exactly 10 cards
	 */
	UPROPERTY(Replicated, BlueprintReadWrite, Category = "Deck Lists")
	TArray<FName> ResourceDeckList;

	// ===== PLAYER FLAGS =====

	/**
	 * Has this player lost the game?
	 */
	UPROPERTY(Replicated, BlueprintReadOnly, Category = "Player Status")
	bool bHasLost;

	/**
	 * Can this player currently take actions? (turn priority)
	 */
	UPROPERTY(Replicated, BlueprintReadOnly, Category = "Player Status")
	bool bHasPriority;

	/**
	 * Has this player placed a resource this turn?
	 * (Players can only place 1 resource per turn)
	 */
	UPROPERTY(Replicated, BlueprintReadWrite, Category = "Turn Flags")
	bool bHasPlacedResourceThisTurn;

	/**
	 * Has this player drawn a card this turn?
	 * (Tracked for effects that trigger on draw)
	 */
	UPROPERTY(Replicated, BlueprintReadWrite, Category = "Turn Flags")
	bool bHasDrawnThisTurn;

	// ===== ZONE QUERIES =====

	/**
	 * Get total number of active resources (can be used for costs)
	 * @return Number of active (untapped) resources
	 */
	UFUNCTION(BlueprintPure, Category = "Resources")
	int32 GetActiveResourceCount() const;

	/**
	 * Get total number of resources (active + rested)
	 * @return Total number of resources in Resource Area
	 */
	UFUNCTION(BlueprintPure, Category = "Resources")
	int32 GetTotalResourceCount() const;

	/**
	 * Get number of shields remaining
	 * @return Number of cards in Shield Stack
	 */
	UFUNCTION(BlueprintPure, Category = "Shields")
	int32 GetShieldCount() const;

	/**
	 * Get number of units in battle area
	 * @return Number of cards in Battle Area
	 */
	UFUNCTION(BlueprintPure, Category = "Battle")
	int32 GetUnitCount() const;

	/**
	 * Get current hand size
	 * @return Number of cards in hand
	 */
	UFUNCTION(BlueprintPure, Category = "Hand")
	int32 GetHandSize() const;

	/**
	 * Get number of cards remaining in deck
	 * @return Number of cards in Deck
	 */
	UFUNCTION(BlueprintPure, Category = "Deck")
	int32 GetDeckSize() const;

	/**
	 * Get number of cards remaining in resource deck
	 * @return Number of cards in Resource Deck
	 */
	UFUNCTION(BlueprintPure, Category = "Deck")
	int32 GetResourceDeckSize() const;

	// ===== ZONE VALIDATION =====

	/**
	 * Can this player play a card with the given cost?
	 * @param Cost The cost to pay
	 * @return True if player has enough active resources
	 */
	UFUNCTION(BlueprintPure, Category = "Resources")
	bool CanPayCost(int32 Cost) const;

	/**
	 * Can this player add a unit to battle area?
	 * @return True if battle area has space
	 */
	UFUNCTION(BlueprintPure, Category = "Battle")
	bool CanAddUnitToBattle() const;

	/**
	 * Can this player add a resource?
	 * @return True if resource area has space
	 */
	UFUNCTION(BlueprintPure, Category = "Resources")
	bool CanAddResource() const;

	// ===== HELPER FUNCTIONS =====

	/**
	 * Reset turn flags (called at start of turn)
	 */
	UFUNCTION(BlueprintCallable, Category = "Turn")
	void ResetTurnFlags();

	/**
	 * Get all cards this player owns across all zones
	 * @return Array of all card instances owned by this player
	 */
	UFUNCTION(BlueprintPure, Category = "Zones")
	TArray<FGCGCardInstance> GetAllCards() const;

	/**
	 * Find a card by instance ID across all zones
	 * @param InstanceID The instance ID to find
	 * @param OutCard The found card (if any)
	 * @param OutZone The zone the card was found in
	 * @return True if card was found
	 */
	UFUNCTION(BlueprintPure, Category = "Zones")
	bool FindCardByInstanceID(int32 InstanceID, FGCGCardInstance& OutCard, EGCGCardZone& OutZone) const;

	// ===== BLUEPRINT EVENTS =====

	/**
	 * Called when a card is added to a zone
	 * @param Card The card that was added
	 * @param Zone The zone it was added to
	 */
	UFUNCTION(BlueprintImplementableEvent, Category = "Events")
	void OnCardAddedToZone(const FGCGCardInstance& Card, EGCGCardZone Zone);

	/**
	 * Called when a card is removed from a zone
	 * @param Card The card that was removed
	 * @param Zone The zone it was removed from
	 */
	UFUNCTION(BlueprintImplementableEvent, Category = "Events")
	void OnCardRemovedFromZone(const FGCGCardInstance& Card, EGCGCardZone Zone);

	/**
	 * Called when player loses the game
	 */
	UFUNCTION(BlueprintImplementableEvent, Category = "Events")
	void OnPlayerLost();

protected:
	// ===== PLAYER ID =====

	/**
	 * Player ID (0-based index: 0, 1 for 1v1; 0-3 for 2v2)
	 * This is different from PlayerId which is the unique network ID
	 */
	UPROPERTY(Replicated, BlueprintReadOnly, Category = "Player")
	int32 PlayerID;
};
