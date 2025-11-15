// GCGDebugSubsystem.h - Comprehensive Debug & Logging System
// Unreal Engine 5.6 - Gundam TCG Implementation
// Provides detailed game state logging, debug commands, and testing utilities

#pragma once

#include "CoreMinimal.h"
#include "Subsystems/GameInstanceSubsystem.h"
#include "GundamTCG/GCGTypes.h"
#include "GCGDebugSubsystem.generated.h"

// Forward declarations
class AGCGPlayerState;
class AGCGGameState;
class UGCGCardDatabase;

/**
 * Debug Log Category
 */
UENUM(BlueprintType)
enum class EGCGDebugCategory : uint8
{
	All             UMETA(DisplayName = "All"),
	GameState       UMETA(DisplayName = "Game State"),
	PlayerState     UMETA(DisplayName = "Player State"),
	Combat          UMETA(DisplayName = "Combat"),
	Effects         UMETA(DisplayName = "Effects"),
	Keywords        UMETA(DisplayName = "Keywords"),
	Zones           UMETA(DisplayName = "Zones"),
	Cards           UMETA(DisplayName = "Cards"),
	Networking      UMETA(DisplayName = "Networking"),
	Validation      UMETA(DisplayName = "Validation")
};

/**
 * Debug Subsystem
 *
 * Provides comprehensive debugging and logging capabilities:
 * - Detailed game state dumps
 * - Event logging (cards played, attacks, damage, etc.)
 * - Cheat commands for testing
 * - Performance profiling
 * - Network debugging
 */
UCLASS()
class GUNDAMTCG_API UGCGDebugSubsystem : public UGameInstanceSubsystem
{
	GENERATED_BODY()

public:
	// ===========================================================================================
	// INITIALIZATION
	// ===========================================================================================

	virtual void Initialize(FSubsystemCollectionBase& Collection) override;
	virtual void Deinitialize() override;

	// ===========================================================================================
	// GAME STATE LOGGING
	// ===========================================================================================

	/**
	 * Log complete game state to console
	 * @param GameState The game state to log
	 */
	UFUNCTION(BlueprintCallable, Category = "Debug|Logging")
	void LogGameState(AGCGGameState* GameState);

	/**
	 * Log player state to console
	 * @param PlayerState The player state to log
	 * @param bDetailed Include detailed card information
	 */
	UFUNCTION(BlueprintCallable, Category = "Debug|Logging")
	void LogPlayerState(AGCGPlayerState* PlayerState, bool bDetailed = false);

	/**
	 * Log all zones for a player
	 * @param PlayerState The player state
	 */
	UFUNCTION(BlueprintCallable, Category = "Debug|Logging")
	void LogPlayerZones(AGCGPlayerState* PlayerState);

	/**
	 * Log a specific zone
	 * @param PlayerState The player state
	 * @param Zone The zone to log
	 */
	UFUNCTION(BlueprintCallable, Category = "Debug|Logging")
	void LogZone(AGCGPlayerState* PlayerState, EGCGCardZone Zone);

	/**
	 * Log a card instance
	 * @param Card The card to log
	 * @param bDetailed Include modifiers and keywords
	 */
	UFUNCTION(BlueprintCallable, Category = "Debug|Logging")
	void LogCard(const FGCGCardInstance& Card, bool bDetailed = true);

	/**
	 * Log current combat state
	 * @param GameState The game state
	 */
	UFUNCTION(BlueprintCallable, Category = "Debug|Logging")
	void LogCombatState(AGCGGameState* GameState);

	// ===========================================================================================
	// EVENT LOGGING
	// ===========================================================================================

	/**
	 * Log card played event
	 * @param PlayerID The player who played the card
	 * @param Card The card played
	 */
	UFUNCTION(BlueprintCallable, Category = "Debug|Events")
	void LogCardPlayed(int32 PlayerID, const FGCGCardInstance& Card);

