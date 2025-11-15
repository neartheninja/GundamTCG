// TCGGameMode.cpp - Example Implementation
// Copy this to your Unreal project's Source folder

#include "TCGGameMode.h"
#include "TCGPlayerState.h"
#include "TCGPlayerController.h"
#include "TCGTypes.h"
#include "Net/UnrealNetwork.h"
#include "EngineUtils.h"
#include "TimerManager.h"

ATCGGameMode::ATCGGameMode()
{
    // Set default classes
    PlayerStateClass = ATCGPlayerState::StaticClass();
    PlayerControllerClass = ATCGPlayerController::StaticClass();

    // Initialize defaults
    CurrentPhase = EGamePhase::REFRESH_PHASE;
    TurnNumber = 0;
    ActivePlayerID = 0;
    bGameInProgress = false;
    bAttackInProgress = false;
    bAllowSoloInPIE = true;
}

void ATCGGameMode::BeginPlay()
{
    Super::BeginPlay();

    UE_LOG(LogTemp, Log, TEXT("TCGGameMode: BeginPlay"));

    // Initialize game after a short delay (let players connect)
    FTimerHandle InitTimerHandle;
    GetWorld()->GetTimerManager().SetTimer(
        InitTimerHandle,
        this,
        &ATCGGameMode::InitializeGame,
        2.0f,
        false
    );
}

void ATCGGameMode::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
    Super::GetLifetimeReplicatedProps(OutLifetimeProps);

    DOREPLIFETIME(ATCGGameMode, CurrentPhase);
    DOREPLIFETIME(ATCGGameMode, TurnNumber);
    DOREPLIFETIME(ATCGGameMode, ActivePlayerID);
    DOREPLIFETIME(ATCGGameMode, bGameInProgress);
}

// ===== GAME FLOW FUNCTIONS =====

void ATCGGameMode::InitializeGame()
{
    UE_LOG(LogTemp, Warning, TEXT("=== INITIALIZING GAME ==="));

    // Get all player states
    TArray<ATCGPlayerState*> PlayerStates;
    for (TActorIterator<ATCGPlayerState> It(GetWorld()); It; ++It)
    {
        PlayerStates.Add(*It);
    }

    // Allow solo testing in PIE (Play in Editor) or when explicitly enabled
    if (PlayerStates.Num() < 2)
    {
        if (GIsEditor || bAllowSoloInPIE)
        {
            UE_LOG(LogTemp, Warning, TEXT("Solo override - proceeding with %d player(s)"), PlayerStates.Num());
        }
        else
        {
            UE_LOG(LogTemp, Error, TEXT("Not enough players! Need 2, have %d"), PlayerStates.Num());
            return;
        }
    }

    // Assign player IDs
    for (int32 i = 0; i < PlayerStates.Num(); i++)
    {
        PlayerStates[i]->TCGPlayerID = i;
        UE_LOG(LogTemp, Log, TEXT("Assigned Player ID %d"), i);
    }

    // Seed basic test data so solo PIE runs don't error
    for (ATCGPlayerState* PS : PlayerStates)
    {
        if (!PS) continue;
        PS->DonDeck.Empty();
        for (int32 d = 0; d < 10; ++d)
        {
            FCardData Don;
            Don.CardName = TEXT("DON");
            Don.CardType = ECardType::DON;
            Don.CurrentZone = ECardZone::DON_DECK;
            Don.OwnerPlayerID = PS->TCGPlayerID;
            PS->DonDeck.Add(Don);
        }
        PS->OnRep_DonDeck();

        // Give a small starter hand for testing
        PS->Hand.Empty();
        for (int32 h = 0; h < 5; ++h)
        {
            FCardData C;
            C.CardID = FString::Printf(TEXT("TEST_CHAR_%d"), h+1);  // Add CardID!
            C.CardName = FString::Printf(TEXT("Test Character %d"), h+1);
            C.CardType = ECardType::CHARACTER;
            C.Power = 3000 + h * 1000;
            C.Cost = 2;
            C.CurrentZone = ECardZone::HAND;
            C.OwnerPlayerID = PS->TCGPlayerID;
            C.InstanceID = 100 + h;
            PS->Hand.Add(C);
        }
        PS->OnRep_Hand();
    }

    // TODO: Load decks from data tables or JSON

    bGameInProgress = true;
    TurnNumber = 1;
    ActivePlayerID = 0; // Player 0 goes first

    // Start the first turn
    StartNewTurn();
}

void ATCGGameMode::StartNewTurn()
{
    UE_LOG(LogTemp, Warning, TEXT("=== TURN %d START - Player %d ==="), TurnNumber, ActivePlayerID);

    CurrentPhase = EGamePhase::REFRESH_PHASE;
    OnPhaseChanged(CurrentPhase);
    OnTurnStarted(ActivePlayerID);

    // Execute refresh phase automatically
    ExecuteRefreshPhase();
}

