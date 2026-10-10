#!/usr/bin/env python3
"""Build a deployment bundle from an already-packaged Fantasy TFS1098 runtime.

This is an operational companion to Studio/Runtime/Tfs1098DeploymentBundle.hpp.
It intentionally does not provision production secrets or mutate production DB values.
"""

from __future__ import annotations

import argparse
import json
import re
import shutil
import stat
from pathlib import Path, PurePosixPath
from typing import NoReturn

_IDENTIFIER = re.compile(r"^[A-Za-z0-9_-]+$")


def fail(message: str) -> NoReturn:
    raise SystemExit(f"error: {message}")


def validate_identifier(value: str, field: str) -> None:
    if not value or not _IDENTIFIER.fullmatch(value):
        fail(f"{field} must contain only letters, digits, '_' or '-'")


def validate_filename(value: str) -> None:
    if not value or value in {".", ".."} or "/" in value or "\\" in value:
        fail("executable name must be a single safe filename")


def validate_install_root(value: str, field: str = "install root") -> None:
    path = PurePosixPath(value)
    if not value.startswith("/") or value == "/" or ".." in path.parts:
        fail(f"{field} must be an absolute Unix path without '..' and cannot be '/'")
    if not all(ch.isalnum() or ch in "/_.-" for ch in value):
        fail(f"{field} contains unsupported characters")


def is_within(candidate: Path, parent: Path) -> bool:
    try:
        candidate.relative_to(parent)
        return True
    except ValueError:
        return False


def write_text(path: Path, text: str) -> None:
    path.parent.mkdir(parents=True, exist_ok=True)
    path.write_text(text, encoding="utf-8", newline="\n")


def make_service(service: str, user: str, install_root: str, executable: str) -> str:
    server_root = f"{install_root}/server"
    return f"""[Unit]
Description=Fantasy TFS 1.4.2 / 10.98 runtime
Wants=network-online.target
After=network-online.target mariadb.service

[Service]
Type=simple
User={user}
Group={user}
WorkingDirectory={server_root}
ExecStart={server_root}/{executable}
Restart=on-failure
RestartSec=5
LimitNOFILE=65535
StandardOutput=journal
StandardError=journal

[Install]
WantedBy=multi-user.target
"""


def make_installer(service: str, user: str, install_root: str, executable: str) -> str:
    return f"""#!/usr/bin/env bash
set -euo pipefail

if [ "${{EUID}}" -ne 0 ]; then
  echo "Run this installer as root (sudo)." >&2
  exit 1
fi

SERVICE_NAME='{service}'
SERVICE_USER='{user}'
INSTALL_ROOT='{install_root}'
TFS_EXECUTABLE='{executable}'
SCRIPT_DIR="$(cd "$(dirname "${{BASH_SOURCE[0]}}")" && pwd)"
BUNDLE_ROOT="$(cd "${{SCRIPT_DIR}}/.." && pwd)"

if ! id "${{SERVICE_USER}}" >/dev/null 2>&1; then
  useradd --system --home "${{INSTALL_ROOT}}" --shell /usr/sbin/nologin "${{SERVICE_USER}}"
fi

mkdir -p "${{INSTALL_ROOT}}"
rm -rf "${{INSTALL_ROOT}}/server"
cp -a "${{BUNDLE_ROOT}}/server" "${{INSTALL_ROOT}}/server"
chmod 0755 "${{INSTALL_ROOT}}/server/${{TFS_EXECUTABLE}}"
chown -R "${{SERVICE_USER}}:${{SERVICE_USER}}" "${{INSTALL_ROOT}}"
install -m 0644 "${{SCRIPT_DIR}}/${{SERVICE_NAME}}.service" "/etc/systemd/system/${{SERVICE_NAME}}.service"
systemctl daemon-reload
systemctl enable --now "${{SERVICE_NAME}}.service"
systemctl --no-pager --full status "${{SERVICE_NAME}}.service" || true
"""


def make_healthcheck(service: str, install_root: str) -> str:
    return f"""#!/usr/bin/env bash
set -euo pipefail
systemctl is-active --quiet {service}
test -d '{install_root}/server'
echo 'FANTASY_VPS_HEALTH PASS'
"""


