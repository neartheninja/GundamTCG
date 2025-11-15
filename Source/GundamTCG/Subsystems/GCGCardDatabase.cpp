// GCGCardDatabase.cpp - Card Database Subsystem Implementation
// Unreal Engine 5.6 - Gundam TCG Implementation

#include "GCGCardDatabase.h"
#include "Engine/DataTable.h"

// ===== SUBSYSTEM LIFECYCLE =====

void UGCGCardDatabase::Initialize(FSubsystemCollectionBase& Collection)
{
	Super::Initialize(Collection);

	UE_LOG(LogTemp, Log, TEXT("UGCGCardDatabase::Initialize - Card Database Subsystem initialized"));

	// Initialize token definitions
	InitializeTokenDefinitions();

	// Load card data if DataTable is set
	if (CardDataTable)
	{
		ReloadCardData();
	}
	else
	{
		UE_LOG(LogTemp, Warning, TEXT("UGCGCardDatabase::Initialize - No CardDataTable set, card lookups will only return tokens"));
	}
}

void UGCGCardDatabase::Deinitialize()
{
	UE_LOG(LogTemp, Log, TEXT("UGCGCardDatabase::Deinitialize - Card Database Subsystem shutdown"));

	// Clear cache
	CardDataCache.Empty();
	TokenDefinitions.Empty();

	Super::Deinitialize();
}

// ===== CARD DATA LOOKUP =====

const FGCGCardData* UGCGCardDatabase::GetCardData(FName CardNumber) const
{
	// Check if this is a token first
	if (TokenDefinitions.Contains(CardNumber))
	{
		return &TokenDefinitions[CardNumber];
	}

	// Check cache
	if (CardDataCache.Contains(CardNumber))
	{
		return CardDataCache[CardNumber];
	}

	// If we have a DataTable, try looking it up directly
	if (CardDataTable)
	{
		FGCGCardData* FoundData = CardDataTable->FindRow<FGCGCardData>(CardNumber, TEXT("GetCardData"));
		if (FoundData)
		{
			return FoundData;
		}
	}

	// Card not found
	UE_LOG(LogTemp, Warning, TEXT("UGCGCardDatabase::GetCardData - Card not found: %s"), *CardNumber.ToString());
	return nullptr;
}

bool UGCGCardDatabase::CardExists(FName CardNumber) const
{
	return GetCardData(CardNumber) != nullptr;
}

TArray<FGCGCardData> UGCGCardDatabase::GetAllCards() const
{
	TArray<FGCGCardData> AllCards;

	if (CardDataTable)
	{
		TArray<FGCGCardData*> AllRows;
		CardDataTable->GetAllRows<FGCGCardData>(TEXT("GetAllCards"), AllRows);

		for (FGCGCardData* Row : AllRows)
		{
			if (Row)
			{
				AllCards.Add(*Row);
			}
		}
	}

	return AllCards;
}

TArray<FGCGCardData> UGCGCardDatabase::GetCardsByType(EGCGCardType CardType) const
{
	TArray<FGCGCardData> FilteredCards;
	TArray<FGCGCardData> AllCards = GetAllCards();

	for (const FGCGCardData& Card : AllCards)
	{
		if (Card.CardType == CardType)
		{
			FilteredCards.Add(Card);
		}
	}

	return FilteredCards;
}

TArray<FGCGCardData> UGCGCardDatabase::GetCardsByColor(EGCGCardColor Color) const
{
	TArray<FGCGCardData> FilteredCards;
	TArray<FGCGCardData> AllCards = GetAllCards();

	for (const FGCGCardData& Card : AllCards)
	{
		if (Card.Colors.Contains(Color))
		{
			FilteredCards.Add(Card);
		}
	}

	return FilteredCards;
}

// ===== TOKEN DEFINITIONS =====

