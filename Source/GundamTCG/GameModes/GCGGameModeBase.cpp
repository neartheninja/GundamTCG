// GCGGameModeBase.cpp - Base Game Mode Implementation
// Unreal Engine 5.6 - Gundam TCG Implementation

#include "GCGGameModeBase.h"
#include "GundamTCG/GameState/GCGGameState.h"
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

	// Validate card database
	if (!CardDatabase)
	{
		UE_LOG(LogTemp, Error, TEXT("AGCGGameModeBase: CardDatabase is not set! Please assign a DataTable in the editor."));
	}
	else
	{
		UE_LOG(LogTemp, Log, TEXT("AGCGGameModeBase: Card database loaded with %d cards"), CardDatabase->GetRowMap().Num());
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
	if (!CardDatabase)
	{
		UE_LOG(LogTemp, Warning, TEXT("AGCGGameModeBase::GetCardData: CardDatabase is null"));
		return nullptr;
	}

	// Look up card in DataTable
	const FGCGCardData* CardData = CardDatabase->FindRow<FGCGCardData>(CardNumber, TEXT("GetCardData"));

	if (!CardData)
	{
		UE_LOG(LogTemp, Warning, TEXT("AGCGGameModeBase::GetCardData: Card '%s' not found in database"), *CardNumber.ToString());
	}

	return CardData;
}

bool AGCGGameModeBase::CardExists(FName CardNumber) const
{
	return GetCardData(CardNumber) != nullptr;
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
	NewInstance.DamageCounters = 0;
	NewInstance.PairedCardInstanceID = 0;
	NewInstance.TurnDeployed = 0;
	NewInstance.bHasAttackedThisTurn = false;
	NewInstance.ActivationCountThisTurn = 0;

	UE_LOG(LogTemp, Verbose, TEXT("AGCGGameModeBase::CreateCardInstance: Created instance %d for card '%s' (Owner: %d, Token: %d)"),
		NewInstance.InstanceID, *CardNumber.ToString(), OwnerPlayerID, bIsToken ? 1 : 0);

	return NewInstance;
}

FGCGCardInstance AGCGGameModeBase::CreateTokenInstance(FName TokenType, int32 OwnerPlayerID)
{
	FGCGCardInstance TokenInstance = CreateCardInstance(TokenType, OwnerPlayerID, true);
	TokenInstance.TokenType = TokenType;

	// Set default stats for common tokens
	if (TokenType == FName("EXBase"))
	{
		// EX Base: 0 AP, 3 HP
		// Note: Actual stats come from card data if it exists
		UE_LOG(LogTemp, Log, TEXT("AGCGGameModeBase::CreateTokenInstance: Created EX Base token (ID: %d)"), TokenInstance.InstanceID);
	}
	else if (TokenType == FName("EXResource"))
	{
		// EX Resource: temporary resource token
		UE_LOG(LogTemp, Log, TEXT("AGCGGameModeBase::CreateTokenInstance: Created EX Resource token (ID: %d)"), TokenInstance.InstanceID);
	}

	return TokenInstance;
}

// ===== INSTANCE ID GENERATION =====

int32 AGCGGameModeBase::GenerateInstanceID()
{
	return NextInstanceID++;
}
