// GCGLinkUnitSubsystem.cpp - Link Unit & Pilot Pairing System Implementation

#include "GCGLinkUnitSubsystem.h"
#include "GCGCardDatabase.h"
#include "../PlayerState/GCGPlayerState.h"

// ===========================================================================================
// INITIALIZATION
// ===========================================================================================

void UGCGLinkUnitSubsystem::Initialize(FSubsystemCollectionBase& Collection)
{
	Super::Initialize(Collection);

	UE_LOG(LogTemp, Log, TEXT("GCGLinkUnitSubsystem: Initialized"));

	// Cache reference to Card Database
	CardDatabase = GetGameInstance()->GetSubsystem<UGCGCardDatabase>();

	if (!CardDatabase)
	{
		UE_LOG(LogTemp, Warning, TEXT("GCGLinkUnitSubsystem: Card Database not found!"));
	}
}

void UGCGLinkUnitSubsystem::Deinitialize()
{
	CardDatabase = nullptr;
	Super::Deinitialize();
}

// ===========================================================================================
// PAIRING OPERATIONS
// ===========================================================================================

FGCGLinkResult UGCGLinkUnitSubsystem::PairPilotWithUnit(
	FGCGCardInstance& LinkUnitInstance,
	FGCGCardInstance& PilotInstance,
	const FGCGCardData* LinkUnitData,
	const FGCGCardData* PilotData)
{
	FGCGLinkResult Result;
	Result.LinkUnitInstanceID = LinkUnitInstance.InstanceID;
	Result.PilotInstanceID = PilotInstance.InstanceID;

	// Validate card data
	if (!LinkUnitData || !PilotData)
	{
		Result.bSuccess = false;
		Result.ErrorMessage = TEXT("Invalid card data");
		return Result;
	}

	// Validate that Link Unit is actually a Link Unit
	if (!LinkUnitData->HasKeyword(EGCGKeyword::LinkUnit))
	{
		Result.bSuccess = false;
		Result.ErrorMessage = FString::Printf(TEXT("%s is not a Link Unit"), *LinkUnitData->CardName.ToString());
		return Result;
	}

	// Validate that Pilot is actually a Pilot
	if (PilotData->CardType != EGCGCardType::Pilot)
	{
		Result.bSuccess = false;
		Result.ErrorMessage = FString::Printf(TEXT("%s is not a Pilot"), *PilotData->CardName.ToString());
		return Result;
	}

	// Validate that Link Unit is not already paired
	if (LinkUnitInstance.PairedCardInstanceID != -1)
	{
		Result.bSuccess = false;
		Result.ErrorMessage = FString::Printf(TEXT("%s is already paired"), *LinkUnitData->CardName.ToString());
		return Result;
	}

	// Validate that Pilot is not already paired
	if (PilotInstance.PairedCardInstanceID != -1)
	{
		Result.bSuccess = false;
		Result.ErrorMessage = FString::Printf(TEXT("%s is already paired"), *PilotData->CardName.ToString());
		return Result;
	}

	// Validate Link requirements
	FGCGLinkResult ValidationResult = ValidateLinkRequirement(LinkUnitInstance, PilotInstance, LinkUnitData, PilotData);
	if (!ValidationResult.bSuccess)
	{
		return ValidationResult; // Return validation failure
	}

	// Pair the cards
	LinkUnitInstance.PairedCardInstanceID = PilotInstance.InstanceID;
	PilotInstance.PairedCardInstanceID = LinkUnitInstance.InstanceID;

	// Link Units can attack on the turn they're deployed when paired
	Result.bCanAttackThisTurn = true;

	Result.bSuccess = true;
	Result.ErrorMessage = FString::Printf(
		TEXT("%s paired with %s"),
		*LinkUnitData->CardName.ToString(),
		*PilotData->CardName.ToString()
	);

	UE_LOG(LogTemp, Log, TEXT("GCGLinkUnitSubsystem: %s"), *Result.ErrorMessage);

	return Result;
}

