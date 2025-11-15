// GCGDebugSubsystem.cpp - Comprehensive Debug & Logging System Implementation
// Unreal Engine 5.6 - Gundam TCG Implementation

#include "GCGDebugSubsystem.h"
#include "GundamTCG/PlayerState/GCGPlayerState.h"
#include "GundamTCG/GameState/GCGGameState.h"
#include "GundamTCG/Subsystems/GCGCardDatabase.h"
#include "GundamTCG/Subsystems/GCGZoneSubsystem.h"
#include "GundamTCG/GameModes/GCGGameMode_1v1.h"
#include "Kismet/GameplayStatics.h"

// ===========================================================================================
// INITIALIZATION
// ===========================================================================================

void UGCGDebugSubsystem::Initialize(FSubsystemCollectionBase& Collection)
{
	Super::Initialize(Collection);

	// Get Card Database reference
	CardDatabase = GetGameInstance()->GetSubsystem<UGCGCardDatabase>();

	// Initialize all categories as enabled by default
	CategoryEnabled.Add(EGCGDebugCategory::All, true);
	CategoryEnabled.Add(EGCGDebugCategory::GameState, true);
	CategoryEnabled.Add(EGCGDebugCategory::PlayerState, true);
	CategoryEnabled.Add(EGCGDebugCategory::Combat, true);
	CategoryEnabled.Add(EGCGDebugCategory::Effects, true);
	CategoryEnabled.Add(EGCGDebugCategory::Keywords, true);
	CategoryEnabled.Add(EGCGDebugCategory::Zones, true);
	CategoryEnabled.Add(EGCGDebugCategory::Cards, true);
	CategoryEnabled.Add(EGCGDebugCategory::Networking, true);
	CategoryEnabled.Add(EGCGDebugCategory::Validation, true);

	UE_LOG(LogTemp, Log, TEXT("GCGDebugSubsystem initialized"));
}

void UGCGDebugSubsystem::Deinitialize()
{
	Super::Deinitialize();

	UE_LOG(LogTemp, Log, TEXT("GCGDebugSubsystem deinitialized"));
}

// ===========================================================================================
// GAME STATE LOGGING
// ===========================================================================================

void UGCGDebugSubsystem::LogGameState(AGCGGameState* GameState)
{
	if (!GameState || !IsDebugCategoryEnabled(EGCGDebugCategory::GameState))
	{
		return;
	}

	UE_LOG(LogTemp, Log, TEXT("========== GAME STATE =========="));
	UE_LOG(LogTemp, Log, TEXT("Turn Number: %d"), GameState->TurnNumber);
	UE_LOG(LogTemp, Log, TEXT("Current Phase: %s"), *GetPhaseName(GameState->CurrentPhase));
	UE_LOG(LogTemp, Log, TEXT("Active Player: %d"), GameState->ActivePlayerID);
	UE_LOG(LogTemp, Log, TEXT("Priority Player: %d"), GameState->PriorityPlayerID);
	UE_LOG(LogTemp, Log, TEXT("Game Mode: %s"), *GameState->GameModeType.ToString());

	// Log combat state
	if (GameState->PendingAttacks.Num() > 0)
	{
		UE_LOG(LogTemp, Log, TEXT("Pending Attacks: %d"), GameState->PendingAttacks.Num());
	}

	// Log all players
	UE_LOG(LogTemp, Log, TEXT("---------- PLAYERS ----------"));
	for (APlayerState* PS : GameState->PlayerArray)
	{
		AGCGPlayerState* PlayerState = Cast<AGCGPlayerState>(PS);
		if (PlayerState)
		{
			LogPlayerState(PlayerState, false);
		}
	}

	UE_LOG(LogTemp, Log, TEXT("================================"));
}

