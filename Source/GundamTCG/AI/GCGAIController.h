// GCGAIController.h - AI Opponent Controller
// Unreal Engine 5.6 - Gundam TCG Implementation
// Provides AI decision-making for single-player and testing

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/PlayerController.h"
#include "GundamTCG/GCGTypes.h"
#include "GCGAIController.generated.h"

// Forward declarations
class AGCGPlayerState;
class AGCGGameState;
class UGCGCardDatabase;

/**
 * AI Difficulty Level
 */
UENUM(BlueprintType)
enum class EGCGAIDifficulty : uint8
{
	Random          UMETA(DisplayName = "Random (Testing)"),
	Easy            UMETA(DisplayName = "Easy"),
	Medium          UMETA(DisplayName = "Medium"),
	Hard            UMETA(DisplayName = "Hard")
};

/**
 * AI Action Type
 */
UENUM(BlueprintType)
enum class EGCGAIActionType : uint8
{
	None            UMETA(DisplayName = "None"),
	PlayCard        UMETA(DisplayName = "Play Card"),
	PlaceResource   UMETA(DisplayName = "Place Resource"),
	Attack          UMETA(DisplayName = "Attack"),
	Block           UMETA(DisplayName = "Block"),
	ActivateAbility UMETA(DisplayName = "Activate Ability"),
	PassPriority    UMETA(DisplayName = "Pass Priority"),
	EndTurn         UMETA(DisplayName = "End Turn")
};

/**
 * AI Action (what AI wants to do)
 */
USTRUCT(BlueprintType)
struct FGCGAIAction
{
	GENERATED_BODY()

	UPROPERTY(BlueprintReadWrite, Category = "AI")
	EGCGAIActionType ActionType = EGCGAIActionType::None;

	UPROPERTY(BlueprintReadWrite, Category = "AI")
	int32 CardInstanceID = -1;

	UPROPERTY(BlueprintReadWrite, Category = "AI")
	int32 TargetInstanceID = -1;

	UPROPERTY(BlueprintReadWrite, Category = "AI")
	int32 TargetPlayerID = -1;

	UPROPERTY(BlueprintReadWrite, Category = "AI")
	float Priority = 0.0f; // Higher = more important

	UPROPERTY(BlueprintReadWrite, Category = "AI")
	FString Reason; // For debugging

	FGCGAIAction() {}

	FGCGAIAction(EGCGAIActionType Type, int32 CardID = -1, float Prio = 0.0f, FString ReasonText = TEXT(""))
		: ActionType(Type), CardInstanceID(CardID), Priority(Prio), Reason(ReasonText) {}
};

/**
 * AI Game State Evaluation
 */
USTRUCT(BlueprintType)
struct FGCGAIGameEvaluation
{
	GENERATED_BODY()

	// Advantage score (-100 to +100, negative = losing, positive = winning)
	UPROPERTY(BlueprintReadWrite, Category = "AI")
	float AdvantageScore = 0.0f;

	// Board control (0-100)
	UPROPERTY(BlueprintReadWrite, Category = "AI")
	float BoardControl = 50.0f;

	// Resource advantage (0-100)
	UPROPERTY(BlueprintReadWrite, Category = "AI")
	float ResourceAdvantage = 50.0f;

	// Card advantage (0-100)
	UPROPERTY(BlueprintReadWrite, Category = "AI")
	float CardAdvantage = 50.0f;

	// Tempo advantage (0-100)
	UPROPERTY(BlueprintReadWrite, Category = "AI")
	float TempoAdvantage = 50.0f;

	// Threat level (0-100, higher = more dangerous opponent state)
	UPROPERTY(BlueprintReadWrite, Category = "AI")
	float ThreatLevel = 50.0f;
};

/**
 * AI Controller
 *
 * Provides AI decision-making for single-player matches and testing.
 * Uses heuristic-based decision tree for card game actions.
 *
 * AI Difficulty Levels:
 * - Random: Makes random legal moves (for testing)
 * - Easy: Basic heuristics, makes obvious mistakes
 * - Medium: Decent heuristics, avoids major mistakes
 * - Hard: Advanced heuristics, near-optimal play
 */
