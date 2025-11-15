// GCGTypes.h - Gundam Card Game Core Data Types
// Unreal Engine 5.6 - Gundam TCG Implementation
// This file contains all core data structures, enums, and types for the Gundam TCG

#pragma once

#include "CoreMinimal.h"
#include "Engine/DataTable.h"
#include "GCGTypes.generated.h"

// ===========================================================================================
// ENUMERATIONS
// ===========================================================================================

/**
 * Card Types in Gundam TCG
 */
UENUM(BlueprintType)
enum class EGCGCardType : uint8
{
    Unit            UMETA(DisplayName = "Unit"),
    Pilot           UMETA(DisplayName = "Pilot"),
    Command         UMETA(DisplayName = "Command"),
    Base            UMETA(DisplayName = "Base"),
    Resource        UMETA(DisplayName = "Resource"),
    Token           UMETA(DisplayName = "Token")        // EX Base, EX Resource, etc.
};

/**
 * Card Colors (1-2 colors per card max)
 */
UENUM(BlueprintType)
enum class EGCGCardColor : uint8
{
    White           UMETA(DisplayName = "White"),
    Blue            UMETA(DisplayName = "Blue"),
    Green           UMETA(DisplayName = "Green"),
    Red             UMETA(DisplayName = "Red"),
    Black           UMETA(DisplayName = "Black"),
    Yellow          UMETA(DisplayName = "Yellow"),
    Colorless       UMETA(DisplayName = "Colorless")
};

/**
 * Game Zones (where cards can be located)
 */
UENUM(BlueprintType)
enum class EGCGCardZone : uint8
{
    None                UMETA(DisplayName = "None"),
    Deck                UMETA(DisplayName = "Deck"),                    // Main Deck (50 cards)
    ResourceDeck        UMETA(DisplayName = "Resource Deck"),           // Resource Deck (10 cards)
    Hand                UMETA(DisplayName = "Hand"),                    // Player's hand
    ResourceArea        UMETA(DisplayName = "Resource Area"),           // Up to 15 (10 + 5 EX)
    BattleArea          UMETA(DisplayName = "Battle Area"),             // Up to 6 Units
    ShieldStack         UMETA(DisplayName = "Shield Stack"),            // Face-down shields
    BaseSection         UMETA(DisplayName = "Base Section"),            // 1 Base card or EX Base
    Trash               UMETA(DisplayName = "Trash"),                   // Discard pile
    Removal             UMETA(DisplayName = "Removal")                  // Removed from game
};

/**
 * Turn Phases
 */
UENUM(BlueprintType)
enum class EGCGTurnPhase : uint8
{
    NotStarted          UMETA(DisplayName = "Not Started"),
    StartPhase          UMETA(DisplayName = "Start Phase"),             // Active Step → Start Step
    DrawPhase           UMETA(DisplayName = "Draw Phase"),              // Draw 1 card
    ResourcePhase       UMETA(DisplayName = "Resource Phase"),          // Place 1 Resource
    MainPhase           UMETA(DisplayName = "Main Phase"),              // Play cards, attack
    EndPhase            UMETA(DisplayName = "End Phase"),               // Action → End → Hand → Cleanup
    GameOver            UMETA(DisplayName = "Game Over")
};

/**
 * Start Phase Steps
 */
UENUM(BlueprintType)
enum class EGCGStartPhaseStep : uint8
{
    None                UMETA(DisplayName = "None"),
    ActiveStep          UMETA(DisplayName = "Active Step"),             // Set all rested cards active
    StartStep           UMETA(DisplayName = "Start Step")               // "At start of turn" triggers
};

/**
 * End Phase Steps
 */
UENUM(BlueprintType)
enum class EGCGEndPhaseStep : uint8
{
    None                UMETA(DisplayName = "None"),
    ActionStep          UMETA(DisplayName = "Action Step"),             // Action timing
    EndStep             UMETA(DisplayName = "End Step"),                // "At end of turn" triggers
    HandStep            UMETA(DisplayName = "Hand Step"),               // Discard to 10
    CleanupStep         UMETA(DisplayName = "Cleanup Step")             // Expire "this turn" effects
};

/**
 * Combat Steps (during attack)
 */
