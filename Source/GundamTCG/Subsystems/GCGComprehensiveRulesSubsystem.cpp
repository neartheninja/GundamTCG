// GCGComprehensiveRulesSubsystem.cpp - Comprehensive Rules Validation Implementation
// Unreal Engine 5.6 - Gundam TCG Implementation

#include "GCGComprehensiveRulesSubsystem.h"
#include "GCGCardDatabase.h"
#include "GundamTCG/PlayerState/GCGPlayerState.h"
#include "GundamTCG/GameState/GCGGameState.h"

void UGCGComprehensiveRulesSubsystem::Initialize(FSubsystemCollectionBase& Collection)
{
	Super::Initialize(Collection);

	UE_LOG(LogTemp, Log, TEXT("UGCGComprehensiveRulesSubsystem::Initialize - Comprehensive Rules validation system initialized"));

	CachedCardDatabase = nullptr;
}

void UGCGComprehensiveRulesSubsystem::Deinitialize()
{
	Super::Deinitialize();

	UE_LOG(LogTemp, Log, TEXT("UGCGComprehensiveRulesSubsystem::Deinitialize - Shutting down"));
}

// ===== SECTION 2: CARD INFORMATION =====

FGCGRulesValidationResult UGCGComprehensiveRulesSubsystem::ValidateRule_2_1_2_MaxCopies(const TArray<FName>& CardNumbers) const
{
	// Rule 2-1-2: A deck can include up to four cards with the same card number

	TMap<FName, int32> CardCounts;

	for (const FName& CardNumber : CardNumbers)
	{
		int32& Count = CardCounts.FindOrAdd(CardNumber, 0);
		Count++;

		if (Count > 4)
		{
			return FGCGRulesValidationResult(
				false,
				TEXT("2-1-2"),
				FString::Printf(TEXT("Card %s appears %d times (maximum 4 copies allowed)"),
					*CardNumber.ToString(), Count)
			);
		}
	}

	return FGCGRulesValidationResult(true, TEXT("2-1-2"));
}

FGCGRulesValidationResult UGCGComprehensiveRulesSubsystem::ValidateRule_2_4_3_DeckColors(const TArray<EGCGCardColor>& DeckColors) const
{
	// Rule 2-4-3: A deck can only include cards of up to two colors (not counting colorless)

	TSet<EGCGCardColor> UniqueColors;

	for (const EGCGCardColor& Color : DeckColors)
	{
		// Don't count Colorless toward the limit
		if (Color != EGCGCardColor::Colorless)
		{
			UniqueColors.Add(Color);
		}
	}

	if (UniqueColors.Num() > 2)
	{
		FString ColorList;
		for (const EGCGCardColor& Color : UniqueColors)
		{
			if (!ColorList.IsEmpty())
			{
				ColorList += TEXT(", ");
			}
			ColorList += UEnum::GetDisplayValueAsText(Color).ToString();
		}

		return FGCGRulesValidationResult(
			false,
			TEXT("2-4-3"),
			FString::Printf(TEXT("Deck has %d colors (%s), maximum 2 colors allowed (not counting Colorless)"),
				UniqueColors.Num(), *ColorList)
		);
	}

	return FGCGRulesValidationResult(true, TEXT("2-4-3"));
}

bool UGCGComprehensiveRulesSubsystem::ValidateRule_2_8_2_CardDestruction(const FGCGCardInstance& Card) const
{
	// Rule 2-8-2: When a card's HP becomes 0, that card is destroyed

	UGCGCardDatabase* CardDB = GetCardDatabase();
	if (!CardDB)
	{
		UE_LOG(LogTemp, Warning, TEXT("UGCGComprehensiveRulesSubsystem::ValidateRule_2_8_2_CardDestruction - Card database not available"));
		return false;
	}

	const FGCGCardData* CardData = CardDB->GetCardData(Card.CardNumber);
	if (!CardData)
	{
		UE_LOG(LogTemp, Warning, TEXT("UGCGComprehensiveRulesSubsystem::ValidateRule_2_8_2_CardDestruction - Card data not found for %s"),
			*Card.CardNumber.ToString());
		return false;
	}

	// Calculate effective HP
	int32 EffectiveHP = CardData->HP;

	// If card is a Unit with a linked Pilot, add Pilot's HP
	// TODO: Implement Pilot pairing HP bonus when Pilot system is ready

	// Card is destroyed if damage >= HP
	return Card.CurrentDamage >= EffectiveHP;
}

