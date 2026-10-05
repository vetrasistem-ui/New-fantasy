# Codex Task — F00 Compatibility Homologation

## Goal

Prove the pinned 10.98 stack end-to-end before importing/modifying upstream code as Fantasy.

## Read first

- `AGENTS.md`
- `docs/UPSTREAMS.md`
- `docs/F00-HOMOLOGATION.md`
- `docs/ARCHITECTURE.md`

## Hard constraints

- Do not change pinned SHAs just to make the test pass.
- Do not introduce Crystal, Canary or 15.x assets/protocol.
- Do not redesign the UI during F00.
- Do not copy third-party source into tracked product folders before license review is recorded.
- Do not use absolute project paths in committed files.

## Procedure

1. Run `scripts/check-layout.ps1`.
2. Run `scripts/bootstrap-upstreams.ps1`.
3. Record environment/toolchain versions.
4. Build TFS candidate unmodified.
5. Prepare local MariaDB and import the upstream schema.
6. Build RME candidate unmodified and validate a 10.98 OTBM fixture: Open -> Edit -> Undo -> Redo -> Save As -> Reopen.
7. Build OTClient candidate unmodified and configure it for the TFS 10.98 test endpoint.
8. Connect, select a test character and enter the game.
9. Modify one visible map tile/object in RME, save, restart runtime if required, enter again and visually confirm the change.
10. Move the test workspace root and rerun the relevant open/build/play path without editing committed absolute paths.

## Evidence to commit

Create `docs/evidence/F00/RESULT.md` containing:

- date;
- OS/toolchain versions;
- exact upstream SHAs;
- exact commands used;
- build result for each component;
- database result;
- RME roundtrip result;
- login/play result;
- end-to-end map visibility result;
- move-folder result;
- blockers and exact error output references.

Do not mark F00 PASS if any end-to-end gate is unverified.
