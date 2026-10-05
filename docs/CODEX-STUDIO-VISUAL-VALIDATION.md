# Codex — Fantasy Studio Visual Foundation Windows Gate

Status: **READY TO EXECUTE AFTER V3 REMOTE BUILD**

Target branch:

```text
feature/studio-visual-foundation
```

Visual reference:

```text
docs/STUDIO-VISUAL-TARGET-V1.md
```

Do not start OTBM/DAT/SPR integration or F06 during this gate. The goal is to accept the Studio shell first.

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

## V01 — Full native Release regression

```powershell
./scripts/build-native.ps1 -Configuration Release
```

This must compile Studio, Server and Client and run the existing tests. Do not bypass failures.

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
- the executable is the V3 visual target wired by `Studio/CMakeLists.txt`.

## V03 — Home / Project Manager visual gate

Compare Home directly with `docs/STUDIO-VISUAL-TARGET-V1.md` and the approved styleboard.

Confirm:

- dark navy/cyan Fantasy identity;
- compact topbar with brand, project/module breadcrumb and action grouping;
- permanent sidebar with clear active-state rail;
- lightweight hero with `FANTASY STUDIO` identity;
- three equal project-action cards;
- Recent Projects compact rows;
- project information panel using real FMAP metrics;
- `Abrir Map Workspace` routes to Map.

Capture:

```text
docs/evidence/STUDIO-VISUAL/home.png
```

Only correct real layout defects, clipping, contrast, hierarchy or ergonomic issues. Do not redesign away from the approved target.

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
- sidebar does not shift between pages.

## V05 — Map Workspace visual + functional regression

Open `Map` and confirm:

```text
top command bar
left sidebar
compact map toolbar
central viewport as dominant area
right Inspector
Minimap
lower Console
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

Use only `MapDocument` / `EditorOperations`. Restore the canonical development FMAP after intentional test edits when required.

Capture:

```text
docs/evidence/STUDIO-VISUAL/map.png
```

## V06 — Items & Assets visual gate

Open `Items & Assets` and confirm:

- tabs `Items / Sprites / Textures / Sounds`;
- narrow library tree;
- wide asset grid;
- search field;
- Grounds / Objects categories;
- semantic cards originate from the real current FMAP;
- `PokeFans / 10.98` is presented only as the next legacy source;
- no DAT/SPR pixels are loaded during this visual gate.

Capture:

```text
docs/evidence/STUDIO-VISUAL/items-assets.png
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

## V08 — Close / cleanup gate

Close the Studio normally.

```powershell
Get-Process fantasy-studio-gui -ErrorAction SilentlyContinue
```

Expected: no process remains. Also confirm no Server/Client orphan process was introduced by this visual phase.

## V09 — Repository integrity

```powershell
git status --short
git diff --check
```

Allowed post-validation changes are only intended evidence/docs and reviewed visual fixes. Do not stage build output, local databases, DAT/SPR/OTB/OTBM or absolute-path files.

## V10 — Closeout

Update:

```text
docs/evidence/STUDIO-VISUAL/RESULT.md
docs/STUDIO-VISUAL-FOUNDATION.md
```

Mark Visual Foundation `PASS` only if:

```text
Release regression PASS
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

After the visual gate is accepted, the next milestone is compatibility with the supplied 10.98 content:

```text
OTBM + matching DAT/SPR/OTB
        ↓
legacy compatibility layer
        ↓
real map + real sprites inside Fantasy Studio
```

Only after that real-map surface is working do we expand the larger editor/tool set.
