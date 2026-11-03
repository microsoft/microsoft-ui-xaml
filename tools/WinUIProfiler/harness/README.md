# WinUI Profiler optimization harness

**VM-only execution.** The devbox is a source/evidence controller, not a build or deployment target. Builds, SDK installation, package signing/registration, agent execution, UI Automation, and profiling belong in an explicitly approved, identity-pinned VM. Proposed patches return for manual review; they are never applied automatically on the devbox.

Schema 2 deliberately rejects the old local execution contract. Do not follow deployment commands in the historical DOCX guides: they predate these restrictions. These changes require execution validation in an isolated VM; source review is not evidence that provisioning, builds, or captures have passed.

## Prerequisites

The entry point displays the list before execution, asks for **IXP VM, local Hyper-V VM (default), or devbox**, then confirms prerequisites individually. Devbox selection is blocked, not a fallback.

1. Choose the application, its existing source repository path or clone URL, and the manually supplied alias for `user/<alias>/profiler_harness`.
2. Supply the profiler repository path/URL and topic branch. Read both repositories' README/build requirements before configuring commands.
3. Provision and boot an isolated VM manually on the chosen virtualization host. Hyper-V must already be available; do not automatically enable host features or change host networking. Do not expose writable host drives/repositories or host credentials to the guest.
4. For IXP, complete the manual process below first. IXPTools initialization can build host dependencies; the harness never imports it or creates/replaces a VM.
5. Complete guest Windows setup, PowerShell 7, Git, Visual Studio C++/MSBuild/Windows SDK requirements, and application-specific prerequisites. Profiler SDK selection uses its `global.json` inside the guest, not the devbox's initialized build environment.
6. Sign in interactively inside the guest as the same account used for PowerShell Direct, app registration, and capture. Record `Get-VM`'s VM ID, the guest's `HKLM:\SOFTWARE\Microsoft\Cryptography` MachineGuid, and guest computer name. Configure all three; the guest must differ from the devbox identity.
7. Take a clean **post-setup** checkpoint before agent/app/profiler installations. Configure its exact name and agree on manual recovery. A pre-boot checkpoint alone is not a ready desktop.
8. Configure and review every build/deploy command with `scope: "guest"` and a working directory inside `C:\WinUIProfilerHarness`. Review signing, package, registry, and certificate changes as VM mutations, not harmless deployment.
9. Supply an app-specific `application.resetScript` that restores equivalent scenario state before every sample. Merely stopping a process does not reset caches, search state, downloads, purchases, or persistent data. The examples intentionally throw until a reset is configured.
10. Install/authenticate Agency and standalone GitHub Copilot independently in the guest. The readiness stage can install missing tools there, but interactive sign-in is manual. Never copy host tokens or put passwords in JSON. For plain Hyper-V, configure environment variable **names** for guest credentials.
11. Verify target runtime/private diagnostic compatibility. The profiler's own pinned runtime does not replace or establish compatibility of the app's loaded WinUI runtime. Readiness must demonstrate app launch, a recorded first drawn frame, and complete before/after snapshots before scenarios run.
12. Approve guest execution explicitly. Any readiness/build/deploy/capture failure stops for manual inspection; do not retry by reinstalling a baseline, switching to the devbox, or replacing host DLLs.

### Manual IXP provisioning

The standalone wizard defaults to **checklist-only**. It does not load the harness or execute app/profiler work. These commands are instructions for manual use on the approved virtualization host, not authorization to run them automatically:

```powershell
# Checklist only: no IXP import or VM creation.
& .\tools\WinUIProfiler\harness\scripts\Initialize-IxpVm.ps1

# Explicitly requested manual provisioning, with interactive approvals.
& .\tools\WinUIProfiler\harness\scripts\Initialize-IxpVm.ps1 -Provision
```

