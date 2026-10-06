# BlackTek MapEditor audit — TFS 1.4.2 / 10.98

Date: 2026-10-06
Status: **AUDIT COMPLETE / ADOPTION PLAN DEFINED**

## Baseline oficial desta auditoria

O New Fantasy passa a tratar como baseline operacional atual:

- Server: **The Forgotten Server 1.4.2** (`otland/forgottenserver`, tag `v1.4.2`, commit `31d6e85de2a86fb3f0e36c63509fba75b855b8bd`)
- Protocol: **10.98**
- Map family: **OTBM v3** (header version 2 no arquivo)
- Asset family: **DAT 10.57-era + SPR + OTB** usada pelo perfil 10.98
- Studio UI: Fantasy Studio V5 em **SDL3 + SDL_Renderer3 + Dear ImGui**
- Projeto real de homologação: PokeFans/PokeJornadas 10.98, sem transformar o Studio em produto específico de Poketibia

Crystal 4.1.8 / protocolo-assets 15.24 não participa desta arquitetura.

## Referência auditada

### BlackTek MapEditor

- Repository: `Black-Tek/BlackTek-MapEditor`
- Branch: `master`
- Commit auditado: `d429c7a4334774983c652bf21764edb396a03c02`
- Papel: **principal oráculo comportamental do Map Editor 10.98**

### Relação com TFS 1.4.2

O BlackTek Server declara que nasceu sobre TFS 1.4.2 e mantém protocolo Tibia 10.98. O MapEditor possui perfil 10.98 explícito, usa `data/1098`, configura `otbm version="3"` e carrega DAT/SPR/OTB.

O OTBM v3 do editor corresponde ao header numérico 2, que é aceito pelo loader do TFS 1.4.2.

## Componentes relevantes encontrados

### Core de mapa

- `map.*`
- `basemap.*`
- `tile.*`
- `item.*`
- `items.*`
- `complexitem.*`
- `position.*`
- `town.*`
- `house.*`
- `spawn.*`
- `waypoints.*`

### IO / compatibilidade 10.98

- `iomap_otbm.*`
- `client_version.*`
- `graphics.*`
- `sprites.h`
- `item_attributes.*`
- `filehandle.*`

### Operações de edição

- `action.*`
- `selection.*`
- `copybuffer.*`
- `editor.*`

O `ActionQueue` mantém batches de alterações com commit, undo e redo e serve como referência importante para garantir uma entrada de histórico por gesto significativo.

### Brushes / ferramentas

- `ground_brush.*`
- `wall_brush.*`
- `doodad_brush.*`
- `raw_brush.*`
- `eraser_brush.*`
- `carpet_brush.*`
- `table_brush.*`
- `creature_brush.*`
- `house_brush.*`
- `house_exit_brush.*`
- `spawn_brush.*`
- `waypoint_brush.*`

### Visual/editor

- `map_drawer.*`
- `map_display.*`
- `graphics.*`
- `palette_*`
- `minimap_window.*`

Esses módulos são referência de comportamento. O Fantasy Studio não adotará wxWidgets nem substituirá sua UI V5 pelo frontend do BlackTek/RME.

## Compatibilidade confirmada que interessa ao New Fantasy

### 1. Perfil 10.98 real

`data/clients.xml` define 10.98 como perfil padrão e visível, usando `data_directory="1098"`, OTBM 3 e DAT format 10.57.

### 2. Dados de edição 10.98

`data/1098/` contém regras de borders, grounds, walls, doodads, tilesets, creatures e OTB de referência. O changelog registra atualizações específicas para 10.98, inclusive wall brushes.

### 3. OTBM compatível com TFS 1.4.2

Os node types e atributos OTBM padrão do BlackTek/RME coincidem com a família esperada pelo TFS 1.4.2. O caminho de compatibilidade que deve ser provado é:

```text
DAT + SPR + OTB
       ↓
Fantasy Studio
       ↓
OTBM v3 + map-house.xml + map-spawn.xml
       ↓
TFS 1.4.2
       ↓
Protocol 10.98
       ↓
Client 10.98
```

### 4. Houses e spawns

O editor lê e grava houses/spawns em XML auxiliares, seguindo o mesmo conceito esperado pelo TFS 1.4.2.

### 5. Assets

O editor possui loading de metadata DAT, sprite data SPR e mapeamento `items.otb`, que serve como oráculo para o nosso pipeline próprio `serverId -> clientId -> appearance -> sprite`.

## Extensões BlackTek que NÃO entram automaticamente

BlackTek evoluiu além do TFS 1.4.2 puro. Portanto, os seguintes recursos ficam fora da baseline até prova de compatibilidade e decisão explícita:

1. **Zone IDs / Zone Brush / TOML de zonas**
   - existem no MapEditor e no BlackTek Server;
   - não fazem parte do TFS 1.4.2 vanilla.

2. **OTBM attribute map 128**
   - o MapEditor consegue serializar um mapa genérico de atributos;
   - o TFS 1.4.2 vanilla não declara esse atributo em seu `iomap.h`;
   - o Fantasy não deve gravá-lo no caminho vanilla sem teste de carga no TFS.

