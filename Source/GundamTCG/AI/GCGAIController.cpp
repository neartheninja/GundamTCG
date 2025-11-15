// GCGAIController.cpp - AI Opponent Controller Implementation
// Unreal Engine 5.6 - Gundam TCG Implementation
// Provides AI decision-making for single-player and testing

#include "GCGAIController.h"
#include "GundamTCG/PlayerState/GCGPlayerState.h"
#include "GundamTCG/GameState/GCGGameState.h"
#include "GundamTCG/Subsystems/GCGCardDatabase.h"
#include "GundamTCG/Subsystems/GCGZoneSubsystem.h"
#include "GundamTCG/Subsystems/GCGCombatSubsystem.h"
#include "GundamTCG/Subsystems/GCGLinkUnitSubsystem.h"
#include "GundamTCG/GameModes/GCGGameMode_1v1.h"
#include "Kismet/GameplayStatics.h"

AGCGAIController::AGCGAIController()
{
	PrimaryActorTick.bCanEverTick = true;
	bReplicates = true;
}

// ===========================================================================================
// INITIALIZATION
// ===========================================================================================

void AGCGAIController::BeginPlay()
{
	Super::BeginPlay();

	// Cache references
	AIPlayerState = Cast<AGCGPlayerState>(PlayerState);
	GameState = Cast<AGCGGameState>(UGameplayStatics::GetGameState(this));

	if (UGameInstance* GameInstance = GetGameInstance())
	{
		CardDatabase = GameInstance->GetSubsystem<UGCGCardDatabase>();
	}

	if (bDebugLogging)
	{
		UE_LOG(LogTemp, Log, TEXT("AI Controller initialized for Player %d with difficulty: %d"),
			AIPlayerState ? AIPlayerState->PlayerID : -1, static_cast<int32>(Difficulty));
	}
}

void AGCGAIController::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

	// Handle thinking delay
	if (bIsThinking && ThinkingTimer > 0.0f)
	{
		ThinkingTimer -= DeltaTime;

		if (ThinkingTimer <= 0.0f)
		{
			bIsThinking = false;
			ExecuteAction(PendingAction);
		}
	}
}

void AGCGAIController::SetDifficulty(EGCGAIDifficulty NewDifficulty)
{
	Difficulty = NewDifficulty;

	if (bDebugLogging)
	{
		UE_LOG(LogTemp, Log, TEXT("AI difficulty set to: %d"), static_cast<int32>(Difficulty));
	}
}

void AGCGAIController::SetThinkingDelay(bool bEnabled, float MinDelay, float MaxDelay)
{
	bUseThinkingDelay = bEnabled;
	MinThinkingDelay = MinDelay;
	MaxThinkingDelay = MaxDelay;
}

// ===========================================================================================
// DECISION MAKING
// ===========================================================================================

FGCGAIAction AGCGAIController::DecideAction()
{
	if (!AIPlayerState || !GameState)
	{
		return FGCGAIAction(EGCGAIActionType::PassPriority);
	}

	LogAIThinking(TEXT("AI deciding action..."));

	// Random difficulty: make random legal move
	if (Difficulty == EGCGAIDifficulty::Random)
	{
		return MakeRandomAction();
	}

	// Otherwise, use heuristic-based decision making
	FGCGAIAction BestAction(EGCGAIActionType::PassPriority, -1, 0.0f, TEXT("Default pass"));

	// Phase-specific decisions
	switch (GameState->CurrentPhase)
	{
		case EGCGTurnPhase::ResourcePhase:
		{
			FGCGAIAction ResourceAction = DecidePlaceResource();
			if (ResourceAction.Priority > BestAction.Priority)
			{
				BestAction = ResourceAction;
			}
			break;
		}

		case EGCGTurnPhase::MainPhase:
		{
			FGCGAIAction PlayAction = DecideCardToPlay();
			if (PlayAction.Priority > BestAction.Priority)
			{
				BestAction = PlayAction;
			}
			break;
		}

		case EGCGTurnPhase::AttackPhase:
		{
			FGCGAIAction AttackAction = DecideAttack();
			if (AttackAction.Priority > BestAction.Priority)
			{
				BestAction = AttackAction;
			}
			break;
		}

		default:
			break;
	}

	LogAIThinking(FString::Printf(TEXT("AI decided: %s (Priority: %.2f) - %s"),
		*UEnum::GetValueAsString(BestAction.ActionType), BestAction.Priority, *BestAction.Reason));

	return BestAction;
}

