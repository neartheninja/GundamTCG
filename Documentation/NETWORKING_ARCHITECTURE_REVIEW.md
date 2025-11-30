# Gundam TCG - Networking Architecture Review

## Executive Summary

**Overall Assessment**: ✅ **Solid Foundation**

Your proposed architecture is well-suited for a v1.0 release of a competitive TCG:
- ✅ Cost-effective (no dedicated servers)
- ✅ Handles both 1v1 and 2v2 modes
- ✅ Supports multiple auth flows (Epic/Guest/Offline)
- ✅ Good persistence strategy
- ✅ Clear upgrade path

**Recommendation**: Proceed with this design, with some refinements outlined below.

---

## 1. Overall Architecture Review

### ✅ Strengths

**Listen Server Model**:
```
Host Player (Listen Server)
  ├── Runs AGCGGameMode_1v1/2v2
  ├── Has server authority
  ├── Owns game state
  └── Replicates to clients

Client Players
  ├── Send Server RPCs
  ├── Receive replicated state
  └── Handle local UI/input
```

**Why This Works**:
- Perfect for turn-based card games (not latency-sensitive like FPS)
- Host has full authority → easier to prevent cheating
- EOS P2P handles NAT traversal
- No infrastructure costs

### ⚠️ Potential Issues & Solutions

#### Issue 1: Host Migration
**Problem**: If host disconnects mid-match, game ends for everyone.

**Solutions**:
1. **V1 (Simple)**: Match is forfeit, host gets loss penalty
2. **V1.5 (Better)**: Implement graceful reconnection:
   ```cpp
   // In AGCGGameMode_1v1
   void HandlePlayerDisconnected(APlayerController* Player)
   {
       if (Player == GetWorld()->GetFirstPlayerController())
       {
           // Host disconnected - pause game, give 60s to reconnect
           bGamePaused = true;
           StartReconnectTimer(60.0f);
       }
       else
       {
           // Client disconnected - they get loss
           ProcessPlayerConcession(GetPlayerID(Player));
       }
   }
   ```

3. **V2 (Advanced)**: Host migration using EOS Session Transfer (complex)

**Recommendation**: Start with V1, add reconnection in V1.5.

#### Issue 2: Spectators
**Problem**: Your current design doesn't mention spectators.

**Solution**:
```cpp
// In session creation
SessionSettings.Set(SETTING_NUMSPECTATORS, 2); // Allow 2 spectators

// In AGCGGameMode_*
virtual void PostLogin(APlayerController* NewPlayer) override
{
    if (bMatchInProgress && SpectatorClass)
    {
        // Late joiners become spectators
        NewPlayer->UnPossess();
        ASpectatorPawn* Spectator = GetWorld()->SpawnActor<ASpectatorPawn>();
        NewPlayer->Possess(Spectator);
    }
}
```

**Recommendation**: Add spectator support in V1.5 for watching friends.

#### Issue 3: Cheating Prevention
**Problem**: Malicious host can cheat (draw specific cards, manipulate RNG).

**Mitigations**:
1. **Client-Side Validation**: Clients verify visible game state
   ```cpp
   // In AGCGPlayerController
   void OnCardPlayed(const FGCGCardInstance& Card)
   {
       // Verify this is a legal play
       if (!LocalValidateCardPlay(Card))
       {
           // Report mismatch to analytics
           ReportSuspiciousActivity("Illegal card play detected");
       }
   }
   ```

2. **Replay Recording**: Save match replay for post-game analysis
3. **Reputation System**: Flag frequent disconnectors/cheaters
4. **Ranked Mode**: Eventually move to dedicated servers

**Recommendation**: Implement client-side validation in V1, accept some cheating risk for casual play.

---

## 2. Identity & Authentication Review

### ✅ Three-Tier Auth Strategy

