// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

using Common;
using Windows.UI.Xaml.Tests.MUXControls.InteractionTests.Common;
using Windows.UI.Xaml.Tests.MUXControls.InteractionTests.Infra;

using Microsoft.Windows.Apps.Test.Automation;
using Microsoft.Windows.Apps.Test.Foundation;
using Microsoft.Windows.Apps.Test.Foundation.Controls;
using Microsoft.Windows.Apps.Test.Foundation.Waiters;

#if USING_TAEF
using WEX.TestExecution;
using WEX.TestExecution.Markup;
using WEX.Logging.Interop;
#else
using Microsoft.VisualStudio.TestTools.UnitTesting;
#endif

using System;
using System.Diagnostics;
using System.Drawing;
using System.IO;
using System.Net.Http;
using System.Runtime.InteropServices;
using System.Threading.Tasks;

using Button = Microsoft.Windows.Apps.Test.Foundation.Controls.Button;

namespace Windows.UI.Xaml.Tests.MUXControls.InteractionTests
{
    [TestClass]
    public class WebView2Tests
    {
        [ClassInitialize]
        [TestProperty("RunAs", "User")]
        [TestProperty("Classification", "Integration")]
        [TestProperty("Platform", "Any")]
        [TestProperty("TestSuite", "A")]
        public static void ClassInitialize(TestContext testContext)
        {
            TestEnvironment.Initialize(testContext);
            EnsureBrowser();
        }

        private static async Task DownloadFileAsync(string url, string filePath)
        {
            using (var client = new HttpClient())
            {
                using (var result = await client.GetAsync(url))
                {
                    result.EnsureSuccessStatusCode();

                    var bytes = await result.Content.ReadAsByteArrayAsync();
                    await File.WriteAllBytesAsync(filePath, bytes);
                }
            }
        }

        private static void DownloadFile(string url, string filePath)
        {
            var task = DownloadFileAsync(url, filePath);
            task.Wait(TimeSpan.FromMinutes(10));
        }

        // Ensure a suitable version of Edge browser or WebView2 runtime is present. If not, get an installer from the web and install it.
        public static void EnsureBrowser()
        {
            // If a previous run of CoreWebView2Initialized_FailedTest failed to clean up the
            // fake browser executable folder key it set, remove it here.
            RemoveFakeBrowserExecutableFolderKey();

            // 1. Get installed Edge browser (or WebView2 Runtime) build version info.
            int browserBuildVersion = GetInstalledBrowserVersion();

            // 2. Get Microsoft.Web.WebView2.Core.dll build version info.
            int sdkBuildVersion = GetSdkBuildVersion();

            // 3. Check if a runtime is already installed, and if it's compatible.
            bool hasCompatibleRuntimeInstalled = GetHasCompatibleRuntimeInstalled(browserBuildVersion, sdkBuildVersion);

            // 4a. If we have a compatible runtime installed, continue on to the tests!
            if (hasCompatibleRuntimeInstalled)
            {
                return;
            }

            // 4b. If we don't have a compatible runtime, try to install one.

            // 5. Before installing, check if Edge is already installing/updating. We can't run the standalone installer if it is.
            bool didWait = WaitForEdgeIfAlreadyInstalling();

            if (didWait && IsEdgeAlreadyInstalling())
            {
                Log.Error("WebView2Tests Init: Edge took more than 3 minutes to install, give up.");
            }
            else if (didWait)
            {
                // If we waited for Edge to finish installing/updating and it finished, check the versions again
                browserBuildVersion = GetInstalledBrowserVersion();
                hasCompatibleRuntimeInstalled = GetHasCompatibleRuntimeInstalled(browserBuildVersion, sdkBuildVersion);
                if (hasCompatibleRuntimeInstalled)
                {
                    return;
                }
            }

            // 6. If Edge wasn't already installing/updating, or it was but installed something old (unlikely),
            // run the standalone installer to install it.
            // This installation can sometimes fail, so we'll try a number of times before failing out.
            int attemptsLeft = 5;
            bool successfullyInstalled = false;
            while (attemptsLeft > 0)
            {
                attemptsLeft--;
                successfullyInstalled = TryInstallingBrowser(attemptsLeft);
                if (successfullyInstalled) break;
            }
            if (attemptsLeft == 0 && !successfullyInstalled)
            {
                Log.Error("WebView2Tests Init: Browser installation failed, out of retries.");
            }
        }

        private static void RemoveFakeBrowserExecutableFolderKey()
        {
            var browserExecutableKey = GetBrowserExecutableFolderKey();
            var browserExecutableFolder = browserExecutableKey.GetValue(TestApplicationInfo.MUXControlsTestApp.ProcessName);
            if (browserExecutableFolder != null)
            {
                browserExecutableKey.DeleteValue(TestApplicationInfo.MUXControlsTestApp.ProcessName);
                Log.Comment("Removed key for {0}", TestApplicationInfo.MUXControlsTestApp.ProcessName);
            }
        }

        private static int GetInstalledBrowserVersion()
        {
            string browserVersionString = string.Empty;
            IntPtr browserVersionPtr;
            int retval = NativeMethods.GetAvailableCoreWebView2BrowserVersionString(string.Empty, out browserVersionPtr);
            if (retval != (int)NativeMethods.HResults.S_OK)
            {
                Log.Warning("WebView2Tests Init: Error: got hresult {0:x8} retrieving GetAvailableCoreWebView2BrowserVersionString", retval);
            }
            else if (browserVersionPtr == IntPtr.Zero)
            {
                Log.Warning("WebView2Tests Init: GetAvailableCoreWebView2BrowserVersionString returned version of 0");
            }
            else
            {
                // eg "80.0.361.48 beta", 361 is the build version
                browserVersionString = Marshal.PtrToStringUni(browserVersionPtr);
                Marshal.FreeCoTaskMem(browserVersionPtr);
            }

            int browserBuildVersion = 0;
            string installedBrowser;

            if (string.IsNullOrEmpty(browserVersionString))
            {
                installedBrowser = "none";
            }
            else
            {
                browserBuildVersion = int.Parse(browserVersionString.Split('.')[2]);
                installedBrowser = string.Format("version {0} [build version: {1}]", browserVersionString, browserBuildVersion);
            }
            Log.Comment("WebView2Tests Init: Found Edge browser/WV2Runtime: {0}", installedBrowser);
            return browserBuildVersion;
        }

        private static int GetSdkBuildVersion()
        {
            int sdkBuildVersion = 0;
            string dllName = "Microsoft.Web.WebView2.Core.dll";
            string dllPath = Path.Combine(Environment.CurrentDirectory, dllName);

            try
            {
                FileVersionInfo sdkVersionInfo = FileVersionInfo.GetVersionInfo(dllPath);
                sdkBuildVersion = sdkVersionInfo.ProductBuildPart;
                Log.Comment("WebView2Tests Init: Found {0}: version {1} [build version: {2}] (file: {3})",
                            dllName,
                            sdkVersionInfo.ProductVersion,
                            sdkBuildVersion,
                            sdkVersionInfo.FileName);
            }
            catch (Exception e)
            {
                Log.Error("WebView2Tests Init: could not find loader at {0} [exception: {1}]", dllPath, e.ToString());
            }
            return sdkBuildVersion;
        }

        private static bool GetHasCompatibleRuntimeInstalled(int browserBuildVersion, int sdkBuildVersion)
        {
            bool hasCompatibleRuntime = false;
            // The full versions of browser/runtime and WebView2 SDK are not directly comparable: 
            // Browser/Runtime version ex: 80.0.361.48
            // WebView2 SDK version ex: 0.8.355
            // Webview2Loader compares the build versions (361 and 355 in above example), we do so here as well
            if (browserBuildVersion >= sdkBuildVersion)
            {
                Log.Comment("WebView2Tests Init: Installed Edge build will be used for testing... ");
                hasCompatibleRuntime = true;
            }
            return hasCompatibleRuntime;
        }

        private static bool WaitForEdgeIfAlreadyInstalling()
        {
            bool didWait = false;
            int curWait = 0;
            int maxWait = 180; // wait for 3 minutes, should finish in 1-2
            while (IsEdgeAlreadyInstalling() && curWait < maxWait)
            {
                Log.Comment("Note: Edge is updating, wait up to 180 seconds for it to finish. Waited {0} seconds so far, wait another 10...", curWait);
                Wait.ForSeconds(10);
                curWait += 10;
                didWait = true;
            }
            return didWait;
        }

        private static bool IsEdgeAlreadyInstalling()
        {
            bool edgeUpdateIsRunning = false;
            // Print if there are any Edge processes running that might interfere with installation
            // (specifically, MicrosoftEdge<Version> or MicrosoftEdgeUpdate).
            foreach (var process in Process.GetProcesses())
            {
                if (process.ProcessName.ToLower().ToString().Contains("edge"))
                {
                    Log.Comment("Note: process '{0}' ({1}) is running", process.ProcessName, process.Id);
                    if (process.ProcessName.ToString().Equals("MicrosoftEdgeUpdate"))
                    {
                        edgeUpdateIsRunning = true;
                    }
                }
            }
            return edgeUpdateIsRunning;
        }

        private static bool TryInstallingBrowser(int attemptsLeft)
        {
            Log.Comment("Downloading Edge installer...");
                DownloadFile("https://go.microsoft.com/fwlink/p/?LinkId=2124703",
                    "MicrosoftEdgeWebview2Setup.exe");
            bool successfullyInstalled = false;
            string installer = "MicrosoftEdgeWebview2Setup.exe";
            // Note: Starting with SDK 0.9.488 / Edge version 83, evergreen WebView2 no longer targets the Stable
            //       browser channel. Instead, it targets another set of binaries, branded Evergreen WebView2 Runtime.
            //       (See https://docs.microsoft.com/en-us/microsoft-edge/webview2/releasenotes).
            string installerArgs_WV2Runtime = "/silent /install";
            Log.Comment(@"WebView2Tests Init: Installing WebView2 runtime: '{0} {1}'", installer, installerArgs_WV2Runtime);
            ProcessStartInfo installerStartInfo = new ProcessStartInfo(installer, installerArgs_WV2Runtime);

            Process installerProcess = Process.Start(installerStartInfo);
            installerProcess.WaitForExit();

            if (installerProcess.ExitCode == 0)
            {
                Log.Comment("WebView2Tests Init: {0} exited successfully", installer);
                successfullyInstalled = true;
            }
            else
            {
                Log.Warning("WebView2Tests Init: {0} failed with exit code {1}! Attempts left: {2}", installer, installerProcess.ExitCode, attemptsLeft);
                // Print information about some of the error codes we sometimes hit
                if (installerProcess.ExitCode == 4)
                {
                    Log.Comment("Exit code 4 = HIGHER_VERSION_EXISTS, Higher version already exists");
                }
                else if (installerProcess.ExitCode == 60)
                {
                    Log.Comment("Exit code 60 = SETUP_SINGLETON_ACQUISITION_FAILED, The setup process could not acquire the exclusive right to modify the installation.");
                }
            }
            return successfullyInstalled;
        }

        [TestCleanup]
        public void TestCleanup()
        {
            TestCleanupHelper.Cleanup();
        }

        internal class WebView2TestSetupHelper : TestSetupHelper
        {
            public WebView2TestSetupHelper(global::System.Collections.Generic.ICollection<string> testNames)
                : base(testNames)
            {
            }

            public override void Dispose()
            {
                try
                {
                    var result = new Edit(FindElement.ById("CleanupResultTextBox"));
                    Button CleanupTestButton = new Button(FindElement.ById("CleanupTest"));
                    string expectedValue = "Cleanup completed.";

                    Log.Comment("Waiting for cleanup...");
                    using (var waiter = new ValueChangedEventWaiter(result, expectedValue))
                    {
                        CleanupTestButton.Invoke();
                        if (!waiter.TryWait())
                        {
                            Log.Warning("Cleanup may have failed.");
                        }
                    }
                }
                catch { }

                base.Dispose();
            }
        }

        private static void WaitForWebMessageResult(string selectedTest)
        {
            var result = new Edit(FindElement.ById("TestResult"));
            var check = new CheckBox(FindElement.ById("MessageReceived"));
            string expectedValue = selectedTest + ": Passed []";

            Log.Comment("Waiting for result...");
            using (var waiter = check.GetToggledWaiter())
            {
                // If the web message hasn't been received, wait for it
                if (check.ToggleState != ToggleState.On && !waiter.TryWait())
                {
                    Log.Error("Expected web message was never received.");
                }
            }

            // By now, we'll have gotten a pass/fail result
            Verify.AreEqual(expectedValue, result.Value);
        }

        private static void CompleteTestAndWaitForWebMessageResult(string selectedTest)
        {
            var result = new Edit(FindElement.ById("TestResult"));
            var check = new CheckBox(FindElement.ById("MessageReceived"));
            Button CompleteTestButton = new Button(FindElement.ById("CompleteTest"));
            string expectedValue = selectedTest + ": Passed []";

            Log.Comment("Waiting for result...");
            CompleteTestButton.Invoke();

            // If the web message hasn't been received, wait for it
            using (var waiter = check.GetToggledWaiter())
            {
                if (check.ToggleState != ToggleState.On && !waiter.TryWait())
                {
                    Log.Error("Expected web message was never received.");
                }
            }

            // By now, we'll have gotten a pass/fail result
            Verify.AreEqual(expectedValue, result.Value);
        }

        private static void CompleteTestAndWaitForResult(string selectedTest)
        {
            var result = new Edit(FindElement.ById("TestResult"));
            Button CompleteTestButton = new Button(FindElement.ById("CompleteTest"));
            string expectedValue = selectedTest + ": Passed []";

            Log.Comment("Waiting for result...");
            using (var waiter = new ValueChangedEventWaiter(result, expectedValue))
            {
                CompleteTestButton.Invoke();
                waiter.TryWait();
            }

            Verify.AreEqual(expectedValue, result.Value);
        }

        private static void WaitForTextBoxValue(Edit textbox, string expectedValue, bool match = true)
        {
            // if value is empty, wait for it
            if (textbox.Value == string.Empty)
            {
                using (var waiter = new ValueChangedEventWaiter(textbox, expectedValue))
                {
                    waiter.Wait();
                }
            }

            if (match == false)
            {
                if (textbox.Value == expectedValue)
                {
                    return;
                }
            }
            else
            {
                if (textbox.Value != expectedValue)
                {
                    return;
                }
            }
        }

        private static void DoSelectAllByKeyboard()
        {
            Log.Comment("Do Select All");
            KeyboardHelper.PressDownModifierKey(ModifierKey.Control);
            TextInput.SendText("a");
            KeyboardHelper.ReleaseModifierKey(ModifierKey.Control);
        }

        private static void CopySelected()
        {
            Log.Comment("Do Copy");
            KeyboardHelper.PressDownModifierKey(ModifierKey.Control);
            TextInput.SendText("c");
            KeyboardHelper.ReleaseModifierKey(ModifierKey.Control);
        }

        private static void PasteClipboard()
        {
            Log.Comment("Do Paste");
            KeyboardHelper.PressDownModifierKey(ModifierKey.Control);
            TextInput.SendText("v");
            KeyboardHelper.ReleaseModifierKey(ModifierKey.Control);
        }

