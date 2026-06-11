// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

using Microsoft.UI.Xaml.Tests.MUXControls.InteractionTests.Common;
using Common;

using System;
using System.Collections.Generic;
using System.Diagnostics;
using System.IO;
using System.Linq;
using System.Reflection;
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
using Windows.UI.Notifications;

namespace Microsoft.UI.Xaml.Tests.MUXControls.InteractionTests.Infra
{
    public class TestApplicationInfo
    {
        public bool InstallFromDirectory { get; private set; }
        public string TestAppPackageFamilyName { get; private set; }
        public string TestAppName { get; private set; }
        public string TestAppMainWindowTitle { get; private set; }
        public bool IsUwpApp { get; private set; }
        public bool IsPackaged { get; private set; }

        // Properties to set if InstallFromDirectory = false
        public string TestAppPackageName { get; private set; }
        public string ProcessName { get; private set; }
        public string InstallerName { get; private set; }
        public string CertSerialNumber { get; private set; }
        public string BaseAppxDir { get; private set; }
        public string UnpackagedExePath { get; private set; }

        // Properties to set if InstallFromDirectory = true
        public string PathToTestAppDirectory { get; private set; }

        public TestApplicationInfo(string testAppName, string testAppMainWindowTitle, string unpackagedExePath)
            : this(testAppPackageName: string.Empty, testAppName: string.Empty, testAppPackageFamilyName: string.Empty, testAppMainWindowTitle, processName: string.Empty, installerName: string.Empty, isUwpApp: false, certSerialNumber: string.Empty, baseAppxDir: string.Empty, isPackaged: false, unpackagedExePath)
        {
        }

        public TestApplicationInfo(string testAppPackageFamilyName, string testAppName, string testAppMainWindowTitle, bool isUwpApp, string pathToTestAppDirectory)
        {
            this.InstallFromDirectory = true;
            this.TestAppPackageFamilyName = testAppPackageFamilyName;
            this.TestAppName = testAppName;
            this.TestAppMainWindowTitle = testAppMainWindowTitle;
            this.IsUwpApp = isUwpApp;
            this.PathToTestAppDirectory = pathToTestAppDirectory;
        }

        public TestApplicationInfo(string testAppPackageName, string testAppName, string testAppPackageFamilyName, string certSerialNumber, string baseAppxDir)
            : this(testAppPackageName, testAppName, testAppPackageFamilyName, testAppPackageName, testAppPackageName, testAppPackageName, certSerialNumber, baseAppxDir)
        {
        }

        public TestApplicationInfo(string testAppPackageName, string testAppName, string testAppPackageFamilyName, string testAppMainWindowTitle, string processName, string installerName, string certSerialNumber, string baseAppxDir)
            : this(testAppPackageName, testAppName, testAppPackageFamilyName, testAppMainWindowTitle, processName, installerName, isUwpApp: true, certSerialNumber, baseAppxDir)
        {
        }


        public TestApplicationInfo(string testAppPackageName, string testAppName, string testAppPackageFamilyName, string testAppMainWindowTitle, string processName, string installerName, bool isUwpApp, string certSerialNumber, string baseAppxDir)
            : this(testAppPackageName, testAppName, testAppPackageFamilyName, testAppMainWindowTitle, processName, installerName, isUwpApp, certSerialNumber, baseAppxDir, isPackaged: true, unpackagedExePath: string.Empty)
        {
        }

        public TestApplicationInfo(string testAppPackageName, string testAppName, string testAppPackageFamilyName, string testAppMainWindowTitle, string processName, string installerName, bool isUwpApp, string certSerialNumber, string baseAppxDir, bool isPackaged, string unpackagedExePath)
        {
            this.InstallFromDirectory = false;
            this.TestAppPackageName = testAppPackageName;
            this.TestAppName = testAppName;
            this.TestAppPackageFamilyName = testAppPackageFamilyName;

            this.TestAppMainWindowTitle = testAppMainWindowTitle;
            this.ProcessName = processName;
            this.InstallerName = installerName;

            this.IsUwpApp = isUwpApp;

            this.CertSerialNumber = certSerialNumber;
            this.BaseAppxDir = baseAppxDir;

            this.IsPackaged = isPackaged;
            this.UnpackagedExePath = unpackagedExePath;
        }

