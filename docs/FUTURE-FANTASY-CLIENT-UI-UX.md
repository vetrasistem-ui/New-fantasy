# Future Phase — Fantasy Client UI/UX & Visual Identity

**Status:** FUTURE / PLANNED

## Objective

Transform the compatible 10.98 graphical client into a client that visually belongs to **Fantasy**, rather than looking like a default Tibia/OTClient installation.

This phase is deliberately separated from protocol/runtime homologation. The functional client must remain compatible with the supported TFS1098 runtime while its presentation, information hierarchy and interaction model evolve toward the game's own identity.

## Position in the roadmap

This phase begins after:

1. Windows client packaging/visual acceptance is stable;
2. VPS/runtime deployment is reproducible;
3. the first gameplay-facing contracts needed by the HUD are sufficiently defined (attributes, equipment, abilities, inventory, quests/objectives and party/social state).

The goal is to avoid redesigning the client repeatedly while gameplay contracts are still moving.

## Non-negotiable rule

The redesign must not require changing the basic 10.98 network/protocol contract simply to achieve a new visual appearance.

Keep these concerns separate:

```text
Protocol / networking / runtime compatibility
                 !=
Client presentation / layout / visual identity
```

UI modules may consume Fantasy-specific extended/system channels later, but visual redesign alone must not contaminate the runtime boundary.

## Visual direction

Target: a modern, clean fantasy-medieval interface with its own identity, compatible with the visual language of the Fantasy game and not a generic OTClient/Tibia skin.

Principles:

- game world remains the visual priority;
- reduce unnecessary chrome and visual noise;
- strong hierarchy for combat-critical information;
- coherent typography, iconography, panels, borders and spacing;
- fantasy-medieval character without becoming visually heavy;
- layouts that scale cleanly from small notebook resolutions to 1080p+;
- consistent feedback for hover, selected, disabled, cooldown, warning and notification states;
- reusable components instead of one-off per-window styling.

## Screens and systems to redesign

### 1. Login and character selection

- Fantasy branding;
- cleaner server/account flow;
- character cards instead of the default list presentation;
- last character / reconnect affordance;
- connection/status feedback;
- future support for news/patch notes without blocking login.

### 2. Main HUD

- player portrait/identity;
- health and mana/resources;
- level/experience;
- important buffs/debuffs;
- compact combat state;
- optional target/focus presentation;
- configurable screen-edge anchoring.

### 3. Action bar / abilities

- primary and secondary hotbars;
- cooldown visualization;
- keybind display;
- disabled/unavailable state;
- resource/requisite feedback;
- drag-and-drop configuration;
- support for class and equipment-driven abilities.

### 4. Inventory and equipment

- Fantasy equipment-slot presentation;
- backpack/container hierarchy;
- item tooltip system;
- stat comparison;
- requirement display (level + attributes);
- clear indication when an equipped item contributes to another item's stat requirement;
- prevent the UI from implying that an item can satisfy its own requirements.

### 5. Character / attributes

- level;
- base and distributed attributes;
- equipment bonuses;
- effective attributes;
- unspent points;
- class/passive information;
- derived stats.

The UI should visibly distinguish natural/distributed points from bonuses supplied by already-equipped equipment.

### 6. Minimap and navigation

- cleaner minimap frame;
- markers;
- player/group markers;
- waypoint/objective support;
- optional expanded map mode;
- future integration with quest/objective tracking.

### 7. Chat and notifications

- modern chat tabs/channels;
- readable combat/system messages;
- non-invasive toast notifications;
- clear distinction between error, warning, reward and system information;
- scalable font and opacity options.

### 8. Quests and objectives

- objective tracker;
- quest log;
- progress states;
- optional map markers;
- compact pinned objectives.

### 9. Party / social / guild

- party frames;
- health/resource visibility according to game rules;
- invites and ready/status states;
- friends/guild presentation;
- future voice/status hooks remain optional and outside the core client requirement.

### 10. Menus and modal windows

- settings;
- keybinds;
- graphics/audio;
- logout/exit;
- confirmations;
- shops/NPC dialogs;
- storage/bank/trade windows;
- reusable modal component system.

## Technical direction

For the TFS1098/OTClient-backed runtime, treat the client as a modular presentation layer.

Prefer:

```text
Fantasy client theme/components
        -> client modules/layouts
        -> existing game/protocol APIs
        -> TFS1098 runtime
```

Avoid:

```text
UI redesign
        -> protocol hacks everywhere
        -> hard-coded gameplay rules inside widgets
```

Game rules remain authoritative outside the UI. Widgets render state and send supported user actions; they do not become the source of truth for gameplay validation.

## Design-system deliverables

Before rebuilding every window, define a small Fantasy client design system:

- typography scale;
- spacing scale;
- panel/card styles;
- buttons and icon buttons;
- tabs;
- progress/resource bars;
- tooltips;
- item slots;
- hotbar slots;
- badges/status chips;
- notifications;
- modal shell;
- scrollbars;
- input fields;
- focus/hover/pressed/disabled states;
- icon grid and naming conventions.

## Configurability

The final client should support player preferences without fragmenting the visual language:

- UI scale;
- chat scale;
- minimap size;
- action-bar placement;
- selected HUD modules;
- transparency where appropriate;
- saved layout/preferences per account or local profile.

## Acceptance gates

The phase is not considered complete because a new skin merely looks attractive.

Minimum gates:

1. login and character selection use Fantasy visual identity;
2. player enters the game without falling back to default OTCv8 windows unexpectedly;
3. HUD, inventory, equipment, hotbar, minimap and chat use the shared component system;
4. no regression in login, movement, combat actions, containers, item use or hotkeys;
5. no protocol change exists solely for cosmetic purposes;
6. UI remains usable at 1024x600, 1366x768 and 1920x1080;
7. important gameplay state remains readable during combat;
8. keyboard/mouse focus does not become trapped by overlays;
9. screenshots are captured for login, character selection, normal exploration, combat and inventory/equipment;
10. Windows client package passes the same visual flow used for homologation.

## Relationship to a future Fantasy Native client

This phase is not throwaway work.

The visual language, component behavior, UX rules, information hierarchy and screen specifications should become the reference for the future Fantasy Native client even if the underlying UI implementation changes.

```text
OTClient-backed Fantasy UI
          |
          |  visual/UX specification
          v
Future Fantasy Native Client
```

Do not couple the design specification to OTCv8 implementation details.

## Out of scope for the first pass

- replacing the entire renderer;
- replacing the network protocol;
- rewriting OTClient from zero;
- final monetization/store UI;
- mobile-specific UI;
- console/gamepad-first navigation;
- every future gameplay system before that system actually exists.

## Definition of success

A player should be able to see the client without prior context and perceive it as **Fantasy**, not as a lightly reskinned Tibia/OTClient client, while the proven TFS1098 compatibility and gameplay behavior remain intact.