FGCGLinkResult UGCGLinkUnitSubsystem::UnpairPilot(
	FGCGCardInstance& LinkUnitInstance,
	FGCGCardInstance& PilotInstance)
{
	FGCGLinkResult Result;
	Result.LinkUnitInstanceID = LinkUnitInstance.InstanceID;
	Result.PilotInstanceID = PilotInstance.InstanceID;

	// Validate that they are actually paired to each other
	if (LinkUnitInstance.PairedCardInstanceID != PilotInstance.InstanceID ||
		PilotInstance.PairedCardInstanceID != LinkUnitInstance.InstanceID)
	{
		Result.bSuccess = false;
		Result.ErrorMessage = TEXT("Cards are not paired to each other");
		return Result;
	}

	// Unpair
	LinkUnitInstance.PairedCardInstanceID = -1;
	PilotInstance.PairedCardInstanceID = -1;

	Result.bSuccess = true;
	Result.ErrorMessage = TEXT("Unpaired successfully");

	UE_LOG(LogTemp, Log, TEXT("GCGLinkUnitSubsystem: Unpaired Link Unit %d and Pilot %d"),
		LinkUnitInstance.InstanceID, PilotInstance.InstanceID);

	return Result;
}

// ===========================================================================================
// VALIDATION
// ===========================================================================================

FGCGLinkResult UGCGLinkUnitSubsystem::ValidateLinkRequirement(
	const FGCGCardInstance& LinkUnitInstance,
	const FGCGCardInstance& PilotInstance,
	const FGCGCardData* LinkUnitData,
	const FGCGCardData* PilotData) const
{
	FGCGLinkResult Result;
	Result.LinkUnitInstanceID = LinkUnitInstance.InstanceID;
	Result.PilotInstanceID = PilotInstance.InstanceID;

	// Validate card data
	if (!LinkUnitData || !PilotData)
	{
		Result.bSuccess = false;
		Result.ErrorMessage = TEXT("Invalid card data");
		return Result;
	}

	// Get Link requirements
	const FGCGLinkRequirement& Requirements = LinkUnitData->LinkRequirements;

	// If no requirements specified, any Pilot can pair
	if (Requirements.RequiredColors.Num() == 0 &&
		Requirements.RequiredTraits.Num() == 0 &&
		Requirements.SpecificCardNumbers.Num() == 0)
	{
		Result.bSuccess = true;
		Result.ErrorMessage = TEXT("No Link requirements - any Pilot can pair");
		return Result;
	}

	// Check specific card requirement (highest priority)
	if (Requirements.SpecificCardNumbers.Num() > 0)
	{
		if (ValidateSpecificCardRequirement(Requirements, PilotData))
		{
			Result.bSuccess = true;
			Result.ErrorMessage = TEXT("Specific card requirement met");
			return Result;
		}
		else
		{
			Result.bSuccess = false;
			Result.ErrorMessage = FString::Printf(
				TEXT("%s does not meet specific card requirement"),
				*PilotData->CardName.ToString()
			);
			return Result;
		}
	}

	// Check color requirement
	if (Requirements.RequiredColors.Num() > 0)
	{
		if (!ValidateColorRequirement(Requirements, PilotData))
		{
			Result.bSuccess = false;
			Result.ErrorMessage = FString::Printf(
				TEXT("%s does not meet color requirement"),
				*PilotData->CardName.ToString()
			);
			return Result;
		}
	}

	// Check trait requirement
	if (Requirements.RequiredTraits.Num() > 0)
	{
		if (!ValidateTraitRequirement(Requirements, PilotData))
		{
			Result.bSuccess = false;
			Result.ErrorMessage = FString::Printf(
				TEXT("%s does not meet trait requirement"),
				*PilotData->CardName.ToString()
			);
			return Result;
		}
	}

	// All requirements met
	Result.bSuccess = true;
	Result.ErrorMessage = TEXT("All Link requirements met");
	return Result;
}

bool UGCGLinkUnitSubsystem::IsPaired(const FGCGCardInstance& UnitInstance) const
{
	return UnitInstance.PairedCardInstanceID != -1;
}

bool UGCGLinkUnitSubsystem::CanLinkUnitAttackThisTurn(const FGCGCardInstance& LinkUnitInstance, int32 CurrentTurn) const
{
	// Link Units can attack on deploy turn if paired
	if (IsPaired(LinkUnitInstance))
	{
		return true; // Bypass summoning sickness
	}

	// Otherwise, normal summoning sickness rules apply
	return LinkUnitInstance.TurnDeployed < CurrentTurn;
}

