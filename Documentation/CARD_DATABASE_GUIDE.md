# Card Database Guide
**Gundam TCG - Unreal Engine 5.6**

This guide explains how to create and manage the card database for the Gundam Trading Card Game.

---

## Overview

The card database uses Unreal Engine's **DataTable** system with the `FGCGCardData` row structure defined in `GCGTypes.h`.

Cards are managed by the `UGCGCardDatabase` subsystem, which provides:
- Card lookup by card number
- Token definitions (EX Base, EX Resource)
- Deck validation
- Card filtering by type/color

---

## Creating the Card Database

### Step 1: Create CSV File

Create a CSV file with your card data. The first row should contain column headers matching the `FGCGCardData` structure.

**Example: GundamTCG_Cards.csv**

```csv
CardNumber,CardName,CardText,CardType,Colors,Level,Cost,AP,HP,Keywords,Rarity,SetNumber,CollectorNumber
GU-001,"RX-78-2 Gundam","[Activate・Main] (1) Rest this card: Draw 1 card.",Unit,"Red",3,3,6,7,"Repair(2)",Rare,SET1,001
GU-002,"Zaku II","[Continuous] While you have 3 or more rested Units, this card gains +2 AP.",Unit,"Green",2,2,4,5,"",Common,SET1,002
GU-003,"Beam Saber","[Activate・Main] Choose 1 of your Units. It gains +3 AP until end of turn.",Command,"Red",0,1,0,0,"",Uncommon,SET1,003
GU-004,"White Base","[Continuous] Your Units with 'Gundam' in their name gain +1 HP.",Base,"Red",0,0,0,10,"",Rare,SET1,004
GU-005,"MS-06S Zaku II (Char)","[On Deploy] You may search your deck for a red Command card, reveal it, and add it to your hand.",Unit,"Red",4,4,7,6,"FirstStrike",SuperRare,SET1,005
GU-006,"GM","",Unit,"Red",2,2,3,4,"",Common,SET1,006
GU-007,"Gouf","[On Attack] If defending player has no active Units, deal 1 damage to their Base.",Unit,"Green",3,3,5,5,"",Uncommon,SET1,007
GU-008,"Emergency Repair","[Activate・Main] Choose 1 of your Units with damage. Remove all damage from it.",Command,"Colorless",0,2,0,0,"Burst",Common,SET1,008
GU-009,"Gundam Hammer","[Activate・Main] Choose 1 enemy Unit. It cannot activate abilities until end of turn.",Command,"Red",0,2,0,0,"",Uncommon,SET1,009
GU-010,"Federation HQ","[Continuous] Once per turn, when you play a red Unit, you may draw 1 card.",Base,"Red",0,0,0,12,"",Rare,SET1,010
```

### Step 2: Import into Unreal Engine

1. **Open Unreal Editor**
2. **Content Browser → Right Click → Miscellaneous → Data Table**
3. **Select Row Structure**: `GCGCardData`
4. **Name the DataTable**: `DT_AllCards` (or similar)
5. **Right-click the DataTable → Reimport with CSV**: Select your CSV file

### Step 3: Assign DataTable to Game Mode

1. **Open your GameMode Blueprint** (or C++ class)
2. **Find the "Card Database" property**
3. **Assign your DataTable**: `DT_AllCards`

The DataTable will automatically be passed to the `UGCGCardDatabase` subsystem on game start.

---

## CSV Format Reference

### Required Columns

