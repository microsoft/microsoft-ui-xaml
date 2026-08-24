# WinUI CI Test System Overview

This document gives a high-level overview of our CI Build and Test System in WinUI.

## Table of Contents

  - [Azure Pipelines](#azure-pipelines)
  - [Build Infrastructure and Agent Pools](#build-infrastructure-and-agent-pools)
  - [Testing](#testing)
  - [The Process in Depth:](#the-process-in-depth)
    - [Create the test payload: CreateTestPayload.ps1](#create-the-test-payload-createtestpayloadps1)
    - [Generating Helix Work Items: GenerateHelixWorkItems.ps1](#generating-helix-work-items-generatehelixworkitemsps1)
    - [Machine setup: testmachine-prerun.cmd](#machine-setup-testmachine-preruncmd)
    - [Test Re-try logic](#test-re-try-logic)
    - [Publish Test Results](#publish-test-results)
    - [Result analysis](#result-analysis)
    - [Publish Test Artifacts](#publish-test-artifacts)
- [Scenario App Tests](#scenario-app-tests)

## Azure Pipelines

For our Nightly builds and for PR validation, we use Azure Pipelines, which is a part of Azure DevOps. 
Azure DevOps is a public service from Microsoft. It is the evolution of what used to be named "Visual Studio Online".  
Official documentation for Azure Pipelines is available here: https://docs.microsoft.com/en-us/azure/devops/pipelines

In our repo we have two main Pipelines:
* WinUI-Xaml-PR: Used to validate PRs 
* WinUI-Xaml-Nightly: This runs nightly to produce a build of WinUI.
* WinUI-Xaml-Official: This produces the official builds of WinUI that may end up shipping publicly.

See [build-pipelines.md](../publishing/build-pipelines.md) for more info on the build Pipelines that are used in this project.

## Build Infrastructure and Agent Pools

Our build infrastructure is an internal system that extends Azure Pipelines to ensure security and compliance. Our Pipelines build on 
top of this system.

The build stage of the Pipelines run on build machines that come from the build infrastructure. But for testing, we want to execute the 
tests on particular versions of Windows OS. So we have a dedicated set of agents for that.

We use hosted Agent Pools to create and manage the set of VMs that we use to execute the tests. For the Test stage of the 
Pipeline instead of executing on the build infrastructure agents, we execute on these dedicated test agents.

## Testing

After the Build stage, we execute the Test stage.

Note: (Up until the WinAppSDK 1.3 release in early 2023, we used a service called "Helix" to run our test suit -- you will 
still see some references to the word "Helix", even though we don't use Helix anymore.)

## The Process in Depth:

When the Pipeline runs, the following takes place.  

**Steps:**

1. Runs the Build stage (this builds both product and test binaries)
2. The Run Tests Stage starts. [**WinUI-RunTests-Stage.yml**](../../build/AzurePipelinesTemplates/WinUI-RunTests-Stage.yml)
3. The output of the Build job is downloaded. [**CreateTestPayload.ps1**](../../test/CreateTestPayload.ps1) is run to produce the TestPayload.
4. We discover the tests from the build and generate the HelixWorkItems. This is done by [**GenerateHelixWorkItems.ps1**](../../Helix/common/pipeline/GenerateHelixWorkItems.ps1).
5. The TestPayload and the work items xml is published to the Pipeline as an artifact.
6. We execute batches of the test work items in parallel on agent VMs that are running the version of Windows that we want to target. We execute [**RunTestPassSliceOnBuildAgent.ps1**](../../Helix/common/pipeline/RunTestPassSliceOnBuildAgent.ps1) on these test machines.
7. Test machine setup ([**testmachine-prerun.cmd**](../../test/scripts/testmachine-prerun.cmd))
8. Failing tests are re-tried as needed
9. Test results are published to the Pipeline.
10. In the case of failing or unreliable tests, we upload supporting files to the HelixTestOutput artifact.

Here's how the pipelines and scripts are organized to do this work:
* Run the Build stage (this builds both product and test binaries)
* The RunTests Stage runs.  **WinUI-RunTests-Stage.yml**
  * Create the test payload.  **WinUI-CreateTestPayload-Job.yml**
    * Download build
    * Call **CreateTestPayload.ps1**
    * **WinUI-CreateHelixProjFile-Steps.yml**
       * Call **GenerateHelixWorkItems.ps1**
         * Call **pipeline/GenerateHelixWorkItems.ps1**
           * Writes out a .proj file for each test group (You can see these in the pipeline artifacts at /helixworkitems).
  * **WinUI-RunTestPassOnPipeline-Job.yml**
    * Runs 20 agents in parallel.  On each agent, we:
      * Download artifacts
      * Call **RunTestPassSliceOnBuildAgent.ps1**
        * Executes **testmachine-prerun.cmd**
        * Read proj file that describes test command to run for this agent (created earlier by **pipeline/GenerateHelixWorkItems.ps1**)
        * Each test command calls **RunHelixWorkItem.ps1**
          * Run the test command (a TAEF query that runs multiple tests)
          * For each failed test, re-run once.  If the re-run failed, run the test 9x in a loop.          
      * PublishTestResults
      * Call **UpdateUnreliableTests.ps1**


### Create the test payload: CreateTestPayload.ps1

Copies files from multiple sources and places them in the required directory structure for running tests.
Sources:
* Build drop
* Scripts copied from repo

### Generating Helix Work Items: GenerateHelixWorkItems.ps1

To execute the tests we batch the set of tests into 'work items'. When we execute the tests we use Azure Pipelines 
parallelization to run multiple test Jobs at once. We distribute the set of work items across these Jobs so that we can
execute the tests in a reasonable amount of time.

We could define a hard-coded set of work items, however this does not scale very well as the list of work items must 
always be kept in sync with the test code as tests get added/removed/etc.  
Instead, we generate the set of Work Items dynamically. This is done by 
[**GenerateHelixWorkItems.ps1**](../../Helix/common/pipeline/GenerateHelixWorkItems.ps1).  
This script runs `te.exe /listproperties` against a set of test binaries and parses the output. It produces a set of 
work items from this.  
There are two strategies the script uses to generate work items:
* **CreateWorkItemPerModule**: We create a work item that runs all the tests in a given test dll.
* **CreateWorkItemPerTestClass**: We create a work item for each test class that is defined in a given test dll.

The default is CreateWorkItemPerModule, but a test class can specify the test metadata 
`HelixWorkItemCreation=CreateWorkItemPerTestClass`. 

E.g.:  
For C++: `TEST_CLASS_PROPERTY(L"HelixWorkItemCreation", L"CreateWorkItemPerTestClass")`  
For C#: `[TestProperty("HelixWorkItemCreation", "CreateWorkItemPerTestClass")]`

When a work item per class is being generated, a single test class can be further subdivided into multiple work items. 
To do this, the test method in the class should define a value for the "TestSuite" property.

E.g.:  
For C++: `TEST_METHOD_PROPERTY(L"TestSuite", L"A")`  
For C#: `[TestProperty("TestSuite", "A")]`

The values for this property can be any string. All test methods with the same value will execute as a work item. So, 
for example, you could split a test class into suites "A", "B" and "C".

You can see the generated Helix Work Items by examining the build artifact 'HelixWorkItems'. 
Each work item executes [`RunHelixWorkItem.cmd`](../../Helix/common/test/RunHelixWorkItem.cmd) with a set of arguments.

### Machine setup: testmachine-prerun.cmd

[**testmachine-prerun.cmd**](../../test/scripts/testmachine-prerun.cmd) is a one-time script that needs to be run on the 
test machines. It configures the machines as needed and installs any required components. Most of the logic is contained 
in these scripts:  
* [TestPass-OneTimeMachineSetupCore.ps1](../../Helix/common/test/TestPass-OneTimeMachineSetupCore.ps1)  
* [TestPass-EnsureMachineStateCore.ps1](../../Helix/common/test/TestPass-EnsureMachineStateCore.ps1)  
* [TestPass-OneTimeMachineSetup.ps1](../../Helix/scripts/TestPass-OneTimeMachineSetup.ps1)  
* [TestPass-EnsureMachineState.ps1](../../Helix/scripts/TestPass-EnsureMachineState.ps1)  

### Test Re-try logic

In the case where a test fails, we re-run it 10 more times. If it passes 5/10 times or more, we consider the test 
'unreliable'. If it passes fewer than 5/10 times we consider the test a failure.

### Publish Test Results

After RunTestPassSliceOnBuildAgent.ps1 has finished executing the tests it will produce a set of testResult xml files.
These xml files are in xUnit format. We use the PublishTestResults Azure Pipelines task to publish these test results to 
the Pipeline. 

Note, TAEF produces test results in its own format that Azure Pipelines does not understand. For this reason we convert
the TAEF result into xUnit format. This is done by [ConvertWttLogToXUnit.ps1](../../Helix/common/test/ConvertWttLogToXUnit.ps1)
which is primarily implemented in [HelixTestHelpers.cs](../../Helix/common/test/HelixTestHelpers.cs).

### Result analysis

After the test results have been published we do some extra post processing on the test results.

**Update Unreliable Tests**

The xUnit results format does not natively support the test re-try logic that we do. So we do an extra step to report 
the data about the multiple runs. For each test that is re-run, we create and upload `_subresults.json` file. This 
includes information about the results of re-running the test. In the Azure Pipeline, we download this json file and 
use it to update the test results in the Pipeline with the extra information. Tests that failed initially. but have a 
sufficiently high pass-rate are marked as "Warning" instead of "Failed".   
Implementation: [**UpdateUnreliableTests-Pipeline.ps1**](../../Helix/common/pipeline/UpdateUnreliableTests-Pipeline.ps1)  
The implementation of this script uses the Azure DevOps api. This is documented here:
* Azure DevOps REST API: https://docs.microsoft.com/en-us/rest/api/azure/devops/

### Publish Test Artifacts

After updating the test results, we publish a test artifact containing files produced by the test execution. This includes
the full logs of the tests and potentially extra files to aid in debugging, for example screenshots that were taken at the
time the test failed or memory dump files.

The Pipeline run used to produce a single test output artifact that contained all of the files. However, the build infrastructure mandates
that each Job produce its own artifact - multiple jobs cannot write to the same artifact. As a result we produce dozens of 
these test output artifacts in the Pipeline. So if you need to find the output for a particular test you will need to download 
the artifact for the Job that ran it. For example `RunTestsStage_RunTests23H2_6_Succeeded`.

# Scenario App Tests

In addition to the main set of functional tests (the devtest suite), we also have a set of Scenario App tests. 
These tests execute in their own Stage, but the process is the same as the main set of tests.

See [test-code-in-WinUI.md](./test-code-in-WinUI.md) for more details on the two test suites.