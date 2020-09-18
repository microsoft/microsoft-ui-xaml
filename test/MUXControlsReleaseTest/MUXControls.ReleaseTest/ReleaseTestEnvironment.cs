// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

using Windows.UI.Xaml.Tests.MUXControls.InteractionTests.Infra;
using Common;

#if USING_TAEF
using WEX.TestExecution;
using WEX.TestExecution.Markup;
using WEX.Logging.Interop;
#else
using Microsoft.VisualStudio.TestTools.UnitTesting;
using Microsoft.VisualStudio.TestTools.UnitTesting.Logging;
#endif

namespace MUXControls.ReleaseTest
{
    // This is marked as a test class to make sure our AssemblyInitialize and AssemblyCleanup
    // fixtures get executed.  It won't actually host any tests.
    [TestClass]
    public class ReleaseTestEnvironment
    {
        [AssemblyInitialize]
        [TestProperty("CoreClrProfile", ".NETCoreApp2.1")]
        [TestProperty("RunFixtureAs:Assembly", "ElevatedUserOrSystem")]
        public static void AssemblyInitialize(TestContext testContext)
        {
#if USING_TAEF
            Log.Comment("TestContext.TestDeploymentDir    = {0}", testContext.TestDeploymentDir);
            Log.Comment("TestContext.TestDir              = {0}", testContext.TestDir);
            Log.Comment("TestContext.TestLogsDir          = {0}", testContext.TestLogsDir);
            Log.Comment("TestContext.TestResultsDirectory = {0}", testContext.TestResultsDirectory);
            Log.Comment("TestContext.DeploymentDirectory  = {0}", testContext.DeploymentDirectory);

            // Enable side-loading
            Log.Comment("Enable side loading apps");
            TestAppInstallHelper.EnableSideloadingApps();

            // Install the test app certificate if we're deploying the test app from the NuGet package.
            // If this is the test app from the OS repo, then it'll have been signed with a test cert
            // that doesn't need installation.
            Log.Comment("Installing the certificate for the test app");
            TestAppInstallHelper.InstallAppxCert(testContext.TestDeploymentDir, TestApplicationInfo.MUXControlsTestApp.TestAppPackageName);
            TestAppInstallHelper.InstallAppxCert(testContext.TestDeploymentDir, TestApplicationInfo.NugetPackageTestAppCX.TestAppPackageName);
#endif
        }
    }
}
