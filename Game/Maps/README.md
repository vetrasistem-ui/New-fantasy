# Maps

Local oficial de todos os mapas do jogo.

A fonte de verdade do mundo principal é:

```text
Game/Maps/World/world.fmap.json
```

O Fantasy Studio deve abrir esse mapa diretamente através de `Open Main Map`, sem procurar arquivos pelo projeto.

## Regras

- FMAP é o formato nativo.
- OTBM não é fonte de verdade.
- Regiões/chunks futuros permanecem sob `Game/Maps/World/`.
- IDs de assets devem ser semânticos sempre que possível.
- arquivos gerados/runtime não devem substituir a fonte FMAP.
- importação OTBM futura deve produzir FMAP e encerrar a dependência depois da conversão.

O fixture inicial de F00 já existe em `Game/Maps/World/world.fmap.json`.
