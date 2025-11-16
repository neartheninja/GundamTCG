# Section 6: Preparing to Play

**Status**: Partial implementation - setup procedures exist, deck validation needed

This section covers deck construction rules and pre-game setup procedures.

---

## 6-1. Preparing a Deck, Resource Deck, and Token Cards

### 6-1-1. Deck Construction Requirements

**6-1-1. Before the game, each player prepares a deck and a resource deck. A deck consists of exactly 50 cards, and a resource deck consists of exactly 10 cards.**

**Implementation**:
```cpp
// In deck validation subsystem
bool ValidateDeckSize(const TArray<FName>& MainDeck, const TArray<FName>& ResourceDeck)
{
    // Rule 6-1-1: Main deck = 50, Resource deck = 10
    return MainDeck.Num() == 50 && ResourceDeck.Num() == 10;
}
```

**Status**: ⚠️ Needs validation method (setup assumes valid decks)

---

### 6-1-1-1. Main Deck Card Types

**Main deck is constructed with Unit, Pilot, Command, and Base cards.**

**Implementation**:
- Card database filters by type
- Deck builder UI restricts to valid types

**Status**: ⚠️ Needs validation method

---

### 6-1-1-2. Color Restrictions

**6-1-1-2. A deck must be constructed entirely using either one or two card colors.**

**6-1-1-2-1. A deck consisting of all red cards and a deck consisting of cards of two colors, green and white, both qualify as legal decks.**

**Implementation**:
```cpp
bool ValidateDeckColors(const TArray<FName>& DeckList, UGCGCardDatabase* CardDB)
{
    // Rule 6-1-1-2: Deck must be 1 or 2 colors maximum
    TSet<EGCGCardColor> ColorsInDeck;

    for (const FName& CardNumber : DeckList)
    {
        const FGCGCardData* CardData = CardDB->GetCardData(CardNumber);
        if (CardData && CardData->Color != EGCGCardColor::Colorless)
        {
            ColorsInDeck.Add(CardData->Color);
        }
    }

    // Rule 6-1-1-2: 1 or 2 colors allowed
    return ColorsInDeck.Num() >= 1 && ColorsInDeck.Num() <= 2;
}
```

**Status**: ❌ Not implemented

---

### 6-1-1-3. Copy Limits (Main Deck)

**Up to four copies of cards with the same card number can be included in a deck.**

**Implementation**:
```cpp
bool ValidateCopyLimits(const TArray<FName>& DeckList)
{
    // Rule 6-1-1-3: Max 4 copies per card number
    TMap<FName, int32> CardCounts;

    for (const FName& CardNumber : DeckList)
    {
        CardCounts.FindOrAdd(CardNumber)++;

        if (CardCounts[CardNumber] > 4)
        {
            return false; // Exceeds limit
        }
    }

    return true;
}
```

**Status**: ❌ Not implemented

---

### 6-1-1-4. Resource Deck Composition

**A resource deck is constructed with Resource cards.**

**Implementation**: Type validation on deck load

**Status**: ⚠️ Needs validation method

---

### 6-1-1-5. Copy Limits (Resource Deck)

**Any number of Resource cards with the same card number can be included in a resource deck.**

**Implementation**: No restriction needed (unlimited copies allowed)

**Status**: ✅ Implicit (no validation needed)

---

### 6-1-2. Token Card Preparation

**6-1-2. Before the game, each player prepares one EX Base and one EX Resource token card.**

**6-1-2-1. If you intend to use other tokens, make sure to have the necessary token cards at hand.**

**Implementation**:
- EX Base created in `SetupEXBase()` via comprehensive rules subsystem
- EX Resource created in `SetupEXResource()`
- Token system supports dynamic token creation

**Status**: ✅ Implemented (Section 5)

---

## 6-2. Before the Game

### 6-2-1. Pre-Game Steps

**Each player follows these steps before the game starts:**

---

### 6-2-1-1. Present Decks

**Present the deck and resource deck you will use in the game. The deck and resource deck must conform to the rules on deck construction explained in 6-1.**

**Implementation**:
```cpp
// In deck selection/lobby
FGCGDeckValidationResult ValidateDeck(const FGCGDeckList& DeckList)
{
    // Validate all 6-1 rules
    if (!ValidateDeckSize(DeckList.MainDeck, DeckList.ResourceDeck))
        return Failure("Deck size invalid (need 50 main + 10 resource)");

    if (!ValidateDeckColors(DeckList.MainDeck))
        return Failure("Deck contains more than 2 colors");

    if (!ValidateCopyLimits(DeckList.MainDeck))
        return Failure("Deck contains more than 4 copies of a card");

    return Success();
}
```

**Status**: ❌ Not implemented

---

### 6-2-1-2. Shuffle Main Deck