void UGCGDebugSubsystem::LogPlayerState(AGCGPlayerState* PlayerState, bool bDetailed)
{
	if (!PlayerState || !IsDebugCategoryEnabled(EGCGDebugCategory::PlayerState))
	{
		return;
	}

	UE_LOG(LogTemp, Log, TEXT("--- Player %d ---"), PlayerState->PlayerID);
	UE_LOG(LogTemp, Log, TEXT("  HP: %d/%d"), PlayerState->HP, PlayerState->MaxHP);
	UE_LOG(LogTemp, Log, TEXT("  Hand: %d cards"), PlayerState->Hand.Num());
	UE_LOG(LogTemp, Log, TEXT("  Deck: %d cards"), PlayerState->Deck.Num());
	UE_LOG(LogTemp, Log, TEXT("  Resources: %d cards"), PlayerState->ResourceArea.Num());
	UE_LOG(LogTemp, Log, TEXT("  Battle Area: %d Units"), PlayerState->BattleArea.Num());
	UE_LOG(LogTemp, Log, TEXT("  Shield Stack: %d shields"), PlayerState->ShieldStack.Num());
	UE_LOG(LogTemp, Log, TEXT("  Trash: %d cards"), PlayerState->Trash.Num());

	if (bDetailed)
	{
		LogPlayerZones(PlayerState);
	}
}

void UGCGDebugSubsystem::LogPlayerZones(AGCGPlayerState* PlayerState)
{
	if (!PlayerState || !IsDebugCategoryEnabled(EGCGDebugCategory::Zones))
	{
		return;
	}

	UE_LOG(LogTemp, Log, TEXT("========== ZONES: Player %d =========="), PlayerState->PlayerID);

	LogZone(PlayerState, EGCGCardZone::Hand);
	LogZone(PlayerState, EGCGCardZone::BattleArea);
	LogZone(PlayerState, EGCGCardZone::ResourceArea);
	LogZone(PlayerState, EGCGCardZone::ShieldStack);
	LogZone(PlayerState, EGCGCardZone::Trash);

	UE_LOG(LogTemp, Log, TEXT("======================================"));
}

void UGCGDebugSubsystem::LogZone(AGCGPlayerState* PlayerState, EGCGCardZone Zone)
{
	if (!PlayerState || !IsDebugCategoryEnabled(EGCGDebugCategory::Zones))
	{
		return;
	}

	TArray<FGCGCardInstance>* ZoneArray = nullptr;

	switch (Zone)
	{
		case EGCGCardZone::Hand:
			ZoneArray = &PlayerState->Hand;
			break;
		case EGCGCardZone::BattleArea:
			ZoneArray = &PlayerState->BattleArea;
			break;
		case EGCGCardZone::ResourceArea:
			ZoneArray = &PlayerState->ResourceArea;
			break;
		case EGCGCardZone::ShieldStack:
			ZoneArray = &PlayerState->ShieldStack;
			break;
		case EGCGCardZone::Trash:
			ZoneArray = &PlayerState->Trash;
			break;
		case EGCGCardZone::Deck:
			ZoneArray = &PlayerState->Deck;
			break;
		case EGCGCardZone::ResourceDeck:
			ZoneArray = &PlayerState->ResourceDeck;
			break;
		default:
			return;
	}

	if (!ZoneArray)
	{
		return;
	}

	UE_LOG(LogTemp, Log, TEXT("--- %s (%d cards) ---"), *GetZoneName(Zone), ZoneArray->Num());

	for (int32 i = 0; i < ZoneArray->Num(); i++)
	{
		const FGCGCardInstance& Card = (*ZoneArray)[i];
		UE_LOG(LogTemp, Log, TEXT("  [%d] %s (ID: %d, %d/%d)"),
			i, *Card.CardName.ToString(), Card.InstanceID, Card.AP, Card.HP);
	}
}

