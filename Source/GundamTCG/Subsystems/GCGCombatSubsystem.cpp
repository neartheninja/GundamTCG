// GCGCombatSubsystem.cpp - Combat System Subsystem Implementation
// Unreal Engine 5.6 - Gundam TCG Implementation

#include "GCGCombatSubsystem.h"
#include "GCGKeywordSubsystem.h"
#include "GCGLinkUnitSubsystem.h"
#include "GundamTCG/PlayerState/GCGPlayerState.h"
#include "GundamTCG/GameState/GCGGameState.h"
#include "GundamTCG/Subsystems/GCGZoneSubsystem.h"

// ===== SUBSYSTEM LIFECYCLE =====

void UGCGCombatSubsystem::Initialize(FSubsystemCollectionBase& Collection)
{
	Super::Initialize(Collection);

	UE_LOG(LogTemp, Log, TEXT("UGCGCombatSubsystem::Initialize - Combat Subsystem initialized"));
}

void UGCGCombatSubsystem::Deinitialize()
{
	UE_LOG(LogTemp, Log, TEXT("UGCGCombatSubsystem::Deinitialize - Combat Subsystem shutdown"));

	Super::Deinitialize();
}

// ===== ATTACK DECLARATION =====

FGCGCombatResult UGCGCombatSubsystem::DeclareAttack(int32 AttackerInstanceID,
	AGCGPlayerState* AttackingPlayer, AGCGPlayerState* DefendingPlayer, AGCGGameState* GameState, int32 TargetUnitInstanceID)
{
	if (!AttackingPlayer || !DefendingPlayer || !GameState)
	{
		return FGCGCombatResult(false, TEXT("Invalid player or game state"));
	}

	// Find attacker in Battle Area
	FGCGCardInstance AttackerInstance;
	EGCGCardZone AttackerZone;
	if (!AttackingPlayer->FindCardByInstanceID(AttackerInstanceID, AttackerInstance, AttackerZone))
	{
		return FGCGCombatResult(false, TEXT("Attacker not found"));
	}

	if (AttackerZone != EGCGCardZone::BattleArea)
	{
		return FGCGCombatResult(false, TEXT("Card is not in Battle Area"));
	}

	// Validate can attack
	FGCGCombatResult ValidationResult = CanAttack(AttackerInstance, AttackingPlayer, GameState);
	if (!ValidationResult.bSuccess)
	{
		return ValidationResult;
	}

	// Rule 8-2-1: Validate attack target (player or rested Unit)
	bool bTargetingPlayer = (TargetUnitInstanceID == 0);

	if (!bTargetingPlayer)
	{
		// Targeting a specific Unit - must be rested
		FGCGCardInstance TargetUnit;
		EGCGCardZone TargetZone;
		if (!DefendingPlayer->FindCardByInstanceID(TargetUnitInstanceID, TargetUnit, TargetZone))
		{
			return FGCGCombatResult(false, TEXT("Target Unit not found"));
		}

		if (TargetZone != EGCGCardZone::BattleArea)
		{
			return FGCGCombatResult(false, TEXT("Target Unit is not in Battle Area"));
		}

		// Rule 8-2-1: Can only attack rested Units
		if (TargetUnit.bIsActive)
		{
			return FGCGCombatResult(false, TEXT("Target Unit must be rested (inactive)"));
		}

		UE_LOG(LogTemp, Log, TEXT("UGCGCombatSubsystem::DeclareAttack - Targeting rested Unit: %s (ID: %d)"),
			*TargetUnit.CardName.ToString(), TargetUnitInstanceID);
	}

	// Create attack declaration
	FGCGAttackDeclaration Attack;
	Attack.AttackerInstanceID = AttackerInstanceID;
	Attack.AttackingPlayerID = AttackingPlayer->GetPlayerID();
	Attack.DefendingPlayerID = DefendingPlayer->GetPlayerID();
	Attack.bTargetingBase = bTargetingPlayer; // True if attacking player, false if attacking Unit
	Attack.TargetUnitInstanceID = TargetUnitInstanceID; // 0 if targeting player
	Attack.BlockerInstanceID = 0; // No blocker yet
	Attack.bResolved = false;

	// Add to current attacks
	GameState->CurrentAttacks.Add(Attack);

	// Mark attacker as having attacked this turn
	for (FGCGCardInstance& BattleCard : AttackingPlayer->BattleArea)
	{
		if (BattleCard.InstanceID == AttackerInstanceID)
		{
			BattleCard.bHasAttackedThisTurn = true;
			// Rest the attacker (attacking rests the unit)
			BattleCard.bIsActive = false;
			break;
		}
	}

	if (bTargetingPlayer)
	{
		UE_LOG(LogTemp, Log, TEXT("UGCGCombatSubsystem::DeclareAttack - Player %d declared attack with %s (ID: %d) on Player %d"),
			AttackingPlayer->GetPlayerID(), *AttackerInstance.CardName.ToString(), AttackerInstanceID,
			DefendingPlayer->GetPlayerID());
	}
	else
	{
		UE_LOG(LogTemp, Log, TEXT("UGCGCombatSubsystem::DeclareAttack - Player %d declared attack with %s (ID: %d) on enemy Unit (ID: %d)"),
			AttackingPlayer->GetPlayerID(), *AttackerInstance.CardName.ToString(), AttackerInstanceID,
			TargetUnitInstanceID);
	}

	// TODO: Trigger "On Attack" effects (Phase 8)

	return FGCGCombatResult(true);
}

