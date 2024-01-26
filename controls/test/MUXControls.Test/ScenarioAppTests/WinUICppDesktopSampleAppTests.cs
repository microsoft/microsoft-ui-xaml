using Common;
using Microsoft.UI.Xaml.Tests.MUXControls.InteractionTests.Common;
using Microsoft.UI.Xaml.Tests.MUXControls.InteractionTests.Infra;
using Microsoft.Windows.Apps.Test.Automation;
using Microsoft.Windows.Apps.Test.Foundation;
using Microsoft.Windows.Apps.Test.Foundation.Controls;
using Microsoft.Windows.Apps.Test.Foundation.Patterns;
using System;
using System.Collections.Generic;
using System.Linq;
using System.Numerics;
using System.Threading;
using System.Runtime.InteropServices;
using System.Threading.Tasks;

using WEX.TestExecution;
using WEX.TestExecution.Markup;
using WEX.Logging.Interop;

namespace Microsoft.UI.Xaml.Tests.MUXControls.InteractionTests
{
    [TestClass]
    public class WinUICppDesktopSampleAppTests
    {
        public static TestApplicationInfo WinUICppDesktopSampleApp
        {
            get
            {
                return new TestApplicationInfo(
                    "WinUICppDesktopSampleApp",
                    "WinUICppDesktopSampleApp_6f07fta6qpts2!App",
                    "WinUICppDesktopSampleApp_6f07fta6qpts2",
                    "WinUICppDesktopSampleApp",
                    "WinUICppDesktopSampleApp.exe",
                    "WinUICppDesktopSampleApp",
                    isUwpApp: false,
                    TestApplicationInfo.MUXCertSerialNumber,
                    TestApplicationInfo.MUXBaseAppxDir);
            }
        }

        [ClassInitialize]
        [TestProperty("RunAs", "User")]
        [TestProperty("Classification", "ScenarioTestSuite")]
        [TestProperty("IgnoreForValidateWindowsAppSDK", "True")]
        [TestProperty("Platform", "Any")]
        [TestProperty("IsolationLevel", "Method")] // Task 32517851: WinUICppDesktopSampleAppTests and WinUICsDesktopSampleAppTests must run with isolationmode=test due to insufficient test cleanup
        public static void ClassInitialize(TestContext testContext)
        {
            TestEnvironment.Initialize(testContext, WinUICppDesktopSampleApp);
        }

        [TestCleanup]
        public void TestCleanup()
        {
        }

        [ClassCleanupAttribute]
        public static void ClassCleanup()
        {
            TestEnvironment.AssemblyCleanupWorker(WinUICppDesktopSampleApp);
        }

        public void WindowChromeCommonSetup()
        {
            Log.Comment("The test harness maximizes the window already; restoring it to normal size for this test");
            TestHelpers.RestoreWindow();
            Log.Comment("window restored");
            // Because it is possible for us to drag the title bar during a test, we need to ensure that we are positioned so that we don't
            // hit the right edge and either go off of it or trigger the docking logic.  So we will just position the window at 50,50.
            TestHelpers.MoveWindow(50, 50);
            // there is test flakyness due to Input automation not working because focus is not present
            // to reduce test flakyness, we will toggle checkbox state and clear it
            // as a result, focus will move to xaml ui
            WinUISampleAppTestsUtils.SelectCheckbox("checkBox"); 
            WinUISampleAppTestsUtils.SelectCheckbox("checkBox"); 
        }

        [TestMethod]
        [Description("Basic tests of a few MUX controls: TextBlock, CheckBox, Button, ToggleButton, Flyout.")]
        [TestProperty("Ignore", "True")]    // http://task.ms/45771651
        public void MUXControlsTest()
        {
            var textBlock = new TextBlock(FindElement.ByName("textBlockMUXControls"));
            Verify.IsNotNull(textBlock);

            TestEnvironment.VerifyAreEqualWithRetry(20,
                () => "Loaded",
                () => textBlock.DocumentText);

            Log.Comment("Testing Button control: Clicking button");
            var button = new Button(FindElement.ByName("button"));
            Verify.IsNotNull(button);
            button.Invoke();

            Log.Comment("Wait For Idle");
            Wait.ForIdle();

            TestEnvironment.VerifyAreEqualWithRetry(20,
                () => "Button.Click",
                () => textBlock.DocumentText);


            Log.Comment("Testing ToggleButton control: Toggling toggleButton");
            var toggleButton = new ToggleButton(FindElement.ByName("toggleButton"));
            Verify.IsNotNull(toggleButton);

            using (var toggleWaiter = toggleButton.GetToggledWaiter())
            {
                toggleButton.Toggle();
                toggleWaiter.Wait();
            }

            Verify.IsTrue(toggleButton.ToggleState == ToggleState.On);

            Log.Comment("Wait For Idle");
            Wait.ForIdle();

            TestEnvironment.VerifyAreEqualWithRetry(20,
                () => "ToggleButton.Click - IsChecked=True",
                () => textBlock.DocumentText);

            var checkBox = new CheckBox(FindElement.ByName("checkBox"));

            TestEnvironment.VerifyAreEqualWithRetry(20,
                () => ToggleState.On,
                () => checkBox.ToggleState);

            Log.Comment("Toggling toggleButton again");
            using (var toggleWaiter = toggleButton.GetToggledWaiter())
            {
                toggleButton.Toggle();
                toggleWaiter.Wait();
            }

            Verify.IsTrue(toggleButton.ToggleState == ToggleState.Off);

            Log.Comment("Wait For Idle");
            Wait.ForIdle();

            TestEnvironment.VerifyAreEqualWithRetry(20,
                () => "ToggleButton.Click - IsChecked=False",
                () => textBlock.DocumentText);

            TestEnvironment.VerifyAreEqualWithRetry(20,
                () => ToggleState.Off,
                () => checkBox.ToggleState);


            Log.Comment("Testing Button-with-Flyout control: Clicking buttonWithFlyout");
            var buttonWithFlyout = new Button(FindElement.ByName("buttonWithFlyout"));
            Verify.IsNotNull(buttonWithFlyout);
            buttonWithFlyout.Invoke();

            Log.Comment("Wait For Idle");
            Wait.ForIdle();

            TestEnvironment.VerifyAreEqualWithRetry(20,
                () => "Flyout.Opened - buttonWithFlyout",
                () => textBlock.DocumentText);

            Log.Comment("Discarding flyout");
            buttonWithFlyout.Click();

            Log.Comment("Wait For Idle");
            Wait.ForIdle();

            TestEnvironment.VerifyAreEqualWithRetry(20,
                () => "Flyout.Closed - buttonWithFlyout",
                () => textBlock.DocumentText);


            Log.Comment("Testing Button-with-ContextFlyout control: Right-Clicking buttonWithContextFlyout");
            var buttonWithContextFlyout = new Button(FindElement.ByName("buttonWithContextFlyout"));
            Verify.IsNotNull(buttonWithContextFlyout);
            buttonWithContextFlyout.Click(PointerButtons.Secondary);

            Log.Comment("Wait For Idle");
            Wait.ForIdle();

            TestEnvironment.VerifyAreEqualWithRetry(20,
                () => "Flyout.Opened - buttonWithContextFlyout",
                () => textBlock.DocumentText);

            Log.Comment("Discarding flyout");
            // Note: Clicking on the center of buttonWithContextFlyout won't dismiss the context menu. The menu is a
            // windowed popup, and the shadow region will eat the click targeted at the center of
            // buttonWithContextFlyout. Click off to the left instead.
            buttonWithContextFlyout.Click(PointerButtons.Primary, 15, buttonWithContextFlyout.BoundingRectangle.Height / 2);

            Log.Comment("Wait For Idle");
            Wait.ForIdle();

            TestEnvironment.VerifyAreEqualWithRetry(20,
                () => "Flyout.Closed - buttonWithContextFlyout",
                () => textBlock.DocumentText);

            TestEnvironment.Application.Close(); // Ensure a crash will fail this test.
        }

        [TestMethod]
        [Description("Basic tests of a few MUXC controls: SplitButton, DropDownButton, RatingControl, TreeView, ColorPicker.")]
        public void MUXCControlsTest()
        {
            SelectMUXCControlsUI();

            var textBlock = new TextBlock(FindElement.ByName("textBlockMUXCControls"));
            Verify.IsNotNull(textBlock);

            var splitButton = new SplitButton(FindElement.ByName("splitButton"));
            Verify.IsNotNull(splitButton);

            Log.Comment("Clicking SplitButton primary button area");
            splitButton.Click(PointerButtons.Primary, 5, splitButton.BoundingRectangle.Height / 2);

            Log.Comment("Wait For Idle");
            Wait.ForIdle();

            TestEnvironment.VerifyAreEqualWithRetry(20,
                () => "SplitButton.Click",
                () => textBlock.DocumentText);

            Log.Comment("Clicking SplitButton secondary button area");
            splitButton.Click(PointerButtons.Primary, splitButton.BoundingRectangle.Width - 5, splitButton.BoundingRectangle.Height / 2);

            Log.Comment("Wait For Idle");
            Wait.ForIdle();

            TestEnvironment.VerifyAreEqualWithRetry(20,
                () => "SplitButtonFlyout.Opened",
                () => textBlock.DocumentText);

            Log.Comment("Clicking SplitButton secondary button area again to close expanded list");
            splitButton.Click(PointerButtons.Primary, splitButton.BoundingRectangle.Width - 5, splitButton.BoundingRectangle.Height / 2);
            Wait.ForIdle();

            Log.Comment("Clicking DropDownButton");
            var dropDownButton = new Button(FindElement.ByName("dropDownButton"));
            Verify.IsNotNull(dropDownButton);

            dropDownButton.Invoke();
            Wait.ForIdle();

            TestEnvironment.VerifyAreEqualWithRetry(20,
                () => "DropDownButton.Click",
                () => textBlock.DocumentText);

            TestEnvironment.Application.Close(); // Ensure a crash will fail this test.
        }

