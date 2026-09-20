Continue Jesse's WinUI binary-size reduction work in this checkout. Complete
one focused experiment, then exit; a supervisor will start a fresh agent.
You have at most two hours. Leave useful progress before time runs out.

FIRST READ
- Repository instructions, description.md, and plan.md.
- Earlier turns under $env:RALPH_HISTORY. Each turn owns a directory there.
- If present, earlier runs under $env:RALPH_LEGACY_HISTORY are read-only
  references for the agent. Do not write new files there.
- Historical evidence and scripts, as read-only references:
  D:\SizeBench\artifacts\winui-size-reduction\20260918-1428-template-folding-9dd96830
Put new measurements, logs, source backups and progress.json in
$env:RALPH_TURN_DIR. Do not overwrite an earlier experiment.
New run data belongs under this checkout's artifacts\ralph directory on D:,
not AppData. Use the inherited TEMP/TMP directory for temporary files.
After a successful turn with a new DLL/PDB pair, the supervisor deletes
DLL/PDB files from older turns in both history locations. It keeps this
turn's snapshots and all older logs, hashes, reports, and progress records.
Do not depend on earlier turns' binaries still being present; use their
reports and rebuild the current source for a fresh baseline.

GOAL
Find and implement another behavior-preserving reduction in the actual file
size of Microsoft.ui.xaml.dll. Run SizeBench before speculative source edits,
then rebuild and run it again to decide whether to keep the change.
Starting commit a8b8ae4c1 saved 111104 bytes (108.5 KiB); description.md
records those changes and later wins. Avoid repeating rejected experiments.
In particular,
noinline alone does not stop LTCG from specializing helpers.

BUILD AND MEASURE
1. Inspect HEAD and the working tree. A previous agent may have timed out.
   Before editing anything, journal which files you own and back up their
   original contents. If recovering interrupted work, preserve its evidence
   and undo only the clearly identified speculative edits. Never silently
   treat a half-finished candidate as the baseline. If ownership is unclear,
   write a blocker in progress.json and exit without changing those files.
2. Establish a baseline built from the current source. Keep x64 Release with
   PGO OFF, using initialization and build in the SAME cmd.exe process:
   $command = 'call "{0}\init.cmd" amd64fre /nopgo /envcheck /notitle && set PGOBuildMode && set Configuration && set Platform && set VCToolsVersion && call "{0}\Build.cmd" mux /q' -f $env:RALPH_REPO_ROOT
   & $env:ComSpec /d /c $command > $buildLog 2>&1
   Check both exit code and the whole build log: Build.cmd can return zero
   on failure. Confirm PGOBuildMode=Off. Do not use Build.cmd /i or
   initrun.ps1; those do not preserve the explicit /nopgo setting.
   Preserve compiler/linker/security settings, exports, metadata and ABI.
   Do not overlap builds or do a broad clean.
   If MSBuild loses an inherited empty GIT_CONFIG_VALUE for core.fsmonitor,
   represent only that empty override as "false" in the child environment.
   Do not change repository/global Git configuration.
3. Copy and hash the DLL/PDB into a fresh baseline directory before analysis:
   BuildOutput\bin\amd64fre\Product\Microsoft.ui.xaml.dll
   BuildOutput\bin\amd64fre\Symbols\Product\Microsoft.ui.xaml.pdb
   Analyze preserved copies, never mutable build outputs.
   Keep source revision/patch, build command/logs, hashes and tool identity.
4. Use this frozen SizeBench CLI for BOTH sides; do not upgrade or republish:
   D:\SizeBench\artifacts\cli-frozen\winui-template-folding-20260918-1428\sizebench-cli.exe
   Its .identity.json, .manifest.json and .cli.md sidecars are alongside the
   deployment directory. Verify hashes, not just its version string.
   Expected managed identity:
   e48a876c7c9dcd2ff9248d28a630f85873439ccb44a9c0a4ecf30c1d286af4f0
   Collect once, then query offline; check every exit code and JSON status:
   & $cli analyze --binary $dll --pdb $pdb --reports summary,sections,coff-groups,libs,compilands,template-foldability --output $analysis
   & $cli query summary --input $analysis --output $summary > $null
   & $cli query template-foldability --input $analysis --limit 20 --output $leads > $null
   Save receipts/stderr. Project only selected fields when browsing nested
   evidence. Missing coverage needs collection; it is not a zero result.
5. Investigate the source and try one bounded optimization. Preserve behavior,
   ownership, error handling, interface shape, and security. Change generators
   or shared definitions instead of hand-editing generated outputs.
   Rebuild identically, preserve the candidate DLL/PDB, and analyze again.
   Confirm the changed code actually rebuilt and the DLL relinked.
6. Accept only a strictly SMALLER ACTUAL DLL FILE attributable to the change.
   Report file, analyzed-section and virtual-section bytes separately.
   An unchanged aligned file length is not a file-size improvement.
   Heuristic savings and overlapping symbol/family totals must not be summed.
   Zero-sized symbols may be aliases; positive function sizes can include
   separated blocks. Neither is automatically a contiguous disassembly range.
   Review semantic and performance risks in addition to the required tests.

