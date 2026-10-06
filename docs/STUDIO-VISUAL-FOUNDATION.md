# Fantasy Studio — Visual Foundation V5

Status: **V5 IMPLEMENTED / WINDOWS TECHNICAL VALIDATION PASS / OWNER ACCEPTANCE PENDING**

Branch: `feature/studio-visual-foundation`

Initial V5 SHA: `b534ab9a9faf61cda9afc0fcd7189a198fc64d12`.

## Owner decisions and target

On `2026-10-06`, the owner rejected the V4 appearance: **V4 TECHNICAL PASS /
VISUAL FAIL**. Its [original result](evidence/STUDIO-VISUAL/RESULT-V4.md) and
screenshots remain historical evidence. Technical success did not freeze the
V4 shell or approve its appearance. All six tracked V4 PNGs remain unchanged
against Git HEAD.

The owner also rejected the first V5 composition as still too far from the
approved images. Its six real captures are preserved in
[V5-FIRST-PASS](evidence/STUDIO-VISUAL/V5-FIRST-PASS/README.md). They predate
the dark native frame and the revised window, Home and Assets proportions,
and do not constitute current visual acceptance evidence.

The refined V5 targets immediate, strong correspondence with the
[approved styleboard](design/STUDIO-VISUAL-V4/approved-styleboard.png), including
its Home, Map and Items & Assets compositions. The styleboard and
[tokens](design/STUDIO-VISUAL-V4/DESIGN_TOKENS.json) remain design references,
never application textures. Palette alone is insufficient: identity,
proportions, controls, panel hierarchy and first impression must also match.
Neither the refinement nor an agent comparison declares the result identical
or owner-approved.

The renderer remains **SDL3 + SDL_Renderer3 + Dear ImGui**. MapDocument,
EditorOperations, FMAP, Server, Protocol and Client core remain unchanged.
**F05.5, F06 and OTBM/DAT/SPR/OTB integration are NOT STARTED** by this revision.

## Refined V5 shell composition

`Studio/CMakeLists.txt` builds `Studio/UI/StudioAppV5.cpp`. Earlier shells remain
historical implementations rather than the current visual target.

- **Identity:** original presentation art in `Studio/UI/Assets/` supplies a
  faceted fantasy emblem and illustrated hero. The artwork belongs to the
  Studio's branding surface; it is not map content, a sprite catalog or a
  depiction of loaded game assets. No styleboard screenshot, crop or control
  panel is used as a runtime texture. See [art provenance](design/STUDIO-VISUAL-V5/ART-PROVENANCE.md).
- **Typography:** Cinzel supplies the display branding under the bundled
  [SIL Open Font License 1.1](../Studio/UI/Assets/Fonts/OFL-Cinzel.txt). The UI
  remains Segoe UI, Arial or DejaVu Sans from the system, with neutral semibold
  headings. No proprietary system font is redistributed. Presentation assets,
  Cinzel and its license are copied beside the executable and resolved using
  relative resource paths; project contracts do not persist local font paths.
- **Topbar:** 46 px command surface with compact identity, Save/Undo/Redo and
  a separate right runtime group. Blue gradients, metallic highlights and
  restrained borders follow the reference. Undo/Redo use real document history;
  reserved commands do not pretend to execute new features.
- **Sidebar:** Home/Map and other full navigation pages use 188 px
  (164 px below 1150 px), with filled blue/silver icons and a strong blue/cyan
  active state. Items & Assets uses a compact 56 px icon rail. The
  CREATE/RUNTIME headings are removed; all ten destinations remain available.
- **Windows frame:** the native titlebar adopts navy and light text using
  Windows 11 DWM appearance attributes. Native close/resize remain available;
  older systems keep their platform frame when unsupported. The refined
  navy frame is confirmed in all six refined captures.
- **Home:** the practical composition uses a 140 px branded hero and three
  equal 138 px project-action cards with centered titles and two-line body
  text. Larger Home workspaces use a 182 px hero. Compact real recent-project
  rows and structured project information remain below; the actual FMAP
  preview belongs to the information panel. No project, date, content or
  progress is invented to fill space. Existing project-dialog/import
  limitations remain explicit without dominating the shell. Both practical
  Home captures show the canonical four-object world and the complete
  hero/cards/lower-panel composition without critical clipping. The final
  hero has no visible vertical seam.
