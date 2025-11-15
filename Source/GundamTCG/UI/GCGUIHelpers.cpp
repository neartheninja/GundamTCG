// GCGUIHelpers.cpp - UI Helper Functions Implementation
// Unreal Engine 5.6 - Gundam TCG Implementation

#include "GCGUIHelpers.h"
#include "GundamTCG/PlayerState/GCGPlayerState.h"
#include "GundamTCG/GameState/GCGGameState.h"
#include "GundamTCG/Subsystems/GCGCardDatabase.h"
#include "GundamTCG/Subsystems/GCGCombatSubsystem.h"

// ===========================================================================================
// CARD DATA CONVERSION
// ===========================================================================================

FGCGUICardData UGCGUIHelpers::ConvertCardToUIData(const FGCGCardInstance& CardInstance, UGCGCardDatabase* CardDatabase)
{
	FGCGUICardData UIData(CardInstance);

	// Load additional data from database if provided
	if (CardDatabase)
	{
		const FGCGCardData* CardData = CardDatabase->GetCardData(CardInstance.CardNumber);
		if (CardData)
		{
			// CardArt and CardFrame could be loaded here from CardData
			// UIData.CardArt = CardData->CardArt;
			// UIData.CardFrame = CardData->CardFrame;
		}
	}

	return UIData;
}

TArray<FGCGUICardData> UGCGUIHelpers::ConvertCardsToUIData(const TArray<FGCGCardInstance>& CardInstances, UGCGCardDatabase* CardDatabase)
{
	TArray<FGCGUICardData> UIDataArray;
	UIDataArray.Reserve(CardInstances.Num());

	for (const FGCGCardInstance& Card : CardInstances)
	{
		UIDataArray.Add(ConvertCardToUIData(Card, CardDatabase));
	}

	return UIDataArray;
}

FText UGCGUIHelpers::GetColorDisplayName(EGCGColor Color)
{
	switch (Color)
	{
		case EGCGColor::Red:     return FText::FromString(TEXT("Red"));
		case EGCGColor::Blue:    return FText::FromString(TEXT("Blue"));
		case EGCGColor::Green:   return FText::FromString(TEXT("Green"));
		case EGCGColor::Yellow:  return FText::FromString(TEXT("Yellow"));
		case EGCGColor::Black:   return FText::FromString(TEXT("Black"));
		case EGCGColor::White:   return FText::FromString(TEXT("White"));
		default:                 return FText::FromString(TEXT("Colorless"));
	}
}

FLinearColor UGCGUIHelpers::GetColorAsLinearColor(EGCGColor Color)
{
	switch (Color)
	{
		case EGCGColor::Red:     return FLinearColor(1.0f, 0.0f, 0.0f);
		case EGCGColor::Blue:    return FLinearColor(0.0f, 0.5f, 1.0f);
		case EGCGColor::Green:   return FLinearColor(0.0f, 0.8f, 0.0f);
		case EGCGColor::Yellow:  return FLinearColor(1.0f, 1.0f, 0.0f);
		case EGCGColor::Black:   return FLinearColor(0.2f, 0.2f, 0.2f);
		case EGCGColor::White:   return FLinearColor(1.0f, 1.0f, 1.0f);
		default:                 return FLinearColor(0.5f, 0.5f, 0.5f);
	}
}

FText UGCGUIHelpers::GetKeywordDisplayName(EGCGKeyword Keyword)
{
	switch (Keyword)
	{
		case EGCGKeyword::Repair:         return FText::FromString(TEXT("Repair"));
		case EGCGKeyword::Breach:         return FText::FromString(TEXT("Breach"));
		case EGCGKeyword::Support:        return FText::FromString(TEXT("Support"));
		case EGCGKeyword::Blocker:        return FText::FromString(TEXT("Blocker"));
		case EGCGKeyword::FirstStrike:    return FText::FromString(TEXT("First Strike"));
		case EGCGKeyword::HighManeuver:   return FText::FromString(TEXT("High Maneuver"));
		case EGCGKeyword::Suppression:    return FText::FromString(TEXT("Suppression"));
		case EGCGKeyword::Burst:          return FText::FromString(TEXT("Burst"));
		case EGCGKeyword::LinkUnit:       return FText::FromString(TEXT("Link Unit"));
		default:                          return FText::FromString(TEXT("Unknown"));
	}
}