FGCGCombatResult UGCGCombatSubsystem::CanAttack(const FGCGCardInstance& AttackerInstance,
	AGCGPlayerState* AttackingPlayer, AGCGGameState* GameState) const
{
	if (!AttackingPlayer || !GameState)
	{
		return FGCGCombatResult(false, TEXT("Invalid player or game state"));
	}

	// Must be a Unit
	if (AttackerInstance.CardType != EGCGCardType::Unit)
	{
		return FGCGCombatResult(false, TEXT("Only Units can attack"));
	}

	// Must be active (not rested)
	if (!AttackerInstance.bIsActive)
	{
		return FGCGCombatResult(false, TEXT("Unit is rested"));
	}

	// Check summoning sickness (can't attack on turn deployed)
	if (HasSummoningSickness(AttackerInstance, GameState))
	{
		return FGCGCombatResult(false, TEXT("Unit has summoning sickness (deployed this turn)"));
	}

	// Can't attack if already attacked this turn
	if (AttackerInstance.bHasAttackedThisTurn)
	{
		return FGCGCombatResult(false, TEXT("Unit has already attacked this turn"));
	}

	return FGCGCombatResult(true);
}

// ===== BLOCKER DECLARATION =====

FGCGCombatResult UGCGCombatSubsystem::DeclareBlocker(int32 AttackIndex, int32 BlockerInstanceID,
	AGCGPlayerState* DefendingPlayer, AGCGGameState* GameState)
{
	if (!DefendingPlayer || !GameState)
	{
		return FGCGCombatResult(false, TEXT("Invalid player or game state"));
	}

	// Validate attack index
	if (!GameState->CurrentAttacks.IsValidIndex(AttackIndex))
	{
		return FGCGCombatResult(false, TEXT("Invalid attack index"));
	}

	FGCGAttackDeclaration& Attack = GameState->CurrentAttacks[AttackIndex];

	// Find blocker in Battle Area
	FGCGCardInstance BlockerInstance;
	EGCGCardZone BlockerZone;
	if (!DefendingPlayer->FindCardByInstanceID(BlockerInstanceID, BlockerInstance, BlockerZone))
	{
		return FGCGCombatResult(false, TEXT("Blocker not found"));
	}

	if (BlockerZone != EGCGCardZone::BattleArea)
	{
		return FGCGCombatResult(false, TEXT("Card is not in Battle Area"));
	}

	// Validate can block
	FGCGCombatResult ValidationResult = CanBlock(BlockerInstance, Attack, DefendingPlayer);
	if (!ValidationResult.bSuccess)
	{
		return ValidationResult;
	}

	// Rule 13-1-6-1: High-Maneuver prevents Blocker activation
	// Find attacker to check for High-Maneuver keyword
	AGCGPlayerState* AttackingPlayer = GameState->GetPlayerByID(Attack.AttackingPlayerID);
	if (AttackingPlayer)
	{
		FGCGCardInstance AttackerInstance;
		EGCGCardZone AttackerZone;
		if (AttackingPlayer->FindCardByInstanceID(Attack.AttackerInstanceID, AttackerInstance, AttackerZone))
		{
			// Check if attacker has High-Maneuver
			if (HasKeyword(AttackerInstance, EGCGKeyword::HighManeuver))
			{
				return FGCGCombatResult(false, TEXT("Cannot block - attacker has High-Maneuver"));
			}
		}
	}

	// Assign blocker
	Attack.BlockerInstanceID = BlockerInstanceID;
	Attack.bTargetingBase = false; // Attack is now blocked

	// Rest the blocker (blocking rests the unit)
	for (FGCGCardInstance& BattleCard : DefendingPlayer->BattleArea)
	{
		if (BattleCard.InstanceID == BlockerInstanceID)
		{
			BattleCard.bIsActive = false;
			break;
		}
	}

	UE_LOG(LogTemp, Log, TEXT("UGCGCombatSubsystem::DeclareBlocker - Player %d declared blocker %s (ID: %d) for attack index %d"),
		DefendingPlayer->GetPlayerID(), *BlockerInstance.CardName.ToString(), BlockerInstanceID, AttackIndex);

	// TODO: Trigger "On Block" effects (Phase 8)

	return FGCGCombatResult(true);
}

