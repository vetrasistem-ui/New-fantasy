# AGENTS.md — New Fantasy

Este arquivo define as regras permanentes para Codex e outros agentes que trabalhem neste repositório.

## Missão

Construir o Fantasy Studio sobre uma única stack 10.98 homologada, mantendo o projeto limpo, previsível, testável e preparado para automação por IA.

## Regras obrigatórias

1. Não alterar a stack oficial sem registrar a decisão em `docs/DECISIONS.md`.
2. Não introduzir Crystal, Canary, protocolo 15.x, appearances 15.x ou uma segunda versão de TFS.
3. Não criar caminhos absolutos em código, configuração ou projeto. Todos os caminhos persistidos devem ser relativos à raiz do projeto.
4. Não duplicar mapa, assets, cliente, servidor ou arquivos de conteúdo em múltiplas pastas.
5. O mapa principal padrão é `Game/Maps/world.otbm`.
6. Mapas ficam somente em `Game/Maps/`.
7. Conteúdo de jogo fica somente em `Game/Content/`.
8. Assets do jogo ficam somente em `Game/Assets/`.
9. Código/base do servidor fica em `Server/`; código/base do cliente fica em `Client/`; código do Studio fica em `Studio/`.
10. Não salvar arquivos gerados de build, logs, cache ou bancos locais no Git.
11. Toda nova função deve ter um caminho claro de validação. Não considerar uma fase PASS apenas porque compila.
12. Alterações no Map Engine devem preservar Open/Edit/Undo/Redo/Save/Reopen.
13. GUI e automação devem chamar o mesmo núcleo de operações sempre que possível. Não criar lógica de mapa exclusiva para a interface.
14. Operações em lote ou geradas por IA devem ser reversíveis por transaction/undo ou snapshot antes de serem liberadas.
15. Nenhum editor novo deve escrever em locais arbitrários do TFS. O Fantasy Project Model é a fonte organizada; adapters cuidam da integração com o runtime.
16. Não incorporar código de terceiros ao produto antes de confirmar a licença e registrar a origem em `docs/UPSTREAMS.md`.

## Estrutura de conteúdo

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

## Gate mínimo para cada mudança

- layout do projeto continua válido;
- nenhum caminho absoluto novo;
- build relevante passa;
- testes relevantes passam;
- arquivos abrem após mover a pasta do projeto;
- se tocar em mapas: abrir, editar, salvar e reabrir sem corrupção;
- se tocar em runtime: Start/Stop não deixa processos órfãos;
- se tocar em conteúdo: referências e IDs são validados.

## Prioridade atual

F00: homologar TFS 1.4.2 + RME 10.98 + OTClient compatível + MariaDB em conjunto. Não desenvolver editores avançados antes deste gate.
