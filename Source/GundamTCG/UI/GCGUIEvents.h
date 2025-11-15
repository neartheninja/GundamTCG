// GCGUIEvents.h - UI Event System & Delegates
// Unreal Engine 5.6 - Gundam TCG Implementation
// Defines all UI events that Blueprint widgets can bind to

#pragma once

#include "CoreMinimal.h"
#include "UObject/NoExportTypes.h"
#include "GundamTCG/GCGTypes.h"
#include "GCGUIEvents.generated.h"

// Forward declarations
class AGCGPlayerState;
class AGCGGameState;

// ===========================================================================================
// UI DATA STRUCTURES
// ===========================================================================================

/**
 * UI-friendly card display data
 * Simplified version of FGCGCardInstance for UI rendering
 */
USTRUCT(BlueprintType)
struct FGCGUICardData
{
	GENERATED_BODY()

	UPROPERTY(BlueprintReadOnly, Category = "UI")
	int32 InstanceID = -1;

	UPROPERTY(BlueprintReadOnly, Category = "UI")
	FName CardNumber;

	UPROPERTY(BlueprintReadOnly, Category = "UI")
	FText CardName;

	UPROPERTY(BlueprintReadOnly, Category = "UI")
	FText CardDescription;

	UPROPERTY(BlueprintReadOnly, Category = "UI")
	EGCGCardType CardType = EGCGCardType::Unit;

	UPROPERTY(BlueprintReadOnly, Category = "UI")
	int32 AP = 0;

	UPROPERTY(BlueprintReadOnly, Category = "UI")
	int32 HP = 0;

	UPROPERTY(BlueprintReadOnly, Category = "UI")
	int32 Cost = 0;

	UPROPERTY(BlueprintReadOnly, Category = "UI")
	int32 DamageTaken = 0;

	UPROPERTY(BlueprintReadOnly, Category = "UI")
	TArray<EGCGColor> Colors;

	UPROPERTY(BlueprintReadOnly, Category = "UI")
	TArray<EGCGKeyword> Keywords;

	UPROPERTY(BlueprintReadOnly, Category = "UI")
	bool bIsActive = false;

	UPROPERTY(BlueprintReadOnly, Category = "UI")
	bool bHasAttackedThisTurn = false;

	UPROPERTY(BlueprintReadOnly, Category = "UI")
	bool bIsPaired = false;

	UPROPERTY(BlueprintReadOnly, Category = "UI")
	UTexture2D* CardArt = nullptr;

	UPROPERTY(BlueprintReadOnly, Category = "UI")
	UTexture2D* CardFrame = nullptr;

	// Constructor from FGCGCardInstance
	FGCGUICardData() {}

	FGCGUICardData(const FGCGCardInstance& CardInstance)
	{
		InstanceID = CardInstance.InstanceID;
		CardNumber = CardInstance.CardNumber;
		CardName = CardInstance.CardName;
		CardDescription = CardInstance.CardDescription;
		CardType = CardInstance.CardType;
		AP = CardInstance.AP;
		HP = CardInstance.HP;
		Cost = CardInstance.Cost;
		DamageTaken = CardInstance.DamageTaken;
		Colors = CardInstance.Colors;
		Keywords = CardInstance.ActiveKeywords;
		bIsActive = CardInstance.bIsActive;
		bHasAttackedThisTurn = CardInstance.bHasAttackedThisTurn;
		bIsPaired = (CardInstance.PairedCardInstanceID != -1);
		// CardArt and CardFrame would be loaded separately
	}
};

/**
 * UI-friendly player status data
 */
USTRUCT(BlueprintType)
struct FGCGUIPlayerStatus
{
	GENERATED_BODY()

	UPROPERTY(BlueprintReadOnly, Category = "UI")
	int32 PlayerID = -1;

	UPROPERTY(BlueprintReadOnly, Category = "UI")
	FString PlayerName;

	UPROPERTY(BlueprintReadOnly, Category = "UI")
	int32 HP = 0;

	UPROPERTY(BlueprintReadOnly, Category = "UI")
	int32 MaxHP = 0;

	UPROPERTY(BlueprintReadOnly, Category = "UI")
	int32 HandCount = 0;

	UPROPERTY(BlueprintReadOnly, Category = "UI")
	int32 DeckCount = 0;

	UPROPERTY(BlueprintReadOnly, Category = "UI")
	int32 ResourceCount = 0;

	UPROPERTY(BlueprintReadOnly, Category = "UI")
	int32 ShieldCount = 0;

	UPROPERTY(BlueprintReadOnly, Category = "UI")
	int32 BattleAreaCount = 0;

	UPROPERTY(BlueprintReadOnly, Category = "UI")
	bool bIsActivePlayer = false;

