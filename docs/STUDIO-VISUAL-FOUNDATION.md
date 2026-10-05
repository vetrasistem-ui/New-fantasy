# Fantasy Studio — Visual Foundation

Status: **IMPLEMENTATION COMPLETE / WINDOWS VISUAL VALIDATION PENDING**

Branch: `feature/studio-visual-foundation`

## Objective

Transform the technical Map Editor shell into the official Fantasy Studio visual foundation while preserving the validated FMAP editor behavior and all F00–F05 contracts.

The approved visual direction is the dark Fantasy Studio styleboard selected by the project owner: deep navy background, cyan/blue accents, compact professional tooling, permanent left navigation, top command bar, central workspace, right inspector and lower console/status area.

This phase establishes the product shell. It does not replace `MapDocument`, FMAP, Server, Protocol or Client domain logic.

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

The UI remains readable without expensive effects. The approved look is achieved with color hierarchy, spacing, borders, compact cards and light accent glow rather than a heavier renderer.

## Implemented product shell

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
- lightweight vector brand mark drawn by the UI layer;
- no external font/image dependency required for the baseline.

## UI-02 — Home / Project Manager — COMPLETE FOR VISUAL FOUNDATION

- branded hero area;
- current-project context;
- New Project / Open Project visual entry points reserved for their real file/dialog flows;
- real Open Map routing;
- Recent Projects area consumes the existing `ProjectManager::recentProjects()` registry when available;
- real world/project counters shown from the loaded FMAP.

No fake project/domain data is inserted.

## UI-03 — Map workspace — COMPLETE FOR VISUAL FOUNDATION

- existing F03 `MapDocument` editor retained;
- central map viewport;
- compact visual tool strip;
- right Inspector with Tile / Item / Object tabs;
- Tile tab keeps the real Ground/Object editing controls;
- Minimap;
- lower Console/status;
- Save, Undo, Redo and editor operations remain wired to the existing core.

The new visual tool strip is presentation/navigation state only where a domain tool has not yet been implemented. It does not invent a second editor core.

## UI-04 — Items & Assets — COMPLETE FOR VISUAL FOUNDATION

- Items / Sprites / Textures / Sounds tabs;
- library tree;
- search input;
- FMAP Grounds/Objects categories;
- preview cards generated from the real semantic asset keys already present in the current FMAP;
- explicit 10.98 / F05.5 source status.

Real sprite pixels are intentionally not loaded here yet; that begins in F05.5 through the shared Fantasy Asset Registry.

## UI-05 — Module shells — COMPLETE

Consistent workspaces exist for:

- Monsters;
- NPCs;
- Spells;
- Quests;
- Systems;
- Server;
- Client.

Each shell states its owning future phase and does not simulate domain data.

## UI-06 — Windows validation — PENDING

The visual implementation must not be marked PASS until a real Windows run proves all of the following:

```text
1. Release build succeeds.
2. fantasy-studio-gui opens on the SDL_Renderer3 baseline.
3. Home matches the approved dark navy/cyan hierarchy.
4. Every sidebar module is reachable.
5. Map workspace keeps the real FMAP visible/editable.
6. Tile selection works.
7. Paint works.
8. Fill works.
9. Add Object works.
10. Remove Object works.
11. Erase works.
12. Undo / Redo work.
13. Save and reopen preserve semantics.
14. Items & Assets shows the current FMAP semantic keys.
15. No orphan process remains after close.
16. F00–F05 regressions remain green.
17. Screenshots of Home, Map and Items & Assets are captured.
```

If remote GitHub runners are still unavailable, perform the Windows local gate first and keep remote CI as `PENDING`, not as a code failure.

## Non-goals

This phase does not implement:

- F05.5 DAT/SPR/OTB bridge;
- PokeFans OTBM → FMAP import;
- F06 persistence/database;
- final custom font packaging;
- advanced particles/shaders/lighting;
- complete Items/Monsters/NPC/Spell/Quest editors;
- public Server/Client operations.

## Next order after the Windows visual gate

```text
Studio Visual Foundation — PASS
        ↓
F05.5 Legacy Asset Bridge + PokeFans 10.98 migration
        ↓
F06 Persistence / Database
```
