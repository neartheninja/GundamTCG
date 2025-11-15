// GCGValidationSubsystem.cpp - Game State Validation & Rule Enforcement Implementation
// Unreal Engine 5.6 - Gundam TCG Implementation

#include "GCGValidationSubsystem.h"
#include "GCGCardDatabase.h"
#include "GundamTCG/PlayerState/GCGPlayerState.h"
#include "GundamTCG/GameState/GCGGameState.h"

// ===========================================================================================
// INITIALIZATION
// ===========================================================================================

void UGCGValidationSubsystem::Initialize(FSubsystemCollectionBase& Collection)
{
	Super::Initialize(Collection);

	UE_LOG(LogTemp, Log, TEXT("GCGValidationSubsystem: Initialized"));

	// Cache reference to Card Database
	CardDatabase = GetGameInstance()->GetSubsystem<UGCGCardDatabase>();

	if (!CardDatabase)
	{
		UE_LOG(LogTemp, Warning, TEXT("GCGValidationSubsystem: Card Database not found!"));
	}
}

void UGCGValidationSubsystem::Deinitialize()
{
	CardDatabase = nullptr;
	Super::Deinitialize();
}

// ===========================================================================================
// FULL GAME STATE VALIDATION
// ===========================================================================================

FGCGValidationResult UGCGValidationSubsystem::ValidateGameState(AGCGGameState* GameState)
{
	FGCGValidationResult Result;

	if (!GameState)
	{
		Result.AddError(TEXT("GameState is nullptr"));
		return Result;
	}

	// Validate turn number
	if (GameState->TurnNumber < 0)
	{
		Result.AddError(FString::Printf(TEXT("Invalid turn number: %d (must be >= 0)"), GameState->TurnNumber));
	}

	// Validate active player ID
	if (GameState->bGameInProgress && (GameState->ActivePlayerID < 0 || GameState->ActivePlayerID > 3))
	{
		Result.AddError(FString::Printf(TEXT("Invalid active player ID: %d"), GameState->ActivePlayerID));
	}

	// Check for duplicate instance IDs across all players
	FGCGValidationResult DuplicateCheck = CheckForDuplicateInstanceIDs(GameState);
	if (!DuplicateCheck.bIsValid)
	{
		Result.Errors.Append(DuplicateCheck.Errors);
		Result.bIsValid = false;
	}

	LogValidationResult(Result, TEXT("GameState"));
	return Result;
}

FGCGValidationResult UGCGValidationSubsystem::ValidatePlayerState(AGCGPlayerState* PlayerState)
{
	FGCGValidationResult Result;

	if (!PlayerState)
	{
		Result.AddError(TEXT("PlayerState is nullptr"));
		return Result;
	}

	// Validate player ID
	if (PlayerState->GetPlayerID() < 0)
	{
		Result.AddError(FString::Printf(TEXT("Invalid player ID: %d"), PlayerState->GetPlayerID()));
	}

	// Validate zone limits
	FGCGValidationResult ZoneLimitResult = ValidateZoneLimits(PlayerState);
	if (!ZoneLimitResult.bIsValid)
	{
		Result.Errors.Append(ZoneLimitResult.Errors);
		Result.bIsValid = false;
	}

	// Validate Battle Area
	FGCGValidationResult BattleAreaResult = ValidateBattleArea(PlayerState);
	if (!BattleAreaResult.bIsValid)
	{
		Result.Errors.Append(BattleAreaResult.Errors);
		Result.bIsValid = false;
	}

	// Validate Resource Area
	FGCGValidationResult ResourceAreaResult = ValidateResourceArea(PlayerState);
	if (!ResourceAreaResult.bIsValid)
	{
		Result.Errors.Append(ResourceAreaResult.Errors);
		Result.bIsValid = false;
	}

	// Check for orphaned cards
	FGCGValidationResult OrphanedCardsResult = CheckForOrphanedCards(PlayerState);
	if (!OrphanedCardsResult.bIsValid)
	{
		Result.Errors.Append(OrphanedCardsResult.Errors);
		Result.bIsValid = false;
	}

	// Check for negative stats
	FGCGValidationResult NegativeStatsResult = CheckForNegativeStats(PlayerState);
	if (!NegativeStatsResult.bIsValid)
	{
		Result.Errors.Append(NegativeStatsResult.Errors);
		Result.bIsValid = false;
	}

	LogValidationResult(Result, FString::Printf(TEXT("Player %d State"), PlayerState->GetPlayerID()));
	return Result;
}