void UGCGDebugSubsystem::LogCard(const FGCGCardInstance& Card, bool bDetailed)
{
	if (!IsDebugCategoryEnabled(EGCGDebugCategory::Cards))
	{
		return;
	}

	UE_LOG(LogTemp, Log, TEXT("========== CARD: %s =========="), *Card.CardName.ToString());
	UE_LOG(LogTemp, Log, TEXT("Card Number: %s"), *Card.CardNumber.ToString());
	UE_LOG(LogTemp, Log, TEXT("Instance ID: %d"), Card.InstanceID);
	UE_LOG(LogTemp, Log, TEXT("Owner: Player %d"), Card.OwnerPlayerID);
	UE_LOG(LogTemp, Log, TEXT("Type: %s"), *GetCardTypeName(Card.CardType));
	UE_LOG(LogTemp, Log, TEXT("Zone: %s"), *GetZoneName(Card.CurrentZone));
	UE_LOG(LogTemp, Log, TEXT("AP/HP: %d/%d"), Card.AP, Card.HP);
	UE_LOG(LogTemp, Log, TEXT("Cost: %d"), Card.Cost);

	if (bDetailed)
	{
		UE_LOG(LogTemp, Log, TEXT("Active: %s"), Card.bIsActive ? TEXT("Yes") : TEXT("No"));
		UE_LOG(LogTemp, Log, TEXT("Attacked This Turn: %s"), Card.bHasAttackedThisTurn ? TEXT("Yes") : TEXT("No"));
		UE_LOG(LogTemp, Log, TEXT("Damage Taken: %d"), Card.DamageTaken);
		UE_LOG(LogTemp, Log, TEXT("Turn Deployed: %d"), Card.TurnDeployed);

		if (Card.ActiveKeywords.Num() > 0)
		{
			UE_LOG(LogTemp, Log, TEXT("Keywords:"));
			for (EGCGKeyword Keyword : Card.ActiveKeywords)
			{
				UE_LOG(LogTemp, Log, TEXT("  - %s"), *GetKeywordName(Keyword));
			}
		}

		if (Card.ActiveModifiers.Num() > 0)
		{
			UE_LOG(LogTemp, Log, TEXT("Modifiers: %d"), Card.ActiveModifiers.Num());
		}

		if (Card.PairedCardInstanceID != -1)
		{
			UE_LOG(LogTemp, Log, TEXT("Paired With: Card ID %d"), Card.PairedCardInstanceID);
		}
	}

	UE_LOG(LogTemp, Log, TEXT("================================"));
}

void UGCGDebugSubsystem::LogCombatState(AGCGGameState* GameState)
{
	if (!GameState || !IsDebugCategoryEnabled(EGCGDebugCategory::Combat))
	{
		return;
	}

	UE_LOG(LogTemp, Log, TEXT("========== COMBAT STATE =========="));
	UE_LOG(LogTemp, Log, TEXT("Pending Attacks: %d"), GameState->PendingAttacks.Num());

	for (int32 i = 0; i < GameState->PendingAttacks.Num(); i++)
	{
		const FGCGAttackInfo& Attack = GameState->PendingAttacks[i];
		UE_LOG(LogTemp, Log, TEXT("--- Attack %d ---"), i);
		UE_LOG(LogTemp, Log, TEXT("  Attacker ID: %d"), Attack.AttackerInstanceID);
		UE_LOG(LogTemp, Log, TEXT("  Attacking Player: %d"), Attack.AttackingPlayerID);
		UE_LOG(LogTemp, Log, TEXT("  Defending Player: %d"), Attack.DefendingPlayerID);
		UE_LOG(LogTemp, Log, TEXT("  Blocker ID: %d"), Attack.BlockerInstanceID);
		UE_LOG(LogTemp, Log, TEXT("  Is Blocked: %s"), Attack.bIsBlocked ? TEXT("Yes") : TEXT("No"));
	}

	UE_LOG(LogTemp, Log, TEXT("=================================="));
}

// ===========================================================================================
// EVENT LOGGING
// ===========================================================================================

void UGCGDebugSubsystem::LogCardPlayed(int32 PlayerID, const FGCGCardInstance& Card)
{
	if (!IsDebugCategoryEnabled(EGCGDebugCategory::Cards))
	{
		return;
	}

	FString LogMessage = FString::Printf(TEXT("[PLAY] Player %d played %s (ID: %d)"),
		PlayerID, *Card.CardName.ToString(), Card.InstanceID);

	UE_LOG(LogTemp, Log, TEXT("%s"), *LogMessage);

	// Add to event log
	EventLog.Add(LogMessage);
	if (EventLog.Num() > MaxEventLogSize)
	{
		EventLog.RemoveAt(0);
	}
}

