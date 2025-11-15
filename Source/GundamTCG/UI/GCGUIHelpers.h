// GCGUIHelpers.h - UI Helper Functions & Utilities
// Unreal Engine 5.6 - Gundam TCG Implementation
// Provides helper functions for converting game data to UI-friendly formats

#pragma once

#include "CoreMinimal.h"
#include "Kismet/BlueprintFunctionLibrary.h"
#include "GundamTCG/GCGTypes.h"
#include "GCGUIEvents.h"
#include "GCGUIHelpers.generated.h"

// Forward declarations
class AGCGPlayerState;
class AGCGGameState;
class UGCGCardDatabase;

/**
 * UI Helper Function Library
 *
 * Blueprint-accessible static functions for UI operations.
 * These functions convert game state data into UI-friendly formats.
 */
UCLASS()
class GUNDAMTCG_API UGCGUIHelpers : public UBlueprintFunctionLibrary
{
	GENERATED_BODY()

public:
	// ===========================================================================================
	// CARD DATA CONVERSION
	// ===========================================================================================

	/**
	 * Convert FGCGCardInstance to FGCGUICardData
	 * @param CardInstance The game card instance
	 * @param CardDatabase Optional card database for additional data
	 * @return UI-friendly card data
	 */
	UFUNCTION(BlueprintPure, Category = "UI Helpers|Cards")
	static FGCGUICardData ConvertCardToUIData(const FGCGCardInstance& CardInstance, UGCGCardDatabase* CardDatabase = nullptr);

	/**
	 * Get multiple cards as UI data
	 * @param CardInstances Array of card instances
	 * @param CardDatabase Optional card database
	 * @return Array of UI-friendly card data
	 */
	UFUNCTION(BlueprintPure, Category = "UI Helpers|Cards")
	static TArray<FGCGUICardData> ConvertCardsToUIData(const TArray<FGCGCardInstance>& CardInstances, UGCGCardDatabase* CardDatabase = nullptr);

	/**
	 * Get card color as display text
	 * @param Color The card color
	 * @return Display name
	 */
	UFUNCTION(BlueprintPure, Category = "UI Helpers|Cards")
	static FText GetColorDisplayName(EGCGColor Color);

	/**
	 * Get card color as color value (for UI tinting)
	 * @param Color The card color
	 * @return Linear color
	 */
	UFUNCTION(BlueprintPure, Category = "UI Helpers|Cards")
	static FLinearColor GetColorAsLinearColor(EGCGColor Color);

	/**
	 * Get keyword as display text
	 * @param Keyword The keyword
	 * @return Display name
	 */
	UFUNCTION(BlueprintPure, Category = "UI Helpers|Cards")
	static FText GetKeywordDisplayName(EGCGKeyword Keyword);

	/**
	 * Get keyword description (for tooltips)
	 * @param Keyword The keyword
	 * @return Description text
	 */
	UFUNCTION(BlueprintPure, Category = "UI Helpers|Cards")
	static FText GetKeywordDescription(EGCGKeyword Keyword);

	/**
	 * Get card type as display text
	 * @param CardType The card type
	 * @return Display name
	 */
	UFUNCTION(BlueprintPure, Category = "UI Helpers|Cards")
	static FText GetCardTypeDisplayName(EGCGCardType CardType);

	/**
	 * Format card stats for display (e.g., "5 AP / 8 HP")
	 * @param AP Attack power
	 * @param HP Hit points
	 * @param DamageTaken Damage taken
	 * @return Formatted string
	 */
	UFUNCTION(BlueprintPure, Category = "UI Helpers|Cards")
	static FText FormatCardStats(int32 AP, int32 HP, int32 DamageTaken);

	// ===========================================================================================
	// PLAYER DATA CONVERSION
	// ===========================================================================================

	/**
	 * Convert player state to UI status data
	 * @param PlayerState The player state
	 * @param GameState The game state (for active player check)
	 * @return UI-friendly player status
	 */
	UFUNCTION(BlueprintPure, Category = "UI Helpers|Players")
	static FGCGUIPlayerStatus ConvertPlayerToUIStatus(AGCGPlayerState* PlayerState, AGCGGameState* GameState = nullptr);

	/**
	 * Get all players as UI status data
	 * @param GameState The game state
	 * @return Array of player statuses
	 */
	UFUNCTION(BlueprintPure, Category = "UI Helpers|Players")
	static TArray<FGCGUIPlayerStatus> GetAllPlayersUIStatus(AGCGGameState* GameState);

	// ===========================================================================================
	// PHASE & ZONE DISPLAY
	// ===========================================================================================