UENUM(BlueprintType)
enum class EGCGCombatStep : uint8
{
    None                UMETA(DisplayName = "None"),
    AttackStep          UMETA(DisplayName = "Attack Step"),             // Declare attack, rest attacker
    BlockStep           UMETA(DisplayName = "Block Step"),              // Optional blocker activation
    ActionStep          UMETA(DisplayName = "Action Step"),             // Alternate action timing
    DamageStep          UMETA(DisplayName = "Damage Step"),             // Resolve combat damage
    BattleEndStep       UMETA(DisplayName = "Battle End Step")          // Expire "this battle" effects
};

/**
 * Keywords (game mechanics)
 */
UENUM(BlueprintType)
enum class EGCGKeyword : uint8
{
    None                UMETA(DisplayName = "None"),
    Repair              UMETA(DisplayName = "Repair"),                  // Repair X - Recover X HP at end of turn
    Breach              UMETA(DisplayName = "Breach"),                  // Breach X - Damage shields when destroying Unit
    Support             UMETA(DisplayName = "Support"),                 // Support X - Buff allies
    Blocker             UMETA(DisplayName = "Blocker"),                 // Can block attacks
    FirstStrike         UMETA(DisplayName = "First Strike"),            // Damage resolves first
    HighManeuver        UMETA(DisplayName = "High-Maneuver"),           // Evasion mechanic
    Suppression         UMETA(DisplayName = "Suppression"),             // Destroy multiple shields
    Burst               UMETA(DisplayName = "Burst"),                   // Shield-only trigger
    LinkUnit            UMETA(DisplayName = "Link Unit")                // Can attack on deploy turn if linked
};

/**
 * Effect Timing (when effects trigger)
 */
UENUM(BlueprintType)
enum class EGCGEffectTiming : uint8
{
    None                        UMETA(DisplayName = "None"),

    // Deployment
    OnDeploy                    UMETA(DisplayName = "On Deploy"),               // When this card enters Battle Area
    OnPlay                      UMETA(DisplayName = "On Play"),                 // When played from hand

    // Combat
    OnAttack                    UMETA(DisplayName = "On Attack"),               // When this Unit attacks
    OnBlock                     UMETA(DisplayName = "On Block"),                // When this Unit blocks
    WhenAttacked                UMETA(DisplayName = "When Attacked"),           // When this Unit is attacked

    // Destruction
    OnDestroyed                 UMETA(DisplayName = "On Destroyed"),            // When this card destroyed
    WhenUnitDestroyed           UMETA(DisplayName = "When Unit Destroyed"),     // When any Unit destroyed
    WhenAttackDestroysUnit      UMETA(DisplayName = "When Attack Destroys Unit"), // When this attack destroys Unit

    // Pairing
    WhenPaired                  UMETA(DisplayName = "When Paired"),             // When Pilot paired with Unit
    WhilePaired                 UMETA(DisplayName = "While Paired"),            // Continuous while paired

    // Shield
    Burst                       UMETA(DisplayName = "Burst"),                   // When revealed from Shield

    // Activated Abilities
    ActivateMain                UMETA(DisplayName = "Activate・Main"),          // Manual activation in Main Phase
    ActivateAction              UMETA(DisplayName = "Activate・Action"),        // Manual activation in Action Step

    // Turn/Phase
    StartOfTurn                 UMETA(DisplayName = "Start of Turn"),
    EndOfTurn                   UMETA(DisplayName = "End of Turn"),
    StartOfBattle               UMETA(DisplayName = "Start of Battle"),
    EndOfBattle                 UMETA(DisplayName = "End of Battle"),

    // Continuous
    Continuous                  UMETA(DisplayName = "Continuous")               // Always active while in play
};

/**
 * Modifier Duration (how long stat changes last)
 */
UENUM(BlueprintType)
enum class EGCGModifierDuration : uint8
{
    Instant                 UMETA(DisplayName = "Instant"),                 // One-time effect
    UntilEndOfTurn          UMETA(DisplayName = "Until End of Turn"),
    UntilEndOfBattle        UMETA(DisplayName = "Until End of Battle"),
    WhileInPlay             UMETA(DisplayName = "While In Play"),
    Permanent               UMETA(DisplayName = "Permanent")
};

/**
 * Damage Source (FAQ Q97-99: Battle damage vs Effect damage)
 * Used to track what type of damage was dealt for effect triggers
 */
UENUM(BlueprintType)
enum class EGCGDamageSource : uint8
{
    None                    UMETA(DisplayName = "None"),
    BattleDamage            UMETA(DisplayName = "Battle Damage"),       // AP damage during combat (Q97)
    EffectDamage            UMETA(DisplayName = "Effect Damage"),       // Damage from card effects
    ShieldDamage            UMETA(DisplayName = "Shield Damage")        // Damage from breaking shields
};

