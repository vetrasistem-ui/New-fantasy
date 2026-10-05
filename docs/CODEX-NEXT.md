# Codex — Start Here

Este é o ponto único de entrada para a próxima execução do Codex no `New-fantasy`.

## Estado entregue pelo trabalho de repositório

A arquitetura nativa já está implementada até o primeiro play técnico:

```text
Fantasy Studio → FMAP
Fantasy Client → TCP → Fantasy Protocol v1 → Fantasy Server → WorldRuntime → FMAP
```

O caminho nativo não depende de OTBM, TFS runtime, protocolo 10.98, DAT/SPR/OTB ou OTClient.

Fases:

```text
F00 PASS
F01 PASS
F02 PASS
F03 PASS — gate visual real e Save/Reopen concluídos
F04 PASS
F05 PASS — first play visual, reconnect e cleanup concluídos
F06 PREPARED / NOT STARTED
```

## Missão do Codex agora

**Não desenvolver novas features. Não iniciar F06.**

Os gates C00–C07 de Windows já passaram em 2026-10-05; o fechamento C08
está publicado no commit `c414819d5c98034dcef1f8e666ad6310b3874908`.
Foundation Checks run 218 permanece PENDING / QUEUED durante o incidente
de alocação de runners reportado pelo GitHub; confirmar o CI antes de avançar.
F06 é a próxima prioridade documentada, mas permanece NOT STARTED
e exige uma nova instrução de execução. Evidência:

```text
docs/evidence/F05/WINDOWS-VALIDATION.md
docs/evidence/F03/RESULT.md
docs/evidence/F05/RESULT.md
```

O roteiro reproduzível dos gates concluídos está em:

```text
docs/CODEX-F05-WINDOWS.md
```

Esse arquivo preserva o contrato de execução completo e a ordem validada:

```text
C00  checkout limpo / SHA
C01  build nativo completo
C02  first play headless em dois processos + reconnect
C03  fechar gate visual do Fantasy Studio
C04  first play visual do Fantasy Client
C05  reconnect visual + cleanup de processos
C06  registrar evidências
C07  regressão completa final
C08  somente então fechar F03/F05 na documentação
```

## Regras de execução

1. Trabalhar sobre `main` atualizado e registrar o SHA inicial.
2. Não pular etapas.
3. Se uma etapa falhar, parar nela.
4. Corrigir a menor camada possível.
5. Executar os testes diretos da correção.
6. Reexecutar a etapa inteira que falhou.
7. Reexecutar regressões afetadas.
8. Registrar erro, diagnóstico e commit da correção.
9. Nunca contornar um problema voltando para TFS/OTBM/10.98.
10. Nunca declarar PASS para teste visual/processo que não foi realmente executado.

## Arquivos que devem permanecer fontes de verdade

```text
AGENTS.md
fantasy.project.json
Shared/Formats/FMAP/
Shared/Protocol/
docs/DECISIONS.md
docs/ROADMAP.md
docs/CODEX-F05-WINDOWS.md
```

Não criar cópias paralelas de FMAP, protocolo, Server, Client ou mapa.

## Resultado esperado da execução

Se C00–C07 passarem, Codex deve produzir evidência em:

```text
docs/evidence/F03/windows/
docs/evidence/F05/windows/
docs/evidence/F05/WINDOWS-VALIDATION.md
```

Somente depois:

- F03 passa para `PASS`;
- F05 passa para `PASS`;
- `AGENTS.md` muda a prioridade para F06;
- `docs/ROADMAP.md` é atualizado;
- nenhuma implementação F06 entra no mesmo commit de fechamento.

Commit final recomendado para o fechamento:

```text
test(windows): close F03 and F05 native-play gates
```

Depois do push, aguardar GitHub Actions no SHA exato e registrar o run/SHA nas evidências.

## Já preparado para depois do fechamento

A F06 já está planejada, mas bloqueada até C08 e o CI do SHA final passarem.

Somente depois disso o Codex pode usar:

```text
docs/F06-PERSISTENCE-PLAN.md
docs/CODEX-AFTER-F05.md
```

Esses arquivos definem PersistenceService/store independente de banco, primeiro adapter SQLite, migrations, contas locais de desenvolvimento, personagens, save/load de posição e o gate de restart/reconnect. Não antecipar essa implementação durante a validação F05.