void UGCGDebugSubsystem::LogAttackDeclared(int32 AttackerID, const FString& AttackerName, int32 DefenderID)
{
	if (!IsDebugCategoryEnabled(EGCGDebugCategory::Combat))
	{
		return;
	}

	FString LogMessage = FString::Printf(TEXT("[ATTACK] %s (ID: %d) attacks Player %d"),
		*AttackerName, AttackerID, DefenderID);

	UE_LOG(LogTemp, Log, TEXT("%s"), *LogMessage);

	EventLog.Add(LogMessage);
	if (EventLog.Num() > MaxEventLogSize)
	{
		EventLog.RemoveAt(0);
	}
}

void UGCGDebugSubsystem::LogBlockerDeclared(int32 BlockerID, const FString& BlockerName, int32 AttackerID)
{
	if (!IsDebugCategoryEnabled(EGCGDebugCategory::Combat))
	{
		return;
	}

	FString LogMessage = FString::Printf(TEXT("[BLOCK] %s (ID: %d) blocks Attacker ID %d"),
		*BlockerName, BlockerID, AttackerID);

	UE_LOG(LogTemp, Log, TEXT("%s"), *LogMessage);

	EventLog.Add(LogMessage);
	if (EventLog.Num() > MaxEventLogSize)
	{
		EventLog.RemoveAt(0);
	}
}

void UGCGDebugSubsystem::LogDamageDealt(int32 DamageAmount, const FString& SourceName, const FString& TargetName)
{
	if (!IsDebugCategoryEnabled(EGCGDebugCategory::Combat))
	{
		return;
	}

	FString LogMessage = FString::Printf(TEXT("[DAMAGE] %s deals %d damage to %s"),
		*SourceName, DamageAmount, *TargetName);

	UE_LOG(LogTemp, Log, TEXT("%s"), *LogMessage);

	EventLog.Add(LogMessage);
	if (EventLog.Num() > MaxEventLogSize)
	{
		EventLog.RemoveAt(0);
	}
}

void UGCGDebugSubsystem::LogEffectTriggered(const FString& EffectName, const FString& SourceCard, const FString& TargetCard)
{
	if (!IsDebugCategoryEnabled(EGCGDebugCategory::Effects))
	{
		return;
	}

	FString LogMessage = FString::Printf(TEXT("[EFFECT] %s triggered by %s (Target: %s)"),
		*EffectName, *SourceCard, *TargetCard);

	UE_LOG(LogTemp, Log, TEXT("%s"), *LogMessage);

	EventLog.Add(LogMessage);
	if (EventLog.Num() > MaxEventLogSize)
	{
		EventLog.RemoveAt(0);
	}
}

void UGCGDebugSubsystem::LogPhaseChange(int32 TurnNumber, EGCGTurnPhase Phase, int32 ActivePlayerID)
{
	if (!IsDebugCategoryEnabled(EGCGDebugCategory::GameState))
	{
		return;
	}

	FString LogMessage = FString::Printf(TEXT("[PHASE] Turn %d - %s (Active: Player %d)"),
		TurnNumber, *GetPhaseName(Phase), ActivePlayerID);

	UE_LOG(LogTemp, Log, TEXT("%s"), *LogMessage);

	EventLog.Add(LogMessage);
	if (EventLog.Num() > MaxEventLogSize)
	{
		EventLog.RemoveAt(0);
	}
}

// ===========================================================================================
// CHEAT COMMANDS (FOR TESTING)
// ===========================================================================================

