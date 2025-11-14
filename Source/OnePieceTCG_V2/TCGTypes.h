// TCGTypes.h - Core Data Types for One Piece TCG
// Place this in: YourProject/Source/YourProject/TCGTypes.h

#pragma once

#include "CoreMinimal.h"
#include "TCGTypes.generated.h"

// ===== GAME PHASES =====

UENUM(BlueprintType)
enum class EGamePhase : uint8
{
    REFRESH_PHASE       UMETA(DisplayName = "Refresh Phase"),
    DRAW_PHASE          UMETA(DisplayName = "Draw Phase"),
    DON_PHASE           UMETA(DisplayName = "DON Phase"),
    MAIN_PHASE          UMETA(DisplayName = "Main Phase"),
    BATTLE_PHASE        UMETA(DisplayName = "Battle Phase"),
    BATTLE_BLOCK_STEP   UMETA(DisplayName = "Battle - Block Step"),
    BATTLE_COUNTER_STEP UMETA(DisplayName = "Battle - Counter Step"),
    BATTLE_DAMAGE_STEP  UMETA(DisplayName = "Battle - Damage Step"),
    END_PHASE           UMETA(DisplayName = "End Phase"),
    GAME_OVER           UMETA(DisplayName = "Game Over")
};

// ===== CARD TYPES =====

UENUM(BlueprintType)
enum class ECardType : uint8
{
    LEADER      UMETA(DisplayName = "Leader"),
    CHARACTER   UMETA(DisplayName = "Character"),
    EVENT       UMETA(DisplayName = "Event"),
    STAGE       UMETA(DisplayName = "Stage"),
    DON         UMETA(DisplayName = "DON")
};

// ===== CARD ZONES =====

UENUM(BlueprintType)
enum class ECardZone : uint8
{
    NONE            UMETA(DisplayName = "None"),
    DECK            UMETA(DisplayName = "Deck"),
    HAND            UMETA(DisplayName = "Hand"),
    LIFE_ZONE       UMETA(DisplayName = "Life Zone"),
    LEADER_ZONE     UMETA(DisplayName = "Leader Zone"),
    DON_DECK        UMETA(DisplayName = "DON Deck"),
    DON_ZONE        UMETA(DisplayName = "DON Zone"),
    CHARACTER_ZONE  UMETA(DisplayName = "Character Zone"),
    STAGE_ZONE      UMETA(DisplayName = "Stage Zone"),
    TRASH           UMETA(DisplayName = "Trash")
};

// ===== CARD COLORS =====

UENUM(BlueprintType)
enum class ECardColor : uint8
{
    RED         UMETA(DisplayName = "Red"),
    GREEN       UMETA(DisplayName = "Green"),
    BLUE        UMETA(DisplayName = "Blue"),
    PURPLE      UMETA(DisplayName = "Purple"),
    BLACK       UMETA(DisplayName = "Black"),
    YELLOW      UMETA(DisplayName = "Yellow"),
    MULTICOLOR  UMETA(DisplayName = "Multicolor")
};

// ===== EFFECT SYSTEM STRUCTURES =====

// Minimal effect row for data-driven card effects
USTRUCT(BlueprintType)
struct FEffectRow : public FTableRowBase
{
    GENERATED_BODY()

    // When does this effect trigger?
    // Examples: "OnPlay", "WhenAttacking", "ActivateMain", "Trigger", "Counter", "EndOfYourTurn"
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Effect")
    FName Timing;

    // What conditions must be met to activate this effect?
    // Examples: "YourTurn", "DonRequirement:5", "TargetType:Character", "TargetColor:Red"
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Effect")
    TArray<FName> Conditions;

    // What does it cost to activate this effect?
    // Examples: "DonRest:2", "TrashSelf", "DonReturn:1", "TrashCard:1:Hand"
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Effect")
    TArray<FName> Costs;

    // What happens when this effect resolves?
    // Examples: "Draw:2", "GivePower:Self:2000:UntilEndOfTurn", "KO:Target", "Rest:Target"
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Effect")
    TArray<FName> Operations;

    // Human-readable description for UI/tooltips
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Effect")
    FText Description;

    FEffectRow()
    {
        Timing = NAME_None;
    }
};

// ===== CARD DEFINITION (STATIC DATA FROM DATATABLE) =====

USTRUCT(BlueprintType)
struct FCardDefinition : public FTableRowBase
{
    GENERATED_BODY()

    // ===== IDENTITY =====

    // Unique card identifier (e.g., "OP01-001", "ST01-012")
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Identity")
    FName CardID;

    // Display name
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Identity")
    FText CardName;

    // Card type
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Identity")
    ECardType CardType = ECardType::CHARACTER;

    // Multiple colors for multicolor cards (e.g., Red/Green)
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Identity")
    TArray<ECardColor> Colors;

    // Card attributes (e.g., "FILM", "Supernovas", "The Four Emperors")
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Identity")
    TArray<FName> Attributes;

