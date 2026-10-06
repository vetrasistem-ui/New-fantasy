# Fantasy Studio Visual Foundation V5 — Windows Result

Status: **TECHNICAL PASS / GUI REGRESSION PASS / SIX REAL CAPTURES RECORDED / OWNER VISUAL ACCEPTANCE PENDING**

Date: `2026-10-06` · Branch: `feature/studio-visual-foundation`

Initial V5 SHA: `b534ab9a9faf61cda9afc0fcd7189a198fc64d12`.
This SHA identifies the synchronization baseline, not the final V5 executable.
Final validated `Studio/UI/StudioAppV5.cpp` Git blob:
`f46cf1d281887432ec1fe11b2af336b7ca134001`.
Final complete build/test results: **PASS, 13/13 CTests**.
The delivery commit is the revision containing this report; this report does
not embed its own circular final commit SHA.

## Owner decisions and reference

The owner rejected V4's appearance on `2026-10-06`: **V4 TECHNICAL PASS /
VISUAL FAIL**. Its [original report](RESULT-V4.md) and six V4 screenshots are
preserved as historical evidence. All six tracked V4 PNGs were checked against
Git HEAD and remain unchanged.

The owner also rejected the first V5 composition on `2026-10-06` as still
too far from the approved images. Its six real native screenshots are
preserved in [V5-FIRST-PASS](V5-FIRST-PASS/README.md). These captures document
the rejected iteration, including its light native frame, overly wide
initial window, stretched cards, tall hero, full library sidebar and extra
asset selection panel. Neither the earlier agent comparison with V4 nor
successful functional checks constitute the owner's visual acceptance.

The refined V5 targets strong, immediate correspondence with the
[approved styleboard](../../design/STUDIO-VISUAL-V4/approved-styleboard.png),
particularly the Home, Map and Items & Assets compositions. Reference images
remain design documentation, never application textures. New captures and
owner review must assess the refined composition; visual PASS is not claimed.

Current GUI source: `Studio/UI/StudioAppV5.cpp`. The renderer remains
**SDL3 + SDL_Renderer3 + Dear ImGui**. MapDocument, EditorOperations, FMAP,
Server, Protocol and Client core remain unchanged. **F05.5, F06 and
OTBM/DAT/SPR/OTB integration are NOT STARTED.** No real sprites or new core
features are introduced by the visual rebuild.

## Refined presentation and packaging

- Original illustrated castle/mountain hero and faceted emblem provide the
  Studio's branding. They are presentation art, not game content, imported
  sprites or FMAP previews. The [provenance record](../../design/STUDIO-VISUAL-V5/ART-PROVENANCE.md)
  documents generation, dimensions, hashes and scope.
- Cinzel supplies display branding under the preserved SIL Open Font License
  1.1. Neutral controls use system Segoe UI/Arial/DejaVu Sans. No proprietary
  system font is redistributed.
- CMake copies `Studio/UI/Assets/` to `StudioVisual/` beside the executable.
  The workflow includes that resource directory in the Studio artifact.
  Hero, emblem, Cinzel and its license were found in the local Release
  resource directory. Resources resolve relative to the executable; project
  contracts do not persist absolute machine paths.
- The preferred 1440×900 client area now fits the usable display with one
  uniform scale factor, preserving its 1.6 aspect ratio. This replaces the
  first pass's independently bounded width and height. All three main
  captures confirm a 1049×656 client area; the smaller set confirms 960×656.
- The topbar is 46 px. Home/Map navigation is 188 px at desktop width and
  164 px below 1150 px. Items & Assets instead uses a 56 px icon rail,
  matching the reference library's compact navigation.
- Home uses a 140 px hero in the practical window size and 138 px project
  cards. Card titles and two-line descriptions are centered. A taller 182 px
  hero remains available for the larger Home workspace. Duplicate title
  chrome and CREATE/RUNTIME sidebar headings are removed.
- The native Windows titlebar adopts navy and light text through Windows 11
  DWM appearance attributes, preserving native close and resize behavior.
  Unsupported older systems retain their platform frame. The refined dark
  frame is confirmed in both sizes of all three new workspace screenshots.
