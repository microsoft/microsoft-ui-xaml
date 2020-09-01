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
using Windows.Foundation;

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

        private readonly string _certSerialNumber;
        private readonly string _baseAppxDir;

        private readonly UICondition _windowCondition = null;
        private readonly UICondition _appFrameWindowCondition = null;

        public Application(string packageName, string packageFamilyName, string appName, string testAppMainWindowTitle, string testAppProcessName, string testAppInstallerName, string certSerialNumber, string baseAppxDir, bool isUWPApp = true)
        {
            _packageName = packageName;
            _packageFamilyName = packageFamilyName;
            _appName = appName;
            _isUWPApp = isUWPApp;
            _certSerialNumber = certSerialNumber;
            _baseAppxDir = baseAppxDir;

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
                // topWindowObj should match either _windowCondition or _appFrameWindowCondition

                if (topWindowObj.Matches(_windowCondition))
                {
                    // If the top level window is CoreWindow, then there is no AppFrame window:
                    CoreWindow = topWindowObj;
                    ApplicationFrameWindow = null;
                }
                else // _appFrameWindowCondition
                {
                    if(!topWindowObj.Matches(_appFrameWindowCondition))
                    {
                        // This should never happen
                        Verify.Fail($"Expected topWindowObj ({UIObjectToLoggableString(topWindowObj)}) to match _appFrameWindowCondition ({_appFrameWindowCondition})");
                    }

                    // Maxmize window to ensure we can find UIA elements
                    var appFrameWindow = new Window(topWindowObj);
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

            if (TestEnvironment.TestContext.Properties.Contains("WaitForAppDebugger"))
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
            InstallTestAppIfNeeded();
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
                    coreWindow = _isUWPApp ? LaunchUWPApp() : LaunchNonUWPApp(_packageName);
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

        private UIObject LaunchUWPApp()
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

#if !USING_TAEF
        private void InstallTestAppIfNeeded()
        {
            string mostRecentlyBuiltAppx = string.Empty;
            DateTime timeMostRecentlyBuilt = DateTime.MinValue;

            var exclude = new[] { "Microsoft.NET.CoreRuntime", "Microsoft.VCLibs" };

            var files = Directory.GetFiles(_baseAppxDir, "*.appx", SearchOption.AllDirectories).Where(f => !exclude.Any(Path.GetFileNameWithoutExtension(f).Contains));

            if (files.Count() == 0)
            {
                throw new Exception(string.Format("Failed to find '*.appx' in {0}'!", _baseAppxDir));
            }

            foreach (string file in files)
            {
                DateTime fileWriteTime = File.GetLastWriteTime(file);

                if (fileWriteTime > timeMostRecentlyBuilt)
                {
                    timeMostRecentlyBuilt = fileWriteTime;
                    mostRecentlyBuiltAppx = file;
                }
            }

            PackageManager packageManager = new PackageManager();
            DeploymentResult result = null;

            var installedPackages = packageManager.FindPackagesForUser(string.Empty, _packageFamilyName);
            foreach (var installedPackage in installedPackages)
            {
                Log.Comment("Test AppX package already installed. Removing existing package by name: {0}", installedPackage.Id.FullName);

                AutoResetEvent removePackageCompleteEvent = new AutoResetEvent(false);
                var removePackageOperation = packageManager.RemovePackageAsync(installedPackage.Id.FullName);
                removePackageOperation.Completed = (operation, status) =>
                {
                    if (status != AsyncStatus.Started)
                    {
                        result = operation.GetResults();
                        removePackageCompleteEvent.Set();
                    }
                };
                removePackageCompleteEvent.WaitOne();

                if (!string.IsNullOrEmpty(result.ErrorText))
                {
                    Log.Error("Removal failed!");
                    Log.Error("Package removal ActivityId = {0}", result.ActivityId);
                    Log.Error("Package removal ErrorText = {0}", result.ErrorText);
                    Log.Error("Package removal ExtendedErrorCode = {0}", result.ExtendedErrorCode);
                }
                else
                {
                    Log.Comment("Removal successful.");
                }
            }

            Log.Comment("Installing AppX...");

            Log.Comment("Checking if the app's certificate is installed...");

            // If the certificate for the app is not present, installing it requires elevation.
            // We'll run Add-AppDevPackage.ps1 without -Force in that circumstance so the user
            // can be prompted to allow elevation.  We don't want to run it without -Force all the time,
            // as that prompts the user to hit enter at the end of the install, which is an annoying
            // and unnecessary step. The parameter is the SHA-1 hash of the certificate.

            var certutilProcess = Process.Start(new ProcessStartInfo("certutil.exe",
                    string.Format("-verifystore TrustedPeople {0}", _certSerialNumber)) {
                UseShellExecute = true
            });
            certutilProcess.WaitForExit();

            if(certutilProcess.ExitCode == 0)
            {
                Log.Comment("Certificate is installed. Installing app...");
            }
            else
            {
                Log.Comment("Certificate is not installed. Installing app and certificate...");
            }

            var powershellProcess = Process.Start(new ProcessStartInfo("powershell",
                    string.Format("-ExecutionPolicy Unrestricted -File {0}\\Add-AppDevPackage.ps1 {1}",
                        Path.GetDirectoryName(mostRecentlyBuiltAppx),
                        certutilProcess.ExitCode == 0 ? "-Force" : "")) {
                UseShellExecute = true
            });
            powershellProcess.WaitForExit();

            if (powershellProcess.ExitCode != 0)
            {
                throw new Exception(string.Format("Failed to install AppX for {0}!", _packageName));
            }
        }
#endif

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

        // UIObjects expose a ToString method to give a human-readable representation of the object.
        // But the string includes '{' and '}' which the test logger does not handle well.
        private string UIObjectToLoggableString(UIObject obj)
        {
            if(obj == null)
            {
                return "Null";
            }
            else
            {
                return obj.ToString().Replace('{', '[').Replace('}', ']');
            }
        }
    }
}
