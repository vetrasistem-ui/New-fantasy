# AGENTS.md — New Fantasy

Este arquivo define regras permanentes para Codex e outros agentes que trabalhem neste repositório.

## Missão

Construir uma plataforma Fantasy própria: Studio, formato de mapa, servidor, protocolo e cliente, mantendo o projeto limpo, testável e preparado para automação por IA.

## Regras obrigatórias

1. Não mudar decisões estruturais sem registrar a mudança em `docs/DECISIONS.md`.
2. TFS 1.4.2, RME 3.7 e OTClient 10.98 são referências isoladas em `.upstream/`; não tratá-los como fonte de verdade do produto.
3. Não copiar código de terceiros para o core Fantasy sem decisão explícita, revisão de licença e registro em `docs/UPSTREAMS.md`.
4. Não criar caminhos absolutos persistidos. Todos os caminhos de projeto devem ser relativos à raiz.
5. Não duplicar mapas, assets, cliente, servidor ou conteúdo em múltiplas pastas.
6. O mapa nativo é FMAP. O manifesto principal fica em `Game/Maps/World/world.fmap.json`.
7. OTBM é formato legado/referência; não deve virar fonte de verdade do novo jogo.
8. O protocolo nativo é `Fantasy Protocol`; 10.98 é apenas bridge/reference temporário.
9. O Fantasy Server é autoritativo: movimento, combate, inventário e estado persistente não podem depender da confiança no cliente.
10. Contratos compartilhados ficam em `Shared/`; não duplicar definições de protocolo ou formato entre Server, Client e Studio.
11. Código do Studio fica em `Studio/`; servidor em `Server/`; cliente em `Client/`; conteúdo em `Game/`.
12. Não salvar builds, logs, caches ou bancos locais no Git.
13. Toda nova função precisa de validação objetiva. Compilar sozinho não significa PASS.
14. GUI, CLI, scripts e Codex devem chamar o mesmo núcleo de operações de domínio sempre que possível.
15. Operações grandes/IA devem ser transacionais, reversíveis ou precedidas por snapshot.
16. Mudanças de protocolo devem atualizar a especificação versionada antes ou junto do código.
17. Mudanças de FMAP devem manter compatibilidade de schema versionada ou fornecer migração explícita.
18. Nunca introduzir dependência de OTBM, 10.98, DAT/SPR/OTB ou TFS no core nativo sem justificativa documentada.
19. A GUI do Map Editor não pode possuir um segundo modelo de mapa: ela deve editar `MapDocument`/FMAP por operações do core.
20. Ações visuais que alteram o mapa devem ser transações compatíveis com undo/redo desde a primeira implementação.
21. Studio e Server devem consumir o FMAP neutro de `Shared/Formats/FMAP/`; não criar parser/serializer FMAP paralelo.
22. Mensagens do cliente expressam intenção. Estado autoritativo de posição/entidades é emitido pelo Server; o cliente nunca envia posição absoluta como verdade.

## Layout essencial

```text
Studio/
Game/
  Maps/
  Content/
  Assets/
Server/
Client/
Shared/
  Protocol/
  Formats/
Database/
Tools/
Projects/
```

## Gate mínimo para cada mudança

- layout continua válido;
- nenhum caminho absoluto novo;
- build relevante passa;
- testes relevantes passam;
- contratos compartilhados continuam consistentes;
- se tocar em FMAP: schema/fixture valida e reabre semanticamente igual;
- se tocar no protocolo: encoder/decoder ou contrato correspondente é testado;
- se tocar no runtime: Start/Stop não deixa processo órfão;
- se tocar em conteúdo: referências semânticas são validadas;
- se tocar no editor visual: a alteração deve ser reproduzível pelo core sem depender do mouse/UI.

## Fases concluídas

- **F00 — Independent Core Foundation: PASS**
- **F01 — Project System: PASS**
- **F02 — Fantasy Map Core: PASS**
- **F03 — Fantasy Map Editor MVP: TECHNICAL PASS; interactive Windows visual check pending**
- **F04 — Fantasy Server World Runtime: PASS**

## Prioridade atual

**F05 — Fantasy Protocol v1 + First Native Play.**

Implementar e provar, nesta ordem:

1. congelar framing binário do Fantasy Protocol v1;
2. separar comandos/intenção do cliente de eventos/estado autoritativo do Server;
3. criar codec compartilhado e testes de endian/framing/limites;
4. implementar `Hello` / `HelloAck` e version handshake;
5. implementar `LoginDev` / `LoginOk` e `EnterWorld`;
6. serializar MapChunk a partir do FMAP;
7. ligar TCP Server + Client mínimo;
8. enviar MoveRequest e receber EntityMove autoritativo;
9. provar personagem entrando no FMAP e andando sem OTBM/10.98;
10. manter todos os gates anteriores verdes.

Não iniciar persistência da F06 antes do primeiro play nativo da F05 estar funcional.
