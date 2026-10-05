# Fantasy Studio Visual Foundation — Validation Result

Status: **REMOTE CI PASS / WINDOWS INTERACTIVE VALIDATION PENDING**

Branch: `feature/studio-visual-foundation`

## Baseline

- Validation SHA: `3183a71170bcdcaf58cd2941f53ac7b97ab37280`
- Windows version: **PENDING local interactive gate**
- GPU / driver: **PENDING local interactive gate**
- Configuration: `Release`
- Renderer baseline: `SDL3 + SDL_Renderer3 + Dear ImGui`

## Automated / build gate

Remote exact-SHA CI on GitHub Actions: **PASS**

- [x] project layout/contracts
- [x] Fantasy Project v2 validator
- [x] FMAP v0 validator
- [x] FMAP multi-chunk validator
- [x] Fantasy Protocol v1 validator
- [x] project relocation
- [x] Studio configure/build/tests
- [x] Server configure/build/tests/native play
- [x] Client configure/build/smoke test
- [x] two-process native play
- [x] Studio Windows artifact published
- [x] native runtime Windows artifact published

Exact-SHA local interactive execution of `./scripts/build-native.ps1 -Configuration Release` is still part of the Windows visual gate below, because it also establishes the local evidence environment.

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

Status: **PASS**

GitHub Actions runners normalized and the visual-foundation branch received a real `windows-latest` runner.

- Run: `#231` / `37380616761`
- Exact SHA: `3183a71170bcdcaf58cd2941f53ac7b97ab37280`
- Result: **SUCCESS**
- Job: `foundation` — **SUCCESS**
- Studio Windows artifact: published
- Native runtime Windows artifact: published

The remote workflow proved validators, Studio/Server/Client builds and tests, native play, two-process play and artifact publication on Windows.

## Final decision

**PENDING WINDOWS INTERACTIVE VISUAL GATE**

Remote CI is closed as PASS. Do not mark the visual foundation fully PASS until the real Windows interactive Home / navigation / Map editing / Items & Assets / Save-Reopen / cleanup gates and screenshots are completed.
