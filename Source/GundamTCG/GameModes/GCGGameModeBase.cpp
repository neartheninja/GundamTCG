// GCGGameModeBase.cpp - Base Game Mode Implementation
// Unreal Engine 5.6 - Gundam TCG Implementation

#include "GCGGameModeBase.h"
#include "GundamTCG/GameState/GCGGameState.h"
#include "GundamTCG/Subsystems/GCGCardDatabase.h"
#include "GameFramework/PlayerState.h"
#include "GameFramework/PlayerController.h"
#include "Engine/DataTable.h"
#include "Kismet/GameplayStatics.h"

AGCGGameModeBase::AGCGGameModeBase()
{
	// Set default classes (will be overridden in Blueprint)
	GameStateClass = AGCGGameState::StaticClass();

	// Initialize instance ID counter
	NextInstanceID = 1;

	// Enable replication
	bReplicates = true;
}

void AGCGGameModeBase::BeginPlay()
{
	Super::BeginPlay();

	// Initialize card database subsystem
	UGCGCardDatabase* CardDB = GetGameInstance()->GetSubsystem<UGCGCardDatabase>();
	if (CardDB)
	{
		// If CardDatabase DataTable is set in the GameMode, pass it to the subsystem
		if (CardDatabase)
		{
			CardDB->SetCardDataTable(CardDatabase);
			UE_LOG(LogTemp, Log, TEXT("AGCGGameModeBase: Set card database DataTable on subsystem"));
		}

		UE_LOG(LogTemp, Log, TEXT("AGCGGameModeBase: %s"), *CardDB->GetDatabaseStats());
	}
	else
	{
		UE_LOG(LogTemp, Error, TEXT("AGCGGameModeBase: Card Database subsystem not found!"));
	}

	// Call Blueprint event
	OnGameInitialized();
}

void AGCGGameModeBase::PostLogin(APlayerController* NewPlayer)
{
	Super::PostLogin(NewPlayer);

	if (NewPlayer)
	{
		// Get player state
		APlayerState* PS = NewPlayer->PlayerState;
		if (PS)
		{
			int32 PlayerID = PS->GetPlayerId();
			UE_LOG(LogTemp, Log, TEXT("AGCGGameModeBase: Player %d joined the game"), PlayerID);

			// Call Blueprint event
			OnPlayerJoined(PlayerID);
		}
	}
}

void AGCGGameModeBase::Logout(AController* Exiting)
{
	if (Exiting)
	{
		APlayerState* PS = Exiting->PlayerState;
		if (PS)
		{
			int32 PlayerID = PS->GetPlayerId();
			UE_LOG(LogTemp, Log, TEXT("AGCGGameModeBase: Player %d left the game"), PlayerID);

			// Call Blueprint event
			OnPlayerLeft(PlayerID);
		}
	}

	Super::Logout(Exiting);
}

// ===== CARD DATABASE =====

const FGCGCardData* AGCGGameModeBase::GetCardData(FName CardNumber) const
{
	// Use the Card Database subsystem
	UGCGCardDatabase* CardDB = GetGameInstance()->GetSubsystem<UGCGCardDatabase>();
	if (!CardDB)
	{
		UE_LOG(LogTemp, Error, TEXT("AGCGGameModeBase::GetCardData: Card Database subsystem not found"));
		return nullptr;
	}

	return CardDB->GetCardData(CardNumber);
}

bool AGCGGameModeBase::CardExists(FName CardNumber) const
{
	UGCGCardDatabase* CardDB = GetGameInstance()->GetSubsystem<UGCGCardDatabase>();
	if (!CardDB)
	{
		return false;
	}

	return CardDB->CardExists(CardNumber);
}

// ===== PLAYER MANAGEMENT =====

AGCGPlayerState* AGCGGameModeBase::GetPlayerStateByID(int32 PlayerID) const
{
	// Iterate through all player states
	for (TActorIterator<APlayerState> It(GetWorld()); It; ++It)
	{
		APlayerState* PS = *It;
		if (PS && PS->GetPlayerId() == PlayerID)
		{
			return Cast<AGCGPlayerState>(PS);
		}
	}

	return nullptr;
}

