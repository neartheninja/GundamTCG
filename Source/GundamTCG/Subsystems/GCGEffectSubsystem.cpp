// GCGEffectSubsystem.cpp - Gundam Card Game Effect Processing Subsystem Implementation

#include "GCGEffectSubsystem.h"
#include "GCGZoneSubsystem.h"
#include "GCGCombatSubsystem.h"
#include "GCGKeywordSubsystem.h"
#include "../PlayerState/GCGPlayerState.h"
#include "../GameState/GCGGameState.h"
#include "../GameModes/GCGGameModeBase.h"

// ===========================================================================================
// SUBSYSTEM LIFECYCLE
// ===========================================================================================

void UGCGEffectSubsystem::Initialize(FSubsystemCollectionBase& Collection)
{
	Super::Initialize(Collection);
	UE_LOG(LogTemp, Log, TEXT("[GCGEffectSubsystem] Initialized"));
}

void UGCGEffectSubsystem::Deinitialize()
{
	Super::Deinitialize();
	UE_LOG(LogTemp, Log, TEXT("[GCGEffectSubsystem] Deinitialized"));
}

// ===========================================================================================
// EFFECT TRIGGERING
// ===========================================================================================

TArray<FGCGEffectResult> UGCGEffectSubsystem::TriggerEffects(EGCGEffectTiming Timing, const FGCGEffectContext& Context, AGCGGameState* GameState)
{
	TArray<FGCGEffectResult> Results;

	if (!GameState)
	{
		UE_LOG(LogTemp, Error, TEXT("[GCGEffectSubsystem] TriggerEffects: Invalid GameState"));
		return Results;
	}

	// Get all players
	AGCGPlayerState* Player1 = GetPlayerByID(1, GameState);
	AGCGPlayerState* Player2 = GetPlayerByID(2, GameState);

	if (!Player1 || !Player2)
	{
		UE_LOG(LogTemp, Error, TEXT("[GCGEffectSubsystem] TriggerEffects: Players not found"));
		return Results;
	}

	// Check all cards in play for effects with this timing
	TArray<AGCGPlayerState*> Players = {Player1, Player2};

	for (AGCGPlayerState* Player : Players)
	{
		// Check Battle Area
		for (const FGCGCardInstance& Card : Player->BattleArea)
		{
			TArray<FGCGEffectResult> CardResults = TriggerCardEffects(Card, Timing, Context, Player, GameState);
			Results.Append(CardResults);
		}

		// Check Base Section
		for (const FGCGCardInstance& Card : Player->BaseSection)
		{
			TArray<FGCGEffectResult> CardResults = TriggerCardEffects(Card, Timing, Context, Player, GameState);
			Results.Append(CardResults);
		}
	}

	return Results;
}

TArray<FGCGEffectResult> UGCGEffectSubsystem::TriggerCardEffects(const FGCGCardInstance& CardInstance,
	EGCGEffectTiming Timing, const FGCGEffectContext& Context, AGCGPlayerState* SourcePlayer, AGCGGameState* GameState)
{
	TArray<FGCGEffectResult> Results;

	if (!SourcePlayer || !GameState)
	{
		return Results;
	}

	// Check all effects on the card
	for (const FGCGEffectData& Effect : CardInstance.Effects)
	{
		// Check if effect timing matches
		if (Effect.Timing != Timing)
		{
			continue;
		}

		// Execute the effect
		FGCGEffectResult Result = ExecuteEffect(Effect, Context, SourcePlayer, GameState);
		Results.Add(Result);
	}

	return Results;
}