FGCGCombatResult UGCGCombatSubsystem::CanBlock(const FGCGCardInstance& BlockerInstance,
	const FGCGAttackDeclaration& Attack, AGCGPlayerState* DefendingPlayer) const
{
	if (!DefendingPlayer)
	{
		return FGCGCombatResult(false, TEXT("Invalid player state"));
	}

	// Must be a Unit
	if (BlockerInstance.CardType != EGCGCardType::Unit)
	{
		return FGCGCombatResult(false, TEXT("Only Units can block"));
	}

	// Must be active (not rested) OR have Blocker keyword
	bool bHasBlockerKeyword = HasKeyword(BlockerInstance, EGCGKeyword::Blocker);
	if (!BlockerInstance.bIsActive && !bHasBlockerKeyword)
	{
		return FGCGCombatResult(false, TEXT("Unit is rested and does not have Blocker keyword"));
	}

	// Rule 13-1-6-1: High-Maneuver prevents Blocker activation
	// Need to check if attacker has High-Maneuver (requires attacker instance)
	// NOTE: This validation needs attacker instance, which requires GameState access
	// Validation should be done in DeclareBlocker where we have full context
	// (Moved to DeclareBlocker method - see line ~195)

	return FGCGCombatResult(true);
}

// ===== DAMAGE CALCULATION =====

