# Real 10.98 Homologation Pack

This document pins the external 10.98 pack used to validate the Fantasy legacy bridge without committing third-party game data to the repository.

## Purpose

The pack is a **homologation fixture**, not a repository dependency and not the Fantasy source of truth.

Validation path:

```text
OTBM + OTB + DAT + SPR + house/spawn XML
            |
            v
LegacyMapProjectLoader
            |
            v
LegacyWorkspaceSession
            |
            v
canonical MapDocument + FantasyAssetRegistry
            |
            v
LegacySpriteTextureCache + LegacyMapCanvasRenderer
            |
            v
Fantasy Studio
```

`LegacyWorkspaceSession` is the reusable boundary used by the V5 Map workspace. The standalone preview uses the same session/renderer path so loader/render defects can be isolated without creating a second map engine.

## Pinned files

| Role | External file | Size | SHA-256 |
| --- | --- | ---: | --- |
| DAT | `1098/Tibia.dat` | 3,562,090 | `26849789b8aef15de8576943ba889c09093c444cd22259d8526f5dd63b4f462b` |
| SPR | `1098/Tibia.spr` | 573,255,184 | `9613e64bd13c7ccefbfc9de80057e9a12c3bf27d3c00bb8dec043c62fe30be9b` |
| OTB | `items(1).otb` | 1,540,256 | `196168beaebedf2a43c46b4af37874c5c53d29eb0fe5ccd3e2f7856d04a8cf30` |
| OTBM | `mapa/global_dash.otbm` | 53,704,504 | `e298ed36554bbcc8ff256c61396eb761e7e170ae7e0feb81d041e9a445cbf90c` |
| houses | `mapa/map-house.xml` | 14,603 | external fixture |
| spawns | `mapa/map-spawn.xml` | 804,966 | external fixture |

Observed binary signatures:

- DAT: `0x000042A3`;
- SPR: `0x57BBD603`;
- OTB description: `OTB 3.5`;
- OTBM numeric format header: `2` (editor OTBM v3 family);
- real OTBM identifier: legacy `0x00000000`;
- real physical root node type: `0` (RME/BlackTek writer-compatible);
- this SPR is the RGBA/alpha-channel variant: colored RLE pixels use 4 bytes (RGBA), not 3 bytes (RGB).

Do not substitute another DAT/SPR pair under the same profile without changing the fingerprint record.

## Complete-project probe — PASS

The exact pinned pack was loaded through the same `LegacyMapProjectLoader` used by the Studio. Result:

```text
PROJECT success=yes profile="pokefans1098" map="global_dash.otbm"
size=30000x30000 canonical_tiles=6106271
houses=125 spawn_areas=6025 towns=3 waypoints=0 asset_registry=38425
PROJECT dat_signature=0x42a3 spr_signature=0x57bbd603
spr_count=575585 warnings=862 errors=0
```

This proves the real large-map **read -> asset resolution -> canonical MapDocument** gate.

The initial materializing OTBM reader exceeded practical memory because it retained the nodal tree, neutral import model and canonical map at once. The real-project loader now uses `OtbmStreamReader` plus `LegacyCanonicalMapBuilder`, emitting tiles directly into canonical storage.

Measured on the Linux homologation runner/container:

| Build | Peak RSS | Wall time | Result |
| --- | ---: | ---: | --- |
| streaming loader before compact Tile storage | ~2.48 GB | ~21.5 s | PASS |
| streaming loader + compact sparse Tile/Item storage | ~1.52 GB | ~35.7 s | PASS |

The compact representation reduces peak memory by about 39%. Further performance/memory tuning may continue, but the real-read gate is satisfied.

## Real sprite render — PASS

The first visual probe exposed a real compatibility defect: `SprReader` assumed three bytes per colored pixel. The pinned SPR uses the OTClient alpha-channel form and stores four bytes per colored pixel. Structural probing of the first 64 non-empty sprites produced `0/64` valid RGB records and `64/64` valid RGBA records, so the reader now detects and decodes both legacy variants.

After the fix, `fantasy-legacy-preview --screenshot` was executed on the exact pinned pack through SDL's offscreen/dummy backend at 1440x900. Result:

```text
CENTER 714 787 7
RENDER visited=1581 ground=1581 items=624 missing=0 cached=89
```

The rendered evidence shows aligned ground, walls/borders, tall objects, furniture and alpha transparency with no missing textures in the captured viewport. The screenshot itself is intentionally not committed because it contains third-party game assets; the reproducible statistics and pack fingerprints are retained here.