/**
 * Target Scope (FAQ Q84: "Your Units" vs "Friendly Units")
 * Defines which Units can be targeted by an effect
 */
UENUM(BlueprintType)
enum class EGCGTargetScope : uint8
{
    Self                    UMETA(DisplayName = "Self"),                // This card only
    YourUnits               UMETA(DisplayName = "Your Units"),          // Only your Units (1v1 and 2v2)
    FriendlyUnits           UMETA(DisplayName = "Friendly Units"),      // Your + teammate's Units (2v2 only)
    EnemyUnits              UMETA(DisplayName = "Enemy Units"),         // Opponent's Units
    AllUnits                UMETA(DisplayName = "All Units"),           // All Units on field
    YourPlayer              UMETA(DisplayName = "Your Player"),         // Yourself
    OpponentPlayer          UMETA(DisplayName = "Opponent Player"),     // Opponent
    AnyPlayer               UMETA(DisplayName = "Any Player")           // Any player
};

// ===========================================================================================
// FORWARD DECLARATIONS
// ===========================================================================================

struct FGCGCardData;
struct FGCGCardInstance;

// ===========================================================================================
// CORE DATA STRUCTURES
// ===========================================================================================

/**
 * Link Requirement (for Link Units)
 * Defines what Pilot is needed to create a Link Unit
 */
USTRUCT(BlueprintType)
struct FGCGLinkRequirement
{
    GENERATED_BODY()

    // Required Pilot colors (any match = valid)
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Link")
    TArray<EGCGCardColor> RequiredColors;

    // Required Pilot traits (all must match)
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Link")
    TArray<FName> RequiredTraits;

    // Specific card number required (if any)
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Link")
    FName SpecificCardNumber;

    FGCGLinkRequirement()
    {
        SpecificCardNumber = NAME_None;
    }
};

/**
 * Effect Condition (requirements to activate effect)
 */
USTRUCT(BlueprintType)
struct FGCGEffectCondition
{
    GENERATED_BODY()

    // Condition type (e.g., "YourTurn", "OpponentTurn", "HasLessHP")
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Effect")
    FName ConditionType;

    // Parameters (e.g., for "DonRequirement:5", Parameter = "5")
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Effect")
    TArray<FString> Parameters;

    FGCGEffectCondition()
    {
        ConditionType = NAME_None;
    }
};

/**
 * Effect Cost (what must be paid to activate effect)
 */
USTRUCT(BlueprintType)
struct FGCGEffectCost
{
    GENERATED_BODY()

    // Cost type (e.g., "RestResources", "TrashSelf", "RestThisUnit")
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Effect")
    FName CostType;

    // Amount (e.g., for "RestResources:3", Amount = 3)
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Effect")
    int32 Amount;

    // Additional parameters
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Effect")
    TArray<FString> Parameters;

    FGCGEffectCost()
    {
        CostType = NAME_None;
        Amount = 0;
    }
};

/**
 * Effect Operation (what the effect does)
 */
USTRUCT(BlueprintType)
struct FGCGEffectOperation
{
    GENERATED_BODY()

    // Operation type (e.g., "Draw", "DealDamage", "DestroyUnit", "GiveAP")
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Effect")
    FName OperationType;

    // Target (e.g., "Self", "TargetUnit", "AllEnemyUnits", "OpponentPlayer")
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Effect")
    FName Target;

    // Target scope (FAQ Q84: for "Your Units" vs "Friendly Units" distinction)
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Effect")
    EGCGTargetScope TargetScope;

    // Amount (e.g., for "Draw:2", Amount = 2)
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Effect")
    int32 Amount;

    // Duration (for modifiers)
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Effect")
    EGCGModifierDuration Duration;

    // Additional parameters
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Effect")
    TArray<FString> Parameters;

    FGCGEffectOperation()
    {
        OperationType = NAME_None;
        Target = NAME_None;
        TargetScope = EGCGTargetScope::Self;
        Amount = 0;
        Duration = EGCGModifierDuration::Instant;
    }
};

/**
 * Effect Data (complete effect definition)
 */
USTRUCT(BlueprintType)
struct FGCGEffectData
{
    GENERATED_BODY()