bool UGCGDebugSubsystem::CheatSpawnCard(int32 PlayerID, FName CardNumber)
{
	UWorld* World = GetWorld();
	if (!World || !CardDatabase)
	{
		return false;
	}

	AGCGGameMode_1v1* GameMode = Cast<AGCGGameMode_1v1>(World->GetAuthGameMode());
	if (!GameMode)
	{
		return false;
	}

	AGCGPlayerState* PlayerState = GameMode->GetPlayerStateByID(PlayerID);
	if (!PlayerState)
	{
		return false;
	}

	UGCGZoneSubsystem* ZoneSubsystem = GetGameInstance()->GetSubsystem<UGCGZoneSubsystem>();
	if (!ZoneSubsystem)
	{
		return false;
	}

	// Get card data
	const FGCGCardData* CardData = CardDatabase->GetCardData(CardNumber);
	if (!CardData)
	{
		UE_LOG(LogTemp, Error, TEXT("Cheat: Card %s not found"), *CardNumber.ToString());
		return false;
	}

	// Create card instance
	FGCGCardInstance NewCard = ZoneSubsystem->CreateCardInstance(*CardData, PlayerID);

	// Add to hand
	PlayerState->Hand.Add(NewCard);

	UE_LOG(LogTemp, Warning, TEXT("CHEAT: Spawned %s in Player %d's hand"), *CardData->CardName.ToString(), PlayerID);
	return true;
}

bool UGCGDebugSubsystem::CheatDrawCards(int32 PlayerID, int32 Count)
{
	UWorld* World = GetWorld();
	if (!World)
	{
		return false;
	}

	AGCGGameMode_1v1* GameMode = Cast<AGCGGameMode_1v1>(World->GetAuthGameMode());
	if (!GameMode)
	{
		return false;
	}

	AGCGPlayerState* PlayerState = GameMode->GetPlayerStateByID(PlayerID);
	if (!PlayerState)
	{
		return false;
	}

	UGCGZoneSubsystem* ZoneSubsystem = GetGameInstance()->GetSubsystem<UGCGZoneSubsystem>();
	if (!ZoneSubsystem)
	{
		return false;
	}

	// Draw cards
	TArray<FGCGCardInstance> DrawnCards = ZoneSubsystem->DrawTopCards(PlayerState, EGCGCardZone::Deck, Count);

	UE_LOG(LogTemp, Warning, TEXT("CHEAT: Player %d drew %d cards"), PlayerID, DrawnCards.Num());
	return DrawnCards.Num() > 0;
}

bool UGCGDebugSubsystem::CheatAddResources(int32 PlayerID, int32 Count)
{
	UWorld* World = GetWorld();
	if (!World || !CardDatabase)
	{
		return false;
	}

	AGCGGameMode_1v1* GameMode = Cast<AGCGGameMode_1v1>(World->GetAuthGameMode());
	if (!GameMode)
	{
		return false;
	}

	AGCGPlayerState* PlayerState = GameMode->GetPlayerStateByID(PlayerID);
	if (!PlayerState)
	{
		return false;
	}

	UGCGZoneSubsystem* ZoneSubsystem = GetGameInstance()->GetSubsystem<UGCGZoneSubsystem>();
	if (!ZoneSubsystem)
	{
		return false;
	}

	// Create dummy resource tokens
	for (int32 i = 0; i < Count; i++)
	{
		FGCGCardInstance ResourceToken;
		ResourceToken.InstanceID = ZoneSubsystem->GenerateInstanceID();
		ResourceToken.OwnerPlayerID = PlayerID;
		ResourceToken.CardNumber = FName(TEXT("RESOURCE_TOKEN"));
		ResourceToken.CardName = FText::FromString(TEXT("Resource Token"));
		ResourceToken.CurrentZone = EGCGCardZone::ResourceArea;
		ResourceToken.CardType = EGCGCardType::Unit; // Dummy type

		PlayerState->ResourceArea.Add(ResourceToken);
	}

	UE_LOG(LogTemp, Warning, TEXT("CHEAT: Added %d resources to Player %d"), Count, PlayerID);
	return true;
}

bool UGCGDebugSubsystem::CheatSetPlayerHP(int32 PlayerID, int32 HP)
{
	UWorld* World = GetWorld();
	if (!World)
	{
		return false;
	}

	AGCGGameMode_1v1* GameMode = Cast<AGCGGameMode_1v1>(World->GetAuthGameMode());
	if (!GameMode)
	{
		return false;
	}

	AGCGPlayerState* PlayerState = GameMode->GetPlayerStateByID(PlayerID);
	if (!PlayerState)
	{
		return false;
	}

	PlayerState->HP = HP;

	UE_LOG(LogTemp, Warning, TEXT("CHEAT: Set Player %d HP to %d"), PlayerID, HP);
	return true;
}

