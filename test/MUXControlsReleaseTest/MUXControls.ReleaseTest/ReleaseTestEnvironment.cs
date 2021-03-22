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
        [TestProperty("Classification", "Integration")]
        [TestProperty("HelixWorkItemCreation", "CreateWorkItemPerTestClass")]
        public static void AssemblyInitialize(TestContext testContext)
        {
            TestEnvironment.AssemblyInitialize(testContext, TestApplicationInfo.NugetPackageTestAppCX.TestAppPackageName + ".cer");
        }
    }
}
