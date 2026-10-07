# Fantasy Map Engine — arquitetura moderna e AI-ready

Status: arquitetura oficial em construção para o Fantasy Studio sobre TFS 1.4.2 / protocolo 10.98.

## Objetivo

Usar BlackTek MapEditor/RME como referência funcional de um editor OpenTibia maduro, sem reproduzir sua arquitetura interna. O Fantasy deve conservar as capacidades úteis — mapa esparso, seleção, ações, clipboard, brushes, autoborder, houses, spawns, OTBM e assets — mas separá-las em serviços modernos e determinísticos que possam ser usados igualmente pela interface V5, automações e agentes de IA.

BlackTek permanece REFERENCE ONLY enquanto a compatibilidade de licença não estiver resolvida. Não copiar código GPL para o core Fantasy.

## Regra principal

**UI, IA e automação nunca alteram `MapStorage` diretamente.**

Todo edit passa pela mesma cadeia:

```text
Human UI / AI / Automation
          |
          v
      Command API
          |
          v
      Validator
          |
          v
   Preview / Map Diff
          |
          v
   Transaction Executor
          |
          v
      MapStorage
          |
          +--> ActionHistory / Undo / Redo
          +--> Dirty Regions
          +--> Renderer refresh
          +--> Audit Log
```

Isso garante que uma edição feita por IA tenha as mesmas regras, validação e capacidade de desfazer de uma edição feita com mouse.

## Comparação estrutural

### BlackTek

O BlackTek concentra muitas responsabilidades em `Editor`: mapa, seleção, action queue, copy buffer, draw/undraw e live editing. O modelo é funcional e maduro, mas possui forte acoplamento com a aplicação wxWidgets e usa estruturas históricas de C++/RME.

### Fantasy

O Fantasy separa responsabilidades:

```text
Studio/MapEngine/
  Core/
    MapTypes
    MapStorage
    SelectionModel
    ActionHistory
    MapRevision

  Commands/
    MapCommand
    CommandExecutor
    CommandValidator
    MapDiff

  Brushes/
    BrushDefinition
    BrushEngine
    GroundRules
    BorderRules
    WallRules
    VariationRules

  Clipboard/
    TileSnapshot
    ClipboardService
    PastePlanner

  Query/
    MapQueryService
    SpatialQuery
    FindReplace

  Formats/
    OtbmReaderAdapter
    OtbmWriterAdapter
    HousesXmlAdapter
    SpawnsXmlAdapter

  Rendering/
    MapViewport
    ViewportQuery
    SpriteCache
    MapRenderer

  AI/
    AiMapAdapter
    AiContextBuilder
    CommandSchema
    CommandAudit
```

Assets 10.98 continuam fora do Map Core e entram por contratos estáveis:

```text
DAT + SPR + OTB
      |
      v
FantasyAssetRegistry
      |
      v
Map Renderer / Inspector / Brush catalog
```

## 1. Domain Core

O domínio não conhece SDL, ImGui, wxWidgets, OpenAI, JSON ou o cliente Tibia.

Tipos principais:

- `Position`
- `Item`
- `Tile`
- `House`
- `Waypoint`
- `CreaturePlacement`
- `SpawnPlacement`
- `MapStorage`

O armazenamento deve ser esparso e indexado espacialmente. A implementação inicial Fantasy usa chunks de 64x64 por floor. A API pública deve acessar por coordenada e viewport, nunca exigir varredura completa do mapa.

## 2. Command API

Toda mudança é representada por um comando serializável e determinístico.

Exemplos conceituais:

```json
{
  "type": "paint_ground",
  "requestId": "ai-001",
  "expectedRevision": 42,
  "positions": [[100, 200, 7], [101, 200, 7]],
  "asset": { "serverId": 4526 },
  "preview": true
}
```

```json
{
  "type": "apply_brush",
  "brush": "terrain.grass",
  "shape": "rectangle",
  "from": [100, 200, 7],
  "to": [120, 220, 7],
  "autoborder": true,
  "preview": true
}
```

A IA não deve inventar operações fora do schema. Primeiro consulta capacidades e assets disponíveis, depois envia comandos válidos.

## 3. Preview obrigatório para ações grandes

O executor deve poder executar em modo `preview` sem alterar o mapa.

Resposta conceitual:

```json
{
  "valid": true,
  "revision": 42,
  "affectedTiles": 317,
  "createdTiles": 28,
  "changedItems": 104,
  "warnings": [],
  "diffId": "preview-8f23"
}
```

A mesma operação pode então ser aplicada sobre a revisão esperada.

Isso serve tanto para IA quanto para ferramentas humanas como Replace All.

## 4. Revisionamento otimista

Cada mapa/documento possui um `revision` crescente.

Um comando pode declarar `expectedRevision`.

Se o mapa mudou desde que a IA analisou a região, o comando deve ser rejeitado em vez de editar coordenadas baseadas em contexto antigo.

Resultado esperado:

```text
REVISION_CONFLICT
expected: 42
current: 45
```

A IA consulta novamente a região e reprojeta a ação.

## 5. Map Diff

Toda operação produz um diff tipado:

- tiles criados;
- tiles removidos;
- tiles modificados;
- itens adicionados/removidos;
- houses alteradas;
- spawns alterados;
- waypoints alterados;
- bounding box afetada.

O diff alimenta simultaneamente:

- Undo/Redo;
- renderer incremental;
- preview visual;
- auditoria;
- respostas para IA;
- testes.

## 6. Selection Model

A seleção é domínio de edição, não estado exclusivo da UI.

Deve suportar:

- tile único;
- multi-selection;
- retângulo;
- seleção por filtro/query;
- itens específicos;
- creatures/spawns;
- bounds.

