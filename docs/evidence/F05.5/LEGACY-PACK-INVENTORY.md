# F05.5 — Inventário do pack 10.98

Status: **SOURCE PACK RECEIVED / COMPATIBILITY REVIEWED**

Este registro documenta os arquivos fornecidos pelo usuário para a homologação TFS 1.4.2 / 10.98. Os binários não são adicionados ao repositório; permanecem fornecidos localmente.

## Arquivos recebidos

### Assets 10.98 — conjunto principal candidato

`1098(2).zip`

- `1098/Tibia.dat`
  - tamanho: 3,562,090 bytes
  - SHA-256: `26849789b8aef15de8576943ba889c09093c444cd22259d8526f5dd63b4f462b`
- `1098/Tibia.spr`
  - tamanho: 573,255,184 bytes
  - SHA-256: `9613e64bd13c7ccefbfc9de80057e9a12c3bf27d3c00bb8dec043c62fe30be9b`

O header do DAT indica um conjunto maior/customizado que o par `1098_semtransparencia` existente no pack_mapper, portanto os dois pares não devem ser misturados sem validação visual.

### Mapa

`mapa.zip`

- `mapa/global_dash.otbm`
  - tamanho: 53,704,504 bytes
  - SHA-256: `e298ed36554bbcc8ff256c61396eb761e7e170ae7e0feb81d041e9a445cbf90c`
  - metadata detectada: `Saved with Remere's Map Editor 3.5`
  - OTBM map version detectada: `2` no header, correspondente a OTBM v3 na nomenclatura do editor
  - item major version detectada: `3`
  - item minor/client id detectado: `57`
- `mapa/map-house.xml`
  - tamanho: 14,603 bytes
- `mapa/map-spawn.xml`
  - tamanho: 804,966 bytes

A combinação `item major=3 / minor=57` coincide com a definição de client `10.98` no `clients.xml` de referência.

### OTB customizado fornecido separadamente

`items(1).otb`

- tamanho: 1,540,256 bytes
- SHA-256: `196168beaebedf2a43c46b4af37874c5c53d29eb0fe5ccd3e2f7856d04a8cf30`
- header inclui `OTB 3.5`

Este arquivo **não é byte-a-byte igual** ao `rme-3.5/data/1098/items.otb` do pack mapper, que possui 1,512,148 bytes e SHA-256 `d7cedecfa1a3f1c83b1aa256f4a86ff98713739a4f3d11acdba96caf7d640dc2`.

Conclusão: o `items(1).otb` deve ser tratado como candidato customizado principal e validado em conjunto com o mapa e o DAT/SPR principal antes de congelar o perfil.

### Pack mapper / referência RME

`pack_mapper.zip`

Contém:

- RME 3.5 (`pack_mapper/rme-3.5/RME.exe`);
- `rme-3.5/data/1098/` com `items.otb`, `items.xml`, `grounds.xml`, `borders.xml`, `doodads.xml`, `materials.xml`, `tilesets.xml`, `walls.xml`, `creatures.xml`;
- `clients.xml` com definição explícita de `10.98`, OTB version `3`, id `57`;
- um segundo par de assets em `1098_semtransparencia/`:
  - `Tibia.dat` — 3,025,545 bytes, SHA-256 `468f20e8fe7da1ce9a19c89a723f33631f0778e34149447d4e3400e8b3636886`;
  - `Tibia.spr` — 331,204,742 bytes, SHA-256 `9ecaf1324d45a1e5bad61041d7b947bbd1ec14d694b32e2d8f0a2351603209e7`.

Os dois pares DAT/SPR são diferentes. O perfil F05.5 não deve combinar DAT de um par com SPR do outro.

### Ferramentas auxiliares

`ObjectBuilder_edited.zip`

- contém `ObjectBuilder.exe` e `ObjectBuilder.swf`;
- útil como ferramenta/oráculo para inspeção e edição de DAT/SPR;
- não entra no core do Fantasy.

`item editor 10.98 32 bits.zip`

- contém `ItemEditor.exe` e plugins;
- útil como ferramenta/oráculo para inspeção de OTB;
- não entra no core do Fantasy.

## Referência adicional: BlackTek MapEditor

A auditoria de 2026-10-06 adicionou como principal referência comportamental:

```text
Black-Tek/BlackTek-MapEditor
d429c7a4334774983c652bf21764edb396a03c02
```

Ele confirma um perfil 10.98 com OTBM v3, DAT/SPR/OTB, houses/spawns e ferramentas de edição próximas do que precisamos. O código permanece REFERENCE ONLY por enquanto; ver `docs/BLACKTEK-MAPEDITOR-1098-AUDIT.md`.

## Decisão operacional para a F05.5

Primeiro perfil a validar:

```text
Mapa:     global_dash.otbm
Houses:   map-house.xml
Spawns:   map-spawn.xml
DAT:      1098(2)/Tibia.dat
SPR:      1098(2)/Tibia.spr
OTB:      items(1).otb
Referências: BlackTek MapEditor + RME 3.5
Runtime hard gate: TFS 1.4.2 vanilla
```

Esse conjunto é **candidato**, não congelado. O perfil só será marcado como canônico após:

1. abrir o mapa com o conjunto completo sem erro de versão;
2. confirmar visualmente que ground/borders/objects correspondem ao esperado;
3. verificar que não há IDs fora do catálogo OTB/DAT sem diagnóstico;
4. comparar com o segundo par `1098_semtransparencia` quando necessário;
5. registrar o conjunto escolhido por hashes exatos;
6. salvar um OTBM de teste e carregá-lo no TFS 1.4.2 vanilla.

## Regra atual de uso

O Fantasy não altera os arquivos originais durante inspeção. A sequência alvo agora é:

```text
OTBM + DAT/SPR/OTB 10.98
          ↓
Fantasy readers + asset registry
          ↓
Fantasy Studio V5
          ↓ edit / Save As
OTBM v3 + houses/spawns
          ↓
TFS 1.4.2
          ↓
Client 10.98
```

FMAP não é requisito desta homologação. A implementação FMAP existente fica preservada como experimento anterior e pode ser reutilizada no futuro, mas não substitui OTBM no gate atual.
