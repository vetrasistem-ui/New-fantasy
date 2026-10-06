# Assets

Assets do jogo e catálogo semântico Fantasy.

## F05.5 — primeiro perfil real

O primeiro alvo de compatibilidade é **PokeFans 10.98** usando um conjunto local correspondente de:

```text
Tibia.dat
Tibia.spr
items.otb
```

O mapa legado relacionado é tratado separadamente pelo bridge OTBM. O pack deve permanecer fora do repositório quando não houver autorização explícita de redistribuição.

## Arquitetura

```text
DAT / SPR / OTB
      ↓
Legacy Asset Bridge
      ↓
Fantasy Asset Registry
      ↓
semantic key
      ↓
Studio + Client
```

O FMAP e a lógica de alto nível continuam trabalhando com **chaves semânticas Fantasy**, e não com sprite offsets ou IDs físicos como contrato de domínio.

Quando um item legado ainda não possui alias amigável, a F05.5 usa bootstrap determinístico:

```text
legacy.pokefans1098.item.<serverId>
```

Exemplo:

```text
legacy.pokefans1098.item.4526
```

Aliases futuros podem mapear essa origem para algo como `terrain.grass.basic` sem mudar o arquivo físico nem espalhar IDs pelo mapa.

## Profile local esperado

O código não deve depender de nomes físicos fixos. Um profile resolve caminhos relativos, por exemplo:

```text
Game/Assets/Legacy/PokeFans1098/Tibia.dat
Game/Assets/Legacy/PokeFans1098/Tibia.spr
Game/Assets/Legacy/PokeFans1098/items.otb
```

O ambiente do usuário pode apontar para outros nomes/locais relativos de trabalho.

## Responsabilidades do registry

- registrar a origem semântica de um asset;
- resolver semantic key ↔ proveniência legado;
- manter metadata neutra compartilhável;
- não armazenar `SDL_Texture*`, cache do renderer ou offsets persistentes de arquivo no domínio;
- não serializar pixels/cache no FMAP.

`Shared/Assets/LegacyAssetRegistry.*` contém o contrato inicial compartilhado. Readers DAT/SPR/OTB ficam na camada de compatibilidade e alimentam esse registry.

## Objetivo visual imediato

Usar o pack local para:

- renderizar grounds reais no Map Editor;
- renderizar borders/objects reais;
- preencher Items & Assets com sprites reais;
- permitir seleção/picker e edição sobre um mapa OTBM real;
- posteriormente mostrar o mesmo conteúdo no Fantasy Client pelo mesmo registry.

## Evolução futura

F05.5 é bootstrap/migração. O Asset Pipeline definitivo continua reservado à F13: formato nativo, atlas, animações, efeitos, substituição progressiva do legado e ferramentas avançadas.
