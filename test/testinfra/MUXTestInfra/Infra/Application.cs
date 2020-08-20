// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

using System;
using System.Diagnostics;
using System.Threading;
using System.Threading.Tasks;
using Windows.UI.Xaml.Tests.MUXControls.InteractionTests.Common;
using Windows.Management.Deployment;
using Common;
using System.Linq;
using System.Collections;
using System.Collections.Generic;
using System.IO;
using System.Reflection;

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
using System.Runtime.InteropServices;

namespace Windows.UI.Xaml.Tests.MUXControls.InteractionTests.Infra
{
    using Window = Microsoft.Windows.Apps.Test.Foundation.Controls.Window;

    public class Application
    {
        private readonly string _packageName;
        private readonly string _packageFamilyName;
        private readonly string _appName;

        private readonly string _appWindowTitle;
        private readonly string _appProcessName;
        private readonly string _appInstallerName;

        private readonly bool _isUWPApp;

        private readonly UICondition _windowCondition = null;
        private readonly UICondition _appFrameWindowCondition = null;

        public Application(string packageName, string packageFamilyName, string appName, string testAppMainWindowTitle, string testAppProcessName, string testAppInstallerName, bool isUWPApp = true)
        {
            _packageName = packageName;
            _packageFamilyName = packageFamilyName;
            _appName = appName;
            _isUWPApp = isUWPApp;

            _appWindowTitle = testAppMainWindowTitle;
            _appProcessName = testAppProcessName;
            _appInstallerName = testAppInstallerName;

            if (_isUWPApp)
            {
                _windowCondition = UICondition.Create("@ClassName='Windows.UI.Core.CoreWindow' AND @Name={0}", _appWindowTitle);
                _appFrameWindowCondition = UICondition.Create("@ClassName='ApplicationFrameWindow' AND @Name={0}", _appWindowTitle);
            }
            else
            {
                _windowCondition = UICondition.Create("@ClassName='Window' AND @Name={0}", _appWindowTitle);
                _appFrameWindowCondition = UICondition.Create("@ClassName='Window' AND @Name={0}", _appWindowTitle);
            }
        }

        #region Properties
        public UIObject CoreWindow { get; private set; }
        public UIObject ApplicationFrameWindow { get; private set; }
        public Process Process { get; private set; }

        public IntPtr Hwnd
        {
            get
            {
                return CoreWindow.NativeWindowHandle;
            }
        }
        #endregion

        #region Methods
        internal void Initialize(bool doLaunch = false, string deploymentDir = null)
        {
            var topWindowCondition = _windowCondition.OrWith(_appFrameWindowCondition);

            UIObject topWindowObj = null;
            bool didFindWindow = UIObject.Root.Children.TryFind(topWindowCondition, out topWindowObj);

            // Only try to launch the app if we couldn't find the window.
            if (doLaunch && !didFindWindow)
            {
                CoreWindow = Launch(deploymentDir);
                if (CoreWindow.Parent.Matches(_appFrameWindowCondition))
                {
                    ApplicationFrameWindow = CoreWindow.Parent;
                }
            }
            else if (didFindWindow)
            {
                if (topWindowObj.Matches(_windowCondition))
                {
                    // If the top level window is CoreWindow, then there is no AppFrame window:
                    CoreWindow = topWindowObj;
                    ApplicationFrameWindow = null;
                }
                else
                {
                    // Maxmize window to ensure we can find UIA elements
                    var appFrameWindow = new Window(CoreWindow);
                    if (appFrameWindow.CanMaximize)
                    {
                        appFrameWindow.SetWindowVisualState(WindowVisualState.Maximized);
                    }

                    Verify.IsTrue(topWindowObj.Matches(_appFrameWindowCondition));
                    ApplicationFrameWindow = topWindowObj;

                    Log.Comment("Looking for CoreWindow...");
                    for (int retries = 0; retries < 5; ++retries)
                    {
                        if (topWindowObj.Children.TryFind(_windowCondition, out var coreWindowObject))
                        {
                            CoreWindow = coreWindowObject;
                            Log.Comment("Found CoreWindow.");
                            break;
                        }

                        Log.Comment("CoreWindow not found. Sleep for 500 ms and retry");
                        Thread.Sleep(500);
                    }
                }
            }

            if (CoreWindow == null)
            {
                // We expect to have a window by this point.
                LogDumpTree();
                throw new UIObjectNotFoundException("Could not find application window.");
            }

            // If this is running on desktop (it has an app frame window) then try to
            // maximize the window.

            if (ApplicationFrameWindow != null)
            {
                var appFrameWindow = new Window(ApplicationFrameWindow);
                if (appFrameWindow.CanMaximize)
                {
                    appFrameWindow.SetWindowVisualState(WindowVisualState.Maximized);
                }
            }

            Process = Process.GetProcessById(CoreWindow.ProcessId);

            Wait.InitializeWaitHelper();

#if USING_TAEF
            if (TestEnvironment.TestContext.Properties.Contains("WaitForAppDebugger"))
#else
            if (TestEnvironment.TestContext.Properties.ContainsKey("WaitForAppDebugger"))
#endif
            {
                Wait.ForAppDebugger();
            }

            TestCleanupHelper.TestSetupHelperPendingDisposals = 0;
        }

