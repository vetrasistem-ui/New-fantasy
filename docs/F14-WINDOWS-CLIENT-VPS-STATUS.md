# F14 — Windows Client Packaging and VPS Deployment Status

**Status:** CONTRACT PASS — packaging/runtime/deployment lifecycle complete; interactive Windows visual acceptance and real remote-VPS smoke remain external gates.

## Windows OTCv8 package — PASS

The Fantasy homologation workflow stages a clean public OTCv8 Windows package through the reusable tool:

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

Verified jobs:

```text
stage-linux-client              PASS
stage-windows-client            PASS
windows-tfs-protocol-smoke      PASS
```

The Windows TFS smoke uses the official TFS 1.4.2 runtime and Fantasy's 10.98 homologation tooling. It proves the Windows runtime/protocol boundary independently from GUI rendering.

## Windows visual acceptance — EXTERNAL / PENDING

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

## VPS deployment bundle — PASS

Fantasy owns two equivalent deployment entrypoints:

```text
Studio/Runtime/Tfs1098DeploymentBundle.hpp
Tools/Tfs1098/build_vps_bundle.py
```

The V5 Server workspace also exposes the C++ bundle builder through **Build VPS bundle**.

An already-packaged Linux TFS1098 runtime is converted into:

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
    ├── manifest.json
    ├── env.example
    ├── healthcheck.sh
    ├── update.sh
    ├── rollback.sh
    └── release-policy.json
```

The bundle deliberately does **not** provision or embed production secrets.

Validation workflow:

```text
.github/workflows/tfs1098-vps-bundle-tool.yml
```

Verified matrix:

```text
Ubuntu:   build lifecycle bundle                PASS
Ubuntu:   reject unsafe output containment      PASS
Ubuntu:   reject unsafe backup root             PASS
Windows:  build lifecycle bundle                PASS
Windows:  reject unsafe output containment      PASS
Windows:  reject unsafe backup root             PASS
```

The tests verify that:

- the source runtime is left intact;
- systemd uses restart and NOFILE policies;
- `install.sh` restores executable permission before service start;
- `env.example` contains placeholders only;
- `healthcheck.sh` verifies the service and installation;
- `update.sh` creates a backup before replacing the server;
- update executes the health check;
- a failed health check follows the automatic rollback path;
- `rollback.sh` can restore the most recent or an explicitly selected backup;
- `release-policy.json` records backup/rollback policy;
- the manifest advertises health/update/rollback support.

## VPS lifecycle contract

### First install

```text
review private production configuration
    -> sudo bash deploy/install.sh
    -> dedicated system user
    -> runtime under /opt/fantasy/...
    -> systemd enable/start
```

### Health

```text
sudo bash deploy/healthcheck.sh
    -> systemd active
    -> installation directory present
    -> FANTASY_VPS_HEALTH PASS
```

### Update

```text
sudo bash deploy/update.sh
    -> timestamped backup
    -> stop service
    -> stage new server
    -> restart
    -> health check
```

### Automatic rollback

If the update health check fails:

```text
stop failed release
    -> restore timestamped backup
    -> fix ownership
    -> restart previous release
    -> update exits as failure
```

### Explicit rollback

```text
sudo bash deploy/rollback.sh
```

uses the latest backup by default; a selected backup path can also be passed explicitly.

## Remaining real deployment gate — EXTERNAL / PENDING

A CI-generated bundle is not a production deployment. The remaining VPS proof requires an actual Linux target with credentials:

```text
copy bundle to VPS
    -> MariaDB configured
    -> production config.lua reviewed privately
    -> sudo bash deploy/install.sh
    -> systemd service active
    -> public login/game ports reachable
    -> client login
    -> game entry
    -> save/logout
    -> service restart
    -> relogin/persistence
```

Then exercise the real lifecycle:

```text
working release
    -> deploy/update.sh
    -> backup exists
    -> health PASS

controlled bad release
    -> health FAIL
    -> automatic rollback
    -> previous service active
    -> client can log in again
```

That remote smoke must be recorded separately and must not be inferred from CI.

## Gate summary

```text
OTCv8 Linux graphical acceptance             PASS
OTCv8 Windows clean package                  PASS
TFS1098 Windows protocol/runtime smoke       PASS
OTCv8 Windows interactive graphical test     EXTERNAL / PENDING
VPS lifecycle bundle contract (Linux)        PASS
VPS lifecycle bundle contract (Windows)      PASS
V5 Build VPS bundle integration              PASS
Real remote VPS install/login/persistence    EXTERNAL / PENDING
Real remote VPS update/rollback               EXTERNAL / PENDING
```

The exact Codex/local acceptance sequence is frozen in:

```text
docs/CODEX-HANDOFF-FOUNDATION-V2.md
```

Headless architecture and deployment contracts are complete; the remaining items require a real interactive Windows desktop, user-owned real assets, or a real remote Linux VPS.