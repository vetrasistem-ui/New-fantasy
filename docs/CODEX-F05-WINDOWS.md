# Codex Handoff — F05 Windows Validation and Closeout

This is the **ordered execution contract** for Codex on a real Windows machine after the repository-side implementation is green.

Do not skip stages. Do not start F06. Do not refactor architecture while validating F03/F05. If a stage fails, fix only the smallest relevant layer, rerun that stage plus its direct regressions, record the failure/fix, then continue.

## Frozen architecture for this validation

```text
Fantasy Studio ───────────────→ FMAP
                                  ↓
Fantasy Client GUI → TCP → Fantasy Protocol v1
                                  ↓
                           Fantasy Server
                                  ↓
                             WorldRuntime
                                  ↓
                                FMAP
```

Required boundaries:

- no OTBM, TFS runtime, 10.98, DAT/SPR/OTB or OTClient in the native play path;
- Server is authoritative;
- Client sends only `MoveRequest(direction)` for movement;
- `MapChunk` uses region origin + FMCP v1 semantic payload;
- F05 TCP listener stays `127.0.0.1` only;
- `LoginDev` is development-only and must never be exposed publicly;
- GUI must consume existing domain/client core; do not implement a second protocol/map model.

---

# C00 — Clean checkout and baseline

From the repository root:

```powershell
git checkout main
git pull --ff-only
git status --short
git rev-parse HEAD
```

Gate:

- working tree clean before validation;
- record starting SHA in `docs/evidence/F05/WINDOWS-VALIDATION.md`;
- if the tree is not clean, stop and identify why before continuing.

---

# C01 — Full reproducible native build

Run:

```powershell
./scripts/build-native.ps1 -Configuration Release
```

This must validate contracts and build/test Studio, Server and Client.

Required PASS:

- layout/contracts;
- Fantasy Project v2;
- FMAP v0 and multi-chunk semantics;
- Fantasy Protocol v1 + FMCP contract;
- project relocation;
- Studio build + CTest;
- Server build + CTest including `fantasy-native-play-tests`;
- Client build + CTest;
- `fantasy-studio-gui.exe` exists;
- `fantasy-server.exe` exists;
- `fantasy-client.exe` exists;
- `fantasy-client-gui.exe` exists.

If any command fails, **stop C01**. Fix and rerun all of C01 before continuing.

---

# C02 — Headless two-process first play

Run:

```powershell
./scripts/run-native-play.ps1 -Configuration Release -Port 17171 -Character "Codex Headless Hero"
```

Expected path:

```text
fantasy-client.exe
    ↓ Hello / LoginDev / MapChunk / MoveRequest / Disconnect
TCP loopback
    ↓
fantasy-server.exe --serve-once
    ↓
WorldRuntime
    ↓
Game/Maps/World/world.fmap.json
```

Required PASS:

- Client enters world at `100,100,7`;
- receives 4 chunks / 64 semantic tiles;
- moves East and receives authoritative `101,100,7`;
- disconnect completes;
- Server exits by itself with exit code 0;
- no orphan Server/Client process remains.

Repeat once with a second character to prove process-level reconnect:

```powershell
./scripts/run-native-play.ps1 -Configuration Release -Port 17172 -Character "Codex Reconnect Hero"
```

Do not continue unless both runs pass.

---

# C03 — Close the pending F03 Studio interactive gate

Before editing the canonical development world, record its state:

```powershell
git status --short
git hash-object Game/Maps/World/world.fmap.json
```

Launch:

```powershell
./build/studio/Release/fantasy-studio-gui.exe .
```

Interactively validate the existing 8 × 8 / four-chunk world:

1. confirm the map window opens without crash;
2. visually confirm tiles from all four chunks are present;
3. select a tile;
4. Paint ground;
5. Undo and Redo;
6. Fill connected across a chunk boundary;
7. Add object;
8. Remove object;
9. Erase tile objects;
10. Save FMAP;
11. close Studio;
12. reopen Studio;
13. confirm the saved visual state persists.

Capture at least:

- one screenshot before edits;
- one screenshot after Save/Reopen showing the changed state.

Store evidence under:

```text
docs/evidence/F03/windows/
```

After captures, restore the canonical development map so validation does not silently change the fixture:

```powershell
git restore Game/Maps/World/world.fmap.json
git status --short
```

