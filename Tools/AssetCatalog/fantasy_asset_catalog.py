#!/usr/bin/env python3
"""Fantasy Asset Catalog V1 authoring/query CLI.

This tool intentionally edits only the semantic catalog. It never guesses what a
sprite means and never rewrites DAT/SPR/OTB/OTBM. The C++ Project Health scanner
remains the authoritative project validator after changes are made.
"""

from __future__ import annotations

import argparse
import json
import os
from pathlib import Path
import re
import sys
import tempfile
from typing import Any, Iterable

SCHEMA_VERSION = 1
CATALOG_RELATIVE_PATH = Path("Assets") / "Catalog" / "asset-catalog.json"
VALID_SOURCES = {"legacy_registry", "modern_asset"}
IDENTIFIER_RE = re.compile(r"^[A-Za-z0-9][A-Za-z0-9._-]*$")


class CatalogError(RuntimeError):
    pass


def require_identifier(value: str, label: str) -> str:
    value = value.strip()
    if not value or not IDENTIFIER_RE.fullmatch(value):
        raise CatalogError(f"invalid {label}: {value!r}")
    return value


def parse_tags(value: str | None) -> list[str]:
    if not value:
        return []
    result: list[str] = []
    seen: set[str] = set()
    for raw in value.split(","):
        tag = raw.strip()
        if not tag:
            continue
        require_identifier(tag, "tag")
        if tag in seen:
            continue
        seen.add(tag)
        result.append(tag)
    return result


def catalog_path(project: str | Path) -> Path:
    return Path(project).expanduser().resolve() / CATALOG_RELATIVE_PATH


def blank_catalog(profile_id: str) -> dict[str, Any]:
    return {
        "schemaVersion": SCHEMA_VERSION,
        "profileId": require_identifier(profile_id, "profile id"),
        "families": [],
        "entries": [],
    }


def read_catalog(path: Path) -> dict[str, Any]:
    try:
        with path.open("r", encoding="utf-8") as handle:
            data = json.load(handle)
    except FileNotFoundError as exc:
        raise CatalogError(f"catalog not found: {path}") from exc
    except json.JSONDecodeError as exc:
        raise CatalogError(f"invalid catalog JSON: {exc}") from exc
    validate_catalog(data)
    return data


def write_catalog(path: Path, data: dict[str, Any]) -> None:
    validate_catalog(data)
    normalize_catalog(data)
    path.parent.mkdir(parents=True, exist_ok=True)
    fd, temporary = tempfile.mkstemp(prefix="asset-catalog-", suffix=".tmp", dir=path.parent)
    try:
        with os.fdopen(fd, "w", encoding="utf-8", newline="\n") as handle:
            json.dump(data, handle, indent=2, ensure_ascii=False)
            handle.write("\n")
        os.replace(temporary, path)
    except Exception:
        try:
            os.unlink(temporary)
        except OSError:
            pass
        raise


def normalize_catalog(data: dict[str, Any]) -> None:
    data["families"].sort(key=lambda item: item["id"])
    data["entries"].sort(key=lambda item: item["id"])


