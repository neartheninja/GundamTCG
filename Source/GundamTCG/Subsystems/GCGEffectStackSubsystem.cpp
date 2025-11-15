// GCGEffectStackSubsystem.cpp - Effect Stack & Priority Resolution System Implementation
// Unreal Engine 5.6 - Gundam TCG Implementation

#include "GCGEffectStackSubsystem.h"
#include "GundamTCG/GameState/GCGGameState.h"
#include "GundamTCG/PlayerState/GCGPlayerState.h"
#include "GundamTCG/Subsystems/GCGEffectSubsystem.h"
#include "Engine/World.h"

// ===========================================================================================
// INITIALIZATION
// ===========================================================================================

void UGCGEffectStackSubsystem::Initialize(FSubsystemCollectionBase& Collection)
{
	Super::Initialize(Collection);

	StackIndexCounter = 0;
	EffectStack.Empty();
	DuringThisTurnEffects.Empty();

	UE_LOG(LogTemp, Log, TEXT("GCGEffectStackSubsystem initialized"));
}

void UGCGEffectStackSubsystem::Deinitialize()
{
	EffectStack.Empty();
	DuringThisTurnEffects.Empty();

	Super::Deinitialize();
}

// ===========================================================================================
// STACK MANAGEMENT
// ===========================================================================================

FGCGEffectStackEntry UGCGEffectStackSubsystem::PushEffect(
	int32 SourceCardInstanceID,
	int32 OwnerPlayerID,
	const FGCGEffectData& EffectData,
	EGCGEffectPriority Priority,
	const TArray<int32>& AffectedUnits)
{
	FGCGEffectStackEntry Entry;
	Entry.SourceCardInstanceID = SourceCardInstanceID;
	Entry.OwnerPlayerID = OwnerPlayerID;
	Entry.EffectData = EffectData;
	Entry.Priority = Priority;
	Entry.StackIndex = StackIndexCounter++;
	Entry.bResolved = false;
	Entry.AffectedUnitInstanceIDs = AffectedUnits;
	Entry.Timestamp = GetWorld()->GetTimeSeconds();

	// Add to stack
	EffectStack.Add(Entry);

	// FAQ Q109: New effects interrupt and resolve first
	// Sort stack to put higher priority effects on top
	SortStackByPriority();

	UE_LOG(LogTemp, Log, TEXT("[Effect Stack] Pushed effect from source %d (Priority: %d, Stack size: %d)"),
		SourceCardInstanceID, static_cast<int32>(Priority), EffectStack.Num());

	return Entry;
}

FGCGEffectStackEntry UGCGEffectStackSubsystem::PopEffect()
{
	if (EffectStack.Num() == 0)
	{
		UE_LOG(LogTemp, Warning, TEXT("[Effect Stack] Attempted to pop from empty stack"));
		return FGCGEffectStackEntry();
	}

	// Pop from end of array (top of stack)
	FGCGEffectStackEntry TopEntry = EffectStack.Last();
	EffectStack.RemoveAt(EffectStack.Num() - 1);

	UE_LOG(LogTemp, Log, TEXT("[Effect Stack] Popped effect from source %d (Stack size: %d)"),
		TopEntry.SourceCardInstanceID, EffectStack.Num());

	return TopEntry;
}

FGCGEffectStackEntry UGCGEffectStackSubsystem::PeekTopEffect() const
{
	if (EffectStack.Num() == 0)
	{
		return FGCGEffectStackEntry();
	}

	return EffectStack.Last();
}

bool UGCGEffectStackSubsystem::IsStackEmpty() const
{
	return EffectStack.Num() == 0;
}

int32 UGCGEffectStackSubsystem::GetStackSize() const
{
	return EffectStack.Num();
}

void UGCGEffectStackSubsystem::ClearStack()
{
	EffectStack.Empty();
	UE_LOG(LogTemp, Log, TEXT("[Effect Stack] Stack cleared"));
}

