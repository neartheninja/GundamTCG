// GCGKeywordSubsystem.cpp - Gundam Card Game Keyword Processing Subsystem Implementation

#include "GCGKeywordSubsystem.h"
#include "GCGCombatSubsystem.h"
#include "GCGZoneSubsystem.h"
#include "../PlayerState/GCGPlayerState.h"
#include "../GameState/GCGGameState.h"

// ===========================================================================================
// SUBSYSTEM LIFECYCLE
// ===========================================================================================

void UGCGKeywordSubsystem::Initialize(FSubsystemCollectionBase& Collection)
{
	Super::Initialize(Collection);
	UE_LOG(LogTemp, Log, TEXT("[GCGKeywordSubsystem] Initialized"));
}

void UGCGKeywordSubsystem::Deinitialize()
{
	Super::Deinitialize();
	UE_LOG(LogTemp, Log, TEXT("[GCGKeywordSubsystem] Deinitialized"));
}

// ===========================================================================================
// KEYWORD CHECKS
// ===========================================================================================

bool UGCGKeywordSubsystem::HasKeyword(const FGCGCardInstance& Card, EGCGKeyword Keyword) const
{
	for (const FGCGKeywordInstance& KeywordInstance : Card.Keywords)
	{
		if (KeywordInstance.Keyword == Keyword)
		{
			return true;
		}
	}
	return false;
}

int32 UGCGKeywordSubsystem::GetKeywordValue(const FGCGCardInstance& Card, EGCGKeyword Keyword) const
{
	int32 TotalValue = 0;

	for (const FGCGKeywordInstance& KeywordInstance : Card.Keywords)
	{
		if (KeywordInstance.Keyword == Keyword)
		{
			TotalValue += KeywordInstance.Value;
		}
	}

	return TotalValue;
}

bool UGCGKeywordSubsystem::DoesKeywordStack(EGCGKeyword Keyword) const
{
	switch (Keyword)
	{
		// Stacking keywords (can have multiple instances)
		case EGCGKeyword::Repair:
		case EGCGKeyword::Breach:
		case EGCGKeyword::Support:
			return true;

		// Non-stacking keywords (only one instance matters)
		case EGCGKeyword::Blocker:
		case EGCGKeyword::FirstStrike:
		case EGCGKeyword::HighManeuver:
		case EGCGKeyword::Suppression:
		case EGCGKeyword::Burst:
		case EGCGKeyword::LinkUnit:
			return false;

		default:
			return false;
	}
}

// ===========================================================================================
// REPAIR KEYWORD (Heal at end of turn)
// ===========================================================================================

FGCGKeywordResult UGCGKeywordSubsystem::ProcessRepair(FGCGCardInstance& Card)
{
	FGCGKeywordResult Result;

	// Check if card has Repair
	if (!HasKeyword(Card, EGCGKeyword::Repair))
	{
		Result.Message = FText::FromString(TEXT("Card does not have Repair keyword"));
		return Result;
	}

	// Get total Repair value (stacks)
	int32 RepairValue = GetKeywordValue(Card, EGCGKeyword::Repair);

	if (RepairValue <= 0)
	{
		Result.Message = FText::FromString(TEXT("Repair value is 0"));
		return Result;
	}

	// Only repair if card has damage
	if (Card.CurrentDamage <= 0)
	{
		Result.bSuccess = true;
		Result.Message = FText::FromString(TEXT("Card has no damage to repair"));
		return Result;
	}

	// Apply healing
	int32 HealingDone = ApplyHealing(Card, RepairValue);

	Result.bSuccess = true;
	Result.HealingDone = HealingDone;
	Result.Message = FText::FromString(FString::Printf(TEXT("Repaired %d damage"), HealingDone));

	LogKeyword(TEXT("Repair"), FString::Printf(TEXT("[%s] recovered %d damage (Repair %d)"),
		*Card.CardName.ToString(), HealingDone, RepairValue));

	return Result;
}

