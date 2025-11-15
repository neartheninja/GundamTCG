// GCGPlayerActionSubsystem.cpp - Player Action Subsystem Implementation
// Unreal Engine 5.6 - Gundam TCG Implementation

#include "GCGPlayerActionSubsystem.h"
#include "GundamTCG/PlayerState/GCGPlayerState.h"
#include "GundamTCG/GameState/GCGGameState.h"
#include "GundamTCG/Subsystems/GCGZoneSubsystem.h"

// ===== SUBSYSTEM LIFECYCLE =====

void UGCGPlayerActionSubsystem::Initialize(FSubsystemCollectionBase& Collection)
{
	Super::Initialize(Collection);

	UE_LOG(LogTemp, Log, TEXT("UGCGPlayerActionSubsystem::Initialize - Player Action Subsystem initialized"));
}

void UGCGPlayerActionSubsystem::Deinitialize()
{
	UE_LOG(LogTemp, Log, TEXT("UGCGPlayerActionSubsystem::Deinitialize - Player Action Subsystem shutdown"));

	Super::Deinitialize();
}

// ===== ACTION EXECUTION =====

FGCGPlayerActionResult UGCGPlayerActionSubsystem::ExecuteAction(const FGCGPlayerActionRequest& Request,
	AGCGPlayerState* PlayerState, AGCGGameState* GameState)
{
	// Validate action first
	FGCGPlayerActionResult ValidationResult = ValidateAction(Request, PlayerState, GameState);
	if (!ValidationResult.bSuccess)
	{
		return ValidationResult;
	}

	// Execute based on action type
	switch (Request.ActionType)
	{
	case EGCGPlayerActionType::PlayCard:
		return ExecutePlayCard(Request.PrimaryCardInstanceID, PlayerState, GameState);

	case EGCGPlayerActionType::DiscardCard:
		return ExecuteDiscard(Request.PrimaryCardInstanceID, PlayerState);

	case EGCGPlayerActionType::PlaceResource:
	{
		bool bFaceUp = Request.Parameters.Contains(FName("FaceUp")) ? Request.Parameters[FName("FaceUp")] > 0 : false;
		return PlaceCardAsResource(Request.PrimaryCardInstanceID, PlayerState, GameState, bFaceUp);
	}

	case EGCGPlayerActionType::PassPriority:
		// Handled by GameMode
		return FGCGPlayerActionResult(true);

	default:
		return FGCGPlayerActionResult(false, TEXT("Action type not yet implemented"));
	}
}

// ===== ACTION VALIDATION =====

FGCGPlayerActionResult UGCGPlayerActionSubsystem::ValidateAction(const FGCGPlayerActionRequest& Request,
	AGCGPlayerState* PlayerState, AGCGGameState* GameState) const
{
	if (!PlayerState || !GameState)
	{
		return FGCGPlayerActionResult(false, TEXT("Invalid player or game state"));
	}

	// Validate player has priority
	FGCGPlayerActionResult PriorityResult = ValidatePlayerPriority(Request.PlayerID, GameState);
	if (!PriorityResult.bSuccess)
	{
		return PriorityResult;
	}

	// Validate based on action type
	switch (Request.ActionType)
	{
	case EGCGPlayerActionType::PlayCard:
	{
		FGCGCardInstance CardInstance;
		EGCGCardZone CardZone;
		if (!PlayerState->FindCardByInstanceID(Request.PrimaryCardInstanceID, CardInstance, CardZone))
		{
			return FGCGPlayerActionResult(false, TEXT("Card not found"));
		}
		return CanPlayCard(CardInstance, PlayerState, GameState);
	}

	case EGCGPlayerActionType::DiscardCard:
	{
		FGCGCardInstance CardInstance;
		EGCGCardZone CardZone;
		if (!PlayerState->FindCardByInstanceID(Request.PrimaryCardInstanceID, CardInstance, CardZone))
		{
			return FGCGPlayerActionResult(false, TEXT("Card not found"));
		}
		if (CardZone != EGCGCardZone::Hand)
		{
			return FGCGPlayerActionResult(false, TEXT("Can only discard cards from hand"));
		}
		return FGCGPlayerActionResult(true);
	}

	case EGCGPlayerActionType::PassPriority:
		return FGCGPlayerActionResult(true);

	default:
		return FGCGPlayerActionResult(false, TEXT("Action type not yet implemented"));
	}
}