// ===========================================================================================
// RESOLUTION
// ===========================================================================================

bool UGCGEffectStackSubsystem::ResolveStack(AGCGGameState* GameState)
{
	if (!GameState)
	{
		UE_LOG(LogTemp, Error, TEXT("[Effect Stack] Cannot resolve stack: GameState is null"));
		return false;
	}

	UE_LOG(LogTemp, Log, TEXT("[Effect Stack] Resolving stack (%d effects)"), EffectStack.Num());

	// FAQ Q109: Process effects until stack is empty
	// New effects can be added during resolution, which will be inserted with priority
	while (!IsStackEmpty())
	{
		// FAQ Q107-Q108: Active player's effects resolve first
		// But since we're using priority sorting, this is handled by SortStackByPriority()

		if (!ResolveSingleEffect(GameState))
		{
			UE_LOG(LogTemp, Warning, TEXT("[Effect Stack] Effect resolution failed, continuing..."));
			// Continue even if one effect fails (FAQ Q111: effects resolve even if source leaves)
		}
	}

	UE_LOG(LogTemp, Log, TEXT("[Effect Stack] Stack resolution complete"));
	return true;
}

bool UGCGEffectStackSubsystem::ResolveSingleEffect(AGCGGameState* GameState)
{
	if (!GameState)
	{
		return false;
	}

	if (IsStackEmpty())
	{
		return false;
	}

	// Pop top effect
	FGCGEffectStackEntry Entry = PopEffect();

	// FAQ Q111: Effects resolve even if source card leaves field
	// We don't check if source still exists, we just execute the effect

	UE_LOG(LogTemp, Log, TEXT("[Effect Stack] Resolving effect from source %d"), Entry.SourceCardInstanceID);

	// Execute effect
	bool bSuccess = ExecuteEffectInternal(Entry, GameState);

	// Mark as resolved
	Entry.bResolved = true;

	// FAQ Q106: Track "during this turn" effects
	if (Entry.EffectData.Description.ToString().Contains(TEXT("during this turn"), ESearchCase::IgnoreCase))
	{
		TrackDuringThisTurnEffect(Entry, GameState->TurnNumber);
	}

	return bSuccess;
}

void UGCGEffectStackSubsystem::SortStackByPriority()
{
	// Sort by priority (higher priority first) then by stack index (lower index first)
	// FAQ Q110: Burst effects get priority
	// FAQ Q112: Negation effects get priority
	EffectStack.Sort(&UGCGEffectStackSubsystem::CompareEffectPriority);
}

TMap<int32, TArray<FGCGEffectStackEntry>> UGCGEffectStackSubsystem::GroupEffectsByPlayer(int32 ActivePlayerID)
{
	TMap<int32, TArray<FGCGEffectStackEntry>> GroupedEffects;

	for (const FGCGEffectStackEntry& Entry : EffectStack)
	{
		if (!GroupedEffects.Contains(Entry.OwnerPlayerID))
		{
			GroupedEffects.Add(Entry.OwnerPlayerID, TArray<FGCGEffectStackEntry>());
		}

		GroupedEffects[Entry.OwnerPlayerID].Add(Entry);
	}

	// FAQ Q107-Q108: Active player resolves effects first
	// (This is for UI display and manual resolution if needed)

	return GroupedEffects;
}

// ===========================================================================================
// PRIORITY HANDLING
// ===========================================================================================

EGCGEffectPriority UGCGEffectStackSubsystem::GetEffectPriority(const FGCGEffectData& EffectData)
{
	// FAQ Q112: Negation effects have priority
	if (IsNegationEffect(EffectData))
	{
		return EGCGEffectPriority::Negation;
	}

	// FAQ Q110: Burst effects get priority
	if (EffectData.Timing == EGCGEffectTiming::Burst)
	{
		return EGCGEffectPriority::Burst;
	}

	// Triggered effects (OnDeploy, OnAttack, etc.)
	if (EffectData.Timing == EGCGEffectTiming::OnDeploy ||
		EffectData.Timing == EGCGEffectTiming::OnAttack ||
		EffectData.Timing == EGCGEffectTiming::OnBlock ||
		EffectData.Timing == EGCGEffectTiming::OnDestroyed ||
		EffectData.Timing == EGCGEffectTiming::WhenPaired)
	{
		return EGCGEffectPriority::Trigger;
	}

	return EGCGEffectPriority::Normal;
}

