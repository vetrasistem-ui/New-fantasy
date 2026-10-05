# F01 Evidence — Project System

Status: **IN_PROGRESS / PARTIAL PASS**

## Automated PASS

Workflow run 39 proved:

- project layout: PASS;
- Fantasy Project v2 manifest validation: PASS;
- FMAP v0 validation: PASS;
- Fantasy Protocol v1 validation: PASS;
- relocation to a different temporary root: PASS;
- Fantasy Server configure/build/smoke: PASS.

The relocation gate copies only tracked project files to a new root and reruns project, FMAP and protocol validation there. This proves that the committed project contracts do not depend on the original repository path.

## Added in F01

- `Shared/Formats/Project/schema-v2.json`
- `scripts/validate-project.ps1`
- `scripts/test-project-relocation.ps1`
- canonical `mainMap`, `protocolSpec` and `mapSchema` path validation

## Remaining before F01 PASS

- implement Project Manager runtime model inside the Studio;
- New Project;
- Open Project;
- Recent Projects;
- Open Main Map command using the manifest rather than file search;
- repeat relocation test through the actual Studio project loader.
