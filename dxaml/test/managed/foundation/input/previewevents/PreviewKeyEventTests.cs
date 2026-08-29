// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.
using System;
using System.Linq;
using System.Text;
using WEX.Logging.Interop;
using WEX.TestExecution;
using WEX.TestExecution.Markup;
using System.Threading.Tasks;

using Private.Infrastructure;
using Microsoft.UI.Xaml.Tests.Common;
using Microsoft.UI.Xaml.Tests.Common.EventsListeners;

using Microsoft.UI.Xaml;
using Microsoft.UI.Xaml.Controls;
using Microsoft.UI.Xaml.Documents;
using Microsoft.UI.Xaml.Input;
using Windows.Foundation;
using Microsoft.UI.Xaml.Markup;
using Microsoft.UI.Xaml.Media;
using Windows.UI.ViewManagement;
using Microsoft.UI.Xaml.Controls.Primitives;
using System.Threading;
using System.Diagnostics;

namespace Microsoft.UI.Xaml.Tests.Input
{
    [TestClass]
    public partial class PreviewKeyEventTests : XamlTestsBase
    {
        public static bool isWpfHostingMode = false;

        [ClassInitialize]
        [TestProperty("BinaryUnderTest", "Microsoft.UI.Xaml.dll")]
        [TestProperty("RunAs", "UAP")]
        [TestProperty("UAP:Praid", "XamlManagedTAEFTests")]
        [TestProperty("Classification", "Integration")]
        [TestProperty("VelocityTestPass:OneCoreStrict", "Desktop")]
        public static void Setup(TestContext context)
        {
            AssemblySetup.CommonTestClassSetup();
            isWpfHostingMode = string.Compare(context.Properties["HostingMode"] as string, "WPF", ignoreCase: true) == 0;
        }

        [ClassCleanup]
        public void ClassCleanup()
        {
            base.CommonClassCleanup();
        }

        [TestMethod]
        [TestProperty("Description", "Verifies the order of keydown events")]
        public void VerifyKeyEventOrder()
        {
            const string rootPanelXaml =
                    @"<StackPanel Orientation='Vertical' xmlns='http://schemas.microsoft.com/winfx/2006/xaml/presentation' xmlns:x='http://schemas.microsoft.com/winfx/2006/xaml' x:Name='rootPanel'>
                        <StackPanel x:Name='innerPanel'>
                            <Button Width='50' x:Name='button1' Content='Button 1'/>
                        </StackPanel>
                        <Button x:Name='button2' Content='Button2' />
                    </StackPanel>";

            StackPanel rootPanel = null;
            StackPanel innerPanel = null;
            Button button1 = null;
            Button button2 = null;
            FrameworkElement[] elementList = null;
            StringBuilder eventOrder = null;

            UIExecutor.Execute(() =>
            {
                rootPanel = (StackPanel)XamlReader.Load(rootPanelXaml);
                button1 = (Button)rootPanel.FindName("button1");
                button2 = (Button)rootPanel.FindName("button2");
                innerPanel = (StackPanel)rootPanel.FindName("innerPanel");

                elementList = new FrameworkElement[4] { button1, button2, innerPanel, rootPanel };
                eventOrder = new StringBuilder();
                TestServices.WindowHelper.WindowContent = rootPanel;
            });

            TestServices.WindowHelper.WaitForIdle();

            Log.Comment("Focus button 1");
            FocusHelper.EnsureFocus(button1, FocusState.Keyboard);

            using (var KeyEventOrderingTester = new KeyEventOrderingTester(elementList, eventOrder))
            using (var stackPanelGotKeyUp = new EventTester<StackPanel, KeyRoutedEventArgs>(rootPanel, "KeyUp"))
            {
                Log.Comment("Press key w");
                TestServices.KeyboardHelper.PressKeySequence("w");

                stackPanelGotKeyUp.Wait();
                UIExecutor.Execute(() =>
                {
                    string expectedEventOrder = "[rootPanelPreviewKeyDown:W][innerPanelPreviewKeyDown:W][button1PreviewKeyDown:W]" +
                                                "[button1KeyDown:W][innerPanelKeyDown:W][rootPanelKeyDown:W]" +
                                                "[rootPanelPreviewKeyUp:W][innerPanelPreviewKeyUp:W][button1PreviewKeyUp:W]"+
                                                "[button1KeyUp:W][innerPanelKeyUp:W][rootPanelKeyUp:W]";
                    Verify.AreEqual(expectedEventOrder, eventOrder.ToString());
                });
            }
        }

        [TestMethod]
        [TestProperty("Description", "Verifies the order of keydown events and overrides")]
        [TestProperty("Hosting:Mode", "UAP")]  // fails in WPF mode in catgates
        public void VerifyPreviewKeyOverridesEventOrder()
        {
            const string rootPanelXaml =
                    @"<StackPanel Orientation='Vertical' xmlns='http://schemas.microsoft.com/winfx/2006/xaml/presentation' xmlns:x='http://schemas.microsoft.com/winfx/2006/xaml' x:Name='rootPanel'>
                        <Button x:Name='button2' Content='Button2' />
                    </StackPanel>";

            StackPanel rootPanel = null;
            ButtonWithOnPreviewKeyOverrides button1 = null;
            Button button2 = null;
            FrameworkElement[] elementList = null;


            UIExecutor.Execute(() =>
            {
                rootPanel = (StackPanel)XamlReader.Load(rootPanelXaml);
                button1 = new ButtonWithOnPreviewKeyOverrides();
                button1.Name = "button1";

                button2 = (Button)rootPanel.FindName("button2");

                rootPanel.Children.Add(button1);

                elementList = new FrameworkElement[3] { button1, button2, rootPanel };
                button1.eventOrder = new StringBuilder();
                TestServices.WindowHelper.WindowContent = rootPanel;
            });

            TestServices.WindowHelper.WaitForIdle();

            Log.Comment("Focus button 1");
            FocusHelper.EnsureFocus(button1, FocusState.Keyboard);

            using (var KeyEventOrderingTester = new KeyEventOrderingTester(elementList, button1.eventOrder))
            using (var stackPanelGotKeyUp = EventTester<StackPanel, KeyRoutedEventArgs>.FromRoutedEvent(rootPanel, "KeyUp", (t, u) => { } /*No action required*/))
            {
                Log.Comment("Press key w");
                TestServices.KeyboardHelper.PressKeySequence("w");

                stackPanelGotKeyUp.Wait();
                UIExecutor.Execute(() =>
                {
                    string expectedEventOrder = 
                    "[rootPanelPreviewKeyDown:W][button1OnPreviewKeyDown:W][button1PreviewKeyDown:W:Handled]" +
                    "[button1KeyDown:W:Handled][rootPanelKeyDown:W:Handled]" +
                    "[rootPanelPreviewKeyUp:W][button1OnPreviewKeyUp:W][button1PreviewKeyUp:W:Handled]" +
                    "[button1KeyUp:W:Handled][rootPanelKeyUp:W:Handled]";
                    Verify.AreEqual(expectedEventOrder, button1.eventOrder.ToString());
                    Verify.IsTrue(button1.HasPreviewKeyDownFired);
                    Verify.IsTrue(button1.HasPreviewKeyUpFired);
                });
            }
        }
        [TestMethod]
        [TestProperty("Description", "Verifies preview key override events fire even when there are no subscribers.")]
        public void VerifyPreviewKeyOverridesFireWhenNoSubscribers()
        {
            const string rootPanelXaml =
                    @"<StackPanel Orientation='Vertical' xmlns='http://schemas.microsoft.com/winfx/2006/xaml/presentation' xmlns:x='http://schemas.microsoft.com/winfx/2006/xaml' x:Name='rootPanel'>
                        <Button x:Name='button2' Content='Button2' />
                    </StackPanel>";

            StackPanel rootPanel = null;
            ButtonWithOnPreviewKeyOverrides button1 = null;

            UIExecutor.Execute(() =>
            {
                rootPanel = (StackPanel)XamlReader.Load(rootPanelXaml);
                button1 = new ButtonWithOnPreviewKeyOverrides();
                button1.Name = "button1";
                rootPanel.Children.Add(button1);
                TestServices.WindowHelper.WindowContent = rootPanel;
            });

            TestServices.WindowHelper.WaitForIdle();

            Log.Comment("Focus button 1");
            FocusHelper.EnsureFocus(button1, FocusState.Keyboard);

            using (var stackPanelGotKeyUp = EventTester<StackPanel, KeyRoutedEventArgs>.FromRoutedEvent(rootPanel, "KeyUp", (t, u) => { } /*No action required*/))
            {
                Log.Comment("Press key w");
                TestServices.KeyboardHelper.PressKeySequence("w");

                button1.WaitForOnPreviewKeyDown();
                button1.WaitForOnPreviewKeyUp();
                stackPanelGotKeyUp.Wait();
            }
        }

        [TestMethod]
        [TestProperty("Description", "Verifies the order of keydown events for several keys")]
        public void VerifyKeyEventOrderForLongKeySequence()
        {
            const string rootPanelXaml =
                    @"<StackPanel Orientation='Vertical' xmlns='http://schemas.microsoft.com/winfx/2006/xaml/presentation' xmlns:x='http://schemas.microsoft.com/winfx/2006/xaml' x:Name='rootPanel'>
                        <StackPanel x:Name='innerPanel'>
                            <Button Width='50' x:Name='button1' Content='Button 1'/>
                        </StackPanel>
                        <Button x:Name='button2' Content='Button2' />
                    </StackPanel>";

            StackPanel rootPanel = null;
            StackPanel innerPanel = null;
            Button button1 = null;
            Button button2 = null;
            FrameworkElement[] elementList = null;
            StringBuilder eventOrder = null;

            UIExecutor.Execute(() =>
            {
                rootPanel = (StackPanel)XamlReader.Load(rootPanelXaml);
                button1 = (Button)rootPanel.FindName("button1");
                button2 = (Button)rootPanel.FindName("button2");
                innerPanel = (StackPanel)rootPanel.FindName("innerPanel");

                elementList = new FrameworkElement[4] { button1, button2, innerPanel, rootPanel };
                eventOrder = new StringBuilder();
                TestServices.WindowHelper.WindowContent = rootPanel;
            });

            TestServices.WindowHelper.WaitForIdle();

            Log.Comment("Focus button 1");
            FocusHelper.EnsureFocus(button1, FocusState.Keyboard);

            using (var KeyEventOrderingTester = new KeyEventOrderingTester(elementList, eventOrder))
            {
                using (var stackPanelGotKeyUp = new EventTester<StackPanel, KeyRoutedEventArgs>(rootPanel, "KeyUp"))
                {
                    Log.Comment("Press long key sequence.");
                    TestServices.KeyboardHelper.PressKeySequence("$d$_a#$d$_v#$u$_v");
                    stackPanelGotKeyUp.Wait();
                }

                using (var stackPanelGotKeyUp = new EventTester<StackPanel, KeyRoutedEventArgs>(rootPanel, "KeyUp"))
                {
                    Log.Comment("Press long key sequence.");
                    TestServices.KeyboardHelper.PressKeySequence("$d$_q#$u$_q");
                    stackPanelGotKeyUp.Wait();
                }

                using (var stackPanelGotKeyUp = new EventTester<StackPanel, KeyRoutedEventArgs>(rootPanel, "KeyUp"))
                {
                    TestServices.KeyboardHelper.PressKeySequence("$u$_a");
                    stackPanelGotKeyUp.Wait();
                    UIExecutor.Execute(() =>
                    {
                        string expectedEventOrder = "[rootPanelPreviewKeyDown:A][innerPanelPreviewKeyDown:A][button1PreviewKeyDown:A]" +
                                                    "[button1KeyDown:A][innerPanelKeyDown:A][rootPanelKeyDown:A]" +
                                                    "[rootPanelPreviewKeyDown:V][innerPanelPreviewKeyDown:V][button1PreviewKeyDown:V]" +
                                                    "[button1KeyDown:V][innerPanelKeyDown:V][rootPanelKeyDown:V]" +
                                                    "[rootPanelPreviewKeyUp:V][innerPanelPreviewKeyUp:V][button1PreviewKeyUp:V]" +
                                                    "[button1KeyUp:V][innerPanelKeyUp:V][rootPanelKeyUp:V]" +
                                                    "[rootPanelPreviewKeyDown:Q][innerPanelPreviewKeyDown:Q][button1PreviewKeyDown:Q]" +
                                                    "[button1KeyDown:Q][innerPanelKeyDown:Q][rootPanelKeyDown:Q]" +
                                                    "[rootPanelPreviewKeyUp:Q][innerPanelPreviewKeyUp:Q][button1PreviewKeyUp:Q]" +
                                                    "[button1KeyUp:Q][innerPanelKeyUp:Q][rootPanelKeyUp:Q]" +
                                                    "[rootPanelPreviewKeyUp:A][innerPanelPreviewKeyUp:A][button1PreviewKeyUp:A]" +
                                                    "[button1KeyUp:A][innerPanelKeyUp:A][rootPanelKeyUp:A]";
                        Verify.AreEqual(expectedEventOrder, eventOrder.ToString());
                    });
                }
            }
        }