        public const string MUXCertSerialNumber = "fd1d6927f4521242f00b20c9df66ea4f97175ee2";

        public static string MUXBaseAppxDir
        {
            get
            {
                string assemblyDir = Path.GetDirectoryName(Assembly.GetExecutingAssembly().Location);
                string baseDirectory = Directory.GetParent(assemblyDir).Parent.FullName;
                return baseDirectory;
            }
        }

        public static TestApplicationInfo MUXControlsTestApp
        {
            get
            {
                return new TestApplicationInfo(
                    "MUXControlsTestApp_6f07fta6qpts2",
                    "MUXControlsTestApp.Desktop",
                    Path.Combine("MUXControlsTestApp", "MUXControlsTestApp.exe"));
            }
        }

        public static TestApplicationInfo MUXControlsInnerLoopTestApp
        {
            get
            {
                string testAppName = "MUXControlsInnerLoopTestApp_6f07fta6qpts2!taef.executionengine.universal.App";
                return new TestApplicationInfo("MUXControlsTestApp", testAppName, "MUXControlsInnerLoopTestApp_6f07fta6qpts2", "MUXControlsInnerLoopTestApp", "MUXControlsTestApp", "MUXControlsTestApp", MUXCertSerialNumber, MUXBaseAppxDir);
            }
        }

        public static TestApplicationInfo MUXExperimentalTestApp
        {
            get
            {
                return new TestApplicationInfo("MUXExperimentalTestApp", "MUXExperimentalTestApp_6f07fta6qpts2!App", "MUXExperimentalTestApp_6f07fta6qpts2", MUXCertSerialNumber, MUXBaseAppxDir);
            }
        }
    }

    public static class WindowsOSVersion
    {
        public const string RS4 = "17134";
        public const string RS5 = "17763";
        public const string _19H1 = "18362";
        public const string _20H2 = "19042";
        public const string _21H2 = "22000";
        public const string _22H2 = "22621";
    }

    // This is marked as a test class to make sure our AssemblyInitialize and AssemblyCleanup
    // fixtures get executed.  It won't actually host any tests.
    [TestClass]
    public class TestEnvironment
    {
        public static TestContext TestContext { get; private set; }

        public static bool IsLogVerbose { get; private set; }

        public static bool IsLogSuperVerbose { get; private set; }

        public static Application Application { get; private set; }

        public static bool ShouldRestartApplication { get; set; }
        public static string CertFileName { get; set; }

        public static void LogVerbose(string format, params object[] args)
        {
            if (IsLogVerbose)
            {
                Log.Comment(format, args);
            }
        }

        public static void LogSuperVerbose(string format, params object[] args)
        {
            if (IsLogSuperVerbose)
            {
                Log.Comment(format, args);
            }
        }

        public static void AssemblyInitialize(TestContext testContext, string certFileName)
        {
            CertFileName = certFileName;

            WinRT.ComWrappersSupport.InitializeComWrappers();

            if (!PlatformConfiguration.IsDevice(DeviceType.OneCore))
            {
                // We need to make the process DPI aware so it properly handles scale factors other than 100%.
                if (NativeMethods.SetProcessDpiAwarenessContext(NativeMethods.DPI_AWARENESS_CONTEXT_PER_MONITOR_AWARE_V2) < 0)
                {
                    throw new Exception("Failed to set process DPI awareness context!  Error = " + Marshal.GetLastWin32Error());
                }
            }

            Log.Comment("TestContext.TestDeploymentDir    = {0}", testContext.TestDeploymentDir);
            Log.Comment("TestContext.TestDir              = {0}", testContext.TestDir);
            Log.Comment("TestContext.TestLogsDir          = {0}", testContext.TestLogsDir);
            Log.Comment("TestContext.TestResultsDirectory = {0}", testContext.TestResultsDirectory);
            Log.Comment("TestContext.DeploymentDirectory  = {0}", testContext.DeploymentDirectory);
        }