FText UGCGUIHelpers::GetKeywordDescription(EGCGKeyword Keyword)
{
	switch (Keyword)
	{
		case EGCGKeyword::Repair:
			return FText::FromString(TEXT("When deployed: Restore HP to friendly Units."));
		case EGCGKeyword::Breach:
			return FText::FromString(TEXT("When attacking: Deals damage to enemy Base even if blocked."));
		case EGCGKeyword::Support:
			return FText::FromString(TEXT("When deployed: Grant bonuses to friendly Units."));
		case EGCGKeyword::Blocker:
			return FText::FromString(TEXT("Can block even when inactive."));
		case EGCGKeyword::FirstStrike:
			return FText::FromString(TEXT("Deals combat damage before Units without First Strike."));
		case EGCGKeyword::HighManeuver:
			return FText::FromString(TEXT("Cannot be blocked by Units without High Maneuver or Blocker."));
		case EGCGKeyword::Suppression:
			return FText::FromString(TEXT("Opponent discards a card when this attacks."));
		case EGCGKeyword::Burst:
			return FText::FromString(TEXT("Can be activated from Shield Stack when revealed."));
		case EGCGKeyword::LinkUnit:
			return FText::FromString(TEXT("Can pair with a Pilot to bypass summoning sickness."));
		default:
			return FText::FromString(TEXT("No description available."));
	}
}

FText UGCGUIHelpers::GetCardTypeDisplayName(EGCGCardType CardType)
{
	switch (CardType)
	{
		case EGCGCardType::Unit:     return FText::FromString(TEXT("Unit"));
		case EGCGCardType::Command:  return FText::FromString(TEXT("Command"));
		case EGCGCardType::Base:     return FText::FromString(TEXT("Base"));
		case EGCGCardType::Pilot:    return FText::FromString(TEXT("Pilot"));
		default:                     return FText::FromString(TEXT("Unknown"));
	}
}

FText UGCGUIHelpers::FormatCardStats(int32 AP, int32 HP, int32 DamageTaken)
{
	if (DamageTaken > 0)
	{
		int32 CurrentHP = HP - DamageTaken;
		return FText::FromString(FString::Printf(TEXT("%d AP / %d HP (-%d)"), AP, CurrentHP, DamageTaken));
	}
	else
	{
		return FText::FromString(FString::Printf(TEXT("%d AP / %d HP"), AP, HP));
	}
}

// ===========================================================================================
// PLAYER DATA CONVERSION
// ===========================================================================================

FGCGUIPlayerStatus UGCGUIHelpers::ConvertPlayerToUIStatus(AGCGPlayerState* PlayerState, AGCGGameState* GameState)
{
	FGCGUIPlayerStatus UIStatus;

	if (!PlayerState)
	{
		return UIStatus;
	}

	UIStatus.PlayerID = PlayerState->PlayerID;
	UIStatus.PlayerName = PlayerState->GetPlayerName();
	UIStatus.HP = PlayerState->HP;
	UIStatus.MaxHP = PlayerState->MaxHP;
	UIStatus.HandCount = PlayerState->Hand.Num();
	UIStatus.DeckCount = PlayerState->Deck.Num();
	UIStatus.ResourceCount = PlayerState->ResourceArea.Num();
	UIStatus.ShieldCount = PlayerState->ShieldStack.Num();
	UIStatus.BattleAreaCount = PlayerState->BattleArea.Num();

	if (GameState)
	{
		UIStatus.bIsActivePlayer = (GameState->ActivePlayerID == PlayerState->PlayerID);
		UIStatus.bHasPriority = (GameState->PriorityPlayerID == PlayerState->PlayerID);
	}

	return UIStatus;
}

