# Architecture — New Fantasy

## Visão geral

A plataforma alvo possui cinco blocos próprios:

```text
Fantasy Studio
      │
Fantasy Data Model
      │
 ┌────┼──────────────┐
 │    │              │
FMAP  Fantasy Protocol  Content Contracts
 │    │              │
 └────┼──────────────┘
      │
Fantasy Server ↔ Fantasy Client
```

TFS 1.4.2, RME 3.7 e OTClient 10.98 são referências externas de comportamento e compatibilidade durante a transição. Eles ficam isolados em `.upstream/` e não são a arquitetura final.

## Regra central

GUI, CLI, scripts e Codex compartilham o mesmo núcleo de domínio.

```text
                 Fantasy Core
                     │
        ┌────────────┼────────────┐
        │            │            │
       GUI          CLI          Codex
        │            │            │
        └────────────┼────────────┘
                     │
              Fantasy Data Model
```

## Mapas

O formato fonte nativo é FMAP.

```text
GUI / Script / Codex
        │
Fantasy Map API
        │
Fantasy Map Model
        │
Game/Maps/World/world.fmap.json
        │
Validator / Compiler
        │
FMAPC (runtime futuro)
        │
Fantasy Server
```

OTBM pode existir como importador/exportador legado enquanto for útil, mas não é a fonte de verdade.

### FMAP

O FMAP deve ser:

- semântico em vez de depender apenas de IDs numéricos;
- versionado por schema;
- dividido por regiões/chunks quando necessário;
- amigável a diff/Git;
- fácil de criar por IA;
- determinístico;
- validável antes de entrar no runtime.

A primeira versão usa JSON estruturado. Uma DSL própria pode ser adicionada depois sem alterar o modelo interno.

## Protocolo

O contrato nativo é o Fantasy Protocol.

```text
Fantasy Client
      │
Fantasy Protocol v1
      │
Fantasy Server
```

O protocolo deve ser versionado e definido primeiro em `Shared/Protocol/`. O servidor é autoritativo.

Durante a transição, um adapter 10.98 pode existir para comparação ou fallback, mas o primeiro grande marco nativo deve funcionar sem ele.

## Layout oficial

```text
New-fantasy/
├── Studio/
│   ├── Core/
│   ├── MapEngine/
│   ├── Project/
│   ├── Runtime/
│   ├── Automation/
│   └── UI/
├── Game/
│   ├── Maps/
│   │   └── World/
│   ├── Content/
│   ├── Assets/
│   ├── Scripts/
│   └── Config/
├── Server/
│   ├── Core/
│   ├── Modules/
│   ├── Config/
│   └── Logs/
├── Client/
│   ├── Core/
│   ├── Modules/
│   ├── UI/
│   └── Assets/
├── Shared/
│   ├── Protocol/
│   └── Formats/
│       └── FMAP/
├── Database/
├── Tools/
├── Projects/
├── scripts/
└── docs/
```

A raiz deve permanecer pequena. Pastas novas na raiz exigem justificativa arquitetural.

## Fantasy Project Model

`fantasy.project.json` contém somente caminhos relativos e contratos do projeto. O projeto deve continuar funcional depois de mover sua pasta para outro disco ou PC.

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

Os editores trabalham com modelos Fantasy próprios. Runtime e formatos externos, quando existirem, são adapters e não a fonte de verdade.

## Servidor

O Fantasy Server é uma implementação própria. O core inicial deve crescer em módulos pequenos e testáveis:

```text
Server/Core
├── App
├── Network
├── Protocol
├── World
├── Entities
├── Map
├── Scheduler
└── Persistence
```

Gameplay de alto nível poderá usar Lua/configuração estruturada mais tarde. O core não deve nascer acoplado a TFS.

## Cliente

O alvo final é um Fantasy Client falando Fantasy Protocol. Durante a transição, um cliente 10.98 pode ser usado como referência/oráculo, mas não dita o contrato final.

## Automação e IA

Operações planejadas:

```text
fantasy project validate
fantasy map validate
fantasy map set-ground
fantasy map add-object
fantasy map fill-area
fantasy map place-template
fantasy server start
fantasy build
fantasy play
```

Alterações grandes devem gerar transaction/snapshot e permitir Preview / Accept / Undo.

## Regra de independência

Nenhum componente legado é obrigatório no caminho final:

```text
FMAP → Fantasy Server → Fantasy Protocol → Fantasy Client
```

Esse é o contrato que guia o desenvolvimento.