// ===== PLAY CARD =====

FGCGPlayerActionResult UGCGPlayerActionSubsystem::PlayCardFromHand(int32 CardInstanceID,
	AGCGPlayerState* PlayerState, AGCGGameState* GameState)
{
	if (!PlayerState || !GameState)
	{
		return FGCGPlayerActionResult(false, TEXT("Invalid player or game state"));
	}

	// Find card in hand
	FGCGCardInstance CardInstance;
	EGCGCardZone CardZone;
	if (!PlayerState->FindCardByInstanceID(CardInstanceID, CardInstance, CardZone))
	{
		return FGCGPlayerActionResult(false, TEXT("Card not found"));
	}

	if (CardZone != EGCGCardZone::Hand)
	{
		return FGCGPlayerActionResult(false, TEXT("Card is not in hand"));
	}

	// Validate can play
	FGCGPlayerActionResult ValidationResult = CanPlayCard(CardInstance, PlayerState, GameState);
	if (!ValidationResult.bSuccess)
	{
		return ValidationResult;
	}

	// Execute play
	return ExecutePlayCard(CardInstanceID, PlayerState, GameState);
}

FGCGPlayerActionResult UGCGPlayerActionSubsystem::CanPlayCard(const FGCGCardInstance& CardInstance,
	AGCGPlayerState* PlayerState, AGCGGameState* GameState) const
{
	if (!PlayerState || !GameState)
	{
		return FGCGPlayerActionResult(false, TEXT("Invalid player or game state"));
	}

	// Check play timing
	FGCGPlayerActionResult TimingResult = ValidatePlayTiming(GameState);
	if (!TimingResult.bSuccess)
	{
		return TimingResult;
	}

	// FAQ Q21-Q22: Check level requirement
	// Get card data to check level requirement
	if (CardDatabase)
	{
		const FGCGCardData* CardData = CardDatabase->GetCardData(CardInstance.CardNumber);
		if (CardData)
		{
			// Player's Lv must be >= Card's Level requirement
			int32 PlayerLv = PlayerState->GetPlayerLv();
			if (PlayerLv < CardData->Level)
			{
				return FGCGPlayerActionResult(false,
					FString::Printf(TEXT("Insufficient Lv (card requires Lv %d, you have Lv %d)"),
						CardData->Level, PlayerLv));
			}
		}
	}

	// Check cost
	if (!CanPayCost(CardInstance.Cost, PlayerState))
	{
		return FGCGPlayerActionResult(false,
			FString::Printf(TEXT("Insufficient resources (need %d, have %d)"),
				CardInstance.Cost, PlayerState->GetActiveResourceCount()));
	}

	// Check zone capacity for Units
	if (CardInstance.CardType == EGCGCardType::Unit)
	{
		if (!PlayerState->CanAddUnitToBattle())
		{
			return FGCGPlayerActionResult(false, TEXT("Battle Area is full (max 6 Units)"));
		}
	}

	// Check Base limit
	if (CardInstance.CardType == EGCGCardType::Base)
	{
		if (PlayerState->BaseSection.Num() > 0)
		{
			return FGCGPlayerActionResult(false, TEXT("Can only have 1 Base (replace EX Base first)"));
		}
	}

	return FGCGPlayerActionResult(true);
}

// ===== COST PAYMENT =====

