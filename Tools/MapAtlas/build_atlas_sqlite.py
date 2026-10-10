"""Build a transactional, derived Atlas V1 database from streaming canonical CSVs."""
import argparse
import csv
import hashlib
import json
import os
from pathlib import Path
import sqlite3


# Column types are explicit: IDs/counts/coordinates are never inferred from rows.
SCHEMAS = {
    "tiles": "tileId:I,x:I,y:I,z:I,houseId:I,flags:I,groundServerId:I,groundClientId:I,topLevelItemCount:I",
    "items": "itemId:I,tileId:I,parentItemId:I,role:T,stackIndex:I,depth:I,serverId:I,clientId:I,countOrSubtype:I",
    "attributes": "ownerType:T,ownerId:I,key:T,valueType:T,intValue:I,textValue:T,positionX:I,positionY:I,positionZ:I",
    "assets": "serverId:I,clientId:I,kind:T,totalUses:I,groundUses:I,topLevelUses:I,nestedUses:I,houseTileUses:I,nonHouseTileUses:I,uniqueTiles:I,uniqueHouses:I,firstX:I,firstY:I,firstZ:I,minX:I,minY:I,minZ:I,maxX:I,maxY:I,maxZ:I,floorMask:I",
    "asset_examples": "serverId:I,clientId:I,x:I,y:I,z:I,houseId:I,role:T,stackIndex:I",
    "houses": "id:I,name:T,exitX:I,exitY:I,exitZ:I,rent:I,townId:I,guildhall:I",
    "towns": "id:I,name:T,templeX:I,templeY:I,templeZ:I",
    "waypoints": "name:T,x:I,y:I,z:I",
    "spawn_areas": "spawnAreaId:I,centerX:I,centerY:I,centerZ:I,radius:I",
    "spawn_entries": "spawnAreaId:I,entryIndex:I,kind:T,x:I,y:I,z:I,name:T,direction:I,intervalSeconds:I",
    "spawn_monster_options": "spawnAreaId:I,entryIndex:I,optionIndex:I,name:T,chance:I",
    "adjacency": "assetA:T,assetB:T,dx:I,dy:I,dz:I,count:I,sameHouseCount:I",
    "cooccurrence": "assetA:T,assetB:T,sameTileCount:I,houseTileCount:I",
}
INDEXES = {
    "tiles": [("tileId",), ("x", "y", "z"), ("houseId",)],
    "items": [("itemId",), ("tileId",), ("serverId",), ("clientId",), ("parentItemId",)],
    "assets": [("serverId",), ("clientId",)],
    "asset_examples": [("serverId",), ("x", "y", "z")],
    "adjacency": [("assetA",), ("assetB",)],
    "cooccurrence": [("assetA",), ("assetB",)],
    "spawn_entries": [("x", "y", "z")],
}
SOURCES = ("otbmPath", "otbPath", "datPath", "sprPath", "housePath", "spawnPath")


def sha256(path):
    digest = hashlib.sha256()
    with path.open("rb") as stream:
        for block in iter(lambda: stream.read(1024 * 1024), b""):
            digest.update(block)
    return digest.hexdigest().upper()


def source_hashes(raw, manifest):
    result = {}
    for key in SOURCES:
        path = Path(manifest[key])
        if path.is_absolute():
            raise ValueError(f"Manifest source path must be relative: {key}")
        resolved = (raw / path).resolve(strict=True)
        result[key] = {"path": path.as_posix(), "sha256": sha256(resolved)}
    return result


def check(connection, counts, manifest):
    for table, field in (("tiles", "canonicalTileCount"), ("items", "itemCount"),
                         ("assets", "assetCount"), ("houses", "houseCount"),
                         ("spawn_areas", "spawnAreaCount"), ("towns", "townCount"),
                         ("waypoints", "waypointCount")):
        if counts[table] != manifest[field]:
            raise ValueError(f"Manifest count mismatch: {table}")
    queries = {
        "orphan items": "SELECT COUNT(*) FROM items i LEFT JOIN tiles t ON t.tileId=i.tileId WHERE t.tileId IS NULL",
        "bad parent": "SELECT COUNT(*) FROM items i LEFT JOIN items p ON p.itemId=i.parentItemId WHERE i.parentItemId IS NOT NULL AND (p.itemId IS NULL OR p.tileId!=i.tileId OR i.depth!=p.depth+1 OR i.role!='content')",
        "bad role": "SELECT COUNT(*) FROM items WHERE role NOT IN ('ground','item','content') OR (parentItemId IS NULL AND (depth!=0 OR role='content')) OR stackIndex<0",
        "bad spatial coordinates": "SELECT COUNT(*) FROM tiles WHERE x NOT BETWEEN 0 AND 65535 OR y NOT BETWEEN 0 AND 65535 OR z NOT BETWEEN 0 AND 15",
        "duplicate tile coordinates": "SELECT COUNT(*) FROM (SELECT x,y,z FROM tiles GROUP BY x,y,z HAVING COUNT(*)>1)",
        "duplicate item IDs": "SELECT COUNT(*) FROM (SELECT itemId FROM items GROUP BY itemId HAVING COUNT(*)>1)",
        "duplicate tile IDs": "SELECT COUNT(*) FROM (SELECT tileId FROM tiles GROUP BY tileId HAVING COUNT(*)>1)",
        "too many examples": "SELECT COUNT(*) FROM (SELECT serverId,clientId FROM asset_examples GROUP BY serverId,clientId HAVING COUNT(*)>50)",
        "bad adjacency": "SELECT COUNT(*) FROM adjacency WHERE dz!=0 OR NOT ((dx=1 AND dy=0) OR (dx=0 AND dy=1)) OR sameHouseCount>count OR sameHouseCount<0",
        "bad cooccurrence": "SELECT COUNT(*) FROM cooccurrence WHERE houseTileCount>sameTileCount OR houseTileCount<0 OR assetA=assetB",
        "orphan spawn": "SELECT COUNT(*) FROM spawn_entries e LEFT JOIN spawn_areas a USING(spawnAreaId) WHERE a.spawnAreaId IS NULL",
        "orphan monster option": "SELECT COUNT(*) FROM spawn_monster_options o LEFT JOIN spawn_entries e USING(spawnAreaId,entryIndex) WHERE e.spawnAreaId IS NULL OR e.kind!='monsterSet'",
        "unknown attribute owner": "SELECT COUNT(*) FROM attributes WHERE ownerType NOT IN ('tile','item') OR valueType NOT IN ('int64','string','Position')",
        "orphan attribute": "SELECT COUNT(*) FROM attributes a WHERE (ownerType='tile' AND NOT EXISTS (SELECT 1 FROM tiles t WHERE t.tileId=a.ownerId)) OR (ownerType='item' AND NOT EXISTS (SELECT 1 FROM items i WHERE i.itemId=a.ownerId))",
        "asset counts": "SELECT COUNT(*) FROM assets a LEFT JOIN (SELECT serverId,clientId,COUNT(*) n FROM items GROUP BY serverId,clientId) i USING(serverId,clientId) WHERE i.n IS NULL OR i.n!=a.totalUses OR a.totalUses!=a.groundUses+a.topLevelUses+a.nestedUses OR a.totalUses!=a.houseTileUses+a.nonHouseTileUses",
    }
    for label, sql in queries.items():
        if connection.execute(sql).fetchone()[0]:
            raise ValueError(f"Atlas sanity check failed: {label}")
    if connection.execute("PRAGMA integrity_check").fetchone()[0] != "ok":
        raise ValueError("SQLite integrity_check failed")


