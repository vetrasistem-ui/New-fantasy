# Fantasy Studio Visual Foundation — Validation Result

Status: **V3 REMOTE CI PASS / WINDOWS INTERACTIVE VISUAL ACCEPTANCE PENDING**

Branch: `feature/studio-visual-foundation`

## Baseline

- Visual implementation SHA: `68f5eb76030a4bbb7f2b8654498778562258dd84`
- Visual shell: `Studio/UI/StudioAppV3.cpp`
- Windows version: **PENDING local interactive gate**
- GPU / driver: **PENDING local interactive gate**
- Configuration: `Release`
- Renderer baseline: `SDL3 + SDL_Renderer3 + Dear ImGui`

## Automated / build gate

Remote exact-SHA GitHub Actions for the V3 visual shell: **PASS**

- Run: `#237` / `37384402410`
- Exact implementation SHA: `68f5eb76030a4bbb7f2b8654498778562258dd84`
- Result: **SUCCESS**

Validated remotely on Windows:

- [x] project layout/contracts
- [x] Fantasy Project v2 validator
- [x] FMAP v0 validator
- [x] FMAP multi-chunk validator
- [x] Fantasy Protocol v1 validator
- [x] project relocation
- [x] V3 Studio configure/build/tests
- [x] Studio Windows artifact published
- [x] Server configure/build/tests/native play
- [x] Client configure/build/smoke test
- [x] two-process native play
- [x] native runtime Windows artifact published

The V3 Studio Windows artifact was produced successfully before the remainder of the native regression completed.

Documentation/launcher commits after the implementation SHA do not change the V3 executable source. Their current branch runs are ordinary revalidation runs.

## Home — interactive acceptance pending

- [ ] Fantasy Studio window opens
- [ ] dark navy/cyan hierarchy matches the approved direction
- [ ] Fantasy brand identity is clear
- [ ] topbar layout is stable
- [ ] sidebar active-state rail is clear
- [ ] hero is visually balanced
- [ ] three project action cards are aligned
- [ ] Recent Projects rows are readable
- [ ] real FMAP project metrics are visible
- [ ] `Abrir Map Workspace` routes to Map

Evidence target:

```text
docs/evidence/STUDIO-VISUAL/home.png
```

## Navigation — interactive acceptance pending

- [ ] Home
- [ ] Map
- [ ] Items & Assets
- [ ] Monsters
- [ ] NPCs
- [ ] Spells
- [ ] Quests
- [ ] Systems
- [ ] Server
- [ ] Client

## Map Workspace — interactive acceptance pending

- [ ] compact tool strip
- [ ] viewport remains dominant
- [ ] Inspector tabs
- [ ] Minimap
- [ ] Console
- [ ] Select tile
- [ ] Paint
- [ ] Undo
- [ ] Redo
- [ ] Fill
- [ ] Add Object
- [ ] Remove Object
- [ ] Erase
- [ ] Save
- [ ] Reopen / semantic persistence
- [ ] canonical FMAP restored after validation when required

Evidence target:

```text
docs/evidence/STUDIO-VISUAL/map.png
```

## Items & Assets — interactive acceptance pending

- [ ] Items / Sprites / Textures / Sounds tabs
- [ ] narrow library tree
- [ ] wide asset grid
- [ ] search
- [ ] Grounds
- [ ] Objects
- [ ] semantic cards originate from current FMAP
- [ ] PokeFans / 10.98 is shown only as the next source
- [ ] no DAT/SPR pixels loaded before the compatibility phase

Evidence target:

```text
docs/evidence/STUDIO-VISUAL/items-assets.png
```

## Resolution / cleanup pending

- [ ] 1440x900 target layout accepted
- [ ] smaller practical window sanity check
- [ ] Studio closes cleanly
- [ ] no orphan Studio process
- [ ] no Server/Client orphan process introduced
- [ ] `git diff --check` clean
- [ ] no build/cache/binary legacy files staged

## Preferred local command

```powershell
./scripts/run-studio-visual.ps1 -Configuration Release -Build
```

The launcher builds when requested, opens the Studio with the repository root and verifies that the GUI leaves no orphan process after normal close.

## Final decision

**PENDING WINDOWS INTERACTIVE VISUAL ACCEPTANCE**

The V3 code/build gate is closed as PASS. The remaining work is visual/ergonomic acceptance on a real Windows display, screenshots, and any small polish corrections discovered there. Only after this acceptance should the project move to opening a real 10.98 OTBM with matching DAT/SPR/OTB and rendering real sprites.