int32 UGCGKeywordSubsystem::ProcessRepairForPlayer(AGCGPlayerState* PlayerState)
{
	if (!PlayerState)
	{
		UE_LOG(LogTemp, Error, TEXT("[GCGKeywordSubsystem] ProcessRepairForPlayer: Invalid PlayerState"));
		return 0;
	}

	int32 TotalHealing = 0;

	// Process Repair for all Units in Battle Area
	for (FGCGCardInstance& Unit : PlayerState->BattleArea)
	{
		if (HasKeyword(Unit, EGCGKeyword::Repair))
		{
			FGCGKeywordResult Result = ProcessRepair(Unit);
			TotalHealing += Result.HealingDone;
		}
	}

	// Process Repair for Base (if it has Repair keyword somehow)
	if (PlayerState->BaseSection.Num() > 0)
	{
		FGCGCardInstance& Base = PlayerState->BaseSection[0];
		if (HasKeyword(Base, EGCGKeyword::Repair))
		{
			FGCGKeywordResult Result = ProcessRepair(Base);
			TotalHealing += Result.HealingDone;
		}
	}

	if (TotalHealing > 0)
	{
		UE_LOG(LogTemp, Log, TEXT("[GCGKeywordSubsystem] Player %d: Total Repair healing = %d"),
			PlayerState->GetPlayerID(), TotalHealing);
	}

	return TotalHealing;
}

// ===========================================================================================
// BREACH KEYWORD (Extra shield damage when destroying Unit)
// ===========================================================================================

FGCGKeywordResult UGCGKeywordSubsystem::ProcessBreach(const FGCGCardInstance& Attacker, AGCGPlayerState* DefendingPlayer, AGCGGameState* GameState)
{
	FGCGKeywordResult Result;

	if (!DefendingPlayer || !GameState)
	{
		Result.Message = FText::FromString(TEXT("Invalid parameters"));
		return Result;
	}

	// Check if attacker has Breach
	if (!HasKeyword(Attacker, EGCGKeyword::Breach))
	{
		Result.Message = FText::FromString(TEXT("Attacker does not have Breach keyword"));
		return Result;
	}

	// Get total Breach value (stacks)
	int32 BreachValue = GetKeywordValue(Attacker, EGCGKeyword::Breach);

	if (BreachValue <= 0)
	{
		Result.Message = FText::FromString(TEXT("Breach value is 0"));
		return Result;
	}

	// Break shields
	int32 ShieldsBroken = BreakShields(BreachValue, DefendingPlayer);

	Result.bSuccess = (ShieldsBroken > 0);
	Result.ShieldsBroken = ShieldsBroken;
	Result.Message = FText::FromString(FString::Printf(TEXT("Breach %d: Broke %d shields"), BreachValue, ShieldsBroken));

	LogKeyword(TEXT("Breach"), FString::Printf(TEXT("[%s] triggered Breach %d - broke %d shields"),
		*Attacker.CardName.ToString(), BreachValue, ShieldsBroken));

	return Result;
}

// ===========================================================================================
// SUPPORT KEYWORD (Buff allies)
// ===========================================================================================

int32 UGCGKeywordSubsystem::CalculateSupportBuff(const FGCGCardInstance& Unit, AGCGPlayerState* PlayerState)
{
	if (!PlayerState)
	{
		return 0;
	}

	int32 TotalBuff = 0;

	// Check all Units in Battle Area for Support keyword
	for (const FGCGCardInstance& Ally : PlayerState->BattleArea)
	{
		// Don't buff yourself
		if (Ally.InstanceID == Unit.InstanceID)
		{
			continue;
		}

		// Check if ally has Support
		if (HasKeyword(Ally, EGCGKeyword::Support))
		{
			int32 SupportValue = GetKeywordValue(Ally, EGCGKeyword::Support);
			TotalBuff += SupportValue;
		}
	}

	return TotalBuff;
}