        private void SelectMUXCControlsUI()
        {
            Log.Comment("Selecting MUXC Controls tab");
            var tviMUXCControls = FindElement.ByName("tviMUXCControls");
            Verify.IsNotNull(tviMUXCControls);
            SelectItem(tviMUXCControls);

            Log.Comment("Wait For Idle");
            Wait.ForIdle();

            Log.Comment("Verifying content is displayed for MUXC Controls tab");
            var stackPanelMUXCControls = FindElement.ByName("stackPanelMUXCControls");
            Verify.IsNotNull(stackPanelMUXCControls);
        }

        [TestMethod]
        [Description("Basic tests of UserControl defined in a separate runtime component.")]
        public void ExternalControlsTest()
        {
            SelectExternalControlsUI();

            var textBlock = new TextBlock(FindElement.ByName("textBlockExternalControls"));
            Verify.IsNotNull(textBlock);

            var userControl1 = FindElement.ByName("userControl1");
            Verify.IsNotNull(userControl1);

            Log.Comment("Selecting PivotItem");
            userControl1.Click(PointerButtons.Primary, 125, userControl1.BoundingRectangle.Height / 2);

            Log.Comment("Wait For Idle");
            Wait.ForIdle();

            TestEnvironment.Application.Close(); // Ensure a crash will fail this test.
        }

        private void SelectExternalControlsUI()
        {
            Log.Comment("Selecting External Controls tab");
            var tviExternalControls = FindElement.ByName("tviExternalControls");
            Verify.IsNotNull(tviExternalControls);
            SelectItem(tviExternalControls);

            Log.Comment("Wait For Idle");
            Wait.ForIdle();

            Log.Comment("Verifying content is displayed for External Controls tab");
            var stackPanelExternalControls = FindElement.ByName("stackPanelExternalControls");
            Verify.IsNotNull(stackPanelExternalControls);
        }

        [TestMethod]
        [Description("Read Window properties for the MainWindow.")]
        public void ReadWindowPropertiesTest()
        {
            SelectWindowUI();
            ClearWindow();
            WinUISampleAppTestsUtils.ClearErrorReport();

            ReadWindowBounds();

            WinUISampleAppTestsUtils.ResetText("buttonResetVisible", "textBlockVisible");
            WinUISampleAppTestsUtils.VerifyPropertyAccessor("buttonGetVisible", "textBlockVisible", "True");

            WinUISampleAppTestsUtils.ResetText("buttonResetContent", "textBlockContent");
            WinUISampleAppTestsUtils.VerifyPropertyAccessor("buttonGetContent", "textBlockContent", "stackPanelRoot");

            WinUISampleAppTestsUtils.ResetText("buttonResetTitle", "textBoxTitle");
            WinUISampleAppTestsUtils.VerifyPropertyAccessor("buttonGetTitle", "textBoxTitle", "WinUICppDesktopSampleApp");

            TestEnvironment.Application.Close(); // Ensure a crash will fail this test.
        }

        [TestMethod]
        [Description("Change the Title property of the MainWindow.")]
        public void ChangeWindowTitleTest()
        {
            SelectWindowUI();
            ClearWindow();

            WinUISampleAppTestsUtils.ClearErrorReport();
            WinUISampleAppTestsUtils.ResetText("buttonResetTitle", "textBoxTitle");
            WinUISampleAppTestsUtils.SetEditText("textBoxTitle", "New Title");
            WinUISampleAppTestsUtils.InvokeButton("buttonSetTitle");
            WinUISampleAppTestsUtils.VerifyPropertyAccessor("buttonGetTitle", "textBoxTitle", "New Title");

            WinUISampleAppTestsUtils.ClearErrorReport();
            WinUISampleAppTestsUtils.ResetText("buttonResetTitle", "textBoxTitle");
            WinUISampleAppTestsUtils.SetEditText("textBoxTitle", "WinUICppDesktopSampleApp");
            WinUISampleAppTestsUtils.InvokeButton("buttonSetTitle");
            WinUISampleAppTestsUtils.VerifyPropertyAccessor("buttonGetTitle", "textBoxTitle", "WinUICppDesktopSampleApp");

            TestEnvironment.Application.Close(); // Ensure a crash will fail this test.
        }

        [TestMethod]
        [Description("Read the IWindowNative's HWND for the MainWindow.")]
        public void ReadMainWindowHandleTest()
        {
            SelectWindowUI();
            ClearWindow();
            WinUISampleAppTestsUtils.ClearErrorReport();
            WinUISampleAppTestsUtils.ResetText("buttonResetHandle", "textBlockHandle");
            WinUISampleAppTestsUtils.InvokeButton("buttonGetHandle");
            WinUISampleAppTestsUtils.VerifyPropertyAccessor("buttonGetHandle", "textBlockHandle", null);

            TestEnvironment.Application.Close(); // Ensure a crash will fail this test.
        }

        [TestMethod]
        [Description("Resize the MainWindow using the IWindowNative's HWND.")]
        public void ResizeMainWindowTest()
        {
            SelectWindowUI();
            ClearWindow();
            WinUISampleAppTestsUtils.ClearErrorReport();

            Log.Comment("Reading original window size");
            ReadWindowBounds();

            var text = new TextBlock(FindElement.ByName("textBlockBoundsWidth"));
            Verify.IsNotNull(text);
            float originalWidth = Convert.ToSingle(text.DocumentText);

            text = new TextBlock(FindElement.ByName("textBlockBoundsHeight"));
            Verify.IsNotNull(text);
            float originalHeight = Convert.ToSingle(text.DocumentText);

            Log.Comment("Using HWND and SetWindowPos with 1000 and 700");
            WinUISampleAppTestsUtils.SetEditText("textBoxWidth", "1000");
            WinUISampleAppTestsUtils.SetEditText("textBoxHeight", "700");
            WinUISampleAppTestsUtils.InvokeButton("buttonSetSize");
            WinUISampleAppTestsUtils.VerifyNoErrorReport();

            Log.Comment("Reading new window size");
            ReadWindowBounds();

            text = new TextBlock(FindElement.ByName("textBlockBoundsWidth"));
            Verify.IsNotNull(text);
            float width = Convert.ToSingle(text.DocumentText);

            text = new TextBlock(FindElement.ByName("textBlockBoundsHeight"));
            Verify.IsNotNull(text);
            float height = Convert.ToSingle(text.DocumentText);

            Log.Comment("Using HWND and SetWindowPos to restore original size");
            int newWidth = (int)(originalWidth + 1000.0f - width);
            int newHeight = (int)(originalHeight + 700.0f - height);

            WinUISampleAppTestsUtils.SetEditText("textBoxWidth", newWidth.ToString());
            WinUISampleAppTestsUtils.SetEditText("textBoxHeight", newHeight.ToString());
            WinUISampleAppTestsUtils.InvokeButton("buttonSetSize");
            WinUISampleAppTestsUtils.VerifyNoErrorReport();

            Log.Comment("Verifying original and final sizes are the same");
            ReadWindowBounds();

            text = new TextBlock(FindElement.ByName("textBlockBoundsWidth"));
            Verify.IsNotNull(text);
            float finalWidth = Convert.ToSingle(text.DocumentText);

            text = new TextBlock(FindElement.ByName("textBlockBoundsHeight"));
            Verify.IsNotNull(text);
            float finalHeight = Convert.ToSingle(text.DocumentText);

            Verify.AreEqual(originalWidth, finalWidth);
            Verify.AreEqual(originalHeight, finalHeight);

            TestEnvironment.Application.Close(); // Ensure a crash will fail this test.
        }

        private void ReadWindowBounds()
        {
            WinUISampleAppTestsUtils.ResetText("buttonResetBounds", "textBlockBoundsHeight");
            WinUISampleAppTestsUtils.InvokeButton("buttonGetBounds");
            WinUISampleAppTestsUtils.VerifyText("textBlockBoundsX", "0.000000");
            WinUISampleAppTestsUtils.VerifyText("textBlockBoundsY", "0.000000");
            WinUISampleAppTestsUtils.VerifyText("textBlockBoundsWidth", null);
            WinUISampleAppTestsUtils.VerifyText("textBlockBoundsHeight", null);
            WinUISampleAppTestsUtils.VerifyNoErrorReport();
        }

        [TestMethod]
        [Description("Activate and close a new Window with XAML markup.")]
        public void ActivateAndCloseMarkupWindowTest()
        {
            ActivateAndCloseRuntimeWindow(isForRuntimeWindow: false);
        }

        [TestMethod]
        [Description("Activate and close a new Window.")]
        public void ActivateAndCloseRuntimeWindowTest()
        {
            ActivateAndCloseRuntimeWindow(isForRuntimeWindow: true);
        }