def validate_catalog(data: dict[str, Any]) -> None:
    if not isinstance(data, dict):
        raise CatalogError("catalog root must be an object")
    if data.get("schemaVersion") != SCHEMA_VERSION:
        raise CatalogError(
            f"unsupported schemaVersion: {data.get('schemaVersion')!r}; expected {SCHEMA_VERSION}"
        )
    require_identifier(str(data.get("profileId", "")), "profile id")

    families = data.get("families")
    entries = data.get("entries")
    if not isinstance(families, list) or not isinstance(entries, list):
        raise CatalogError("families and entries must be arrays")

    family_ids: set[str] = set()
    for family in families:
        if not isinstance(family, dict):
            raise CatalogError("family must be an object")
        family_id = require_identifier(str(family.get("id", "")), "family id")
        if family_id in family_ids:
            raise CatalogError(f"duplicate family id: {family_id}")
        family_ids.add(family_id)
        if not str(family.get("name", "")).strip():
            raise CatalogError(f"family {family_id}: name is required")
        validate_tags(family.get("tags", []), f"family {family_id}")
        brush = family.get("brushRef")
        if brush is not None:
            require_identifier(str(brush), f"family {family_id} brushRef")

    entry_ids: set[str] = set()
    for entry in entries:
        if not isinstance(entry, dict):
            raise CatalogError("entry must be an object")
        entry_id = require_identifier(str(entry.get("id", "")), "entry id")
        if entry_id in entry_ids:
            raise CatalogError(f"duplicate entry id: {entry_id}")
        entry_ids.add(entry_id)
        source = str(entry.get("source", ""))
        if source not in VALID_SOURCES:
            raise CatalogError(f"entry {entry_id}: unsupported source {source!r}")
        require_identifier(str(entry.get("assetRef", "")), f"entry {entry_id} assetRef")
        family_id = str(entry.get("familyId", ""))
        if family_id:
            require_identifier(family_id, f"entry {entry_id} familyId")
            if family_id not in family_ids:
                raise CatalogError(f"entry {entry_id}: missing family {family_id}")
        role = str(entry.get("role", ""))
        if role:
            require_identifier(role, f"entry {entry_id} role")
        validate_tags(entry.get("tags", []), f"entry {entry_id}")
        weight = entry.get("weight", 1)
        if not isinstance(weight, int) or isinstance(weight, bool) or weight <= 0:
            raise CatalogError(f"entry {entry_id}: weight must be a positive integer")


def validate_tags(tags: Any, owner: str) -> None:
    if not isinstance(tags, list):
        raise CatalogError(f"{owner}: tags must be an array")
    seen: set[str] = set()
    for tag in tags:
        tag = require_identifier(str(tag), f"{owner} tag")
        if tag in seen:
            raise CatalogError(f"{owner}: duplicate tag {tag}")
        seen.add(tag)


def find_family(data: dict[str, Any], family_id: str) -> dict[str, Any] | None:
    return next((item for item in data["families"] if item["id"] == family_id), None)


def find_entry(data: dict[str, Any], entry_id: str) -> dict[str, Any] | None:
    return next((item for item in data["entries"] if item["id"] == entry_id), None)


def cmd_init(args: argparse.Namespace) -> None:
    path = catalog_path(args.project)
    if path.exists() and not args.force:
        raise CatalogError(f"catalog already exists: {path}; use --force to replace it")
    data = blank_catalog(args.profile)
    write_catalog(path, data)
    print(f"ASSET_CATALOG INIT {path}")


def cmd_family_add(args: argparse.Namespace) -> None:
    path = catalog_path(args.project)
    data = read_catalog(path)
    family_id = require_identifier(args.id, "family id")
    family = find_family(data, family_id)
    value = {
        "id": family_id,
        "name": args.name.strip(),
        "tags": parse_tags(args.tags),
        "brushRef": require_identifier(args.brush, "brush ref") if args.brush else None,
    }
    if not value["name"]:
        raise CatalogError("family name is required")
    if family is None:
        data["families"].append(value)
        action = "ADD"
    else:
        family.clear()
        family.update(value)
        action = "UPDATE"
    write_catalog(path, data)
    print(f"ASSET_CATALOG FAMILY_{action} {family_id}")


def cmd_family_remove(args: argparse.Namespace) -> None:
    path = catalog_path(args.project)
    data = read_catalog(path)
    family_id = require_identifier(args.id, "family id")
    members = [entry["id"] for entry in data["entries"] if entry.get("familyId") == family_id]
    if members and not args.with_entries:
        raise CatalogError(
            f"family {family_id} still has {len(members)} entries; use --with-entries to remove them too"
        )
    before = len(data["families"])
    data["families"] = [family for family in data["families"] if family["id"] != family_id]
    if before == len(data["families"]):
        raise CatalogError(f"family not found: {family_id}")
    if args.with_entries:
        data["entries"] = [entry for entry in data["entries"] if entry.get("familyId") != family_id]
    write_catalog(path, data)
    print(f"ASSET_CATALOG FAMILY_REMOVE {family_id}")