    // Card types/tribes (e.g., "Straw Hat Crew", "Navy", "Revolutionary Army")
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Identity")
    TArray<FName> Types;

    // ===== STATS =====

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Stats")
    int32 Cost = 0;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Stats")
    int32 Power = 0; // For Characters and Leaders

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Stats")
    int32 Life = 0; // For Leaders only

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Stats")
    int32 Counter = 0; // Counter value for Characters (1000, 2000, etc.)

    // ===== PRESENTATION =====

    // Rules text for the card
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Presentation", meta = (MultiLine = true))
    FText CardText;

    // Card artwork (soft reference to avoid loading all art into memory)
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Presentation")
    TSoftObjectPtr<UTexture2D> CardArt;

    // ===== KEYWORDS AND FLAGS =====

    // Keywords (e.g., Blocker, Rush, DoubleAttack, Banish)
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Keywords")
    TSet<FName> Keywords;

    // Does this card have a [Trigger] effect?
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Keywords")
    bool bHasTrigger = false;

    // ===== EFFECTS DATA =====

    // Effects that trigger at various timings
    // For MVP, we store effect IDs that reference FEffectRow entries
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Effects")
    TArray<FName> EffectIDs;

    // Alternative: Inline effect definitions (for simpler cards)
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Effects")
    TArray<FEffectRow> InlineEffects;

    // Default constructor
    FCardDefinition()
    {
        CardID = NAME_None;
        CardName = FText::FromString("Unknown Card");
        CardType = ECardType::CHARACTER;
        Cost = 0;
        Power = 0;
        Life = 0;
        Counter = 0;
        bHasTrigger = false;
    }

    // Helper: Check if card has a specific keyword
    bool HasKeyword(FName Keyword) const
    {
        return Keywords.Contains(Keyword);
    }

    // Helper: Get primary color (first in array)
    ECardColor GetPrimaryColor() const
    {
        return Colors.Num() > 0 ? Colors[0] : ECardColor::RED;
    }

    // Helper: Is this a multicolor card?
    bool IsMulticolor() const
    {
        return Colors.Num() > 1;
    }
};

// ===== ACTIVE MODIFIER (RUNTIME BUFFS/DEBUFFS) =====

UENUM(BlueprintType)
enum class EModifierDuration : uint8
{
    UntilEndOfTurn      UMETA(DisplayName = "Until End of Turn"),
    UntilEndOfBattle    UMETA(DisplayName = "Until End of Battle"),
    WhileInPlay         UMETA(DisplayName = "While In Play"),
    Permanent           UMETA(DisplayName = "Permanent")
};

USTRUCT(BlueprintType)
struct FActiveModifier
{
    GENERATED_BODY()

    // What stat is being modified?
    UPROPERTY(BlueprintReadWrite, EditAnywhere)
    FName ModifierType; // "Power", "Cost", "Counter"

    // Modifier amount (can be negative)
    UPROPERTY(BlueprintReadWrite, EditAnywhere)
    int32 Amount = 0;

    // How long does this last?
    UPROPERTY(BlueprintReadWrite, EditAnywhere)
    EModifierDuration Duration = EModifierDuration::UntilEndOfTurn;

    // Source card instance ID (who applied this modifier?)
    UPROPERTY(BlueprintReadWrite, EditAnywhere)
    int32 SourceInstanceID = 0;

    FActiveModifier()
    {
        ModifierType = NAME_None;
        Amount = 0;
        Duration = EModifierDuration::UntilEndOfTurn;
        SourceInstanceID = 0;
    }
};

// ===== CARD INSTANCE (RUNTIME GAME STATE) =====

USTRUCT(BlueprintType)
struct FCardInstance
{
    GENERATED_BODY()

    // Unique instance ID for this card in play
    UPROPERTY(BlueprintReadWrite, EditAnywhere)
    int32 InstanceID = 0;

    // Reference to static card definition (lookup key into DataTable)
    UPROPERTY(BlueprintReadWrite, EditAnywhere)
    FName CardDefinitionID;

    // Current zone location
    UPROPERTY(BlueprintReadWrite, EditAnywhere)
    ECardZone CurrentZone = ECardZone::NONE;

    // Is this card rested (tapped/exhausted)?
    UPROPERTY(BlueprintReadWrite, EditAnywhere)
    bool bIsRested = false;

    // Number of DON attached to this card
    UPROPERTY(BlueprintReadWrite, EditAnywhere)
    int32 AttachedDonCount = 0;

    // Who owns this card?
    UPROPERTY(BlueprintReadWrite, EditAnywhere)
    int32 OwnerPlayerID = 0;

    // Active modifiers (power buffs, cost reductions, etc.)
    UPROPERTY(BlueprintReadWrite, EditAnywhere)
    TArray<FActiveModifier> ActiveModifiers;

    // Default constructor
    FCardInstance()
    {
        InstanceID = 0;
        CardDefinitionID = NAME_None;
        CurrentZone = ECardZone::NONE;
        bIsRested = false;
        AttachedDonCount = 0;
        OwnerPlayerID = 0;
    }