bool AGCGAIController::ExecuteAction(const FGCGAIAction& Action)
{
	if (!AIPlayerState || !GameState)
	{
		return false;
	}

	AGCGGameMode_1v1* GameMode = Cast<AGCGGameMode_1v1>(UGameplayStatics::GetGameMode(this));
	if (!GameMode)
	{
		return false;
	}

	LogAIThinking(FString::Printf(TEXT("Executing action: %s"), *UEnum::GetValueAsString(Action.ActionType)));

	switch (Action.ActionType)
	{
		case EGCGAIActionType::PlayCard:
			return GameMode->RequestPlayCard(AIPlayerState->PlayerID, Action.CardInstanceID);

		case EGCGAIActionType::PlaceResource:
			return GameMode->RequestPlaceResource(AIPlayerState->PlayerID, Action.CardInstanceID);

		case EGCGAIActionType::Attack:
			return GameMode->RequestDeclareAttack(AIPlayerState->PlayerID, Action.CardInstanceID, Action.TargetPlayerID);

		case EGCGAIActionType::Block:
			// Block actions need attack index stored in TargetInstanceID
			return GameMode->RequestDeclareBlocker(AIPlayerState->PlayerID, Action.TargetInstanceID, Action.CardInstanceID);

		case EGCGAIActionType::EndTurn:
			GameMode->RequestEndTurn(AIPlayerState->PlayerID);
			return true;

		case EGCGAIActionType::PassPriority:
		default:
			// Pass priority - do nothing
			return true;
	}
}

// ===========================================================================================
// PHASE-SPECIFIC DECISIONS
// ===========================================================================================

FGCGAIAction AGCGAIController::DecideCardToPlay()
{
	TArray<FGCGCardInstance> PlayableCards = GetPlayableCards();

	if (PlayableCards.Num() == 0)
	{
		return FGCGAIAction(EGCGAIActionType::PassPriority, -1, 0.0f, TEXT("No playable cards"));
	}

	// Evaluate each playable card
	FGCGAIAction BestAction(EGCGAIActionType::PassPriority);
	float BestScore = -1000.0f;

	for (const FGCGCardInstance& Card : PlayableCards)
	{
		float Score = EvaluateCardPlay(Card);

		// Difficulty modifiers
		if (Difficulty == EGCGAIDifficulty::Easy)
		{
			// Easy AI: Add random noise, sometimes makes mistakes
			Score += FMath::RandRange(-20.0f, 10.0f);
		}
		else if (Difficulty == EGCGAIDifficulty::Medium)
		{
			// Medium AI: Small random noise
			Score += FMath::RandRange(-5.0f, 5.0f);
		}

		if (Score > BestScore)
		{
			BestScore = Score;
			BestAction = FGCGAIAction(
				EGCGAIActionType::PlayCard,
				Card.InstanceID,
				Score,
				FString::Printf(TEXT("Play %s (Score: %.1f)"), *Card.CardName.ToString(), Score)
			);
		}
	}

	// Check if we should pass instead
	if (ShouldPassPriority() || BestScore < 10.0f)
	{
		return FGCGAIAction(EGCGAIActionType::PassPriority, -1, 0.0f, TEXT("Decided to pass"));
	}

	return BestAction;
}

