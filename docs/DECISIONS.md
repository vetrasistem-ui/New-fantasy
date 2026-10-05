# Architecture Decisions

## ADR-001 — One official runtime family

**Decision:** New Fantasy 1.0 targets TFS 1.4.2 / protocol 10.98 only.

**Reason:** reduce compatibility matrix, simplify Studio adapters, client, assets, maps and testing.

**Consequence:** Crystal, Canary and 15.x are out of scope for this repository unless a future major architecture decision changes this explicitly.

---

## ADR-002 — RME becomes the Studio foundation

**Decision:** start from a Remere's Map Editor codebase compatible with 10.98 and progressively transform it into Fantasy Studio instead of embedding a separate editor executable.

**Reason:** maps are the most complex editor domain and already have a mature engine, undo/redo, brushes, houses, spawns and OTBM I/O.

**Consequence:** Fantasy-specific features should be layered cleanly around the map core whenever possible.

---

## ADR-003 — Clean fixed folder layout

**Decision:** maps, content, assets, server and client have single official locations.

**Reason:** prevent the previous project's scattered/duplicated files and manual searching.

**Consequence:** every Studio tool has a default path and must not create alternative storage locations without a new architecture decision.

---

## ADR-004 — Relative paths only

**Decision:** persisted project paths are relative to project root.

**Reason:** projects must survive folder moves, another drive, another PC and packaging.

---

## ADR-005 — Studio and game grow together after F05

**Decision:** do not build the entire Studio before game development starts.

**Reason:** new editors should solve real game-production needs and be validated by actual content.

---

## ADR-006 — Automation is a first-class interface

**Decision:** GUI, scripts and Codex should converge on shared core operations rather than duplicating logic.

**Reason:** enables AI-assisted map/content creation while preserving deterministic validation and undo.

---

## ADR-007 — Upstream code is pinned and reviewed

**Decision:** external bases are pinned to exact SHAs and cannot be silently updated.

**Reason:** reproducibility and compatibility.

**Consequence:** incorporation/distribution also requires license review and documentation of obligations.