UCLASS()
class GUNDAMTCG_API AGCGAIController : public APlayerController
{
	GENERATED_BODY()

public:
	AGCGAIController();

	// ===========================================================================================
	// INITIALIZATION
	// ===========================================================================================

	virtual void BeginPlay() override;
	virtual void Tick(float DeltaTime) override;

	/**
	 * Set AI difficulty
	 * @param Difficulty The difficulty level
	 */
	UFUNCTION(BlueprintCallable, Category = "AI")
	void SetDifficulty(EGCGAIDifficulty Difficulty);

	/**
	 * Enable/disable AI thinking delay (makes AI more human-like)
	 * @param bEnabled Enable delay
	 * @param MinDelay Minimum delay in seconds
	 * @param MaxDelay Maximum delay in seconds
	 */
	UFUNCTION(BlueprintCallable, Category = "AI")
	void SetThinkingDelay(bool bEnabled, float MinDelay = 1.0f, float MaxDelay = 3.0f);

	// ===========================================================================================
	// DECISION MAKING
	// ===========================================================================================

	/**
	 * AI decides what action to take
	 * Called when it's AI's turn to act
	 * @return The action to take
	 */
	UFUNCTION(BlueprintCallable, Category = "AI")
	FGCGAIAction DecideAction();

	/**
	 * Execute an AI action
	 * @param Action The action to execute
	 * @return True if action was executed successfully
	 */
	UFUNCTION(BlueprintCallable, Category = "AI")
	bool ExecuteAction(const FGCGAIAction& Action);

	// ===========================================================================================
	// PHASE-SPECIFIC DECISIONS
	// ===========================================================================================

	/**
	 * Decide which card to play during Main Phase
	 * @return AI action (PlayCard or PassPriority)
	 */
	UFUNCTION(BlueprintCallable, Category = "AI|Decisions")
	FGCGAIAction DecideCardToPlay();

	/**
	 * Decide whether to manually place a resource
	 * @return AI action (PlaceResource or PassPriority)
	 */
	UFUNCTION(BlueprintCallable, Category = "AI|Decisions")
	FGCGAIAction DecidePlaceResource();

	/**
	 * Decide which Unit should attack
	 * @return AI action (Attack or PassPriority)
	 */
	UFUNCTION(BlueprintCallable, Category = "AI|Decisions")
	FGCGAIAction DecideAttack();

	/**
	 * Decide whether to block an attack
	 * @param AttackIndex The attack to block
	 * @return AI action (Block or PassPriority)
	 */
	UFUNCTION(BlueprintCallable, Category = "AI|Decisions")
	FGCGAIAction DecideBlock(int32 AttackIndex);

	/**
	 * Decide which cards to discard (hand limit)
	 * @param DiscardCount How many cards to discard
	 * @return Array of card instance IDs to discard
	 */
	UFUNCTION(BlueprintCallable, Category = "AI|Decisions")
	TArray<int32> DecideDiscard(int32 DiscardCount);

	// ===========================================================================================
	// GAME STATE EVALUATION
	// ===========================================================================================

	/**
	 * Evaluate current game state
	 * @return Evaluation result
	 */
	UFUNCTION(BlueprintCallable, Category = "AI|Evaluation")
	FGCGAIGameEvaluation EvaluateGameState();

	/**
	 * Evaluate a potential card play
	 * @param CardInstance The card to evaluate
	 * @return Priority score (higher = better)
	 */
	UFUNCTION(BlueprintCallable, Category = "AI|Evaluation")
	float EvaluateCardPlay(const FGCGCardInstance& CardInstance);

	/**
	 * Evaluate a potential attack
	 * @param AttackerInstance The attacker
	 * @param TargetPlayerID The target player
	 * @return Priority score (higher = better)
	 */
	UFUNCTION(BlueprintCallable, Category = "AI|Evaluation")
	float EvaluateAttack(const FGCGCardInstance& AttackerInstance, int32 TargetPlayerID);