int32 UGCGComprehensiveRulesSubsystem::ValidateRule_2_9_2_PlayerLv(const AGCGPlayerState* PlayerState) const
{
	// Rule 2-9-2: A player's Lv is equal to the number of cards in their resource area
	// that are active, plus the number of EX Resource tokens they control

	if (!PlayerState)
	{
		UE_LOG(LogTemp, Warning, TEXT("UGCGComprehensiveRulesSubsystem::ValidateRule_2_9_2_PlayerLv - Invalid PlayerState"));
		return 0;
	}

	int32 ActiveResources = 0;
	int32 EXResources = 0;

	for (const FGCGCardInstance& Resource : PlayerState->ResourceArea)
	{
		// EX Resource tokens always contribute to Lv (even if rested)
		if (Resource.bIsToken && Resource.TokenType == FName("EXResource"))
		{
			EXResources++;
		}
		// Regular resources only contribute if active (not rested)
		else if (Resource.bIsActive)
		{
			ActiveResources++;
		}
	}

	int32 TotalLv = ActiveResources + EXResources;

	UE_LOG(LogTemp, VeryVerbose, TEXT("UGCGComprehensiveRulesSubsystem::ValidateRule_2_9_2_PlayerLv - Player %d Lv: %d (Active: %d, EX: %d)"),
		PlayerState->GetPlayerID(), TotalLv, ActiveResources, EXResources);

	return TotalLv;
}

// ===== SECTION 3: CARD TYPES =====

FGCGRulesValidationResult UGCGComprehensiveRulesSubsystem::ValidateRule_3_2_3_OnlyUnitsAttack(const FGCGCardInstance& Card) const
{
	// Rule 3-2-3: Only Units can attack

	UGCGCardDatabase* CardDB = GetCardDatabase();
	if (!CardDB)
	{
		return FGCGRulesValidationResult(false, TEXT("3-2-3"), TEXT("Card database not available"));
	}

	const FGCGCardData* CardData = CardDB->GetCardData(Card.CardNumber);
	if (!CardData)
	{
		return FGCGRulesValidationResult(false, TEXT("3-2-3"), TEXT("Card data not found"));
	}

	if (CardData->CardType != EGCGCardType::Unit)
	{
		FString TypeName = UEnum::GetDisplayValueAsText(CardData->CardType).ToString();
		return FGCGRulesValidationResult(
			false,
			TEXT("3-2-3"),
			FString::Printf(TEXT("Only Units can attack (this is a %s)"), *TypeName)
		);
	}

	return FGCGRulesValidationResult(true, TEXT("3-2-3"));
}

FGCGRulesValidationResult UGCGComprehensiveRulesSubsystem::ValidateRule_3_2_4_SummoningSickness(
	const FGCGCardInstance& Unit,
	bool bDeployedThisTurn,
	bool bIsLinkUnit) const
{
	// Rule 3-2-4: Newly deployed Units cannot attack
	// Exception: Link Units can attack immediately (Rule 3-2-6-3)

	if (bDeployedThisTurn && !bIsLinkUnit)
	{
		return FGCGRulesValidationResult(
			false,
			TEXT("3-2-4"),
			TEXT("Unit cannot attack on turn it was deployed (summoning sickness)")
		);
	}

	return FGCGRulesValidationResult(true, TEXT("3-2-4"));
}

