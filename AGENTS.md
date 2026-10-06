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
7. OTBM é formato legado/importação; pode ser aberto por uma camada de compatibilidade, mas não vira fonte de verdade do runtime nativo.
8. O protocolo nativo é `Fantasy Protocol`; 10.98 é apenas bridge/reference temporário.
9. O Fantasy Server é autoritativo: movimento, combate, inventário e estado persistente não podem depender da confiança no cliente.
10. Contratos compartilhados ficam em `Shared/`; não duplicar definições de protocolo, FMCP, rede, FMAP ou Asset Registry entre Server, Client e Studio.
11. Código do Studio fica em `Studio/`; servidor em `Server/`; cliente em `Client/`; conteúdo em `Game/`.
12. Não salvar builds, logs, caches ou bancos locais no Git.
13. Toda nova função precisa de validação objetiva. Compilar sozinho não significa PASS.
14. GUI, CLI, scripts e Codex devem chamar o mesmo núcleo de operações de domínio sempre que possível.
15. Operações grandes/IA devem ser transacionais, reversíveis ou precedidas por snapshot.
16. Mudanças de protocolo devem atualizar a especificação versionada antes ou junto do código.
17. Mudanças de FMAP devem manter compatibilidade de schema versionada ou fornecer migração explícita.
18. Nunca introduzir dependência de OTBM, 10.98, DAT/SPR/OTB ou TFS no core nativo sem justificativa documentada.
19. A GUI do Map Editor não pode possuir um segundo modelo nativo de mapa: ela deve editar `MapDocument`/FMAP por operações do core. Um `LegacyMapImportModel` temporário é permitido apenas dentro da camada de import/compatibilidade.
20. Ações visuais que alteram o mapa devem ser transações compatíveis com undo/redo desde a primeira implementação.
21. Studio e Server devem consumir o FMAP neutro de `Shared/Formats/FMAP/`; não criar parser/serializer FMAP paralelo.
22. Mensagens do cliente expressam intenção. Estado autoritativo de posição/entidades é emitido pelo Server; o cliente nunca envia posição absoluta como verdade.
23. `MapChunk` v1 usa o codec compartilhado FMCP v1. O Client resolve tiles com `regionOrigin + chunkOffset + tileLocal`; não inventar uma segunda convenção de coordenadas.
24. O transporte `LoginDev` da F05 é **somente desenvolvimento/loopback**. Não alterar bind para `0.0.0.0`, IP público ou VPS público sem uma fase explícita de segurança/autenticação.
25. O Client headless e o Client GUI devem consumir o mesmo `DevelopmentClient`; a GUI não pode implementar protocolo/rede próprios.
26. SDL3 + SDL_Renderer3 + Dear ImGui formam o baseline visual 2D. SDL_GPU é opcional/futuro e não deve virar requisito do core, gameplay, protocolo, mapa ou autoridade.
27. Na F05.5, DAT/SPR/OTB/OTBM e XMLs de houses/spawns ficam fora do Git salvo autorização explícita de redistribuição. O bridge trabalha com pack local configurável e registra hashes/evidências, não cópias do conteúdo.
28. O parser OTBM não pode mutar `MapDocument` diretamente: deve produzir primeiro um modelo de import neutro e diagnóstico loss-aware.
29. Nenhum item/atributo legado desconhecido pode ser descartado silenciosamente. Registrar unknown/unsupported com contagem/contexto antes de converter para FMAP.
30. O OTBM de origem é imutável durante a migração: validar hash antes/depois das sessões e nunca salvar por cima do source.

## Layout essencial

```text
Studio/
Game/
  Maps/
  Content/
  Assets/
Server/
  Core/
  Network/
Client/
  Core/
  UI/
Shared/
  Assets/
  Protocol/
  Formats/
  Network/
Tests/
  Integration/
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
- se tocar em protocolo/FMCP: especificação, codec e roundtrip correspondente passam;
- se tocar em rede: framing não pode depender de limites de pacote TCP e o listener F05 continua loopback-only;
- se tocar no runtime: Start/Stop não deixa processo órfão;
- se tocar em conteúdo: referências semânticas são validadas;
- se tocar no editor visual: a alteração deve ser reproduzível pelo core sem depender do mouse/UI;
- se tocar no Client GUI: `fantasy-client.exe` headless deve continuar funcional;
- se tocar em legado: source hashes permanecem iguais, unknowns são reportados e nenhum binário legado entra no commit.

## Fases concluídas

- **F00 — Independent Core Foundation: PASS**
- **F01 — Project System: PASS**
- **F02 — Fantasy Map Core: PASS**
- **F03 — Fantasy Map Editor MVP: PASS**
- **F04 — Fantasy Server World Runtime: PASS**
- **F05 — Fantasy Protocol v1 + First Native Play: PASS**
- **Fantasy Studio Visual Foundation V5: PASS / merged**

## Prioridade atual

**F05.5 — PokeFans 10.98 Legacy Compatibility + Real Map Bootstrap: IN PROGRESS.**

Branch ativa:

```text
feature/f05.5-pokefans-1098
```

Objetivo imediato:

```text
PokeFans 10.98
OTBM + DAT + SPR + OTB + houses/spawns
        ↓
Legacy Compatibility Layer
        ↓
Fantasy Asset Registry + LegacyMapImportModel
        ↓
Fantasy Studio V5
        ↓
mapa real com sprites reais
```

Primeiro perfil suportado é PokeFans 10.98. Outras versões só entram depois que esse caminho estiver validado e sem espalhar condicionais de versão pelo core.

## F06

**F06 — Persistence / Database: NOT STARTED.**

Não iniciar F06 enquanto o primeiro milestone F05.5 (mapa OTBM real + sprites reais + diagnóstico + source preservado) não estiver fechado ou enquanto o owner não mudar explicitamente a prioridade.