FGCGCardData UGCGCardDatabase::GetTokenData(FName TokenType) const
{
	if (TokenDefinitions.Contains(TokenType))
	{
		return TokenDefinitions[TokenType];
	}

	UE_LOG(LogTemp, Warning, TEXT("UGCGCardDatabase::GetTokenData - Token not found: %s"), *TokenType.ToString());
	return FGCGCardData();
}

bool UGCGCardDatabase::IsToken(FName CardNumber) const
{
	return TokenDefinitions.Contains(CardNumber);
}

// ===== CARD VALIDATION =====

bool UGCGCardDatabase::ValidateDeck(const TArray<FName>& DeckList, TArray<FString>& OutErrors) const
{
	OutErrors.Empty();
	bool bIsValid = true;

	// Check deck size
	if (DeckList.Num() != 50)
	{
		OutErrors.Add(FString::Printf(TEXT("Deck must contain exactly 50 cards (found %d)"), DeckList.Num()));
		bIsValid = false;
	}

	// Check for invalid cards
	for (const FName& CardNumber : DeckList)
	{
		if (!CardExists(CardNumber))
		{
			OutErrors.Add(FString::Printf(TEXT("Card not found in database: %s"), *CardNumber.ToString()));
			bIsValid = false;
		}
	}

	// Check card limits (max 4 copies of any card, except Base which is max 1)
	TMap<FName, int32> CardCounts;
	for (const FName& CardNumber : DeckList)
	{
		CardCounts.FindOrAdd(CardNumber)++;
	}

	for (const auto& CardCount : CardCounts)
	{
		const FGCGCardData* CardData = GetCardData(CardCount.Key);
		if (CardData)
		{
			// Base cards: max 1
			if (CardData->CardType == EGCGCardType::Base && CardCount.Value > 1)
			{
				OutErrors.Add(FString::Printf(TEXT("Base cards limited to 1 copy: %s (found %d)"),
					*CardData->CardName.ToString(), CardCount.Value));
				bIsValid = false;
			}
			// All other cards: max 4
			else if (CardData->CardType != EGCGCardType::Base && CardCount.Value > 4)
			{
				OutErrors.Add(FString::Printf(TEXT("Cards limited to 4 copies: %s (found %d)"),
					*CardData->CardName.ToString(), CardCount.Value));
				bIsValid = false;
			}
		}
	}

	return bIsValid;
}

bool UGCGCardDatabase::ValidateResourceDeck(const TArray<FName>& ResourceDeckList, TArray<FString>& OutErrors) const
{
	OutErrors.Empty();
	bool bIsValid = true;

	// Check resource deck size
	if (ResourceDeckList.Num() != 10)
	{
		OutErrors.Add(FString::Printf(TEXT("Resource Deck must contain exactly 10 cards (found %d)"), ResourceDeckList.Num()));
		bIsValid = false;
	}

	// Check for invalid cards
	for (const FName& CardNumber : ResourceDeckList)
	{
		if (!CardExists(CardNumber))
		{
			OutErrors.Add(FString::Printf(TEXT("Card not found in database: %s"), *CardNumber.ToString()));
			bIsValid = false;
		}
	}

	// Note: Resource deck can contain any cards (no copy limits apply)

	return bIsValid;
}

// ===== DATATABLE MANAGEMENT =====

void UGCGCardDatabase::SetCardDataTable(UDataTable* NewDataTable)
{
	CardDataTable = NewDataTable;

	if (CardDataTable)
	{
		UE_LOG(LogTemp, Log, TEXT("UGCGCardDatabase::SetCardDataTable - Card data table set"));
		ReloadCardData();
	}
}