FGCGCombatResult UGCGCombatSubsystem::ResolveAttack(FGCGAttackDeclaration& Attack,
	AGCGPlayerState* AttackingPlayer, AGCGPlayerState* DefendingPlayer, AGCGGameState* GameState)
{
	if (!AttackingPlayer || !DefendingPlayer || !GameState)
	{
		return FGCGCombatResult(false, TEXT("Invalid player or game state"));
	}

	FGCGCombatResult Result(true);

	// Find attacker
	FGCGCardInstance AttackerInstance;
	EGCGCardZone AttackerZone;
	if (!AttackingPlayer->FindCardByInstanceID(Attack.AttackerInstanceID, AttackerInstance, AttackerZone))
	{
		return FGCGCombatResult(false, TEXT("Attacker not found"));
	}

	// Get Keyword Subsystem for keyword processing
	UGCGKeywordSubsystem* KeywordSubsystem = GetGameInstance()->GetSubsystem<UGCGKeywordSubsystem>();

	// Calculate Support buffs (Phase 7)
	int32 AttackerSupportBuff = 0;
	if (KeywordSubsystem)
	{
		AttackerSupportBuff = KeywordSubsystem->CalculateSupportBuff(AttackerInstance, AttackingPlayer);
	}

	int32 AttackerAP = AttackerInstance.AP + AttackerSupportBuff;

	// Check if attack is blocked
	if (Attack.BlockerInstanceID > 0)
	{
		// Find blocker
		FGCGCardInstance BlockerInstance;
		EGCGCardZone BlockerZone;
		if (!DefendingPlayer->FindCardByInstanceID(Attack.BlockerInstanceID, BlockerInstance, BlockerZone))
		{
			return FGCGCombatResult(false, TEXT("Blocker not found"));
		}

		// Calculate blocker's Support buff (Phase 7)
		int32 BlockerSupportBuff = 0;
		if (KeywordSubsystem)
		{
			BlockerSupportBuff = KeywordSubsystem->CalculateSupportBuff(BlockerInstance, DefendingPlayer);
		}

		int32 BlockerAP = BlockerInstance.AP + BlockerSupportBuff;

		// Check for First Strike keyword (Phase 7)
		bool bFirstStrikeResolved = false;
		bool bBlockerDestroyedByFirstStrike = false;

		if (KeywordSubsystem && KeywordSubsystem->HasFirstStrikeAdvantage(AttackerInstance, BlockerInstance))
		{
			// Process First Strike
			FGCGKeywordResult FirstStrikeResult = KeywordSubsystem->ProcessFirstStrike(AttackerInstance, BlockerInstance, bBlockerDestroyedByFirstStrike);
			bFirstStrikeResolved = FirstStrikeResult.bSuccess;

			if (bBlockerDestroyedByFirstStrike)
			{
				// Blocker destroyed by First Strike - no retaliation
				bool bBlockerDestroyed = DealDamageToUnit(Attack.BlockerInstanceID, AttackerAP, DefendingPlayer);
				Result.bBlockerDestroyed = bBlockerDestroyed;

				UE_LOG(LogTemp, Log, TEXT("UGCGCombatSubsystem::ResolveAttack - First Strike destroyed blocker (no retaliation)"));

				// Check for Breach keyword (Phase 7)
				if (KeywordSubsystem->HasKeyword(AttackerInstance, EGCGKeyword::Breach))
				{
					FGCGKeywordResult BreachResult = KeywordSubsystem->ProcessBreach(AttackerInstance, DefendingPlayer, GameState);
					Result.ShieldsBroken += BreachResult.ShieldsBroken;
				}

				return Result;
			}
		}

		// Normal combat (both deal damage)
		if (!bFirstStrikeResolved)
		{
			bool bAttackerDestroyed = DealDamageToUnit(Attack.AttackerInstanceID, BlockerAP, AttackingPlayer);
			bool bBlockerDestroyed = DealDamageToUnit(Attack.BlockerInstanceID, AttackerAP, DefendingPlayer);

			Result.bAttackerDestroyed = bAttackerDestroyed;
			Result.bBlockerDestroyed = bBlockerDestroyed;

			UE_LOG(LogTemp, Log, TEXT("UGCGCombatSubsystem::ResolveAttack - Blocked combat resolved (Attacker destroyed: %d, Blocker destroyed: %d)"),
				bAttackerDestroyed ? 1 : 0, bBlockerDestroyed ? 1 : 0);

			// Check for Breach keyword if blocker was destroyed (Phase 7)
			if (bBlockerDestroyed && KeywordSubsystem && KeywordSubsystem->HasKeyword(AttackerInstance, EGCGKeyword::Breach))
			{
				FGCGKeywordResult BreachResult = KeywordSubsystem->ProcessBreach(AttackerInstance, DefendingPlayer, GameState);
				Result.ShieldsBroken += BreachResult.ShieldsBroken;
			}
		}
	}
	else
	{
		// Unblocked attack - deal damage to player
		int32 ShieldsBroken = 0;
		bool bPlayerLost = false;

		// Check for Suppression keyword (Phase 7)
		if (KeywordSubsystem && KeywordSubsystem->HasKeyword(AttackerInstance, EGCGKeyword::Suppression))
		{
			// Suppression: Destroy all shields simultaneously
			FGCGKeywordResult SuppressionResult = KeywordSubsystem->ProcessSuppression(AttackerInstance, DefendingPlayer, GameState);
			ShieldsBroken = SuppressionResult.ShieldsBroken;
			Result.DamageDealt = SuppressionResult.DamageDealt;

			// Check if player lost
			bPlayerLost = DefendingPlayer->bHasLost;

			UE_LOG(LogTemp, Log, TEXT("UGCGCombatSubsystem::ResolveAttack - Suppression destroyed %d shields"),
				ShieldsBroken);
		}
		else
		{
			// Normal player damage (one shield at a time)
			bPlayerLost = DealDamageToPlayer(AttackerAP, DefendingPlayer, GameState, ShieldsBroken);
			Result.DamageDealt = AttackerAP;
		}

		Result.ShieldsBroken = ShieldsBroken;

		UE_LOG(LogTemp, Log, TEXT("UGCGCombatSubsystem::ResolveAttack - Unblocked attack dealt %d damage (Shields broken: %d, Player lost: %d)"),
			AttackerAP, ShieldsBroken, bPlayerLost ? 1 : 0);

		// Check if player lost
		if (bPlayerLost)
		{
			// Game over - attacker wins
			// This will be handled by the GameMode
		}
	}

	// Mark attack as resolved
	Attack.bResolved = true;

	return Result;
}