// ===========================================================================================
// ZONE VALIDATION
// ===========================================================================================

FGCGValidationResult UGCGValidationSubsystem::ValidateZoneLimits(AGCGPlayerState* PlayerState)
{
	FGCGValidationResult Result;

	if (!PlayerState)
	{
		Result.AddError(TEXT("PlayerState is nullptr"));
		return Result;
	}

	// Battle Area: Max 6 Units
	if (PlayerState->BattleArea.Num() > 6)
	{
		Result.AddError(FString::Printf(TEXT("Battle Area exceeds limit: %d > 6"), PlayerState->BattleArea.Num()));
	}

	// Resource Area: Max 15 Resources
	if (PlayerState->ResourceArea.Num() > 15)
	{
		Result.AddError(FString::Printf(TEXT("Resource Area exceeds limit: %d > 15"), PlayerState->ResourceArea.Num()));
	}

	// Hand: Warn if over 10 (should be discarded at end of turn)
	if (PlayerState->Hand.Num() > 10)
	{
		Result.AddWarning(FString::Printf(TEXT("Hand exceeds soft limit: %d > 10 (should discard at end of turn)"),
			PlayerState->Hand.Num()));
	}

	// Shield Stack: Max 6 in 1v1 (8 in 2v2, validated separately)
	if (PlayerState->ShieldStack.Num() > 6)
	{
		Result.AddWarning(FString::Printf(TEXT("Shield Stack exceeds 1v1 limit: %d > 6 (may be valid in 2v2)"),
			PlayerState->ShieldStack.Num()));
	}

	return Result;
}

FGCGValidationResult UGCGValidationSubsystem::ValidateBattleArea(AGCGPlayerState* PlayerState)
{
	FGCGValidationResult Result;

	if (!PlayerState)
	{
		Result.AddError(TEXT("PlayerState is nullptr"));
		return Result;
	}

	// Count Units and Pilots
	int32 UnitCount = 0;
	int32 PilotCount = 0;

	for (const FGCGCardInstance& Card : PlayerState->BattleArea)
	{
		if (Card.CardType == EGCGCardType::Unit)
		{
			UnitCount++;
		}
		else if (Card.CardType == EGCGCardType::Pilot)
		{
			PilotCount++;
		}
		else
		{
			Result.AddError(FString::Printf(TEXT("Invalid card type in Battle Area: %s (ID: %d)"),
				*Card.CardName.ToString(), Card.InstanceID));
		}

		// Validate card stats
		FGCGValidationResult CardResult = ValidateCardInstance(Card, PlayerState);
		if (!CardResult.bIsValid)
		{
			Result.Errors.Append(CardResult.Errors);
			Result.bIsValid = false;
		}
	}

	// Battle Area limit
	if (PlayerState->BattleArea.Num() > 6)
	{
		Result.AddError(FString::Printf(TEXT("Battle Area exceeds limit: %d > 6 (Units: %d, Pilots: %d)"),
			PlayerState->BattleArea.Num(), UnitCount, PilotCount));
	}

	return Result;
}

FGCGValidationResult UGCGValidationSubsystem::ValidateResourceArea(AGCGPlayerState* PlayerState)
{
	FGCGValidationResult Result;

	if (!PlayerState)
	{
		Result.AddError(TEXT("PlayerState is nullptr"));
		return Result;
	}

	// Check each Resource
	for (const FGCGCardInstance& Card : PlayerState->ResourceArea)
	{
		if (Card.CardType != EGCGCardType::Resource)
		{
			Result.AddError(FString::Printf(TEXT("Invalid card type in Resource Area: %s (Type: %d, ID: %d)"),
				*Card.CardName.ToString(), (int32)Card.CardType, Card.InstanceID));
		}
	}

	// Resource Area limit
	if (PlayerState->ResourceArea.Num() > 15)
	{
		Result.AddError(FString::Printf(TEXT("Resource Area exceeds limit: %d > 15"), PlayerState->ResourceArea.Num()));
	}

	return Result;
}