TArray<FGCGUIPlayerStatus> UGCGUIHelpers::GetAllPlayersUIStatus(AGCGGameState* GameState)
{
	TArray<FGCGUIPlayerStatus> Statuses;

	if (!GameState)
	{
		return Statuses;
	}

	for (APlayerState* PS : GameState->PlayerArray)
	{
		AGCGPlayerState* PlayerState = Cast<AGCGPlayerState>(PS);
		if (PlayerState)
		{
			Statuses.Add(ConvertPlayerToUIStatus(PlayerState, GameState));
		}
	}

	return Statuses;
}

// ===========================================================================================
// PHASE & ZONE DISPLAY
// ===========================================================================================

FText UGCGUIHelpers::GetPhaseDisplayName(EGCGTurnPhase Phase)
{
	switch (Phase)
	{
		case EGCGTurnPhase::StartPhase:      return FText::FromString(TEXT("Start Phase"));
		case EGCGTurnPhase::DrawPhase:       return FText::FromString(TEXT("Draw Phase"));
		case EGCGTurnPhase::ResourcePhase:   return FText::FromString(TEXT("Resource Phase"));
		case EGCGTurnPhase::MainPhase:       return FText::FromString(TEXT("Main Phase"));
		case EGCGTurnPhase::AttackPhase:     return FText::FromString(TEXT("Attack Phase"));
		case EGCGTurnPhase::EndPhase:        return FText::FromString(TEXT("End Phase"));
		default:                             return FText::FromString(TEXT("Unknown Phase"));
	}
}

FText UGCGUIHelpers::GetPhaseDescription(EGCGTurnPhase Phase)
{
	switch (Phase)
	{
		case EGCGTurnPhase::StartPhase:
			return FText::FromString(TEXT("Untap all cards. Triggers start-of-turn effects."));
		case EGCGTurnPhase::DrawPhase:
			return FText::FromString(TEXT("Draw 1 card from your deck."));
		case EGCGTurnPhase::ResourcePhase:
			return FText::FromString(TEXT("Place 1 card from your hand as a resource."));
		case EGCGTurnPhase::MainPhase:
			return FText::FromString(TEXT("Play Units, Commands, and activate effects."));
		case EGCGTurnPhase::AttackPhase:
			return FText::FromString(TEXT("Declare attacks with your Units."));
		case EGCGTurnPhase::EndPhase:
			return FText::FromString(TEXT("Discard down to 10 cards. Triggers end-of-turn effects."));
		default:
			return FText::FromString(TEXT(""));
	}
}

FText UGCGUIHelpers::GetZoneDisplayName(EGCGCardZone Zone)
{
	switch (Zone)
	{
		case EGCGCardZone::Hand:          return FText::FromString(TEXT("Hand"));
		case EGCGCardZone::Deck:          return FText::FromString(TEXT("Deck"));
		case EGCGCardZone::ResourceDeck:  return FText::FromString(TEXT("Resource Deck"));
		case EGCGCardZone::ResourceArea:  return FText::FromString(TEXT("Resource Area"));
		case EGCGCardZone::BattleArea:    return FText::FromString(TEXT("Battle Area"));
		case EGCGCardZone::ShieldStack:   return FText::FromString(TEXT("Shield Stack"));
		case EGCGCardZone::BaseSection:   return FText::FromString(TEXT("Base Section"));
		case EGCGCardZone::Trash:         return FText::FromString(TEXT("Trash"));
		case EGCGCardZone::Removal:       return FText::FromString(TEXT("Removed from Game"));
		default:                          return FText::FromString(TEXT("Unknown Zone"));
	}
}

FText UGCGUIHelpers::GetCombatStepDisplayName(EGCGCombatStep Step)
{
	switch (Step)
	{
		case EGCGCombatStep::AttackDeclaration:   return FText::FromString(TEXT("Declare Attackers"));
		case EGCGCombatStep::BlockDeclaration:    return FText::FromString(TEXT("Declare Blockers"));
		case EGCGCombatStep::ActionWindow:        return FText::FromString(TEXT("Combat Actions"));
		case EGCGCombatStep::DamageResolution:    return FText::FromString(TEXT("Resolve Damage"));
		case EGCGCombatStep::BattleEnd:           return FText::FromString(TEXT("End of Combat"));
		default:                                  return FText::FromString(TEXT("None"));
	}
}

