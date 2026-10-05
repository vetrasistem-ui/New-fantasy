# F05 Windows Validation

Status: **PASS — C00–C07 COMPLETE; F03/F05 WINDOWS VISUAL GATES CLOSED**

This file is the evidence ledger for `docs/CODEX-F05-WINDOWS.md`.

## Current run — official SDL_Renderer3 baseline

Current repository SHA: `621ca7b499330b61f653345590f1897c3b1f76d2`.
Validation date: 2026-10-05 (America/Sao_Paulo).
Windows 11 Home 10.0.26200; PowerShell x64 7.6.5; Release configuration;
Intel HD Graphics 5500, driver 20.19.15.4703 (2017-06-08).
The sections below this current-run record retain the earlier SDL_GPU attempts
as historical evidence; those source changes are not part of this baseline.

The requested `backup/c03-sdlgpu-c072870` branch preserves `c072870`.
Because the complete evidence was committed separately in `a4a3287`, an
additional `backup/c03-evidence-a4a3287` branch preserves those documents.
After `git fetch origin`, local `main` was reset to `origin/main`. Only
`docs/evidence/F05/WINDOWS-VALIDATION.md` and
`docs/evidence/F03/windows/STARTUP-FAILURE.md` were recovered from the evidence
backup. Both GUI source files match the official remote SDL_Renderer3 versions.

Observed current results:

- C01: PASS; exact full build-native command exited 0. All contract/relocation
  validators passed; Studio 5/5, Server 6/6, Client 2/2 CTests passed.
- C02 repeated on this SHA: PASS at ports 17171 and 17172; each session received
  4 chunks / 64 tiles, spawn 100,100,7 and authoritative East move 101,100,7;
  clean Disconnect and Server exit 0. No runtime processes remained afterwards.
- C03: PASS from the beginning, with all four chunks visible; tile selection,
  Paint at 97,97,7, Undo, Redo, Fill of 17 tiles across the two upper chunks,
  Add Object, Remove Object, Erase of one remaining object at 98,97,7 and Save.
  Reopening preserved the changed visual state and the saved FMAP semantics.
  Studio exited 0. Generated local imgui.ini arranged panels within the display;
  it is a validation cache, not a committed product setting.
- C04: PASS; GUI opens connected at 100,100,7 with 4 chunks / 64 tiles,
  semantic grounds, object and player markers. Automated arrows did not move
  the player; temporary SDL key diagnostics proved that Computer Use `Right`
  injects `Keypad 6` (key and scancode), not `RightArrow`. This is an automation
  input limitation, not evidence of a failed authoritative movement request.
  A trial ImGui capture option did not resolve the injected keypad input;
  that change and the diagnostic were removed. Both GUI sources again match
  origin/main. On 2026-10-05 the operator confirmed physical arrow validation
  with real screenshots: initial 100,100,7; East 101,100,7; West 99,100,7;
  South 100,101,7; North 100,99,7. In each reported capture the GUI was connected,
  Chunks=4, the visual map/player/object markers were present, and Status showed
  the position returned by Server. These are observed endpoints, not a claim
  that the listed directions were executed consecutively from one position.
  Codex independently captured the live GUI at 101,100,7 with
  `Status: Server position: 101,100,7` as
  `docs/evidence/F05/windows/c04-renderer3-east-confirmed.jpg`.
  The paths of the operator's remaining screenshots have been requested;
  their files are not yet archived here. The required initial and moved
  captures are archived here independently.
  Bounds rejection: PASS, confirmed manually and independently captured by
  Codex at 103,100,7 with `Fantasy Server error 2001: movement rejected by
  authoritative world runtime`. The position/player stayed on the last valid
  tile. Screenshot: `docs/evidence/F05/windows/c04-renderer3-rejected.jpg`.
  Closing the GUI sent Disconnect; Client and Server exited 0; Server reported
  frames_received=80, frames_sent=86, clean_disconnect=true, state=STOPPED.
  The visual script exited 0 and process/socket checks found no orphan runtime
  processes or listening/established C04 socket.
  A sandbox-only socket failure (WSA 10013) was resolved by running the same
  loopback script with permitted local networking. The listener was observed
  at 127.0.0.1:17173. Incomplete sessions closed cleanly; Server exit 0.
  The completed session used unchanged official sources. Its initial screenshot is
  `docs/evidence/F05/windows/c04-renderer3-initial.jpg`.