        private static void WaitForCopyCompleted()
        {
            string expectedValue = "HTML files ready";
            var status1 = new Edit(FindElement.ById("Status1"));
            WaitForTextBoxValue(status1, expectedValue);
        }

        private static void WaitForLoadCompleted()
        {
            // if load completed then status2 will have name of the webview
            var status2 = new Edit(FindElement.ById("Status2"));
            WaitForTextBoxValue(status2, string.Empty, false /* match = false for not match */);
        }

        private static void WaitForEnsureCompleted()
        {
            var status1 = new Edit(FindElement.ById("Status1"));
            WaitForTextBoxValue(status1, "EnsureCoreWebView2Async() completed", true);
        }

        private static void ChooseTest(string testName, bool waitForLoadCompleted = true)
        {
            WaitForCopyCompleted();

            string comboBoxName = "TestNameComboBox";
            Log.Comment("Retrieve ComboBox item with name '{0}'.", comboBoxName);
            ComboBox comboBox = new ComboBox(FindElement.ById(comboBoxName));
            comboBox.SelectItemByName(testName);
            Wait.ForIdle();

            if (waitForLoadCompleted)
            {
                WaitForLoadCompleted();
            }
        }

        // Replace calls to WaitForFocus with waiters when task 23527231 is completed making WebMessageReceivedHandler available.
        private static void WaitForFocus(string target)
        {
            var anaheimFocusTextBox = new Edit(FindElement.ById("AnaheimFocusTextBox"));
            var expectedFocusMessage = "Focus on: " + target;
            WaitForTextBoxValue(anaheimFocusTextBox, expectedFocusMessage);
            Log.Comment("Focus is on " + UIObject.Focused);
            Verify.AreEqual(anaheimFocusTextBox.Value, expectedFocusMessage);
        }

        // 'target' and 'tabs' should be in accordance with the html page being navigated on.
        private static void SetWebViewElementFocus(string target, uint tabs)
        {
            Button x1 = new Button(FindElement.ById("TabStopButton1")); // Xaml TabStop 1
            x1.SetFocus();
            Wait.ForIdle();
            Verify.IsTrue(x1.HasKeyboardFocus, "TabStopButton1 has keyboard focus");

            KeyboardHelper.PressKey(Key.Tab, ModifierKey.None, tabs);
            WaitForFocus(target);
        }

        [TestMethod]
        [TestProperty("TestSuite", "A")]
        public void BasicRenderingTest()
        {
            using (var setup = new WebView2TestSetupHelper(new[] { "WebView2 Tests", "navigateToBasicWebView2" }))
            {
                ChooseTest("BasicRenderingTest");
                Wait.ForMilliseconds(100); // rendering a webpage takes some time after navigation completed

                // To verify, check for the appropriate pixel(s) using User32 and GDI32 calls here in the test runner.
                WebView2Temporary.WebView2RenderingVerifier.VerifyInstances("BasicRenderingTest"); 
                CompleteTestAndWaitForResult("BasicRenderingTest");
            }
        }

        private static void MouseClickTestCommon(PointerButtons mouseButton)
        {
            using (var setup = new WebView2TestSetupHelper(new[] { "WebView2 Tests", "navigateToBasicWebView2" }))
            {
                string selectedTest = "Invalid";
                if (mouseButton == PointerButtons.Primary)
                {
                    selectedTest = "MouseLeftClickTest";
                }
                else if (mouseButton == PointerButtons.Middle)
                {
                    selectedTest = "MouseMiddleClickTest";
                }
                else if (mouseButton == PointerButtons.Secondary)
                {
                    selectedTest = "MouseRightClickTest";
                }
                else if (mouseButton == PointerButtons.XButton1)
                {
                    selectedTest = "MouseXButton1ClickTest";
                }
                else if (mouseButton == PointerButtons.XButton2)
                {
                    selectedTest = "MouseXButton2ClickTest";
                }
                ChooseTest(selectedTest);

                var webview = FindElement.ById("MyWebView2");
                Rectangle bounds = webview.BoundingRectangle;
                Log.Comment("Bounds = X:{0}, Y:{1}, Width:{2}, Height:{3}", bounds.X, bounds.Y, bounds.Width, bounds.Height);
                // TODO_WebView2Input - Task 30555367 - Stack Overflow in WebView2 pointer tests when (MUXC) test infra calls WV2.GetClickablePoint()
                //                      Workaround to avoid any calls to WebView2.GetClickablePoint
                /*
                var point = webview.GetClickablePoint();
                Log.Comment("ClickablePoint = X:{0}, Y:{1}", point.X, point.Y);
                // Move mouse to top left coordinate (location of web button) relative from the center of webview
                InputHelper.MoveMouse(webview,
                    -bounds.Width/2 + 20, -bounds.Height/2 + 20);
                */
                var point = new Point(bounds.X + 20, bounds.Y + 20);
                Log.Comment("Move mouse to ({0}, {1})", bounds.X + 20, bounds.Y + 20);
                PointerInput.Move(point);

                PointerInput.Press(mouseButton);
                PointerInput.Release(mouseButton);
                Wait.ForIdle();

                WaitForWebMessageResult(selectedTest);
            }
        }

        [TestMethod]
        [TestProperty("TestSuite", "A")]
        public void MouseLeftClickTest()
        {
            MouseClickTestCommon(PointerButtons.Primary);
        }

        [TestMethod]
        [TestProperty("TestSuite", "A")]
        public void MouseMiddleClickTest()
        {
            MouseClickTestCommon(PointerButtons.Middle);
        }

        [TestMethod]
        [TestProperty("TestSuite", "A")]
        public void MouseRightClickTest()
        {
            MouseClickTestCommon(PointerButtons.Secondary);
        }

        [TestMethod]
        [TestProperty("TestSuite", "A")]
        public void MouseXButton1ClickTest()
        {
            MouseClickTestCommon(PointerButtons.XButton1);
        }

        [TestMethod]
        [TestProperty("TestSuite", "A")]
        public void MouseXButton2ClickTest()
        {
            MouseClickTestCommon(PointerButtons.XButton2);
        }

        [TestMethod]
        [TestProperty("TestSuite", "A")]
        public void MouseWheelScrollTest()
        {
            using (var setup = new WebView2TestSetupHelper(new[] { "WebView2 Tests", "navigateToBasicWebView2" }))
            {
                ChooseTest("MouseWheelScrollTest");

                var webview = FindElement.ById("MyWebView2");
                // Mouse wheel delta amount required per initial velocity unit
                const int mouseWheelDeltaForVelocityUnit = -4000;  // scroll downwards
                // TODO_WebView2Input - Task 30555367 - Stack Overflow in WebView2 pointer tests when (MUXC) test infra calls WV2.GetClickablePoint()
                //                      Workaround to avoid any calls to WebView2.GetClickablePoint
                /*
                InputHelper.RotateWheel(webview, mouseWheelDeltaForVelocityUnit);
                */
                Rectangle bounds = webview.BoundingRectangle;
                var point = new Point(bounds.X + bounds.Width/2, bounds.Y + bounds.Height/2);
                PointerInput.Move(point);
                MouseWheelInput.RotateWheel(mouseWheelDeltaForVelocityUnit);

                WaitForWebMessageResult("MouseWheelScrollTest");
            }
        }

        [TestMethod]
        [TestProperty("TestSuite", "A")]
        public void CursorUpdateTest()
        {
            using (var setup = new WebView2TestSetupHelper(new[] { "WebView2 Tests", "navigateToBasicWebView2" }))
            {
                // On ChooseTest, replace the default webview with one that exposes the ProtectedCursor member
                ChooseTest("CursorUpdateTest");

                // Clear the cache so we can find the new webview
                ElementCache.Clear();
                var webview = FindElement.ById("MyWebView2");
                Rectangle bounds = webview.BoundingRectangle;
                Log.Comment("Bounds = X:{0}, Y:{1}, Width:{2}, Height:{3}", bounds.X, bounds.Y, bounds.Width, bounds.Height);

                // Top left coordinate (location of web button) relative from the center of webview
                // TODO_WebView2Input - Task 30555367 - Stack Overflow in WebView2 pointer tests when (MUXC) test infra calls WV2.GetClickablePoint()
                //                      Workaround to avoid any calls to WebView2.GetClickablePoint
                /*
                InputHelper.MoveMouse(webview, -bounds.Width / 2 + 20, -bounds.Height / 2 + 20);
                */
                var point = new Point(bounds.X + 20, bounds.Y + 20);
                PointerInput.Move(point);
                Wait.ForIdle();

                // Checks that the ProtectedCursor type has changed to the one we expect
                CompleteTestAndWaitForResult("CursorUpdateTest");
            }
        }

        [TestMethod]
        [TestProperty("TestSuite", "A")]
        public void CursorClickUpdateTest()
        {
            using (var setup = new WebView2TestSetupHelper(new[] { "WebView2 Tests", "navigateToBasicWebView2" }))
            {
                // On ChooseTest, replace the default webview with one that exposes the ProtectedCursor member
                ChooseTest("CursorClickUpdateTest");

                // Clear the cache so we can find the new webview
                ElementCache.Clear();
                var webview = FindElement.ById("MyWebView2");
                Rectangle bounds = webview.BoundingRectangle;
                Log.Comment("Bounds = X:{0}, Y:{1}, Width:{2}, Height:{3}", bounds.X, bounds.Y, bounds.Width, bounds.Height);

                // Click somewhere in the webview that's not the button a few times
                var centerOfWebView = new Point(bounds.X + bounds.Width / 2, bounds.Y + bounds.Height / 2);
                PointerInput.Move(centerOfWebView);

                PointerInput.Press(PointerButtons.Primary);
                PointerInput.Release(PointerButtons.Primary);

                PointerInput.Press(PointerButtons.Primary);
                PointerInput.Release(PointerButtons.Primary);

                PointerInput.Press(PointerButtons.Primary);
                PointerInput.Release(PointerButtons.Primary);

                var insideButton = new Point(bounds.X + 20, bounds.Y + 20);
                PointerInput.Move(insideButton);
                Wait.ForIdle();

                // Checks that the ProtectedCursor type has changed to the one we expect
                CompleteTestAndWaitForResult("CursorClickUpdateTest");
            }
        }

        [TestMethod]
        [TestProperty("TestSuite", "A")]
        public void NavigationErrorTest()
        {
            using (var setup = new WebView2TestSetupHelper(new[] { "WebView2 Tests", "navigateToBasicWebView2" }))
            {
                ChooseTest("NavigationErrorTest");
                CompleteTestAndWaitForResult("NavigationErrorTest");
            }
        }

        // For each of the Focus_* tests: 
        // 1) We Tab/Shift+Tab/Click amongst xaml controls(x1, x2) and web controls(w1, w2). 
        // 2) After each such change:
        //     a. Validate Edge focus state by waiting for web message sent when an element gets focus
        //     b. Validate Xaml focus using MITA APIs directly
        [TestMethod]
        [TestProperty("TestSuite", "A")]
        public void Focus_BasicTabTest()
        {
            using (var setup = new WebView2TestSetupHelper(new[] { "WebView2 Tests", "navigateToBasicWebView2" }))
            {
                ChooseTest("Focus_BasicTabTest");

                Button x1 = new Button(FindElement.ById("TabStopButton1"));     // Xaml TabStop 1
                Button x2 = new Button(FindElement.ById("TabStopButton2"));     // Xaml TabStop 2

                Log.Comment("Description: x1 -> w1 -> w2 -> x2 ");
                Log.Comment("Focus on x1");
                x1.SetFocus();
                Wait.ForIdle();
                Verify.IsTrue(x1.HasKeyboardFocus, "TabStopButton1 has keyboard focus");

                Log.Comment("Tab x1 -> w1");
                KeyboardHelper.PressKey(Key.Tab);
                WaitForFocus("w1");

                Log.Comment("Tab w1 -> w2");
                KeyboardHelper.PressKey(Key.Tab);
                WaitForFocus("w2");

                Log.Comment("Tab w2 -> x2");
                using (var xamlFocusWaiter = new FocusAcquiredWaiter("TabStopButton2"))
                {
                    KeyboardHelper.PressKey(Key.Tab);
                    xamlFocusWaiter.Wait();
                    Log.Comment("Focus is on " + UIObject.Focused);
                    Verify.IsTrue(x2.HasKeyboardFocus, "TabStopButton2 has keyboard focus");
                }

                CompleteTestAndWaitForResult("Focus_BasicTabTest");
            }
        }

        // See comment on Focus_BasicTabTest() for details on test mechanism.
        [TestMethod]
        [TestProperty("TestSuite", "A")]
        public void Focus_ReverseTabTest()
        {
            using (var setup = new WebView2TestSetupHelper(new[] { "WebView2 Tests", "navigateToBasicWebView2" }))
            {
                ChooseTest("Focus_ReverseTabTest");

                Button x1 = new Button(FindElement.ById("TabStopButton1"));     // Xaml TabStop 1
                Button x2 = new Button(FindElement.ById("TabStopButton2"));     // Xaml TabStop 2

                Log.Comment("Description: x2 shift-> w2 shift-> w1 shift-> x1");
                Log.Comment("Focus on x2");
                x2.SetFocus();
                Wait.ForIdle();
                Verify.IsTrue(x2.HasKeyboardFocus, "TabStopButton2 has keyboard focus");

                Log.Comment("Shift+Tab x2 -> w2");
                KeyboardHelper.PressKey(Key.Tab, ModifierKey.Shift);
                WaitForFocus("w2");

                Log.Comment("Shift+Tab w2 -> w1");
                KeyboardHelper.PressKey(Key.Tab, ModifierKey.Shift);
                WaitForFocus("w1");

                Log.Comment("Shift+Tab w1 -> x1");
                using (var xamlFocusWaiter = new FocusAcquiredWaiter("TabStopButton1"))
                {

                    KeyboardHelper.PressKey(Key.Tab, ModifierKey.Shift);
                    xamlFocusWaiter.Wait();
                    Log.Comment("Focus is on " + UIObject.Focused);
                    Verify.IsTrue(x1.HasKeyboardFocus, "TabStopButton1 has keyboard focus");
                }

                CompleteTestAndWaitForResult("Focus_ReverseTabTest");
            }
        }

        // See comment on Focus_BasicTabTest() for details on test mechanism.
        [TestMethod]
        [TestProperty("TestSuite", "A")]
        public void Focus_BackAndForthTabTest()
        {
            using (var setup = new WebView2TestSetupHelper(new[] { "WebView2 Tests", "navigateToBasicWebView2" }))
            {
                ChooseTest("Focus_BackAndForthTabTest");

                Button x1 = new Button(FindElement.ById("TabStopButton1"));     // Xaml TabStop 1

                Log.Comment("Description: x1 -> w1 shift-> x1 -> w1 -> x1");
                Log.Comment("Focus on x1");
                x1.SetFocus();
                Wait.ForIdle();
                Verify.IsTrue(x1.HasKeyboardFocus, "TabStopButton1 has keyboard focus");

                Log.Comment("Tab x1 -> w1");
                KeyboardHelper.PressKey(Key.Tab);
                WaitForFocus("w1");

                Log.Comment("Shift+Tab w1 -> x1");
                using (var xamlFocusWaiter = new FocusAcquiredWaiter("TabStopButton1"))
                {
                    KeyboardHelper.PressKey(Key.Tab, ModifierKey.Shift);
                    xamlFocusWaiter.Wait();
                    Log.Comment("Focus is on " + UIObject.Focused);
                    Verify.IsTrue(x1.HasKeyboardFocus, "TabStopButton1 has keyboard focus");
                }

                Log.Comment("Tab x1 -> w1");
                KeyboardHelper.PressKey(Key.Tab);
                WaitForFocus("w1");

                Log.Comment("Shift+Tab w1 -> x1");
                using (var xamlFocusWaiter = new FocusAcquiredWaiter("TabStopButton1"))
                {
                    KeyboardHelper.PressKey(Key.Tab, ModifierKey.Shift);
                    xamlFocusWaiter.Wait();
                    Log.Comment("Focus is on " + UIObject.Focused);
                    Verify.IsTrue(x1.HasKeyboardFocus, "TabStopButton1 has keyboard focus");
                }

                CompleteTestAndWaitForResult("Focus_BackAndForthTabTest");
            }
        }