        private UIObject Launch(string deploymentDir)
        {
            UIObject coreWindow = null;

            // When running from MUXControls repo we want to install the app.
            // When running in TestMD we also want to install the app.            
#if USING_TAEF
            TestAppInstallHelper.InstallTestAppIfNeeded(deploymentDir, _packageName, _packageFamilyName, _appInstallerName);
#else
            BuildAndInstallTestAppIfNeeded();
#endif


            Log.Comment("Launching app {0}", _appName);

            coreWindow = LaunchApp();

            Verify.IsNotNull(coreWindow, "coreWindow");

            Log.Comment("Waiting for the close-app invoker to be found to signal that the app has launched successfully...");

            for (int retries = 0; retries < 5; ++retries)
            {
                UIObject obj;
                coreWindow.Descendants.TryFind(UICondition.Create("@AutomationId='__CloseAppInvoker'"), out obj);
                if (obj != null)
                {
                    Log.Comment("Invoker found!");
                    break;
                }

                Log.Comment("Invoker not found. Sleeping for 500 ms before trying again...");
                Thread.Sleep(500);
            }

            var unhandledExceptionReportingTextBox = new Edit(coreWindow.Descendants.Find(UICondition.Create("@AutomationId='__UnhandledExceptionReportingTextBox'")));
            var valueChangedSource = new PropertyChangedEventSource(unhandledExceptionReportingTextBox, Scope.Element, UIProperty.Get("Value.Value"));
            valueChangedSource.Start(new TestAppCrashDetector());

            Log.Comment("15056441 tracing, device family:" + Windows.System.Profile.AnalyticsInfo.VersionInfo.DeviceFamily);

            return coreWindow;
        }

        private UIObject LaunchApp()
        {
            UIObject coreWindow = null;

            // Launch sometimes times out but the app is just slow to launch and Launch has what appears to be
            // a 5-second timeout built in. 5 seconds isn't always enough in VM scenarios. If we try again, 
            // Launch will see that the app is already running and move on.
            const int MaxLaunchRetries = 5;
            for (int retries = 1; retries <= MaxLaunchRetries; ++retries)
            {
                try
                {
                    Log.Comment("Attempting launch, try #{0}...", retries);
                    coreWindow = _isUWPApp ? LaunchUWPApp(_packageName) : LaunchNonUWPApp(_packageName);
                    Log.Comment("Launch successful!");
                    break;
                }
                catch (Exception ex)
                {
                    Log.Comment("Failed to launch app. Exception: " + ex.ToString());
                    
                    if (retries < MaxLaunchRetries)
                    {
                        Log.Comment("UAPApp.Launch might not have waited long enough, trying again {0}", retries);
                        Thread.Sleep(TimeSpan.FromSeconds(10)); // Give a healthy wait time.
                    }
                    else
                    {
                        Log.Comment("Dumping UIA tree...");
                        LogDumpTree();
                        Log.Error("Could not launch app {0} with top-level window condition '{1}'!", _appName, CreateTopLevelWindowCondition().ToString());
                        throw;
                    }
                }
            }

            return coreWindow;
        }

        private UIObject LaunchUWPApp(string packageName)
        {
            Debug.Assert(_isUWPApp);
            var nameCondition = UICondition.CreateFromName(_appWindowTitle);
            var topLevelWindowCondition = CreateTopLevelWindowCondition().AndWith(nameCondition);
            return UAPApp.Launch(_appName, topLevelWindowCondition);
        }

