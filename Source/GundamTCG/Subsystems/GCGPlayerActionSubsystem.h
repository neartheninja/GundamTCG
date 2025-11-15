// GCGPlayerActionSubsystem.h - Player Action Subsystem
// Unreal Engine 5.6 - Gundam TCG Implementation
// Handles player action validation and execution

#pragma once

#include "CoreMinimal.h"
#include "Subsystems/GameInstanceSubsystem.h"
#include "GundamTCG/GCGTypes.h"
#include "GCGPlayerActionSubsystem.generated.h"

// Forward declarations
class AGCGPlayerState;
class AGCGGameState;
class UGCGZoneSubsystem;

/**
 * Player Action Request Types
 */
UENUM(BlueprintType)
enum class EGCGPlayerActionType : uint8
{
	None,
	PlayCard,           // Play a card from hand
	ActivateAbility,    // Activate an ability on a card
	DeclareAttack,      // Declare an attack
	DeclareBlocker,     // Declare a blocker
	PassPriority,       // Pass priority to opponent
	DiscardCard,        // Discard a card from hand
	PlaceResource,      // Place a card from hand as resource (manual placement)
};

/**
 * Player Action Request
 * Contains all information needed to validate and execute a player action
 */
USTRUCT(BlueprintType)
struct FGCGPlayerActionRequest
{
	GENERATED_BODY()

	/** Type of action being requested */
	UPROPERTY(BlueprintReadWrite)
	EGCGPlayerActionType ActionType;

	/** Player making the request */
	UPROPERTY(BlueprintReadWrite)
	int32 PlayerID;

	/** Primary card involved (e.g., card being played, attacker, etc.) */
	UPROPERTY(BlueprintReadWrite)
	int32 PrimaryCardInstanceID;

	/** Secondary card involved (e.g., blocker, target, etc.) */
	UPROPERTY(BlueprintReadWrite)
	int32 SecondaryCardInstanceID;

	/** Additional parameters (ability index, etc.) */
	UPROPERTY(BlueprintReadWrite)
	TMap<FName, int32> Parameters;

	FGCGPlayerActionRequest()
		: ActionType(EGCGPlayerActionType::None)
		, PlayerID(-1)
		, PrimaryCardInstanceID(0)
		, SecondaryCardInstanceID(0)
	{}
};

/**
 * Player Action Result
 * Contains information about whether an action succeeded and why
 */
USTRUCT(BlueprintType)
struct FGCGPlayerActionResult
{
	GENERATED_BODY()

	/** Did the action succeed? */
	UPROPERTY(BlueprintReadOnly)
	bool bSuccess;

	/** Error message if action failed */
	UPROPERTY(BlueprintReadOnly)
	FString ErrorMessage;

	/** Additional result data */
	UPROPERTY(BlueprintReadOnly)
	TMap<FName, int32> ResultData;

	FGCGPlayerActionResult()
		: bSuccess(false)
		, ErrorMessage(TEXT(""))
	{}

	FGCGPlayerActionResult(bool bInSuccess, const FString& InErrorMessage = TEXT(""))
		: bSuccess(bInSuccess)
		, ErrorMessage(InErrorMessage)
	{}
};

/**
 * Player Action Subsystem
 *
 * This subsystem handles all player action validation and execution:
 * - Playing cards from hand
 * - Paying costs
 * - Activating abilities
 * - Declaring attacks/blocks
 * - Priority passing
 *
 * All player actions go through this subsystem for validation before execution.
 */
UCLASS()
class GUNDAMTCG_API UGCGPlayerActionSubsystem : public UGameInstanceSubsystem
{
	GENERATED_BODY()

public:
	// ===== SUBSYSTEM LIFECYCLE =====

	virtual void Initialize(FSubsystemCollectionBase& Collection) override;
	virtual void Deinitialize() override;

	// ===== ACTION EXECUTION =====

	/**
	 * Execute a player action request
	 * This validates the action and executes it if valid
	 * @param Request The action request
	 * @param PlayerState The player making the request
	 * @param GameState The current game state
	 * @return Action result (success/failure with error message)
	 */
	UFUNCTION(BlueprintCallable, Category = "Player Actions")
	FGCGPlayerActionResult ExecuteAction(const FGCGPlayerActionRequest& Request,
		AGCGPlayerState* PlayerState, AGCGGameState* GameState);

	// ===== ACTION VALIDATION =====

	/**
	 * Validate if a player can perform an action
	 * @param Request The action request
	 * @param PlayerState The player making the request
	 * @param GameState The current game state
	 * @return Validation result
	 */
	UFUNCTION(BlueprintPure, Category = "Player Actions")
	FGCGPlayerActionResult ValidateAction(const FGCGPlayerActionRequest& Request,
		AGCGPlayerState* PlayerState, AGCGGameState* GameState) const;