def make_update(service: str, user: str, install_root: str, backup_root: str) -> str:
    return f"""#!/usr/bin/env bash
set -euo pipefail
if [[ ${{EUID}} -ne 0 ]]; then echo 'Run as root'; exit 1; fi
SCRIPT_DIR="$(cd "$(dirname "${{BASH_SOURCE[0]}}")" && pwd)"
BUNDLE_ROOT="$(cd "${{SCRIPT_DIR}}/.." && pwd)"
INSTALL_ROOT='{install_root}'
BACKUP_ROOT='{backup_root}'
STAMP="$(date -u +%Y%m%dT%H%M%SZ)"
BACKUP="${{BACKUP_ROOT}}/${{STAMP}}"
mkdir -p "${{BACKUP}}"
if [[ -d "${{INSTALL_ROOT}}/server" ]]; then cp -a "${{INSTALL_ROOT}}/server/." "${{BACKUP}}/" || true; fi
systemctl stop {service} || true
rm -rf "${{INSTALL_ROOT}}/server"
mkdir -p "${{INSTALL_ROOT}}/server"
cp -a "${{BUNDLE_ROOT}}/server/." "${{INSTALL_ROOT}}/server/"
chown -R {user}:{user} "${{INSTALL_ROOT}}/server"
systemctl start {service}
if ! "${{SCRIPT_DIR}}/healthcheck.sh"; then
  echo 'Health check failed; rolling back'
  systemctl stop {service} || true
  rm -rf "${{INSTALL_ROOT}}/server"
  mkdir -p "${{INSTALL_ROOT}}/server"
  cp -a "${{BACKUP}}/." "${{INSTALL_ROOT}}/server/"
  chown -R {user}:{user} "${{INSTALL_ROOT}}/server"
  systemctl start {service}
  exit 2
fi
echo "FANTASY_VPS_UPDATE PASS backup=${{BACKUP}}"
"""


def make_rollback(service: str, user: str, install_root: str, backup_root: str) -> str:
    return f"""#!/usr/bin/env bash
set -euo pipefail
if [[ ${{EUID}} -ne 0 ]]; then echo 'Run as root'; exit 1; fi
INSTALL_ROOT='{install_root}'
BACKUP_ROOT='{backup_root}'
BACKUP="${{1:-$(find "${{BACKUP_ROOT}}" -mindepth 1 -maxdepth 1 -type d | sort | tail -n 1)}}"
if [[ -z "${{BACKUP}}" || ! -d "${{BACKUP}}" ]]; then echo 'No backup found'; exit 1; fi
systemctl stop {service} || true
rm -rf "${{INSTALL_ROOT}}/server"
mkdir -p "${{INSTALL_ROOT}}/server"
cp -a "${{BACKUP}}/." "${{INSTALL_ROOT}}/server/"
chown -R {user}:{user} "${{INSTALL_ROOT}}/server"
systemctl start {service}
echo "FANTASY_VPS_ROLLBACK PASS backup=${{BACKUP}}"
"""


def make_readme(service: str, install_root: str) -> str:
    return f"""# Fantasy TFS1098 VPS bundle

This bundle was generated from an already-packaged Fantasy TFS1098 runtime.

## Before installation

1. Install/configure MariaDB and import the TFS 1.4.2 schema.
2. Review `server/config.lua` and set production database credentials, public IP and ports.
3. Keep production credentials out of source control and out of distributable artifacts.
4. Open only the required login/game/status ports in the VPS firewall.
5. `deploy/env.example` is a placeholder only; do not commit production secrets.

## First install

```bash
cd deploy
sudo bash ./install.sh
```

## Update / health / rollback

```bash
sudo bash ./update.sh
sudo bash ./healthcheck.sh
sudo bash ./rollback.sh
```

The service is installed as `{service}.service` under `{install_root}`.
Logs are available with:

```bash
journalctl -u {service} -f
```
"""