        // See comment on Focus_BasicTabTest() for details on test mechanism.
        [TestMethod]
        [TestProperty("TestSuite", "A")]
        public void Focus_MouseActivateTest()
        {
            using (var setup = new WebView2TestSetupHelper(new[] { "WebView2 Tests", "navigateToBasicWebView2" }))
            {
                ChooseTest("Focus_MouseActivateTest");

                Button x1 = new Button(FindElement.ById("TabStopButton1"));     // Xaml TabStop 1

                Log.Comment("Test1: Focus on x1, Click on w1, Shift+Tab w2 -> w1, Click on x2");
                Log.Comment("Focus on x1");
                x1.SetFocus();
                Wait.ForIdle();
                Verify.IsTrue(x1.HasKeyboardFocus, "TabStopButton1 has keyboard focus");

                Log.Comment("Click on w2");

                Rectangle bounds = FindElement.ById("MyWebView2").BoundingRectangle;
                const int w2_offsetX = 100;
                const int w2_offsetY = 200;

                // top left coordinate (location of web button) relative from the center of webview
                InputHelper.MoveMouse(FindElement.ById("MyWebView2"), -bounds.Width / 2 + w2_offsetX, -bounds.Height / 2 + w2_offsetY);
                PointerInput.Press(PointerButtons.Primary);
                PointerInput.Release(PointerButtons.Primary);

                WaitForFocus("w2");

                Log.Comment("Shift+Tab w2 -> w1");
                KeyboardHelper.PressKey(Key.Tab, ModifierKey.Shift);
                WaitForFocus("w1");

                Log.Comment("Click on x1");
                using (var xamlFocusWaiter = new FocusAcquiredWaiter("TabStopButton1"))
                {
                    InputHelper.MoveMouse(x1, 10, 10);
                    PointerInput.Press(PointerButtons.Primary);
                    PointerInput.Release(PointerButtons.Primary);
                    xamlFocusWaiter.Wait();
                    Verify.IsTrue(x1.HasKeyboardFocus, "TabStopButton1 has keyboard focus");
                }

                CompleteTestAndWaitForResult("Focus_MouseActivateTest");
            }
        }

        [TestMethod]
        [TestProperty("TestSuite", "A")]
        public void ExecuteScriptTest()
        {
            using (var setup = new WebView2TestSetupHelper(new[] { "WebView2 Tests", "navigateToBasicWebView2" }))
            {
                ChooseTest("ExecuteScriptTest");
                CompleteTestAndWaitForResult("ExecuteScriptTest");
            }
        }

        [TestMethod]
        [TestProperty("TestSuite", "A")]
        public void MultipleWebviews_FocusTest()
        {
            using (var setup = new WebView2TestSetupHelper(new[] { "WebView2 Tests", "navigateToBasicWebView2" }))
            {
                ChooseTest("MultipleWebviews_FocusTest");

                Button x1 = new Button(FindElement.ById("TabStopButton1"));     // Xaml TabStop 1
                Button x2 = new Button(FindElement.ById("TabStopButton2"));     // Xaml TabStop 2
                UIObject webviewControl = new UIObject(FindElement.ById("MyWebView2"));
                UIObject webviewControlB = new UIObject(FindElement.ById("MyWebView2B"));

                Log.Comment("Description: first tabstop1 -> webview1 -> second tabstop2 -> WebView2");
                Log.Comment("Focus on tabstop1");
                x1.SetFocus();
                Wait.ForIdle();
                Verify.IsTrue(x1.HasKeyboardFocus, "TabStopButton1 has keyboard focus");

                Log.Comment("Tab tabstop1 -> MyWebView2");
                KeyboardHelper.PressKey(Key.Tab);
                WaitForFocus("w1");

                Log.Comment("Tab to move to next element within first webview page");
                KeyboardHelper.PressKey(Key.Tab);

                Log.Comment("Tab MyWebView2 -> tabstop2");
                using (var xamlFocusWaiter = new FocusAcquiredWaiter("TabStopButton2"))
                {
                    KeyboardHelper.PressKey(Key.Tab);
                    xamlFocusWaiter.Wait();
                    Verify.IsTrue(x2.HasKeyboardFocus, "TabStopButton2 has keyboard focus");
                }

                Log.Comment("Tab tabstop2 -> MyWebView2B");
                KeyboardHelper.PressKey(Key.Tab);
                WaitForFocus("w1B");

                using (var xamlFocusWaiter = new FocusAcquiredWaiter("TabStopButton2"))
                {
                    KeyboardHelper.PressKey(Key.Tab, ModifierKey.Shift);
                    xamlFocusWaiter.Wait();
                    Verify.IsTrue(x2.HasKeyboardFocus, "TabStopButton2 has keyboard focus");
                }

                CompleteTestAndWaitForResult("MultipleWebviews_FocusTest");
            }
        }

        [TestMethod]
        [TestProperty("TestSuite", "B")]
        public void MultipleWebviews_BasicRenderingTest()
        {
            using (var setup = new WebView2TestSetupHelper(new[] { "WebView2 Tests", "navigateToBasicWebView2" }))
            {
                ChooseTest("MultipleWebviews_BasicRenderingTest");
                CompleteTestAndWaitForResult("MultipleWebviews_BasicRenderingTest");
                Wait.ForIdle();
                ElementCache.Clear();
                WebView2Temporary.WebView2RenderingVerifier.VerifyInstances("MultipleWebviews_BasicRenderingTest");
            }
        }

        [TestMethod]
        [TestProperty("TestSuite", "B")]
        [TestProperty("Ignore", "True")] // TODO: Investigate why LanguageTest is failing on latest WebView2 runtime
        public void MultipleWebviews_LanguageTest()
        {
            using (var setup = new WebView2TestSetupHelper(new[] { "WebView2 Tests", "navigateToBasicWebView2" }))
            {
                ChooseTest("MultipleWebviews_LanguageTest");
                CompleteTestAndWaitForResult("MultipleWebviews_LanguageTest");
            }
        }

        // CopyPasteTest tests basic copy and pasting scenarios involving the WebView2 control as follows:
        // 1) Copying from text housed in a XAML control in the app to the webview's textbox.
        // 2) [Xaml text -> WebView2] After overwriting the clipboard's DataPackage, verify successful 'copy to webview'
        //    by copying from the textbox and pasting to an external Xaml textbox for verification.
        // 3) [WebView2 text -> Xaml] Overwrite webview textbox's contents and attempt to copy this out to 
        //    the Xaml app to provide an isolated 'copy from webview' test.
        [TestMethod]
        [TestProperty("TestSuite", "B")]
        [TestProperty("Ignore", "True")] // Task 37000273
        public void CopyPasteTest()
        {
            using (var setup = new WebView2TestSetupHelper(new[] { "WebView2 Tests", "navigateToBasicWebView2" }))
            {
                ChooseTest("CopyPasteTest");

                var CopyPasteTextBox1 = new Edit(FindElement.ById("CopyPasteTextBox1"));
                var CopyPasteTextBox2 = new Edit(FindElement.ById("CopyPasteTextBox2"));

                // Copy text to SimpleInputPage's text box page.
                Log.Comment("Copy and paste text to SimpleInputPage's text box page...");
                CopyPasteTextBox1.SetFocus();
                DoSelectAllByKeyboard();
                CopySelected();
                SetWebViewElementFocus("w2", 2);
                // Do ctrl+V to paste into html.
                PasteClipboard();

                using (var clearClipboardWaiter = new ValueChangedEventWaiter(CopyPasteTextBox2, "Clear"))
                {
                    // "Clear" clipboard by copying text between TextBoxes on Xaml page.
                    Log.Comment("Clear clipboard by copying text between TextBoxes on Xaml page...");
                    CopyPasteTextBox1.SetValue("Clear");
                    CopyPasteTextBox1.SetFocus();
                    Wait.ForIdle();
                    Verify.IsTrue(CopyPasteTextBox1.HasKeyboardFocus);
                    DoSelectAllByKeyboard();
                    CopySelected();
                    CopyPasteTextBox2.SetFocus();
                    Wait.ForIdle();
                    Verify.IsTrue(CopyPasteTextBox2.HasKeyboardFocus);
                    DoSelectAllByKeyboard();
                    PasteClipboard();
                    clearClipboardWaiter.Wait();
                }

                // Selecting html content from step 1 and copying out of webview for verification.
                SetWebViewElementFocus("w2", 2);
                using (var pasteTestWaiter = new ValueChangedEventWaiter(CopyPasteTextBox2, "PasteTest"))
                {
                    Log.Comment("Selecting html content from step 1 and copying out of webview for verification...");
                    DoSelectAllByKeyboard();
                    CopySelected();
                    CopyPasteTextBox2.SetFocus();
                    DoSelectAllByKeyboard();
                    PasteClipboard();
                    pasteTestWaiter.Wait();
                }

                // Inject new text into html textbox and copy out to verify copy from webview.
                SetWebViewElementFocus("w2", 2);
                using (var copyTestWaiter = new ValueChangedEventWaiter(CopyPasteTextBox1, "CopyTest"))
                {
                    Log.Comment("Inject new text into html textbox and copy out to verify copy from webview...");
                    TextInput.SendText("CopyTest");
                    DoSelectAllByKeyboard();
                    CopySelected();
                    CopyPasteTextBox1.SetFocus();
                    DoSelectAllByKeyboard();
                    PasteClipboard();
                    copyTestWaiter.Wait();
                }

                CompleteTestAndWaitForResult("CopyPasteTest");
            }
        }

        [TestMethod]
        [TestProperty("TestSuite", "B")]
        public void BasicKeyboardTest()
        {
            using (var setup = new WebView2TestSetupHelper(new[] { "WebView2 Tests", "navigateToBasicWebView2" }))
            {
                ChooseTest("BasicKeyboardTest");

                var CopyPasteTextBox2 = new Edit(FindElement.ById("CopyPasteTextBox2"));
                // Result should be "Hello 123 World" via:
                // Write Hello Wor after navigating to textbox in webview.
                Button x1 = new Button(FindElement.ById("TabStopButton1")); // Xaml TabStop 1
                x1.SetFocus();
                Wait.ForIdle();
                Verify.IsTrue(x1.HasKeyboardFocus, "TabStopButton1 has keyboard focus");

                Log.Comment("Tab to w1...");
                KeyboardHelper.PressKey(Key.Tab);
                WaitForFocus("w1");

                Log.Comment("Inject 'Hello Wor'...");
                TextInput.SendText("Hello Wor");

                Log.Comment("Inject left arrow three times...");
                KeyboardHelper.PressKey(Key.Left);
                KeyboardHelper.PressKey(Key.Left);
                KeyboardHelper.PressKey(Key.Left);

                Log.Comment("Inject '123 '...");
                TextInput.SendText("123 ");

                Log.Comment("Inject right arrow three times...");
                KeyboardHelper.PressKey(Key.Right);
                KeyboardHelper.PressKey(Key.Right);
                KeyboardHelper.PressKey(Key.Right);

                Log.Comment("Inject 'ld'...");
                TextInput.SendText("ld");

                Log.Comment("Test simultaneous keyboard inputs by injecting shift+left twice...");
                KeyboardHelper.PressKey(Key.Left, ModifierKey.Shift);
                KeyboardHelper.PressKey(Key.Left, ModifierKey.Shift);
                TextInput.SendText("m");

                // Copy out to PasteBox1 for verification.
                DoSelectAllByKeyboard();
                CopySelected();
                CopyPasteTextBox2.SetFocus();
                PasteClipboard();

                string expectedText = "Hello 123 Worm";
                string textResult = CopyPasteTextBox2.GetText();
                Verify.IsTrue(textResult == expectedText,
                              string.Format("Test {0}: Expected text {1} did not match with sampled text {2}.",
                                            "BasicKeyboardTest", expectedText, textResult));

                CompleteTestAndWaitForResult("BasicKeyboardTest");
            }
        }

        [TestMethod]
        [TestProperty("TestSuite", "B")]
        public void WebMessageReceivedTest()
        {
            using (var setup = new WebView2TestSetupHelper(new[] { "WebView2 Tests", "navigateToBasicWebView2" }))
            {
                ChooseTest("WebMessageReceivedTest");

                // Tab to button
                Button x1 = new Button(FindElement.ById("TabStopButton1")); // Xaml TabStop 1
                x1.SetFocus();
                Wait.ForIdle();
                Verify.IsTrue(x1.HasKeyboardFocus, "TabStopButton1 has keyboard focus");
                KeyboardHelper.PressKey(Key.Tab);
                KeyboardHelper.PressKey(Key.Tab);
                WaitForFocus("b1");

                // Trigger WebMessage from button press.
                KeyboardHelper.PressKey(Key.Enter);

                WaitForWebMessageResult("WebMessageReceivedTest");
            }
        }

        [TestMethod]
        [TestProperty("TestSuite", "B")]
        public void MouseCaptureTest()
        {
            using (var setup = new WebView2TestSetupHelper(new[] { "WebView2 Tests", "navigateToBasicWebView2" }))
            {
                ChooseTest("MouseCaptureTest");

                var CopyPasteTextBox2 = new Edit(FindElement.ById("CopyPasteTextBox2"));
                Rectangle bounds = FindElement.ById("MyWebView2").BoundingRectangle;
                // Offsets map pointer to the start of the textbox
                const int w2_offsetX = 30;
                const int w2_offsetY = 60;

                // Inject text into text box.
                Button x1 = new Button(FindElement.ById("TabStopButton1")); // Xaml TabStop 1
                x1.SetFocus();
                Wait.ForIdle();
                Verify.IsTrue(x1.HasKeyboardFocus, "TabStopButton1 has keyboard focus");
                KeyboardHelper.PressKey(Key.Tab);
                WaitForFocus("w1");

                TextInput.SendText("MouseCaptureResult");

                // Select text with mouse left-click, drag to outside of wv2, then release
                var startPoint = new Point(bounds.X + w2_offsetX, bounds.Y + w2_offsetY);
                var outsidePoint = new Point(bounds.X + w2_offsetX + bounds.Width, bounds.Y + w2_offsetY);

                PointerInput.Move(startPoint);

                // Click once in edit box - this helps selection work in MITA, but is not necessary in manual testing
                PointerInput.Press(PointerButtons.Primary);
                PointerInput.Release(PointerButtons.Primary);

                PointerInput.Press(PointerButtons.Primary);
                PointerInput.Move(outsidePoint);
                PointerInput.Release(PointerButtons.Primary);

                // Move mouse back across text
                // If the WebView did not correctly handle captured mouse input then the text will be deselected
                // due to the WebView still thinking that the mouse's left-button is pressed.
                PointerInput.Move(startPoint);

                using (var pasteTestWaiter = new ValueChangedEventWaiter(CopyPasteTextBox2, "MouseCaptureResult"))
                {
                    CopySelected();
                    CopyPasteTextBox2.SetFocus();
                    DoSelectAllByKeyboard();
                    PasteClipboard();
                    pasteTestWaiter.Wait();
                }

                CompleteTestAndWaitForResult("MouseCaptureTest");
            }
        }