bool UGCGComprehensiveRulesSubsystem::ValidateRule_3_2_6_4_LinkCondition(
	const FString& UnitLinkCondition,
	const FString& PilotName,
	const TArray<FName>& PilotTraits) const
{
	// Rule 3-2-6-4: Link condition matching with [xyz] partial name syntax

	if (UnitLinkCondition.IsEmpty())
	{
		// No link condition specified - any Pilot can link
		return true;
	}

	// Check for bracketed partial name syntax [xyz]
	if (UnitLinkCondition.Contains(TEXT("[")))
	{
		int32 StartIdx = UnitLinkCondition.Find(TEXT("["));
		int32 EndIdx = UnitLinkCondition.Find(TEXT("]"), ESearchCase::IgnoreCase, ESearchDir::FromStart, StartIdx);

		if (EndIdx != INDEX_NONE)
		{
			// Extract text between brackets
			int32 Length = EndIdx - StartIdx - 1;
			FString RequiredText = UnitLinkCondition.Mid(StartIdx + 1, Length);

			// Check if Pilot name contains this text
			if (PilotName.Contains(RequiredText))
			{
				UE_LOG(LogTemp, Verbose, TEXT("ValidateRule_3_2_6_4 - Link condition satisfied: Pilot '%s' contains '%s'"),
					*PilotName, *RequiredText);
				return true;
			}
		}
	}

	// Check for exact name match
	if (UnitLinkCondition.Equals(PilotName, ESearchCase::IgnoreCase))
	{
		UE_LOG(LogTemp, Verbose, TEXT("ValidateRule_3_2_6_4 - Link condition satisfied: Exact name match '%s'"),
			*PilotName);
		return true;
	}

	// Check for trait match
	for (const FName& Trait : PilotTraits)
	{
		if (UnitLinkCondition.Contains(Trait.ToString()))
		{
			UE_LOG(LogTemp, Verbose, TEXT("ValidateRule_3_2_6_4 - Link condition satisfied: Pilot has trait '%s'"),
				*Trait.ToString());
			return true;
		}
	}

	UE_LOG(LogTemp, Warning, TEXT("ValidateRule_3_2_6_4 - Link condition NOT satisfied: Pilot '%s' does not match '%s'"),
		*PilotName, *UnitLinkCondition);
	return false;
}

FGCGRulesValidationResult UGCGComprehensiveRulesSubsystem::ValidateRule_3_3_3_PilotRequiresUnit(
	const AGCGPlayerState* PlayerState,
	const FGCGCardInstance& Pilot) const
{
	// Rule 3-3-3: Pilot can only exist in battle area if paired with Unit

	if (!PlayerState)
	{
		return FGCGRulesValidationResult(false, TEXT("3-3-3"), TEXT("Invalid player state"));
	}

	// Must have at least one Unit in battle area
	if (PlayerState->BattleArea.Num() == 0)
	{
		return FGCGRulesValidationResult(
			false,
			TEXT("3-3-3"),
			TEXT("Cannot play Pilot - no Units in battle area")
		);
	}

	// Check if at least one Unit can accept this Pilot
	UGCGCardDatabase* CardDB = GetCardDatabase();
	if (!CardDB)
	{
		return FGCGRulesValidationResult(false, TEXT("3-3-3"), TEXT("Card database not available"));
	}

	const FGCGCardData* PilotData = CardDB->GetCardData(Pilot.CardNumber);
	if (!PilotData)
	{
		return FGCGRulesValidationResult(false, TEXT("3-3-3"), TEXT("Pilot card data not found"));
	}

	bool bHasValidTarget = false;
	for (const FGCGCardInstance& Unit : PlayerState->BattleArea)
	{
		const FGCGCardData* UnitData = CardDB->GetCardData(Unit.CardNumber);
		if (UnitData && UnitData->CardType == EGCGCardType::Unit)
		{
			// Check if Unit already has a Pilot (Rule 3-3-4)
			// TODO: Check Unit.LinkedPilot.IsSet() when field is added

			// Check if link condition is satisfied (Rule 3-2-6-4)
			if (ValidateRule_3_2_6_4_LinkCondition(
				UnitData->LinkCondition.ToString(),
				PilotData->CardName.ToString(),
				PilotData->Traits))
			{
				bHasValidTarget = true;
				break;
			}
		}
	}

	if (!bHasValidTarget)
	{
		return FGCGRulesValidationResult(
			false,
			TEXT("3-3-3"),
			TEXT("Cannot play Pilot - no valid Units to pair with")
		);
	}

	return FGCGRulesValidationResult(true, TEXT("3-3-3"));
}

FGCGRulesValidationResult UGCGComprehensiveRulesSubsystem::ValidateRule_3_3_4_OnePilotPerUnit(
	const FGCGCardInstance& Unit,
	bool bAlreadyHasPilot) const
{
	// Rule 3-3-4: Unit can have at most one Pilot

	if (bAlreadyHasPilot)
	{
		return FGCGRulesValidationResult(
			false,
			TEXT("3-3-4"),
			TEXT("Unit already has a Pilot - cannot pair another")
		);
	}

	return FGCGRulesValidationResult(true, TEXT("3-3-4"));
}

