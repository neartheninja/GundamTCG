// GCGLinkUnitSubsystem.h - Link Unit & Pilot Pairing System
// Handles pairing Pilots with Link Units and Link requirement validation

#pragma once

#include "CoreMinimal.h"
#include "Subsystems/GameInstanceSubsystem.h"
#include "GCGTypes.h"
#include "GCGLinkUnitSubsystem.generated.h"

// Forward declarations
class UGCGCardDatabase;
class AGCGPlayerState;

/**
 * Result of a Link Unit operation
 */
USTRUCT(BlueprintType)
struct FGCGLinkResult
{
	GENERATED_BODY()

	UPROPERTY(BlueprintReadWrite, Category = "Link Unit")
	bool bSuccess = false;

	UPROPERTY(BlueprintReadWrite, Category = "Link Unit")
	FString ErrorMessage;

	UPROPERTY(BlueprintReadWrite, Category = "Link Unit")
	int32 LinkUnitInstanceID = -1;

	UPROPERTY(BlueprintReadWrite, Category = "Link Unit")
	int32 PilotInstanceID = -1;

	UPROPERTY(BlueprintReadWrite, Category = "Link Unit")
	bool bCanAttackThisTurn = false; // Link Units can attack on deploy turn
};

/**
 * Link Unit & Pilot Pairing Subsystem
 *
 * Manages the pairing of Pilots with Link Units and validates Link requirements.
 * Link Units can attack on the turn they're deployed when paired with a valid Pilot.
 */
UCLASS()
class GUNDAMTCG_API UGCGLinkUnitSubsystem : public UGameInstanceSubsystem
{
	GENERATED_BODY()

public:
	// ===========================================================================================
	// INITIALIZATION
	// ===========================================================================================

	virtual void Initialize(FSubsystemCollectionBase& Collection) override;
	virtual void Deinitialize() override;

	// ===========================================================================================
	// PAIRING OPERATIONS
	// ===========================================================================================

	/**
	 * Pair a Pilot with a Link Unit
	 *
	 * @param LinkUnitInstance - The Link Unit card instance
	 * @param PilotInstance - The Pilot card instance
	 * @param LinkUnitData - Static card data for the Link Unit
	 * @param PilotData - Static card data for the Pilot
	 * @return Result with success/failure and pairing details
	 */
	UFUNCTION(BlueprintCallable, Category = "Link Unit")
	FGCGLinkResult PairPilotWithUnit(
		UPARAM(ref) FGCGCardInstance& LinkUnitInstance,
		UPARAM(ref) FGCGCardInstance& PilotInstance,
		const FGCGCardData* LinkUnitData,
		const FGCGCardData* PilotData
	);

	/**
	 * Unpair a Pilot from a Link Unit
	 *
	 * @param LinkUnitInstance - The Link Unit to unpair
	 * @param PilotInstance - The Pilot to unpair
	 * @return Result with success/failure
	 */
	UFUNCTION(BlueprintCallable, Category = "Link Unit")
	FGCGLinkResult UnpairPilot(
		UPARAM(ref) FGCGCardInstance& LinkUnitInstance,
		UPARAM(ref) FGCGCardInstance& PilotInstance
	);

	// ===========================================================================================
	// VALIDATION
	// ===========================================================================================

	/**
	 * Validate if a Pilot can pair with a Link Unit
	 *
	 * @param LinkUnitInstance - The Link Unit instance
	 * @param PilotInstance - The Pilot instance
	 * @param LinkUnitData - Static card data for the Link Unit
	 * @param PilotData - Static card data for the Pilot
	 * @return Result with success/failure and validation details
	 */
	UFUNCTION(BlueprintPure, Category = "Link Unit")
	FGCGLinkResult ValidateLinkRequirement(
		const FGCGCardInstance& LinkUnitInstance,
		const FGCGCardInstance& PilotInstance,
		const FGCGCardData* LinkUnitData,
		const FGCGCardData* PilotData
	) const;