        [TestMethod]
        [TestProperty("Description", "Verifies the order of keydown events from a popup")]
        [TestProperty("Hosting:Mode", "UAP")]
        public void VerifyKeyEventOrderFromPopup()
        {
            const string rootPanelXaml =
                    @"<StackPanel Orientation='Vertical' xmlns='http://schemas.microsoft.com/winfx/2006/xaml/presentation' xmlns:x='http://schemas.microsoft.com/winfx/2006/xaml' x:Name='rootPanel'>
                        <StackPanel x:Name='innerPanel'>
                            <Button Width='50' x:Name='button1' Content='Button 1'/>
                            <Popup x:Name='popup' IsLightDismissEnabled='true'>
                                <Button x:Name='button2'/>
                            </Popup>
                        </StackPanel>
                    </StackPanel>";

            StackPanel rootPanel = null;
            StackPanel innerPanel = null;
            Button button1 = null;
            Button button2 = null;
            Popup popup = null;
            FrameworkElement[] elementList = null;
            StringBuilder eventOrder = null;

            UIExecutor.Execute(() =>
            {
                rootPanel = (StackPanel)XamlReader.Load(rootPanelXaml);
                button1 = (Button)rootPanel.FindName("button1");
                button2 = (Button)rootPanel.FindName("button2");
                popup = (Popup)rootPanel.FindName("popup");
                innerPanel = (StackPanel)rootPanel.FindName("innerPanel");

                elementList = new FrameworkElement[5] { button1, innerPanel, rootPanel, popup, button2 };
                eventOrder = new StringBuilder();
                TestServices.WindowHelper.WindowContent = rootPanel;
            });

            TestServices.WindowHelper.WaitForIdle();

            using (var popupOpened = new EventTester<Popup, object>(popup, "Opened"))
            {
                UIExecutor.Execute(() =>
                {
                    popup.IsOpen = true;
                });

                popupOpened.Wait();
                FocusHelper.EnsureFocus(button2, FocusState.Keyboard);
            }

            using (var KeyEventOrderingTester = new KeyEventOrderingTester(elementList, eventOrder))
            using (var rootPanelGotKeyUp = new EventTester<StackPanel, KeyRoutedEventArgs>(rootPanel, "KeyUp"))
            {
                Log.Comment("Press key w");
                TestServices.KeyboardHelper.PressKeySequence("w");

                rootPanelGotKeyUp.Wait();
                UIExecutor.Execute(() =>
                {
                    // In case of Key events, we route the key events from the popup root to the visual root (rootPanel).
                    string expectedEventOrder = "[rootPanelPreviewKeyDown:W][button2PreviewKeyDown:W]"+
                                                "[button2KeyDown:W][rootPanelKeyDown:W]" +
                                                "[rootPanelPreviewKeyUp:W][button2PreviewKeyUp:W]" +
                                                "[button2KeyUp:W][rootPanelKeyUp:W]";
                    Verify.AreEqual(expectedEventOrder, eventOrder.ToString());
                    popup.IsOpen = false;
                });
                TestServices.WindowHelper.WaitForIdle();

            }
        }

        [TestMethod]
        [TestProperty("Description", "Verifies the order of keydown events from a popup")]
        [TestProperty("Hosting:Mode", "UAP")]
        public void VerifyKeyEventOrderFromUnparentedPopup()
        {
            const string rootPanelXaml =
                    @"<StackPanel Orientation='Vertical' xmlns='http://schemas.microsoft.com/winfx/2006/xaml/presentation' xmlns:x='http://schemas.microsoft.com/winfx/2006/xaml' x:Name='rootPanel'>
                        <StackPanel x:Name='innerPanel'>
                            <Button Width='50' x:Name='button1' Content='Button 1'/>
                        </StackPanel>
                    </StackPanel>";

            StackPanel rootPanel = null;
            StackPanel innerPanel = null;
            Button button1 = null;
            Button button2 = null;
            Popup popup = null;
            FrameworkElement[] elementList = null;
            StringBuilder eventOrder = null;

            UIExecutor.Execute(() =>
            {
                rootPanel = (StackPanel)XamlReader.Load(rootPanelXaml);
                button1 = (Button)rootPanel.FindName("button1");
                popup = new Popup();
                popup.Name = "popup";
                button2 = new Button();
                button2.Name = "button2";
                popup.Child = button2;

                innerPanel = (StackPanel)rootPanel.FindName("innerPanel");

                elementList = new FrameworkElement[5] { rootPanel, button1, button2, innerPanel, popup};
                eventOrder = new StringBuilder();
                TestServices.WindowHelper.WindowContent = rootPanel;
            });

            TestServices.WindowHelper.WaitForIdle();

            using (var popupOpened = new EventTester<Popup, object>(popup, "Opened"))
            {
                UIExecutor.Execute(() =>
                {
                    popup.IsOpen = true;
                });

                popupOpened.Wait();
                FocusHelper.EnsureFocus(button2, FocusState.Keyboard);
            }

            using (var KeyEventOrderingTester = new KeyEventOrderingTester(elementList, eventOrder))
            using (var rootPanelGotKeyUp = new EventTester<StackPanel, KeyRoutedEventArgs>(rootPanel, "KeyUp"))
            {
                Log.Comment("Press key w");
                TestServices.KeyboardHelper.PressKeySequence("w");

                rootPanelGotKeyUp.Wait();
                UIExecutor.Execute(() =>
                {
                    // In case of Key events, we route the key events from the popup root to the visual root (rootPanel).
                    string expectedEventOrder = "[rootPanelPreviewKeyDown:W][button2PreviewKeyDown:W]" +
                                                "[button2KeyDown:W][rootPanelKeyDown:W]"+
                                                "[rootPanelPreviewKeyUp:W][button2PreviewKeyUp:W]" +
                                                "[button2KeyUp:W][rootPanelKeyUp:W]";
                    Verify.AreEqual(expectedEventOrder, eventOrder.ToString());
                    popup.IsOpen = false;
                });
                TestServices.WindowHelper.WaitForIdle();

            }
        }

        [TestMethod]
        [TestProperty("Description", "Verifies the order of keydown events during engagement.")]
        public void VerifyKeyEventOrderWithEngagement()
        {
            const string rootPanelXaml =
                    @"<StackPanel Orientation='Vertical' xmlns='http://schemas.microsoft.com/winfx/2006/xaml/presentation' xmlns:x='http://schemas.microsoft.com/winfx/2006/xaml' x:Name='rootPanel'>
                            <Button Width='50' x:Name='button0' Content='Button 0'/>
                            <UserControl x:Name='engagedControl' IsFocusEngagementEnabled='true'>
                                <StackPanel x:Name='innerPanel'>
                                    <Button x:Name='focusTrap'> FocusTrap </Button>
                                    <Button Width='50' x:Name='button1' Content='Button 1'/>
                                </StackPanel>
                            </UserControl>
                    </StackPanel>";

            StackPanel rootPanel = null;
            StackPanel innerPanel = null;
            UserControl engagedControl = null;
            Button button0 = null;
            Button button1 = null;
            Button focusTrap = null;
            FrameworkElement[] elementList = null;
            StringBuilder eventOrder = null;

            var verifyRightShoulder = new Action<object, KeyRoutedEventArgs>((source, args) =>
            {
                Verify.AreEqual(global::Windows.System.VirtualKey.GamepadRightShoulder, args.OriginalKey);
            });

            var verifyBKey = new Action<object, KeyRoutedEventArgs>((source, args) =>
            {
                Verify.AreEqual(global::Windows.System.VirtualKey.GamepadB, args.OriginalKey);
            });

            UIExecutor.Execute(() =>
            {
                rootPanel = (StackPanel)XamlReader.Load(rootPanelXaml);
                button0 = (Button)rootPanel.FindName("button0");
                button1 = (Button)rootPanel.FindName("button1");
                focusTrap = (Button)rootPanel.FindName("focusTrap");
                innerPanel = (StackPanel)rootPanel.FindName("innerPanel");
                engagedControl = (UserControl)rootPanel.FindName("engagedControl");
                elementList = new FrameworkElement[6] { button0, button1, focusTrap, innerPanel, rootPanel, engagedControl };
                eventOrder = new StringBuilder();
                TestServices.WindowHelper.WindowContent = rootPanel;
            });
            TestServices.WindowHelper.WaitForIdle();

            Log.Comment("Focus button0");
            FocusHelper.EnsureFocus(button0, FocusState.Keyboard);


            using (var KeyEventOrderingTester = new KeyEventOrderingTester(elementList, eventOrder))
            {
                using (var focusEngaged = new EventTester<UserControl, FocusEngagedEventArgs>(engagedControl, "FocusEngaged"))
                using (var button1GotFocus = new EventTester<Button, RoutedEventArgs>(button1, "GotFocus"))
                {
                    Log.Comment("Injecting Gamepad Down");
                    TestServices.KeyboardHelper.GamepadDpadDown();

                    Log.Comment("Injecting Gamepad A");
                    TestServices.KeyboardHelper.GamepadA();
                    focusEngaged.Wait();
                    Log.Comment("Focus engaged.");
                    TestServices.KeyboardHelper.GamepadDpadDown();
                    button1GotFocus.Wait();
                    Log.Comment("Button1 GotFocus.");
                }

                TestServices.WindowHelper.WaitForIdle();

                UIExecutor.Execute(() =>
                {
                    Verify.IsTrue(engagedControl.IsFocusEngaged);
                });

                TestServices.WindowHelper.WaitForIdle();

                using (var rootPanelGotKeyUp = EventTester<StackPanel, KeyRoutedEventArgs>.FromRoutedEvent(rootPanel, "KeyUp", verifyRightShoulder))
                {
                    Log.Comment("Press Gamepad right shoulder.");
                    TestServices.KeyboardHelper.GamepadRightShoulder();

                    rootPanelGotKeyUp.Wait();
                }

                using (var rootPanelGotKeyUp = EventTester<StackPanel, KeyRoutedEventArgs>.FromRoutedEvent(rootPanel, "KeyUp", verifyBKey))
                {
                    Log.Comment("Press Gamepad B.");
                    TestServices.KeyboardHelper.GamepadB();

                    rootPanelGotKeyUp.Wait();
                    TestServices.WindowHelper.WaitForIdle();
                    UIExecutor.Execute(() =>
                    {
                        string expectedEventOrder =
                        "[rootPanelPreviewKeyDown:GamepadDPadDown][button0PreviewKeyDown:GamepadDPadDown]" + //Gamepad Dpad
                        "[button0KeyDown:GamepadDPadDown][rootPanelKeyDown:GamepadDPadDown]" +
                        "[rootPanelPreviewKeyUp:GamepadDPadDown]" +
                        "[rootPanelKeyUp:GamepadDPadDown]" +
                        "[rootPanelPreviewKeyDown:GamepadA][rootPanelKeyDown:GamepadA]" +                   //Key Down Events not fired on engagedControl due to focus engagement logic.
                        "[rootPanelPreviewKeyUp:GamepadA][engagedControlPreviewKeyUp:GamepadA]" +
                        "[engagedControlKeyUp:GamepadA:Handled][rootPanelKeyUp:GamepadA:Handled]" +               //KeyUp is handled since we now engage engagedControl
                        "[rootPanelPreviewKeyDown:GamepadDPadDown][engagedControlPreviewKeyDown:GamepadDPadDown][innerPanelPreviewKeyDown:GamepadDPadDown][focusTrapPreviewKeyDown:GamepadDPadDown]" + 
                        "[focusTrapKeyDown:GamepadDPadDown][innerPanelKeyDown:GamepadDPadDown][engagedControlKeyDown:GamepadDPadDown][rootPanelKeyDown:GamepadDPadDown:Handled]" +    //Moving focus to button1
                        "[rootPanelPreviewKeyUp:GamepadDPadDown][engagedControlPreviewKeyUp:GamepadDPadDown][innerPanelPreviewKeyUp:GamepadDPadDown][button1PreviewKeyUp:GamepadDPadDown]" +
                        "[button1KeyUp:GamepadDPadDown][innerPanelKeyUp:GamepadDPadDown][engagedControlKeyUp:GamepadDPadDown][rootPanelKeyUp:GamepadDPadDown:Handled]" + //Coerced to handled due to engagement
                        "[rootPanelPreviewKeyDown:GamepadRightShoulder][engagedControlPreviewKeyDown:GamepadRightShoulder][innerPanelPreviewKeyDown:GamepadRightShoulder][button1PreviewKeyDown:GamepadRightShoulder]" + //Pressing RightShoulder
                        "[button1KeyDown:GamepadRightShoulder][innerPanelKeyDown:GamepadRightShoulder][engagedControlKeyDown:GamepadRightShoulder][rootPanelKeyDown:GamepadRightShoulder:Handled]" +
                        "[rootPanelPreviewKeyUp:GamepadRightShoulder][engagedControlPreviewKeyUp:GamepadRightShoulder][innerPanelPreviewKeyUp:GamepadRightShoulder][button1PreviewKeyUp:GamepadRightShoulder]" +
                        "[button1KeyUp:GamepadRightShoulder][innerPanelKeyUp:GamepadRightShoulder][engagedControlKeyUp:GamepadRightShoulder][rootPanelKeyUp:GamepadRightShoulder:Handled]" +
                        "[rootPanelPreviewKeyDown:GamepadB][engagedControlPreviewKeyDown:GamepadB][innerPanelPreviewKeyDown:GamepadB][button1PreviewKeyDown:GamepadB]" + //Disengaging with B Button
                        "[button1KeyDown:GamepadB][innerPanelKeyDown:GamepadB][engagedControlKeyDown:GamepadB][rootPanelKeyDown:GamepadB:Handled]" +
                        "[rootPanelPreviewKeyUp:GamepadB][engagedControlPreviewKeyUp:GamepadB][innerPanelPreviewKeyUp:GamepadB][button1PreviewKeyUp:GamepadB]" +
                        "[button1KeyUp:GamepadB][innerPanelKeyUp:GamepadB][engagedControlKeyUp:GamepadB][rootPanelKeyUp:GamepadB:Handled]";

                        Verify.AreEqual(expectedEventOrder, eventOrder.ToString());
                        Verify.IsFalse(engagedControl.IsFocusEngaged);
                    });
                }
            }
        }


