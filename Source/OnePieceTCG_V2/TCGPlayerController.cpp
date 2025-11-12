// TCGPlayerController.cpp - Example Implementation
// Copy this to your Unreal project's Source folder

#include "TCGPlayerController.h"
#include "TCGGameMode.h"
#include "TCGPlayerState.h"
#include "Blueprint/UserWidget.h"
#include "UObject/ConstructorHelpers.h"
#include "TCGHandWidget.h"

ATCGPlayerController::ATCGPlayerController()
{
    bReplicates = true;
    bShowMouseCursor = true; // Show cursor for UI interactions
    HandWidget = nullptr;

    // Auto-assign WBP_TCG_Hand if it exists in /Game
    static ConstructorHelpers::FClassFinder<UUserWidget> HandWidgetBP(TEXT("/Game/WBP_TCG_Hand"));
    if (HandWidgetBP.Succeeded())
    {
        HandWidgetClass = HandWidgetBP.Class;
    }
}

void ATCGPlayerController::BeginPlay()
{
    Super::BeginPlay();

    // Cache references
    TCGGameMode = Cast<ATCGGameMode>(GetWorld()->GetAuthGameMode());
    TCGPlayerState = Cast<ATCGPlayerState>(PlayerState);

    UE_LOG(LogTemp, Log, TEXT("TCGPlayerController: BeginPlay (Role: %s, IsLocal: %s)"),
        (GetLocalRole() == ROLE_Authority) ? TEXT("Server") : TEXT("Client"),
        IsLocalController() ? TEXT("YES") : TEXT("NO"));

    // Only spawn UI on the local client
    if (!IsLocalController())
    {
        UE_LOG(LogTemp, Log, TEXT("Not local controller, skipping UI creation"));
        return;
    }

    // Create hand widget
    if (HandWidgetClass)
    {
        UE_LOG(LogTemp, Log, TEXT("Creating hand widget from class..."));
        HandWidget = CreateWidget<UUserWidget>(this, HandWidgetClass);

        if (HandWidget)
        {
            UE_LOG(LogTemp, Warning, TEXT("Hand widget created successfully! Adding to viewport..."));
            HandWidget->AddToViewport(999);

            // Enable mouse cursor and set input mode
            bShowMouseCursor = true;

            FInputModeGameAndUI InputMode;
            InputMode.SetHideCursorDuringCapture(false);
            InputMode.SetLockMouseToViewportBehavior(EMouseLockMode::DoNotLock);
            SetInputMode(InputMode);

            UE_LOG(LogTemp, Warning, TEXT("Hand widget added to viewport with mouse cursor enabled"));

            // If this hand widget is our C++ widget, push current hand data once
            if (UTCGHandWidget* TCGHand = Cast<UTCGHandWidget>(HandWidget))
            {
                if (TCGPlayerState)
                {
                    TCGHand->UpdateHandDisplay(TCGPlayerState->Hand);
                }
            }

            // Subscribe to PlayerState hand updates so UI refreshes after GameMode seeds data
            if (TCGPlayerState)
            {
                TCGPlayerState->OnHandUpdatedEvent.AddDynamic(this, &ATCGPlayerController::HandleOnHandUpdated);
            }
        }
        else
        {
            UE_LOG(LogTemp, Error, TEXT("Failed to create hand widget!"));
        }
    }
    else
    {
        UE_LOG(LogTemp, Warning, TEXT("HandWidgetClass is not set in PlayerController!"));
    }
}

void ATCGPlayerController::HandleOnHandUpdated()
{
    if (!IsLocalController())
    {
        return;
    }

    if (UTCGHandWidget* TCGHand = Cast<UTCGHandWidget>(HandWidget))
    {
        if (TCGPlayerState)
        {
            UE_LOG(LogTemp, Log, TEXT("UI: Refreshing hand display (%d cards)"), TCGPlayerState->Hand.Num());
            TCGHand->UpdateHandDisplay(TCGPlayerState->Hand);
        }
    }
}

// ===== CLIENT → SERVER REQUESTS =====

// ===== DRAW CARD =====

bool ATCGPlayerController::Server_RequestDrawCard_Validate()
{
    // Basic validation
    return true;
}

void ATCGPlayerController::Server_RequestDrawCard_Implementation()
{
    UE_LOG(LogTemp, Log, TEXT("Server: Draw card request from Player %d"), GetMyPlayerID());

    if (!TCGPlayerState)
    {
        UE_LOG(LogTemp, Error, TEXT("PlayerState not found!"));
        return;
    }

    // Check if it's this player's turn
    if (TCGGameMode && TCGGameMode->ActivePlayerID != GetMyPlayerID())
    {
        Client_ShowError("Not your turn!");
        return;
    }

    // Check if already drawn this turn
    if (TCGPlayerState->bHasDrawnThisTurn)
    {
        Client_ShowError("Already drew a card this turn!");
        return;
    }

    // Draw the card
    if (TCGPlayerState->DrawCard())
    {
        TCGPlayerState->bHasDrawnThisTurn = true;
    }
}

// ===== PLAY CHARACTER =====

bool ATCGPlayerController::Server_RequestPlayCharacter_Validate(int32 HandIndex)
{
    // Prevent out-of-bounds access
    if (HandIndex < 0 || HandIndex > 100) // Sanity check
    {
        return false;
    }
    return true;
}