// ===== SECTION 4: GAME LOCATIONS =====

FGCGRulesValidationResult UGCGComprehensiveRulesSubsystem::ValidateRule_4_4_2_ResourceAreaLimit(const AGCGPlayerState* PlayerState) const
{
	// Rule 4-4-2: Max 15 resources in resource area
	// Rule 4-4-2-1: Max 5 EX Resources

	if (!PlayerState)
	{
		return FGCGRulesValidationResult(false, TEXT("4-4-2"), TEXT("Invalid player state"));
	}

	int32 TotalResources = PlayerState->ResourceArea.Num();

	// Rule 4-4-2: Max 15 resources
	if (TotalResources >= 15)
	{
		return FGCGRulesValidationResult(
			false,
			TEXT("4-4-2"),
			FString::Printf(TEXT("Resource area is full (%d/15 resources)"), TotalResources)
		);
	}

	// Rule 4-4-2-1: Max 5 EX Resources
	int32 EXResourceCount = 0;
	for (const FGCGCardInstance& Resource : PlayerState->ResourceArea)
	{
		if (Resource.bIsToken && Resource.TokenType == FName("EXResource"))
		{
			EXResourceCount++;
		}
	}

	if (EXResourceCount >= 5)
	{
		return FGCGRulesValidationResult(
			false,
			TEXT("4-4-2-1"),
			FString::Printf(TEXT("EX Resource limit reached (%d/5 EX Resources)"), EXResourceCount)
		);
	}

	return FGCGRulesValidationResult(true, TEXT("4-4-2"));
}

FGCGRulesValidationResult UGCGComprehensiveRulesSubsystem::ValidateRule_4_5_4_BattleAreaLimit(const AGCGPlayerState* PlayerState) const
{
	// Rule 4-5-4: Max 6 Units in battle area

	if (!PlayerState)
	{
		return FGCGRulesValidationResult(false, TEXT("4-5-4"), TEXT("Invalid player state"));
	}

	UGCGCardDatabase* CardDB = GetCardDatabase();
	if (!CardDB)
	{
		return FGCGRulesValidationResult(false, TEXT("4-5-4"), TEXT("Card database not available"));
	}

	// Count only Units (not Pilots stored beneath them)
	int32 UnitCount = 0;
	for (const FGCGCardInstance& Card : PlayerState->BattleArea)
	{
		const FGCGCardData* CardData = CardDB->GetCardData(Card.CardNumber);
		if (CardData && CardData->CardType == EGCGCardType::Unit)
		{
			UnitCount++;
		}
	}

	if (UnitCount >= 6)
	{
		return FGCGRulesValidationResult(
			false,
			TEXT("4-5-4"),
			FString::Printf(TEXT("Battle area is full (%d/6 Units)"), UnitCount)
		);
	}

	return FGCGRulesValidationResult(true, TEXT("4-5-4"));
}

FGCGRulesValidationResult UGCGComprehensiveRulesSubsystem::ValidateRule_4_6_3_BaseSectionLimit(const AGCGPlayerState* PlayerState) const
{
	// Rule 4-6-3: Max 1 Base in base section

	if (!PlayerState)
	{
		return FGCGRulesValidationResult(false, TEXT("4-6-3"), TEXT("Invalid player state"));
	}

	if (PlayerState->BaseSection.Num() >= 1)
	{
		return FGCGRulesValidationResult(
			false,
			TEXT("4-6-3"),
			TEXT("Base section already has a Base (maximum 1)")
		);
	}

	return FGCGRulesValidationResult(true, TEXT("4-6-3"));
}

