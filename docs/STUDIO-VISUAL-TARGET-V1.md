# Fantasy Studio — Visual Target v1

Status: **APPROVED TARGET / V4 VISUAL FAIL / V5 OWNER ACCEPTANCE PENDING**

This document freezes the visual direction selected for Fantasy Studio before real 10.98 assets are integrated.

The owner rejected the V4 appearance on `2026-10-06`: **V4 TECHNICAL PASS /
VISUAL FAIL**. Its [original result](evidence/STUDIO-VISUAL/RESULT-V4.md) and
screenshots remain historical evidence. The supplied package's
[approved styleboard](design/STUDIO-VISUAL-V4/approved-styleboard.png) and
[tokens](design/STUDIO-VISUAL-V4/DESIGN_TOKENS.json) remain the primary reference.
V5 must show an immediate, strong correspondence with the approved Home, Map
and Items & Assets images, rather than merely share their palette or theme.
These images are design references only, never application textures. This
phase does not start F05.5, F06, real sprites or legacy format integration.

## Product character

Fantasy Studio must read as premium professional 2D game-creation software.
Its first impression must clearly recall the approved styleboard. V5 rebuilds
the composition, identity and controls as well as correcting visual defects.

The visual language is:

- deep navy workspace;
- compact professional tooling;
- cyan/blue active accents;
- restrained glow only on important identity/active elements;
- clear panel hierarchy;
- fantasy identity in branding, not in every control;
- dense enough for editing work while remaining readable for long sessions.

Apply the palette as layered surfaces, not flat gray fills and a wireframe of
panel borders. Local navy/blue gradients, metallic highlights and coherent
icons provide the reference's depth without excessive glow.

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

The V5 topbar is a compact 46 px surface, always visible, containing:

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
- Play follows the reference's blue/cyan treatment, Stop uses a red indicator,
  and Build/settings remain neutral; actual disabled states stay honest;
- no heavy ornamental background behind the whole topbar.

## Sidebar target

The V5 sidebar is 188 px wide (164 px below 1150 px), with no CREATE/RUNTIME
group headings. Items & Assets uses the reference's compact 56 px icon rail;
all ten destinations remain available with identifying tooltips.
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
- selected module has a strong cyan/blue background/border hierarchy;
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

- hero uses a strong faceted emblem, display wordmark and original illustrative
  presentation art, matching the approved Home composition immediately;
- action cards have equal proportions and clear hierarchy;
- preserve the preferred window aspect ratio when fitting a smaller display;
  the practical Home uses a 140 px hero and 138 px cards with centered text;
- recent projects behave like compact rows, not oversized cards;
- project information is a structured secondary panel;
- the Home screen must immediately look like Fantasy Studio even before a real OTBM is loaded.

V5 presentation artwork lives in `Studio/UI/Assets/`; its hero/emblem are
branding, not map content or a catalog of loaded game assets. See
[art provenance](design/STUDIO-VISUAL-V5/ART-PROVENANCE.md). No supplied
screenshot, logo crop, fake map art or invented sprite is an application texture.
Recent rows must use real projects and metadata. Integrate an actual FMAP
preview into the information panel on the right. Do not invent projects,
dates or metrics to match the reference's density. Existing project/import
dialogs remain reserved, with their limitations explicit.

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
- toolbar is one compact icon-first strip;
- active tool is clearly highlighted;
- inspector is visually separate but not oversized;
- console spans the full workspace width, dark and lower priority, with cyan
  used for meaningful messages;
- minimap and inspector do not cover the editing canvas;
- current FMAP editing functions remain untouched.

Initial map framing fits the real FMAP into the canvas. Semantic ground colors
must agree across canvas, Home previews, Minimap and Assets. They visualize
existing keys without changing map data or introducing final sprite art.
Inspector and Minimap form one cohesive right column outside the canvas.

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

Use compact square preview cells rather than V4's large horizontal swatches.
Friendly labels retain access to the real semantic keys in details/tooltips.
Place search above the tree/grid, and keep unconnected tabs honestly empty.
Use the compact icon rail and a 156 px library tree, with selection details in
the footer/tooltips rather than an extra right panel that narrows the grid.
No equipment catalog or legacy source is invented for presentation.

## Typography

The styleboard uses a fantasy display direction for branding and a neutral UI face for tools.

For the baseline implementation:

- UI readability has priority;
- use Segoe UI or an equivalent neutral system UI font;
- pixel, terminal and monospaced display typography are rejected for the UI;
- do not bundle unlicensed font files;
- display typography is confined to branding: V5 uses Cinzel under
  [SIL Open Font License 1.1](../Studio/UI/Assets/Fonts/OFL-Cinzel.txt);
- copy original presentation assets, Cinzel and its license beside the
  executable and resolve them with relative resource paths;
- load proprietary system UI fonts at runtime, without redistributing them
  or persisting absolute font paths in project contracts.

## Resolution targets

Primary visual target:

```text
1440 × 900
```

The shell must remain usable at smaller practical desktop sizes. No critical
control may depend on a single exact resolution. Record dimensions actually
observed; preferred code geometry is not proof of exact-resolution inspection.

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

V5 screenshots must be clearly closer to the approved images than V4, and
build/tests, editing behavior and cleanup must pass. Owner approval remains
required; technical success alone does not mark visual PASS or begin F05.5,
F06 or legacy integration.