FGCGEffectResult UGCGEffectSubsystem::ExecuteEffect(const FGCGEffectData& Effect, const FGCGEffectContext& Context,
	AGCGPlayerState* SourcePlayer, AGCGGameState* GameState)
{
	if (!SourcePlayer || !GameState)
	{
		return FGCGEffectResult(false, FText::FromString(TEXT("Invalid player or game state")));
	}

	// Check conditions
	if (!CheckConditions(Effect.Conditions, Context, SourcePlayer, GameState))
	{
		return FGCGEffectResult(false, FText::FromString(TEXT("Conditions not met")));
	}

	// Check costs
	if (!CanPayCosts(Effect.Costs, Context, SourcePlayer, GameState))
	{
		return FGCGEffectResult(false, FText::FromString(TEXT("Cannot pay costs")));
	}

	// Pay costs
	if (!PayCosts(Effect.Costs, Context, SourcePlayer, GameState))
	{
		return FGCGEffectResult(false, FText::FromString(TEXT("Failed to pay costs")));
	}

	// Execute operations
	FGCGEffectResult Result = ExecuteOperations(Effect.Operations, Context, SourcePlayer, GameState);
	Result.bSuccess = true;

	LogEffect(TEXT("ExecuteEffect"), FString::Printf(TEXT("Effect executed: %s"), *Effect.Description.ToString()));

	return Result;
}

// ===========================================================================================
// EFFECT VALIDATION
// ===========================================================================================

bool UGCGEffectSubsystem::CheckConditions(const TArray<FGCGEffectCondition>& Conditions, const FGCGEffectContext& Context,
	AGCGPlayerState* SourcePlayer, AGCGGameState* GameState)
{
	// No conditions = always true
	if (Conditions.Num() == 0)
	{
		return true;
	}

	// All conditions must be met
	for (const FGCGEffectCondition& Condition : Conditions)
	{
		if (!CheckCondition(Condition, Context, SourcePlayer, GameState))
		{
			return false;
		}
	}

	return true;
}

bool UGCGEffectSubsystem::CheckCondition(const FGCGEffectCondition& Condition, const FGCGEffectContext& Context,
	AGCGPlayerState* SourcePlayer, AGCGGameState* GameState)
{
	if (!SourcePlayer || !GameState)
	{
		return false;
	}

	FName ConditionType = Condition.ConditionType;

	// YourTurn - Check if it's the source player's turn
	if (ConditionType == FName(TEXT("YourTurn")))
	{
		return (GameState->ActivePlayerID == Context.SourcePlayerID);
	}

	// OpponentTurn - Check if it's opponent's turn
	if (ConditionType == FName(TEXT("OpponentTurn")))
	{
		return (GameState->ActivePlayerID != Context.SourcePlayerID);
	}

	// HasActiveResources - Check if player has X active resources
	if (ConditionType == FName(TEXT("HasActiveResources")))
	{
		if (Condition.Parameters.Num() > 0)
		{
			int32 RequiredResources = FCString::Atoi(*Condition.Parameters[0]);
			return (SourcePlayer->GetActiveResourceCount() >= RequiredResources);
		}
	}

	// TODO Phase 8: Add more condition types as needed
	// - HasLessHP, HasMoreHP
	// - UnitInPlay, UnitNotInPlay
	// - ShieldsRemaining
	// etc.

	UE_LOG(LogTemp, Warning, TEXT("[GCGEffectSubsystem] Unknown condition type: %s"), *ConditionType.ToString());
	return false;
}

// ===========================================================================================
// EFFECT COSTS
// ===========================================================================================

bool UGCGEffectSubsystem::CanPayCosts(const TArray<FGCGEffectCost>& Costs, const FGCGEffectContext& Context,
	AGCGPlayerState* SourcePlayer, AGCGGameState* GameState)
{
	// No costs = always true
	if (Costs.Num() == 0)
	{
		return true;
	}

	// Check all costs
	for (const FGCGEffectCost& Cost : Costs)
	{
		FName CostType = Cost.CostType;

		// RestResources - Check if enough active resources
		if (CostType == FName(TEXT("RestResources")))
		{
			if (SourcePlayer->GetActiveResourceCount() < Cost.Amount)
			{
				return false;
			}
		}

		// RestThisUnit - Check if source card is active
		else if (CostType == FName(TEXT("RestThisUnit")))
		{
			FGCGCardInstance CardInstance;
			EGCGCardZone Zone;
			if (SourcePlayer->FindCardByInstanceID(Context.SourceCardInstanceID, CardInstance, Zone))
			{
				if (!CardInstance.bIsActive)
				{
					return false; // Card already rested
				}
			}
			else
			{
				return false; // Card not found
			}
		}

		// TODO Phase 8: Add more cost types as needed
	}

	return true;
}