- **Map:** compact icon-first toolbar, dominant canvas, a cohesive right
  Inspector/Minimap column and a quieter Console spanning the whole workspace
  width. Initial map framing fits the actual bounds, including an off-center
  spawn. Semantic terrain colors stay consistent across canvas, previews and
  Minimap. Reserved Zone/Path/Event/Config controls are disabled and introduce
  no domain feature. These are presentation changes, not final sprites or
  changes to map data.
- **Items & Assets:** a 56 px navigation rail, tabs, search, a 156 px library
  tree and a compact grid of square semantic previews follow the approved
  library composition. The extra Selection panel and SDK footer are removed;
  selected cells and the real key below the library retain selection feedback.
  Search matches semantic keys and friendly labels case-insensitively.
  Categories and selection consume the current FMAP rather than invented
  catalog entries.
- **Future modules:** reserved shells remain honest and use the same visual
  language, without simulated gameplay or phase progress.

Select, Paint, Fill, Add Object, Remove Object, Erase, Undo, Redo, Save and
Reopen continue to use the existing shared editor operations. Global keyboard
Undo/Redo is guarded while a text input is focused. Rebuilding the presentation
does not authorize a second map model or new core features.

## Practical window behavior

The preferred client area remains 1440×900. Initial size now uses a uniform
fit within the display's usable area, preserving the preferred aspect ratio
rather than independently limiting width and height. All six refined
captures confirm a 1049×656 main client area (1051×688 with native frame)
and a 960×656 smaller client area (962×688 with frame). Home's complete
composition, the Map controls and all ten catalog keys remain visible
without critical clipping at both practical sizes.

Critical controls must remain reachable; narrower panels may scroll without
covering the canvas. Fixed shell geometry does not persist an `imgui.ini`
cache at the project root.

The current Windows display is 1366×768. Exact 1440×900 inspection cannot be
claimed from preferred code dimensions; the result must record the window
sizes actually observed and identify any unobserved resolution.

## Validation and owner gate

See [the V5 Windows result and capture status](evidence/STUDIO-VISUAL/RESULT.md)
and [the validation procedure](CODEX-STUDIO-VISUAL-VALIDATION.md).

The existing GUI regression evidence includes all navigation destinations,
shared Map operations, semantic Save/Reopen, case-insensitive `CARVALHO` and
uppercase accented `ÁGUA` search, all four tabs, selected-key feedback and
the focused-input keyboard Undo guard.

The final `scripts/build-native.ps1 -Configuration Release` run passed:
Studio 5/5, Server 6/6 and Client 2/2, for 13/13 CTests. Shared contracts,
layout/project/FMAP/multi-chunk/protocol validators and project relocation
passed. No compiler/CMake warnings or errors occurred. Final
`Studio/UI/StudioAppV5.cpp` Git blob:
`f46cf1d281887432ec1fe11b2af336b7ca134001`.
The refined runtime reported SDL_Renderer3 `direct3d11`, Segoe UI and Cinzel.

Canonical FMAP restoration passed with Git blob
`1442d649ff73ba39881f876a4bde33394df2784c`; `Game/` has no diff. The refined
launcher closed normally with exit code 0, and zero Fantasy processes were
found before deliberately reopening Home for review. All six Home/Map/Assets
screenshots are recorded and inspected. Studio remains intentionally open on
Home for owner review; no unexpected Fantasy processes were observed.
Repository whitespace and staged-path checks passed; only intended visual
source, packaging, licensed presentation resources, documentation and real
evidence belong to this delivery. The delivery response records the final
commit SHA, push outcome and post-commit working-tree status.

Required current screenshots are `home-v5.png`, `map-v5.png` and
`items-assets-v5.png` in `docs/evidence/STUDIO-VISUAL/`, plus their
`-small.png` counterparts for practical sizing. First-pass V5 captures
remain under `V5-FIRST-PASS/` and must not be mistaken for the refined result.

The technical gate passed Release build/tests, complete navigation, Map
operations, semantic Save/Reopen, practical sizing, normal close/cleanup and
real screenshots. Direct comparison finds the refined Home composition closer
to the approved image than the first V5 pass, particularly at the smaller
size. Original branding/artwork, one real recent project, the small semantic
FMAP preview and the ten-key library still differ from the supplied images.
Build success and an agent's improvement assessment do not confer visual PASS.
**Only the owner can accept the refined V5 appearance and freeze the visual
foundation.** This delivery does not begin legacy integration or persistence.
