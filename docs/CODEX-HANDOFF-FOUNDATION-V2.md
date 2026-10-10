# Codex Handoff — Fantasy Foundation V2

**Validated implementation SHA:** `909f702e048fc2f390bcdd907839632f19c9046d`

**Repository:** `vetrasistem-ui/New-fantasy`  
**Branch:** `feature/fantasy-map-core-v1`  
**PR:** `#3`  
**Handoff rule:** do not redesign the architecture during local acceptance. Fix concrete integration/runtime/UI defects against the contracts already present.

## CI state at handoff

The validated implementation SHA completed:

```text
Foundation Checks                 SUCCESS
OTCv8 Visual Homologation Client  SUCCESS
TFS1098 VPS Bundle Tool           SUCCESS
```

Windows Foundation evidence includes:

```text
Fantasy Studio configure/build       PASS
Fantasy Studio full CTest matrix     PASS
Fantasy Studio artifact upload       PASS
Fantasy Server configure/build       PASS
Fantasy Server/native integration    PASS
Fantasy Client configure/build       PASS
Fantasy Client smoke                 PASS
Two-process native play              PASS
Native runtime artifact upload       PASS
```

Linux evidence includes:

```text
legacy/runtime configure             PASS
legacy/runtime build                 PASS
TFS1098 runtime workflow tests       PASS
```

The VPS deployment contract has two independent implementations under test:

```text
C++ Tfs1098DeploymentBundle contract   PASS Windows + Ubuntu
Python operational bundle builder      PASS Windows + Ubuntu
unsafe output/backup-path rejection    PASS Windows + Ubuntu
```

The standalone C++ contract lives at:

```text
Tools/Tfs1098/DeploymentBundleContract/
```

## Architecture frozen for acceptance

The source of truth is:

```text
Fantasy Project
  -> Fantasy authoring/domain contracts
  -> versioned JSON persistence
  -> authoring repository / health / build pipeline
  -> Runtime Backend
  -> TFS1098 vanilla or OTC Extended
```

Do not move TFS classes/headers into Fantasy Core.

Do not hard-code gameplay-specific Pokémon concepts into the neutral Foundation.

Do not expose authored systems to fixed numeric extended opcodes. Systems author semantic channels; the TFS1098 adapter owns opcode allocation.

## Foundation V2 implemented before handoff

### Project/domain

Implemented and tested:

- Zones/Regions;
- Appearance + attachments/layers/effects/shader reference;
- Entity archetypes + tags/components;
- Items;
- Creatures;
- Classes;
- attributes;
- equipment requirements and bonuses;
- minimum-level hard gate;
- anti-circular self-bonus equipment rule;
- System Lab Trigger/Condition/Action foundation;
- Visual/Hybrid/Code System package contract;
- System variables/events/timers/test scenarios;
- system dependency validation/cycle detection/order;
- semantic channel aggregation;
- Asset Profile;
- modern Fantasy asset definition;
- legacy asset migration plan/preview;
- migration receipt + rollback plan;
- advanced brush contracts;
- deterministic brush variant selection.

### Persistence / CRUD

Versioned JSON persistence is implemented for the authoring model, with unknown-schema rejection.

`FantasyAuthoringRepository` owns deterministic save/load/list/remove behavior.

Canonical authoring layout:

```text
Game/
  Zones/
  Appearances/
  Entities/
  Items/
  Creatures/
  Classes/
  Systems/
  Brushes/

Assets/
  Profiles/
  Modern/
  Migrations/

build/
```

### Runtime / Server Workspace V2

Implemented, bound to V5 and compiled/tested:

- `vanilla` compatibility profile;
- `otc_extended` compatibility profile;
- Server Workspace V2 controller;
- V5 compatibility-profile selector;
- OTCv8 client package selection;
- semantic channels derived automatically from authored Systems;
- runtime capabilities display;
- Project Health display;
- Prepare Extended Runtime;
- Extended Opcode registry;
- TFS Lua bridge;
- OTCv8 bridge/module;
- stable/idempotent channel bindings;
- existing Export OTBM / Package Runtime / Start / Stop / Build VPS controls retained;
- runtime PID/status/log retained.

The Server page is no longer an unbound shell. Codex must visually accept and refine it in a real desktop session, not invent another runtime architecture.

