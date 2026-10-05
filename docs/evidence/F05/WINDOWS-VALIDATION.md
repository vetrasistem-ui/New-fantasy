# F05 Windows Validation

Status: **PENDING — DO NOT MARK PASS UNTIL EXECUTED**

This file is the evidence ledger for `docs/CODEX-F05-WINDOWS.md`.

Codex must replace every `PENDING` below with observed evidence. Do not infer or fabricate results.

## Environment

```text
Windows version: PENDING
Starting repository SHA: PENDING
Final repository SHA tested: PENDING
Configuration: Release
```

## C00 — Clean checkout

```text
Result: PENDING
Working tree clean before validation: PENDING
Starting SHA: PENDING
Notes: PENDING
```

## C01 — Full native build

Command:

```powershell
./scripts/build-native.ps1 -Configuration Release
```

```text
Result: PENDING
Studio build/tests: PENDING
Server build/tests: PENDING
Client build/tests: PENDING
fantasy-native-play-tests: PENDING
Expected executables present: PENDING
Notes/fixes: PENDING
```

## C02 — Headless two-process first play

Run 1:

```powershell
./scripts/run-native-play.ps1 -Configuration Release -Port 17171 -Character "Codex Headless Hero"
```

Run 2:

```powershell
./scripts/run-native-play.ps1 -Configuration Release -Port 17172 -Character "Codex Reconnect Hero"
```

```text
Run 1 result: PENDING
Run 2 result: PENDING
Initial position: PENDING
Moved position: PENDING
Chunks/tiles: PENDING
Server exit codes: PENDING
Orphan process check: PENDING
Notes/fixes: PENDING
```

## C03 — F03 Studio interactive gate

Expected screenshot directory:

```text
docs/evidence/F03/windows/
```

```text
Result: PENDING
Studio opened: PENDING
Four chunks visually present: PENDING
Paint: PENDING
Undo/Redo: PENDING
Fill across chunk boundary: PENDING
Add/Remove/Erase object: PENDING
Save/Reopen persistence: PENDING
Canonical world restored after evidence: PENDING
Before screenshot: PENDING
After Save/Reopen screenshot: PENDING
Notes/fixes: PENDING
```

## C04 — Visual Fantasy Client first play

Command:

```powershell
./scripts/run-visual-play.ps1 -Configuration Release -Port 17173 -Character "Codex Visual Hero"
```

Expected screenshot directory:

```text
docs/evidence/F05/windows/
```

```text
Result: PENDING
GUI opened: PENDING
Connected/in-world: PENDING
Initial position 100,100,7: PENDING
Four chunks / 64 tiles visible: PENDING
Semantic placeholder rendering: PENDING
Object markers: PENDING
Player marker: PENDING
Right move -> 101,100,7: PENDING
Left move -> 100,100,7: PENDING
North/South authoritative: PENDING
Rejected movement remains server-authoritative: PENDING
Close sends Disconnect: PENDING
Server exits automatically: PENDING
Initial screenshot: PENDING
Moved screenshot: PENDING
Notes/fixes: PENDING
```

## C05 — Visual reconnect / cleanup

Command:

```powershell
./scripts/run-visual-play.ps1 -Configuration Release -Port 17174 -Character "Codex Visual Reconnect"
```

```text
Result: PENDING
Reconnect clean: PENDING
Spawn reset: PENDING
Map resent: PENDING
Movement works: PENDING
No orphan processes: PENDING
Notes/fixes: PENDING
```

## C06 — Architecture confirmations

```text
Listener stayed loopback-only: PENDING
No OTBM used: PENDING
No protocol 10.98 used: PENDING
No TFS runtime used: PENDING
No OTClient used: PENDING
Client movement remained intent-only: PENDING
Server remained authoritative: PENDING
```

## C07 — Final regression

Commands:

```powershell
./scripts/build-native.ps1 -Configuration Release
./scripts/run-native-play.ps1 -Configuration Release -Port 17175 -Character "Final Regression Hero"
git status --short
git diff --check
```

```text
Result: PENDING
All automated gates green: PENDING
Final native play green: PENDING
Accidental FMAP changes: PENDING
Unexpected build/log/cache files: PENDING
Absolute persisted paths: PENDING
Orphan processes: PENDING
Final SHA: PENDING
```

## C08 — Closeout

Only fill this section after C00–C07 are actually PASS.

```text
F03 changed to PASS: PENDING
F05 changed to PASS: PENDING
ROADMAP updated: PENDING
F03 result updated: PENDING
F05 result updated: PENDING
AGENTS priority changed to F06: PENDING
F06 implementation intentionally not started in closeout commit: PENDING
Final closeout commit: PENDING
GitHub Actions run on exact final SHA: PENDING
```

## Failures and fixes

For every encountered failure, append an entry:

```text
Stage:
Symptom:
Failing layer:
Root cause:
Smallest fix:
Commit SHA:
Direct tests rerun:
Stage rerun result:
Affected regression result:
```
