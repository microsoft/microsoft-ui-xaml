# Build Pipelines

## Table of Contents

- [Overview](#overview)
- [Build Infrastructure](#build-infrastructure)
- [Hosted Agent Pools and Managed Images](#hosted-agent-pools-and-managed-images)
- [Main Pipelines](#main-pipelines)
  - [WinUI-Xaml-PR](#winui-xaml-pr)
  - [WinUI-Xaml-Nightly](#winui-xaml-nightly)
- [Other Pipelines](#other-pipelines)
  - [WinUI-Xaml-Release](#winui-xaml-release)
  - [WinUI-PGOInstrument](#winui-pgoinstrument)
- [Common Pipeline Stages](#common-pipeline-stages)
  - [WinUI-BuildWinUI-Stage](#winui-buildwinui-stage)
  - [WinUI-RunTests-Stage](#winui-runtests-stage)
  - [WinUI-RunStaticTests-Stage](#winui-runstatictests-stage)
    - [Running WinMD Compat tests locally (VerifyWinMDCompat.ps1)](#running-winmd-compat-tests-locally-verifywinmdcompatps1)

## Overview

We have a number of Azure Pipelines that we use to run builds and test passes.

[General documentation on Azure Pipelines is available here.](https://docs.microsoft.com/en-us/azure/devops/pipelines)

We have multiple different Pipelines in use in our repo. They execute similar jobs. They differ mostly in the specific
configurations and stages that they run. The Pipelines are defined by .yml files checked in under the 'build' directory.
Most of the logic  is shared via common templates (under 'build\AzurePipelinesTemplates').

Having multiple Pipelines allows us to control which configurations and stages get run at what times.

Some of these Pipelines get triggered automatically. Any of them can be manually queued if required.

## Build Infrastructure
All of these Pipelines use an internal build infrastructure that extends Azure Pipelines to ensure security and compliance.

The build machines (i.e. Agent Pools) come from this build infrastructure. We don't manage or control the build machines.

The build machines get created on demand when a Pipeline gets run. They are then thrown away once the build completes.
As such you don't need to worry about the build machines getting into a bad state - a build will always run on a fresh
instance of the machine.

## Hosted Agent Pools and Managed Images

We want to execute the tests on specific versions of Windows OS. To do this we run the Test Stage of the Pipelines on
special Agent Pools. These are hosted AzureDevOps Agent Pools.
These pools are dedicated to this project's testing pipelines. We can use the Azure portal
to configure the pool as needed.

We use the following agent pools for testing:

* WinDevPool-Test: A pool for regular x64 agents. 
* WinDevPool-Arm64: This contains Arm64 agents that we can use to run arm64 tests on.

The Agents in these agent pools are configured with images running the appropriate Windows version.
These images are all managed images. Managed images are built
on top of Image Factory.

## Main Pipelines

Some of these pipelines push to the
WinUI.Dependencies feed,
for others the way to get the built packages is from the pipeline run's artifacts.
Specifically, the transport package for WinUI, published by the Nightly/CI/Release pipelines, is
available in the WinUI.Dependencies feed.


### WinUI-Xaml-PR
WinUI-Xaml-PR
This builds the product in multiple configs (e.g. x86chk, x64fre). Runs tests for x86chk on 20H2, including Sample App tests.
This Pipeline is used to validate Pull Requests. A Pull Request must get a successful run of this Pipeline before
getting merged to main.

### WinUI-Xaml-Nightly
WinUI-Xaml-Nightly
This builds the product in a larger set of configs. It runs the tests on a wider set of configs than the PR Pipeline.
It also build and runs the Sample App tests.
This is configured to run against `main` every night.


## Other Pipelines

### WinUI-Xaml-Release
WinUI-Xaml-Official
This is the Pipeline that we use to create an official release that we plan to make available publicly. 
The main difference between this Pipeline and the others is that this Pipeline produces binaries that are signed by 'Microsoft Corporation'.

### WinUI-PGOInstrument
WinUI-PGOInstrument
This Pipeline runs our PGO instrumentation. See [perf-pgo.md](../perf-pgo.md) for details.


## Common Pipeline Stages

There are some stages common to most pipelines. These can be run independently when running a pipeline manually by clicking on "Stages to run"
and unchecking the stages that you don't want run.  Note that some stages depend on artifacts produced by earlier stages, though - if so,
that dialog will report as much - and the later stages will fail if you uncheck the stages they depend on.

### WinUI-BuildWinUI-Stage
WinUI-BuildWinUI-Stage.yml
Builds the WinUI 3 product and test binaries and resources.

### WinUI-RunTests-Stage
WinUI-RunTests-Stage.yml
Runs runtime tests: unit tests, API tests, and interaction tests. Can also be used to run scenario tests
if ScenarioTestSuite is passed in as the test suite.

To run tests from this stage locally locally, you can use test\CreateTestPayload.cmd to generate a test payload directory, and then either
locally or on a VM, run testmachine-prerun.cmd once and then use runtests.cmd thereafter to specify what tests to run.
Microsoft.UI.Xaml.Controls.dll tests can also be run by opening controls\MUXControls.sln in Visual Studio, building within Visual Studio,
and then selecting the tests to run via Test Explorer.

### WinUI-RunStaticTests-Stage
WinUI-RunStaticTests-Stage.yml

Runs static analysis on the built binaries: currently, only performs WinMD compat tests.

These WinMD compat tests will fail if a new type is added to a shipped contract or if a type in a shipped contract has
been modified in some way (e.g., adding a new property). To remedy these failures, you should add a new contract
version in dxaml\xcp\tools\XCPTypesAutoGen\XamlOM\Model\Contracts.cs and then attach the new types, properties, or
methods to that new contract version instead of the existing contract version. Note that the number used in Xaml
codegen's `[Platform]` and `[Version]` tags do not need to match the version of the contract. You can have a
`[Platform(2, typeof(Microsoft.UI.Xaml.WinUIContract), 4)]` that ties type version 2 to contract version 4, for example.

#### Running WinMD Compat tests locally (VerifyWinMDCompat.ps1)

To run these tests yourself locally, you can run the script `build\PipelineScripts\VerifyWinMDCompat.ps1`. After fixing
issues as above and rebuilding the IDL files, you should run this to ensure that compat issues have been fixed.