FGCGValidationResult UGCGValidationSubsystem::ValidateShieldStack(AGCGPlayerState* PlayerState, int32 MaxShields)
{
	FGCGValidationResult Result;

	if (!PlayerState)
	{
		Result.AddError(TEXT("PlayerState is nullptr"));
		return Result;
	}

	// Shield count
	if (PlayerState->ShieldStack.Num() > MaxShields)
	{
		Result.AddError(FString::Printf(TEXT("Shield Stack exceeds limit: %d > %d"),
			PlayerState->ShieldStack.Num(), MaxShields));
	}

	return Result;
}

// ===========================================================================================
// CARD VALIDATION
// ===========================================================================================

FGCGValidationResult UGCGValidationSubsystem::ValidateCardInstance(const FGCGCardInstance& CardInstance,
	AGCGPlayerState* PlayerState)
{
	FGCGValidationResult Result;

	// Validate instance ID
	if (CardInstance.InstanceID < 0)
	{
		Result.AddError(FString::Printf(TEXT("Invalid instance ID: %d"), CardInstance.InstanceID));
	}

	// Validate owner ID
	if (CardInstance.OwnerPlayerID < 0)
	{
		Result.AddError(FString::Printf(TEXT("Invalid owner ID: %d (Card: %s)"),
			CardInstance.OwnerPlayerID, *CardInstance.CardName.ToString()));
	}

	// Validate stats
	FGCGValidationResult StatsResult = ValidateCardStats(CardInstance);
	if (!StatsResult.bIsValid)
	{
		Result.Errors.Append(StatsResult.Errors);
		Result.bIsValid = false;
	}

	// Validate modifiers
	FGCGValidationResult ModifiersResult = ValidateCardModifiers(CardInstance);
	if (!ModifiersResult.bIsValid)
	{
		Result.Errors.Append(ModifiersResult.Errors);
		Result.bIsValid = false;
	}

	return Result;
}

FGCGValidationResult UGCGValidationSubsystem::ValidateCardStats(const FGCGCardInstance& CardInstance)
{
	FGCGValidationResult Result;

	// Check for negative stats
	if (CardInstance.AP < 0)
	{
		Result.AddError(FString::Printf(TEXT("Negative AP: %d (Card: %s, ID: %d)"),
			CardInstance.AP, *CardInstance.CardName.ToString(), CardInstance.InstanceID));
	}

	if (CardInstance.HP < 0 && CardInstance.CardType == EGCGCardType::Unit)
	{
		Result.AddError(FString::Printf(TEXT("Negative HP on Unit: %d (Card: %s, ID: %d)"),
			CardInstance.HP, *CardInstance.CardName.ToString(), CardInstance.InstanceID));
	}

	if (CardInstance.Cost < 0)
	{
		Result.AddError(FString::Printf(TEXT("Negative Cost: %d (Card: %s, ID: %d)"),
			CardInstance.Cost, *CardInstance.CardName.ToString(), CardInstance.InstanceID));
	}

	// Check damage counters
	if (CardInstance.DamageCounters < 0)
	{
		Result.AddError(FString::Printf(TEXT("Negative damage counters: %d (Card: %s, ID: %d)"),
			CardInstance.DamageCounters, *CardInstance.CardName.ToString(), CardInstance.InstanceID));
	}

	// Warn if damage exceeds HP
	if (CardInstance.DamageCounters > CardInstance.HP && CardInstance.CardType == EGCGCardType::Unit)
	{
		Result.AddWarning(FString::Printf(TEXT("Damage exceeds HP: %d > %d (Card: %s should be destroyed)"),
			CardInstance.DamageCounters, CardInstance.HP, *CardInstance.CardName.ToString()));
	}

	return Result;
}

FGCGValidationResult UGCGValidationSubsystem::ValidateCardModifiers(const FGCGCardInstance& CardInstance)
{
	FGCGValidationResult Result;

	for (const FGCGActiveModifier& Modifier : CardInstance.ActiveModifiers)
	{
		// Validate modifier source
		if (Modifier.SourceCardInstanceID < 0)
		{
			Result.AddWarning(FString::Printf(TEXT("Invalid modifier source ID: %d (Card: %s)"),
				Modifier.SourceCardInstanceID, *CardInstance.CardName.ToString()));
		}

		// Validate modifier turn
		if (Modifier.AppliedOnTurn < 0)
		{
			Result.AddWarning(FString::Printf(TEXT("Invalid modifier turn: %d (Card: %s)"),
				Modifier.AppliedOnTurn, *CardInstance.CardName.ToString()));
		}
	}

	return Result;
}