        private UIObject LaunchNonUWPApp(string packageName)
        {
            Debug.Assert(!_isUWPApp);
            var nameCondition = UICondition.CreateFromName(packageName);
            var topLevelWindowCondition = UICondition.CreateFromClassName("Window").AndWith(nameCondition);
            UIObject coreWindow = null;

            try
            {
                coreWindow = UAPApp.Launch(_appName, topLevelWindowCondition);
            }
            catch
            {
                // UAP.Launch launches the app, but fails trying to find a CoreWindow in the launched app. If the 
                // App is not UWP (as in the case of WPF app hosting a XAML Island), We can fallback to looking for 
                // just a window and use it as the root for future queries.
                // not a uwp app, see if we can find any window with the condition that matches.
                coreWindow = FindCore.ByNameAndClassName(root: UIObject.Root, name: packageName, className: "Window", shouldWait: true);
            }

            return coreWindow;
        }

        public void Close()
        {
            int appWindowsProccessId = GetProcessIdFromAppWindow();

            if (!CloseAppWindowWithCloseButton())
            {
                Log.Comment("Failed to close application window. We will fall back to terminating the app process.");
            }

            EnsureApplicationProcessHasExited(appWindowsProccessId);

            if (Process != null)
            {
                Process.Dispose();
                Process = null;
            }
        }

        private void EnsureApplicationProcessHasExited(int appWindowsProccessId)
        {
            // Ensure that the application process has exited. 
            // Terminate the process and wait if necessary.

            // To ensure that we don't end up in an unexpected state, we use multiple ways to 
            // find application processes
            // 1. Find processes by name matching the app name
            // 2. Use the proc id from the app window UIA object
            // 3. Use the Process obj we found when this Application object was initialized.
            // This is just a sanity check. Under normal circumstances, there should only be 
            // one app process. 
            
            var appProcesses = Process.GetProcessesByName(_appProcessName).ToList();

            if (appWindowsProccessId != -1)
            {
                try
                {
                    appProcesses.Add(Process.GetProcessById(appWindowsProccessId));
                }
                catch (Exception)
                {
                    // Ignore. GetProcessById throws if the process has already exited.
                }
            }

            if (Process != null)
            {
                appProcesses.Add(Process);
            }

            foreach (var proc in appProcesses)
            {
                if (!proc.HasExited)
                {
                    if (!KillProcessAndWaitForExit(proc))
                    {
                        throw new Exception($"Unable to kill process: {proc}");
                    }
                }
            }
        }

        private int GetProcessIdFromAppWindow()
        {
            if (UIObject.Root.Children.TryFind(_windowCondition, out UIObject topWindowObj))
            {
                return topWindowObj.ProcessId;
            }
            else
            {
                return -1;
            }
        }

        private bool CloseAppWindowWithCloseButton()
        {
            var topWindowCondition = _windowCondition.OrWith(_appFrameWindowCondition);

            UIObject topWindowObj = null;
            bool didFindWindow = UIObject.Root.Children.TryFind(topWindowCondition, out topWindowObj);
            if(!didFindWindow)
            {
                Log.Comment("Application.CloseAppWindowWithCloseButton: Cound not find app window.");
                return false;
            }

            Log.Comment("Closing application: {0}", topWindowObj);
            UIObject closeAppInvoker;
            if (!topWindowObj.Descendants.TryFind(UICondition.Create("@AutomationId='__CloseAppInvoker'"), out closeAppInvoker))
            {
                Log.Comment("Application.CloseAppWindowWithCloseButton: Failed to find close app invoker.");
                return false;
            }

            Log.Comment("Invoking CloseAppInvoker {0}", closeAppInvoker);
            (new Button(closeAppInvoker)).Invoke();

            bool didWindowClose = WaitForWindowToClose(topWindowCondition);
            if (!didWindowClose)
            {
                Log.Comment("Application.CloseAppWindowWithCloseButton: Window did not close");
                return false;
            }

            return true;
        }

        private static bool KillProcessAndWaitForExit(Process process)
        {
            Log.Comment($"Killing process {process}");
            if (process.HasExited)
            {
                return true;
            }
            else
            {
                process.Kill();
                return process.WaitForExit(10000 /*milliseconds*/);
            }
        }

