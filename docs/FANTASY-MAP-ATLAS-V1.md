# Fantasy Map Atlas V1

Status: implemented, awaiting C++ compilation and small-map acceptance.

The Atlas consumes the existing loader and canonical model. It is a derived index,
not a replacement map, runtime or persistence system. It never reads items.xml.

## Gates

Build Studio targets `fantasy-legacy-inspect` and `fantasy-map-atlas-test`.
Run `fantasy-map-atlas-test <empty-output-directory>` and then run
`python Tools/MapAtlas/test_build_atlas_sqlite.py`.

Export a small generated map using `fantasy-legacy-inspect --project` with all six
source paths and `--atlas-export <empty-raw-directory>`. Build its database with
`python Tools/MapAtlas/build_atlas_sqlite.py --input <raw> --output <new-directory>/small.sqlite`.
Check canonical/export/database counts, nesting, attributes and acceptance queries
before exporting global_dash. Never substitute empty auxiliary XMLs on the real map.

## Contracts

- CSVs stream from sparse MapStorage traversal. Sorting one 64-row stripe at a
  time gives deterministic z/y/x IDs without buffering the whole map.
- Every canonical ground, top-level item and nested content is emitted. Depth is
  zero for ground/top-level; stack indexes are zero based within their roles.
- Statistics count uses including contents; uniqueTiles counts once per asset per
  tile. Registry misses retain IDs and use kind=unknown. No ID is invented.
- The first 50 uses per asset in z/y/x, stack, depth-first order are examples.
- Spatial relations use distinct ground/top-level assets per tile, exclude
  nested contents, and encode keys as serverId:clientId. Cooccurrence counts
  unordered different-asset pairs. Adjacency counts E/S only, retaining direction
  and same-asset neighbors.
- Extra sameHouseCount on adjacency requires both tiles to share a nonzero
  houseId; houseTileCount on cooccurrence counts house tiles. These support the
  requested house-scoped evidence queries.
- Query 5 accepts an explicit hypothetical wall ID and searches candidates
  between two instances on the same axis and house. It does not label assets.
- Attribute CSV supports int64, string and Position, with CSV quote/newline
  escaping. Source paths are relative to the manifest directory.
- Nonempty outputs are refused. C++ publishes a sibling .partial directory only
  after completion. Python streams CSVs in 5000-row batches, validates counts,
  references, indexes and SQLite integrity, checks six source hashes before/after,
  and records source/CSV/database hashes in atlas-build.json. Failed databases
  remain .partial; retries require a fresh output directory.

Source hashes must also be checked before/after a complete real-map session.
The C++ exporter does not replace loader diagnostics or validate TFS play/visuals.
No real-map PASS may be claimed before its export and SQL gates run.