	/**
	 * Get phase display name
	 * @param Phase The turn phase
	 * @return Display name
	 */
	UFUNCTION(BlueprintPure, Category = "UI Helpers|Game State")
	static FText GetPhaseDisplayName(EGCGTurnPhase Phase);

	/**
	 * Get phase description (for tooltips)
	 * @param Phase The turn phase
	 * @return Description text
	 */
	UFUNCTION(BlueprintPure, Category = "UI Helpers|Game State")
	static FText GetPhaseDescription(EGCGTurnPhase Phase);

	/**
	 * Get zone display name
	 * @param Zone The card zone
	 * @return Display name
	 */
	UFUNCTION(BlueprintPure, Category = "UI Helpers|Zones")
	static FText GetZoneDisplayName(EGCGCardZone Zone);

	/**
	 * Get combat step display name
	 * @param Step The combat step
	 * @return Display name
	 */
	UFUNCTION(BlueprintPure, Category = "UI Helpers|Combat")
	static FText GetCombatStepDisplayName(EGCGCombatStep Step);

	// ===========================================================================================
	// COMBAT DATA CONVERSION
	// ===========================================================================================

	/**
	 * Convert attack info to UI attack data
	 * @param AttackInfo The attack info from game state
	 * @param GameState The game state
	 * @return UI-friendly attack data
	 */
	UFUNCTION(BlueprintPure, Category = "UI Helpers|Combat")
	static FGCGUIAttackData ConvertAttackToUIData(const FGCGAttackInfo& AttackInfo, AGCGGameState* GameState);

	/**
	 * Get all pending attacks as UI data
	 * @param GameState The game state
	 * @return Array of UI-friendly attack data
	 */
	UFUNCTION(BlueprintPure, Category = "UI Helpers|Combat")
	static TArray<FGCGUIAttackData> GetAllAttacksUIData(AGCGGameState* GameState);

	// ===========================================================================================
	// VALIDATION & LEGALITY CHECKS
	// ===========================================================================================

	/**
	 * Check if a card can be played from hand
	 * @param PlayerState The player trying to play the card
	 * @param CardInstance The card to check
	 * @return True if playable
	 */
	UFUNCTION(BlueprintPure, Category = "UI Helpers|Validation")
	static bool CanPlayCard(AGCGPlayerState* PlayerState, const FGCGCardInstance& CardInstance);

	/**
	 * Check if a Unit can attack
	 * @param PlayerState The player who owns the Unit
	 * @param UnitInstance The Unit to check
	 * @param GameState The game state
	 * @return True if can attack
	 */
	UFUNCTION(BlueprintPure, Category = "UI Helpers|Validation")
	static bool CanUnitAttack(AGCGPlayerState* PlayerState, const FGCGCardInstance& UnitInstance, AGCGGameState* GameState);

	/**
	 * Check if a Unit can block
	 * @param UnitInstance The Unit to check
	 * @return True if can block
	 */
	UFUNCTION(BlueprintPure, Category = "UI Helpers|Validation")
	static bool CanUnitBlock(const FGCGCardInstance& UnitInstance);

	/**
	 * Check if it's the local player's turn
	 * @param GameState The game state
	 * @param LocalPlayerID The local player ID
	 * @return True if it's their turn
	 */
	UFUNCTION(BlueprintPure, Category = "UI Helpers|Validation")
	static bool IsLocalPlayerTurn(AGCGGameState* GameState, int32 LocalPlayerID);

	// ===========================================================================================
	// DRAG & DROP VALIDATION
	// ===========================================================================================

	/**
	 * Validate a drag-drop operation
	 * @param CardInstance The card being dragged
	 * @param SourceZone Where the card is from
	 * @param TargetZone Where the card is being dropped
	 * @param PlayerState The player performing the action
	 * @return True if valid drop
	 */
	UFUNCTION(BlueprintPure, Category = "UI Helpers|Drag Drop")
	static bool ValidateDragDrop(const FGCGCardInstance& CardInstance, EGCGCardZone SourceZone, EGCGCardZone TargetZone, AGCGPlayerState* PlayerState);

	/**
	 * Get valid drop zones for a card
	 * @param CardInstance The card being dragged
	 * @param SourceZone Where the card is from
	 * @param PlayerState The player performing the action
	 * @return Array of valid zones to drop into
	 */
	UFUNCTION(BlueprintPure, Category = "UI Helpers|Drag Drop")
	static TArray<EGCGCardZone> GetValidDropZones(const FGCGCardInstance& CardInstance, EGCGCardZone SourceZone, AGCGPlayerState* PlayerState);

