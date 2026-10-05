# F03 Evidence — Fantasy Map Editor MVP

Status: **IN_PROGRESS / TECHNICAL GATES PASS — INTERACTIVE WINDOWS VALIDATION PENDING**

## Visual foundation frozen

ADR-012 selects the native visual stack:

- SDL3 `release-3.4.18` / commit `829a65d769d935c4852f8159e964312c0957260a`;
- SDL_GPU for viewport rendering;
- Dear ImGui `v1.92.9b` / commit `f1cc2ae15e53a861a874c3034aae6798fde194ab`;
- official Dear ImGui SDL3 + SDL_GPU backends.

This stack is deliberately isolated from the map domain. The GUI edits the existing `MapDocument`; it does not own a second map representation.

## Native editor shell implemented

`Studio/UI/EditorApp.cpp` provides the first real Fantasy map editor application:

- opens the current Fantasy project through `ProjectManager`;
- loads the canonical FMAP directly;
- SDL3 native window;
- SDL_GPU device/render pass;
- Dear ImGui editor panels;
- 2D tile viewport;
- floor up/down;
- middle-mouse pan;
- mouse-wheel zoom;
- tile selection;
- semantic ground brush;
- semantic object add/remove;
- connected ground Fill;
- Erase Tile Objects;
- Undo / Redo;
- FMAP Save / Ctrl+S;
- World panel;
- basic region minimap with development-spawn marker;
- clean GPU/SDL/ImGui shutdown path.

The visible Paint, Fill, Add Object, Remove Object and Erase controls now call `EditorOperations`; the GUI no longer reimplements those domain mutations. This is the same operation layer intended for future CLI/Codex automation.

The viewport currently renders semantic grounds as deterministic placeholder colors. Real Fantasy sprites/assets belong to the later asset pipeline and are not required to prove the native editor architecture.

## Shared editor operations

`Studio/MapEngine/EditorOperations.*` provides reusable transactional commands above `MapDocument`:

- paint ground;
- add object;
- remove object;
- connected 4-neighbour ground fill;
- erase all objects from a tile.

Fill and eraser operations are covered by headless undo/redo tests so GUI, CLI and future Codex automation can share map behavior.

### Cross-chunk behaviour

FMAP v0 multi-chunk coordinate semantics are frozen in `Shared/Formats/FMAP/README.md`:

```text
global tile = region.origin + chunk.offset + tile.local
```

`chunk.x/y` are region-local tile offsets, while `tile.x/y` are chunk-local offsets. Missing tiles remain sparse; two chunks may not resolve to the same tile coordinate on the same floor.

`Game/Maps/World/multichunk-fixture.fmap.json` is the canonical F03 multi-chunk fixture. It contains four chunks and a connected ground area crossing a chunk boundary.

`fillConnectedGround` resolves adjacency in region-local tile space, so a fill operation crosses chunk boundaries correctly while remaining one transaction. Undo and Redo cover the entire cross-chunk edit.

## Main development world

`Game/Maps/World/world.fmap.json` is now an 8 × 8 development area with 64 tiles split across four chunks. It contains multiple semantic grounds and objects so the default Studio project exercises multi-chunk rendering rather than the original two-tile fixture.

## Automated evidence

Workflow run **83** (`48820b38a7b1f9dbd52c02637743f4133d2dbff5`) passed completely on Windows after the GUI was moved onto the shared editor operation layer:

- layout/contracts: PASS;
- Fantasy Project v2: PASS;
- FMAP v0: PASS;
- FMAP multi-chunk coordinate validator: PASS;
- Fantasy Protocol v1: PASS;
- project relocation: PASS;
- SDL3 + Dear ImGui dependency configure: PASS;
- Fantasy Studio build, including `fantasy-studio-gui.exe`: PASS;
- Studio CTest suite, including FMAP roundtrip and cross-chunk Fill + Undo/Redo: PASS;
- Windows Studio binaries artifact upload: PASS;
- Fantasy Server build/smoke regression: PASS.

## Gates demonstrated

- visual toolkit/rendering choice: **PASS**;
- native GUI target compiles on Windows: **PASS**;
- open current project/main FMAP path is wired: **PASS**;
- 2D viewport implementation exists: **PASS (build-level)**;
- floor controls: **PASS (build-level)**;
- pan/zoom: **PASS (build-level)**;
- tile selection: **PASS (build-level)**;
- ground/object brush uses shared native operations: **PASS (build-level + headless tests)**;
- Fill/Erase visible GUI controls use shared operations: **PASS (build-level + headless domain tests)**;
- fill/erase operations reversible: **PASS (headless)**;
- cross-chunk coordinate semantics: **PASS**;
- cross-chunk connected Fill: **PASS (headless)**;
- multi-chunk fixture validation: **PASS**;
- 64-tile / four-chunk main development world: **PASS (validation + build)**;
- basic minimap implementation: **PASS (build-level)**;
- Save/Reopen semantic roundtrip: **PASS (headless)**.

## Only remaining gate before F03 PASS

A real interactive Windows session must still prove the user-facing path:

1. launch `fantasy-studio-gui.exe`;
2. inspect the 8 × 8 four-chunk world visually;
3. select tiles and exercise Paint / Fill / Add / Remove / Erase / Undo / Redo;
4. Save FMAP;
5. close the application;
6. reopen it;
7. confirm the saved visual state is preserved;
8. capture visual evidence.

Until that interactive run is performed, F03 remains IN_PROGRESS even though the automated technical gates pass.

No RME, OTBM or 10.98 code is present in the F03 native path.
