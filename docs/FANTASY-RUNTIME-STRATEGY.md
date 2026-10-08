# Fantasy Runtime Strategy

**Status:** official direction as of 2026-10-08.

## Decision

Fantasy is our product and our architecture. The Forgotten Server 1.4.2 is the **first supported runtime**, not the source of truth and not the permanent core of the platform.

The shortest path is therefore:

```text
Fantasy Studio
      |
Fantasy Core / Data Model
      |
Runtime Backend contract
      |
TFS 1.4.2 / 10.98 exporter-adapter
      |
Generated runtime package
      |
TFS 1.4.2 process + compatible 10.98 client
```

Later, the same Fantasy project may target a native runtime:

```text
Fantasy Studio
      |
Fantasy Core / Data Model
      |
Runtime Backend contract
      |
Fantasy Native Runtime
      |
Fantasy Server + Fantasy Client
```

The project does not need to change its source data when the runtime changes.

## Ownership boundary

The following are Fantasy-owned components and must not depend on TFS internal C++ classes:

- Studio shell and editors;
- Map Core and MapDocument;
- Command API and Query API;
- selection, transactions, preview, undo/redo and revisions;
- Asset Registry and future native asset format;
- Brush Engine;
- AI/automation contracts;
- project model;
- future creature/item/quest/system domain models;
- runtime backend interface;
- TFS exporter/adapter code written by Fantasy;
- future Fantasy native client/server/runtime.

TFS remains an external GPL component. If a TFS build is distributed, its GPL obligations are handled for that component separately.

## Hard boundary

No TFS source file, class hierarchy, internal header or copied implementation is allowed inside Fantasy Core.

Forbidden dependency shape:

```text
Fantasy Core -> TFS::Player / TFS::Creature / TFS::Map / TFS headers
```

Allowed dependency shape:

```text
Fantasy Project
    -> Fantasy domain model
    -> Fantasy TFS1098 exporter/adapter
    -> OTBM / XML / Lua / config / DB migrations
    -> external TFS 1.4.2 runtime
```

The adapter may know the TFS 1.4.2 file/runtime contract. The Fantasy domain must not know TFS implementation details.

## Source of truth

A Fantasy project is always authored through Fantasy models. Legacy/Tibia formats are compatibility formats.

```text
OTBM / DAT / SPR / OTB / legacy Lua
                |
                v
        Fantasy Importers
                |
                v
        Fantasy Data Model
                |
        +-------+--------+
        |                |
        v                v
 TFS1098 Exporter   Fantasy Native Runtime
```

During the current map phase, OTBM remains necessary for real compatibility and homologation. The long-term rule is that imported content becomes Fantasy content after conversion.

## First runtime backend: TFS 1.4.2 / 10.98

The first production-capable backend should generate or manage a complete TFS-compatible runtime package from Fantasy project data.

Initial responsibilities:

1. export/compile map data to TFS-compatible OTBM v3;
2. export houses and spawns XML;
3. resolve asset/item identifiers for the selected 10.98 profile;
4. generate runtime configuration without placing TFS logic in the Studio core;
5. later generate Lua/configuration from Fantasy Systems where possible;
6. start/stop an external TFS process through a process boundary;
7. collect logs/status without linking TFS code into Fantasy.

Communication with an external runtime may use files, process I/O, sockets or a deliberately versioned control API. The TFS executable remains a separate program.

## Native runtime is incremental, not a restart

We do not stop the Studio to build a new server now.

Native runtime work begins only as small replaceable capabilities:

```text
Native V0
- load Fantasy map
- assets/rendering
- player transform
- movement
- collision
- camera

Native V1
- entities/creatures
- items/inventory
- effects
- combat

Native V2
- authoritative multiplayer
- persistence
- NPCs
- quests
- abilities/systems
```

TFS remains available until the native runtime proves enough capability to replace it.

## Current execution order

This architectural decision does **not** change the active editor sequence:

1. finish real DAT/SPR renderer in Studio V5;
2. load a real large map in canonical Map Core;
3. validate selection/clipboard/brushes on real data;
4. implement OTBM v3 Writer;
5. Save -> Reopen;
6. validate exported map in vanilla TFS 1.4.2;
7. validate with a compatible 10.98 client;
8. then grow the TFS runtime backend around the proven export path.

This keeps the shortest path to a usable product while preserving long-term independence.

## Legacy laboratories

Real legacy projects are compatibility/stress laboratories, not the architectural base:

- **PokeJornadas:** functional/reference project for complex gameplay and Studio needs;
- **PokeAimar:** heavy legacy map/assets/migration stress-test;
- **BlackTek/RME:** behavior and editor maturity references.

Their code/assets are not silently copied into Fantasy Core. Import/provenance and licensing remain explicit.

## License rule

The product architecture is designed so that proprietary or differently licensed Fantasy components can remain separate from an external GPL runtime. This is an engineering boundary, not legal advice. Before commercial distribution, the exact packaging and runtime communication model must receive a license/legal review.

If TFS is distributed, preserve its license/notices and satisfy the source-code obligations applicable to that distributed TFS build.

## Definition of success

The transition is successful when one Fantasy project can target either backend without changing its authored source model:

```text
                    Fantasy Project
                          |
              +-----------+-----------+
              |                       |
              v                       v
      TFS 1.4.2 / 10.98        Fantasy Native
        supported now           supported later
```

At that point TFS is a supported runtime, not the identity of Fantasy.