    // When does this effect trigger?
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Effect")
    EGCGEffectTiming Timing;

    // What conditions must be met?
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Effect")
    TArray<FGCGEffectCondition> Conditions;

    // What does it cost to activate?
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Effect")
    TArray<FGCGEffectCost> Costs;

    // What happens when it resolves?
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Effect")
    TArray<FGCGEffectOperation> Operations;

    // Human-readable description (for UI)
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Effect")
    FText Description;

    // Optional: Can this effect be activated multiple times per turn?
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Effect")
    bool bOncePerTurn;

    FGCGEffectData()
    {
        Timing = EGCGEffectTiming::None;
        bOncePerTurn = false;
    }
};

/**
 * Active Modifier (runtime stat modification)
 */
USTRUCT(BlueprintType)
struct FGCGActiveModifier
{
    GENERATED_BODY()

    // What stat is being modified? ("AP", "HP", "Cost")
    UPROPERTY(BlueprintReadWrite, Category = "Modifier")
    FName ModifierType;

    // Modifier amount (can be negative)
    UPROPERTY(BlueprintReadWrite, Category = "Modifier")
    int32 Amount;

    // How long does this last?
    UPROPERTY(BlueprintReadWrite, Category = "Modifier")
    EGCGModifierDuration Duration;

    // Who applied this modifier?
    UPROPERTY(BlueprintReadWrite, Category = "Modifier")
    int32 SourceInstanceID;

    // Turn this modifier was created (for cleanup)
    UPROPERTY(BlueprintReadWrite, Category = "Modifier")
    int32 CreatedOnTurn;

    FGCGActiveModifier()
    {
        ModifierType = NAME_None;
        Amount = 0;
        Duration = EGCGModifierDuration::Instant;
        SourceInstanceID = 0;
        CreatedOnTurn = 0;
    }
};

/**
 * Keyword Instance (runtime keyword with parameters)
 * Used for keywords like Repair X, Breach X, Support X
 */
USTRUCT(BlueprintType)
struct FGCGKeywordInstance
{
    GENERATED_BODY()

    UPROPERTY(BlueprintReadWrite, Category = "Keyword")
    EGCGKeyword Keyword;

    // Parameter value (e.g., X in "Repair X")
    UPROPERTY(BlueprintReadWrite, Category = "Keyword")
    int32 Value;

    // Source of this keyword (card that granted it)
    UPROPERTY(BlueprintReadWrite, Category = "Keyword")
    int32 SourceInstanceID;

    FGCGKeywordInstance()
    {
        Keyword = EGCGKeyword::None;
        Value = 0;
        SourceInstanceID = 0;
    }

    FGCGKeywordInstance(EGCGKeyword InKeyword, int32 InValue, int32 InSource = 0)
        : Keyword(InKeyword), Value(InValue), SourceInstanceID(InSource)
    {
    }
};

/**
 * Card Data (static definition from DataTable)
 * This is the master card definition - one per unique card
 */
USTRUCT(BlueprintType)
struct FGCGCardData : public FTableRowBase
{
    GENERATED_BODY()

    // ===== IDENTITY =====

    // Unique card identifier (e.g., "GCG-001", "GCG-ST01-012")
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Identity")
    FName CardNumber;

    // Display name
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Identity")
    FText CardName;

    // Card type
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Identity")
    EGCGCardType CardType;

    // Colors (1-2 max)
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Identity")
    TArray<EGCGCardColor> Colors;

    // Traits (e.g., "Mobile Suit", "Gundam", "Earth Federation")
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Identity")
    TArray<FName> Traits;

    // ===== STATS =====

    // Level requirement (1-10)
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Stats")
    int32 Level;

    // Resource cost to play
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Stats")
    int32 Cost;

    // Attack Power
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Stats")
    int32 AP;

    // Hit Points
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Stats")
    int32 HP;

    // ===== KEYWORDS =====

    // Keywords with values (e.g., Repair 2, Breach 1)
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Keywords")
    TArray<FGCGKeywordInstance> Keywords;

    // ===== EFFECTS =====

    // Effect definitions
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Effects")
    TArray<FGCGEffectData> Effects;

    // ===== LINK REQUIREMENTS (for Units) =====

    // Link requirement (for Link Units)
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Link")
    FGCGLinkRequirement LinkRequirement;

    // Does this card satisfy link requirements?
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Link")
    bool bCanBePilot;

    // ===== PRESENTATION =====

