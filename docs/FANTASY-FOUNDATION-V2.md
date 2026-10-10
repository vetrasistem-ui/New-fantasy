# Fantasy Foundation V2

**Status:** headless architecture implemented; final cross-platform CI is the promotion gate.

## Goal

Foundation V2 moves Fantasy beyond a map/runtime compatibility project into a runtime-neutral 2D game-authoring platform without moving TFS implementation types into Fantasy Core.

The architecture remains:

```text
Fantasy Studio / Editors
        |
Fantasy-owned authoring models
        |
Project persistence / validation / build pipeline
        |
Runtime adapter contract
        |
+------------------------+
|                        |
TFS 1.4.2 / 10.98     Fantasy Native (later)
```

TFS remains an external runtime adapter. The authored game model belongs to Fantasy.

## Authoring layout

Foundation V2 owns deterministic project paths:

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

Typed filenames are deterministic and identifier-based, for example:

```text
Game/Zones/zone.town.center.zone.json
Game/Systems/system.zone.welcome.system.json
Game/Items/item.health_potion.item.json
Assets/Modern/asset.ground.grass.asset.json
```

`FantasyProjectLayoutV2` is the canonical path contract.

## JSON persistence

`FantasyPersistence` writes versioned JSON and validates the model again when it is loaded.

Supported persisted authoring objects currently include:

- zones;
- appearances;
- entity archetypes;
- items;
- creatures;
- classes;
- systems;
- asset profiles;
- asset migration plans;
- modern assets;
- brushes.

Unknown schema versions fail explicitly instead of being interpreted silently.

`FantasyAuthoringRepository` provides the headless CRUD layer used by future editor surfaces:

```text
save
load
list IDs
remove
```

The editor does not need to know filename construction or raw JSON details.

## Zones / Regions

`ZoneDefinition` is a first-class Fantasy concept.

A zone can contain:

- rectangles;
- explicit tiles;
- semantic tags;
- metadata.

The contract is runtime-neutral and can later be exported to TFS scripts or consumed directly by Fantasy Native.

Example uses:

```text
safe areas
PvP areas
houses
encounter regions
boss rooms
city areas
biomes
quest regions
social hubs
```

## Appearance

`AppearanceDefinition` separates appearance from entity/gameplay type.

It supports:

- base visual;
- attachment slots;
- layer ordering;
- X/Y offsets;
- named effects;
- optional shader reference.

This can express capes, wings, auras, equipment overlays, halos, particles and other visual extensions without introducing game-specific classes into Fantasy Core.

## Entity model

`EntityArchetype` uses tags + components rather than a TFS class hierarchy.

Example:

```text
entity.npc.merchant
  tags:
    entity.interactable
    npc.merchant

  components:
    interaction.shop
    dialogue.basic
```

The same architecture can model medieval NPCs, PokéTibia-style trainers, bosses, companion controllers or other game genres.

## Items, creatures and classes

Foundation V2 adds neutral authoring contracts for:

```text
ItemDefinition
CreatureDefinition
ClassDefinition
EquipmentDefinition
```

These are Fantasy objects. Legacy OTB/XML representations are export/import compatibility formats.

## Character attributes and equipment rules

The equipment contract implements the agreed Fantasy rule:

```text
minimum level
+ attribute requirements
```

Minimum level is mandatory and cannot be bypassed by equipment bonuses.

Attribute requirements are calculated from:

```text
natural attributes
+ distributed attributes
+ bonuses from equipment already equipped
```

The candidate item's own bonus is not included while evaluating whether that item can be equipped, preventing circular self-requirement exploits.

Example:

```text
Bow
  minimum level: 50
  strength: 30
  agility: 40

Player
  level: 50
  strength: 30
  agility: 20

Already-equipped boots
  agility +20

Result: bow may be equipped.
```

A level 10 player still cannot equip the level 50 bow even with enough bonus attributes.

## System Lab foundation

The first System Lab contract is runtime-neutral:

```text
Trigger
Condition
Action
```

Current trigger kinds include:

- login;
- logout;
- channel received;
- enter zone;
- leave zone;
- item use;
- entity defeat;
- timer;
- interaction.

Current condition kinds include:

- tag;
- component;
- zone membership;
- inventory;
- currency;
- progress;
- attributes;
- ownership.

Current actions include:

- send system message;
- spawn/remove entity;
- modify component;
- grant/remove item;
- update progress;
- change appearance;
- play effect;
- open client module;
- persist value.

`FantasySystemCatalog` validates dependencies, rejects dependency cycles, computes deterministic execution order and aggregates required semantic runtime channels.

### Advanced System Lab contract

`SystemPackageDefinition` adds the next authoring layer:

```text
Visual / Hybrid / Code mode
variables
persistent variables
events
timers
system tests
code entry (Hybrid/Code)
```

Structural test scenarios validate that their trigger/action references exist before a runtime export occurs.

## Semantic channels / TFS1098 OTC Extended

Authored systems reference semantic channels such as:

```text
ui.inventory
ui.notification
system.quest:v1
```

They never author fixed opcode numbers.

For `otc_extended`, the TFS adapter maps those semantic names into the Fantasy-owned opcode range and generates:

```text
TFS Lua Extended Opcode bridge
OTCv8 Lua bridge
OTCv8 .otmod module
creaturescript registration
login registration
```

The vanilla profile remains separate and does not advertise semantic system-channel support.

## Server Workspace V2 controller

`FantasyServerWorkspaceController` is the non-visual controller intended for the V5 Server page.

It owns:

- persisted compatibility profile (`vanilla` / `otc_extended`);
- TFS runtime template path;
- packaged runtime path;
- selected client package path;
- required channels derived from authored Systems;
- runtime capabilities;
- Project Health;
- Prepare Runtime operation.

The remaining UI work is a visual binding to this controller, not runtime-architecture work.

## Project Health

`FantasyProjectHealthScanner` inspects the real project directory.

It validates:

- project root and manifest;
- main map;
- every persisted Foundation object;
- System dependencies/cycles;
- asset profile existence;
- TFS runtime profile;
- runtime template configuration;
- OTC client package when `otc_extended` is selected.

Build preparation stops when blocking health issues exist.

## Build pipeline

`FantasyBuildPipeline` defines the headless sequence:

```text
Project Health
    -> load authored Systems
    -> validate dependency graph
    -> collect semantic channels
    -> prepare runtime profile
    -> generate bridges when extended
    -> create build manifest
    -> create client manifest
    -> generate VPS lifecycle files when requested
```

Supported logical build targets are:

```text
Validate
Runtime
Client
VPS
```

Validation always runs first. A VPS build requires a runtime build.

## Build manifest

`FantasyBuildManifest` records:

- project ID;
- build version;
- runtime ID;
- compatibility profile;
- asset profile;
- generated artifacts;
- optional checksums.

This is the base for future reproducible publishing and launcher updates.

## Client manifest / launcher contract

`FantasyClientManifest` owns:

- client ID/version;
- runtime/profile;
- asset profile;
- server host;
- login/game ports;
- update channel;
- required/optional modules;
- artifacts/checksums.

`FantasyClientUpdatePlanner` compares two compatible manifests and produces deterministic operations:

```text
Download
Remove
Keep
```

A normal update cannot silently switch runtime, compatibility profile or asset profile.

## Asset profiles

`AssetProfile` pins coherent asset sources by role and optional checksum.

Legacy projects can describe compatible sets such as:

```text
DAT
SPR
OTB
```

The design prevents implicit mixing of unrelated asset packs.

## Modern Fantasy assets

`ModernAssetDefinition` is the first runtime-neutral asset contract.

It supports:

- kind (ground, border, wall, doodad, item, creature, outfit, effect, missile, UI, generic);
- dimensions;
- semantic tags;
- visual layers;
- frame durations;
- offsets;
- terrain transition rules.

This is the base for eventually leaving DAT/SPR as an import/export compatibility format rather than Fantasy's permanent authoring format.

## Asset migration

`AssetMigrationPlan` supports:

```text
AddAsNew
ReplaceObject
ReplaceVisualOnly
```

`FantasyAssetMigrationEngine` performs deterministic ID allocation and validates replacement targets.

`AssetMigrationReceipt` records the applied changes and required backups.

Rollback is generated in reverse application order:

```text
remove newly-created target
restore replaced object
restore replaced visual
```

The visual comparison of real legacy/modern sprites remains a local Codex gate.

## Brush contracts

Foundation V2 defines reusable brush types:

```text
Terrain
AutoBorder
Wall
Doodad
Carpet
Table
Erase
```

Brush variants use weighted deterministic selection. Transition rules are ordered by explicit priority.

This allows previews and final commands to resolve the same variant from the same seed.

## VPS lifecycle

The deployment lifecycle generator adds:

```text
env.example
healthcheck.sh
update.sh
rollback.sh
release-policy.json
```

Update flow:

```text
backup current server
stop service
stage new server
start service
health check
  PASS -> keep release
  FAIL -> automatic rollback
```

Production secrets are not embedded in the generated source-controlled contract.

A real remote VPS remains an external acceptance gate.

## Windows installer contract

`FantasyWindowsInstallerBundle` generates a per-user Windows installation bundle with:

```text
install.ps1
uninstall.ps1
installer-manifest.json
```

Default installation scope is under `%LOCALAPPDATA%/Fantasy/...`, with optional desktop shortcut creation.

Actual installation/uninstallation acceptance remains a Codex/Windows interactive gate.

## Tests

Foundation V2 has dedicated suites for:

- runtime/domain contracts;
- JSON persistence roundtrips;
- authoring repository CRUD;
- build pipeline and Server Workspace controller;
- advanced System Lab/client update/asset rollback/installer contracts;
- existing TFS1098 runtime workflow integration.

Cross-platform CI is the promotion gate. Interactive Windows graphics, real user-owned assets and a real VPS are deliberately separate gates.
