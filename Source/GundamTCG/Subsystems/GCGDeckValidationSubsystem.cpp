// GCGDeckValidationSubsystem.cpp - Deck Construction Validation Implementation
// Unreal Engine 5.6 - Gundam TCG Implementation

#include "GCGDeckValidationSubsystem.h"
#include "GCGCardDatabase.h"

// ===== SUBSYSTEM LIFECYCLE =====

void UGCGDeckValidationSubsystem::Initialize(FSubsystemCollectionBase& Collection)
{
	Super::Initialize(Collection);

	UE_LOG(LogTemp, Log, TEXT("UGCGDeckValidationSubsystem::Initialize - Deck Validation Subsystem initialized"));

	// Cache card database
	CachedCardDatabase = GetGameInstance()->GetSubsystem<UGCGCardDatabase>();
	if (!CachedCardDatabase)
	{
		UE_LOG(LogTemp, Error, TEXT("UGCGDeckValidationSubsystem::Initialize - Card Database not found"));
	}
}

void UGCGDeckValidationSubsystem::Deinitialize()
{
	UE_LOG(LogTemp, Log, TEXT("UGCGDeckValidationSubsystem::Deinitialize - Deck Validation Subsystem shutdown"));

	CachedCardDatabase = nullptr;

	Super::Deinitialize();
}

// ===== COMPLETE DECK VALIDATION =====

FGCGDeckValidationResult UGCGDeckValidationSubsystem::ValidateCompleteDeck(const TArray<FName>& MainDeckList, const TArray<FName>& ResourceDeckList)
{
	FGCGDeckValidationResult Result;

	UE_LOG(LogTemp, Log, TEXT("UGCGDeckValidationSubsystem::ValidateCompleteDeck - Validating deck (Main: %d, Resource: %d)"),
		MainDeckList.Num(), ResourceDeckList.Num());

	// Rule 6-1-1: Validate deck sizes (50 + 10)
	if (!ValidateDeckSize(MainDeckList, ResourceDeckList))
	{
		Result.AddError(FString::Printf(TEXT("Invalid deck size: Main deck has %d cards (need 50), Resource deck has %d cards (need 10)"),
			MainDeckList.Num(), ResourceDeckList.Num()));
	}

	// Rule 6-1-1-1: Validate main deck card types
	TArray<FName> InvalidMainCards;
	if (!ValidateMainDeckCardTypes(MainDeckList, InvalidMainCards))
	{
		FString InvalidCardsList = FString::Join(InvalidMainCards, TEXT(", "));
		Result.AddError(FString::Printf(TEXT("Main deck contains invalid card types: %s (only Unit, Pilot, Command, Base allowed)"),
			*InvalidCardsList));
	}

	// Rule 6-1-1-2: Validate deck colors (1-2 colors max)
	TSet<EGCGCardColor> ColorsFound;
	if (!ValidateDeckColors(MainDeckList, ColorsFound))
	{
		Result.AddError(FString::Printf(TEXT("Deck uses too many colors (%d colors found, maximum 2 allowed)"),
			ColorsFound.Num()));
	}

	// Rule 6-1-1-3: Validate copy limits (max 4 per card)
	TMap<FName, int32> OverLimitCards;
	if (!ValidateCopyLimits(MainDeckList, OverLimitCards))
	{
		for (const TPair<FName, int32>& Pair : OverLimitCards)
		{
			Result.AddError(FString::Printf(TEXT("Card '%s' has %d copies (maximum 4 allowed)"),
				*Pair.Key.ToString(), Pair.Value));
		}
	}

	// Rule 6-1-1-4: Validate resource deck card types
	TArray<FName> InvalidResourceCards;
	if (!ValidateResourceDeckCardTypes(ResourceDeckList, InvalidResourceCards))
	{
		FString InvalidCardsList = FString::Join(InvalidResourceCards, TEXT(", "));
		Result.AddError(FString::Printf(TEXT("Resource deck contains non-Resource cards: %s"),
			*InvalidCardsList));
	}

	// Log result
	if (Result.bIsValid)
	{
		UE_LOG(LogTemp, Log, TEXT("UGCGDeckValidationSubsystem::ValidateCompleteDeck - Deck is VALID"));
	}
	else
	{
		UE_LOG(LogTemp, Warning, TEXT("UGCGDeckValidationSubsystem::ValidateCompleteDeck - Deck is INVALID (%d errors)"),
			Result.Errors.Num());
		for (const FString& Error : Result.Errors)
		{
			UE_LOG(LogTemp, Warning, TEXT("  - %s"), *Error);
		}
	}

	return Result;
}

// ===== INDIVIDUAL RULE VALIDATION =====

bool UGCGDeckValidationSubsystem::ValidateDeckSize(const TArray<FName>& MainDeckList, const TArray<FName>& ResourceDeckList) const
{
	// Rule 6-1-1: Main deck = 50, Resource deck = 10
	bool bMainDeckValid = MainDeckList.Num() == 50;
	bool bResourceDeckValid = ResourceDeckList.Num() == 10;

	return bMainDeckValid && bResourceDeckValid;
}

