// TCGPlayerState.h - Player Data & Card Zones
// Place this in: YourProject/Source/YourProject/TCGPlayerState.h

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/PlayerState.h"
#include "TCGTypes.h"
#include "TCGPlayerState.generated.h"

DECLARE_DYNAMIC_MULTICAST_DELEGATE(FOnHandUpdatedNative);

UCLASS(BlueprintType, Blueprintable)
class GUNDAMTCG_API ATCGPlayerState : public APlayerState
{
    GENERATED_BODY()

public:
    ATCGPlayerState();

    virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;

    // ===== PLAYER INFO =====

    UPROPERTY(Replicated, BlueprintReadOnly, Category = "Player")
    int32 TCGPlayerID;

    UPROPERTY(Replicated, BlueprintReadOnly, Category = "Player")
    int32 AvailableDon;

    UPROPERTY(Replicated, BlueprintReadOnly, Category = "Player")
    bool bHasDrawnThisTurn;

    // ===== CARD ZONES =====

    UPROPERTY(ReplicatedUsing = OnRep_Hand, BlueprintReadOnly, Category = "Zones")
    TArray<FCardData> Hand;

    UPROPERTY(ReplicatedUsing = OnRep_Deck, BlueprintReadOnly, Category = "Zones")
    TArray<FCardData> Deck;

    UPROPERTY(ReplicatedUsing = OnRep_Life, BlueprintReadOnly, Category = "Zones")
    TArray<FCardData> Life;

    UPROPERTY(ReplicatedUsing = OnRep_Leader, BlueprintReadOnly, Category = "Zones")
    FCardData LeaderCard;

    UPROPERTY(ReplicatedUsing = OnRep_DonDeck, BlueprintReadOnly, Category = "Zones")
    TArray<FCardData> DonDeck;

    UPROPERTY(ReplicatedUsing = OnRep_DonZone, BlueprintReadOnly, Category = "Zones")
    TArray<FCardData> DonZone;

    UPROPERTY(ReplicatedUsing = OnRep_CharacterZone, BlueprintReadOnly, Category = "Zones")
    TArray<FCardData> CharacterZone;

    UPROPERTY(ReplicatedUsing = OnRep_StageZone, BlueprintReadOnly, Category = "Zones")
    TArray<FCardData> StageZone;

    UPROPERTY(ReplicatedUsing = OnRep_Trash, BlueprintReadOnly, Category = "Zones")
    TArray<FCardData> Trash;

    // ===== DECK OPERATIONS =====

    UFUNCTION(BlueprintCallable, Category = "Deck")
    bool DrawCard();

    UFUNCTION(BlueprintCallable, Category = "Deck")
    void DrawCards(int32 Count);

    UFUNCTION(BlueprintCallable, Category = "Deck")
    void ShuffleDeck();

    // ===== DON OPERATIONS =====

    UFUNCTION(BlueprintCallable, Category = "DON")
    bool AddDonToZone();

    UFUNCTION(BlueprintCallable, Category = "DON")
    bool AttachDonToCharacter(int32 CharacterInstanceID);

    UFUNCTION(BlueprintCallable, Category = "DON")
    bool DetachDonFromCharacter(int32 CharacterInstanceID);

    UFUNCTION(BlueprintCallable, Category = "DON")
    void RefreshAllDon();

    // ===== CHARACTER OPERATIONS =====

    UFUNCTION(BlueprintCallable, Category = "Characters")
    bool PlayCharacter(int32 HandIndex);

    UFUNCTION(BlueprintCallable, Category = "Characters")
    bool RestCharacter(int32 CharacterInstanceID);

    UFUNCTION(BlueprintCallable, Category = "Characters")
    bool RefreshCharacter(int32 CharacterInstanceID);

    UFUNCTION(BlueprintCallable, Category = "Characters")
    void RefreshAllCharacters();

    UFUNCTION(BlueprintCallable, Category = "Characters")
    void KOCharacter(int32 CharacterInstanceID);

    // ===== LIFE OPERATIONS =====

    UFUNCTION(BlueprintCallable, Category = "Life")
    void ApplyCardDamage(int32 DamageAmount);

    UFUNCTION(BlueprintPure, Category = "Life")
    bool HasLost() const;

    // ===== HELPER FUNCTIONS =====

    UFUNCTION(BlueprintPure, Category = "Helper")
    FCardData FindCardByInstanceID(int32 InstanceID, ECardZone Zone);

    UFUNCTION(BlueprintPure, Category = "Helper")
    int32 GetCharacterTotalPower(int32 CharacterInstanceID) const;

    // ===== REPLICATION CALLBACKS =====

    UFUNCTION()
    void OnRep_Hand();

    UFUNCTION()
    void OnRep_Deck();

    UFUNCTION()
    void OnRep_Life();

    UFUNCTION()
    void OnRep_Leader();

    UFUNCTION()
    void OnRep_DonDeck();

    UFUNCTION()
    void OnRep_DonZone();

    UFUNCTION()
    void OnRep_CharacterZone();

    UFUNCTION()
    void OnRep_StageZone();

    UFUNCTION()
    void OnRep_Trash();

    // ===== BLUEPRINT EVENTS =====

    UFUNCTION(BlueprintImplementableEvent, Category = "Events")
    void OnHandUpdated();

    // Native multicast for C++/BP bindings (UI can subscribe)
    UPROPERTY(BlueprintAssignable, Category = "Events")
    FOnHandUpdatedNative OnHandUpdatedEvent;

    UFUNCTION(BlueprintImplementableEvent, Category = "Events")
    void OnLifeUpdated();

    UFUNCTION(BlueprintImplementableEvent, Category = "Events")
    void OnDonZoneUpdated();

    UFUNCTION(BlueprintImplementableEvent, Category = "Events")
    void OnCharacterZoneUpdated();
};