TEST VALIDATION BEFORE COMMITTING AN ACCEPTED CANDIDATE
- Run this section only for a candidate you intend to keep. Failed or
  rejected experiments do not require tests, VM setup, or the prodtest build
  below. Restore only your speculative source changes, preserve the findings,
  and make the description-only rejection commit described below.
- Before committing an accepted candidate, run the full
  CalendarViewIntegrationTests suite on the local VM
  ge_current-260820-Desktop, in WPF mode with amd64fre /nopgo.
  Test the final candidate source state. Do not reuse a prior turn's result.
- Ensure the VM is running with an unlocked, logged-in desktop and no other
  test run. Use the cached VM credentials. Do not change machine permissions.
  If the VM, credentials, or desktop are unavailable, record a blocker and
  exit without committing.
- Build the product and test targets with the same explicit PGO-off setup:
  $testBuildCommand = 'call "{0}\init.cmd" amd64fre /nopgo /envcheck /notitle && set PGOBuildMode && set Configuration && set Platform && call "{0}\Build.cmd" prodtest /q' -f $env:RALPH_REPO_ROOT
  & $env:ComSpec /d /c $testBuildCommand > $testBuildLog 2>&1
  Check the exit code and whole build log, and confirm PGOBuildMode=Off.
  If this build changes the measured DLL, refresh its preserved snapshot,
  hashes and size analysis before deciding whether the candidate saves bytes.
- Refresh and deploy the payload, then run the tests:
  $testCommand = 'call "{0}\init.cmd" amd64fre /nopgo /envcheck /notitle && set PGOBuildMode && pwsh -NoProfile -File "{0}\tools\run-tests-on-vm.ps1" -VMName "ge_current-260820-Desktop" -Platform x64 -Configuration fre "*CalendarViewIntegrationTests*" -HostingMode WPF' -f $env:RALPH_REPO_ROOT
  & $env:ComSpec /d /c $testCommand > $testLog 2>&1
  Keep logs and failure artifacts under $env:RALPH_TURN_DIR and use inherited
  TEMP/TMP. Do not use -SkipPayload or exclude either known failing test.
  Confirm the deployed Microsoft.ui.xaml.dll matches the final built DLL
  by SHA256, including the payload's root and Test directory copies.
- The 2026-09-19 baseline ran all 121 tests: 119 passed, 2 failed, and none
  were blocked, skipped, or not run. The only allowed failures are:
  TestCICEvents and VerifySelfAdaptivePanel, both with the assertion
  IsTrue(didOutputMatchMaster). These may be fre-specific; that cause has
  not been established. Evidence:
  D:\x1\artifacts\ralph-validation\20260919-calendarview-wpf
- Zero failures or a subset of those two known failures is acceptable.
  A count of two alone is NOT sufficient: check the exact failing names
  and assertions. Any additional failing test, new failure mode, crash,
  blocked/skipped/missing test, or incomplete run must be fixed and the
  full suite rerun before committing the candidate. Do not widen the
  allowed-failure list. If unresolved or out of time, do not commit the
  candidate. Either record a blocker and exit, or reject the experiment and
  follow the description-only rejection path without another test run.
- Inspect the complete TAEF summary and failure details, not just the process
  exit code: the runner returns nonzero for the two allowed failures too.
  Record the counts, failing names, build flavor, PGO state, hosting mode,
  tested DLL hash and log paths in progress.json and description.md.

KEEP OR REJECT
- Build, size analysis, and the test validation above are required before
  committing an accepted candidate. Tests are not required for a
  description-only rejection commit. Do not claim all tests passed when the
  two known failures remain.
- If the candidate does not improve file size, fails the build, or has
  unjustified behavioral/performance risk, preserve the findings and undo
  ONLY your own speculative changes. Do not commit rejected experiments.
- If it is good, update description.md with the change, baseline/after sizes,
  incremental and cumulative savings, provenance and limitations. Preserve
  earlier measurements and distinguish earlier test runs from this turn.
  Review the diff, stage only your intended source files and description.md,
  then make a local commit. Include this trailer:
  Co-authored-by: Copilot App <223556219+Copilot@users.noreply.github.com>
- If it's reject result, still update description.md with your findings, remove your code
  changes, and commit.  So if it's a reject result, we should see a commit with just a description.md update.
  Do not run tests on the rejected candidate or rerun them after restoring
  the source. Record "Tests not run: experiment rejected" if no tests ran;
  otherwise preserve the actual results and state that no rerun was required.
- Record the outcome, commit, rejected ideas, CLI concerns and useful
  next leads in progress.json so the next fresh agent can pick up the work.

GUARDRAILS
Stay on the existing branch. Never push, amend, stash, reset, switch branches,
use git clean, or broadly restore files. Preserve unrelated/staged work and
preexisting untracked files. If unrelated changes are already staged, or
another writer changes your files, record a blocker and stop. Do not edit or
commit ralph.ps1, ralph-instructions.md, or the supplied plan.md. Do not change
SizeBench, build flags, or machine permissions.
Do not start detached/background work or extra agents; wait for your build
and analysis processes to finish. On ambiguity or infrastructure failure,
write a clear blocker and exit rather than claiming success.