FGCGAIAction AGCGAIController::DecidePlaceResource()
{
	if (!AIPlayerState)
	{
		return FGCGAIAction(EGCGAIActionType::PassPriority);
	}

	// Check if we already placed resource this turn (tracked in player state)
	if (AIPlayerState->bPlacedResourceThisTurn)
	{
		return FGCGAIAction(EGCGAIActionType::PassPriority, -1, 0.0f, TEXT("Already placed resource"));
	}

	// Check if we have cards in hand to place as resource
	if (AIPlayerState->Hand.Num() == 0)
	{
		return FGCGAIAction(EGCGAIActionType::PassPriority, -1, 0.0f, TEXT("No cards in hand"));
	}

	// Find lowest value card to place as resource
	FGCGCardInstance BestCardToPlace;
	float LowestValue = 1000.0f;
	bool bFoundCard = false;

	for (const FGCGCardInstance& Card : AIPlayerState->Hand)
	{
		float CardValue = GetCardValue(Card);

		if (CardValue < LowestValue)
		{
			LowestValue = CardValue;
			BestCardToPlace = Card;
			bFoundCard = true;
		}
	}

	if (!bFoundCard)
	{
		return FGCGAIAction(EGCGAIActionType::PassPriority);
	}

	// Decide whether to place resource or keep card
	// Early game: always place resource
	// Late game: only if we need resources
	int32 ResourceCount = AIPlayerState->ResourceArea.Num();
	int32 TurnNumber = GameState->TurnNumber;

	bool bShouldPlace = false;

	if (TurnNumber <= 5)
	{
		// Early game: always place
		bShouldPlace = true;
	}
	else if (ResourceCount < 10)
	{
		// Mid game: place if we don't have many resources
		bShouldPlace = true;
	}
	else
	{
		// Late game: only place very low value cards
		bShouldPlace = (LowestValue < 20.0f);
	}

	if (bShouldPlace)
	{
		return FGCGAIAction(
			EGCGAIActionType::PlaceResource,
			BestCardToPlace.InstanceID,
			50.0f,
			FString::Printf(TEXT("Place %s as resource (Value: %.1f)"), *BestCardToPlace.CardName.ToString(), LowestValue)
		);
	}

	return FGCGAIAction(EGCGAIActionType::PassPriority, -1, 0.0f, TEXT("Keep hand for plays"));
}

FGCGAIAction AGCGAIController::DecideAttack()
{
	TArray<FGCGCardInstance> AttackableUnits = GetAttackableUnits();

	if (AttackableUnits.Num() == 0)
	{
		return FGCGAIAction(EGCGAIActionType::PassPriority, -1, 0.0f, TEXT("No attackable units"));
	}

	// Evaluate each potential attack
	FGCGAIAction BestAction(EGCGAIActionType::PassPriority);
	float BestScore = -1000.0f;

	// In 1v1, target is always the opponent
	int32 OpponentID = (AIPlayerState->PlayerID == 0) ? 1 : 0;

	for (const FGCGCardInstance& Attacker : AttackableUnits)
	{
		float Score = EvaluateAttack(Attacker, OpponentID);

		// Difficulty modifiers
		if (Difficulty == EGCGAIDifficulty::Easy)
		{
			// Easy AI: Random attacks, doesn't evaluate well
			Score = FMath::RandRange(0.0f, 50.0f);
		}
		else if (Difficulty == EGCGAIDifficulty::Medium)
		{
			// Medium AI: Small random noise
			Score += FMath::RandRange(-10.0f, 10.0f);
		}

		if (Score > BestScore)
		{
			BestScore = Score;
			BestAction = FGCGAIAction(
				EGCGAIActionType::Attack,
				Attacker.InstanceID,
				Score,
				FString::Printf(TEXT("Attack with %s (Score: %.1f)"), *Attacker.CardName.ToString(), Score)
			);
			BestAction.TargetPlayerID = OpponentID;
		}
	}

	// Check if we should attack
	if (BestScore > 20.0f)
	{
		return BestAction;
	}

	return FGCGAIAction(EGCGAIActionType::PassPriority, -1, 0.0f, TEXT("No good attacks"));
}