```
Epic Account Login
  ├── Pros: Cross-device, friends list, achievements
  ├── Cons: Requires Epic account
  └── Use: Primary auth method

Guest Login (Device ID)
  ├── Pros: No account needed, still use EOS features
  ├── Cons: Device-locked, lost if uninstalled
  └── Use: Casual players, quick start

Offline Profile
  ├── Pros: No internet, full privacy
  ├── Cons: No online features, local only
  └── Use: AI practice, LAN parties
```

### 💡 Recommendations

#### Add Account Linking
Allow guest users to upgrade to Epic accounts later:

```cpp
// UGCGAccountManager
UFUNCTION(BlueprintCallable)
bool LinkDeviceIDToEpicAccount()
{
    // 1. User is logged in as guest (Device ID)
    // 2. User clicks "Sign in with Epic"
    // 3. After Epic auth, call EOS Link API
    // 4. Transfer EOS Player Data Storage from Device ID to Epic ID
    // 5. Update local save to remember Epic ID

    IOnlineIdentityPtr Identity = Online::GetIdentityInterface();
    Identity->Login(/*Epic Auth*/);

    // On success, copy player data
    TransferPlayerDataToNewAccount(OldProductUserId, NewProductUserId);
}
```

**Benefit**: Players can start as guest, then preserve progress when they commit.

#### Profile Migration Warning
```cpp
// Show this when user logs in with Device ID
void ShowDeviceIDWarning()
{
    // "Guest accounts are tied to this device. If you uninstall,
    // your progress will be lost. Sign in with Epic to sync across devices."
}
```

---

## 3. Session & Match Lifecycle

### ✅ Well-Designed Flow

Your host/client flows are solid. Here's how to implement them:

### Implementation: Host Match Creation

```cpp
// UGCGLobbyManager.h
UCLASS()
class UGCGLobbyManager : public UGameInstanceSubsystem
{
    GENERATED_BODY()

public:
    /**
     * Create and host a new match
     * @param Mode "1v1" or "2v2"
     * @param DeckID Deck to use
     * @param bRanked Is this ranked matchmaking?
     */
    UFUNCTION(BlueprintCallable, Category = "Lobby")
    void CreateMatch(FString Mode, FName DeckID, bool bRanked = false);

    /**
     * Find available matches
     * @param Mode Filter by mode ("1v1", "2v2", or "" for all)
     * @param bRankedOnly Only show ranked matches
     */
    UFUNCTION(BlueprintCallable, Category = "Lobby")
    void FindMatches(FString Mode, bool bRankedOnly = false);

    /**
     * Join a specific match by session ID
     */
    UFUNCTION(BlueprintCallable, Category = "Lobby")
    void JoinMatch(const FOnlineSessionSearchResult& SearchResult);

    // Callbacks
    UPROPERTY(BlueprintAssignable)
    FOnMatchCreatedDelegate OnMatchCreated;

    UPROPERTY(BlueprintAssignable)
    FOnMatchFoundDelegate OnMatchesFound;

    UPROPERTY(BlueprintAssignable)
    FOnMatchJoinedDelegate OnMatchJoined;

private:
    IOnlineSessionPtr SessionInterface;
    TSharedPtr<FOnlineSessionSearch> SessionSearch;
};
```

### Implementation: Match Creation