FGCGRulesValidationResult UGCGComprehensiveRulesSubsystem::ValidateRule_4_8_4_HandSizeLimit(
	const AGCGPlayerState* PlayerState,
	bool bIsEndPhase) const
{
	// Rule 4-8-4: Hand limit of 10 cards, enforced during end phase

	if (!PlayerState)
	{
		return FGCGRulesValidationResult(false, TEXT("4-8-4"), TEXT("Invalid player state"));
	}

	const int32 MaxHandSize = 10;
	int32 CurrentHandSize = PlayerState->Hand.Num();

	// Rule 4-8-4: Hand limit only enforced during end phase
	if (!bIsEndPhase)
	{
		// Can temporarily exceed limit during other phases
		return FGCGRulesValidationResult(true, TEXT("4-8-4"));
	}

	if (CurrentHandSize > MaxHandSize)
	{
		int32 CardsToDiscard = CurrentHandSize - MaxHandSize;
		return FGCGRulesValidationResult(
			false,
			TEXT("4-8-4"),
			FString::Printf(TEXT("Hand size exceeds limit (%d/%d) - must discard %d card(s)"),
				CurrentHandSize, MaxHandSize, CardsToDiscard)
		);
	}

	return FGCGRulesValidationResult(true, TEXT("4-8-4"));
}

// ===== SECTION 5: ESSENTIAL GAME TERMINOLOGY =====

int32 UGCGComprehensiveRulesSubsystem::RecoverHP(FGCGCardInstance& Card, int32 RecoveryAmount) const
{
	// Rule 5-6-3: Undamaged cards cannot recover HP
	if (Card.CurrentDamage == 0)
	{
		UE_LOG(LogTemp, Verbose, TEXT("RecoverHP - Card has no damage, cannot recover"));
		return 0;
	}

	// Rule 5-6-1: Remove damage counters
	int32 ActualRecovery = FMath::Min(RecoveryAmount, Card.CurrentDamage);

	// Rule 5-6-2: Cannot exceed max HP (remove all counters at most)
	Card.CurrentDamage -= ActualRecovery;

	UE_LOG(LogTemp, Log, TEXT("RecoverHP - Recovered %d HP (requested %d, current damage now %d)"),
		ActualRecovery, RecoveryAmount, Card.CurrentDamage);

	return ActualRecovery;
}

bool UGCGComprehensiveRulesSubsystem::IsCardColorless(const FGCGCardInstance& Card) const
{
	// Rule 5-17-2-3: Tokens are treated as having no color
	if (Card.bIsToken)
	{
		return true;
	}

	// Check card data for color
	UGCGCardDatabase* CardDB = GetCardDatabase();
	if (!CardDB)
	{
		return true; // Default to colorless if can't access database
	}

	const FGCGCardData* CardData = CardDB->GetCardData(Card.CardNumber);
	if (!CardData || CardData->Color.Num() == 0)
	{
		return true;
	}

	// Check if all colors are Colorless
	for (const EGCGCardColor& Color : CardData->Color)
	{
		if (Color != EGCGCardColor::Colorless)
		{
			return false;
		}
	}

	return true;
}

int32 UGCGComprehensiveRulesSubsystem::GetEffectiveCardCost(const FGCGCardInstance& Card) const
{
	// Rule 5-17-2-4: Token cost is 0
	if (Card.bIsToken)
	{
		return 0;
	}

	UGCGCardDatabase* CardDB = GetCardDatabase();
	if (!CardDB)
	{
		return 0;
	}

	const FGCGCardData* CardData = CardDB->GetCardData(Card.CardNumber);
	if (!CardData)
	{
		return 0;
	}

	return CardData->Cost;
}

int32 UGCGComprehensiveRulesSubsystem::GetEffectiveCardLevel(const FGCGCardInstance& Card) const
{
	// Rule 5-17-2-4: Token Lv is 0
	if (Card.bIsToken)
	{
		return 0;
	}

	UGCGCardDatabase* CardDB = GetCardDatabase();
	if (!CardDB)
	{
		return 0;
	}

	const FGCGCardData* CardData = CardDB->GetCardData(Card.CardNumber);
	if (!CardData)
	{
		return 0;
	}

	return CardData->Level;
}