        public static void AssemblyCleanup()
        {
#if INNERLOOP_BUILD
            AssemblyCleanupWorker(TestApplicationInfo.MUXControlsInnerLoopTestApp);
#else
            AssemblyCleanupWorker(TestApplicationInfo.MUXControlsTestApp);
#endif
        }

        public static void AssemblyCleanupWorker(TestApplicationInfo testAppInfo)
        {
            // This executed in a different context from the tests, so it doesn't have a reference
            // to Application object created by them, so just create a local one (configured to not launch the
            // app) so that we can close it.
            var app = CreateApplication(testAppInfo);
            app.Close();

            Log.Comment("Killing processes we might have inadvertently started...");
            var killList = new string[] { "microsoft.photos", "HxAccounts", "HxCalendarAppImm", "HxOutlook", "HxTsr" };
            foreach (var process in Process.GetProcesses().Where(p => killList.Contains(p.ProcessName.ToLower())))
            {
                Log.Comment("Killing process '{0}' ({1}).", process.ProcessName, process.Id);
                process.Kill();
                process.WaitForExit();
            }
        }

        private static Application CreateApplication(TestApplicationInfo info)
        {
            if (info.InstallFromDirectory)
            {
                return new Application(
                    info.TestAppPackageFamilyName,
                    info.TestAppName,
                    info.TestAppMainWindowTitle,
                    info.IsUwpApp,
                    info.PathToTestAppDirectory);
            }
            else
            {
                return new Application(
                    info.TestAppPackageName,
                    info.TestAppPackageFamilyName,
                    info.TestAppName,
                    info.TestAppMainWindowTitle,
                    info.ProcessName,
                    info.InstallerName,
                    info.CertSerialNumber,
                    info.BaseAppxDir,
                    info.IsUwpApp,
                    info.UnpackagedExePath,
                    info.IsPackaged);
            }
        }

        public static void Initialize(TestContext testContext)
        {
#if INNERLOOP_BUILD
            Initialize(testContext, TestApplicationInfo.MUXControlsInnerLoopTestApp);
#else
            Initialize(testContext, TestApplicationInfo.MUXControlsTestApp);
#endif
        }

        public static void Cleanup(TestApplicationInfo testAppInfo)
        {
            AssemblyCleanupWorker(testAppInfo);
        }

        // Tests classes call this from their ClassInitialize methods to init our Application instance
        // and launching the application if necessary.
        public static void Initialize(TestContext testContext, TestApplicationInfo testAppInfo)
        {
            if (testAppInfo.IsPackaged)
            {
                Log.Comment("Enabling side loading apps...");
                TestAppInstallHelper.EnableSideloadingApps();

                if (!string.IsNullOrEmpty(CertFileName))
                {
                    Log.Comment("Installing the certificate for the test app...");
                    TestAppInstallHelper.InstallAppxCert(testContext.TestDeploymentDir, CertFileName);
                }
            }
            
            TestContext = testContext;

            IsLogVerbose = TestContext.Properties.Contains("LogVerbose");
            IsLogSuperVerbose = TestContext.Properties.Contains("LogSuperVerbose");

            if (TestContext.Properties.Contains("WaitForDebugger"))
            {
                var processId = Process.GetCurrentProcess().Id;
                while (!Debugger.IsAttached)
                {
                    Log.Comment(string.Format("Waiting for a debugger to attach (processId = {0})...", processId));
                    Thread.Sleep(1000);
                }

                Debugger.Break();
            }

            Application = CreateApplication(testAppInfo);

            bool shouldMaximize = true;

            if (TestContext.Properties.Contains("MaximizeWindowAtStart"))
            {
                shouldMaximize = (bool)TestContext.Properties["MaximizeWindowAtStart"];
            }

            // Initialize relies on TestEnvironment.Application to be set, so we'll call this method
            // outside of the constructor.
            if (!Application.Initialize(true, TestContext.TestDeploymentDir, shouldMaximize))
            {
                Log.Error("Application failed to initialize.");
            }
        }

