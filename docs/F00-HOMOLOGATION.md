# F00 — Homologation Plan

A F00 existe para impedir que o novo Studio repita o problema de integrar componentes incompatíveis antes de provar o ciclo completo.

## Ambiente alvo

- Windows x64 como plataforma primária de desenvolvimento.
- Git disponível.
- Toolchain de C++ e dependências definida por cada upstream durante a prova inicial.
- MariaDB local para homologação.

## Ordem de execução

### 00A — Server

1. Obter `otland/forgottenserver` no SHA registrado em `docs/UPSTREAMS.md`.
2. Compilar sem alterações Fantasy.
3. Importar schema limpo.
4. Iniciar e encerrar de forma controlada.
5. Registrar comando de build, dependências e hash do binário.

PASS somente se o build for reproduzível e Start/Stop não depender de passos manuais obscuros.

### 00B — Map Engine

1. Obter `hampusborgos/rme` no SHA registrado.
2. Compilar sem modificações Fantasy.
3. Configurar dados 10.98 compatíveis.
4. Abrir fixture OTBM.
5. Alterar um tile/objeto conhecido.
6. Undo/Redo.
7. Save As.
8. Fechar e reabrir.
9. Confirmar semanticamente a alteração e a preservação do restante.

### 00C — Client

1. Obter `opentibiabr/otclient` no SHA registrado.
2. Compilar sem modificações Fantasy.
3. Configurar protocolo/endpoint 10.98.
4. Conectar no TFS homologado.
5. Receber lista/personagem e entrar no mapa.

### 00D — End-to-end

1. Abrir fixture no RME.
2. Fazer alteração visível.
3. Salvar.
4. Iniciar TFS usando o mapa salvo.
5. Entrar pelo OTClient.
6. Confirmar visualmente a alteração.

### 00E — Move-folder / relative paths

Após montar o workspace de teste, mover a raiz para outro caminho e repetir Open/Build/Play sem editar caminhos persistidos.

### 00F — Freeze

Registrar:

- commits finais;
- versões de toolchain;
- versão MariaDB;
- asset pack/hash;
- comandos de build;
- fixtures de teste;
- resultados PASS/FAIL;
- licenças e obrigações aplicáveis.

## Critério de saída

F00 somente pode virar **PASS** quando `Server + Database + Client + Map + Assets` forem testados juntos. Sucessos isolados não fecham a fase.

## O que não fazer na F00

- redesign visual grande;
- Monster Editor;
- Quest Editor;
- VPS;
- site;
- migração de assets 15.24;
- integração com Crystal/Canary;
- mudanças de gameplay.

A F00 prova a fundação; o produto começa a crescer depois dela.