        [TestMethod]
        [TestProperty("Description", "Simple scenario that \vVerifies the order of keydown events when engaging and disengaging.")]
        public void VerifyKeyEventOrderWhenEngaging()
        { 
                    const string rootPanelXaml =
                    @"<StackPanel Orientation='Vertical' xmlns='http://schemas.microsoft.com/winfx/2006/xaml/presentation' xmlns:x='http://schemas.microsoft.com/winfx/2006/xaml' x:Name='rootPanel'>
                            <Button Width='50' x:Name='button0' Content='Button 0'/>
                            <UserControl x:Name='engagedControl' IsFocusEngagementEnabled='true' IsTabStop='true'>
                                <StackPanel x:Name='innerPanel'>
                                    <Button x:Name='focusTrap'> FocusTrap </Button>
                                    <Button Width='50' x:Name='button1' Content='Button 1'/>
                                </StackPanel>
                            </UserControl>
                    </StackPanel>";

            StackPanel rootPanel = null;
            StackPanel innerPanel = null;
            UserControl engagedControl = null;
            Button button0 = null;
            Button button1 = null;
            Button focusTrap = null;
            FrameworkElement[] elementList = null;
            StringBuilder eventOrder = null;

            var preventDisengagement = new Action<object, KeyRoutedEventArgs>((source, args) =>
            {
                args.Handled = true;
            });

            var verifyBKey = new Action<object, KeyRoutedEventArgs>((source, args) =>
            {
                Verify.AreEqual(global::Windows.System.VirtualKey.GamepadB, args.OriginalKey);
            });

            UIExecutor.Execute(() =>
            {
                rootPanel = (StackPanel)XamlReader.Load(rootPanelXaml);
                button0 = (Button)rootPanel.FindName("button0");
                button1 = (Button)rootPanel.FindName("button1");
                focusTrap = (Button)rootPanel.FindName("focusTrap");
                innerPanel = (StackPanel)rootPanel.FindName("innerPanel");
                engagedControl = (UserControl)rootPanel.FindName("engagedControl");
                elementList = new FrameworkElement[6] { button0, button1, focusTrap, innerPanel, rootPanel, engagedControl };
                eventOrder = new StringBuilder();
                TestServices.WindowHelper.WindowContent = rootPanel;
            });
            TestServices.WindowHelper.WaitForIdle();

            Log.Comment("Focus engaged control.");
            FocusHelper.EnsureFocus(engagedControl, FocusState.Keyboard);

            using (var KeyEventOrderingTester = new KeyEventOrderingTester(elementList, eventOrder))
            {
                using (var focusEngaged = new EventTester<UserControl, FocusEngagedEventArgs>(engagedControl, "FocusEngaged"))
                using (var button1GotFocus = new EventTester<Button, RoutedEventArgs>(button1, "GotFocus"))
                {
                    Log.Comment("Injecting Gamepad A");
                    TestServices.KeyboardHelper.GamepadA();
                    focusEngaged.Wait();
                    Log.Comment("Focus engaged.");
                    TestServices.KeyboardHelper.GamepadDpadDown();
                    button1GotFocus.Wait();
                    Log.Comment("Button1 GotFocus.");
                }

                TestServices.WindowHelper.WaitForIdle();

                UIExecutor.Execute(() =>
                {
                    Verify.IsTrue(engagedControl.IsFocusEngaged);
                });

                TestServices.WindowHelper.WaitForIdle();

                using (var buttonGotKeyUp = EventTester<Button, KeyRoutedEventArgs>.FromRoutedEvent(button1, "KeyUp", preventDisengagement))
                {
                    Log.Comment("Press Gamepad B.");
                    TestServices.KeyboardHelper.GamepadB();
                    buttonGotKeyUp.Wait();

                    UIExecutor.Execute(() =>
                    {
                        Verify.IsTrue(engagedControl.IsFocusEngaged);
                    });

                    TestServices.WindowHelper.WaitForIdle();
                }

                using (var rootPanelGotKeyUp = EventTester<StackPanel, KeyRoutedEventArgs>.FromRoutedEvent(rootPanel, "KeyUp", verifyBKey))
                {
                    Log.Comment("Press Gamepad B.");
                    TestServices.KeyboardHelper.GamepadB();

                    rootPanelGotKeyUp.Wait();
                    TestServices.WindowHelper.WaitForIdle();
                    UIExecutor.Execute(() =>
                    {
                        string expectedEventOrder =
                        "[rootPanelPreviewKeyDown:GamepadA][rootPanelKeyDown:GamepadA]" +
                        "[rootPanelPreviewKeyUp:GamepadA][engagedControlPreviewKeyUp:GamepadA]" +
                        "[engagedControlKeyUp:GamepadA:Handled][rootPanelKeyUp:GamepadA:Handled]" +
                        "[rootPanelPreviewKeyDown:GamepadDPadDown][engagedControlPreviewKeyDown:GamepadDPadDown][innerPanelPreviewKeyDown:GamepadDPadDown][focusTrapPreviewKeyDown:GamepadDPadDown]" +
                        "[focusTrapKeyDown:GamepadDPadDown][innerPanelKeyDown:GamepadDPadDown][engagedControlKeyDown:GamepadDPadDown][rootPanelKeyDown:GamepadDPadDown:Handled]" +
                        "[rootPanelPreviewKeyUp:GamepadDPadDown][engagedControlPreviewKeyUp:GamepadDPadDown][innerPanelPreviewKeyUp:GamepadDPadDown][button1PreviewKeyUp:GamepadDPadDown]" +
                        "[button1KeyUp:GamepadDPadDown][innerPanelKeyUp:GamepadDPadDown][engagedControlKeyUp:GamepadDPadDown][rootPanelKeyUp:GamepadDPadDown:Handled]" +
                        "[rootPanelPreviewKeyDown:GamepadB][engagedControlPreviewKeyDown:GamepadB][innerPanelPreviewKeyDown:GamepadB][button1PreviewKeyDown:GamepadB]" +
                        "[button1KeyDown:GamepadB][innerPanelKeyDown:GamepadB][engagedControlKeyDown:GamepadB][rootPanelKeyDown:GamepadB:Handled]" +
                        "[rootPanelPreviewKeyUp:GamepadB][engagedControlPreviewKeyUp:GamepadB][innerPanelPreviewKeyUp:GamepadB][button1PreviewKeyUp:GamepadB]" +
                        "[button1KeyUp:GamepadB][innerPanelKeyUp:GamepadB:Handled][engagedControlKeyUp:GamepadB:Handled][rootPanelKeyUp:GamepadB:Handled]" +
                        "[rootPanelPreviewKeyDown:GamepadB][engagedControlPreviewKeyDown:GamepadB][innerPanelPreviewKeyDown:GamepadB][button1PreviewKeyDown:GamepadB]" +
                        "[button1KeyDown:GamepadB][innerPanelKeyDown:GamepadB][engagedControlKeyDown:GamepadB][rootPanelKeyDown:GamepadB:Handled]" +
                        "[rootPanelPreviewKeyUp:GamepadB][engagedControlPreviewKeyUp:GamepadB][innerPanelPreviewKeyUp:GamepadB][button1PreviewKeyUp:GamepadB]" +
                        "[button1KeyUp:GamepadB][innerPanelKeyUp:GamepadB][engagedControlKeyUp:GamepadB][rootPanelKeyUp:GamepadB:Handled]";

                        Verify.AreEqual(expectedEventOrder, eventOrder.ToString());
                        Verify.IsFalse(engagedControl.IsFocusEngaged);
                    });
                }
            }
        }

        [TestMethod]
        [TestProperty("Description", "Verifies the order of keydown events on reparenting")]
        [TestProperty("Hosting:Mode", "UAP")] // fails in WPF mode due to final release queue is not empty cleanup issue
        public void VerifyKeyDownEventOrderWhenReparenting()
        {
            const string rootPanelXaml =
                    @"<StackPanel Orientation='Vertical' xmlns='http://schemas.microsoft.com/winfx/2006/xaml/presentation' xmlns:x='http://schemas.microsoft.com/winfx/2006/xaml' x:Name='rootPanel'>
                        <Button Width='50' x:Name='button0' Content='Button 0'/>
                        <StackPanel x:Name='innerPanel1'>
                            <Button Width='50' x:Name='button1' Content='Button 1'/>
                        </StackPanel>
                        <StackPanel x:Name='innerPanel2'>
                        </StackPanel>
                    </StackPanel>";

            StackPanel rootPanel = null;
            StackPanel innerPanel1 = null;
            StackPanel innerPanel2 = null;
            Button button0 = null;
            Button button1 = null;
            FrameworkElement[] elementList = null;
            StringBuilder eventOrder = null;


            var innerPanelPreviewKeyDownHandler = new Action<object, KeyRoutedEventArgs>((source, args) =>
            {
                Log.Comment("Reparenting button1.");
                innerPanel1.Children.Clear();
                innerPanel2.Children.Add(button1);
                Log.Comment("Ensure button1 is still focused.");
                Verify.IsTrue(button1.Focus(FocusState.Keyboard));
            });


            UIExecutor.Execute(() =>
            {
                rootPanel = (StackPanel)XamlReader.Load(rootPanelXaml);
                button0 = (Button)rootPanel.FindName("button0");
                button1 = (Button)rootPanel.FindName("button1");
                innerPanel1 = (StackPanel)rootPanel.FindName("innerPanel1");
                innerPanel2 = (StackPanel)rootPanel.FindName("innerPanel2");

                elementList = new FrameworkElement[5] { button0, button1, innerPanel1, rootPanel, innerPanel2 };
                eventOrder = new StringBuilder();
                TestServices.WindowHelper.WindowContent = rootPanel;
            });

            TestServices.WindowHelper.WaitForIdle();

            Log.Comment("Focus button 1");
            FocusHelper.EnsureFocus(button1, FocusState.Keyboard);

            using (var KeyEventOrderingTester = new KeyEventOrderingTester(elementList, eventOrder))
            using (var reparentingPreviewEventHandler = new EventTester<StackPanel, KeyRoutedEventArgs>(innerPanel1, "PreviewKeyDown", innerPanelPreviewKeyDownHandler))
            using (var stackPanelGotKeyUp = new EventTester<StackPanel, KeyRoutedEventArgs>(rootPanel, "KeyUp"))
            {
                Log.Comment("Press key w");
                TestServices.KeyboardHelper.PressKeySequence("w");

                stackPanelGotKeyUp.Wait();
                UIExecutor.Execute(() =>
                {
                    Verify.IsTrue(reparentingPreviewEventHandler.HasFired);
                    string expectedEventOrder = "[rootPanelPreviewKeyDown:W][innerPanel1PreviewKeyDown:W][button1PreviewKeyDown:W]" +
                                                "[button1KeyDown:W][innerPanel2KeyDown:W][rootPanelKeyDown:W]" +
                                                "[rootPanelPreviewKeyUp:W][innerPanel2PreviewKeyUp:W][button1PreviewKeyUp:W]" +
                                                "[button1KeyUp:W][innerPanel2KeyUp:W][rootPanelKeyUp:W]";
                    Verify.AreEqual(expectedEventOrder, eventOrder.ToString());
                });
            }
        }

