# Fantasy Studio — Visual Foundation

Status: **IN PROGRESS**

Branch: `feature/studio-visual-foundation`

## Objective

Transform the current technical Map Editor shell into the official Fantasy Studio visual foundation while preserving the validated FMAP editor behavior and all F00–F05 contracts.

The approved visual direction is the dark Fantasy Studio styleboard selected by the project owner: deep navy background, cyan/blue accents, compact professional tooling, permanent left navigation, top command bar, central workspace, right inspector and lower console/status area.

This phase establishes the product shell. It does not replace MapDocument, FMAP, Server, Protocol or Client domain logic.

## Visual tokens

Reference palette:

```text
Background       #081220
Surface          #111827
Panel            #1E293B
Primary          #0EA5E9
Primary bright   #22D3EE
Hover            #3B82F6
Text primary     #E2E8F0
Text secondary   #94A3B8
Success          #10B981
Warning          #F59E0B
Error            #EF4444
```

The UI must remain readable without expensive effects. Glow/shadows are optional polish, not architectural requirements.

## Main shell

```text
Fantasy Studio
├── Topbar
│   ├── product/project identity
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
└── Workspace
    ├── active module
    ├── Inspector
    └── Console / status
```

## Delivery stages

### UI-00 — Design tokens

- centralize ImGui colors, rounding, spacing and component sizing;
- keep SDL3 + SDL_Renderer3 + Dear ImGui;
- no renderer upgrade.

### UI-01 — Studio shell

- fixed topbar;
- fixed sidebar;
- active-page state;
- central workspace bounds;
- no domain changes.

### UI-02 — Home / Project Manager shell

- current project card;
- New Project / Open Project / Import Project entry points as UI shells;
- Open Map action routes to the real Map workspace;
- recent-project integration may follow using existing ProjectManager APIs.

### UI-03 — Map workspace

- retain the real F03 MapDocument editor;
- central map viewport;
- right inspector/brushes;
- minimap/world information;
- lower console/status;
- Save, Undo, Redo and editing operations remain wired to the existing core.

### UI-04 — Module shells

Create consistent placeholder workspaces for Items & Assets, Monsters, NPCs, Spells, Quests, Systems, Server and Client. A placeholder must clearly say when the subsystem is not implemented; it must not fake domain functionality.

### UI-05 — Validation

- Studio opens on Windows on the SDL_Renderer3 baseline;
- Home and every sidebar module can be selected;
- Map page still edits the canonical FMAP through MapDocument;
- Paint / Fill / Add Object / Remove Object / Erase / Undo / Redo / Save remain functional;
- Save/Reopen semantics stay unchanged;
- no absolute paths introduced;
- F00–F05 automated regressions remain green when runners are available;
- real Windows screenshots are captured before marking this visual foundation PASS.

## Non-goals

This phase does not implement:

- F05.5 DAT/SPR/OTB bridge;
- PokeFans OTBM → FMAP import;
- F06 persistence/database;
- final custom font packaging;
- advanced particles/shaders/lighting;
- complete Items/Monsters/NPC/Spell/Quest editors;
- public Server/Client operations.

## Next order

```text
Studio Visual Foundation
        ↓
F05.5 Legacy Asset Bridge + PokeFans 10.98 migration
        ↓
F06 Persistence / Database
```