def cmd_entry_add(args: argparse.Namespace) -> None:
    path = catalog_path(args.project)
    data = read_catalog(path)
    entry_id = require_identifier(args.id, "entry id")
    family_id = require_identifier(args.family, "family id") if args.family else ""
    if family_id and find_family(data, family_id) is None:
        raise CatalogError(f"missing family: {family_id}")
    value = {
        "id": entry_id,
        "source": args.source,
        "assetRef": require_identifier(args.asset_ref, "asset ref"),
        "familyId": family_id,
        "role": require_identifier(args.role, "role") if args.role else "",
        "tags": parse_tags(args.tags),
        "weight": args.weight,
    }
    entry = find_entry(data, entry_id)
    if entry is None:
        data["entries"].append(value)
        action = "ADD"
    else:
        entry.clear()
        entry.update(value)
        action = "UPDATE"
    write_catalog(path, data)
    print(f"ASSET_CATALOG ENTRY_{action} {entry_id}")


def cmd_entry_remove(args: argparse.Namespace) -> None:
    path = catalog_path(args.project)
    data = read_catalog(path)
    entry_id = require_identifier(args.id, "entry id")
    before = len(data["entries"])
    data["entries"] = [entry for entry in data["entries"] if entry["id"] != entry_id]
    if before == len(data["entries"]):
        raise CatalogError(f"entry not found: {entry_id}")
    write_catalog(path, data)
    print(f"ASSET_CATALOG ENTRY_REMOVE {entry_id}")


def cmd_list(args: argparse.Namespace) -> None:
    data = read_catalog(catalog_path(args.project))
    print(f"profile={data['profileId']} families={len(data['families'])} entries={len(data['entries'])}")
    for family in data["families"]:
        members = sum(1 for entry in data["entries"] if entry.get("familyId") == family["id"])
        print(f"FAMILY {family['id']} members={members} tags={','.join(family['tags'])}")
    for entry in data["entries"]:
        print(
            "ENTRY "
            f"{entry['id']} source={entry['source']} ref={entry['assetRef']} "
            f"family={entry.get('familyId', '') or '-'} role={entry.get('role', '') or '-'} "
            f"tags={','.join(entry.get('tags', []))}"
        )


def has_all_tags(entry: dict[str, Any], required: Iterable[str]) -> bool:
    available = set(entry.get("tags", []))
    return all(tag in available for tag in required)


def cmd_query(args: argparse.Namespace) -> None:
    data = read_catalog(catalog_path(args.project))
    required = parse_tags(args.tags)
    family = args.family.strip() if args.family else ""
    role = args.role.strip() if args.role else ""
    if family:
        require_identifier(family, "family id")
    if role:
        require_identifier(role, "role")
    matches = [
        entry
        for entry in data["entries"]
        if has_all_tags(entry, required)
        and (not family or entry.get("familyId") == family)
        and (not role or entry.get("role") == role)
    ]
    payload = {
        "profileId": data["profileId"],
        "query": {"tags": required, "familyId": family, "role": role},
        "matches": matches,
    }
    print(json.dumps(payload, indent=2, ensure_ascii=False))


def cmd_context(args: argparse.Namespace) -> None:
    data = read_catalog(catalog_path(args.project))
    families: list[dict[str, Any]] = []
    for family in data["families"]:
        members = [
            {
                "id": entry["id"],
                "role": entry.get("role", ""),
                "tags": entry.get("tags", []),
                "assetRef": entry["assetRef"],
                "source": entry["source"],
                "weight": entry.get("weight", 1),
            }
            for entry in data["entries"]
            if entry.get("familyId") == family["id"]
        ]
        families.append(
            {
                "id": family["id"],
                "name": family["name"],
                "tags": family["tags"],
                "brushRef": family.get("brushRef"),
                "members": members,
            }
        )
    ungrouped = [
        entry for entry in data["entries"] if not entry.get("familyId")
    ]
    payload = {
        "schema": "fantasy.asset-context.v1",
        "profileId": data["profileId"],
        "instruction": "Use semantic ids/tags/families; never infer visual meaning from numeric ids alone.",
        "families": families,
        "ungrouped": ungrouped,
    }
    if args.compact:
        print(json.dumps(payload, separators=(",", ":"), ensure_ascii=False))
    else:
        print(json.dumps(payload, indent=2, ensure_ascii=False))


