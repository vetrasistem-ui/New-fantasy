# Fantasy Studio — Visual Foundation V4

Status: **V4 IMPLEMENTED / OWNER VISUAL ACCEPTANCE PENDING**

Branch: `feature/studio-visual-foundation`

## Scope and reference

The owner rejected V3's terminal typography and visual distance from the
styleboard. The supplied `Fantasy-Studio-Visual-Package-V4.zip` now guides the
shell rebuild. The [approved styleboard](design/STUDIO-VISUAL-V4/approved-styleboard.png)
and [design tokens](design/STUDIO-VISUAL-V4/DESIGN_TOKENS.json) are documentation
references, never runtime textures.

Initial branch SHA: `c91b926d08ef5fd61dff7c56b7395dff578f501f`.

The renderer remains **SDL3 + SDL_Renderer3 + Dear ImGui**. MapDocument,
EditorOperations, FMAP, Server, Protocol and Client core remain unchanged.
F05.5, F06 and OTBM/DAT/SPR/OTB integration are **NOT STARTED** by this revision.

## V4 shell

`Studio/CMakeLists.txt` builds `Studio/UI/StudioAppV4.cpp`. V3 is preserved in
Git history rather than used as the current visual direction. The existing V2
source remains an unbuilt historical reference.

- **Typography:** neutral system UI fonts (Segoe UI on Windows, with Arial or
  DejaVu Sans fallbacks); system semibold headings; serif branding only. No
  proprietary font file is redistributed and no pixel/terminal UI fallback is
  used. Font paths are resolved at runtime, not stored in project contracts.
- **Identity:** an original faceted F/sword vector mark, serif wordmark and
  layered abstract hero with restrained cyan accents. No supplied screenshot,
  cropped logo, fake sprite or invented map art is embedded in the application.
- **Topbar:** vertically aligned commands with coherent vector icons, secondary
  project/module context and a fixed right action group. Undo/Redo reflect real
  document history. Runtime/settings commands remain visibly reserved.
- **Sidebar:** one consistent icon family for all ten destinations, compact
  rows, quiet inactive states and a cyan rail for the active workspace.
- **Home:** equal Novo/Abrir/Importar cards, compact project information, real
  region/chunk/tile/object counts and `Abrir Map Workspace`. Existing CLI/launcher
  project entry points remain explicit; GUI project dialogs/import are reserved.
  Recent rows and the world preview consume the current FMAP directly.
- **Map:** icon/text tool strip, dominant editing canvas, right Inspector,
  Minimap rendered from actual tiles/objects/spawn, and a quieter lower Console.
  Canvas clipping/grid alignment follow the viewport; paths shown in the Console
  are project-relative. Inspector actions continue to call EditorOperations;
  Undo/Redo are grouped in the topbar and keep their keyboard shortcuts.
- **Items & Assets:** narrow library tree, search and tabs, broad semantic grid,
  readable full keys and cyan selected borders. Swatches are labelled semantic
  references, not final sprite artwork. The legacy source is `NOT CONNECTED`.
- **Future modules:** honest shells in the same visual language, without fake
  progress percentages, catalog contents or gameplay functionality.

## Practical window behavior

The preferred client area is 1440x900. Initial size is bounded by the display's
usable area so titlebar and commands remain reachable. The practical minimum is
960x640, also bounded by the display. Width-sensitive panels preserve their
separate bounds; long secondary text wraps or clips within its own region.
Fixed shell geometry does not load/save an `imgui.ini` cache at the project root.

This Windows machine has a 1366x768 display. The exact 1440x900 visual approval
requires a display that can show that client area completely; it must not be
reported as observed merely because the layout supports the preferred size.

## Validation and owner gate

See [the Windows result and real screenshots](evidence/STUDIO-VISUAL/RESULT.md)
and [the validation procedure](CODEX-STUDIO-VISUAL-VALIDATION.md).

The gate requires Release build/tests, all sidebar destinations, Map operation
regression, semantic Save/Reopen, responsive window checks, normal close/cleanup
and real screenshots. Technical checks do not substitute for appearance approval.

**Only the owner can accept the V4 appearance and freeze this foundation.**
No legacy integration or persistence work begins as part of this delivery.