// ===========================================================================================
// QUERY FUNCTIONS
// ===========================================================================================

FGCGCardInstance* UGCGLinkUnitSubsystem::GetPairedPilot(const FGCGCardInstance& LinkUnitInstance, AGCGPlayerState* PlayerState) const
{
	if (!PlayerState || LinkUnitInstance.PairedCardInstanceID == -1)
	{
		return nullptr;
	}

	// Search BattleArea for the paired Pilot
	for (FGCGCardInstance& Card : PlayerState->BattleArea)
	{
		if (Card.InstanceID == LinkUnitInstance.PairedCardInstanceID)
		{
			return &Card;
		}
	}

	return nullptr;
}

FGCGCardInstance* UGCGLinkUnitSubsystem::GetPairedLinkUnit(const FGCGCardInstance& PilotInstance, AGCGPlayerState* PlayerState) const
{
	if (!PlayerState || PilotInstance.PairedCardInstanceID == -1)
	{
		return nullptr;
	}

	// Search BattleArea for the paired Link Unit
	for (FGCGCardInstance& Card : PlayerState->BattleArea)
	{
		if (Card.InstanceID == PilotInstance.PairedCardInstanceID)
		{
			return &Card;
		}
	}

	return nullptr;
}

TArray<FGCGCardInstance*> UGCGLinkUnitSubsystem::GetAllLinkUnits(AGCGPlayerState* PlayerState) const
{
	TArray<FGCGCardInstance*> LinkUnits;

	if (!PlayerState || !CardDatabase)
	{
		return LinkUnits;
	}

	for (FGCGCardInstance& Card : PlayerState->BattleArea)
	{
		const FGCGCardData* CardData = CardDatabase->GetCardData(Card.CardNumber);
		if (CardData && CardData->HasKeyword(EGCGKeyword::LinkUnit))
		{
			LinkUnits.Add(&Card);
		}
	}

	return LinkUnits;
}

TArray<FGCGCardInstance*> UGCGLinkUnitSubsystem::GetAllPilots(AGCGPlayerState* PlayerState) const
{
	TArray<FGCGCardInstance*> Pilots;

	if (!PlayerState || !CardDatabase)
	{
		return Pilots;
	}

	for (FGCGCardInstance& Card : PlayerState->BattleArea)
	{
		const FGCGCardData* CardData = CardDatabase->GetCardData(Card.CardNumber);
		if (CardData && CardData->CardType == EGCGCardType::Pilot)
		{
			Pilots.Add(&Card);
		}
	}

	return Pilots;
}

// ===========================================================================================
// HELPER FUNCTIONS
// ===========================================================================================

bool UGCGLinkUnitSubsystem::ValidateColorRequirement(const FGCGLinkRequirement& Requirements, const FGCGCardData* PilotData) const
{
	if (!PilotData || Requirements.RequiredColors.Num() == 0)
	{
		return true; // No color requirement
	}

	// Pilot must have at least one of the required colors
	for (EGCGCardColor RequiredColor : Requirements.RequiredColors)
	{
		if (PilotData->Colors.Contains(RequiredColor))
		{
			return true;
		}
	}

	return false;
}

bool UGCGLinkUnitSubsystem::ValidateTraitRequirement(const FGCGLinkRequirement& Requirements, const FGCGCardData* PilotData) const
{
	if (!PilotData || Requirements.RequiredTraits.Num() == 0)
	{
		return true; // No trait requirement
	}

	// Pilot must have ALL required traits
	for (const FName& RequiredTrait : Requirements.RequiredTraits)
	{
		if (!PilotData->HasTrait(RequiredTrait))
		{
			return false;
		}
	}

	return true;
}

bool UGCGLinkUnitSubsystem::ValidateSpecificCardRequirement(const FGCGLinkRequirement& Requirements, const FGCGCardData* PilotData) const
{
	if (!PilotData || Requirements.SpecificCardNumbers.Num() == 0)
	{
		return true; // No specific card requirement
	}

	// Pilot must be one of the specific cards
	return Requirements.SpecificCardNumbers.Contains(PilotData->CardNumber);
}