TArray<FGCGCardInstance> UGCGKeywordSubsystem::GetUnitsWithSupport(AGCGPlayerState* PlayerState)
{
	TArray<FGCGCardInstance> Result;

	if (!PlayerState)
	{
		return Result;
	}

	for (const FGCGCardInstance& Unit : PlayerState->BattleArea)
	{
		if (HasKeyword(Unit, EGCGKeyword::Support))
		{
			Result.Add(Unit);
		}
	}

	return Result;
}

// ===========================================================================================
// FIRST STRIKE KEYWORD (Deal damage first in combat)
// ===========================================================================================

bool UGCGKeywordSubsystem::HasFirstStrikeAdvantage(const FGCGCardInstance& Attacker, const FGCGCardInstance& Defender) const
{
	bool bAttackerHasFirstStrike = HasKeyword(Attacker, EGCGKeyword::FirstStrike);
	bool bDefenderHasFirstStrike = HasKeyword(Defender, EGCGKeyword::FirstStrike);

	// Attacker has advantage if they have FirstStrike and defender doesn't
	return bAttackerHasFirstStrike && !bDefenderHasFirstStrike;
}

FGCGKeywordResult UGCGKeywordSubsystem::ProcessFirstStrike(const FGCGCardInstance& Attacker, FGCGCardInstance& Defender, bool& OutDefenderDestroyed)
{
	FGCGKeywordResult Result;
	OutDefenderDestroyed = false;

	// Check if attacker has First Strike advantage
	if (!HasFirstStrikeAdvantage(Attacker, Defender))
	{
		Result.Message = FText::FromString(TEXT("No First Strike advantage"));
		return Result;
	}

	// Calculate damage
	int32 AttackerAP = Attacker.GetTotalAP();
	int32 Damage = AttackerAP;

	// Apply damage to defender
	Defender.CurrentDamage += Damage;

	Result.bSuccess = true;
	Result.bFirstStrikeDamage = true;
	Result.DamageDealt = Damage;

	// Check if defender is destroyed
	if (Defender.IsDestroyed())
	{
		OutDefenderDestroyed = true;
		Result.Message = FText::FromString(FString::Printf(TEXT("First Strike: Dealt %d damage - Defender destroyed!"), Damage));

		LogKeyword(TEXT("FirstStrike"), FString::Printf(TEXT("[%s] dealt %d damage to [%s] - Destroyed (no retaliation)"),
			*Attacker.CardName.ToString(), Damage, *Defender.CardName.ToString()));
	}
	else
	{
		Result.Message = FText::FromString(FString::Printf(TEXT("First Strike: Dealt %d damage"), Damage));

		LogKeyword(TEXT("FirstStrike"), FString::Printf(TEXT("[%s] dealt %d damage to [%s] (First Strike)"),
			*Attacker.CardName.ToString(), Damage, *Defender.CardName.ToString()));
	}

	return Result;
}

// ===========================================================================================
// HIGH-MANEUVER KEYWORD (Evasion mechanic)
// ===========================================================================================

bool UGCGKeywordSubsystem::CanEvadeWithHighManeuver(const FGCGCardInstance& Defender, AGCGPlayerState* PlayerState) const
{
	if (!PlayerState)
	{
		return false;
	}

	// Check if defender has High-Maneuver
	if (!HasKeyword(Defender, EGCGKeyword::HighManeuver))
	{
		return false;
	}

	// Check if player has at least 1 active resource to pay cost
	int32 ActiveResources = PlayerState->GetActiveResourceCount();
	return (ActiveResources >= 1);
}

FGCGKeywordResult UGCGKeywordSubsystem::ProcessHighManeuver(const FGCGCardInstance& Defender, AGCGPlayerState* PlayerState)
{
	FGCGKeywordResult Result;

	if (!CanEvadeWithHighManeuver(Defender, PlayerState))
	{
		Result.Message = FText::FromString(TEXT("Cannot evade (no High-Maneuver or insufficient resources)"));
		return Result;
	}

	// Pay cost: Rest 1 resource
	bool bCostPaid = false;
	for (FGCGCardInstance& Resource : PlayerState->ResourceArea)
	{
		if (Resource.bIsActive)
		{
			Resource.bIsActive = false; // Rest the resource
			bCostPaid = true;
			break;
		}
	}

	if (!bCostPaid)
	{
		Result.Message = FText::FromString(TEXT("Failed to pay evasion cost"));
		return Result;
	}

	// Evasion successful
	Result.bSuccess = true;
	Result.bEvaded = true;
	Result.Message = FText::FromString(TEXT("High-Maneuver: Attack evaded!"));

	LogKeyword(TEXT("HighManeuver"), FString::Printf(TEXT("[%s] evaded attack (paid 1 resource)"),
		*Defender.CardName.ToString()));

	return Result;
}