	/**
	 * Evaluate a potential block
	 * @param BlockerInstance The blocker
	 * @param AttackerInstance The attacker
	 * @return Priority score (higher = better)
	 */
	UFUNCTION(BlueprintCallable, Category = "AI|Evaluation")
	float EvaluateBlock(const FGCGCardInstance& BlockerInstance, const FGCGCardInstance& AttackerInstance);

	// ===========================================================================================
	// HELPER FUNCTIONS
	// ===========================================================================================

	/**
	 * Get all valid actions AI can take
	 * @return Array of valid actions
	 */
	UFUNCTION(BlueprintCallable, Category = "AI|Helpers")
	TArray<FGCGAIAction> GetValidActions();

	/**
	 * Get all cards AI can play from hand
	 * @return Array of playable cards
	 */
	UFUNCTION(BlueprintCallable, Category = "AI|Helpers")
	TArray<FGCGCardInstance> GetPlayableCards();

	/**
	 * Get all Units that can attack
	 * @return Array of attackable Units
	 */
	UFUNCTION(BlueprintCallable, Category = "AI|Helpers")
	TArray<FGCGCardInstance> GetAttackableUnits();

	/**
	 * Get all Units that can block
	 * @return Array of blocker Units
	 */
	UFUNCTION(BlueprintCallable, Category = "AI|Helpers")
	TArray<FGCGCardInstance> GetBlockerUnits();

	/**
	 * Get card value estimate (for prioritization)
	 * @param CardInstance The card
	 * @return Estimated value (0-100)
	 */
	UFUNCTION(BlueprintCallable, Category = "AI|Helpers")
	float GetCardValue(const FGCGCardInstance& CardInstance);

	/**
	 * Check if AI should pass priority
	 * @return True if should pass
	 */
	UFUNCTION(BlueprintCallable, Category = "AI|Helpers")
	bool ShouldPassPriority();

	// ===========================================================================================
	// RANDOM AI (Testing)
	// ===========================================================================================

	/**
	 * Make a random legal move
	 * @return Random action
	 */
	UFUNCTION(BlueprintCallable, Category = "AI|Random")
	FGCGAIAction MakeRandomAction();

	// ===========================================================================================
	// DEBUG
	// ===========================================================================================

	/**
	 * Log AI thinking process
	 * @param Message The message to log
	 */
	UFUNCTION(BlueprintCallable, Category = "AI|Debug")
	void LogAIThinking(const FString& Message);

	/**
	 * Enable/disable AI debug logging
	 * @param bEnabled Enable logging
	 */
	UFUNCTION(BlueprintCallable, Category = "AI|Debug")
	void SetDebugLogging(bool bEnabled);

	// ===========================================================================================
	// PROPERTIES
	// ===========================================================================================

	// AI difficulty level
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "AI")
	EGCGAIDifficulty Difficulty = EGCGAIDifficulty::Medium;

	// Enable thinking delay (makes AI more human-like)
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "AI")
	bool bUseThinkingDelay = true;

	// Minimum thinking delay (seconds)
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "AI")
	float MinThinkingDelay = 1.0f;

	// Maximum thinking delay (seconds)
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "AI")
	float MaxThinkingDelay = 3.0f;

	// Enable debug logging
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "AI")
	bool bDebugLogging = false;

protected:
	// Cached player state
	UPROPERTY()
	AGCGPlayerState* AIPlayerState = nullptr;

	// Cached game state
	UPROPERTY()
	AGCGGameState* GameState = nullptr;

	// Cached card database
	UPROPERTY()
	UGCGCardDatabase* CardDatabase = nullptr;

	// Current thinking timer
	UPROPERTY()
	float ThinkingTimer = 0.0f;

	// Is AI currently thinking
	UPROPERTY()
	bool bIsThinking = false;

	// Pending action to execute
	UPROPERTY()
	FGCGAIAction PendingAction;
};