        [TestMethod]
        [TestProperty("TestSuite", "B")]
        public void ReloadTest()
        {
            using (var setup = new WebView2TestSetupHelper(new[] { "WebView2 Tests", "navigateToBasicWebView2" }))
            {
                ChooseTest("ReloadTest");
                CompleteTestAndWaitForResult("ReloadTest");
            }
        }

        [TestMethod]
        [TestProperty("TestSuite", "B")]
        public void NavigateToStringTest()
        {
            using (var setup = new WebView2TestSetupHelper(new[] { "WebView2 Tests", "navigateToBasicWebView2" }))
            {
                ChooseTest("NavigateToStringTest");
                CompleteTestAndWaitForResult("NavigateToStringTest");
            }
        }

        // A textblock is bound to the webview's Source property with TwoWay binding and is 
        // validated as follows:
        // Initial page is loaded, the uri is copied from the textblock and modified in a textbox which on 
        // Enter-pressed updates the textBlock/Source value. This should cause the webview to load the 
        // new value in the textBlock, demonstrating binding from textBlock text->Source. The success of this is 
        // validated by invoking the 'navigateToPageWithText' script on the currently loaded page, 
        // if 'SimplePage.html' is not loaded this will fail. If 'navigateToPageWithText' succeeds,
        // the webview should load 'SimplePageWithText.html' and the uri should be reflected in the
        // bound textbox, demonstrating binding from Source->textBlock text.
        [TestMethod]
        [TestProperty("TestSuite", "B")]
        public void SourceBindingTest()
        {
            using (var setup = new WebView2TestSetupHelper(new[] { "WebView2 Tests", "navigateToBasicWebView2" }))
            {
                ChooseTest("SourceBindingTest");

                var uriTextBox = new Edit(FindElement.ById("UriTextBox"));
                var actualUriTextBlock = new Edit(FindElement.ById("ActualUriTextBlock"));

                actualUriTextBlock.SetFocus();
                DoSelectAllByKeyboard();
                CopySelected();
                uriTextBox.SetFocus();
                DoSelectAllByKeyboard();
                PasteClipboard();

                // Modify local resource Uri to load SimplePage.html
                KeyboardHelper.PressDownModifierKey(ModifierKey.Control);
                KeyboardHelper.PressDownModifierKey(ModifierKey.Shift);
                for (int tab = 0; tab < 3; tab++)
                {
                    KeyboardHelper.PressKey(Key.Left);
                }
                KeyboardHelper.ReleaseModifierKey(ModifierKey.Shift);
                KeyboardHelper.ReleaseModifierKey(ModifierKey.Control);
                TextInput.SendText("SimplePage.html");

                KeyboardHelper.PressKey(Key.Enter);

                CompleteTestAndWaitForResult("SourceBindingTest");
            }
        }

        // CanGoBack and CanGoForward properties and used in implementation of methods, tested by proxy.
        [TestMethod]
        [TestProperty("TestSuite", "B")]
        public void GoBackAndForwardTest()
        {
            using (var setup = new WebView2TestSetupHelper(new[] { "WebView2 Tests", "navigateToBasicWebView2" }))
            {
                ChooseTest("GoBackAndForwardTest");
                CompleteTestAndWaitForResult("GoBackAndForwardTest");
            }
        }

        [TestMethod]
        [TestProperty("TestSuite", "B")]
        public void NavigationStartingTest()
        {
            using (var setup = new WebView2TestSetupHelper(new[] { "WebView2 Tests", "navigateToBasicWebView2" }))
            {
                ChooseTest("NavigationStartingTest");
                CompleteTestAndWaitForResult("NavigationStartingTest");
            }
        }

        [TestMethod]
        [TestProperty("TestSuite", "B")]
        public void NavigationStartingInvalidTest()
        {
            using (var setup = new WebView2TestSetupHelper(new[] { "WebView2 Tests", "navigateToBasicWebView2" }))
            {
                ChooseTest("NavigationStartingInvalidTest");
                CompleteTestAndWaitForResult("NavigationStartingInvalidTest");
            }
        }

        [TestMethod]
        [TestProperty("TestSuite", "B")]
        public void ResizeTest()
        {
            using (var setup = new WebView2TestSetupHelper(new[] { "WebView2 Tests", "navigateToBasicWebView2" }))
            {
                ChooseTest("ResizeTest");
                CompleteTestAndWaitForWebMessageResult("ResizeTest");
            }
        }

        [TestMethod]
        [TestProperty("Ignore", "True")] // Task 31708332: WebView2 Touch Tests still failing on Helix
        [TestProperty("TestSuite", "B")]
        public void BasicTapTouchTest()
        {
            if (!PlatformConfiguration.IsOsVersionGreaterThanOrEqual(OSVersion.Redstone5))
            {
                Log.Warning("This test requires RS5+ functionality");
                return;
            }

            using (var setup = new WebView2TestSetupHelper(new[] { "WebView2 Tests", "navigateToBasicWebView2" }))
            {
                ChooseTest("BasicTapTouchTest");

                var webview = FindElement.ById("MyWebView2");
                // TODO_WebView2Input - Task 30555367 - Stack Overflow in WebView2 pointer tests when (MUXC) test infra calls WV2.GetClickablePoint()
                //                      Workaround to avoid any calls to WebView2.GetClickablePoint
                /*
                InputHelper.Tap(webview);
                */
                Rectangle bounds = webview.BoundingRectangle;
                webview.Tap(bounds.Width / 2, bounds.Height / 2);
                WaitForWebMessageResult("BasicTapTouchTest");
            }
        }

        [TestMethod]
        [TestProperty("Ignore", "True")] // Task 31708332: WebView2 Touch Tests still failing on Helix
        [TestProperty("TestSuite", "B")]
        public void BasicFlingTouchTest()
        {
            if (!PlatformConfiguration.IsOsVersionGreaterThanOrEqual(OSVersion.Redstone5))
            {
                Log.Warning("This test requires RS5+ functionality");
                return;
            }
            using (var setup = new WebView2TestSetupHelper(new[] { "WebView2 Tests", "navigateToBasicWebView2" }))
            {
                ChooseTest("BasicFlingTouchTest");

                var webview = FindElement.ById("MyWebView2");

                // Use tap to initialize status text box so that subsequent waiter isn't triggered by flick's initial tap.
                // TODO_WebView2Input - Task 30555367 - Stack Overflow in WebView2 pointer tests when (MUXC) test infra calls WV2.GetClickablePoint()
                //                      Workaround to avoid any calls to WebView2.GetClickablePoint
                /*
                InputHelper.Tap(webview);
                */
                Rectangle bounds = webview.BoundingRectangle;
                webview.Tap(bounds.Width / 2, bounds.Height / 2);

                // Basic Flick Injection to the West (causing scroll to the East) to the end of the page.
                /*
                InputHelper.Flick(webview, 250, Direction.West);
                */
                Point center = new Point(bounds.Width / 2, bounds.Height / 2);
                InputHelper.Flick(webview, 300, Direction.West);
                WaitForWebMessageResult("BasicFlingTouchTest");
            }
        }

        [TestMethod]
        [TestProperty("Ignore", "True")] // Task 25510116: WebView2Tests.BasicStretchTouchTest is failing on Helix 
        [TestProperty("TestSuite", "B")]
        public void BasicStretchTouchTest()
        {
            if (!PlatformConfiguration.IsOsVersionGreaterThanOrEqual(OSVersion.Redstone5))
            {
                Log.Warning("This test requires RS5+ functionality");
                return;
            }
            using (var setup = new WebView2TestSetupHelper(new[] { "WebView2 Tests", "navigateToBasicWebView2" }))
            {
                ChooseTest("BasicStretchTouchTest");

                var webview = FindElement.ById("MyWebView2");

                // Stretch webview by 200 pixels with one finger stationary.
                InputHelper.Stretch(webview, 200, Direction.East, true);
                Log.Comment("Waiting for webview stretch completion");
                InputHelper.Tap(webview);

                WaitForWebMessageResult("BasicStretchTouchTest");
            }
        }

        [TestMethod]
        [TestProperty("TestSuite", "C")]
        public void BasicPanTouchTest()
        {
            if (!PlatformConfiguration.IsOsVersionGreaterThanOrEqual(OSVersion.Redstone5))
            {
                Log.Warning("This test requires RS5+ functionality");
                return;
            }
            using (var setup = new WebView2TestSetupHelper(new[] { "WebView2 Tests", "navigateToBasicWebView2" }))
            {
                ChooseTest("BasicPanTouchTest");

                var webview = FindElement.ById("MyWebView2");
                // TODO_WebView2Input - Task 30555367 - Stack Overflow in WebView2 pointer tests when (MUXC) test infra calls WV2.GetClickablePoint()
                //                      Workaround to avoid any calls to WebView2.GetClickablePoint
                /*
                InputHelper.Pan(webview, 250, Direction.West);
                */
                Rectangle bounds = webview.BoundingRectangle;
                Point center = new Point(bounds.Width / 2 + bounds.X, bounds.Height / 2 + bounds.Y);
                InputHelper.Pan(webview, center, 250, Direction.West);

                WaitForWebMessageResult("BasicPanTouchTest");
            }
        }

        [TestMethod]
        [TestProperty("TestSuite", "C")]
        [TestProperty("Ignore", "True")] // Task 31708332: WebView2 Touch Tests still failing on Helix
        public void BasicLongPressTouchTest()
        {
            if (!PlatformConfiguration.IsOsVersionGreaterThanOrEqual(OSVersion.Redstone5))
            {
                Log.Warning("This test requires RS5+ functionality");
                return;
            }
            using (var setup = new WebView2TestSetupHelper(new[] { "WebView2 Tests", "navigateToBasicWebView2" }))
            {
                ChooseTest("BasicLongPressTouchTest");

                var webview = FindElement.ById("MyWebView2");
                // Long Press for 2 seconds, triggering commandView.
                // TODO_WebView2Input - Task 30555367 - Stack Overflow in WebView2 pointer tests when (MUXC) test infra calls WV2.GetClickablePoint()
                //                      Workaround to avoid any calls to WebView2.GetClickablePoint
                /*
                InputHelper.TapAndHold(webview, 2000);
                */
                Rectangle bounds = webview.BoundingRectangle;
                InputHelper.TapAndHold(webview, bounds.Width/2, bounds.Height/2, 2000);

                WaitForWebMessageResult("BasicLongPressTouchTest");
            }
        }

        [TestMethod]
        [TestProperty("TestSuite", "C")]
        public void ScaledTouchTest()
        {
            using (IDisposable page1 = new WebView2TestSetupHelper(new[] { "WebView2 Tests", "navigateToBasicWebView2" }))
            {
                ChooseTest("ScaledTouchTest");

                var webview = FindElement.ById("MyWebView2");
                // Get the bounds of the webview without any scaling
                Rectangle preScaleBounds = webview.BoundingRectangle;

                // Scale the webview up by 1.2 in both directions and get the new bounds
                CompleteTestAndWaitForResult("ScaledTouchTest");
                Rectangle postScaleBounds = webview.BoundingRectangle;
                int halfDelta = (postScaleBounds.Width - preScaleBounds.Width) / 2;

                // TODO_WebView2Input - Task 30555367 - Stack Overflow in WebView2 pointer tests when (MUXC) test infra calls WV2.GetClickablePoint()
                //                      Workaround to avoid any calls to WebView2.GetClickablePoint
                /*
                // Test tap injection outside of bounds of unscaled button.
                InputHelper.Tap(webview, preScaleBounds.Width + halfDelta, preScaleBounds.Height / 2);
                */
                webview.Tap(preScaleBounds.Width + halfDelta, preScaleBounds.Height / 2);

                WaitForWebMessageResult("ScaledTouchTest");
            }
        }

        [TestMethod]
        [TestProperty("TestSuite", "C")]
        public void MoveTest()
        {
            using (var setup = new WebView2TestSetupHelper(new[] { "WebView2 Tests", "navigateToBasicWebView2" }))
            {
                ChooseTest("MoveTest");
                CompleteTestAndWaitForResult("MoveTest");
            }
        }

        // Bug 44117609: WinUI2: WebView2Tests.ReparentElementTest is unreliable and has been disabled
        // [TestMethod]
        // [TestProperty("TestSuite", "C")]
        public void ReparentElementTest()
        {
            using (var setup = new WebView2TestSetupHelper(new[] { "WebView2 Tests", "navigateToBasicWebView2" }))
            {
                ChooseTest("ReparentElementTest");
                Wait.ForMilliseconds(100); // The rendering verifier needs extra time for the webview to finish rendering

                // TODO_WebView2: Check that the webview is not visible when it is out of the tree.

                CompleteTestAndWaitForResult("ReparentElementTest");

                // Ensure that the moved WebView is still visible
                WebView2Temporary.WebView2RenderingVerifier.VerifyInstances("ReparentElementTest");
            }
        }

        [TestMethod]
        [TestProperty("TestSuite", "C")]
        public void SourceBeforeLoadTest()
        {
            using (var setup = new WebView2TestSetupHelper(new[] { "WebView2 Tests", "navigateToBasicWebView2" }))
            {
                ChooseTest("SourceBeforeLoadTest");
                CompleteTestAndWaitForResult("SourceBeforeLoadTest");
            }
        }

        [TestMethod]
        [TestProperty("TestSuite", "C")]
        public void VisibilityHiddenTest()
        {
            using (var setup = new WebView2TestSetupHelper(new[] { "WebView2 Tests", "navigateToBasicWebView2" }))
            {
                ChooseTest("VisibilityHiddenTest");
                CompleteTestAndWaitForWebMessageResult("VisibilityHiddenTest");
            }
        }

        [TestMethod]
        [TestProperty("TestSuite", "C")]
        public void VisibilityTurnedOnTest()
        {
            using (var setup = new WebView2TestSetupHelper(new[] { "WebView2 Tests", "navigateToBasicWebView2" }))
            {
                ChooseTest("VisibilityTurnedOnTest");

                // If we got a visibility message when the web view first appeared, make sure we get it again
                var check = new CheckBox(FindElement.ById("MessageReceived"));
                check.Uncheck();

                CompleteTestAndWaitForWebMessageResult("VisibilityTurnedOnTest");
            }
        }

        [TestMethod]
        [TestProperty("TestSuite", "C")]
        public void ParentVisibilityHiddenTest()
        {
            using (var setup = new WebView2TestSetupHelper(new[] { "WebView2 Tests", "navigateToBasicWebView2" }))
            {
                ChooseTest("ParentVisibilityHiddenTest");
                CompleteTestAndWaitForWebMessageResult("ParentVisibilityHiddenTest");
            }
        }

