# GundamTCG Migration Guide
## From One Piece TCG to Gundam TCG

This guide will walk you through the complete migration process to clean up One Piece TCG assets and rename the project to GundamTCG.

---

## Overview

**Current State:**
- Project Name: OnePieceTCG_V2
- Module Name: OnePieceTCG_V2
- Total Size: 864 MB
- One Piece Assets: ~277 MB (32%)

**Target State:**
- Project Name: GundamTCG
- Module Name: GundamTCG
- Estimated Size: ~587 MB
- Clean Gundam TCG foundation

---

## Prerequisites

1. **Backup your work:**
   ```bash
   git status
   git add -A
   git commit -m "Pre-migration checkpoint"
   ```

2. **Close all applications:**
   - Close Unreal Editor
   - Close Visual Studio / Rider
   - Close any file explorers viewing the project

3. **Verify Git LFS is working:**
   ```bash
   git lfs version
   ```

---

## Migration Steps

### Step 1: Clean Up One Piece Assets (~5 minutes)

Run the cleanup script to remove all One Piece TCG assets:

```bash
cleanup_onepiece_assets.bat
```

**What it does:**
- Deletes One Piece card sets (OP01-OP06, EB01-EB02, ST01-ST28, etc.)
- Removes One Piece rule PDFs (~33 MB)
- Deletes Don token assets
- Cleans up empty placeholder directories

**Space Savings:** ~277 MB (32% reduction)

**Verification:**
```bash
git status
```

You should see many deleted files in `Content/Cards/` and `Content/ref docs/`

---

### Step 2: Rename Module to GundamTCG (~2 minutes)

Run the rename script to change the project module name:

```bash
rename_to_gundam.bat
```

**What it does:**
- Renames `OnePieceTCG_V2.uproject` → `GundamTCG.uproject`
- Renames `Source/OnePieceTCG_V2/` → `Source/GundamTCG/`
- Updates all module references in code
- Updates all `#include` statements
- Renames Target files and Build.cs files
- Cleans up old build artifacts

**Verification:**
```bash
dir /b *.uproject
# Should show: GundamTCG.uproject

dir /b Source\
# Should show: GundamTCG, GundamTCG.Target.cs, GundamTCGEditor.Target.cs
```

---

### Step 3: Regenerate Project Files (~1 minute)

1. Right-click `GundamTCG.uproject` in File Explorer
2. Select **"Generate Visual Studio project files"**
3. Wait for the process to complete

This creates:
- `GundamTCG.sln` (Visual Studio solution)
- `.vcxproj` files for the new module

---

### Step 4: Build the Project (~5-10 minutes)

1. Open `GundamTCG.sln` in Visual Studio
2. Set build configuration to **"Development Editor"**
3. Build the solution: **Ctrl+Shift+B**
4. Wait for compilation to complete

**Expected Output:**
```
Build succeeded.
    0 Warning(s)
    0 Error(s)
```

