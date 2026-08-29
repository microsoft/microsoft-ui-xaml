# Perf How-To

## Table of Contents

- [Overview](#overview)
- [Automatic monitoring of WinUI framework performance](#automatic-monitoring-of-winui-framework-performance)
- [Self-service Dev performance test runs in Azure DevOps](#self-service-dev-performance-test-runs-in-azure-devops)
- [Manual testing of any executable on Dev machine](#manual-testing-of-any-executable-on-dev-machine)
  - [Provision machine](#provision-machine)
  - [Quiet OS activity (Optional)](#quiet-os-activity-optional)
  - [Execute scenarios](#execute-scenarios)
    - [Deploy and run for UWP / Desktop apps](#deploy-and-run-for-uwp--desktop-apps)
  - [Process traces](#process-traces)
  - [Analyze results](#analyze-results)
  - [Scenarios](#scenarios)
- [Setup of Azure self-hosted physical hardware](#setup-of-azure-self-hosted-physical-hardware)

## Overview

Performance framework supports running precise measurement of CPU and reference set usage.  Currently the following use scenarios are supported:

* Automatic monitoring of WinUI framework performance (UWP and Desktop apps).
* Self-service Dev performance test runs in Azure DevOps.
* Manual testing of any executable on Dev machine (e.g. UWP, Desktop, Win32, XAML compiler).

## Automatic monitoring of WinUI framework performance

Every completed build of scenario apps in CI pipeline run on `main` branch will kick off an automatic run of performance tests.  The tests run for about 45 minutes in the WinUI-PerfTest pipeline.  After completion, the data will be available in two locations:

* All traces, processed data and reports are stored on the perf analysis network share.
* Traces from the current run and cumulative results are stored in run's pipeline artifacts.

The reporting component generates graphs and tables.

Currently, the best place to start looking for meaningful changes is by examining these tables:

* \*_report-top-regressions-improvements-delta-n.html - contains **the largest absolute** changes from baseline to trial versions.
* \*_report-top-historical-regressions-improvements-delta-n.html - contains data about **the largest relative** changes between consecutive versions.
* \*_report-all-changes-delta-n.html - contains all measured quantities and **absolute** changes from baseline to trial versions.

For all file names above, the _-delta-n_ suffix indicates what baseline is used.  It is calculated by latest - n shifts.

If there's something that stands out in \*_report-top-regressions-improvements-delta-n.html, the next step would be to look at appropriate historical graph which would tell whether this is within normal variance of the measurement or something worthy of investigation.  You can click on scenario name link to go to historical plot for this scenario.

Note that currently only amd64fre builds are tested and the OS version used is 20H2.

## Self-service Dev performance test runs in Azure DevOps

If you would like to understand performance implications of your change, there is a way to do that without setting everything up on you Dev box.  It will run in the same, controlled environment so comparisons between different runs are going to be meaningful.  Here are the steps to do that:

1. Start with baseline.  Either pick existing build (CI or Nightly pipelines) or kick off a new one.  Once artifacts are available, copy Build Id from the run's URL.
2. Navigate to the WinUI-PerfTest pipeline in Azure DevOps.
3. Click on `Run pipeline` and set the following variables:
  * userBuildIdConsumed - this is the Build Id from above.
  * userBuildPipelineId - pipeline definition id of the pipeline supplying the built scenario apps.  Use one of:
    ```
    CI      - 38157
    Nightly - 51598
    ```
  * userExperimentName - short name for what you're testing.  Use filename legal characters only and do not use spaces.
  * userPerfProfiles - what you'd like to measure.  Note, that all available profiles are defined in ```perf\profiles\profiles.json```.  You can specify multiple profiles by concatenating them with `#`.  E.g. `cpu#refset`.
     ```
    cpu           - CPU measurements, more iterations makes it more precise and useful in relative comparisons between versions.
    refset        - Reference Set measurements.
    cpu-stacks    - CPU measurements with stack information.  Less number of runs, used for detailed investigations.
    refset-stacks - Reference Set measurements with stack information.  Used for detailed investigations.
     ```
  * userPerfTestSet - set of scenarios to execute is calculated by matching provided value of this property agains set of `Tags` of all scenarios.  A good starting point is - `xamlperf`.  All available scenarios are defined in ```perf\profiles\scenarios.json```
4. Click `Run`.  It should take less than 45 minutes.
5. Repeat steps 1-4 for what you are comparing to baseline (trial).  Use a new Build Id and keep all the other fields **exactly the same**.

The results will be found on the perf analysis network share under `experiments\<your-alias>\<userExperimentName>` or in perf run artifacts (see section above).

## Manual testing of any executable on Dev machine

### Provision machine

First, you need to build needed components for testing.  After executing `init <arch><flavor>` on VS Dev Command Prompt:

* Go to `perf\scenarios` directory and execute `init.cmd <build-version>`.  If you'd like to use local build, don't specify `<build-version>` (but be sure to build.cmd & pack.cmd first).
* Run `msb MeasureMUX-set.sln` or the solution which contains your measurement app.
* Go to `perf\scripts` directory.
* **If you want to run tests on dedicated hardware:** run `provision.cmd <directory>`, where `<directory>` is a directory which will receive all necessary files that need to be copied to test machine.  Once the command completes, copy the files to your test machine.

### Quiet OS activity (Optional)

The CPU tests will be more reliable if there's less OS activity.  It is **strongly advised** to change OS settings only on a dedicated machine and not on your Dev box to avoid breaking it.  At this point there's no official script for quieting down the machine, but the following changes will improve results:

* Disable PreFetch by setting `HKEY_LOCAL_MACHINE\SYSTEM\CurrentControlSet\Control\Session Manager\Memory Management\PrefetchParameters\EnablePrefetcher` to 0.
* Disable CPU throttling by changing power profile in Control Panel to 'Performance'.
* Disable Windows Defender.

### Execute scenarios

#### Deploy and run for UWP / Desktop apps

Go to `scripts` directory and execute:
```run-set.cmd <scenarioTagsAndProfiles> <appsPath>```

`<scenarioTagsAndProfiles>` is an argument specifying selection criteria for scenarios and profiles to execute.  It has the following format: ```scenarioTag1[#scenarioTagN]/profileName1[#profileNameN]```.  `scenarioTag` is matched against contents of `Tags` property of scenarios defined in ```perf\profiles\scenarios.json``` and `profileName` is matched against `Name` property of profiles defined in ```perf\profiles\profiles.json```.
`<appsPath>` is an argument specifying path containing scenario apps.

**Note**: The `XAML.wprp` file used by the infrastructure declares several hardware counters; if the device where the tests are running does not fully support all of those counters, the execution will fail with a generic error message like the one below:

`Error: TAEF: [HRESULT 0xC5580628] Caught critical exception during test execution. (Failed to start the EtwLogger trace.)`

From an elevated command prompt, run `wpr -pmcsources` to verify what counters the device supports, and comment out any from the `XAML.wprp` file which do not apply. 

### Process traces

Once all tests are completed they will be placed in `%LOCALAPPDATA%\WinAppPerf\current` directory.  Go to `scripts` directory and run `process.cmd <experiment>` command.  The results will be placed in `%LOCALAPPDATA%\\WinAppPerf\\experiments` directory.  If an experiment already exists the results will be merged, otherwise new experiment directory is created.

### Analyze results

The experiments results directory contains the original trace files and a number of CSV files.  In root directory there is a file containing merged results for all runs `cumulative.csv`.  In `processed` directory each shift directory will contain a number of files:

* `*.raw.csv` raw data extracted from scenario trace.  These files will show all data from each run of scenario (e.g. UWP apps in CPU profile are run multiple times, so values from each iteration will be visible here).
* `*.stat.csv` contain filtered statistics extracted from raw data.
* `shift.agg.csv` is the file which will be merged into `cumulative.csv` in directory above.

### Scenarios

The following apps are currently used for performance measurement / PGO training of WinUI:

| Scenario app                   | Type    | Language | Description                       | Used in |
|--------------------------------|---------|----------|-----------------------------------|---------|
| XAMLPerf.ButtonApp.Cpp.MUX     | UWP     | C++      | Grid of 225 buttons               | Measure |
| XAMLPerf.ControlsApp.Cs.MUX    | UWP     | C#       | Multiple controls and navigation  | PGO     |
| XAMLPerf.HelloWorldApp.Cs.MUX  | Desktop | C#       | Simple text rendering             | PGO     |
| XAMLPerf.HelloWorldApp.Cpp.MUX | Desktop | C++      | Simple text rendering             | Measure |
| XAMLPerf.MinApp.Cpp.MUX        | UWP     | C++      | Empty app created from VSIX       | Measure |
| XAMLPerf.MinApp.Cs.MUX         | UWP     | C#       | Empty app created from VSIX       | Measure |
| XAMLPerf.NavApp.Cs.MUX         | UWP     | C#       | NavigationView usage              | Measure |
| XAMLPerf.ScrollingApp.Cs.MUX   | UWP     | C#       | Multi-modal scrolling             | PGO     |
| XAMLPerf.TextBlockApp.Cpp.MUX  | UWP     | C++      | Text rendering of 1125 TextBlocks | Measure |

Below are equivalent applications using system XAML.  They are used for comparison measurements to MUX.

| Scenario app                   | Type    | Language | Description                       | Used in |
|--------------------------------|---------|----------|-----------------------------------|---------|
| XAMLPerf.MinApp.Cpp.WUX        | UWP     | C++      | Empty app created from VSIX       | Measure |
| XAMLPerf.MinApp.Cs.WUX         | UWP     | C#       | Empty app created from VSIX       | Measure |
| XAMLPerf.NavApp.Cs.WUX         | UWP     | C#       | NavigationView usage              | Measure |

## Setup of Azure self-hosted physical hardware

1. Install desired version of Windows on perf box.  Picking LTSC variant is highly recommended for a couple of reasons: it doesn't update as often and there are less installed apps and running services.  The currently installed image is 20H2 LTSC.

2. Create local admin user named perftest.

3. Install and configure Azure self-host agent by following [these instructions](https://docs.microsoft.com/en-us/azure/devops/pipelines/agents/v2-windows?view=azure-devops).  Use the following settings (abbreviated):

    ```
    C:\vsts-agent-win-x64-2.185.1>config
    ...
    Enter server URL > https://dev.azure.com/<your-org>
    Enter authentication type (press enter for PAT) >
    Enter personal access token > ****************************************************
    ...
    Enter agent pool (press enter for default) > WinUI-PerfTest
    Enter agent name (press enter for DESKTOP-Q6QS2SB) >
    ...
    Enter work folder (press enter for _work) >
    Enter Perform an unzip for tasks for each step. (press enter for N) >
    Enter run agent as service? (Y/N) (press enter for N) >
    Enter configure autologon and run agent on startup? (Y/N) (press enter for N) > y
    Enter User account to use for autologon > perftest
    Enter Password for the account DESKTOP-Q6QS2SB\perftest > *********
    ...
    Enter Restart the machine at a later time? (Y/N) (press enter for N) >y
    ```

4. Do not restart yet, as there is one modification that needs to be made.  By default on startup the agent will be executed in non-elevated user context, but to run ETWLogger it needs to be run in elevated one.  Here's a workaround:

    * Open regedit and go to `HKEY_CURRENT_USER\SOFTWARE\Microsoft\Windows\CurrentVersion\Run`.
    * Azure agent should have created a key that looks something like this:
      ```
      VSTSAgent = C:\WINDOWS\system32\cmd.exe /D /S /C start "Agent with AutoLogon" "C:\vsts-agent-win-x64-2.185.1\run.cmd" --startuptype autostartup
      ```
    * Copy the contents of the key into text file `c:\start-agent.cmd`.
    * Replace the key value with:
      ```
      VSTSAgent = powershell -Command "Start-Process c:\start-agent.cmd" -Verb RunAs
      ```
    * Lastly, disable admin prompts so the agent can startup without user intervention after reboot.  Search for `secpol.msc` and navigate to `Local Policies > Security Options > User Account Control: Behavior of the elevation prompt for administrators in Admin Approval Mode`.  Change the value to `Elevate without prompting`.

5. Disable miscellaneous startup items.  Search for `Startup apps`.  Turn off all unneeded, except for the command for starting Azure selfhost agent.

6. Compile and deploy perf infrastructure.

    * On Dev box, open and initialize dev environment to `x64fre`.
    * Go to `perf\scripts` directory and execute `provision <network-share-path>`.  Use a share name that the perf box has access to.
    * On perf box copy quieting TAEF test from the perf analysis share:
      ```
      xcopy <perf-share>\bin\Microsoft.PerfGates.Test.Config.dll c:\perf\test /y
      ```
    * Make sure that perf box has read-write access to the share storing results, e.g. create local user on machine that is providing the share (in this case `nunatak\perftest`) and store these credentials on perf box.

7. Search for `Developer Settings` and enable `Developer Mode`.

8. Install the [Visual C++ Redistributable](https://learn.microsoft.com/en-us/cpp/windows/latest-supported-vc-redist) for x64.

9. Install and start TAEF service.

    * Copy TAEF binaries from the appropriate internal distribution location. 
    * From elevated prompt execute:
      ```
      wex.services.exe /install:te.service /localonly /runas:system
      net start system
      ```

10. Install Python 3.11 from official site to enable report generation.  For convenience install it for all users in `c:\python311`