// ===========================================================================================
// SUPPRESSION KEYWORD (Destroy multiple shields simultaneously)
// ===========================================================================================

FGCGKeywordResult UGCGKeywordSubsystem::ProcessSuppression(const FGCGCardInstance& Attacker, AGCGPlayerState* DefendingPlayer, AGCGGameState* GameState)
{
	FGCGKeywordResult Result;

	if (!DefendingPlayer || !GameState)
	{
		Result.Message = FText::FromString(TEXT("Invalid parameters"));
		return Result;
	}

	// Check if attacker has Suppression
	if (!HasKeyword(Attacker, EGCGKeyword::Suppression))
	{
		Result.Message = FText::FromString(TEXT("Attacker does not have Suppression keyword"));
		return Result;
	}

	// Get shield count
	int32 ShieldCount = DefendingPlayer->GetShieldCount();

	if (ShieldCount == 0)
	{
		// No shields - damage goes to base
		FGCGCardInstance& Base = DefendingPlayer->BaseSection[0];
		int32 AttackerAP = Attacker.GetTotalAP();
		Base.CurrentDamage += AttackerAP;

		Result.bSuccess = true;
		Result.DamageDealt = AttackerAP;
		Result.Message = FText::FromString(FString::Printf(TEXT("Suppression: No shields - dealt %d to Base"), AttackerAP));

		// Check if player lost
		if (Base.CurrentDamage >= Base.HP)
		{
			DefendingPlayer->bHasLost = true;
			UE_LOG(LogTemp, Warning, TEXT("[GCGKeywordSubsystem] Player %d lost (Base destroyed by Suppression)"),
				DefendingPlayer->GetPlayerID());
		}
	}
	else
	{
		// Break ALL shields simultaneously
		int32 ShieldsBroken = BreakShields(ShieldCount, DefendingPlayer);

		Result.bSuccess = true;
		Result.ShieldsBroken = ShieldsBroken;
		Result.Message = FText::FromString(FString::Printf(TEXT("Suppression: Destroyed all %d shields!"), ShieldsBroken));

		LogKeyword(TEXT("Suppression"), FString::Printf(TEXT("[%s] destroyed all %d shields simultaneously"),
			*Attacker.CardName.ToString(), ShieldsBroken));
	}

	return Result;
}

// ===========================================================================================
// BURST KEYWORD (Shield trigger effects)
// ===========================================================================================

bool UGCGKeywordSubsystem::HasBurst(const FGCGCardInstance& Card) const
{
	return HasKeyword(Card, EGCGKeyword::Burst);
}