This marks the **real DAT/SPR renderer gate PASS**. Interactive V5 UX/manual acceptance remains separate from decoder/render correctness.

## Real edit smoke — PASS

The inspector executed the canonical editing path directly on the loaded `global_dash` document without writing the source map:

1. select a real tile;
2. preview a ground brush without mutation;
3. apply the brush through the shared Command API;
4. Undo / Redo / Undo;
5. capture selection to clipboard;
6. paste with Replace mode;
7. Undo / Redo / Undo;
8. cut selected tile;
9. Undo and verify exact restoration.

Result:

```text
EDIT_SMOKE PASS center=714,787,7 target=719,785,7
ground_from=34168 ground_to=34116 paste_target=718,785,7
initial_revision=0 final_revision=10 tiles=6106271
```

This proves real-data selection, brush preview/apply, revisioned Undo/Redo, clipboard paste/cut and exact restoration against the 6.1M-tile canonical map.

## OTBM Writer + semantic reopen — PASS

After the real edit gate passed, the Fantasy OTBM v3 writer was exercised against the exact canonical `global_dash` document. The source map was never overwritten; the inspector refuses source/output path equality and wrote a new file.

Writer result:

```text
WRITE_OTBM success=yes
 tiles=6106271
 items=6499901
 areas=277
 bytes=64919912
```

The generated file was reopened immediately through `LegacyMapProjectLoader` with the same OTB/DAT/SPR/XML profile. Result:

```text
REOPEN PASS
 tiles=6106271
 houses=125
 spawn_areas=6025
 towns=3
 waypoints=0
 sample=equal
```

Local homologation output fingerprint:

```text
SHA-256 ae640e8b3a27b8ef108de56673c5b991ac99de5d6cfacb7516479ea2cbf74bed
```

This is a **Fantasy semantic roundtrip PASS**.

## Vanilla TFS 1.4.2 load — PASS

The generated OTBM was then loaded by the official upstream TFS v1.4.2 Ubuntu GCC release (`31d6e85d`) using the pinned 10.98 `items.otb` profile.

Because this execution environment did not include a MariaDB daemon, a minimal local MySQL-protocol startup harness supplied only the mandatory bootstrap/config replies and empty persistence results. The harness does not parse OTBM, OTB, map XML or items and therefore cannot make an incompatible map pass `IOMap`.

Relevant TFS output:

```text
The Forgotten Server - Version v1.4.2
Git SHA1 31d6e85d dated 2022-05-08T20:27:14-04:00

>> Loading items
>> Loading map
> Map size: 30000x30000.
> Map loading time: 2.897 seconds.
>> Loaded all modules, server starting up...
>> Forgotten Server Online!
```

A repeat with the house/spawn XML files placed under the exact filenames preserved by the OTBM metadata produced a map load time of `2.757 s`; after correcting the filenames there were no house/spawn *file-not-found* errors.

The vanilla datapack then warned that project-specific Pokémon/NPC definitions such as `Charizard`, `Dragonite`, `Lucario` and custom NPC XML files were missing. Those warnings are expected content gaps in vanilla TFS, not OTBM compatibility failures. Their appearance confirms that TFS had already accepted the generated map and proceeded to process the real project spawn/NPC references.

Detailed runtime evidence: `docs/TFS142-OTBM-RUNTIME-HOMOLOGATION.md`.

## Gate status

1. **PASS** — Windows/Linux CI and CTests green on the renderer/bridge foundation.
2. **PASS** — complete-project probe on the exact real 10.98 pack.
3. **PASS** — real DAT/SPR render with RGBA detection, bounded viewport traversal and `missing=0`.
4. **PASS (code path)** — the same `LegacyWorkspaceSession` + renderer is embedded in the V5 Map workspace; interactive/manual V5 visual acceptance can still be repeated on Windows.
5. **PASS** — real selection, brush preview/apply, Undo/Redo and clipboard operations.
6. **PASS** — OTBM v3 writer creates a new file and Fantasy reopens it with semantic counts/sample preserved.
7. **PASS** — official vanilla TFS 1.4.2 loads the generated OTBM and reaches server Online.
8. **PENDING** — compatible 10.98 client logs in, enters the world and walks the generated map.
9. After that gate, grow the `Tfs1098` runtime backend around the proven export/runtime path.