        public void ActivateAndCloseRuntimeWindow(bool isForRuntimeWindow)
        {
            SelectWindowUI();
            ClearWindow();
            WinUISampleAppTestsUtils.ClearErrorReport();
            WinUISampleAppTestsUtils.ClearLogEvents();

            if (!isForRuntimeWindow)
            {
                WinUISampleAppTestsUtils.SelectRadioButton("radioButtonMarkupWindow");
            }

            WinUISampleAppTestsUtils.InvokeButton("buttonCreateWindow");
            WinUISampleAppTestsUtils.VerifyText("textBoxLastEvent", "ButtonCreateWindow_Click - exit");
            WinUISampleAppTestsUtils.VerifyNoErrorReport();
            WinUISampleAppTestsUtils.InvokeButton("buttonActivateWindow");
            WinUISampleAppTestsUtils.VerifyText("textBoxLastEvent", "Window_Activated - Window=Main, Handled=False, WindowActivationState=0");
            WinUISampleAppTestsUtils.VerifyNoErrorReport();
            WinUISampleAppTestsUtils.ResetText("buttonResetVisible", "textBlockVisible");
            WinUISampleAppTestsUtils.VerifyPropertyAccessor("buttonGetVisible", "textBlockVisible", "True");
            WinUISampleAppTestsUtils.InvokeButton("buttonCloseWindow");
            WinUISampleAppTestsUtils.VerifyText("textBoxLastEvent", "ButtonCloseWindow_Click - exit");
            WinUISampleAppTestsUtils.VerifyNoErrorReport();
            WinUISampleAppTestsUtils.InvokeButton("buttonDiscardWindow");
            WinUISampleAppTestsUtils.VerifyText("textBoxLastEvent", "ButtonDiscardWindow_Click - exit");
            WinUISampleAppTestsUtils.VerifyNoErrorReport();

            List<string> expectedLogEvents = new List<string>();
            expectedLogEvents.Add("ButtonCreateWindow_Click - entry");
            if (isForRuntimeWindow)
            {
                expectedLogEvents.Add("ButtonCreateWindow_Click - setting Window.Content");
            }
            expectedLogEvents.Add("ButtonCreateWindow_Click - exit");
            expectedLogEvents.Add("ButtonActivateWindow_Click - entry");
            expectedLogEvents.Add("Window_Activated - Window=Main, Handled=False, WindowActivationState=1");
            expectedLogEvents.Add("Window_Activated - Window=Secondary, Handled=False, WindowActivationState=0");
            expectedLogEvents.Add("Window_VisibilityChanged - Handled=False, Visible=True");
            expectedLogEvents.Add("ButtonActivateWindow_Click - exit");
            expectedLogEvents.Add("Window_Activated - Window=Secondary, Handled=False, WindowActivationState=1");
            expectedLogEvents.Add("Window_Activated - Window=Main, Handled=False, WindowActivationState=0");
            expectedLogEvents.Add("ButtonCloseWindow_Click - entry");
            expectedLogEvents.Add("Window_Closed - Handled=False");
            expectedLogEvents.Add("Window_VisibilityChanged - Handled=False, Visible=False");
            expectedLogEvents.Add("ButtonCloseWindow_Click - exit");
            expectedLogEvents.Add("ButtonDiscardWindow_Click - entry");
            expectedLogEvents.Add("ButtonDiscardWindow_Click - exit");
            WinUISampleAppTestsUtils.VerifyLogEvents(expectedLogEvents.ToArray());

            TestEnvironment.Application.Close(); // Ensure a crash will fail this test.
        }

        [TestMethod]
        [Description("Activate a new Window and verify events")]
        public void ActivateMarkupWindowToggleActivateButtonsAndVerifyEvents()
        {
            SelectWindowUI();
            ClearWindow();
            WinUISampleAppTestsUtils.ClearErrorReport();
            WinUISampleAppTestsUtils.ClearLogEvents();

            WinUISampleAppTestsUtils.SelectRadioButton("radioButtonMarkupWindow");
            WinUISampleAppTestsUtils.InvokeButton("buttonCreateWindow");
            WinUISampleAppTestsUtils.VerifyText("textBoxLastEvent", "ButtonCreateWindow_Click - exit");
            WinUISampleAppTestsUtils.VerifyNoErrorReport();
            WinUISampleAppTestsUtils.InvokeButton("buttonActivateWindow");
            WinUISampleAppTestsUtils.VerifyText("textBoxLastEvent", "Window_Activated - Window=Main, Handled=False, WindowActivationState=0");
            WinUISampleAppTestsUtils.VerifyNoErrorReport();

            UIObject markupWindowRoot = FindElement.GetDesktopTopLevelWindow("WinUI Desktop - Markup Window");
            Verify.IsNotNull(markupWindowRoot);

            WinUISampleAppTestsUtils.InvokeButtonForRoot(markupWindowRoot, "buttonActivateMainWindow");
            WinUISampleAppTestsUtils.VerifyText("textBoxLastEvent", "ButtonActivateMainWindow_Click - exit");
            WinUISampleAppTestsUtils.VerifyNoErrorReport();
            WinUISampleAppTestsUtils.InvokeButton("buttonCloseWindow");
            WinUISampleAppTestsUtils.VerifyText("textBoxLastEvent", "ButtonCloseWindow_Click - exit");
            WinUISampleAppTestsUtils.VerifyNoErrorReport();
            WinUISampleAppTestsUtils.InvokeButton("buttonDiscardWindow");
            WinUISampleAppTestsUtils.VerifyText("textBoxLastEvent", "ButtonDiscardWindow_Click - exit");
            WinUISampleAppTestsUtils.VerifyNoErrorReport();

            List<string> expectedLogEvents = new List<string>();
            expectedLogEvents.Add("ButtonCreateWindow_Click - entry");
            expectedLogEvents.Add("ButtonCreateWindow_Click - exit");
            expectedLogEvents.Add("ButtonActivateWindow_Click - entry");
            expectedLogEvents.Add("Window_Activated - Window=Main, Handled=False, WindowActivationState=1");
            expectedLogEvents.Add("Window_Activated - Window=Secondary, Handled=False, WindowActivationState=0");
            expectedLogEvents.Add("Window_VisibilityChanged - Handled=False, Visible=True");
            expectedLogEvents.Add("ButtonActivateWindow_Click - exit");

            // these events occur (from observation) because of focus changes
            // first: to MainWindow in order to read textBoxLastEvent
            expectedLogEvents.Add("Window_Activated - Window=Secondary, Handled=False, WindowActivationState=1");
            expectedLogEvents.Add("Window_Activated - Window=Main, Handled=False, WindowActivationState=0");
            // second: back to MarkupWindow to click buttonActivateMainWindow
            expectedLogEvents.Add("Window_Activated - Window=Main, Handled=False, WindowActivationState=1");
            expectedLogEvents.Add("Window_Activated - Window=Secondary, Handled=False, WindowActivationState=0");

            expectedLogEvents.Add("ButtonActivateMainWindow_Click - entry");
            expectedLogEvents.Add("Window_Activated - Window=Secondary, Handled=False, WindowActivationState=1");
            expectedLogEvents.Add("Window_Activated - Window=Main, Handled=False, WindowActivationState=0");
            expectedLogEvents.Add("ButtonActivateMainWindow_Click - exit");
            expectedLogEvents.Add("ButtonCloseWindow_Click - entry");
            expectedLogEvents.Add("Window_Closed - Handled=False");
            expectedLogEvents.Add("Window_VisibilityChanged - Handled=False, Visible=False");
            expectedLogEvents.Add("ButtonCloseWindow_Click - exit");
            expectedLogEvents.Add("ButtonDiscardWindow_Click - entry");
            expectedLogEvents.Add("ButtonDiscardWindow_Click - exit");
            WinUISampleAppTestsUtils.VerifyLogEvents(expectedLogEvents.ToArray());

            TestEnvironment.Application.Close(); // Ensure a crash will fail this test.
        }

        [TestMethod]
        [Description("Window lifetime test.")]
        public void ClosingAllWindowsMustExitApp()
        {
            SelectWindowUI();
            ClearWindow();
            WinUISampleAppTestsUtils.ClearErrorReport();
            WinUISampleAppTestsUtils.ClearLogEvents();
            int processID = TestEnvironment.Application.GetProcessIdFromAppWindow();

            WinUISampleAppTestsUtils.SelectRadioButton("radioButtonMarkupWindow");
            WinUISampleAppTestsUtils.InvokeButton("buttonSetWindowFree");

            WinUISampleAppTestsUtils.InvokeButton("buttonCreateWindow");
            WinUISampleAppTestsUtils.VerifyText("textBoxLastEvent", "ButtonCreateWindow_Click - exit");
            WinUISampleAppTestsUtils.VerifyNoErrorReport();
            WinUISampleAppTestsUtils.InvokeButton("buttonActivateWindow");
            WinUISampleAppTestsUtils.VerifyText("textBoxLastEvent", "Window_Activated - Window=Main, Handled=False, WindowActivationState=0");
            WinUISampleAppTestsUtils.VerifyNoErrorReport();

            UIObject markupWindowRoot = FindElement.GetDesktopTopLevelWindow("WinUI Desktop - Markup Window");
            Verify.IsNotNull(markupWindowRoot);

            WinUISampleAppTestsUtils.InvokeButtonForRoot(markupWindowRoot, "buttonActivateMainWindow");
            WinUISampleAppTestsUtils.VerifyText("textBoxLastEvent", "ButtonActivateMainWindow_Click - exit");
            WinUISampleAppTestsUtils.VerifyNoErrorReport();

            // Alt+f4 to close only MainAppWindow:
            Log.Comment("Verify that pressing Alt-f4 closes the Main App Window");
            KeyboardHelper.PressKey(Key.F4, ModifierKey.Alt, 1 /*Num of Presses*/, true /*skip wait*/);

            // Skip wait for idle as closing the window will cause it to fail, instead the test will wait for a fixed amount of time
            Wait.ForMilliseconds(100);

            // Alt+f4 to close the created top level Window:
            Log.Comment("Verify that pressing Alt-f4 closes the created Window");
            KeyboardHelper.PressKey(Key.F4, ModifierKey.Alt, 1 /*Num of Presses*/, true /*skip wait*/);

            bool isClosed = false;
            Wait.RetryUntilEvalFuncSuccessOrTimeout(
                () =>
                {
                    isClosed = TestHelpers.IsWindowClosed(processID);
                    return isClosed;
                },
                retryTimoutByMilliseconds: 1000
            );

            // Log.Comment("Verify that app has been exited successfully");
            Verify.IsTrue(isClosed);
            TestEnvironment.Application.WaitForExit(); // Ensure a crash will fail this test.
        }