FGCGKeywordResult UGCGKeywordSubsystem::ProcessBurst(const FGCGCardInstance& ShieldCard, AGCGPlayerState* PlayerState, AGCGGameState* GameState)
{
	FGCGKeywordResult Result;

	if (!PlayerState || !GameState)
	{
		Result.Message = FText::FromString(TEXT("Invalid parameters"));
		return Result;
	}

	// Check if card has Burst
	if (!HasBurst(ShieldCard))
	{
		Result.Message = FText::FromString(TEXT("Card does not have Burst keyword"));
		return Result;
	}

	// Get Zone Subsystem to move card to hand
	UGCGZoneSubsystem* ZoneSubsystem = GetGameInstance()->GetSubsystem<UGCGZoneSubsystem>();
	if (!ZoneSubsystem)
	{
		Result.Message = FText::FromString(TEXT("Zone Subsystem not available"));
		return Result;
	}

	// Find the card in Trash (it was just broken from ShieldStack)
	FGCGCardInstance* CardInTrash = nullptr;
	for (FGCGCardInstance& Card : PlayerState->Trash)
	{
		if (Card.InstanceID == ShieldCard.InstanceID)
		{
			CardInTrash = &Card;
			break;
		}
	}

	if (!CardInTrash)
	{
		Result.Message = FText::FromString(TEXT("Burst card not found in Trash"));
		return Result;
	}

	// Move card from Trash to Hand (Burst effect)
	bool bMoved = ZoneSubsystem->MoveCard(*CardInTrash, EGCGCardZone::Trash, EGCGCardZone::Hand, PlayerState, GameState);

	if (bMoved)
	{
		Result.bSuccess = true;
		Result.Message = FText::FromString(TEXT("Burst: Card returned to hand!"));

		LogKeyword(TEXT("Burst"), FString::Printf(TEXT("[%s] triggered Burst - returned to hand"),
			*ShieldCard.CardName.ToString()));
	}
	else
	{
		Result.Message = FText::FromString(TEXT("Failed to move Burst card to hand"));
	}

	// TODO Phase 8: Trigger Burst effect (if card has effect with Burst timing)

	return Result;
}

// ===========================================================================================
// LINK UNIT KEYWORD (Bypass summoning sickness when paired with Pilot)
// ===========================================================================================

bool UGCGKeywordSubsystem::IsLinkUnit(const FGCGCardInstance& Card) const
{
	return HasKeyword(Card, EGCGKeyword::LinkUnit);
}

bool UGCGKeywordSubsystem::IsPairedWithPilot(const FGCGCardInstance& LinkUnit, AGCGPlayerState* PlayerState) const
{
	if (!PlayerState)
	{
		return false;
	}

	// Check if LinkUnit has a paired card
	if (LinkUnit.PairedCardInstanceID == 0)
	{
		return false;
	}

	// Find the paired card in Battle Area
	for (const FGCGCardInstance& Card : PlayerState->BattleArea)
	{
		if (Card.InstanceID == LinkUnit.PairedCardInstanceID)
		{
			// Check if paired card is a Pilot
			return (Card.CardType == EGCGCardType::Pilot);
		}
	}

	return false;
}

bool UGCGKeywordSubsystem::CanLinkUnitAttack(const FGCGCardInstance& LinkUnit, AGCGPlayerState* PlayerState, AGCGGameState* GameState) const
{
	if (!PlayerState || !GameState)
	{
		return false;
	}

	// If not a Link Unit, use normal summoning sickness rules
	if (!IsLinkUnit(LinkUnit))
	{
		return (LinkUnit.TurnDeployed < GameState->TurnNumber);
	}

	// Link Unit can attack if paired with Pilot (bypass summoning sickness)
	if (IsPairedWithPilot(LinkUnit, PlayerState))
	{
		return true;
	}

	// Not paired - use normal summoning sickness rules
	return (LinkUnit.TurnDeployed < GameState->TurnNumber);
}

// ===========================================================================================
// KEYWORD UTILITY
// ===========================================================================================

FString UGCGKeywordSubsystem::GetKeywordName(EGCGKeyword Keyword) const
{
	switch (Keyword)
	{
		case EGCGKeyword::Repair:        return TEXT("Repair");
		case EGCGKeyword::Breach:        return TEXT("Breach");
		case EGCGKeyword::Support:       return TEXT("Support");
		case EGCGKeyword::Blocker:       return TEXT("Blocker");
		case EGCGKeyword::FirstStrike:   return TEXT("First Strike");
		case EGCGKeyword::HighManeuver:  return TEXT("High-Maneuver");
		case EGCGKeyword::Suppression:   return TEXT("Suppression");
		case EGCGKeyword::Burst:         return TEXT("Burst");
		case EGCGKeyword::LinkUnit:      return TEXT("Link Unit");
		default:                         return TEXT("Unknown");
	}
}