// ===========================================================================================
// COMBAT DATA CONVERSION
// ===========================================================================================

FGCGUIAttackData UGCGUIHelpers::ConvertAttackToUIData(const FGCGAttackInfo& AttackInfo, AGCGGameState* GameState)
{
	FGCGUIAttackData UIData;

	// Basic attack info
	UIData.AttackingPlayerID = AttackInfo.AttackingPlayerID;
	UIData.DefendingPlayerID = AttackInfo.DefendingPlayerID;
	UIData.bIsBlocked = AttackInfo.bIsBlocked;

	// TODO: Find attacker and blocker card instances from game state
	// and convert them to UI data
	// This requires searching through player states

	return UIData;
}

TArray<FGCGUIAttackData> UGCGUIHelpers::GetAllAttacksUIData(AGCGGameState* GameState)
{
	TArray<FGCGUIAttackData> AttacksUIData;

	if (!GameState)
	{
		return AttacksUIData;
	}

	for (const FGCGAttackInfo& Attack : GameState->PendingAttacks)
	{
		AttacksUIData.Add(ConvertAttackToUIData(Attack, GameState));
	}

	return AttacksUIData;
}

// ===========================================================================================
// VALIDATION & LEGALITY CHECKS
// ===========================================================================================

bool UGCGUIHelpers::CanPlayCard(AGCGPlayerState* PlayerState, const FGCGCardInstance& CardInstance)
{
	if (!PlayerState)
	{
		return false;
	}

	// Check if we have enough resources
	if (CardInstance.Cost > PlayerState->ResourceArea.Num())
	{
		return false;
	}

	// Check Battle Area limit for Units
	if (CardInstance.CardType == EGCGCardType::Unit && PlayerState->BattleArea.Num() >= 6)
	{
		return false;
	}

	return true;
}

bool UGCGUIHelpers::CanUnitAttack(AGCGPlayerState* PlayerState, const FGCGCardInstance& UnitInstance, AGCGGameState* GameState)
{
	if (!PlayerState || !GameState)
	{
		return false;
	}

	// Must be active
	if (!UnitInstance.bIsActive)
	{
		return false;
	}

	// Can't have already attacked
	if (UnitInstance.bHasAttackedThisTurn)
	{
		return false;
	}

	// Check summoning sickness (would need CombatSubsystem)
	// Simplified check: can attack if deployed before this turn
	if (UnitInstance.TurnDeployed >= GameState->TurnNumber)
	{
		// Check for Link Unit exception
		if (!UnitInstance.ActiveKeywords.Contains(EGCGKeyword::LinkUnit) || UnitInstance.PairedCardInstanceID == -1)
		{
			return false;
		}
	}

	return true;
}

bool UGCGUIHelpers::CanUnitBlock(const FGCGCardInstance& UnitInstance)
{
	// Must be active OR have Blocker keyword
	return UnitInstance.bIsActive || UnitInstance.ActiveKeywords.Contains(EGCGKeyword::Blocker);
}

bool UGCGUIHelpers::IsLocalPlayerTurn(AGCGGameState* GameState, int32 LocalPlayerID)
{
	if (!GameState)
	{
		return false;
	}

	return GameState->ActivePlayerID == LocalPlayerID;
}

// ===========================================================================================
// DRAG & DROP VALIDATION
// ===========================================================================================