### Foundation V2 authoring surfaces bound to V5

The real runtime V5 now routes the previous placeholder pages into repository-backed authoring surfaces.

Current compiled bindings:

```text
Items & Assets
  -> Items
  -> Appearances
  -> Modern Assets
  -> Asset Profiles
  -> Brushes
  -> Asset Migrations

Monsters
  -> Creatures
  -> Classes

NPCs
  -> Entity Archetypes

Spells page
  -> Class / progression compact editor

Quests / Systems
  -> System Lab compact editor
  -> Zones
  -> Project Health

Client
  -> Project Health / build-readiness surface
```

These surfaces call `FantasyAuthoringRepository` and domain validators. Widgets do not handcraft JSON or write TFS files directly.

The current editors intentionally expose compact/core fields first. Advanced nested UI such as full component rows, Appearance attachment/effect editors and the final visual block graph for System Lab remains a local UX refinement on top of already implemented domain/persistence contracts. Do not replace the domain model to implement those interactions.

### Build / distribution

Implemented and tested:

- real Project Health scanner;
- `Validate -> Runtime -> Client -> VPS` build plan;
- build manifest;
- client manifest;
- client update planner (`Download / Remove / Keep`);
- VPS deployment bundle;
- systemd service;
- first-install script;
- `env.example`;
- health check;
- backup-before-update;
- update script;
- automatic rollback on failed health check;
- explicit rollback script;
- release policy manifest;
- standalone C++ deployment contract test;
- cross-platform Python deployment builder contract;
- Windows per-user installer/uninstaller bundle contract.

Production credentials are deliberately excluded from generated source-controlled artifacts.

## Local Windows acceptance sequence

Start from the validated branch/head and first reproduce CI locally:

```powershell
cmake -S Studio -B build/studio
cmake --build build/studio --config Release
ctest --test-dir build/studio -C Release --output-on-failure

cmake -S Server -B build/server
cmake --build build/server --config Release
ctest --test-dir build/server -C Release --output-on-failure

cmake -S Client -B build/client
cmake --build build/client --config Release
ctest --test-dir build/client -C Release --output-on-failure

./scripts/run-native-play.ps1 -Configuration Release -Port 17171 -Character "Codex Hero"
```

The deployment contract can also be reproduced independently:

```powershell
cmake -S Tools/Tfs1098/DeploymentBundleContract -B build/vps-cpp
cmake --build build/vps-cpp --config Release
ctest --test-dir build/vps-cpp -C Release --output-on-failure
```

Do not promote a local change when these regressions fail.

## Codex gate A — OTCv8 Windows interactive graphical acceptance

This cannot be inferred from CI.

Required proof:

```text
normal Windows desktop session
  -> clean packaged OTCv8 Windows client
  -> user-owned matching 10.98 DAT/SPR
  -> TFS1098 runtime
  -> login
  -> character list
  -> enter game
  -> map visibly rendered
  -> movement visible
  -> screenshot/evidence
```

The reusable packager is:

```text
Tools/Tfs1098/package_otcv8_windows_client.ps1
```

The CI/public package intentionally excludes Tibia DAT/SPR. Inject only user-owned matching assets locally with the packager's `-AssetsRoot` option.

Acceptance should test both the preferred executable and fallback when useful:

```text
otclient_dx.exe
otclient_gl.exe
```

Do not mark this PASS from a headless protocol smoke alone.

## Codex gate B — V5 visual / interaction acceptance

The functional bindings already compile in the Studio executable. Codex should open them and test the existing implementation rather than rebinding the architecture from scratch.

Server V2 must be visually verified for:

```text
Vanilla / OTC Extended selector
TFS template directory
Runtime output
OTCv8 client package directory
Required semantic channels
Runtime capabilities
Project Health
Prepare Extended Runtime
Export OTBM
Package Runtime
Start / Stop
Build VPS Bundle
Logs/status
```

When `Vanilla` is selected:

```text
system-channel capability = false
OTC bridge is not required
client package is not required
```

When `OTC Extended` is selected:

```text
client package is required
channels come from authored Systems
Prepare Runtime generates/updates registry + server/client bridge
system-channel capability = true
```

Visually test:

- docking/overlap;
- resizing;
- scroll;
- DPI scaling;
- focus;
- keyboard shortcuts;
- error/status visibility;
- Map workspace regression;
- switching between all new Foundation V2 pages;
- create/save/load/delete flows where exposed;
- Project Health refresh after authoring changes.

## Codex gate C — advanced authoring interaction refinement

Core editor surfaces are already wired to the repository/domain layer. Local work is limited to interaction and visual refinement where the compact editor does not yet expose every nested domain field.

Priority refinements:

```text
Appearance attachments / effects / layers
Entity component rows + property editing
Item / Creature component rows
full Trigger / Condition / Action visual graph
System Lab variables / timers / events / isolated scenarios
zone selection from the real map viewport
asset migration preview / conflict visualization
brush visual preview
Project Health / Build action presentation
```

Required architecture remains:

```text
UI
 -> controller/repository/domain
 -> persistence
```

Never replace it with:

```text
UI widget
 -> handcrafted JSON / direct TFS file
```

## Codex gate D — real legacy asset migration visual acceptance

Use actual user-owned/reference legacy + modern packs only in the local environment.

Exercise all modes:

```text
AddAsNew
ReplaceObject
ReplaceVisualOnly
```

Required proof:

- preview target IDs;
- collision handling;
- visual comparison;
- apply migration;
- generated receipt;
- rollback in reverse application order;
- reopened project remains valid.

Do not replace the neutral migration engine with a one-off DAT/SPR copier.

## Codex gate E — Windows installer acceptance

The headless generator already produces the contract.

Local acceptance must prove:

```text
install.ps1
  -> installs under %LOCALAPPDATA%/Fantasy/...
  -> executable starts
  -> StudioVisual/resources resolve
  -> desktop shortcut works when enabled

uninstall.ps1
  -> removes installed product
  -> removes generated shortcut
  -> does not delete unrelated user/project data
```

This is an interactive OS gate, not a CI inference.

## Codex gate F — real remote Linux VPS

Use a real Linux VPS with explicit credentials supplied locally/outside source control.

Current bundle contract contains:

```text
server/
deploy/
  fantasy-tfs1098.service
  install.sh
  README.md
  manifest.json
  env.example
  healthcheck.sh
  update.sh
  rollback.sh
  release-policy.json
```

Required first-install proof:

```text
copy bundle to VPS
  -> configure MariaDB
  -> import official TFS schema
  -> configure private production config.lua / credentials
  -> sudo bash deploy/install.sh
  -> systemd active
  -> login/game port reachable
  -> real client login
  -> game entry
```

Required persistence proof:

```text
login
 -> change/save state
 -> logout
 -> restart service
 -> relogin
 -> persisted state restored
```

Required update proof:

```text
known working release installed
 -> run deploy/update.sh
 -> backup created
 -> new service starts
 -> deploy/healthcheck.sh PASS
```

Required rollback proof:

```text
intentionally invalid test release in controlled test
 -> health check fails
 -> automatic rollback executes
 -> previous server restored
 -> service returns active
```

Never commit production DB passwords, SSH keys or other secrets to the repository or generated public artifacts.

## Codex gate G — large real-project performance/visual acceptance

Use the real large 10.98 project and user/reference assets to validate:

- map opening time;
- camera movement;
- renderer stability;
- editing;
- Undo/Redo;
- clipboard;
- save;
- reopen;
- memory behavior;
- large-zone editing;
- brush preview/commit consistency.

This is not a reason to redesign MapDocument or return TFS types to Fantasy Core.

## Definition of Codex completion

Codex is complete when the remaining environment-dependent gates have evidence, not merely when the project compiles.

Final expected state:

```text
Headless architecture / persistence / build contracts     PASS (already proven by CI)
Foundation V2 functional V5 bindings                      PASS compile/CTest
C++ + Python VPS deployment contracts                     PASS Windows + Ubuntu
Windows OTCv8 interactive graphical acceptance            PASS locally
V5 visual/editor interaction                              PASS locally
Advanced nested authoring interactions                    PASS locally
Real asset migration visual/rollback                      PASS locally
Windows install/uninstall                                 PASS locally
Real VPS install/login/persistence/update/rollback         PASS remotely
```

If a local gate exposes a defect, fix the smallest implementation layer responsible and rerun both the local gate and the repository regression suites.
