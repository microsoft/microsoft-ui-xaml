# WPF Leak Detection Default

## Goal

Run leak detection by default for every WPF-hosted test. Opt out the 312 native tests and 14 managed test classes named in the failure list with `IgnoreLeaksForTest()`.

## Plan

1. Change `WindowHelper::ShutdownXaml()` so WPF shutdown-time leak detection runs without an explicit `EnableLeakDetection()` call.
   - Keep `/p:ForceLeakDetection` supported for diagnostics.
   - Keep `IgnoreLeaksForTest()` as the opt-out.
   - Preserve current UAP behavior.

2. Add an opt-out for every entry in `wpf-leak-detection-failures.txt`.
   - For native entries, call `IgnoreLeaksForTest()` at the start of the named test, not its whole class.
   - For managed class entries, call `IgnoreLeaksForTest()` in class cleanup immediately before `base.CommonClassCleanup()`.
   - Add a one-line comment explaining that managed leak detection runs at class cleanup, not after each test.
   - Reuse existing opt-outs where already present.
   - Native per-test opt-outs need no hosting-mode condition. Guard managed class opt-outs when the class contains both WPF and UAP tests.

3. Remove explicit `EnableLeakDetection()` calls that become redundant.
   - Preserve calls that test expected-leak behavior or otherwise exercise leak-detection infrastructure.

4. Update infrastructure coverage.
   - Remove all the test code and test infra code added in this topic branch that deals with the wpf-hosted test opt-in.
   - Prove a normal WPF test checks leaks without opting in.
   - Prove `IgnoreLeaksForTest()` suppresses the WPF check.
   - Prove forced and expected-leak scenarios still behave as intended.
   - For all tests and classes we touched, make sure we see a test run on VM for that test class with WPF-hosting mode the passes.

5. Delete `wpf-leak-detection-failures.txt` after every entry has a matching code opt-out.

6. Validate the migration.
   - Build the affected test projects and infrastructure.
   - Run representative clean, leaking, ignored, forced, and expected-leak tests on the VM.
   - Run the WPF-hosted suite, or the broadest practical existing WPF test selection, to find missing opt-outs.
   - Confirm every former list entry is accounted for and no unrelated hosting mode changed.

7. Commit the complete migration as one commit.

8. Clean up the diff.
    - Compare the current topic branch with main (5a1af81c6), and look at the diff.  Remove the cruft we don't need anymore for our current project.
    - Update any .md files to reflect the latest.
    - Build and run some tests to validate the cleanup.
    - Commit.

