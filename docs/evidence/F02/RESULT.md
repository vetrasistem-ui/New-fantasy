# F02 Evidence — Fantasy Map Core

Status: **PASS**

## Delivered

Implemented a native map core in `Studio/MapEngine/` with no RME, OTBM, TFS, DAT/SPR or 10.98 dependency in the runtime path.

Core model:

- `WorldInfo` / `World`;
- `Region`;
- `Chunk`;
- `Tile`;
- `Position` / `Size`;
- semantic ground/object keys;
- deterministic FMAP load/save;
- semantic validation;
- transactional mutations;
- commit / rollback;
- undo / redo.

## Native FMAP codec

`loadFmap()` reads `Game/Maps/World/world.fmap.json` directly into the native model.

`saveFmap()` writes the native model back to FMAP JSON. A dependency-free JSON parser/writer is included in `Studio/MapEngine/Json.hpp` so the Studio does not depend on an external JSON runtime for its core map contract.

## Semantic validation

The core rejects or reports:

- empty world/region identities;
- invalid region sizes;
- duplicate region IDs;
- duplicate chunks;
- duplicate tiles;
- invalid semantic asset keys;
- duplicate object keys on a tile;
- empty tags;
- development spawn outside the represented world/floor.

Numeric-only legacy-style asset IDs are intentionally rejected from native map mutations.

## Transaction model

All map mutations in `MapDocument` require a transaction. The current F02 mutation API covers:

- change ground;
- add object;
- remove object;
- commit;
- rollback;
- undo;
- redo.

This is the foundation required for GUI editing and future Codex/AI operations to call the same reversible domain operations.

## Automated evidence

Workflow run 52 on Windows:

- layout/contracts: PASS;
- Fantasy Project v2: PASS;
- FMAP v0 contract: PASS;
- Fantasy Protocol v1: PASS;
- project relocation: PASS;
- Fantasy Studio configure/build: PASS;
- Studio CTest suite, including `fantasy-map-core-tests`: PASS;
- Fantasy Server configure/build/smoke: PASS.

`fantasy-map-core-tests` proves:

1. the committed FMAP fixture loads into the native model;
2. semantic asset keys validate;
3. FMAP save → reopen is semantically identical;
4. ground/object mutations commit;
5. undo restores the exact previous state;
6. redo reapplies the state;
7. rollback cancels an open transaction;
8. invalid legacy-style keys are rejected.

## F02 gate result

All F02 roadmap gates are closed.

Next phase: **F03 — Fantasy Map Editor MVP**.
