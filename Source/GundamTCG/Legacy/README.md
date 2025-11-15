# Legacy One Piece TCG Code

This folder contains the original One Piece TCG implementation code that was created before the Gundam TCG refactor.

**DO NOT USE THESE FILES** - They are kept for reference only.

## Files in this folder:
- TCGTypes.h - Old One Piece type definitions
- TCGGameMode.h/cpp - Old game mode implementation
- TCGPlayerController.h/cpp - Old player controller
- TCGPlayerState.h/cpp - Old player state
- TCGHandWidget.h/cpp - Old hand widget

## Current Gundam TCG Code:
Use the following files instead:
- Source/OnePieceTCG_V2/GCGTypes.h - Gundam type definitions
- Source/GundamTCG/GameModes/GCGGameModeBase.h/cpp - New game mode base
- Source/GundamTCG/GameModes/GCGGameMode_1v1.h/cpp - New 1v1 game mode
- Source/GundamTCG/GameState/GCGGameState.h/cpp - New game state
- Source/GundamTCG/PlayerState/GCGPlayerState.h/cpp - New player state
- Source/GundamTCG/Subsystems/* - All new subsystem implementations

These files may be deleted in the future once confirmed they are not needed.