        [TestMethod]
        [TestProperty("TestSuite", "C")]
        public void ParentVisibilityTurnedOnTest()
        {
            using (var setup = new WebView2TestSetupHelper(new[] { "WebView2 Tests", "navigateToBasicWebView2" }))
            {
                ChooseTest("ParentVisibilityTurnedOnTest");

                // If we got a visibility message when the web view first appeared, make sure we get it again
                var check = new CheckBox(FindElement.ById("MessageReceived"));
                check.Uncheck();

                CompleteTestAndWaitForWebMessageResult("ParentVisibilityTurnedOnTest");
            }
        }

        [TestMethod]
        [TestProperty("Ignore", "True")] // TODO_WebView2: Enable when we can change DPI for a test
        [TestProperty("TestSuite", "C")]
        public void SpecificTouchTest()
        {
            using (var setup = new WebView2TestSetupHelper(new[] { "WebView2 Tests", "navigateToBasicWebView2" }))
            {
                ChooseTest("SpecificTouchTest");

                var webview = FindElement.ById("MyWebView2");
                var status2 = new Edit(FindElement.ById("Status2"));

                // Click the button in the middle of the webview. It should be square 35.
                InputHelper.MoveMouse(webview, 0, 0);
                Log.Comment("Do left click...");
                PointerInput.Press(PointerButtons.Primary);
                PointerInput.Release(PointerButtons.Primary);
                Log.Comment("Done clicking");
                WaitForWebMessageResult("SpecificTouchTest");
                var clickResult = status2.GetText();

                // Tap that same button
                Log.Comment("Do tap...");
                InputHelper.Tap(webview);
                Log.Comment("Done tapping");
                WaitForWebMessageResult("SpecificTouchTest");
                var tapResult = status2.GetText();

                Verify.AreEqual(clickResult, tapResult);
            }
        }

        [TestMethod]
        [TestProperty("TestSuite", "C")]
        public void QueryCoreWebView2BasicTest()
        {
            using (var setup = new WebView2TestSetupHelper(new[] { "WebView2 Tests", "navigateToBasicWebView2" }))
            {
                ChooseTest("QueryCoreWebView2BasicTest", false /* waitForLoadCompleted */);
                CompleteTestAndWaitForResult("QueryCoreWebView2BasicTest");
            }
        }

        [TestMethod]
        [TestProperty("TestSuite", "C")]
        public void CoreWebView2InitializedTest()
        {
            using (var setup = new WebView2TestSetupHelper(new[] { "WebView2 Tests", "navigateToBasicWebView2" }))
            {
                ChooseTest("CoreWebView2InitializedTest", false /* waitForLoadCompleted */);
                CompleteTestAndWaitForResult("CoreWebView2InitializedTest");
            }
        }

        [TestMethod]
        [TestProperty("TestSuite", "C")]
        public void CoreWebView2Initialized_FailedTest()
        {
            using (var setup = new WebView2TestSetupHelper(new[] { "WebView2 Tests", "navigateToBasicWebView2" }))
            {
                ChooseTest("CoreWebView2Initialized_FailedTest", false /* waitForLoadCompleted */);

                // Imitate Edge not being installed by setting a reg key to point Edge at a location where the WebView2 runtime does not exist
                // https://docs.microsoft.com/en-us/microsoft-edge/webview2/reference/win32/webview2-idl?view=webview2-1.0.865-prerelease#createcorewebview2environmentwithoptions
                var browserExecutableKey = GetBrowserExecutableFolderKey();
                Log.Comment("Setting key for MuxControlsTestApp.exe");
                browserExecutableKey.SetValue("MuxControlsTestApp.exe", "c:\\badpath");

                try
                {
                    CompleteTestAndWaitForResult("CoreWebView2Initialized_FailedTest");
                }
                finally
                {
                    browserExecutableKey.DeleteValue("MuxControlsTestApp.exe");
                    Log.Comment("Removed key for MuxControlsTestApp.exe");
                }
            }
        }

        private static Microsoft.Win32.RegistryKey GetBrowserExecutableFolderKey()
        {
            return Microsoft.Win32.Registry.LocalMachine.CreateSubKey(@"Software\Policies\Microsoft\Edge\WebView2\BrowserExecutableFolder", true);
        }

        [TestMethod]
        [TestProperty("TestSuite", "C")]
        [TestProperty("Ignore", "True")] // Task 31425073: Unreliable tests: MenuBarTests.KeyboardNavigationWithArrowKeysTest and WebView2CleanedUpTest
        public void WebView2CleanedUpTest()
        {
            using (var setup = new WebView2TestSetupHelper(new[] { "WebView2 Tests", "navigateToBasicWebView2" }))
            {
                // Go back a page (WebView2 should be cleaned up)
                Log.Comment("Go back a page");
                Button backButton = new Button(FindElement.ByNameOrId("__BackButton"));
                backButton.Invoke();
                Wait.ForMilliseconds(1000);

                // Go forward again to the test page
                // Clear element cache so we build a new one for the current page
                Log.Comment("Return to WebView2 test page");
                ElementCache.Clear();
                Button forwardButton = new Button(FindElement.ByNameOrId("navigateToBasicWebView2"));
                forwardButton.Invoke();
                Wait.ForMilliseconds(1000);

                Log.Comment("Find Minimize and Maximize buttons");
                ElementCache.Clear();
                var minimizeButton = new Button(FindElement.ByNameOrId("Minimize"));
                var maximizeButton = new Button(FindElement.ByNameOrId("Restore"));

                // Choose test without any extra waiting. We want to minimize and maximize the app
                // while the page is loading
                string comboBoxName = "TestNameComboBox";
                Log.Comment("Retrieve ComboBox item with name '{0}'.", comboBoxName);
                ComboBox comboBox = new ComboBox(FindElement.ById(comboBoxName));
                comboBox.SelectItemByName("WebView2CleanedUpTest");

                // Minimize and restore the app to trigger the XamlRootChanged event. We are ensuring 
                // that we only handle the event when the WebView2 (and its XamlRoot) are valid.
                // Otherwise, a bad pointer to an invalid WebView2 will crash the app
                minimizeButton.Invoke();
                Log.Comment("Invoked minimize button");
                Wait.ForMilliseconds(1000);
                maximizeButton.Invoke();
                Log.Comment("Invoked maximize button");
                Wait.ForMilliseconds(1000);
            }
        }

        [TestMethod]
        [TestProperty("TestSuite", "D")]
        public void WindowHiddenTest()
        {
            using (var setup = new WebView2TestSetupHelper(new[] { "WebView2 Tests", "navigateToBasicWebView2" }))
            {
                ChooseTest("WindowHiddenTest");

                var appFrameWindow = new Microsoft.Windows.Apps.Test.Foundation.Controls.Window(TestEnvironment.Application.ApplicationFrameWindow);
                // If we got a visibility message when the web view first appeared, make sure we get it again
                var check = new CheckBox(FindElement.ById("MessageReceived"));
                check.Uncheck();

                // Minimize and restore the app to trigger the XamlRootChanged event. The CoreWebView2
                // should be hidden and unhidden as well, each of which will send a web message. We 
                // then listen for the second message.
                Log.Comment("Minimize window");
                appFrameWindow.SetWindowVisualState(WindowVisualState.Minimized);
                Wait.ForMilliseconds(1000);

                Log.Comment("Maximizing window");
                appFrameWindow.SetWindowVisualState(WindowVisualState.Maximized);
                Wait.ForMilliseconds(1000);

                WaitForWebMessageResult("WindowHiddenTest");
            }
        }

        [TestMethod]
        [TestProperty("TestSuite", "D")]
        public void WindowlessPopupTest()
        {
            using (var setup = new WebView2TestSetupHelper(new[] { "WebView2 Tests", "navigateToBasicWebView2" }))
            {
                // Insert popup with empty webview
                ChooseTest("WindowlessPopupTest", false /* waitForLoadCompleted */);

                // Navigates popup to a page
                CompleteTestAndWaitForResult("WindowlessPopupTest");

                // Click button in webview to verify interaction works
                var webview = FindElement.ById("popWebView2");
                Rectangle bounds = webview.BoundingRectangle;
                Log.Comment("Bounds = X:{0}, Y:{1}, Width:{2}, Height:{3}", bounds.X, bounds.Y, bounds.Width, bounds.Height);
                // TODO_WebView2Input - Task 30555367 - Stack Overflow in WebView2 pointer tests when (MUXC) test infra calls WV2.GetClickablePoint()
                //                      Workaround to avoid any calls to WebView2.GetClickablePoint
                /*
                var point = webview.GetClickablePoint();
                Log.Comment("ClickablePoint = X:{0}, Y:{1}", point.X, point.Y);
                // Move mouse to top left coordinate (location of web button) relative from the center of webview
                Log.Comment("MoveMouse to offset = X:{0}, Y:{1}", -bounds.Width / 2 + 20, -bounds.Height / 2 + 20);
                InputHelper.MoveMouse(webview,
                    -bounds.Width / 2 + 20, -bounds.Height / 2 + 20);
                */
                var point = new Point(bounds.X + 20, bounds.Y + 20);
                Log.Comment("Move mouse to ({0}, {1})", bounds.X + 20, bounds.Y + 20);
                PointerInput.Move(point);
                PointerInput.Press(PointerButtons.Primary);
                PointerInput.Release(PointerButtons.Primary);
                Wait.ForIdle();

                WaitForWebMessageResult("WindowlessPopupTest");
            }
        }

        [TestMethod]
        [TestProperty("TestSuite", "D")]
        public void PointerReleaseWithoutPressTest()
        {
            using (var setup = new WebView2TestSetupHelper(new[] { "WebView2 Tests", "navigateToBasicWebView2" }))
            {
                ChooseTest("PointerReleaseWithoutPressTest");

                var webview = FindElement.ById("MyWebView2");
                Rectangle bounds = webview.BoundingRectangle;

                // First, click outside the webview. Then with the mouse button still pressed, 
                // drag into the webview and release the mouse button. This should neither 
                // send a message to CoreWebView2, nor should it crash the app.

                var point = new Point(bounds.X + bounds.Width + 20, bounds.Y + 20);
                PointerInput.Move(point);
                PointerInput.Press(PointerButtons.Primary);
                point = new Point(bounds.X + bounds.Width - 20, bounds.Y + 20);
                PointerInput.Move(point);
                PointerInput.Release(PointerButtons.Primary);

                CompleteTestAndWaitForResult("PointerReleaseWithoutPressTest");
            }
        }

        [TestMethod]
        [TestProperty("TestSuite", "D")]
        public void HostNameToFolderMappingTest()
        {
            using (var setup = new WebView2TestSetupHelper(new[] { "WebView2 Tests", "navigateToBasicWebView2" }))
            {
                ChooseTest("HostNameToFolderMappingTest");

                CompleteTestAndWaitForResult("HostNameToFolderMappingTest");

                var webview = FindElement.ById("MyWebView2");
                Rectangle bounds = webview.BoundingRectangle;
                Log.Comment("Bounds = X:{0}, Y:{1}, Width:{2}, Height:{3}", bounds.X, bounds.Y, bounds.Width, bounds.Height);

                var point = new Point(bounds.X + 20, bounds.Y + 20);
                Log.Comment("Move mouse to ({0}, {1})", bounds.X + 20, bounds.Y + 20);
                PointerInput.Move(point);

                PointerButtons mouseButton = PointerButtons.Primary;
                PointerInput.Press(mouseButton);
                PointerInput.Release(mouseButton);

                WaitForWebMessageResult("HostNameToFolderMappingTest");
            }
        }

        [TestMethod]
        [TestProperty("TestSuite", "D")]
        public void NavigateToVideoTest()
        {
            using (var setup = new WebView2TestSetupHelper(new[] { "WebView2 Tests", "navigateToBasicWebView2" }))
            {
                ChooseTest("NavigateToVideoTest");
                CompleteTestAndWaitForResult("NavigateToVideoTest");
            }
        }

        [TestMethod]
        [TestProperty("TestSuite", "D")]
        public void NavigateToLocalImageTest()
        {
            using (var setup = new WebView2TestSetupHelper(new[] { "WebView2 Tests", "navigateToBasicWebView2" }))
            {
                ChooseTest("NavigateToLocalImageTest");
                CompleteTestAndWaitForWebMessageResult("NavigateToLocalImageTest");
            }
        }

        [TestMethod]
        [TestProperty("Ignore", "True")] // TODO_WebView2: Enable when we can change DPI for a test
        [TestProperty("TestSuite", "D")]
        public void CloseThenDPIChangeTest()
        {
            using (var setup = new WebView2TestSetupHelper(new[] { "WebView2 Tests", "navigateToBasicWebView2" }))
            {
                ChooseTest("CloseThenDPIChangeTest");
                // Close the webview
                CompleteTestAndWaitForResult("CloseThenDPIChangeTest");
                // TODO: somehow change the DPI and ensure there is no crash
                // This test can be run manually by choosing the test, clicking the "Complete Test" button,
                // and then moving the application window to a monitor with a different DPI
            }
        }

        [TestMethod]
        [TestProperty("TestSuite", "D")]
        [TestProperty("Ignore", "True")] // Test fails because .NET UWP doesn't support Object -> VARIANT marshalling.
        public void AddHostObjectToScriptTest()
        {
            using (var setup = new WebView2TestSetupHelper(new[] { "WebView2 Tests", "navigateToBasicWebView2" }))
            {
                ChooseTest("AddHostObjectToScriptTest");
                CompleteTestAndWaitForWebMessageResult("AddHostObjectToScriptTest");
            }
        }

        [TestMethod]
        [TestProperty("TestSuite", "D")]
        public void UserAgentTest()
        {
            using (var setup = new WebView2TestSetupHelper(new[] { "WebView2 Tests", "navigateToBasicWebView2" }))
            {
                ChooseTest("UserAgentTest");
                CompleteTestAndWaitForResult("UserAgentTest");
            }
        }

        [TestMethod]
        [TestProperty("TestSuite", "D")]
        public void NonAsciiUriTest()
        {
            using (var setup = new WebView2TestSetupHelper(new[] { "WebView2 Tests", "navigateToBasicWebView2" }))
            {
                // Navigate to a uri with a non-ascii characters
                ChooseTest("NonAsciiUriTest");
                CompleteTestAndWaitForResult("NonAsciiUriTest");

                // At the end of the test, we should only have gotten one NavigationStarting message.
                // All messages would have been printed to the MessageLog, so count the NavigationStarting
                // messages there.
                int count = 0;
                var logBox = new Edit(FindElement.ById("MessageLog"));
                var messageWords = logBox.GetText().Split(' ');
                foreach (string word in messageWords)
                {
                    if (word.Equals("NavigationStarting")) count++;
                }

                Verify.AreEqual(count, 1);
            }
        }

