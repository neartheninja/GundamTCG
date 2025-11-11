// TCGGameMode.h - Game Flow Manager
// Place this in: YourProject/Source/YourProject/TCGGameMode.h

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/GameMode.h"
#include "TCGTypes.h"
#include "TCGGameMode.generated.h"

// Forward declarations
class ATCGPlayerState;
class ATCGPlayerController;

UCLASS()
class ONEPIECETCG_V2_API ATCGGameMode : public AGameMode
{
    GENERATED_BODY()

public:
    ATCGGameMode();

    virtual void BeginPlay() override;
    virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;

    // ===== GAME STATE =====

    UPROPERTY(Replicated, BlueprintReadOnly, Category = "Game State")
    EGamePhase CurrentPhase;

    UPROPERTY(Replicated, BlueprintReadOnly, Category = "Game State")
    int32 TurnNumber;

    UPROPERTY(Replicated, BlueprintReadOnly, Category = "Game State")
    int32 ActivePlayerID;

    UPROPERTY(Replicated, BlueprintReadOnly, Category = "Game State")
    bool bGameInProgress;

    UPROPERTY(BlueprintReadOnly, Category = "Game State")
    bool bAttackInProgress;

    UPROPERTY(BlueprintReadOnly, Category = "Game State")
    FAttackData CurrentAttack;

    // ===== GAME FLOW FUNCTIONS =====

    UFUNCTION(BlueprintCallable, Category = "Game Flow")
    void InitializeGame();

    UFUNCTION(BlueprintCallable, Category = "Game Flow")
    void StartNewTurn();

    UFUNCTION(BlueprintCallable, Category = "Game Flow")
    void AdvancePhase();

    UFUNCTION(BlueprintCallable, Category = "Game Flow")
    void EndTurn();

    UFUNCTION(BlueprintCallable, Category = "Game Flow")
    void EndGame(int32 WinningPlayerID);

    // ===== PHASE HANDLERS =====

    UFUNCTION(BlueprintCallable, Category = "Phases")
    void ExecuteRefreshPhase();

    UFUNCTION(BlueprintCallable, Category = "Phases")
    void ExecuteDrawPhase();

    UFUNCTION(BlueprintCallable, Category = "Phases")
    void ExecuteDonPhase();

    UFUNCTION(BlueprintCallable, Category = "Phases")
    void ExecuteMainPhase();

    UFUNCTION(BlueprintCallable, Category = "Phases")
    void ExecuteBattlePhase();

    UFUNCTION(BlueprintCallable, Category = "Phases")
    void ExecuteEndPhase();

    // ===== ATTACK FLOW =====

    UFUNCTION(BlueprintCallable, Category = "Battle")
    void RequestAttack(const FAttackData& AttackData);

    UFUNCTION(BlueprintCallable, Category = "Battle")
    void SubmitBlockerChoice(bool bWantsToBlock, const FCardData& BlockerCard);

    UFUNCTION(BlueprintCallable, Category = "Battle")
    void SubmitCounterCard(const FCardData& CounterCard);

    UFUNCTION(BlueprintCallable, Category = "Battle")
    void ResolveAttackDamage();

    // ===== VALIDATION FUNCTIONS =====

    UFUNCTION(BlueprintPure, Category = "Validation")
    bool CanAttackInCurrentPhase() const;

    UFUNCTION(BlueprintPure, Category = "Validation")
    bool IsCardValidAttacker(const FCardData& Card) const;

    UFUNCTION(BlueprintPure, Category = "Validation")
    bool IsValidTarget(const FCardData& Attacker, const FCardData& Target) const;

    // ===== HELPER FUNCTIONS =====

    UFUNCTION(BlueprintPure, Category = "Helper")
    ATCGPlayerState* GetPlayerStateByID(int32 PlayerID);

    UFUNCTION(BlueprintPure, Category = "Helper")
    ATCGPlayerController* GetPlayerControllerByID(int32 PlayerID);

    FCardData* FindCardInZone(TArray<FCardData>& Zone, int32 InstanceID);

    void SendErrorToPlayer(int32 PlayerID, const FString& ErrorMessage);

    // ===== BLUEPRINT EVENTS =====

    UFUNCTION(BlueprintImplementableEvent, Category = "Events")
    void OnPhaseChanged(EGamePhase NewPhase);

    UFUNCTION(BlueprintImplementableEvent, Category = "Events")
    void OnTurnStarted(int32 PlayerID);

    UFUNCTION(BlueprintImplementableEvent, Category = "Events")
    void OnGameEnded(int32 WinningPlayerID);

    UFUNCTION(BlueprintImplementableEvent, Category = "Events")
    void OnAttackDeclared(const FAttackData& AttackData);
};
