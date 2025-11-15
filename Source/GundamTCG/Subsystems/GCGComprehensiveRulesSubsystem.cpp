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

// ===== PLACEHOLDER METHODS FOR SECTIONS 3-13 =====
// These will be implemented when comprehensive rules sections are provided

FGCGRulesValidationResult UGCGComprehensiveRulesSubsystem::ValidateSection3_Placeholder() const
{
	return FGCGRulesValidationResult(true, TEXT("3-X"), TEXT("Section 3 not yet implemented"));
}

FGCGRulesValidationResult UGCGComprehensiveRulesSubsystem::ValidateSection4_Placeholder() const
{
	return FGCGRulesValidationResult(true, TEXT("4-X"), TEXT("Section 4 not yet implemented"));
}

FGCGRulesValidationResult UGCGComprehensiveRulesSubsystem::ValidateSection5_Placeholder() const
{
	return FGCGRulesValidationResult(true, TEXT("5-X"), TEXT("Section 5 not yet implemented"));
}

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