        [TestMethod]
        [TestProperty("TestSuite", "D")]
        public void OffTreeWebViewInputTest()
        {
            using (var setup = new WebView2TestSetupHelper(new[] { "WebView2 Tests", "navigateToBasicWebView2" }))
            {
                // Remove the existing webview that was already added to the xaml tree
                ChooseTest("OffTreeWebViewInputTest", false /* waitForLoadCompleted */);

                // Create a new webview, and call EnsureCoreWebView2Async() on it without adding it to the tree.
                // Then, add it to the tree so we can see it, and navigate
                CompleteTestAndWaitForResult("OffTreeWebViewInputTest");
                WaitForLoadCompleted();

                // Clear the cache so we can find the new webview
                ElementCache.Clear();
                var webview = FindElement.ById("MyWebView2");

                // Click in the webview, to ensure we can interact with it, and its HWNDs are parented correctly
                Rectangle bounds = webview.BoundingRectangle;
                Log.Comment("Bounds = X:{0}, Y:{1}, Width:{2}, Height:{3}", bounds.X, bounds.Y, bounds.Width, bounds.Height);
                // TODO_WebView2Input - Task 30555367 - Stack Overflow in WebView2 pointer tests when (MUXC) test infra calls WV2.GetClickablePoint()
                //                      Workaround to avoid any calls to WebView2.GetClickablePoint
                /*
                var point = webview.GetClickablePoint();
                Log.Comment("ClickablePoint = X:{0}, Y:{1}", point.X, point.Y);
                // Move mouse to top left coordinate (location of web button) relative from the center of webview
                InputHelper.MoveMouse(webview,
                    -bounds.Width/2 + 20, -bounds.Height/2 + 20);
                */
                var point = new Point(bounds.X + 20, bounds.Y + 20);
                Log.Comment("Move mouse to ({0}, {1})", bounds.X + 20, bounds.Y + 20);
                PointerInput.Move(point);

                PointerInput.Press(PointerButtons.Primary);
                PointerInput.Release(PointerButtons.Primary);
                Wait.ForIdle();

                WaitForWebMessageResult("OffTreeWebViewInputTest");
            }
        }
        
        [TestMethod]
        [TestProperty("TestSuite", "D")]
        [TestProperty("Ignore", "True")] // Passes locally, test can be run manually
        public void HtmlDropdownTest()
        {
            using (var setup = new WebView2TestSetupHelper(new[] { "WebView2 Tests", "navigateToBasicWebView2" }))
            {
                ChooseTest("HtmlDropdownTest");

                // Click on the select (dropdown) element, which covers the center of the WebView2
                var webview = FindElement.ById("MyWebView2");
                Rectangle bounds = webview.BoundingRectangle;
                var point = new Point(bounds.X + bounds.Width / 2, bounds.Y + bounds.Height / 2);
                Log.Comment("Move mouse to ({0}, {1})", point.X, point.Y);
                PointerInput.Move(point);
                PointerInput.Press(PointerButtons.Primary);
                PointerInput.Release(PointerButtons.Primary);

                var newPoint = new Point(point.X, point.Y + 110);
                Log.Comment("Move mouse to another dropdown option");
                PointerInput.Move(newPoint);
                Log.Comment("Click other option");
                PointerInput.Press(PointerButtons.Primary);
                PointerInput.Release(PointerButtons.Primary);

                // On app side, ensure the right option was selected
                CompleteTestAndWaitForResult("HtmlDropdownTest");
            }
        }

        [TestMethod]
        [TestProperty("TestSuite", "D")]
        public void HiddenThenVisibleTest()
        {
            using (var setup = new WebView2TestSetupHelper(new[] { "WebView2 Tests", "navigateToBasicWebView2" }))
            {
                ChooseTest("HiddenThenVisibleTest");
                CompleteTestAndWaitForResult("HiddenThenVisibleTest");

                // Clear the cache so we can find the new webview
                ElementCache.Clear();
                var webview = FindElement.ById("MyWebView2");
                Verify.IsTrue(webview.IsOffscreen == false);

                // Click in the webview to ensure we can interact with it
                Rectangle bounds = webview.BoundingRectangle;
                Log.Comment("Bounds = X:{0}, Y:{1}, Width:{2}, Height:{3}", bounds.X, bounds.Y, bounds.Width, bounds.Height);
                var point = new Point(bounds.X + 20, bounds.Y + 20);
                Log.Comment("Move mouse to ({0}, {1})", bounds.X + 20, bounds.Y + 20);
                PointerInput.Move(point);

                PointerInput.Press(PointerButtons.Primary);
                PointerInput.Release(PointerButtons.Primary);
                Wait.ForIdle();

                WaitForWebMessageResult("HiddenThenVisibleTest");
            }
        }

        [TestMethod]
        [TestProperty("TestSuite", "D")]
        public void ParentHiddenThenVisibleTest()
        {
            using (var setup = new WebView2TestSetupHelper(new[] { "WebView2 Tests", "navigateToBasicWebView2" }))
            {
                ChooseTest("ParentHiddenThenVisibleTest");
                CompleteTestAndWaitForResult("ParentHiddenThenVisibleTest");

                // Clear the cache so we can find the new webview
                ElementCache.Clear();
                var webview = FindElement.ById("MyWebView2");
                Verify.IsTrue(webview.IsOffscreen == false);

                // Click in the webview to ensure we can interact with it
                Rectangle bounds = webview.BoundingRectangle;
                Log.Comment("Bounds = X:{0}, Y:{1}, Width:{2}, Height:{3}", bounds.X, bounds.Y, bounds.Width, bounds.Height);
                var point = new Point(bounds.X + 20, bounds.Y + 20);
                Log.Comment("Move mouse to ({0}, {1})", bounds.X + 20, bounds.Y + 20);
                PointerInput.Move(point);

                PointerInput.Press(PointerButtons.Primary);
                PointerInput.Release(PointerButtons.Primary);
                Wait.ForIdle();

                WaitForWebMessageResult("ParentHiddenThenVisibleTest");
            }
        }

        [TestMethod]
        [TestProperty("TestSuite", "D")]
        public void LifetimeTabTest()
        {
            using (var setup = new WebView2TestSetupHelper(new[] { "WebView2 Tests", "navigateToBasicWebView2" }))
            {
                // Part 1: Tab with no core webview

                Log.Comment("Part 1: Tab with no core webview");
                TabTwicePastWebView2();

                // Part 2: Tab with core webview, ensured but not navigated

                Log.Comment("Part 2: Tab with core webview, not navigated");
                Button ensureButton = new Button(FindElement.ById("EnsureCWV2Button"));
                ensureButton.Invoke();
                WaitForEnsureCompleted();
                TabTwicePastWebView2();

                // Part 3: Tab with closed core webview

                Log.Comment("Part 3: Tab with closed core webview");
                ChooseTest("LifetimeTabTest" /* waitForLoadCompleted */);
                CompleteTestAndWaitForResult("LifetimeTabTest");  // Closes the CoreWebView2
                TabTwicePastWebView2();
            }
        }

        private static void TabTwicePastWebView2()
        {
            Button x1 = new Button(FindElement.ById("TabStopButton1"));     // Xaml TabStop 1
            Button x2 = new Button(FindElement.ById("TabStopButton2"));     // Xaml TabStop 2

            Log.Comment("Focus on x1");
            x1.SetFocus();
            Wait.ForIdle();
            Verify.IsTrue(x1.HasKeyboardFocus, "TabStopButton1 has keyboard focus");

            Log.Comment("Tab x1 -> webview -> x2");
            using (var xamlFocusWaiter = new FocusAcquiredWaiter("TabStopButton2"))
            {
                KeyboardHelper.PressKey(Key.Tab);
                KeyboardHelper.PressKey(Key.Tab);
                xamlFocusWaiter.Wait();
                Log.Comment("Focus is on " + UIObject.Focused);
                Verify.IsTrue(x2.HasKeyboardFocus, "TabStopButton2 has keyboard focus");
            }
        }

        private static void BeginSubTest(string testName, string testDescription)
        {
            Log.Comment(Environment.NewLine + testName + ": " + testDescription);

            Log.Comment("Resetting event and exception counts...");
            Button resetCounts_Button = new Button(FindElement.ById("ResetCounts_Button"));
            resetCounts_Button.Invoke();
            Wait.ForIdle();
        }

        private static void EndSubTest(string testName)
        {
            Log.Comment(testName + " finished...");
        }

        public static void ValidateExceptionCounts(
            int closedExceptionCountExpected,
            string caption = null
        )
        {
            Log.Comment("ValidateExceptionCounts: " + caption);

            Verify.AreEqual(closedExceptionCountExpected.ToString(),
                            (new Edit(FindElement.ById("ClosedExceptionCount"))).Value,
                            "ClosedExceptionCount should be " + closedExceptionCountExpected);
        }

        public static void ValidateEventCounts(
            int coreWebView2InitializedCountExpected,
            int ensureCoreWebView2CompletionCountExpected,
            int navigationStartingCountExpected,
            int navigationCompletedCountExpected,
            int loadedCountExpected,
            int unloadedCountExpected,
            int coreProcessFailedCountExpected,
            string caption = null
        )
        {
            Log.Comment("ValidateEventCounts: " + caption);

            Verify.AreEqual(coreWebView2InitializedCountExpected.ToString(),
                            (new Edit(FindElement.ById("CoreWebView2InitializedCount"))).Value,
                            "CoreWebView2InitializedCount should be " + coreWebView2InitializedCountExpected);

            Verify.AreEqual(ensureCoreWebView2CompletionCountExpected.ToString(),
                            (new Edit(FindElement.ById("EnsureCoreWebView2CompletionCount"))).Value,
                            "EnsureCoreWebView2CompletionCount should be " + ensureCoreWebView2CompletionCountExpected);

            Verify.AreEqual(navigationStartingCountExpected.ToString(),
                            (new Edit(FindElement.ById("NavigationStartingCount"))).Value,
                            "NavigationStartingCount should be " + navigationStartingCountExpected);

            Verify.AreEqual(navigationCompletedCountExpected.ToString(),
                            (new Edit(FindElement.ById("NavigationCompletedCount"))).Value,
                            "NavigationCompletedCount should be " + navigationCompletedCountExpected);

            Verify.AreEqual(loadedCountExpected.ToString(),
                            (new Edit(FindElement.ById("LoadedCount"))).Value,
                            "LoadedCount should be " + loadedCountExpected);

            Verify.AreEqual(unloadedCountExpected.ToString(),
                            (new Edit(FindElement.ById("UnloadedCount"))).Value,
                            "UnloadedCount should be " + unloadedCountExpected);

            Verify.AreEqual(coreProcessFailedCountExpected.ToString(),
                            (new Edit(FindElement.ById("CoreProcessFailedCount"))).Value,
                            "CoreProcessFailedCount should be " + coreProcessFailedCountExpected);
        }

