# Architecture Decisions

## ADR-001 — Runtime 10.98 is reference, not destination

**Status:** supersedes the previous decision that made TFS 1.4.2 / 10.98 the final runtime.

**Decision:** TFS 1.4.2, RME 3.7 and a 10.98-compatible client remain pinned as reference implementations, behavioral oracles and fallback tools during migration only.

**Reason:** preserve a known-working baseline without forcing the final platform to inherit legacy formats and protocol constraints.

---

## ADR-002 — Fantasy Studio is not based on RME code by default

**Status:** supersedes the previous plan to transform RME directly into the product.

**Decision:** build the Fantasy Studio/Map Engine as our own implementation. RME is used to study expected editing behavior and legacy OTBM interoperability when useful.

**Reason:** allow a native map model, AI-first automation and independent evolution.

---

## ADR-003 — Clean fixed folder layout

**Decision:** maps, content, assets, server, client and shared contracts have single official locations.

**Reason:** prevent scattered/duplicated files and manual searching.

---

## ADR-004 — Relative paths only

**Decision:** persisted project paths are relative to project root.

**Reason:** projects must survive folder moves, another drive, another PC and packaging.

---

## ADR-005 — Studio and game grow together

**Decision:** after the native first-play gate, new editors are built to solve real production needs of the game.

**Reason:** avoid building large unused toolsets.

---

## ADR-006 — Automation is a first-class interface

**Decision:** GUI, scripts and Codex converge on shared domain operations.

**Reason:** AI-assisted creation must use the same validated map/content engine as manual editing.

---

## ADR-007 — External references are pinned and isolated

**Decision:** external repositories used as references are pinned to exact SHAs under local `.upstream/` clones and are not silently copied into product folders.

**Reason:** reproducibility, license clarity and clean-room boundaries.

---

## ADR-008 — FMAP is the native map source

**Decision:** the Fantasy map source of truth is FMAP, initially represented as versioned structured JSON and later optionally compiled to FMAPC for runtime efficiency.

**Reason:** make maps semantic, chunkable, diff-friendly and much easier for AI to create and modify.

**Consequence:** OTBM becomes legacy import/export/reference only.

---

## ADR-009 — Fantasy Server is a native server

**Decision:** implement Fantasy Server as our own authoritative runtime instead of making a deep fork of TFS the permanent core.

**Reason:** own the map loader, entities, protocol, data model and future gameplay architecture.

**Consequence:** TFS remains a behavioral oracle/fallback during development.

---

## ADR-010 — Fantasy Protocol replaces 10.98

**Decision:** create a versioned Fantasy Protocol shared by Fantasy Server and Fantasy Client.

**Reason:** remove historical protocol limitations and support native map chunks, custom entities, effects, systems and future features without legacy workarounds.

**Migration:** a 10.98 adapter may exist temporarily for comparison; it is not part of the final path.

---

## ADR-011 — Shared contracts are canonical

**Decision:** protocol, FMAP schemas and common data definitions live under `Shared/`.

**Reason:** Server, Client, Studio and Codex must not maintain divergent copies of the same contracts.

---

## ADR-012 — Fantasy Studio visual stack uses SDL3 + SDL_GPU + Dear ImGui

**Decision:** the F03 visual editor is built on SDL3 for platform/window/input, SDL_GPU for map/editor rendering, and Dear ImGui for editor panels and tooling UI.

Pinned foundation at decision time:

- SDL `release-3.4.18`, commit `829a65d769d935c4852f8159e964312c0957260a`;
- Dear ImGui `v1.92.9b`, commit `f1cc2ae15e53a861a874c3034aae6798fde194ab`.

**Reason:** both stacks are C/C++ friendly, cross-platform, permissively licensed, work well with C++20, and Dear ImGui ships maintained SDL3 + SDL_GPU backends. SDL_GPU also avoids coupling the map viewport to a legacy widget toolkit or to SDL_Renderer's more limited graphics path.

**Boundary:** SDL/ImGui are visual/platform dependencies only. `MapDocument`, FMAP, project logic and automation remain independent from the GUI. The visual editor may be replaced later without changing the native map data model.

**Dependency policy:** dependencies are fetched/pinned by build configuration or isolated under external dependency folders; their source is not copied into the Fantasy core and their license notices remain preserved.