Only screenshot/evidence files and deliberate documentation changes may remain.

If the visual state does not survive Save/Reopen, F03 remains pending and C04 must not be used to declare F05 complete.

---

# C04 — Visual Fantasy Client first native play

Run:

```powershell
./scripts/run-visual-play.ps1 -Configuration Release -Port 17173 -Character "Codex Visual Hero"
```

The script launches the real `fantasy-server.exe --serve-once` and then `fantasy-client-gui.exe`.

Interactively validate:

1. GUI opens without crash;
2. Client reports connected / in-world state;
3. position begins at `100,100,7`;
4. four chunks are represented;
5. the full 64-tile development area is visually present;
6. semantic grounds render as deterministic placeholder tile colors;
7. object markers appear where semantic objects exist;
8. player marker is visible at the authoritative position;
9. Right Arrow moves to `101,100,7`;
10. Left Arrow returns to `100,100,7`;
11. Up/Down movement is also server-authoritative;
12. movement rejected by world bounds/blocked tiles is shown as a server error/status rather than moving locally;
13. closing the window sends Disconnect;
14. Server stops automatically with no orphan process.

Capture at least:

- initial in-world screenshot;
- screenshot after one authoritative movement.

Store under:

```text
docs/evidence/F05/windows/
```

The placeholder rendering is acceptable for F05. **Do not start the final asset/sprite pipeline merely to improve this validation view.**

---

# C05 — Visual reconnect and cleanup

Run a second complete visual session:

```powershell
./scripts/run-visual-play.ps1 -Configuration Release -Port 17174 -Character "Codex Visual Reconnect"
```

Required PASS:

- new Client connects cleanly;
- starts again at development spawn;
- receives the map again;
- movement works;
- closing Client stops Server;
- Task Manager / process query shows no orphan Fantasy process.

Optional PowerShell check after close:

```powershell
Get-Process fantasy-server,fantasy-client,fantasy-client-gui -ErrorAction SilentlyContinue
```

Expected: no remaining process from the validation.

---

# C06 — Evidence file

Create/update:

```text
docs/evidence/F05/WINDOWS-VALIDATION.md
```

Record:

- Windows version;
- repository SHA tested;
- C01 result;
- both C02 headless runs;
- F03 interactive result and screenshot paths;
- first C04 visual result and screenshot paths;
- C05 reconnect result;
- observed initial/moved coordinates;
- Server/Client exit codes where available;
- confirmation that listener stayed loopback-only;
- confirmation that no OTBM/10.98/TFS runtime participated;
- any failure encountered and the exact fix/commit used.

Do not write `PASS` for a gate that was not actually executed.

---

# C07 — Final regression after interactive validation

After restoring any edited fixture and after all fixes/evidence changes:

```powershell
./scripts/build-native.ps1 -Configuration Release
./scripts/run-native-play.ps1 -Configuration Release -Port 17175 -Character "Final Regression Hero"
git status --short
git diff --check
```

Required:

- all automated gates green;
- final headless two-process run green;
- no accidental FMAP fixture changes;
- no build/log/cache/executable files staged;
- no absolute paths persisted;
- no orphan processes.

---

# C08 — Close F03/F05 documentation only after evidence

Only after C00–C07 pass:

1. change F03 from `TECHNICAL PASS / interactive pending` to **PASS**;
2. change F05 to **PASS**;
3. update `docs/ROADMAP.md`;
4. update `docs/evidence/F03/RESULT.md` with Windows evidence;
5. update `docs/evidence/F05/RESULT.md` with final first-play evidence;
6. update `AGENTS.md` priority from F05 to F06;
7. do **not** implement F06 in the same validation commit.

Recommended final commit scope:

```text
test(windows): close F03 and F05 native-play gates
```

Then push and wait for GitHub Actions on that exact SHA. Record the successful run number/SHA in both evidence files.

---

# Failure policy for Codex

For every failed stage:

```text
STOP
 ↓
identify failing layer
 ↓
make smallest correction
 ↓
run direct unit/integration tests
 ↓
rerun the entire current C-stage
 ↓
rerun prior regression gate affected by the change
 ↓
record evidence
 ↓
continue
```

Do not compensate for a failure by bypassing validation, switching to 10.98/OTClient/TFS, weakening server authority, changing FMAP coordinate meaning, or creating duplicate Client/Server/Map implementations.