        [TestMethod]
        [TestProperty("Description", "Verifies the order of keydown events on deleting subtree")]
        [TestProperty("Hosting:Mode", "UAP")]
        public void VerifyKeyDownEventOrderWhenDeletingSubtree()
        {
            const string rootPanelXaml =
                    @"<StackPanel Orientation='Vertical' xmlns='http://schemas.microsoft.com/winfx/2006/xaml/presentation' xmlns:x='http://schemas.microsoft.com/winfx/2006/xaml' x:Name='rootPanel'>
                        <StackPanel x:Name='innerPanel1'>
                            <Button Width='50' x:Name='button1' Content='Button 1'/>
                        </StackPanel>
                    </StackPanel>";

            StackPanel rootPanel = null;
            StackPanel innerPanel1 = null;
            Button button1 = null;
            FrameworkElement[] elementList = null;
            StringBuilder eventOrder = null;


            var rootPanelPreviewKeyDownHandler = new Action<object, KeyRoutedEventArgs>((source, args) =>
            {
                Log.Comment("Removing children of rootPanel.");
                rootPanel.Children.Clear();
            });


            UIExecutor.Execute(() =>
            {
                rootPanel = (StackPanel)XamlReader.Load(rootPanelXaml);
                button1 = (Button)rootPanel.FindName("button1");
                innerPanel1 = (StackPanel)rootPanel.FindName("innerPanel1");

                elementList = new FrameworkElement[3] { button1, innerPanel1, rootPanel };
                eventOrder = new StringBuilder();
                TestServices.WindowHelper.WindowContent = rootPanel;
            });

            TestServices.WindowHelper.WaitForIdle();

            Log.Comment("Focus button 1");
            FocusHelper.EnsureFocus(button1, FocusState.Keyboard);

            using (var KeyEventOrderingTester = new KeyEventOrderingTester(elementList, eventOrder))
            using (var deletingSubtreePreviewEventHandler = new EventTester<StackPanel, KeyRoutedEventArgs>(rootPanel, "PreviewKeyDown", rootPanelPreviewKeyDownHandler))
            using (var stackPanelGotKeyUp = new EventTester<StackPanel, KeyRoutedEventArgs>(rootPanel, "KeyUp"))
            {
                Log.Comment("Press key w");
                TestServices.KeyboardHelper.PressKeySequence("w");

                stackPanelGotKeyUp.Wait();
                UIExecutor.Execute(() =>
                {
                    Verify.IsTrue(deletingSubtreePreviewEventHandler.HasFired);
                    string expectedEventOrder = "[rootPanelPreviewKeyDown:W][rootPanelKeyDown:W][rootPanelPreviewKeyUp:W][rootPanelKeyUp:W]";
                    Verify.AreEqual(expectedEventOrder, eventOrder.ToString());
                });
            }
        }

        [TestMethod]
        [TestProperty("Description", "Verifies the order of keydown events on adding to subtree")]
        [TestProperty("Hosting:Mode", "UAP")] // fails due release queue not empty
        public void VerifyKeyDownEventOrderWhenAddingToSubtree()
        {
            const string rootPanelXaml =
                    @"<StackPanel Orientation='Vertical' xmlns='http://schemas.microsoft.com/winfx/2006/xaml/presentation' xmlns:x='http://schemas.microsoft.com/winfx/2006/xaml' x:Name='rootPanel'>
                        <StackPanel x:Name='innerPanel1'>
                            <Button Width='50' x:Name='button1' Content='Button 1'/>
                        </StackPanel>
                        <Button x:Name='button0' Content= 'Button0' />
                    </StackPanel>";

            StackPanel rootPanel = null;
            StackPanel innerPanel1 = null;
            StackPanel panelToAdd = null;
            Button button0 = null;
            Button button1 = null;
            FrameworkElement[] elementList = null;
            StringBuilder eventOrder = null;


            var rootPanelPreviewKeyDownHandler = new Action<object, KeyRoutedEventArgs>((source, args) =>
            {
                Log.Comment("Inserting panelToAdd.");
                rootPanel.Children.Add(panelToAdd);

                Log.Comment("Removing innerPanel1.");
                rootPanel.Children.Remove(innerPanel1);
                panelToAdd.Children.Add(innerPanel1);

                //Ensure Focus is on button1
                Verify.IsTrue(button1.Focus(FocusState.Keyboard));
            });


            UIExecutor.Execute(() =>
            {
                rootPanel = (StackPanel)XamlReader.Load(rootPanelXaml);
                button0 = (Button)rootPanel.FindName("button0");
                button1 = (Button)rootPanel.FindName("button1");
                innerPanel1 = (StackPanel)rootPanel.FindName("innerPanel1");
                panelToAdd = new StackPanel();
                panelToAdd.Name = "panelToAdd";

                elementList = new FrameworkElement[4] { button1, innerPanel1, rootPanel, panelToAdd };
                eventOrder = new StringBuilder();
                TestServices.WindowHelper.WindowContent = rootPanel;
            });

            TestServices.WindowHelper.WaitForIdle();

            Log.Comment("Focus button 1");
            FocusHelper.EnsureFocus(button1, FocusState.Keyboard);

            using (var KeyEventOrderingTester = new KeyEventOrderingTester(elementList, eventOrder))
            using (var deletingSubtreePreviewEventHandler = new EventTester<StackPanel, KeyRoutedEventArgs>(rootPanel, "PreviewKeyDown", rootPanelPreviewKeyDownHandler))
            using (var stackPanelGotKeyUp = new EventTester<StackPanel, KeyRoutedEventArgs>(rootPanel, "KeyUp"))
            {
                Log.Comment("Press key s");
                TestServices.KeyboardHelper.PressKeySequence("s");

                stackPanelGotKeyUp.Wait();
                UIExecutor.Execute(() =>
                {
                    Verify.IsTrue(deletingSubtreePreviewEventHandler.HasFired);
                    string expectedEventOrder = "[rootPanelPreviewKeyDown:S][innerPanel1PreviewKeyDown:S][button1PreviewKeyDown:S]" +
                                                "[button1KeyDown:S][innerPanel1KeyDown:S][panelToAddKeyDown:S][rootPanelKeyDown:S]" +
                                                "[rootPanelPreviewKeyUp:S][panelToAddPreviewKeyUp:S][innerPanel1PreviewKeyUp:S][button1PreviewKeyUp:S]" +
                                                "[button1KeyUp:S][innerPanel1KeyUp:S][panelToAddKeyUp:S][rootPanelKeyUp:S]";
                    Verify.AreEqual(expectedEventOrder, eventOrder.ToString());
                });
            }
        }

        [TestMethod]
        [TestProperty("Description", "Verifies the order of keydown events when changing focus.")]
        [TestProperty("Hosting:Mode", "UAP")] // fails due release queue not empty
        public void VerifyKeyEventOrderWhenChangingFocus()
        {
            const string rootPanelXaml =
                    @"<StackPanel Orientation='Vertical' xmlns='http://schemas.microsoft.com/winfx/2006/xaml/presentation' xmlns:x='http://schemas.microsoft.com/winfx/2006/xaml' x:Name='rootPanel'>
                        <StackPanel x:Name='innerPanel1'>
                            <Button Width='50' x:Name='button1' Content='Button 1'/>
                            <Button Width='50' x:Name='button2' Content='Button 2'/>
                        </StackPanel>
                    </StackPanel>";

            StackPanel rootPanel = null;
            StackPanel innerPanel1 = null;

            Button button1 = null;
            Button button2 = null;
            FrameworkElement[] elementList = null;
            StringBuilder eventOrder1 = null;
            StringBuilder eventOrder2 = null;

            var innerPanelPreviewKeyDownHandler = new Action<object, KeyRoutedEventArgs>((source, args) =>
            {
                Log.Comment("Focus button 2");
                Verify.IsTrue(button2.Focus(FocusState.Keyboard));
            });

            var innerPanelPreviewKeyUpHandler = new Action<object, KeyRoutedEventArgs>((source, args) =>
            {
                Log.Comment("Focus button 1");
                Verify.IsTrue(button1.Focus(FocusState.Keyboard));
            });


            UIExecutor.Execute(() =>
            {
                rootPanel = (StackPanel)XamlReader.Load(rootPanelXaml);
                button1 = (Button)rootPanel.FindName("button1");
                button2 = (Button)rootPanel.FindName("button2");
                innerPanel1 = (StackPanel)rootPanel.FindName("innerPanel1");

                elementList = new FrameworkElement[4] { button1, button2, innerPanel1, rootPanel };
                eventOrder1 = new StringBuilder();
                eventOrder2 = new StringBuilder();
                TestServices.WindowHelper.WindowContent = rootPanel;
            });

            TestServices.WindowHelper.WaitForIdle();

            Log.Comment("Focus button 1");
            FocusHelper.EnsureFocus(button1, FocusState.Keyboard);

            using (var KeyEventOrderingTester = new KeyEventOrderingTester(elementList, eventOrder1))
            using (var focusChangedKeyDown = new EventTester<StackPanel, KeyRoutedEventArgs>(innerPanel1, "PreviewKeyDown", innerPanelPreviewKeyDownHandler))
            using (var stackPanelGotKeyUp = new EventTester<StackPanel, KeyRoutedEventArgs>(rootPanel, "KeyUp"))
            {
                Log.Comment("Press key a");
                TestServices.KeyboardHelper.PressKeySequence("a");

                stackPanelGotKeyUp.Wait();
                UIExecutor.Execute(() =>
                {
                    Verify.IsTrue(focusChangedKeyDown.HasFired);
                    string expectedEventOrder1 = "[rootPanelPreviewKeyDown:A][innerPanel1PreviewKeyDown:A][button1PreviewKeyDown:A]" +
                                                 "[button2KeyDown:A][innerPanel1KeyDown:A][rootPanelKeyDown:A]" +
                                                 "[rootPanelPreviewKeyUp:A][innerPanel1PreviewKeyUp:A][button2PreviewKeyUp:A]" +
                                                 "[button2KeyUp:A][innerPanel1KeyUp:A][rootPanelKeyUp:A]";
                    Verify.AreEqual(expectedEventOrder1, eventOrder1.ToString());
                });
            }

            using (var KeyEventOrderingTester = new KeyEventOrderingTester(elementList, eventOrder2))
            using (var focusChangedKeyUp = new EventTester<StackPanel, KeyRoutedEventArgs>(innerPanel1, "PreviewKeyUp", innerPanelPreviewKeyUpHandler))
            using (var stackPanelGotKeyUp = new EventTester<StackPanel, KeyRoutedEventArgs>(rootPanel, "KeyUp"))
            {
                Log.Comment("Press key b");
                TestServices.KeyboardHelper.PressKeySequence("b");

                stackPanelGotKeyUp.Wait();
                UIExecutor.Execute(() =>
                {
                    Verify.IsTrue(focusChangedKeyUp.HasFired);
                    string expectedEventOrder2 = "[rootPanelPreviewKeyDown:B][innerPanel1PreviewKeyDown:B][button2PreviewKeyDown:B]" +
                                                 "[button2KeyDown:B][innerPanel1KeyDown:B][rootPanelKeyDown:B]"+
                                                 "[rootPanelPreviewKeyUp:B][innerPanel1PreviewKeyUp:B][button2PreviewKeyUp:B]" +
                                                 "[button1KeyUp:B][innerPanel1KeyUp:B][rootPanelKeyUp:B]";
                    Verify.AreEqual(expectedEventOrder2, eventOrder2.ToString());
                });
            }
        }


