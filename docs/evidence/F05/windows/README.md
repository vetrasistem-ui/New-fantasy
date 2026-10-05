# F05 Windows Visual Evidence

Store evidence from the **real Fantasy Client GUI first-play validation** here.

Required minimum before F05 can be marked PASS:

```text
initial-in-world.png
authoritative-move.png
```

Equivalent descriptive filenames are acceptable when their exact paths are recorded in `docs/evidence/F05/WINDOWS-VALIDATION.md`.

The evidence should show:

- `fantasy-client-gui.exe` connected through Fantasy Protocol v1;
- the four development chunks / 64 tiles rendered;
- player marker at initial authoritative position `100,100,7`;
- a second screenshot after a server-authoritative move, e.g. East to `101,100,7`.

A reconnect session must also be executed even if it does not need an additional screenshot, and the process-cleanup result must be recorded in `WINDOWS-VALIDATION.md`.

Placeholder semantic tile colors are acceptable for F05. Do not introduce the final sprite/asset pipeline merely to improve this gate.
