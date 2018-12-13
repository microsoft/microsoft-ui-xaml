This directory contains code and configuration files to run WinUI tests in Helix.

Helix is a cloud hosted test execution environment which is accessed via the Arcade SDK.
More details:
* [Arcade](https://github.com/dotnet/arcade)
* [Helix](https://github.com/dotnet/arcade/tree/master/src/Microsoft.DotNet.Helix/Sdk)

WinUI tests are scheduled in Helix by the Azure DevOps Pipeline: [RunHelixTests.yml](../RunHelixTests.yml).

The workflow is as follows:
1. NuGet Restore is called on the packages.config in this directory. This downloads any runtime dependencies that are needed to run tests.
2. PrepareHelixPayload.ps1 is called. This copies the necessary files from various locations into a Helix payload directory. This directory is what will get sent to the Helix machines.
3. RunTestsInHelix.proj is executed. This proj has a dependency on [Microsoft.DotNet.Helix.Sdk](https://github.com/dotnet/arcade/tree/master/src/Microsoft.DotNet.Helix/Sdk) which it uses to publish the Helix payload directory and to schedule the Helix Work Items. The WinUI tests are parallelized into multiple Helix Work Items.
4. Each Helix Work Item calls [runtests.cmd](runtests.cmd) with a specific query to pass to [TAEF](https://docs.microsoft.com/en-us/windows-hardware/drivers/taef/) which runs the tests.
5. TAEF produces logs in WTT format. Helix is able to process logs in XUnit format. We run [ConvertWttLogToXUnit.ps1](ConvertWttLogToXUnit.ps1) to convert the logs into the necessary format.
6. RunTestsInHelix.proj has EnableAzurePipelinesReporter set to true. This allows the XUnit formatted test results to be reported back to the Azure DevOps Pipeline.