bool UGCGEffectSubsystem::PayCosts(const TArray<FGCGEffectCost>& Costs, const FGCGEffectContext& Context,
	AGCGPlayerState* SourcePlayer, AGCGGameState* GameState)
{
	if (!CanPayCosts(Costs, Context, SourcePlayer, GameState))
	{
		return false;
	}

	// Pay all costs
	for (const FGCGEffectCost& Cost : Costs)
	{
		if (!PayCost(Cost, Context, SourcePlayer, GameState))
		{
			return false;
		}
	}

	return true;
}

bool UGCGEffectSubsystem::PayCost(const FGCGEffectCost& Cost, const FGCGEffectContext& Context,
	AGCGPlayerState* SourcePlayer, AGCGGameState* GameState)
{
	if (!SourcePlayer || !GameState)
	{
		return false;
	}

	FName CostType = Cost.CostType;

	// RestResources - Rest X resources
	if (CostType == FName(TEXT("RestResources")))
	{
		int32 Remaining = Cost.Amount;
		for (FGCGCardInstance& Resource : SourcePlayer->ResourceArea)
		{
			if (Remaining <= 0)
			{
				break;
			}

			if (Resource.bIsActive)
			{
				Resource.bIsActive = false; // Rest the resource
				Remaining--;
			}
		}

		return (Remaining == 0);
	}

	// RestThisUnit - Rest the source card
	else if (CostType == FName(TEXT("RestThisUnit")))
	{
		FGCGCardInstance* CardInstance = nullptr;
		EGCGCardZone Zone;
		FGCGCardInstance TempCard;

		if (SourcePlayer->FindCardByInstanceID(Context.SourceCardInstanceID, TempCard, Zone))
		{
			// Find and rest the card
			if (Zone == EGCGCardZone::BattleArea)
			{
				for (FGCGCardInstance& Card : SourcePlayer->BattleArea)
				{
					if (Card.InstanceID == Context.SourceCardInstanceID)
					{
						Card.bIsActive = false;
						return true;
					}
				}
			}
		}

		return false;
	}

	// TrashSelf - Move source card to Trash
	else if (CostType == FName(TEXT("TrashSelf")))
	{
		UGCGZoneSubsystem* ZoneSubsystem = GetGameInstance()->GetSubsystem<UGCGZoneSubsystem>();
		if (!ZoneSubsystem)
		{
			return false;
		}

		FGCGCardInstance CardInstance;
		EGCGCardZone Zone;
		if (SourcePlayer->FindCardByInstanceID(Context.SourceCardInstanceID, CardInstance, Zone))
		{
			return ZoneSubsystem->MoveCard(CardInstance, Zone, EGCGCardZone::Trash, SourcePlayer, GameState);
		}

		return false;
	}

	UE_LOG(LogTemp, Warning, TEXT("[GCGEffectSubsystem] Unknown cost type: %s"), *CostType.ToString());
	return false;
}

// ===========================================================================================
// EFFECT OPERATIONS
// ===========================================================================================

FGCGEffectResult UGCGEffectSubsystem::ExecuteOperations(const TArray<FGCGEffectOperation>& Operations,
	const FGCGEffectContext& Context, AGCGPlayerState* SourcePlayer, AGCGGameState* GameState)
{
	FGCGEffectResult CombinedResult(true);

	for (const FGCGEffectOperation& Operation : Operations)
	{
		FGCGEffectResult OpResult = ExecuteOperation(Operation, Context, SourcePlayer, GameState);

		// Combine results
		CombinedResult.CardsDrawn += OpResult.CardsDrawn;
		CombinedResult.DamageDealt += OpResult.DamageDealt;
		CombinedResult.UnitsDestroyed += OpResult.UnitsDestroyed;
		CombinedResult.APGranted += OpResult.APGranted;
		CombinedResult.AffectedCardIDs.Append(OpResult.AffectedCardIDs);
	}

	return CombinedResult;
}

