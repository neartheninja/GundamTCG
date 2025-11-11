// TCGPlayerState.cpp - Example Implementation
// Copy this to your Unreal project's Source folder

#include "TCGPlayerState.h"
#include "Net/UnrealNetwork.h"
#include "Engine/Engine.h"

ATCGPlayerState::ATCGPlayerState()
{
    // Enable replication
    bReplicates = true;
    bAlwaysRelevant = true;
    NetUpdateFrequency = 10.0f;

    // Initialize defaults
    TCGPlayerID = 0;
    AvailableDon = 0;
    bHasDrawnThisTurn = false;
}

void ATCGPlayerState::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
    Super::GetLifetimeReplicatedProps(OutLifetimeProps);

    // Replicate all zone arrays
    DOREPLIFETIME(ATCGPlayerState, Hand);
    DOREPLIFETIME(ATCGPlayerState, Deck);
    DOREPLIFETIME(ATCGPlayerState, Life);
    DOREPLIFETIME(ATCGPlayerState, LeaderCard);
    DOREPLIFETIME(ATCGPlayerState, DonDeck);
    DOREPLIFETIME(ATCGPlayerState, DonZone);
    DOREPLIFETIME(ATCGPlayerState, CharacterZone);
    DOREPLIFETIME(ATCGPlayerState, StageZone);
    DOREPLIFETIME(ATCGPlayerState, Trash);

    // Replicate stats
    DOREPLIFETIME(ATCGPlayerState, TCGPlayerID);
    DOREPLIFETIME(ATCGPlayerState, AvailableDon);
    DOREPLIFETIME(ATCGPlayerState, bHasDrawnThisTurn);
}

// ===== DECK OPERATIONS =====

bool ATCGPlayerState::DrawCard()
{
    if (Deck.Num() == 0)
    {
        UE_LOG(LogTemp, Warning, TEXT("Player %d: Cannot draw - deck is empty!"), TCGPlayerID);
        return false;
    }

    // Take top card from deck
    FCardData DrawnCard = Deck[0];
    Deck.RemoveAt(0);

    // Add to hand
    DrawnCard.CurrentZone = ECardZone::HAND;
    Hand.Add(DrawnCard);

    UE_LOG(LogTemp, Log, TEXT("Player %d drew: %s (Hand size: %d, Deck: %d)"),
        TCGPlayerID, *DrawnCard.CardName, Hand.Num(), Deck.Num());

    // Trigger UI update
    if (GetLocalRole() == ROLE_Authority)
    {
        OnRep_Hand(); // Call manually on server
        OnRep_Deck();
    }

    return true;
}

void ATCGPlayerState::DrawCards(int32 Count)
{
    for (int32 i = 0; i < Count; i++)
    {
        if (!DrawCard())
        {
            break; // Stop if deck is empty
        }
    }
}

void ATCGPlayerState::ShuffleDeck()
{
    if (Deck.Num() <= 1)
    {
        return;
    }

    UE_LOG(LogTemp, Log, TEXT("Player %d: Shuffling deck (%d cards)"), TCGPlayerID, Deck.Num());

    // Fisher-Yates shuffle
    for (int32 i = Deck.Num() - 1; i > 0; i--)
    {
        int32 j = FMath::RandRange(0, i);
        Deck.Swap(i, j);
    }

    if (GetLocalRole() == ROLE_Authority)
    {
        OnRep_Deck();
    }
}

// ===== DON OPERATIONS =====

bool ATCGPlayerState::AddDonToZone()
{
    if (DonDeck.Num() == 0)
    {
        UE_LOG(LogTemp, Warning, TEXT("Player %d: DON deck is empty!"), TCGPlayerID);
        return false;
    }

    // Move DON from DON deck to DON zone
    FCardData DonCard = DonDeck[0];
    DonDeck.RemoveAt(0);

    DonCard.CurrentZone = ECardZone::DON_ZONE;
    DonCard.bIsRested = false; // DON enters active
    DonZone.Add(DonCard);

    AvailableDon++;

    UE_LOG(LogTemp, Log, TEXT("Player %d: Added DON (Total: %d)"), TCGPlayerID, DonZone.Num());

    if (GetLocalRole() == ROLE_Authority)
    {
        OnRep_DonZone();
        OnRep_DonDeck();
    }

    return true;
}

bool ATCGPlayerState::AttachDonToCharacter(int32 CharacterInstanceID)
{
    // Find character
    FCardData* Character = nullptr;
    for (FCardData& Card : CharacterZone)
    {
        if (Card.InstanceID == CharacterInstanceID)
        {
            Character = &Card;
            break;
        }
    }

    if (!Character)
    {
        UE_LOG(LogTemp, Warning, TEXT("Character not found!"));
        return false;
    }

    // Find an active DON
    FCardData* ActiveDon = nullptr;
    for (FCardData& Don : DonZone)
    {
        if (!Don.bIsRested)
        {
            ActiveDon = &Don;
            break;
        }
    }

    if (!ActiveDon)
    {
        UE_LOG(LogTemp, Warning, TEXT("No active DON available!"));
        return false;
    }

    // Attach DON
    ActiveDon->bIsRested = true; // Use the DON
    Character->AttachedDonCount++;
    Character->Power += 1000; // +1000 power per DON in One Piece TCG

    UE_LOG(LogTemp, Log, TEXT("Attached DON to %s (Power: %d)"),
        *Character->CardName, Character->Power);

    if (GetLocalRole() == ROLE_Authority)
    {
        OnRep_DonZone();
        OnRep_CharacterZone();
    }

    return true;
}