    // Card artwork
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Presentation")
    TSoftObjectPtr<UTexture2D> CardArt;

    // Rules text
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Presentation", meta = (MultiLine = true))
    FText CardText;

    // Flavor text
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Presentation", meta = (MultiLine = true))
    FText FlavorText;

    // ===== METADATA =====

    // Set (e.g., "Starter Deck 01", "Booster Pack 03")
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Metadata")
    FName Set;

    // Rarity (for collection system)
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Metadata")
    FName Rarity;

    // Card number in set
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Metadata")
    int32 CollectorNumber;

    // Default constructor
    FGCGCardData()
    {
        CardNumber = NAME_None;
        CardName = FText::FromString("Unknown Card");
        CardType = EGCGCardType::Unit;
        Level = 1;
        Cost = 0;
        AP = 0;
        HP = 0;
        bCanBePilot = false;
        Set = NAME_None;
        Rarity = NAME_None;
        CollectorNumber = 0;
    }

    // ===== HELPER FUNCTIONS =====

    // Get primary color (first in array)
    EGCGCardColor GetPrimaryColor() const
    {
        return Colors.Num() > 0 ? Colors[0] : EGCGCardColor::Colorless;
    }

    // Is this a multicolor card?
    bool IsMulticolor() const
    {
        return Colors.Num() > 1;
    }

    // Check if card has a specific keyword
    bool HasKeyword(EGCGKeyword Keyword) const
    {
        for (const FGCGKeywordInstance& KW : Keywords)
        {
            if (KW.Keyword == Keyword)
                return true;
        }
        return false;
    }

    // Get keyword value (e.g., X in "Repair X")
    int32 GetKeywordValue(EGCGKeyword Keyword) const
    {
        for (const FGCGKeywordInstance& KW : Keywords)
        {
            if (KW.Keyword == Keyword)
                return KW.Value;
        }
        return 0;
    }

    // Check if card has a specific trait
    bool HasTrait(FName Trait) const
    {
        return Traits.Contains(Trait);
    }

    // Get total keyword value (stacking multiple instances)
    int32 GetTotalKeywordValue(EGCGKeyword Keyword) const
    {
        int32 Total = 0;
        for (const FGCGKeywordInstance& KW : Keywords)
        {
            if (KW.Keyword == Keyword)
                Total += KW.Value;
        }
        return Total;
    }
};

/**
 * Card Instance (runtime card state)
 * This represents a specific copy of a card in a game
 */
USTRUCT(BlueprintType)
struct FGCGCardInstance
{
    GENERATED_BODY()

    // ===== IDENTITY =====

    // Unique runtime instance ID
    UPROPERTY(BlueprintReadWrite, Category = "Instance")
    int32 InstanceID;

    // Reference to static card definition
    UPROPERTY(BlueprintReadWrite, Category = "Instance")
    FName CardNumber;

    // ===== ZONE & STATE =====

    // Current zone
    UPROPERTY(BlueprintReadWrite, Category = "Zone")
    EGCGCardZone CurrentZone;

    // Is this card active (false = rested/exhausted)?
    UPROPERTY(BlueprintReadWrite, Category = "State")
    bool bIsActive;

    // Accumulated damage
    UPROPERTY(BlueprintReadWrite, Category = "State")
    int32 DamageCounters;

    // ===== OWNERSHIP =====

    // Who owns this card?
    UPROPERTY(BlueprintReadWrite, Category = "Ownership")
    int32 OwnerPlayerID;

    // Who controls this card? (for "take control" effects)
    UPROPERTY(BlueprintReadWrite, Category = "Ownership")
    int32 ControllerPlayerID;

    // ===== PAIRING (for Units with Pilots) =====

    // Paired card instance ID (0 = not paired)
    UPROPERTY(BlueprintReadWrite, Category = "Pairing")
    int32 PairedCardInstanceID;

    // ===== TOKEN =====

    // Is this a token? (EX Base, EX Resource, etc.)
    UPROPERTY(BlueprintReadWrite, Category = "Token")
    bool bIsToken;

    // Token type (if token)
    UPROPERTY(BlueprintReadWrite, Category = "Token")
    FName TokenType;

    // ===== RUNTIME MODIFIERS =====

    // Active stat modifiers
    UPROPERTY(BlueprintReadWrite, Category = "Modifiers")
    TArray<FGCGActiveModifier> ActiveModifiers;