- C05: PASS; new visual session connected successfully on port 17174 with
  `Codex Visual Reconnect`, reset to spawn 100,100,7, and received/rendered the
  four chunks / 64 tiles and object/player markers. Screenshot:
  `docs/evidence/F05/windows/c05-renderer3-initial.jpg`.
  Listener/connection query independently confirmed 127.0.0.1:17174.
  Physical Right was confirmed in the live GUI at 101,100,7 with
  `Status: Server position: 101,100,7`. Screenshot:
  `docs/evidence/F05/windows/c05-renderer3-east.jpg`.
  Closing the GUI completed Disconnect; Client and Server exited 0; Server
  reported frames_received=18, frames_sent=24, clean_disconnect=true,
  state=STOPPED. The visual script exited 0. Process query found zero Fantasy
  Studio/Server/Client/Client GUI processes; zero active C05 sockets remained.
- C06: PASS; this ledger records the tested SHA/environment, C01/C02 results,
  C03/C04/C05 real visual captures, movement coordinates and cleanup evidence.
  Both visual sessions used the real Server on loopback only. No OTBM, TFS,
  protocol 10.98, DAT/SPR/OTB or OTClient participated. Movement used the existing
  DevelopmentClient MoveRequest(direction), with Server-authoritative replies.
  No permanent GUI/core/protocol/network/FMAP source correction was needed
  after adoption of the official SDL_Renderer3 baseline.
- C07: PASS; full `build-native.ps1 -Configuration Release` exited 0 with
  all validators and 13 CTests (Studio 5/5, Server 6/6, Client 2/2) passing.
  Final headless session on 17175 (`Final Regression Hero`) received 4 chunks /
  64 tiles, spawned at 100,100,7, moved East to 101,100,7 and disconnected.
  Client/Server exit 0; Server frames_received=4, frames_sent=10,
  clean_disconnect=true, state=STOPPED. Final Fantasy process count=0.
  `git diff --check` passed; both GUI sources and canonical FMAP match origin/main.
- C08: F03/F05 marked PASS after all preceding gates passed. Roadmap, result
  evidence and AGENTS priority are updated. F06 is the next documented phase
  but no F06 implementation is included. Closeout commit/CI: awaiting push.
- Canonical FMAP hash before edits:
  `1442d649ff73ba39881f876a4bde33394df2784c`.

Current local logs remain ignored under `build/validation/Renderer3-*`.
The generated imgui.ini cache was moved under build/validation after closing
the last GUI. Canonical FMAP and both GUI sources match origin/main.
F03/F05 are closed; F06 is not started.

Real C03 screenshots under `docs/evidence/F03/windows/`:
`c03-renderer3-before.jpg`, `c03-renderer3-paint-redo.jpg`,
`c03-renderer3-fill.jpg`, `c03-renderer3-add-object.jpg`,
`c03-renderer3-remove-object.jpg`, `c03-renderer3-erase.jpg`,
`c03-renderer3-save.jpg`, `c03-renderer3-save-reopen.jpg`.

## Earlier SDL_GPU validation (historical)

Validation date: 2026-10-05 (America/Sao_Paulo).
The C03 launch failed before interactive validation. C04–C08 are
PENDING / NOT EXECUTED. Do not infer or fabricate results.
F03/F05 are not formally closed, and F06 is not started.
C00/C01/C02 passed on the starting SHA. After FIX-C03-01/02, the Studio
and Client rebuilt successfully and their 7 corresponding CTests passed.

