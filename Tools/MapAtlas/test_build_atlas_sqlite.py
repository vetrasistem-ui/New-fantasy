import csv
import importlib.util
import json
from pathlib import Path
import sqlite3
import tempfile
import unittest

spec = importlib.util.spec_from_file_location("atlas_builder", Path(__file__).with_name("build_atlas_sqlite.py"))
builder = importlib.util.module_from_spec(spec)
spec.loader.exec_module(builder)


class AtlasBuilderTests(unittest.TestCase):
    def setUp(self):
        self.test_root = Path(__file__).resolve().parents[2] / "build" / "map-atlas-tests"
        self.test_root.mkdir(parents=True, exist_ok=True)

    def fixture(self, root, orphan=False):
        raw = root / "raw"
        raw.mkdir()
        source = root / "source.bin"
        source.write_bytes(b"immutable synthetic source")
        manifest = dict(schemaVersion=1, canonicalTileCount=1, itemCount=2, assetCount=2,
                        houseCount=0, spawnAreaCount=0, townCount=0, waypointCount=0)
        manifest.update({key: "../source.bin" for key in builder.SOURCES})
        (raw / "atlas-manifest.json").write_text(json.dumps(manifest))
        records = {
            "tiles": [[1, 5, 6, 7, 0, 1, 100, 200, 0]],
            "items": [[1, 1, "", "ground", 0, 0, 100, 200, 1],
                      [2, 1, 999 if orphan else 1, "content", 0, 1, 101, 201, 2]],
            "attributes": [["item", 2, "note", "string", "", 'quoted,"text"\nline', "", "", ""],
                           ["tile", 1, "target", "Position", "", "", 8, 9, 7],
                           ["item", 1, "actionId", "int64", -20, "", "", "", ""]],
            "assets": [[100, 200, "ground", 1, 1, 0, 0, 0, 1, 1, 0, 5, 6, 7, 5, 6, 7, 5, 6, 7, 128],
                       [101, 201, "object", 1, 0, 0, 1, 0, 1, 1, 0, 5, 6, 7, 5, 6, 7, 5, 6, 7, 128]],
        }
        for table, schema in builder.SCHEMAS.items():
            with (raw / (table.replace("_", "-") + ".csv")).open("w", newline="", encoding="utf-8") as stream:
                writer = csv.writer(stream)
                writer.writerow([field.split(":")[0] for field in schema.split(",")])
                writer.writerows(records.get(table, []))
        return raw, source

    def test_nested_attributes_counts_hashes_and_indexes(self):
        with tempfile.TemporaryDirectory(dir=self.test_root) as folder:
            root = Path(folder)
            raw, source = self.fixture(root)
            original = builder.sha256(source)
            output = root / "atlas.sqlite"
            metadata = builder.build(raw, output)
            self.assertEqual(metadata["rowCounts"]["items"], 2)
            self.assertEqual(metadata["sqliteSha256"], builder.sha256(output))
            self.assertEqual(builder.sha256(source), original)
            with sqlite3.connect(output) as db:
                self.assertEqual(db.execute("SELECT parentItemId,depth FROM items WHERE itemId=2").fetchone(), (1, 1))
                self.assertEqual(db.execute("SELECT textValue FROM attributes WHERE valueType='string'").fetchone()[0], 'quoted,"text"\nline')
                self.assertTrue(db.execute("SELECT name FROM sqlite_master WHERE name='idx_tiles_x_y_z'").fetchone())
            db.close()
            with self.assertRaises(FileExistsError):
                builder.build(raw, output)

    def test_orphan_rejected_without_publishing_database(self):
        with tempfile.TemporaryDirectory(dir=self.test_root) as folder:
            root = Path(folder)
            raw, _ = self.fixture(root, orphan=True)
            output = root / "atlas.sqlite"
            with self.assertRaisesRegex(ValueError, "bad parent"):
                builder.build(raw, output)
            self.assertFalse(output.exists())
            self.assertFalse((root / "atlas-build.json").exists())

    def test_manifest_count_mismatch_rejected(self):
        with tempfile.TemporaryDirectory(dir=self.test_root) as folder:
            root = Path(folder)
            raw, _ = self.fixture(root)
            manifest = json.loads((raw / "atlas-manifest.json").read_text())
            manifest["itemCount"] = 3
            (raw / "atlas-manifest.json").write_text(json.dumps(manifest))
            with self.assertRaisesRegex(ValueError, "Manifest count mismatch"):
                builder.build(raw, root / "atlas.sqlite")


if __name__ == "__main__":
    unittest.main()
