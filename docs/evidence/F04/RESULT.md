# F04 Evidence — Fantasy Server World Runtime

Status: **PASS**

## Native world runtime

`Server/Core/WorldRuntime.*` turns Fantasy Server into the first authoritative native world runtime.

Implemented and validated:

- direct load of canonical `Game/Maps/World/world.fmap.json`;
- global tile index derived from `region.origin + chunk.offset + tile.local`;
- duplicate resolved tile detection;
- authoritative walkability for missing, `blocked` and `non-walkable` tiles;
- minimal entity model with generated runtime IDs;
- spawn validation and occupied-tile rejection;
- cardinal movement validation;
- movement across chunk boundaries without OTBM conversion;
- deterministic Start → Ready → Tick → Stop lifecycle;
- deterministic tick scheduler with delayed tasks and cancellation;
- scheduler/entity cleanup on Stop.

The server smoke path loads the real FMAP development world, starts the runtime, creates a development entity, moves it, advances the runtime and shuts down cleanly.

## Shared FMAP ownership complete

The temporary Server → Studio dependency has been removed.

The canonical reusable C++ FMAP implementation now lives in:

```text
Shared/Formats/FMAP/
├── schema-v0.json
├── FmapCore.hpp
├── FmapCore.cpp
├── Json.hpp
└── README.md
```

The neutral namespace is `fantasy::fmap`. Both Fantasy Studio and Fantasy Server compile the same `Shared/Formats/FMAP/FmapCore.cpp` implementation. `Studio/MapEngine/FmapCore.hpp` is only a thin compatibility facade and the old Studio-owned `FmapCore.cpp` / `Json.hpp` implementations were removed.

This ownership is frozen by ADR-013.

## Automated evidence

Workflow run **110** for commit `0f2da65eece569074b5268d4e880a13033813696` completed successfully on Windows after the shared FMAP extraction and explicit blocked-tile runtime test.

PASS gates in that run:

- project layout/contracts;
- Fantasy Project v2;
- FMAP v0;
- FMAP multi-chunk semantics;
- Fantasy Protocol v1 specification validation;
- project relocation;
- Fantasy Studio configure/build;
- complete Studio CTest suite;
- Windows Studio artifact upload;
- Fantasy Server configure/build;
- Fantasy Server smoke test and `fantasy-server-world-tests`.

`Server/Core/WorldRuntimeTests.cpp` proves:

- 64 development tiles are indexed;
- development spawn resolves to a walkable tile;
- duplicate occupancy is rejected;
- normal movement works;
- movement across the `x=99 → x=100` chunk boundary works;
- unsupported diagonal movement is rejected;
- missing tiles are non-walkable;
- a real tile tagged `blocked` is non-walkable and rejects entity spawn;
- scheduled tasks fire on the intended tick;
- cancelled tasks do not execute;
- Stop clears entities and scheduled work.

## F04 conclusion

All F04 technical gates are closed. The native runtime path is now:

```text
FMAP
 ↓
Shared FMAP Core
 ↓
Fantasy Server WorldRuntime
 ↓
Tiles / Entities / Movement / Walkability / Scheduler
```

No OTBM, TFS runtime or protocol 10.98 is present in this path.

The project may advance to **F05 — Fantasy Protocol v1 + First Native Play**.