bool UGCGEffectStackSubsystem::IsNegationEffect(const FGCGEffectData& EffectData)
{
	// Check if effect text contains negation keywords
	FString EffectText = EffectData.Description.ToString();

	// Common negation phrases
	if (EffectText.Contains(TEXT("negate"), ESearchCase::IgnoreCase) ||
		EffectText.Contains(TEXT("prevent"), ESearchCase::IgnoreCase) ||
		EffectText.Contains(TEXT("can't"), ESearchCase::IgnoreCase) ||
		EffectText.Contains(TEXT("cannot"), ESearchCase::IgnoreCase))
	{
		return true;
	}

	return false;
}

bool UGCGEffectStackSubsystem::IsContinuousEffect(const FGCGEffectData& EffectData)
{
	// FAQ Q105: Continuous effects only affect Units in play at activation
	// Check if this is a continuous effect that modifies Units

	for (const FGCGEffectOperation& Operation : EffectData.Operations)
	{
		// Continuous effects typically have WhileInPlay or UntilEndOfTurn duration
		if (Operation.Duration == EGCGModifierDuration::WhileInPlay ||
			Operation.Duration == EGCGModifierDuration::UntilEndOfTurn ||
			Operation.Duration == EGCGModifierDuration::UntilEndOfBattle)
		{
			// And they target Units
			if (Operation.TargetScope == EGCGTargetScope::YourUnits ||
				Operation.TargetScope == EGCGTargetScope::FriendlyUnits ||
				Operation.TargetScope == EGCGTargetScope::EnemyUnits ||
				Operation.TargetScope == EGCGTargetScope::AllUnits)
			{
				return true;
			}
		}
	}

	return false;
}

// ===========================================================================================
// SNAPSHOT MANAGEMENT (FAQ Q105)
// ===========================================================================================

TArray<int32> UGCGEffectStackSubsystem::TakeUnitSnapshot(const FGCGEffectData& EffectData, AGCGGameState* GameState)
{
	TArray<int32> Snapshot;

	if (!GameState)
	{
		return Snapshot;
	}

	// FAQ Q105: "All your Units get AP+2" only affects Units in play NOW
	// Take snapshot of all Units currently in play that would be affected

	for (APlayerState* PS : GameState->PlayerArray)
	{
		AGCGPlayerState* PlayerState = Cast<AGCGPlayerState>(PS);
		if (PlayerState)
		{
			for (const FGCGCardInstance& Unit : PlayerState->BattleArea)
			{
				// Add to snapshot
				// TODO: Filter by effect's target scope
				Snapshot.Add(Unit.InstanceID);
			}
		}
	}

	UE_LOG(LogTemp, Log, TEXT("[Effect Stack] Took Unit snapshot: %d Units"), Snapshot.Num());

	return Snapshot;
}

bool UGCGEffectStackSubsystem::IsUnitInSnapshot(int32 UnitInstanceID, const FGCGEffectStackEntry& EffectEntry)
{
	return EffectEntry.AffectedUnitInstanceIDs.Contains(UnitInstanceID);
}

// ===========================================================================================
// DURATION TRACKING (FAQ Q106)
// ===========================================================================================

