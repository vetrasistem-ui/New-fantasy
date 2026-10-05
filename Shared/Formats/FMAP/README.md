# FMAP v0 — Coordinate Semantics

This document freezes the coordinate rule used by the native Fantasy map path during F03.

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

## Why v0 uses offsets instead of chunk indexes

It keeps the first native format independent from a hard-coded chunk dimension while the runtime is still being designed. A future FMAP version may add an explicit fixed `chunkSize` for faster streaming/runtime lookup. If that happens, it must be a versioned schema change or migration rather than silently changing v0 meaning.

## Test fixture

`Game/Maps/World/multichunk-fixture.fmap.json` is the canonical F03 fixture for verifying cross-chunk behaviour. It contains four chunks and a connected grass area that crosses from chunk offset `0,0` into `4,0`.
