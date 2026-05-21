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

using Microsoft.UI.Xaml.Tests.Common;

using Microsoft.UI.Xaml.Hosting;
using Microsoft.UI.Xaml.Controls;

namespace Microsoft.UI.Xaml.Tests.Foundation.Win32.Common
{
    [TestClass]
    public class WindowsXamlManagerTestsNonCentennial : XamlTestsBase
    {
        [ClassInitialize]
        [TestProperty("BinaryUnderTest", "Microsoft.UI.Xaml.dll")]
        [TestProperty("Classification", "Integration")]
        [TestProperty("IsolationLevel", "Method")]
        [TestProperty("TestPass:IncludeOnlyOn", "Desktop")]
        [TestProperty("Ignore", "True")] // With new appcompat non-centennial applications need app.manifest to be built with taef.exe
        public static void Setup(TestContext context)
        {
        }

        [ClassCleanup]
        public void ClassCleanup()
        {
            base.CommonClassCleanup();
        }

        [TestCleanup]
        public override void TestCleanup()
        {
            // Don't call base
        }

        [TestMethod]
        public void CanNestWindowsXamlManager()
        {
            using(WindowsXamlManager.InitializeForCurrentThread())
            {
                Log.Comment("Manager 1");
                var bt1 = new Button() { Height = 10, Width = 10, };
                Verify.IsNotNull(bt1);
                using(WindowsXamlManager.InitializeForCurrentThread())
                {
                    Log.Comment("Nest manager 2");
                    var bt2 = new Button() { Height = 10, Width = 10, };
                    Verify.IsNotNull(bt2);
                }
                var bt3 = new Button() { Height = 10, Width = 10, };
                Verify.IsNotNull(bt3);
            }
            Log.Comment("Nesting completed");
        }

        [TestMethod]
        public void CanCreateMultipleWindowsXamlManager()
        {
            using(WindowsXamlManager.InitializeForCurrentThread())
            {
                Log.Comment("Manager 1");
                var bt1 = new Button() { Height = 10, Width = 10, };
                Verify.IsNotNull(bt1);
            }
            using(WindowsXamlManager.InitializeForCurrentThread())
            {
                Log.Comment("Manager 2");
                var bt2 = new Button() { Height = 10, Width = 10, };
                Verify.IsNotNull(bt2);
            }
            Log.Comment("Creation completed");
        }

        [TestMethod]
        public void CanCreateMultipleDesktopWindowXamlSource()
        {
            using(new DesktopWindowXamlSource())
            {
                var bt1 = new Button() { Height = 10, Width = 10, };
                Verify.IsNotNull(bt1);
                Log.Comment("XamlSource 1");
            }
            using(new DesktopWindowXamlSource())
            {
                var bt2 = new Button() { Height = 10, Width = 10, };
                Verify.IsNotNull(bt2);
                Log.Comment("XamlSource 2");
            }
        }

        [TestMethod]
        public void CanNestDesktopWindowXamlSource()
        {
            using(new DesktopWindowXamlSource())
            {
                Log.Comment("XamlSource 1");
                var bt1 = new Button() { Height = 10, Width = 10, };
                Verify.IsNotNull(bt1);
                using(new DesktopWindowXamlSource())
                {
                    Log.Comment("Nest XamlSource 2");
                    var bt2 = new Button() { Height = 10, Width = 10, };
                    Verify.IsNotNull(bt2);
                }
                var bt3 = new Button() { Height = 10, Width = 10, };
                Verify.IsNotNull(bt3);
            }
            Log.Comment("Nesting completed");
        }

        [TestMethod]
        [TestProperty("Description", "Validates that DesktopWindowXamlSource is blocked on RS5.")]
        public void ValidateDesktopWindowXamlSourceIsBlockedOnRS5()
        {
            Exception caughtException = VerifyExtensions.Throws<Exception>(() =>
            {
                Xaml.Hosting.DesktopWindowXamlSource source = new Xaml.Hosting.DesktopWindowXamlSource();
            });
            Verify.IsTrue(caughtException.Message.Contains("WindowsXamlManager and DesktopWindowXamlSource are supported for apps targeting Windows version 10.0.18226.0 and later.  Please check either the application manifest or package manifest and ensure the MaxTestedVersion property is updated."));
        }

        [TestMethod]
        [TestProperty("Description", "Validates that XamlManager is blocked on RS5.")]
        public void ValidateXamlManagerIsBlockedOnRS5()
        {
            Exception caughtException = VerifyExtensions.Throws<Exception>(() =>
            {
                WindowsXamlManager.InitializeForCurrentThread();
            });
            Verify.IsTrue(caughtException.Message.Contains("WindowsXamlManager and DesktopWindowXamlSource are supported for apps targeting Windows version 10.0.18226.0 and later.  Please check either the application manifest or package manifest and ensure the MaxTestedVersion property is updated."));
        }
    }
}
