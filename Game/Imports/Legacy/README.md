# Local legacy map imports

This directory is reserved for source maps and sidecar files used during compatibility/migration work.

The first F05.5 target is PokeFans 10.98, for example:

```text
Game/Imports/Legacy/PokeFans1098/
├── global_dash.otbm
├── map-house.xml
└── map-spawn.xml
```

Source files are treated as immutable inputs. Record SHA-256 before and after an import/open session and never save over the original OTBM.

Do not commit third-party OTBM/XML content unless redistribution rights are explicitly established.