        [TestMethod]
        [TestProperty("Description", "Verifies the order of keydown events when changing and redirecting focus.")]
        public void VerifyKeyEventOrderWhenRedirectingFocus()
        {
            const string rootPanelXaml =
                    @"<StackPanel Orientation='Vertical' xmlns='http://schemas.microsoft.com/winfx/2006/xaml/presentation' xmlns:x='http://schemas.microsoft.com/winfx/2006/xaml' x:Name='rootPanel'>
                        <StackPanel x:Name='innerPanel1'>
                            <Button Width='50' x:Name='button1' Content='Button 1'/>
                            <Button Width='50' x:Name='button2' Content='Button 2'/>
                            <Button Width='50' x:Name='button3' Content='Button 3'/>
                        </StackPanel>
                    </StackPanel>";

            StackPanel rootPanel = null;
            StackPanel innerPanel1 = null;

            Button button1 = null;
            Button button2 = null;
            Button button3 = null;
            FrameworkElement[] elementList = null;
            StringBuilder eventOrder = null;

            var innerPanelPreviewKeyDownHandler = new Action<object, KeyRoutedEventArgs>((source, args) =>
            {
                Log.Comment("Focus button 2");
                Verify.IsTrue(button2.Focus(FocusState.Keyboard));
            });

            var button2GettingFocusHandler = new Action<object, GettingFocusEventArgs>((source, args) =>
            {
                Log.Comment("Redirect focus to button 3");
                args.NewFocusedElement = button3;
            });

            UIExecutor.Execute(() =>
            {
                rootPanel = (StackPanel)XamlReader.Load(rootPanelXaml);
                button1 = (Button)rootPanel.FindName("button1");
                button2 = (Button)rootPanel.FindName("button2");
                button3 = (Button)rootPanel.FindName("button3");
                innerPanel1 = (StackPanel)rootPanel.FindName("innerPanel1");

                elementList = new FrameworkElement[5] { button1, button2, button3, innerPanel1, rootPanel };
                eventOrder = new StringBuilder();
                TestServices.WindowHelper.WindowContent = rootPanel;
            });

            TestServices.WindowHelper.WaitForIdle();

            Log.Comment("Focus button 1");
            FocusHelper.EnsureFocus(button1, FocusState.Keyboard);

            using (var KeyEventOrderingTester = new KeyEventOrderingTester(elementList, eventOrder))
            using (var focusChangedKeyDown = new EventTester<StackPanel, KeyRoutedEventArgs>(innerPanel1, "PreviewKeyDown", innerPanelPreviewKeyDownHandler))
            using (var button2GettingFocus = new EventTester<Button, GettingFocusEventArgs>(button2, "GettingFocus", button2GettingFocusHandler))
            using (var stackPanelGotKeyUp = new EventTester<StackPanel, KeyRoutedEventArgs>(rootPanel, "KeyUp"))
            {
                Log.Comment("Press key a");
                TestServices.KeyboardHelper.PressKeySequence("a");

                stackPanelGotKeyUp.Wait();
                UIExecutor.Execute(() =>
                {
                    Verify.IsTrue(focusChangedKeyDown.HasFired);
                    string expectedEventOrder1 = "[rootPanelPreviewKeyDown:A][innerPanel1PreviewKeyDown:A][button1PreviewKeyDown:A]" +
                                                 "[button3KeyDown:A][innerPanel1KeyDown:A][rootPanelKeyDown:A]" +
                                                 "[rootPanelPreviewKeyUp:A][innerPanel1PreviewKeyUp:A][button3PreviewKeyUp:A]" +
                                                 "[button3KeyUp:A][innerPanel1KeyUp:A][rootPanelKeyUp:A]";
                    Verify.AreEqual(expectedEventOrder1, eventOrder.ToString());
                });
            }
        }

        [TestMethod]
        [TestProperty("Description", "Verifies that the handled flag is shared between preview key and key events.")]
        public void VerifyPreviewKeyEventHandledFlag()
        {
            const string rootPanelXaml =
                    @"<StackPanel Orientation='Vertical' xmlns='http://schemas.microsoft.com/winfx/2006/xaml/presentation' xmlns:x='http://schemas.microsoft.com/winfx/2006/xaml' x:Name='rootPanel'>
                        <StackPanel x:Name='innerPanel'>
                            <Button Width='50' x:Name='button1' Content='Button 1'/>
                        </StackPanel>
                    </StackPanel>";

            StackPanel rootPanel = null;
            StackPanel innerPanel = null;
            Button button1 = null;
            FrameworkElement[] elementList = null;
            StringBuilder eventOrder = null;

            var setHandledPreviewKeyHandler = new Action<object, KeyRoutedEventArgs>((source, args) =>
            {
                Verify.IsFalse(args.Handled);
                Log.Comment("Set handled to true");
                args.Handled = true;
            });

            var unsetHandledPreviewKeyHandler = new Action<object, KeyRoutedEventArgs>((source, args) =>
            {
                Verify.IsTrue(args.Handled);
                Log.Comment("Set handled to false");
                args.Handled = false;
            });

            var checkHandledIsTrue = new Action<object, KeyRoutedEventArgs>((source, args) =>
            {
                Verify.IsTrue(args.Handled);
            });

            UIExecutor.Execute(() =>
            {
                rootPanel = (StackPanel)XamlReader.Load(rootPanelXaml);
                button1 = (Button)rootPanel.FindName("button1");
                innerPanel = (StackPanel)rootPanel.FindName("innerPanel");

                elementList = new FrameworkElement[3] { button1, innerPanel, rootPanel };
                eventOrder = new StringBuilder();
                TestServices.WindowHelper.WindowContent = rootPanel;
            });

            TestServices.WindowHelper.WaitForIdle();

            Log.Comment("Focus button 1");
            FocusHelper.EnsureFocus(button1, FocusState.Keyboard);

            using (var KeyEventOrderingTester = new KeyEventOrderingTester(elementList, eventOrder))
            //Register key-down events using addhandler
            using (var rootPanelPreviewKeyDown = EventTester<StackPanel, KeyRoutedEventArgs>.FromRoutedEvent(rootPanel, "PreviewKeyDown", setHandledPreviewKeyHandler))
            //Register key-up events using addhandler
            using (var rootPanelPreviewKeyUp = EventTester<StackPanel, KeyRoutedEventArgs>.FromRoutedEvent(rootPanel, "PreviewKeyUp", setHandledPreviewKeyHandler))
            using (var innerPanelPreviewKeyUp = EventTester<StackPanel, KeyRoutedEventArgs>.FromRoutedEvent(innerPanel, "PreviewKeyUp", unsetHandledPreviewKeyHandler))
            using (var buttonPreviewKeyUp = EventTester<Button, KeyRoutedEventArgs>.FromRoutedEvent(button1, "PreviewKeyUp", setHandledPreviewKeyHandler))
            using (var stackPanelGotKeyUp = EventTester<StackPanel, KeyRoutedEventArgs>.FromRoutedEvent(rootPanel, "KeyUp", checkHandledIsTrue))
            {
                Log.Comment("Press key w");
                TestServices.KeyboardHelper.PressKeySequence("w");

                stackPanelGotKeyUp.Wait();
                UIExecutor.Execute(() =>
                {
                    Verify.IsTrue(rootPanelPreviewKeyDown.HasFired);
                    Verify.IsTrue(rootPanelPreviewKeyUp.HasFired);
                    Verify.IsTrue(innerPanelPreviewKeyUp.HasFired);
                    Verify.IsTrue(buttonPreviewKeyUp.HasFired);

                    string expectedEventOrder = "[rootPanelPreviewKeyDown:W][innerPanelPreviewKeyDown:W:Handled][button1PreviewKeyDown:W:Handled]"+
                                                "[button1KeyDown:W:Handled][innerPanelKeyDown:W:Handled][rootPanelKeyDown:W:Handled]"+
                                                "[rootPanelPreviewKeyUp:W][innerPanelPreviewKeyUp:W:Handled][button1PreviewKeyUp:W]"+
                                                "[button1KeyUp:W:Handled][innerPanelKeyUp:W:Handled][rootPanelKeyUp:W:Handled]";
                    Verify.AreEqual(expectedEventOrder, eventOrder.ToString());
                });
            }
        }

        [TestMethod]
        [TestProperty("Description", "Verifies the order of keydown events")]
        public void VerifyKeyEventOrderForHyperlink()
        {
            const string rootPanelXaml =
                    @"<StackPanel Orientation='Vertical' xmlns='http://schemas.microsoft.com/winfx/2006/xaml/presentation' xmlns:x='http://schemas.microsoft.com/winfx/2006/xaml' x:Name='rootPanel'>
                        <StackPanel x:Name='innerPanel'>
                            <TextBlock x:Name='txb1'>
                                <Hyperlink x:Name='hy1'> Text </Hyperlink>
                            </TextBlock>
                        </StackPanel>
                    </StackPanel>";

            StackPanel rootPanel = null;
            StackPanel innerPanel = null;
            Hyperlink hyperlink1 = null;
            TextBlock txb1 = null;
            FrameworkElement[] elementList = null;
            StringBuilder eventOrder = null;

            UIExecutor.Execute(() =>
            {
                rootPanel = (StackPanel)XamlReader.Load(rootPanelXaml);
                hyperlink1 = (Hyperlink)rootPanel.FindName("hy1");
                innerPanel = (StackPanel)rootPanel.FindName("innerPanel");
                txb1 = (TextBlock)rootPanel.FindName("txb1");
                hyperlink1 = (Hyperlink)rootPanel.FindName("hy1");

                //Hyperlink is not in the elementList since key events are not defined on hyperlink.
                elementList = new FrameworkElement[3] { txb1, innerPanel, rootPanel };
                eventOrder = new StringBuilder();
                TestServices.WindowHelper.WindowContent = rootPanel;
            });

            TestServices.WindowHelper.WaitForIdle();

            Log.Comment("Focus hyperlink1");
            FocusHelper.EnsureFocus(hyperlink1, FocusState.Keyboard);

            using (var KeyEventOrderingTester = new KeyEventOrderingTester(elementList, eventOrder))
            using (var stackPanelGotKeyUp = new EventTester<StackPanel, KeyRoutedEventArgs>(rootPanel, "KeyUp"))
            {
                Log.Comment("Press key w");
                TestServices.KeyboardHelper.PressKeySequence("w");

                stackPanelGotKeyUp.Wait();
                UIExecutor.Execute(() =>
                {
                    string expectedEventOrder = "[rootPanelPreviewKeyDown:W][innerPanelPreviewKeyDown:W][txb1PreviewKeyDown:W]" +
                                                "[txb1KeyDown:W][innerPanelKeyDown:W][rootPanelKeyDown:W]"+
                                                "[rootPanelPreviewKeyUp:W][innerPanelPreviewKeyUp:W][txb1PreviewKeyUp:W]" +
                                                "[txb1KeyUp:W][innerPanelKeyUp:W][rootPanelKeyUp:W]";
                    Verify.AreEqual(expectedEventOrder, eventOrder.ToString());
                });
            }
        }