    // Helper: Calculate total power including modifiers and DON
    int32 GetTotalPower(const FCardDefinition& Definition) const
    {
        int32 TotalPower = Definition.Power;

        // Add DON bonuses (+1000 per DON)
        TotalPower += AttachedDonCount * 1000;

        // Add modifier bonuses
        for (const FActiveModifier& Mod : ActiveModifiers)
        {
            if (Mod.ModifierType == FName("Power"))
            {
                TotalPower += Mod.Amount;
            }
        }

        return TotalPower;
    }

    // Helper: Get effective cost including modifiers
    int32 GetTotalCost(const FCardDefinition& Definition) const
    {
        int32 TotalCost = Definition.Cost;

        for (const FActiveModifier& Mod : ActiveModifiers)
        {
            if (Mod.ModifierType == FName("Cost"))
            {
                TotalCost += Mod.Amount;
            }
        }

        return FMath::Max(0, TotalCost); // Cost can't go below 0
    }
};

// ===== LEGACY CARD DATA (DEPRECATED - Use FCardInstance + FCardDefinition) =====
// Keeping for backward compatibility during transition

USTRUCT(BlueprintType)
struct FCardData
{
    GENERATED_BODY()

    // Unique instance ID for this card in play
    UPROPERTY(BlueprintReadWrite, EditAnywhere)
    int32 InstanceID = 0;

    // Card database ID (for looking up card info)
    UPROPERTY(BlueprintReadWrite, EditAnywhere)
    FString CardID;

    // Basic Info
    UPROPERTY(BlueprintReadWrite, EditAnywhere)
    FString CardName;

    UPROPERTY(BlueprintReadWrite, EditAnywhere)
    ECardType CardType = ECardType::CHARACTER;

    UPROPERTY(BlueprintReadWrite, EditAnywhere)
    ECardColor Color = ECardColor::RED;

    // Stats
    UPROPERTY(BlueprintReadWrite, EditAnywhere)
    int32 Cost = 0;

    UPROPERTY(BlueprintReadWrite, EditAnywhere)
    int32 Power = 0;

    UPROPERTY(BlueprintReadWrite, EditAnywhere)
    int32 Counter = 0; // Counter value (1000, 2000, etc.)

    UPROPERTY(BlueprintReadWrite, EditAnywhere)
    int32 Life = 0; // For leader cards

    // Game State
    UPROPERTY(BlueprintReadWrite, EditAnywhere)
    ECardZone CurrentZone = ECardZone::NONE;

    UPROPERTY(BlueprintReadWrite, EditAnywhere)
    bool bIsRested = false; // Tapped/Exhausted state

    UPROPERTY(BlueprintReadWrite, EditAnywhere)
    int32 AttachedDonCount = 0; // Number of DON attached to this card

    // Owner Info
    UPROPERTY(BlueprintReadWrite, EditAnywhere)
    int32 OwnerPlayerID = 0;

    // Card Text (for reference)
    UPROPERTY(BlueprintReadWrite, EditAnywhere)
    FString CardText;

    UPROPERTY(BlueprintReadWrite, EditAnywhere)
    FString TriggerText;

    // Visual
    UPROPERTY(BlueprintReadWrite, EditAnywhere)
    FString CardImagePath;

    // Default constructor
    FCardData()
    {
        InstanceID = 0;
        CardID = "";
        CardName = "Unknown Card";
        CardType = ECardType::CHARACTER;
        Color = ECardColor::RED;
        Cost = 0;
        Power = 0;
        Counter = 0;
        Life = 0;
        CurrentZone = ECardZone::NONE;
        bIsRested = false;
        AttachedDonCount = 0;
        OwnerPlayerID = 0;
        CardText = "";
        TriggerText = "";
        CardImagePath = "";
    }
};

// ===== ATTACK DATA STRUCTURE =====

USTRUCT(BlueprintType)
struct FAttackData
{
    GENERATED_BODY()

    UPROPERTY(BlueprintReadWrite, EditAnywhere)
    int32 AttackingPlayerID = 0;

    UPROPERTY(BlueprintReadWrite, EditAnywhere)
    int32 DefendingPlayerID = 0;

    UPROPERTY(BlueprintReadWrite, EditAnywhere)
    FCardData AttackerCard;

    UPROPERTY(BlueprintReadWrite, EditAnywhere)
    FCardData TargetCard;

    UPROPERTY(BlueprintReadWrite, EditAnywhere)
    bool bIsTargetingLeader = false;

    UPROPERTY(BlueprintReadWrite, EditAnywhere)
    int32 TotalAttackerPower = 0; // After modifiers

    UPROPERTY(BlueprintReadWrite, EditAnywhere)
    int32 TotalDefenderPower = 0; // After modifiers

    // Default constructor
    FAttackData()
    {
        AttackingPlayerID = 0;
        DefendingPlayerID = 0;
        bIsTargetingLeader = false;
        TotalAttackerPower = 0;
        TotalDefenderPower = 0;
    }
};