FGCGAIAction AGCGAIController::DecideBlock(int32 AttackIndex)
{
	if (!GameState || AttackIndex >= GameState->PendingAttacks.Num())
	{
		return FGCGAIAction(EGCGAIActionType::PassPriority);
	}

	const FGCGAttackInfo& Attack = GameState->PendingAttacks[AttackIndex];
	TArray<FGCGCardInstance> BlockerUnits = GetBlockerUnits();

	if (BlockerUnits.Num() == 0)
	{
		return FGCGAIAction(EGCGAIActionType::PassPriority, -1, 0.0f, TEXT("No blockers available"));
	}

	// Get attacker info
	UGCGZoneSubsystem* ZoneSubsystem = GetGameInstance()->GetSubsystem<UGCGZoneSubsystem>();
	if (!ZoneSubsystem)
	{
		return FGCGAIAction(EGCGAIActionType::PassPriority);
	}

	AGCGPlayerState* AttackerPlayerState = nullptr;
	for (APlayerState* PS : GameState->PlayerArray)
	{
		AGCGPlayerState* GCGPS = Cast<AGCGPlayerState>(PS);
		if (GCGPS && GCGPS->PlayerID == Attack.AttackingPlayerID)
		{
			AttackerPlayerState = GCGPS;
			break;
		}
	}

	if (!AttackerPlayerState)
	{
		return FGCGAIAction(EGCGAIActionType::PassPriority);
	}

	FGCGCardInstance* AttackerInstance = ZoneSubsystem->FindCardByInstanceID(AttackerPlayerState, Attack.AttackerInstanceID);
	if (!AttackerInstance)
	{
		return FGCGAIAction(EGCGAIActionType::PassPriority);
	}

	// Evaluate each potential blocker
	FGCGAIAction BestAction(EGCGAIActionType::PassPriority);
	float BestScore = -1000.0f;

	for (const FGCGCardInstance& Blocker : BlockerUnits)
	{
		float Score = EvaluateBlock(Blocker, *AttackerInstance);

		if (Difficulty == EGCGAIDifficulty::Easy)
		{
			// Easy AI: Random blocking decisions
			Score = FMath::RandRange(-20.0f, 40.0f);
		}
		else if (Difficulty == EGCGAIDifficulty::Medium)
		{
			// Medium AI: Small random noise
			Score += FMath::RandRange(-5.0f, 5.0f);
		}

		if (Score > BestScore)
		{
			BestScore = Score;
			BestAction = FGCGAIAction(
				EGCGAIActionType::Block,
				Blocker.InstanceID,
				Score,
				FString::Printf(TEXT("Block with %s (Score: %.1f)"), *Blocker.CardName.ToString(), Score)
			);
			BestAction.TargetInstanceID = AttackIndex; // Store attack index
		}
	}

	// Check if we should block
	// Hard AI: Always blocks if favorable
	// Medium/Easy: Sometimes doesn't block even when favorable
	float BlockThreshold = 20.0f;
	if (Difficulty == EGCGAIDifficulty::Easy)
	{
		BlockThreshold = 40.0f; // Easy AI rarely blocks
	}
	else if (Difficulty == EGCGAIDifficulty::Medium)
	{
		BlockThreshold = 30.0f;
	}

	if (BestScore > BlockThreshold)
	{
		return BestAction;
	}

	return FGCGAIAction(EGCGAIActionType::PassPriority, -1, 0.0f, TEXT("Let attack through"));
}

TArray<int32> AGCGAIController::DecideDiscard(int32 DiscardCount)
{
	TArray<int32> CardsToDiscard;

	if (!AIPlayerState || AIPlayerState->Hand.Num() == 0)
	{
		return CardsToDiscard;
	}

	// Create array of cards with their values
	TArray<TPair<int32, float>> CardValues; // InstanceID, Value
	for (const FGCGCardInstance& Card : AIPlayerState->Hand)
	{
		float Value = GetCardValue(Card);
		CardValues.Add(TPair<int32, float>(Card.InstanceID, Value));
	}

	// Sort by value (ascending)
	CardValues.Sort([](const TPair<int32, float>& A, const TPair<int32, float>& B) {
		return A.Value < B.Value;
	});

	// Discard lowest value cards
	for (int32 i = 0; i < FMath::Min(DiscardCount, CardValues.Num()); i++)
	{
		CardsToDiscard.Add(CardValues[i].Key);
	}

	return CardsToDiscard;
}

// ===========================================================================================
// GAME STATE EVALUATION
// ===========================================================================================

