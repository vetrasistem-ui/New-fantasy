# F03 Windows startup failure — C03

Date: 2026-10-05 (America/Sao_Paulo).

Status: **HISTORICAL SDL_GPU FAILURE — SUPERSEDED BY OFFICIAL SDL_RENDERER3 BASELINE**.

## Current baseline follow-up

Official `main` is now `621ca7b499330b61f653345590f1897c3b1f76d2`, with
SDL3 + SDL_Renderer3 + Dear ImGui as the 2D baseline (ADR-019).
The historical correction `c072870` is retained in
`backup/c03-sdlgpu-c072870`; it was not restored into the GUI code.
The complete historical evidence is additionally preserved in
`backup/c03-evidence-a4a3287`.

The full native build on the new SHA passed all 13 CTests and validators.
Both headless sessions passed again. The new Studio opened and remained
responsive; the former SDL_GPU initialization errors did not recur.
C03 was rerun from the beginning and passed: all four chunks were visible;
tile selection, Paint, Undo, Redo, Fill across chunks (17 tiles), Add Object,
Remove Object and Erase passed. Save/Reopen preserved the grass Paint at
global 97,97,7, the 17 sand tiles across the two upper chunks, and empty objects
at 98,97,7. The reopened visual state was captured in
`c03-renderer3-save-reopen.jpg`; the initial state is `c03-renderer3-before.jpg`.
Studio exited 0. The canonical FMAP was restored with its original hash
`1442d649ff73ba39881f876a4bde33394df2784c`.
C04/C05 visual play, authoritative bounds rejection and clean reconnect/
Disconnect/Stop passed. C07 passed all 13 CTests and the final two-process
headless play. F03/F05 are formally PASS. F06 is not implemented.

The remainder of this document preserves the earlier failures and diagnostic
outputs without treating the old SDL_GPU commit as current code.

Tested repository SHA: `21cc7466b3c737e23f062f5cee7b38f6a501cefb`.

## Observed environment

- Windows 11 Home, version 10.0.26200.
- PowerShell 7.6.5, x64.
- Intel(R) HD Graphics 5500.
- Graphics driver 20.19.15.4703, dated 2017-06-08.
- SDL3 release-3.4.18, commit `829a65d769d935c4852f8159e964312c0957260a`.
- ImGui v1.92.9b, commit `f1cc2ae15e53a861a874c3034aae6798fde194ab`.

## Launch and diagnostic repeat

From the repository root, the Release Studio was launched with argument `.`:

```powershell
./build/studio/Release/fantasy-studio-gui.exe .
```

The process exited before the editor could be used:

```text
Fantasy Studio GUI error: SDL_CreateGPUDevice failed: No supported SDL_GPU backend found!
```

The same launch was repeated with `SDL_LOGGING=*=verbose` set only for the
diagnostic process. The previous environment value was restored afterwards.
Observed output, exit code 1:

```text
SDL chose video backend 'windows'
WARNING: D3D12: Tier 2 Resource Binding is not supported
Fantasy Studio GUI error: SDL_CreateGPUDevice failed: No supported SDL_GPU backend found!
```

The failing layer is SDL_GPU backend/device selection in the current Windows
graphics environment. The pinned SDL D3D12 backend rejects a device lacking
Tier 2 Resource Binding for its requested resource capabilities. No supported
GPU backend was selected. This evidence does not establish whether a different
driver would resolve the failure.

No code was changed in this initial attempt. The subsequent Tier 1 initialization
fix and its new failure are recorded below. No driver was installed.

## FIX-C03-01/02 — Official Tier 1 initialization policy

Correction commit: `c0728702b910216de7c92142a98191985fb4f85e`
(`fix(graphics): support D3D12 tier-1 Intel GPUs`).

Both `Studio/UI/EditorApp.cpp` and `Client/UI/ClientApp.cpp` now call
`SDL_CreateGPUDeviceWithProperties`, preserving SPIRV, DXIL, MSL, METALLIB
and debug mode. Temporary properties are destroyed after creating the device;
creation errors are preserved before cleanup. On Windows only, the initialization
enables `SDL_PROP_GPU_DEVICE_CREATE_D3D12_ALLOW_FEWER_RESOURCE_SLOTS_BOOLEAN`.
The backend name remains unspecified.

The [official SDL documentation](https://wiki.libsdl.org/SDL3/SDL_CreateGPUDeviceWithProperties)
provides this policy for Tier 1 Haswell/Broadwell devices using at most eight
storage resources. The pinned ImGui shaders use zero storage buffers/textures.
SDL3 + SDL_GPU + Dear ImGui remain the visual stack. Gameplay, networking,
protocol and FMAP were not changed.

## FIX-C03-03 — Build, direct regressions and startup repeat

Both requested Release builds passed. The corresponding Studio CTests passed
5/5; Client CTests passed 2/2. Logs remain in ignored
`build/validation/FIX-C03-03-build.txt`.

The Studio was relaunched with `.` and automatic backend selection. It exited
1 before interactive validation. Repeating with temporary verbose SDL logging
also exited 1 and reported:

```text
SDL chose video backend 'windows'
WARNING: D3D12: DXIL is not supported and DXBC is not being provided
Fantasy Studio GUI error: SDL_CreateGPUDeviceWithProperties failed: No supported SDL_GPU backend found!
```

The earlier Tier 2 warning is absent. The next observed rejection is shader
support in the D3D12 backend: DXIL is unavailable and the requested formats
do not include DXBC. No further shader/backend/renderer correction was applied.

Per the requested failure policy, execution stopped at this new failure.
C03 interactive operations and Save/Reopen remain NOT EXECUTED; no screenshots
were captured. The Client correction has build/CTest coverage only; C04/C05
were not executed. C06/C07/C08 were not advanced. No Fantasy process remained
after the failed launches. No F06 implementation or formal F03/F05 closure.

## Validation boundaries

- C01 passed: Studio 5/5, Server 6/6, Client 2/2 CTests.
- Both C02 two-process sessions passed with clean disconnect and no orphans.
- All C03 interactive operations: **PENDING / NOT EXECUTED**.
- Screenshots: **NOT CAPTURED**.
- C04–C08: **PENDING / NOT EXECUTED**; F03/F05 remain formally pending.
- Canonical FMAP unchanged: `1442d649ff73ba39881f876a4bde33394df2784c`.

Local diagnostic files remain under ignored `build/validation/`; they are not
staged or committed. The overall ledger is
`docs/evidence/F05/WINDOWS-VALIDATION.md`.