	UPROPERTY(BlueprintReadOnly, Category = "UI")
	bool bHasPriority = false;
};

/**
 * UI-friendly attack display data
 */
USTRUCT(BlueprintType)
struct FGCGUIAttackData
{
	GENERATED_BODY()

	UPROPERTY(BlueprintReadOnly, Category = "UI")
	int32 AttackIndex = -1;

	UPROPERTY(BlueprintReadOnly, Category = "UI")
	FGCGUICardData Attacker;

	UPROPERTY(BlueprintReadOnly, Category = "UI")
	FGCGUICardData Blocker;

	UPROPERTY(BlueprintReadOnly, Category = "UI")
	int32 AttackingPlayerID = -1;

	UPROPERTY(BlueprintReadOnly, Category = "UI")
	int32 DefendingPlayerID = -1;

	UPROPERTY(BlueprintReadOnly, Category = "UI")
	bool bIsBlocked = false;
};

/**
 * UI drag-drop data
 */
USTRUCT(BlueprintType)
struct FGCGUIDragDropData
{
	GENERATED_BODY()

	UPROPERTY(BlueprintReadWrite, Category = "UI")
	int32 CardInstanceID = -1;

	UPROPERTY(BlueprintReadWrite, Category = "UI")
	EGCGCardZone SourceZone = EGCGCardZone::Hand;

	UPROPERTY(BlueprintReadWrite, Category = "UI")
	EGCGCardZone TargetZone = EGCGCardZone::None;

	UPROPERTY(BlueprintReadWrite, Category = "UI")
	bool bIsValid = false;
};

// ===========================================================================================
// EVENT DELEGATES
// ===========================================================================================

/**
 * Game State Events
 */

// Called when a new turn starts
DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FOnTurnStart, int32, TurnNumber, int32, ActivePlayerID);

// Called when turn phase changes
DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FOnPhaseChange, EGCGTurnPhase, NewPhase, int32, ActivePlayerID);

// Called when priority changes
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnPriorityChange, int32, PriorityPlayerID);

// Called when game ends
DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FOnGameEnd, int32, WinnerPlayerID, FString, VictoryReason);

/**
 * Player State Events
 */

// Called when player HP changes
DECLARE_DYNAMIC_MULTICAST_DELEGATE_ThreeParams(FOnPlayerHPChanged, int32, PlayerID, int32, NewHP, int32, Delta);

// Called when a player draws cards
DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FOnCardsDrawn, int32, PlayerID, int32, CardCount);

// Called when hand size changes
DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FOnHandSizeChanged, int32, PlayerID, int32, NewHandSize);

// Called when resource count changes
DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FOnResourceCountChanged, int32, PlayerID, int32, NewResourceCount);

// Called when shield count changes
DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FOnShieldCountChanged, int32, PlayerID, int32, NewShieldCount);

/**
 * Card Events
 */

// Called when a card is played
DECLARE_DYNAMIC_MULTICAST_DELEGATE_ThreeParams(FOnCardPlayed, int32, PlayerID, const FGCGUICardData&, CardData, EGCGCardZone, ToZone);

// Called when a card moves between zones
DECLARE_DYNAMIC_MULTICAST_DELEGATE_FourParams(FOnCardMoved, int32, CardInstanceID, EGCGCardZone, FromZone, EGCGCardZone, ToZone, int32, OwnerPlayerID);

// Called when a card is destroyed
DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FOnCardDestroyed, int32, CardInstanceID, const FGCGUICardData&, CardData);

// Called when a card's stats change
DECLARE_DYNAMIC_MULTICAST_DELEGATE_FourParams(FOnCardStatsChanged, int32, CardInstanceID, int32, NewAP, int32, NewHP, int32, DamageTaken);

// Called when a card is activated/deactivated
DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FOnCardActiveStateChanged, int32, CardInstanceID, bool, bIsActive);

// Called when cards are paired (Link Unit system)
DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FOnCardsPaired, int32, LinkUnitInstanceID, int32, PilotInstanceID);

// Called when cards are unpaired
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnCardsUnpaired, int32, LinkUnitInstanceID);

/**
 * Combat Events
 */

// Called when attack is declared
DECLARE_DYNAMIC_MULTICAST_DELEGATE_ThreeParams(FOnAttackDeclared, int32, AttackerInstanceID, int32, AttackingPlayerID, int32, DefendingPlayerID);

// Called when blocker is declared
DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FOnBlockerDeclared, int32, BlockerInstanceID, int32, AttackIndex);

// Called when damage is dealt to a card
DECLARE_DYNAMIC_MULTICAST_DELEGATE_ThreeParams(FOnDamageDealtToCard, int32, CardInstanceID, int32, DamageAmount, int32, SourceInstanceID);