	// ===========================================================================================
	// FORMATTING UTILITIES
	// ===========================================================================================

	/**
	 * Format large numbers with commas (e.g., 1000 -> "1,000")
	 * @param Number The number to format
	 * @return Formatted text
	 */
	UFUNCTION(BlueprintPure, Category = "UI Helpers|Formatting")
	static FText FormatNumber(int32 Number);

	/**
	 * Format HP display (e.g., "8/10" or "8/10 (-2)")
	 * @param CurrentHP Current HP
	 * @param MaxHP Maximum HP
	 * @param DamageTaken Damage taken
	 * @return Formatted text
	 */
	UFUNCTION(BlueprintPure, Category = "UI Helpers|Formatting")
	static FText FormatHP(int32 CurrentHP, int32 MaxHP, int32 DamageTaken);

	/**
	 * Get color for HP display (green = healthy, yellow = damaged, red = critical)
	 * @param CurrentHP Current HP
	 * @param MaxHP Maximum HP
	 * @return Color for HP text
	 */
	UFUNCTION(BlueprintPure, Category = "UI Helpers|Formatting")
	static FLinearColor GetHPColor(int32 CurrentHP, int32 MaxHP);

	// ===========================================================================================
	// CARD SEARCH & FILTERING
	// ===========================================================================================

	/**
	 * Filter cards by type
	 * @param Cards Cards to filter
	 * @param CardType Type to filter by
	 * @return Filtered cards
	 */
	UFUNCTION(BlueprintPure, Category = "UI Helpers|Filtering")
	static TArray<FGCGCardInstance> FilterCardsByType(const TArray<FGCGCardInstance>& Cards, EGCGCardType CardType);

	/**
	 * Filter cards by color
	 * @param Cards Cards to filter
	 * @param Color Color to filter by
	 * @return Filtered cards
	 */
	UFUNCTION(BlueprintPure, Category = "UI Helpers|Filtering")
	static TArray<FGCGCardInstance> FilterCardsByColor(const TArray<FGCGCardInstance>& Cards, EGCGColor Color);

	/**
	 * Filter cards by keyword
	 * @param Cards Cards to filter
	 * @param Keyword Keyword to filter by
	 * @return Filtered cards
	 */
	UFUNCTION(BlueprintPure, Category = "UI Helpers|Filtering")
	static TArray<FGCGCardInstance> FilterCardsByKeyword(const TArray<FGCGCardInstance>& Cards, EGCGKeyword Keyword);

	/**
	 * Sort cards by cost (ascending or descending)
	 * @param Cards Cards to sort
	 * @param bAscending Sort order
	 * @return Sorted cards
	 */
	UFUNCTION(BlueprintPure, Category = "UI Helpers|Filtering")
	static TArray<FGCGCardInstance> SortCardsByCost(const TArray<FGCGCardInstance>& Cards, bool bAscending = true);

	/**
	 * Sort cards by AP
	 * @param Cards Cards to sort
	 * @param bAscending Sort order
	 * @return Sorted cards
	 */
	UFUNCTION(BlueprintPure, Category = "UI Helpers|Filtering")
	static TArray<FGCGCardInstance> SortCardsByAP(const TArray<FGCGCardInstance>& Cards, bool bAscending = false);

	// ===========================================================================================
	// ANIMATION & VFX HELPERS
	// ===========================================================================================

	/**
	 * Get animation duration for zone transition
	 * @param FromZone Source zone
	 * @param ToZone Destination zone
	 * @return Duration in seconds
	 */
	UFUNCTION(BlueprintPure, Category = "UI Helpers|Animation")
	static float GetZoneTransitionDuration(EGCGCardZone FromZone, EGCGCardZone ToZone);

	/**
	 * Should this card play an enter animation?
	 * @param CardInstance The card
	 * @param ToZone The zone it's entering
	 * @return True if should animate
	 */
	UFUNCTION(BlueprintPure, Category = "UI Helpers|Animation")
	static bool ShouldPlayEnterAnimation(const FGCGCardInstance& CardInstance, EGCGCardZone ToZone);

	/**
	 * Get screen position for a zone
	 * @param Zone The zone
	 * @param PlayerID The player (for opponent zones)
	 * @param ViewportSize The viewport size
	 * @return Screen position (0-1 normalized)
	 */
	UFUNCTION(BlueprintPure, Category = "UI Helpers|Layout")
	static FVector2D GetZoneScreenPosition(EGCGCardZone Zone, int32 PlayerID, FVector2D ViewportSize);
};