void UGCGCardDatabase::ReloadCardData()
{
	CardDataCache.Empty();

	if (!CardDataTable)
	{
		UE_LOG(LogTemp, Warning, TEXT("UGCGCardDatabase::ReloadCardData - No DataTable set"));
		return;
	}

	// Get all rows from the DataTable
	TArray<FGCGCardData*> AllRows;
	CardDataTable->GetAllRows<FGCGCardData>(TEXT("ReloadCardData"), AllRows);

	// Cache them for faster lookups
	for (FGCGCardData* Row : AllRows)
	{
		if (Row)
		{
			CardDataCache.Add(Row->CardNumber, Row);
		}
	}

	UE_LOG(LogTemp, Log, TEXT("UGCGCardDatabase::ReloadCardData - Loaded %d cards from DataTable"), CardDataCache.Num());
}

// ===== STATISTICS =====

int32 UGCGCardDatabase::GetCardCount() const
{
	return CardDataCache.Num();
}

FString UGCGCardDatabase::GetDatabaseStats() const
{
	int32 TotalCards = GetCardCount();
	int32 UnitCount = GetCardsByType(EGCGCardType::Unit).Num();
	int32 CommandCount = GetCardsByType(EGCGCardType::Command).Num();
	int32 BaseCount = GetCardsByType(EGCGCardType::Base).Num();

	return FString::Printf(TEXT("Card Database: %d total cards (%d Units, %d Commands, %d Bases, %d Tokens)"),
		TotalCards, UnitCount, CommandCount, BaseCount, TokenDefinitions.Num());
}

// ===== INTERNAL HELPERS =====

void UGCGCardDatabase::InitializeTokenDefinitions()
{
	UE_LOG(LogTemp, Log, TEXT("UGCGCardDatabase::InitializeTokenDefinitions - Initializing token definitions"));

	// EX Base token
	FGCGCardData EXBase = CreateEXBaseTokenData();
	TokenDefinitions.Add(FName("EXBase"), EXBase);

	// EX Resource token
	FGCGCardData EXResource = CreateEXResourceTokenData();
	TokenDefinitions.Add(FName("EXResource"), EXResource);

	UE_LOG(LogTemp, Log, TEXT("UGCGCardDatabase::InitializeTokenDefinitions - Initialized %d token definitions"),
		TokenDefinitions.Num());
}

FGCGCardData UGCGCardDatabase::CreateEXBaseTokenData() const
{
	FGCGCardData EXBase;

	// Basic info
	EXBase.CardNumber = FName("EXBase");
	EXBase.CardName = FText::FromString(TEXT("EX Base"));
	EXBase.CardType = EGCGCardType::Base;

	// Colors: Colorless
	EXBase.Colors.Empty();

	// Level and Cost
	EXBase.Level = 0;
	EXBase.Cost = 0;

	// Stats
	EXBase.AP = 0;
	EXBase.HP = 3;

	// No keywords
	EXBase.Keywords.Empty();

	// No effects (EX Base has no text)
	EXBase.Effects.Empty();

	// Flavor
	EXBase.CardText = FText::FromString(TEXT("An emergency base used when no Base card is available."));

	// Token flag
	EXBase.bIsToken = true;

	// Rarity
	EXBase.Rarity = EGCGCardRarity::Token;

	return EXBase;
}

FGCGCardData UGCGCardDatabase::CreateEXResourceTokenData() const
{
	FGCGCardData EXResource;

	// Basic info
	EXResource.CardNumber = FName("EXResource");
	EXResource.CardName = FText::FromString(TEXT("EX Resource"));
	EXResource.CardType = EGCGCardType::Unit; // Resources are just cards, type doesn't matter when face-down

	// Colors: Colorless
	EXResource.Colors.Empty();

	// Level and Cost
	EXResource.Level = 0;
	EXResource.Cost = 0;

	// Stats (not relevant for resources)
	EXResource.AP = 0;
	EXResource.HP = 0;

	// No keywords
	EXResource.Keywords.Empty();

	// No effects
	EXResource.Effects.Empty();

	// Flavor
	EXResource.CardText = FText::FromString(TEXT("An extra resource given to the player going second for balancing."));

	// Token flag
	EXResource.bIsToken = true;

	// Rarity
	EXResource.Rarity = EGCGCardRarity::Token;

	return EXResource;
}
