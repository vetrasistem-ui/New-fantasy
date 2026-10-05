# Server

Runtime de servidor do Fantasy.

Estrutura planejada:

```text
Server/
├── Core/
├── Modules/
├── Config/
└── Logs/
```

`Core/` receberá a base TFS 1.4.2 homologada. Recursos próprios devem preferir `Modules/` ou adapters bem definidos em vez de alterações desnecessárias espalhadas pelo core.
