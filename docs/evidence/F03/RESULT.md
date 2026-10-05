# F03 Evidence — Fantasy Map Editor MVP

Status: **IN_PROGRESS / STRONG PARTIAL PASS**

## Visual foundation frozen

ADR-012 selects the native visual stack:

- SDL3 `release-3.4.18` / commit `829a65d769d935c4852f8159e964312c0957260a`;
- SDL_GPU for viewport rendering;
- Dear ImGui `v1.92.9b` / commit `f1cc2ae15e53a861a874c3034aae6798fde194ab`;
- official Dear ImGui SDL3 + SDL_GPU backends.

This stack is deliberately isolated from the map domain. The GUI edits the existing `MapDocument`; it does not own a second map representation.

## Native editor shell implemented

`Studio/UI/EditorApp.cpp` now provides the first real Fantasy map editor application:

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
- Undo / Redo;
- FMAP Save / Ctrl+S;
- World panel;
- basic region minimap with development-spawn marker;
- clean GPU/SDL/ImGui shutdown path.

The viewport currently renders semantic grounds as deterministic placeholder colors. Real Fantasy sprites/assets belong to the later asset pipeline and are not required to prove the native editor architecture.

## Shared editor operations

`Studio/MapEngine/EditorOperations.*` adds reusable transactional commands above `MapDocument`:

- paint ground;
- add object;
- remove object;
- connected 4-neighbour ground fill;
- erase all objects from a tile.

Fill and eraser operations are covered by headless undo/redo tests so they can be called by GUI, CLI or future Codex automation without reimplementing map behavior.

### Cross-chunk behaviour

FMAP v0 multi-chunk coordinate semantics are now frozen in `Shared/Formats/FMAP/README.md`:

```text
global tile = region.origin + chunk.offset + tile.local
```

`chunk.x/y` are region-local tile offsets, while `tile.x/y` are chunk-local offsets. Missing tiles remain sparse; two chunks may not resolve to the same tile coordinate on the same floor.

`Game/Maps/World/multichunk-fixture.fmap.json` is the canonical F03 multi-chunk fixture. It contains four chunks and a connected ground area crossing a chunk boundary.

`fillConnectedGround` now resolves adjacency in region-local tile space, so a fill operation crosses chunk boundaries correctly while remaining one transaction. Undo and Redo cover the entire cross-chunk edit.

## Automated evidence

Workflow run **77** (`70cf98b39f3062cec4e7ba6a03703c86154cd899`) passed on Windows:

- layout/contracts: PASS;
- Fantasy Project v2: PASS;
- FMAP v0: PASS;
- FMAP multi-chunk coordinate validator: PASS;
- Fantasy Protocol v1: PASS;
- project relocation: PASS;
- SDL3 + Dear ImGui dependency configure: PASS;
- Fantasy Studio build, including `fantasy-studio-gui.exe`: PASS;
- Studio CTest suite, including cross-chunk fill + undo/redo: PASS;
- Windows Studio binaries artifact upload: PASS;
- Fantasy Server build/smoke regression: PASS.

## Gates already demonstrated

- visual toolkit/rendering choice: **PASS**;
- native GUI target compiles on Windows: **PASS**;
- open current project/main FMAP path is wired: **PASS**;
- 2D viewport implementation exists: **PASS (build-level)**;
- floor controls: **PASS (build-level)**;
- pan/zoom: **PASS (build-level)**;
- tile selection: **PASS (build-level)**;
- ground/object brush uses native transactions: **PASS (build-level + headless domain tests)**;
- fill/erase domain operations reversible: **PASS (headless)**;
- cross-chunk coordinate semantics: **PASS**;
- cross-chunk connected fill: **PASS (headless)**;
- multi-chunk fixture validation: **PASS**;
- basic minimap implementation: **PASS (build-level)**;
- Save/Reopen semantic roundtrip: **PASS (headless)**.

## Still required before F03 PASS

- perform a real interactive Windows launch of `fantasy-studio-gui.exe` and capture visual evidence;
- wire the already-tested fill/eraser commands into visible GUI controls;
- validate interactive Save → close → reopen in the GUI;
- validate viewport behaviour interactively with the multi-chunk fixture or a larger generated map.

No RME, OTBM or 10.98 code is present in the F03 native path.
