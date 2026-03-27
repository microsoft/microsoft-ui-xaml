// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.
using System;
using System.Diagnostics;
using System.IO;
using System.Runtime.InteropServices;
using System.Threading;
using System.Threading.Tasks;
using Windows.ApplicationModel.Core;
using Microsoft.UI.Xaml.Controls;
using Microsoft.UI.Xaml.Markup;

using Private.Infrastructure;
using WEX.TestExecution;
using WEX.TestExecution.Markup;
using WEX.Logging.Interop;

namespace Microsoft.UI.Xaml.Tests.Common
{
    /// <summary>
    /// Represents the base class for all the managed tests.
    /// </summary>
    [TestClass]
    public abstract class XamlTestsBase
    {
        private static TestContext _context;
        public static TestContext Context { get { return _context;  } }

        public static bool IsBVT { get; protected set; }

        static XamlTestsBase()
        {
            // This will make sure the TestServices are initialized.
            // It will wait for the window to be ready and wait for
            // a *native* debugger to attach if the WaitForDebugger
            // parameter is specified.
            CommonTestSetupHelper.CommonTestClassSetup();
        }

        public void CommonClassCleanup()
        {
            CommonTestSetupHelper.CommonTestClassCleanup();
        }

        /// Common cleanup method. Derived tests can override and call base if they need something more specific.
        [TestCleanup]
        public virtual void TestCleanup()
        {
            TestServices.WindowHelper.ResetWindowContentAndWaitForIdle();
            TestServices.WindowHelper.VerifyTestCleanup();
        }

        #region Static methods
        protected static void SetupBase(TestContext context)
        {
            _context = context;

            if (_context.Properties.Contains("WaitForManagedDebugger"))
            {
                WaitForManagedDebugger();
            }
        }

        private static void WaitForManagedDebugger()
        {
            while (!Debugger.IsAttached)
            {
                WEX.Logging.Interop.Log.Comment("Waiting for a managed debugger to attach.");
                Task.Delay(1000).Wait();
            }

            Debugger.Break();
        }
        #endregion

        #region LoadXaml methods
        public static T LoadXaml<T>(string xaml)
        {
            return (T)XamlReader.Load(xaml);
        }

        public static string LoadFile(string filename)
        {
            string path = Path.Combine(Context.TestDeploymentDir, @"resources\managed\");
            path += filename;
            string content = File.ReadAllText(path);
            return content;
        }

        public static T LoadXamlFile<T>(string filename)
        {
            string xaml = LoadFile(filename);
            return LoadXaml<T>(xaml);
        }

        #endregion
    }
}
