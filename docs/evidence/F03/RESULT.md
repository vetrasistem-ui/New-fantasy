# F03 Evidence — Fantasy Map Editor MVP

Status: **IN_PROGRESS**

## Visual foundation frozen

ADR-012 selects the native visual stack:

- SDL3 `release-3.4.18` / commit `829a65d769d935c4852f8159e964312c0957260a`;
- SDL_GPU for viewport rendering;
- Dear ImGui `v1.92.9b` / commit `f1cc2ae15e53a861a874c3034aae6798fde194ab`;
- official Dear ImGui SDL3 + SDL_GPU backends.

This stack is deliberately isolated from the map domain. The GUI must edit the existing `MapDocument`; it does not own a second map representation.

## Why SDL_GPU

The pinned Dear ImGui release provides a maintained SDL_GPU renderer backend. The renderer therefore starts on SDL_GPU rather than coupling the viewport to the limited SDL_Renderer path.

## F03 gate plan

1. GUI shell/window starts and shuts down cleanly;
2. open current project and main FMAP through `ProjectManager`;
3. render the selected floor in a 2D viewport;
4. pan/zoom and tile selection;
5. ground/object brush calls `MapDocument` transactions;
6. fill/erase operations are reversible;
7. basic minimap;
8. Save/Reopen remains semantically identical;
9. headless tests continue to prove the same operations without GUI input.

No RME, OTBM or 10.98 code is introduced into the F03 native path.
