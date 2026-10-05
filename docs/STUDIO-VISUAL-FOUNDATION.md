# Fantasy Studio — Visual Foundation

Status: **VISUAL POLISH IN PROGRESS / REMOTE CI BASELINE PASS**

Branch: `feature/studio-visual-foundation`

## Objective

Transform the technical Map Editor shell into the official Fantasy Studio visual foundation while preserving the validated FMAP editor behavior and all F00–F05 contracts.

The approved visual direction is the Fantasy Studio styleboard selected by the project owner: deep navy background, cyan/blue accents, compact professional tooling, permanent left navigation, top command bar, central workspace, right inspector and lower console/status area.

The product shell comes first. Real 10.98 assets and OTBM content are the next milestone, not a prerequisite for finishing the shell.

## Current product priority

```text
1. Finish the Fantasy Studio visual base
2. Open a real 10.98 OTBM with real sprites/assets
3. Use that real map as the working production surface
4. Expand editing tools and domain editors
5. Persistence/database comes later
```

Do not start F06 while the visual base and real-map bridge are still open.

## Approved visual target

The frozen reference contract is documented in:

```text
docs/STUDIO-VISUAL-TARGET-V1.md
```

Core tokens:

```text
Background       #081220
Surface          #111827
Panel            #1E293B
Primary          #0EA5E9
Primary bright   #22D3EE
Hover            #3B82F6
Active           #60A5FA
Text primary     #E2E8F0
Text secondary   #94A3B8
Success          #10B981
Warning          #F59E0B
Error            #EF4444
```

The UI remains lightweight on SDL3 + SDL_Renderer3 + Dear ImGui. The approved look is created through hierarchy, compact spacing, restrained borders, active-state cyan and limited identity glow rather than expensive effects.

## Current implementation — V3 polish shell

The executable target `fantasy-studio-gui` now points to:

```text
Studio/UI/StudioAppV3.cpp
```

`StudioAppV2.cpp` remains temporarily in-tree only as a rollback reference until the Windows visual gate closes.

V3 tightens the visual direction without replacing any map/domain core:

- stronger Fantasy identity in topbar and Home;
- compact breadcrumb project/module context;
- right-aligned Save / Undo / Redo and runtime-control grouping;
- sidebar active-state accent rail and cleaner hierarchy;
- technical status pill at the bottom of the sidebar;
- richer but lightweight Home hero;
- Project Manager cards aligned to the approved proportions;
- compact Recent Projects rows with generated neutral previews;
- structured project-stat cards using real FMAP counts;
- Map toolbar with clear selected tool state;
- viewport-first Map layout with Inspector, Minimap and Console separation;
- Items & Assets library tree kept narrow while the grid receives most width;
- real semantic FMAP keys remain the only current asset-card source;
- explicit PokeFans / 10.98 source state without loading legacy pixels yet;
- future modules keep consistent visual shells without fake domain data.

## Product shell

```text
Fantasy Studio
├── Topbar
│   ├── Fantasy mark + product name
│   ├── project / active module context
│   ├── Save / Undo / Redo
│   └── Play / Stop / Build / Settings
├── Sidebar
│   ├── Home
│   ├── Map
│   ├── Items & Assets
│   ├── Monsters
│   ├── NPCs
│   ├── Spells
│   ├── Quests
│   ├── Systems
│   ├── Server
│   └── Client
└── Workspaces
    ├── Home / Project Manager
    ├── Map Editor
    │   ├── compact tool strip
    │   ├── real FMAP viewport
    │   ├── Inspector tabs
    │   ├── Minimap
    │   └── Console
    ├── Items & Assets
    │   ├── tabs
    │   ├── library tree
    │   ├── search
    │   └── semantic asset grid
    └── module shells for future domain editors
```

## Core-preservation rules

The visual phase must not:

- create a second map model;
- bypass `MapDocument` or `EditorOperations`;
- add OTBM/DAT/SPR/OTB dependencies to the native map core;
- replace SDL_Renderer3 with a heavier renderer;
- start F06 persistence;
- fake final asset pixels just to make the UI look complete.

Save, Paint, Fill, Add Object, Remove Object, Erase, Undo and Redo remain wired to the validated editor core.

## Windows gate after polish

Remote CI proved the previous visual baseline on Windows. The polished V3 shell must compile in the same workflow and then receive one real interactive Windows pass before visual acceptance.

Interactive acceptance:

```text
1. Fantasy Studio opens on real Windows.
2. Home follows the approved visual target.
3. Sidebar/topbar hierarchy is clear and stable.
4. Every module is reachable without crash.
5. Map remains editable through MapDocument/EditorOperations.
6. Paint / Fill / Add / Remove / Erase / Undo / Redo / Save-Reopen pass.
7. Items & Assets layout is usable and visually consistent.
8. Studio closes without orphan processes.
9. Home / Map / Items & Assets screenshots are recorded.
```

## What comes immediately after visual acceptance

```text
PokeFans / 10.98 source pack
        ↓
DAT + SPR + OTB
        +
OTBM + houses + spawns
        ↓
Legacy compatibility layer
        ↓
Fantasy Asset Registry + map import/open path
        ↓
Fantasy Studio renders the real map with real sprites
```

The Studio is expected to support legacy OTBM + matching DAT/SPR/OTB as an editing/import compatibility surface while keeping Fantasy's native architecture isolated from those formats.

## After the real map opens correctly

```text
real map + real sprites
        ↓
map editing refinement
        ↓
Items & Assets real catalog
        ↓
Monster / NPC / Spell / Quest / Systems tools
        ↓
other game-production systems
```

## Non-goals of the current visual phase

- DAT/SPR/OTB decoding;
- OTBM conversion/open compatibility implementation;
- persistence/database;
- complete Items/Monsters/NPC/Spell/Quest editors;
- production Server/Client controls.

Those remain deliberately outside the visual PR until the Studio appearance is accepted.