FGCGAIGameEvaluation AGCGAIController::EvaluateGameState()
{
	FGCGAIGameEvaluation Eval;

	if (!AIPlayerState || !GameState)
	{
		return Eval;
	}

	// Get opponent
	AGCGPlayerState* OpponentState = nullptr;
	for (APlayerState* PS : GameState->PlayerArray)
	{
		AGCGPlayerState* GCGPS = Cast<AGCGPlayerState>(PS);
		if (GCGPS && GCGPS->PlayerID != AIPlayerState->PlayerID)
		{
			OpponentState = GCGPS;
			break;
		}
	}

	if (!OpponentState)
	{
		return Eval;
	}

	// Board Control (Units on field)
	int32 OurUnits = AIPlayerState->BattleArea.Num();
	int32 TheirUnits = OpponentState->BattleArea.Num();
	Eval.BoardControl = 50.0f + (OurUnits - TheirUnits) * 10.0f;
	Eval.BoardControl = FMath::Clamp(Eval.BoardControl, 0.0f, 100.0f);

	// Resource Advantage
	int32 OurResources = AIPlayerState->ResourceArea.Num();
	int32 TheirResources = OpponentState->ResourceArea.Num();
	Eval.ResourceAdvantage = 50.0f + (OurResources - TheirResources) * 5.0f;
	Eval.ResourceAdvantage = FMath::Clamp(Eval.ResourceAdvantage, 0.0f, 100.0f);

	// Card Advantage (hand + deck)
	int32 OurCards = AIPlayerState->Hand.Num() + AIPlayerState->Deck.Num();
	int32 TheirCards = OpponentState->Hand.Num() + OpponentState->Deck.Num();
	Eval.CardAdvantage = 50.0f + (OurCards - TheirCards) * 2.0f;
	Eval.CardAdvantage = FMath::Clamp(Eval.CardAdvantage, 0.0f, 100.0f);

	// Tempo Advantage (total AP on board)
	int32 OurAP = 0;
	for (const FGCGCardInstance& Unit : AIPlayerState->BattleArea)
	{
		OurAP += Unit.AP;
	}

	int32 TheirAP = 0;
	for (const FGCGCardInstance& Unit : OpponentState->BattleArea)
	{
		TheirAP += Unit.AP;
	}

	Eval.TempoAdvantage = 50.0f + (OurAP - TheirAP) * 3.0f;
	Eval.TempoAdvantage = FMath::Clamp(Eval.TempoAdvantage, 0.0f, 100.0f);

	// Threat Level (opponent's board strength)
	Eval.ThreatLevel = FMath::Clamp(TheirAP * 2.0f, 0.0f, 100.0f);

	// Overall Advantage Score
	Eval.AdvantageScore = (Eval.BoardControl - 50.0f) +
	                      (Eval.ResourceAdvantage - 50.0f) * 0.5f +
	                      (Eval.CardAdvantage - 50.0f) * 0.5f +
	                      (Eval.TempoAdvantage - 50.0f) * 1.5f;

	return Eval;
}

float AGCGAIController::EvaluateCardPlay(const FGCGCardInstance& CardInstance)
{
	if (!CardDatabase)
	{
		return 0.0f;
	}

	const FGCGCardData* CardData = CardDatabase->GetCardData(CardInstance.CardNumber);
	if (!CardData)
	{
		return 0.0f;
	}

	float Score = 0.0f;

	// Base value: card stats
	Score += CardInstance.AP * 5.0f;
	Score += CardInstance.HP * 3.0f;

	// Card type bonuses
	switch (CardData->CardType)
	{
		case EGCGCardType::Unit:
			Score += 20.0f; // Units are valuable
			break;
		case EGCGCardType::Command:
			Score += 15.0f; // Commands have immediate effect
			break;
		case EGCGCardType::Pilot:
			Score += 10.0f; // Pilots enable Link Units
			break;
		default:
			break;
	}

	// Keyword bonuses
	if (CardData->HasKeyword(EGCGKeyword::Repair))
	{
		Score += 15.0f; // Healing is valuable
	}
	if (CardData->HasKeyword(EGCGKeyword::Breach))
	{
		Score += 20.0f; // Direct damage is strong
	}
	if (CardData->HasKeyword(EGCGKeyword::FirstStrike))
	{
		Score += 10.0f;
	}
	if (CardData->HasKeyword(EGCGKeyword::HighManeuver))
	{
		Score += 12.0f;
	}

	// Evaluate based on game state
	FGCGAIGameEvaluation GameEval = EvaluateGameState();

	// If we're behind on board, prioritize Units
	if (GameEval.BoardControl < 40.0f && CardData->CardType == EGCGCardType::Unit)
	{
		Score += 15.0f;
	}

	// If opponent has strong board, prioritize removal/combat
	if (GameEval.ThreatLevel > 60.0f)
	{
		if (CardData->HasKeyword(EGCGKeyword::Breach))
		{
			Score += 10.0f;
		}
	}

	return Score;
}

