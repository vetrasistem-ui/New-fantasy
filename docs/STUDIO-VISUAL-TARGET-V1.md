# Fantasy Studio — Visual Target v1

Status: **APPROVED TARGET / V4 OWNER ACCEPTANCE PENDING**

This document freezes the visual direction selected for Fantasy Studio before real 10.98 assets are integrated.

The owner rejected the V3 appearance and supplied `Fantasy-Studio-Visual-Package-V4.zip`.
Its [approved styleboard](design/STUDIO-VISUAL-V4/approved-styleboard.png) is the
primary visual reference; [V4 tokens](design/STUDIO-VISUAL-V4/DESIGN_TOKENS.json)
make the typography requirement explicit. These are design references only,
never application textures. The current phase ends at owner approval of real
V4 screenshots, without starting F05.5, F06 or legacy format integration.

## Product character

Fantasy Studio must read as a professional 2D game-production tool, not as a generic dashboard and not as a clone of RME.

The visual language is:

- deep navy workspace;
- compact professional tooling;
- cyan/blue active accents;
- restrained glow only on important identity/active elements;
- clear panel hierarchy;
- fantasy identity in branding, not in every control;
- dense enough for editing work while remaining readable for long sessions.

## Reference hierarchy

```text
Application frame
├── top command bar
├── permanent left navigation
└── active workspace
    ├── primary canvas/content
    ├── secondary inspector/library
    └── lower console/status when relevant
```

The Studio must continue to work on the existing SDL3 + SDL_Renderer3 + Dear ImGui baseline.

## Global tokens

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

### Shape language

- small radii; no oversized rounded-card look;
- thin cyan/blue borders on selected/important surfaces;
- dark internal panels separated mainly by value and subtle borders;
- buttons are compact and rectangular;
- active state is obvious but should not flood an entire window with cyan.

## Topbar target

The topbar is always visible and should contain:

```text
Fantasy mark + FANTASY STUDIO
project / current module context
Save / Undo / Redo
Play / Stop / Build / Settings
```

Requirements:

- identity stays visually left-aligned;
- project/module context is secondary;
- action controls stay aligned right;
- Play uses success semantics, Stop uses error semantics, Build/settings remain neutral;
- no heavy ornamental background behind the whole topbar.

## Sidebar target

Permanent order:

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

Requirements:

- compact rows;
- selected module has cyan/blue background/border hierarchy;
- inactive rows remain quiet;
- future modules may be shells but remain visible so the complete Studio architecture is obvious;
- the bottom may show low-priority technical state such as `Fantasy Protocol v1` and `FMAP native`.

## Home / Project Manager target

The Home screen is the strongest identity surface.

Structure:

```text
Hero / Fantasy identity
Project actions
├── Novo Projeto
├── Abrir Projeto
└── Importar/Abrir mapa
Lower content
├── Projetos Recentes
└── Informações do Projeto
```

Visual acceptance:

- hero is noticeably richer than other panels but remains lightweight;
- action cards have equal proportions and clear hierarchy;
- recent projects behave like compact rows, not oversized cards;
- project information is a structured secondary panel;
- the Home screen must immediately look like Fantasy Studio even before a real OTBM is loaded.

## Map Editor target

The Map workspace is the productivity center.

Structure:

```text
Map toolbar
Central viewport
Right inspector
├── Tile
├── Item
└── Object
Minimap / world context
Console / status
```

Visual acceptance:

- viewport has maximum practical area;
- toolbar is one compact strip;
- active tool is clearly highlighted;
- inspector is visually separate but not oversized;
- console is dark and lower priority, with cyan used for meaningful messages;
- minimap and inspector do not cover the editing canvas;
- current FMAP editing functions remain untouched.

Until real sprites arrive, semantic/placeholder rendering is acceptable. Placeholder rendering must not attempt to imitate final 10.98 art.

## Items & Assets target

Structure:

```text
Tabs: Items / Sprites / Textures / Sounds
Search
Category/library tree
Asset grid
```

Visual acceptance:

- library tree stays narrow;
- asset grid receives most of the width;
- selected asset/card has a strong but clean cyan highlight;
- grid is ready to receive real DAT/SPR textures later without redesigning the page;
- no fake final artwork is bundled merely to make the page look finished.

## Typography

The styleboard uses a fantasy display direction for branding and a neutral UI face for tools.

For the baseline implementation:

- UI readability has priority;
- use Segoe UI or an equivalent neutral system UI font;
- pixel, terminal and monospaced display typography are rejected for the UI;
- do not bundle unlicensed font files;
- display/serif typography is confined to branding; load licensed system fonts
  at runtime rather than redistributing proprietary font files.

## Resolution targets

Primary visual target:

```text
1440 × 900
```

The shell must remain usable at smaller practical desktop sizes. No critical control may depend on a single exact resolution.

## What must be visually finished before the OTBM phase

```text
Topbar
Sidebar
Home / Project Manager
Map workspace shell
Inspector / Minimap / Console hierarchy
Items & Assets shell
buttons / tabs / inputs / cards / selected states
consistent palette and spacing
```

Real DAT/SPR pixels, OTB metadata and OTBM map content are deliberately the next milestone, not part of the visual-polish gate.

## Freeze rule

Once the owner accepts the Studio appearance, this becomes the visual foundation. Future phases may extend components but should not redesign the global shell unless a real usability problem is discovered.