```cpp
// UGCGLobbyManager.cpp
void UGCGLobbyManager::CreateMatch(FString Mode, FName DeckID, bool bRanked)
{
    IOnlineSubsystem* OnlineSub = IOnlineSubsystem::Get();
    SessionInterface = OnlineSub->GetSessionInterface();

    // Configure session settings
    FOnlineSessionSettings SessionSettings;
    SessionSettings.bIsLANMatch = false;
    SessionSettings.bUsesPresence = true;
    SessionSettings.bAllowJoinInProgress = false; // Card game - can't join mid-match
    SessionSettings.bAllowJoinViaPresence = true;
    SessionSettings.bShouldAdvertise = true;
    SessionSettings.bUseLobbiesIfAvailable = true;

    // Set player count
    if (Mode == "1v1")
    {
        SessionSettings.NumPublicConnections = 2;
    }
    else if (Mode == "2v2")
    {
        SessionSettings.NumPublicConnections = 4;
    }

    // Custom settings for matchmaking filters
    SessionSettings.Set(SETTING_GAMEMODE, Mode, EOnlineDataAdvertisementType::ViaOnlineService);
    SessionSettings.Set(SETTING_MAPNAME, FString("Match_") + Mode, EOnlineDataAdvertisementType::ViaOnlineService);
    SessionSettings.Set(SETTING_RANKED, bRanked, EOnlineDataAdvertisementType::ViaOnlineService);

    // Store deck selection for when match starts
    UGCGGameInstance* GameInstance = Cast<UGCGGameInstance>(GetGameInstance());
    GameInstance->SelectedDeckID = DeckID;

    // Create session
    SessionInterface->OnCreateSessionCompleteDelegates.AddUObject(this, &UGCGLobbyManager::OnCreateSessionComplete);
    SessionInterface->CreateSession(0, NAME_GameSession, SessionSettings);
}

void UGCGLobbyManager::OnCreateSessionComplete(FName SessionName, bool bWasSuccessful)
{
    if (bWasSuccessful)
    {
        // Transition to game map as listen server
        UGameplayStatics::OpenLevel(GetWorld(), FName("Match_1v1"), true, "listen");
        OnMatchCreated.Broadcast(true);
    }
    else
    {
        OnMatchCreated.Broadcast(false);
    }
}
```

### 💡 Recommendations

#### Add Lobby System
Before starting match, show a lobby where players can:
- See who joined
- Chat
- Ready up
- Select deck (if not selected earlier)
- Host can kick/invite

```cpp
// Create lobby map: Lobby_1v1.umap, Lobby_2v2.umap
// Use AGCGGameModeBase with minimal logic
// When all players ready, host calls:
void UGCGLobbyManager::StartMatch()
{
    // Transition from lobby to actual match
    UGameplayStatics::OpenLevel(GetWorld(), FName("Match_1v1"), true, "listen");
}
```

#### Add Matchmaking Queue
For ranked play, implement a queue system:

```cpp
UCLASS()
class UGCGMatchmakingSubsystem : public UGameInstanceSubsystem
{
    // Instead of browsing sessions, join a queue
    void JoinQueue(FString Mode, int32 PlayerSkillRating);

    // EOS finds similar-skilled opponent
    // When match found, auto-connect both players
};
```

**Recommendation**: V1 = manual server browser, V1.5 = matchmaking queue.

---

## 4. 1v1 vs 2v2 Implementation

### ✅ Good Separation

Your approach to use separate GameModes is correct:

```
AGCGGameModeBase (base class)
  ├── AGCGGameMode_1v1 (two players)
  └── AGCGGameMode_2v2 (four players, teams)
```

### 💡 Recommendations for 2v2

#### Team Assignment

```cpp
// AGCGGameMode_2v2.h
UCLASS()
class AGCGGameMode_2v2 : public AGCGGameModeBase
{
    GENERATED_BODY()

public:
    virtual void PostLogin(APlayerController* NewPlayer) override;

    /** Assign player to a team based on join order */
    void AssignPlayerToTeam(AGCGPlayerState* PlayerState);

    /** Get all players on a team */
    TArray<AGCGPlayerState*> GetTeamPlayers(int32 TeamID) const;

    /** Team turn order: Team A Player 1 → Team B Player 1 → Team A Player 2 → ... */
    virtual int32 GetNextPlayerID(int32 CurrentPlayerID) override;

protected:
    /** Team A = 0, Team B = 1 */
    UPROPERTY(Replicated)
    TArray<int32> TeamAPlayerIDs;

    UPROPERTY(Replicated)
    TArray<int32> TeamBPlayerIDs;
};
```

#### Shared Team Resources