float AGCGAIController::EvaluateAttack(const FGCGCardInstance& AttackerInstance, int32 TargetPlayerID)
{
	if (!AIPlayerState || !GameState)
	{
		return 0.0f;
	}

	float Score = 0.0f;

	// Base score: attacker's AP
	Score += AttackerInstance.AP * 10.0f;

	// Check keywords
	if (AttackerInstance.ActiveKeywords.Contains(EGCGKeyword::FirstStrike))
	{
		Score += 15.0f; // FirstStrike is valuable in combat
	}

	if (AttackerInstance.ActiveKeywords.Contains(EGCGKeyword::HighManeuver))
	{
		Score += 10.0f; // Can't be blocked
	}

	// Get opponent state
	AGCGPlayerState* OpponentState = nullptr;
	for (APlayerState* PS : GameState->PlayerArray)
	{
		AGCGPlayerState* GCGPS = Cast<AGCGPlayerState>(PS);
		if (GCGPS && GCGPS->PlayerID == TargetPlayerID)
		{
			OpponentState = GCGPS;
			break;
		}
	}

	if (OpponentState)
	{
		// If opponent has no blockers, attack is safer
		int32 PotentialBlockers = 0;
		for (const FGCGCardInstance& OpponentUnit : OpponentState->BattleArea)
		{
			if (OpponentUnit.bIsActive && OpponentUnit.ActiveKeywords.Contains(EGCGKeyword::Blocker))
			{
				PotentialBlockers++;
			}
		}

		if (PotentialBlockers == 0)
		{
			Score += 20.0f; // Safe attack
		}
		else
		{
			Score -= PotentialBlockers * 5.0f; // Risky attack
		}

		// If opponent is low on shields, attacking is more valuable
		if (OpponentState->ShieldStack.Num() <= 2)
		{
			Score += 25.0f; // Potential game-winning attack
		}
	}

	return Score;
}

float AGCGAIController::EvaluateBlock(const FGCGCardInstance& BlockerInstance, const FGCGCardInstance& AttackerInstance)
{
	float Score = 0.0f;

	// Check if blocker can survive
	int32 BlockerHP = BlockerInstance.HP - BlockerInstance.DamageTaken;
	int32 AttackerAP = AttackerInstance.AP;
	int32 BlockerAP = BlockerInstance.AP;
	int32 AttackerHP = AttackerInstance.HP - AttackerInstance.DamageTaken;

	// Can we kill attacker?
	bool bKillsAttacker = (BlockerAP >= AttackerHP);
	bool bDiesBlocking = (AttackerAP >= BlockerHP);

	if (bKillsAttacker && !bDiesBlocking)
	{
		// Favorable trade: we survive and kill attacker
		Score += 50.0f;
		Score += AttackerInstance.AP * 5.0f; // Bonus for killing strong attacker
	}
	else if (bKillsAttacker && bDiesBlocking)
	{
		// Even trade: both die
		// Evaluate based on card value
		float AttackerValue = GetCardValue(AttackerInstance);
		float BlockerValue = GetCardValue(BlockerInstance);

		if (AttackerValue > BlockerValue)
		{
			Score += 30.0f; // Good trade
		}
		else
		{
			Score += 10.0f; // Even trade
		}
	}
	else if (!bKillsAttacker && !bDiesBlocking)
	{
		// Both survive: chump block to prevent damage
		Score += 15.0f;
	}
	else
	{
		// We die, attacker survives: bad trade
		Score -= 20.0f;
	}

	// If attacker has Breach, blocking is more important
	if (AttackerInstance.ActiveKeywords.Contains(EGCGKeyword::Breach))
	{
		Score += 25.0f; // Prevent direct Base damage
	}

	// If we're low on shields, blocking is critical
	if (AIPlayerState && AIPlayerState->ShieldStack.Num() <= 2)
	{
		Score += 20.0f;
	}

	return Score;
}