- Filled blue/silver navigation icons replace prominent wireframe symbols.
  The Map toolbar keeps functional tools and adds honest disabled
  Zone/Path/Event/Config controls, without introducing their domain features.
- Map, previews, Minimap and Assets use coherent semantic terrain colors.
  Compact square asset cells and friendly labels retain the actual FMAP keys.
  The extra Assets Selection panel and SDK footer are removed; selection
  remains visible through the selected cell and its real key.

## Automated technical gate

The final complete Release build/test run is **PASS**, after the final
one-line hero seam correction. This run validated the source blob identified
above; earlier builds do not substitute for this final technical gate.

Executed command:

```powershell
./scripts/build-native.ps1 -Configuration Release
```

| Final check | Current status |
| --- | --- |
| Complete native Release build | PASS |
| Studio CTests | PASS — 5/5 |
| Server CTests | PASS — 6/6 |
| Client CTests | PASS — 2/2 |
| Shared contracts and layout/project/FMAP/multi-chunk/protocol validators | PASS |
| Project relocation | PASS |
| Final source blob and runtime identification | PASS — blob above; `direct3d11`, Segoe UI and Cinzel |

All 13 CTests passed. No compiler or CMake warnings/errors occurred. SDL's
optional PkgConfig/LibUSB discovery was unavailable on this Windows setup;
neither is required for the observed Studio build. The refined launcher
reported the SDL_Renderer3 `direct3d11` backend, Segoe UI interface font and
Cinzel branding font. No remote CI success is claimed.

## Home and navigation

**Interactive regression checks PASS; refined appearance review remains pending.**

- Home opened with the new emblem/Cinzel identity and original landscape.
- Both final Home captures show the canonical four-object world, the seam-fixed
  hero, three complete cards and both lower panels without critical clipping.
- Three equal Novo/Abrir/Importar cards retain honest tooltips for reserved
  project-dialog/import functionality.
- Recent Projects uses real workspace projects. Project information on the
  right shows an actual FMAP preview and real metadata/counts.
- `Abrir Mapa` opened the Map workspace.
- All ten sidebar destinations opened without crash:
  Home, Map, Items & Assets, Monsters, NPCs, Spells, Quests, Systems, Server
  and Client. Future modules remain reserved shells. The refined library
  intentionally uses compact icon navigation; Home/Map keep the full sidebar.

The six new captures confirm the revised Home proportions, compact Assets
rail and dark frame. Direct comparison finds Home closer in composition to
the approved image than the rejected first V5 pass. This independent audit
does not confer owner acceptance or inherit approval from an earlier revision.

## Map operations and semantic Save/Reopen

**PASS through the real GUI and shared editor operations.**

The complete operation/Save/Reopen regression was executed during the V5
rebuild before the final artwork and proportion refinements. The refined
layout was then checked for selection coordinates and access to every editing
control at both practical sizes. These final composition changes do not
replace or modify the shared operation callbacks.

| Operation | Observed result |
| --- | --- |
| Four chunks | All 64 tiles rendered, with the actual world also visible in Minimap |
| Select | Selected global tile `97,97,7`; Inspector shows matching coordinates |
| Paint | Ground changed to `terrain.grass.basic` |
| Undo / Redo | Reverted/reapplied the ground edit |
| Fill between chunks | Seed `96,96,7`; 17 connected tiles became `terrain.sand.basic` across two chunks |
| Add Object | Added `nature.tree.oak.small` at `97,97,7`; marker appeared |
| Remove Object | Removed the object; marker disappeared |
| Erase | Removed the re-added object |
| Save | Saved grass/oak at `97,97,7` and the 17 filled tiles |
| Close / Reopen | A new Studio process loaded the same saved edits |
| Semantic persistence | Entire saved JSON matched the expected edit set, with no unrelated semantic change |
| Focused-input keyboard guard | Global keyboard Undo did not alter the map while a text input was focused |

Saved-file SHA-256 before and after reopening:
`8432EB4745C4B6CFAE077EBF94B2861A8156377966EF09579AB7E540EB972934`.
The test world stays at four chunks/64 tiles. The final added oak produces
five objects in that temporary saved state.