| Column | Type | Description | Example |
|--------|------|-------------|---------|
| `CardNumber` | FName | Unique card identifier | `GU-001` |
| `CardName` | FText | Display name of the card | `RX-78-2 Gundam` |
| `CardText` | FText | Full card text with abilities | `[Activate・Main] (1)...` |
| `CardType` | Enum | Unit, Command, or Base | `Unit` |
| `Colors` | Array | Card colors (Red, Blue, Green, Yellow, Colorless) | `Red` or `Red,Blue` |
| `Level` | int32 | Card level (0-10) | `3` |
| `Cost` | int32 | Resource cost to play | `3` |
| `AP` | int32 | Attack Power (Units only) | `6` |
| `HP` | int32 | Hit Points (Units/Bases only) | `7` |
| `Keywords` | String | Keyword abilities | `Repair(2),Blocker` |
| `Rarity` | Enum | Common, Uncommon, Rare, SuperRare, SecretRare, Token | `Rare` |
| `SetNumber` | FText | Set identifier | `SET1` |
| `CollectorNumber` | FText | Card number within set | `001` |

### Card Types

- **Unit**: Combatant cards (have AP/HP)
- **Command**: Instant effects (no stats)
- **Base**: Base card (only HP, max 1 per deck)

### Colors

Valid colors: `Red`, `Blue`, `Green`, `Yellow`, `Colorless`

Multiple colors: Separate with commas: `Red,Blue`

### Keywords

Valid keywords (see GCGTypes.h for full list):
- `Repair(X)` - Repair X damage at end of turn
- `Breach(X)` - Break X shields when dealing damage
- `Support(X)` - Give X AP to supported Unit
- `Blocker` - Can block attacks
- `FirstStrike` - Deals damage before opponent
- `HighManeuver` - Cannot be blocked
- `Suppression` - Defending Unit cannot activate
- `Burst` - Can activate from shields
- `LinkUnit` - Pair with another Unit

Multiple keywords: `Repair(2),Blocker,FirstStrike`

Keyword with value: `Repair(2)` or `Support(1)`

---

## Token Definitions

Tokens (EX Base, EX Resource) are **hard-coded** in the `UGCGCardDatabase` subsystem and do not need to be in the CSV:

### EX Base Token
- **Type**: Base
- **AP**: 0
- **HP**: 3
- **Text**: Emergency base used when no Base card is available
- **Used**: Automatically provided to all players at game start

### EX Resource Token
- **Type**: Resource (any card type works)
- **AP**: 0
- **HP**: 0
- **Text**: Extra resource for player going second
- **Used**: Given to Player 2 for balancing

---

## Card Text Formatting

Use square brackets for ability types:

### Ability Types
- `[Continuous]` - Always active
- `[On Deploy]` - Triggers when played
- `[On Attack]` - Triggers when declaring attack
- `[On Block]` - Triggers when blocking
- `[On Damage]` - Triggers when taking damage
- `[On Destroy]` - Triggers when destroyed
- `[Activate・Main]` - Can activate during Main Phase
- `[Activate・Action]` - Can activate during Action timing

### Costs
- `(X)` - Pay X resources
- `Rest this card` - Exhaust this card
- `Discard 1 card` - Discard from hand

### Examples
```
[Activate・Main] (1) Rest this card: Draw 1 card.
[Continuous] Your Units with 'Gundam' in their name gain +1 HP.
[On Deploy] Search your deck for a red Command card and add it to your hand.
[On Attack] If this Unit is alone, it gains +2 AP until end of turn.
```

---

## Deck Validation Rules

The `UGCGCardDatabase` subsystem validates decks:

### Main Deck (50 cards)
- Must contain exactly 50 cards
- Max 4 copies of any card (except Base)
- Max 1 Base card
- All cards must exist in database

### Resource Deck (10 cards)
- Must contain exactly 10 cards
- No copy limits
- Any cards can be resources

### Validation Example

```cpp
// C++ Code
UGCGCardDatabase* CardDB = GetGameInstance()->GetSubsystem<UGCGCardDatabase>();

TArray<FName> MainDeck = {...}; // 50 card numbers
TArray<FString> Errors;

if (CardDB->ValidateDeck(MainDeck, Errors))
{
    // Deck is valid
}
else
{
    // Print errors
    for (const FString& Error : Errors)
    {
        UE_LOG(LogTemp, Warning, TEXT("%s"), *Error);
    }
}
```