        [TestMethod]
        [TestProperty("TestSuite", "D")]
        public void BasicCoreObjectCreationAndDestructionTest()
        {
            using (var setup = new WebView2TestSetupHelper(new[] { "WebView2 Tests", "navigateToCoreObjectsWebView2" }))
            {
                ChooseTest("BasicCoreObjectCreationAndDestructionTest", false /* waitForLoadCompleted */);

                Button resetCounts_Button = new Button(FindElement.ById("ResetCounts_Button"));
                resetCounts_Button.Invoke();
                Wait.ForIdle();

                var coreWebView2InitializedCount = new Edit(FindElement.ById("CoreWebView2InitializedCount"));
                var ensureCoreWebView2CompletionCount = new Edit(FindElement.ById("EnsureCoreWebView2CompletionCount")); 
                var navigationCompletedCount = new Edit(FindElement.ById("NavigationCompletedCount"));
                var loadedCount = new Edit(FindElement.ById("LoadedCount"));
                var unloadedCount = new Edit(FindElement.ById("UnloadedCount"));

                BeginSubTest("Preload Test #1", "CreateOffline -> SetSource -> Wait(NavCompleted) -> EnterTree -> Wait(Loaded)");
                Button create_OfflineElement_Button = new Button(FindElement.ById("Create_OfflineElement_Button"));
                Button setSource_OfflineElement_Button = new Button(FindElement.ById("SetSource_OfflineElement_Button"));
                Button add_OfflineElement_Button = new Button(FindElement.ById("Add_OfflineElement_Button"));

                // Create offline element and set source
                Log.Comment("Create offline element and set source...");
                using (var navigationCompletedWaiter = new ValueChangedEventWaiter(navigationCompletedCount, "1"))
                {
                    create_OfflineElement_Button.Invoke();
                    setSource_OfflineElement_Button.Invoke();
                    navigationCompletedWaiter.Wait();
                }

                ValidateEventCounts(
                    1,      // coreWebView2InitializedCountExpected
                    0,      // ensureCoreWebView2CompletionCountExpected
                    1,      // navigationStartingCountExpected
                    1,      // navigationCompletedCountExpected
                    0,      // loadedCountExpected
                    0,      // unloadedCountExpected
                    0,      // coreProcessFailedCountExpected
                    "Created offline w/ source (not live yet)"
                );

                Log.Comment("Add to tree...");
                using (var loadedWaiter = new ValueChangedEventWaiter(loadedCount, "1"))
                {
                    add_OfflineElement_Button.Invoke();
                    loadedWaiter.Wait();
                }

                ValidateEventCounts(
                    1,      // coreWebView2InitializedCountExpected
                    0,      // ensureCoreWebView2CompletionCountExpected
                    1,      // navigationStartingCountExpected
                    1,      // navigationCompletedCountExpected
                    1,      // loadedCountExpected
                    0,      // unloadedCountExpected
                    0,      // coreProcessFailedCountExpected
                    "Added to tree"
                );
                EndSubTest("Preload Test #1");

                BeginSubTest("Preload Test #2", "CreateOffline -> EnsureCWV2() -> Wait(EnsureCWV2_Completion) -> EnterTree -> Wait(Loaded) -> SetSource");
                Button ensureCoreWebView2_OfflineElement_Button = new Button(FindElement.ById("EnsureCoreWebView2_OfflineElement_Button"));

                Log.Comment("Create offline element and EnsureCWV2()...");
                using (var ensureCoreWebView2CompletionWaiter = new ValueChangedEventWaiter(ensureCoreWebView2CompletionCount, "1"))
                {
                    create_OfflineElement_Button.Invoke();
                    ensureCoreWebView2_OfflineElement_Button.Invoke();
                    ensureCoreWebView2CompletionWaiter.Wait();
                }

                ValidateEventCounts(
                    1,      // coreWebView2InitializedCountExpected
                    1,      // ensureCoreWebView2CompletionCountExpected
                    0,      // navigationStartingCountExpected
                    0,      // navigationCompletedCountExpected
                    0,      // loadedCountExpected
                    0,      // unloadedCountExpected
                    0,      // coreProcessFailedCountExpected
                    "Created offline without source (not live yet)"
                );

                Log.Comment("Add to tree...");
                using (var loadedWaiter = new ValueChangedEventWaiter(loadedCount, "1"))
                {
                    add_OfflineElement_Button.Invoke();
                    loadedWaiter.Wait();
                }

                ValidateEventCounts(
                    1,      // coreWebView2InitializedCountExpected
                    1,      // ensureCoreWebView2CompletionCountExpected
                    0,      // navigationStartingCountExpected
                    0,      // navigationCompletedCountExpected
                    1,      // loadedCountExpected
                    0,      // unloadedCountExpected
                    0,      // coreProcessFailedCountExpected
                    "Added to tree"
                );

                Log.Comment("Set source...");
                using (var navigationCompletedWaiter = new ValueChangedEventWaiter(navigationCompletedCount, "1"))
                {
                    setSource_OfflineElement_Button.Invoke();
                    navigationCompletedWaiter.Wait();
                }

                ValidateEventCounts(
                    1,      // coreWebView2InitializedCountExpected
                    1,      // ensureCoreWebView2CompletionCountExpected
                    1,      // navigationStartingCountExpected
                    1,      // navigationCompletedCountExpected
                    1,      // loadedCountExpected
                    0,      // unloadedCountExpected
                    0,      // coreProcessFailedCountExpected
                    "Set source"
                );
                EndSubTest("Preload Test #2");

                BeginSubTest("Live Element Test #1", "CreateLive -> Set Source -> Wait(NavCompleted/Loaded)");
                Button create_LiveElement_Button = new Button(FindElement.ById("Create_LiveElement_Button"));
                Button setSource_LiveElement_Button = new Button(FindElement.ById("SetSource_LiveElement_Button"));

                // Notice logic in the waiters here to handle race between NavCompleted and Loaded
                Log.Comment("Create live element and set source...");
                using (var navigationCompletedWaiter = new ValueChangedEventWaiter(navigationCompletedCount, "1"))
                {
                    create_LiveElement_Button.Invoke();
                    setSource_LiveElement_Button.Invoke();
                    navigationCompletedWaiter.Wait();
                }

                if (loadedCount.Value != "1")
                {
                    using (var loadedWaiter = new ValueChangedEventWaiter(loadedCount, "1"))
                    {
                        loadedWaiter.Wait();
                    }
                }

                ValidateEventCounts(
                    1,      // coreWebView2InitializedCountExpected
                    0,      // ensureCoreWebView2CompletionCountExpected
                    1,      // navigationStartingCountExpected
                    1,      // navigationCompletedCountExpected
                    1,      // loadedCountExpected
                    0,      // unloadedCountExpected
                    0,      // coreProcessFailedCountExpected
                    "Create live with source"
                );
                EndSubTest("Live Element Test #1");

                BeginSubTest("Live Element Test #2", "CreateLive -> EnsureCWV2() -> Wait(EnsureCWV2_Completion/Loaded) -> SetSource");
                Button ensureCoreWebView2_LiveElement_Button = new Button(FindElement.ById("EnsureCoreWebView2_LiveElement_Button"));

                Log.Comment("Create live element and EnsureCWV2()...");
                using (var ensureCoreWebView2CompletionWaiter = new ValueChangedEventWaiter(ensureCoreWebView2CompletionCount, "1"))
                {
                    create_LiveElement_Button.Invoke();
                    ensureCoreWebView2_LiveElement_Button.Invoke();
                    ensureCoreWebView2CompletionWaiter.Wait();
                }

                if (loadedCount.Value != "1")
                {
                    using (var loadedWaiter = new ValueChangedEventWaiter(loadedCount, "1"))
                    {
                        loadedWaiter.Wait();
                    }
                }
                ValidateEventCounts(
                    1,      // coreWebView2InitializedCountExpected
                    1,      // ensureCoreWebView2CompletionCountExpected
                    0,      // navigationStartingCountExpected
                    0,      // navigationCompletedCountExpected
                    1,      // loadedCountExpected
                    0,      // unloadedCountExpected
                    0,      // coreProcessFailedCountExpected
                    "Create live without source"
                );

                Log.Comment("Set source...");
                using (var navigationCompletedWaiter = new ValueChangedEventWaiter(navigationCompletedCount, "1"))
                {
                    setSource_LiveElement_Button.Invoke();
                    navigationCompletedWaiter.Wait();
                }

                ValidateEventCounts(
                    1,      // coreWebView2InitializedCountExpected
                    1,      // ensureCoreWebView2CompletionCountExpected
                    1,      // navigationStartingCountExpected
                    1,      // navigationCompletedCountExpected
                    1,      // loadedCountExpected
                    0,      // unloadedCountExpected
                    0,      // coreProcessFailedCountExpected
                    "Source set"
                );
                EndSubTest("Live Element Test #2");

                BeginSubTest("Markup Test #1", "Create_XamlReaderLoad -> EnterTree-> EnsureCWV2() -> Wait(EnsureCWV2_Completion/Loaded) -> SetSource");
                Button create_MarkupElement_Blank_Button = new Button(FindElement.ById("Create_MarkupElement_Blank_Button"));
                Button ensureCoreWebView2_MarkupElement_Button = new Button(FindElement.ById("EnsureCoreWebView2_MarkupElement_Button"));
                Button setSource_MarkupElement_Button = new Button(FindElement.ById("SetSource_MarkupElement_Button"));

                Log.Comment("Load from Markup and add to tree");
                using (var loadedWwaiter = new ValueChangedEventWaiter(loadedCount, "1"))
                {
                    create_MarkupElement_Blank_Button.Invoke();
                    loadedWwaiter.Wait();
                }
                ValidateEventCounts(
                    0,      // coreWebView2InitializedCountExpected
                    0,      // ensureCoreWebView2CompletionCountExpected
                    0,      // navigationStartingCountExpected
                    0,      // navigationCompletedCountExpected
                    1,      // loadedCountExpected
                    0,      // unloadedCountExpected
                    0,      // coreProcessFailedCountExpected
                    "Loaded from markup and added to tree"
                );

                Log.Comment("Call EnsureCWV2()");
                using (var ensureCoreWebView2CompletionWaiter = new ValueChangedEventWaiter(ensureCoreWebView2CompletionCount, "1"))
                {
                    ensureCoreWebView2_MarkupElement_Button.Invoke();
                    ensureCoreWebView2CompletionWaiter.Wait();
                }
                ValidateEventCounts(
                    1,      // coreWebView2InitializedCountExpected
                    1,      // ensureCoreWebView2CompletionCountExpected
                    0,      // navigationStartingCountExpected
                    0,      // navigationCompletedCountExpected
                    1,      // loadedCountExpected
                    0,      // unloadedCountExpected
                    0,      // coreProcessFailedCountExpected
                    "Loaded from markup and added to tree"
                );

                Log.Comment("Set Source");
                using (var navigationCompletedWaiter = new ValueChangedEventWaiter(navigationCompletedCount, "1"))
                {
                    setSource_MarkupElement_Button.Invoke();
                    navigationCompletedWaiter.Wait();
                }

                ValidateEventCounts(
                    1,      // coreWebView2InitializedCountExpected
                    1,      // ensureCoreWebView2CompletionCountExpected
                    1,      // navigationStartingCountExpected
                    1,      // navigationCompletedCountExpected
                    1,      // loadedCountExpected
                    0,      // unloadedCountExpected
                    0,      // coreProcessFailedCountExpected
                    "Source set"
                );
                EndSubTest("Markup Test #1");

                // MarkupTest #2: XamlReader.Load() WV2 with Source Set
                BeginSubTest("Markup Test #2", "Create_XamlReaderLoad (with Source set)");
                Button create_MarkupElement_Source_Button = new Button(FindElement.ById("Create_MarkupElement_Source_Button"));

                Log.Comment("Create Live element w/ Source");
                using (var navigationCompletedWaiter = new ValueChangedEventWaiter(navigationCompletedCount, "1"))
                {
                    create_MarkupElement_Source_Button.Invoke();
                    navigationCompletedWaiter.Wait();
                }

                if (loadedCount.Value != "1")
                {
                    using (var loadedWaiter = new ValueChangedEventWaiter(loadedCount, "1"))
                    {
                        loadedWaiter.Wait();
                    }
                }

                ValidateEventCounts(
                    1,      // coreWebView2InitializedCountExpected
                    0,      // ensureCoreWebView2CompletionCountExpected
                    1,      // navigationStartingCountExpected
                    1,      // navigationCompletedCountExpected
                    1,      // loadedCountExpected
                    0,      // unloadedCountExpected
                    0,      // coreProcessFailedCountExpected
                    "Loaded from markup w/ Source"
                );
                EndSubTest("Markup Test #2");

                Log.Comment("Close() API Tests: Force close the 3 WV2s we created above");
                // Close() API Test: Force close the 3 WV2' we created above
                //   Part A: Close WV2 element used earlier for "Offline (pre-load) Creation" test
                BeginSubTest("Close Test #1", "Close WV2 element used earlier for 'Offline (pre-load) Creation' test");
                Button releaseReference_OfflineElement_Button = new Button(FindElement.ById("ReleaseReference_OfflineElement_Button"));
                Button close_OfflineElement_Button = new Button(FindElement.ById("Close_OfflineElement_Button"));
                Button remove_OfflineElement_Button = new Button(FindElement.ById("Remove_OfflineElement_Button"));

                Log.Comment("Remove from tree, then Close");
                using (var unloadedWaiter = new ValueChangedEventWaiter(unloadedCount, "1"))
                {
                    remove_OfflineElement_Button.Invoke();
                    close_OfflineElement_Button.Invoke();
                    unloadedWaiter.Wait();
                }
                ValidateEventCounts(
                    0,      // coreWebView2InitializedCountExpected
                    0,      // ensureCoreWebView2CompletionCountExpected
                    0,      // navigationStartingCountExpected
                    0,      // navigationCompletedCountExpected
                    0,      // loadedCountExpected
                    1,      // unloadedCountExpected
                    0,      // coreProcessFailedCountExpected
                    "Remove, Close"
                );

                Log.Comment("Try to set Source -> expect RO_E_CLOSED");
                setSource_OfflineElement_Button.Invoke();
                ValidateExceptionCounts(
                    1       // closedExceptionCountExpected
                );
                EndSubTest("Close Test #1");

                BeginSubTest("Close Test #2", "Close WV2 element used earlier for 'Live (in-tree) Creation' test");
                Button remove_LiveElement_Button = new Button(FindElement.ById("Remove_LiveElement_Button"));
                Button close_LiveElement_Button = new Button(FindElement.ById("Close_LiveElement_Button"));

                // Test just Close(). No events to wait on here.
                Log.Comment("Close");
                close_LiveElement_Button.Invoke();

                ValidateEventCounts(
                    0,      // coreWebView2InitializedCountExpected
                    0,      // ensureCoreWebView2CompletionCountExpected
                    0,      // navigationStartingCountExpected
                    0,      // navigationCompletedCountExpected
                    0,      // loadedCountExpected
                    0,      // unloadedCountExpected
                    0,      // coreProcessFailedCountExpected
                    "Close"
                );

                Log.Comment("Try EnsureCWV2 -> expect RO_E_CLOSED");
                ensureCoreWebView2_LiveElement_Button.Invoke();
                ValidateExceptionCounts(
                    1       // closedExceptionCountExpected
                );
                EndSubTest("Close Test #2");

                BeginSubTest("Close Test #3", "Close WV2 element used earlier for 'Markup-based Creation' test");
                Button remove_MarkupElement_Button = new Button(FindElement.ById("Remove_MarkupElement_Button"));
                Button close_MarkupElement_Button = new Button(FindElement.ById("Close_MarkupElement_Button"));

                Log.Comment("Close, then Remove from tree");
                using (var unloadedWaiter = new ValueChangedEventWaiter(unloadedCount, "1"))
                {
                    close_MarkupElement_Button.Invoke();
                    remove_MarkupElement_Button.Invoke();
                    unloadedWaiter.Wait();
                }

                ValidateEventCounts(
                    0,      // coreWebView2InitializedCountExpected
                    0,      // ensureCoreWebView2CompletionCountExpected
                    0,      // navigationStartingCountExpected
                    0,      // navigationCompletedCountExpected
                    0,      // loadedCountExpected
                    1,      // unloadedCountExpected
                    0,      // coreProcessFailedCountExpected
                    "Close, Remove"
                );

                Log.Comment("Try EnsureCWV2 -> expect RO_E_CLOSED");
                ensureCoreWebView2_MarkupElement_Button.Invoke();
                ValidateExceptionCounts(
                    1       // closedExceptionCountExpected
                );
                EndSubTest("Close Test #3");
            }
        }

        // Check Status3 textbox to ensure the right web page is loadeed
        private static void ValidateLoadedSource(string loadedSource)
        {
            string status3Text = (new Edit(FindElement.ById("Status3"))).Value;
            Verify.IsTrue(status3Text.Contains(loadedSource), "NavigationStarting path should contain the source (" + loadedSource + ")");
        }

        private static void ValidateEventDetail(string eventDetail)
        {
            string eventDetailText = (new Edit(FindElement.ById("EventDetail"))).Value;
            Verify.IsTrue(eventDetailText.Contains(eventDetail), "EventDetail should contain (" + eventDetail + ")");
        }


