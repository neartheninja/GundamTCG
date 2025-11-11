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

// ===== CARD DATA STRUCTURE =====

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