FGCGEffectResult UGCGEffectSubsystem::ExecuteOperation(const FGCGEffectOperation& Operation,
	const FGCGEffectContext& Context, AGCGPlayerState* SourcePlayer, AGCGGameState* GameState)
{
	if (!SourcePlayer || !GameState)
	{
		return FGCGEffectResult(false, FText::FromString(TEXT("Invalid player or game state")));
	}

	FName OperationType = Operation.OperationType;

	// Draw - Draw cards
	if (OperationType == FName(TEXT("Draw")))
	{
		AGCGPlayerState* TargetPlayer = nullptr;
		int32 TargetCardID = 0;
		if (ResolveTarget(Operation.Target, Context, SourcePlayer, GameState, TargetPlayer, TargetCardID))
		{
			return OP_DrawCards(Operation.Amount, TargetPlayer, GameState);
		}
	}

	// DealDamageToUnit - Damage a unit
	else if (OperationType == FName(TEXT("DealDamageToUnit")))
	{
		AGCGPlayerState* TargetPlayer = nullptr;
		int32 TargetCardID = 0;
		if (ResolveTarget(Operation.Target, Context, SourcePlayer, GameState, TargetPlayer, TargetCardID))
		{
			return OP_DealDamageToUnit(Operation.Amount, TargetCardID, TargetPlayer, GameState);
		}
	}

	// DealDamageToPlayer - Damage opponent
	else if (OperationType == FName(TEXT("DealDamageToPlayer")))
	{
		AGCGPlayerState* TargetPlayer = nullptr;
		int32 TargetCardID = 0;
		if (ResolveTarget(Operation.Target, Context, SourcePlayer, GameState, TargetPlayer, TargetCardID))
		{
			return OP_DealDamageToPlayer(Operation.Amount, TargetPlayer, GameState);
		}
	}

	// DestroyUnit - Destroy a unit
	else if (OperationType == FName(TEXT("DestroyUnit")))
	{
		AGCGPlayerState* TargetPlayer = nullptr;
		int32 TargetCardID = 0;
		if (ResolveTarget(Operation.Target, Context, SourcePlayer, GameState, TargetPlayer, TargetCardID))
		{
			return OP_DestroyUnit(TargetCardID, TargetPlayer, GameState);
		}
	}

	// GiveAP - Grant AP buff
	else if (OperationType == FName(TEXT("GiveAP")))
	{
		AGCGPlayerState* TargetPlayer = nullptr;
		int32 TargetCardID = 0;
		if (ResolveTarget(Operation.Target, Context, SourcePlayer, GameState, TargetPlayer, TargetCardID))
		{
			return OP_GiveAP(Operation.Amount, Operation.Duration, TargetCardID, TargetPlayer,
				Context.SourceCardInstanceID, GameState);
		}
	}

	// GiveHP - Grant HP buff
	else if (OperationType == FName(TEXT("GiveHP")))
	{
		AGCGPlayerState* TargetPlayer = nullptr;
		int32 TargetCardID = 0;
		if (ResolveTarget(Operation.Target, Context, SourcePlayer, GameState, TargetPlayer, TargetCardID))
		{
			return OP_GiveHP(Operation.Amount, Operation.Duration, TargetCardID, TargetPlayer,
				Context.SourceCardInstanceID, GameState);
		}
	}

	// GrantKeyword - Grant keyword to card
	else if (OperationType == FName(TEXT("GrantKeyword")))
	{
		// Parse keyword from parameters
		if (Operation.Parameters.Num() > 0)
		{
			FString KeywordName = Operation.Parameters[0];
			EGCGKeyword Keyword = EGCGKeyword::None;

			// Convert string to keyword enum
			if (KeywordName == TEXT("Blocker")) Keyword = EGCGKeyword::Blocker;
			else if (KeywordName == TEXT("FirstStrike")) Keyword = EGCGKeyword::FirstStrike;
			else if (KeywordName == TEXT("Repair")) Keyword = EGCGKeyword::Repair;
			// TODO: Add more keywords

			if (Keyword != EGCGKeyword::None)
			{
				AGCGPlayerState* TargetPlayer = nullptr;
				int32 TargetCardID = 0;
				if (ResolveTarget(Operation.Target, Context, SourcePlayer, GameState, TargetPlayer, TargetCardID))
				{
					return OP_GrantKeyword(Keyword, Operation.Amount, TargetCardID, TargetPlayer,
						Context.SourceCardInstanceID);
				}
			}
		}
	}

	UE_LOG(LogTemp, Warning, TEXT("[GCGEffectSubsystem] Unknown operation type: %s"), *OperationType.ToString());
	return FGCGEffectResult(false, FText::FromString(TEXT("Unknown operation type")));
}

