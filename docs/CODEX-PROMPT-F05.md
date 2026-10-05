# Prompt para Codex — fechamento organizado F03/F05

Use este texto para iniciar a próxima sessão do Codex no repositório `vetrasistem-ui/New-fantasy`.

---

Você está trabalhando no repositório **New-fantasy**. Sua missão nesta sessão é **validar e fechar F03/F05 em Windows real**, sem iniciar F06 e sem mudar a arquitetura.

Antes de qualquer alteração:

1. leia `AGENTS.md`;
2. leia `docs/CODEX-NEXT.md`;
3. leia integralmente `docs/CODEX-F05-WINDOWS.md`;
4. leia `docs/DECISIONS.md` e `docs/ROADMAP.md` apenas para confirmar os limites da fase;
5. execute as etapas **C00 → C08 exatamente na ordem** definida em `docs/CODEX-F05-WINDOWS.md`.

Arquitetura congelada para esta validação:

```text
Fantasy Studio → FMAP
Fantasy Client GUI → TCP → Fantasy Protocol v1 → Fantasy Server → WorldRuntime → FMAP
```

Regras obrigatórias:

- não usar OTBM no caminho nativo;
- não usar TFS runtime, protocolo 10.98, DAT/SPR/OTB ou OTClient para contornar falhas;
- Server permanece autoritativo;
- Client envia apenas intenção de movimento `MoveRequest(direction)`;
- não criar parser/modelo FMAP paralelo;
- não criar protocolo/rede paralelos dentro da GUI;
- `LoginDev` e listener F05 permanecem somente loopback/local;
- não iniciar persistência/F06 nesta sessão;
- não declarar PASS para gate que não foi realmente executado;
- se algo falhar: pare na etapa, identifique a camada, faça a menor correção possível, rode testes diretos, reexecute a etapa inteira e as regressões afetadas, registre a falha e a correção.

Evidência obrigatória deve ser preenchida em:

```text
docs/evidence/F05/WINDOWS-VALIDATION.md
```

Screenshots devem ficar em:

```text
docs/evidence/F03/windows/
docs/evidence/F05/windows/
```

O fechamento formal só pode ocorrer após C00–C07 PASS reais. Só então, em C08:

- marcar F03 como PASS;
- marcar F05 como PASS;
- atualizar `docs/ROADMAP.md`;
- atualizar evidências F03/F05;
- mudar a prioridade de `AGENTS.md` para F06;
- **não implementar F06 no mesmo commit**;
- fazer push e aguardar GitHub Actions no SHA exato;
- registrar run/SHA final nas evidências.

Comece agora pela **C00 — Clean checkout and baseline** e prossiga etapa por etapa. Não pule diretamente para testes visuais e não faça refatorações fora do escopo.

---
