// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

using Microsoft.UI.Xaml.Tests.MUXControls.InteractionTests.Common;
using Common;

using System;
using System.Collections.Generic;
using System.Diagnostics;
using System.Linq;
using System.Runtime.InteropServices;
using System.Threading;
using System.Threading.Tasks;
using System.Text;

using WEX.TestExecution;
using WEX.TestExecution.Markup;
using WEX.Logging.Interop;

using Microsoft.Windows.Apps.Test.Automation;
using Microsoft.Windows.Apps.Test.Foundation;
using Microsoft.Windows.Apps.Test.Foundation.Controls;
using Microsoft.Windows.Apps.Test.Foundation.Patterns;
using Microsoft.Windows.Apps.Test.Foundation.Waiters;

namespace Microsoft.UI.Xaml.Tests.MUXControls.InteractionTests.Infra
{
    
    // This is marked as a test class to make sure our AssemblyInitialize and AssemblyCleanup
    // fixtures get executed.  It won't actually host any tests.
    [TestClass]
    public class TestAssembly
    {
        [AssemblyInitialize]
        [TestProperty("CoreClrProfile", "localDotNet")]
        [TestProperty("Hosting:Mode", "UAP")]
        [TestProperty("HelixWorkItemCreation", "CreateWorkItemPerTestClass")]
        [TestProperty("TestPass:MinOSVer", WindowsOSVersion.RS4)]
        [TestProperty("IsolationLevel", "Class")]
        public static void AssemblyInitialize(TestContext testContext)
        {
            TestEnvironment.AssemblyInitialize(testContext, "MUXControlsTestApp.cer");
        }

        [AssemblyCleanup]
        public static void AssemblyCleanup()
        {
            TestEnvironment.AssemblyCleanup();
        }
    }
}