AGCGPlayerController* AGCGGameModeBase::GetPlayerControllerByID(int32 PlayerID) const
{
	// Iterate through all player controllers
	for (TActorIterator<APlayerController> It(GetWorld()); It; ++It)
	{
		APlayerController* PC = *It;
		if (PC && PC->PlayerState && PC->PlayerState->GetPlayerId() == PlayerID)
		{
			return Cast<AGCGPlayerController>(PC);
		}
	}

	return nullptr;
}

TArray<AGCGPlayerState*> AGCGGameModeBase::GetAllPlayerStates() const
{
	TArray<AGCGPlayerState*> PlayerStates;

	for (TActorIterator<APlayerState> It(GetWorld()); It; ++It)
	{
		AGCGPlayerState* GCGPS = Cast<AGCGPlayerState>(*It);
		if (GCGPS)
		{
			PlayerStates.Add(GCGPS);
		}
	}

	return PlayerStates;
}

// ===== GAME STATE ACCESS =====

AGCGGameState* AGCGGameModeBase::GetGCGGameState() const
{
	return Cast<AGCGGameState>(GameState);
}

// ===== UTILITY FUNCTIONS =====

FGCGCardInstance AGCGGameModeBase::CreateCardInstance(FName CardNumber, int32 OwnerPlayerID, bool bIsToken)
{
	FGCGCardInstance NewInstance;

	// Generate unique instance ID
	NewInstance.InstanceID = GenerateInstanceID();
	NewInstance.CardNumber = CardNumber;
	NewInstance.OwnerPlayerID = OwnerPlayerID;
	NewInstance.ControllerPlayerID = OwnerPlayerID;
	NewInstance.bIsToken = bIsToken;
	NewInstance.CurrentZone = EGCGCardZone::None;
	NewInstance.bIsActive = true;
	NewInstance.CurrentDamage = 0;
	NewInstance.TurnDeployed = 0;
	NewInstance.bHasAttackedThisTurn = false;
	NewInstance.ActivationCountThisTurn = 0;

	// Get card data from database
	const FGCGCardData* CardData = GetCardData(CardNumber);
	if (CardData)
	{
		// Copy card data to instance
		NewInstance.CardName = CardData->CardName;
		NewInstance.CardType = CardData->CardType;
		NewInstance.Colors = CardData->Colors;
		NewInstance.Level = CardData->Level;
		NewInstance.Cost = CardData->Cost;
		NewInstance.AP = CardData->AP;
		NewInstance.HP = CardData->HP;
		NewInstance.Keywords = CardData->Keywords;
		// Effects and modifiers are added dynamically during gameplay
	}
	else
	{
		UE_LOG(LogTemp, Warning, TEXT("AGCGGameModeBase::CreateCardInstance: Card data not found for '%s'"), *CardNumber.ToString());
	}

	UE_LOG(LogTemp, Verbose, TEXT("AGCGGameModeBase::CreateCardInstance: Created instance %d for card '%s' (Owner: %d, Token: %d)"),
		NewInstance.InstanceID, *CardNumber.ToString(), OwnerPlayerID, bIsToken ? 1 : 0);

	return NewInstance;
}

FGCGCardInstance AGCGGameModeBase::CreateTokenInstance(FName TokenType, int32 OwnerPlayerID)
{
	// Create token instance (stats are loaded from card database token definitions)
	FGCGCardInstance TokenInstance = CreateCardInstance(TokenType, OwnerPlayerID, true);
	TokenInstance.TokenType = TokenType;

	UE_LOG(LogTemp, Log, TEXT("AGCGGameModeBase::CreateTokenInstance: Created %s token (ID: %d, AP: %d, HP: %d)"),
		*TokenType.ToString(), TokenInstance.InstanceID, TokenInstance.AP, TokenInstance.HP);

	return TokenInstance;
}

// ===== INSTANCE ID GENERATION =====

int32 AGCGGameModeBase::GenerateInstanceID()
{
	return NextInstanceID++;
}
