// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

using System;
using System.Diagnostics;
using System.Threading;
using System.Threading.Tasks;
using Microsoft.UI.Xaml.Tests.MUXControls.InteractionTests.Common;
using Windows.Management.Deployment;
using Common;
using System.Linq;
using System.Collections;
using System.Collections.Generic;
using System.IO;
using System.Reflection;
using System.Runtime.CompilerServices;
using System.Runtime.InteropServices;

using WEX.TestExecution;
using WEX.TestExecution.Markup;
using WEX.Logging.Interop;

using Microsoft.Windows.Apps.Test.Automation;
using Microsoft.Windows.Apps.Test.Foundation;
using Microsoft.Windows.Apps.Test.Foundation.Controls;
using Microsoft.Windows.Apps.Test.Foundation.Patterns;
using Microsoft.Windows.Apps.Test.Foundation.Waiters;
using Windows.Foundation;

namespace Microsoft.UI.Xaml.Tests.MUXControls.InteractionTests.Infra
{
    using Window = Microsoft.Windows.Apps.Test.Foundation.Controls.Window;

    public class Application
    {
        private readonly bool _installFromDirectory;
        private readonly string _appWindowTitle;
        private readonly string _appName;
        private readonly bool _isUWPApp;
        private readonly bool _isPackaged;

        // Properties to set if _installFromDirectory = false
        private readonly string _packageName;
        private readonly string _packageFamilyName;
        private readonly string _appProcessName;
        private readonly string _appInstallerName;
        private readonly string _certSerialNumber;
        private readonly string _baseAppxDir;
        private readonly string _unpackagedExePath;
        
        // Properties to set if _installFromDirectory = true
        private readonly string _pathToTestAppDirectory;

        private readonly UICondition _windowCondition = null;
        private readonly UICondition _appFrameWindowCondition = null;

        private AutoResetEvent _processExitedEvent;
        private AutoResetEvent _appWatcherThreadStarted;

        private static readonly UICondition _desktopWindowCondition =
                UICondition.CreateFromClassName("Window") // WPF class name
                .OrWith(UICondition.CreateFromClassName("WinUIDesktopWin32WindowClass")); // Win32 class name

        public Application(string packageFamilyName, string appName, string testAppMainWindowTitle, bool isUWPApp, string pathToTestAppDirectory)
            : this(packageName: string.Empty, packageFamilyName, appName, testAppMainWindowTitle, testAppProcessName: string.Empty, testAppInstallerName: string.Empty, certSerialNumber: string.Empty, baseAppxDir: string.Empty, isUWPApp, unpackagedExePath: string.Empty, isPackaged: true, pathToTestAppDirectory)
        {
            _installFromDirectory = true;
        }

        public Application(string packageName, string packageFamilyName, string appName, string testAppMainWindowTitle, string testAppProcessName, string testAppInstallerName, string certSerialNumber, string baseAppxDir, bool isUWPApp, string unpackagedExePath, bool isPackaged)
            : this(packageName, packageFamilyName, appName, testAppMainWindowTitle, testAppProcessName, testAppInstallerName, certSerialNumber, baseAppxDir, isUWPApp, unpackagedExePath, isPackaged, pathToTestAppDirectory: string.Empty)
        {
            _installFromDirectory = false;
        }

