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

---

## ADR-013 — Shared owns the neutral FMAP C++ core

**Decision:** the reusable C++ implementation of the FMAP model, validation, JSON IO and `MapDocument` lives in `Shared/Formats/FMAP/` under the neutral namespace `fantasy::fmap`.

**Reason:** Studio and Server must consume the same map implementation instead of maintaining separate parsers, serializers or semantic rules. FMAP is a platform contract shared by the whole Fantasy ecosystem, not a Studio implementation detail.

**Boundary:** Studio-specific editor operations remain in `Studio/MapEngine/`; Server-specific world runtime remains in `Server/Core/`. The Studio may expose a thin compatibility facade over `fantasy::fmap`, but it must not duplicate the FMAP implementation.

**Consequence:** changes to FMAP model/IO/validation are made once in `Shared/` and are validated against both Studio and Server builds/tests.

---

## ADR-014 — Fantasy Protocol v1 uses framed TCP and intent-only client commands

**Decision:** Fantasy Protocol v1 runs over TCP with a fixed 16-byte little-endian envelope: `FNTY` magic, protocol version, message type, payload length and sequence. Payload length excludes the envelope, payloads are capped at 4 MiB, and variable-length fields use a `uint32` length prefix.

**Authority:** client messages express intent; the Server publishes authoritative state. Movement is therefore split into `MoveRequest(direction)` from Client to Server and `EntityMove(entityId, x, y, z, direction)` from Server to Client. The client does not send an absolute position as authoritative truth.

**Reason:** TCP provides reliable ordered delivery for the initial top-down RPG runtime while explicit framing prevents the code from depending on TCP packet boundaries. Separating intent from state makes the authoritative-server rule enforceable in the protocol itself.

**Boundary:** transport can be revisited in a future protocol version if real gameplay measurements justify it; v1 code must not silently alter framing or authority semantics.

---

## ADR-015 — MapChunk uses FMCP v1 and carries region origin

**Decision:** Fantasy Protocol v1 `MapChunk` carries `regionId`, `regionOriginX/Y`, FMAP region-local `chunkX/Y`, floor, revision and an independently versioned semantic payload named **FMCP v1**.

**FMCP v1:** the payload preserves FMAP tile-local coordinates, semantic ground/object keys and tags. It does not translate map content to Tibia item IDs or another legacy asset model.

**Reason:** FMAP v0 global tile coordinates are resolved as `regionOrigin + chunkOffset + tileLocal`. Sending region origin in the outer message gives the Client enough information to reconstruct exact world coordinates without coupling it to the Server's complete FMAP file.

**Boundary:** FMCP is versioned independently from the outer Fantasy Protocol frame. A later streaming/compression format may supersede it only through an explicit version/migration decision.

---

## ADR-016 — F05 TCP listener is development-only and loopback-only

**Decision:** the first playable TCP path binds `127.0.0.1` only and uses `LoginDev` solely for local development validation.

**Reason:** F05 exists to prove the native Client → Protocol → Server → FMAP loop, not to prematurely define public multiplayer security and operations.

**Security boundary:** this listener must not be exposed on a public VPS/interface. Public networking requires a later gate with real authentication/session tokens, connection and rate limits, timeouts, abuse controls, production logging/metrics and a deliberate transport-encryption decision.

---

## ADR-017 — First visual Fantasy Client reuses the approved SDL3 stack

**Decision:** the first native visual client uses the same pinned SDL3 + SDL_GPU + Dear ImGui dependency family as the Studio for its F05 validation UI.

**Reason:** reuse an already approved/pinned Windows-capable visual foundation and avoid introducing a second throwaway rendering toolkit before the first native-play gate closes.

**Boundary:** Dear ImGui is only the F05 diagnostic/first-play presentation layer. `DevelopmentClient`, Fantasy Protocol, FMCP and gameplay state remain independent from the GUI. Future game UI/rendering may evolve without changing those contracts.

---

## ADR-018 — F06 persistence uses a database-independent Server boundary

**Status:** **PREPARED / activates only after F03 and F05 are formally PASS.**

**Decision:** gameplay/runtime code will depend on a `PersistenceService` / persistence-store contract instead of direct SQL. The first F06 adapter is planned for SQLite because it gives deterministic local Windows development and CI without an external database service.

**Reason:** prove account/character/save-load semantics and restart persistence with the smallest operational surface while keeping SQL and engine-specific types out of `WorldRuntime`, protocol and Client code.

**Boundary:** SQLite is the first adapter, not a permanent scaling commitment. Before website/public-account integration or production scaling, a PostgreSQL-class adapter may be introduced behind the same persistence contract if required by the measured workload.

**Security:** F06 does not define public authentication and must not store plaintext passwords. `LoginDev` remains loopback/development-only.

**Activation gate:** no F06 code is implemented until the Windows interactive F03/F05 closeout, evidence update and exact-SHA CI run are green.