bool UGCGUIHelpers::ValidateDragDrop(const FGCGCardInstance& CardInstance, EGCGCardZone SourceZone, EGCGCardZone TargetZone, AGCGPlayerState* PlayerState)
{
	// Hand -> Battle Area (play Unit/Pilot)
	if (SourceZone == EGCGCardZone::Hand && TargetZone == EGCGCardZone::BattleArea)
	{
		if (CardInstance.CardType == EGCGCardType::Unit || CardInstance.CardType == EGCGCardType::Pilot)
		{
			return CanPlayCard(PlayerState, CardInstance);
		}
	}

	// Hand -> Resource Area (place resource)
	if (SourceZone == EGCGCardZone::Hand && TargetZone == EGCGCardZone::ResourceArea)
	{
		return !PlayerState->bPlacedResourceThisTurn;
	}

	return false;
}

TArray<EGCGCardZone> UGCGUIHelpers::GetValidDropZones(const FGCGCardInstance& CardInstance, EGCGCardZone SourceZone, AGCGPlayerState* PlayerState)
{
	TArray<EGCGCardZone> ValidZones;

	if (SourceZone == EGCGCardZone::Hand)
	{
		// Can play Units/Pilots to Battle Area
		if (CardInstance.CardType == EGCGCardType::Unit || CardInstance.CardType == EGCGCardType::Pilot)
		{
			if (CanPlayCard(PlayerState, CardInstance))
			{
				ValidZones.Add(EGCGCardZone::BattleArea);
			}
		}

		// Can place any card as resource
		if (!PlayerState->bPlacedResourceThisTurn)
		{
			ValidZones.Add(EGCGCardZone::ResourceArea);
		}
	}

	return ValidZones;
}

// ===========================================================================================
// FORMATTING UTILITIES
// ===========================================================================================

FText UGCGUIHelpers::FormatNumber(int32 Number)
{
	FString NumberString = FString::FromInt(Number);
	FString FormattedString;

	int32 CharCount = 0;
	for (int32 i = NumberString.Len() - 1; i >= 0; i--)
	{
		if (CharCount > 0 && CharCount % 3 == 0)
		{
			FormattedString.InsertAt(0, TEXT(","));
		}
		FormattedString.InsertAt(0, FString::Chr(NumberString[i]));
		CharCount++;
	}

	return FText::FromString(FormattedString);
}

FText UGCGUIHelpers::FormatHP(int32 CurrentHP, int32 MaxHP, int32 DamageTaken)
{
	if (DamageTaken > 0)
	{
		return FText::FromString(FString::Printf(TEXT("%d/%d (-%d)"), CurrentHP, MaxHP, DamageTaken));
	}
	else
	{
		return FText::FromString(FString::Printf(TEXT("%d/%d"), CurrentHP, MaxHP));
	}
}

FLinearColor UGCGUIHelpers::GetHPColor(int32 CurrentHP, int32 MaxHP)
{
	if (MaxHP <= 0)
	{
		return FLinearColor::Gray;
	}

	float HPPercent = static_cast<float>(CurrentHP) / static_cast<float>(MaxHP);

	if (HPPercent > 0.66f)
	{
		return FLinearColor::Green;
	}
	else if (HPPercent > 0.33f)
	{
		return FLinearColor::Yellow;
	}
	else
	{
		return FLinearColor::Red;
	}
}

// ===========================================================================================
// CARD SEARCH & FILTERING
// ===========================================================================================

TArray<FGCGCardInstance> UGCGUIHelpers::FilterCardsByType(const TArray<FGCGCardInstance>& Cards, EGCGCardType CardType)
{
	TArray<FGCGCardInstance> Filtered;

	for (const FGCGCardInstance& Card : Cards)
	{
		if (Card.CardType == CardType)
		{
			Filtered.Add(Card);
		}
	}

	return Filtered;
}

TArray<FGCGCardInstance> UGCGUIHelpers::FilterCardsByColor(const TArray<FGCGCardInstance>& Cards, EGCGColor Color)
{
	TArray<FGCGCardInstance> Filtered;

	for (const FGCGCardInstance& Card : Cards)
	{
		if (Card.Colors.Contains(Color))
		{
			Filtered.Add(Card);
		}
	}

	return Filtered;
}

