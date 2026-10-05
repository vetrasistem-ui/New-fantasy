# FMAP v0 — Coordinate Semantics and Shared Core

This document freezes the coordinate rule used by the native Fantasy map path and the ownership of the reusable FMAP implementation.

## Shared ownership

FMAP is a platform contract, not a Studio-only implementation.

The neutral C++ implementation now lives in:

```text
Shared/Formats/FMAP/
├── schema-v0.json
├── FmapCore.hpp
├── FmapCore.cpp
├── Json.hpp
└── README.md
```

`Fantasy Studio` and `Fantasy Server` compile the same `Shared/Formats/FMAP/FmapCore.cpp`. The Studio keeps only a compatibility facade at `Studio/MapEngine/FmapCore.hpp` so editor code can continue using its existing namespace while the shared implementation remains owned by `fantasy::fmap`.

No second FMAP parser/serializer should be created in Server or Client.

## Coordinate spaces

FMAP v0 uses three nested coordinate spaces:

```text
global tile = region.origin + chunk.offset + tile.local
```

For X/Y:

```text
globalX = region.origin.x + chunk.x + tile.x
globalY = region.origin.y + chunk.y + tile.y
```

For Z/floor, `chunk.floor` is authoritative for tiles in that chunk.

## Meaning of fields

- `region.origin.x/y`: global tile origin of a region.
- `region.size.width/height`: allowed global tile extent of that region.
- `chunk.x/y`: chunk origin expressed as a **region-local tile offset**, not a chunk ordinal/index.
- `chunk.floor`: floor for all tiles stored in the chunk.
- `tile.x/y`: **chunk-local tile offset**.

Example:

```text
region origin = 100,100
chunk offset  = 4,0
tile local    = 0,2
----------------------
global tile   = 104,102
```

A tile at `chunk(0,0) / tile(3,2)` is therefore directly adjacent to a tile at `chunk(4,0) / tile(0,2)` only when the first chunk also contains the region-local position `3,2` and the second begins at `4,0`.

## Invariants

1. Chunk X/Y offsets are non-negative in FMAP v0 production maps.
2. Tile X/Y offsets are non-negative.
3. A resolved global tile must remain inside its region bounds.
4. Two tiles on the same region/floor may not resolve to the same region-local/global coordinate, even when they come from different chunks.
5. Chunks may be sparse in FMAP v0. Missing tiles are not implicitly created.
6. Editing operations that depend on adjacency, such as Fill, must resolve neighbours in region-local/global tile space and therefore may cross chunk boundaries.
7. Chunk storage is an implementation detail. Gameplay/map semantics must not change when an equivalent tile is moved between chunks while preserving its resolved global position.
8. Studio and Server must consume the same shared FMAP model/IO implementation.

## Why v0 uses offsets instead of chunk indexes

It keeps the first native format independent from a hard-coded chunk dimension while the runtime is still being designed. A future FMAP version may add an explicit fixed `chunkSize` for faster streaming/runtime lookup. If that happens, it must be a versioned schema change or migration rather than silently changing v0 meaning.

## Test fixture

`Game/Maps/World/multichunk-fixture.fmap.json` is the canonical cross-chunk fixture. It contains four chunks and a connected grass area that crosses from chunk offset `0,0` into `4,0`.
