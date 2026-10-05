# F00 — Known Limitations

F00 freezes only the first native contracts. It does not claim that the world runtime or network runtime is complete.

## FMAP v0

- Experimental source format; breaking changes are allowed before FMAP v1.
- Chunk coordinates are logical offsets inside a region in v0.
- Tiles currently contain only semantic ground/object keys and tags.
- Houses, spawns, teleports, zones, elevation rules and object attributes are intentionally deferred.
- FMAPC binary/runtime compilation is not implemented yet.
- The authoritative runtime loader is not implemented yet; current validation is contract-level only.

## Fantasy Protocol v1 draft

- TCP is the initial transport.
- Binary serialization/framing is specified semantically but not implemented yet.
- `LoginDev` is development-only and must not become production authentication.
- `EntityMove` remains bidirectional only for the early prototype; the server remains authoritative and will validate all movement.
- Compression, TLS/session security, reconnect, capabilities negotiation and rate limiting are deferred.
- Message IDs are frozen only for the F00 draft set; additional IDs may be added before protocol v1 stable.

## Fantasy Server

- Current executable is a C++20 skeleton with smoke-test only.
- No networking, FMAP loading, entity runtime, scheduler or persistence exists yet.
- TFS 1.4.2 remains a behavior reference/fallback only and is not linked into the native build.

## Legacy references

RME/OTBM, TFS 1.4.2 and the 10.98-compatible client may be used to compare expected MMORPG behavior or import legacy data. They are not dependencies of the native build path.
