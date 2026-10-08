# Poketibia TFS 1.4 Runtime Audit

**Status:** reference audit, 2026-10-08.

## Purpose

This document records what was observed in the user-provided Poketibia `server-auditoria.zip` and `client-auditoria.zip` subsets and how that reference should influence Fantasy without becoming Fantasy's codebase.

The base is valuable because it demonstrates a complex Poketibia built on a TFS 1.4 lineage with an OTClientV8-derived client and 10.98-era assets/protocol compatibility.

## Provenance and license boundary

The server source explicitly identifies itself as being based on TFS 1.4 and retains GPL-2.0 notices inherited from The Forgotten Server.

Fantasy rule:

- this base is a **behavior/capability/reference laboratory**;
- do not copy its C++ implementation into Fantasy Core;
- do not make Fantasy domain models depend on its classes or headers;
- any external TFS-derived runtime remains a separate GPL component;
- provenance must be retained for every behavior or compatibility rule learned from this base.

## Version profile

Vanilla TFS 1.4.2 declares client compatibility `1097..1098` and version string `10.98`.

The audited fork declares:

```text
CLIENT_VERSION_MIN = 1097
CLIENT_VERSION_MAX = 1099
CLIENT_VERSION_STR = 10.99
```

The supplied client starts with `APP_VERSION = 1098` and enables a set of OTClientV8/custom game features. Treat this reference as a **10.98/10.99-compatible customized runtime**, not as a pristine 10.98 profile.

Fantasy's first official compatibility target remains **vanilla TFS 1.4.2 + 10.98**. Features discovered here belong in optional runtime capabilities/extensions.

## Structural delta from vanilla TFS 1.4.2

The fork's `src/CMakeLists.txt` keeps the normal TFS server domains and adds or substitutes several runtime units.

Observed added/custom units include:

- `auras`;
- `duelist`;
- `iocreateaccountdata`;
- `iocreatecharacterdata`;
- `market`;
- `pathfinding`;
- `protocolcreateaccount`;
- `protocolcreatecharacter`;
- `shaders`;
- `stats`;
- `waitlist`;
- `wings`;
- `zone`.

Relative to the official v1.4.2 CMake source list, `iomarket` and `storeinbox` are not present under those original units, indicating the fork has diverged in those areas as well.

This is not simply TFS with Lua content layered on top. The engine, Lua bridge, protocol and client were modified together.

## Generic runtime capabilities discovered

### Extended-opcode event bus

The server accepts OTClient extended opcode packet `0x32`, routes it through `Game::parsePlayerExtendedOpcode`, and exposes `CreatureEvent.onExtendedOpcode` to Lua.

The client uses `ProtocolGame.registerExtendedOpcode(...)` and `sendExtendedOpcode(...)` across many modules.

This is the most important reusable pattern in the base: complex game systems can remain mostly script/UI driven while the runtime exposes one versioned message channel.

Fantasy interpretation:

```text
Fantasy System / UI
        |
versioned system message
        |
Tfs1098 Runtime Backend
        |
TFS Extended Opcode + Lua event
```

Do not hard-code individual Poketibia opcodes into Fantasy Core. Model a generic message contract and let a backend map named Fantasy channels to runtime-specific opcode numbers.

### Runtime feature negotiation

The server advertises non-standard OTClient features including:

- `GameExtendedOpcode`;
- `GameWingsAndAura`;
- `GameOutfitShaders`.

The client explicitly enables additional rendering/protocol features such as alpha-channel sprites, U16 magic effects, aura offsets/front-back drawing and related OTClientV8 capabilities.

Fantasy should represent this as a backend capability set rather than assumptions baked into projects.

### Cosmetic attachment system

The fork adds first-class:

- Auras;
- Wings;
- Outfit Shaders.

Auras and wings have server-side definitions loaded from XML and include IDs, client IDs, names and optional speed/premium metadata. Shaders are similarly loaded and referenced by name/ID.

The player's outfit state was extended with wing/aura/shader data and the protocol serializes those values to the custom client.

Fantasy should generalize this concept as **appearance attachments/effects**, not as three hard-coded Poketibia-only concepts.

Suggested neutral model:

```text
Appearance
  baseLook
  attachments[]
  shader/effect
  layers
  offsets
  ownership/unlock rules
```

### Zone system

The fork includes a first-class `Zone` runtime type with an ID, name, position ranges and tile membership. The Lua bridge exposes zone queries on `Game`, `Tile` and `Creature`.

This is directly relevant to Fantasy System Lab because zones can drive:

- events;
- loot modifiers;
- PvP rules;
- quests;
- environmental effects;
- music/lighting;
- encounters;
- scripted triggers.

Fantasy should have a neutral Zone/Region domain rather than importing this exact implementation.

### Script bridge extensions

Observed custom Lua-facing capabilities include:

- `Game.createDuelist`;
- `Game.sendAnimatedText`;
- `Game.getZoneById`;
- `Position.shakeScreen`;
- Zone object methods;
- Tile/Creature zone access;
- Player aura/wing/shader management;
- Player current/cached Pokeball access;
- catch-point/broken-ball data;
- `Duelist` scripting;
- custom guild states;
- custom `ItemType` flags;
- `MonsterType.catchChance` and Pokémon rank data;
- `CreatureEvent.onExtendedOpcode`.

This reinforces the desired Fantasy architecture: gameplay tools should target a stable semantic scripting API while the backend translates that to TFS Lua/C++ capabilities.

### Custom item semantics

The fork extends item definitions with at least:

- `isPokeball`;
- `isHolder`.

Fantasy should not place these two booleans in the generic engine. The reusable lesson is to support **semantic item tags/components** that a runtime exporter can translate into TFS-compatible attributes, XML or generated scripts.