FGCGCardInstance UGCGComprehensiveRulesSubsystem::CreateEXBaseToken(int32 OwnerPlayerID) const
{
	// Rule 5-17-3-1: EX Base token (0 AP / 3 HP)
	FGCGCardInstance EXBase;

	EXBase.bIsToken = true;
	EXBase.TokenType = FName("EXBase");
	EXBase.CardNumber = FName("TOKEN_EXBase");
	EXBase.OwnerPlayerID = OwnerPlayerID;
	EXBase.CurrentZone = EGCGCardZone::BaseSection;
	EXBase.bIsActive = true;
	EXBase.CurrentDamage = 0;

	// Token stats stored in instance for tokens
	// EX Base: 0 AP, 3 HP (retrieved from token data when needed)

	UE_LOG(LogTemp, Log, TEXT("CreateEXBaseToken - Created EX Base for Player %d (0 AP / 3 HP)"),
		OwnerPlayerID);

	return EXBase;
}

bool UGCGComprehensiveRulesSubsystem::MatchesTraitCondition(const FString& Condition, const TArray<FName>& CardTraits) const
{
	// Rule 5-19-1: "/" means "or"

	if (Condition.IsEmpty())
	{
		return true; // Empty condition matches everything
	}

	// Check for "/" operator
	if (Condition.Contains(TEXT("/")))
	{
		TArray<FString> Options;
		Condition.ParseIntoArray(Options, TEXT("/"));

		// Check if card has ANY of the traits (OR logic)
		for (const FString& Option : Options)
		{
			FString TrimmedOption = Option.TrimStartAndEnd();
			FName TraitName(*TrimmedOption);

			if (CardTraits.Contains(TraitName))
			{
				UE_LOG(LogTemp, Verbose, TEXT("MatchesTraitCondition - Matched trait '%s' from condition '%s'"),
					*TrimmedOption, *Condition);
				return true;
			}
		}

		UE_LOG(LogTemp, Verbose, TEXT("MatchesTraitCondition - No traits matched condition '%s'"), *Condition);
		return false;
	}

	// Single trait check
	FName TraitName(*Condition.TrimStartAndEnd());
	return CardTraits.Contains(TraitName);
}

// ===== PLACEHOLDER METHODS FOR SECTIONS 6-13 =====
// These will be implemented when comprehensive rules sections are provided

FGCGRulesValidationResult UGCGComprehensiveRulesSubsystem::ValidateSection6_Placeholder() const
{
	return FGCGRulesValidationResult(true, TEXT("6-X"), TEXT("Section 6 not yet implemented"));
}

FGCGRulesValidationResult UGCGComprehensiveRulesSubsystem::ValidateSection7_Placeholder() const
{
	return FGCGRulesValidationResult(true, TEXT("7-X"), TEXT("Section 7 not yet implemented"));
}

FGCGRulesValidationResult UGCGComprehensiveRulesSubsystem::ValidateSection8_Placeholder() const
{
	return FGCGRulesValidationResult(true, TEXT("8-X"), TEXT("Section 8 not yet implemented"));
}

FGCGRulesValidationResult UGCGComprehensiveRulesSubsystem::ValidateSection9_Placeholder() const
{
	return FGCGRulesValidationResult(true, TEXT("9-X"), TEXT("Section 9 not yet implemented"));
}

FGCGRulesValidationResult UGCGComprehensiveRulesSubsystem::ValidateSection10_Placeholder() const
{
	return FGCGRulesValidationResult(true, TEXT("10-X"), TEXT("Section 10 not yet implemented"));
}

FGCGRulesValidationResult UGCGComprehensiveRulesSubsystem::ValidateSection11_Placeholder() const
{
	return FGCGRulesValidationResult(true, TEXT("11-X"), TEXT("Section 11 not yet implemented"));
}

FGCGRulesValidationResult UGCGComprehensiveRulesSubsystem::ValidateSection12_Placeholder() const
{
	return FGCGRulesValidationResult(true, TEXT("12-X"), TEXT("Section 12 not yet implemented"));
}

FGCGRulesValidationResult UGCGComprehensiveRulesSubsystem::ValidateSection13_Placeholder() const
{
	return FGCGRulesValidationResult(true, TEXT("13-X"), TEXT("Section 13 not yet implemented"));
}

// ===== HELPER FUNCTIONS =====

UGCGCardDatabase* UGCGComprehensiveRulesSubsystem::GetCardDatabase() const
{
	if (!CachedCardDatabase)
	{
		CachedCardDatabase = GetGameInstance()->GetSubsystem<UGCGCardDatabase>();
	}

	return CachedCardDatabase;
}
