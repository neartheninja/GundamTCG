// TCGHandWidget.h - Hand Display Widget for One Piece TCG
// Place this in: YourProject/Source/YourProject/TCGHandWidget.h

#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "TCGTypes.h"
#include "TCGHandWidget.generated.h"

class UHorizontalBox;
class UUserWidget;

/**
 * Widget that displays player's hand of cards in a horizontal layout
 * Supports dynamic card spawning, hover effects, and card selection
 */
UCLASS()
class ONEPIECETCG_V2_API UTCGHandWidget : public UUserWidget
{
    GENERATED_BODY()

public:
    UTCGHandWidget(const FObjectInitializer& ObjectInitializer);

    // ===== WIDGET REFERENCES =====

    /** The horizontal box container that holds all card widgets */
    UPROPERTY(BlueprintReadWrite, meta = (BindWidget))
    UHorizontalBox* CardContainer;

    // ===== CARD DISPLAY SETTINGS =====

    /** The card widget class to spawn for each card in hand */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Hand Settings")
    TSubclassOf<UUserWidget> CardWidgetClass;

    /** Maximum number of cards to display */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Hand Settings")
    int32 MaxCardsInHand = 10;

    /** Spacing between cards in pixels */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Hand Settings")
    float CardSpacing = 10.0f;

    /** Card width in pixels */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Hand Settings")
    float CardWidth = 180.0f;

    /** Card height in pixels */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Hand Settings")
    float CardHeight = 252.0f;

    /** Scale factor for cards (1.0 = 100%) */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Hand Settings")
    float CardScale = 1.0f;

    // ===== HOVER SETTINGS =====

    /** How much to lift card up on hover (in pixels) */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Hover Settings")
    float HoverLiftAmount = 30.0f;

    /** How much to scale card on hover (1.1 = 110%) */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Hover Settings")
    float HoverScaleAmount = 1.1f;

    /** Duration of hover animation in seconds */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Hover Settings")
    float HoverAnimationDuration = 0.2f;

    // ===== SELECTION SETTINGS =====

    /** Currently selected card index (-1 = none) */
    UPROPERTY(BlueprintReadOnly, Category = "Selection")
    int32 SelectedCardIndex = -1;

    /** Color to tint selected card */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Selection")
    FLinearColor SelectedCardTint = FLinearColor(1.0f, 1.0f, 0.5f, 1.0f); // Yellow tint

    // ===== HAND DATA =====

    /** Current cards in hand */
    UPROPERTY(BlueprintReadWrite, Category = "Hand Data")
    TArray<FCardData> HandCards;

    /** Cache of spawned card widgets */
    UPROPERTY()
    TArray<UUserWidget*> SpawnedCardWidgets;

    // ===== PUBLIC FUNCTIONS =====

    /** Update the hand display with new card data */
    UFUNCTION(BlueprintCallable, Category = "Hand")
    void UpdateHandDisplay(const TArray<FCardData>& NewHandCards);

    /** Add a single card to the hand */
    UFUNCTION(BlueprintCallable, Category = "Hand")
    void AddCardToHand(const FCardData& NewCard);

    /** Remove a card from the hand by index */
    UFUNCTION(BlueprintCallable, Category = "Hand")
    void RemoveCardFromHand(int32 CardIndex);

    /** Clear all cards from hand */
    UFUNCTION(BlueprintCallable, Category = "Hand")
    void ClearHand();

    /** Select a card by index */
    UFUNCTION(BlueprintCallable, Category = "Selection")
    void SelectCard(int32 CardIndex);

    /** Deselect all cards */
    UFUNCTION(BlueprintCallable, Category = "Selection")
    void DeselectAll();

    /** Get the currently selected card data */
    UFUNCTION(BlueprintPure, Category = "Selection")
    FCardData GetSelectedCard() const;

    /** Check if a card is currently selected */
    UFUNCTION(BlueprintPure, Category = "Selection")
    bool HasSelectedCard() const;

    // ===== BLUEPRINT EVENTS =====

    /** Called when a card is clicked */
    UFUNCTION(BlueprintImplementableEvent, Category = "Events")
    void OnCardClicked(int32 CardIndex, const FCardData& CardData);

    /** Called when a card is hovered */
    UFUNCTION(BlueprintImplementableEvent, Category = "Events")
    void OnCardHovered(int32 CardIndex, const FCardData& CardData);

    /** Called when a card is unhovered */
    UFUNCTION(BlueprintImplementableEvent, Category = "Events")
    void OnCardUnhovered(int32 CardIndex);

    /** Called when hand is updated */
    UFUNCTION(BlueprintImplementableEvent, Category = "Events")
    void OnHandUpdated();

protected:
    virtual void NativeConstruct() override;
    virtual void NativeTick(const FGeometry& MyGeometry, float InDeltaTime) override;

private:
    /** Rebuild the entire hand display */
    void RebuildHandDisplay();

    /** Spawn a single card widget */
    UUserWidget* SpawnCardWidget(const FCardData& CardData, int32 CardIndex);

    /** Setup click handling for a card widget */
    void SetupCardClickHandler(UUserWidget* CardWidget, int32 CardIndex);
};