// ===========================================================================================
// SPECIFIC OPERATIONS
// ===========================================================================================

FGCGEffectResult UGCGEffectSubsystem::OP_DrawCards(int32 Amount, AGCGPlayerState* TargetPlayer, AGCGGameState* GameState)
{
	FGCGEffectResult Result(true);

	if (!TargetPlayer || !GameState)
	{
		Result.bSuccess = false;
		return Result;
	}

	UGCGZoneSubsystem* ZoneSubsystem = GetGameInstance()->GetSubsystem<UGCGZoneSubsystem>();
	if (!ZoneSubsystem)
	{
		Result.bSuccess = false;
		return Result;
	}

	// Draw cards
	for (int32 i = 0; i < Amount; i++)
	{
		if (TargetPlayer->Deck.Num() == 0)
		{
			// Deck empty - player loses
			TargetPlayer->bHasLost = true;
			UE_LOG(LogTemp, Warning, TEXT("[GCGEffectSubsystem] Player %d lost (drew from empty deck)"),
				TargetPlayer->GetPlayerID());
			break;
		}

		FGCGCardInstance DrawnCard = ZoneSubsystem->DrawTopCard(TargetPlayer);
		if (DrawnCard.InstanceID != 0)
		{
			Result.CardsDrawn++;
			Result.AffectedCardIDs.Add(DrawnCard.InstanceID);
		}
	}

	LogEffect(TEXT("DrawCards"), FString::Printf(TEXT("Player %d drew %d cards"),
		TargetPlayer->GetPlayerID(), Result.CardsDrawn));

	return Result;
}

FGCGEffectResult UGCGEffectSubsystem::OP_DealDamageToUnit(int32 Amount, int32 TargetInstanceID,
	AGCGPlayerState* TargetPlayer, AGCGGameState* GameState)
{
	FGCGEffectResult Result(true);

	if (!TargetPlayer || !GameState)
	{
		Result.bSuccess = false;
		return Result;
	}

	// Find unit in Battle Area
	for (FGCGCardInstance& Unit : TargetPlayer->BattleArea)
	{
		if (Unit.InstanceID == TargetInstanceID)
		{
			Unit.CurrentDamage += Amount;
			Result.DamageDealt = Amount;
			Result.AffectedCardIDs.Add(TargetInstanceID);

			// FAQ Q97-99: Track damage source (effect damage vs battle damage)
			Unit.LastDamageSource = EGCGDamageSource::EffectDamage;

			LogEffect(TEXT("DealDamageToUnit"), FString::Printf(TEXT("Dealt %d damage to %s (%d/%d HP)"),
				Amount, *Unit.CardName.ToString(), Unit.CurrentDamage, Unit.HP));

			// Check if unit destroyed
			if (Unit.IsDestroyed())
			{
				Result.UnitsDestroyed = 1;
				// TODO: Actually destroy the unit (move to Trash)
			}

			return Result;
		}
	}

	Result.bSuccess = false;
	return Result;
}