bool UGCGDebugSubsystem::CheatHealAllUnits(int32 PlayerID)
{
	UWorld* World = GetWorld();
	if (!World)
	{
		return false;
	}

	AGCGGameMode_1v1* GameMode = Cast<AGCGGameMode_1v1>(World->GetAuthGameMode());
	if (!GameMode)
	{
		return false;
	}

	AGCGPlayerState* PlayerState = GameMode->GetPlayerStateByID(PlayerID);
	if (!PlayerState)
	{
		return false;
	}

	// Heal all Units in Battle Area
	for (FGCGCardInstance& Unit : PlayerState->BattleArea)
	{
		Unit.DamageTaken = 0;
	}

	UE_LOG(LogTemp, Warning, TEXT("CHEAT: Healed all Units for Player %d"), PlayerID);
	return true;
}

bool UGCGDebugSubsystem::CheatKillEnemyUnits(int32 PlayerID)
{
	UWorld* World = GetWorld();
	if (!World)
	{
		return false;
	}

	AGCGGameMode_1v1* GameMode = Cast<AGCGGameMode_1v1>(World->GetAuthGameMode());
	AGCGGameState* GameState = Cast<AGCGGameState>(World->GetGameState());
	if (!GameMode || !GameState)
	{
		return false;
	}

	UGCGZoneSubsystem* ZoneSubsystem = GetGameInstance()->GetSubsystem<UGCGZoneSubsystem>();
	if (!ZoneSubsystem)
	{
		return false;
	}

	int32 KilledCount = 0;

	// Find all enemy players
	for (APlayerState* PS : GameState->PlayerArray)
	{
		AGCGPlayerState* EnemyState = Cast<AGCGPlayerState>(PS);
		if (EnemyState && EnemyState->PlayerID != PlayerID)
		{
			// Kill all Units in Battle Area
			TArray<FGCGCardInstance> UnitsToKill = EnemyState->BattleArea;
			for (const FGCGCardInstance& Unit : UnitsToKill)
			{
				ZoneSubsystem->MoveCard(EnemyState, Unit.InstanceID, EGCGCardZone::BattleArea, EGCGCardZone::Trash);
				KilledCount++;
			}
		}
	}

	UE_LOG(LogTemp, Warning, TEXT("CHEAT: Killed %d enemy Units"), KilledCount);
	return KilledCount > 0;
}

bool UGCGDebugSubsystem::CheatSkipToPhase(EGCGTurnPhase Phase)
{
	UWorld* World = GetWorld();
	if (!World)
	{
		return false;
	}

	AGCGGameState* GameState = Cast<AGCGGameState>(World->GetGameState());
	if (!GameState)
	{
		return false;
	}

	GameState->CurrentPhase = Phase;

	UE_LOG(LogTemp, Warning, TEXT("CHEAT: Skipped to phase %s"), *GetPhaseName(Phase));
	return true;
}

bool UGCGDebugSubsystem::CheatEndTurn()
{
	UWorld* World = GetWorld();
	if (!World)
	{
		return false;
	}

	AGCGGameMode_1v1* GameMode = Cast<AGCGGameMode_1v1>(World->GetAuthGameMode());
	if (!GameMode)
	{
		return false;
	}

	GameMode->EndTurn();

	UE_LOG(LogTemp, Warning, TEXT("CHEAT: Ended turn"));
	return true;
}

// ===========================================================================================
// PERFORMANCE PROFILING
// ===========================================================================================

void UGCGDebugSubsystem::StartProfiling(const FString& SectionName)
{
	double CurrentTime = FPlatformTime::Seconds();
	ProfilingStartTimes.Add(SectionName, CurrentTime);

	UE_LOG(LogTemp, Log, TEXT("[PROFILE START] %s"), *SectionName);
}

