// TCGHandWidget.cpp - Hand Display Widget Implementation

#include "TCGHandWidget.h"
#include "Components/HorizontalBox.h"
#include "Components/HorizontalBoxSlot.h"
#include "Blueprint/WidgetTree.h"
#include "Components/Button.h"
#include "Components/Image.h"
#include "UObject/UnrealType.h"
#include "UObject/ConstructorHelpers.h"

UTCGHandWidget::UTCGHandWidget(const FObjectInitializer& ObjectInitializer)
    : Super(ObjectInitializer)
{
    // Set default values
    MaxCardsInHand = 10;
    CardSpacing = 10.0f;
    CardWidth = 180.0f;
    CardHeight = 252.0f;
    CardScale = 1.0f;
    HoverLiftAmount = 30.0f;
    HoverScaleAmount = 1.1f;
    HoverAnimationDuration = 0.2f;
    SelectedCardIndex = -1;
    SelectedCardTint = FLinearColor(1.0f, 1.0f, 0.5f, 1.0f);

    // Default card widget class if not set in BP
    static ConstructorHelpers::FClassFinder<UUserWidget> CardBP(TEXT("/Game/WBP_TCG_Card"));
    if (CardBP.Succeeded())
    {
        CardWidgetClass = CardBP.Class;
    }
}

void UTCGHandWidget::NativeConstruct()
{
    Super::NativeConstruct();

    // Initialize the hand display
    if (CardContainer)
    {
        CardContainer->ClearChildren();
    }
}

void UTCGHandWidget::NativeTick(const FGeometry& MyGeometry, float InDeltaTime)
{
    Super::NativeTick(MyGeometry, InDeltaTime);
}

void UTCGHandWidget::UpdateHandDisplay(const TArray<FCardData>& NewHandCards)
{
    HandCards = NewHandCards;
    RebuildHandDisplay();
    OnHandUpdated();
}

void UTCGHandWidget::AddCardToHand(const FCardData& NewCard)
{
    if (HandCards.Num() >= MaxCardsInHand)
    {
        UE_LOG(LogTemp, Warning, TEXT("Hand is full! Cannot add more cards."));
        return;
    }

    HandCards.Add(NewCard);
    RebuildHandDisplay();
    OnHandUpdated();
}

void UTCGHandWidget::RemoveCardFromHand(int32 CardIndex)
{
    if (CardIndex < 0 || CardIndex >= HandCards.Num())
    {
        UE_LOG(LogTemp, Warning, TEXT("Invalid card index: %d"), CardIndex);
        return;
    }

    HandCards.RemoveAt(CardIndex);

    // Adjust selected index if needed
    if (SelectedCardIndex == CardIndex)
    {
        SelectedCardIndex = -1;
    }
    else if (SelectedCardIndex > CardIndex)
    {
        SelectedCardIndex--;
    }

    RebuildHandDisplay();
    OnHandUpdated();
}

void UTCGHandWidget::ClearHand()
{
    HandCards.Empty();
    SelectedCardIndex = -1;
    RebuildHandDisplay();
    OnHandUpdated();
}

void UTCGHandWidget::SelectCard(int32 CardIndex)
{
    if (CardIndex < 0 || CardIndex >= HandCards.Num())
    {
        UE_LOG(LogTemp, Warning, TEXT("Invalid card index for selection: %d"), CardIndex);
        return;
    }

    SelectedCardIndex = CardIndex;

    // Visual feedback would be handled in Blueprint
    // You can implement highlighting here or in BP
}

void UTCGHandWidget::DeselectAll()
{
    SelectedCardIndex = -1;
}

FCardData UTCGHandWidget::GetSelectedCard() const
{
    if (SelectedCardIndex >= 0 && SelectedCardIndex < HandCards.Num())
    {
        return HandCards[SelectedCardIndex];
    }
    return FCardData(); // Return empty card data
}

bool UTCGHandWidget::HasSelectedCard() const
{
    return SelectedCardIndex >= 0 && SelectedCardIndex < HandCards.Num();
}