The wizard walks through elevation/Hyper-V, switch selection, RAM/CPU/disk capacity, separately approved IXP dependency initialization/defaults, authorized media or cached-media fallback, and new VM/storage names. It requires at least 4 GB guest RAM, 4 GB remaining free host RAM, and 160 GB free storage. `yes`, `YES`, `y`, and `Y` approve prerequisite prompts; final `CREATE <VM name>` authorization remains exact.

Creation uses no force replacement, unattended setup, automatic start, TShell, kernel debugging, CD keys, or optional-feature requests. Inspect IXPTools' own summary and abort on unwanted installs or replacement. Saved/default IXP tools may still affect native provisioning; empty tool arrays are not a verified isolation guarantee. Offline fallback may still access the network.

The result records the VM ID and verified pre-boot checkpoint with `harnessReady: false`. Boot and complete guest setup manually, then create the post-setup checkpoint required by the harness. Partial provisioning is retained for inspection, not automatically deleted or retried.

For an IXP harness run, initialize IXPTools **manually in the same elevated PowerShell session** after reviewing its bootstrap side effects, and invoke the entry point with `&`, not a fresh `pwsh` process. Only verified PowerShell Direct sessions are used; ambient `connectd`, `cmdx`, `putd/getd`, and `execd -AsUser` are not execution routes.

## Configuration and invocation

Use `examples\store.hyperv.json` or `examples\store.ixp.json`. `store.ixp.preview.json` is a reuse-only compatibility investigation example, not proof that its guest can launch/profile the app. Replace illustrative GUIDs, guest names, paths, aliases, checkpoint names, and the throwing reset script before execution. The schema rejects legacy provisioning options, local deployment commands, automatic cleanup commands, and host Agency source paths. Provisioning parameters belong only in the separate manual wizard.

The dedicated guest staging root must be `C:\WinUIProfilerHarness` or a suffixed variant such as `C:\WinUIProfilerHarness-Store`. Host output is constrained to harness `.artifacts` with reparse-point checks. Command tokens resolve to guest paths:

| Token | Guest location |
|---|---|
| `{appRepo}` | `source\app`, or a guest candidate worktree |
| `{profilerRepo}` | `source\profiler` |
| `{harnessRoot}` | Current harness source staged inside the guest profiler repository |
| `{artifactsRoot}` | Guest staging `artifacts` |
| `{runDirectory}` | Guest run directory |
| `{targetAppRoot}`, `{targetProfilerRoot}`, `{targetDriverRoot}` | Guest deployment directories |

Existing repositories are read as tracked/nonignored source snapshots without host branch checkout or credential metadata. Uninitialized submodules/reparse points are refused. Missing repositories are cloned in the VM using guest authentication. Topic branches are created there; `baseRef` applies to new clones, whereas existing dirty snapshots preserve reviewed source changes when `allowDirty` is explicit.

After prerequisites are complete, the following is a **manual invocation**, not a command to execute during source hardening:

```powershell
& .\tools\WinUIProfiler\harness\Invoke-ProfilerHarness.ps1 `
  -Stage system-ready `
  -Config .\tools\WinUIProfiler\harness\examples\store.hyperv.json
```

Interactive target selection defaults to Hyper-V and must match the JSON target. Real execution requires prerequisite confirmations followed by `EXECUTE <VM name>`. Explicit unattended authorization requires all three: `-TargetKind hyperv|ixp`, `-PrerequisitesConfirmed`, and `-ApproveVmExecution`; these switches do not bypass VM identity, state, checkpoint, or guest-account checks.

`-Stage validate` reads configuration only. `-DryRun -TargetKind hyperv|ixp` records a plan without launching processes, connecting to a guest, building, or installing; it reports `planned`, not completed readiness. Neither is permission to run tests on the devbox.

## Reusable stages

