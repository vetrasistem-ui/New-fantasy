# F04 Evidence — Fantasy Server World Runtime

Status: **IN_PROGRESS / FIRST RUNTIME SLICE IMPLEMENTED**

## Current implementation

`Server/Core/WorldRuntime.*` turns the Fantasy Server skeleton into the first native world runtime slice.

Implemented:

- direct load of the canonical `Game/Maps/World/world.fmap.json`;
- global tile index derived from `region.origin + chunk.offset + tile.local`;
- duplicate global tile detection;
- authoritative walkability for missing/blocked tiles;
- minimal entity model with generated runtime IDs;
- spawn validation;
- occupied-tile rejection;
- cardinal movement validation;
- cross-chunk movement without conversion to OTBM;
- deterministic Start → Ready → Tick → Stop lifecycle;
- deterministic tick scheduler with delayed tasks and cancellation;
- scheduler/entity cleanup on Stop.

The server smoke path now loads the real FMAP development world, spawns a development entity, moves it, ticks the runtime and shuts down cleanly.

## Automated tests

`Server/Core/WorldRuntimeTests.cpp` validates:

- 64 development tiles are indexed;
- development spawn resolves to a real walkable tile;
- duplicate occupancy is rejected;
- movement from the development spawn works;
- movement across the `x=99 → x=100` chunk boundary works;
- diagonal movement is rejected in this F04 slice;
- missing tiles are non-walkable;
- scheduled tasks fire on the intended tick;
- cancelled tasks do not execute;
- Stop clears entities and scheduled work.

## Temporary dependency to remove before F04 PASS

The first F04 slice deliberately reuses the already-tested FMAP C++ implementation currently located in `Studio/MapEngine/FmapCore.cpp` rather than creating a duplicate server parser.

This keeps one implementation while the runtime is proven, but the dependency direction is temporary. Before F04 can be declared PASS, the reusable FMAP model/IO/validation layer must be extracted to `Shared/` so both Studio and Server consume the same neutral Fantasy contract without Server depending on Studio source paths.

## F04 remaining gates

- CI PASS for the new runtime/scheduler tests;
- extract shared FMAP runtime contract into `Shared/`;
- prove Studio and Server both use the extracted shared FMAP implementation;
- add explicit blocked-tile fixture/test rather than relying only on missing-tile walkability;
- ensure all previous F00–F03 regressions remain green.

No OTBM, TFS runtime or protocol 10.98 is introduced into the native world path.