        private static bool WaitForWindowToClose(UICondition topWindowCondition)
        {
            // We'll wait until the window closes.  For some reason, ProcessClosedWaiter
            // doesn't seem to actually work, so we'll instead just check for the window
            // until the check fails.
            int triesLeft = 20;

            bool didFindWindow = true;

            do
            {
                if (triesLeft == 0)
                {
                    return false;
                }

                Wait.ForMilliseconds(100);
                didFindWindow = UIObject.Root.Children.TryFind(topWindowCondition, out UIObject topWindowObj);
                triesLeft--;
            } while (didFindWindow);

            return true;
        }

        public void GoBack()
        {
            Log.Comment("Going to the previous page...");

            // The System Back button sometimes cannot be found at the first attempt,
            // or invoking it sometimes fails. Retry a few times.
            for (int retries = 0; ; retries++)
            {
                try
                {
                    // If the process has closed prematurely, then there's no back button to press,
                    // so we'll just no-op.
                    if (Process.HasExited)
                    {
                        Log.Comment("Process already exited");
                        return;
                    }

                    Log.Comment("Invoking the back button...");
                    FindElement.ById<Button>("__GoBackInvoker").InvokeAndWait(TimeSpan.FromSeconds(10));
                    Log.Comment("Invoke successful.");

                    // We're now exiting the page we were previously on, so everything has changed.  As such, we should clear our
                    // element cache, in order to ensure that we don't accidentally retrieve any stale UI objects.
                    ElementCache.Clear();

                    // Successfully found and invoked the Back button. Exit the retry loop.
                    break;
                }
                catch (Exception e)
                {
                    string log = "Failed to find and invoke Back button. Error: " + e.ToString();

                    if (retries == 10)
                    {
                        Log.Error(log);
                        DumpHelper.DumpFullContext();
                        throw;
                    }
                    else
                    {
                        Log.Warning(log);
                        if (retries % 2 == 0)
                        {
                            Log.Comment("Clearing element cache which may be stale.");
                            ElementCache.Clear();
                        }
                        Log.Comment("Back button was not found. Trying again ({0}).", retries);
                        Thread.Sleep(150);
                    }
                }
            }
        }

        private UICondition CreateTopLevelWindowCondition()
        {
            string deviceFamily = Windows.System.Profile.AnalyticsInfo.VersionInfo.DeviceFamily;
            if (deviceFamily.Equals("Windows.Desktop", StringComparison.OrdinalIgnoreCase)
                || deviceFamily.Equals("Windows.Server", StringComparison.OrdinalIgnoreCase)
                || deviceFamily.Equals("Windows.Team", StringComparison.OrdinalIgnoreCase))
            {
                return UICondition.CreateFromClassName("ApplicationFrameWindow");
            }
            else
            {
                return UICondition.CreateFromClassName("Windows.UI.Core.CoreWindow");
            }
        }