The canvas initially fits the actual map, keeping rendering and selection
on the same coordinates. Inspector, Minimap and the full-width Console stay
outside the editing canvas. The generic fit accounts for map bounds when the
spawn is off-center. Global Undo/Redo is guarded during text input.
These changes remain in the presentation layer.

**Canonical FMAP restoration PASS.** After the regression run, the canonical
`Game/Maps/World/world.fmap.json` was restored. Its Git blob is
`1442d649ff73ba39881f876a4bde33394df2784c`, confirmed after the final build;
`Game/` has no Git diff. The test edits remain evidence rather than delivered
content. The Map screenshots preserve the earlier tested in-memory edit
state; both final Home captures show the restored four-object canonical world.

## Items & Assets

**Functional regression checks PASS; refined appearance review remains pending.**

- Items, Sprites, Textures and Sounds tabs opened; unconnected tabs display
  honest empty states.
- Searching `tree` returned Carvalho and Pinheiro through their semantic
  keys. Selecting Terrenos with that query showed no matches. Clearing the
  query restored the six terrain references.
- Searching `CARVALHO` also returned Carvalho, proving case-insensitive
  matching of the friendly display label as well as the semantic key.
- Searching uppercase accented `ÁGUA` returned Água in the newest GUI run.
- All/Terrenos/Objetos categories worked; selecting Carvalho produced the
  blue/cyan selected cell and its real key.
- The catalog contains six ground and four object references from the actual
  FMAP. No invented equipment or legacy catalog fills the screen.
- The refined composition uses the 56 px navigation rail, a 156 px category
  tree and the main square-cell grid. The separate Selection panel is removed
  at all widths. The selected key remains below the library.

The newest run retested all four tabs and the selected Carvalho key in the
footer. Both refined captures expose all ten real catalog entries, including
the complete six-terrain row, without critical clipping. The 56 px rail,
tree/grid hierarchy and absence of the Selection panel are confirmed.

## Real screenshots and practical sizing

The rejected first-pass screenshots remain unmodified in
[V5-FIRST-PASS](V5-FIRST-PASS/README.md). Their main client area was
1334×656 with a 1336×688 native capture; smaller captures used a 960×656 client
area with a 962×688 native capture. They are historical evidence, not refined
visual acceptance evidence.

All six refined screenshots below are recorded and independently inspected.
Their native dimensions were also verified from the PNG headers.

| Evidence | Client area | Native capture | Status |
| --- | --- | --- | --- |
| [Home](home-v5.png) | 1049×656 | 1051×688 | RECORDED / INSPECTED |
| [Map](map-v5.png) | 1049×656 | 1051×688 | RECORDED / INSPECTED |
| [Items & Assets](items-assets-v5.png) | 1049×656 | 1051×688 | RECORDED / INSPECTED |
| [Home — smaller](home-v5-small.png) | 960×656 | 962×688 | RECORDED / INSPECTED |
| [Map — smaller](map-v5-small.png) | 960×656 | 962×688 | RECORDED / INSPECTED |
| [Items & Assets — smaller](items-assets-v5-small.png) | 960×656 | 962×688 | RECORDED / INSPECTED |

Recorded evidence came from the running Windows application. Native captures
were encoded as PNG without cropping, retouching, compositing or replacing
application pixels. The rejected first-pass files remain separately archived;
the current six files document the refined composition.

The actual display is 1366×768. Exact 1440×900 inspection remains unobserved;
no exact-resolution PASS is inferred from preferred code dimensions.
Both Map sizes show the complete 8×8 world, selected tile `97,97,7`, editing
toolbar, Inspector, Minimap and full-width Console without critical clipping.
The Map captures show the temporary five-object edit state. The Assets
captures show all ten semantic keys and Carvalho selected. Both Home captures
show four chunks, 64 tiles and four objects from the restored canonical world.
The hero, card text, lower panels, relative map path and Abrir Mapa button
are complete at both practical sizes. No critical clipping is evident in
the six inspected screenshots.

## Independent comparison and remaining differences