## Environment

```text
Windows version: Microsoft Windows 11 Home, 10.0.26200 (PowerShell x64 7.6.5)
Starting repository SHA: 21cc7466b3c737e23f062f5cee7b38f6a501cefb
Graphics correction SHA tested: c0728702b910216de7c92142a98191985fb4f85e
Final repository SHA tested: PENDING
Configuration: Release
```

## C00 — Clean checkout

```text
Result: PASS
Working tree clean before validation: YES; git status --short produced no output
Starting SHA: 21cc7466b3c737e23f062f5cee7b38f6a501cefb
Notes: main checked out; git pull --ff-only reported Already up to date.
Git 2.55.0.windows.5; CMake 3.31.6-msvc6; VS 2022 Build Tools 17.14.41;
MSVC x64 19.44.35229. CMake was found in the VS installation and will be
added to PATH only for the validation process. No source changes.
```

## C01 — Full native build

Command:

```powershell
./scripts/build-native.ps1 -Configuration Release
```

```text
Result: PASS; build script exited 0
Studio build/tests: PASS; 5/5 CTests
Server build/tests: PASS; 6/6 CTests
Client build/tests: PASS; 2/2 CTests
fantasy-native-play-tests: PASS
Expected executables present: YES; Studio GUI, Server, Client CLI, Client GUI
Notes/fixes: No source changes or fixes were required. MSVC C++20 build,
Windows SDK 10.0.26100.0. Native command failures were configured to stop
the build so a failed external command could not be reported as PASS.
All six contract/relocation validators passed before compilation.
SDL commit: 829a65d769d935c4852f8159e964312c0957260a
ImGui commit: f1cc2ae15e53a861a874c3034aae6798fde194ab
Local transcript: build/validation/C01-build.txt (ignored; not staged)
```

## C02 — Headless two-process first play

Run 1:

```powershell
./scripts/run-native-play.ps1 -Configuration Release -Port 17171 -Character "Codex Headless Hero"
```

Run 2:

```powershell
./scripts/run-native-play.ps1 -Configuration Release -Port 17172 -Character "Codex Reconnect Hero"
```

```text
Run 1 result: PASS; port 17171; Codex Headless Hero
Run 2 result: PASS; port 17172; Codex Reconnect Hero
Initial position: 100,100,7 in both sessions
Moved position: 101,100,7 in both sessions (East)
Chunks/tiles: 4 chunks / 64 tiles in both sessions
Server exit codes: 0 in both sessions; script verifies WaitForExit and ExitCode
Client exit codes: 0 in both sessions; script verifies LASTEXITCODE
Orphan process check: 0 Fantasy Server/Client/Client GUI processes after each run
Notes/fixes: Each server reported listen=127.0.0.1 on its assigned port,
mode=serve-once, frames_received=4, frames_sent=10,
clean_disconnect=true and state=STOPPED. Client reported DISCONNECTED.
No source changes or fixes were required.
Local transcript: build/validation/C02-headless.txt (ignored; not staged)
```

## C03 — F03 Studio interactive gate

Expected screenshot directory:

```text
docs/evidence/F03/windows/
```

```text
Result: FAIL / BLOCKED after FIX-C03; GPU shader support rejected before visual validation
Studio opened: NO; process exited 1 before the interactive editor was usable
Four chunks visually present: PENDING
Paint: PENDING
Undo/Redo: PENDING
Fill across chunk boundary: PENDING
Add/Remove/Erase object: PENDING
Save/Reopen persistence: PENDING
Canonical world restored after evidence: PENDING
Before screenshot: PENDING
After Save/Reopen screenshot: PENDING
Notes/fixes: Launch attempted; interactive validation PENDING / NOT EXECUTED.
This session has no native desktop observation/input capability. A human
must execute all C03 visual operations and capture genuine screenshots.
Canonical FMAP hash before edits: 1442d649ff73ba39881f876a4bde33394df2784c
Pre-launch working tree: only this deliberate evidence-ledger change.
Observed error: SDL_CreateGPUDevice failed: No supported SDL_GPU backend found!
Diagnostic repeat with SDL_LOGGING=*=verbose exited 1 and reported:
SDL chose video backend 'windows'
WARNING: D3D12: Tier 2 Resource Binding is not supported
Installed GPU: Intel(R) HD Graphics 5500; driver 20.19.15.4703 (2017-06-08).
No SDL/Vulkan driver override was present before the diagnostic.
The initial attempt made no code changes. FIX-C03-01/02 below subsequently
changed only graphical device initialization; no alternate renderer was added.
Failure evidence: docs/evidence/F03/windows/STARTUP-FAILURE.md
```

