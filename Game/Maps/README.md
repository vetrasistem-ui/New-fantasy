# Maps

Local oficial de todos os mapas do jogo.

A fonte de verdade do mundo principal é:

```text
Game/Maps/World/world.fmap.json
```

O Fantasy Studio deve abrir esse mapa diretamente através de `Open Main Map`, sem procurar arquivos pelo projeto.

## Estado atual do mundo de desenvolvimento

O `world.fmap.json` deixou de ser o fixture mínimo de dois tiles. Durante F03 ele representa uma pequena área 8 × 8 com 64 tiles distribuídos em quatro chunks, incluindo grama, terra, estrada, pedra, areia, água e alguns objetos semânticos. Isso permite validar viewport, minimapa, seleção e operações atravessando limites de chunk sem transformar o mapa principal em um arquivo enorme.

Há também um fixture específico para regras multi-chunk:

```text
Game/Maps/World/multichunk-fixture.fmap.json
```

As regras de coordenadas estão congeladas em `Shared/Formats/FMAP/README.md`.

## Regras

- FMAP é o formato nativo.
- OTBM não é fonte de verdade.
- Regiões/chunks futuros permanecem sob `Game/Maps/World/`.
- IDs de assets devem ser semânticos sempre que possível.
- arquivos gerados/runtime não devem substituir a fonte FMAP.
- importação OTBM futura deve produzir FMAP e encerrar a dependência depois da conversão.
- chunks são armazenamento; a posição semântica de um tile é resolvida por `region.origin + chunk.offset + tile.local`.
