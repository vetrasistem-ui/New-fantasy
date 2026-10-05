# Codex Task — F00 Independent Core Foundation

## Goal

Prove the first native Fantasy contracts without using OTBM, TFS or protocol 10.98 in the product path.

## Read first

- `AGENTS.md`
- `docs/ARCHITECTURE.md`
- `docs/DECISIONS.md`
- `docs/F00-HOMOLOGATION.md`
- `docs/UPSTREAMS.md`

## Hard constraints

- Do not copy TFS, RME or OTClient source into product folders.
- Do not make OTBM the source of truth.
- Do not make protocol 10.98 the native protocol.
- Do not use absolute project paths.
- Keep F00 small: contracts + server skeleton + validation only.

## Procedure

1. Run `scripts/check-layout.ps1`.
2. Validate `fantasy.project.json` and `Game/Maps/World/world.fmap.json`.
3. Review `Shared/Formats/FMAP/schema-v0.json`; add deterministic validation tests without expanding scope unnecessarily.
4. Review `Shared/Protocol/protocol-v1.yaml`; ensure message IDs and field names are unique/consistent.
5. Configure and build the native server:
   - `cmake -S Server -B build/server`
   - `cmake --build build/server --config Release`
6. Run:
   - `ctest --test-dir build/server -C Release --output-on-failure`
7. Add the smallest useful F00 tests for FMAP/header and protocol contract consistency.
8. Do not implement gameplay, production networking, database or UI in F00.

## Optional reference work

`scripts/bootstrap-upstreams.ps1` may be used to obtain pinned TFS/RME/OTClient references under `.upstream/`. They are comparison material only.

## Evidence to commit

Create `docs/evidence/F00/RESULT.md` with:

- date;
- OS/toolchain versions;
- layout validation result;
- FMAP validation result;
- protocol contract validation result;
- server configure/build result;
- smoke-test result;
- exact commands;
- blockers/known limitations.

F00 is PASS only when every native foundation gate is verified.
