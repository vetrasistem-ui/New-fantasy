# Codex — Fantasy Studio Visual Foundation Windows Gate

Status: **READY TO EXECUTE**

Target branch:

```text
feature/studio-visual-foundation
```

Do not start F05.5 or F06 during this gate.

## V00 — Sync and freeze baseline

```powershell
git checkout feature/studio-visual-foundation
git pull --ff-only
git status --short
git rev-parse HEAD
```

Expected: clean working tree before validation.

Record the exact SHA in:

```text
docs/evidence/STUDIO-VISUAL/RESULT.md
```

## V01 — Full native Release build

Run:

```powershell
./scripts/build-native.ps1 -Configuration Release
```

This must compile Studio, Server and Client and run the existing tests.

If compilation fails, stop and fix only the visual-foundation branch. Do not bypass compile errors and do not start F05.5/F06.

## V02 — Launch Fantasy Studio

Locate the generated GUI executable, normally one of:

```text
build/studio/Release/fantasy-studio-gui.exe
build/studio/fantasy-studio-gui.exe
```

If needed:

```powershell
Get-ChildItem -Recurse build/studio -Filter fantasy-studio-gui.exe
```

Launch from repository root with the project root as argument:

```powershell
& <path-to-fantasy-studio-gui.exe> (Get-Location).Path
```

Gate:

- window title is `Fantasy Studio`;
- application opens without renderer error;
- no SDL_GPU requirement is introduced;
- initial page is Home.

## V03 — Home / Project Manager visual gate

Confirm visually:

- dark navy/cyan Fantasy Studio identity;
- Fantasy brand mark in topbar and hero;
- permanent sidebar;
- topbar context;
- hero banner;
- project action cards;
- project information panel;
- real FMAP counters visible;
- `Abrir Map Workspace` routes to Map.

Capture screenshot:

```text
docs/evidence/STUDIO-VISUAL/home.png
```

Do not alter the reference style direction during validation. Only correct layout defects, clipping, illegible contrast or regressions.

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
- selected module remains visually obvious;
- future modules clearly remain shells and do not invent fake functionality.

## V05 — Map Workspace visual + functional regression

Open `Map`.

Confirm visual structure:

```text
top command bar
left sidebar
map toolbar
central viewport
right Inspector
Minimap
lower Console
```

Then repeat the real F03 editor operations on the development fixture:

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

Use only the existing `MapDocument` / `EditorOperations` path.

Before finishing the gate, restore the canonical development FMAP if the validation intentionally changed fixture content.

Capture screenshot:

```text
docs/evidence/STUDIO-VISUAL/map.png
```

## V06 — Items & Assets visual gate

Open `Items & Assets`.

Confirm:

- tabs `Items / Sprites / Textures / Sounds`;
- library tree;
- search field;
- `Grounds` and `Objects` categories;
- semantic asset cards come from the real current FMAP;
- no DAT/SPR pixel loading is performed yet;
- UI clearly identifies F05.5 / PokeFans 10.98 as the next asset source.

Test search using one semantic key visible in the current fixture.

Capture screenshot:

```text
docs/evidence/STUDIO-VISUAL/items-assets.png
```

## V07 — Close / cleanup gate

Close the Studio normally.

Verify:

```powershell
Get-Process fantasy-studio-gui -ErrorAction SilentlyContinue
```

Expected: no process remains.

Also confirm no Server/Client orphan process was introduced by this visual phase.

## V08 — Repository integrity

Run:

```powershell
git status --short
git diff --check
```

Allowed changes after validation are only the intended evidence/docs and any explicitly reviewed visual fixes.

No build output, local database, DAT/SPR/OTB/OTBM or absolute-path file may be staged.

## V09 — Closeout

Update:

```text
docs/evidence/STUDIO-VISUAL/RESULT.md
docs/STUDIO-VISUAL-FOUNDATION.md
```

Mark the visual foundation `PASS` only if:

```text
Release build PASS
existing tests PASS
Home visual PASS
all navigation PASS
Map functional regression PASS
Save/Reopen PASS
Items & Assets PASS
cleanup PASS
screenshots recorded
working tree clean except intended evidence commit
```

Remote GitHub CI may remain `PENDING` when runner infrastructure is unavailable. Record that condition separately; do not misclassify a cancelled/unallocated runner as a product test failure.

After this gate is closed, the next implementation phase is:

```text
F05.5 — Legacy Asset Bridge + PokeFans 10.98 + OTBM -> FMAP
```