| Stage/skill | Contract |
|---|---|
| `system-ready` | Verify existing VM/guest identity and desktop, prepare guest agent tools, build/deploy profiler then app, and record launch/first-frame/private-snapshot evidence. |
| `app-build-deploy` | Run app-specific guest commands; package, sign, and register only in the guest. Reusable for candidates. |
| `profiler-build-deploy` | Resolve the guest SDK, build the profiler, publish the driver, and copy complete outputs guest-to-guest. State is pinned to VM/guest IDs. |
| `create-automation-test` | Run the selected nonediting guest engine, validate its bounded JSON DSL, and save only under host `.artifacts`. |
| `capture` | Reset app state, wait for the first recorded drawn frame, snapshot, execute the scenario, snapshot again, and preserve evidence. |
| `profile-and-apply-changes` | Build/capture/compare guest candidates; export a source patch for manual review, never apply it on the devbox. |

The five skill wrappers expose the same target/prerequisite/consent parameters. `-SkipBuild` reuses **guest** outputs only, never host binaries. No stage automatically starts, restores, or replaces a VM.

Example scenario generation, for manual use after readiness:

```powershell
& .\tools\WinUIProfiler\harness\Invoke-ProfilerHarness.ps1 `
  -Stage create-automation-test `
  -Config .\tools\WinUIProfiler\harness\examples\store.hyperv.json `
  -Engine agency `
  -ScenarioPrompt "Search Store for Netflix and open the result without installing or purchasing." `
  -ScenarioPath .\tools\WinUIProfiler\harness\.artifacts\generated-store-scenario.json
```

Review selectors before capture. The DSL can automate third-party apps exposing UI Automation, but **private WinUI profiling is not supported merely because UI Automation works**. The target must pass the private-diagnostics readiness probe. Scenarios do not execute arbitrary scripts or launch processes; installation/purchase actions require intentional scenario selection.

## Comparison, evidence, and recovery

Optimization rebuilds the baseline in the guest, captures 3-10 reset samples per version (default 3), and analyzes all baseline directories. `session-evidence.json` exposes recorded frames and object event spans for nonediting agents; first/last observed events are not proof of allocation/destruction or complete lifetime. Snapshot correlation links, original SQLite files, and loss/error information remain available.

Candidate edits happen in guest worktrees, not host worktrees. Binary, empty, and duplicate patches are rejected. At most five distinct candidates are validated; unusable proposals have a separate cap of three times the attempt limit. Every paired baseline/candidate sample must meet the configured threshold with valid frame/ETW evidence. This conservative gate is not a claim of statistical significance.

On consistent improvement, write `successful-attempt.json`, the patch, analysis, and a manual-review decision, then stop. Otherwise stop at the attempt/proposal limit. Build, deployment, timeout, or capture failures halt immediately, preserve evidence, and require manual intervention. Guest command stdout/stderr are logged separately, and a timed-out command's PowerShell process is stopped by its specific PID; descendant processes can remain. There is no automatic host `git apply`, candidate cleanup, baseline reinstall, or checkpoint restore. Guest tasks/worktrees and remaining guest processes require manual inspection.

Host `.artifacts` contains logs, request/result JSON, SQLite data, snapshots, reports, and inert source patches. Guest ZIP entries are validated before extraction and restricted to evidence types; binaries/scripts/packages and path traversal are refused. Host DLLs are not deployment destinations.

Store scripts are guest-guarded before signing, SDK installation, policy/trust changes, or package operations. Store removal is current-user, not `-AllUsers`, but developer/capability policies and `LocalMachine\TrustedPeople` still affect the whole **guest**. A developer-unlock prerequisite may require a manual guest reboot. Do not reboot or repair the devbox as a workaround.

`StorePackage.ps1 -Action Restore`/`wsreset` and recorded registry/package metadata are **not transactional rollback** and cannot guarantee restoring original all-user registrations. Use the approved clean VM checkpoint for manual recovery.

## Validation boundary

`Tests\Harness.Tests.ps1` refuses execution outside a guest with approved distinct host/guest identity environment variables. The suite includes package-signing/build-tool cases and must run only in an explicitly authorized disposable VM. Do not remove its guard to run it on the devbox. VM execution validation, profiler/driver builds, and agent CLI permission/authentication compatibility must be completed before treating this harness as ready.