FGCGEffectResult UGCGEffectSubsystem::OP_DealDamageToPlayer(int32 Amount, AGCGPlayerState* TargetPlayer, AGCGGameState* GameState)
{
	FGCGEffectResult Result(true);

	if (!TargetPlayer || !GameState)
	{
		Result.bSuccess = false;
		return Result;
	}

	UGCGCombatSubsystem* CombatSubsystem = GetGameInstance()->GetSubsystem<UGCGCombatSubsystem>();
	if (!CombatSubsystem)
	{
		Result.bSuccess = false;
		return Result;
	}

	// Use combat subsystem to deal player damage (handles shields and base)
	int32 ShieldsBroken = 0;
	bool bPlayerLost = CombatSubsystem->DealDamageToPlayer(Amount, TargetPlayer, GameState, ShieldsBroken);

	Result.ShieldsBroken = ShieldsBroken;
	Result.DamageDealt = Amount;

	LogEffect(TEXT("DealDamageToPlayer"), FString::Printf(TEXT("Dealt %d damage to player %d (Shields broken: %d)"),
		Amount, TargetPlayer->GetPlayerID(), ShieldsBroken));

	return Result;
}

FGCGEffectResult UGCGEffectSubsystem::OP_DestroyUnit(int32 TargetInstanceID, AGCGPlayerState* TargetPlayer, AGCGGameState* GameState)
{
	FGCGEffectResult Result(true);

	if (!TargetPlayer || !GameState)
	{
		Result.bSuccess = false;
		return Result;
	}

	UGCGZoneSubsystem* ZoneSubsystem = GetGameInstance()->GetSubsystem<UGCGZoneSubsystem>();
	if (!ZoneSubsystem)
	{
		Result.bSuccess = false;
		return Result;
	}

	// Find and destroy unit
	FGCGCardInstance CardToDestroy;
	EGCGCardZone Zone;
	if (TargetPlayer->FindCardByInstanceID(TargetInstanceID, CardToDestroy, Zone))
	{
		if (Zone == EGCGCardZone::BattleArea)
		{
			bool bMoved = ZoneSubsystem->MoveCard(CardToDestroy, Zone, EGCGCardZone::Trash, TargetPlayer, GameState);
			if (bMoved)
			{
				Result.UnitsDestroyed = 1;
				Result.AffectedCardIDs.Add(TargetInstanceID);

				LogEffect(TEXT("DestroyUnit"), FString::Printf(TEXT("Destroyed %s"),
					*CardToDestroy.CardName.ToString()));

				return Result;
			}
		}
	}

	Result.bSuccess = false;
	return Result;
}

FGCGEffectResult UGCGEffectSubsystem::OP_GiveAP(int32 Amount, EGCGModifierDuration Duration, int32 TargetInstanceID,
	AGCGPlayerState* TargetPlayer, int32 SourceInstanceID, AGCGGameState* GameState)
{
	FGCGEffectResult Result(true);

	if (!TargetPlayer || !GameState)
	{
		Result.bSuccess = false;
		return Result;
	}

	// Find target card
	FGCGCardInstance* TargetCard = nullptr;

	for (FGCGCardInstance& Card : TargetPlayer->BattleArea)
	{
		if (Card.InstanceID == TargetInstanceID)
		{
			TargetCard = &Card;
			break;
		}
	}

	if (TargetCard)
	{
		AddModifier(*TargetCard, FName(TEXT("AP")), Amount, Duration, SourceInstanceID, GameState);
		Result.APGranted = Amount;
		Result.AffectedCardIDs.Add(TargetInstanceID);

		LogEffect(TEXT("GiveAP"), FString::Printf(TEXT("Granted +%d AP to %s"),
			Amount, *TargetCard->CardName.ToString()));

		return Result;
	}

	Result.bSuccess = false;
	return Result;
}

FGCGEffectResult UGCGEffectSubsystem::OP_GiveHP(int32 Amount, EGCGModifierDuration Duration, int32 TargetInstanceID,
	AGCGPlayerState* TargetPlayer, int32 SourceInstanceID, AGCGGameState* GameState)
{
	FGCGEffectResult Result(true);

	if (!TargetPlayer || !GameState)
	{
		Result.bSuccess = false;
		return Result;
	}

	// Find target card
	FGCGCardInstance* TargetCard = nullptr;

	for (FGCGCardInstance& Card : TargetPlayer->BattleArea)
	{
		if (Card.InstanceID == TargetInstanceID)
		{
			TargetCard = &Card;
			break;
		}
	}

	if (TargetCard)
	{
		AddModifier(*TargetCard, FName(TEXT("HP")), Amount, Duration, SourceInstanceID, GameState);
		Result.AffectedCardIDs.Add(TargetInstanceID);

		LogEffect(TEXT("GiveHP"), FString::Printf(TEXT("Granted +%d HP to %s"),
			Amount, *TargetCard->CardName.ToString()));

		return Result;
	}

	Result.bSuccess = false;
	return Result;
}