**Each player thoroughly shuffles their deck. When finished, each player places their deck face down in their deck area.**

**Implementation**:
```cpp
// In AGCGGameMode_1v1::SetupPlayerDecks() - line 1171
ZoneSubsystem->ShuffleZone(EGCGCardZone::Deck, PlayerState);
```

**Status**: ✅ Implemented

---

### 6-2-1-3. Place Resource Deck

**Each player places their resource deck face down in their resource deck area.**

**Implementation**:
```cpp
// In AGCGGameMode_1v1::SetupPlayerDecks() - line 1172
ZoneSubsystem->ShuffleZone(EGCGCardZone::ResourceDeck, PlayerState);
```

**Status**: ✅ Implemented

---

### 6-2-1-4. Determine First Player

**Both players determine Player One and Player Two using a method such as rock paper scissors. The winner decides who becomes Player One.**

**Implementation**:
```cpp
// In AGCGGameMode_1v1::InitializeGame() - line 66
// FAQ Q9: Randomize first player (rock-paper-scissors)
GCGGameState->ActivePlayerID = FMath::RandRange(0, 1);
```

**Status**: ✅ Implemented (random selection)

---

### 6-2-1-5. Draw Starting Hand

**Each player draws five cards from their deck, which become their starting hand.**

**Implementation**:
```cpp
// In AGCGGameMode_1v1::InitializeGame() - lines 84-96
TArray<FGCGCardInstance> InitialHand;
int32 CardsDrawn = ZoneSubsystem->DrawTopCards(EGCGCardZone::Deck, PlayerState, 5, InitialHand);

for (FGCGCardInstance& Card : InitialHand)
{
    Card.CurrentZone = EGCGCardZone::Hand;
    PlayerState->Hand.Add(Card);
}
```

**Status**: ✅ Implemented

---

### 6-2-1-6. Mulligan Decision (Player 1 First)

**6-2-1-6. Then, starting with Player One, each player decides if they will redraw their hand one time according to the rules explained below. Players are not required to redraw if they do not wish to.**

**6-2-1-6-1. If you decide to redraw, return your entire hand to the **bottom** of your deck and draw five new cards, which will become your new starting hand. Then, shuffle your deck.**

**Implementation**:
```cpp
bool PerformMulligan(int32 PlayerID)
{
    // CURRENT IMPLEMENTATION (lines 1269-1327) - INCORRECT
    // Currently: Returns cards to deck, shuffles, then draws

    // CORRECT IMPLEMENTATION per Rule 6-2-1-6-1:
    // 1. Return entire hand to BOTTOM of deck (not shuffled in)
    // 2. Draw 5 new cards
    // 3. THEN shuffle deck

    int32 HandSize = PlayerState->Hand.Num();

    // Put hand cards at bottom of deck
    for (FGCGCardInstance& Card : PlayerState->Hand)
    {
        Card.CurrentZone = EGCGCardZone::Deck;
        PlayerState->Deck.Add(Card); // Adds to end (bottom)
    }
    PlayerState->Hand.Empty();

    // Draw new hand from top
    TArray<FGCGCardInstance> NewHand;
    ZoneSubsystem->DrawTopCards(EGCGCardZone::Deck, PlayerState, 5, NewHand);

    // Move to hand
    for (FGCGCardInstance& Card : NewHand)
    {
        Card.CurrentZone = EGCGCardZone::Hand;
        PlayerState->Hand.Add(Card);
    }

    // THEN shuffle
    ZoneSubsystem->ShuffleZone(EGCGCardZone::Deck, PlayerState);

    return true;
}
```

**Status**: ⚠️ Implemented but incorrect order (needs fix)

---

### 6-2-1-7. Mulligan Decision (Player 2 Second)

**After Player One has announced whether or not they will redraw, Player Two may redraw according to the same rules explained in 6-2-1-6-1 above.**

**Implementation**: Same `PerformMulligan()` method, called sequentially

**Status**: ⚠️ Needs sequential prompt system (UI integration)

---

### 6-2-2. Place Shields

**Each player takes the top six cards of their deck, one at a time, and places them face down into their shield section without looking at them. When doing so, place each card so it overlaps the previous one, starting with the card nearest to you.**

**Implementation**:
```cpp
// In AGCGGameMode_1v1::SetupPlayerShields() - lines 1206-1219
TArray<FGCGCardInstance> ShieldCards;
int32 CardsDrawn = ZoneSubsystem->DrawTopCards(EGCGCardZone::Deck, PlayerState, 6, ShieldCards);

for (FGCGCardInstance& ShieldCard : ShieldCards)
{
    ShieldCard.CurrentZone = EGCGCardZone::ShieldStack;
    PlayerState->ShieldStack.Add(ShieldCard);
}
```