        private Application(string packageName, string packageFamilyName, string appName, string testAppMainWindowTitle, string testAppProcessName, string testAppInstallerName, string certSerialNumber, string baseAppxDir, bool isUWPApp, string unpackagedExePath, bool isPackaged, string pathToTestAppDirectory)
        {
            _packageName = packageName;
            _packageFamilyName = packageFamilyName;
            _appName = appName;
            _isUWPApp = isUWPApp;
            _isPackaged = isPackaged;
            _unpackagedExePath = unpackagedExePath;
            _certSerialNumber = certSerialNumber;
            _baseAppxDir = baseAppxDir;

            _appWindowTitle = testAppMainWindowTitle;
            _appProcessName = testAppProcessName;
            _appInstallerName = testAppInstallerName;
            _pathToTestAppDirectory = pathToTestAppDirectory;

            _appWindowTitle = testAppMainWindowTitle;
            _appProcessName = testAppProcessName;
            _appInstallerName = testAppInstallerName;

            if (_isUWPApp && _isPackaged)
            {
                _windowCondition = UICondition.Create("@ClassName='Windows.UI.Core.CoreWindow' AND @Name={0}", _appWindowTitle);
                _appFrameWindowCondition = UICondition.Create("@ClassName='ApplicationFrameWindow' AND @Name={0}", _appWindowTitle);
            }
            else
            {
                UICondition windowCondition = _desktopWindowCondition.AndWith(UICondition.CreateFromName(_appWindowTitle));
                _windowCondition = windowCondition;
                _appFrameWindowCondition = windowCondition;
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
        internal bool Initialize(bool doLaunch = false, string deploymentDir = null)
        {
            var topWindowCondition = _windowCondition.OrWith(_appFrameWindowCondition);

            UIObject topWindowObj = null;
            bool didFindWindow = UIObject.Root.Children.TryFind(topWindowCondition, out topWindowObj);

            // Only try to launch the app if we couldn't find the window.
            if (doLaunch && !didFindWindow)
            {
                CoreWindow = Launch(deploymentDir);
                
                if (CoreWindow == null)
                {
                    return false;
                }

                foreach (UIObject obj in CoreWindow.Ancestors)
                {
                    if (obj.Matches(_appFrameWindowCondition))
                    {
                        ApplicationFrameWindow = CoreWindow.Parent;
                        break;
                    }
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
                MaximizeWindow(ApplicationFrameWindow);
            }
            else if (!_isUWPApp && CoreWindow != null)
            {
                // If we're not a UWP app, then the CoreWindow object we got is a Window that can be maximized.
                MaximizeWindow(CoreWindow);
            }

            Process = Process.GetProcessById(CoreWindow.ProcessId);

            _processExitedEvent = new AutoResetEvent(false /*signaled*/);
            _appWatcherThreadStarted = new AutoResetEvent(false /*signaled*/);
            ThreadPool.QueueUserWorkItem(AppWatcherThread, this);

            if (!_appWatcherThreadStarted.WaitOne(5000))
            {
                throw new Exception("Failed to start AppWatcherThread.");
            }

            Wait.InitializeWaitHelper();

            if (TestEnvironment.TestContext.Properties.Contains("WaitForAppDebugger"))
            {
                Wait.ForAppDebugger();
            }

            TestCleanupHelper.TestSetupHelperPendingDisposals = 0;
            return true;
        }

        // This function runs on a thread pool thread, and waits on the the app's process so that we know right away if
        // it crashed (or exited unexpectedly).
        static void AppWatcherThread(Object state) 
        {
            Application app = (Application)state;
            Process process = app.Process;
            AutoResetEvent processExitedEvent = app._processExitedEvent;
            IntPtr processHandle = NativeMethods.OpenProcess(
                NativeMethods.ProcessAccessFlags.PROCESS_QUERY_INFORMATION, false /*inheritHandle*/, (uint)process.Id);
            if (processHandle == IntPtr.Zero)
            {
                throw new Exception("AppWatcherThread: OpenProcess failed.");
            }

            try
            {
                app._appWatcherThreadStarted.Set();
                bool waitForExitResult = process.WaitForExit(15 * 60 * 1000);
                if (!waitForExitResult)
                {
                    throw new Exception("AppWatcherThread: Test app process ran for an unexpectedly long time.");
                }

                uint exitCode = 0;
                if (!NativeMethods.GetExitCodeProcess(processHandle, out exitCode))
                {
                    throw new Exception("AppWatcherThread: GetExitCodeProcess failed.");
                }

                if (exitCode == 0)
                {
                    // Process exited sucessfully.
                }
                else if (exitCode == 0xffffffff)
                {
                    Log.Comment("AppWatcherThread: Test app process returned 0xffffffff, it was probably forcibly killed.");
                }
                else
                {
                    Verify.Fail(string.Format("AppWatcherThread: Test app process appears to have crashed, returned exit code 0x{0:X}.", exitCode));
                }
            }
            finally
            {
                NativeMethods.CloseHandle(processHandle);
                processExitedEvent.Set();
            }
        }

        public void KillAndWaitForExit()
        {
            KillProcessAndWaitForExit(Process);
        }

        public void WaitForExit()
        {            
            bool result = _processExitedEvent.WaitOne(20 * 1000);
            if (!result)
            {
                Verify.Fail("Test app process did not exit when expected.");
            }
        }

        private UIObject Launch(string deploymentDir)
        {
            UIObject coreWindow = null;
            bool packageInstalled = false;

            if (_isPackaged)
            {
                if (_installFromDirectory)
                {
                    packageInstalled = TestAppInstallHelper.InstallTestAppFromDirectoryIfNeeded(Path.Combine(deploymentDir, _pathToTestAppDirectory), _packageFamilyName);
                }
                else
                {
                    packageInstalled = TestAppInstallHelper.InstallTestAppFromPackageIfNeeded(deploymentDir, _packageName, _packageFamilyName, _appInstallerName, _appProcessName);
                }
            }

            if (_isPackaged && !packageInstalled)
            {
                return null;
            }

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

            Log.Comment("15056441 tracing, device family:" + global::Windows.System.Profile.AnalyticsInfo.VersionInfo.DeviceFamily);

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
                    if (_isPackaged)
                    {
                        coreWindow = _isUWPApp ? LaunchUWPApp() : LaunchNonUWPApp();
                    }
                    else
                    {
                        using (AppLaunchWaiter launchWaiter = new AppLaunchWaiter(_windowCondition))
                        {
                            string unpackagedExeFullPath = Path.Combine(Path.GetDirectoryName(Assembly.GetExecutingAssembly().Location), "UnpackagedApps", _unpackagedExePath);

                            if (!File.Exists(unpackagedExeFullPath))
                            {
                                Verify.Fail($"Executable not found at {unpackagedExeFullPath}!");
                                return null;
                            }

                            //Instead of launching the process directly it is invoked through explorer.exe, resulting in the process starting unelevated. 
                            ProcessStartInfo unelevatedProcessStartInfo = new ProcessStartInfo();
                            unelevatedProcessStartInfo.FileName = "C:\\Windows\\explorer.exe";
                            unelevatedProcessStartInfo.Arguments = unpackagedExeFullPath;

                            Process.Start(unelevatedProcessStartInfo);

                            launchWaiter.Wait();
                            coreWindow = launchWaiter.Source;
                        }
                    }
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

        private UIObject LaunchNonUWPApp()
        {
            Debug.Assert(!_isUWPApp);
            var nameCondition = UICondition.CreateFromName(_appWindowTitle);
            var topLevelWindowCondition = _desktopWindowCondition.AndWith(nameCondition);

            UIObject window;

            using (AppLaunchWaiter waiter = new AppLaunchWaiter(topLevelWindowCondition))
            {
                // We need the app itself to be launched as a non-elevated process, as referencing the activatable WinRT types
                // specified in AppxManifest.xml is not currently supported in an elevated process.
                // Calling CoCreateInstance with CLSCTX_LOCAL_SERVER gets us an instance that will not launch an elevated app.
                // This is essentially what UAPApp.Launch does, except for this step - using UAPApp.Launch here would result in
                // the app being elevated.
                if (NativeMethods.CoCreateInstance(
                    NativeMethods.CLSID_ApplicationActivationManager,
                    IntPtr.Zero,
                    NativeMethods.CLSCTX.CLSCTX_LOCAL_SERVER,
                    NativeMethods.CLSID_IApplicationActivationManager,
                    out object applicationActivationManagerAsObject) != 0)
                {
                    throw new Exception("Failed to create ApplicationActivationManager!");
                }

                var applicationActivationManager = (NativeMethods.IApplicationActivationManager)applicationActivationManagerAsObject;
                applicationActivationManager.ActivateApplication(_appName, null, NativeMethods.ActivateOptions.None, out uint processId);
                
                waiter.Wait();
                window = waiter.Source;
            }

            NativeMethods.SetForegroundWindow(window.NativeWindowHandle);
            return window;
        }

        public void Close()
        {
            int appWindowsProcessId = GetProcessIdFromAppWindow();

            if (appWindowsProcessId == -1)
            {
                Log.Comment("App window not found, process may have already exited.");
            }
            else
            {
                if (CloseAppWindowWithCloseButton())
                {
                    if (!Process.GetProcessById(appWindowsProcessId).WaitForExit(20 * 1000 /*milliseconds*/))
                    {
                        Log.Comment("Invoked the close button, but the process has not exited as expected.");
                    }
                }
                else
                {
                    Log.Comment("Failed to close application window. We will fall back to terminating the app process.");
                }

                EnsureApplicationProcessHasExited(appWindowsProcessId);
            }

            if (Process != null)
            {
                Process.Dispose();
                Process = null;
            }
        }

        private class ProcessComparer : IEqualityComparer<Process>
        {
            public bool Equals(Process x, Process y)
            {
                return
                    (x == null && y != null) ||
                    (x != null && y == null) ||
                    (x.Id != y.Id);
            }

            public int GetHashCode(Process obj)
            {
                return obj.GetHashCode();
            }
        }

        private void EnsureApplicationProcessHasExited(int appWindowsProcessId)
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

            var appProcesses = Process.GetProcessesByName(Path.GetFileNameWithoutExtension(_appProcessName)).ToList();

            if (appWindowsProcessId != -1)
            {
                try
                {
                    appProcesses.Add(Process.GetProcessById(appWindowsProcessId));
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

            foreach (var proc in appProcesses.Distinct(new ProcessComparer()))
            {
                if (!proc.HasExited && !KillProcessAndWaitForExit(proc))
                {
                    throw new Exception($"Unable to kill process: {proc}");
                }
            }
        }

        public int GetProcessIdFromAppWindow()
        {
            var topWindowCondition = _windowCondition.OrWith(_appFrameWindowCondition);

            if (UIObject.Root.Children.TryFind(topWindowCondition, out UIObject topWindowObj))
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
            try
            {
                var topWindowCondition = _windowCondition.OrWith(_appFrameWindowCondition);

                bool didFindWindow = UIObject.Root.Children.TryFind(topWindowCondition, out UIObject topWindowObj);
                if(!didFindWindow)
                {
                    Log.Comment("Application.CloseAppWindowWithCloseButton: Could not find app window.");
                    return false;
                }

                Log.Comment("Closing application: {0}", topWindowObj);
                if (!topWindowObj.Descendants.TryFind(UICondition.Create("@AutomationId='__CloseAppInvoker'"), out UIObject closeAppInvoker))
                {
                    Log.Comment("Application.CloseAppWindowWithCloseButton: Failed to find close app invoker");
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
            }
            catch (COMException e)
            {
                Log.Comment("Failed to close due to exception: " + e.ToString());
                return false;
            }

            return true;
        }

        private static bool KillProcessAndWaitForExit(Process process)
        {
            Log.Comment($"Killing process {process.Id}");
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

            bool didFindWindow;
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
            string deviceFamily = global::Windows.System.Profile.AnalyticsInfo.VersionInfo.DeviceFamily;
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

        public bool SupportsBackButton { get { return _isUWPApp; }}
        public bool SupportsExtendViewIntoTitleBar { get { return _isUWPApp; }}
        
        private void MaximizeWindow(UIObject windowObject)
        {
            var window = new Window(windowObject);
            if (window.CanMaximize)
            {
                window.SetWindowVisualState(WindowVisualState.Maximized);
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
    
        private static class NativeMethods
        {
            public enum ActivateOptions
            {
                None = 0x00000000,  // No flags set
                DesignMode = 0x00000001,  // The application is being activated for design mode, and thus will not be able to
                                          // to create an immersive window. Window creation must be done by design tools which
                                          // load the necessary components by communicating with a designer-specified service on
                                          // the site chain established on the activation manager.  The splash screen normally
                                          // shown when an application is activated will also not appear.  Most activations
                                          // will not use this flag.
                NoErrorUI = 0x00000002,  // Do not show an error dialog if the app fails to activate.                                
                NoSplashScreen = 0x00000004,  // Do not show the splash screen when activating the app.
            }

            public const string CLSID_ApplicationActivationManager_String = "45ba127d-10a8-46ea-8ab7-56ea9078943c";
            public const string CLSID_IApplicationActivationManager_String = "2e941141-7f97-4756-ba1d-9decde894a3d";

            public static readonly Guid CLSID_ApplicationActivationManager = new Guid(CLSID_ApplicationActivationManager_String);
            public static readonly Guid CLSID_IApplicationActivationManager = new Guid(CLSID_IApplicationActivationManager_String);

            [ComImport, Guid(CLSID_IApplicationActivationManager_String), InterfaceType(ComInterfaceType.InterfaceIsIUnknown)]
            public interface IApplicationActivationManager
            {
                // Activates the specified immersive application for the "Launch" contract, passing the provided arguments
                // string into the application.  Callers can obtain the process Id of the application instance fulfilling this contract.
                IntPtr ActivateApplication([In] String appUserModelId, [In] String arguments, [In] ActivateOptions options, [Out] out UInt32 processId);
                IntPtr ActivateForFile([In] String appUserModelId, [In] IntPtr /*IShellItemArray* */ itemArray, [In] String verb, [Out] out UInt32 processId);
                IntPtr ActivateForProtocol([In] String appUserModelId, [In] IntPtr /* IShellItemArray* */itemArray, [Out] out UInt32 processId);
            }

            [DllImport("api-ms-win-ntuser-ie-window-l1-1-0.dll", SetLastError = true)]
            public static extern bool SetForegroundWindow(IntPtr hWnd);

            [DllImport("ole32.dll", CharSet = CharSet.Unicode, ExactSpelling = true, PreserveSig = false)]
            public static extern UInt32 CoCreateInstance(
                [In, MarshalAs(UnmanagedType.LPStruct)] Guid rclsid,
                IntPtr pUnkOuter,
                CLSCTX dwClsContext,
                [In, MarshalAs(UnmanagedType.LPStruct)] Guid riid,
                [MarshalAs(UnmanagedType.IUnknown)] out object rReturnedComObject);

            [Flags]
            public enum CLSCTX : uint
            {
                CLSCTX_INPROC_SERVER = 0x1,
                CLSCTX_INPROC_HANDLER = 0x2,
                CLSCTX_LOCAL_SERVER = 0x4,
                CLSCTX_INPROC_SERVER16 = 0x8,
                CLSCTX_REMOTE_SERVER = 0x10,
                CLSCTX_INPROC_HANDLER16 = 0x20,
                CLSCTX_RESERVED1 = 0x40,
                CLSCTX_RESERVED2 = 0x80,
                CLSCTX_RESERVED3 = 0x100,
                CLSCTX_RESERVED4 = 0x200,
                CLSCTX_NO_CODE_DOWNLOAD = 0x400,
                CLSCTX_RESERVED5 = 0x800,
                CLSCTX_NO_CUSTOM_MARSHAL = 0x1000,
                CLSCTX_ENABLE_CODE_DOWNLOAD = 0x2000,
                CLSCTX_NO_FAILURE_LOG = 0x4000,
                CLSCTX_DISABLE_AAA = 0x8000,
                CLSCTX_ENABLE_AAA = 0x10000,
                CLSCTX_FROM_DEFAULT_CONTEXT = 0x20000,
                CLSCTX_ACTIVATE_32_BIT_SERVER = 0x40000,
                CLSCTX_ACTIVATE_64_BIT_SERVER = 0x80000,
                CLSCTX_INPROC = CLSCTX_INPROC_SERVER | CLSCTX_INPROC_HANDLER,
                CLSCTX_SERVER = CLSCTX_INPROC_SERVER | CLSCTX_LOCAL_SERVER | CLSCTX_REMOTE_SERVER,
                CLSCTX_ALL = CLSCTX_SERVER | CLSCTX_INPROC_HANDLER
            }

            [DllImport("kernel32.dll", SetLastError = true)]
            public static extern IntPtr OpenProcess(
                ProcessAccessFlags dwDesiredAccess,
                bool bInheritHandle,
                uint dwProcessId);

            [Flags]
            public enum ProcessAccessFlags : uint
            {
                PROCESS_TERMINATE = 0x0001,
                PROCESS_CREATE_THREAD = 0x0002,
                PROCESS_SET_SESSIONID = 0x0004,
                PROCESS_VM_OPERATION = 0x0008,
                PROCESS_VM_READ = 0x0010,
                PROCESS_VM_WRITE = 0x0020,
                PROCESS_DUP_HANDLE = 0x0040,
                PROCESS_CREATE_PROCESS = 0x0080,
                PROCESS_SET_QUOTA = 0x0100,
                PROCESS_SET_INFORMATION = 0x0200,
                PROCESS_QUERY_INFORMATION = 0x0400,
                PROCESS_SUSPEND_RESUME = 0x0800,
                PROCESS_QUERY_LIMITED_INFORMATION = 0x1000,
                SYNCHRONIZE = 0x00100000,
                PROCESS_ALL_ACCESS = 0x1F0FFF
            }

            [DllImport("kernel32.dll", SetLastError = true)]
            public static extern bool GetExitCodeProcess(
                IntPtr hProcess,
                out uint lpExitCode);

            [DllImport("kernel32.dll", SetLastError = true)]
            public static extern bool CloseHandle(IntPtr hObject);
        }
    }
}
