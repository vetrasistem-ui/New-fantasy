# Fantasy TFS1098 Capability Matrix

**Status:** planning baseline derived from vanilla TFS 1.4.2, the current Fantasy 10.98 compatibility work and the audited Poketibia TFS 1.4 / OTClientV8 reference.

## Goal

Define what belongs in Fantasy itself, what belongs in the `Tfs1098` backend, what should stay as a game/system template, and what belongs exclusively to the client/UI layer.

The matrix deliberately avoids copying the audited fork's architecture. It converts observed needs into Fantasy-owned semantic capabilities.

## Layer definitions

- **Core/Domain** — neutral Fantasy concepts authored by Studio and reusable by any runtime.
- **Tfs1098 Backend** — adapter/exporter/runtime integration that maps Fantasy concepts to external TFS 1.4.2/10.98-compatible files, Lua, protocol extensions or process controls.
- **System Template** — optional gameplay package created in System Lab; not required by all games.
- **Client/UI** — visual module, HUD or interaction surface; may communicate through semantic system channels.

## Priority legend

- **P0** — required foundation for the first useful TFS-backed platform.
- **P1** — important advanced capability after the first runtime loop works.
- **P2** — reusable template/tooling once generic foundations are stable.
- **Later** — native-runtime/operations capability with no reason to block the current editor/runtime path.

## Capability matrix

| Capability | Evidence / need | Fantasy target layer | Priority | Rule |
|---|---|---|---|---|
| Runtime backend selection | TFS first, Fantasy Native later | Core/Domain | P0 | Project chooses backend without changing authored data |
| Runtime capability descriptor | Fork/client negotiate non-standard features | Core + Backend | P0 | Query capabilities instead of assuming every client/runtime supports them |
| Versioned system-message channel | Extensive Extended Opcode use | Core + Backend | P0 | Semantic channel names; numeric opcodes allocated by backend/profile |
| Script event bridge | Fork routes extended opcode to Lua events | Tfs1098 Backend | P0 | Generated/runtime adapter exposes safe Lua event hooks |
| Asset profile | Multiple incompatible DAT/SPR/OTB packs exist | Core + Backend | P0 | A project pins one coherent asset profile; never mix packs implicitly |
| Semantic item tags | `isPokeball`, `isHolder` show game-specific item flags are needed | Core/Domain | P0 | Generic tags/components, not hard-coded Pokémon flags |
| Entity archetype/components | Duelist demonstrates specialized entities | Core/Domain | P0 | Compose capabilities instead of subclassing TFS classes in Fantasy |
| Appearance attachments | Wings/Auras | Core/Domain | P1 | Generic appearance attachment slots/layers |
| Appearance shader/effect | Outfit shaders | Core/Domain + Client | P1 | Named effect reference; backend/client maps implementation |
| Zone/Region | Fork has named/id zones exposed to scripts | Core/Domain | P1 | First-class regions usable by systems, editor and runtime |
| Region triggers/rules | Zone events/loot/rules | System Template | P1 | Trigger/condition/action logic in System Lab |
| Generic capture mechanic | Pokéball/catch state in fork | System Template + optional domain components | P1 | Capture is a reusable mechanic, never a mandatory engine concept |
| Collection/companion roster | Pokémon roster/caches | System Template | P1 | Generic owned/summonable entity collection |
| Encounter/trainer controller | Duelist summons/controls roster | System Template | P1 | AI controller + encounter roster + rewards/dialogue |
| Custom item counters/progression | catch points and broken-ball counts | System Template | P1 | Generic keyed progress/counter storage |
| Client module manifest | OTClientV8 uses many independent modules | Client/UI | P1 | Fantasy client package declares optional modules and dependencies |
| UI <-> system messages | Bank/shop/craft/etc use opcodes | Client/UI + System Template | P1 | UI binds to semantic system channels |
| Screen shake | Exposed by fork Lua | Client/UI effect | P2 | Generic camera effect action |
| Animated text | Exposed by fork Lua | Client/UI effect | P2 | Generic world/UI effect action |
| Particle/effect channel | Reference base uses effects/shaders | Client/UI effect | P2 | Generic effect definition with runtime-specific renderer support |
| Spectating | Client/server have spectate support | Backend + Client/UI | P2 | Optional runtime capability |
| Craft | Client module + server system | System Template | P2 | Build from generic recipe/inventory primitives |
| Bank | Client module + server system | System Template | P2 | Currency/account storage template |
| Shop | Client module + server system | System Template | P2 | Catalog/currency/transaction template |
| Market | Client/server contain market functionality | System Template / Backend adapter | P2 | Optional economic system |
| Daily rewards | Client module | System Template | P2 | Schedule/state/reward template |
| Battle pass | Client module | System Template | P2 | Progress/reward track template |
| Contracts/tasks | Client modules + Duelist contract metadata | System Template | P2 | Quest/objective abstraction |
| Pokédex / bestiary | Client dex module | System Template | P2 | Generic discovery/encyclopedia template |
| Evolution | Dedicated opcode/module | System Template | P2 | Generic transformation/progression action |
| Autoloot | Several sync opcodes | System Template | P2 | Loot-rule/filter template |
| Boss health/ranking | Client modules | System Template + Client/UI | P2 | Encounter telemetry UI |
| Guild custom states | Fork Lua API extends guild status | System Template | P2 | Extendable guild metadata rather than hard-coded fork semantics |
| Account creation protocol | Fork adds client-side creation handlers | Tfs1098 Backend/Auth | Later | Separate from map/editor; use explicit auth capability |
| Character creation protocol | Fork adds direct character creation | Tfs1098 Backend/Auth | Later | Optional runtime/auth module |
| Runtime stats/slow-operation telemetry | Fork has `Stats` subsystem | Runtime operations | Later | Observability API independent of game logic |

