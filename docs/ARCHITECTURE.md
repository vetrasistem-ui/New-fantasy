# Architecture — New Fantasy

## Visão geral

O New Fantasy é organizado em quatro camadas principais:

```text
Fantasy Studio
      │
Fantasy Project Model
      │
Adapters / Automation API
      │
Server + Client + Map Engine
      │
Game Data
```

A aplicação nasce a partir de um Map Engine compatível com 10.98, mas o editor de mapas é apenas o primeiro módulo do Studio.

## Regra central

A interface gráfica e o Codex não devem ter motores separados.

```text
                 Fantasy Core
                     │
        ┌────────────┼────────────┐
        │            │            │
       GUI          CLI          Codex
        │            │            │
        └────────────┼────────────┘
                     │
                 Game Data
```

Para mapas:

```text
GUI / Script / Codex
        │
Fantasy Map API
        │
Map Engine
        │
IOMapOTBM
        │
Game/Maps/world.otbm
```

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
├── Database/
│   ├── Migrations/
│   └── Seeds/
├── Tools/
├── Projects/
├── scripts/
└── docs/
```

Pastas internas podem crescer, mas a raiz não deve virar um depósito de arquivos.

## Fantasy Project Model

O Studio nunca deve depender de busca manual para localizar recursos principais. `fantasy.project.json` define os caminhos oficiais e todos são relativos à raiz.

O usuário deve conseguir mover o projeto de `C:\Fantasy` para `D:\Fantasy` e continuar abrindo-o sem editar arquivos.

## Conteúdo

A organização lógica padrão é:

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

O Studio pode gerar/adaptar dados para o TFS, mas não deve espalhar a fonte de verdade do conteúdo em locais arbitrários.

## Adapters

Cada editor trabalha com um modelo Fantasy e um adapter de runtime.

```text
Monster Editor
      │
Fantasy Monster Model
      │
TFS Monster Adapter
      │
Runtime data
```

O mesmo princípio vale para Items, NPCs, Spells, Quests e outros domínios.

## Automação e IA

A automação deve entrar cedo no projeto. Operações planejadas:

```text
fantasy project validate
fantasy map open
fantasy map save
fantasy map set-ground
fantasy map add-item
fantasy map fill-area
fantasy map place-template
fantasy build
fantasy play
```

A primeira implementação pode ser CLI. Comunicação em tempo real pode vir depois.

## Segurança de edição

Operações grandes, especialmente geradas por IA, devem ser transacionais ou gerar snapshot. O usuário deve poder comparar, aceitar ou desfazer alterações em lote.

## Regra de compatibilidade

A plataforma 1.0 suporta uma única família de runtime: TFS 1.4.2 / protocolo 10.98. Suporte a outras versões não faz parte do escopo inicial.