        public static void LogDumpTree(UIObject root = null)
        {
            Log.Comment("============ Dump Tree ============");
            LogDumpTreeWorker("", root ?? Application.CoreWindow);
            Log.Comment("===================================");
        }

        public static void WaitUntilElementLoadedById(string automationId)
        {
            WaitUntilElementLoaded(automationId, ElementKeyType.AutomationId);
        }

        public static void WaitUntilElementLoadedByName(string name)
        {
            WaitUntilElementLoaded(name, ElementKeyType.Name);
        }

        public static void WaitUntilElementLoadedByNameOrId(string nameOrId)
        {
            WaitUntilElementLoaded(nameOrId, ElementKeyType.NameOrAutomationId);
        }

        public static void WaitUntilElementLoadedByClassName(string className)
        {
            WaitUntilElementLoaded(className, ElementKeyType.ClassName);
        }

        private enum ElementKeyType
        {
            AutomationId,
            Name,
            NameOrAutomationId,
            ClassName,
        }

        private static void WaitUntilElementLoaded(string key, ElementKeyType keyType)
        {
            // Wait until the element with the specified element has been loaded before continuing.
            // ElementAddedWaiter doesn't seem to work for this purpose, so this is the next best thing.
            int triesLeft = 50;
            UIObject element = null;

            using (TimeWaiter waiter = new TimeWaiter(TimeSpan.FromMilliseconds(100)))
            {
                while (element == null && triesLeft-- > 0)
                {
                    switch (keyType)
                    {
                        case ElementKeyType.AutomationId:
                            element = TryFindElement.ById(key);
                            break;
                        case ElementKeyType.Name:
                            element = TryFindElement.ByName(key);
                            break;
                        case ElementKeyType.NameOrAutomationId:
                            element = TryFindElement.ByNameOrId(key);
                            break;
                        case ElementKeyType.ClassName:
                            element = TryFindElement.ByClassName(key);
                            break;
                        default:
                            throw new InvalidOperationException(string.Format("Invalid element key type: {0}", keyType.ToString()));
                    }

                    waiter.Wait();
                    waiter.Reset();
                }
            }

            if (triesLeft == 0)
            {
                throw new WaiterTimedOutException(string.Format("Could not find '{0}'!", key));
            }
        }

        private static void LogDumpTreeWorker(string indent, UIObject current)
        {
            Log.Comment(indent + current.GetDescription());
            indent += "  ";
            foreach (var uiObject in current.Children)
            {
                LogDumpTreeWorker(indent, uiObject);
            }
        }

        public static void VerifyAreEqualWithRetry(int maxRetries, Func<object> expectedFunc, Func<object> actualFunc, Action retryAction = null)
        {
            if (retryAction == null)
            {
                retryAction = () =>
                {
                    Task.Delay(TimeSpan.FromMilliseconds(50)).Wait();
                    ElementCache.Clear(); /* Test is flaky sometimes -- perhaps element cache is stale? Clear it and try again. */
                };
            }

            for (int retry = 0; retry <= maxRetries; retry++)
            {
                object expected = expectedFunc();
                object actual = actualFunc();
                if (Object.Equals(expected, actual) || retry == maxRetries)
                {
                    Log.Comment("Actual retry times: " + retry);
                    Verify.AreEqual(expected, actual);
                    return;
                }
                else
                {
                    retryAction();
                }
            }
        }
        
        private static class NativeMethods
        {
            public static UIntPtr DPI_AWARENESS_CONTEXT_PER_MONITOR_AWARE = new UIntPtr(0xfffffffd);
            public static UIntPtr DPI_AWARENESS_CONTEXT_PER_MONITOR_AWARE_V2 = new UIntPtr(0xfffffffc);

            [DllImport("user32.dll")]
            public static extern int SetProcessDpiAwarenessContext([In] UIntPtr value);
        }
    }
}