        [TestMethod]
        [Description("Window lifetime test.")]
        public void ExitAppMustCloseAllOpenWindows()
        {
            SelectWindowUI();
            ClearWindow();
            WinUISampleAppTestsUtils.ClearErrorReport();
            WinUISampleAppTestsUtils.ClearLogEvents();
            int processID = TestEnvironment.Application.GetProcessIdFromAppWindow();

            WinUISampleAppTestsUtils.SelectRadioButton("radioButtonMarkupWindow");
            WinUISampleAppTestsUtils.InvokeButton("buttonSetWindowFree");

            WinUISampleAppTestsUtils.InvokeButton("buttonCreateWindow");
            WinUISampleAppTestsUtils.VerifyText("textBoxLastEvent", "ButtonCreateWindow_Click - exit");
            WinUISampleAppTestsUtils.VerifyNoErrorReport();
            WinUISampleAppTestsUtils.InvokeButton("buttonActivateWindow");
            WinUISampleAppTestsUtils.VerifyText("textBoxLastEvent", "Window_Activated - Window=Main, Handled=False, WindowActivationState=0");
            WinUISampleAppTestsUtils.VerifyNoErrorReport();

            UIObject markupWindowRoot = FindElement.GetDesktopTopLevelWindow("WinUI Desktop - Markup Window");
            Verify.IsNotNull(markupWindowRoot);

            WinUISampleAppTestsUtils.InvokeButtonForRoot(markupWindowRoot, "buttonActivateMainWindow");
            WinUISampleAppTestsUtils.VerifyText("textBoxLastEvent", "ButtonActivateMainWindow_Click - exit");
            WinUISampleAppTestsUtils.VerifyNoErrorReport();

            WinUISampleAppTestsUtils.InvokeButton("buttonExitAppFromWinUI", true /*Skip Wait*/);

            bool isClosed = false;
            Wait.RetryUntilEvalFuncSuccessOrTimeout(
                () =>
                {
                    isClosed = TestHelpers.IsWindowClosed(processID);
                    return isClosed;
                },
                retryTimoutByMilliseconds: 1000
            );

            // Log.Comment("Verify that app has been exited successfully");
            Verify.IsTrue(isClosed);
            TestEnvironment.Application.WaitForExit(); // Ensure a crash will fail this test.
        }

        [TestMethod]
        [Description("CustomWindow local instance should keep window alive.")]
        public void LocalCustomWindowLifeTime()
        {
            SelectWindowUI();
            ClearWindow();
            WinUISampleAppTestsUtils.ClearErrorReport();
            WinUISampleAppTestsUtils.ClearLogEvents();
            int processID = TestEnvironment.Application.GetProcessIdFromAppWindow();

            WinUISampleAppTestsUtils.SelectRadioButton("radioButtonBlankCustomWindow");

            WinUISampleAppTestsUtils.InvokeButton("buttonCreateWindow");
            WinUISampleAppTestsUtils.VerifyNoErrorReport();

            UIObject CustomWindowRoot = FindElement.GetDesktopTopLevelWindow("WinUI Desktop - Custom Window");
            Verify.IsNotNull(CustomWindowRoot);

            // Alt+Tab to bring CustomWindow front
            Log.Comment("Alt+Tab to bring CustomWindow front");
            KeyboardHelper.PressKey(Key.Tab, ModifierKey.Alt, 1 /*Num of Presses*/, true /*skip wait*/);
            Wait.ForMilliseconds(100);

            // Alt+f4 to close Customwindow
            Log.Comment("Verify that pressing Alt-f4 closes the CustomWindow");
            KeyboardHelper.PressKey(Key.F4, ModifierKey.Alt, 1 /*Num of Presses*/, true /*skip wait*/);

            // CustomWindow destructor calls Application::Exit. Below code will verify that all windows are closed and app exits gracefully.
            bool isClosed = false;
            Wait.RetryUntilEvalFuncSuccessOrTimeout(
                () =>
                {
                    isClosed = TestHelpers.IsWindowClosed(processID);
                    return isClosed;
                },
                retryTimoutByMilliseconds: 1000
            );

            // Log.Comment("Verify that app has been exited successfully");
            Verify.IsTrue(isClosed);
            TestEnvironment.Application.WaitForExit(); // Ensure a crash will fail this test.
        }

        [TestMethod]
        [Description("This test tests the event order on minimize, restore, maximize for Main Window")]
        [TestProperty("Ignore", "True")]    // 32252939: event order has changed during WinUI Desktop
        public void WindowChromeTestMainWindowEvents()
        {
            WindowChromeCommonSetup();
            SelectWindowUI();
            ClearWindow();
            WinUISampleAppTestsUtils.ClearErrorReport();
            WinUISampleAppTestsUtils.ClearLogEvents();

            WinUISampleAppTestsUtils.SelectRadioButton("radioButtonMainWindow");

            WinUISampleAppTestsUtils.InvokeButton("buttonCreateWindow");
            WinUISampleAppTestsUtils.VerifyText("textBoxLastEvent", "ButtonCreateWindow_Click - exit");
            WinUISampleAppTestsUtils.VerifyNoErrorReport();
            TestHelpers.MinimizeWindow();

            // TODO: Task 32252939: Test Failure: event order has changed during WinUI Desktop window minimization
            // The last event fired was XamlRoot_Changed at the time of the 0.5 GA release
            //WinUISampleAppTestsUtils.VerifyText("textBoxLastEvent", "XamlRoot_Changed event");
            WinUISampleAppTestsUtils.VerifyText("textBoxLastEvent", "Window_Activated - Window=Main, Handled=False, WindowActivationState=1");

            TestHelpers.RestoreWindow();
            WinUISampleAppTestsUtils.VerifyText("textBoxLastEvent", "Window_Activated - Window=Main, Handled=False, WindowActivationState=0");
            TestHelpers.MaximizeWindow();
            TestHelpers.RestoreWindow();
            WinUISampleAppTestsUtils.VerifyNoErrorReport();

            List<string> expectedLogEvents = new List<string>();
            expectedLogEvents.Add("ButtonCreateWindow_Click - entry");
            expectedLogEvents.Add("ButtonCreateWindow_Click - exit");
            expectedLogEvents.Add("Window_VisibilityChanged - Handled=False, Visible=False");

            // TODO: Task 32252939: Test Failure: event order has changed during WinUI Desktop window minimization
            // These next two events were in the opposite order for 0.5 GA release
            expectedLogEvents.Add("XamlRoot_Changed event");
            expectedLogEvents.Add("Window_Activated - Window=Main, Handled=False, WindowActivationState=1");

            expectedLogEvents.Add("Window_Activated - Window=Main, Handled=False, WindowActivationState=0");
            expectedLogEvents.Add("Window_VisibilityChanged - Handled=False, Visible=True");
            expectedLogEvents.Add("Window_Activated - Window=Main, Handled=False, WindowActivationState=0");
            expectedLogEvents.Add("XamlRoot_Changed event");
            expectedLogEvents.Add("Window_Activated - Window=Main, Handled=False, WindowActivationState=1");
            expectedLogEvents.Add("Window_Activated - Window=Main, Handled=False, WindowActivationState=0");
            expectedLogEvents.Add("Window_SizeChanged - Handled=False");
            expectedLogEvents.Add("XamlRoot_Changed event");
            expectedLogEvents.Add("Window_SizeChanged - Handled=False");
            expectedLogEvents.Add("XamlRoot_Changed event");
            WinUISampleAppTestsUtils.VerifyLogEvents(expectedLogEvents.ToArray());

            TestEnvironment.Application.Close(); // Ensure a crash will fail this test.
        }

        [TestMethod]
        [Description("Test that on setting Window closed event as handled, window doesn't close on clicking close button")]
         public void TestHandlingClosedEventPreventsWindowFromClosing()
        {
            UIObject root = TestEnvironment.Application.ApplicationFrameWindow ?? TestEnvironment.Application.CoreWindow;
            int processID = TestEnvironment.Application.GetProcessIdFromAppWindow();
            WindowChromeCommonSetup();
            SelectWindowUI();

            WinUISampleAppTestsUtils.SelectCheckbox("closingCheckbox"); // check
            TestHelpers.TryCaptionButtonLeftClick(TestHelpers.CaptionButtons.Close, true);
            Wait.ForSeconds(1);
            Verify.IsFalse(TestHelpers.IsWindowClosed(processID), "Window did not close because event has been handled as true");

            WinUISampleAppTestsUtils.SelectCheckbox("closingCheckbox"); //uncheck
            TestHelpers.TryCaptionButtonLeftClick(TestHelpers.CaptionButtons.Close, true);
            Wait.ForSeconds(3); // instead the test will wait for a fixed amount of time
            Verify.IsTrue(TestHelpers.IsWindowClosed(processID));  // don't add a retry here as certain bugs will missed
            Log.Comment("Window has been closed successfully");

            TestEnvironment.Application.Close(); // Ensure a crash will fail this test.
        }

