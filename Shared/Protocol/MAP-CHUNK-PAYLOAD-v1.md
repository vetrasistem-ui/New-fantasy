# FMCP v1 — Fantasy MapChunk semantic payload

`Fantasy Protocol` message `MapChunk` owns chunk identity and enough region metadata for the client to resolve global tile positions:

```text
regionId
regionOriginX
regionOriginY
chunkX
chunkY
floor
revision
payload: bytes
```

The `payload` bytes use **FMCP v1**. This is deliberately separate from the outer protocol frame so the semantic map payload can evolve/version independently.

## Binary layout

All integers are little-endian.

```text
4 bytes   magic = ASCII "FMCP"
uint16    payloadVersion = 1
uint32    tileCount

repeat tileCount times:
    int32   tileX              # chunk-local offset
    int32   tileY              # chunk-local offset
    string  ground             # semantic asset key
    uint32  objectCount
    repeat objectCount:
        string objectKey       # semantic asset key
    uint32  tagCount
    repeat tagCount:
        string tag
```

`string` is encoded as:

```text
uint32 byteLength
byte[byteLength] UTF-8
```

## Semantics

- `regionOriginX/Y`, `chunkX/Y` and `floor` are **not duplicated inside FMCP**; they come from the outer `MapChunk` message.
- FMAP v0 keeps `chunkX/Y` as region-local offsets and `tileX/Y` as chunk-local offsets.
- The client resolves a tile globally using `global = regionOrigin + chunkOffset + tileLocal`.
- Grounds and objects remain semantic keys such as `terrain.grass.basic` and `nature.tree.oak.small`; the network does not replace them with legacy Tibia item IDs.
- Tags are preserved because they may affect rendering/gameplay metadata.
- F05 initial login sends the complete containing FMAP region on the player's floor. Later interest-management/streaming may send only nearby chunks without changing FMCP v1.

## Defensive limits in the reference C++ codec

- maximum outer Fantasy Protocol payload: 4 MiB;
- maximum tiles per FMCP payload: 65,536;
- maximum objects or tags in one tile list: 1,024;
- maximum encoded string length: 4,096 bytes;
- wrong magic, unsupported payload version, truncation, invalid semantic asset keys and trailing bytes are rejected.

## Canonical implementation

```text
Shared/Protocol/MapChunkPayload.hpp
Shared/Protocol/MapChunkPayload.cpp
Shared/Protocol/MapChunkPayloadTests.cpp
```

Server and Client must consume this shared codec rather than defining private MapChunk payload formats.