        [TestMethod]
        [TestProperty("Description", "Verify the orderign of AccessKeys and KeyEvents")]
        [TestProperty("Hosting:Mode", "UAP")]
        public void VerifyAccessKeysPrecedePreviewKeyEvents()
        {
            const string rootPanelXaml =
                @"<StackPanel Orientation='Vertical' xmlns='http://schemas.microsoft.com/winfx/2006/xaml/presentation' xmlns:x='http://schemas.microsoft.com/winfx/2006/xaml' x:Name='rootPanel'>
                    <StackPanel x:Name='innerPanel'>
                        <Button Width='50' x:Name='button1' Content='Button 1' AccessKey='W'/>
                    </StackPanel>
                    <Button x:Name='button2' Content='Button2' />
                </StackPanel>";

            StackPanel rootPanel = null;
            StackPanel innerPanel = null;
            Button button1 = null;
            Button button2 = null;
            FrameworkElement[] elementList = null;
            StringBuilder eventOrder = null;

            UIExecutor.Execute(() =>
            {
                rootPanel = (StackPanel)XamlReader.Load(rootPanelXaml);
                button1 = (Button)rootPanel.FindName("button1");
                button2 = (Button)rootPanel.FindName("button2");
                innerPanel = (StackPanel)rootPanel.FindName("innerPanel");

                elementList = new FrameworkElement[4] { button1, button2, innerPanel, rootPanel };
                eventOrder = new StringBuilder();
                TestServices.WindowHelper.WindowContent = rootPanel;
            });

            TestServices.WindowHelper.WaitForIdle();

            Log.Comment("Focus button 1");
            FocusHelper.EnsureFocus(button1, FocusState.Keyboard);

            using (var KeyEventOrderingTester = new KeyEventOrderingTester(elementList, eventOrder))
            {
                using (var akDisplayRequested = new EventTester<Button, AccessKeyDisplayRequestedEventArgs>(button1, "AccessKeyDisplayRequested"))
                {
                    Log.Comment("Press Alt key");
                    TestServices.KeyboardHelper.Alt();
                    akDisplayRequested.Wait();
                }
                using (var akInvoked = new EventTester<Button, AccessKeyInvokedEventArgs>(button1, "AccessKeyInvoked"))
                {
                    Log.Comment("Press key w");
                    TestServices.KeyboardHelper.PressKeySequence("w");
                    akInvoked.Wait();
                    UIExecutor.Execute(() =>
                    {
                        string expectedEventOrder = "[rootPanelPreviewKeyDown:Menu][innerPanelPreviewKeyDown:Menu][button1PreviewKeyDown:Menu]" +
                                                    "[button1KeyDown:Menu][innerPanelKeyDown:Menu][rootPanelKeyDown:Menu]" +
                                                    //We do not get KeyDown events for W since the AccessKey was invoked here
                                                    "[rootPanelPreviewKeyUp:W][innerPanelPreviewKeyUp:W][button1PreviewKeyUp:W]" +
                                                    "[button1KeyUp:W][innerPanelKeyUp:W][rootPanelKeyUp:W]";
                        Verify.AreEqual(expectedEventOrder, eventOrder.ToString());
                    });

                    TestServices.KeyboardHelper.Escape();
                    TestServices.WindowHelper.WaitForIdle();
                }
            }
        }

        [TestMethod]
        [TestProperty("Description", "Verifies the event order of CharacterReceived and key events.")]
        public void VerifyCharacterReceivedEventOrder()
        {
            const string rootPanelXaml =
                @"<StackPanel Orientation='Vertical' xmlns='http://schemas.microsoft.com/winfx/2006/xaml/presentation' xmlns:x='http://schemas.microsoft.com/winfx/2006/xaml' x:Name='rootPanel'>
                    <StackPanel x:Name='innerPanel'>
                        <Button Width='50' x:Name='button1' Content='Button 1'/>
                    </StackPanel>
                    <Button x:Name='button2' Content='Button2' />
                </StackPanel>";

            StackPanel rootPanel = null;
            StackPanel innerPanel = null;
            Button button1 = null;
            Button button2 = null;
            FrameworkElement[] elementList = null;
            StringBuilder eventOrder = null;

            UIExecutor.Execute(() =>
            {
                rootPanel = (StackPanel)XamlReader.Load(rootPanelXaml);
                button1 = (Button)rootPanel.FindName("button1");
                button2 = (Button)rootPanel.FindName("button2");
                innerPanel = (StackPanel)rootPanel.FindName("innerPanel");

                elementList = new FrameworkElement[4] { button1, button2, innerPanel, rootPanel };
                eventOrder = new StringBuilder();
                TestServices.WindowHelper.WindowContent = rootPanel;
            });

            TestServices.WindowHelper.WaitForIdle();

            Log.Comment("Focus button 1");
            FocusHelper.EnsureFocus(button1, FocusState.Keyboard);

            // Injecting character alpha
            using (var KeyEventOrderingTester = new KeyEventOrderingTester(elementList, eventOrder, true /*characterReceivedEventToo*/))
            using (var characterReceived = new EventTester<StackPanel, CharacterReceivedRoutedEventArgs>(rootPanel, "CharacterReceived"))
            {
                TestServices.KeyboardHelper.PressKeySequence("a");
                characterReceived.Wait();
            }

            string expectedEventOrder =
            "[rootPanelPreviewKeyDown:A][innerPanelPreviewKeyDown:A][button1PreviewKeyDown:A]" +
            "[button1KeyDown:A][innerPanelKeyDown:A][rootPanelKeyDown:A]" +
            "[button1CharacterReceived:a][innerPanelCharacterReceived:a][rootPanelCharacterReceived:a]" +
            "[rootPanelPreviewKeyUp:A][innerPanelPreviewKeyUp:A][button1PreviewKeyUp:A]" +
            "[button1KeyUp:A][innerPanelKeyUp:A][rootPanelKeyUp:A]";
            Verify.AreEqual(expectedEventOrder, eventOrder.ToString());

            TestServices.WindowHelper.WaitForIdle();
        }

        [TestMethod]
        [TestProperty("Description", "Verifies the event order of CharacterReceived and key events when using alt numeric key codes.")]
        [TestProperty("TestPass:IncludeOnlyOn", "Desktop")] // CharacterReceived event is not getting fired on all Onecore SKUs for Alt+Numpad key codes
        [TestProperty("Hosting:Mode", "UAP")]
        public void VerifyCharacterReceivedEventOrderWithAltNumericKeyCodes()
        {
            const string rootPanelXaml =
                @"<StackPanel Orientation='Vertical' xmlns='http://schemas.microsoft.com/winfx/2006/xaml/presentation' xmlns:x='http://schemas.microsoft.com/winfx/2006/xaml' x:Name='rootPanel'>
                    <StackPanel x:Name='innerPanel'>
                        <Button Width='50' x:Name='button1' Content='Button 1'/>
                    </StackPanel>
                    <Button x:Name='button2' Content='Button2' />
                </StackPanel>";

            uint alphaKeyCode = 945;
            var characterReceivedHandler = new Action<object, CharacterReceivedRoutedEventArgs>((source, args) =>
            {
                Verify.AreEqual(alphaKeyCode, args.Character);
            });

            StackPanel rootPanel = null;
            StackPanel innerPanel = null;
            Button button1 = null;
            Button button2 = null;
            FrameworkElement[] elementList = null;
            StringBuilder eventOrder = null;

            UIExecutor.Execute(() =>
            {
                rootPanel = (StackPanel)XamlReader.Load(rootPanelXaml);
                button1 = (Button)rootPanel.FindName("button1");
                button2 = (Button)rootPanel.FindName("button2");
                innerPanel = (StackPanel)rootPanel.FindName("innerPanel");

                elementList = new FrameworkElement[4] { button1, button2, innerPanel, rootPanel };
                eventOrder = new StringBuilder();
                TestServices.WindowHelper.WindowContent = rootPanel;
            });

            TestServices.WindowHelper.WaitForIdle();

            Log.Comment("Focus button 1");
            FocusHelper.EnsureFocus(button1, FocusState.Keyboard);

            using (var numlockKeyUpReceived = new EventTester<Button, KeyRoutedEventArgs>(button1, "KeyUp"))
            {
                TestServices.KeyboardHelper.PressKeySequence("$d$_numlock#$u$_numlock");
                numlockKeyUpReceived.Wait();
            }

            // Injecting character alpha
            using (var KeyEventOrderingTester = new KeyEventOrderingTester(elementList, eventOrder, true /*characterReceivedEventToo*/))
            using (var characterReceived = new EventTester<Button, CharacterReceivedRoutedEventArgs>(button1, "CharacterReceived", characterReceivedHandler))
            {
                TestServices.KeyboardHelper.PressKeySequence("$d$_alt#$d$_n2#$u$_n2#$d$_n2#$u$_n2#$d$_n4#$u$_n4#$u$_alt");
                characterReceived.Wait();
            }

            using (var numlockKeyUpReceived = new EventTester<Button, KeyRoutedEventArgs>(button1, "KeyUp"))
            {
                TestServices.KeyboardHelper.PressKeySequence("$d$_numlock#$u$_numlock");
                numlockKeyUpReceived.Wait();
            }

            string expectedEventOrder =
            "[rootPanelPreviewKeyDown:Menu][innerPanelPreviewKeyDown:Menu][button1PreviewKeyDown:Menu]" +
            "[button1KeyDown:Menu][innerPanelKeyDown:Menu][rootPanelKeyDown:Menu]" +
            "[rootPanelPreviewKeyDown:NumberPad2][innerPanelPreviewKeyDown:NumberPad2][button1PreviewKeyDown:NumberPad2]" +
            "[button1KeyDown:NumberPad2][innerPanelKeyDown:NumberPad2][rootPanelKeyDown:NumberPad2]" +
            "[rootPanelPreviewKeyUp:NumberPad2][innerPanelPreviewKeyUp:NumberPad2][button1PreviewKeyUp:NumberPad2]" +
            "[button1KeyUp:NumberPad2][innerPanelKeyUp:NumberPad2][rootPanelKeyUp:NumberPad2]" +
            "[rootPanelPreviewKeyDown:NumberPad2][innerPanelPreviewKeyDown:NumberPad2][button1PreviewKeyDown:NumberPad2]" +
            "[button1KeyDown:NumberPad2][innerPanelKeyDown:NumberPad2][rootPanelKeyDown:NumberPad2]" +
            "[rootPanelPreviewKeyUp:NumberPad2][innerPanelPreviewKeyUp:NumberPad2][button1PreviewKeyUp:NumberPad2]" +
            "[button1KeyUp:NumberPad2][innerPanelKeyUp:NumberPad2][rootPanelKeyUp:NumberPad2]" +
            "[rootPanelPreviewKeyDown:NumberPad4][innerPanelPreviewKeyDown:NumberPad4][button1PreviewKeyDown:NumberPad4]" +
            "[button1KeyDown:NumberPad4][innerPanelKeyDown:NumberPad4][rootPanelKeyDown:NumberPad4]" +
            "[rootPanelPreviewKeyUp:NumberPad4][innerPanelPreviewKeyUp:NumberPad4][button1PreviewKeyUp:NumberPad4]" +
            "[button1KeyUp:NumberPad4][innerPanelKeyUp:NumberPad4][rootPanelKeyUp:NumberPad4]" +
            "[rootPanelPreviewKeyUp:Menu][innerPanelPreviewKeyUp:Menu][button1PreviewKeyUp:Menu]" +
            "[button1KeyUp:Menu][innerPanelKeyUp:Menu][rootPanelKeyUp:Menu]" +
            "[button1CharacterReceived:\u03B1][innerPanelCharacterReceived:\u03B1][rootPanelCharacterReceived:\u03B1]";
            Verify.AreEqual(expectedEventOrder, eventOrder.ToString());

            TestServices.WindowHelper.WaitForIdle();
        }