        [TestMethod]
        [Description("Validate the running cleanup code in DispatcherQueue.ShutdownStarting handler is safe.")]
         public void RunCleanupCodeInShutdownStarting()
        {
            UIObject root = TestEnvironment.Application.ApplicationFrameWindow ?? TestEnvironment.Application.CoreWindow;
            WindowChromeCommonSetup();
            SelectWindowUI();

            Log.Comment("Check cleanupCheckbox");
            WinUISampleAppTestsUtils.SelectCheckbox("cleanupCheckbox");
            Log.Comment("Clicking Close button");
            TestHelpers.TryCaptionButtonLeftClick(TestHelpers.CaptionButtons.Close, true);
            // The validation here is that the test process doesn't crash during shutdown.
            TestEnvironment.Application.WaitForExit();
        }

        [TestMethod]
        [Description("Performs simple verification of x:Uid property substitution")]
        public void SimpleXUidVerification()
        {
            // Verify string on Page 1 (exercises x:Uid)
            WinUISampleAppTestsUtils.VerifyText("titleTextBlock", "Welcome to Page 1!");
            WinUISampleAppTestsUtils.VerifyText("explicitAttachedPropertyViaXUidTextBlock", "AutomationProperties.Name set via x:Uid with [using:Microsoft.UI.Xaml.Automation]");
            WinUISampleAppTestsUtils.VerifyText("implicitAttachedPropertyViaXUidTextBlock", "AutomationProperties.Name set via x:Uid without [using:Microsoft.UI.Xaml.Automation]");

            Log.Comment("Navigating to Page 2");
            var button = new Button(FindElement.ByName("navigationButton"));
            button.Invoke();
            Log.Comment("Wait For Idle");
            Wait.ForIdle();

            ElementCache.Clear();

            // Verify string on Page 2 (exercises x:Uid)
            WinUISampleAppTestsUtils.VerifyText("titleTextBlock", "Welcome to Page 2!");

            Log.Comment("Navigating to Page 1");
            button = new Button(FindElement.ByName("navigationButton"));
            button.Invoke();
            Log.Comment("Wait For Idle");
            Wait.ForIdle();

            ElementCache.Clear();

            // Verify string on Page 1 (exercises x:Uid)
            WinUISampleAppTestsUtils.VerifyText("TestTextBlock", "Loaded");
            WinUISampleAppTestsUtils.VerifyText("titleTextBlock", "Welcome to Page 1!");

            TestEnvironment.Application.Close(); // Ensure a crash will fail this test.
        }

        private void SelectWindowUI()
        {
            Log.Comment("Selecting Window Class tab");
            var tviWindowClass = FindElement.ByName("tviWindowClass");
            Verify.IsNotNull(tviWindowClass);
            SelectItem(tviWindowClass);

            Log.Comment("Wait For Idle");
            Wait.ForIdle();

            Log.Comment("Verifying content is displayed for Window Class tab");
            var stackPanelWindowClass = FindElement.ByName("stackPanelWindowClass");
            Verify.IsNotNull(stackPanelWindowClass);
        }

        private void ClearWindow()
        {
            if (WinUISampleAppTestsUtils.InvokeButtonIfEnabled("buttonCloseWindow"))
            {
                Log.Comment("Closed window");
            }
            if (WinUISampleAppTestsUtils.InvokeButtonIfEnabled("buttonDiscardWindow"))
            {
                Log.Comment("Discarded window");
            }
        }

        [TestMethod]
        [Description("Window Chrome tests : basic test of functioning of min, max and close buttons")]
        public void WindowChromeMinimizeMaximizeCloseTest()
        {
            WindowChromeCommonSetup();

            UIObject root = TestEnvironment.Application.ApplicationFrameWindow ?? TestEnvironment.Application.CoreWindow;
            int processID = TestEnvironment.Application.GetProcessIdFromAppWindow();

            // minimize button testing
            Log.Comment("Testing if window can be minimized");
            Log.Comment("Clicking minimize button");
            TestHelpers.TryCaptionButtonLeftClick(TestHelpers.CaptionButtons.Minimize);
            Wait.ForIdle();
            Verify.IsTrue(TestHelpers.IsWindowMinimized());

            TestHelpers.RestoreWindow();
            Log.Comment("window restored");

            // maximize button testing
            Log.Comment("Testing if window can be maximized");
            Log.Comment("Clicking maximize button");
            TestHelpers.TryCaptionButtonLeftClick(TestHelpers.CaptionButtons.Maximize);
            Wait.ForIdle();
            Verify.IsTrue(TestHelpers.IsWindowMaximized());

            //restore down testing for maximize button
            Log.Comment("Clicking restore down button");
            TestHelpers.TryCaptionButtonLeftClick(TestHelpers.CaptionButtons.Maximize);
            Wait.ForIdle();
            Verify.IsTrue(TestHelpers.IsWindowRestored());

            // test close button
            Log.Comment("Clicking Close button");
            TestHelpers.TryCaptionButtonLeftClick(TestHelpers.CaptionButtons.Close, true);
 
            // It isn't valid to call waitforidle while the app is closing so we need to wait and see if the window actually closes
            for (var i = 0; i < 10 && !TestHelpers.IsWindowClosed(processID); i++)
            {
                Log.Comment("Waiting for window to close ({0})", i);
                Wait.ForMilliseconds(1500);
            }

            Verify.IsTrue(TestHelpers.IsWindowClosed(processID));
            TestEnvironment.Application.WaitForExit(); // Ensure a crash will fail this test.
        }

        [TestMethod]
        [Description("Window Chrome tests : test if system menu gets launched by Alt+space and right click on custom titlebar")]
        public void WindowChromeSystemMenuLaunchTest()
        {
            UIObject root = TestEnvironment.Application.ApplicationFrameWindow ?? TestEnvironment.Application.CoreWindow;
            IntPtr parentHwnd = TestEnvironment.Application.Hwnd;

            WindowChromeCommonSetup();

            KeyboardHelper.PressKey(Key.Space, ModifierKey.Alt);

            IntPtr systemMenuHwnd = TestHelpers.NativeMethods.FindWindowEx(IntPtr.Zero, IntPtr.Zero, TestHelpers.NativeMethods.CLASSID_CONTEXTMENU, "");
            Verify.AreNotEqual(systemMenuHwnd, IntPtr.Zero, "system menu did get created by Alt + Space");

            KeyboardHelper.PressKey(Key.Escape); //hide system menu

            UICondition condition = UICondition.CreateFromId("customTitleBarText");
            var customtitlebar = root.Descendants.Find(condition);
            InputHelper.RightClick(customtitlebar, 5, 10);  // offset is added to account for DPI related issues

            IntPtr systemMenuHwndViaRightClick = TestHelpers.NativeMethods.FindWindowEx(IntPtr.Zero, IntPtr.Zero, TestHelpers.NativeMethods.CLASSID_CONTEXTMENU, "");
            Verify.AreNotEqual(systemMenuHwndViaRightClick, IntPtr.Zero, "system menu did get created by right click on custom titlebar");

            TestEnvironment.Application.Close(); // Ensure a crash will fail this test.
        }

