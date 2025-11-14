# Quick Start - GundamTCG Migration

## TL;DR - Fast Migration (15 minutes)

### 1. Backup
```bash
git add -A
git commit -m "Pre-migration checkpoint"
```

### 2. Close Everything
- Close Unreal Editor
- Close Visual Studio

### 3. Run Cleanup
```bash
cleanup_onepiece_assets.bat
```
Type `yes` when prompted. **Saves 277 MB.**

### 4. Run Rename
```bash
rename_to_gundam.bat
```
Type `yes` when prompted. **Renames to GundamTCG.**

### 5. Regenerate Project
- Right-click `GundamTCG.uproject`
- Select "Generate Visual Studio project files"

### 6. Build
- Open `GundamTCG.sln` in Visual Studio
- Press `Ctrl+Shift+B` to build
- Press `F5` to launch editor

### 7. Fix Blueprints
- Click "Fix Up" if prompted for asset redirectors
- Verify blueprints compile

### 8. Commit
```bash
git add -A
git commit -m "Migration complete: Removed One Piece assets, renamed to GundamTCG"
```

### 9. (Optional) Clean LFS
```bash
git lfs prune
```
**Saves additional ~100 MB.**

---

## What Changed?

| Before | After |
|--------|-------|
| OnePieceTCG_V2.uproject | GundamTCG.uproject |
| Source/OnePieceTCG_V2/ | Source/GundamTCG/ |
| 864 MB | ~587 MB |
| One Piece cards | Clean foundation |

---

## Next Steps

1. **Update game rules** in `Source/GundamTCG/TCGTypes.h`
2. **Import Gundam cards** into `Content/Cards/`
3. **Adapt turn phases** in `TCGGameMode.cpp`
4. **Update blueprints** for Gundam mechanics

---

## Troubleshooting

**Build fails?**
1. Delete: `Binaries/`, `Intermediate/`, `.vs/`, `*.sln`
2. Regenerate project files
3. Rebuild

**Blueprints broken?**
1. Open blueprint
2. File → Reparent Blueprint
3. Select C++ class
4. Compile

**Need help?**
See [MIGRATION_GUIDE.md](MIGRATION_GUIDE.md) for full details.

---

## Files Created

- ✅ `cleanup_onepiece_assets.bat` - Remove One Piece assets
- ✅ `rename_to_gundam.bat` - Rename to GundamTCG
- ✅ `MIGRATION_GUIDE.md` - Full migration guide
- ✅ `QUICK_START.md` - This file
- ✅ `.gitignore` - Updated with GundamTCG rules