        [TestMethod]
        [TestProperty("Description", "Verifies CharacterReceived is not fired if Access Keys are fired in hotkey mode.")]
        [TestProperty("Hosting:Mode", "UAP")]
        public void VerifyCharacterReceivedNotFiredIfAccessKeyFired()
        {
            const string rootPanelXaml =
                @"<StackPanel Orientation='Vertical' xmlns='http://schemas.microsoft.com/winfx/2006/xaml/presentation' xmlns:x='http://schemas.microsoft.com/winfx/2006/xaml' x:Name='rootPanel'>
                    <StackPanel x:Name='innerPanel'>
                        <Button Width='50' x:Name='button1' Content='Button 1' AccessKey='1'/>
                    </StackPanel>
                    <Button x:Name='button2' Content='Button2' />
                </StackPanel>";

            StackPanel rootPanel = null;
            StackPanel innerPanel = null;
            Button button1 = null;
            Button button2 = null;
            FrameworkElement[] elementList = null;
            StringBuilder eventOrder = null;

            UIExecutor.Execute(() =>
            {
                rootPanel = (StackPanel)XamlReader.Load(rootPanelXaml);
                button1 = (Button)rootPanel.FindName("button1");
                button2 = (Button)rootPanel.FindName("button2");
                innerPanel = (StackPanel)rootPanel.FindName("innerPanel");

                elementList = new FrameworkElement[4] { button1, button2, innerPanel, rootPanel };
                eventOrder = new StringBuilder();
                TestServices.WindowHelper.WindowContent = rootPanel;
            });

            TestServices.WindowHelper.WaitForIdle();

            Log.Comment("Focus button 1");
            FocusHelper.EnsureFocus(button1, FocusState.Keyboard);

            using (var numlockKeyUpReceived = new EventTester<Button, KeyRoutedEventArgs>(button1, "KeyUp"))
            {
                TestServices.KeyboardHelper.PressKeySequence("$d$_numlock#$u$_numlock");
                numlockKeyUpReceived.Wait();
            }

            using (var KeyEventOrderingTester = new KeyEventOrderingTester(elementList, eventOrder, true /*characterReceivedEventToo*/))
            {
                using (var buttonClicked = new EventTester<Button, RoutedEventArgs>(button1, "Click"))
                {
                    TestServices.KeyboardHelper.PressKeySequence("$d$_alt#$d$_1");
                    buttonClicked.Wait();
                }
                using (var numpad1KeyUpReceived = new EventTester<Button, KeyRoutedEventArgs>(button1, "KeyUp"))
                {
                    TestServices.KeyboardHelper.PressKeySequence("$u$_1");
                    numpad1KeyUpReceived.Wait();
                }
                using (var altKeyUpReceived = new EventTester<Button, KeyRoutedEventArgs>(button1, "KeyUp"))
                {
                    TestServices.KeyboardHelper.PressKeySequence("$u$_alt");
                    altKeyUpReceived.Wait();
                }
            }

            using (var numlockKeyUpReceived = new EventTester<Button, KeyRoutedEventArgs>(button1, "KeyUp"))
            {
                TestServices.KeyboardHelper.PressKeySequence("$d$_numlock#$u$_numlock");
                numlockKeyUpReceived.Wait();
            }

            string expectedEventOrder = 
            "[rootPanelPreviewKeyDown:Menu][innerPanelPreviewKeyDown:Menu][button1PreviewKeyDown:Menu]" +
            "[button1KeyDown:Menu][innerPanelKeyDown:Menu][rootPanelKeyDown:Menu]" +
            //We do not get key down events for numpad 1 since the Access Key was fired
            "[rootPanelPreviewKeyUp:Number1][innerPanelPreviewKeyUp:Number1][button1PreviewKeyUp:Number1]" +
            "[button1KeyUp:Number1][innerPanelKeyUp:Number1][rootPanelKeyUp:Number1]" +
            "[rootPanelPreviewKeyUp:Menu][innerPanelPreviewKeyUp:Menu][button1PreviewKeyUp:Menu]" +
            "[button1KeyUp:Menu][innerPanelKeyUp:Menu][rootPanelKeyUp:Menu]";
            Verify.AreEqual(expectedEventOrder, eventOrder.ToString());

            TestServices.WindowHelper.WaitForIdle();
        }

        [TestMethod]
        [TestProperty("Description", "Verifies the event order of CharacterReceived and key events when character received is handled.")]
        [TestProperty("TestPass:IncludeOnlyOn", "Desktop")] // CharacterReceived event is not getting fired on all Onecore SKUs for Alt+Numpad key codes
        [TestProperty("Hosting:Mode", "UAP")]
        public void VerifyCharacterReceivedEventCanBeInterceptedWhenHandled()
        {
            const string rootPanelXaml =
                @"<StackPanel Orientation='Vertical' xmlns='http://schemas.microsoft.com/winfx/2006/xaml/presentation' xmlns:x='http://schemas.microsoft.com/winfx/2006/xaml' x:Name='rootPanel'>
                    <StackPanel x:Name='innerPanel'>
                        <Button x:Name='button1' Content='Button1'/>
                    </StackPanel>
                    <Button x:Name='button2' Content='Button2' />
                </StackPanel>";

            var button1CharacterReceivedHandler = new Action<object, CharacterReceivedRoutedEventArgs>((source, args) =>
            {
                Log.Comment("Setting Handled to true");
                args.Handled = true;
            });

            StackPanel rootPanel = null;
            StackPanel innerPanel = null;
            Button button1 = null;
            Button button2 = null;
            FrameworkElement[] elementList = null;
            StringBuilder eventOrder = null;

            UIExecutor.Execute(() =>
            {
                rootPanel = (StackPanel)XamlReader.Load(rootPanelXaml);
                button1 = (Button)rootPanel.FindName("button1");
                button2 = (Button)rootPanel.FindName("button2");
                innerPanel = (StackPanel)rootPanel.FindName("innerPanel");

                elementList = new FrameworkElement[4] { button1, button2, innerPanel, rootPanel };
                eventOrder = new StringBuilder();
                TestServices.WindowHelper.WindowContent = rootPanel;
            });

            TestServices.WindowHelper.WaitForIdle();

            Log.Comment("Focus button1");
            FocusHelper.EnsureFocus(button1, FocusState.Keyboard);

            using (var numlockKeyUpReceived = new EventTester<Button, KeyRoutedEventArgs>(button1, "KeyUp"))
            {
                TestServices.KeyboardHelper.PressKeySequence("$d$_numlock#$u$_numlock");
                numlockKeyUpReceived.Wait();
            }

            // Injecting alpha
            using (var KeyEventOrderingTester = new KeyEventOrderingTester(elementList, eventOrder, true /*characterReceivedEventToo*/))
            using (var characterReceived = EventTester<Button, CharacterReceivedRoutedEventArgs>.FromRoutedEvent(button1, "CharacterReceived", button1CharacterReceivedHandler))
            {
                TestServices.KeyboardHelper.PressKeySequence("$d$_alt#$d$_n2#$u$_n2#$d$_n2#$u$_n2#$d$_n4#$u$_n4#$u$_alt");
                characterReceived.Wait();
                TestServices.WindowHelper.WaitForIdle();
            }

            using (var numlockKeyUpReceived = new EventTester<Button, KeyRoutedEventArgs>(button1, "KeyUp"))
            {
                TestServices.KeyboardHelper.PressKeySequence("$d$_numlock#$u$_numlock");
                numlockKeyUpReceived.Wait();
            }

            string expectedEventOrder =
            "[rootPanelPreviewKeyDown:Menu][innerPanelPreviewKeyDown:Menu][button1PreviewKeyDown:Menu]" +
            "[button1KeyDown:Menu][innerPanelKeyDown:Menu][rootPanelKeyDown:Menu]" +
            "[rootPanelPreviewKeyDown:NumberPad2][innerPanelPreviewKeyDown:NumberPad2][button1PreviewKeyDown:NumberPad2]" +
            "[button1KeyDown:NumberPad2][innerPanelKeyDown:NumberPad2][rootPanelKeyDown:NumberPad2]" +
            "[rootPanelPreviewKeyUp:NumberPad2][innerPanelPreviewKeyUp:NumberPad2][button1PreviewKeyUp:NumberPad2]" +
            "[button1KeyUp:NumberPad2][innerPanelKeyUp:NumberPad2][rootPanelKeyUp:NumberPad2]" +
            "[rootPanelPreviewKeyDown:NumberPad2][innerPanelPreviewKeyDown:NumberPad2][button1PreviewKeyDown:NumberPad2]" +
            "[button1KeyDown:NumberPad2][innerPanelKeyDown:NumberPad2][rootPanelKeyDown:NumberPad2]" +
            "[rootPanelPreviewKeyUp:NumberPad2][innerPanelPreviewKeyUp:NumberPad2][button1PreviewKeyUp:NumberPad2]" +
            "[button1KeyUp:NumberPad2][innerPanelKeyUp:NumberPad2][rootPanelKeyUp:NumberPad2]" +
            "[rootPanelPreviewKeyDown:NumberPad4][innerPanelPreviewKeyDown:NumberPad4][button1PreviewKeyDown:NumberPad4]" +
            "[button1KeyDown:NumberPad4][innerPanelKeyDown:NumberPad4][rootPanelKeyDown:NumberPad4]" +
            "[rootPanelPreviewKeyUp:NumberPad4][innerPanelPreviewKeyUp:NumberPad4][button1PreviewKeyUp:NumberPad4]" +
            "[button1KeyUp:NumberPad4][innerPanelKeyUp:NumberPad4][rootPanelKeyUp:NumberPad4]" +
            "[rootPanelPreviewKeyUp:Menu][innerPanelPreviewKeyUp:Menu][button1PreviewKeyUp:Menu]" +
            "[button1KeyUp:Menu][innerPanelKeyUp:Menu][rootPanelKeyUp:Menu]" +
            "[button1CharacterReceived:\u03B1][innerPanelCharacterReceived:\u03B1:Handled][rootPanelCharacterReceived:\u03B1:Handled]";
            Verify.AreEqual(expectedEventOrder, eventOrder.ToString());

            TestServices.WindowHelper.WaitForIdle();
        }

        #region TextBoxBase derived control tests

        [TestMethod]
        [TestProperty("Description", "Verifies the event order of CharacterReceived and key events for TextBox.")]
        [TestProperty("Hosting:Mode", "UAP")]
        public void VerifyCharacterReceivedEventOrderWithTextBox()
        {
            const string rootPanelXaml =
                @"<StackPanel Orientation='Vertical' xmlns='http://schemas.microsoft.com/winfx/2006/xaml/presentation' xmlns:x='http://schemas.microsoft.com/winfx/2006/xaml' x:Name='rootPanel'>
                    <StackPanel x:Name='innerPanel'>
                        <TextBox Width='50' x:Name='txb1'/>
                    </StackPanel>
                    <Button x:Name='button2' Content='Button2' />
                </StackPanel>";

            StackPanel rootPanel = null;
            StackPanel innerPanel = null;
            TextBox txb1 = null;
            Button button2 = null;
            FrameworkElement[] elementList = null;
            StringBuilder eventOrder = null;

            UIExecutor.Execute(() =>
            {
                rootPanel = (StackPanel)XamlReader.Load(rootPanelXaml);
                txb1 = (TextBox)rootPanel.FindName("txb1");
                button2 = (Button)rootPanel.FindName("button2");
                innerPanel = (StackPanel)rootPanel.FindName("innerPanel");

                elementList = new FrameworkElement[4] { txb1, button2, innerPanel, rootPanel };
                eventOrder = new StringBuilder();
                TestServices.WindowHelper.WindowContent = rootPanel;
            });

            TestServices.WindowHelper.WaitForIdle();

            Log.Comment("Focus txb1");
            FocusHelper.EnsureFocus(txb1, FocusState.Keyboard);

            using (var KeyEventOrderingTester = new KeyEventOrderingTester(elementList, eventOrder, true /*characterReceivedEventToo*/))
            using (var characterReceived = new EventTester<StackPanel, CharacterReceivedRoutedEventArgs>(rootPanel, "CharacterReceived"))
            {
                TestServices.KeyboardHelper.PressKeySequence("$d$_shift#$d$_w#$u$_w");
                characterReceived.Wait();

                //We need to wait for the Shift key up to ensure test reliability.
                using (var rootPanelGotKeyUp = new EventTester<StackPanel, KeyRoutedEventArgs>(rootPanel, "KeyUp"))
                {
                    TestServices.KeyboardHelper.PressKeySequence("$u$_shift");
                    rootPanelGotKeyUp.Wait();
                }

            }

            string expectedEventOrder =
            "[rootPanelPreviewKeyDown:Shift][innerPanelPreviewKeyDown:Shift][txb1PreviewKeyDown:Shift]" +
            "[txb1KeyDown:Shift][innerPanelKeyDown:Shift][rootPanelKeyDown:Shift]" +
            "[rootPanelPreviewKeyDown:W][innerPanelPreviewKeyDown:W][txb1PreviewKeyDown:W]" +
            "[txb1KeyDown:W][innerPanelKeyDown:W][rootPanelKeyDown:W]" +
            //We receive capital W as a character
            "[txb1CharacterReceived:W][innerPanelCharacterReceived:W][rootPanelCharacterReceived:W]" +
            "[rootPanelPreviewKeyUp:W][innerPanelPreviewKeyUp:W][txb1PreviewKeyUp:W]" +
            "[txb1KeyUp:W][innerPanelKeyUp:W][rootPanelKeyUp:W]" +
            "[rootPanelPreviewKeyUp:Shift][innerPanelPreviewKeyUp:Shift][txb1PreviewKeyUp:Shift]" +
            "[txb1KeyUp:Shift][innerPanelKeyUp:Shift][rootPanelKeyUp:Shift]";
            Verify.AreEqual(expectedEventOrder, eventOrder.ToString());
            TestServices.WindowHelper.WaitForIdle();
        }