        [TestMethod]
        [Description("Window Chrome tests : test if system menu launched from right click has menu items correctly enabled or disabled. Also test when window is maximized")]
        public void WindowChromeSystemMenuItemsStateTest()
        {
            UIObject root = TestEnvironment.Application.ApplicationFrameWindow ?? TestEnvironment.Application.CoreWindow;
            int processID = TestEnvironment.Application.GetProcessIdFromAppWindow();
            IntPtr parentHwnd = TestEnvironment.Application.Hwnd;

            WindowChromeCommonSetup();

            // Showing system menu via Right click messes up its parenting in UIA tree.  Task filed : 30164940
            // UICondition condition = UICondition.CreateFromId("customTitleBarText");
            // var customtitlebar = root.Descendants.Find(condition);
            // InputHelper.RightClick(customtitlebar, 5, 10);  // offset is added to account for DPI related issues
            // Wait.ForIdle();

            KeyboardHelper.PressKey(Key.Space, ModifierKey.Alt); // ideally right click should be used to bring system menu for real testing

            IntPtr systemMenuHwnd = TestHelpers.NativeMethods.FindWindowEx(IntPtr.Zero, IntPtr.Zero, TestHelpers.NativeMethods.CLASSID_CONTEXTMENU, "");
            Verify.AreNotEqual(systemMenuHwnd, IntPtr.Zero);

            UICondition restorecondition = UICondition.RawTree.AndWith(UICondition.CreateFromId(TestHelpers.NativeMethods.SYSTEMENUITEM_RESTORE));
            UICondition movecondition = UICondition.RawTree.AndWith(UICondition.CreateFromId(TestHelpers.NativeMethods.SYSTEMENUITEM_MOVE));
            UICondition sizecondition = UICondition.RawTree.AndWith(UICondition.CreateFromId(TestHelpers.NativeMethods.SYSTEMENUITEM_SIZE));
            UICondition minimizecondition = UICondition.RawTree.AndWith(UICondition.CreateFromId(TestHelpers.NativeMethods.SYSTEMENUITEM_MINIMIZE));
            UICondition maximizecondition = UICondition.RawTree.AndWith(UICondition.CreateFromId(TestHelpers.NativeMethods.SYSTEMENUITEM_MAXIMIZE));
            UICondition closecondition = UICondition.RawTree.AndWith(UICondition.CreateFromId(TestHelpers.NativeMethods.SYSTEMENUITEM_CLOSE));

            // test if restore menu item is disabled
            var restoreMenuItem = root.Descendants.Find(restorecondition);
            Verify.IsNotNull(restoreMenuItem);
            Verify.IsFalse(restoreMenuItem.IsEnabled, "Restore system menu item is disabled");

            // test if move menu item is enabled
            var moveMenuItem = root.Descendants.Find(movecondition);
            Verify.IsNotNull(moveMenuItem);
            Verify.IsTrue(moveMenuItem.IsEnabled, "Move system menu item is enabled");

            // test if size menu item is enabled
            var sizeMenuItem = root.Descendants.Find(sizecondition);
            Verify.IsNotNull(sizeMenuItem);
            Verify.IsTrue(sizeMenuItem.IsEnabled, "Size system menu item is enabled");

           // test if minimize menu item is enabled
            var minimizeMenuItem = root.Descendants.Find(minimizecondition);
            Verify.IsNotNull(minimizeMenuItem);
            Verify.IsTrue(minimizeMenuItem.IsEnabled, "Minimize system menu item is enabled");

            // test if maximize menu item is enabled
            var maximizeMenuItem = root.Descendants.Find(maximizecondition);
            Verify.IsNotNull(maximizeMenuItem);
            Verify.IsTrue(maximizeMenuItem.IsEnabled, "Maximize system menu item is enabled");

            // test if close menu item is enabled
            var closeMenuItem = root.Descendants.Find(closecondition);
            Verify.IsNotNull(closeMenuItem);
            Verify.IsTrue(closeMenuItem.IsEnabled, "Close system menu item is enabled");

            // ---------------------------------------------------------------------------------
            //maximize window and test if menu item states change
            KeyboardHelper.PressKey(Key.x);
            Wait.ForIdle();
            Verify.IsTrue(TestHelpers.IsWindowMaximized());
            // InputHelper.RightClick(customtitlebar, 5, 10);  // Showing system menu via Right click messes up its parenting in UIA tree.  Task filed : 30164940
            KeyboardHelper.PressKey(Key.Space, ModifierKey.Alt);
            // ---------------------------------------------------------------------------------

            // test if restore menu item is enabled now
            restoreMenuItem = root.Descendants.Find(restorecondition);
            Verify.IsNotNull(restoreMenuItem);
            Verify.IsTrue(restoreMenuItem.IsEnabled, "Restore system menu item is enabled on maximized window");

            // test if move menu item is enabled
            moveMenuItem = root.Descendants.Find(movecondition);
            Verify.IsNotNull(moveMenuItem);
            Verify.IsFalse(moveMenuItem.IsEnabled, "Move system menu item is disabled on maximized window");

            // test if size menu item is enabled
            sizeMenuItem = root.Descendants.Find(sizecondition);
            Verify.IsNotNull(sizeMenuItem);
            Verify.IsFalse(sizeMenuItem.IsEnabled, "Size system menu item is disabled on maximized window");

           // test if minimize menu item is enabled
            minimizeMenuItem = root.Descendants.Find(minimizecondition);
            Verify.IsNotNull(minimizeMenuItem);
            Verify.IsTrue(minimizeMenuItem.IsEnabled, "Minimize system menu item is enabled on maximized window");

            // test if maximize menu item is enabled
            maximizeMenuItem = root.Descendants.Find(maximizecondition);
            Verify.IsNotNull(maximizeMenuItem);
            Verify.IsFalse(maximizeMenuItem.IsEnabled, "Maximize system menu item is disabled on maximized window");

            // test if close menu item is enabled
            closeMenuItem = root.Descendants.Find(closecondition);
            Verify.IsNotNull(closeMenuItem);
            Verify.IsTrue(closeMenuItem.IsEnabled, "Close system menu item is enabled on maximized window");

            TestEnvironment.Application.Close(); // Ensure a crash will fail this test.
        }

        [TestMethod]
        [Description("Window Chrome tests : test system menu item actions for min, max and close")]
        public void WindowChromeSystemMenuMinMaxCloseItemsActionTest()
        {
            UIObject root = TestEnvironment.Application.ApplicationFrameWindow ?? TestEnvironment.Application.CoreWindow;
            int processID = TestEnvironment.Application.GetProcessIdFromAppWindow();
            IntPtr parentHwnd = TestEnvironment.Application.Hwnd;

            WindowChromeCommonSetup();

            UICondition condition = UICondition.CreateFromId("customTitleBarText");
            var customtitlebar = root.Descendants.Find(condition);

            Log.Comment("Launching system menu by right clicking on custom title bar");
            InputHelper.RightClick(customtitlebar, 5, 10);
            Log.Comment("testing minimize menu item");
            KeyboardHelper.PressKey(Key.n);
            Wait.ForIdle();
            Verify.IsTrue(TestHelpers.IsWindowMinimized());

            TestHelpers.RestoreWindow();
            Log.Comment("window restored");

            Log.Comment("testing maximize menu item");
            InputHelper.RightClick(customtitlebar, 5, 10);
            KeyboardHelper.PressKey(Key.x);
            Wait.ForIdle();
            Verify.IsTrue(TestHelpers.IsWindowMaximized());

            Log.Comment("testing restore menu item");
            InputHelper.RightClick(customtitlebar, 5, 10);
            KeyboardHelper.PressKey(Key.r);
            Wait.ForIdle();
            Verify.IsTrue(TestHelpers.IsWindowRestored());

            Log.Comment("testing close menu item");
            InputHelper.RightClick(customtitlebar, 5, 10);
            KeyboardHelper.PressKey(Key.c, ModifierKey.None, 1, true); // skip wait for idle as closing the app will cause it to fail
            Wait.ForMilliseconds(1500); // instead the test will wait for a fixed amount of time
            Verify.IsTrue(TestHelpers.IsWindowClosed(processID));

            TestEnvironment.Application.WaitForExit(); // Ensure a crash will fail this test.
        }

        [TestMethod]
        [Description("Window Chrome tests : test system menu item action for size item using keyboard to change window size")]
        public void WindowChromeSystemMenuSizeItemActionTest()
        {
            UIObject root = TestEnvironment.Application.ApplicationFrameWindow ?? TestEnvironment.Application.CoreWindow;
            IntPtr parentHwnd = TestEnvironment.Application.Hwnd;

            WindowChromeCommonSetup();
            UICondition condition = UICondition.CreateFromId("customTitleBarText");
            var customtitlebar = root.Descendants.Find(condition);

            var initialDimensionsRect = root.BoundingRectangle;
            Log.Comment("Initial window dimensions Width:{0} Height:{1}", initialDimensionsRect.Width, initialDimensionsRect.Height);
            InputHelper.RightClick(customtitlebar, 5, 10);
            KeyboardHelper.PressKey(Key.s); //use shortcut to select size menu item
            Wait.ForIdle();

            KeyboardHelper.PressKey(Key.Right); //should select right edge
            KeyboardHelper.PressKey(Key.Right); // should increase width from right side
            KeyboardHelper.PressKey(Key.Enter); //should end size mode

            var finalDimensionsRect = root.BoundingRectangle;
            Log.Comment("Final window dimensions Width:{0} Height:{1}", finalDimensionsRect.Width, finalDimensionsRect.Height);
            Verify.IsGreaterThan(finalDimensionsRect.Width, initialDimensionsRect.Width, "window width did increase");
            Verify.AreEqual(finalDimensionsRect.Height, initialDimensionsRect.Height, "window height remains same");

            TestEnvironment.Application.Close(); // Ensure a crash will fail this test.
        }

        [TestMethod]
        [Description("Window Chrome tests : test system menu item action for move item using keyboard to move the window")]
        public void WindowChromeSystemMenuMoveItemActionTest()
        {
            UIObject root = TestEnvironment.Application.ApplicationFrameWindow ?? TestEnvironment.Application.CoreWindow;
            IntPtr parentHwnd = TestEnvironment.Application.Hwnd;

            WindowChromeCommonSetup();
            UICondition condition = UICondition.CreateFromId("customTitleBarText");
            var customtitlebar = root.Descendants.Find(condition);

            var initialDimensionsRect = root.BoundingRectangle;
            Log.Comment("Initial window dimensions X:{0} Y:{1}", initialDimensionsRect.X, initialDimensionsRect.Y);
            InputHelper.RightClick(customtitlebar, 5, 10);
            KeyboardHelper.PressKey(Key.m); //use shortcut to select move menu item
            Wait.ForIdle();

            KeyboardHelper.PressKey(Key.Right); // should move window right side
            KeyboardHelper.PressKey(Key.Enter); //should end move mode

            var finalDimensionsRect = root.BoundingRectangle;
            Log.Comment("Final window dimensions X:{0} Y:{1}", finalDimensionsRect.X, finalDimensionsRect.Y);
            Verify.IsGreaterThan(finalDimensionsRect.X, initialDimensionsRect.X, "window did move right in X axis");
            Verify.AreEqual(finalDimensionsRect.Y, initialDimensionsRect.Y, "window did not move in Y axis");

            TestEnvironment.Application.Close(); // Ensure a crash will fail this test.
        }