```cpp
// AGCGPlayerState.h
// Add team-shared resources

UPROPERTY(Replicated)
int32 TeamID; // 0 or 1

// In 2v2 mode, these are shared per team:
// - Shield stack (team shares one stack)
// - Base (team has one base)
// - Battle area limit (6 units total for team)

// AGCGGameMode_2v2 manages:
TArray<FGCGCardInstance> TeamA_SharedShieldStack;
TArray<FGCGCardInstance> TeamA_SharedBase;
int32 TeamA_TotalUnitsInPlay; // Max 6

TArray<FGCGCardInstance> TeamB_SharedShieldStack;
TArray<FGCGCardInstance> TeamB_SharedBase;
int32 TeamB_TotalUnitsInPlay; // Max 6

// Individual players still have:
// - Their own hand
// - Their own deck
// - Their own resources
// - Their own graveyard
```

#### Communication

```cpp
// Allow team chat in 2v2
UFUNCTION(Server, Reliable)
void Server_SendTeamMessage(const FString& Message);

// Only visible to teammates
UFUNCTION(Client, Reliable)
void Client_ReceiveTeamMessage(const FString& SenderName, const FString& Message);
```

---

## 5. Persistence & Saves

### ✅ Hybrid Approach is Smart

```
Local SaveGame (USaveGame)
  └── Offline cache, settings, last login

EOS Player Data Storage
  └── Cloud sync for collection/decks

EOS Stats
  └── Win/loss records, leaderboards
```

### Implementation: Player Data Storage

```cpp
// UGCGAccountManager.h
UCLASS()
class UGCGAccountManager : public UGameInstanceSubsystem
{
    GENERATED_BODY()

public:
    /** Load player data from EOS */
    UFUNCTION(BlueprintCallable)
    void LoadPlayerData();

    /** Save player data to EOS */
    UFUNCTION(BlueprintCallable)
    void SavePlayerData();

    /** Player's card collection */
    UPROPERTY(BlueprintReadOnly)
    TMap<FName, int32> CardCollection; // CardNumber -> Count

    /** Player's saved decks */
    UPROPERTY(BlueprintReadOnly)
    TArray<FGCGDeckList> SavedDecks;

    /** Player stats */
    UPROPERTY(BlueprintReadOnly)
    FGCGPlayerStats Stats;

private:
    void OnLoadComplete(const FOnlineError& Error, const TArray<FOnlineUserFile>& Files);
    void OnSaveComplete(const FOnlineError& Error);

    FString SerializePlayerData() const;
    void DeserializePlayerData(const FString& JsonData);
};
```

### Data Schema (JSON)

```json
{
  "version": 1,
  "lastUpdated": "2025-11-15T19:00:00Z",
  "collection": {
    "GND-001": 4,
    "GND-002": 2,
    "GND-003": 4
  },
  "decks": [
    {
      "id": "deck_1",
      "name": "Red/White Aggro",
      "mainDeck": ["GND-001", "GND-001", "GND-002"],
      "resourceDeck": ["GND-R01", "GND-R02"],
      "colors": ["Red", "White"]
    }
  ],
  "stats": {
    "wins_1v1": 10,
    "losses_1v1": 5,
    "wins_2v2": 3,
    "losses_2v2": 2
  },
  "settings": {
    "preferredColor": "Red",
    "avatarId": "avatar_1",
    "playmatkId": "playmat_default"
  }
}
```

### 💡 Recommendations

#### Add Conflict Resolution
If player plays on two devices:

```cpp
void UGCGAccountManager::LoadPlayerData()
{
    // Compare local timestamp vs cloud timestamp
    if (LocalData.lastUpdated > CloudData.lastUpdated)
    {
        // Local is newer - ask user
        ShowConflictDialog("Local save is newer. Overwrite cloud?");
    }
    else
    {
        // Cloud is newer - use it
        ApplyCloudData(CloudData);
    }
}
```

#### Add Backup to Local Save
Always cache cloud data locally:

```cpp
void OnLoadComplete(const TArray<FOnlineUserFile>& Files)
{
    // Save to local cache
    UGCGSaveGame* SaveGame = Cast<UGCGSaveGame>(UGameplayStatics::LoadGameFromSlot("Profile", 0));
    SaveGame->CachedCloudData = Files[0].Data;
    UGameplayStatics::SaveGameToSlot(SaveGame, "Profile", 0);

    // Use cloud data
    DeserializePlayerData(Files[0].Data);
}
```

**Benefit**: If EOS is down, player can still access their decks.

#### Implement Auto-Save
Save after key actions:

```cpp
// After editing deck
void OnDeckSaved()
{
    AccountManager->SavePlayerData();
}

// After match end
void OnMatchEnd()
{
    // Update stats
    AccountManager->Stats.Wins_1v1++;
    AccountManager->SavePlayerData();
}

// Periodic save
void Tick(float DeltaTime)
{
    AutoSaveTimer += DeltaTime;
    if (AutoSaveTimer > 300.0f) // Every 5 minutes
    {
        AccountManager->SavePlayerData();
        AutoSaveTimer = 0.0f;
    }
}
```

---

## 6. Offline Play

### ✅ Good Fallback Strategy

```
Offline Modes:
  ├── vs AI (1v1)
  ├── Hotseat (2 players, 1 device)
  └── LAN (direct IP)
```

### Implementation: AI Opponent

```cpp
// You already have GCGAIController.h
// Extend it for offline play:

void AGCGGameMode_1v1::StartOfflineMatch(FName PlayerDeckID, FName AIDeckID, EGCAIDifficulty Difficulty)
{
    // Player 1 = Human
    AGCGPlayerState* HumanPlayer = CreatePlayerState(0);
    HumanPlayer->LoadDeck(PlayerDeckID);

    // Player 2 = AI
    AGCGPlayerState* AIPlayer = CreatePlayerState(1);
    AIPlayer->LoadDeck(AIDeckID);

    AGCGAIController* AIController = GetWorld()->SpawnActor<AGCGAIController>();
    AIController->Possess(AIPlayer->GetPawn());
    AIController->SetDifficulty(Difficulty);

    // Start game
    BeginMatch();
}
```

### Implementation: Hotseat Mode

```cpp
// In main menu
void StartHotseatMatch(FName Player1DeckID, FName Player2DeckID)
{
    // Both players use same controller, passing device
    // Use local profiles, no networking

    UGameplayStatics::OpenLevel(GetWorld(), "Match_1v1", false, "?Mode=Hotseat");
}

// AGCGGameMode_1v1
void AGCGGameMode_1v1::BeginPlay()
{
    if (IsHotseatMode())
    {
        // Disable replication
        // Show "Pass device to opponent" prompts
        // Hide opponent's hand
    }
}
```

---

## 7. Integration with Existing Codebase

### Current State Analysis

Your existing systems are ready:
- ✅ `AGCGGameMode_1v1` - Turn management, rules
- ✅ `AGCGGameMode_2v2` - Team variant
- ✅ `AGCGPlayerState` - Player data, resources, zones
- ✅ `AGCGGameState` - Match state, phase
- ✅ All subsystems (combat, validation, effects, etc.)

### What's Missing for Networking

#### 1. Replication Setup

```cpp
// AGCGPlayerState.h
// Add replication

virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override
{
    Super::GetLifetimeReplicatedProps(OutLifetimeProps);

    // Replicate to everyone
    DOREPLIFETIME(AGCGPlayerState, PlayerID);
    DOREPLIFETIME(AGCGPlayerState, TeamID);
    DOREPLIFETIME(AGCGPlayerState, bHasLost);
    DOREPLIFETIME(AGCGPlayerState, BattleArea);
    DOREPLIFETIME(AGCGPlayerState, ResourceArea);
    DOREPLIFETIME(AGCGPlayerState, ShieldStack);
    DOREPLIFETIME(AGCGPlayerState, BaseSection);
    DOREPLIFETIME(AGCGPlayerState, Graveyard);

    // Replicate to owner only (private info)
    DOREPLIFETIME_CONDITION(AGCGPlayerState, Hand, COND_OwnerOnly);
    DOREPLIFETIME_CONDITION(AGCGPlayerState, Deck, COND_OwnerOnly);
}
```