bool UGCGCombatSubsystem::DealDamageToUnit(int32 TargetInstanceID, int32 Damage, AGCGPlayerState* PlayerState)
{
	if (!PlayerState || Damage <= 0)
	{
		return false;
	}

	// Find unit in Battle Area
	for (FGCGCardInstance& BattleCard : PlayerState->BattleArea)
	{
		if (BattleCard.InstanceID == TargetInstanceID)
		{
			// Add damage
			BattleCard.CurrentDamage += Damage;

			// FAQ Q97-99: Track damage source (battle damage vs effect damage)
			BattleCard.LastDamageSource = EGCGDamageSource::BattleDamage;

			UE_LOG(LogTemp, Log, TEXT("UGCGCombatSubsystem::DealDamageToUnit - Dealt %d damage to %s (Total: %d/%d HP)"),
				Damage, *BattleCard.CardName.ToString(), BattleCard.CurrentDamage, BattleCard.HP);

			// Check if unit is destroyed
			if (BattleCard.CurrentDamage >= BattleCard.HP)
			{
				UE_LOG(LogTemp, Log, TEXT("UGCGCombatSubsystem::DealDamageToUnit - %s destroyed"),
					*BattleCard.CardName.ToString());
				return DestroyUnit(TargetInstanceID, PlayerState);
			}

			return false; // Unit survived
		}
	}

	return false;
}

bool UGCGCombatSubsystem::DealDamageToPlayer(int32 Damage, AGCGPlayerState* DefendingPlayer,
	AGCGGameState* GameState, int32& OutShieldsBroken)
{
	if (!DefendingPlayer || !GameState || Damage <= 0)
	{
		OutShieldsBroken = 0;
		return false;
	}

	OutShieldsBroken = 0;

	// TODO: Check for Breach keyword to break extra shields (Phase 7)
	int32 ShieldsToBreak = 1; // Base 1 shield per damage instance

	// Break shields first
	if (HasShields(DefendingPlayer))
	{
		OutShieldsBroken = BreakShields(ShieldsToBreak, DefendingPlayer);

		UE_LOG(LogTemp, Log, TEXT("UGCGCombatSubsystem::DealDamageToPlayer - Player %d shields broken: %d (Remaining: %d)"),
			DefendingPlayer->GetPlayerID(), OutShieldsBroken, DefendingPlayer->GetShieldCount());

		// TODO: Check for Burst keyword on broken shields (Phase 7)

		// Damage was absorbed by shields
		return false;
	}

	// Comprehensive Rules 1-2-2-1: Battle damage with no shields = defeat
	// "When either player receives battle damage from a Unit while they have no cards
	// in their shield area, that player fulfills the conditions for defeat."
	UE_LOG(LogTemp, Warning, TEXT("UGCGCombatSubsystem::DealDamageToPlayer - Player %d took battle damage with NO SHIELDS - DEFEAT"),
		DefendingPlayer->GetPlayerID());

	// Mark player as having met defeat conditions
	// (Actual loss will be processed during next Rules Management)
	DefendingPlayer->bHasLost = true;

	// Still apply damage to Base for tracking purposes
	if (DefendingPlayer->BaseSection.Num() > 0)
	{
		FGCGCardInstance& Base = DefendingPlayer->BaseSection[0];
		Base.CurrentDamage += Damage;

		// FAQ Q97-99: Track damage source (battle damage from combat)
		Base.LastDamageSource = EGCGDamageSource::BattleDamage;

		UE_LOG(LogTemp, Warning, TEXT("UGCGCombatSubsystem::DealDamageToPlayer - Player %d Base took %d damage (Total: %d/%d HP)"),
			DefendingPlayer->GetPlayerID(), Damage, Base.CurrentDamage, Base.HP);
	}

	return true; // Player lost (will be processed by Rules Management)
}

