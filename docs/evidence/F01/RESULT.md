# F01 Evidence — Project System

Status: **PASS**

## Automated evidence

Workflow run 46 proved the full F01 runtime path on Windows:

- project layout: PASS;
- Fantasy Project v2 manifest validation: PASS;
- FMAP v0 validation: PASS;
- Fantasy Protocol v1 validation: PASS;
- relocation to a different temporary root: PASS;
- Fantasy Studio configure/build: PASS;
- Fantasy Studio Project Manager tests: PASS;
- Fantasy Server configure/build/smoke: PASS.

## Project Manager delivered

Implemented in `Studio/Project/ProjectManager.*` and exposed by `fantasy-studio`:

- `New Project` creates the canonical project layout and starter FMAP;
- `Open Project` loads `fantasy.project.json` and resolves only safe relative paths;
- `Open Main Map` resolves the canonical map from the manifest without file search;
- `Recent Projects` stores workspace-relative entries only, with deduplication and a bounded list;
- project creation copies the versioned FMAP schema and Fantasy Protocol specification from the template root;
- reopening after moving the project to a different directory is covered by tests.

## Runtime commands

```text
fantasy-studio --open-project <project-root-or-manifest>
fantasy-studio --open-main-map <project-root-or-manifest>
fantasy-studio --new-project <target-root> <name> <template-root>
fantasy-studio --remember-project <workspace-root> <project-root>
fantasy-studio --recent-projects <workspace-root>
```

## F01 gate result

All required F01 gates are closed. Project paths remain relative to the project root and no arbitrary map search is required.

Next phase: **F02 — Fantasy Map Core**.