void UGCGEffectStackSubsystem::TrackDuringThisTurnEffect(const FGCGEffectStackEntry& EffectEntry, int32 TurnNumber)
{
	// FAQ Q106: "During this turn" effects persist even if source is destroyed

	if (!DuringThisTurnEffects.Contains(TurnNumber))
	{
		DuringThisTurnEffects.Add(TurnNumber, TArray<FGCGEffectStackEntry>());
	}

	DuringThisTurnEffects[TurnNumber].Add(EffectEntry);

	UE_LOG(LogTemp, Log, TEXT("[Effect Stack] Tracked 'during this turn' effect for turn %d"), TurnNumber);
}

void UGCGEffectStackSubsystem::CleanupExpiredTurnEffects(int32 TurnNumber)
{
	// Remove effects from turns that have ended
	TArray<int32> TurnsToRemove;

	for (const TPair<int32, TArray<FGCGEffectStackEntry>>& Pair : DuringThisTurnEffects)
	{
		if (Pair.Key < TurnNumber)
		{
			TurnsToRemove.Add(Pair.Key);
		}
	}

	for (int32 Turn : TurnsToRemove)
	{
		DuringThisTurnEffects.Remove(Turn);
		UE_LOG(LogTemp, Log, TEXT("[Effect Stack] Cleaned up expired turn %d effects"), Turn);
	}
}

// ===========================================================================================
// DEBUG
// ===========================================================================================

void UGCGEffectStackSubsystem::PrintStack() const
{
	UE_LOG(LogTemp, Log, TEXT("========== EFFECT STACK =========="));
	UE_LOG(LogTemp, Log, TEXT("Stack size: %d"), EffectStack.Num());

	for (int32 i = EffectStack.Num() - 1; i >= 0; i--)
	{
		const FGCGEffectStackEntry& Entry = EffectStack[i];
		UE_LOG(LogTemp, Log, TEXT("[%d] Source: %d, Owner: %d, Priority: %d"),
			i, Entry.SourceCardInstanceID, Entry.OwnerPlayerID, static_cast<int32>(Entry.Priority));
	}

	UE_LOG(LogTemp, Log, TEXT("=================================="));
}

TArray<FGCGEffectStackEntry> UGCGEffectStackSubsystem::GetStackAsArray() const
{
	return EffectStack;
}

// ===========================================================================================
// INTERNAL HELPERS
// ===========================================================================================

bool UGCGEffectStackSubsystem::CompareEffectPriority(const FGCGEffectStackEntry& A, const FGCGEffectStackEntry& B)
{
	// Sort by priority first (higher priority = resolves first)
	if (A.Priority != B.Priority)
	{
		return static_cast<int32>(A.Priority) > static_cast<int32>(B.Priority);
	}

	// If same priority, sort by stack index (lower index = added earlier = resolves first)
	return A.StackIndex < B.StackIndex;
}

bool UGCGEffectStackSubsystem::ExecuteEffectInternal(const FGCGEffectStackEntry& EffectEntry, AGCGGameState* GameState)
{
	if (!GameState)
	{
		return false;
	}

	// Get effect subsystem
	UGCGEffectSubsystem* EffectSubsystem = GetGameInstance()->GetSubsystem<UGCGEffectSubsystem>();
	if (!EffectSubsystem)
	{
		UE_LOG(LogTemp, Error, TEXT("[Effect Stack] EffectSubsystem not found"));
		return false;
	}

	// Execute the effect using the effect subsystem
	// TODO: This requires the effect subsystem to expose an ExecuteEffect function
	// For now, log that we would execute
	UE_LOG(LogTemp, Log, TEXT("[Effect Stack] Executing effect from source %d (Owner: %d)"),
		EffectEntry.SourceCardInstanceID, EffectEntry.OwnerPlayerID);

	// FAQ Q105: If continuous effect, only affect Units in snapshot
	if (IsContinuousEffect(EffectEntry.EffectData))
	{
		UE_LOG(LogTemp, Log, TEXT("[Effect Stack] Continuous effect: affecting %d Units from snapshot"),
			EffectEntry.AffectedUnitInstanceIDs.Num());
		// TODO: Filter execution to only affect Units in snapshot
	}

	return true;
}