    // Temporary keywords (granted by effects)
    UPROPERTY(BlueprintReadWrite, Category = "Modifiers")
    TArray<FGCGKeywordInstance> TemporaryKeywords;

    // ===== TRACKING =====

    // Turn this card was deployed
    UPROPERTY(BlueprintReadWrite, Category = "Tracking")
    int32 TurnDeployed;

    // Has this Unit attacked this turn?
    UPROPERTY(BlueprintReadWrite, Category = "Tracking")
    bool bHasAttackedThisTurn;

    // Number of times activated this turn (for once-per-turn abilities)
    UPROPERTY(BlueprintReadWrite, Category = "Tracking")
    int32 ActivationCountThisTurn;

    // Last damage source (FAQ Q97-99: for "destroyed with damage" effects)
    UPROPERTY(BlueprintReadWrite, Category = "Tracking")
    EGCGDamageSource LastDamageSource;

    // Default constructor
    FGCGCardInstance()
    {
        InstanceID = 0;
        CardNumber = NAME_None;
        CurrentZone = EGCGCardZone::None;
        bIsActive = true;
        DamageCounters = 0;
        OwnerPlayerID = 0;
        ControllerPlayerID = 0;
        PairedCardInstanceID = 0;
        bIsToken = false;
        TokenType = NAME_None;
        TurnDeployed = 0;
        bHasAttackedThisTurn = false;
        ActivationCountThisTurn = 0;
        LastDamageSource = EGCGDamageSource::None;
    }

    // ===== HELPER FUNCTIONS =====

    // Calculate total AP (including modifiers)
    int32 GetTotalAP(const FGCGCardData* CardData) const
    {
        if (!CardData) return 0;

        int32 TotalAP = CardData->AP;

        // Add modifiers
        for (const FGCGActiveModifier& Mod : ActiveModifiers)
        {
            if (Mod.ModifierType == FName("AP"))
            {
                TotalAP += Mod.Amount;
            }
        }

        return FMath::Max(0, TotalAP);
    }

    // Calculate total HP (including modifiers)
    int32 GetTotalHP(const FGCGCardData* CardData) const
    {
        if (!CardData) return 0;

        int32 TotalHP = CardData->HP;

        // Add modifiers
        for (const FGCGActiveModifier& Mod : ActiveModifiers)
        {
            if (Mod.ModifierType == FName("HP"))
            {
                TotalHP += Mod.Amount;
            }
        }

        return FMath::Max(0, TotalHP);
    }

    // Calculate total Cost (including modifiers)
    int32 GetTotalCost(const FGCGCardData* CardData) const
    {
        if (!CardData) return 0;

        int32 TotalCost = CardData->Cost;

        // Add modifiers
        for (const FGCGActiveModifier& Mod : ActiveModifiers)
        {
            if (Mod.ModifierType == FName("Cost"))
            {
                TotalCost += Mod.Amount;
            }
        }

        return FMath::Max(0, TotalCost);
    }

    // Is this card destroyed? (damage >= HP)
    bool IsDestroyed(const FGCGCardData* CardData) const
    {
        if (!CardData) return false;
        return DamageCounters >= GetTotalHP(CardData);
    }

    // Can this card attack this turn?
    bool CanAttackThisTurn(int32 CurrentTurn, const FGCGCardData* CardData) const
    {
        // Already attacked
        if (bHasAttackedThisTurn)
            return false;

        // Must be active
        if (!bIsActive)
            return false;

        // Newly deployed Units can't attack...
        if (TurnDeployed == CurrentTurn)
        {
            // ...unless they're Link Units
            if (PairedCardInstanceID != 0 && CardData)
            {
                // Is this a Link Unit? (has link requirement satisfied)
                return true; // Link Unit can attack on deploy turn
            }
            return false;
        }

        return true;
    }

    // Get combined keywords (base + temporary)
    TArray<FGCGKeywordInstance> GetAllKeywords(const FGCGCardData* CardData) const
    {
        TArray<FGCGKeywordInstance> AllKeywords;

        if (CardData)
        {
            AllKeywords.Append(CardData->Keywords);
        }

        AllKeywords.Append(TemporaryKeywords);

        return AllKeywords;
    }

    // Get total keyword value (stacking)
    int32 GetTotalKeywordValue(EGCGKeyword Keyword, const FGCGCardData* CardData) const
    {
        int32 Total = 0;

        if (CardData)
        {
            Total += CardData->GetTotalKeywordValue(Keyword);
        }

        for (const FGCGKeywordInstance& KW : TemporaryKeywords)
        {
            if (KW.Keyword == Keyword)
                Total += KW.Value;
        }

        return Total;
    }
};