bool UGCGPlayerActionSubsystem::PayCost(int32 Cost, AGCGPlayerState* PlayerState)
{
	if (!PlayerState)
	{
		return false;
	}

	// Check if player can pay
	if (!CanPayCost(Cost, PlayerState))
	{
		UE_LOG(LogTemp, Warning, TEXT("UGCGPlayerActionSubsystem::PayCost - Cannot pay cost %d (have %d resources + %d EX)"),
			Cost, PlayerState->GetActiveResourceCount(), PlayerState->GetEXResourceCount());
		return false;
	}

	// Pay cost: First rest regular active resources, then remove EX Resources
	int32 RemainingCost = Cost;

	// Phase 1: Rest regular active resources
	for (FGCGCardInstance& Resource : PlayerState->ResourceArea)
	{
		if (RemainingCost <= 0)
		{
			break;
		}

		// Skip EX Resources in this phase
		if (Resource.bIsToken && Resource.TokenType == FName("EXResource"))
		{
			continue;
		}

		if (Resource.bIsActive)
		{
			Resource.bIsActive = false; // Rest the resource
			RemainingCost--;
			UE_LOG(LogTemp, Verbose, TEXT("UGCGPlayerActionSubsystem::PayCost - Rested resource %s (ID: %d)"),
				*Resource.CardName.ToString(), Resource.InstanceID);
		}
	}

	// Phase 2: Remove EX Resources if needed
	TArray<int32> EXResourcesToRemove;
	for (int32 i = 0; i < PlayerState->ResourceArea.Num() && RemainingCost > 0; i++)
	{
		FGCGCardInstance& Resource = PlayerState->ResourceArea[i];
		if (Resource.bIsToken && Resource.TokenType == FName("EXResource"))
		{
			EXResourcesToRemove.Add(i);
			RemainingCost--;
			UE_LOG(LogTemp, Verbose, TEXT("UGCGPlayerActionSubsystem::PayCost - Marked EX Resource for removal (ID: %d)"),
				Resource.InstanceID);
		}
	}

	// Remove EX Resources (in reverse order to maintain indices)
	for (int32 i = EXResourcesToRemove.Num() - 1; i >= 0; i--)
	{
		int32 Index = EXResourcesToRemove[i];
		UE_LOG(LogTemp, Log, TEXT("UGCGPlayerActionSubsystem::PayCost - Removed EX Resource from Resource Area"));
		PlayerState->ResourceArea.RemoveAt(Index);
	}

	if (RemainingCost > 0)
	{
		UE_LOG(LogTemp, Error, TEXT("UGCGPlayerActionSubsystem::PayCost - Failed to pay full cost (remaining: %d)"), RemainingCost);
		return false;
	}

	UE_LOG(LogTemp, Log, TEXT("UGCGPlayerActionSubsystem::PayCost - Successfully paid cost of %d"), Cost);
	return true;
}

bool UGCGPlayerActionSubsystem::CanPayCost(int32 Cost, AGCGPlayerState* PlayerState) const
{
	if (!PlayerState)
	{
		return false;
	}

	// Can pay with active resources + EX Resources
	int32 AvailableResources = PlayerState->GetActiveResourceCount() + PlayerState->GetEXResourceCount();
	return AvailableResources >= Cost;
}

// ===== RESOURCE PLACEMENT =====

FGCGPlayerActionResult UGCGPlayerActionSubsystem::PlaceCardAsResource(int32 CardInstanceID,
	AGCGPlayerState* PlayerState, AGCGGameState* GameState, bool bFaceUp)
{
	if (!PlayerState || !GameState)
	{
		return FGCGPlayerActionResult(false, TEXT("Invalid player or game state"));
	}

	// Check resource limit
	if (!PlayerState->CanAddResource())
	{
		return FGCGPlayerActionResult(false, TEXT("Resource Area is full (max 15)"));
	}

	// Check if player has already placed a resource this turn
	if (PlayerState->bHasPlacedResourceThisTurn)
	{
		return FGCGPlayerActionResult(false, TEXT("Already placed a resource this turn"));
	}

	// Get zone subsystem
	UGCGZoneSubsystem* ZoneSubsystem = GetZoneSubsystem();
	if (!ZoneSubsystem)
	{
		return FGCGPlayerActionResult(false, TEXT("Zone subsystem not found"));
	}

	// Find card in hand
	FGCGCardInstance CardInstance;
	EGCGCardZone CardZone;
	if (!PlayerState->FindCardByInstanceID(CardInstanceID, CardInstance, CardZone))
	{
		return FGCGPlayerActionResult(false, TEXT("Card not found"));
	}

	if (CardZone != EGCGCardZone::Hand)
	{
		return FGCGPlayerActionResult(false, TEXT("Can only place cards from hand as resources"));
	}

	// Move card to Resource Area
	if (!ZoneSubsystem->MoveCard(CardInstance, EGCGCardZone::Hand, EGCGCardZone::ResourceArea,
		PlayerState, GameState, true))
	{
		return FGCGPlayerActionResult(false, TEXT("Failed to move card to Resource Area"));
	}

	// Mark as placed resource this turn
	PlayerState->bHasPlacedResourceThisTurn = true;

	// TODO: Set face-up/face-down flag on card instance (Phase 7)

	UE_LOG(LogTemp, Log, TEXT("UGCGPlayerActionSubsystem::PlaceCardAsResource - Player %d placed resource: %s (ID: %d, FaceUp: %d)"),
		PlayerState->GetPlayerID(), *CardInstance.CardName.ToString(), CardInstanceID, bFaceUp ? 1 : 0);

	return FGCGPlayerActionResult(true);
}

