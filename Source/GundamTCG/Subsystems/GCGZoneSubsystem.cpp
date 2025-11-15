// GCGZoneSubsystem.cpp - Zone Management Subsystem Implementation
// Unreal Engine 5.6 - Gundam TCG Implementation

#include "GCGZoneSubsystem.h"
#include "GundamTCG/PlayerState/GCGPlayerState.h"
#include "GundamTCG/GameState/GCGGameState.h"

// ===== SUBSYSTEM LIFECYCLE =====

void UGCGZoneSubsystem::Initialize(FSubsystemCollectionBase& Collection)
{
	Super::Initialize(Collection);

	UE_LOG(LogTemp, Log, TEXT("UGCGZoneSubsystem::Initialize - Zone Management Subsystem initialized"));
}

void UGCGZoneSubsystem::Deinitialize()
{
	UE_LOG(LogTemp, Log, TEXT("UGCGZoneSubsystem::Deinitialize - Zone Management Subsystem shutdown"));

	Super::Deinitialize();
}

// ===== CARD MOVEMENT =====

bool UGCGZoneSubsystem::MoveCard(FGCGCardInstance& Card, EGCGCardZone FromZone, EGCGCardZone ToZone,
	AGCGPlayerState* PlayerState, AGCGGameState* GameState, bool bValidateLimits)
{
	if (!PlayerState)
	{
		UE_LOG(LogTemp, Error, TEXT("UGCGZoneSubsystem::MoveCard - PlayerState is null"));
		return false;
	}

	// Validate zone transition
	if (!ValidateZoneTransition(FromZone, ToZone, Card))
	{
		UE_LOG(LogTemp, Warning, TEXT("UGCGZoneSubsystem::MoveCard - Invalid zone transition from %s to %s"),
			*GetZoneName(FromZone), *GetZoneName(ToZone));
		return false;
	}

	// Check if destination zone can accept this card
	if (bValidateLimits && !CanAddToZone(ToZone, PlayerState, GameState, Card.CardType))
	{
		UE_LOG(LogTemp, Warning, TEXT("UGCGZoneSubsystem::MoveCard - Cannot add card to zone %s (at capacity or invalid)"),
			*GetZoneName(ToZone));
		return false;
	}

	// Get zone arrays
	TArray<FGCGCardInstance>* FromZoneArray = GetZoneArray(FromZone, PlayerState);
	TArray<FGCGCardInstance>* ToZoneArray = GetZoneArray(ToZone, PlayerState);

	if (!FromZoneArray || !ToZoneArray)
	{
		UE_LOG(LogTemp, Error, TEXT("UGCGZoneSubsystem::MoveCard - Failed to get zone arrays"));
		return false;
	}

	// Find card in source zone
	int32 CardIndex = FromZoneArray->IndexOfByPredicate([&Card](const FGCGCardInstance& Instance)
	{
		return Instance.InstanceID == Card.InstanceID;
	});

	if (CardIndex == INDEX_NONE)
	{
		UE_LOG(LogTemp, Warning, TEXT("UGCGZoneSubsystem::MoveCard - Card not found in source zone %s"),
			*GetZoneName(FromZone));
		return false;
	}

	// Apply zone exit rules
	ApplyZoneExitRules(Card, FromZone);

	// Remove from source zone
	FromZoneArray->RemoveAt(CardIndex);

	// Update card's current zone
	Card.CurrentZone = ToZone;

	// Apply zone entry rules
	ApplyZoneEntryRules(Card, ToZone);

	// Add to destination zone
	ToZoneArray->Add(Card);

	UE_LOG(LogTemp, Log, TEXT("UGCGZoneSubsystem::MoveCard - Moved card %s (ID: %d) from %s to %s"),
		*Card.CardName.ToString(), Card.InstanceID, *GetZoneName(FromZone), *GetZoneName(ToZone));

	return true;
}

int32 UGCGZoneSubsystem::MoveCards(TArray<FGCGCardInstance>& Cards, EGCGCardZone FromZone, EGCGCardZone ToZone,
	AGCGPlayerState* PlayerState, AGCGGameState* GameState, bool bValidateLimits)
{
	int32 SuccessfulMoves = 0;

	for (FGCGCardInstance& Card : Cards)
	{
		if (MoveCard(Card, FromZone, ToZone, PlayerState, GameState, bValidateLimits))
		{
			SuccessfulMoves++;
		}
	}

	UE_LOG(LogTemp, Log, TEXT("UGCGZoneSubsystem::MoveCards - Moved %d/%d cards from %s to %s"),
		SuccessfulMoves, Cards.Num(), *GetZoneName(FromZone), *GetZoneName(ToZone));

	return SuccessfulMoves;
}

