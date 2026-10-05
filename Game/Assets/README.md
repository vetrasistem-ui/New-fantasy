# Assets

Assets do jogo e catálogo semântico Fantasy.

## Estratégia atual

Nesta etapa o Fantasy reutiliza **DAT + SPR + OTB** como fonte prática de assets para acelerar o desenvolvimento do Map Editor e do Client sem precisar recriar todo o conteúdo visual do zero.

Essa decisão é de **compatibilidade e bootstrap**, não de dependência arquitetural permanente.

O FMAP e a lógica de alto nível continuam trabalhando com **chaves semânticas Fantasy**, e não com IDs soltos de Tibia.

Exemplo conceitual:

```text
terrain.grass.basic -> legacy item/sprite mapping
nature.tree.oak.small -> legacy item/sprite mapping
```

## Regra arquitetural

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

O `Legacy Asset Bridge` é responsável por ler os formatos legados e fornecer metadados/sprites ao catálogo Fantasy. Studio e Client devem consumir o mesmo catálogo.

O FMAP não deve persistir caminhos físicos de sprite, offsets internos de arquivo ou IDs legados como contrato de domínio. Esses detalhes ficam encapsulados na camada de assets.

## Objetivo de curto prazo

Usar os assets legados para:

- renderizar grounds reais no Map Editor;
- renderizar borders/objects reais;
- disponibilizar uma palette inicial de assets;
- mostrar o mesmo mapa com os mesmos sprites no Fantasy Client;
- validar seleção, posicionamento, Save/Reopen e movimento com conteúdo visual real.

## Evolução futura

O projeto poderá gradualmente substituir qualquer asset legado por conteúdo nativo Fantasy sem mudar o FMAP ou as APIs de alto nível.

Importadores/migração completa, atlas próprios, animações, efeitos e ferramentas avançadas continuam reservados ao Asset Pipeline futuro.

Os arquivos DAT/SPR/OTB usados no desenvolvimento devem ser fornecidos pelo ambiente do usuário/projeto e mantidos fora do repositório quando não houver autorização para redistribuição.
