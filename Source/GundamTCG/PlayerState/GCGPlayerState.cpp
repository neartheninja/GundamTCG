// GCGPlayerState.cpp - Player State Implementation
// Unreal Engine 5.6 - Gundam TCG Implementation

#include "GCGPlayerState.h"
#include "Net/UnrealNetwork.h"

AGCGPlayerState::AGCGPlayerState()
{
	// Initialize player ID
	PlayerID = -1;

	// Initialize player flags
	bHasLost = false;
	bHasPriority = false;
	bHasPlacedResourceThisTurn = false;
	bHasDrawnThisTurn = false;

	// All zone arrays start empty (will be populated during game setup)

	// Enable replication
	bReplicates = true;
	bAlwaysRelevant = true; // Player state is always relevant to all clients
}

void AGCGPlayerState::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);

	// Replicate player ID
	DOREPLIFETIME(AGCGPlayerState, PlayerID);

	// Replicate all zones
	DOREPLIFETIME(AGCGPlayerState, Deck);
	DOREPLIFETIME(AGCGPlayerState, ResourceDeck);
	DOREPLIFETIME(AGCGPlayerState, Hand);
	DOREPLIFETIME(AGCGPlayerState, ResourceArea);
	DOREPLIFETIME(AGCGPlayerState, BattleArea);
	DOREPLIFETIME(AGCGPlayerState, ShieldStack);
	DOREPLIFETIME(AGCGPlayerState, BaseSection);
	DOREPLIFETIME(AGCGPlayerState, Trash);
	DOREPLIFETIME(AGCGPlayerState, Removal);

	// Replicate deck lists
	DOREPLIFETIME(AGCGPlayerState, MainDeckList);
	DOREPLIFETIME(AGCGPlayerState, ResourceDeckList);

	// Replicate player flags
	DOREPLIFETIME(AGCGPlayerState, bHasLost);
	DOREPLIFETIME(AGCGPlayerState, bHasPriority);
	DOREPLIFETIME(AGCGPlayerState, bHasPlacedResourceThisTurn);
	DOREPLIFETIME(AGCGPlayerState, bHasDrawnThisTurn);
}

// ===== PLAYER IDENTIFICATION =====

void AGCGPlayerState::SetPlayerID(int32 NewPlayerID)
{
	PlayerID = NewPlayerID;
	UE_LOG(LogTemp, Log, TEXT("AGCGPlayerState::SetPlayerID - Player ID set to %d"), PlayerID);
}

// ===== ZONE QUERIES =====

int32 AGCGPlayerState::GetActiveResourceCount() const
{
	int32 ActiveCount = 0;

	for (const FGCGCardInstance& Resource : ResourceArea)
	{
		if (Resource.bIsActive)
		{
			ActiveCount++;
		}
	}

	return ActiveCount;
}

int32 AGCGPlayerState::GetTotalResourceCount() const
{
	return ResourceArea.Num();
}

int32 AGCGPlayerState::GetShieldCount() const
{
	return ShieldStack.Num();
}

int32 AGCGPlayerState::GetUnitCount() const
{
	return BattleArea.Num();
}

int32 AGCGPlayerState::GetHandSize() const
{
	return Hand.Num();
}

int32 AGCGPlayerState::GetDeckSize() const
{
	return Deck.Num();
}

int32 AGCGPlayerState::GetResourceDeckSize() const
{
	return ResourceDeck.Num();
}

// ===== ZONE VALIDATION =====

bool AGCGPlayerState::CanPayCost(int32 Cost) const
{
	return GetActiveResourceCount() >= Cost;
}

bool AGCGPlayerState::CanAddUnitToBattle() const
{
	// Max 6 units in battle area
	// Note: In team battle, this limit is shared across the team
	// The GameMode or ZoneSubsystem should handle team battle logic
	return BattleArea.Num() < 6;
}

bool AGCGPlayerState::CanAddResource() const
{
	// Max 15 resources
	return ResourceArea.Num() < 15;
}

// ===== HELPER FUNCTIONS =====

void AGCGPlayerState::ResetTurnFlags()
{
	bHasPlacedResourceThisTurn = false;
	bHasDrawnThisTurn = false;

	UE_LOG(LogTemp, Log, TEXT("AGCGPlayerState::ResetTurnFlags - Player %d turn flags reset"), PlayerID);
}

TArray<FGCGCardInstance> AGCGPlayerState::GetAllCards() const
{
	TArray<FGCGCardInstance> AllCards;

	// Add cards from all zones
	AllCards.Append(Deck);
	AllCards.Append(ResourceDeck);
	AllCards.Append(Hand);
	AllCards.Append(ResourceArea);
	AllCards.Append(BattleArea);
	AllCards.Append(ShieldStack);
	AllCards.Append(BaseSection);
	AllCards.Append(Trash);
	AllCards.Append(Removal);

	return AllCards;
}

bool AGCGPlayerState::FindCardByInstanceID(int32 InstanceID, FGCGCardInstance& OutCard, EGCGCardZone& OutZone) const
{
	// Helper lambda to search a zone
	auto SearchZone = [InstanceID, &OutCard](const TArray<FGCGCardInstance>& Zone) -> bool
	{
		const FGCGCardInstance* Found = Zone.FindByPredicate([InstanceID](const FGCGCardInstance& Card)
		{
			return Card.InstanceID == InstanceID;
		});

		if (Found)
		{
			OutCard = *Found;
			return true;
		}
		return false;
	};

	// Search each zone
	if (SearchZone(Deck))
	{
		OutZone = EGCGCardZone::Deck;
		return true;
	}

	if (SearchZone(ResourceDeck))
	{
		OutZone = EGCGCardZone::ResourceDeck;
		return true;
	}

	if (SearchZone(Hand))
	{
		OutZone = EGCGCardZone::Hand;
		return true;
	}

	if (SearchZone(ResourceArea))
	{
		OutZone = EGCGCardZone::ResourceArea;
		return true;
	}

	if (SearchZone(BattleArea))
	{
		OutZone = EGCGCardZone::BattleArea;
		return true;
	}

	if (SearchZone(ShieldStack))
	{
		OutZone = EGCGCardZone::ShieldStack;
		return true;
	}

	if (SearchZone(BaseSection))
	{
		OutZone = EGCGCardZone::BaseSection;
		return true;
	}

	if (SearchZone(Trash))
	{
		OutZone = EGCGCardZone::Trash;
		return true;
	}

	if (SearchZone(Removal))
	{
		OutZone = EGCGCardZone::Removal;
		return true;
	}

	// Card not found in any zone
	OutZone = EGCGCardZone::None;
	return false;
}