        [TestMethod]
        [TestProperty("Description", "Verifies the event order of CharacterReceived and key events for PasswordBox.")]
        [TestProperty("Hosting:Mode", "UAP")]
        public void VerifyCharacterReceivedEventOrderWithPasswordBox()
        {
            const string rootPanelXaml =
                @"<StackPanel Orientation='Vertical' xmlns='http://schemas.microsoft.com/winfx/2006/xaml/presentation' xmlns:x='http://schemas.microsoft.com/winfx/2006/xaml' x:Name='rootPanel'>
                    <StackPanel x:Name='innerPanel'>
                        <PasswordBox Width='50' x:Name='passwordBox1'/>
                    </StackPanel>
                    <Button x:Name='button2' Content='Button2' />
                </StackPanel>";

            var rootPanelCharacterReceivedHandler = new Action<object, CharacterReceivedRoutedEventArgs>((source, args) =>
            {
                Verify.IsFalse(args.Handled);
            });

            StackPanel rootPanel = null;
            StackPanel innerPanel = null;
            PasswordBox passwordBox1 = null;
            Button button2 = null;
            FrameworkElement[] elementList = null;
            StringBuilder eventOrder = null;

            UIExecutor.Execute(() =>
            {
                rootPanel = (StackPanel)XamlReader.Load(rootPanelXaml);
                passwordBox1 = (PasswordBox)rootPanel.FindName("passwordBox1");
                button2 = (Button)rootPanel.FindName("button2");
                innerPanel = (StackPanel)rootPanel.FindName("innerPanel");

                elementList = new FrameworkElement[4] { passwordBox1, button2, innerPanel, rootPanel };
                eventOrder = new StringBuilder();
                TestServices.WindowHelper.WindowContent = rootPanel;
            });

            TestServices.WindowHelper.WaitForIdle();

            Log.Comment("Focus passwordBox1");
            FocusHelper.EnsureFocus(passwordBox1, FocusState.Keyboard);

            using (var KeyEventOrderingTester = new KeyEventOrderingTester(elementList, eventOrder, true /*characterReceivedEventToo*/))
            using (var characterReceived = EventTester<StackPanel, CharacterReceivedRoutedEventArgs>.FromRoutedEvent(rootPanel, "CharacterReceived", rootPanelCharacterReceivedHandler))
            {
                TestServices.KeyboardHelper.PressKeySequence("p");
                characterReceived.Wait();
            }

            string expectedEventOrder =
            "[rootPanelPreviewKeyDown:P][innerPanelPreviewKeyDown:P][passwordBox1PreviewKeyDown:P]" +
            "[passwordBox1KeyDown:P][innerPanelKeyDown:P][rootPanelKeyDown:P]" +
            "[passwordBox1CharacterReceived:p][innerPanelCharacterReceived:p][rootPanelCharacterReceived:p]" +
            "[rootPanelPreviewKeyUp:P][innerPanelPreviewKeyUp:P][passwordBox1PreviewKeyUp:P]" + 
            "[passwordBox1KeyUp:P][innerPanelKeyUp:P][rootPanelKeyUp:P]";
            Verify.AreEqual(expectedEventOrder, eventOrder.ToString());

            TestServices.WindowHelper.WaitForIdle();
        }

        [TestMethod]
        [TestProperty("Description", "Verifies the event order of CharacterReceived and key events for RichEditBox.")]
        [TestProperty("Hosting:Mode", "UAP")]
        public void VerifyCharacterReceivedEventOrderWithRichEditBox()
        {
            const string rootPanelXaml =
                @"<StackPanel Orientation='Vertical' xmlns='http://schemas.microsoft.com/winfx/2006/xaml/presentation' xmlns:x='http://schemas.microsoft.com/winfx/2006/xaml' x:Name='rootPanel'>
                    <StackPanel x:Name='innerPanel'>
                        <RichEditBox Width='50' x:Name='richEditBox'/>
                    </StackPanel>
                    <Button x:Name='button2' Content='Button2' />
                </StackPanel>";

            StackPanel rootPanel = null;
            StackPanel innerPanel = null;
            RichEditBox richEditBox = null;
            Button button2 = null;
            FrameworkElement[] elementList = null;
            StringBuilder eventOrder = null;

            UIExecutor.Execute(() =>
            {
                rootPanel = (StackPanel)XamlReader.Load(rootPanelXaml);
                richEditBox = (RichEditBox)rootPanel.FindName("richEditBox");
                button2 = (Button)rootPanel.FindName("button2");
                innerPanel = (StackPanel)rootPanel.FindName("innerPanel");

                elementList = new FrameworkElement[4] { richEditBox, button2, innerPanel, rootPanel };
                eventOrder = new StringBuilder();
                TestServices.WindowHelper.WindowContent = rootPanel;
            });

            TestServices.WindowHelper.WaitForIdle();

            Log.Comment("Focus richEditBox");
            FocusHelper.EnsureFocus(richEditBox, FocusState.Keyboard);

            using (var KeyEventOrderingTester = new KeyEventOrderingTester(elementList, eventOrder, true /*characterReceivedEventToo*/))
            using (var characterReceived = new EventTester<StackPanel, CharacterReceivedRoutedEventArgs>(rootPanel, "CharacterReceived"))
            {
                TestServices.KeyboardHelper.PressKeySequence("r");
                characterReceived.Wait();
            }

            string expectedEventOrder =
            "[rootPanelPreviewKeyDown:R][innerPanelPreviewKeyDown:R][richEditBoxPreviewKeyDown:R]" +
            "[richEditBoxKeyDown:R][innerPanelKeyDown:R][rootPanelKeyDown:R]" +
            "[richEditBoxCharacterReceived:r][innerPanelCharacterReceived:r][rootPanelCharacterReceived:r]" +
            "[rootPanelPreviewKeyUp:R][innerPanelPreviewKeyUp:R][richEditBoxPreviewKeyUp:R]" +
            "[richEditBoxKeyUp:R][innerPanelKeyUp:R][rootPanelKeyUp:R]";
            Verify.AreEqual(expectedEventOrder, eventOrder.ToString());
            TestServices.WindowHelper.WaitForIdle();
        }

        [TestMethod]
        [TestProperty("Description", "Verifies the event order of key events and CharacterReceived when a transient selection flyout is shown.")]
        public void VerifyEventOrderingWithSelectionFlyout()
        {
            const string rootPanelXaml =
                @"<StackPanel Orientation='Vertical' x:Name='rootPanel' xmlns='http://schemas.microsoft.com/winfx/2006/xaml/presentation' xmlns:x='http://schemas.microsoft.com/winfx/2006/xaml'>
                    <StackPanel x:Name='innerPanel'>
                        <TextBox Width='50' x:Name='txb1' Text='lorem ipsum' Margin='100,100,100,100' />
                    </StackPanel>
                    <Button x:Name='button2' Content='Button2' />
                </StackPanel>";

            StackPanel rootPanel = null;
            StackPanel innerPanel = null;
            TextBox txb1 = null;
            Button button2 = null;
            FrameworkElement[] elementList = null;
            StringBuilder eventOrder = null;

            UIExecutor.Execute(() =>
            {
                rootPanel = (StackPanel)XamlReader.Load(rootPanelXaml);
                txb1 = (TextBox)rootPanel.FindName("txb1");
                button2 = (Button)rootPanel.FindName("button2");
                innerPanel = (StackPanel)rootPanel.FindName("innerPanel");
                TestServices.WindowHelper.WindowContent = rootPanel;
            });

            TestServices.WindowHelper.WaitForIdle();

            Log.Comment("Focus the text box.");
            FocusHelper.EnsureFocus(txb1, FocusState.Keyboard);

            FlyoutBase selectionFlyout = null;
            AutoResetEvent selectionOpenedEvent = new AutoResetEvent(false);
            AutoResetEvent selectionClosedEvent = new AutoResetEvent(false);

            UIExecutor.Execute(() =>
            {
                selectionFlyout = txb1.SelectionFlyout;
                Verify.IsNotNull(selectionFlyout);

                selectionFlyout.Opened += (sender, args) =>
                {
                    Log.Comment("Selection flyout opened.");
                    selectionOpenedEvent.Set();
                };

                selectionFlyout.Closed += (sender, args) =>
                {
                    Log.Comment("Selection flyout closed.");
                    selectionClosedEvent.Set();
                };
            });

            TestServices.WindowHelper.WaitForIdle();

            Log.Comment("Tap twice to select text and show the selection flyout.");
            TestServices.InputHelper.Tap(txb1);
            TestServices.WindowHelper.WaitForIdle();
            TestServices.InputHelper.Tap(txb1);

            selectionOpenedEvent.WaitOne();
            TestServices.WindowHelper.WaitForIdle();

            UIExecutor.Execute(() =>
            {
                Log.Comment("Retrieve the flyout presenter.");
                var flyoutPopup = VisualTreeHelper.GetOpenPopupsForXamlRoot(txb1.XamlRoot).Where(popup => popup.Child is FlyoutPresenter).Last();
                FlyoutPresenter flyoutPresenter = (FlyoutPresenter)flyoutPopup.Child;
                flyoutPresenter.Name = "flyoutPresenter";

                CommandBar flyoutCommandBar = (CommandBar)flyoutPresenter.Content;
                AppBarButton firstButton = (AppBarButton)flyoutCommandBar.PrimaryCommands[0];
                firstButton.Name = "firstButton";

                elementList = new FrameworkElement[] { txb1, button2, rootPanel, innerPanel, flyoutPresenter, firstButton };
                eventOrder = new StringBuilder();
            });

            using (var KeyEventOrderingTester = new KeyEventOrderingTester(elementList, eventOrder, true /*characterReceivedEventToo*/))
            using (var rootPanelGotKeyUp = new EventTester<StackPanel, KeyRoutedEventArgs>(rootPanel, "KeyUp"))
            {
                TestServices.KeyboardHelper.PressKeySequence("w");
                rootPanelGotKeyUp.Wait();
            }

            selectionClosedEvent.WaitOne();
            TestServices.WindowHelper.WaitForIdle();

            string expectedEventOrder =
            "[rootPanelPreviewKeyDown:W][innerPanelPreviewKeyDown:W][txb1PreviewKeyDown:W]" +
            "[txb1KeyDown:W][innerPanelKeyDown:W][rootPanelKeyDown:W]" +
            "[txb1CharacterReceived:w][innerPanelCharacterReceived:w][rootPanelCharacterReceived:w]" +
            "[rootPanelPreviewKeyUp:W][innerPanelPreviewKeyUp:W][txb1PreviewKeyUp:W]" +
            "[txb1KeyUp:W][innerPanelKeyUp:W][rootPanelKeyUp:W]";

            Verify.AreEqual(expectedEventOrder, eventOrder.ToString());
            TestServices.WindowHelper.WaitForIdle();
        }

        #endregion

        #region Helpers
        public partial class ButtonWithOnPreviewKeyOverrides : Button
        {
            private readonly AutoResetEvent OnPreviewKeyDownEvent = new AutoResetEvent(false);
            private readonly AutoResetEvent OnPreviewKeyUpEvent = new AutoResetEvent(false);
            public bool HasPreviewKeyDownFired = false;
            public bool HasPreviewKeyUpFired = false;
            public StringBuilder eventOrder = null;

            protected override void OnPreviewKeyDown(KeyRoutedEventArgs args)
            {
                args.Handled = true;
                HasPreviewKeyDownFired = true;
                OnPreviewKeyDownEvent.Set();
                eventOrder?.Append("[" + this.Name + "OnPreviewKeyDown:" + args.Key + "]");
            }

            protected override void OnPreviewKeyUp(KeyRoutedEventArgs args)
            {
                args.Handled = true;
                HasPreviewKeyUpFired = true;
                OnPreviewKeyUpEvent.Set();
                eventOrder?.Append("[" + this.Name + "OnPreviewKeyUp:" + args.Key + "]");
            }

            public void WaitForOnPreviewKeyDown()
            {
                this.OnPreviewKeyDownEvent.WaitOne(TimeSpan.FromSeconds(1));
            }

            public void WaitForOnPreviewKeyUp()
            {
                this.OnPreviewKeyUpEvent.WaitOne(TimeSpan.FromSeconds(1));
            }
        }

        #endregion
    }
}