The first V5 pass introduced richer branding, navy surfaces, an icon-first
Map toolbar and a square-cell library compared with V4, but the owner rejected
its correspondence with the approved model. Technical success and those
improvements were insufficient.

The refinement addresses the concrete composition differences identified
during that comparison:

- **Home:** uniform window fitting avoids the stretched first-pass shell.
  The shorter hero and taller cards rebalance the upper area; centered
  descriptions follow the approved card arrangement more closely. Direct
  comparison with the supplied Home target confirms a closer hero/cards/lower
  composition. In the smaller capture, cards are approximately 249×138
  (ratio 1.80) and the hero is approximately 763×140 (ratio 5.45), close to
  the target's approximately 165×92 cards (1.79) and 514×94 hero (5.47).
  These measurements describe composition rather than pixel identity.
- **Shell:** the native navy titlebar and filled blue/silver icons address
  the light frame and wireframe visual language.
- **Map:** a cohesive Inspector/Minimap column, full-width Console and
  compact icon toolbar retain the reference workspace hierarchy.
- **Assets:** the compact icon rail and removal of the extra Selection panel
  give the tree and main grid the reference's priority.

The six inspected captures confirm the dark frame, solid icons, revised Home,
compact library rail and panel hierarchy corrections. They improve the
shell's correspondence with the approved layout compared with the rejected
first pass. The main grid has ample empty space because only ten actual
semantic keys exist; this is not filled with invented catalog entries.
The Home hero has no visible vertical seam in either final capture. No
refined image is declared identical or visually approved.

Remaining visible content/design differences:

- The hero and emblem are original presentation artwork, so their illustration
  and lettering details differ from the approved artwork.
- Recent Projects has one real project rather than the reference's four
  populated rows. No fake project is added to imitate that density.
- Map/Assets show semantic terrain blocks and object markers rather than the
  reference's rich sprite content. Sprite and legacy integration are excluded.
- The library contains the actual FMAP categories and ten real keys, rather
  than the reference's dense equipment inventory.
- Home's real FMAP preview is a small square world overview rather than the
  target's broad sprite-rich scene. Together with the single recent project,
  this leaves the lower Home area less densely populated than the reference.
- The main-window cards remain proportionally wider than those in the target;
  the smaller-window cards are closer. The wordmark/emblem scale and lettering
  remain recognizably different from the exact supplied branding.
- No critical clipping is evident at either inspected size. Content richness
  and library density remain visibly different from the sprite-rich reference.

## Close, cleanup and repository closeout

Normal closure of the refined Studio returned exit code 0 and passed the
launcher's clean-close gate. A process inventory before the final review
launch found zero Fantasy processes. Home was then reopened deliberately
for final screenshots and owner review. Studio is intentionally left open on
Home for that review; no unexpected Fantasy runtime processes were observed.
The clean-close evidence is separate from this deliberate review process.
The final inventory confirms that only the deliberately opened Studio review
process remains; no Server, Client or Client GUI process is running.

Build output, local logs/scripts and caches stay under ignored `build/`.
Only intended visual source, original presentation art, Cinzel with its license,
documentation and real screenshots belong in the delivery. Do not commit the
saved test map, executables, caches, local databases or legacy game assets.

Final `git diff --check` and `git diff --cached --check` passed. Staged path
review contains only intended presentation source/resources, packaging,
documentation and evidence. There is no Game/core diff, build/cache/binary
output, local database or legacy game asset in the staged delivery. The
licensed Cinzel font is a documented presentation resource. Final build/tests, canonical map
restoration, all six screenshot checks and normal cleanup passed. An
intentional Studio review window may remain open; it is not an orphan.
The delivery commit contains this report; its final SHA, push outcome and
post-commit working-tree status are recorded in the delivery response.

## Owner decision

**Refined V5 visual acceptance remains PENDING.** The first V5 rejection is
preserved explicitly. Build/tests, functional regression, canonical map
restoration, normal cleanup and all six refined captures are recorded.
The technical delivery remains separate from visual acceptance. Only the owner
may accept the appearance and freeze the visual foundation. Neither these
checks nor that acceptance starts F05.5, F06 or legacy integration.