// ===== ZONE VALIDATION =====

bool UGCGZoneSubsystem::CanAddToZone(EGCGCardZone Zone, AGCGPlayerState* PlayerState, AGCGGameState* GameState, EGCGCardType CardType) const
{
	if (!PlayerState)
	{
		return false;
	}

	// Check zone capacity
	if (IsZoneAtCapacity(Zone, PlayerState, GameState, CardType))
	{
		return false;
	}

	// Zone-specific validation
	switch (Zone)
	{
	case EGCGCardZone::BattleArea:
		// Only Units can be in Battle Area
		return CardType == EGCGCardType::Unit;

	case EGCGCardZone::ResourceArea:
		// Any card can be a resource (face-up or face-down)
		return true;

	case EGCGCardZone::BaseSection:
		// Only Base cards (or EX Base tokens)
		return CardType == EGCGCardType::Base;

	case EGCGCardZone::Hand:
	case EGCGCardZone::Deck:
	case EGCGCardZone::ResourceDeck:
	case EGCGCardZone::ShieldStack:
	case EGCGCardZone::Trash:
	case EGCGCardZone::Removal:
		// These zones accept any card type
		return true;

	default:
		return false;
	}
}

int32 UGCGZoneSubsystem::GetZoneCount(EGCGCardZone Zone, AGCGPlayerState* PlayerState) const
{
	if (!PlayerState)
	{
		return 0;
	}

	const TArray<FGCGCardInstance>* ZoneArray = GetZoneArray(Zone, PlayerState);
	return ZoneArray ? ZoneArray->Num() : 0;
}

int32 UGCGZoneSubsystem::GetZoneMaxCapacity(EGCGCardZone Zone, EGCGCardType CardType) const
{
	switch (Zone)
	{
	case EGCGCardZone::BattleArea:
		// Max 6 Units in Battle Area
		return 6;

	case EGCGCardZone::ResourceArea:
		// Max 15 Resources
		return 15;

	case EGCGCardZone::BaseSection:
		// Max 1 Base
		return 1;

	case EGCGCardZone::Hand:
	case EGCGCardZone::Deck:
	case EGCGCardZone::ResourceDeck:
	case EGCGCardZone::ShieldStack:
	case EGCGCardZone::Trash:
	case EGCGCardZone::Removal:
		// Unlimited capacity
		return -1;

	default:
		return -1;
	}
}

bool UGCGZoneSubsystem::IsZoneAtCapacity(EGCGCardZone Zone, AGCGPlayerState* PlayerState, AGCGGameState* GameState, EGCGCardType CardType) const
{
	int32 MaxCapacity = GetZoneMaxCapacity(Zone, CardType);

	// -1 means unlimited
	if (MaxCapacity == -1)
	{
		return false;
	}

	// For team battle, Battle Area limit is shared across team
	if (Zone == EGCGCardZone::BattleArea && GameState && GameState->bIsTeamBattle)
	{
		// Get teammate
		AGCGPlayerState* Teammate = nullptr;
		const FGCGTeamInfo* Team = GameState->GetTeamForPlayer(PlayerState->GetPlayerID());
		if (Team)
		{
			for (int32 TeamPlayerID : Team->PlayerIDs)
			{
				if (TeamPlayerID != PlayerState->GetPlayerID())
				{
					// TODO: Get teammate PlayerState (need access to GameState's PlayerArray)
					// For now, just check this player's count
					break;
				}
			}
		}
	}

	int32 CurrentCount = GetZoneCount(Zone, PlayerState);
	return CurrentCount >= MaxCapacity;
}

// ===== ZONE QUERIES =====

TArray<FGCGCardInstance> UGCGZoneSubsystem::GetCardsInZone(EGCGCardZone Zone, AGCGPlayerState* PlayerState) const
{
	if (!PlayerState)
	{
		return TArray<FGCGCardInstance>();
	}

	const TArray<FGCGCardInstance>* ZoneArray = GetZoneArray(Zone, PlayerState);
	return ZoneArray ? *ZoneArray : TArray<FGCGCardInstance>();
}