// ===== DISCARD =====

bool UGCGPlayerActionSubsystem::DiscardCard(int32 CardInstanceID, AGCGPlayerState* PlayerState)
{
	if (!PlayerState)
	{
		return false;
	}

	FGCGPlayerActionResult Result = ExecuteDiscard(CardInstanceID, PlayerState);
	return Result.bSuccess;
}

int32 UGCGPlayerActionSubsystem::DiscardToHandLimit(const TArray<int32>& CardInstanceIDs,
	AGCGPlayerState* PlayerState, int32 TargetHandSize)
{
	if (!PlayerState)
	{
		return 0;
	}

	int32 DiscardedCount = 0;

	for (int32 CardInstanceID : CardInstanceIDs)
	{
		if (PlayerState->GetHandSize() <= TargetHandSize)
		{
			break; // Reached target hand size
		}

		if (DiscardCard(CardInstanceID, PlayerState))
		{
			DiscardedCount++;
		}
	}

	UE_LOG(LogTemp, Log, TEXT("UGCGPlayerActionSubsystem::DiscardToHandLimit - Player %d discarded %d cards to reach hand limit"),
		PlayerState->GetPlayerID(), DiscardedCount);

	return DiscardedCount;
}

// ===== INTERNAL VALIDATION =====

FGCGPlayerActionResult UGCGPlayerActionSubsystem::ValidatePlayTiming(AGCGGameState* GameState) const
{
	if (!GameState)
	{
		return FGCGPlayerActionResult(false, TEXT("Invalid game state"));
	}

	// Can only play cards during Main Phase
	if (GameState->CurrentPhase != EGCGTurnPhase::MainPhase)
	{
		return FGCGPlayerActionResult(false, TEXT("Can only play cards during Main Phase"));
	}

	return FGCGPlayerActionResult(true);
}

FGCGPlayerActionResult UGCGPlayerActionSubsystem::ValidatePlayerPriority(int32 PlayerID, AGCGGameState* GameState) const
{
	if (!GameState)
	{
		return FGCGPlayerActionResult(false, TEXT("Invalid game state"));
	}

	// Check if it's this player's turn
	if (GameState->ActivePlayerID != PlayerID)
	{
		return FGCGPlayerActionResult(false, TEXT("Not your turn"));
	}

	// TODO: Add priority system for response timing (Phase 8)

	return FGCGPlayerActionResult(true);
}

// ===== INTERNAL EXECUTION =====

