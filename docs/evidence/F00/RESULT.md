# F00 Evidence — Native Foundation

Status: **PASS**

## Automated evidence

GitHub Actions workflow: `Foundation Checks`

Validated on Windows runner in workflow run 29:

```text
Checkout                              PASS
Validate project layout and contracts PASS
Validate FMAP v0                      PASS
Validate Fantasy Protocol v1          PASS
Record toolchain                       PASS
Configure Fantasy Server              PASS
Build Fantasy Server                  PASS
Smoke-test Fantasy Server             PASS
```

The run completed successfully for commit `fb6eb763b25440f4c520a4f2d701365fa511e684`.

## Contracts frozen for F00

- `Game/Maps/World/world.fmap.json`
- `Shared/Formats/FMAP/schema-v0.json`
- `Shared/Protocol/protocol-v1.yaml`
- `fantasy.project.json` schemaVersion 2
- `Server/CMakeLists.txt`
- `Server/Core/main.cpp`

## Guarantees proven

- project layout and persisted paths are relative;
- the native path uses FMAP rather than OTBM;
- the native protocol contract is Fantasy Protocol v1 draft rather than 10.98;
- FMAP fixture passes structural and semantic validation;
- protocol message IDs/names/types/directions pass automated consistency checks;
- Fantasy Server configures, builds and smoke-tests as C++20 on the Windows CI runner;
- TFS, RME and OTClient are not linked into the native build path.

## Known limitations

See `docs/F00-LIMITATIONS.md`.

F00 does not prove networking, a visual editor, runtime FMAP loading or gameplay. Those belong to subsequent phases.
