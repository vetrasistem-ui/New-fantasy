# F00 — Independent Core Foundation

A F00 agora prova que New Fantasy possui contratos próprios antes de avançar para mapa visual, rede real ou gameplay.

## Objetivo

Criar uma fundação mínima que não dependa de OTBM, TFS ou protocolo 10.98 no caminho nativo.

## Ambiente alvo

- Windows x64 como plataforma primária.
- C++20 + CMake para o Fantasy Server inicial.
- Git.
- PowerShell para gates locais.
- Referências externas disponíveis opcionalmente em `.upstream/`.

## 00A — Layout e contratos

1. Validar a estrutura oficial com `scripts/check-layout.ps1`.
2. Criar `Shared/Protocol/`.
3. Criar `Shared/Formats/FMAP/`.
4. Garantir que `fantasy.project.json` use caminhos relativos.

PASS: layout e manifestos são determinísticos e não exigem arquivos espalhados.

## 00B — FMAP v0

Criar um formato fonte mínimo contendo:

- schema/version;
- world id/name;
- tile size;
- region/chunk metadata;
- posição de spawn de desenvolvimento;
- tiles/objects semânticos mínimos.

Fixture oficial: `Game/Maps/World/world.fmap.json`.

PASS: o arquivo valida, pode ser lido novamente e mantém equivalência semântica.

## 00C — Fantasy Protocol v1 mínimo

Definir em `Shared/Protocol/` pelo menos:

- envelope/version;
- Hello / HelloAck;
- LoginDev / LoginOk;
- EnterWorld;
- MapChunk;
- EntityAdd;
- EntityMove;
- EntityRemove;
- Disconnect/Error.

Nesta fase o contrato pode ser especificação/schema; rede real entra depois.

PASS: IDs, campos e versões não são ambíguos e existem fixtures de mensagens.

## 00D — Fantasy Server skeleton

Criar um executável próprio que:

1. compile em C++20;
2. informe versão/build;
3. carregue configuração mínima;
4. tenha ciclo Start -> Ready -> Stop limpo;
5. não dependa de código TFS.

PASS: CI compila e executa smoke test.

## 00E — Boundary com referências legadas

TFS 1.4.2, RME 3.7 e OTClient 10.98 podem ser baixados com o script de upstream apenas para:

- comparar comportamento;
- estudar fixtures;
- validar importadores/adapters futuros;
- servir de fallback temporário.

Não copiar código deles para o core nativo durante F00.

## 00F — Freeze dos contratos v0/v1

Registrar:

- schema FMAP inicial;
- versão inicial do Fantasy Protocol;
- toolchain do servidor;
- comandos de build;
- fixtures;
- resultados PASS/FAIL;
- limitações conhecidas.

## Critério de saída

F00 vira **PASS** somente quando:

```text
layout PASS
FMAP fixture PASS
Protocol contract PASS
Fantasy Server build PASS
Fantasy Server smoke PASS
```

Não é necessário login real ou cliente gráfico para fechar F00.

## Fora da F00

- UI final do Studio;
- editor visual completo;
- banco de produção;
- combate;
- monsters/quests;
- website;
- VPS;
- streaming de chunks em rede;
- compatibilidade completa com mapas antigos.

A F00 congela a linguagem básica do ecossistema. O produto funcional cresce a partir da F01.