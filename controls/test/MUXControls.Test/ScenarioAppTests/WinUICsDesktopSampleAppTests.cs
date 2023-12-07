using Common;
using Microsoft.UI.Xaml.Tests.MUXControls.InteractionTests.Infra;
using Microsoft.UI.Xaml.Tests.MUXControls.InteractionTests.Common;
using Microsoft.Windows.Apps.Test.Automation;
using Microsoft.Windows.Apps.Test.Foundation;
using Microsoft.Windows.Apps.Test.Foundation.Controls;
using System;
using System.Collections.Generic;
using System.Linq;
using System.Numerics;
using System.Threading;
using System.Threading.Tasks;

using WEX.TestExecution;
using WEX.TestExecution.Markup;
using WEX.Logging.Interop;

using WinAppsTestWindow = Microsoft.Windows.Apps.Test.Foundation.Controls.Window;

namespace Microsoft.UI.Xaml.Tests.MUXControls.InteractionTests
{
    [TestClass]
    public class WinUICsDesktopSampleAppTests
    {
        public static TestApplicationInfo WinUICsDesktopSampleApp
        {
            get
            {
                return new TestApplicationInfo(
                    "WinUICsDesktopSampleApp",
                    "WinUICsDesktopSampleApp_6f07fta6qpts2!App",
                    "WinUICsDesktopSampleApp_6f07fta6qpts2",
                    "WinUI Desktop - 0",
                    "WinUICsDesktopSampleApp.exe",
                    "WinUICsDesktopSampleApp",
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
        [TestProperty("IsolationLevel", "Test")] // Task 32517851: WinUICppDesktopSampleAppTests and WinUICsDesktopSampleAppTests must run with isolationmode=test due to insufficient test cleanup
        public static void ClassInitialize(TestContext testContext)
        {
            TestEnvironment.Initialize(testContext, WinUICsDesktopSampleApp);
        }

        [TestInitialize]
        public static void TestInitialize()
        {
            ClickSafeElement();
        }

        /// <summary>
        /// Click an element in the app to ensure that the desired app window has focus
        /// <summary>
        /// <param name="mainWindowIndex">
        /// The index of the window containing MainWindow content that should be 
        /// focused. Default value is '0' (the first MainWindow created)
        /// </param>
        static void ClickSafeElement(int mainWindowIndex = 0)
        {
            Log.Comment($"Click safeElementToClick in window {mainWindowIndex}");
            UIObject root = FindElement.GetDesktopTopLevelWindow($"WinUI Desktop - {mainWindowIndex}");
            Verify.IsNotNull(root);
            
            var safeElementToClick = FindElement.GetDescendantByName(root, "safeElementToClick");
            Verify.IsNotNull(safeElementToClick);
            InputHelper.LeftClick(safeElementToClick);
            Wait.ForIdle();

            Log.Comment("Clicked safeElementToClick");
        }

        [TestCleanup]
        public void TestCleanup()
        {

        }

        [ClassCleanupAttribute]
        public static void ClassCleanup()
        {
            TestEnvironment.AssemblyCleanupWorker(WinUICsDesktopSampleApp);
        }

        [TestMethod]
        [Description("Invoke a Button coming from a separate class library.")]
        public void SimpleLaunchTest()
        {
            WinUISampleAppTestsUtils.VerifyText("TestTextBlock", "Loaded");
            WinUISampleAppTestsUtils.InvokeButton("MyButton");
            WinUISampleAppTestsUtils.VerifyText("TestTextBlock", "Clicked");
        }

        [TestMethod]
        [TestProperty("Ignore", "True")] // TODO 29901003: WinUICsDesktopSampleAppTests.CreateSecondMainWindowTest is unreliable
        [Description("Create a second instance of the MainWindow and then close it.")]
        public void CreateSecondMainWindowTest()
        {
            WinUISampleAppTestsUtils.VerifyText("TestTextBlock", null);
            WinUISampleAppTestsUtils.InvokeButton("CreateWindowButton");
            WinUISampleAppTestsUtils.InvokeButton("CloseLastestWindowButton");
        }

        [TestMethod]
        [Description("Validate DependencyObject.Dispatcher returns null")]
        public void TestDependencyObjectDispatcher()
        {
            WinUISampleAppTestsUtils.InvokeButton("buttonGetDODispatcher");
            WinUISampleAppTestsUtils.VerifyText("textBlockDODispatcher", "null");
        }

        [TestMethod]
        [Description("Read Window properties for the MainWindow.")]
        public void ReadWindowPropertiesTest()
        {
            SelectWindowUI();
            ClearWindow();
            WinUISampleAppTestsUtils.ClearErrorReport();
            SelectWindowName("MainWindow");
            ReadWindowProperties(true, "rootStackPanel", "WinUI Desktop - 0");
        }

        [TestMethod]
        [Description("Read Window properties for a new Window with XAML markup.")]
        public void ReadMarkupWindowPropertiesTest()
        {
            ReadWindowProperties("MarkupWindow", "scrollViewer", "WinUI Desktop - Markup Window");
        }

        [TestMethod]
        [Description("Read Window properties for a new Window.")]
        public void ReadRuntimeWindowPropertiesTest()
        {
            ReadWindowProperties("RuntimeWindow", "stackPanelRoot", "WinUI Desktop - Runtime Window");
        }

        public void ReadWindowProperties(string windowName, string contentName, string title)
        {
            SelectWindowUI();
            ClearWindow();
            WinUISampleAppTestsUtils.ClearErrorReport();
            SelectWindowName(windowName);
            WinUISampleAppTestsUtils.InvokeButton("buttonCreateWindow");
            WinUISampleAppTestsUtils.VerifyText("textBoxLastEvent", "ButtonCreateWindow_Click - exit");
            WinUISampleAppTestsUtils.VerifyNoErrorReport();
            ReadWindowProperties(false, contentName, title);
            WinUISampleAppTestsUtils.InvokeButton("buttonDiscardWindow");
            WinUISampleAppTestsUtils.VerifyText("textBoxLastEvent", "ButtonDiscardWindow_Click - exit");
            WinUISampleAppTestsUtils.VerifyNoErrorReport();
        }

        private void ReadWindowProperties(bool isVisible, string contentName, string title)
        {
            WinUISampleAppTestsUtils.ResetText("buttonResetBounds", "textBlockBoundsHeight");
            WinUISampleAppTestsUtils.InvokeButton("buttonGetBounds");
            WinUISampleAppTestsUtils.VerifyText("textBlockBoundsX", "0");
            WinUISampleAppTestsUtils.VerifyText("textBlockBoundsY", "0");
            WinUISampleAppTestsUtils.VerifyText("textBlockBoundsWidth", null);
            WinUISampleAppTestsUtils.VerifyText("textBlockBoundsHeight", null);
            WinUISampleAppTestsUtils.VerifyNoErrorReport();

            WinUISampleAppTestsUtils.ResetText("buttonResetVisible", "textBlockVisible");
            WinUISampleAppTestsUtils.VerifyPropertyAccessor("buttonGetVisible", "textBlockVisible", isVisible ? "True" : "False");

            WinUISampleAppTestsUtils.ResetText("buttonResetContent", "textBlockContent");
            WinUISampleAppTestsUtils.VerifyPropertyAccessor("buttonGetContent", "textBlockContent", contentName);

            WinUISampleAppTestsUtils.ResetText("buttonResetTitle", "textBoxTitle");
            WinUISampleAppTestsUtils.VerifyPropertyAccessor("buttonGetTitle", "textBoxTitle", title);

            WinUISampleAppTestsUtils.ResetText("buttonResetCoreWindow", "textBlockCoreWindow");
            WinUISampleAppTestsUtils.VerifyPropertyAccessor("buttonGetCoreWindow", "textBlockCoreWindow", "null");

            WinUISampleAppTestsUtils.ResetText("buttonResetDispatcher", "textBlockDispatcher");
            WinUISampleAppTestsUtils.VerifyPropertyAccessor("buttonGetDispatcher", "textBlockDispatcher", "null");

            WinUISampleAppTestsUtils.ResetText("buttonResetCompositor", "textBlockCompositor");
            WinUISampleAppTestsUtils.VerifyPropertyAccessor("buttonGetCompositor", "textBlockCompositor", "Microsoft.UI.Composition.Compositor");

            WinUISampleAppTestsUtils.ResetText("buttonResetCurrent", "textBlockCurrent");
            WinUISampleAppTestsUtils.VerifyPropertyAccessor("buttonGetCurrent", "textBlockCurrent", "null");
        }

        [TestMethod]
        [Description("Change the Title property of the MainWindow.")]
        public void ChangeWindowTitleTest()
        {
            SelectWindowUI();
            ClearWindow();

            WinUISampleAppTestsUtils.ClearErrorReport();
            SelectWindowName("MainWindow");
            WinUISampleAppTestsUtils.ResetText("buttonResetTitle", "textBoxTitle");
            WinUISampleAppTestsUtils.SetEditText("textBoxTitle", "New Title");
            WinUISampleAppTestsUtils.InvokeButton("buttonSetTitle");
            WinUISampleAppTestsUtils.VerifyPropertyAccessor("buttonGetTitle", "textBoxTitle", "New Title");

            WinUISampleAppTestsUtils.ClearErrorReport();
            WinUISampleAppTestsUtils.ResetText("buttonResetTitle", "textBoxTitle");
            WinUISampleAppTestsUtils.SetEditText("textBoxTitle", "WinUI Desktop");
            WinUISampleAppTestsUtils.InvokeButton("buttonSetTitle");
            WinUISampleAppTestsUtils.VerifyPropertyAccessor("buttonGetTitle", "textBoxTitle", "WinUI Desktop");
        }

        [TestMethod]
        [Description("Activate and close a new Window with XAML markup.")]
        public void ActivateAndCloseMarkupWindowTest()
        {
            ActivateAndCloseWindow("MarkupWindow");
        }

        [TestMethod]
        [Description("Activate and close a new Window.")]
        public void ActivateAndCloseRuntimeWindowTest()
        {
            ActivateAndCloseWindow("RuntimeWindow");
        }

        private void ActivateAndCloseWindow(string windowName)
        {
            SelectWindowUI();
            ClearWindow();
            WinUISampleAppTestsUtils.ClearErrorReport();
            SelectWindowName(windowName);
            WinUISampleAppTestsUtils.ClearLogEvents();
            WinUISampleAppTestsUtils.InvokeButton("buttonCreateWindow");
            WinUISampleAppTestsUtils.VerifyText("textBoxLastEvent", "ButtonCreateWindow_Click - exit");
            WinUISampleAppTestsUtils.VerifyNoErrorReport();
            WinUISampleAppTestsUtils.InvokeButton("buttonActivateWindow");
            WinUISampleAppTestsUtils.VerifyText("textBoxLastEvent", "Window_Activated for MainWindow - Handled=False, WindowActivationState=CodeActivated");
            WinUISampleAppTestsUtils.VerifyNoErrorReport();
            WinUISampleAppTestsUtils.ResetText("buttonResetVisible", "textBlockVisible");
            WinUISampleAppTestsUtils.VerifyPropertyAccessor("buttonGetVisible", "textBlockVisible", "True");
            WinUISampleAppTestsUtils.InvokeButton("buttonCloseWindow");
            WinUISampleAppTestsUtils.VerifyText("textBoxLastEvent", "ButtonCloseWindow_Click - exit");
            WinUISampleAppTestsUtils.VerifyNoErrorReport();
            WinUISampleAppTestsUtils.InvokeButton("buttonDiscardWindow");
            WinUISampleAppTestsUtils.VerifyText("textBoxLastEvent", "ButtonDiscardWindow_Click - exit");
            WinUISampleAppTestsUtils.VerifyNoErrorReport();

            bool isForRuntimeWindow = windowName == "RuntimeWindow";
            List<string> expectedLogEvents = new List<string>();

            expectedLogEvents.Add("ButtonCreateWindow_Click - entry");
            if (isForRuntimeWindow)
            {
                expectedLogEvents.Add("ButtonCreateWindow_Click - setting Window.Content");
            }
            expectedLogEvents.Add("ButtonCreateWindow_Click - exit");
            expectedLogEvents.Add("ButtonActivateWindow_Click - entry");
            expectedLogEvents.Add("Window_Activated for MainWindow - Handled=False, WindowActivationState=Deactivated");
            expectedLogEvents.Add($"Window_Activated for {windowName} - Handled=False, WindowActivationState=CodeActivated");
            expectedLogEvents.Add($"Window_VisibilityChanged for {windowName} - Handled=False, Visible=True");
            expectedLogEvents.Add("ButtonActivateWindow_Click - exit");
            expectedLogEvents.Add($"Window_Activated for {windowName} - Handled=False, WindowActivationState=Deactivated");
            expectedLogEvents.Add("Window_Activated for MainWindow - Handled=False, WindowActivationState=CodeActivated");
            expectedLogEvents.Add("ButtonCloseWindow_Click - entry");
            expectedLogEvents.Add($"Window_Closed for {windowName} - Handled=False");
            expectedLogEvents.Add($"Window_VisibilityChanged for {windowName} - Handled=False, Visible=False");
            expectedLogEvents.Add("ButtonCloseWindow_Click - exit");
            expectedLogEvents.Add("ButtonDiscardWindow_Click - entry");
            expectedLogEvents.Add("ButtonDiscardWindow_Click - exit");
            WinUISampleAppTestsUtils.VerifyLogEvents(expectedLogEvents.ToArray());
        }

        [TestMethod]
        [Description("Activate and close a new Window with a mouse click.")]
        public void ActivateAndCloseRuntimeWindowWithMouseClickTest()
        {
            ActivateAndCloseRuntimeWindowWithInput(withMouse: true);
        }

        [TestMethod]
        [Description("Activate and close a new Window with a keystroke.")]
        [TestProperty("Ignore", "True")] // Task 29732453: DCPP: Application crash in Microsoft.UI.Input.dll when closing Desktop Window while processing KeyUp event
        public void ActivateAndCloseRuntimeWindowWithKeystrokeTest()
        {
            ActivateAndCloseRuntimeWindowWithInput(withMouse: false);
        }

        private void ActivateAndCloseRuntimeWindowWithInput(bool withMouse)
        {
            SelectWindowUI();
            ClearWindow();
            WinUISampleAppTestsUtils.ClearErrorReport();
            SelectWindowName("RuntimeWindow");
            WinUISampleAppTestsUtils.ClearLogEvents();

            WinUISampleAppTestsUtils.InvokeButton("buttonCreateWindow");
            WinUISampleAppTestsUtils.VerifyText("textBoxLastEvent", "ButtonCreateWindow_Click - exit");
            WinUISampleAppTestsUtils.VerifyNoErrorReport();
            WinUISampleAppTestsUtils.InvokeButton("buttonActivateWindow");
            WinUISampleAppTestsUtils.VerifyText("textBoxLastEvent", "Window_Activated for MainWindow - Handled=False, WindowActivationState=CodeActivated");
            WinUISampleAppTestsUtils.VerifyNoErrorReport();

            UIObject root = FindElement.GetDesktopTopLevelWindow("WinUI Desktop - Runtime Window");
            Verify.IsNotNull(root);
            UIObject buttonCloseRuntimeWindowAsUIObject = FindElement.GetDescendantByName(root, "buttonCloseRuntimeWindow");
            Verify.IsNotNull(buttonCloseRuntimeWindowAsUIObject);
            Verify.IsTrue(buttonCloseRuntimeWindowAsUIObject.IsEnabled);

            Button buttonCloseRuntimeWindow = new Button(buttonCloseRuntimeWindowAsUIObject);
            buttonCloseRuntimeWindow.SetFocus();

            WinUISampleAppTestsUtils.VerifyText("textBoxLastEvent", "Window_Activated for RuntimeWindow - Handled=False, WindowActivationState=CodeActivated");
            UICondition closecondition = UICondition.RawTree.AndWith(UICondition.CreateFromId("Close"));
            var closeCaptionButton = new Button(root.Descendants.Find(closecondition));
            if (withMouse)
            {
                Log.Comment("Invoking buttonCloseRuntimeWindow with mouse click");
                // buttonCloseRuntimeWindow.Click(); // bug 31792148 : remove code related to closeCaptionButton and uncomment this once fixed
                InputHelper.LeftClick(closeCaptionButton);
                
            }
            else
            {
                Log.Comment("Invoking buttonCloseRuntimeWindow with space key");
                KeyboardHelper.PressKey(Key.Space);
            }
            Wait.ForIdle();

            WinUISampleAppTestsUtils.VerifyNoErrorReport();
            WinUISampleAppTestsUtils.InvokeButton("buttonDiscardWindow");
            WinUISampleAppTestsUtils.VerifyText("textBoxLastEvent", "ButtonDiscardWindow_Click - exit");
            WinUISampleAppTestsUtils.VerifyNoErrorReport();
        }

        [TestMethod]
        [Description("Use the DispatcherQueue property of the MainWindow.")]
        public void MainWindowDispatcherQueueTest()
        {
            UseWindowDispatcherQueue("MainWindow");
        }

        [TestMethod]
        [Description("Use the DispatcherQueue property of a new Window with XAML markup.")]
        public void MarkupWindowDispatcherQueueTest()
        {
            UseWindowDispatcherQueue("MarkupWindow");
        }

        [TestMethod]
        [Description("Use the DispatcherQueue property of a new Window.")]
        public void RuntimeWindowDispatcherQueueTest()
        {
            UseWindowDispatcherQueue("RuntimeWindow");
        }

        [TestMethod]
        [Description("Minimize the window and ensure there's no Window.SizeChanged event.")]
        public void MinimizeTest()
        {
            SelectWindowUI();
            WinUISampleAppTestsUtils.ClearLogEvents();

            Log.Comment("> Minimizing the window");
            TestHelpers.MinimizeWindow();
            Wait.ForIdle();
            Log.Comment("> Restoring the window");
            TestHelpers.RestoreWindow(true);

            string[] expectedLogEvents = new string[5];
            int logIndex = 0;
            // No Window_SizeChanged notification for 0x0 here...
            expectedLogEvents[logIndex++] = "Window_VisibilityChanged for MainWindow - Handled=False, Visible=False";
            expectedLogEvents[logIndex++] = "Window_Activated for MainWindow - Handled=False, WindowActivationState=Deactivated";
            expectedLogEvents[logIndex++] = "Window_Activated for MainWindow - Handled=False, WindowActivationState=CodeActivated";
            // ...and No Window_SizeChanged notification for restored size here.
            expectedLogEvents[logIndex++] = "Window_VisibilityChanged for MainWindow - Handled=False, Visible=True";
            expectedLogEvents[logIndex++] = "Window_Activated for MainWindow - Handled=False, WindowActivationState=CodeActivated";
            WinUISampleAppTestsUtils.VerifyLogEvents(expectedLogEvents);
        }

        private void UseWindowDispatcherQueue(string windowName)
        {
            SelectWindowUI();
            ClearWindow();
            WinUISampleAppTestsUtils.ClearErrorReport();
            SelectWindowName(windowName);

            if (windowName != "MainWindow")
            {
                WinUISampleAppTestsUtils.InvokeButton("buttonCreateWindow");
                WinUISampleAppTestsUtils.VerifyText("textBoxLastEvent", "ButtonCreateWindow_Click - exit");
                WinUISampleAppTestsUtils.VerifyNoErrorReport();
            }

            WinUISampleAppTestsUtils.InvokeButton("buttonUseDispatcherQueue");
            WinUISampleAppTestsUtils.VerifyText("textBoxLastEvent", "ButtonUseDispatcherQueue_Click - async enqueued work");
            WinUISampleAppTestsUtils.VerifyNoErrorReport();

            if (windowName != "MainWindow")
            {
                WinUISampleAppTestsUtils.InvokeButton("buttonDiscardWindow");
                WinUISampleAppTestsUtils.VerifyText("textBoxLastEvent", "ButtonDiscardWindow_Click - exit");
                WinUISampleAppTestsUtils.VerifyNoErrorReport();
            }
        }

        private void SelectWindowUI()
        {
            ClickSafeElement();

            var navigationViewItemWindow = FindElement.ByName("navigationViewItemWindow");
            Verify.IsNotNull(navigationViewItemWindow);
            navigationViewItemWindow.SetFocus();
            Verify.IsTrue(navigationViewItemWindow.HasKeyboardFocus);
            if (!Convert.ToBoolean(navigationViewItemWindow.GetProperty(UIProperty.Get("SelectionItem.IsSelected"))))
            {
                Log.Comment("Selecting navigationViewItemWindow with keyboard");
                KeyboardHelper.PressKey(Key.Enter);
                Wait.ForIdle();
                Verify.IsTrue(Convert.ToBoolean(navigationViewItemWindow.GetProperty(UIProperty.Get("SelectionItem.IsSelected"))));
            }
        }

        private void SelectWindowName(string windowName)
        {
            Log.Comment($"Selecting window {windowName}");
            ComboBox comboBoxWindowName = new ComboBox(FindElement.ByName("comboBoxWindowName"));
            Verify.IsNotNull(comboBoxWindowName);
            if (comboBoxWindowName.Selection[0].Name != windowName)
            {
                comboBoxWindowName.SelectItemByName(windowName);
                Wait.ForIdle();
            }
            Log.Comment($"Selected window is now {comboBoxWindowName.Selection[0].Name}");
        }

        private void ClearWindow()
        {
            ComboBox comboBoxWindowName = new ComboBox(FindElement.ByName("comboBoxWindowName"));
            Verify.IsNotNull(comboBoxWindowName);
            if (comboBoxWindowName.Selection[0].Name == "MainWindow")
            {
                return;
            }
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
        [Description("Verify that an ElementName Binding in a Window can resolve a Source that lives on a non-Entered property")]
        public void ElementNameBindingInWindowTest()
        {
            SelectBindingUI();

            // Target TextBlock's Text is bound via ElementName to the Text of a TextBlock that
            // lives on the NavigationView via custom attached property
            WinUISampleAppTestsUtils.VerifyText("elementNameBindingTextBlock", "lorem ipsum");
        }

        private void SelectBindingUI()
        {
            ClickSafeElement();

            var navigationViewItemBinding = FindElement.ByName("navigationViewItemBinding");
            Verify.IsNotNull(navigationViewItemBinding);
            navigationViewItemBinding.SetFocus();
            Verify.IsTrue(navigationViewItemBinding.HasKeyboardFocus);
            if (!Convert.ToBoolean(navigationViewItemBinding.GetProperty(UIProperty.Get("SelectionItem.IsSelected"))))
            {
                Log.Comment("Selecting navigationViewItemBinding with keyboard");
                KeyboardHelper.PressKey(Key.Enter);
                Wait.ForIdle();
                Verify.IsTrue(Convert.ToBoolean(navigationViewItemBinding.GetProperty(UIProperty.Get("SelectionItem.IsSelected"))));
            }
        }

        [TestMethod]
        [Description("Verify that more than one ContentDialog with PlacementMode=EntireControlInPopup can be opened per Window/XamlRoot.")]
        public void MultipleOpenContentDialogsTest()
        {
            SelectWindowUI();
            ClearWindow();
            WinUISampleAppTestsUtils.ClearErrorReport();
            WinUISampleAppTestsUtils.InvokeButton("buttonOpenContentDialog");
            WinUISampleAppTestsUtils.VerifyText("textBoxLastEvent", "ContentDialog_opened");
            SelectWindowName("MarkupWindow");
            WinUISampleAppTestsUtils.InvokeButton("buttonCreateWindow");
            WinUISampleAppTestsUtils.VerifyNoErrorReport();
            WinUISampleAppTestsUtils.InvokeButton("buttonActivateWindow");
            WinUISampleAppTestsUtils.VerifyNoErrorReport();

            UIObject markupWindowRoot = FindElement.GetDesktopTopLevelWindow("WinUI Desktop - Markup Window");
            Verify.IsNotNull(markupWindowRoot);
         
            WinUISampleAppTestsUtils.InvokeButtonForRoot(markupWindowRoot, "buttonOpenContentDialog");
            WinUISampleAppTestsUtils.VerifyTextForRoot(markupWindowRoot, "textBlockContentDialogStatus", "Opened");

            WinUISampleAppTestsUtils.InvokeButtonForRoot(markupWindowRoot, "buttonCloseContentDialog");
            WinUISampleAppTestsUtils.VerifyTextForRoot(markupWindowRoot, "textBlockContentDialogStatus", "Closed");

            WinUISampleAppTestsUtils.InvokeButton("buttonCloseWindow");
            WinUISampleAppTestsUtils.VerifyNoErrorReport();
            WinUISampleAppTestsUtils.InvokeButton("buttonCloseContentDialog");
            WinUISampleAppTestsUtils.VerifyText("textBoxLastEvent", "ContentDialog_closed");
        }

        #if MUX_PRERELEASE // requires multi-window; task 31465879
        [TestMethod]
        [Description("Verify that the user can right-click on TextBoxes in two different Windows and get a context menu on the correct Window")]
        #endif
        public void TextBoxContextMenuWorksOnMultipleWindows()
        {
            SelectWindowUI();
            ClearWindow();
            WinUISampleAppTestsUtils.ClearErrorReport();

            UIObject mainWindow = TestEnvironment.Application.ApplicationFrameWindow ?? TestEnvironment.Application.CoreWindow;

            Log.Comment("Open MarkupWindow");
            SelectWindowName("MarkupWindow");
            WinUISampleAppTestsUtils.InvokeButton("buttonCreateWindow");
            WinUISampleAppTestsUtils.VerifyNoErrorReport();
            WinUISampleAppTestsUtils.InvokeButton("buttonActivateWindow");

            // Find the secondary window -- named "WinUI Desktop - Markup Window"
            Log.Comment("Find the MarkupWindow");
            var markupWindow = FindElement.GetDesktopTopLevelWindow("WinUI Desktop - Markup Window");

            Verify.AreEqual(TestHelpers.NativeMethods.GetForegroundWindow(), mainWindow.NativeWindowHandle);

            Log.Comment("Activate and work with MarkupWindow");

            //
            // As we work with the MarkupWindow, we have to avoid a many of the helper functions we normally use, because they'll
            // activate the MainWindow again.
            //
            Log.Comment("Right-click on markupWindowTextBox and expect a context menu with an entry for 'Select All'");
            RightClickTextBoxAndLookForContextMenu(markupWindow, "markupWindowTextBox");

            // Back to the MainWindow!
            Log.Comment("Back to the MainWindow: Right-click on mainWindowTextBox and expect a context menu with an entry for 'Select All'");
            RightClickTextBoxAndLookForContextMenu(mainWindow, "mainWindowTextBox");

            Log.Comment("One more time on the MarkupWindow");
            RightClickTextBoxAndLookForContextMenu(markupWindow, "markupWindowTextBox");

            Log.Comment("Press ESC to close the popup; otherwise, closing the window with the popup open will trigger bug 38478786");
            // Note: Wait.ForIdle does not work correctly when you have multiple Windows, because it will always cause the main Window to get focus
            KeyboardHelper.PressKey(Key.Escape, skipWait: true);
            Thread.Sleep(500);

            Log.Comment("ClickSafeElement to ensure focus is back on the MainWindow");
            var safeElementToClick = FindElement.ByName("safeElementToClick");
            safeElementToClick.SetFocus();
            InputHelper.LeftClick(safeElementToClick);

            WinUISampleAppTestsUtils.InvokeButton("buttonCloseWindow");
            WinUISampleAppTestsUtils.VerifyNoErrorReport();
        }

        void RightClickTextBoxAndLookForContextMenu(UIObject topLevelWindow, string textBoxName)
        {
            UICondition condition = UICondition.CreateFromName(textBoxName);
            UIObject textBox = null;                  
            topLevelWindow.Descendants.TryFind(condition, out textBox);
            textBox.SetFocus();
            Thread.Sleep(500);

            Verify.AreEqual(TestHelpers.NativeMethods.GetForegroundWindow(), topLevelWindow.NativeWindowHandle);

            textBox.Click(PointerButtons.Secondary, 30, 13);

            UIObject selectAllButton = null; 
            var selectAllCondition = UICondition.CreateFromName("Select All");
            topLevelWindow.Descendants.TryFind(selectAllCondition, out selectAllButton);

            Verify.IsNotNull(selectAllButton);
        }

        [TestMethod]
        [Description("Verify that a keyboard accelerator event goes only to the focused window")]
        public void VerifyMultiWindowAccelerator()
        {
            const string initialText = "lorem ipsum";
            const string changedText = "go bears!";
            const int windowCount = 3;

            WinUISampleAppTestsUtils.VerifyText("TestTextBlock", null);

            // Create the desired number of windows
            for (int i = 1; i < windowCount; i++)
            {
                WinUISampleAppTestsUtils.InvokeButton("CreateWindowButton");    
            }

            // Save references to each MainWindow
            var windowRoots = new List<UIObject>();
            for (int index = 0; index < windowCount; index++)
            {
                // Find the 'index'th MainWindow
                var root = FindElement.GetDesktopTopLevelWindow($"WinUI Desktop - {index}");
                windowRoots.Add(root);
            }

            for (int index = 0; index < windowCount; index++)
            {
                var root = windowRoots[index];

                Log.Comment($"Verifying Window {index} '{root.Name}'");

                // Maximize the window we are testing so it's in the foreground
                var window = new WinAppsTestWindow(root);
                if (window.CanMaximize)
                {
                    window.SetWindowVisualState(WindowVisualState.Maximized);
                }

                // Ensure the 'index'th MainWindow has focus
                ClickSafeElement(index);

                // Verify initial text
                WinUISampleAppTestsUtils.VerifyTextForRoot(root, "AcceleratorTextBlock", initialText);

                // Send accelerator (Ctrl-j) to toggle text
                KeyboardHelper.PressKey(root, Key.j, ModifierKey.Control);

                // Verify new text
                WinUISampleAppTestsUtils.VerifyTextForRoot(root, "AcceleratorTextBlock", changedText);

                // Verify that text in other windows did not change
                foreach (var otherIndex in Enumerable.Range(0, windowCount).Where(i => i != index))
                {
                    Log.Comment($"Verifying accelerator was not processed by Window {otherIndex}...");

                    var otherRoot = windowRoots[otherIndex];
                    WinUISampleAppTestsUtils.VerifyTextForRoot(otherRoot, "AcceleratorTextBlock", initialText);

                    Log.Comment("...finished.");
                }

                // Send accelerator to toggle the text back
                KeyboardHelper.PressKey(root, Key.j, ModifierKey.Control);
                WinUISampleAppTestsUtils.VerifyTextForRoot(root, "AcceleratorTextBlock", initialText);

                Log.Comment($"Finished verifying Window {index}");
            }

            Log.Comment("The process sometimes doesn't exit cleanly after this test, and the infra force-kills it.  Investigate: http://task.ms/44784011");
        }

        [TestMethod]
        [Description("Verify that when the RootScrollViewer of the window has focus, events still tunnel properly")]
        public void PreviewKeyDownOnRootScrollViewerTest()
        {
            SelectKeyPressUI();

            // Click on the textblock to give focus to page
            var downTextBlock = FindElement.ByName("downTextBlock");
            InputHelper.LeftClick(downTextBlock);
            Wait.ForIdle();

            // Press a key
            KeyboardHelper.PressKey(Key.Space);
            Wait.ForIdle();

            WinUISampleAppTestsUtils.VerifyText("downTextBlock", "Last key down detected was: Space");
        }

        [TestMethod]
        [Description("Verify that when focus is in a popup, event handlers on the XamlRoot contents still receieve tunneling events")]
        public void PreviewKeyUpOnPopupOpenTest()
        {
            SelectKeyPressUI();

            // Invoke the button to open the popup
            WinUISampleAppTestsUtils.InvokeButton("openPopupButton");
            Wait.ForIdle();

            // Focus on the button in the popup to make sure it has focus
            var popupButton = FindElement.ByName("popupButton");
            Verify.IsNotNull(popupButton);
            popupButton.SetFocus();
            Wait.ForIdle();

            // Press a key
            KeyboardHelper.PressKey(Key.Space);
            Wait.ForIdle();

            WinUISampleAppTestsUtils.VerifyText("upTextBlock", "Last key up detected was: Space");
        }

        private void SelectKeyPressUI()
        {
            ClickSafeElement();

            var navigationViewItemKeyPress = FindElement.ByName("navigationViewItemKeyPress");
            Verify.IsNotNull(navigationViewItemKeyPress);
            navigationViewItemKeyPress.SetFocus();
            Verify.IsTrue(navigationViewItemKeyPress.HasKeyboardFocus);
            if (!Convert.ToBoolean(navigationViewItemKeyPress.GetProperty(UIProperty.Get("SelectionItem.IsSelected"))))
            {
                Log.Comment("Selecting navigationViewItemKeyPress with keyboard");
                KeyboardHelper.PressKey(Key.Enter);
                Wait.ForIdle();
                Verify.IsTrue(Convert.ToBoolean(navigationViewItemKeyPress.GetProperty(UIProperty.Get("SelectionItem.IsSelected"))));
            }
        }
    }
}