### Duelist / trainer entity

`Duelist` is a custom `Creature` subtype representing an autonomous trainer-like entity. It has targeting/friend logic, movement/combat AI, health, voices, a list of Pokémon/monster types, contract metadata and logic to summon a random Pokémon.

Fantasy should generalize this as an **entity archetype + AI controller + encounter roster**, not copy a special Duelist class.

Potential Fantasy components:

```text
Entity
 + Transform
 + Appearance
 + Health
 + AIController
 + EncounterRoster
 + Dialogue/Voice
 + Reward/Contract metadata
```

### Runtime telemetry

The fork contains a `Stats` thread/system for dispatcher/Lua/SQL/special execution measurements and slow-operation reporting.

This is useful as an operational reference for future Fantasy runtime observability, but it is not needed in the first TFS1098 backend milestone.

### Account/character creation protocol

The fork includes custom protocol handlers and database helpers for account and character creation from the client.

This is an optional backend capability. Fantasy should keep account/auth architecture separate from the editor/map milestone and expose it later through a runtime/auth adapter.

## Pokémon-specific engine state

Some Poketibia concepts were moved directly into C++ in this fork:

- current/used Pokeball state and caches on `Player`;
- per-Pokémon broken-ball counters;
- catch points;
- `MonsterType.catchChance`;
- Pokémon rank;
- `isPokeball` / holder item metadata;
- trainer/Duelist Pokémon roster.

These are evidence that the game outgrew purely external Lua in some performance/convenience areas. They are **not** evidence that Fantasy must hard-code Pokémon concepts.

Fantasy should model equivalent mechanics generically:

```text
capture mechanic
collection/companion entity
capture device/item tag
species/archetype metadata
progress/counter component
encounter roster
```

Poketibia then becomes one configuration of those primitives.

## Client architecture observations

The supplied client is an OTClientV8-derived client with full C++ source plus Lua/OTUI modules.

Observed module domains include:

- bank;
- boss health/ranking;
- account/login UI;
- contracts;
- action bar;
- autoloot;
- battle;
- craft;
- creature information;
- daily rewards;
- dex;
- donation/payment UI;
- evolution;
- guild management;
- inventory;
- market;
- minimap;
- NPC dialogue;
- pass;
- Pokébar;
- Pokémon moves/team;
- shaders;
- shop;
- spectate;
- starter guide;
- tasks;
- updater.

The architecture demonstrates an important separation that Fantasy should preserve: many game features can be delivered as **client modules + server system messages** rather than by changing the low-level protocol for each feature.

## Observed extended-opcode examples

The audited client uses many named/literal extended opcode channels. Examples observed include:

- 17 — inventory-related custom channel;
- 20 — pass;
- 23 — evolution;
- 52 — Pokémon skill/move bar;
- 54 — NPC dialogue;
- 58 — creature/loot information;
- 69 — new Pokémon UI;
- 70 — shader control;
- 72 — craft;
- 74 — payment/donation flow;
- 75 — bank;
- 76 — minimap;
- 77 — broken-ball/capture information;
- 78 — roulette/system UI;
- 87 — shop;
- 89 — information;
- 95 — task/kill system;
- 97 — contracts;
- 99 — guild management;
- 114 — spectate;
- 106/107/108/109/170 — autoloot synchronization.

The numbers themselves are reference implementation details. Fantasy should use semantic channel identifiers and allocate/translate numeric opcodes inside the TFS backend profile.

## Asset profile

The audited client/server subset represents a heavily extended 10.98-era asset profile rather than the stock pack already used by Fantasy for the current map gate.

Important rule: DAT/SPR/OTB files from different packs must not be mixed. A DAT from this reference requires its corresponding SPR/OTB profile for faithful rendering/runtime compatibility.

The current Fantasy 10.98 pack remains the map-editor homologation profile until an explicit second asset profile is imported.

## What Fantasy should learn vs. what it should not copy

### Learn / reproduce as independent capabilities

- generic runtime capability negotiation;
- generic system message / extended-opcode bridge;
- appearance attachment/effect model;
- Zone/Region domain;
- scriptable entity archetypes and AI controllers;
- semantic item tags;
- generic capture/collection mechanics;
- modular client UI driven by system messages;
- runtime telemetry/diagnostics;
- optional auth/account adapter.

### Do not copy into Fantasy Core

- TFS class hierarchy;
- the fork's `Player`, `Creature`, `Monster`, `Duelist` implementations;
- raw protocol implementation;
- GPL source files;
- hard-coded Pokeball/Pokémon fields;
- numeric opcode allocation as a Fantasy-wide contract;
- assumptions that every game must use Wings/Auras/Pokémon.

## Reference role going forward

Use the references as follows:

```text
Vanilla TFS 1.4.2
  -> compatibility oracle / first external runtime

Audited Poketibia TFS 1.4 fork
  -> advanced runtime capability laboratory

Audited OTClientV8 fork
  -> modular client/UI/protocol laboratory

PokeAimar + PokeJornadas
  -> legacy 8.54 migration/content laboratories

Fantasy
  -> independent authoring model, Studio and runtime-backend contracts
```

## Immediate consequences

1. Keep the active real-map/editor gate unchanged.
2. Add runtime capability descriptors before implementing Poketibia-specific systems.
3. Design System Lab messages around semantic channels, not fixed opcode numbers.
4. Generalize cosmetics as attachments/effects.
5. Generalize Pokémon capture/rosters as optional components/templates.
6. Treat this base as the primary advanced reference for what a mature TFS-backed game may require.
7. Revisit the full source only when implementing the corresponding Fantasy capability; retain provenance for every finding.
