# How to Repro a Xaml Test Failure

This page is a quick-start guide for folks who need to investigate a test failure with minimal ramp-up.

Here's how you can repro the test failures, given a link to a pipeline:

1. Download the TestPayload folder from the artifacts link.  The artifacts link is here on the pipeline page:

![](images/pipeline-run-header.jpg)

2. Extract the zip on your test machine.  You can often use your dev machine for this, but for some tests it's most reliable to
use a VM with resolution 1024x768.

3. In an admin cmd prompt on your test machine, navigate to testpayload\x86chk, and run "testmachine-prerun.cmd".  (Note
   you may see some failures here due to packages being in use or already installed, those are generally ignorable)

4. Run your test by running "runtests <testname>".  If the failing test starts with "WPF-", use the "-wpfMode" switch.
   Wildcards like * are honored for "testname", it's just a wrapper for TAEF's te.exe.

If the test app crashes, the dmp will be in c:\dumps.

See [testing-faq.md](testing/testing-faq.md) for more information.






