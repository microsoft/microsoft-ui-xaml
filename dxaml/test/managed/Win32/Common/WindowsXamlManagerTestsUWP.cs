// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.
using System;
using System.Diagnostics;
using System.Collections.Generic;
using System.Text;
using System.Threading;
using System.Threading.Tasks;
using System.Linq;

using WEX.Logging.Interop;
using WEX.TestExecution;
using WEX.TestExecution.Markup;

using Private.Infrastructure;
using Microsoft.UI.Xaml.Tests.Common;

using Microsoft.UI.Xaml.Hosting;

namespace Microsoft.UI.Xaml.Tests.Foundation.Win32.Common
{
    [TestClass]
    public class WindowsXamlManagerTestsUWP : XamlTestsBase
    {
        [ClassInitialize]
        [TestProperty("BinaryUnderTest", "Microsoft.UI.Xaml.dll")]
        [TestProperty("Classification", "Integration")]
        [TestProperty("IsolationLevel", "Method")]
        [TestProperty("RunAs", "UAP")]
        [TestProperty("UAP:Praid", "XamlManagedTAEFTests")]
        [TestProperty("Hosting:Mode", "UAP")]
        [TestProperty("TestPass:IncludeOnlyOn", "Desktop")]
        public static void Setup(TestContext context)
        {
            AssemblySetup.CommonTestClassSetup();
        }

        [TestCleanup]
        public override void TestCleanup()
        {
            // Don't call base
        }

        [ClassCleanup]
        public void ClassCleanup()
        {
            base.CommonClassCleanup();
        }

        [TestMethod]
        public void CannotCreateWindowsXamlManagerInUIThread()
        {
            UIExecutor.Execute(() =>
            {
                Exception caughtException = VerifyExtensions.Throws<Exception>(()=>
                    {
                        using(WindowsXamlManager.InitializeForCurrentThread()){}
                    });
                Verify.IsTrue(caughtException.Message.Contains("https://go.microsoft.com/fwlink/?linkid=875495"));
            });
        }

        [TestMethod]
        public void CannotCreateWindowsXamlManager()
        {
            Exception caughtException = VerifyExtensions.Throws<Exception>(()=>
                {
                    using(WindowsXamlManager.InitializeForCurrentThread()){}
                });
            Verify.IsTrue(caughtException.Message.Contains("https://go.microsoft.com/fwlink/?linkid=875495"));
        }

        [TestMethod]
        public void CannotCreateDesktopWindowXamlSourceInUIThread()
        {
            UIExecutor.Execute(() =>
            {
                Exception caughtException = VerifyExtensions.Throws<Exception>(()=>
                    {
                        using(new DesktopWindowXamlSource()){}
                    });
                Verify.IsTrue(caughtException.Message.Contains("https://go.microsoft.com/fwlink/?linkid=875495"));
            });
        }

        [TestMethod]
        public void CannotCreateDesktopWindowXamlSource()
        {
            Exception caughtException = VerifyExtensions.Throws<Exception>(()=>
                {
                    using(new DesktopWindowXamlSource()){}
                });
            Verify.IsTrue(caughtException.Message.Contains("https://go.microsoft.com/fwlink/?linkid=875495"));
        }
    }
}
