# Fantasy Studio Visual Foundation — Validation Result

Status: **PENDING WINDOWS VALIDATION**

Branch: `feature/studio-visual-foundation`

## Baseline

- Validation SHA: **PENDING**
- Windows version: **PENDING**
- GPU / driver: **PENDING**
- Configuration: `Release`
- Renderer baseline: `SDL3 + SDL_Renderer3 + Dear ImGui`

## Automated / build gate

- [ ] `./scripts/build-native.ps1 -Configuration Release`
- [ ] Studio compile PASS
- [ ] Studio CTests PASS
- [ ] Server regressions PASS
- [ ] Client regressions PASS

## Home

- [ ] Fantasy Studio window opens
- [ ] dark navy/cyan theme
- [ ] brand mark / identity
- [ ] sidebar
- [ ] topbar
- [ ] hero
- [ ] project cards
- [ ] project information / real FMAP counts
- [ ] route to Map

Evidence:

```text
docs/evidence/STUDIO-VISUAL/home.png
```

## Navigation

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

## Map Workspace

- [ ] tool strip
- [ ] viewport
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

Evidence:

```text
docs/evidence/STUDIO-VISUAL/map.png
```

## Items & Assets

- [ ] Items / Sprites / Textures / Sounds tabs
- [ ] library tree
- [ ] search
- [ ] Grounds
- [ ] Objects
- [ ] semantic cards originate from current FMAP
- [ ] no DAT/SPR pixels loaded before F05.5

Evidence:

```text
docs/evidence/STUDIO-VISUAL/items-assets.png
```

## Cleanup / integrity

- [ ] Studio closes cleanly
- [ ] no orphan Studio process
- [ ] no Server/Client orphan process introduced
- [ ] `git diff --check` clean
- [ ] no build/cache/binary legacy files staged

## Remote CI

Status: **PENDING while GitHub runner infrastructure is unavailable/unstable**

A cancelled/unallocated runner is not counted as a product test failure. Record the first completed exact-SHA remote run here when infrastructure normalizes.

- Run: **PENDING**
- Result: **PENDING**

## Final decision

**PENDING**

Do not mark PASS until every mandatory Windows visual/functional gate above is evidenced.
