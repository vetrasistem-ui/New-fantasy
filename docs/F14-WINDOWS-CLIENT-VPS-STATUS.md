# F14 — Windows Client Packaging and VPS Deployment Status

**Status:** PARTIAL PASS — packaging/runtime contract complete; interactive Windows visual acceptance and real remote-VPS smoke remain pending.

## Windows OTCv8 package — PASS

The Fantasy homologation workflow now stages a clean public OTCv8 Windows package through the reusable tool:

```text
Tools/Tfs1098/package_otcv8_windows_client.ps1
```

The package includes the public client runtime needed for local acceptance:

```text
otclient_dx.exe
otclient_gl.exe
d3dcompiler_47.dll
libEGL.dll
libGLESv2.dll
init.lua
modules/
mods/
data/
layouts/
fantasy-client-manifest.json
```

The CI artifact intentionally excludes Tibia DAT/SPR data. User-owned matching 10.98 assets can be injected locally by running the packager with `-AssetsRoot`.

The generated manifest records:

```text
client=otcv8
protocol=1098
platform=windows
preferredExecutable=otclient_dx.exe
alternateExecutable=otclient_gl.exe
assetsIncluded=false   # CI/public artifact
```

The workflow gate is:

```text
.github/workflows/otcv8-visual-homologation.yml
```

Current verified jobs:

```text
stage-linux-client              PASS
stage-windows-client            PASS
windows-tfs-protocol-smoke      PASS
```

The Windows TFS smoke uses the official TFS 1.4.2 runtime and Fantasy's 10.98 homologation tooling. It proves the Windows runtime/protocol boundary independently from GUI rendering.

## Windows visual acceptance — PENDING

A GitHub-hosted Windows runner does not provide the same reliable interactive desktop acceptance path as a normal user session. Therefore these PASS results must **not** be described as a full Windows graphical acceptance.

The remaining Windows client gate is:

```text
Windows desktop session
    -> packaged OTCv8 client
    -> user-owned 10.98 DAT/SPR
    -> Fantasy/TFS1098 runtime
    -> login
    -> character list
    -> game entry
    -> rendered map screenshot
```

Linux graphical OTCv8 acceptance is already PASS and documented separately.

## VPS deployment bundle — PASS (builder/contract)

Fantasy already owns the C++ deployment builder:

```text
Studio/Runtime/Tfs1098DeploymentBundle.hpp
```

A cross-platform operational command is now also available:

```text
Tools/Tfs1098/build_vps_bundle.py
```

It converts an already-packaged TFS1098 runtime into:

```text
bundle/
├── server/
│   ├── tfs
│   ├── config.lua
│   ├── data/
│   └── ...
└── deploy/
    ├── fantasy-tfs1098.service
    ├── install.sh
    ├── README.md
    └── manifest.json
```

The command deliberately does **not** provision or embed production secrets.

Validation workflow:

```text
.github/workflows/tfs1098-vps-bundle-tool.yml
```

Verified matrix:

```text
Ubuntu:   build bundle                          PASS
Ubuntu:   reject unsafe output containment      PASS
Windows:  build bundle                          PASS
Windows:  reject unsafe output containment      PASS
```

The tests also verify that the source runtime is left intact, the systemd service contains restart/NOFILE policy, and `install.sh` restores executable permission before starting the service.

## Remaining real deployment gate

A local/CI bundle is not the same as a production deployment. The remaining VPS proof requires an actual Linux target with credentials:

```text
copy bundle to VPS
    -> MariaDB configured
    -> production config.lua reviewed
    -> sudo bash deploy/install.sh
    -> systemd service active
    -> public login port reachable
    -> client login
    -> game entry
    -> service restart
    -> relogin/persistence
```

That remote smoke must be recorded separately and should not be inferred from the current CI PASS.

## Gate summary

```text
OTCv8 Linux graphical acceptance            PASS
OTCv8 Windows clean package                 PASS
TFS1098 Windows protocol/runtime smoke      PASS
OTCv8 Windows interactive graphical test    PENDING
VPS deployment bundle contract (Linux)      PASS
VPS deployment bundle contract (Windows)    PASS
Real remote VPS deployment                  PENDING
```

After these two environment-dependent acceptance checks, the project can move cleanly into higher-level Fantasy-owned gameplay systems and later the planned Fantasy Client UI/UX phase.