// ===========================================================================================
// DECK VALIDATION
// ===========================================================================================

FGCGValidationResult UGCGValidationSubsystem::ValidateDeckList(const FGCGDeckList& DeckList)
{
	FGCGValidationResult Result;

	// Main Deck: Must be exactly 50 cards
	if (DeckList.MainDeck.Num() != 50)
	{
		Result.AddError(FString::Printf(TEXT("Invalid Main Deck size: %d (must be exactly 50)"),
			DeckList.MainDeck.Num()));
	}

	// Resource Deck: Must be exactly 10 cards
	if (DeckList.ResourceDeck.Num() != 10)
	{
		Result.AddError(FString::Printf(TEXT("Invalid Resource Deck size: %d (must be exactly 10)"),
			DeckList.ResourceDeck.Num()));
	}

	// Check for max 4 copies of each card in Main Deck
	TMap<FName, int32> CardCounts;
	for (const FName& CardNumber : DeckList.MainDeck)
	{
		int32& Count = CardCounts.FindOrAdd(CardNumber);
		Count++;

		if (Count > 4)
		{
			Result.AddError(FString::Printf(TEXT("Too many copies of card %s: %d (max 4)"),
				*CardNumber.ToString(), Count));
		}
	}

	// Check Resource Deck (can have up to 4 copies)
	TMap<FName, int32> ResourceCounts;
	for (const FName& CardNumber : DeckList.ResourceDeck)
	{
		int32& Count = ResourceCounts.FindOrAdd(CardNumber);
		Count++;

		if (Count > 4)
		{
			Result.AddError(FString::Printf(TEXT("Too many copies of Resource %s: %d (max 4)"),
				*CardNumber.ToString(), Count));
		}
	}

	return Result;
}

FGCGValidationResult UGCGValidationSubsystem::ValidateDeckDuringGame(AGCGPlayerState* PlayerState)
{
	FGCGValidationResult Result;

	if (!PlayerState)
	{
		Result.AddError(TEXT("PlayerState is nullptr"));
		return Result;
	}

	// Count card numbers across all zones
	TMap<FName, int32> CardCounts;

	auto CountCards = [&CardCounts](const TArray<FGCGCardInstance>& Zone)
	{
		for (const FGCGCardInstance& Card : Zone)
		{
			if (!Card.bIsToken)
			{
				int32& Count = CardCounts.FindOrAdd(Card.CardNumber);
				Count++;
			}
		}
	};

	// Count all zones
	CountCards(PlayerState->Deck);
	CountCards(PlayerState->Hand);
	CountCards(PlayerState->BattleArea);
	CountCards(PlayerState->ResourceArea);
	CountCards(PlayerState->ShieldStack);
	CountCards(PlayerState->Trash);
	CountCards(PlayerState->Removal);

	// Check for more than 4 copies (excluding tokens)
	for (const auto& Pair : CardCounts)
	{
		if (Pair.Value > 4)
		{
			Result.AddError(FString::Printf(TEXT("Player %d has more than 4 copies of %s: %d"),
				PlayerState->GetPlayerID(), *Pair.Key.ToString(), Pair.Value));
		}
	}

	return Result;
}

// ===========================================================================================
// COMBAT VALIDATION
// ===========================================================================================

FGCGValidationResult UGCGValidationSubsystem::ValidateAttackDeclaration(
	const FGCGCardInstance& AttackerInstance,
	AGCGPlayerState* AttackingPlayer,
	AGCGGameState* GameState)
{
	FGCGValidationResult Result;

	if (!AttackingPlayer || !GameState)
	{
		Result.AddError(TEXT("PlayerState or GameState is nullptr"));
		return Result;
	}

	// Must be a Unit
	if (AttackerInstance.CardType != EGCGCardType::Unit)
	{
		Result.AddError(FString::Printf(TEXT("Attacker is not a Unit: %s (Type: %d)"),
			*AttackerInstance.CardName.ToString(), (int32)AttackerInstance.CardType));
	}

	// Must be active (not rested)
	if (!AttackerInstance.bIsActive)
	{
		Result.AddError(FString::Printf(TEXT("Attacker is rested: %s"), *AttackerInstance.CardName.ToString()));
	}

	// Check if already attacked this turn
	if (AttackerInstance.bHasAttackedThisTurn)
	{
		Result.AddError(FString::Printf(TEXT("Attacker already attacked this turn: %s"),
			*AttackerInstance.CardName.ToString()));
	}

	// Check summoning sickness (unless Link Unit paired)
	if (AttackerInstance.TurnDeployed == GameState->TurnNumber && AttackerInstance.PairedCardInstanceID == -1)
	{
		Result.AddError(FString::Printf(TEXT("Attacker has summoning sickness: %s (deployed turn %d, current turn %d)"),
			*AttackerInstance.CardName.ToString(), AttackerInstance.TurnDeployed, GameState->TurnNumber));
	}

	return Result;
}