FGCGPlayerActionResult UGCGPlayerActionSubsystem::ExecutePlayCard(int32 CardInstanceID,
	AGCGPlayerState* PlayerState, AGCGGameState* GameState)
{
	if (!PlayerState || !GameState)
	{
		return FGCGPlayerActionResult(false, TEXT("Invalid player or game state"));
	}

	UGCGZoneSubsystem* ZoneSubsystem = GetZoneSubsystem();
	if (!ZoneSubsystem)
	{
		return FGCGPlayerActionResult(false, TEXT("Zone subsystem not found"));
	}

	// Find card in hand
	FGCGCardInstance CardInstance;
	EGCGCardZone CardZone;
	if (!PlayerState->FindCardByInstanceID(CardInstanceID, CardInstance, CardZone))
	{
		return FGCGPlayerActionResult(false, TEXT("Card not found"));
	}

	// Pay cost
	if (!PayCost(CardInstance.Cost, PlayerState))
	{
		return FGCGPlayerActionResult(false, TEXT("Failed to pay cost"));
	}

	// Determine destination zone based on card type
	EGCGCardZone DestinationZone = EGCGCardZone::None;
	switch (CardInstance.CardType)
	{
	case EGCGCardType::Unit:
		DestinationZone = EGCGCardZone::BattleArea;
		break;

	case EGCGCardType::Base:
		DestinationZone = EGCGCardZone::BaseSection;
		// If there's an EX Base, remove it first
		if (PlayerState->BaseSection.Num() > 0 && PlayerState->BaseSection[0].bIsToken)
		{
			PlayerState->BaseSection.RemoveAt(0);
			UE_LOG(LogTemp, Log, TEXT("UGCGPlayerActionSubsystem::ExecutePlayCard - Removed EX Base token"));
		}
		break;

	case EGCGCardType::Command:
		// Commands go to trash after resolution (for now, just move to trash)
		// TODO: Implement command resolution and effect execution (Phase 8)
		DestinationZone = EGCGCardZone::Trash;
		UE_LOG(LogTemp, Warning, TEXT("UGCGPlayerActionSubsystem::ExecutePlayCard - Command effects not yet implemented"));
		break;

	default:
		return FGCGPlayerActionResult(false, TEXT("Unknown card type"));
	}

	// Move card to destination zone
	if (!ZoneSubsystem->MoveCard(CardInstance, EGCGCardZone::Hand, DestinationZone,
		PlayerState, GameState, true))
	{
		return FGCGPlayerActionResult(false, TEXT("Failed to move card to play area"));
	}

	// Set turn deployed for Units
	if (CardInstance.CardType == EGCGCardType::Unit)
	{
		// Update the card in the BattleArea
		for (FGCGCardInstance& BattleCard : PlayerState->BattleArea)
		{
			if (BattleCard.InstanceID == CardInstanceID)
			{
				BattleCard.TurnDeployed = GameState->TurnNumber;
				break;
			}
		}
	}

	// TODO: Trigger "On Deploy" effects (Phase 8)

	UE_LOG(LogTemp, Log, TEXT("UGCGPlayerActionSubsystem::ExecutePlayCard - Player %d played %s (ID: %d, Cost: %d) to %s"),
		PlayerState->GetPlayerID(), *CardInstance.CardName.ToString(), CardInstanceID,
		CardInstance.Cost, *ZoneSubsystem->GetZoneName(DestinationZone));

	return FGCGPlayerActionResult(true);
}

FGCGPlayerActionResult UGCGPlayerActionSubsystem::ExecuteDiscard(int32 CardInstanceID, AGCGPlayerState* PlayerState)
{
	if (!PlayerState)
	{
		return FGCGPlayerActionResult(false, TEXT("Invalid player state"));
	}

	UGCGZoneSubsystem* ZoneSubsystem = GetZoneSubsystem();
	if (!ZoneSubsystem)
	{
		return FGCGPlayerActionResult(false, TEXT("Zone subsystem not found"));
	}

	// Find card in hand
	FGCGCardInstance CardInstance;
	EGCGCardZone CardZone;
	if (!PlayerState->FindCardByInstanceID(CardInstanceID, CardInstance, CardZone))
	{
		return FGCGPlayerActionResult(false, TEXT("Card not found"));
	}

	if (CardZone != EGCGCardZone::Hand)
	{
		return FGCGPlayerActionResult(false, TEXT("Card is not in hand"));
	}

	// Move card to trash
	// Note: We pass nullptr for GameState since discard doesn't need game state validation
	if (!ZoneSubsystem->MoveCard(CardInstance, EGCGCardZone::Hand, EGCGCardZone::Trash,
		PlayerState, nullptr, false))
	{
		return FGCGPlayerActionResult(false, TEXT("Failed to move card to trash"));
	}

	UE_LOG(LogTemp, Log, TEXT("UGCGPlayerActionSubsystem::ExecuteDiscard - Player %d discarded %s (ID: %d)"),
		PlayerState->GetPlayerID(), *CardInstance.CardName.ToString(), CardInstanceID);

	return FGCGPlayerActionResult(true);
}

// ===== PRIVATE HELPERS =====

UGCGZoneSubsystem* UGCGPlayerActionSubsystem::GetZoneSubsystem() const
{
	return GetGameInstance()->GetSubsystem<UGCGZoneSubsystem>();
}
