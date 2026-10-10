# Fantasy Runtime Strategy

**Status:** official direction as of 2026-10-09.

## Decision

Fantasy is our product and our architecture. The Forgotten Server 1.4.2 is the **first supported runtime**, not the source of truth and not the permanent core of the platform.

The current supported path is:

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
TFS 1.4.2 process + MariaDB + compatible 10.98 client
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

`Tfs1098RuntimeBackend` V1 implements the first real external-runtime boundary.

Implemented responsibilities:

1. export the canonical map through the Fantasy OTBM v3 writer;
2. stage an external TFS 1.4.2 template into a generated runtime directory;
3. overlay the selected 10.98 `items.otb`;
4. preserve/copy house and spawn XML files expected by the map metadata;
5. generate or patch `config.lua` with the Fantasy target map name;
6. start and stop the external TFS process without linking TFS into Fantasy Core;
7. expose runtime state, PID and incremental stdout/stderr logs;
8. provide Windows `CreateProcess` and POSIX `fork/exec` process boundaries;
9. keep portable target settings in `Game/Config/tfs1098.runtime.json`;
10. keep the external TFS installation/template path as machine-level Studio configuration.

Future backend work includes generation of Lua/configuration from Fantasy Systems, richer health/status checks, user-friendly database provisioning and deployment-oriented packaging.

## V5 Server workspace

The accepted V5 shell remains the visual baseline. The runtime-enabled V5 entry adds an operational **Server** workspace without moving TFS code into the editor domain.

Current controls:

```text
TFS template directory   (machine-level)
Map name                 (project profile)
Runtime output           (project-relative profile)

Save target
Export OTBM
Package runtime
Start
Stop

Runtime state / PID
Incremental runtime log
```

The Map workspace `Save` action exports the current canonical `MapDocument` to the configured TFS1098 export path.

The executable build uses `StudioAppV5RuntimeEntry.cpp`; the earlier `StudioAppV5Entry.cpp` remains in the repository as a rollback/reference point for the accepted shell integration.

## Runtime tests

### Backend lifecycle — PASS

`fantasy-runtime-backend-tests` proves:

```text
synthetic TFS template
      -> package
      -> launch real child process
      -> capture logs
      -> observe Running/PID
      -> stop
      -> observe Stopped
```

This passes on Windows and Linux.

### Export/package workflow — PASS

`fantasy-tfs1098-workflow-tests` proves:

```text
MapDocument
    -> LegacyOtbmWriter
    -> Tfs1098RuntimeProfile
    -> Tfs1098RuntimeBackend::packageProject
    -> generated TFS-compatible runtime layout
```

This passes on Windows and Linux. The OTBM writer output buffer is heap-backed, avoiding the Windows stack-overflow regression found while adding this workflow test.

## Real MariaDB and persistence — PASS

The earlier protocol gate used a deterministic MySQL-protocol fixture only for persistence rows. That was sufficient to prove map/protocol compatibility but intentionally did not count as production database persistence.

The real-database homologation now passes through:

```text
.github/workflows/tfs142-mariadb-homologation.yml
```

The workflow is manual (`workflow_dispatch`) and uses:

- MariaDB 10.11;
- official TFS 1.4.2 `schema.sql`;
- official TFS 1.4.2 release binary;
- a real account `fantasy`;
- a real player `Fantasy Test`;
- protocol/client version 10.98;
- `Tools/Tfs1098/tfs1098_protocol_probe.py` for the gameworld handshake.

### Bootstrap proof

Observed:

```text
players_online_after_start=0
db_version_after_start=30
TFS142_MARIADB_BOOTSTRAP PASS

> Updating database to version 29 (account storages)
> Database has been updated to version 30.
>> Forgotten Server Online!
```

This proves that the official schema is accepted, TFS connects to real MariaDB, performs its own schema migration and executes startup persistence maintenance.

### Save / restart / relogin proof

Two complete player sessions were executed with a full TFS process restart between them.

```text
initial_state=100,100,7|0|0

cycle1_players_online_during_session=1
GAME_PACKET opcode=0x17
GAME_LOGIN PASS
GAME_LOGOUT PASS
cycle1_players_online_after_logout=0
state_after_cycle1=95,117,7|1791544061|1791544065

cycle2_players_online_during_session=1
GAME_PACKET opcode=0x17
GAME_LOGIN PASS
GAME_LOGOUT PASS
cycle2_players_online_after_logout=0
state_after_cycle2=95,117,7|1791544071|1791544075

TFS1098_MARIADB_PERSISTENCE PASS
persisted_position=95,117,7
```

The seeded invalid placement `100,100,7` was resolved by TFS to a valid runtime position `95,117,7`; that position was saved on logout and reused after a complete server restart. `players_online`, `lastlogin` and `lastlogout` also changed through the real database lifecycle.

Detailed evidence is kept in `TFS142-OTBM-RUNTIME-HOMOLOGATION.md`.

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

## Current execution status

1. **PASS** — real large 10.98 project -> canonical Map Core;
2. **PASS** — real DAT/SPR renderer with RGBA variant support;
3. **PASS** — real selection/brush/Undo-Redo/clipboard edit smoke;
4. **PASS** — OTBM v3 writer;
5. **PASS** — Fantasy Save -> Reopen semantic roundtrip;
6. **PASS** — generated OTBM accepted by official vanilla TFS 1.4.2;
7. **PASS** — compatible 10.98 login -> character list -> game entry -> movement;
8. **PASS** — `Tfs1098RuntimeBackend` lifecycle on Windows and Linux;
9. **PASS** — portable per-project TFS1098 target profile;
10. **PASS** — V5 Server workspace with Export/Package/Start/Stop/status/PID/logs;
11. **PASS** — headless MapDocument -> OTBM -> profile -> TFS package workflow;
12. **PASS** — official TFS 1.4.2 + real MariaDB bootstrap and schema migration;
13. **PASS** — real 10.98 login -> logout/save -> TFS restart -> relogin persistence;
14. **NEXT** — selected full Windows/OTClient 10.98 visual acceptance and deployment/VPS packaging;
15. **AFTER** — convert advanced PokéTibia reference capabilities into generic Fantasy-owned systems.

This keeps the shortest path to a usable product while preserving long-term independence.

## Reference laboratories

Real projects and upstreams are laboratories, not the architectural base:

- **Vanilla TFS 1.4.2:** compatibility oracle and first supported external runtime;
- **Audited PokéTibia TFS 1.4 / OTClientV8 base:** primary advanced runtime capability laboratory for extended opcodes, modular UI, zones, appearance extensions, generic capture/collection needs and other mature TFS-backed gameplay patterns; see `POKETIBIA-TFS14-RUNTIME-AUDIT.md` and `FANTASY-TFS1098-CAPABILITY-MATRIX.md`;
- **PokeJornadas:** functional/reference project for complex gameplay and Studio needs;
- **PokeAimar:** heavy legacy map/assets/migration stress-test;
- **BlackTek/RME:** behavior and editor maturity references.

Code/assets from those references are not silently copied into Fantasy Core. Import/provenance and licensing remain explicit. The advanced PokéTibia reference does **not** replace the vanilla TFS 1.4.2/10.98 compatibility target.

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