bool ATCGPlayerState::DetachDonFromCharacter(int32 CharacterInstanceID)
{
    // Find character
    FCardData* Character = nullptr;
    for (FCardData& Card : CharacterZone)
    {
        if (Card.InstanceID == CharacterInstanceID)
        {
            Character = &Card;
            break;
        }
    }

    if (!Character || Character->AttachedDonCount == 0)
    {
        return false;
    }

    // Remove DON
    Character->AttachedDonCount--;
    Character->Power -= 1000;

    // Find a rested DON to "free"
    for (FCardData& Don : DonZone)
    {
        if (Don.bIsRested)
        {
            Don.bIsRested = false;
            break;
        }
    }

    if (GetLocalRole() == ROLE_Authority)
    {
        OnRep_DonZone();
        OnRep_CharacterZone();
    }

    return true;
}

void ATCGPlayerState::RefreshAllDon()
{
    for (FCardData& Don : DonZone)
    {
        Don.bIsRested = false;
    }

    UE_LOG(LogTemp, Log, TEXT("Player %d: Refreshed all DON"), TCGPlayerID);

    if (GetLocalRole() == ROLE_Authority)
    {
        OnRep_DonZone();
    }
}

// ===== CHARACTER OPERATIONS =====

bool ATCGPlayerState::PlayCharacter(int32 HandIndex)
{
    if (HandIndex < 0 || HandIndex >= Hand.Num())
    {
        UE_LOG(LogTemp, Warning, TEXT("Invalid hand index: %d"), HandIndex);
        return false;
    }

    FCardData& Card = Hand[HandIndex];

    // Check if it's a character
    if (Card.CardType != ECardType::CHARACTER)
    {
        UE_LOG(LogTemp, Warning, TEXT("Card is not a character!"));
        return false;
    }

    // Check cost
    int32 ActiveDonCount = 0;
    for (const FCardData& Don : DonZone)
    {
        if (!Don.bIsRested)
        {
            ActiveDonCount++;
        }
    }

    if (ActiveDonCount < Card.Cost)
    {
        UE_LOG(LogTemp, Warning, TEXT("Not enough DON! Need %d, have %d"), Card.Cost, ActiveDonCount);
        return false;
    }

    // Pay cost (rest DON)
    int32 DonToRest = Card.Cost;
    for (FCardData& Don : DonZone)
    {
        if (DonToRest > 0 && !Don.bIsRested)
        {
            Don.bIsRested = true;
            DonToRest--;
        }
    }

    // Move card to character zone
    Card.CurrentZone = ECardZone::CHARACTER_ZONE;
    Card.bIsRested = false; // Enters active (no summoning sickness in OP TCG)

    CharacterZone.Add(Card);
    Hand.RemoveAt(HandIndex);

    UE_LOG(LogTemp, Log, TEXT("Player %d played character: %s"), TCGPlayerID, *Card.CardName);

    if (GetLocalRole() == ROLE_Authority)
    {
        OnRep_Hand();
        OnRep_CharacterZone();
        OnRep_DonZone();
    }

    return true;
}

bool ATCGPlayerState::RestCharacter(int32 CharacterInstanceID)
{
    for (FCardData& Card : CharacterZone)
    {
        if (Card.InstanceID == CharacterInstanceID)
        {
            if (Card.bIsRested)
            {
                return false; // Already rested
            }

            Card.bIsRested = true;

            if (GetLocalRole() == ROLE_Authority)
            {
                OnRep_CharacterZone();
            }

            return true;
        }
    }

    return false;
}

bool ATCGPlayerState::RefreshCharacter(int32 CharacterInstanceID)
{
    for (FCardData& Card : CharacterZone)
    {
        if (Card.InstanceID == CharacterInstanceID)
        {
            Card.bIsRested = false;

            if (GetLocalRole() == ROLE_Authority)
            {
                OnRep_CharacterZone();
            }

            return true;
        }
    }

    return false;
}

void ATCGPlayerState::RefreshAllCharacters()
{
    for (FCardData& Card : CharacterZone)
    {
        Card.bIsRested = false;
    }

    UE_LOG(LogTemp, Log, TEXT("Player %d: Refreshed all characters"), TCGPlayerID);

    if (GetLocalRole() == ROLE_Authority)
    {
        OnRep_CharacterZone();
    }
}