void UTCGHandWidget::RebuildHandDisplay()
{
    if (!CardContainer)
    {
        UE_LOG(LogTemp, Error, TEXT("CardContainer is null! Make sure it's bound in the widget blueprint."));
        return;
    }

    if (!CardWidgetClass)
    {
        UE_LOG(LogTemp, Error, TEXT("CardWidgetClass is not set! Please assign WBP_TCG_Card in the widget settings."));
        return;
    }

    // Clear existing widgets
    CardContainer->ClearChildren();
    SpawnedCardWidgets.Empty();

    UE_LOG(LogTemp, Log, TEXT("RebuildHandDisplay: Spawning %d card widgets"), HandCards.Num());

    // Spawn new card widgets for each card in hand
    for (int32 i = 0; i < HandCards.Num(); i++)
    {
        UUserWidget* CardWidget = SpawnCardWidget(HandCards[i], i);
        if (CardWidget)
        {
            SpawnedCardWidgets.Add(CardWidget);

            // Add to horizontal box
            UHorizontalBoxSlot* BoxSlot = CardContainer->AddChildToHorizontalBox(CardWidget);
            if (BoxSlot)
            {
                // Set padding/spacing
                BoxSlot->SetPadding(FMargin(CardSpacing / 2.0f, 0.0f, CardSpacing / 2.0f, 0.0f));
                BoxSlot->SetHorizontalAlignment(HAlign_Center);
                BoxSlot->SetVerticalAlignment(VAlign_Bottom);
            }
            else
            {
                UE_LOG(LogTemp, Warning, TEXT("Failed to add card widget to CardContainer at index %d"), i);
            }
        }
        else
        {
            UE_LOG(LogTemp, Warning, TEXT("SpawnCardWidget returned null at index %d"), i);
        }
    }
}

UUserWidget* UTCGHandWidget::SpawnCardWidget(const FCardData& CardData, int32 CardIndex)
{
    if (!CardWidgetClass)
    {
        return nullptr;
    }

    // Create the card widget
    UUserWidget* CardWidget = CreateWidget<UUserWidget>(GetWorld(), CardWidgetClass);
    if (!CardWidget)
    {
        UE_LOG(LogTemp, Error, TEXT("Failed to create card widget!"));
        return nullptr;
    }

    // If the card widget defines an exposed int variable named "CardIndex",
    // set it here so the BP can use it for click handling.
    {
        FProperty* Prop = CardWidget->GetClass()->FindPropertyByName(FName("CardIndex"));
        if (FIntProperty* IntProp = CastField<FIntProperty>(Prop))
        {
            IntProp->SetPropertyValue_InContainer(CardWidget, CardIndex);
        }
    }

    // Set the card data on the widget
    // This assumes your card widget has a "SetCardData" function
    // We'll call it via Blueprint interface
    UFunction* SetCardDataFunc = CardWidget->FindFunction(FName("SetCardData"));
    if (SetCardDataFunc)
    {
        struct FSetCardDataParams
        {
            FCardData CardData;
        };

        FSetCardDataParams Params;
        Params.CardData = CardData;
        CardWidget->ProcessEvent(SetCardDataFunc, &Params);
        UE_LOG(LogTemp, Verbose, TEXT("Called SetCardData on card widget: %s"), *CardData.CardName);
    }
    else
    {
        UE_LOG(LogTemp, Verbose, TEXT("SetCardData function not found on card widget class"));
    }

    // Setup click handler
    SetupCardClickHandler(CardWidget, CardIndex);

    return CardWidget;
}

void UTCGHandWidget::SetupCardClickHandler(UUserWidget* CardWidget, int32 CardIndex)
{
    if (!CardWidget)
    {
        return;
    }

    // Try to find a button component in the card widget
    // This is a simple approach - you might need to adjust based on your card widget structure
    TArray<UWidget*> AllWidgets;
    CardWidget->WidgetTree->GetAllWidgets(AllWidgets);

    for (UWidget* Widget : AllWidgets)
    {
        if (UButton* Button = Cast<UButton>(Widget))
        {
            // Bind click event
            // Note: This is tricky in C++ - might be better handled in Blueprint
            // For now, we'll leave this to be implemented in Blueprint
            break;
        }
    }
}