// ===========================================================================================
// COMBAT DATA STRUCTURES
// ===========================================================================================

/**
 * Attack Data (tracks a single attack)
 */
USTRUCT(BlueprintType)
struct FGCGAttackData
{
    GENERATED_BODY()

    // Attacker instance ID
    UPROPERTY(BlueprintReadWrite, Category = "Attack")
    int32 AttackerInstanceID;

    // Original target instance ID (0 if targeting player)
    UPROPERTY(BlueprintReadWrite, Category = "Attack")
    int32 OriginalTargetInstanceID;

    // Current target instance ID (may change due to Blocker)
    UPROPERTY(BlueprintReadWrite, Category = "Attack")
    int32 CurrentTargetInstanceID;

    // Is the target a player? (false = Unit)
    UPROPERTY(BlueprintReadWrite, Category = "Attack")
    bool bTargetingPlayer;

    // Target player ID (if targeting player)
    UPROPERTY(BlueprintReadWrite, Category = "Attack")
    int32 TargetPlayerID;

    // Has a blocker been activated?
    UPROPERTY(BlueprintReadWrite, Category = "Attack")
    bool bBlockerActivated;

    // Blocker instance ID
    UPROPERTY(BlueprintReadWrite, Category = "Attack")
    int32 BlockerInstanceID;

    // Current combat step
    UPROPERTY(BlueprintReadWrite, Category = "Attack")
    EGCGCombatStep CurrentCombatStep;

    FGCGAttackData()
    {
        AttackerInstanceID = 0;
        OriginalTargetInstanceID = 0;
        CurrentTargetInstanceID = 0;
        bTargetingPlayer = false;
        TargetPlayerID = 0;
        bBlockerActivated = false;
        BlockerInstanceID = 0;
        CurrentCombatStep = EGCGCombatStep::None;
    }
};

// ===========================================================================================
// TEAM BATTLE (2v2) DATA STRUCTURES
// ===========================================================================================

/**
 * Team Info (for 2v2 mode)
 */
USTRUCT(BlueprintType)
struct FGCGTeamInfo
{
    GENERATED_BODY()

    // Team ID (0 = Team A, 1 = Team B)
    UPROPERTY(BlueprintReadWrite, Category = "Team")
    int32 TeamID;

    // Player IDs on this team (2 players)
    UPROPERTY(BlueprintReadWrite, Category = "Team")
    TArray<int32> PlayerIDs;

    // Team leader ID (first player, has final say in decisions)
    UPROPERTY(BlueprintReadWrite, Category = "Team")
    int32 TeamLeaderID;

    // Total Units on field (max 6 per team)
    UPROPERTY(BlueprintReadWrite, Category = "Team")
    int32 TotalUnitsOnField;

    // Shared Base (1 per team)
    UPROPERTY(BlueprintReadWrite, Category = "Team")
    FGCGCardInstance SharedBase;

    // Shared Shield Stack (8 shields total, 4 per player)
    UPROPERTY(BlueprintReadWrite, Category = "Team")
    TArray<FGCGCardInstance> SharedShieldStack;

    FGCGTeamInfo()
    {
        TeamID = 0;
        TeamLeaderID = 0;
        TotalUnitsOnField = 0;
    }
};

// ===========================================================================================
// DECK & COLLECTION DATA STRUCTURES
// ===========================================================================================

/**
 * Deck List (50-card Main Deck + 10-card Resource Deck)
 */
USTRUCT(BlueprintType)
struct FGCGDeckList
{
    GENERATED_BODY()

    // Deck name
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Deck")
    FText DeckName;

    // Main Deck (50 cards exactly)
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Deck")
    TArray<FName> MainDeck;

    // Resource Deck (10 cards exactly)
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Deck")
    TArray<FName> ResourceDeck;

    // Colors (1-2 max)
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Deck")
    TArray<EGCGCardColor> DeckColors;

    // Is this deck valid?
    UPROPERTY(BlueprintReadOnly, Category = "Deck")
    bool bIsValid;

    FGCGDeckList()
    {
        DeckName = FText::FromString("New Deck");
        bIsValid = false;
    }
};

// ===========================================================================================
// END OF FILE
// ===========================================================================================