	/**
	 * Log attack declared event
	 * @param AttackerID Attacker instance ID
	 * @param AttackerName Attacker name
	 * @param DefenderID Defender player ID
	 */
	UFUNCTION(BlueprintCallable, Category = "Debug|Events")
	void LogAttackDeclared(int32 AttackerID, const FString& AttackerName, int32 DefenderID);

	/**
	 * Log blocker declared event
	 * @param BlockerID Blocker instance ID
	 * @param BlockerName Blocker name
	 * @param AttackerID Attacker instance ID
	 */
	UFUNCTION(BlueprintCallable, Category = "Debug|Events")
	void LogBlockerDeclared(int32 BlockerID, const FString& BlockerName, int32 AttackerID);

	/**
	 * Log damage dealt event
	 * @param DamageAmount Amount of damage
	 * @param SourceName Source of damage
	 * @param TargetName Target of damage
	 */
	UFUNCTION(BlueprintCallable, Category = "Debug|Events")
	void LogDamageDealt(int32 DamageAmount, const FString& SourceName, const FString& TargetName);

	/**
	 * Log effect triggered event
	 * @param EffectName Name of effect
	 * @param SourceCard Source card
	 * @param TargetCard Target card
	 */
	UFUNCTION(BlueprintCallable, Category = "Debug|Events")
	void LogEffectTriggered(const FString& EffectName, const FString& SourceCard, const FString& TargetCard);

	/**
	 * Log turn/phase change event
	 * @param TurnNumber Current turn
	 * @param Phase New phase
	 * @param ActivePlayerID Active player
	 */
	UFUNCTION(BlueprintCallable, Category = "Debug|Events")
	void LogPhaseChange(int32 TurnNumber, EGCGTurnPhase Phase, int32 ActivePlayerID);

	// ===========================================================================================
	// CHEAT COMMANDS (FOR TESTING)
	// ===========================================================================================

	/**
	 * Spawn a card in player's hand
	 * @param PlayerID The player
	 * @param CardNumber The card to spawn
	 * @return True if successful
	 */
	UFUNCTION(BlueprintCallable, Category = "Debug|Cheats", meta = (DevelopmentOnly))
	bool CheatSpawnCard(int32 PlayerID, FName CardNumber);

	/**
	 * Draw X cards for a player
	 * @param PlayerID The player
	 * @param Count Number of cards to draw
	 * @return True if successful
	 */
	UFUNCTION(BlueprintCallable, Category = "Debug|Cheats", meta = (DevelopmentOnly))
	bool CheatDrawCards(int32 PlayerID, int32 Count);

	/**
	 * Add X resources to a player
	 * @param PlayerID The player
	 * @param Count Number of resources to add
	 * @return True if successful
	 */
	UFUNCTION(BlueprintCallable, Category = "Debug|Cheats", meta = (DevelopmentOnly))
	bool CheatAddResources(int32 PlayerID, int32 Count);

	/**
	 * Set a player's HP
	 * @param PlayerID The player
	 * @param HP New HP value
	 * @return True if successful
	 */
	UFUNCTION(BlueprintCallable, Category = "Debug|Cheats", meta = (DevelopmentOnly))
	bool CheatSetPlayerHP(int32 PlayerID, int32 HP);

	/**
	 * Heal all Units in Battle Area
	 * @param PlayerID The player
	 * @return True if successful
	 */
	UFUNCTION(BlueprintCallable, Category = "Debug|Cheats", meta = (DevelopmentOnly))
	bool CheatHealAllUnits(int32 PlayerID);

	/**
	 * Kill all enemy Units
	 * @param PlayerID The player (enemies of this player)
	 * @return True if successful
	 */
	UFUNCTION(BlueprintCallable, Category = "Debug|Cheats", meta = (DevelopmentOnly))
	bool CheatKillEnemyUnits(int32 PlayerID);

	/**
	 * Skip to specific phase
	 * @param Phase The phase to skip to
	 * @return True if successful
	 */
	UFUNCTION(BlueprintCallable, Category = "Debug|Cheats", meta = (DevelopmentOnly))
	bool CheatSkipToPhase(EGCGTurnPhase Phase);