def cmd_validate(args: argparse.Namespace) -> None:
    path = catalog_path(args.project)
    data = read_catalog(path)
    print(
        f"ASSET_CATALOG PASS profile={data['profileId']} "
        f"families={len(data['families'])} entries={len(data['entries'])} path={path}"
    )


def build_parser() -> argparse.ArgumentParser:
    parser = argparse.ArgumentParser(description="Fantasy Asset Catalog V1 authoring/query tool")
    sub = parser.add_subparsers(dest="command", required=True)

    init = sub.add_parser("init", help="create a project asset catalog")
    init.add_argument("--project", required=True)
    init.add_argument("--profile", required=True)
    init.add_argument("--force", action="store_true")
    init.set_defaults(func=cmd_init)

    family = sub.add_parser("family", help="add/update/remove semantic asset families")
    family_sub = family.add_subparsers(dest="family_command", required=True)
    family_add = family_sub.add_parser("add")
    family_add.add_argument("--project", required=True)
    family_add.add_argument("--id", required=True)
    family_add.add_argument("--name", required=True)
    family_add.add_argument("--tags", default="")
    family_add.add_argument("--brush", default="")
    family_add.set_defaults(func=cmd_family_add)
    family_remove = family_sub.add_parser("remove")
    family_remove.add_argument("--project", required=True)
    family_remove.add_argument("--id", required=True)
    family_remove.add_argument("--with-entries", action="store_true")
    family_remove.set_defaults(func=cmd_family_remove)

    entry = sub.add_parser("entry", help="add/update/remove semantic catalog entries")
    entry_sub = entry.add_subparsers(dest="entry_command", required=True)
    entry_add = entry_sub.add_parser("add")
    entry_add.add_argument("--project", required=True)
    entry_add.add_argument("--id", required=True)
    entry_add.add_argument("--source", choices=sorted(VALID_SOURCES), default="legacy_registry")
    entry_add.add_argument("--asset-ref", required=True)
    entry_add.add_argument("--family", default="")
    entry_add.add_argument("--role", default="")
    entry_add.add_argument("--tags", default="")
    entry_add.add_argument("--weight", type=int, default=1)
    entry_add.set_defaults(func=cmd_entry_add)
    entry_remove = entry_sub.add_parser("remove")
    entry_remove.add_argument("--project", required=True)
    entry_remove.add_argument("--id", required=True)
    entry_remove.set_defaults(func=cmd_entry_remove)

    listing = sub.add_parser("list", help="print families and entries")
    listing.add_argument("--project", required=True)
    listing.set_defaults(func=cmd_list)

    query = sub.add_parser("query", help="query catalog entries by semantic tags/family/role")
    query.add_argument("--project", required=True)
    query.add_argument("--tags", default="")
    query.add_argument("--family", default="")
    query.add_argument("--role", default="")
    query.set_defaults(func=cmd_query)

    context = sub.add_parser("context", help="emit an AI-friendly semantic asset context")
    context.add_argument("--project", required=True)
    context.add_argument("--compact", action="store_true")
    context.set_defaults(func=cmd_context)

    validate = sub.add_parser("validate", help="validate catalog schema and references between families/entries")
    validate.add_argument("--project", required=True)
    validate.set_defaults(func=cmd_validate)

    return parser


def main(argv: list[str] | None = None) -> int:
    parser = build_parser()
    args = parser.parse_args(argv)
    try:
        args.func(args)
        return 0
    except (CatalogError, ValueError, OSError) as exc:
        print(f"ASSET_CATALOG ERROR: {exc}", file=sys.stderr)
        return 2


if __name__ == "__main__":
    raise SystemExit(main())