        private void BuildAndInstallTestAppIfNeeded()
        {
            string[] architectures = { "x86", "x64", "ARM" };

            // First, we need to figure out what the most recently built architecture was.
            // Since MUXControls' interaction tests need to be built as AnyCPU, we can't just check our own architecture,
            // so we'll check the last-write times of Microsoft.UI.Xaml.dll and MUXControlsTestApp.exe
            // and go with what the latest was.
            string baseDirectory = Directory.GetParent(Path.GetDirectoryName(Assembly.GetExecutingAssembly().Location)).Parent.FullName;

            string mostRecentlyBuiltArchitecture = string.Empty;
            DateTime timeMostRecentlyBuilt = DateTime.MinValue;

            foreach (string architecture in architectures)
            {
                string muxPath = Path.Combine(baseDirectory, architecture, "Microsoft.UI.Xaml", "Microsoft.UI.Xaml.dll");
                string testAppExePath = Path.Combine(baseDirectory, architecture, _packageName, _packageName + ".exe");

                if (File.Exists(muxPath) && File.Exists(testAppExePath))
                {
                    DateTime muxWriteTime = File.GetLastWriteTime(muxPath);
                    DateTime testAppExeWriteTime = File.GetLastWriteTime(testAppExePath);

                    if (muxWriteTime > timeMostRecentlyBuilt || testAppExeWriteTime > timeMostRecentlyBuilt)
                    {
                        timeMostRecentlyBuilt = muxWriteTime > testAppExeWriteTime ? muxWriteTime : testAppExeWriteTime;
                        mostRecentlyBuiltArchitecture = architecture;
                    }
                }
            }

            if (mostRecentlyBuiltArchitecture.Length == 0)
            {
                Log.Warning("Could not find most recently built architecture!  Defaulting to x86.");
                mostRecentlyBuiltArchitecture = "x86";
            }

            // We'll see if we need to install the app.
            // Since we can't run as administrator in MSTest, we need to call out
            // to a script that'll install the app for us.
            string architectureDirectory = Path.Combine(baseDirectory, mostRecentlyBuiltArchitecture);
            string testAppDirectory = Path.Combine(architectureDirectory, _packageName);
            string appxDirectory = Path.Combine(testAppDirectory, "AppPackages", _packageName + "_Test");
            string appxPath = Path.Combine(appxDirectory, _packageName + ".appx");
            bool appXPackagingNecessary = false;

            if (!File.Exists(appxPath))
            {
                Log.Comment($".appx not found at '{appxPath}'");
                // If the AppX doesn't even exist, then we definitely need to package it.
                appXPackagingNecessary = true;
            }
            else
            {
                // Otherwise, we need to package it if any of its contents have been built since the last packaging.
                DateTime appxWriteTime = File.GetLastWriteTime(appxPath);
                DateTime exeWriteTime = File.GetLastWriteTime(Path.Combine(testAppDirectory, _packageName + ".exe"));
                DateTime dllWriteTime = File.GetLastWriteTime(Path.Combine(architectureDirectory, "Microsoft.UI.Xaml", "Microsoft.UI.Xaml.dll"));

                appXPackagingNecessary =
                    exeWriteTime > appxWriteTime ||
                    dllWriteTime > appxWriteTime;

                Log.Comment($"AppX packaging necessary: {appXPackagingNecessary} (appxWriteTime = {appxWriteTime}, exeWriteTime = {exeWriteTime}, dllWriteTime = {dllWriteTime})");
            }

            // Only package the AppX or install the app if we need to - otherwise, we'll get unnecessary console windows showing up
            // for a brief time on every test run, which would get very annoying very quickly.
            if (appXPackagingNecessary)
            {
                Log.Comment("Packaging and installing AppX...");

                string buildAndInstallScript = Path.Combine(baseDirectory, "AnyCPU", "MUXControls.Test", "BuildAndInstallAppX.ps1");

                ProcessStartInfo powershellProcessStartInfo =
                    new ProcessStartInfo("powershell",
                        string.Format("-ExecutionPolicy Unrestricted -File {0} {1} {2} {3} {4}",
                            buildAndInstallScript,
                            _packageName,
                            mostRecentlyBuiltArchitecture,
                            _appName,
                            _packageFamilyName));

                powershellProcessStartInfo.UseShellExecute = true;

                Process powershellProcess = Process.Start(powershellProcessStartInfo);
                powershellProcess.WaitForExit();

                if (powershellProcess.ExitCode != 0)
                {
                    throw new Exception(string.Format("Failed to package and install AppX for {0}!", _packageName));
                }
            }
            else
            {
                PackageManager packageManager = new PackageManager();
                if (packageManager.FindPackagesForUser(string.Empty, _packageFamilyName).Count() == 0)
                {
                    Log.Comment("Packaging and installing AppX...");

                    string buildAndInstallScript = Path.Combine(baseDirectory, "AnyCPU", "MUXControls.Test", "InstallAppX.ps1");

                    ProcessStartInfo powershellProcessStartInfo =
                        new ProcessStartInfo("powershell",
                            string.Format("-ExecutionPolicy Unrestricted -File {0} {1}",
                                buildAndInstallScript,
                                appxDirectory));

                    powershellProcessStartInfo.UseShellExecute = true;

                    Process powershellProcess = Process.Start(powershellProcessStartInfo);
                    powershellProcess.WaitForExit();

                    if (powershellProcess.ExitCode != 0)
                    {
                        throw new Exception(string.Format("Failed to install AppX for {0}!", _packageName));
                    }
                }
            }
        }

        #endregion

        private void LogDumpTree()
        {
            try
            {
                TestEnvironment.LogDumpTree(UIObject.Root);
            }
            catch(Exception e)
            {
                Log.Comment(e.Message);
            }
        }
    }
}