        [TestMethod]
        [Description("Window Chrome tests : test if trying to drag custom titlebar drags the window")]
        public void WindowChromeDragTest()
        {
            UIObject root = TestEnvironment.Application.ApplicationFrameWindow ?? TestEnvironment.Application.CoreWindow;
            IntPtr parentHwnd = TestEnvironment.Application.Hwnd;

            WindowChromeCommonSetup();
            UICondition condition = UICondition.CreateFromId("customTitleBarText");
            var customtitlebar = root.Descendants.Find(condition);

            global::System.Drawing.Point startPoint = new global::System.Drawing.Point()
            {
                X = customtitlebar.BoundingRectangle.X + 5,
                Y = customtitlebar.BoundingRectangle.Y + 10
            };
            DragTest(startPoint);

            TestEnvironment.Application.Close(); // Ensure a crash will fail this test.
        }

        [TestMethod]
        [Description("Window Chrome tests : Resize window from different points using mouse")]
        public void WindowChromeMouseResizeTest()
        {
            UIObject root = TestEnvironment.Application.ApplicationFrameWindow ?? TestEnvironment.Application.CoreWindow;
            IntPtr parentHwnd = TestEnvironment.Application.Hwnd;

            WindowChromeCommonSetup();
            UICondition condition = UICondition.CreateFromId("customTitleBarText");
            var customtitlebar = root.Descendants.Find(condition);

            var windowDimensionsRect = root.BoundingRectangle;
            var toprightcorner = windowDimensionsRect.X + windowDimensionsRect.Width - 5; // for resizing horizontally
            global::System.Drawing.Point startPoint = new global::System.Drawing.Point()
            {
                X = toprightcorner,
                Y = windowDimensionsRect.Y + 10
            };
            ResizeTest(startPoint); // increases width by resizing on right side of the window near top right corner

            //updated window dimensions
            windowDimensionsRect = root.BoundingRectangle;
            toprightcorner = windowDimensionsRect.X + windowDimensionsRect.Width - 5;
            var middleofrightedge = windowDimensionsRect.Y + (windowDimensionsRect.Height / 2); // right edge of window near the middle
            startPoint = new global::System.Drawing.Point()
            {
                X = toprightcorner,
                Y = middleofrightedge
            };

            ResizeTest(startPoint, -150); // decreases width by resizing on right side of the window near the middle height

            TestEnvironment.Application.Close(); // Ensure a crash will fail this test.
        }

        [TestMethod]
        [Description("Window Chrome tests : test if dynamically changing titlebar from Custom->Original->Custom works fine")]
        public void WindowChromeDynamicTitlebarChangeTest()
        {
            UIObject root = TestEnvironment.Application.ApplicationFrameWindow ?? TestEnvironment.Application.CoreWindow;
            IntPtr parentHwnd = TestEnvironment.Application.Hwnd;

            WindowChromeCommonSetup();

            Log.Comment("Switching from custom titlebar to original win32 titlebar");
            var button = new Button(FindElement.ByName("titlebarChange"));
            button.Invoke();
            bool didThrowException = false;
            try
            {
                UICondition condition = UICondition.CreateFromId("customTitleBarText");
                var customtitlebar = root.Descendants.Find(condition);
            }
            catch (UIObjectNotFoundException)
            {

                didThrowException = true;
            }
            Verify.IsTrue(didThrowException, "automation could not find custom titlebar as it has been hidden by switching to original titlebar");

            Log.Comment("Switching from original Win32 titlebar to custom titlebar");
            button.Invoke();

            bool didNotThrowException = true;
            try
            {
                UICondition condition = UICondition.CreateFromId("customTitleBarText");
                var customtitlebar = root.Descendants.Find(condition);
            }
            catch (UIObjectNotFoundException)
            {

                didNotThrowException = false;
            }
            Verify.IsTrue(didNotThrowException, "automation did find custom titlebar as it has been reactivated");

            TestEnvironment.Application.Close(); // Ensure a crash will fail this test.
        }

        [TestMethod]
        [Description("Window Chrome tests : test that doing double click anywhere on custom titlebar maximizes the window. Also, doing it again restores the window")]
        public void WindowChromeDoubleClickMaximizeAndRestoreTest()
        {
            UIObject root = TestEnvironment.Application.ApplicationFrameWindow ?? TestEnvironment.Application.CoreWindow;
            IntPtr parentHwnd = TestEnvironment.Application.Hwnd;

            WindowChromeCommonSetup();
            UICondition condition = UICondition.CreateFromId("customTitleBarText");
            var customtitlebar = root.Descendants.Find(condition);
            InputHelper.LeftDoubleClick(customtitlebar, 5, 10);
            Wait.ForIdle();
            Verify.IsTrue(TestHelpers.IsWindowMaximized());

            InputHelper.LeftDoubleClick(customtitlebar, 5, 10);
            Wait.ForIdle();
            Verify.IsTrue(TestHelpers.IsWindowRestored());

            TestEnvironment.Application.Close(); // Ensure a crash will fail this test.
        }

        [TestMethod]
        [Description("Window Chrome tests : test dynamically changing size of custom titlebar and see it gets reflected for its dragging area too")]
        public void WindowChromeTitlebarSizeChangeTest()
        {
            UIObject root = TestEnvironment.Application.ApplicationFrameWindow ?? TestEnvironment.Application.CoreWindow;
            IntPtr parentHwnd = TestEnvironment.Application.Hwnd;

            WindowChromeCommonSetup();
            UICondition condition = UICondition.CreateFromId("customTitleBarText");
            var customtitlebar = root.Descendants.Find(condition);

            SelectTestFromSpecialTestsMenu("titlebarSizeChange"); // it will increase title bar size to 500

            global::System.Drawing.Point startPoint = new global::System.Drawing.Point()
            {
                X = customtitlebar.BoundingRectangle.X + 5,
                Y = customtitlebar.BoundingRectangle.Y + 10
            };
            startPoint.Y += 200; // pick a point in now very large titlebar, this would fail in a normal size titlebar
            DragTest(startPoint);

            TestEnvironment.Application.Close(); // Ensure a crash will fail this test.
        }

        [TestMethod]
        [TestProperty("TestPass:MinOSVer", WindowsOSVersion._19H1)] // 46636362: Temporarily disabled from a test failure in RS5, stemming from system CoreMessaging.
        [Description("Window Chrome tests : Test if no titlebar is provided and Window Chrome is enabled, caption buttons still appear on right and a default titlebar area is given which covers entire non client area")]
        public void WindowChromeNullTitlebarTest()
        {
            UIObject root = TestEnvironment.Application.ApplicationFrameWindow ?? TestEnvironment.Application.CoreWindow;
            IntPtr parentHwnd = TestEnvironment.Application.Hwnd;

            WindowChromeCommonSetup();
            UICondition condition = UICondition.CreateFromId("customTitleBarText");
            var customtitlebar = root.Descendants.Find(condition);

            SelectTestFromSpecialTestsMenu("titlebarnullptr"); // it will set default titlebar

            Log.Comment("Testing if window can be minimized");
            Log.Comment("Clicking minimize button");
            TestHelpers.TryCaptionButtonLeftClick(TestHelpers.CaptionButtons.Minimize);

            Wait.ForIdle();
            Verify.IsTrue(TestHelpers.IsWindowMinimized());

            TestHelpers.RestoreWindow();
            Log.Comment("window restored");

            Log.Comment("Testing if window can be still be dragged");
            var windowDimensionsRect = root.BoundingRectangle;
            global::System.Drawing.Point startPoint = new global::System.Drawing.Point()
            {
                X = windowDimensionsRect.Width - (TestHelpers.CAPTION_BUTTON_WIDTH * 3 ) - 30, // pick a point next to minimize button ensuring it is still draggable
                Y = windowDimensionsRect.Y + TestHelpers.CAPTION_BUTTON_HEIGHT / 2
            };
            DragTest(startPoint);

            WindowChromeMinimizeMaximizeCloseTest();
        }


        [TestMethod]
        [Description("Window Chrome tests : Test if window can be maximized and restored and the drag region remains correct")]
        public void WindowChromeNullTitlebarDragRegionUpdateTest()
        {
            UIObject root = TestEnvironment.Application.ApplicationFrameWindow ?? TestEnvironment.Application.CoreWindow;
            IntPtr parentHwnd = TestEnvironment.Application.Hwnd;

            WindowChromeCommonSetup();
            UICondition condition = UICondition.CreateFromId("customTitleBarText");
            var customtitlebar = root.Descendants.Find(condition);

            SelectTestFromSpecialTestsMenu("titlebarnullptr"); // it will set titlebar as null

            Log.Comment("Testing if caption buttons have moved to top right corner of the app");
            Log.Comment("Clicking maximize button");
            TestHelpers.TryCaptionButtonLeftClick(TestHelpers.CaptionButtons.Maximize);

            Wait.ForIdle();
            Verify.IsTrue(TestHelpers.IsWindowMaximized());

            TestHelpers.RestoreWindow();
            Log.Comment("window restored");


            Log.Comment("Testing if window can still be dragged from provided drag region and there is no mismatch between Dragbar window and drag region");
            var windowDimensionsRect = root.BoundingRectangle;
            global::System.Drawing.Point startPoint = new global::System.Drawing.Point()
            {
                X = windowDimensionsRect.Width - (TestHelpers.CAPTION_BUTTON_WIDTH * 3 ) - 30, // pick a point next to minimize button ensuring it is still draggable
                Y = windowDimensionsRect.Y + TestHelpers.CAPTION_BUTTON_HEIGHT / 2
            };
            DragTest(startPoint);

            TestEnvironment.Application.Close(); // Ensure a crash will fail this test.
        }

