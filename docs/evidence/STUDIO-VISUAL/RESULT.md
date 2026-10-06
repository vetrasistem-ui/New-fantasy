# Fantasy Studio Visual Foundation V4 — Windows Result

Status: **TECHNICAL GATES PASS / OWNER VISUAL ACCEPTANCE PENDING**

Date: `2026-10-05` · Branch: `feature/studio-visual-foundation`

Initial SHA: `c91b926d08ef5fd61dff7c56b7395dff578f501f`.
The implementation and evidence are in the revision containing this report;
the initial SHA is the synchronization baseline, not the V4 executable revision.

Validated V4 source Git blob: `3a8792a5ef8e9de513fc3e4ea73332b8394543c2`.

## Reference and scope

The supplied V4 package replaces the rejected V3 appearance as the design
direction. Package SHA-256:
`194B2E3D39039F0CBCAECA407B215AB6AA3C393278A975192EE0E3097CF75E84`.
The [styleboard](../../design/STUDIO-VISUAL-V4/approved-styleboard.png) and
[tokens](../../design/STUDIO-VISUAL-V4/DESIGN_TOKENS.json) are documentation
references only. No screenshot or crop is used as an application texture.

Current GUI source: `Studio/UI/StudioAppV4.cpp`. The renderer remains
SDL3 + SDL_Renderer3 + Dear ImGui. MapDocument, EditorOperations, FMAP, Server,
Protocol and Client core were not changed. **F05.5 and F06 were not started.**
No OTBM/DAT/SPR/OTB integration was introduced.

## Environment and automated checks

| Check | Observed result |
| --- | --- |
| OS | Windows 11 Home, `10.0.26200` |
| GPU | Intel HD Graphics 5500 |
| Driver | `20.19.15.4703` |
| Configuration | Release |
| Runtime renderer | `direct3d11`, reported by the V4 executable |
| Runtime UI font | `segoeui.ttf`, loaded from the system |
| Display | 1366×768 |
| Full native build | PASS |
| Studio CTests | 5/5 PASS |
| Server CTests | 6/6 PASS |
| Client CTests | 2/2 PASS |
| Layout/project/FMAP/multi-chunk/protocol validators | PASS |
| Project relocation | PASS |

Executed `./scripts/build-native.ps1 -Configuration Release` after the final
V4 source changes. All **13 CTests passed**. No compiler warning or error
remained. Optional SDL PkgConfig/LibUSB discovery messages do not represent
required Windows dependency failures.

During implementation, an ImGui overload ambiguity was corrected and the
relocation check was rerun with the V4 source replacement in Git's tracked file
list. The successful full build above includes both corrections; neither
validator nor test was skipped or weakened.

The existing two-process native play also passed during this task: Ready,
four chunks/64 tiles, initial position `100,100,7`, authoritative move to
`101,100,7`, clean Disconnect/Stop. Server and Client source were unchanged.
This report claims local Windows checks; historical V3 remote CI is not V4 CI
evidence.

## Home and navigation

**Interactive inspection completed; appearance approval remains with the owner.**

- Neutral proportional UI typography, original vector identity and abstract
  navy/cyan hero replace the terminal typography and previous hero treatment.
- Topbar commands and ten sidebar destinations keep consistent icon sizes,
  spacing and selected states. Undo/Redo reflect real document history.
- Novo/Abrir/Importar cards share dimensions; project counts and previews come
  from the current FMAP. No fake project, date, sprite or progress value fills
  the screen.
- `Abrir Map Workspace` opens Map successfully.
- All ten destinations opened without crash: Home, Map, Items & Assets,
  Monsters, NPCs, Spells, Quests, Systems, Server and Client.

GUI project dialogs/import and future runtime/settings functionality remain
reserved, with their existing CLI/launcher entry points explained. These shells
are not claimed as newly implemented product features.

## Map functional regression

**PASS through the real GUI and shared editor operations.**

