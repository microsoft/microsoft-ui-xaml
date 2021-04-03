// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

using Windows.UI.Xaml.Tests.MUXControls.InteractionTests.Common;
using Common;

using System;
using System.Collections.Generic;
using System.Diagnostics;
using System.Linq;
using System.Runtime.InteropServices;
using System.Threading;
using System.Threading.Tasks;
using System.Text;

#if USING_TAEF
using WEX.TestExecution;
using WEX.TestExecution.Markup;
using WEX.Logging.Interop;
#else
using Microsoft.VisualStudio.TestTools.UnitTesting;
using Microsoft.VisualStudio.TestTools.UnitTesting.Logging;
#endif

using Microsoft.Windows.Apps.Test.Automation;
using Microsoft.Windows.Apps.Test.Foundation;
using Microsoft.Windows.Apps.Test.Foundation.Controls;
using Microsoft.Windows.Apps.Test.Foundation.Patterns;
using Microsoft.Windows.Apps.Test.Foundation.Waiters;
using Windows.UI.Notifications;
using System.IO;
using System.Reflection;

namespace Windows.UI.Xaml.Tests.MUXControls.InteractionTests.Infra
{
    public class TestApplicationInfo
    {
        public string TestAppPackageName { get; private set; }
        public string TestAppName { get; private set; }
        public string TestAppPackageFamilyName { get; private set; }

        public string TestAppMainWindowTitle { get; private set; }
        public string ProcessName { get; private set; }
        public string InstallerName { get; private set; }
        public bool IsUwpApp { get; private set; }
        public bool IsPackaged { get; private set; }
        public string CertSerialNumber { get; private set; }

        public string BaseAppxDir { get; private set; }
        public string UnpackagedExePath { get; private set; }

        public TestApplicationInfo(string testAppMainWindowTitle, string unpackagedExePath)
            : this(testAppPackageName: string.Empty, testAppName: string.Empty, testAppPackageFamilyName: string.Empty, testAppMainWindowTitle, processName: string.Empty, installerName: string.Empty, certSerialNumber: string.Empty, baseAppxDir: string.Empty)
        {
            IsPackaged = false;
            UnpackagedExePath = unpackagedExePath;
        }

        public TestApplicationInfo(string testAppPackageName, string testAppName, string testAppPackageFamilyName, string certSerialNumber, string baseAppxDir)
            : this(testAppPackageName, testAppName, testAppPackageFamilyName, testAppPackageName, testAppPackageName, testAppPackageName, certSerialNumber, baseAppxDir)
        {
            IsPackaged = true;
            UnpackagedExePath = string.Empty;
        }

        public TestApplicationInfo(string testAppPackageName, string testAppName, string testAppPackageFamilyName, string testAppMainWindowTitle, string processName, string installerName, string certSerialNumber, string baseAppxDir)
            : this(testAppPackageName, testAppName, testAppPackageFamilyName, testAppMainWindowTitle, processName, installerName, isUwpApp: true, certSerialNumber, baseAppxDir)
        {
            IsPackaged = true;
            UnpackagedExePath = string.Empty;
        }

        public TestApplicationInfo(string testAppPackageName, string testAppName, string testAppPackageFamilyName, string testAppMainWindowTitle, string processName, string installerName, bool isUwpApp, string certSerialNumber, string baseAppxDir)
        {
            this.TestAppPackageName = testAppPackageName;
            this.TestAppName = testAppName;
            this.TestAppPackageFamilyName = testAppPackageFamilyName;

            this.TestAppMainWindowTitle = testAppMainWindowTitle;
            this.ProcessName = processName;
            this.InstallerName = installerName;

            this.IsUwpApp = isUwpApp;

            this.CertSerialNumber = certSerialNumber;
            this.BaseAppxDir = baseAppxDir;
        }

        private const string MUXCertSerialNumber = "fd1d6927f4521242f00b20c9df66ea4f97175ee2";

