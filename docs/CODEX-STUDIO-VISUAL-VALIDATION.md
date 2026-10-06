# Codex — Fantasy Studio Visual Foundation V5 Windows Gate

Status: **V5 TECHNICAL VALIDATION PASS / OWNER VISUAL ACCEPTANCE PENDING**

V5 baseline SHA: `b534ab9a9faf61cda9afc0fcd7189a198fc64d12`.

The owner rejected V4 on `2026-10-06`: **V4 TECHNICAL PASS / VISUAL FAIL**.
Preserved evidence is in `docs/evidence/STUDIO-VISUAL/RESULT-V4.md` and the
unchanged V4 screenshots. Do not treat those checks as V5 validation.
The owner also rejected the first V5 composition. Its six real captures remain
in `docs/evidence/STUDIO-VISUAL/V5-FIRST-PASS/`; the current result evaluates
the subsequent proportions, artwork and compact library revision.

Target branch:

```text
feature/studio-visual-foundation
```

Visual reference:

```text
docs/STUDIO-VISUAL-TARGET-V1.md
```

Do not start F05.5, F06, OTBM/DAT/SPR/OTB integration, real sprites or new core
features during this gate. Keep SDL3 + SDL_Renderer3 + Dear ImGui and the
existing MapDocument / EditorOperations / FMAP / Server / Protocol / Client core.

## V00 — Sync and freeze baseline

```powershell
git checkout feature/studio-visual-foundation
git pull --ff-only
git status --short
git rev-parse HEAD
```

Expected: clean working tree before validation. Record the exact SHA in:

```text
docs/evidence/STUDIO-VISUAL/RESULT.md
```

Do not discard unrelated changes or synchronize over an in-progress rebuild.

## V01 — Full native Release regression

```powershell
./scripts/build-native.ps1 -Configuration Release
```

This must compile Studio, Server and Client and run all existing tests and
validators. Correct relevant warnings/errors. Record observed V5 counts and
results; do not bypass failures or reuse V4 checks as V5 proof.

## V02 — One-command Studio launch

The preferred visual launcher is now:

```powershell
./scripts/run-studio-visual.ps1 -Configuration Release
```

If the Studio binary is missing or you intentionally want to rebuild just the Studio first:

```powershell
./scripts/run-studio-visual.ps1 -Configuration Release -Build
```

The script locates `fantasy-studio-gui.exe`, launches it with the repository root as the project argument, and checks that no Studio process remains after normal close.

Required launch behavior:

- window title is `Fantasy Studio`;
- no renderer error;
- SDL_Renderer3 remains the baseline;
- initial page is Home;
- the executable is the V5 visual target `Studio/UI/StudioAppV5.cpp`, wired by `Studio/CMakeLists.txt`;
- UI typography is a neutral system font, never the rejected pixel/terminal font;
- branding uses Cinzel with the bundled SIL Open Font License 1.1;
- original hero/emblem resources, Cinzel and its license are copied beside
  the executable and resolve through relative paths;
- presentation art is branding, not fake gameplay content; supplied
  screenshots/crops remain design references, never application textures;
- no absolute resource or font path is persisted in a project contract.

## V03 — Home / Project Manager visual gate

Compare Home directly with `docs/STUDIO-VISUAL-TARGET-V1.md`, the approved Home
image/styleboard and V4's real screenshot. First impression must clearly
recall the approved reference; palette similarity alone is insufficient.

Confirm:

- deep navy/blue depth, strong faceted emblem and display wordmark;
- integrated 46 px topbar with Save/Undo/Redo and the right runtime group;
- stable 188 px sidebar (164 px below 1150 px) without CREATE/RUNTIME headings, coherent icons
  and strong blue/cyan active selection;
- rich original illustrated hero with `FANTASY STUDIO` identity;
- three equal project-action cards;
- Recent Projects compact rows using real projects;
- structured information on the right with actual FMAP preview and metrics;
- `Abrir Mapa` routes to Map.

No invented project/date/content fills the layout. Reserved project/import
dialogs retain honest limitations without dominating the presentation.

Capture:

```text
docs/evidence/STUDIO-VISUAL/home-v5.png
```

Correct visual distance as well as clipping, contrast, hierarchy and ergonomic
defects. Rebuild toward the approved target, not a new unrelated direction.

## V04 — Navigation gate

Click every sidebar destination:

```text
Home
Map
Items & Assets
Monsters
NPCs
Spells
Quests
Systems
Server
Client
```