// ===== SHIELD SYSTEM =====

int32 UGCGCombatSubsystem::BreakShields(int32 Count, AGCGPlayerState* DefendingPlayer)
{
	if (!DefendingPlayer || Count <= 0)
	{
		return 0;
	}

	UGCGZoneSubsystem* ZoneSubsystem = GetZoneSubsystem();
	if (!ZoneSubsystem)
	{
		return 0;
	}

	int32 ShieldsBroken = 0;
	int32 ShieldsToBreak = FMath::Min(Count, DefendingPlayer->GetShieldCount());

	// Get card database for checking Burst keyword
	UGCGCardDatabase* CardDatabase = GetGameInstance()->GetSubsystem<UGCGCardDatabase>();

	for (int32 i = 0; i < ShieldsToBreak; ++i)
	{
		if (DefendingPlayer->ShieldStack.Num() > 0)
		{
			// Take top shield (index 0)
			FGCGCardInstance ShieldCard = DefendingPlayer->ShieldStack[0];
			DefendingPlayer->ShieldStack.RemoveAt(0);

			// Rule 5-10-3: Reveal shield before placing in trash
			UE_LOG(LogTemp, Log, TEXT("UGCGCombatSubsystem::BreakShields - Shield revealed: %s (ID: %d)"),
				*ShieldCard.CardName.ToString(), ShieldCard.InstanceID);

			// Rule 5-10-3: Check for Burst keyword
			bool bHasBurst = false;
			if (CardDatabase)
			{
				const FGCGCardData* CardData = CardDatabase->GetCardData(ShieldCard.CardNumber);
				if (CardData && CardData->Keywords.Contains(EGCGKeyword::Burst))
				{
					bHasBurst = true;

					// Rule 5-10-3: Player decides whether to activate Burst
					// TODO: Implement UI prompt for Burst activation (requires Effect System)
					// For now, we log that Burst is available
					UE_LOG(LogTemp, Warning, TEXT("UGCGCombatSubsystem::BreakShields - Shield has 【Burst】! Player %d should be prompted to activate (not yet implemented)"),
						DefendingPlayer->GetPlayerID());

					// TODO: If player chooses to activate:
					// 1. Trigger Burst effect (UGCGEffectSubsystem::TriggerBurstEffect)
					// 2. Wait for effect resolution
					// 3. Then move to trash

					// TEMPORARY: Auto-log for testing
					// In full implementation, this would pause and wait for player choice
				}
			}

			// Move shield to trash
			ShieldCard.CurrentZone = EGCGCardZone::Trash;
			DefendingPlayer->Trash.Add(ShieldCard);

			ShieldsBroken++;

			if (bHasBurst)
			{
				UE_LOG(LogTemp, Log, TEXT("UGCGCombatSubsystem::BreakShields - Shield with Burst moved to trash: %s"),
					*ShieldCard.CardName.ToString());
			}
		}
	}

	return ShieldsBroken;
}

bool UGCGCombatSubsystem::HasShields(AGCGPlayerState* PlayerState) const
{
	return PlayerState && PlayerState->GetShieldCount() > 0;
}

// ===== COMBAT RESOLUTION =====

FGCGCombatResult UGCGCombatSubsystem::ResolveAllAttacks(AGCGPlayerState* AttackingPlayer,
	AGCGPlayerState* DefendingPlayer, AGCGGameState* GameState)
{
	if (!AttackingPlayer || !DefendingPlayer || !GameState)
	{
		return FGCGCombatResult(false, TEXT("Invalid player or game state"));
	}

	FGCGCombatResult TotalResult(true);

	// Resolve each attack
	for (FGCGAttackDeclaration& Attack : GameState->CurrentAttacks)
	{
		if (!Attack.bResolved)
		{
			FGCGCombatResult AttackResult = ResolveAttack(Attack, AttackingPlayer, DefendingPlayer, GameState);

			// Accumulate results
			TotalResult.DamageDealt += AttackResult.DamageDealt;
			TotalResult.ShieldsBroken += AttackResult.ShieldsBroken;
			TotalResult.bAttackerDestroyed = TotalResult.bAttackerDestroyed || AttackResult.bAttackerDestroyed;
			TotalResult.bBlockerDestroyed = TotalResult.bBlockerDestroyed || AttackResult.bBlockerDestroyed;
		}
	}

	UE_LOG(LogTemp, Log, TEXT("UGCGCombatSubsystem::ResolveAllAttacks - Resolved %d attacks (Total damage: %d, Shields broken: %d)"),
		GameState->CurrentAttacks.Num(), TotalResult.DamageDealt, TotalResult.ShieldsBroken);

	return TotalResult;
}