bool UGCGZoneSubsystem::FindCardInZone(EGCGCardZone Zone, AGCGPlayerState* PlayerState, int32 InstanceID, FGCGCardInstance& OutCard) const
{
	if (!PlayerState)
	{
		return false;
	}

	const TArray<FGCGCardInstance>* ZoneArray = GetZoneArray(Zone, PlayerState);
	if (!ZoneArray)
	{
		return false;
	}

	const FGCGCardInstance* FoundCard = ZoneArray->FindByPredicate([InstanceID](const FGCGCardInstance& Card)
	{
		return Card.InstanceID == InstanceID;
	});

	if (FoundCard)
	{
		OutCard = *FoundCard;
		return true;
	}

	return false;
}

// ===== ZONE MANIPULATION =====

bool UGCGZoneSubsystem::ShuffleZone(EGCGCardZone Zone, AGCGPlayerState* PlayerState)
{
	if (!PlayerState)
	{
		return false;
	}

	// Only certain zones can be shuffled
	if (Zone != EGCGCardZone::Deck && Zone != EGCGCardZone::ResourceDeck)
	{
		UE_LOG(LogTemp, Warning, TEXT("UGCGZoneSubsystem::ShuffleZone - Cannot shuffle zone %s"), *GetZoneName(Zone));
		return false;
	}

	TArray<FGCGCardInstance>* ZoneArray = GetZoneArray(Zone, PlayerState);
	if (!ZoneArray)
	{
		return false;
	}

	// Fisher-Yates shuffle
	int32 LastIndex = ZoneArray->Num() - 1;
	for (int32 i = 0; i <= LastIndex; ++i)
	{
		int32 Index = FMath::RandRange(i, LastIndex);
		if (i != Index)
		{
			ZoneArray->Swap(i, Index);
		}
	}

	UE_LOG(LogTemp, Log, TEXT("UGCGZoneSubsystem::ShuffleZone - Shuffled %s (%d cards)"),
		*GetZoneName(Zone), ZoneArray->Num());

	return true;
}

bool UGCGZoneSubsystem::DrawTopCard(EGCGCardZone Zone, AGCGPlayerState* PlayerState, FGCGCardInstance& OutCard)
{
	if (!PlayerState)
	{
		return false;
	}

	TArray<FGCGCardInstance>* ZoneArray = GetZoneArray(Zone, PlayerState);
	if (!ZoneArray || ZoneArray->Num() == 0)
	{
		UE_LOG(LogTemp, Warning, TEXT("UGCGZoneSubsystem::DrawTopCard - Zone %s is empty"), *GetZoneName(Zone));
		return false;
	}

	// Top card is at index 0
	OutCard = (*ZoneArray)[0];
	ZoneArray->RemoveAt(0);

	UE_LOG(LogTemp, Log, TEXT("UGCGZoneSubsystem::DrawTopCard - Drew card %s (ID: %d) from %s"),
		*OutCard.CardName.ToString(), OutCard.InstanceID, *GetZoneName(Zone));

	return true;
}

int32 UGCGZoneSubsystem::DrawTopCards(EGCGCardZone Zone, AGCGPlayerState* PlayerState, int32 Count, TArray<FGCGCardInstance>& OutCards)
{
	OutCards.Empty();

	for (int32 i = 0; i < Count; ++i)
	{
		FGCGCardInstance DrawnCard;
		if (DrawTopCard(Zone, PlayerState, DrawnCard))
		{
			OutCards.Add(DrawnCard);
		}
		else
		{
			break;
		}
	}

	UE_LOG(LogTemp, Log, TEXT("UGCGZoneSubsystem::DrawTopCards - Drew %d/%d cards from %s"),
		OutCards.Num(), Count, *GetZoneName(Zone));

	return OutCards.Num();
}

bool UGCGZoneSubsystem::PeekTopCard(EGCGCardZone Zone, AGCGPlayerState* PlayerState, FGCGCardInstance& OutCard) const
{
	if (!PlayerState)
	{
		return false;
	}

	const TArray<FGCGCardInstance>* ZoneArray = GetZoneArray(Zone, PlayerState);
	if (!ZoneArray || ZoneArray->Num() == 0)
	{
		return false;
	}

	// Top card is at index 0
	OutCard = (*ZoneArray)[0];
	return true;
}