| Operation | Observed result |
| --- | --- |
| Four chunks | All 64 tiles rendered; actual world also shown in Minimap |
| Select | Global `97,97,7` shown as local tile `1,1` in chunk `0,0` |
| Paint | Changed selected ground to `terrain.grass.basic` |
| Undo / Redo | Topbar and keyboard shortcuts reverted/reapplied the edit |
| Fill across chunks | Seed `96,96,7`; 17 connected tiles changed to `terrain.sand.basic` across two chunks |
| Add Object | `nature.tree.oak.small` appeared on the selected tile |
| Remove Object | Object marker disappeared |
| Erase | Reported removal of one object after re-adding it |
| Save | FMAP saved with grass/oak at `97,97,7` and the 17 filled tiles |
| Close / Reopen | New GUI process loaded those exact edits and object marker |
| Semantic persistence | Entire saved JSON compared with the expected edit set; no unrelated difference |

Saved-file SHA-256 before and after reopening:
`8432EB4745C4B6CFAE077EBF94B2861A8156377966EF09579AB7E540EB972934`.
The map stayed at four chunks/64 tiles; the final test edit added one object
(five objects in that temporary saved state).

After normal close, the canonical FMAP was restored. Its Git blob is
`1442d649ff73ba39881f876a4bde33394df2784c`, and the map has no Git diff.
The Map screenshots intentionally retain the successful saved/reopened test
state; that test edit is not committed as content.

## Items & Assets

**Functional inspection PASS; appearance approval pending.**

- Items, Sprites, Textures and Sounds tabs opened. Unconnected tabs display an
  honest empty state.
- Search for `tree` showed the two matching semantic object keys; Grounds then
  showed no matches. Clearing the search restored the catalog.
- All, Grounds and Objects categories worked. Full keys remain readable; the
  selected oak card has a cyan border and selection detail.
- The narrow library tree leaves the main grid dominant. Colors are explicitly
  semantic swatches rather than final sprite art. The future legacy source is
  `NOT CONNECTED`.

## Practical sizing, screenshots and cleanup

Full-window screenshots were captured from the running Windows application
using Computer Use. Native JPEG captures were encoded as PNG without cropping,
retouching, compositing or replacing UI pixels.

| Evidence | Client area | Capture including native frame |
| --- | --- | --- |
| [Home](home-v4.png) | 1334×656 | 1336×688 |
| [Map](map-v4.png) | 1334×656 | 1336×688 |
| [Items & Assets](items-assets-v4.png) | 1334×656 | 1336×688 |
| [Home — smaller](home-v4-small.png) | 960×656 | 962×688 |
| [Map — smaller](map-v4-small.png) | 960×656 | 962×688 |
| [Items & Assets — smaller](items-assets-v4-small.png) | 960×656 | 962×688 |

At the smaller size, Home remains readable and Map's editing controls, Minimap
and Console remain reachable. The Assets All grid scrolls to the last object;
its Objects category fits all four cards. The smaller Assets screenshot shows
that category deliberately. Small Home and both Map captures use the saved test
state; main Home uses the canonical project.

**Exact 1440×900 inspection is pending:** this display cannot show that client
area completely. No exact-resolution visual PASS is inferred from the preferred
window size or code geometry.

Normal Alt+F4 closure returned exit code 0 and the launcher's clean-close PASS.
Process inspection then found no Fantasy Studio, Server, Client or Client GUI
process. The final review launch intentionally leaves one Studio on Home;
it is the owner's review window, not an orphan runtime process.

Build output, local validation scripts and caches stay under ignored `build/`.
Screenshots and design documentation are the only image artifacts delivered.
`git diff --check` and the staged whitespace check passed. The staged path
review excluded build/cache output, executables, font binaries, legacy assets,
map edits and changes to Server/Client/Shared core.

## Owner decision

**V4 visual acceptance is PENDING.** Home/Map/Items & Assets are ready for the
owner to inspect using these real captures and the Studio left open on Home.
Technical success does not mark the overall visual foundation PASS or authorize
F05.5, F06 or legacy asset integration.