def build(raw, output):
    raw = Path(raw).resolve(strict=True)
    output = Path(output).resolve()
    metadata_path = output.parent / "atlas-build.json"
    temporary = output.with_name(output.name + ".partial")
    if output.exists() or temporary.exists() or metadata_path.exists():
        raise FileExistsError("Atlas destination/build metadata exists; use a new output directory")
    manifest_path = raw / "atlas-manifest.json"
    manifest = json.loads(manifest_path.read_text(encoding="utf-8-sig"))
    if manifest["schemaVersion"] != 1:
        raise ValueError("Unsupported Atlas schemaVersion")
    before = source_hashes(raw, manifest)
    manifest_hash = sha256(manifest_path)
    output.parent.mkdir(parents=True, exist_ok=True)
    counts = {}
    csv_hashes = {}
    connection = sqlite3.connect(temporary)
    try:
        with connection:
            for table, definition in SCHEMAS.items():
                fields = [value.split(":") for value in definition.split(",")]
                ddl = ",".join(f'"{name}" {"INTEGER" if typ == "I" else "TEXT"}' for name, typ in fields)
                connection.execute(f'CREATE TABLE "{table}" ({ddl})')
                source = raw / (table.replace("_", "-") + ".csv")
                csv_hashes[source.name] = sha256(source)
                sql = f'INSERT INTO "{table}" VALUES ({",".join("?" for _ in fields)})'
                count = 0
                with source.open(encoding="utf-8-sig", newline="") as stream:
                    reader = csv.DictReader(stream)
                    if reader.fieldnames != [name for name, _ in fields]:
                        raise ValueError(f"CSV header mismatch: {source.name}")
                    batch = []
                    for row in reader:
                        if None in row or any(value is None for value in row.values()):
                            raise ValueError(f"Malformed CSV row in {source.name}")
                        batch.append(tuple(None if typ == "I" and row[name] == "" else
                                           int(row[name]) if typ == "I" else row[name]
                                           for name, typ in fields))
                        count += 1
                        if len(batch) == 5000:
                            connection.executemany(sql, batch)
                            batch.clear()
                    if batch:
                        connection.executemany(sql, batch)
                if sha256(source) != csv_hashes[source.name]:
                    raise ValueError(f"CSV changed while reading: {source.name}")
                counts[table] = count
            for table, groups in INDEXES.items():
                for columns in groups:
                    name = f'idx_{table}_{"_".join(columns)}'
                    connection.execute(f'CREATE INDEX "{name}" ON "{table}" ({",".join(columns)})')
            check(connection, counts, manifest)
            if source_hashes(raw, manifest) != before or sha256(manifest_path) != manifest_hash:
                raise ValueError("Atlas inputs changed during database build")
        connection.execute("ANALYZE")
        connection.commit()
    finally:
        connection.close()
    metadata = {"schemaVersion": 1, "manifestSha256": manifest_hash, "sources": before,
                "csvSha256": csv_hashes, "rowCounts": counts,
                "sqlitePath": output.name, "sqliteSha256": sha256(temporary),
                "sourceHashBase": Path(os.path.relpath(raw, output.parent)).as_posix()}
    # Publish only after all validation passed; failed builds retain .partial evidence.
    temporary.rename(output)
    metadata_path.write_text(json.dumps(metadata, indent=2) + "\n", encoding="utf-8")
    return metadata


if __name__ == "__main__":
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--input", required=True, type=Path)
    parser.add_argument("--output", required=True, type=Path)
    args = parser.parse_args()
    print(json.dumps(build(args.input, args.output), indent=2))