void UGCGDebugSubsystem::EndProfiling(const FString& SectionName)
{
	double* StartTime = ProfilingStartTimes.Find(SectionName);
	if (!StartTime)
	{
		UE_LOG(LogTemp, Warning, TEXT("[PROFILE] Section %s was not started"), *SectionName);
		return;
	}

	double EndTime = FPlatformTime::Seconds();
	double Duration = (EndTime - *StartTime) * 1000.0; // Convert to milliseconds

	// Update totals
	double* TotalTime = ProfilingTotalTimes.Find(SectionName);
	if (TotalTime)
	{
		*TotalTime += Duration;
	}
	else
	{
		ProfilingTotalTimes.Add(SectionName, Duration);
	}

	// Update call count
	int32* CallCount = ProfilingCallCounts.Find(SectionName);
	if (CallCount)
	{
		(*CallCount)++;
	}
	else
	{
		ProfilingCallCounts.Add(SectionName, 1);
	}

	UE_LOG(LogTemp, Log, TEXT("[PROFILE END] %s - Duration: %.3f ms"), *SectionName, Duration);

	// Remove from start times
	ProfilingStartTimes.Remove(SectionName);
}

void UGCGDebugSubsystem::LogProfilingSummary()
{
	UE_LOG(LogTemp, Log, TEXT("========== PROFILING SUMMARY =========="));

	for (const TPair<FString, double>& Pair : ProfilingTotalTimes)
	{
		const FString& SectionName = Pair.Key;
		double TotalTime = Pair.Value;
		int32 CallCount = ProfilingCallCounts.FindRef(SectionName);
		double AverageTime = (CallCount > 0) ? (TotalTime / CallCount) : 0.0;

		UE_LOG(LogTemp, Log, TEXT("%s:"), *SectionName);
		UE_LOG(LogTemp, Log, TEXT("  Total: %.3f ms"), TotalTime);
		UE_LOG(LogTemp, Log, TEXT("  Calls: %d"), CallCount);
		UE_LOG(LogTemp, Log, TEXT("  Average: %.3f ms"), AverageTime);
	}

	UE_LOG(LogTemp, Log, TEXT("======================================="));
}

// ===========================================================================================
// DEBUG SETTINGS
// ===========================================================================================

void UGCGDebugSubsystem::SetDebugCategoryEnabled(EGCGDebugCategory Category, bool bEnabled)
{
	CategoryEnabled.Add(Category, bEnabled);

	UE_LOG(LogTemp, Log, TEXT("Debug category %s: %s"),
		*UEnum::GetValueAsString(Category), bEnabled ? TEXT("ENABLED") : TEXT("DISABLED"));
}

bool UGCGDebugSubsystem::IsDebugCategoryEnabled(EGCGDebugCategory Category) const
{
	// Check if "All" is disabled
	const bool* AllEnabled = CategoryEnabled.Find(EGCGDebugCategory::All);
	if (AllEnabled && !(*AllEnabled))
	{
		return false;
	}

	// Check specific category
	const bool* CategoryValue = CategoryEnabled.Find(Category);
	return CategoryValue ? *CategoryValue : true;
}

void UGCGDebugSubsystem::EnableAllDebugCategories()
{
	for (TPair<EGCGDebugCategory, bool>& Pair : CategoryEnabled)
	{
		Pair.Value = true;
	}

	UE_LOG(LogTemp, Log, TEXT("All debug categories ENABLED"));
}

void UGCGDebugSubsystem::DisableAllDebugCategories()
{
	for (TPair<EGCGDebugCategory, bool>& Pair : CategoryEnabled)
	{
		Pair.Value = false;
	}

	UE_LOG(LogTemp, Log, TEXT("All debug categories DISABLED"));
}

// ===========================================================================================
// HELPER FUNCTIONS
// ===========================================================================================

FString UGCGDebugSubsystem::GetPhaseName(EGCGTurnPhase Phase)
{
	return UEnum::GetValueAsString(Phase);
}

FString UGCGDebugSubsystem::GetZoneName(EGCGCardZone Zone)
{
	return UEnum::GetValueAsString(Zone);
}

FString UGCGDebugSubsystem::GetCardTypeName(EGCGCardType CardType)
{
	return UEnum::GetValueAsString(CardType);
}

FString UGCGDebugSubsystem::GetKeywordName(EGCGKeyword Keyword)
{
	return UEnum::GetValueAsString(Keyword);
}