void ATCGGameMode::AdvancePhase()
{
    // Define phase order
    EGamePhase NextPhase;

    switch (CurrentPhase)
    {
        case EGamePhase::REFRESH_PHASE:
            NextPhase = EGamePhase::DRAW_PHASE;
            break;
        case EGamePhase::DRAW_PHASE:
            NextPhase = EGamePhase::DON_PHASE;
            break;
        case EGamePhase::DON_PHASE:
            NextPhase = EGamePhase::MAIN_PHASE;
            break;
        case EGamePhase::MAIN_PHASE:
            NextPhase = EGamePhase::BATTLE_PHASE;
            break;
        case EGamePhase::BATTLE_PHASE:
            NextPhase = EGamePhase::END_PHASE;
            break;
        case EGamePhase::END_PHASE:
            EndTurn();
            return;
        default:
            return;
    }

    CurrentPhase = NextPhase;
    OnPhaseChanged(CurrentPhase);

    // Auto-execute certain phases
    switch (CurrentPhase)
    {
        case EGamePhase::DRAW_PHASE:
            ExecuteDrawPhase();
            break;
        case EGamePhase::DON_PHASE:
            ExecuteDonPhase();
            break;
    }
}

void ATCGGameMode::EndTurn()
{
    UE_LOG(LogTemp, Warning, TEXT("=== TURN %d END ==="), TurnNumber);

    ExecuteEndPhase();

    // Switch to next player
    ActivePlayerID = (ActivePlayerID + 1) % 2; // Toggle between 0 and 1
    TurnNumber++;

    StartNewTurn();
}

void ATCGGameMode::EndGame(int32 WinningPlayerID)
{
    UE_LOG(LogTemp, Warning, TEXT("=== GAME OVER - Player %d WINS! ==="), WinningPlayerID);

    CurrentPhase = EGamePhase::GAME_OVER;
    bGameInProgress = false;

    OnGameEnded(WinningPlayerID);

    // Notify all players
    for (TActorIterator<ATCGPlayerController> It(GetWorld()); It; ++It)
    {
        // You can show victory/defeat screen here
    }
}

// ===== PHASE HANDLERS =====

void ATCGGameMode::ExecuteRefreshPhase()
{
    UE_LOG(LogTemp, Log, TEXT("Executing Refresh Phase"));

    ATCGPlayerState* ActivePlayer = GetPlayerStateByID(ActivePlayerID);
    if (ActivePlayer)
    {
        // Refresh (untap) all DON
        ActivePlayer->RefreshAllDon();

        // Refresh all characters
        ActivePlayer->RefreshAllCharacters();
    }

    // Auto-advance to draw phase
    FTimerHandle RefreshTimerHandle;
    GetWorld()->GetTimerManager().SetTimer(
        RefreshTimerHandle,
        this,
        &ATCGGameMode::AdvancePhase,
        1.0f,
        false
    );
}

void ATCGGameMode::ExecuteDrawPhase()
{
    UE_LOG(LogTemp, Log, TEXT("Executing Draw Phase"));

    ATCGPlayerState* ActivePlayer = GetPlayerStateByID(ActivePlayerID);
    if (ActivePlayer)
    {
        // Draw 1 card (skip on first turn in actual OP TCG rules)
        if (TurnNumber > 1)
        {
            ActivePlayer->DrawCard();
        }
    }

    // Auto-advance after drawing
    FTimerHandle DrawTimerHandle;
    GetWorld()->GetTimerManager().SetTimer(
        DrawTimerHandle,
        this,
        &ATCGGameMode::AdvancePhase,
        1.5f,
        false
    );
}

void ATCGGameMode::ExecuteDonPhase()
{
    UE_LOG(LogTemp, Log, TEXT("Executing DON Phase"));

    ATCGPlayerState* ActivePlayer = GetPlayerStateByID(ActivePlayerID);
    if (ActivePlayer)
    {
        // Add 2 DON to zone (or 1, depending on turn number in actual rules)
        int32 DonToAdd = (TurnNumber == 1) ? 1 : 2;

        for (int32 i = 0; i < DonToAdd; i++)
        {
            ActivePlayer->AddDonToZone();
        }
    }

    // Auto-advance to main phase
    FTimerHandle DonTimerHandle;
    GetWorld()->GetTimerManager().SetTimer(
        DonTimerHandle,
        this,
        &ATCGGameMode::AdvancePhase,
        1.5f,
        false
    );
}

void ATCGGameMode::ExecuteMainPhase()
{
    UE_LOG(LogTemp, Log, TEXT("Main Phase - Waiting for player actions"));

    // Player can now:
    // - Play characters
    // - Play events
    // - Activate abilities
    // - Attach DON to characters

    // Player must manually advance to battle phase via UI button
}

