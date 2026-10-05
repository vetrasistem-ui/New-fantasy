# Server

Runtime nativo do Fantasy.

```text
Server/
├── Core/
├── Modules/
├── Config/
└── Logs/
```

O objetivo é construir **Fantasy Server**, não incorporar TFS como core permanente.

## F00

O primeiro executável é propositalmente mínimo:

```powershell
cmake -S Server -B build/server
cmake --build build/server --config Release
ctest --test-dir build/server -C Release --output-on-failure
```

Ele serve para provar toolchain, CI e ciclo básico antes de introduzir rede, mundo ou persistência.

## Direção

O core deve crescer em módulos pequenos:

```text
App
Network
Protocol
World
Map
Entities
Scheduler
Persistence
```

TFS 1.4.2 permanece somente como referência de comportamento em `.upstream/`.
