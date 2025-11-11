// TCGPlayerController.h - Input & Client-Server Communication
// Place this in: YourProject/Source/YourProject/TCGPlayerController.h

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/PlayerController.h"
#include "TCGTypes.h"
#include "TCGPlayerController.generated.h"

// Forward declarations
class ATCGGameMode;
class ATCGPlayerState;

UCLASS()
class ONEPIECETCG_V2_API ATCGPlayerController : public APlayerController
{
    GENERATED_BODY()

public:
    ATCGPlayerController();

    virtual void BeginPlay() override;

    // ===== CACHED REFERENCES =====

    UPROPERTY(BlueprintReadOnly, Category = "References")
    ATCGGameMode* TCGGameMode;

    UPROPERTY(BlueprintReadOnly, Category = "References")
    ATCGPlayerState* TCGPlayerState;

    // ===== CLIENT → SERVER REQUESTS =====

    UFUNCTION(Server, Reliable, WithValidation, BlueprintCallable, Category = "Requests")
    void Server_RequestDrawCard();
    void Server_RequestDrawCard_Implementation();
    bool Server_RequestDrawCard_Validate();

    UFUNCTION(Server, Reliable, WithValidation, BlueprintCallable, Category = "Requests")
    void Server_RequestPlayCharacter(int32 HandIndex);
    void Server_RequestPlayCharacter_Implementation(int32 HandIndex);
    bool Server_RequestPlayCharacter_Validate(int32 HandIndex);

    UFUNCTION(Server, Reliable, WithValidation, BlueprintCallable, Category = "Requests")
    void Server_RequestAttack(const FAttackData& AttackData);
    void Server_RequestAttack_Implementation(const FAttackData& AttackData);
    bool Server_RequestAttack_Validate(const FAttackData& AttackData);

    UFUNCTION(Server, Reliable, WithValidation, BlueprintCallable, Category = "Requests")
    void Server_SubmitBlocker(bool bWantsToBlock, const FCardData& BlockerCard);
    void Server_SubmitBlocker_Implementation(bool bWantsToBlock, const FCardData& BlockerCard);
    bool Server_SubmitBlocker_Validate(bool bWantsToBlock, const FCardData& BlockerCard);

    UFUNCTION(Server, Reliable, WithValidation, BlueprintCallable, Category = "Requests")
    void Server_SubmitCounter(const FCardData& CounterCard);
    void Server_SubmitCounter_Implementation(const FCardData& CounterCard);
    bool Server_SubmitCounter_Validate(const FCardData& CounterCard);

    UFUNCTION(Server, Reliable, WithValidation, BlueprintCallable, Category = "Requests")
    void Server_RequestEndTurn();
    void Server_RequestEndTurn_Implementation();
    bool Server_RequestEndTurn_Validate();

    // ===== SERVER → CLIENT EVENTS =====

    UFUNCTION(Client, Reliable, BlueprintCallable, Category = "Client Events")
    void Client_ShowBlockerChoice(const FAttackData& AttackData);
    void Client_ShowBlockerChoice_Implementation(const FAttackData& AttackData);

    UFUNCTION(Client, Reliable, BlueprintCallable, Category = "Client Events")
    void Client_ShowCounterChoice(const FAttackData& AttackData);
    void Client_ShowCounterChoice_Implementation(const FAttackData& AttackData);

    UFUNCTION(Client, Reliable, BlueprintCallable, Category = "Client Events")
    void Client_ShowDamage(int32 DamageAmount);
    void Client_ShowDamage_Implementation(int32 DamageAmount);

    UFUNCTION(Client, Reliable, BlueprintCallable, Category = "Client Events")
    void Client_ShowError(const FString& ErrorMessage);
    void Client_ShowError_Implementation(const FString& ErrorMessage);

    // ===== HELPER FUNCTIONS =====

    UFUNCTION(BlueprintPure, Category = "Helper")
    bool IsMyTurn() const;

    UFUNCTION(BlueprintPure, Category = "Helper")
    int32 GetMyPlayerID() const;

    // ===== BLUEPRINT EVENTS =====

    UFUNCTION(BlueprintImplementableEvent, Category = "UI Events")
    void OnShowBlockerChoiceUI(const FAttackData& AttackData);

    UFUNCTION(BlueprintImplementableEvent, Category = "UI Events")
    void OnShowCounterChoiceUI(const FAttackData& AttackData);

    UFUNCTION(BlueprintImplementableEvent, Category = "UI Events")
    void OnShowDamageEffect(int32 DamageAmount);

    UFUNCTION(BlueprintImplementableEvent, Category = "UI Events")
    void OnShowErrorMessage(const FString& ErrorMessage);
};
