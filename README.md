# New Fantasy

Novo ciclo do Fantasy Studio, reconstruído com uma fundação limpa e compatível com o ecossistema TFS 1.4.2 / protocolo 10.98.

## Objetivo

Transformar um Map Editor compatível com 10.98 em um Studio completo capaz de criar, editar, testar e publicar um jogo próprio, mantendo o fluxo familiar do ecossistema OTServer e adicionando ferramentas modernas, automação e suporte ao Codex.

## Princípios

- Uma única stack oficial e congelada após homologação.
- Estrutura de pastas simples, previsível e sem duplicações.
- Caminhos relativos; nenhum caminho absoluto deve ser salvo no projeto.
- O mapa principal fica sempre em `Game/Maps/world.otbm`.
- Cada domínio do jogo tem uma pasta própria.
- O Studio abre automaticamente a pasta correta para cada ferramenta.
- A GUI e o Codex devem usar o mesmo núcleo de operações do Studio.
- Nada de misturar Crystal, Canary, 15.24 ou múltiplas versões de TFS neste ciclo.
- Primeiro marco: `Open Project -> Play -> servidor inicia -> cliente abre -> personagem entra no mapa`.

## Stack candidata da F00

| Componente | Base candidata |
| --- | --- |
| Servidor | The Forgotten Server 1.4.2 / 10.98 |
| Map Engine | Remere's Map Editor clássico v3.7, compatível com 10.98 |
| Cliente | OpenTibiaBR OTClient, commit homologado para TFS 1.4.2 / 10.98 |
| Mapa | OTBM v3 / 10.98 |
| Assets | DAT + SPR + OTB 10.98 |
| Scripts | Lua / Revscriptsys |
| Banco | MariaDB |
| Plataforma primária | Windows x64 |

Os SHAs candidatos estão em `docs/UPSTREAMS.md`. Nenhuma dependência externa será tratada como base oficial até passar pela F00 de homologação e pela revisão de licença aplicável.

## Estrutura

```text
New-fantasy/
├── Studio/
├── Game/
│   ├── Maps/
│   ├── Content/
│   ├── Assets/
│   ├── Scripts/
│   └── Config/
├── Server/
├── Client/
├── Database/
├── Tools/
├── Projects/
├── scripts/
└── docs/
```

Consulte `docs/ARCHITECTURE.md` e `docs/ROADMAP.md` antes de alterar a estrutura.

## Estado

**F00 — FOUNDATION / PREPARATION: IN_PROGRESS**

O repositório contém inicialmente o contrato de arquitetura, estrutura limpa, regras para Codex e plano de homologação. O próximo passo técnico é trazer as três bases candidatas para um ambiente de homologação, compilar e executar o gate completo antes de incorporá-las ao produto.