# Codex Handoff — Fantasy Foundation V2

**Validated implementation SHA:** `40a14c437f56e0a309aafbe7193ae6e87d88f78b`

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

The VPS builder contract also passes on both Windows and Ubuntu.

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

### Runtime / Server workspace backend

Implemented and tested:

- `vanilla` compatibility profile;
- `otc_extended` compatibility profile;
- Server Workspace V2 controller;
- client package selection state;
- semantic channels derived automatically from authored Systems;
- project health state;
- profile-dependent capabilities;
- Prepare Runtime;
- Extended Opcode registry;
- TFS Lua bridge;
- OTCv8 bridge/module;
- stable/idempotent channel bindings.

The remaining V5 Server-page work is visual binding to this controller, not runtime design.

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

Acceptance must test both the preferred executable and fallback when useful:

```text
otclient_dx.exe
otclient_gl.exe
```

Do not mark this PASS from a headless protocol smoke alone.

## Codex gate B — V5 UI visual binding/acceptance

The logic/controller already exists. Codex should bind the accepted V5 shell to the existing controllers rather than creating a new architecture.

At minimum, Server V2 should visually expose:

```text
Compatibility profile:
  Vanilla
  OTC Extended

TFS template directory
Runtime output
OTCv8 client package directory
Required semantic channels
Runtime capabilities
Project Health
Prepare Runtime
Export OTBM
Package Runtime
Start / Stop
Build VPS Bundle
Logs/status
```

When `Vanilla` is selected:

```text
system-channel capability = false
OTC bridge is not generated
client package is not required
```

When `OTC Extended` is selected:

```text
client package is required
channels come from authored Systems
Prepare Runtime generates/updates registry + server/client bridge
system-channel capability = true
```

Visual acceptance should also verify:

- no broken docking/overlap;
- resizing;
- scroll;
- DPI scaling;
- modal focus;
- keyboard shortcuts;
- error/status visibility;
- no regression in Map workspace.

## Codex gate C — authoring editor surfaces

The data/persistence/CRUD contracts already exist. Local UI work should consume them instead of writing raw JSON from widgets.

Editor surfaces to bind/test visually:

```text
Zones Editor
Appearance Editor
Item Editor
Creature / Monsters Editor
Class / Attribute / Equipment Editor
System Lab
Modern Assets / Migration
Brush configuration
Project Health / Build
```

Required UI architecture rule:

```text
UI
 -> controller/repository/domain
 -> persistence
```

Avoid:

```text
UI widget
 -> handcrafted JSON / TFS file directly
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
Windows OTCv8 interactive graphical acceptance            PASS locally
V5 visual bindings/editor interaction                     PASS locally
Real asset migration visual/rollback                      PASS locally
Windows install/uninstall                                 PASS locally
Real VPS install/login/persistence/update/rollback         PASS remotely
```

If a local gate exposes a defect, fix the smallest implementation layer responsible and rerun both the local gate and the repository regression suites.