void ATCGGameMode::ExecuteBattlePhase()
{
    UE_LOG(LogTemp, Log, TEXT("Battle Phase - Ready to declare attacks"));

    // Player can now:
    // - Declare attacks with active characters

    // Player must manually advance to end phase when done attacking
}

void ATCGGameMode::ExecuteEndPhase()
{
    UE_LOG(LogTemp, Log, TEXT("Executing End Phase"));

    ATCGPlayerState* ActivePlayer = GetPlayerStateByID(ActivePlayerID);
    if (ActivePlayer)
    {
        // End of turn effects
        // - Check hand size limit
        // - Trigger end-of-turn abilities
        // - etc.
    }
}

// ===== ATTACK FLOW =====

void ATCGGameMode::RequestAttack(const FAttackData& AttackData)
{
    UE_LOG(LogTemp, Log, TEXT("Attack requested: %s → %s"),
        *AttackData.AttackerCard.CardName,
        *AttackData.TargetCard.CardName);

    // Validation
    if (!CanAttackInCurrentPhase())
    {
        SendErrorToPlayer(AttackData.AttackingPlayerID, "Not in Battle Phase!");
        return;
    }

    if (AttackData.AttackingPlayerID != ActivePlayerID)
    {
        SendErrorToPlayer(AttackData.AttackingPlayerID, "Not your turn!");
        return;
    }

    ATCGPlayerState* AttackerState = GetPlayerStateByID(AttackData.AttackingPlayerID);
    if (!AttackerState)
    {
        return;
    }

    // Find the actual card instance
    FCardData* AttackerCard = FindCardInZone(
        AttackerState->CharacterZone,
        AttackData.AttackerCard.InstanceID
    );

    if (!AttackerCard)
    {
        SendErrorToPlayer(AttackData.AttackingPlayerID, "Attacker not found!");
        return;
    }

    if (!IsCardValidAttacker(*AttackerCard))
    {
        SendErrorToPlayer(AttackData.AttackingPlayerID, "That card cannot attack!");
        return;
    }

    // Attack is valid! Process it
    UE_LOG(LogTemp, Warning, TEXT("Attack validated! Processing..."));

    // Rest the attacker
    AttackerCard->bIsRested = true;
    AttackerState->OnRep_CharacterZone();

    // Store attack data
    CurrentAttack = AttackData;
    bAttackInProgress = true;

    // Broadcast event
    OnAttackDeclared(CurrentAttack);

    // Advance to block step
    CurrentPhase = EGamePhase::BATTLE_BLOCK_STEP;
    OnPhaseChanged(CurrentPhase);

    // Ask defender to block
    ATCGPlayerController* DefenderController = GetPlayerControllerByID(AttackData.DefendingPlayerID);
    if (DefenderController)
    {
        DefenderController->Client_ShowBlockerChoice(CurrentAttack);
    }
}

void ATCGGameMode::SubmitBlockerChoice(bool bWantsToBlock, const FCardData& BlockerCard)
{
    if (CurrentPhase != EGamePhase::BATTLE_BLOCK_STEP)
    {
        UE_LOG(LogTemp, Warning, TEXT("Block submitted outside block step!"));
        return;
    }

    if (bWantsToBlock)
    {
        UE_LOG(LogTemp, Log, TEXT("Defender blocks with: %s"), *BlockerCard.CardName);

        ATCGPlayerState* DefenderState = GetPlayerStateByID(CurrentAttack.DefendingPlayerID);
        FCardData* Blocker = FindCardInZone(DefenderState->CharacterZone, BlockerCard.InstanceID);

        if (!Blocker || Blocker->bIsRested)
        {
            UE_LOG(LogTemp, Error, TEXT("Invalid blocker!"));
            return;
        }

        // Redirect attack
        CurrentAttack.TargetCard = *Blocker;
        CurrentAttack.bIsTargetingLeader = false;

        // Rest blocker
        Blocker->bIsRested = true;
        DefenderState->OnRep_CharacterZone();
    }
    else
    {
        UE_LOG(LogTemp, Log, TEXT("Defender chooses not to block"));
    }

    // Advance to counter step
    CurrentPhase = EGamePhase::BATTLE_COUNTER_STEP;
    OnPhaseChanged(CurrentPhase);

    // Show counter UI to both players
    ATCGPlayerController* AttackerController = GetPlayerControllerByID(CurrentAttack.AttackingPlayerID);
    ATCGPlayerController* DefenderController = GetPlayerControllerByID(CurrentAttack.DefendingPlayerID);

    if (AttackerController)
    {
        AttackerController->Client_ShowCounterChoice(CurrentAttack);
    }
    if (DefenderController)
    {
        DefenderController->Client_ShowCounterChoice(CurrentAttack);
    }
}

