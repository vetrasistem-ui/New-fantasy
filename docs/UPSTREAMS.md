# Upstreams — Reference implementations

Este documento registra projetos externos usados apenas como referência técnica, comparação de comportamento e fallback de desenvolvimento. Eles **não são a arquitetura final** do New Fantasy.

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

## Boundary rule

Reference clones live locally under `.upstream/` and remain ignored by Git.

Do not copy source from these repositories into `Studio/`, `Server/`, `Client/` or `Shared/` unless a later ADR explicitly approves that dependency and its licensing obligations.

The native path remains:

```text
FMAP → Fantasy Server → Fantasy Protocol → Fantasy Client
```

## Update rule

Changing a pinned reference SHA requires:

1. documented reason;
2. comparison rerun if relevant;
3. update to this file;
4. ADR only if the change affects architecture rather than reference evidence.