Assim uma IA pode executar semanticamente:

```text
select all tiles inside house 27
replace item 4526 with 4771 in selection
```

sem simular mouse.

## 7. Brush Engine data-driven

BlackTek/RME usa classes especializadas e XML de materials/brushes. O Fantasy preserva o conceito, mas os brushes devem expor definições de dados independentes da UI.

Tipos iniciais:

- Ground Brush
- Border Brush
- Wall Brush
- Door Brush
- Doodad Brush
- Raw Item Brush
- Eraser
- House Brush
- House Exit
- Spawn Brush
- Creature Brush

Cada brush deve informar:

- id estável;
- nome;
- categoria;
- assets usados;
- tamanho/shape permitido;
- regras de vizinhança;
- regras de variação;
- se precisa recalcular bordas;
- validações.

Isso permite à IA perguntar `list_brushes(category=terrain)` e aplicar um brush pelo ID estável.

## 8. Autoborder e wall rules

Autoborder não pode ficar escondido em lógica de UI.

Deve ser um serviço determinístico que recebe região + vizinhança + regras e devolve um `MapDiff`.

O mesmo vale para wall alignment e portas.

Isso possibilita operações de alto nível:

```text
crie uma ilha de grama 20x30 cercada de areia
```

A IA planeja a geometria, chama brushes e o engine resolve as peças corretas de borda.

## 9. Query API para IA

A IA precisa observar o mapa sem receber o mapa inteiro.

Queries previstas:

- `get_tile(x,y,z)`
- `get_region(rect,z)`
- `summarize_region(rect,z)`
- `find_items(serverId, bounds?)`
- `find_by_attribute(key,value,bounds?)`
- `list_houses()`
- `get_house(id)`
- `list_spawns(bounds?)`
- `list_assets(query/category)`
- `list_brushes(category)`
- `get_selection()`
- `get_map_bounds()`

Resultados devem ser compactos, pagináveis e usar IDs/posições estáveis.

## 10. AI adapter

`AiMapAdapter` não contém inteligência de mapa. Ele traduz ferramentas externas para Command/Query API.

Fluxo:

```text
Natural language
      |
      v
AI planner
      |
      +--> Query API
      |
      v
structured command plan
      |
      v
preview
      |
      v
validate/apply
```

Dessa forma o Map Engine funciona sem IA e qualquer modelo futuro pode usar o mesmo contrato.

## 11. Segurança de edição

Ações de IA devem ter:

- limites de número de tiles por comando;
- preview para operações acima do limite configurado;
- revisão esperada;
- validação de IDs/assets;
- bloqueio de coordenadas inválidas;
- transação atômica;
- Undo completo;
- provenance/origin (`human`, `ai`, `automation`);
- request ID;
- log de comandos.

Não permitir que a IA execute ponteiros, scripts C++ ou mutações arbitrárias dentro do processo do editor.

## 12. OTBM como boundary adapter

O Map Core não deve ser a árvore binária OTBM.

```text
OTBM -> OtbmReaderAdapter -> MapStorage
MapStorage -> OtbmWriterAdapter -> OTBM v3
```

O adapter deve preservar atributos necessários ao TFS 1.4.2/10.98 e reportar atributos desconhecidos sem descartá-los silenciosamente.

Houses e spawns auxiliares seguem adapters próprios para `map-house.xml` e `map-spawn.xml`.

## 13. Renderer independente

O renderer consulta somente a área visível:

```text
Camera/Viewport
      |
      v
ViewportQuery
      |
      v
visible chunks/tiles
      |
      v
SpriteCache
      |
      v
SDL_Renderer3
```

Nenhuma edição deve depender do renderer estar ativo. Isso permite testes headless e execução de comandos por IA/CLI.

## 14. Headless Map Engine

O objetivo explícito é que o futuro Fantasy possua uma ferramenta headless:

```text
fantasy-map-cli project.fantasy query ...
fantasy-map-cli project.fantasy preview commands.json
fantasy-map-cli project.fantasy apply commands.json
fantasy-map-cli project.fantasy validate
```

A UI V5 será apenas um cliente privilegiado do mesmo engine.

## 15. Ordem de implementação

### Foundation A — Map Core

1. tipos reais de mapa;
2. storage esparso/chunked;
3. selection model;
4. action history;
5. revision + MapDiff;
6. command executor;
7. query service.

### Foundation B — 10.98

8. DAT/SPR/OTB registry completo;
9. OTBM reader -> MapStorage;
10. houses/spawns XML;
11. sprite cache;
12. viewport renderer real.

### Foundation C — edição madura

13. clipboard;
14. brush engine;
15. ground/autoborder;
16. walls/doors;
17. houses/spawns/creatures;
18. item properties;
19. find/replace;
20. minimap.

### Foundation D — persistência

21. OTBM writer v3;
22. roundtrip;
23. TFS 1.4.2 gate;
24. client 10.98 gate.

### Foundation E — AI

25. command schema externo;
26. headless CLI;
27. AI context/query adapter;
28. preview visual de comandos AI;
29. audit/provenance;
30. end-to-end AI edit -> OTBM -> TFS.

## Gate final do Map Editor V1

O Map Editor V1 só é PASS quando:

```text
PokeJornadas/PokeFans map + DAT/SPR/OTB
              |
              v
        Fantasy Studio V5
              |
       human OR AI edits
              |
              v
        OTBM v3 + XML
              |
              v
          TFS 1.4.2
              |
              v
        client 10.98
```

E a alteração realizada no Studio aparece corretamente no jogo, com save/reopen e Undo/Redo comprovados.