void ATCGGameMode::SubmitCounterCard(const FCardData& CounterCard)
{
    // Implementation similar to blocker
    // Add counter power to appropriate card
    // When both players have submitted (or timeout), resolve damage
}

void ATCGGameMode::ResolveAttackDamage()
{
    if (!bAttackInProgress)
    {
        return;
    }

    UE_LOG(LogTemp, Warning, TEXT("=== RESOLVING DAMAGE ==="));

    CurrentPhase = EGamePhase::BATTLE_DAMAGE_STEP;

    ATCGPlayerState* AttackerState = GetPlayerStateByID(CurrentAttack.AttackingPlayerID);
    ATCGPlayerState* DefenderState = GetPlayerStateByID(CurrentAttack.DefendingPlayerID);

    int32 AttackerPower = CurrentAttack.AttackerCard.Power;
    int32 DefenderPower = CurrentAttack.TargetCard.Power;

    if (AttackerPower > DefenderPower)
    {
        // Attacker wins
        UE_LOG(LogTemp, Warning, TEXT("ATTACKER WINS!"));

        if (CurrentAttack.bIsTargetingLeader)
        {
            // Deal damage to leader
            DefenderState->ApplyCardDamage(1);

            if (DefenderState->HasLost())
            {
                EndGame(CurrentAttack.AttackingPlayerID);
                return;
            }
        }
        else
        {
            // K.O. defender
            DefenderState->KOCharacter(CurrentAttack.TargetCard.InstanceID);
        }
    }
    else if (DefenderPower > AttackerPower)
    {
        // Defender wins
        UE_LOG(LogTemp, Warning, TEXT("DEFENDER WINS!"));
        AttackerState->KOCharacter(CurrentAttack.AttackerCard.InstanceID);
    }
    else
    {
        // Tie - both K.O.
        UE_LOG(LogTemp, Warning, TEXT("TIE!"));
        if (!CurrentAttack.bIsTargetingLeader)
        {
            AttackerState->KOCharacter(CurrentAttack.AttackerCard.InstanceID);
            DefenderState->KOCharacter(CurrentAttack.TargetCard.InstanceID);
        }
    }

    // Cleanup
    bAttackInProgress = false;
    CurrentPhase = EGamePhase::BATTLE_PHASE;
    OnPhaseChanged(CurrentPhase);
}

// ===== VALIDATION FUNCTIONS =====

bool ATCGGameMode::CanAttackInCurrentPhase() const
{
    return CurrentPhase == EGamePhase::BATTLE_PHASE;
}

bool ATCGGameMode::IsCardValidAttacker(const FCardData& Card) const
{
    // Card must be active (not rested)
    if (Card.bIsRested)
    {
        return false;
    }

    // Card must be a character with power
    if (Card.CardType != ECardType::CHARACTER)
    {
        return false;
    }

    if (Card.Power <= 0)
    {
        return false;
    }

    // TODO: Check for summoning sickness (if applicable in your rules)

    return true;
}

bool ATCGGameMode::IsValidTarget(const FCardData& Attacker, const FCardData& Target) const
{
    // Can attack any character or leader
    // Some games have restrictions (e.g., must attack characters before leader)
    // Implement your rules here

    return true;
}

// ===== HELPER FUNCTIONS =====

ATCGPlayerState* ATCGGameMode::GetPlayerStateByID(int32 TCGPlayerID)
{
    for (TActorIterator<ATCGPlayerState> It(GetWorld()); It; ++It)
    {
        if ((*It)->TCGPlayerID == TCGPlayerID)
        {
            return *It;
        }
    }
    return nullptr;
}

ATCGPlayerController* ATCGGameMode::GetPlayerControllerByID(int32 TCGPlayerID)
{
    for (TActorIterator<ATCGPlayerController> It(GetWorld()); It; ++It)
    {
        if ((*It)->GetMyPlayerID() == TCGPlayerID)
        {
            return *It;
        }
    }
    return nullptr;
}

FCardData* ATCGGameMode::FindCardInZone(TArray<FCardData>& Zone, int32 InstanceID)
{
    for (FCardData& Card : Zone)
    {
        if (Card.InstanceID == InstanceID)
        {
            return &Card;
        }
    }
    return nullptr;
}

void ATCGGameMode::SendErrorToPlayer(int32 TCGPlayerID, const FString& ErrorMessage)
{
    ATCGPlayerController* PC = GetPlayerControllerByID(TCGPlayerID);
    if (PC)
    {
        PC->Client_ShowError(ErrorMessage);
    }

    UE_LOG(LogTemp, Warning, TEXT("Error for Player %d: %s"), TCGPlayerID, *ErrorMessage);
}