// ===========================================================================================
// HELPER FUNCTIONS
// ===========================================================================================

TArray<FGCGAIAction> AGCGAIController::GetValidActions()
{
	TArray<FGCGAIAction> Actions;

	// This is a comprehensive list of all possible actions
	// For now, we use phase-specific decision functions

	return Actions;
}

TArray<FGCGCardInstance> AGCGAIController::GetPlayableCards()
{
	TArray<FGCGCardInstance> PlayableCards;

	if (!AIPlayerState || !CardDatabase)
	{
		return PlayableCards;
	}

	int32 AvailableResources = AIPlayerState->ResourceArea.Num();

	for (const FGCGCardInstance& Card : AIPlayerState->Hand)
	{
		// Check if we have enough resources
		if (Card.Cost <= AvailableResources)
		{
			// Additional checks based on card type
			const FGCGCardData* CardData = CardDatabase->GetCardData(Card.CardNumber);
			if (!CardData)
			{
				continue;
			}

			// Check Battle Area limit for Units
			if (CardData->CardType == EGCGCardType::Unit && AIPlayerState->BattleArea.Num() >= 6)
			{
				continue; // Battle Area full
			}

			PlayableCards.Add(Card);
		}
	}

	return PlayableCards;
}

TArray<FGCGCardInstance> AGCGAIController::GetAttackableUnits()
{
	TArray<FGCGCardInstance> AttackableUnits;

	if (!AIPlayerState || !GameState)
	{
		return AttackableUnits;
	}

	UGCGCombatSubsystem* CombatSubsystem = GetGameInstance()->GetSubsystem<UGCGCombatSubsystem>();
	if (!CombatSubsystem)
	{
		return AttackableUnits;
	}

	for (const FGCGCardInstance& Unit : AIPlayerState->BattleArea)
	{
		// Check if Unit can attack
		if (Unit.bIsActive && !Unit.bHasAttackedThisTurn)
		{
			// Check summoning sickness
			if (!CombatSubsystem->HasSummoningSickness(Unit, GameState))
			{
				AttackableUnits.Add(Unit);
			}
		}
	}

	return AttackableUnits;
}

TArray<FGCGCardInstance> AGCGAIController::GetBlockerUnits()
{
	TArray<FGCGCardInstance> BlockerUnits;

	if (!AIPlayerState)
	{
		return BlockerUnits;
	}

	for (const FGCGCardInstance& Unit : AIPlayerState->BattleArea)
	{
		// Check if Unit can block
		if (Unit.bIsActive)
		{
			BlockerUnits.Add(Unit);
		}
	}

	return BlockerUnits;
}

float AGCGAIController::GetCardValue(const FGCGCardInstance& CardInstance)
{
	if (!CardDatabase)
	{
		return 0.0f;
	}

	const FGCGCardData* CardData = CardDatabase->GetCardData(CardInstance.CardNumber);
	if (!CardData)
	{
		return 0.0f;
	}

	float Value = 0.0f;

	// Base value: stats
	Value += CardInstance.AP * 3.0f;
	Value += CardInstance.HP * 2.0f;

	// Card type
	switch (CardData->CardType)
	{
		case EGCGCardType::Unit:
			Value += 15.0f;
			break;
		case EGCGCardType::Command:
			Value += 10.0f;
			break;
		case EGCGCardType::Pilot:
			Value += 8.0f;
			break;
		default:
			break;
	}

	// Keywords
	Value += CardData->Keywords.Num() * 5.0f;

	// Effects
	Value += CardData->Effects.Num() * 8.0f;

	// Cost efficiency
	if (CardData->Cost > 0)
	{
		Value = Value / FMath::Sqrt(static_cast<float>(CardData->Cost));
	}

	return Value;
}