// ===== SPECIAL ZONE OPERATIONS =====

int32 UGCGZoneSubsystem::ActivateAllCards(AGCGPlayerState* PlayerState, EGCGCardZone Zone)
{
	if (!PlayerState)
	{
		return 0;
	}

	int32 ActivatedCount = 0;

	// If Zone is None, activate cards in all relevant zones
	if (Zone == EGCGCardZone::None)
	{
		ActivatedCount += ActivateAllCards(PlayerState, EGCGCardZone::BattleArea);
		ActivatedCount += ActivateAllCards(PlayerState, EGCGCardZone::ResourceArea);
		return ActivatedCount;
	}

	TArray<FGCGCardInstance>* ZoneArray = GetZoneArray(Zone, PlayerState);
	if (!ZoneArray)
	{
		return 0;
	}

	for (FGCGCardInstance& Card : *ZoneArray)
	{
		if (!Card.bIsActive)
		{
			Card.bIsActive = true;
			ActivatedCount++;
		}
	}

	UE_LOG(LogTemp, Log, TEXT("UGCGZoneSubsystem::ActivateAllCards - Activated %d cards in %s"),
		ActivatedCount, *GetZoneName(Zone));

	return ActivatedCount;
}

int32 UGCGZoneSubsystem::RestAllCards(AGCGPlayerState* PlayerState, EGCGCardZone Zone)
{
	if (!PlayerState)
	{
		return 0;
	}

	TArray<FGCGCardInstance>* ZoneArray = GetZoneArray(Zone, PlayerState);
	if (!ZoneArray)
	{
		return 0;
	}

	int32 RestedCount = 0;

	for (FGCGCardInstance& Card : *ZoneArray)
	{
		if (Card.bIsActive)
		{
			Card.bIsActive = false;
			RestedCount++;
		}
	}

	UE_LOG(LogTemp, Log, TEXT("UGCGZoneSubsystem::RestAllCards - Rested %d cards in %s"),
		RestedCount, *GetZoneName(Zone));

	return RestedCount;
}

int32 UGCGZoneSubsystem::ClearAllDamage(AGCGPlayerState* PlayerState, EGCGCardZone Zone)
{
	if (!PlayerState)
	{
		return 0;
	}

	TArray<FGCGCardInstance>* ZoneArray = GetZoneArray(Zone, PlayerState);
	if (!ZoneArray)
	{
		return 0;
	}

	int32 ClearedCount = 0;

	for (FGCGCardInstance& Card : *ZoneArray)
	{
		if (Card.CurrentDamage > 0)
		{
			Card.CurrentDamage = 0;
			ClearedCount++;
		}
	}

	UE_LOG(LogTemp, Log, TEXT("UGCGZoneSubsystem::ClearAllDamage - Cleared damage from %d cards in %s"),
		ClearedCount, *GetZoneName(Zone));

	return ClearedCount;
}

// ===== HELPER FUNCTIONS =====

FString UGCGZoneSubsystem::GetZoneName(EGCGCardZone Zone)
{
	switch (Zone)
	{
	case EGCGCardZone::None:
		return TEXT("None");
	case EGCGCardZone::Deck:
		return TEXT("Deck");
	case EGCGCardZone::ResourceDeck:
		return TEXT("Resource Deck");
	case EGCGCardZone::Hand:
		return TEXT("Hand");
	case EGCGCardZone::ResourceArea:
		return TEXT("Resource Area");
	case EGCGCardZone::BattleArea:
		return TEXT("Battle Area");
	case EGCGCardZone::ShieldStack:
		return TEXT("Shield Stack");
	case EGCGCardZone::BaseSection:
		return TEXT("Base Section");
	case EGCGCardZone::Trash:
		return TEXT("Trash");
	case EGCGCardZone::Removal:
		return TEXT("Removal");
	default:
		return TEXT("Unknown");
	}
}

bool UGCGZoneSubsystem::IsZonePublic(EGCGCardZone Zone)
{
	switch (Zone)
	{
	case EGCGCardZone::BattleArea:
	case EGCGCardZone::ResourceArea:
	case EGCGCardZone::BaseSection:
	case EGCGCardZone::Trash:
		return true;

	case EGCGCardZone::Deck:
	case EGCGCardZone::ResourceDeck:
	case EGCGCardZone::Hand:
	case EGCGCardZone::ShieldStack:
	case EGCGCardZone::Removal:
		return false;

	default:
		return false;
	}
}