        [TestMethod]
        [Description("Window Chrome tests :Test that content dialog doesn't prevent input from going to caption buttons")]
        public void WindowChromeContentDialogDoesntBlockInputOnCaptionButtonsTest()
        {
            UIObject root = TestEnvironment.Application.ApplicationFrameWindow ?? TestEnvironment.Application.CoreWindow;
            IntPtr parentHwnd = TestEnvironment.Application.Hwnd;

            WindowChromeCommonSetup();
            UICondition condition = UICondition.CreateFromId("customTitleBarText");
            var customtitlebar = root.Descendants.Find(condition);

            Log.Comment("Launching content dialog");
            SelectTestFromSpecialTestsMenu("contentDialogWithTitlebar");

            Log.Comment("Testing if content dialog has indeed launched by trying to click on a button");
            // check if button has not been clicked yet
            var clickStatusBox = new TextBlock(root.Descendants.Find(UICondition.CreateFromId("textBlockMUXControls")));
            Verify.AreEqual("Loaded", clickStatusBox.DocumentText, "textBlockMUXControls is not Loaded, button has been clicked already");

            // do a click and then check status, if content dialog has launched, the status should still be empty
            var clickButton = new Button(root.Descendants.Find(UICondition.CreateFromId("button")));
            InputHelper.LeftClick(clickButton);
            Verify.AreEqual("Loaded", clickStatusBox.DocumentText, "button can still be clicked, content dialog didn't launch");


            Log.Comment("Clicking maximize button");
            TestHelpers.TryCaptionButtonLeftClick(TestHelpers.CaptionButtons.Maximize);

            Wait.ForIdle();
            Verify.IsTrue(TestHelpers.IsWindowMaximized());

            TestHelpers.RestoreWindow();
            Log.Comment("window restored");

            // doing additional drag test too
            Log.Comment("Testing if window is no longer draggable");
            var windowDimensionsRect = root.BoundingRectangle;
            global::System.Drawing.Point startPoint = new global::System.Drawing.Point()
            {
                X = windowDimensionsRect.Width - (TestHelpers.CAPTION_BUTTON_WIDTH * 3 ) - 30, // pick a point next to minimize button ensuring it is still draggable
                Y = windowDimensionsRect.Y + TestHelpers.CAPTION_BUTTON_HEIGHT / 2
            };
            DragTest(startPoint, 150, false); // drag should fail
            
            TestEnvironment.Application.Close(); // Ensure a crash will fail this test.
        }

        [TestMethod]
        [Description("Verify that an ElementName Binding in a Window can resolve a Source that lives on a non-Entered property")]
        public void ElementNameBindingInWindowTest()
        {
            SelectBindingUI();

            // Target TextBlock's Text is bound via ElementName to the Text of a TextBlock that
            // lives on the root StackPanel via custom attached property
            WinUISampleAppTestsUtils.VerifyText("elementNameBindingTextBlock", "lorem ipsum");

            TestEnvironment.Application.Close(); // Ensure a crash will fail this test.
        }

        [TestMethod]
        [Description("Tests if AppWindow apis are accessible")]
        public void AppWindowApiTest()
        {
            WindowChromeCommonSetup();
            Log.Comment("Test for testing if Window.AppWindow api is functional");
            SelectTestFromSpecialTestsMenu("appwindowobj");
            Log.Comment("Testing AppWindow.IsVisible() api accessible through Window.AppWindow");
            WinUISampleAppTestsUtils.VerifyText("textBlockMUXControls", "AppWindow api : true");

            TestEnvironment.Application.Close(); // Ensure a crash will fail this test.
        }

        [TestMethod]
        [Description("Verifies that WinUI uses a custom ResourceManager provided by the app")]
        public void VerifyCustomResourceManager()
        {
            // This test clicks a button Page 1 that in theory tries to load content
            // from `ms-appx:///786B1897-FEB4-4FC8-9796-9F7DD21582CC.xaml`, but there is no resource at that URI.
            // However, the app is using a custom ResourceManager that will provide
            // a "404" page in response to a missing resource instead of crashing.
            Log.Comment("Loading 'ms-appx:///786B1897-FEB4-4FC8-9796-9F7DD21582CC.xaml'");
            var button = new Button(FindElement.ByName("missingPageNavigationButton"));
            button.Invoke();
            Log.Comment("Wait For Idle");
            Wait.ForIdle();

            ElementCache.Clear();

            // Verify that the "404" page has been loaded
            WinUISampleAppTestsUtils.VerifyText("titleTextBlock", "Error 404 - Page Not Found");

            TestEnvironment.Application.Close(); // Ensure a crash will fail this test.
        }

        [TestMethod]
        [Description("Verifies that opening a DatePicker flyout does not crash the app.")]
        public void VerifyCanOpenDatePicker()
        {
            var datePicker = FindElement.ByName("DatePicker");
            Log.Comment("Opening date picker.");
            datePicker.Click();
            Wait.ForIdle();
            Log.Comment("Closing date picker.");
            datePicker.Click();
            Wait.ForIdle();

            TestEnvironment.Application.Close(); // Ensure a crash will fail this test.
        }

        [TestMethod]
        [Description("Verifies that attaching [bindable] to a type not appearing in XAML causes its type info to be properly added to XamlTypeInfo.g.cpp")]
        public void VerifyBindableAttributeCreatesTypeInfo()
        {
            var textBlock = new TextBlock(FindElement.ById("ViewModelBindingTextBlock"));
            Verify.AreEqual<string>("Text from ViewModel", textBlock.DocumentText);

            TestEnvironment.Application.Close(); // Ensure a crash will fail this test.
        }

        private void SelectBindingUI()
        {
            Log.Comment("Selecting Binding tab");
            var tviBinding = FindElement.ByName("tviBinding");
            Verify.IsNotNull(tviBinding);
            SelectItem(tviBinding);

            Log.Comment("Wait For Idle");
            Wait.ForIdle();

            Log.Comment("Verifying content is displayed for Binding tab");
            var stackPanelBinding = FindElement.ByName("stackPanelBinding");
            Verify.IsNotNull(stackPanelBinding);
        }

        private static void SelectItem(UIObject element)
        {
            var implementation = new SelectionItemImplementation<UIObject>(element, UIObject.Factory);
            Verify.IsTrue(implementation.IsAvailable);
            implementation.Select();
        }

        private void SelectTestFromSpecialTestsMenu(string menuItemName)
        {
            UIObject root = TestEnvironment.Application.ApplicationFrameWindow ?? TestEnvironment.Application.CoreWindow;
            WindowChromeCommonSetup();

            var specialTestsDropDownButton = new Button(root.Descendants.Find(UICondition.CreateFromId("windowChromeSpecialTests")));
            specialTestsDropDownButton.Invoke();
            Wait.ForIdle();

            UICondition itemCondition = UICondition.CreateFromId(menuItemName);
            var testitem = new Button(root.Descendants.Find(itemCondition));
            testitem.Invoke();
            Wait.ForIdle();
        }

        private void DragTest(global::System.Drawing.Point startPoint, int dragAmount = 150, bool shouldMatch = true)
        {
            DragToMoveOrResize(startPoint, dragAmount, false, shouldMatch);
        }

        private void ResizeTest(global::System.Drawing.Point startPoint, int moveAmount = 150, bool shouldMatch = true)
        {
            DragToMoveOrResize(startPoint, moveAmount, true, shouldMatch);
        }

        private void DragToMoveOrResize(global::System.Drawing.Point startPoint, int amount, bool isResize, bool shouldMatch)
        {
            UIObject root = TestEnvironment.Application.ApplicationFrameWindow ?? TestEnvironment.Application.CoreWindow;
            var initialDimensionsRect = root.BoundingRectangle;
            Log.Comment("Initial window dimensions X:{0} Y:{1}", initialDimensionsRect.X, initialDimensionsRect.Y);
            InputHelper.MoveMouse(startPoint);
            Log.Comment("Start Point X:{0} Y:{1}", startPoint.X, startPoint.Y);

            global::System.Drawing.Point endPoint = new global::System.Drawing.Point()
            {
                X = startPoint.X + amount,
                Y = startPoint.Y
            };
            Log.Comment("Expected End Point X:{0} Y:{1}", endPoint.X, endPoint.Y);

            PointerInput.ClickDrag(endPoint, PointerButtons.Primary, 1000);
            Wait.ForIdle();

            var finalDimensionsRect = root.BoundingRectangle;
            Log.Comment("Final window dimensions X:{0} Y:{1}", finalDimensionsRect.X, finalDimensionsRect.Y);
            if (!isResize)  //move
            {
                if (shouldMatch)
                {
                    Verify.IsGreaterThan(finalDimensionsRect.X, initialDimensionsRect.X, "window did move right in X axis");
                }
                else
                {
                    Verify.AreEqual(finalDimensionsRect.X, initialDimensionsRect.X, "window did not move right in X axis");

                }

            }
            else  // resize
            {
                if (shouldMatch)
                {
                    Verify.AreNotEqual(finalDimensionsRect.Width, initialDimensionsRect.Width, "window did resize in width");
                }
                else
                {
                    Verify.AreEqual(finalDimensionsRect.Width, initialDimensionsRect.Width, "window did not resize in width");
                }
            }
        }

    }
}