void ATCGPlayerController::Server_RequestPlayCharacter_Implementation(int32 HandIndex)
{
    UE_LOG(LogTemp, Log, TEXT("Server: Play character request (index %d) from Player %d"),
        HandIndex, GetMyPlayerID());

    if (!TCGPlayerState)
    {
        return;
    }

    // Check phase (must be Main Phase)
    if (TCGGameMode && TCGGameMode->CurrentPhase != EGamePhase::MAIN_PHASE)
    {
        Client_ShowError("Can only play characters in Main Phase!");
        return;
    }

    // Check turn
    if (TCGGameMode && TCGGameMode->ActivePlayerID != GetMyPlayerID())
    {
        Client_ShowError("Not your turn!");
        return;
    }

    // Validate hand index
    if (HandIndex < 0 || HandIndex >= TCGPlayerState->Hand.Num())
    {
        Client_ShowError("Invalid card!");
        return;
    }

    // Attempt to play
    if (!TCGPlayerState->PlayCharacter(HandIndex))
    {
        Client_ShowError("Cannot play that character!");
    }
}

// ===== ATTACK =====

bool ATCGPlayerController::Server_RequestAttack_Validate(const FAttackData& AttackData)
{
    // Basic validation
    if (AttackData.AttackerCard.InstanceID <= 0)
    {
        return false; // Invalid attacker
    }

    if (AttackData.AttackingPlayerID != GetMyPlayerID())
    {
        return false; // Trying to attack with someone else's card
    }

    return true;
}

void ATCGPlayerController::Server_RequestAttack_Implementation(const FAttackData& AttackData)
{
    UE_LOG(LogTemp, Log, TEXT("Server: Attack request - %s → %s"),
        *AttackData.AttackerCard.CardName,
        *AttackData.TargetCard.CardName);

    if (!TCGGameMode)
    {
        return;
    }

    // Forward to GameMode for processing
    TCGGameMode->RequestAttack(AttackData);
}

// ===== BLOCKER =====

bool ATCGPlayerController::Server_SubmitBlocker_Validate(bool bWantsToBlock, const FCardData& BlockerCard)
{
    return true;
}

void ATCGPlayerController::Server_SubmitBlocker_Implementation(bool bWantsToBlock, const FCardData& BlockerCard)
{
    UE_LOG(LogTemp, Log, TEXT("Server: Blocker choice - %s (Block: %s)"),
        *BlockerCard.CardName,
        bWantsToBlock ? TEXT("YES") : TEXT("NO"));

    if (!TCGGameMode)
    {
        return;
    }

    TCGGameMode->SubmitBlockerChoice(bWantsToBlock, BlockerCard);
}

// ===== COUNTER =====

bool ATCGPlayerController::Server_SubmitCounter_Validate(const FCardData& CounterCard)
{
    return true;
}

void ATCGPlayerController::Server_SubmitCounter_Implementation(const FCardData& CounterCard)
{
    UE_LOG(LogTemp, Log, TEXT("Server: Counter card submitted - %s"), *CounterCard.CardName);

    if (!TCGGameMode)
    {
        return;
    }

    TCGGameMode->SubmitCounterCard(CounterCard);
}

// ===== END TURN =====

bool ATCGPlayerController::Server_RequestEndTurn_Validate()
{
    return true;
}

void ATCGPlayerController::Server_RequestEndTurn_Implementation()
{
    UE_LOG(LogTemp, Log, TEXT("Server: End turn request from Player %d"), GetMyPlayerID());

    if (!TCGGameMode)
    {
        return;
    }

    // Verify it's this player's turn
    if (TCGGameMode->ActivePlayerID != GetMyPlayerID())
    {
        Client_ShowError("Not your turn!");
        return;
    }

    TCGGameMode->EndTurn();
}

// ===== SERVER → CLIENT EVENTS =====

void ATCGPlayerController::Client_ShowBlockerChoice_Implementation(const FAttackData& AttackData)
{
    UE_LOG(LogTemp, Log, TEXT("Client: Showing blocker choice UI"));

    // Call Blueprint event to create widget
    OnShowBlockerChoiceUI(AttackData);
}

void ATCGPlayerController::Client_ShowCounterChoice_Implementation(const FAttackData& AttackData)
{
    UE_LOG(LogTemp, Log, TEXT("Client: Showing counter choice UI"));

    // Call Blueprint event to create widget
    OnShowCounterChoiceUI(AttackData);
}

void ATCGPlayerController::Client_ShowDamage_Implementation(int32 DamageAmount)
{
    UE_LOG(LogTemp, Log, TEXT("Client: Showing damage effect (%d damage)"), DamageAmount);

    // Call Blueprint event for visual effects
    OnShowDamageEffect(DamageAmount);
}

void ATCGPlayerController::Client_ShowError_Implementation(const FString& ErrorMessage)
{
    UE_LOG(LogTemp, Warning, TEXT("Client: Error - %s"), *ErrorMessage);

    // Call Blueprint event to show error popup
    OnShowErrorMessage(ErrorMessage);

    // Also show on screen for debugging
    if (GEngine)
    {
        GEngine->AddOnScreenDebugMessage(-1, 5.0f, FColor::Red, ErrorMessage);
    }
}

// ===== HELPER FUNCTIONS =====

bool ATCGPlayerController::IsMyTurn() const
{
    if (!TCGGameMode)
    {
        return false;
    }

    return TCGGameMode->ActivePlayerID == GetMyPlayerID();
}

int32 ATCGPlayerController::GetMyPlayerID() const
{
    if (!TCGPlayerState)
    {
        return -1;
    }

    return TCGPlayerState->TCGPlayerID;
}
