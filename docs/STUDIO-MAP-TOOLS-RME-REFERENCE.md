# Fantasy Studio — RME Map Tools Reference

Date: `2026-10-06`

Status: **REFERENCE INVENTORY COMPLETE / FANTASY IMPLEMENTATION NOT CLAIMED**

This document records the owner's interactive evaluation of the supplied RME package. It is a behavioral reference only. No RME binary/source/assets are incorporated into Fantasy.

## Confirmed reference behaviors

| Tool / behavior | Result | Fantasy takeaway |
| --- | --- | --- |
| Terrain palette | CONFIRMED | Named material selection should drive brush intent |
| Square/circle brush + size preview | CONFIRMED | Preview must match the exact footprint used by mutation |
| Terrain stamping / automatic borders | CONFIRMED AS REFERENCE | Border generation belongs to explicit asset/brush rules; not yet implemented in Fantasy |
| Undo / Redo | CONFIRMED | One meaningful editor operation/gesture should be one history entry |
| Ground picker | CONFIRMED | Picking/selection stays separate from mutating tools |
| Single tile/item selection | CONFIRMED | Inspector/picker may identify the item under cursor |
| Single selected item Copy/Paste | CONFIRMED | Does not prove whole-tile/area clipboard semantics |
| Object palette / placement | CONFIRMED | Palette → preview → viewport placement is a useful loop |
| Item properties / field stack | CONFIRMED READ-ONLY | Inspector should expose semantic/legacy metadata without forcing mutation |
| Grid | CONFIRMED | View-only control |
| Zoom | CONFIRMED | View-only control, reset supported |
| Floors / lower-floor reference | CONFIRMED | Integrated floor navigation is required for real map editing |
| Go to XYZ | CONFIRMED | Direct coordinate navigation is valuable on large maps |
| Minimap navigation | CONFIRMED | Minimap click should reposition viewport |

## Still pending / not accepted from the RME session

- rectangle / multi-tile selection;
- whole-area clipboard;
- Cut;
- area Erase;
- Save/Reopen;
- houses/creatures/spawns/import workflows.

Do not treat those behaviors as proven requirements merely because related menu entries exist.

## Strongest transferable editing loop

```text
choose material/tool
      ↓
see actual footprint preview
      ↓
apply directly in viewport
      ↓
one history operation per gesture
      ↓
Undo / Redo
```

## Fantasy contracts

- Brush geometry belongs to the Fantasy editor-operation layer.
- Preview and mutation must query the same footprint calculation.
- Selection is transient editor intent, not a second map document.
- View controls (grid, zoom, floors, XYZ, Minimap) must not mutate content.
- Clipboard payloads, when implemented, should use neutral semantic tile snapshots rather than raw UI state.
- Cut/Paste need atomic transaction semantics and exact Undo/Redo.
- Automatic borders depend on Fantasy asset/brush rules; observing them in RME does not authorize a legacy runtime dependency.

## Relationship to F05.5

The RME study is now closed as a reference task. F05.5 should first open the real PokeFans 10.98 OTBM with real DAT/SPR/OTB rendering. The native editing tools should then be implemented/tested on that real surface rather than optimized against placeholder colored tiles.