// Called when damage is dealt to a player
DECLARE_DYNAMIC_MULTICAST_DELEGATE_ThreeParams(FOnDamageDealtToPlayer, int32, PlayerID, int32, DamageAmount, int32, SourceInstanceID);

// Called when combat step changes
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnCombatStepChange, EGCGCombatStep, NewStep);

/**
 * Effect Events
 */

// Called when an effect is triggered
DECLARE_DYNAMIC_MULTICAST_DELEGATE_ThreeParams(FOnEffectTriggered, int32, SourceCardID, FString, EffectName, int32, TargetCardID);

// Called when a keyword is applied
DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FOnKeywordApplied, int32, CardInstanceID, EGCGKeyword, Keyword);

// Called when a modifier is applied
DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FOnModifierApplied, int32, CardInstanceID, const FGCGModifier&, Modifier);

/**
 * Input Prompt Events
 */

// Called when player needs to make a choice
DECLARE_DYNAMIC_MULTICAST_DELEGATE_ThreeParams(FOnChoiceRequired, int32, PlayerID, FString, PromptText, TArray<FString>, Options);

// Called when player needs to select a card
DECLARE_DYNAMIC_MULTICAST_DELEGATE_FourParams(FOnCardSelectionRequired, int32, PlayerID, FString, PromptText, TArray<int32>, ValidCardIDs, int32, SelectCount);

// Called when player needs to select a target
DECLARE_DYNAMIC_MULTICAST_DELEGATE_ThreeParams(FOnTargetSelectionRequired, int32, PlayerID, FString, PromptText, TArray<int32>, ValidTargetIDs);

// Called when player needs to discard cards
DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FOnDiscardRequired, int32, PlayerID, int32, DiscardCount);

/**
 * UI Interaction Events
 */

// Called when a card is hovered
DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FOnCardHovered, int32, CardInstanceID, bool, bIsHovered);

// Called when a card is selected
DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FOnCardSelected, int32, CardInstanceID, bool, bIsSelected);

// Called when a zone is clicked
DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FOnZoneClicked, EGCGCardZone, Zone, int32, PlayerID);

/**
 * UI Manager Class
 *
 * This class manages all UI events and provides helper functions for UI operations.
 * Blueprint widgets should bind to events on this manager.
 */
UCLASS(BlueprintType)
class GUNDAMTCG_API UGCGUIEventManager : public UObject
{
	GENERATED_BODY()

public:
	// ===========================================================================================
	// GAME STATE EVENTS
	// ===========================================================================================

	UPROPERTY(BlueprintAssignable, Category = "UI Events|Game State")
	FOnTurnStart OnTurnStart;

	UPROPERTY(BlueprintAssignable, Category = "UI Events|Game State")
	FOnPhaseChange OnPhaseChange;

	UPROPERTY(BlueprintAssignable, Category = "UI Events|Game State")
	FOnPriorityChange OnPriorityChange;

	UPROPERTY(BlueprintAssignable, Category = "UI Events|Game State")
	FOnGameEnd OnGameEnd;

	// ===========================================================================================
	// PLAYER STATE EVENTS
	// ===========================================================================================

	UPROPERTY(BlueprintAssignable, Category = "UI Events|Player State")
	FOnPlayerHPChanged OnPlayerHPChanged;

	UPROPERTY(BlueprintAssignable, Category = "UI Events|Player State")
	FOnCardsDrawn OnCardsDrawn;

	UPROPERTY(BlueprintAssignable, Category = "UI Events|Player State")
	FOnHandSizeChanged OnHandSizeChanged;

	UPROPERTY(BlueprintAssignable, Category = "UI Events|Player State")
	FOnResourceCountChanged OnResourceCountChanged;

	UPROPERTY(BlueprintAssignable, Category = "UI Events|Player State")
	FOnShieldCountChanged OnShieldCountChanged;

	// ===========================================================================================
	// CARD EVENTS
	// ===========================================================================================

	UPROPERTY(BlueprintAssignable, Category = "UI Events|Cards")
	FOnCardPlayed OnCardPlayed;

	UPROPERTY(BlueprintAssignable, Category = "UI Events|Cards")
	FOnCardMoved OnCardMoved;

	UPROPERTY(BlueprintAssignable, Category = "UI Events|Cards")
	FOnCardDestroyed OnCardDestroyed;

	UPROPERTY(BlueprintAssignable, Category = "UI Events|Cards")
	FOnCardStatsChanged OnCardStatsChanged;

	UPROPERTY(BlueprintAssignable, Category = "UI Events|Cards")
	FOnCardActiveStateChanged OnCardActiveStateChanged;

