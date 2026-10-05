# Fantasy Studio — Visual Foundation

Status: **VISUAL POLISH IN PROGRESS / REMOTE CI PASS**

Branch: `feature/studio-visual-foundation`

## Objective

Transform the technical Map Editor shell into the official Fantasy Studio visual foundation while preserving the validated FMAP editor behavior and all F00–F05 contracts.

The approved visual direction is the Fantasy Studio styleboard selected by the project owner: deep navy background, cyan/blue accents, compact professional tooling, permanent left navigation, top command bar, central workspace, right inspector and lower console/status area.

This phase establishes the product shell first. It does not replace `MapDocument`, FMAP, Server, Protocol or Client domain logic.

## Current product priority

The project order is now intentionally visual-first:

```text
1. Finish the Fantasy Studio visual base
2. Open a real 10.98 OTBM with real sprites/assets
3. Use that real map as the working production surface
4. Then expand editing tools and domain editors
5. Persistence/database comes later
```

Do not start F06 while the visual base and real-map bridge are still open.

## Visual tokens

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

The UI must remain readable and lightweight on the SDL_Renderer3 baseline. The approved appearance is achieved through hierarchy, spacing, borders, compact cards, accent states and restrained glow rather than a heavier renderer.

## Product shell already implemented

```text
Fantasy Studio
├── Topbar
│   ├── Fantasy brand mark + product name
│   ├── project / active module context
│   ├── Save / Undo / Redo
│   └── Play / Stop / Build / Settings visual controls
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
    │   ├── tool strip
    │   ├── real FMAP viewport
    │   ├── Inspector tabs
    │   ├── Minimap
    │   └── Console
    ├── Items & Assets
    │   ├── tabs
    │   ├── library tree
    │   ├── search
    │   └── semantic asset grid from the real FMAP
    └── module shells for future domain editors
```

## UI-00 — Design tokens — COMPLETE

- colors, borders, rounding, spacing and component behavior centralized in `StudioTheme`;
- SDL3 + SDL_Renderer3 + Dear ImGui preserved;
- no renderer upgrade and no GPU feature requirement added.

## UI-01 — Studio shell — COMPLETE

- fixed topbar;
- permanent sidebar;
- active-page state;
- central workspace bounds;
- lightweight Fantasy brand mark drawn by the UI layer;
- no external font/image dependency required for the baseline.

## UI-02 — Home / Project Manager — FUNCTIONAL BASE COMPLETE

- branded hero area;
- current-project context;
- project action cards;
- real Open Map routing;
- recent project registry integration when available;
- real FMAP/project counters.

No fake project/domain data is inserted.

## UI-03 — Map workspace — FUNCTIONAL BASE COMPLETE

- existing F03 `MapDocument` editor retained;
- central map viewport;
- compact visual tool strip;
- right Inspector with Tile / Item / Object tabs;
- real Ground/Object editing controls;
- Minimap;
- lower Console/status;
- Save, Undo, Redo and editor operations remain wired to the existing core.

The visual toolbar does not create a second editor core.

## UI-04 — Items & Assets — VISUAL BASE COMPLETE

- Items / Sprites / Textures / Sounds tabs;
- library tree;
- search input;
- FMAP Grounds/Objects categories;
- preview cards generated from semantic asset keys already present in FMAP;
- explicit 10.98 / F05.5 source status.

Real sprite pixels are intentionally deferred to the next phase.

## UI-05 — Module shells — COMPLETE

Consistent visual workspaces exist for Monsters, NPCs, Spells, Quests, Systems, Server and Client. They reserve the product structure without pretending the future domain logic exists.

## UI-06 — Visual fidelity polish — IN PROGRESS

Before closing the visual phase, refine the current implementation toward the approved styleboard:

- stronger Fantasy identity in topbar/Home while keeping the UI lightweight;
- cleaner navy surface hierarchy;
- cyan edge/accent treatment without excessive glow;
- more polished sidebar active states;
- consistent icon/badge language for navigation and map tools;
- stronger separation between viewport, inspector, minimap and console;
- Home cards closer to the approved Project Manager proportions;
- Map toolbar spacing/selection states closer to the styleboard;
- Items & Assets tree/grid proportions closer to the reference;
- consistent button sizes, tabs, panels, search fields and status components;
- keep the layout usable at 1440×900 and lower practical resolutions.

Do not block this polish on OTBM, DAT/SPR/OTB or persistence work.

## UI-07 — Windows interactive visual gate — PENDING AFTER POLISH

Remote CI already passed for the visual branch baseline. The final interactive gate happens only after the polish pass is ready.

Required evidence:

```text
1. Fantasy Studio opens on real Windows.
2. Home visually follows the approved styleboard direction.
3. Sidebar/topbar hierarchy is clear and stable.
4. Every module can be reached without crash.
5. Map workspace remains editable through MapDocument/EditorOperations.
6. Paint / Fill / Add / Remove / Erase / Undo / Redo / Save-Reopen pass.
7. Items & Assets layout is usable and consistent.
8. Studio closes without orphan processes.
9. Home / Map / Items & Assets screenshots are recorded.
```

## What comes immediately after visual PASS

The next milestone is not database work. It is the first real-world production surface:

```text
PokeFans / 10.98 source pack
        ↓
DAT + SPR + OTB
        +
OTBM + houses + spawns
        ↓
Legacy Asset Bridge + OTBM Importer
        ↓
FMAP + Fantasy Asset Registry
        ↓
Fantasy Studio renders the real map with real sprites
```

The first target package already inventoried for F05.5 is the supplied 10.98 set centered on `global_dash.otbm`, the matching house/spawn XMLs and the candidate DAT/SPR/OTB set.

## After the real map opens correctly

Only after the Studio can open a real OTBM-derived FMAP with real sprites do we expand the rest of the editor surface:

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

## Non-goals of the visual phase

This phase does not implement:

- DAT/SPR/OTB decoding;
- OTBM → FMAP conversion;
- persistence/database;
- complete Items/Monsters/NPC/Spell/Quest editors;
- production Server/Client controls.

Those remain deliberately outside the visual PR until the Studio appearance is accepted.
