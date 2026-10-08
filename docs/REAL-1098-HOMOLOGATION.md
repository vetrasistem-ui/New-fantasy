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

`LegacyWorkspaceSession` is the reusable boundary intended for the permanent V5 Map workspace. The standalone preview uses exactly this session so renderer/loader defects can be isolated without creating a second map engine.

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
- OTBM numeric format header: `2` (editor OTBM v3 family).

Do not substitute another DAT/SPR pair under the same profile without changing the fingerprint record.

## Complete-project probe

After building the Studio target:

```powershell
.\fantasy-legacy-inspect.exe --project `
  --profile pokefans1098 `
  --otbm C:\path\mapa\global_dash.otbm `
  --otb C:\path\items(1).otb `
  --dat C:\path\1098\Tibia.dat `
  --spr C:\path\1098\Tibia.spr `
  --house C:\path\mapa\map-house.xml `
  --spawn C:\path\mapa\map-spawn.xml
```

The probe must return exit code `0` and report `PROJECT success=yes` before the real visual gate can be marked PASS.

## Visual preview

```powershell
.\fantasy-legacy-preview.exe `
  --profile pokefans1098 `
  --otbm C:\path\mapa\global_dash.otbm `
  --otb C:\path\items(1).otb `
  --dat C:\path\1098\Tibia.dat `
  --spr C:\path\1098\Tibia.spr `
  --house C:\path\mapa\map-house.xml `
  --spawn C:\path\mapa\map-spawn.xml
```

The preview uses the same canonical `MapDocument`, `LegacyWorkspaceSession`, real DAT/SPR texture cache and canvas renderer planned for the V5 Map workspace. It is a temporary homologation surface, not a second editor UI.

## Gate order

1. CI builds and CTests remain green.
2. Complete-project probe passes on this exact pack.
3. Preview opens the real map and renders real sprites with bounded viewport traversal.
4. The same session is embedded in the V5 Map workspace.
5. Real selection, command edits, clipboard and brushes are exercised.
6. Only then implement the OTBM v3 Writer.
7. Save -> reopen -> TFS 1.4.2 -> compatible 10.98 client.
