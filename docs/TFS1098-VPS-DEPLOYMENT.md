# TFS1098 VPS Deployment

## Goal

Fantasy keeps TFS 1.4.2 / protocol 10.98 as an external runtime. A project is exported by Fantasy, packaged into a TFS runtime, and only then converted into a Linux VPS deployment bundle.

The deployment path does **not** copy TFS implementation types into Fantasy Core.

```text
Fantasy Project / MapDocument
        ↓
OTBM + runtime companions
        ↓
Tfs1098RuntimeBackend::packageProject
        ↓
local packaged TFS runtime
        ↓
Tfs1098DeploymentBundle
        ↓
Linux VPS bundle
```

## Bundle layout

The deployment builder produces:

```text
<bundle>/
├── server/
│   ├── tfs
│   ├── config.lua
│   ├── data/
│   └── ... packaged TFS runtime files
└── deploy/
    ├── fantasy-tfs1098.service
    ├── install.sh
    ├── manifest.json
    └── README.md
```

`server/` is the already-packaged runtime. `deploy/` contains host integration only.

## systemd behavior

The generated service uses:

- a dedicated non-login service user;
- a fixed working directory under `/opt/fantasy/...`;
- `Restart=on-failure`;
- a short restart delay;
- `LimitNOFILE=65535`;
- stdout/stderr through the systemd journal;
- `network-online.target` and MariaDB ordering.

This is intended for a 24/7 VPS runtime rather than for local Studio process control.

## Installation

Before deployment:

1. install and configure MariaDB;
2. import the TFS 1.4.2 schema and allow the official runtime migration to the current supported version;
3. review `server/config.lua`;
4. set production database credentials, public IP and ports;
5. do not commit production credentials;
6. copy the bundle to the VPS.

Then run:

```bash
cd deploy
sudo bash ./install.sh
```

Using `bash` explicitly is intentional: a bundle generated or zipped on Windows may not preserve the Unix executable bit on `install.sh`. During installation the script also applies `chmod 0755` to the staged TFS executable, so a Linux runtime bundle created on a Windows Studio machine remains executable after transfer.

The installer creates the service user when needed, installs the runtime under the configured `/opt/fantasy/...` root, installs the systemd unit, reloads systemd and enables/starts the service.

Runtime logs:

```bash
journalctl -u fantasy-tfs1098 -f
```

## Security boundary

The generated bundle deliberately does not manage production secrets for the user. Database passwords and other sensitive production values remain deployment configuration. Fantasy must never commit or publish those values.

The builder validates service/user identifiers, executable name and install root to prevent unsafe path traversal in generated deployment scripts. It also rejects deployment output paths that are inside the source runtime or contain the source runtime, preventing cleanup of the output directory from deleting runtime files.

## Current gate

The deployment builder is covered by the same multiplatform runtime backend test suite. The test verifies:

- packaged runtime copied to `server/`;
- generated systemd unit;
- dedicated service user and working directory;
- restart policy;
- `install.sh` enabling the service;
- deployment manifest;
- rejection of an unsafe install root;
- rejection of deployment output/runtime containment in either direction;
- source runtime remains intact after rejected unsafe output paths.

A real remote-VPS smoke test remains a separate deployment gate because it requires an actual target machine and credentials.