bool UGCGZoneSubsystem::IsZoneOrdered(EGCGCardZone Zone)
{
	switch (Zone)
	{
	case EGCGCardZone::Deck:
	case EGCGCardZone::ResourceDeck:
	case EGCGCardZone::ShieldStack:
		return true;

	case EGCGCardZone::Hand:
	case EGCGCardZone::BattleArea:
	case EGCGCardZone::ResourceArea:
	case EGCGCardZone::BaseSection:
	case EGCGCardZone::Trash:
	case EGCGCardZone::Removal:
		return false;

	default:
		return false;
	}
}

// ===== INTERNAL HELPERS =====

TArray<FGCGCardInstance>* UGCGZoneSubsystem::GetZoneArray(EGCGCardZone Zone, AGCGPlayerState* PlayerState) const
{
	if (!PlayerState)
	{
		return nullptr;
	}

	switch (Zone)
	{
	case EGCGCardZone::Deck:
		return &PlayerState->Deck;
	case EGCGCardZone::ResourceDeck:
		return &PlayerState->ResourceDeck;
	case EGCGCardZone::Hand:
		return &PlayerState->Hand;
	case EGCGCardZone::ResourceArea:
		return &PlayerState->ResourceArea;
	case EGCGCardZone::BattleArea:
		return &PlayerState->BattleArea;
	case EGCGCardZone::ShieldStack:
		return &PlayerState->ShieldStack;
	case EGCGCardZone::BaseSection:
		return &PlayerState->BaseSection;
	case EGCGCardZone::Trash:
		return &PlayerState->Trash;
	case EGCGCardZone::Removal:
		return &PlayerState->Removal;
	default:
		return nullptr;
	}
}

bool UGCGZoneSubsystem::ValidateZoneTransition(EGCGCardZone FromZone, EGCGCardZone ToZone, const FGCGCardInstance& Card) const
{
	// Can't move to/from None
	if (FromZone == EGCGCardZone::None || ToZone == EGCGCardZone::None)
	{
		return false;
	}

	// Can't move to same zone
	if (FromZone == ToZone)
	{
		return false;
	}

	// Removal zone is special - cards can go in but typically don't come out
	if (FromZone == EGCGCardZone::Removal)
	{
		UE_LOG(LogTemp, Warning, TEXT("UGCGZoneSubsystem::ValidateZoneTransition - Cards typically don't leave Removal zone"));
		// Allow it but warn
	}

	// All other transitions are valid
	return true;
}

void UGCGZoneSubsystem::ApplyZoneEntryRules(FGCGCardInstance& Card, EGCGCardZone Zone)
{
	switch (Zone)
	{
	case EGCGCardZone::BattleArea:
		// Units enter battle area rested (unless effect says otherwise)
		Card.bIsActive = false;
		break;

	case EGCGCardZone::ResourceArea:
		// Resources enter active
		Card.bIsActive = true;
		break;

	case EGCGCardZone::Hand:
		// Cards in hand are always active (conceptually)
		Card.bIsActive = true;
		break;

	case EGCGCardZone::Trash:
	case EGCGCardZone::Removal:
		// Clear all state when going to trash/removal
		Card.bIsActive = false;
		Card.CurrentDamage = 0;
		Card.Counters.Empty();
		Card.AttachedCards.Empty();
		break;

	default:
		// No special rules
		break;
	}
}

void UGCGZoneSubsystem::ApplyZoneExitRules(FGCGCardInstance& Card, EGCGCardZone Zone)
{
	// Remove attached cards when leaving play zones
	if (Zone == EGCGCardZone::BattleArea || Zone == EGCGCardZone::ResourceArea || Zone == EGCGCardZone::BaseSection)
	{
		if (Card.AttachedCards.Num() > 0)
		{
			UE_LOG(LogTemp, Log, TEXT("UGCGZoneSubsystem::ApplyZoneExitRules - Card %s leaving %s with %d attached cards"),
				*Card.CardName.ToString(), *GetZoneName(Zone), Card.AttachedCards.Num());

			// TODO: Move attached cards to appropriate zone (typically Trash)
			// For now, just clear the array
			Card.AttachedCards.Empty();
		}
	}

	// No other special exit rules for now
}