void UGCGCombatSubsystem::ClearAttacks(AGCGGameState* GameState)
{
	if (!GameState)
	{
		return;
	}

	int32 AttackCount = GameState->CurrentAttacks.Num();
	GameState->CurrentAttacks.Empty();
	GameState->bAttackInProgress = false;

	UE_LOG(LogTemp, Log, TEXT("UGCGCombatSubsystem::ClearAttacks - Cleared %d attacks"), AttackCount);
}

// ===== INTERNAL HELPERS =====

bool UGCGCombatSubsystem::HasSummoningSickness(const FGCGCardInstance& CardInstance, AGCGGameState* GameState) const
{
	if (!GameState)
	{
		return false;
	}

	// Check if deployed this turn
	bool bDeployedThisTurn = (CardInstance.TurnDeployed == GameState->TurnNumber);

	if (!bDeployedThisTurn)
	{
		// Not deployed this turn - no summoning sickness
		return false;
	}

	// Check for Link Unit keyword (Phase 9)
	// Link Units can attack on deployment turn if paired with a Pilot
	UGCGLinkUnitSubsystem* LinkUnitSubsystem = GetGameInstance()->GetSubsystem<UGCGLinkUnitSubsystem>();
	if (LinkUnitSubsystem && LinkUnitSubsystem->CanLinkUnitAttackThisTurn(CardInstance, GameState->TurnNumber))
	{
		// Link Unit is paired - no summoning sickness
		return false;
	}

	// Has summoning sickness
	return true;
}

bool UGCGCombatSubsystem::HasKeyword(const FGCGCardInstance& CardInstance, EGCGKeyword Keyword) const
{
	for (const FGCGKeywordInstance& KeywordInstance : CardInstance.Keywords)
	{
		if (KeywordInstance.Keyword == Keyword)
		{
			return true;
		}
	}
	return false;
}

int32 UGCGCombatSubsystem::GetKeywordValue(const FGCGCardInstance& CardInstance, EGCGKeyword Keyword) const
{
	for (const FGCGKeywordInstance& KeywordInstance : CardInstance.Keywords)
	{
		if (KeywordInstance.Keyword == Keyword)
		{
			return KeywordInstance.Value;
		}
	}
	return 0;
}

bool UGCGCombatSubsystem::DestroyUnit(int32 TargetInstanceID, AGCGPlayerState* PlayerState)
{
	if (!PlayerState)
	{
		return false;
	}

	UGCGZoneSubsystem* ZoneSubsystem = GetZoneSubsystem();
	if (!ZoneSubsystem)
	{
		return false;
	}

	// Find unit in Battle Area
	FGCGCardInstance UnitInstance;
	EGCGCardZone UnitZone;
	if (!PlayerState->FindCardByInstanceID(TargetInstanceID, UnitInstance, UnitZone))
	{
		return false;
	}

	if (UnitZone != EGCGCardZone::BattleArea)
	{
		return false;
	}

	// Move to trash
	if (ZoneSubsystem->MoveCard(UnitInstance, EGCGCardZone::BattleArea, EGCGCardZone::Trash,
		PlayerState, nullptr, false))
	{
		UE_LOG(LogTemp, Log, TEXT("UGCGCombatSubsystem::DestroyUnit - %s destroyed and moved to trash"),
			*UnitInstance.CardName.ToString());

		// TODO: Trigger "On Destroy" effects (Phase 8)

		return true;
	}

	return false;
}

UGCGZoneSubsystem* UGCGCombatSubsystem::GetZoneSubsystem() const
{
	return GetGameInstance()->GetSubsystem<UGCGZoneSubsystem>();
}