---

## API Reference

### UGCGCardDatabase Functions

#### Card Lookup
```cpp
// Get card data by card number
const FGCGCardData* GetCardData(FName CardNumber) const;

// Check if card exists
bool CardExists(FName CardNumber) const;

// Get all cards
TArray<FGCGCardData> GetAllCards() const;

// Get cards by type
TArray<FGCGCardData> GetCardsByType(EGCGCardType CardType) const;

// Get cards by color
TArray<FGCGCardData> GetCardsByColor(EGCGCardColor Color) const;
```

#### Token Operations
```cpp
// Get token data
FGCGCardData GetTokenData(FName TokenType) const;

// Check if card number is a token
bool IsToken(FName CardNumber) const;
```

#### Deck Validation
```cpp
// Validate main deck
bool ValidateDeck(const TArray<FName>& DeckList, TArray<FString>& OutErrors) const;

// Validate resource deck
bool ValidateResourceDeck(const TArray<FName>& ResourceDeckList, TArray<FString>& OutErrors) const;
```

#### DataTable Management
```cpp
// Set card data table
void SetCardDataTable(UDataTable* NewDataTable);

// Reload card data
void ReloadCardData();

// Get statistics
int32 GetCardCount() const;
FString GetDatabaseStats() const;
```

---

## Example: Creating a Unit Card

### CSV Entry
```csv
CardNumber,CardName,CardText,CardType,Colors,Level,Cost,AP,HP,Keywords,Rarity,SetNumber,CollectorNumber
GU-011,"RX-93 ν Gundam","[Continuous] This card cannot be chosen by your opponent's Command effects.|[Activate・Main] (2) Rest this card: Search your deck for a card and add it to your hand.",Unit,"Red,Blue",8,8,10,10,"Repair(3),Blocker",SecretRare,SET1,011
```

### Result
- **Card Number**: GU-011
- **Name**: RX-93 ν Gundam
- **Type**: Unit
- **Colors**: Red + Blue (multicolor)
- **Level**: 8
- **Cost**: 8 resources
- **Stats**: 10 AP / 10 HP
- **Keywords**: Repair(3), Blocker
- **Abilities**:
  1. Cannot be targeted by opponent's Commands
  2. Pay 2, Rest: Search deck for any card

---

## Best Practices

1. **Use consistent card numbering**: `SET#-###` (e.g., SET1-001, SET2-015)
2. **Test cards in-engine**: Import small batches and test before adding all cards
3. **Use clear ability text**: Follow the formatting examples above
4. **Include all stat fields**: Even if 0, include AP/HP for clarity
5. **Validate CSV before import**: Use a CSV validator to check formatting
6. **Keep token definitions in code**: Don't add EXBase/EXResource to CSV
7. **Use Excel/Google Sheets**: Makes editing easier, export as CSV when done

---

## Troubleshooting

### Card not found in database
- Check card number spelling (case-sensitive)
- Verify CSV was imported correctly
- Check DataTable is assigned to GameMode
- Run `CardDB->ReloadCardData()` if CSV changed

### Import errors
- Ensure CSV uses commas as delimiters
- Check for special characters in card text (use pipes `|` for line breaks in CSV)
- Verify all columns match `FGCGCardData` structure
- Remove any extra commas or quotes

### Deck validation failing
- Check card numbers exist in database
- Verify deck size (50 for main, 10 for resource)
- Check copy limits (4 per card, 1 for Base)
- Look at error messages for specific issues

---

## Next Steps

After setting up your card database:

1. **Phase 5**: Player Actions - Play cards from hand
2. **Phase 6**: Combat System - Attack and damage resolution
3. **Phase 7**: Keyword System - Implement keyword mechanics
4. **Phase 8**: Effect System - Full card effect parsing and execution

For detailed implementation guides, see the main `GUNDAM_TCG_UE5_ARCHITECTURE.md` document.