#### 2. Server RPCs for Actions

```cpp
// AGCGPlayerController.h
UCLASS()
class AGCGPlayerController : public APlayerController
{
    GENERATED_BODY()

public:
    // Client → Server: Play card
    UFUNCTION(Server, Reliable, WithValidation)
    void Server_PlayCard(const FGCGCardInstance& Card, EGCGCardZone TargetZone);

    // Client → Server: Declare attack
    UFUNCTION(Server, Reliable, WithValidation)
    void Server_DeclareAttack(const FGCGCardInstance& Attacker, const FGCGCardInstance& Target);

    // Client → Server: End turn
    UFUNCTION(Server, Reliable, WithValidation)
    void Server_EndTurn();

    // Client → Server: Concede
    UFUNCTION(Server, Reliable, WithValidation)
    void Server_Concede();

    // Server → Client: Show choices (mulligan, blocker, etc.)
    UFUNCTION(Client, Reliable)
    void Client_ShowMulliganChoice();

    UFUNCTION(Client, Reliable)
    void Client_ShowBlockerChoice(const FGCGAttackInfo& AttackInfo);
};
```

#### 3. Validation

```cpp
bool AGCGPlayerController::Server_PlayCard_Validate(const FGCGCardInstance& Card, EGCGCardZone TargetZone)
{
    // Anti-cheat: Verify this is a legal action
    AGCGPlayerState* PS = GetPlayerState<AGCGPlayerState>();

    // 1. Is it this player's turn?
    if (GetGameMode()->GetActivePlayerID() != PS->GetPlayerID())
        return false;

    // 2. Does player own this card?
    if (!PS->Hand.Contains(Card))
        return false;

    // 3. Can they afford to play it?
    UGCGPlayerActionSubsystem* ActionSubsystem = GetGameInstance()->GetSubsystem<UGCGPlayerActionSubsystem>();
    if (!ActionSubsystem->CanPlayCard(PS, Card).bSuccess)
        return false;

    return true;
}

void AGCGPlayerController::Server_PlayCard_Implementation(const FGCGCardInstance& Card, EGCGCardZone TargetZone)
{
    // Validated - execute on server
    AGCGGameMode_1v1* GameMode = GetWorld()->GetAuthGameMode<AGCGGameMode_1v1>();
    GameMode->ProcessCardPlay(GetPlayerState<AGCGPlayerState>(), Card, TargetZone);
}
```

---

## 8. Recommended File Structure

```
Source/GundamTCG/
├── Online/
│   ├── GCGAccountManager.h/cpp         # EOS auth, player data
│   ├── GCGLobbyManager.h/cpp           # Session creation/joining
│   ├── GCGMatchmakingSubsystem.h/cpp   # Ranked queue (V1.5)
│   ├── GCGPlayerController.h/cpp       # Network RPCs
│   └── GCGOnlineTypes.h                # Online-specific structs
│
├── GameModes/
│   ├── GCGGameModeBase.h/cpp           # Shared logic
│   ├── GCGGameMode_1v1.h/cpp           # 1v1 rules (networking-ready)
│   ├── GCGGameMode_2v2.h/cpp           # 2v2 team rules
│   └── GCGGameMode_Offline.h/cpp       # AI/Hotseat
│
├── PlayerState/
│   └── GCGPlayerState.h/cpp            # Add replication
│
├── GameState/
│   └── GCGGameState.h/cpp              # Add replication
│
└── SaveGame/
    └── GCGSaveGame.h/cpp               # Local persistence
```

---

## 9. Implementation Roadmap

### Phase 1: Core Networking (4-6 weeks)
- [ ] Add replication to PlayerState/GameState
- [ ] Implement Server RPCs in PlayerController
- [ ] Create GCGAccountManager (EOS auth)
- [ ] Create GCGLobbyManager (sessions)
- [ ] Test 1v1 over LAN
- [ ] Test 1v1 over EOS P2P