FGCGEffectResult UGCGEffectSubsystem::OP_GrantKeyword(EGCGKeyword Keyword, int32 Value, int32 TargetInstanceID,
	AGCGPlayerState* TargetPlayer, int32 SourceInstanceID)
{
	FGCGEffectResult Result(true);

	if (!TargetPlayer)
	{
		Result.bSuccess = false;
		return Result;
	}

	// Find target card
	FGCGCardInstance* TargetCard = nullptr;

	for (FGCGCardInstance& Card : TargetPlayer->BattleArea)
	{
		if (Card.InstanceID == TargetInstanceID)
		{
			TargetCard = &Card;
			break;
		}
	}

	if (TargetCard)
	{
		// Add keyword
		FGCGKeywordInstance NewKeyword(Keyword, Value, SourceInstanceID);
		TargetCard->TemporaryKeywords.Add(NewKeyword);
		Result.AffectedCardIDs.Add(TargetInstanceID);

		LogEffect(TEXT("GrantKeyword"), FString::Printf(TEXT("Granted keyword to %s"),
			*TargetCard->CardName.ToString()));

		return Result;
	}

	Result.bSuccess = false;
	return Result;
}

// ===========================================================================================
// MODIFIER MANAGEMENT
// ===========================================================================================

void UGCGEffectSubsystem::AddModifier(FGCGCardInstance& Card, FName ModifierType, int32 Amount,
	EGCGModifierDuration Duration, int32 SourceInstanceID, AGCGGameState* GameState)
{
	if (!GameState)
	{
		return;
	}

	FGCGActiveModifier Modifier;
	Modifier.ModifierType = ModifierType;
	Modifier.Amount = Amount;
	Modifier.Duration = Duration;
	Modifier.SourceInstanceID = SourceInstanceID;
	Modifier.CreatedOnTurn = GameState->TurnNumber;

	Card.ActiveModifiers.Add(Modifier);

	UE_LOG(LogTemp, Log, TEXT("[GCGEffectSubsystem] Added modifier: %s +%d to card %s (Duration: %d)"),
		*ModifierType.ToString(), Amount, *Card.CardName.ToString(), (int32)Duration);
}

void UGCGEffectSubsystem::RemoveModifiersBySource(FGCGCardInstance& Card, int32 SourceInstanceID)
{
	Card.ActiveModifiers.RemoveAll([SourceInstanceID](const FGCGActiveModifier& Modifier)
	{
		return Modifier.SourceInstanceID == SourceInstanceID;
	});
}

void UGCGEffectSubsystem::CleanupExpiredModifiers(FGCGCardInstance& Card, AGCGGameState* GameState,
	bool bEndOfTurn, bool bEndOfBattle)
{
	if (!GameState)
	{
		return;
	}

	Card.ActiveModifiers.RemoveAll([bEndOfTurn, bEndOfBattle, GameState](const FGCGActiveModifier& Modifier)
	{
		// Remove instant modifiers (shouldn't be in array anyway)
		if (Modifier.Duration == EGCGModifierDuration::Instant)
		{
			return true;
		}

		// Remove end of turn modifiers
		if (bEndOfTurn && Modifier.Duration == EGCGModifierDuration::UntilEndOfTurn)
		{
			return true;
		}

		// Remove end of battle modifiers
		if (bEndOfBattle && Modifier.Duration == EGCGModifierDuration::UntilEndOfBattle)
		{
			return true;
		}

		return false;
	});
}