## P0 implementation contract

Before implementing any Poketibia gameplay template, Fantasy should establish these neutral primitives:

```text
RuntimeCapabilities
SystemChannelId (semantic/string or stable UUID)
RuntimeMessage { channel, version, payload }
AssetProfileId
SemanticTag / Component metadata
EntityDefinition + components
```

The TFS backend may then provide a mapping such as:

```text
Fantasy channel: inventory.sync
    -> TFS profile opcode: N

Fantasy channel: creature.dex
    -> TFS profile opcode: N
```

Numeric values belong to the backend/profile, not to the Fantasy project model.

## P1 domain direction

### Appearance

```text
AppearanceDefinition
  baseVisual
  attachments[]
    slot
    visualRef
    layer
    offset
  effects[]
  shaderRef?
```

This can represent wings, auras, equipment visuals, halos, particles or game-specific overlays without introducing `Wing`/`Aura` classes into the engine.

### Zones

```text
ZoneDefinition
  id
  name
  geometry/tiles
  tags
  metadata

System rule
  trigger: entity enters Zone(tag=...)
  conditions: ...
  actions: ...
```

### Entity archetypes

```text
EntityDefinition
  Transform
  Appearance
  Health
  Movement
  Combat
  AIController
  Inventory?
  EncounterRoster?
  Interaction?
  ScriptBindings?
```

A Poketibia trainer, medieval NPC, boss controller or survival-game enemy can use the same foundation.

## Tfs1098 backend responsibilities after map homologation

The backend grows around the proven map export path in this order:

1. map / houses / spawns / assets profile;
2. generated runtime config;
3. process start/stop/log collection;
4. semantic system-channel registry -> extended opcode mapping;
5. generated Lua bridge/event registration;
6. generated semantic item tags/data;
7. appearance extensions when a compatible OTClient profile advertises them;
8. optional system templates;
9. account/auth extensions last.

## Compatibility profiles

Keep distinct profiles instead of pretending every 10.98 installation is identical.

```text
Tfs1098Vanilla
  TFS 1.4.2
  official 10.98 compatibility gate
  stock-compatible protocol/features

Tfs1098OtcExtended (future)
  TFS 1.4 lineage + OTClientV8 extensions
  Extended Opcode
  optional appearance attachments/shaders
  additional feature negotiation

Legacy854 (import only / compatibility lab)
  PokeAimar/PokeJornadas-era content
```

The audited Poketibia base is evidence for the second profile, not a reason to change the vanilla target.

## System Lab implications

System Lab should eventually expose the same generic building blocks required by this reference base:

```text
Triggers
- login/logout
- message/channel received
- enter/leave zone
- item use
- entity defeat
- timer/schedule
- interaction

Conditions
- tags/components
- zone membership
- inventory/currency
- progress counters
- entity/species/archetype
- ownership/unlock

Actions
- send system message
- spawn/remove entity
- modify component/state
- grant/remove item
- update progress
- change appearance/effect
- play UI/world effect
- open/update client module
- persist keyed data
```

This is enough to reproduce many Poketibia-style systems without hard-coding Poketibia into Fantasy.

## Non-goals

Do not make the following part of Fantasy Core merely because the audited fork does:

- `Pokeball` as a required engine class;
- Pokémon species fields on generic monsters;
- a dedicated `Duelist` subclass;
- fixed opcode numbers;
- TFS `Player`/`Creature`/`Monster` types;
- GPL implementation copied from the reference fork;
- direct dependency on OTClientV8 internals.

## Decision

The audited Poketibia base becomes the **primary advanced TFS-backed gameplay reference**.

Vanilla TFS 1.4.2 remains the **first compatibility/runtime target**.

Fantasy remains the **independent authoring/domain layer**, with generic capabilities that can target either an extended TFS backend or a future Fantasy Native Runtime.