def build(args: argparse.Namespace) -> Path:
    validate_identifier(args.service_name, "service name")
    validate_identifier(args.service_user, "service user")
    validate_filename(args.executable)
    validate_install_root(args.install_root)
    backup_root = args.backup_root or f"/opt/fantasy/backups/{args.service_name}"
    validate_install_root(backup_root, "backup root")
    if backup_root == args.install_root:
        fail("backup root and install root must differ")

    runtime = Path(args.runtime).expanduser().resolve()
    output = Path(args.output).expanduser().resolve()
    if not runtime.is_dir():
        fail(f"runtime directory is missing or invalid: {runtime}")
    if runtime == output or is_within(output, runtime) or is_within(runtime, output):
        fail("deployment output and runtime directory must not contain one another")

    runtime_executable = runtime / args.executable
    if not runtime_executable.is_file():
        fail(f"runtime executable is missing: {runtime_executable}")

    if output.exists():
        if not args.force:
            fail(f"output already exists; pass --force to replace it: {output}")
        shutil.rmtree(output)

    server = output / "server"
    deploy = output / "deploy"
    shutil.copytree(runtime, server)
    deploy.mkdir(parents=True, exist_ok=True)

    service_path = deploy / f"{args.service_name}.service"
    installer_path = deploy / "install.sh"
    readme_path = deploy / "README.md"
    manifest_path = deploy / "manifest.json"
    env_path = deploy / "env.example"
    health_path = deploy / "healthcheck.sh"
    update_path = deploy / "update.sh"
    rollback_path = deploy / "rollback.sh"
    policy_path = deploy / "release-policy.json"

    write_text(
        service_path,
        make_service(args.service_name, args.service_user, args.install_root, args.executable),
    )
    write_text(
        installer_path,
        make_installer(args.service_name, args.service_user, args.install_root, args.executable),
    )
    write_text(readme_path, make_readme(args.service_name, args.install_root))
    write_text(
        env_path,
        "# Example only. Do not commit production secrets.\n"
        "DB_HOST=127.0.0.1\n"
        "DB_PORT=3306\n"
        "DB_NAME=fantasy\n"
        "DB_USER=fantasy\n"
        "DB_PASSWORD=CHANGE_ME\n",
    )
    write_text(health_path, make_healthcheck(args.service_name, args.install_root))
    write_text(
        update_path,
        make_update(args.service_name, args.service_user, args.install_root, backup_root),
    )
    write_text(
        rollback_path,
        make_rollback(args.service_name, args.service_user, args.install_root, backup_root),
    )

    manifest = {
        "schemaVersion": 1,
        "runtime": "tfs1098",
        "serviceName": args.service_name,
        "serviceUser": args.service_user,
        "installRoot": args.install_root,
        "executable": args.executable,
        "containsSecrets": False,
        "supportsHealthCheck": True,
        "supportsUpdate": True,
        "supportsRollback": True,
    }
    write_text(manifest_path, json.dumps(manifest, indent=2) + "\n")

    policy = {
        "schemaVersion": 1,
        "serviceName": args.service_name,
        "serviceUser": args.service_user,
        "installRoot": args.install_root,
        "backupRoot": backup_root,
        "backupBeforeUpdate": True,
        "rollbackOnFailedHealthCheck": True,
    }
    write_text(policy_path, json.dumps(policy, indent=2) + "\n")

    try:
        for script in (installer_path, health_path, update_path, rollback_path):
            script.chmod(script.stat().st_mode | stat.S_IXUSR | stat.S_IXGRP | stat.S_IXOTH)
        staged_executable = server / args.executable
        staged_executable.chmod(staged_executable.stat().st_mode | stat.S_IXUSR | stat.S_IXGRP | stat.S_IXOTH)
    except OSError:
        # A Windows filesystem/archive may not preserve Unix executable bits.
        # The install script always restores the staged TFS executable permission.
        pass

    print("FANTASY_TFS1098_VPS_BUNDLE PASS")
    print(f"runtime={runtime}")
    print(f"output={output}")
    print(f"service={args.service_name}.service")
    print(f"installRoot={args.install_root}")
    print(f"backupRoot={backup_root}")
    return output


def parse_args() -> argparse.Namespace:
    parser = argparse.ArgumentParser(description="Build a Fantasy TFS1098 Linux VPS deployment bundle")
    parser.add_argument("--runtime", required=True, help="already-packaged TFS1098 runtime directory")
    parser.add_argument("--output", required=True, help="deployment bundle output directory")
    parser.add_argument("--service-name", default="fantasy-tfs1098")
    parser.add_argument("--service-user", default="fantasy")
    parser.add_argument("--install-root", default="/opt/fantasy/fantasy-tfs1098")
    parser.add_argument("--backup-root", default=None, help="backup directory for update/rollback lifecycle")
    parser.add_argument("--executable", default="tfs")
    parser.add_argument("--force", action="store_true", help="replace an existing output directory")
    return parser.parse_args()


if __name__ == "__main__":
    build(parse_args())
