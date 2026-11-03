# WinUI Profiler optimization harness

This harness is a deterministic controller around the existing WinUI Profiler. It prepares an app repository and profiler repository, builds and deploys both, creates portable UI Automation scenarios, captures first-frame and post-scenario snapshots, and runs at most five distinct optimization attempts.

The controller owns branching, commands, attempt limits, duplicate rejection, evidence, and success criteria. Agency or GitHub Copilot can propose a scenario or edit an isolated app worktree, but a proposal is accepted only after the configured build, deployment, capture, and metric comparison succeed.

The stage architecture, complete harness flow, and exact invocation commands are in [WinUIProfiler-Harness-Architecture-and-Invocation-Guide.docx](WinUIProfiler-Harness-Architecture-and-Invocation-Guide.docx). Detailed requirement traceability and implementation status are in [WinUIProfiler-Harness-Architecture-and-Runbook.docx](WinUIProfiler-Harness-Architecture-and-Runbook.docx).

## Layout

- `Invoke-ProfilerHarness.ps1`: single command-line entry point.
- `ProfilerHarness.psm1`: deterministic workflow and local/Hyper-V/Ixp providers.
- `Driver`: headless profiler and UI Automation executable.
- `skills`: the five reusable skill boundaries requested by the harness design.
- `examples/store.local.json`: local Microsoft Store configuration using `user/akanpatel/profiler_harness` and `user/akanpatel2206/profiler_harness`.
- `examples/store.ixp.json`: preferred create-if-missing Ixp configuration for a dedicated `ge_current` VM over PSRemote.
- `examples/store.ixp.preview.json`: reuse-only configuration for the diagnosed `rs_prerelease` preview VM.
- `.artifacts`: ignored run logs, deployment state, captures, sessions, snapshots, patches, and reports.

## Prerequisites

- PowerShell 7 and Git.
- .NET 10 SDK and the WinUI Profiler build prerequisites. Driver publishing reads the exact SDK version from `tools\WinUIProfiler\global.json` and selects an installation that contains it, even when an initialized MUX shell has placed the repository-local .NET 9 host first on `PATH`.
- The target user must have an active interactive desktop session for UI Automation.
- Local Store builds require an elevated PowerShell session, Developer Mode, Visual Studio 2026 Enterprise, and the Store repository's NativeAOT build prerequisites. The Store example reads its `global.json` and installs the exact missing .NET SDK side-by-side under `%LOCALAPPDATA%\Microsoft\dotnet` with Microsoft's `dotnet-install.ps1`.
- Local Store deployment temporarily replaces the inbox Store package for all users. The first deployment enables `HKLM\SOFTWARE\Microsoft\SecurityManager\InternalDevUnlock` and stops before changing any package so Windows can be rebooted once.
- Store payload preparation signs a copied MSIX with a dedicated self-signed harness certificate. Deployment imports that certificate into `LocalMachine\TrustedPeople`, installs the signed package, and records enough state to remove the certificate during restoration.
- Hyper-V targets require a configured source VHD and credentials supplied only through environment variables.
- Ixp targets require IxpTools. The harness keeps `connectd`, transfer, execution, and result collection in the same PowerShell process. `connectionMode` supports `auto`, `background`, and `psremote`.
- Creating a missing Ixp VM normally requires authoritative build media through the Windows build share or a working IxpTools ESGraph/Drop path. The canonical `ge_current` example enables `New-TestMachine -AllowOffline` so IxpTools can fall back to already-cached VHDs and tools.
- For remote targets, the harness deploys the signed Agency installation already available on the host. It auto-detects `Get-Command agency`; set `agents.agencySourcePath` to an Agency installation directory or `agency.exe` when an override is needed. If neither is available, the harness falls back to Microsoft `InstallTool.ps1`, which can require interactive authentication.
- Standalone GitHub Copilot CLI is installed separately through `npm install --global @github/copilot`.
- When an Ixp target does not have npm, the harness installs `OpenJS.NodeJS.LTS` silently through WinGet before installing standalone GitHub Copilot CLI.
- First use of either agent may require interactive sign-in. Credentials and tokens do not belong in harness JSON.

