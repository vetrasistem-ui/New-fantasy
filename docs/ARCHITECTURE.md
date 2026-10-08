# Architecture — New Fantasy

## Visão geral

Fantasy é uma plataforma própria. O Studio, os modelos de domínio, o Map Core, o Asset Core e a automação não dependem da implementação interna de um runtime específico.

A arquitetura alvo é:

```text
Fantasy Studio
      |
Fantasy Data Model / Core
      |
Runtime Backend Contract
      |
 +----+-------------------+
 |                        |
TFS 1.4.2 / 10.98   Fantasy Native Runtime
(first supported)       (future)
```

TFS 1.4.2, RME/BlackTek e clientes legados são referências externas de comportamento e compatibilidade. TFS 1.4.2 é também o **primeiro runtime suportado**, através de uma camada própria de exportação/adaptação. Ele não é a fonte de verdade do projeto Fantasy.

A estratégia detalhada está em `docs/FANTASY-RUNTIME-STRATEGY.md`.

## Regra central

GUI, CLI, scripts e IA compartilham o mesmo núcleo de domínio.

```text
                 Fantasy Core
                     |
        +------------+------------+
        |            |            |
       GUI          CLI           IA
        |            |            |
        +------------+------------+
                     |
              Fantasy Data Model
                     |
             Runtime Backend
```

Nenhuma dessas superfícies deve alterar estruturas do mapa diretamente fora das APIs canônicas de comando/transação.

## Limite de runtime

`Studio/Runtime/RuntimeBackend.hpp` define o limite neutro entre o produto Fantasy e seus runtimes.

O Core não inclui headers, classes ou hierarquias internas do TFS.

```text
PROIBIDO
Fantasy Core -> TFS::Player / TFS::Creature / TFS::Map

PERMITIDO
Fantasy Project
    -> Fantasy model
    -> TFS1098 exporter/adapter
    -> OTBM/XML/Lua/config/DB migrations
    -> external TFS process
```

No futuro, o mesmo modelo Fantasy poderá ser consumido pelo runtime nativo sem alterar o projeto fonte.

## Mapas

O modelo fonte nativo é o Fantasy Map Model/MapDocument. O formato persistido nativo continuará evoluindo como FMAP.

```text
GUI / Script / IA
        |
Command + Query API
        |
Fantasy Map Model / MapDocument
        |
       FMAP
        |
 +------+------------------+
 |                         |
OTBM exporter         Fantasy Native
(TFS backend)           runtime
```

OTBM é formato de interoperabilidade e homologação. Durante a fase atual, leitura e escrita OTBM v3 são gates obrigatórios porque o primeiro runtime suportado é TFS 1.4.2/10.98.

### FMAP

O FMAP deve ser:

- semântico em vez de depender apenas de IDs numéricos;
- versionado por schema;
- dividido por regiões/chunks quando necessário;
- amigável a diff/Git;
- fácil de criar por IA;
- determinístico;
- validável antes de entrar em qualquer runtime.

A persistência final pode usar JSON estruturado, formato compilado ou ambos, sem alterar o modelo interno.

## Assets

DAT/SPR/OTB e outros formatos legados são entradas de compatibilidade.

```text
DAT / SPR / OTB / PNG / outros
          |
    Fantasy Importers
          |
 Fantasy Asset Registry
          |
 future Fantasy Asset Format
```

Depois de importado, o conteúdo deve poder ser referenciado por identidade Fantasy, sem exigir que o runtime nativo conheça DAT/SPR/OTB.

## Protocolo

No curto prazo, a homologação usa um cliente/runtime compatível com protocolo 10.98 por meio do backend TFS.

O protocolo nativo Fantasy permanece um objetivo futuro para Fantasy Client + Fantasy Server. Ele não deve bloquear a entrega do Studio nem a homologação do primeiro runtime.

## Layout oficial

```text
New-fantasy/
├── Studio/
│   ├── Core/
│   ├── MapEngine/
│   ├── Project/
│   ├── Runtime/
│   ├── Automation/
│   ├── Rendering/
│   └── UI/
├── Game/
│   ├── Maps/
│   ├── Content/
│   ├── Assets/
│   ├── Scripts/
│   └── Config/
├── Server/                # native runtime experiments/future
├── Client/                # native client experiments/future
├── Shared/
│   ├── Protocol/
│   └── Formats/
├── Database/
├── Tools/
├── Projects/
├── scripts/
└── docs/
```

A raiz deve permanecer pequena. Pastas novas na raiz exigem justificativa arquitetural.

## Fantasy Project Model

`fantasy.project.json` contém somente caminhos relativos e contratos do projeto. O projeto deve continuar funcional depois de mover sua pasta para outro disco ou PC.

O arquivo do projeto não deve conter dependências em tipos C++ do TFS. A escolha de runtime é configuração de alvo, não identidade do projeto.

Exemplo conceitual:

```text
project.runtime = tfs1098
```

ou futuramente:

```text
project.runtime = fantasy-native
```

## Conteúdo

```text
Game/Content/
├── Items/
├── Monsters/
├── NPCs/
├── Quests/
├── Spells/
├── Classes/
├── Skills/
├── Loot/
└── Systems/
```

Os editores trabalham com modelos Fantasy próprios. O backend TFS converte esses modelos para contratos suportados pelo runtime externo quando necessário.

## Primeiro runtime — TFS 1.4.2 / 10.98

O caminho mais curto para produto utilizável é:

```text
Fantasy Studio
      |
Fantasy Core
      |
TFS1098 Backend
      |
OTBM v3 + XML + Lua/config
      |
TFS 1.4.2
      |
compatible 10.98 client
```

O backend deve permanecer isolado. Se TFS for distribuído, suas obrigações de licença são tratadas no componente TFS distribuído; isso não autoriza copiar sua implementação para o Fantasy Core.

## Fantasy Native Runtime

O runtime próprio cresce incrementalmente e não substitui o TFS antes de provar capacidade suficiente.

```text
V0: mapa + assets + player + movimento + colisão + câmera
V1: entidades + itens + inventário + efeitos + combate
V2: multiplayer autoritativo + persistência + NPC + quests + systems
```

O trabalho nativo deve reutilizar os modelos Fantasy e não exigir reautoria do projeto.

## Cliente

No curto prazo, um cliente 10.98 compatível é ferramenta de homologação do backend TFS.

No longo prazo, Fantasy Client consome o runtime/protocolo nativo e os assets Fantasy. O cliente legado não dita a arquitetura final.

## Automação e IA

Operações planejadas:

```text
fantasy project validate
fantasy map validate
fantasy map set-ground
fantasy map add-object
fantasy map fill-area
fantasy map place-template
fantasy runtime package --target tfs1098
fantasy runtime start --target tfs1098
fantasy build
fantasy play
```

Alterações grandes devem gerar transaction/diff e permitir Preview / Accept / Undo.

## Legacy labs

PokeJornadas, PokeAimar e mapas/asset packs compatíveis são laboratórios reais para importação, stress, migração e validação. BlackTek/RME são referências de comportamento do editor.

Nenhuma dessas bases define o Core Fantasy.

## Regra de independência

O objetivo não é remover o TFS imediatamente. O objetivo é garantir que ele seja **substituível**.

A regra que guia o desenvolvimento é:

```text
Fantasy Project -> Fantasy Core -> Runtime Backend
```

Hoje:

```text
Runtime Backend -> TFS 1.4.2 / 10.98
```

Futuramente:

```text
Runtime Backend -> Fantasy Native Runtime
```

Se a troca de backend puder acontecer sem reescrever o projeto, a independência arquitetural foi alcançada.