	UPROPERTY(BlueprintAssignable, Category = "UI Events|Cards")
	FOnCardsPaired OnCardsPaired;

	UPROPERTY(BlueprintAssignable, Category = "UI Events|Cards")
	FOnCardsUnpaired OnCardsUnpaired;

	// ===========================================================================================
	// COMBAT EVENTS
	// ===========================================================================================

	UPROPERTY(BlueprintAssignable, Category = "UI Events|Combat")
	FOnAttackDeclared OnAttackDeclared;

	UPROPERTY(BlueprintAssignable, Category = "UI Events|Combat")
	FOnBlockerDeclared OnBlockerDeclared;

	UPROPERTY(BlueprintAssignable, Category = "UI Events|Combat")
	FOnDamageDealtToCard OnDamageDealtToCard;

	UPROPERTY(BlueprintAssignable, Category = "UI Events|Combat")
	FOnDamageDealtToPlayer OnDamageDealtToPlayer;

	UPROPERTY(BlueprintAssignable, Category = "UI Events|Combat")
	FOnCombatStepChange OnCombatStepChange;

	// ===========================================================================================
	// EFFECT EVENTS
	// ===========================================================================================

	UPROPERTY(BlueprintAssignable, Category = "UI Events|Effects")
	FOnEffectTriggered OnEffectTriggered;

	UPROPERTY(BlueprintAssignable, Category = "UI Events|Effects")
	FOnKeywordApplied OnKeywordApplied;

	UPROPERTY(BlueprintAssignable, Category = "UI Events|Effects")
	FOnModifierApplied OnModifierApplied;

	// ===========================================================================================
	// INPUT PROMPT EVENTS
	// ===========================================================================================

	UPROPERTY(BlueprintAssignable, Category = "UI Events|Input")
	FOnChoiceRequired OnChoiceRequired;

	UPROPERTY(BlueprintAssignable, Category = "UI Events|Input")
	FOnCardSelectionRequired OnCardSelectionRequired;

	UPROPERTY(BlueprintAssignable, Category = "UI Events|Input")
	FOnTargetSelectionRequired OnTargetSelectionRequired;

	UPROPERTY(BlueprintAssignable, Category = "UI Events|Input")
	FOnDiscardRequired OnDiscardRequired;

	// ===========================================================================================
	// UI INTERACTION EVENTS
	// ===========================================================================================

	UPROPERTY(BlueprintAssignable, Category = "UI Events|Interaction")
	FOnCardHovered OnCardHovered;

	UPROPERTY(BlueprintAssignable, Category = "UI Events|Interaction")
	FOnCardSelected OnCardSelected;

	UPROPERTY(BlueprintAssignable, Category = "UI Events|Interaction")
	FOnZoneClicked OnZoneClicked;

	// ===========================================================================================
	// HELPER FUNCTIONS (for triggering events from C++)
	// ===========================================================================================

	/**
	 * Broadcast turn start event
	 * Call this from game mode when turn starts
	 */
	UFUNCTION(BlueprintCallable, Category = "UI Events")
	void BroadcastTurnStart(int32 TurnNumber, int32 ActivePlayerID)
	{
		OnTurnStart.Broadcast(TurnNumber, ActivePlayerID);
	}

	/**
	 * Broadcast phase change event
	 */
	UFUNCTION(BlueprintCallable, Category = "UI Events")
	void BroadcastPhaseChange(EGCGTurnPhase NewPhase, int32 ActivePlayerID)
	{
		OnPhaseChange.Broadcast(NewPhase, ActivePlayerID);
	}

	/**
	 * Broadcast card played event
	 */
	UFUNCTION(BlueprintCallable, Category = "UI Events")
	void BroadcastCardPlayed(int32 PlayerID, const FGCGUICardData& CardData, EGCGCardZone ToZone)
	{
		OnCardPlayed.Broadcast(PlayerID, CardData, ToZone);
	}

	/**
	 * Broadcast attack declared event
	 */
	UFUNCTION(BlueprintCallable, Category = "UI Events")
	void BroadcastAttackDeclared(int32 AttackerInstanceID, int32 AttackingPlayerID, int32 DefendingPlayerID)
	{
		OnAttackDeclared.Broadcast(AttackerInstanceID, AttackingPlayerID, DefendingPlayerID);
	}

	/**
	 * Broadcast damage dealt to player event
	 */
	UFUNCTION(BlueprintCallable, Category = "UI Events")
	void BroadcastDamageDealtToPlayer(int32 PlayerID, int32 DamageAmount, int32 SourceInstanceID)
	{
		OnDamageDealtToPlayer.Broadcast(PlayerID, DamageAmount, SourceInstanceID);
	}

	// Additional broadcast helpers can be added as needed
};
