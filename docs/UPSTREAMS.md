# Upstreams — References and approved dependencies

Este documento registra projetos externos usados como referência técnica, comparação de comportamento, fallback de desenvolvimento ou dependência explicitamente aprovada por ADR.

## TFS reference

- Repository: `otland/forgottenserver`
- Release: `v1.4.2`
- Commit: `31d6e85de2a86fb3f0e36c63509fba75b855b8bd`
- Protocol family: `10.98`
- Role: server behavior oracle / fallback
- Status: **REFERENCE ONLY**

## RME reference

- Repository: `hampusborgos/rme`
- Release: `v3.7`
- Commit: `6aceb3c6a311e6e1c0b24a0bf06cf383fb152766`
- Legacy map family: 10.98 / OTBM v3
- Role: map-editing behavior oracle and future legacy import/export validation
- Status: **REFERENCE ONLY**

## Client reference

- Repository: `opentibiabr/otclient`
- Commit: `396f0b396741bdd4469f27cf9376103930712cff`
- Known compatibility: TFS 1.4.2 / 10.98
- Role: client/protocol behavior oracle and temporary fallback
- Status: **REFERENCE ONLY**

## SDL3 — approved visual/platform dependency

- Repository: `libsdl-org/SDL`
- Release: `release-3.4.18`
- Commit: `829a65d769d935c4852f8159e964312c0957260a`
- License: zlib
- Role: window/input/platform layer and SDL_GPU graphics abstraction for Fantasy Studio and the F05 native visual Client
- Status: **APPROVED DEPENDENCY — ADR-012 / ADR-017**

## Dear ImGui — approved tooling/first-play UI dependency

- Repository: `ocornut/imgui`
- Release: `v1.92.9b`
- Commit: `f1cc2ae15e53a861a874c3034aae6798fde194ab`
- License: MIT
- Role: Studio editor panels and the F05 diagnostic/first-play Client UI using official SDL3 + SDL_GPU backends
- Status: **APPROVED DEPENDENCY — ADR-012 / ADR-017**

## Boundary rule

Reference-only clones may live locally under `.upstream/` and remain ignored by Git.

Approved dependencies are pinned explicitly and may be consumed by build tooling, but their source is not silently copied into the Fantasy core. License notices must remain preserved according to their licenses.

Do not copy source from reference-only repositories into `Studio/`, `Server/`, `Client/` or `Shared/` unless a later ADR explicitly approves that dependency and its licensing obligations.

The native gameplay path remains:

```text
FMAP → Fantasy Server → Fantasy Protocol → Fantasy Client
```

Visual/platform dependencies do not define the map/server/protocol data model.

## Update rule

Changing a pinned reference/dependency SHA requires:

1. documented reason;
2. comparison/build rerun if relevant;
3. update to this file;
4. ADR update if the change affects architecture rather than routine dependency maintenance.