        private static string MUXBaseAppxDir
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
#if USING_TAEF
                string testAppName = "MUXControlsTestApp_8wekyb3d8bbwe!taef.executionengine.universal.App";
#else
                string testAppName = "MUXControlsTestApp_8wekyb3d8bbwe!App";
#endif
                return new TestApplicationInfo("MUXControlsTestApp", testAppName, "MUXControlsTestApp_8wekyb3d8bbwe", MUXCertSerialNumber, MUXBaseAppxDir);
            }
        }

        public static TestApplicationInfo MUXControlsInnerLoopTestApp
        {
            get
            {
#if USING_TAEF
                string testAppName = "MUXControlsInnerLoopTestApp_8wekyb3d8bbwe!taef.executionengine.universal.App";
#else
                string testAppName = "MUXControlsInnerLoopTestApp_8wekyb3d8bbwe!App";
#endif
                return new TestApplicationInfo("MUXControlsTestApp", testAppName, "MUXControlsInnerLoopTestApp_8wekyb3d8bbwe", "MUXControlsInnerLoopTestApp", "MUXControlsTestApp", "MUXControlsTestApp", MUXCertSerialNumber, MUXBaseAppxDir);
            }
        }

        public static TestApplicationInfo NugetPackageTestApp
        {
            get
            {
                return new TestApplicationInfo("NugetPackageTestApp", "NugetPackageTestApp_8wekyb3d8bbwe!App", "NugetPackageTestApp_8wekyb3d8bbwe", MUXCertSerialNumber, MUXBaseAppxDir);
            }
        }

        public static TestApplicationInfo AppThatUsesMUXIndirectly
        {
            get
            {
                return new TestApplicationInfo("AppThatUsesMUXIndirectly", "AppThatUsesMUXIndirectly_8wekyb3d8bbwe!App", "AppThatUsesMUXIndirectly_8wekyb3d8bbwe", MUXCertSerialNumber, MUXBaseAppxDir);
            }
        }

        public static TestApplicationInfo NugetPackageTestAppCX
        {
            get
            {
                return new TestApplicationInfo("NugetPackageTestAppCX", "NugetPackageTestAppCX_8wekyb3d8bbwe!App", "NugetPackageTestAppCX_8wekyb3d8bbwe", MUXCertSerialNumber, MUXBaseAppxDir);
            }
        }

        public static TestApplicationInfo XamlIslandsTestApp
        {
            get
            {
                return new TestApplicationInfo("WpfApp", "WpfApp_8wekyb3d8bbwe!App", "WpfApp_8wekyb3d8bbwe", "WpfApp", "WpfApp.exe", "WpfApp", isUwpApp: false, MUXCertSerialNumber, MUXBaseAppxDir);
            }
        }

        public static TestApplicationInfo XamlIslandsTestAppUnpackaged
        {
            get
            {
                return new TestApplicationInfo("WpfApp", @"WpfApp\WpfApp.exe");
            }
        }
    }

    public class TestEnvironment
    {
        public static TestContext TestContext { get; private set; }

        public static bool IsLogVerbose { get; private set; }

        public static bool IsLogSuperVerbose { get; private set; }

        public static Application Application { get; private set; }

        public static bool ShouldRestartApplication { get; set; }

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
            if (!PlatformConfiguration.IsDevice(DeviceType.OneCore))
            {
                // We need to make the process DPI aware so it properly handles scale factors other than 100%.
                // DPI_AWARENESS_CONTEXT_PER_MONITOR_AWARE_V2 only existed RS2 and up, so we'll fall back to
                // DPI_AWARENESS_CONTEXT_PER_MONITOR_AWARE below RS2.
                if (SetProcessDpiAwarenessContext(
                    PlatformConfiguration.IsOsVersionGreaterThanOrEqual(OSVersion.Redstone2) ?
                        DPI_AWARENESS_CONTEXT_PER_MONITOR_AWARE_V2 :
                        DPI_AWARENESS_CONTEXT_PER_MONITOR_AWARE) < 0)
                {
                    throw new Exception("Failed to set process DPI awareness context!  Error = " + Marshal.GetLastWin32Error());
                }
            }

#if USING_TAEF
            Log.Comment("TestContext.TestDeploymentDir    = {0}", testContext.TestDeploymentDir);
            Log.Comment("TestContext.TestDir              = {0}", testContext.TestDir);
            Log.Comment("TestContext.TestLogsDir          = {0}", testContext.TestLogsDir);
            Log.Comment("TestContext.TestResultsDirectory = {0}", testContext.TestResultsDirectory);
            Log.Comment("TestContext.DeploymentDirectory  = {0}", testContext.DeploymentDirectory);

            // Enable side-loading
            Log.Comment("Enable side loading apps");
            TestAppInstallHelper.EnableSideloadingApps();

            // Install the test app certificate if we're deploying the MUXControlsTestApp from the NuGet package.
            // If this is the MUXControlsTestApp from the OS repo, then it'll have been signed with a test cert
            // that doesn't need installation.
            Log.Comment("Installing the certificate for the test app");
            TestAppInstallHelper.InstallAppxCert(testContext.TestDeploymentDir, certFileName);
#endif
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

        public static void Initialize(TestContext testContext)
        {
#if INNERLOOP_BUILD
            Initialize(testContext, TestApplicationInfo.MUXControlsInnerLoopTestApp);
#else
            Initialize(testContext, TestApplicationInfo.MUXControlsTestApp);
#endif
        }

        // Tests classes call this from their ClassInitialize methods to init our Application instance
        // and launching the application if necessary.
        public static void Initialize(TestContext testContext, TestApplicationInfo testAppInfo)
        {
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

            // Initialize relies on TestEnvironment.Application to be set, so we'll call this method
            // outside of the constructor.
#if USING_TAEF
            Application.Initialize(true, TestContext.TestDeploymentDir);
#else
            Application.Initialize(true);
#endif
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

        private static UIntPtr DPI_AWARENESS_CONTEXT_PER_MONITOR_AWARE = new UIntPtr(0xfffffffd);
        private static UIntPtr DPI_AWARENESS_CONTEXT_PER_MONITOR_AWARE_V2 = new UIntPtr(0xfffffffc);

        [DllImport("user32.dll")]
        private static extern int SetProcessDpiAwarenessContext([In] UIntPtr value);
    }
}
