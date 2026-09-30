---
summary: TableView build and runtime verification
---

# TableView build and runtime verification

## Build (product)
`init.cmd` must run in the SAME cmd process as the build, and needs `.\` (this environment sets `NoDefaultCurrentDirectoryInExePath`):

```
cmd /c "cd /d <repo> && call .\init.cmd && set PreferredToolArchitecture=x64 && set CL_MPCount=2 && call .\Build.cmd product /q /b"
```

- `PreferredToolArchitecture=x64` is required; default x86 host tools exhaust the PCH (`C3859` / `C1076`).
- `CL_MPCount=2` with `/b`; otherwise MultiProcessorCompilation x 16 cores spawns ~32 `cl` processes and exhausts system commit.
- Incremental product build ~9-22 min; full ~74 min.

## Pack and sample
Always pack with a NEW version string. NuGet caches by version, so re-packing the same version is silently ignored and you test a stale component.

```
call .\pack.component.cmd /version 3.0.0-<new>
.\initrun.ps1 msb /q /restore Samples\TableViewSampleApp\TableViewSampleApp.csproj /p:Platform=x64 /p:WinUIVersion=3.0.0-<new>
```

Close any running `TableViewSampleApp` first: a live instance locks the output DLLs and the build fails with `MSB3027` after 10 copy retries, which can leave a mixed-version output folder. Delete `BuildOutput\obj\amd64chk\Samples\TableViewSampleApp` and rebuild if that happens.

## Fast validation without a product build
- Syntax-only compile a changed TU using the recorded project command line from `BuildOutput\obj\amd64chk\controls\dev\dll-tabular\...\CL.command.1.tlog` with `/c` and `/Fo` stripped and `/Zs` added. Full `/W4 /WX` fidelity in seconds.
- `cl.exe` never sees XAML, so `/Zs` cannot catch a XAML parse or template-apply failure. The row template is applied at first row realization.

## Runtime verification
- A headless host session is a FALSE NEGATIVE for anything template-related: with no foreground desktop the control template is never instantiated, so the app "runs" while a template-apply failure goes undetected. Gate on `input-health` reporting a non-zero `foregroundHwnd` before believing a smoke test.
- Use a VM with a real desktop for keyboard/focus work; `input-health` there reports `Ready` with `hotkeyReady: true`.
- Always take the hwnd from `launch`, never match by window title; VMs accumulate stale sample windows.

_Confirmed: Ran this exact build/pack/sample sequence repeatedly this session; hit and diagnosed the MSB3027 lock failure, the stale-version cache, and a headless-host false negative where the app ran on the host but crashed at startup on a VM with a real desktop._