Required:

- no crash;
- selected module remains obvious;
- future modules remain honest shells;
- Home/Map and future modules retain full navigation; Items & Assets switches
  to the reference's compact icon rail without losing any destination;
- reserved runtime pages introduce no unintended processes or simulated gameplay.

## V05 — Map Workspace visual + functional regression

Open `Map` and confirm:

```text
top command bar
left sidebar
compact icon-first map toolbar
dominant initially fitted viewport
cohesive right Inspector / Minimap column
discreet full-width lower Console
```

Repeat the validated editor operations:

```text
select tile
Paint
Undo
Redo
Fill
Add Object
Remove Object
Erase
Save FMAP
reopen Studio
confirm persisted saved state
```

Confirm four chunks/existing tiles and Fill across chunk boundaries. Semantic
ground colors must agree with Home previews, Minimap and Assets without
changing FMAP. Check selection coordinates after fitting/layout changes;
drawing and hit testing preserve `regionOrigin + chunkOffset + tileLocal`.

Use only `MapDocument` / `EditorOperations`, retaining transactional undo/redo.
Verify the complete saved state against the expected edits, not only a visible
marker. After deliberate test edits and normal close, restore the canonical
development FMAP; do not commit regression-test content.

Capture:

```text
docs/evidence/STUDIO-VISUAL/map-v5.png
```

## V06 — Items & Assets visual gate

Open `Items & Assets` and confirm:

- tabs `Items / Sprites / Textures / Sounds`;
- 56 px icon navigation rail and 156 px library tree;
- narrow library tree;
- broad compact asset grid with square semantic previews;
- search field;
- All / Grounds / Objects categories, query filtering and clear selection;
- semantic cards originate from the real current FMAP;
- friendly labels retain real semantic keys in details/tooltips;
- search by semantic key and translated label, including case and accents;
- selected key remains available in the footer without an extra right panel;
- unconnected tabs remain honestly empty;
- last rows remain reachable by scrolling in smaller windows;
- no invented equipment catalog, screenshot-derived sprite or DAT/SPR/OTB
  content is loaded during this visual gate.

Capture:

```text
docs/evidence/STUDIO-VISUAL/items-assets-v5.png
```

## V07 — Resolution / layout sanity

Primary target:

```text
1440 x 900
```

Also resize to a smaller practical desktop window and confirm:

- topbar remains usable;
- sidebar stays visible;
- Map viewport remains reachable;
- Inspector does not cover the viewport;
- Items & Assets tree/grid remain usable;
- no critical control is clipped permanently.

Record client and capture dimensions actually observed. If the display cannot
show 1440x900 completely, report that limit instead of inferring an
exact-resolution PASS from code geometry.

## V08 — Close / cleanup gate

Close the Studio normally.

```powershell
Get-Process fantasy-studio-gui -ErrorAction SilentlyContinue
```

Expected: no process remains. Also confirm no Server/Client orphan process was introduced by this visual phase.

A later deliberate owner-review launch is recorded separately from the
successful cleanup check.

## V09 — Repository integrity

```powershell
git status --short
git diff --check
```

Review only intended visual code, docs, original presentation resources,
Cinzel with its license and real evidence. Exclude build output, logs/caches,
binaries, local databases, legacy game assets, test-map edits and absolute
local paths. Preserve all V4 screenshots.

Record genuine full-window captures from the running application. Native
captures may be encoded as PNG; cropping, compositing or retouching UI pixels
does not prove actual appearance.

## V10 — Closeout

Update:

```text
docs/evidence/STUDIO-VISUAL/RESULT.md
docs/STUDIO-VISUAL-FOUNDATION.md
```

Mark Visual Foundation `PASS` only after the owner accepts the real V5 screenshots
and all of the following gates pass:

```text
Release regression PASS
V5 clearly closer to approved images than V4
Home visual PASS
all navigation PASS
Map functional regression PASS
Save/Reopen PASS
Items & Assets PASS
resolution sanity PASS
cleanup PASS
screenshots recorded
working tree clean except intended evidence commit
```

Record observed build/test counts, implementation SHA and remaining visual
differences in RESULT.md. Report local checks as local results; do not imply
unobserved remote CI success. Technical checks and an agent's comparison do
not replace owner appearance approval. Until that approval, keep visual
acceptance pending. F05.5, F06 and legacy integration remain NOT STARTED.
