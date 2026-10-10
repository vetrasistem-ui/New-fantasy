#!/usr/bin/env python3

from __future__ import annotations

import contextlib
import io
import json
from pathlib import Path
import tempfile
import unittest

import fantasy_asset_catalog as catalog


class FantasyAssetCatalogCliTests(unittest.TestCase):
    def setUp(self) -> None:
        self.temp = tempfile.TemporaryDirectory()
        self.project = Path(self.temp.name) / "Project"
        self.project.mkdir(parents=True)

    def tearDown(self) -> None:
        self.temp.cleanup()

    def run_cli(self, *args: str) -> tuple[int, str, str]:
        stdout = io.StringIO()
        stderr = io.StringIO()
        with contextlib.redirect_stdout(stdout), contextlib.redirect_stderr(stderr):
            code = catalog.main(list(args))
        return code, stdout.getvalue(), stderr.getvalue()

    def test_end_to_end_authoring_query_and_context(self) -> None:
        code, _, _ = self.run_cli(
            "init", "--project", str(self.project), "--profile", "assets.1098.test"
        )
        self.assertEqual(code, 0)

        code, _, _ = self.run_cli(
            "family", "add", "--project", str(self.project),
            "--id", "terrain.sand.basic", "--name", "Basic Sand",
            "--tags", "terrain,sand,ground"
        )
        self.assertEqual(code, 0)

        code, _, _ = self.run_cli(
            "entry", "add", "--project", str(self.project),
            "--id", "terrain.sand.center.a",
            "--asset-ref", "legacy.test.item.4526",
            "--family", "terrain.sand.basic",
            "--role", "center",
            "--tags", "terrain,sand,ground,desert,beach",
            "--weight", "3"
        )
        self.assertEqual(code, 0)

        code, output, _ = self.run_cli(
            "query", "--project", str(self.project), "--tags", "terrain,sand"
        )
        self.assertEqual(code, 0)
        query = json.loads(output)
        self.assertEqual(len(query["matches"]), 1)
        self.assertEqual(query["matches"][0]["id"], "terrain.sand.center.a")

        code, output, _ = self.run_cli("context", "--project", str(self.project))
        self.assertEqual(code, 0)
        context = json.loads(output)
        self.assertEqual(context["schema"], "fantasy.asset-context.v1")
        self.assertEqual(context["families"][0]["members"][0]["role"], "center")

        code, output, _ = self.run_cli("validate", "--project", str(self.project))
        self.assertEqual(code, 0)
        self.assertIn("ASSET_CATALOG PASS", output)

    def test_missing_family_is_rejected(self) -> None:
        self.assertEqual(
            self.run_cli("init", "--project", str(self.project), "--profile", "assets.1098.test")[0],
            0,
        )
        code, _, error = self.run_cli(
            "entry", "add", "--project", str(self.project),
            "--id", "terrain.water.center",
            "--asset-ref", "legacy.test.item.4608",
            "--family", "terrain.water.missing"
        )
        self.assertEqual(code, 2)
        self.assertIn("missing family", error)

    def test_family_remove_requires_explicit_member_removal(self) -> None:
        self.assertEqual(
            self.run_cli("init", "--project", str(self.project), "--profile", "assets.1098.test")[0],
            0,
        )
        self.assertEqual(
            self.run_cli(
                "family", "add", "--project", str(self.project),
                "--id", "structure.wall.stone", "--name", "Stone Wall"
            )[0],
            0,
        )
        self.assertEqual(
            self.run_cli(
                "entry", "add", "--project", str(self.project),
                "--id", "structure.wall.stone.segment",
                "--asset-ref", "legacy.test.item.1029",
                "--family", "structure.wall.stone"
            )[0],
            0,
        )

        code, _, error = self.run_cli(
            "family", "remove", "--project", str(self.project),
            "--id", "structure.wall.stone"
        )
        self.assertEqual(code, 2)
        self.assertIn("still has 1 entries", error)

        code, _, _ = self.run_cli(
            "family", "remove", "--project", str(self.project),
            "--id", "structure.wall.stone", "--with-entries"
        )
        self.assertEqual(code, 0)
        saved = catalog.read_catalog(catalog.catalog_path(self.project))
        self.assertEqual(saved["families"], [])
        self.assertEqual(saved["entries"], [])


if __name__ == "__main__":
    unittest.main()