TArray<FGCGCardInstance> UGCGUIHelpers::FilterCardsByKeyword(const TArray<FGCGCardInstance>& Cards, EGCGKeyword Keyword)
{
	TArray<FGCGCardInstance> Filtered;

	for (const FGCGCardInstance& Card : Cards)
	{
		if (Card.ActiveKeywords.Contains(Keyword))
		{
			Filtered.Add(Card);
		}
	}

	return Filtered;
}

TArray<FGCGCardInstance> UGCGUIHelpers::SortCardsByCost(const TArray<FGCGCardInstance>& Cards, bool bAscending)
{
	TArray<FGCGCardInstance> Sorted = Cards;

	Sorted.Sort([bAscending](const FGCGCardInstance& A, const FGCGCardInstance& B) {
		return bAscending ? (A.Cost < B.Cost) : (A.Cost > B.Cost);
	});

	return Sorted;
}

TArray<FGCGCardInstance> UGCGUIHelpers::SortCardsByAP(const TArray<FGCGCardInstance>& Cards, bool bAscending)
{
	TArray<FGCGCardInstance> Sorted = Cards;

	Sorted.Sort([bAscending](const FGCGCardInstance& A, const FGCGCardInstance& B) {
		return bAscending ? (A.AP < B.AP) : (A.AP > B.AP);
	});

	return Sorted;
}

// ===========================================================================================
// ANIMATION & VFX HELPERS
// ===========================================================================================

float UGCGUIHelpers::GetZoneTransitionDuration(EGCGCardZone FromZone, EGCGCardZone ToZone)
{
	// Hand to Battle Area: 0.5s
	if (FromZone == EGCGCardZone::Hand && ToZone == EGCGCardZone::BattleArea)
	{
		return 0.5f;
	}

	// Hand to Resource: 0.3s
	if (FromZone == EGCGCardZone::Hand && ToZone == EGCGCardZone::ResourceArea)
	{
		return 0.3f;
	}

	// Battle Area to Trash: 0.4s
	if (FromZone == EGCGCardZone::BattleArea && ToZone == EGCGCardZone::Trash)
	{
		return 0.4f;
	}

	// Default: 0.3s
	return 0.3f;
}

bool UGCGUIHelpers::ShouldPlayEnterAnimation(const FGCGCardInstance& CardInstance, EGCGCardZone ToZone)
{
	// Play animation when entering Battle Area
	if (ToZone == EGCGCardZone::BattleArea)
	{
		return true;
	}

	// Play animation when entering Base Section
	if (ToZone == EGCGCardZone::BaseSection)
	{
		return true;
	}

	return false;
}

FVector2D UGCGUIHelpers::GetZoneScreenPosition(EGCGCardZone Zone, int32 PlayerID, FVector2D ViewportSize)
{
	// Normalized screen positions (0-1)
	// Layout assumes Player 0 at bottom, opponents at top

	bool bIsLocalPlayer = (PlayerID == 0); // Simplified assumption

	switch (Zone)
	{
		case EGCGCardZone::Hand:
			return bIsLocalPlayer ? FVector2D(0.5f, 0.9f) : FVector2D(0.5f, 0.1f);

		case EGCGCardZone::BattleArea:
			return bIsLocalPlayer ? FVector2D(0.5f, 0.6f) : FVector2D(0.5f, 0.4f);

		case EGCGCardZone::ResourceArea:
			return bIsLocalPlayer ? FVector2D(0.2f, 0.7f) : FVector2D(0.2f, 0.3f);

		case EGCGCardZone::Deck:
			return bIsLocalPlayer ? FVector2D(0.1f, 0.8f) : FVector2D(0.1f, 0.2f);

		case EGCGCardZone::Trash:
			return bIsLocalPlayer ? FVector2D(0.9f, 0.8f) : FVector2D(0.9f, 0.2f);

		case EGCGCardZone::ShieldStack:
			return bIsLocalPlayer ? FVector2D(0.8f, 0.7f) : FVector2D(0.8f, 0.3f);

		default:
			return FVector2D(0.5f, 0.5f);
	}
}
