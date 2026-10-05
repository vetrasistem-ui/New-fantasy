# F00 Evidence — Native Foundation

Status: **IN_PROGRESS / PARTIAL PASS**

## Automated evidence

GitHub Actions workflow: `Foundation Checks`

Validated on Windows runner:

- project layout and relative-path contract: **PASS**;
- native FMAP fixture header (`format=FMAP`, `version=0`): **PASS**;
- Fantasy Server CMake configure: **PASS**;
- Fantasy Server C++20 build: **PASS**;
- `fantasy-server --smoke-test` through CTest: **PASS**.

Observed successful steps in workflow run 24:

```text
Checkout                              PASS
Validate project layout and contracts PASS
Configure Fantasy Server              PASS
Build Fantasy Server                  PASS
Smoke-test Fantasy Server             PASS
```

## Contracts present

- `Game/Maps/World/world.fmap.json`
- `Shared/Formats/FMAP/schema-v0.json`
- `Shared/Protocol/protocol-v1.yaml`
- `Server/CMakeLists.txt`
- `Server/Core/main.cpp`

## Still required before F00 PASS

- validate the full FMAP fixture against the JSON Schema, not only its header;
- add automated consistency checks for Fantasy Protocol message IDs/names/types;
- freeze the F00 toolchain/version evidence;
- record known limitations of FMAP v0 and Fantasy Protocol v1 draft.

No OTBM, TFS code or protocol 10.98 is required by the native F00 build path.