bool AGCGAIController::ShouldPassPriority()
{
	// Simple heuristic: pass if we've done enough actions this turn
	// More sophisticated AI would consider game state

	if (!GameState)
	{
		return true;
	}

	// Early game: be aggressive
	if (GameState->TurnNumber <= 3)
	{
		return false;
	}

	// Random chance to pass (makes AI less predictable)
	if (Difficulty == EGCGAIDifficulty::Easy)
	{
		return FMath::RandRange(0.0f, 1.0f) > 0.7f;
	}
	else if (Difficulty == EGCGAIDifficulty::Medium)
	{
		return FMath::RandRange(0.0f, 1.0f) > 0.5f;
	}

	return false;
}

// ===========================================================================================
// RANDOM AI (TESTING)
// ===========================================================================================

FGCGAIAction AGCGAIController::MakeRandomAction()
{
	if (!AIPlayerState || !GameState)
	{
		return FGCGAIAction(EGCGAIActionType::PassPriority);
	}

	TArray<FGCGAIAction> PossibleActions;

	// Get all possible actions based on current phase
	switch (GameState->CurrentPhase)
	{
		case EGCGTurnPhase::ResourcePhase:
		{
			if (!AIPlayerState->bPlacedResourceThisTurn && AIPlayerState->Hand.Num() > 0)
			{
				// Random card from hand
				int32 RandomIndex = FMath::RandRange(0, AIPlayerState->Hand.Num() - 1);
				PossibleActions.Add(FGCGAIAction(
					EGCGAIActionType::PlaceResource,
					AIPlayerState->Hand[RandomIndex].InstanceID,
					1.0f,
					TEXT("Random resource placement")
				));
			}
			break;
		}

		case EGCGTurnPhase::MainPhase:
		{
			TArray<FGCGCardInstance> PlayableCards = GetPlayableCards();
			for (const FGCGCardInstance& Card : PlayableCards)
			{
				PossibleActions.Add(FGCGAIAction(
					EGCGAIActionType::PlayCard,
					Card.InstanceID,
					1.0f,
					FString::Printf(TEXT("Random play: %s"), *Card.CardName.ToString())
				));
			}
			break;
		}

		case EGCGTurnPhase::AttackPhase:
		{
			TArray<FGCGCardInstance> AttackableUnits = GetAttackableUnits();
			int32 OpponentID = (AIPlayerState->PlayerID == 0) ? 1 : 0;

			for (const FGCGCardInstance& Unit : AttackableUnits)
			{
				FGCGAIAction AttackAction(
					EGCGAIActionType::Attack,
					Unit.InstanceID,
					1.0f,
					FString::Printf(TEXT("Random attack: %s"), *Unit.CardName.ToString())
				);
				AttackAction.TargetPlayerID = OpponentID;
				PossibleActions.Add(AttackAction);
			}
			break;
		}

		default:
			break;
	}

	// Always add pass option
	PossibleActions.Add(FGCGAIAction(EGCGAIActionType::PassPriority, -1, 1.0f, TEXT("Random pass")));

	// Pick random action
	if (PossibleActions.Num() > 0)
	{
		int32 RandomIndex = FMath::RandRange(0, PossibleActions.Num() - 1);
		return PossibleActions[RandomIndex];
	}

	return FGCGAIAction(EGCGAIActionType::PassPriority);
}

// ===========================================================================================
// DEBUG
// ===========================================================================================

void AGCGAIController::LogAIThinking(const FString& Message)
{
	if (bDebugLogging)
	{
		UE_LOG(LogTemp, Log, TEXT("[AI Player %d] %s"), AIPlayerState ? AIPlayerState->PlayerID : -1, *Message);
	}
}

void AGCGAIController::SetDebugLogging(bool bEnabled)
{
	bDebugLogging = bEnabled;
}