	// ===== PLAY CARD =====

	/**
	 * Play a card from hand
	 * @param CardInstanceID The card to play
	 * @param PlayerState The player playing the card
	 * @param GameState The current game state
	 * @return Action result
	 */
	UFUNCTION(BlueprintCallable, Category = "Player Actions")
	FGCGPlayerActionResult PlayCardFromHand(int32 CardInstanceID,
		AGCGPlayerState* PlayerState, AGCGGameState* GameState);

	/**
	 * Validate if player can play a card
	 * @param CardInstance The card to validate
	 * @param PlayerState The player
	 * @param GameState The current game state
	 * @return Validation result
	 */
	UFUNCTION(BlueprintPure, Category = "Player Actions")
	FGCGPlayerActionResult CanPlayCard(const FGCGCardInstance& CardInstance,
		AGCGPlayerState* PlayerState, AGCGGameState* GameState) const;

	// ===== COST PAYMENT =====

	/**
	 * Pay cost for a card or ability
	 * @param Cost The cost to pay (number of resources to rest)
	 * @param PlayerState The player paying the cost
	 * @return True if cost was successfully paid
	 */
	UFUNCTION(BlueprintCallable, Category = "Player Actions")
	bool PayCost(int32 Cost, AGCGPlayerState* PlayerState);

	/**
	 * Check if player can pay a cost
	 * @param Cost The cost to check
	 * @param PlayerState The player
	 * @return True if player has enough active resources
	 */
	UFUNCTION(BlueprintPure, Category = "Player Actions")
	bool CanPayCost(int32 Cost, AGCGPlayerState* PlayerState) const;

	// ===== RESOURCE PLACEMENT =====

	/**
	 * Place a card from hand as a resource (face-down)
	 * @param CardInstanceID The card to place as resource
	 * @param PlayerState The player
	 * @param GameState The current game state
	 * @param bFaceUp Should the resource be face-up?
	 * @return Action result
	 */
	UFUNCTION(BlueprintCallable, Category = "Player Actions")
	FGCGPlayerActionResult PlaceCardAsResource(int32 CardInstanceID,
		AGCGPlayerState* PlayerState, AGCGGameState* GameState, bool bFaceUp = false);

	// ===== DISCARD =====

	/**
	 * Discard a card from hand
	 * @param CardInstanceID The card to discard
	 * @param PlayerState The player
	 * @return True if card was discarded
	 */
	UFUNCTION(BlueprintCallable, Category = "Player Actions")
	bool DiscardCard(int32 CardInstanceID, AGCGPlayerState* PlayerState);

	/**
	 * Discard cards from hand to reach target hand size
	 * @param CardInstanceIDs The cards to discard
	 * @param PlayerState The player
	 * @param TargetHandSize The target hand size (default 10)
	 * @return Number of cards discarded
	 */
	UFUNCTION(BlueprintCallable, Category = "Player Actions")
	int32 DiscardToHandLimit(const TArray<int32>& CardInstanceIDs,
		AGCGPlayerState* PlayerState, int32 TargetHandSize = 10);

protected:
	// ===== INTERNAL VALIDATION =====

	/**
	 * Validate play timing (is it the right phase?)
	 * @param GameState The current game state
	 * @return Validation result
	 */
	FGCGPlayerActionResult ValidatePlayTiming(AGCGGameState* GameState) const;

	/**
	 * Validate player has priority
	 * @param PlayerID The player
	 * @param GameState The current game state
	 * @return Validation result
	 */
	FGCGPlayerActionResult ValidatePlayerPriority(int32 PlayerID, AGCGGameState* GameState) const;

	// ===== INTERNAL EXECUTION =====

	/**
	 * Execute play card action
	 * @param CardInstanceID The card to play
	 * @param PlayerState The player
	 * @param GameState The current game state
	 * @return Action result
	 */
	FGCGPlayerActionResult ExecutePlayCard(int32 CardInstanceID,
		AGCGPlayerState* PlayerState, AGCGGameState* GameState);

	/**
	 * Execute discard action
	 * @param CardInstanceID The card to discard
	 * @param PlayerState The player
	 * @return Action result
	 */
	FGCGPlayerActionResult ExecuteDiscard(int32 CardInstanceID, AGCGPlayerState* PlayerState);

private:
	/**
	 * Get zone subsystem
	 */
	UGCGZoneSubsystem* GetZoneSubsystem() const;
};
