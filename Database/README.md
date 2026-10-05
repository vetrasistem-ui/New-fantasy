# Database

Banco e evolução de schema do Fantasy.

```text
Database/
├── Migrations/
└── Seeds/
```

Credenciais, dumps locais e bancos de desenvolvimento não entram no Git.

## Estado atual

F06 ainda **não começou**. A implementação permanece bloqueada até o fechamento visual real de F03/F05 no Windows.

Plano aprovado para a próxima fase:

```text
WorldRuntime / Session
        ↓
PersistenceService
        ↓
IPersistenceStore
        ↓
SQLiteStore (primeiro adapter F06)
        ↓
Database/Migrations
```

O banco não será acessado diretamente por `WorldRuntime`, Client ou protocolo.

Referências de planejamento:

```text
docs/F06-PERSISTENCE-PLAN.md
docs/CODEX-AFTER-F05.md
```

## Disciplina futura de migrations

Quando F06 for ativada:

- migrations serão numeradas e append-only;
- testes sempre criarão banco temporário próprio;
- reabrir banco já migrado será um gate obrigatório;
- falha de migration interromperá startup;
- arquivos `.db`, `.sqlite`, dumps e credenciais continuarão fora do Git;
- nenhuma senha em texto puro será criada na fase F06.