### Phase 2: Persistence (2-3 weeks)
- [ ] Implement GCGSaveGame (local)
- [ ] Integrate EOS Player Data Storage
- [ ] Implement EOS Stats
- [ ] Add deck import/export
- [ ] Test account linking (guest → Epic)

### Phase 3: 2v2 Mode (3-4 weeks)
- [ ] Implement AGCGGameMode_2v2 networking
- [ ] Team assignment logic
- [ ] Shared resources replication
- [ ] Team chat
- [ ] Test 2v2 over EOS

### Phase 4: Polish (3-4 weeks)
- [ ] Reconnection handling
- [ ] Spectator mode
- [ ] Replay recording
- [ ] Client-side validation
- [ ] Anti-cheat logging
- [ ] Offline AI mode

### Phase 5: Matchmaking (V1.5, 2-3 weeks)
- [ ] Ranked queue system
- [ ] Skill rating (ELO)
- [ ] Leaderboards
- [ ] Season system

**Total Estimate**: ~12-16 weeks for V1, +3 weeks for V1.5

---

## 10. Critical Recommendations

### Must-Haves for V1

1. **Replication First**: Add replication tags to all game state before testing multiplayer
2. **Server Authority**: All game logic must run on server (host), clients only send inputs
3. **Validation**: Every Server RPC must validate (anti-cheat)
4. **Reconnection**: At minimum, handle disconnects gracefully
5. **Save Backups**: Always cache cloud data locally

### Nice-to-Haves for V1.5

1. **Spectators**: Watch friends play
2. **Replays**: Record and playback matches
3. **Matchmaking Queue**: Better than server browser
4. **Host Migration**: Advanced but valuable

### Future (V2+)

1. **Dedicated Servers**: For ranked competitive play
2. **Custom Backend**: Economy, trading, tournaments
3. **Cross-Platform**: PC, Mobile, Console
4. **Voice Chat**: Team communication in 2v2

---

## 11. Security Considerations

### Current Risks

**Listen Server Model Vulnerabilities**:
- Host can manipulate game state
- Host can see opponent's hand (memory inspection)
- Host can manipulate RNG

### Mitigations

1. **Client-Side Validation** (V1)
   - Clients verify visible game state
   - Flag suspicious activity
   - Report to analytics

2. **Replay Audit** (V1.5)
   - Save all matches
   - Players can report cheating
   - Review replays manually/automatically

3. **Reputation System** (V1.5)
   - Track disconnect rate
   - Track cheat reports
   - Ban repeat offenders

4. **Dedicated Servers** (V2)
   - Move ranked to dedicated servers
   - Casual can stay P2P

**Recommendation**: Accept some cheating risk in V1 casual play, move to dedicated servers for ranked.

---

## 12. Final Verdict

### ✅ Approved Architecture

Your design is solid for a V1 release:

**Strengths**:
- Cost-effective (free EOS)
- Handles 1v1 and 2v2
- Good auth flexibility
- Smart persistence strategy
- Clear upgrade path

**Weaknesses**:
- Host cheating possible (mitigated in V2)
- No host migration (acceptable for V1)
- P2P can have connection issues (EOS helps)

**Overall Grade**: **A-**

### Action Items

1. **Implement replication** on PlayerState/GameState
2. **Create GCGPlayerController** with Server RPCs
3. **Integrate EOS** (auth + sessions)
4. **Test thoroughly** on different networks
5. **Add validation** to all RPCs

### Next Steps

Would you like me to:
1. Implement the GCGPlayerController with all Server RPCs?
2. Add replication to your existing PlayerState/GameState?
3. Create the GCGAccountManager for EOS integration?
4. Create the GCGLobbyManager for session management?

All the hard logic (game rules) is already done. Networking is "just" wrapping it with replication and RPCs!