3. **Live map editing protocol do RME/BlackTek**
   - útil como referência futura;
   - não entra no primeiro gate.

4. Recursos específicos do BlackTek Server
   - qualquer feature que dependa do runtime BlackTek não pode ser confundida com compatibilidade TFS 1.4.2.

## Licença e proveniência

Há uma inconsistência que impede copiar código automaticamente:

- arquivos-fonte auditados carregam cabeçalho **GNU GPL v3 ou posterior**;
- o arquivo `LICENSE.rtf` na raiz contém uma EULA antiga que fala em distribuição apenas da forma não modificada.

Até a estratégia de licença ser explicitamente resolvida:

- BlackTek MapEditor é **REFERENCE ONLY**;
- não copiar arquivos-fonte para `Studio/`, `Shared/`, `Server/` ou `Client/`;
- qualquer comportamento reimplementado deve manter rastreabilidade de referência;
- os readers atuais do Fantasy continuam como implementação própria/isolada;
- nenhuma distribuição final deve depender de código BlackTek copiado sem decisão de licença.

## Comparação com o estado atual do New Fantasy

A branch `feature/f05.5-pokefans-1098` já contém leitores próprios para:

- SPR;
- DAT;
- OTB;
- OTBM;
- `LegacyAssetRegistry`;
- modelo neutro de leitura;
- testes determinísticos.

Essa base deve ser preservada e validada diferencialmente contra BlackTek/RME e contra o loader real do TFS 1.4.2.

## Decisão de arquitetura resultante

### Ativo agora

```text
Fantasy Studio V5
      │
      ├── Map tools próprios
      ├── readers/writers próprios 10.98
      └── asset resolver próprio
               │
        OTBM v3 / DAT / SPR / OTB
               │
            TFS 1.4.2
               │
          Protocol 10.98
               │
          Client 10.98
```

### Referências

1. **BlackTek MapEditor** — referência principal de Map Editor 10.98.
2. **RME 3.7** — referência histórica/secundária e comparação de comportamento.
3. **TFS 1.4.2** — fonte de verdade para o que o runtime deve aceitar.

### Congelado, não apagado

- FMAP como runtime principal;
- Fantasy Server próprio;
- Fantasy Protocol próprio;
- Fantasy Client/protocolo de primeiro-play nativo.

Esses componentes permanecem como experimento técnico já realizado e podem ser reaproveitados futuramente, mas não comandam a baseline atual.

## Plano técnico imediato

### BT00 — Pin e matriz de compatibilidade

- fixar SHAs de TFS 1.4.2 e BlackTek MapEditor;
- registrar formatos permitidos;
- bloquear extensões BlackTek não homologadas.

### BT01 — Differential OTBM reader

Para a mesma fixture/mapa 10.98:

- abrir no BlackTek;
- abrir no reader Fantasy;
- comparar width/height, tiles, floors, item server IDs, flags, houses, towns e waypoints;
- depois carregar o mesmo arquivo no TFS 1.4.2.

### BT02 — Asset resolution

Validar:

```text
OTB serverId
   ↓
DAT clientId/appearance
   ↓
SPR sprite/frame
```

com diagnóstico explícito para qualquer item não resolvido.

### BT03 — Render real

- abrir o mapa PokeFans 10.98;
- renderizar ground/border/object com sprites reais no Studio V5;
- manter viewport, inspector, minimapa e navegação existentes.

### BT04 — OTBM writer mínimo

Implementar writer próprio apenas para o subconjunto TFS 1.4.2 necessário:

- OTBM v3;
- tile flags padrão;
- items e atributos padrão;
- towns/waypoints;
- houses/spawns auxiliares;
- sem Zone TOML e sem attribute-map 128 no primeiro gate.

### BT05 — Roundtrip real

```text
OTBM original
   ↓ open
Fantasy Studio
   ↓ edit
OTBM novo
   ↓ load
TFS 1.4.2
```

O TFS precisa carregar sem erro e sem perda silenciosa de conteúdo suportado.

### BT06 — Play 10.98

- subir TFS 1.4.2;
- conectar cliente 10.98;
- entrar no mapa editado;
- movimentar;
- reconnect e shutdown limpos.

### BT07 — Map Editor homologado

Validar sobre mapa real:

- paint/erase;
- ground + autoborder quando implementado;
- objects/raw;
- selection/copy/paste;
- Undo/Redo;
- floors;
- houses;
- spawns/creatures;
- minimap;
- Save/Reopen;
- TFS load após edição.

## Gate de sucesso

O Map Engine do New Fantasy só será considerado homologado para esta baseline quando:

1. abrir um OTBM 10.98 real;
2. resolver DAT/SPR/OTB reais;
3. renderizar sprites reais;
4. editar e salvar OTBM v3;
5. preservar houses/spawns necessários;
6. o arquivo salvo carregar no TFS 1.4.2 vanilla;
7. um cliente 10.98 entrar e jogar no mapa resultante;
8. nenhuma extensão específica do BlackTek for exigida para esse fluxo;
9. a UI V5 do Fantasy Studio continuar sendo a superfície do produto.