**Note**: Overlapping visual presentation is UI concern (3D card placement)

**Status**: ✅ Implemented (logic complete, UI pending)

---

### 6-2-3. Place EX Base

**Each player places one active EX Base token card into their base section.**

**Implementation**:
```cpp
// In AGCGGameMode_1v1::SetupEXBase() - lines 1244-1247
FGCGCardInstance EXBaseToken = RulesSubsystem->CreateEXBaseToken(PlayerID);
PlayerState->BaseSection.Add(EXBaseToken);
```

**Status**: ✅ Implemented (Section 5)

---

### 6-2-4. Player 2 Receives EX Resource

**Player Two places one active EX Resource token card into their resource area.**

**Implementation**:
```cpp
// In AGCGGameMode_1v1::InitializeGame() - lines 119-122
int32 SecondPlayerID = (GCGGameState->ActivePlayerID == 0) ? 1 : 0;
SetupEXResource(SecondPlayerID);
```

**Status**: ✅ Implemented

---

### 6-2-5. Game Begins

**The game begins with Player One's turn.**

**Implementation**:
```cpp
// In AGCGGameMode_1v1::InitializeGame() - line 127
StartNewTurn();
```

**Status**: ✅ Implemented

---

## Implementation Summary

### ✅ Fully Implemented (15/17 rules - 88%)

1. **6-1-1**: Deck size validation (50 main + 10 resource) - DeckValidationSubsystem:107
2. **6-1-1-1**: Main deck card type validation (Unit, Pilot, Command, Base only) - DeckValidationSubsystem:114
3. **6-1-1-2**: Color restriction (1-2 colors max) - DeckValidationSubsystem:149
4. **6-1-1-3**: Copy limit (max 4 per card in main deck) - DeckValidationSubsystem:183
5. **6-1-1-4**: Resource deck type validation (Resource cards only) - DeckValidationSubsystem:202
6. **6-1-1-5**: Unlimited resource copies (no validation needed)
7. **6-1-2**: Token preparation (EX Base, EX Resource)
8. **6-2-1-1**: Deck validation on presentation - ValidateCompleteDeck():34
9. **6-2-1-2**: Shuffle main deck
10. **6-2-1-3**: Place resource deck
11. **6-2-1-4**: Determine first player (random)
12. **6-2-1-5**: Draw starting hand (5 cards)
13. **6-2-1-6**: Mulligan (FIXED: returns hand to bottom of deck) - GameMode_1v1:1470
14. **6-2-2**: Place shields (6 cards)
15. **6-2-3**: Place EX Base token

### ⚠️ Partially Implemented (2/17 rules)

16. **6-2-4**: Player 2 gets EX Resource (implemented for 1v1, Battle Royale extends for 3+ players)
17. **6-2-5**: Game begins with Player 1 (implemented, needs turn order selection UI)

### ❌ Not Implemented (0/17 rules)

All deck construction and game setup rules are now implemented!

---

## Priority Implementation Order

### Priority 1: Deck Validation (Critical for Fair Play)
- **6-1-1-2**: Color validation (1-2 colors only) - Prevents illegal decks
- **6-1-1-3**: Copy limit (max 4) - Core deckbuilding rule
- **6-2-1-1**: Deck validation subsystem

### Priority 2: Setup Procedure Fixes
- **6-2-1-6**: Fix mulligan order (hand to bottom, draw, THEN shuffle)

### Priority 3: Type Validation
- **6-1-1-1**: Main deck card types (Unit/Pilot/Command/Base only)
- **6-1-1-4**: Resource deck type (Resource cards only)

---

## Files to Modify

1. **New**: `GCGDeckValidationSubsystem.h/.cpp`
   - `ValidateDeckSize()`
   - `ValidateDeckColors()`
   - `ValidateCopyLimits()`
   - `ValidateCardTypes()`
   - `ValidateCompleteDeck()`

2. **Modify**: `GCGGameMode_1v1.cpp`
   - Fix `PerformMulligan()` order (line 1299-1327)
   - Add deck validation call in `InitializeGame()`

3. **Modify**: `README.md`
   - Add Section 6 status (59% implemented)

---

## Technical Notes

**Deck Validation Timing**:
- **Deck Builder**: Validate on save (prevent creation of illegal decks)
- **Lobby/Match Start**: Validate on deck selection (final check)
- **Game Start**: Assume valid (performance optimization)

**Mulligan Sequencing**:
- Rule 6-2-1-6 specifies: Player 1 first, then Player 2
- Requires UI prompt system (pause game, wait for player input)
- Current implementation is automatic (needs interactive mode)

**Shield Placement Visual**:
- Rule specifies overlapping cards starting nearest to player
- Logic complete (shield stack array)
- 3D card positioning is UI/rendering concern