FString UGCGKeywordSubsystem::GetKeywordDescription(EGCGKeyword Keyword, int32 Value) const
{
	switch (Keyword)
	{
		case EGCGKeyword::Repair:
			return FString::Printf(TEXT("Repair %d: Recover %d damage at end of turn"), Value, Value);

		case EGCGKeyword::Breach:
			return FString::Printf(TEXT("Breach %d: When this destroys a Unit, break %d shields"), Value, Value);

		case EGCGKeyword::Support:
			return FString::Printf(TEXT("Support %d: All friendly Units get +%d AP"), Value, Value);

		case EGCGKeyword::Blocker:
			return TEXT("Blocker: Can redirect attacks to this Unit");

		case EGCGKeyword::FirstStrike:
			return TEXT("First Strike: Deals damage before opponent (no retaliation if opponent destroyed)");

		case EGCGKeyword::HighManeuver:
			return TEXT("High-Maneuver: Pay 1 resource to evade an attack");

		case EGCGKeyword::Suppression:
			return TEXT("Suppression: Destroys all shields simultaneously when dealing player damage");

		case EGCGKeyword::Burst:
			return TEXT("Burst: When broken as a shield, return to hand and trigger effect");

		case EGCGKeyword::LinkUnit:
			return TEXT("Link Unit: Can attack on deployment turn when paired with a Pilot");

		default:
			return TEXT("Unknown keyword");
	}
}

bool UGCGKeywordSubsystem::HasBlocker(const FGCGCardInstance& Card) const
{
	return HasKeyword(Card, EGCGKeyword::Blocker);
}

// ===========================================================================================
// INTERNAL HELPERS
// ===========================================================================================

int32 UGCGKeywordSubsystem::ApplyHealing(FGCGCardInstance& Card, int32 Amount)
{
	if (Card.CurrentDamage <= 0)
	{
		return 0; // No damage to heal
	}

	int32 ActualHealing = FMath::Min(Amount, Card.CurrentDamage);
	Card.CurrentDamage -= ActualHealing;

	return ActualHealing;
}

int32 UGCGKeywordSubsystem::BreakShields(int32 Count, AGCGPlayerState* PlayerState)
{
	if (!PlayerState)
	{
		return 0;
	}

	// Use Combat Subsystem to break shields
	UGCGCombatSubsystem* CombatSubsystem = GetGameInstance()->GetSubsystem<UGCGCombatSubsystem>();
	if (!CombatSubsystem)
	{
		UE_LOG(LogTemp, Error, TEXT("[GCGKeywordSubsystem] Combat Subsystem not available for shield breaking"));
		return 0;
	}

	// Break shields (handled by Combat Subsystem)
	// This is a simple implementation - Combat Subsystem has the full logic
	int32 ShieldsBroken = 0;
	int32 ShieldsToBreak = FMath::Min(Count, PlayerState->ShieldStack.Num());

	// Get Zone Subsystem
	UGCGZoneSubsystem* ZoneSubsystem = GetGameInstance()->GetSubsystem<UGCGZoneSubsystem>();
	if (!ZoneSubsystem)
	{
		return 0;
	}

	// Break shields from top of stack
	for (int32 i = 0; i < ShieldsToBreak; i++)
	{
		if (PlayerState->ShieldStack.Num() > 0)
		{
			FGCGCardInstance Shield = PlayerState->ShieldStack[0];

			// Move shield to Trash
			bool bMoved = ZoneSubsystem->MoveCard(Shield, EGCGCardZone::ShieldStack, EGCGCardZone::Trash, PlayerState, nullptr);
			if (bMoved)
			{
				ShieldsBroken++;

				// TODO Phase 7: Check for Burst keyword and process if present
			}
		}
	}

	return ShieldsBroken;
}

void UGCGKeywordSubsystem::LogKeyword(const FString& KeywordName, const FString& Message) const
{
	UE_LOG(LogTemp, Log, TEXT("[GCGKeywordSubsystem] %s: %s"), *KeywordName, *Message);
}