FGCGValidationResult UGCGValidationSubsystem::ValidateBlockerDeclaration(
	const FGCGCardInstance& BlockerInstance,
	AGCGPlayerState* DefendingPlayer)
{
	FGCGValidationResult Result;

	if (!DefendingPlayer)
	{
		Result.AddError(TEXT("DefendingPlayer is nullptr"));
		return Result;
	}

	// Must be a Unit
	if (BlockerInstance.CardType != EGCGCardType::Unit)
	{
		Result.AddError(FString::Printf(TEXT("Blocker is not a Unit: %s (Type: %d)"),
			*BlockerInstance.CardName.ToString(), (int32)BlockerInstance.CardType));
	}

	// Must be active (not rested)
	if (!BlockerInstance.bIsActive)
	{
		Result.AddError(FString::Printf(TEXT("Blocker is rested: %s"), *BlockerInstance.CardName.ToString()));
	}

	// Must have Blocker keyword (checked in actual combat system, just warning here)
	bool bHasBlocker = false;
	for (const FGCGKeywordInstance& Keyword : BlockerInstance.Keywords)
	{
		if (Keyword.Keyword == EGCGKeyword::Blocker)
		{
			bHasBlocker = true;
			break;
		}
	}

	if (!bHasBlocker)
	{
		Result.AddWarning(FString::Printf(TEXT("Blocker does not have Blocker keyword: %s"),
			*BlockerInstance.CardName.ToString()));
	}

	return Result;
}

// ===========================================================================================
// RULE ENFORCEMENT
// ===========================================================================================

FGCGValidationResult UGCGValidationSubsystem::CheckForDuplicateInstanceIDs(AGCGGameState* GameState)
{
	FGCGValidationResult Result;

	if (!GameState)
	{
		Result.AddError(TEXT("GameState is nullptr"));
		return Result;
	}

	TSet<int32> AllInstanceIDs;
	TMap<int32, int32> DuplicateIDs; // InstanceID -> Count

	// Lambda to check zone
	auto CheckZone = [&AllInstanceIDs, &DuplicateIDs](const TArray<FGCGCardInstance>& Zone, int32 PlayerID, const FString& ZoneName)
	{
		for (const FGCGCardInstance& Card : Zone)
		{
			if (AllInstanceIDs.Contains(Card.InstanceID))
			{
				int32& Count = DuplicateIDs.FindOrAdd(Card.InstanceID);
				Count++;
			}
			else
			{
				AllInstanceIDs.Add(Card.InstanceID);
				DuplicateIDs.Add(Card.InstanceID, 1);
			}
		}
	};

	// Check all players (up to 4 for 2v2)
	for (int32 PlayerID = 0; PlayerID < 4; PlayerID++)
	{
		// Get player state (implementation depends on GameMode)
		// For now, just validate the concept
	}

	// Report duplicates
	for (const auto& Pair : DuplicateIDs)
	{
		if (Pair.Value > 1)
		{
			Result.AddError(FString::Printf(TEXT("Duplicate instance ID found: %d (appears %d times)"),
				Pair.Key, Pair.Value));
		}
	}

	return Result;
}