If you get errors, check the [Troubleshooting](#troubleshooting) section.

---

### Step 5: Launch Unreal Editor (~2 minutes)

1. In Visual Studio, press **F5** to launch the editor
2. Unreal may prompt: **"Module has been recompiled, reload?"** → Click **Yes**
3. Unreal may show asset redirector prompts → Click **Fix Up** to update references

**First Launch Checklist:**
- [ ] Editor opens without crashes
- [ ] No critical errors in Output Log
- [ ] Blueprints load correctly (BP_CardActor, BP_TCGGameMode, etc.)
- [ ] Card back assets are visible in Content Browser

---

### Step 6: Verify Asset References (~5 minutes)

Check that blueprints still reference the correct C++ classes:

1. Open `Content/BP_TCGGameMode`
   - Verify it's still a child of `ATCGGameMode`
2. Open `Content/BP_TCGPlayerController`
   - Verify it's still a child of `ATCGPlayerController`
3. Open `Content/BP_TCGPlayerState`
   - Verify it's still a child of `ATCGPlayerState`
4. Open `Content/BP_CardActor`
   - Verify it compiles without errors

If any blueprints show errors, you may need to manually re-parent them to the C++ classes.

---

### Step 7: Commit Changes (~2 minutes)

Once everything is working:

```bash
git status
git add -A
git commit -m "Complete migration: Remove One Piece assets, rename to GundamTCG

- Removed One Piece card sets (OP01-OP06, EB01-EB02, ST01-ST28, etc.)
- Removed One Piece rule PDFs
- Renamed module from OnePieceTCG_V2 to GundamTCG
- Updated all code references and #includes
- Regenerated project files
- Space savings: ~277 MB (32% reduction)
"
```

---

### Step 8: Clean Git LFS Cache (Optional, ~5 minutes)

Remove incomplete Git LFS downloads to save additional space:

```bash
git lfs prune
```

This cleans up the `.git/lfs/incomplete/` directory and may save ~100 MB.

---

## Updated .gitignore

The `.gitignore` file has been updated with GundamTCG-specific rules:

**New Ignores Added:**
- One Piece TCG legacy assets (prevent accidental re-adds)
- Reference documentation folders
- Cleanup scripts
- Git LFS temporary files
- Unreal plugins binaries
- Crash reports and shader cache

**What This Means:**
- If you accidentally download One Piece assets, they won't be committed
- PDF documentation files will be ignored (store externally)
- Temporary build artifacts are properly excluded

---

## Project Structure After Migration

```
GundamTCG/
├── GundamTCG.uproject               # Renamed project file
├── Config/
│   ├── DefaultEngine.ini
│   ├── DefaultGame.ini
│   └── DefaultInput.ini
├── Content/
│   ├── Cards/
│   │   ├── CardBacks/               # KEPT - Generic card backs
│   │   └── Data/                    # KEPT - Card data structures
│   ├── BP_CardActor.uasset          # KEPT - Generic card actor
│   ├── BP_TCGGameMode.uasset        # KEPT - Game mode (needs Gundam adaptation)
│   ├── BP_TCGPlayerController.uasset
│   └── BP_TCGPlayerState.uasset
├── Source/
│   ├── GundamTCG/                   # RENAMED from OnePieceTCG_V2
│   │   ├── GundamTCG.h/cpp
│   │   ├── GundamTCG.Build.cs
│   │   ├── TCGGameMode.h/cpp
│   │   ├── TCGPlayerController.h/cpp
│   │   ├── TCGPlayerState.h/cpp
│   │   ├── TCGHandWidget.h/cpp
│   │   └── TCGTypes.h
│   ├── GundamTCG.Target.cs          # RENAMED
│   └── GundamTCGEditor.Target.cs    # RENAMED
├── Gundam TCG Ref/
│   ├── comprehensiverules_en.pdf    # Gundam TCG rules
│   └── playsheet_en.pdf             # Gundam TCG playsheet
├── cleanup_onepiece_assets.bat      # Cleanup script
├── rename_to_gundam.bat             # Rename script
└── MIGRATION_GUIDE.md               # This file
```

---

## What's Next?

Now that you have a clean GundamTCG foundation, here are the next development steps:

### 1. Adapt Game Rules for Gundam TCG

**One Piece TCG Mechanics to Replace:**
- **DON System** → Gundam TCG resource system
- **Turn Phases** → Adapt to Gundam turn structure
- **Card Types** → CHARACTER, LEADER, EVENT, STAGE → Gundam equivalents
- **Keywords** → Rush, Blocker, Counter, Trigger → Gundam keywords

**Files to Update:**
- `Source/GundamTCG/TCGTypes.h` - Card definition structures
- `Source/GundamTCG/TCGGameMode.cpp` - Turn phase system
- `Source/GundamTCG/TCGPlayerState.cpp` - Zone management

### 2. Import Gundam TCG Card Data

**Tasks:**
- Create Gundam card CSV file
- Import card images
- Update DataTable with Gundam cards
- Create card sets (organized by series/expansion)

**Recommended Structure:**
```
Content/Cards/
├── Gundam/
│   ├── GCW01/                       # Gundam Card Wars Set 01
│   ├── GCW02/                       # Gundam Card Wars Set 02
│   └── Starter_Decks/
│       ├── SD_ZeonStarter/
│       └── SD_FederationStarter/
```

### 3. Update UI and Visuals

**Tasks:**
- Replace card backs with Gundam-themed designs
- Update playmat graphics
- Adapt hand widget styling
- Create Gundam-specific UI elements

### 4. Implement Gundam-Specific Features

**Examples:**
- Mobile Suit deployment mechanics
- Pilot/MS interaction system
- Special Gundam card effects
- Gundam universe-specific zones

---

## Troubleshooting

### Build Errors After Rename

**Error:** `Cannot find module 'OnePieceTCG_V2'`

**Solution:**
1. Delete `Binaries/`, `Intermediate/`, `.vs/`
2. Delete all `.sln` and `.vcxproj` files
3. Right-click `GundamTCG.uproject` → Generate Visual Studio project files
4. Rebuild

---

### Blueprints Show Broken References

**Error:** Blueprints can't find parent C++ classes

**Solution:**
1. Open the blueprint in Unreal Editor
2. Click **File → Reparent Blueprint**
3. Select the correct C++ class (e.g., `ATCGGameMode`)
4. Compile and save

---

### Git LFS Not Downloading Assets

**Error:** `.uasset` files are placeholders

**Solution:**
```bash
git lfs install
git lfs fetch --all
git lfs pull
```

---

### Editor Crashes on Launch

**Error:** Unreal Editor crashes during startup

**Solution:**
1. Check `Saved/Logs/` for crash logs
2. Delete `Saved/`, `DerivedDataCache/`
3. Rebuild project in Visual Studio
4. Launch editor again

---

## File Manifest

**Scripts Created:**
- `cleanup_onepiece_assets.bat` - Removes One Piece TCG assets
- `rename_to_gundam.bat` - Renames module to GundamTCG
- `MIGRATION_GUIDE.md` - This guide

**Files Modified:**
- `.gitignore` - Added GundamTCG-specific rules

**Files That Will Be Renamed:**
- `OnePieceTCG_V2.uproject` → `GundamTCG.uproject`
- `Source/OnePieceTCG_V2/` → `Source/GundamTCG/`
- All `OnePieceTCG_V2.*` files → `GundamTCG.*`

**Files That Will Be Deleted:**
- `Content/Cards/OP01/` through `OP06/` (~347 MB)
- `Content/Cards/EB01/`, `EB02/` (~48 MB)
- `Content/Cards/Don/` (~440 KB)
- `Content/ref docs/*.pdf` (~33 MB)
- 38+ empty placeholder directories

---

## Summary

**Before Migration:**
- Project: OnePieceTCG_V2
- Size: 864 MB
- Assets: One Piece TCG cards and rules

**After Migration:**
- Project: GundamTCG
- Size: ~587 MB
- Assets: Clean foundation ready for Gundam TCG

**Space Savings:** 277 MB (32% reduction)

**Time Required:** ~20-30 minutes total

---

## Support

If you encounter issues during migration:

1. Check the [Troubleshooting](#troubleshooting) section
2. Review git status: `git status`
3. Check Unreal logs: `Saved/Logs/`
4. Verify all prerequisites are met

**Rollback Option:**

If something goes wrong, you can rollback to the pre-migration state:

```bash
git reset --hard HEAD
git clean -fd
```

This will restore the project to the last committed state.

---

## Checklist

Use this checklist to track your migration progress:

- [ ] Backup project (git commit)
- [ ] Close Unreal Editor and Visual Studio
- [ ] Run `cleanup_onepiece_assets.bat`
- [ ] Verify cleanup (git status)
- [ ] Run `rename_to_gundam.bat`
- [ ] Generate Visual Studio project files
- [ ] Build project in Visual Studio
- [ ] Launch Unreal Editor
- [ ] Fix up blueprint asset redirectors
- [ ] Verify all blueprints compile
- [ ] Test core functionality
- [ ] Commit changes to git
- [ ] Run `git lfs prune` (optional)
- [ ] Begin Gundam TCG implementation

---

Good luck with your Gundam TCG project! 🎴