	/**
	 * Check if a Unit is currently paired with a Pilot
	 *
	 * @param UnitInstance - The Unit to check
	 * @return True if paired with a Pilot
	 */
	UFUNCTION(BlueprintPure, Category = "Link Unit")
	bool IsPaired(const FGCGCardInstance& UnitInstance) const;

	/**
	 * Check if a Link Unit can attack this turn (bypasses summoning sickness when paired)
	 *
	 * @param LinkUnitInstance - The Link Unit to check
	 * @param CurrentTurn - The current turn number
	 * @return True if can attack
	 */
	UFUNCTION(BlueprintPure, Category = "Link Unit")
	bool CanLinkUnitAttackThisTurn(const FGCGCardInstance& LinkUnitInstance, int32 CurrentTurn) const;

	// ===========================================================================================
	// QUERY FUNCTIONS
	// ===========================================================================================

	/**
	 * Get the Pilot paired with a Link Unit
	 *
	 * @param LinkUnitInstance - The Link Unit
	 * @param PlayerState - The player state containing cards
	 * @return Pointer to the paired Pilot instance, or nullptr if not paired
	 */
	UFUNCTION(BlueprintPure, Category = "Link Unit")
	FGCGCardInstance* GetPairedPilot(const FGCGCardInstance& LinkUnitInstance, AGCGPlayerState* PlayerState) const;

	/**
	 * Get the Link Unit paired with a Pilot
	 *
	 * @param PilotInstance - The Pilot
	 * @param PlayerState - The player state containing cards
	 * @return Pointer to the paired Link Unit instance, or nullptr if not paired
	 */
	UFUNCTION(BlueprintPure, Category = "Link Unit")
	FGCGCardInstance* GetPairedLinkUnit(const FGCGCardInstance& PilotInstance, AGCGPlayerState* PlayerState) const;

	/**
	 * Get all Link Units in a player's Battle Area
	 *
	 * @param PlayerState - The player state
	 * @return Array of Link Unit instances
	 */
	UFUNCTION(BlueprintPure, Category = "Link Unit")
	TArray<FGCGCardInstance*> GetAllLinkUnits(AGCGPlayerState* PlayerState) const;

	/**
	 * Get all Pilots in a player's Battle Area
	 *
	 * @param PlayerState - The player state
	 * @return Array of Pilot instances
	 */
	UFUNCTION(BlueprintPure, Category = "Link Unit")
	TArray<FGCGCardInstance*> GetAllPilots(AGCGPlayerState* PlayerState) const;

	// ===========================================================================================
	// HELPER FUNCTIONS
	// ===========================================================================================

private:
	/**
	 * Validate color requirement for Link pairing
	 *
	 * @param Requirements - The Link requirements
	 * @param PilotData - The Pilot card data
	 * @return True if color requirement met
	 */
	bool ValidateColorRequirement(const FGCGLinkRequirement& Requirements, const FGCGCardData* PilotData) const;

	/**
	 * Validate trait requirement for Link pairing
	 *
	 * @param Requirements - The Link requirements
	 * @param PilotData - The Pilot card data
	 * @return True if trait requirement met
	 */
	bool ValidateTraitRequirement(const FGCGLinkRequirement& Requirements, const FGCGCardData* PilotData) const;

	/**
	 * Validate specific card requirement for Link pairing
	 *
	 * @param Requirements - The Link requirements
	 * @param PilotData - The Pilot card data
	 * @return True if specific card requirement met
	 */
	bool ValidateSpecificCardRequirement(const FGCGLinkRequirement& Requirements, const FGCGCardData* PilotData) const;

	// ===========================================================================================
	// PROPERTIES
	// ===========================================================================================

	// Cached reference to Card Database
	UPROPERTY()
	UGCGCardDatabase* CardDatabase = nullptr;
};