### FIX-C03-01 / FIX-C03-02 / FIX-C03-03

Correction commit: `c0728702b910216de7c92142a98191985fb4f85e`
(`fix(graphics): support D3D12 tier-1 Intel GPUs`).

`Studio/UI/EditorApp.cpp` and `Client/UI/ClientApp.cpp` now create temporary
SDL properties and call `SDL_CreateGPUDeviceWithProperties`. SPIRV, DXIL,
MSL, METALLIB and debug mode remain enabled. Windows additionally enables
`SDL_PROP_GPU_DEVICE_CREATE_D3D12_ALLOW_FEWER_RESOURCE_SLOTS_BOOLEAN`.
No backend name is forced. Properties are destroyed after device creation,
and an error is captured before destroying them. Property setup failures also
release the properties/window and quit SDL.

The [official SDL documentation](https://wiki.libsdl.org/SDL3/SDL_CreateGPUDeviceWithProperties)
allows Tier 1 when at most eight storage resources are used across shader
stages. The pinned ImGui vertex and fragment shaders each declare zero storage
buffers/textures, so this initialization policy fits the existing rendering.

Executed sequentially:

```powershell
cmake --build build/studio --config Release
cmake --build build/client --config Release
ctest --test-dir build/studio -C Release --output-on-failure
ctest --test-dir build/client -C Release --output-on-failure
```

Both builds exited 0; Studio CTests 5/5 and Client CTests 2/2 passed.
Local transcript: `build/validation/FIX-C03-03-build.txt` (ignored).

The exact C03 Studio launch was then repeated without backend overrides.
It exited 1. A second diagnostic launch with temporary `SDL_LOGGING=*=verbose`
also exited 1 and produced:

```text
SDL chose video backend 'windows'
WARNING: D3D12: DXIL is not supported and DXBC is not being provided
Fantasy Studio GUI error: SDL_CreateGPUDeviceWithProperties failed: No supported SDL_GPU backend found!
```

The Tier 2 warning no longer appears; the new rejection occurs at the D3D12
shader-format support check. C03 remains failed, and all its visual operations,
Save/Reopen and screenshots remain NOT EXECUTED. C04/C05 and C06/C07 were
not advanced. Client graphical compatibility is compiled but not interactively
validated. No further renderer/shader changes were made after this failure.
No Fantasy process remained. Canonical FMAP hash remained
`1442d649ff73ba39881f876a4bde33394df2784c`.

## C04 — Visual Fantasy Client first play

Command:

```powershell
./scripts/run-visual-play.ps1 -Configuration Release -Port 17173 -Character "Codex Visual Hero"
```

Expected screenshot directory:

```text
docs/evidence/F05/windows/
```

```text
Result: PENDING
GUI opened: PENDING
Connected/in-world: PENDING
Initial position 100,100,7: PENDING
Four chunks / 64 tiles visible: PENDING
Semantic placeholder rendering: PENDING
Object markers: PENDING
Player marker: PENDING
Right move -> 101,100,7: PENDING
Left move -> 100,100,7: PENDING
North/South authoritative: PENDING
Rejected movement remains server-authoritative: PENDING
Close sends Disconnect: PENDING
Server exits automatically: PENDING
Initial screenshot: PENDING
Moved screenshot: PENDING
Notes/fixes: PENDING
```

## C05 — Visual reconnect / cleanup

Command:

```powershell
./scripts/run-visual-play.ps1 -Configuration Release -Port 17174 -Character "Codex Visual Reconnect"
```

```text
Result: PENDING
Reconnect clean: PENDING
Spawn reset: PENDING
Map resent: PENDING
Movement works: PENDING
No orphan processes: PENDING
Notes/fixes: PENDING
```

## C06 — Architecture confirmations

```text
Listener stayed loopback-only: PENDING
No OTBM used: PENDING
No protocol 10.98 used: PENDING
No TFS runtime used: PENDING
No OTClient used: PENDING
Client movement remained intent-only: PENDING
Server remained authoritative: PENDING
```

## C07 — Final regression

Commands:

```powershell
./scripts/build-native.ps1 -Configuration Release
./scripts/run-native-play.ps1 -Configuration Release -Port 17175 -Character "Final Regression Hero"
git status --short
git diff --check
```

```text
Result: PENDING
All automated gates green: PENDING
Final native play green: PENDING
Accidental FMAP changes: PENDING
Unexpected build/log/cache files: PENDING
Absolute persisted paths: PENDING
Orphan processes: PENDING
Final SHA: PENDING
```

## C08 — Closeout

Only fill this section after C00–C07 are actually PASS.

```text
F03 changed to PASS: PENDING
F05 changed to PASS: PENDING
ROADMAP updated: PENDING
F03 result updated: PENDING
F05 result updated: PENDING
AGENTS priority changed to F06: PENDING
F06 implementation intentionally not started in closeout commit: PENDING
Final closeout commit: PENDING
GitHub Actions run on exact final SHA: PENDING
```

## Failures and fixes

For every encountered failure, append an entry:

```text
Stage: C03
Symptom: Studio exited 1 with No supported SDL_GPU backend found!
Failing layer: SDL_GPU device selection / Windows graphics environment
Root cause: Current graphics environment failed SDL_GPU backend probing;
  diagnostic explicitly reports unavailable D3D12 Tier 2 Resource Binding.
Smallest fix: PENDING; requires a compatible graphics environment. No code fix.
Commit SHA: No correction commit; tested baseline remains
  21cc7466b3c737e23f062f5cee7b38f6a501cefb.
Direct tests rerun: Exact Studio startup with temporary verbose SDL logging.
Stage rerun result: Startup failed again with exit 1; interactive checks not run.
Affected regression result: C01 and both C02 sessions passed before this failure
  on the starting SHA. Later graphics corrections are recorded below.
  C07 NOT EXECUTED because C03 is blocked.
Canonical FMAP: Unchanged; hash 1442d649ff73ba39881f876a4bde33394df2784c.
Screenshots: NOT CAPTURED; no invented visual evidence.
```

```text
Stage: FIX-C03-03 / C03 startup repeat
Symptom: Studio exits 1 after enabling official D3D12 Tier 1 compatibility.
Failing layer: SDL D3D12 shader-format capability check.
Root cause observed: DXIL is not supported and DXBC is not being provided.
Smallest fix applied: SDL_CreateGPUDeviceWithProperties and Windows
  ALLOW_FEWER_RESOURCE_SLOTS in existing Studio/Client GPU initialization.
Commit SHA: c0728702b910216de7c92142a98191985fb4f85e
Direct tests rerun: Studio/Client Release builds PASS; CTests 5/5 and 2/2 PASS.
Stage rerun result: C03 startup fails again, exit 1, with a new shader diagnostic;
  no Tier 2 warning appears. Visual C03 checks remain NOT EXECUTED.
Affected regression result: Direct Studio/Client regressions passed;
  C07 full regression NOT EXECUTED because C03 failed.
Next correction: NOT IMPLEMENTED; stopped at the newly observed failure.
Cleanup: 0 Fantasy processes; canonical FMAP unchanged.
```