        //[TestMethod] Bug 45686914: WinUI2: WebView2Tests.ConcurrentCreationRequestsTest is failing in main.
        [TestProperty("TestSuite", "D")]
        public void ConcurrentCreationRequestsTest()
        {
            // Use two sources to test that concurrent creations/navigations end up in the expected state
            string source1_filename = "SimplePage.html";
            string source2_filename = "SimplePageWithButton.html";

            using (var setup = new WebView2TestSetupHelper(new[] { "WebView2 Tests", "navigateToCoreObjectsWebView2" }))
            {
                ChooseTest("ConcurrentCreationRequestsTest", false /* waitForLoadCompleted */);

                var coreWebView2InitializedCount = new Edit(FindElement.ById("CoreWebView2InitializedCount"));
                var ensureCoreWebView2CompletionCount = new Edit(FindElement.ById("EnsureCoreWebView2CompletionCount")); 
                var navigationCompletedCount = new Edit(FindElement.ById("NavigationCompletedCount"));
                var loadedCount = new Edit(FindElement.ById("LoadedCount"));

                Button create_ConcurrentCreationElement_Button = new Button(FindElement.ById("Create_ConcurrentCreationElement_Button"));
                Button source_Source_Button = new Button(FindElement.ById("Source_Source_Button"));
                Button source_Ensure_Button = new Button(FindElement.ById("Source_Ensure_Button"));
                Button ensure_Source_Button = new Button(FindElement.ById("Ensure_Source_Button"));
                Button ensure_Ensure_Button = new Button(FindElement.ById("Ensure_Ensure_Button"));
                Button remove_ConcurrentCreationElement_Button = new Button(FindElement.ById("Remove_ConcurrentCreationElement_Button"));
                Button close_ConcurrentCreationElement_Button = new Button(FindElement.ById("Close_ConcurrentCreationElement_Button"));

                BeginSubTest("Concurrent Creation Test #1: Source, Source","Set Source #1, then Source #2. Expect creation to run once and navigate to Source #2 only.");
                using (var navigationCompletedWaiter = new ValueChangedEventWaiter(navigationCompletedCount, "1"))
                {
                    create_ConcurrentCreationElement_Button.Invoke();
                    source_Source_Button.Invoke();
                    navigationCompletedWaiter.Wait();
                }

                if (loadedCount.Value != "1")
                {
                    using (var loadedWaiter = new ValueChangedEventWaiter(loadedCount, "1"))
                    {
                        loadedWaiter.Wait();
                    }
                }

                ValidateLoadedSource(source2_filename);

                ValidateEventCounts(
                    1,      // coreWebView2InitializedCountExpected
                    0,      // ensureCoreWebView2CompletionCountExpected
                    1,      // navigationStartingCountExpected
                    1,      // navigationCompletedCountExpected
                    1,      // loadedCountExpected
                    0,      // unloadedCountExpected
                    0,      // coreProcessFailedCountExpected
                    "Source, Source"

                );
                EndSubTest("Concurrent Creation Test #1: Source, Source");

                BeginSubTest("Concurrent Creation Test #2: Source, EnsureCWV2", "Set Source, then call EnsureCoreWebView2(). Both calls should succeed.");
                using (var navigationCompletedWaiter = new ValueChangedEventWaiter(navigationCompletedCount, "1"))
                {
                    create_ConcurrentCreationElement_Button.Invoke();
                    source_Ensure_Button.Invoke();
                    navigationCompletedWaiter.Wait();
                }

                if (loadedCount.Value != "1")
                {
                    using (var loadedWaiter = new ValueChangedEventWaiter(loadedCount, "1"))
                    {
                        loadedWaiter.Wait();
                    }
                }

                ValidateLoadedSource(source1_filename);

                ValidateEventCounts(
                    1,      // coreWebView2InitializedCountExpected
                    1,      // ensureCoreWebView2CompletionCountExpected
                    1,      // navigationStartingCountExpected
                    1,      // navigationCompletedCountExpected
                    1,      // loadedCountExpected
                    0,      // unloadedCountExpected
                    0,      // coreProcessFailedCountExpected
                    "Source, EnsureCWV2"
                    );
                EndSubTest("Concurrent Creation Test #2: Source, EnsureCWV2");

                BeginSubTest("Concurrent Creation Test #3: EnsureCWV2, Source", "Call EnsureCWV(), then set Source. Both calls should succeed.");
                using (var navigationCompletedWaiter = new ValueChangedEventWaiter(navigationCompletedCount, "1"))
                {
                    create_ConcurrentCreationElement_Button.Invoke();
                    ensure_Source_Button.Invoke();
                    navigationCompletedWaiter.Wait();
                }

                if (loadedCount.Value != "1")
                {
                    using (var loadedWaiter = new ValueChangedEventWaiter(loadedCount, "1"))
                    {
                        loadedWaiter.Wait();
                    }
                }

                ValidateLoadedSource(source2_filename);

                ValidateEventCounts(
                    1,      // coreWebView2InitializedCountExpected
                    1,      // ensureCoreWebView2CompletionCountExpected
                    1,      // navigationStartingCountExpected
                    1,      // navigationCompletedCountExpected
                    1,      // loadedCountExpected
                    0,      // unloadedCountExpected
                    0,      // coreProcessFailedCountExpected
                    "Source, EnsureCWV2"
                    );
                EndSubTest("Concurrent Creation Test #3: Source, EnsureCWV2");

                BeginSubTest("Concurrent Creation Test #4: EnsureCWV2, EnsureCWV2", "Call EnsureCWV(), then  EnsureCWV() again.");
                using (var ensureCoreWebView2CompletionWaiter = new ValueChangedEventWaiter(ensureCoreWebView2CompletionCount, "2"))
                {
                    create_ConcurrentCreationElement_Button.Invoke();
                    ensure_Ensure_Button.Invoke();
                }

                if (coreWebView2InitializedCount.Value != "1")
                {
                    using (var coreWebView2InitializedWaiter = new ValueChangedEventWaiter(coreWebView2InitializedCount, "1"))
                    {
                        coreWebView2InitializedWaiter.Wait();
                    }
                }

                if (loadedCount.Value != "1")
                {
                    using (var loadedWaiter = new ValueChangedEventWaiter(loadedCount, "1"))
                    {
                        loadedWaiter.Wait();
                    }
                }

                // No source specified, so skip ValidateLoadedSource()

                ValidateEventCounts(
                    1,      // coreWebView2InitializedCountExpected
                    2,      // ensureCoreWebView2CompletionCountExpected
                    0,      // navigationStartingCountExpected
                    0,      // navigationCompletedCountExpected
                    1,      // loadedCountExpected
                    0,      // unloadedCountExpected
                    0,      // coreProcessFailedCountExpected
                    "EnsureCWV2, EnsureCWV2"
                    );
                EndSubTest("Concurrent Creation Test #4: EnsureCWV2, EnsureCWV2");
            }
        }

        [TestMethod]
        [TestProperty("TestSuite", "D")]
        [TestProperty("Ignore", "True")] // Bug 42203617
        public void EdgeProcessFailedTest()
        {
            using (var setup = new WebView2TestSetupHelper(new[] { "WebView2 Tests", "navigateToCoreObjectsWebView2" }))
            {
                ChooseTest("EdgeProcessFailedTest", false /* waitForLoadCompleted */);

                var coreWebView2InitializedCount = new Edit(FindElement.ById("CoreWebView2InitializedCount"));
                var ensureCoreWebView2CompletionCount = new Edit(FindElement.ById("EnsureCoreWebView2CompletionCount")); 
                var navigationCompletedCount = new Edit(FindElement.ById("NavigationCompletedCount"));
                var loadedCount = new Edit(FindElement.ById("LoadedCount"));
                var coreProcessFailedCount = new Edit(FindElement.ById("CoreProcessFailedCount"));

                Button Create_CoreProcessFailed_Button = new Button(FindElement.ById("Create_CoreProcessFailed_Button"));
                Button Bad_Source_Browser_Button = new Button(FindElement.ById("Bad_Source_Browser_Button"));
                Button Bad_Source_Render_Button = new Button(FindElement.ById("Bad_Source_Render_Button"));
                Button Good_Source_Button = new Button(FindElement.ById("Good_Source_Button"));
                Button Reload_Button = new Button(FindElement.ById("Reload_Button"));

                BeginSubTest("CoreProccessFailed Event Test", "Set bad source #1 (browser crash) -> Set good source -> Set bad source #2 (tab crash) -> Reload");
                using (var coreProcessFailedWaiter = new ValueChangedEventWaiter(coreProcessFailedCount, "1"))
                {
                    Create_CoreProcessFailed_Button.Invoke();
                    Bad_Source_Browser_Button.Invoke();
                    coreProcessFailedWaiter.Wait();
                }

                if (loadedCount.Value != "1")
                {
                    using (var loadedWaiter = new ValueChangedEventWaiter(loadedCount, "1"))
                    {
                        loadedWaiter.Wait();
                    }
                }

                ValidateEventCounts(
                    1,      // coreWebView2InitializedCountExpected
                    0,      // ensureCoreWebView2CompletionCountExpected
                    0,      // navigationStartingCountExpected
                    0,      // navigationCompletedCountExpected
                    1,      // loadedCountExpected
                    0,      // unloadedCountExpected
                    1,      // coreProcessFailedCountExpected
                    "Bad Source #1 (browser crash)"
                );

                // Make sure we bad source #1 led to WebView2ProcessFailedKind.BrowserProcessExited
                ValidateEventDetail("BrowserProcessExited");

                using (var navigationCompletedWaiter = new ValueChangedEventWaiter(navigationCompletedCount, "1"))
                {
                    Good_Source_Button.Invoke();
                    navigationCompletedWaiter.Wait();
                }

                ValidateEventCounts(
                    2,      // coreWebView2InitializedCountExpected
                    0,      // ensureCoreWebView2CompletionCountExpected
                    1,      // navigationStartingCountExpected
                    1,      // navigationCompletedCountExpected
                    1,      // loadedCountExpected
                    0,      // unloadedCountExpected
                    1,      // coreProcessFailedCountExpected
                    "Good source"
                );

                using (var coreProcessFailedWaiter2 = new ValueChangedEventWaiter(coreProcessFailedCount, "2"))
                {
                    Bad_Source_Render_Button.Invoke();
                    coreProcessFailedWaiter2.Wait();
                }

                ValidateEventCounts(
                    2,      // coreWebView2InitializedCountExpected
                    0,      // ensureCoreWebView2CompletionCountExpected
                    1,      // navigationStartingCountExpected
                    1,      // navigationCompletedCountExpected
                    1,      // loadedCountExpected
                    0,      // unloadedCountExpected
                    2,      // coreProcessFailedCountExpected
                    "Bad Source #2 (tab crash)"
                );

                // Make sure we bad source #2 led to WebView2ProcessFailedKind.RenderProcessExited
                ValidateEventDetail("RenderProcessExited");

                using (var navigationCompletedWaiter2 = new ValueChangedEventWaiter(navigationCompletedCount, "2"))
                {
                    Reload_Button.Invoke();
                    navigationCompletedWaiter2.Wait();
                }

                ValidateEventCounts(
                    2,      // coreWebView2InitializedCountExpected
                    0,      // ensureCoreWebView2CompletionCountExpected
                    2,      // navigationStartingCountExpected
                    2,      // navigationCompletedCountExpected
                    1,      // loadedCountExpected
                    0,      // unloadedCountExpected
                    2,      // coreProcessFailedCountExpected
                    "Reload() [should succeed after tab crash]]"
                );

                EndSubTest("ProccessFailed Test");
            }
        }

        private static class NativeMethods
        {
            public const string LoaderName = "WebView2Loader.dll";
            [DllImport(LoaderName, CharSet = CharSet.Unicode)]
            public static extern int GetAvailableCoreWebView2BrowserVersionString(string browserExecutableFolder, out IntPtr versionInfo);

            public enum HResults : long
            {
                S_OK = 0x0
            }
        }
    }
}

// for our webview rendering verification hack.  Because we don't currently have a RenderTargetBitmap yet,
// we need to use some GDI32 and User32 functions to do it.
namespace WebView2Temporary
{
    using System.Runtime.InteropServices;

    public class WebView2RenderingVerifier
    {
        private static class NativeMethods
        {
            public static uint PW_RENDERFULLCONTENT = 2;
            [DllImport("user32.dll")] public static extern bool PrintWindow(IntPtr hWnd, IntPtr hdc, uint flags);
            [DllImport("user32.dll")] public static extern IntPtr GetDC(IntPtr hWnd);
            [DllImport("user32.dll")] public static extern IntPtr ReleaseDC(IntPtr hWnd, IntPtr hdc);
            [DllImport("user32.dll", CharSet = CharSet.Unicode)] public static extern IntPtr FindWindowEx(IntPtr parentHandle, IntPtr childAfter, string className, string windowTitle);
            [DllImport("user32.dll")] public static extern int GetWindowRect(IntPtr hWnd, ref RECT rectangle);
            [DllImport("user32.dll")] public static extern int GetDpiForWindow([In] IntPtr hWnd);

            [StructLayout(LayoutKind.Sequential)]
            public struct RECT
            {
                public int Left;        // x position of upper-left corner
                public int Top;         // y position of upper-left corner
                public int Right;       // x position of lower-right corner
                public int Bottom;      // y position of lower-right corner
            }

            [DllImport("gdi32.dll")] public static extern IntPtr CreateCompatibleDC(IntPtr hdc);
            [DllImport("gdi32.dll")] public static extern bool DeleteDC(IntPtr hdc);
            [DllImport("gdi32.dll")] public static extern IntPtr CreateCompatibleBitmap(IntPtr hdc, int width, int height);
            [DllImport("gdi32.dll")] public static extern IntPtr SelectObject(IntPtr hdc, IntPtr obj);
            [DllImport("gdi32.dll")] public static extern bool DeleteObject(IntPtr obj);
            [DllImport("gdi32.dll")] public static extern uint GetPixel(IntPtr hdc, int x, int y);
        }
        
        public static void VerifyInstances(string testName)
        {
            // Get HWND of application window
            UIObject root = TestEnvironment.Application.ApplicationFrameWindow ?? TestEnvironment.Application.CoreWindow;
            var windowHandle = root.NativeWindowHandle;

            CheckWindowPixel(windowHandle, testName);
        }

        static void CheckWindowPixel(IntPtr targetHwnd, string testName)
        {
            Log.Comment("Checking hwnd 0x{0:x6} for rendering.", targetHwnd);

            var targetRect = new NativeMethods.RECT();
            _ = NativeMethods.GetWindowRect(targetHwnd, ref targetRect);

            // Create a device context and bitmap consistent with the target hwnd and check the pixel.
            IntPtr targetHdc = NativeMethods.GetDC(targetHwnd);
            if (targetHdc == IntPtr.Zero)
            {
                Log.Warning(string.Format("Test:{0}: Unable to get chrome hdc", testName));
            }
            Verify.AreNotEqual(targetHdc, IntPtr.Zero);
            if (targetHdc != IntPtr.Zero)
            {
                IntPtr memoryHdc = NativeMethods.CreateCompatibleDC(targetHdc);
                if (memoryHdc == IntPtr.Zero)
                {
                    Log.Warning(string.Format("Test:{0}: Failed to create memory DC", testName));
                }
                Verify.AreNotEqual(memoryHdc, IntPtr.Zero);
                if (memoryHdc != IntPtr.Zero)
                {
                    var width = targetRect.Right - targetRect.Left;
                    var height = targetRect.Bottom - targetRect.Top;

                    IntPtr memoryBitmap = NativeMethods.CreateCompatibleBitmap(targetHdc, width, height);
                    if (memoryBitmap == IntPtr.Zero)
                    {
                        Log.Warning(string.Format("Test:{0}: Failed to create bitmap", testName));
                    }
                    Verify.AreNotEqual(memoryBitmap, IntPtr.Zero);
                    if (memoryBitmap != IntPtr.Zero)
                    {
                        NativeMethods.SelectObject(memoryHdc, memoryBitmap);

                        // Print the window into the memory Hdc.
                        NativeMethods.PrintWindow(targetHwnd, memoryHdc, NativeMethods.PW_RENDERFULLCONTENT);

                        var dpi = NativeMethods.GetDpiForWindow(targetHwnd);
                        var scale = dpi / 96.0;

                        // Look at the pixel.
                        const uint targetPixelColor = 0x0080ff; // Orange

                        // The tests that use this verification may not always pass locally, depending on the DPI.
                        // All pass at 100% DPI, and BasicRenderingTest and ReparentElementTest will also pass at 150%.
                        // When we complete Task 25907893 - Adding WebView tests for DPI changes, we will want to ensure that these tests are consistent in either passing or failing at high DPI.
                        Log.Warning(string.Format("These tests may fail locally if monitor DPI is not set to 100%. However, they pass in the lab."));
                        CheckWebViewPixel("MyWebView2", scale, targetPixelColor, memoryHdc);

                        if (testName == "MultipleWebviews_BasicRenderingTest")
                        {
                            CheckWebViewPixel("MyWebView2B", scale, targetPixelColor, memoryHdc);
                            CheckWebViewPixel("MyWebView2C", scale, targetPixelColor, memoryHdc);
                        }
                    }
                    NativeMethods.DeleteDC(memoryHdc);
                }
                NativeMethods.ReleaseDC(targetHwnd, targetHdc);
            }
        }
        static void CheckWebViewPixel(string webViewName, double scale, uint targetPixelColor, IntPtr memoryHdc)
        {
            var webview = FindElement.ById(webViewName);
            Rectangle bounds = webview.BoundingRectangle;

            int xPosition = Convert.ToInt32(50 + (bounds.X * scale));
            int yPosition = Convert.ToInt32(20 + (bounds.Y * scale));
            uint pixelColor = NativeMethods.GetPixel(memoryHdc, xPosition, yPosition);

            Verify.AreEqual<UInt32>(
                pixelColor, targetPixelColor,
                string.Format("Pixel at {0},{1}(actual color is 0x{2:x6}) should match target color of orange (0x{3:x6})",
                    xPosition, yPosition, pixelColor, targetPixelColor));
        }
    }
}