FGCGValidationResult UGCGValidationSubsystem::CheckForOrphanedCards(AGCGPlayerState* PlayerState)
{
	FGCGValidationResult Result;

	if (!PlayerState)
	{
		Result.AddError(TEXT("PlayerState is nullptr"));
		return Result;
	}

	// Check each zone for cards with wrong CurrentZone value
	auto CheckZone = [&Result](const TArray<FGCGCardInstance>& Zone, EGCGCardZone ExpectedZone, const FString& ZoneName)
	{
		for (const FGCGCardInstance& Card : Zone)
		{
			if (Card.CurrentZone != ExpectedZone)
			{
				Result.AddError(FString::Printf(TEXT("Card in wrong zone: %s (ID: %d) is in %s but CurrentZone = %d)"),
					*Card.CardName.ToString(), Card.InstanceID, *ZoneName, (int32)Card.CurrentZone));
			}
		}
	};

	CheckZone(PlayerState->Deck, EGCGCardZone::Deck, TEXT("Deck"));
	CheckZone(PlayerState->Hand, EGCGCardZone::Hand, TEXT("Hand"));
	CheckZone(PlayerState->BattleArea, EGCGCardZone::BattleArea, TEXT("BattleArea"));
	CheckZone(PlayerState->ResourceArea, EGCGCardZone::ResourceArea, TEXT("ResourceArea"));
	CheckZone(PlayerState->ShieldStack, EGCGCardZone::ShieldStack, TEXT("ShieldStack"));
	CheckZone(PlayerState->Trash, EGCGCardZone::Trash, TEXT("Trash"));
	CheckZone(PlayerState->Removal, EGCGCardZone::Removal, TEXT("Removal"));

	return Result;
}

FGCGValidationResult UGCGValidationSubsystem::CheckForNegativeStats(AGCGPlayerState* PlayerState)
{
	FGCGValidationResult Result;

	if (!PlayerState)
	{
		Result.AddError(TEXT("PlayerState is nullptr"));
		return Result;
	}

	// Check all zones
	auto CheckZone = [&Result](const TArray<FGCGCardInstance>& Zone, const FString& ZoneName)
	{
		for (const FGCGCardInstance& Card : Zone)
		{
			if (Card.AP < 0)
			{
				Result.AddError(FString::Printf(TEXT("Negative AP in %s: %s (AP: %d, ID: %d)"),
					*ZoneName, *Card.CardName.ToString(), Card.AP, Card.InstanceID));
			}

			if (Card.HP < 0 && Card.CardType == EGCGCardType::Unit)
			{
				Result.AddError(FString::Printf(TEXT("Negative HP in %s: %s (HP: %d, ID: %d)"),
					*ZoneName, *Card.CardName.ToString(), Card.HP, Card.InstanceID));
			}

			if (Card.Cost < 0)
			{
				Result.AddError(FString::Printf(TEXT("Negative Cost in %s: %s (Cost: %d, ID: %d)"),
					*ZoneName, *Card.CardName.ToString(), Card.Cost, Card.InstanceID));
			}
		}
	};

	CheckZone(PlayerState->Deck, TEXT("Deck"));
	CheckZone(PlayerState->Hand, TEXT("Hand"));
	CheckZone(PlayerState->BattleArea, TEXT("BattleArea"));
	CheckZone(PlayerState->ResourceArea, TEXT("ResourceArea"));
	CheckZone(PlayerState->ShieldStack, TEXT("ShieldStack"));
	CheckZone(PlayerState->Trash, TEXT("Trash"));
	CheckZone(PlayerState->Removal, TEXT("Removal"));

	return Result;
}

// ===========================================================================================
// LOGGING
// ===========================================================================================

void UGCGValidationSubsystem::LogValidationResult(const FGCGValidationResult& Result, const FString& Context)
{
	if (!bValidationLoggingEnabled)
	{
		return;
	}

	if (Result.bIsValid && Result.Warnings.Num() == 0)
	{
		UE_LOG(LogTemp, Log, TEXT("Validation [%s]: PASSED"), *Context);
	}
	else if (Result.bIsValid && Result.Warnings.Num() > 0)
	{
		UE_LOG(LogTemp, Warning, TEXT("Validation [%s]: PASSED with %d warnings"), *Context, Result.Warnings.Num());
		for (const FString& Warning : Result.Warnings)
		{
			UE_LOG(LogTemp, Warning, TEXT("  - %s"), *Warning);
		}
	}
	else
	{
		UE_LOG(LogTemp, Error, TEXT("Validation [%s]: FAILED with %d errors"), *Context, Result.Errors.Num());
		for (const FString& Error : Result.Errors)
		{
			UE_LOG(LogTemp, Error, TEXT("  - %s"), *Error);
		}
	}
}

void UGCGValidationSubsystem::SetValidationLogging(bool bEnabled)
{
	bValidationLoggingEnabled = bEnabled;
	UE_LOG(LogTemp, Log, TEXT("GCGValidationSubsystem: Logging %s"), bEnabled ? TEXT("enabled") : TEXT("disabled"));
}