## Start with validation and a dry run

```powershell
cd Q:\Repo\microsoft-ui-xaml

pwsh .\tools\WinUIProfiler\harness\Invoke-ProfilerHarness.ps1 `
  -Stage validate `
  -Config .\tools\WinUIProfiler\harness\examples\store.local.json

pwsh .\tools\WinUIProfiler\harness\Invoke-ProfilerHarness.ps1 `
  -Stage system-ready `
  -Config .\tools\WinUIProfiler\harness\examples\store.local.json `
  -DryRun
```

The real readiness run prepares both topic branches, verifies or installs both agents, builds/deploys Store, builds WinUI Profiler, publishes the self-contained headless driver, and records deployment state:

```powershell
pwsh .\tools\WinUIProfiler\harness\Invoke-ProfilerHarness.ps1 `
  -Stage system-ready `
  -Config .\tools\WinUIProfiler\harness\examples\store.local.json
```

The Store example runs the repository's checked-in `Build-NativeAot.ps1 -BuildOnly` workflow, signs a copied MSIX, and deploys that signed package while avoiding unattended Visual Studio/DTE prompts. Signing makes the package trusted and installable; it does not enable or authorize the Windows App SDK system composition engine. On the first run, `system-ready` can stop with a reboot-required message after enabling `InternalDevUnlock`; rerun the identical command after reboot.

The checked-in Ixp configuration creates or reuses the dedicated `WinUIProfilerHarness-ge-current-latest` VM from the latest usable `ge_current` Desktop build. It enables CD keys during VM creation:

```powershell
pwsh .\tools\WinUIProfiler\harness\Invoke-ProfilerHarness.ps1 `
  -Stage validate `
  -Config .\tools\WinUIProfiler\harness\examples\store.ixp.json

pwsh .\tools\WinUIProfiler\harness\Invoke-ProfilerHarness.ps1 `
  -Stage system-ready `
  -Config .\tools\WinUIProfiler\harness\examples\store.ixp.json
```

On the current DevBox, configuration validation passes and offline mode is enabled. IxpTools can create the VM offline only when a matching `ge_current` VHD and required tools already exist in its local cache. If creation reports that no offline media is available, restore `\\ntdev\release` or ESGraph/Drop access once to populate the cache, then rerun the identical `system-ready` command.

IxpTools requires a one-time interactive defaults setup before an unattended `-Force` run. If `system-ready` reports `We need to prompt for required information`, open an elevated `pwsh` window and run:

```powershell
newvm -SetDefaults
```

Use `C:\VMs\WinUIProfilerHarness` for the save path, `Desktop` for the flavor, and `Default Switch` for the virtual switch. Choose the region/mirror, VHD cache, local-admin password, kernel-debugging, and default-tool settings appropriate for your environment. The checked-in harness configuration overrides the VM name, branch, memory, processor count, tools, WinGet apps, CD-key state, and offline behavior. After setup completes, rerun the same `store.ixp.json` `system-ready` command.

Use `store.ixp.preview.json` only to reproduce the diagnosed blocker on `WinUIProfilerHarness-ge-current-directwex-latest`. This configuration is reuse-only and rejects a guest older than build 27200:

```powershell
pwsh .\tools\WinUIProfiler\harness\Invoke-ProfilerHarness.ps1 `
  -Stage validate `
  -Config .\tools\WinUIProfiler\harness\examples\store.ixp.preview.json