void UGCGEffectSubsystem::CleanupAllModifiers(AGCGPlayerState* PlayerState, AGCGGameState* GameState,
	bool bEndOfTurn, bool bEndOfBattle)
{
	if (!PlayerState || !GameState)
	{
		return;
	}

	// Cleanup modifiers in Battle Area
	for (FGCGCardInstance& Card : PlayerState->BattleArea)
	{
		CleanupExpiredModifiers(Card, GameState, bEndOfTurn, bEndOfBattle);
	}

	// Cleanup modifiers in Base Section
	for (FGCGCardInstance& Card : PlayerState->BaseSection)
	{
		CleanupExpiredModifiers(Card, GameState, bEndOfTurn, bEndOfBattle);
	}

	// Cleanup temporary keywords at end of turn
	if (bEndOfTurn)
	{
		for (FGCGCardInstance& Card : PlayerState->BattleArea)
		{
			Card.TemporaryKeywords.Empty();
		}
	}
}

// ===========================================================================================
// UTILITY
// ===========================================================================================

AGCGPlayerState* UGCGEffectSubsystem::GetPlayerByID(int32 PlayerID, AGCGGameState* GameState)
{
	if (!GameState)
	{
		return nullptr;
	}

	// Get GameMode to access player states
	UWorld* World = GetWorld();
	if (!World)
	{
		return nullptr;
	}

	AGCGGameModeBase* GameMode = World->GetAuthGameMode<AGCGGameModeBase>();
	if (!GameMode)
	{
		return nullptr;
	}

	return GameMode->GetPlayerStateByID(PlayerID);
}

AGCGPlayerState* UGCGEffectSubsystem::GetOpponentPlayer(int32 CurrentPlayerID, AGCGGameState* GameState)
{
	int32 OpponentID = (CurrentPlayerID == 1) ? 2 : 1;
	return GetPlayerByID(OpponentID, GameState);
}

void UGCGEffectSubsystem::LogEffect(const FString& EffectName, const FString& Message) const
{
	UE_LOG(LogTemp, Log, TEXT("[GCGEffectSubsystem] %s: %s"), *EffectName, *Message);
}

// ===========================================================================================
// INTERNAL HELPERS
// ===========================================================================================

bool UGCGEffectSubsystem::ResolveTarget(FName TargetName, const FGCGEffectContext& Context, AGCGPlayerState* SourcePlayer,
	AGCGGameState* GameState, AGCGPlayerState*& OutPlayerState, int32& OutCardInstanceID)
{
	OutPlayerState = nullptr;
	OutCardInstanceID = 0;

	// Self - Source card
	if (TargetName == FName(TEXT("Self")))
	{
		OutPlayerState = SourcePlayer;
		OutCardInstanceID = Context.SourceCardInstanceID;
		return true;
	}

	// SourcePlayer - Source player
	if (TargetName == FName(TEXT("SourcePlayer")))
	{
		OutPlayerState = SourcePlayer;
		return true;
	}

	// OpponentPlayer - Opponent player
	if (TargetName == FName(TEXT("OpponentPlayer")))
	{
		OutPlayerState = GetOpponentPlayer(Context.SourcePlayerID, GameState);
		return (OutPlayerState != nullptr);
	}

	// TargetUnit - Specified target
	if (TargetName == FName(TEXT("TargetUnit")))
	{
		if (Context.TargetCardInstanceID != 0)
		{
			// Find which player owns the target
			AGCGPlayerState* Player1 = GetPlayerByID(1, GameState);
			AGCGPlayerState* Player2 = GetPlayerByID(2, GameState);

			FGCGCardInstance TempCard;
			EGCGCardZone Zone;

			if (Player1 && Player1->FindCardByInstanceID(Context.TargetCardInstanceID, TempCard, Zone))
			{
				OutPlayerState = Player1;
				OutCardInstanceID = Context.TargetCardInstanceID;
				return true;
			}

			if (Player2 && Player2->FindCardByInstanceID(Context.TargetCardInstanceID, TempCard, Zone))
			{
				OutPlayerState = Player2;
				OutCardInstanceID = Context.TargetCardInstanceID;
				return true;
			}
		}
	}

	// TODO Phase 8: Add more target types
	// - AllFriendlyUnits
	// - AllEnemyUnits
	// - RandomUnit
	// etc.

	return false;
}