void ATCGPlayerState::KOCharacter(int32 CharacterInstanceID)
{
    for (int32 i = 0; i < CharacterZone.Num(); i++)
    {
        if (CharacterZone[i].InstanceID == CharacterInstanceID)
        {
            FCardData KOedCard = CharacterZone[i];
            KOedCard.CurrentZone = ECardZone::TRASH;
            KOedCard.bIsRested = false;
            KOedCard.AttachedDonCount = 0;

            Trash.Add(KOedCard);
            CharacterZone.RemoveAt(i);

            UE_LOG(LogTemp, Warning, TEXT("Player %d: %s was K.O.'d!"), TCGPlayerID, *KOedCard.CardName);

            if (GetLocalRole() == ROLE_Authority)
            {
                OnRep_CharacterZone();
                OnRep_Trash();
            }

            return;
        }
    }
}

// ===== LIFE OPERATIONS =====

void ATCGPlayerState::ApplyCardDamage(int32 DamageAmount)
{
    UE_LOG(LogTemp, Warning, TEXT("Player %d takes %d damage!"), TCGPlayerID, DamageAmount);

    for (int32 i = 0; i < DamageAmount; i++)
    {
        if (Life.Num() == 0)
        {
            UE_LOG(LogTemp, Error, TEXT("Player %d has LOST!"), TCGPlayerID);
            break;
        }

        // Move top life card to hand
        FCardData LifeCard = Life[0];
        Life.RemoveAt(0);

        LifeCard.CurrentZone = ECardZone::HAND;
        Hand.Add(LifeCard);

        UE_LOG(LogTemp, Log, TEXT("Life card moved to hand: %s"), *LifeCard.CardName);
    }

    if (GetLocalRole() == ROLE_Authority)
    {
        OnRep_Life();
        OnRep_Hand();
    }
}

bool ATCGPlayerState::HasLost() const
{
    return Life.Num() == 0;
}

// ===== HELPER FUNCTIONS =====

FCardData ATCGPlayerState::FindCardByInstanceID(int32 InstanceID, ECardZone Zone)
{
    TArray<FCardData>* TargetZone = nullptr;

    switch (Zone)
    {
        case ECardZone::HAND:
            TargetZone = &Hand;
            break;
        case ECardZone::DECK:
            TargetZone = &Deck;
            break;
        case ECardZone::CHARACTER_ZONE:
            TargetZone = &CharacterZone;
            break;
        case ECardZone::DON_ZONE:
            TargetZone = &DonZone;
            break;
        case ECardZone::LIFE_ZONE:
            TargetZone = &Life;
            break;
        case ECardZone::TRASH:
            TargetZone = &Trash;
            break;
        default:
            return FCardData();
    }

    for (const FCardData& Card : *TargetZone)
    {
        if (Card.InstanceID == InstanceID)
        {
            return Card;
        }
    }

    return FCardData();
}

int32 ATCGPlayerState::GetCharacterTotalPower(int32 CharacterInstanceID) const
{
    for (const FCardData& Card : CharacterZone)
    {
        if (Card.InstanceID == CharacterInstanceID)
        {
            // Base power + DON bonuses
            return Card.Power + (Card.AttachedDonCount * 1000);
        }
    }

    return 0;
}

// ===== REPLICATION CALLBACKS =====

void ATCGPlayerState::OnRep_Hand()
{
    UE_LOG(LogTemp, Verbose, TEXT("Player %d: Hand updated (%d cards)"), TCGPlayerID, Hand.Num());
    OnHandUpdated(); // Trigger Blueprint event
}

void ATCGPlayerState::OnRep_Deck()
{
    UE_LOG(LogTemp, Verbose, TEXT("Player %d: Deck updated (%d cards)"), TCGPlayerID, Deck.Num());
}

void ATCGPlayerState::OnRep_Life()
{
    UE_LOG(LogTemp, Verbose, TEXT("Player %d: Life updated (%d cards)"), TCGPlayerID, Life.Num());
    OnLifeUpdated(); // Trigger Blueprint event
}

void ATCGPlayerState::OnRep_Leader()
{
    UE_LOG(LogTemp, Verbose, TEXT("Player %d: Leader updated"), TCGPlayerID);
}

void ATCGPlayerState::OnRep_DonDeck()
{
    UE_LOG(LogTemp, Verbose, TEXT("Player %d: DON Deck updated (%d cards)"), TCGPlayerID, DonDeck.Num());
}

void ATCGPlayerState::OnRep_DonZone()
{
    UE_LOG(LogTemp, Verbose, TEXT("Player %d: DON Zone updated (%d cards)"), TCGPlayerID, DonZone.Num());
    OnDonZoneUpdated(); // Trigger Blueprint event
}

void ATCGPlayerState::OnRep_CharacterZone()
{
    UE_LOG(LogTemp, Verbose, TEXT("Player %d: Character Zone updated (%d cards)"), TCGPlayerID, CharacterZone.Num());
    OnCharacterZoneUpdated(); // Trigger Blueprint event
}

void ATCGPlayerState::OnRep_StageZone()
{
    UE_LOG(LogTemp, Verbose, TEXT("Player %d: Stage Zone updated (%d cards)"), TCGPlayerID, StageZone.Num());
}

void ATCGPlayerState::OnRep_Trash()
{
    UE_LOG(LogTemp, Verbose, TEXT("Player %d: Trash updated (%d cards)"), TCGPlayerID, Trash.Num());
}