```

The preview VM completes `system-ready`, signed Store deployment, profiler deployment, and agent readiness. Store launch and capture are blocked on that VM because its OS reports composition change `62917618` disabled, the OS compositor reports no switcher support, and `TrySetProcessEngine(System)` returns `selected=false`. The preferred `store.ixp.json` configuration now uses `ge_current` instead. The harness preserves failed launch evidence instead of reporting success.

The first Ixp Store deployment can likewise stop after enabling `InternalDevUnlock` in the guest. Reboot that VM, then rerun the same command. A clean Ixp image does not need an inbox Store package; restoration removes the harness package and restores the previous registry state.

Remote Agency deployment copies only the tool installation, never the host's credentials or tokens. The first Agency or GitHub Copilot use in a new guest can still require an interactive sign-in.

Restore the supported Store package after local profiling:

```powershell
pwsh .\tools\WinUIProfiler\harness\scripts\StorePackage.ps1 `
  -Action Restore `
  -RepoRoot Q:\Repo\SFC.Client.SFUI `
  -StatePath .\tools\WinUIProfiler\harness\.artifacts\state\store-original-package.json
```

Restoration uses Windows' supported `wsreset.exe -i` recovery path, waits for Store registration, removes any harness-added signing certificate, and restores the prior `InternalDevUnlock` registry value.

## Create or run an automation scenario

Generate a validated scenario with either engine:

```powershell
pwsh .\tools\WinUIProfiler\harness\Invoke-ProfilerHarness.ps1 `
  -Stage create-automation-test `
  -Config .\tools\WinUIProfiler\harness\examples\store.local.json `
  -Engine agency `
  -ScenarioPrompt "Launch Store, search for Netflix, and open the result without installing or purchasing anything." `
  -ScenarioPath .\tools\WinUIProfiler\harness\examples\generated-store-scenario.json
```

Capture only:

```powershell
pwsh .\tools\WinUIProfiler\harness\Invoke-ProfilerHarness.ps1 `
  -Stage capture `
  -Config .\tools\WinUIProfiler\harness\examples\store.local.json
```

The driver waits for a recorded drawn frame rather than sleeping, takes the initial snapshot, executes the validated UI Automation DSL, settles, takes the final snapshot, and writes `capture-result.json` with snapshot paths, the SQLite session path, frame metrics, object counts, and ETW loss information.

## Run the bounded optimization loop

```powershell
pwsh .\tools\WinUIProfiler\harness\Invoke-ProfilerHarness.ps1 `
  -Stage profile-and-apply-changes `
  -Config .\tools\WinUIProfiler\harness\examples\store.local.json `
  -Engine agency
```

Each proposal is made in a temporary Git worktree. Empty and duplicate patches are rejected without consuming one of the five distinct validation attempts. Proposal generation is separately capped at three times the configured attempt count, so repeated unusable advice cannot loop forever. A distinct patch is built, deployed, and profiled. A patch is applied to the configured app topic branch only when the configured primary metric improves by the configured threshold and the capture has no lost or dropped events. The controller then stops for manual review. After five distinct unsuccessful suggestions, or after the bounded proposal budget cannot produce that many distinct changes, it writes `manual-intervention-required.json` and stops.

## Target configuration

`target.kind` selects:

- `local`: build, deploy, launch, automate, and profile on the current machine.
- `hyperv`: create/reuse a VM, copy payloads with PowerShell Direct, and launch the driver through an interactive scheduled task. Set `userNameEnvironmentVariable` and `passwordEnvironmentVariable`; never put credentials in JSON.
- `ixp`: create/reuse a named Ixp VM with explicit branch/media settings, transfer with `putd/getd`, and launch visible UI with `execd -AsUser`. Set `ensureCreated` to `false` for a reuse-only configuration.

Remote app deployment is app-specific. Configure `application.payloadPath` and provider deployment commands for the app's package format. `store.ixp.json` stages the NativeAOT Store package, copies it into the isolated guest, and registers it for the interactive user.

## Safety and evidence

- Repository preparation refuses tracked changes unless `allowDirty` is explicitly set.
- Branches must be named `user/<alias>/profiler_harness`.
- Agent processes receive a disabled Git push URL and are told not to commit or push.
- Attempt edits happen outside the main app worktree.
- JSON scenarios cannot execute arbitrary scripts.
- Partial snapshots are never treated as complete by WinUI Profiler.
- Run folders preserve command stdout/stderr, configuration identity, SQLite sessions, snapshots, patches, evaluations, and the final decision.