bool UGCGDeckValidationSubsystem::ValidateMainDeckCardTypes(const TArray<FName>& MainDeckList, TArray<FName>& OutInvalidCards)
{
	// Rule 6-1-1-1: Main deck can only contain Unit, Pilot, Command, Base cards

	UGCGCardDatabase* CardDB = GetCardDatabase();
	if (!CardDB)
	{
		UE_LOG(LogTemp, Error, TEXT("UGCGDeckValidationSubsystem::ValidateMainDeckCardTypes - Card database not available"));
		return false;
	}

	OutInvalidCards.Empty();

	for (const FName& CardNumber : MainDeckList)
	{
		const FGCGCardData* CardData = CardDB->GetCardData(CardNumber);
		if (!CardData)
		{
			OutInvalidCards.Add(CardNumber);
			continue;
		}

		// Check if card type is valid for main deck
		if (CardData->CardType != EGCGCardType::Unit &&
			CardData->CardType != EGCGCardType::Pilot &&
			CardData->CardType != EGCGCardType::Command &&
			CardData->CardType != EGCGCardType::Base)
		{
			OutInvalidCards.Add(CardNumber);
		}
	}

	return OutInvalidCards.Num() == 0;
}

bool UGCGDeckValidationSubsystem::ValidateDeckColors(const TArray<FName>& MainDeckList, TSet<EGCGCardColor>& OutColorsFound)
{
	// Rule 6-1-1-2: Deck must be constructed using 1 or 2 colors only
	// Rule 6-1-1-2-1: Colorless cards (Resources, Tokens) don't count toward color limit

	UGCGCardDatabase* CardDB = GetCardDatabase();
	if (!CardDB)
	{
		UE_LOG(LogTemp, Error, TEXT("UGCGDeckValidationSubsystem::ValidateDeckColors - Card database not available"));
		return false;
	}

	OutColorsFound.Empty();

	for (const FName& CardNumber : MainDeckList)
	{
		const FGCGCardData* CardData = CardDB->GetCardData(CardNumber);
		if (!CardData)
		{
			continue; // Unknown card (will be caught by type validation)
		}

		// Only count non-colorless cards
		if (CardData->Color != EGCGCardColor::Colorless)
		{
			OutColorsFound.Add(CardData->Color);
		}
	}

	// Deck must have at least 1 color and at most 2 colors
	int32 ColorCount = OutColorsFound.Num();
	return ColorCount >= 1 && ColorCount <= 2;
}

bool UGCGDeckValidationSubsystem::ValidateCopyLimits(const TArray<FName>& MainDeckList, TMap<FName, int32>& OutOverLimitCards)
{
	// Rule 6-1-1-3: Up to 4 copies of cards with the same card number

	OutOverLimitCards.Empty();

	TMap<FName, int32> CardCounts = CountCardCopies(MainDeckList);

	for (const TPair<FName, int32>& Pair : CardCounts)
	{
		if (Pair.Value > 4)
		{
			OutOverLimitCards.Add(Pair.Key, Pair.Value);
		}
	}

	return OutOverLimitCards.Num() == 0;
}

bool UGCGDeckValidationSubsystem::ValidateResourceDeckCardTypes(const TArray<FName>& ResourceDeckList, TArray<FName>& OutInvalidCards)
{
	// Rule 6-1-1-4: Resource deck must contain only Resource cards

	UGCGCardDatabase* CardDB = GetCardDatabase();
	if (!CardDB)
	{
		UE_LOG(LogTemp, Error, TEXT("UGCGDeckValidationSubsystem::ValidateResourceDeckCardTypes - Card database not available"));
		return false;
	}

	OutInvalidCards.Empty();

	for (const FName& CardNumber : ResourceDeckList)
	{
		const FGCGCardData* CardData = CardDB->GetCardData(CardNumber);
		if (!CardData)
		{
			OutInvalidCards.Add(CardNumber);
			continue;
		}

		// Only Resource cards allowed
		if (CardData->CardType != EGCGCardType::Resource)
		{
			OutInvalidCards.Add(CardNumber);
		}
	}

	return OutInvalidCards.Num() == 0;
}

// ===== HELPER FUNCTIONS =====

TMap<FName, int32> UGCGDeckValidationSubsystem::CountCardCopies(const TArray<FName>& DeckList) const
{
	TMap<FName, int32> CardCounts;

	for (const FName& CardNumber : DeckList)
	{
		CardCounts.FindOrAdd(CardNumber)++;
	}

	return CardCounts;
}

TSet<EGCGCardColor> UGCGDeckValidationSubsystem::GetDeckColors(const TArray<FName>& DeckList)
{
	TSet<EGCGCardColor> Colors;

	UGCGCardDatabase* CardDB = GetCardDatabase();
	if (!CardDB)
	{
		return Colors;
	}

	for (const FName& CardNumber : DeckList)
	{
		const FGCGCardData* CardData = CardDB->GetCardData(CardNumber);
		if (CardData && CardData->Color != EGCGCardColor::Colorless)
		{
			Colors.Add(CardData->Color);
		}
	}

	return Colors;
}

bool UGCGDeckValidationSubsystem::CardExists(FName CardNumber) const
{
	UGCGCardDatabase* CardDB = GetCardDatabase();
	if (!CardDB)
	{
		return false;
	}

	return CardDB->GetCardData(CardNumber) != nullptr;
}

// ===== INTERNAL HELPERS =====

UGCGCardDatabase* UGCGDeckValidationSubsystem::GetCardDatabase() const
{
	if (CachedCardDatabase)
	{
		return CachedCardDatabase;
	}

	// Try to get it again if cache failed
	return GetGameInstance()->GetSubsystem<UGCGCardDatabase>();
}