	/**
	 * End current turn immediately
	 * @return True if successful
	 */
	UFUNCTION(BlueprintCallable, Category = "Debug|Cheats", meta = (DevelopmentOnly))
	bool CheatEndTurn();

	// ===========================================================================================
	// PERFORMANCE PROFILING
	// ===========================================================================================

	/**
	 * Start profiling a section
	 * @param SectionName Name of section
	 */
	UFUNCTION(BlueprintCallable, Category = "Debug|Profiling")
	void StartProfiling(const FString& SectionName);

	/**
	 * End profiling a section and log duration
	 * @param SectionName Name of section
	 */
	UFUNCTION(BlueprintCallable, Category = "Debug|Profiling")
	void EndProfiling(const FString& SectionName);

	/**
	 * Log profiling summary
	 */
	UFUNCTION(BlueprintCallable, Category = "Debug|Profiling")
	void LogProfilingSummary();

	// ===========================================================================================
	// DEBUG SETTINGS
	// ===========================================================================================

	/**
	 * Enable/disable debug logging for a category
	 * @param Category The category
	 * @param bEnabled Enable or disable
	 */
	UFUNCTION(BlueprintCallable, Category = "Debug|Settings")
	void SetDebugCategoryEnabled(EGCGDebugCategory Category, bool bEnabled);

	/**
	 * Check if a category is enabled
	 * @param Category The category
	 * @return True if enabled
	 */
	UFUNCTION(BlueprintPure, Category = "Debug|Settings")
	bool IsDebugCategoryEnabled(EGCGDebugCategory Category) const;

	/**
	 * Enable all debug categories
	 */
	UFUNCTION(BlueprintCallable, Category = "Debug|Settings")
	void EnableAllDebugCategories();

	/**
	 * Disable all debug categories
	 */
	UFUNCTION(BlueprintCallable, Category = "Debug|Settings")
	void DisableAllDebugCategories();

	// ===========================================================================================
	// HELPER FUNCTIONS
	// ===========================================================================================

	/**
	 * Get phase name as string
	 * @param Phase The phase
	 * @return Phase name
	 */
	UFUNCTION(BlueprintPure, Category = "Debug|Helpers")
	static FString GetPhaseName(EGCGTurnPhase Phase);

	/**
	 * Get zone name as string
	 * @param Zone The zone
	 * @return Zone name
	 */
	UFUNCTION(BlueprintPure, Category = "Debug|Helpers")
	static FString GetZoneName(EGCGCardZone Zone);

	/**
	 * Get card type name as string
	 * @param CardType The card type
	 * @return Card type name
	 */
	UFUNCTION(BlueprintPure, Category = "Debug|Helpers")
	static FString GetCardTypeName(EGCGCardType CardType);

	/**
	 * Get keyword name as string
	 * @param Keyword The keyword
	 * @return Keyword name
	 */
	UFUNCTION(BlueprintPure, Category = "Debug|Helpers")
	static FString GetKeywordName(EGCGKeyword Keyword);

	// ===========================================================================================
	// PROPERTIES
	// ===========================================================================================

private:
	// Cached reference to Card Database
	UPROPERTY()
	UGCGCardDatabase* CardDatabase = nullptr;

	// Debug category enable/disable flags
	UPROPERTY()
	TMap<EGCGDebugCategory, bool> CategoryEnabled;

	// Profiling timers
	UPROPERTY()
	TMap<FString, double> ProfilingStartTimes;

	UPROPERTY()
	TMap<FString, double> ProfilingTotalTimes;

	UPROPERTY()
	TMap<FString, int32> ProfilingCallCounts;

	// Event log (optional, for replay/debugging)
	UPROPERTY()
	TArray<FString> EventLog;

	// Maximum event log size
	UPROPERTY()
	int32 MaxEventLogSize = 1000;
};
