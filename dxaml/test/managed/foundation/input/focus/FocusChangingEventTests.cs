// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.
using System;
using System.Text;
using System.Threading.Tasks;
using WEX.Logging.Interop;
using WEX.TestExecution;
using WEX.TestExecution.Markup;

using Private.Infrastructure;
using Microsoft.UI.Xaml.Tests.Common;

using Microsoft.UI.Xaml;
using Microsoft.UI.Xaml.Controls;
using Microsoft.UI.Xaml.Documents;
using Microsoft.UI.Xaml.Input;
using Windows.Foundation;
using Microsoft.UI.Xaml.Markup;
using Windows.UI.ViewManagement;

namespace Microsoft.UI.Xaml.Tests.Focus
{
    [TestClass]
    public class FocusChangingEventTests : XamlTestsBase
    {
        [ClassInitialize]
        [TestProperty("BinaryUnderTest", "Microsoft.UI.Xaml.dll")]
        [TestProperty("RunAs", "UAP")]
        [TestProperty("UAP:Praid", "XamlManagedTAEFTests")]
        [TestProperty("Classification", "Integration")]
        [TestProperty("VelocityTestPass:OneCoreStrict", "Desktop")]
        public static void Setup(TestContext context)
        {
            AssemblySetup.CommonTestClassSetup();
        }

        [ClassCleanup]
        public void ClassCleanup()
        {
            base.CommonClassCleanup();
        }

        [TestMethod]
        [TestProperty("Description", "Verifies that focus can be redirected using the GettingFocus event")]
        public async Task GettingFocusCanRedirectFocus()
        {
            const string rootPanelXaml =
                    @"<StackPanel Orientation='Vertical' xmlns='http://schemas.microsoft.com/winfx/2006/xaml/presentation' xmlns:x='http://schemas.microsoft.com/winfx/2006/xaml'>
                            <Button Width='50' x:Name='button1' Content='Button 1'/>
                            <Button Width='50' x:Name='button2' Content='Button 2'/>
                            <Button Width='50' x:Name='button3' Content='Button 3'/>
                    </StackPanel>";

            StackPanel rootPanel = null;
            Button button1 = null;
            Button button2 = null;
            Button button3 = null;
            FrameworkElement[] elementList = null;
            StringBuilder eventOrder = null;

            var button2GettingFocusHandler = new Action<object, GettingFocusEventArgs>((source, args) =>
            {
                VerifyGettingFocusEventArgParameters(args, button1, button2, FocusState.Keyboard, FocusNavigationDirection.Next, false /*Handled*/, FocusInputDeviceKind.Keyboard, false /*Cancel*/);
                Log.Comment("Redirecting Focus to Button 3");
                args.NewFocusedElement = button3;
            });

            UIExecutor.Execute(() =>
            {
                rootPanel = (StackPanel)XamlReader.Load(rootPanelXaml);
                button1 = (Button)rootPanel.FindName("button1");
                button2 = (Button)rootPanel.FindName("button2");
                button3 = (Button)rootPanel.FindName("button3");
                elementList = new FrameworkElement[3] { button1, button2, button3 };
                eventOrder = new StringBuilder();
                TestServices.WindowHelper.WindowContent = rootPanel;
            });
            TestServices.WindowHelper.WaitForIdle();

            Log.Comment("Focus button 1");
            FocusHelper.EnsureFocus(button1, FocusState.Keyboard);

            using (var focusEventOrderTester = new FocusEventOrderingTester(elementList, eventOrder))
            using (var gettingFocusEventTester = new EventTester<UIElement, GettingFocusEventArgs>(button2, "GettingFocus", button2GettingFocusHandler))
            using (var button3GotFocus = new EventTester<Button, RoutedEventArgs>(button3, "GotFocus"))
            {
                Log.Comment("Injecting tab");
                TestServices.KeyboardHelper.Tab();

                await button3GotFocus.VerifyEventRaised();
                UIExecutor.Execute(() =>
                {
                    Verify.AreEqual(button3.FocusState, FocusState.Keyboard);
                    Verify.AreEqual(FocusManager.GetFocusedElement(TestServices.WindowHelper.WindowContent.XamlRoot), button3);
                });

                TestServices.WindowHelper.WaitForIdle();
                Verify.AreEqual("[button1LosingFocus:1][FocusManagerLosingFocus][button2GettingFocus][FocusManagerGettingFocus][button1LosingFocus][FocusManagerLosingFocus][button3GettingFocus][FocusManagerGettingFocus][button1LostFocus][FocusManagerLostFocus][button3GotFocus][FocusManagerGotFocus]", eventOrder.ToString());
            }
        }

        [TestMethod]
        [TestProperty("Description", "Verifies that focus can be redirected multiple times using the GettingFocus event")]
        public void GettingFocusCanRedirectFocusMultipleTimes()
        {
            const string rootPanelXaml =
                    @"<StackPanel Orientation='Vertical' xmlns='http://schemas.microsoft.com/winfx/2006/xaml/presentation' xmlns:x='http://schemas.microsoft.com/winfx/2006/xaml'>
                            <Button Width='50' x:Name='button1' Content='Button 1'/>
                            <Button Width='50' x:Name='button2' Content='Button 2'/>
                            <Button Width='50' x:Name='button3' Content='Button 3'/>
                            <Button Width='50' x:Name='button4' Content='Button 4'/>
                    </StackPanel>";

            StackPanel rootPanel = null;
            Button button1 = null;
            Button button2 = null;
            Button button3 = null;
            Button button4 = null;
            FrameworkElement[] elementList = null;
            StringBuilder eventOrder = null;

            var button2GettingFocusHandler = new Action<object, GettingFocusEventArgs>((source, args) =>
            {
                VerifyGettingFocusEventArgParameters(args, button1, button2, FocusState.Keyboard, FocusNavigationDirection.Next, false /*Handled*/, FocusInputDeviceKind.Keyboard, false /*Cancel*/);
                Log.Comment("Redirecting Focus to Button 3");
                args.NewFocusedElement = button3;
            });

            var button3GettingFocusHandler = new Action<object, GettingFocusEventArgs>((source, args) =>
            {
                VerifyGettingFocusEventArgParameters(args, button1, button3, FocusState.Keyboard, FocusNavigationDirection.Next, false /*Handled*/, FocusInputDeviceKind.Keyboard, false /*Cancel*/);
                Log.Comment("Redirecting Focus to Button 4");
                args.NewFocusedElement = button4;
            });

            UIExecutor.Execute(() =>
            {
                rootPanel = (StackPanel)XamlReader.Load(rootPanelXaml);
                button1 = (Button)rootPanel.FindName("button1");
                button2 = (Button)rootPanel.FindName("button2");
                button3 = (Button)rootPanel.FindName("button3");
                button4 = (Button)rootPanel.FindName("button4");
                elementList = new FrameworkElement[4] { button1, button2, button3, button4 };
                eventOrder = new StringBuilder();
                TestServices.WindowHelper.WindowContent = rootPanel;
            });

            TestServices.WindowHelper.WaitForIdle();

            Log.Comment("Focus button 1");
            FocusHelper.EnsureFocus(button1, FocusState.Keyboard);

            using (var focusEventOrderTester = new FocusEventOrderingTester(elementList, eventOrder))
            using (var button2GettingFocus = new EventTester<UIElement, GettingFocusEventArgs>(button2, "GettingFocus", button2GettingFocusHandler))
            using (var button3GettingFocus = new EventTester<UIElement, GettingFocusEventArgs>(button3, "GettingFocus", button3GettingFocusHandler))
            using (var button4GotFocus = new EventTester<Button, RoutedEventArgs>(button4, "GotFocus"))
            {
                Log.Comment("Injecting tab");
                TestServices.KeyboardHelper.Tab();

                button4GotFocus.Wait();
                UIExecutor.Execute(() =>
                {
                    Verify.IsTrue(button4.FocusState == FocusState.Keyboard);
                    Verify.AreEqual(FocusManager.GetFocusedElement(TestServices.WindowHelper.WindowContent.XamlRoot), button4);
                });

                TestServices.WindowHelper.WaitForIdle();
                Verify.AreEqual("[button1LosingFocus:1][FocusManagerLosingFocus][button2GettingFocus][FocusManagerGettingFocus][button1LosingFocus][FocusManagerLosingFocus][button3GettingFocus][FocusManagerGettingFocus][button1LosingFocus][FocusManagerLosingFocus][button4GettingFocus][FocusManagerGettingFocus][button1LostFocus][FocusManagerLostFocus][button4GotFocus][FocusManagerGotFocus]", eventOrder.ToString());
            }

            TestServices.WindowHelper.WaitForIdle();
        }

        [TestMethod]
        [TestProperty("Description", "Verifies that focus changes can be cancelled using the LosingFocus event")]
        public void LosingFocusCanCancelFocusChange()
        {
            const string rootPanelXaml =
                    @"<StackPanel Orientation='Vertical' xmlns='http://schemas.microsoft.com/winfx/2006/xaml/presentation' xmlns:x='http://schemas.microsoft.com/winfx/2006/xaml'>
                                <Button Width='50' x:Name='button1' Content='Button 1'/>
                                <Button Width='50' x:Name='button2' Content='Button 2'/>
                        </StackPanel>";

            StackPanel rootPanel = null;
            Button button1 = null;
            Button button2 = null;
            FrameworkElement[] elementList = null;
            StringBuilder eventOrder = null;

            var button1LosingFocusHandler = new Action<object, LosingFocusEventArgs>((source, args) =>
            {
                VerifyLosingFocusEventArgParameters(args, button1, button2, FocusState.Keyboard, FocusNavigationDirection.Next, false /*Handled*/, FocusInputDeviceKind.Keyboard, false /*Cancel*/);
                Log.Comment("Canceling focus change");
                args.Cancel = true;
            });

            UIExecutor.Execute(() =>
            {
                rootPanel = (StackPanel)XamlReader.Load(rootPanelXaml);
                button1 = (Button)rootPanel.FindName("button1");
                button2 = (Button)rootPanel.FindName("button2");
                elementList = new FrameworkElement[2] { button1, button2 };
                eventOrder = new StringBuilder();
                TestServices.WindowHelper.WindowContent = rootPanel;
            });

            TestServices.WindowHelper.WaitForIdle();

            Log.Comment("Focus button 1");
            FocusHelper.EnsureFocus(button1, FocusState.Keyboard);

            using (var focusEventOrderTester = new FocusEventOrderingTester(elementList, eventOrder))
            using (var button1LosingFocus = new EventTester<Button, LosingFocusEventArgs>(button1, "LosingFocus", button1LosingFocusHandler))
            using (var button1LostFocus = new EventTester<Button, RoutedEventArgs>(button1, "LostFocus"))
            {
                Log.Comment("Injecting tab");
                TestServices.KeyboardHelper.Tab();

                button1LostFocus.WaitForNoThrow(TimeSpan.FromMilliseconds(100));
                TestServices.WindowHelper.WaitForIdle();
                UIExecutor.Execute(() =>
                {
                    Verify.IsFalse(button1LostFocus.HasFired);
                    Verify.AreEqual(button1.FocusState, FocusState.Keyboard);
                    Verify.AreEqual(button1, FocusManager.GetFocusedElement(TestServices.WindowHelper.WindowContent.XamlRoot));
                    Log.Comment("Checking focus order");
                    Verify.AreEqual("[button1LosingFocus:1][FocusManagerLosingFocus:Canceled]", eventOrder.ToString());
                });
            }
            TestServices.WindowHelper.WaitForIdle();
        }

        [TestMethod]
        [TestProperty("WorkItem", "10112476")]
        [TestProperty("Description", "Verifies that focus changes are cancelled when redirecting to the old focused element")]
        [TestProperty("TestPass:IncludeOnlyOn", "Desktop")]
        public void LosingFocusCanBeCancelledWhenRedirectingToOldFocusedElement()
        {
            const string rootPanelXaml =
                    @"<StackPanel Orientation='Horizontal' xmlns='http://schemas.microsoft.com/winfx/2006/xaml/presentation' xmlns:x='http://schemas.microsoft.com/winfx/2006/xaml'>
                                <Button Width='50' Height='50' x:Name='button1' Content='Button 1'/>
                                <Button Width='50' Height='50' x:Name='button2' Content='Button 2'/>
                        </StackPanel>";

            StackPanel rootPanel = null;
            Button button1 = null;
            Button button2 = null;

            FrameworkElement[] elementList = null;
            StringBuilder eventOrder = null;

            var button1LosingFocusHandler = new Action<object, LosingFocusEventArgs>((source, args) =>
            {
                VerifyLosingFocusEventArgParameters(args, button1, button2, FocusState.Keyboard, FocusNavigationDirection.Right, false /*Handled*/, FocusInputDeviceKind.Mouse, false /*Cancel*/);
                Log.Comment("Canceling focus change by redirecting to old element");
                args.NewFocusedElement = args.OldFocusedElement;
            });

            UIExecutor.Execute(() =>
            {
                rootPanel = (StackPanel)XamlReader.Load(rootPanelXaml);
                button1 = (Button)rootPanel.FindName("button1");
                button2 = (Button)rootPanel.FindName("button2");
                elementList = new FrameworkElement[2] { button1, button2 };
                eventOrder = new StringBuilder();
                TestServices.WindowHelper.WindowContent = rootPanel;
            });

            TestServices.WindowHelper.WaitForIdle();

            // clicking explicitly so that the input device changes to mouse as well as the focus goes to button 1.
            // this prevents subtle differences in initial focus in XAML Island vs UWP
            using (var button1Click = new EventTester<Button, RoutedEventArgs>(button1, "Click"))
            {
                Log.Comment("Focus button 1 using a pointer");
                TestServices.InputHelper.LeftMouseClick(button1);
                button1Click.Wait();
            }
            // it is ensured that all pending FocusManager events are caught so that they don't mess up the ordering of the following events
            FocusHelper.EnsureFocus(button1, FocusState.Pointer);

            using (var focusEventOrderTester = new FocusEventOrderingTester(elementList, eventOrder))
            using (var button1LosingFocus = new EventTester<Button, LosingFocusEventArgs>(button1, "LosingFocus", button1LosingFocusHandler))
            using (var button1LostFocus = new EventTester<Button, RoutedEventArgs>(button1, "LostFocus"))
            {
                UIExecutor.Execute(() =>
                {
                    Log.Comment("TryMoveFocus to the right");
                    FindNextElementOptions options = new FindNextElementOptions();
                    options.SearchRoot = TestServices.WindowHelper.WindowContent;
                    Verify.IsTrue(FocusManager.TryMoveFocus(FocusNavigationDirection.Right, options));
                });

                button1LostFocus.WaitForNoThrow(TimeSpan.FromMilliseconds(100));
                TestServices.WindowHelper.WaitForIdle();

                UIExecutor.Execute(() =>
                {
                    Verify.IsFalse(button1LostFocus.HasFired);
                    Verify.AreEqual(FocusManager.GetFocusedElement(TestServices.WindowHelper.WindowContent.XamlRoot), button1);
                    Verify.AreEqual("[button1LosingFocus:1][FocusManagerLosingFocus]", eventOrder.ToString());
                });
            }
        }

        [TestMethod]
        [TestProperty("Description", "Verifies that focus changes can be cancelled after redirection using the GettingFocus handler")]
        public void GettingFocusCanCancelFocusRedirect()
        {
            const string rootPanelXaml =
                    @"<StackPanel Orientation='Vertical' xmlns='http://schemas.microsoft.com/winfx/2006/xaml/presentation' xmlns:x='http://schemas.microsoft.com/winfx/2006/xaml'>
                            <Button Width='50' x:Name='button1' Content='Button 1'/>
                            <Button Width='50' x:Name='button2' Content='Button 2'/>
                            <Button Width='50' x:Name='button3' Content='Button 3'/>
                            <Button Width='50' x:Name='button4' Content='Button 4'/>
                    </StackPanel>";

            StackPanel rootPanel = null;
            Button button1 = null;
            Button button2 = null;
            Button button3 = null;
            FrameworkElement[] elementList = null;
            StringBuilder eventOrder = null;

            var button2GettingFocusHandler = new Action<object, GettingFocusEventArgs>((source, args) =>
            {
                VerifyGettingFocusEventArgParameters(args, button1, button2, FocusState.Keyboard, FocusNavigationDirection.Down, false /*Handled*/, FocusInputDeviceKind.GameController, false /*Cancel*/);
                Log.Comment("Redirecting Focus to Button 3");
                args.NewFocusedElement = button3;
            });

            var button3GettingFocusHandler = new Action<object, GettingFocusEventArgs>((source, args) =>
            {
                VerifyGettingFocusEventArgParameters(args, button1, button3, FocusState.Keyboard, FocusNavigationDirection.Down, false /*Handled*/, FocusInputDeviceKind.GameController, false /*Cancel*/);
                Log.Comment("Cancelling focus change");
                args.Cancel = true;
            });


            UIExecutor.Execute(() =>
            {
                rootPanel = (StackPanel)XamlReader.Load(rootPanelXaml);
                button1 = (Button)rootPanel.FindName("button1");
                button2 = (Button)rootPanel.FindName("button2");
                button3 = (Button)rootPanel.FindName("button3");
                elementList = new FrameworkElement[3] { button1, button2, button3 };
                eventOrder = new StringBuilder();
                TestServices.WindowHelper.WindowContent = rootPanel;
            });

            TestServices.WindowHelper.WaitForIdle();

            Log.Comment("Focus button 1");
            FocusHelper.EnsureFocus(button1, FocusState.Keyboard);

            using (var focusEventOrderTester = new FocusEventOrderingTester(elementList, eventOrder))
            using (var button2GettingFocus = new EventTester<Button, GettingFocusEventArgs>(button2, "GettingFocus", button2GettingFocusHandler))
            using (var button3GettingFocus = new EventTester<Button, GettingFocusEventArgs>(button3, "GettingFocus", button3GettingFocusHandler))
            using (var button3GotFocus = new EventTester<Button, RoutedEventArgs>(button3, "GotFocus"))
            {
                Log.Comment("Injecting Gamepad Down");
                TestServices.KeyboardHelper.GamepadDpadDown();
                button3GotFocus.WaitForNoThrow(TimeSpan.FromMilliseconds(100));

                TestServices.WindowHelper.WaitForIdle();

                UIExecutor.Execute(() =>
                {
                    Verify.IsFalse(button3GotFocus.HasFired);
                    Verify.AreEqual(button1.FocusState, FocusState.Keyboard);
                    Verify.AreEqual(FocusManager.GetFocusedElement(TestServices.WindowHelper.WindowContent.XamlRoot), button1);
                });

                TestServices.WindowHelper.WaitForIdle();
                Verify.AreEqual("[button1LosingFocus:1][FocusManagerLosingFocus][button2GettingFocus][FocusManagerGettingFocus][button1LosingFocus][FocusManagerLosingFocus][button3GettingFocus][FocusManagerGettingFocus:Canceled]", eventOrder.ToString());
            }
        }

        [TestMethod]
        [TestProperty("Description", "Verifies that the Focus method cannot be called in Changing Focus handlers")]
        public void ExceptionThrownOnFocusCallInChangingFocus()
        {
            const string rootPanelXaml =
                    @"<StackPanel Orientation='Vertical' xmlns='http://schemas.microsoft.com/winfx/2006/xaml/presentation' xmlns:x='http://schemas.microsoft.com/winfx/2006/xaml'>
                                <Button Width='50' x:Name='button1' Content='Button 1'/>
                                <Button Width='50' x:Name='button2' Content='Button 2'/>
                                <Button Width='50' x:Name='button3' Content='Button 3'/>
                        </StackPanel>";

            StackPanel rootPanel = null;
            Button button1 = null;
            Button button2 = null;
            Button button3 = null;
            FrameworkElement[] elementList = null;
            StringBuilder eventOrder = null;

            var button1LosingFocusHandler = new Action<object, LosingFocusEventArgs>((source, args) =>
            {
                try
                {
                    VerifyLosingFocusEventArgParameters(args, button1, button2, FocusState.Keyboard, FocusNavigationDirection.Next, false /*Handled*/, FocusInputDeviceKind.Keyboard, false /*Cancel*/);
                    Log.Comment("Trying to change Focus");
                    button3.Focus(FocusState.Pointer);
                    Verify.Fail("Exception was not thrown on attempted Focus change");
                }
                catch (Exception ex)
                {
                    Verify.AreEqual(ex.HResult, E_FOCUS_CHANGE_NOT_ALLOWED_DURING_FOCUS_CHANGING);
                    Log.Comment("Exception thrown");
                }
            });

            var button2GettingFocusHandler = new Action<object, GettingFocusEventArgs>((source, args) =>
            {
                try
                {
                    VerifyGettingFocusEventArgParameters(args, button1, button2, FocusState.Keyboard, FocusNavigationDirection.Next, false /*Handled*/, FocusInputDeviceKind.Keyboard, false /*Cancel*/);
                    Log.Comment("Trying to change Focus");
                    button3.Focus(FocusState.Pointer);
                    Verify.Fail("Exception was not thrown on attempted Focus change");
                }
                catch (Exception ex)
                {
                    Verify.AreEqual(ex.HResult, E_FOCUS_CHANGE_NOT_ALLOWED_DURING_FOCUS_CHANGING);
                    Log.Comment("Exception thrown");
                }
            });


            UIExecutor.Execute(() =>
            {
                rootPanel = (StackPanel)XamlReader.Load(rootPanelXaml);
                button1 = (Button)rootPanel.FindName("button1");
                button2 = (Button)rootPanel.FindName("button2");
                button3 = (Button)rootPanel.FindName("button3");
                elementList = new FrameworkElement[3] { button1, button2, button3 };
                eventOrder = new StringBuilder();
                TestServices.WindowHelper.WindowContent = rootPanel;
            });

            TestServices.WindowHelper.WaitForIdle();

            Log.Comment("Focus button 1");
            FocusHelper.EnsureFocus(button1, FocusState.Keyboard);

            using (var focusEventOrderTester = new FocusEventOrderingTester(elementList, eventOrder))
            using (var button1LosingFocus = new EventTester<Button, LosingFocusEventArgs>(button1, "LosingFocus", button1LosingFocusHandler))
            using (var button2GettingFocus = new EventTester<Button, GettingFocusEventArgs>(button2, "GettingFocus", button2GettingFocusHandler))
            using (var button1LostFocus = new EventTester<Button, RoutedEventArgs>(button1, "LostFocus"))
            {
                Log.Comment("Injecting tab");
                TestServices.KeyboardHelper.Tab();
                button1LostFocus.Wait();

                UIExecutor.Execute(() =>
                {
                    Verify.AreEqual(FocusState.Keyboard, button2.FocusState);
                    Verify.AreEqual(FocusManager.GetFocusedElement(TestServices.WindowHelper.WindowContent.XamlRoot), button2);
                });

                TestServices.WindowHelper.WaitForIdle();
                Verify.AreEqual("[button1LosingFocus:1][FocusManagerLosingFocus][button2GettingFocus][FocusManagerGettingFocus][button1LostFocus][FocusManagerLostFocus][button2GotFocus][FocusManagerGotFocus]", eventOrder.ToString());
            }
        }

        [TestMethod]
        [TestProperty("Description", "Verifies that the AutomationPeer.SetFocus method cannot be called in Changing Focus handlers")]
        public void ExceptionThrownOnAutomationPeerSetFocusCallInChangingFocus()
        {
            const string rootPanelXaml =
                    @"<StackPanel Orientation='Vertical' xmlns='http://schemas.microsoft.com/winfx/2006/xaml/presentation' xmlns:x='http://schemas.microsoft.com/winfx/2006/xaml'>
                                <Button Width='50' x:Name='button1' Content='Button 1'/>
                                <Button Width='50' x:Name='button2' Content='Button 2'/>
                                <Button Width='50' x:Name='button3' Content='Button 3'/>
                        </StackPanel>";

            StackPanel rootPanel = null;
            Button button1 = null;
            Button button2 = null;
            Button button3 = null;
            Automation.Peers.AutomationPeer button3AP = null;
            FrameworkElement[] elementList = null;
            StringBuilder eventOrder = null;

            var button1LosingFocusHandler = new Action<object, LosingFocusEventArgs>((source, args) =>
            {
                try
                {
                    VerifyLosingFocusEventArgParameters(args, button1, button2, FocusState.Keyboard, FocusNavigationDirection.Next, false /*Handled*/, FocusInputDeviceKind.Keyboard, false /*Cancel*/);
                    Log.Comment("Trying to change Focus");
                    button3AP.SetFocus();
                    Verify.Fail("Exception was not thrown on attempted Focus change");
                }
                catch (Exception ex)
                {
                    Verify.AreEqual(ex.HResult, E_FOCUS_CHANGE_NOT_ALLOWED_DURING_FOCUS_CHANGING);
                    Log.Comment("Exception thrown");
                }
            });

            var button2GettingFocusHandler = new Action<object, GettingFocusEventArgs>((source, args) =>
            {
                try
                {
                    VerifyGettingFocusEventArgParameters(args, button1, button2, FocusState.Keyboard, FocusNavigationDirection.Next, false /*Handled*/, FocusInputDeviceKind.Keyboard, false /*Cancel*/);
                    Log.Comment("Trying to change Focus");
                    button3AP.SetFocus();
                    Verify.Fail("Exception was not thrown on attempted Focus change");
                }
                catch (Exception ex)
                {
                    Verify.AreEqual(ex.HResult, E_FOCUS_CHANGE_NOT_ALLOWED_DURING_FOCUS_CHANGING);
                    Log.Comment("Exception thrown");
                }
            });


            UIExecutor.Execute(() =>
            {
                rootPanel = (StackPanel)XamlReader.Load(rootPanelXaml);
                button1 = (Button)rootPanel.FindName("button1");
                button2 = (Button)rootPanel.FindName("button2");
                button3 = (Button)rootPanel.FindName("button3");
                button3AP = Automation.Peers.FrameworkElementAutomationPeer.CreatePeerForElement(button3);
                elementList = new FrameworkElement[3] { button1, button2, button3 };
                eventOrder = new StringBuilder();
                TestServices.WindowHelper.WindowContent = rootPanel;
            });

            TestServices.WindowHelper.WaitForIdle();

            Log.Comment("Focus button 1");
            FocusHelper.EnsureFocus(button1, FocusState.Keyboard);

            using (var focusEventOrderTester = new FocusEventOrderingTester(elementList, eventOrder))
            using (var button1LosingFocus = new EventTester<Button, LosingFocusEventArgs>(button1, "LosingFocus", button1LosingFocusHandler))
            using (var button2GettingFocus = new EventTester<Button, GettingFocusEventArgs>(button2, "GettingFocus", button2GettingFocusHandler))
            using (var button2GotFocus = new EventTester<Button, RoutedEventArgs>(button2, "GotFocus"))
            {
                Log.Comment("Injecting tab");
                TestServices.KeyboardHelper.Tab();
                button2GotFocus.Wait();

                UIExecutor.Execute(() =>
                {
                    Verify.AreEqual(FocusState.Keyboard, button2.FocusState);
                    Verify.AreEqual(FocusManager.GetFocusedElement(TestServices.WindowHelper.WindowContent.XamlRoot), button2);
                });

                TestServices.WindowHelper.WaitForIdle();
                Verify.AreEqual("[button1LosingFocus:1][FocusManagerLosingFocus][button2GettingFocus][FocusManagerGettingFocus][button1LostFocus][FocusManagerLostFocus][button2GotFocus][FocusManagerGotFocus]", eventOrder.ToString());
            }
        }

        [TestMethod]
        [TestProperty("Description", "Verifies that the TryMoveFocus method returns false but does not throw an exception in Changing Focus handlers")]
        public void ExceptionNotThrownOnTryMoveFocusCallWhenChangingFocus()
        {
            const string rootPanelXaml =
                    @"<StackPanel Orientation='Vertical' xmlns='http://schemas.microsoft.com/winfx/2006/xaml/presentation' xmlns:x='http://schemas.microsoft.com/winfx/2006/xaml'>
                                <Button Width='50' x:Name='button0' Content='Button 0'/>
                                <Button Width='50' x:Name='button1' Content='Button 1'/>
                                <Button Width='50' x:Name='button2' Content='Button 2'/>
                        </StackPanel>";

            StackPanel rootPanel = null;
            Button button0 = null;
            Button button1 = null;
            Button button2 = null;
            FrameworkElement[] elementList = null;
            StringBuilder eventOrder = null;

            var button1LosingFocusHandler = new Action<object, LosingFocusEventArgs>((source, args) =>
            {
                try
                {
                    Log.Comment("In Button1LosingFocus");
                    VerifyLosingFocusEventArgParameters(args, button1, button2, FocusState.Programmatic, FocusNavigationDirection.None, false /*Handled*/, FocusInputDeviceKind.Keyboard, false /*Cancel*/);
                    Log.Comment("Trying to change Focus");
                    FindNextElementOptions options = new FindNextElementOptions();
                    options.SearchRoot = TestServices.WindowHelper.WindowContent;
                    Verify.IsFalse(FocusManager.TryMoveFocus(FocusNavigationDirection.Next, options));
                }
                catch
                {
                    Verify.Fail("Exception thrown in losing focus handler");
                }
            });

            var button2GettingFocusHandler = new Action<object, GettingFocusEventArgs>((source, args) =>
            {
                try
                {
                    Log.Comment("In Button2GettingFocus");
                    VerifyGettingFocusEventArgParameters(args, button1, button2, FocusState.Programmatic, FocusNavigationDirection.None, false /*Handled*/, FocusInputDeviceKind.Keyboard, false /*Cancel*/);
                    Log.Comment("Trying to change Focus");
                    FindNextElementOptions options = new FindNextElementOptions();
                    options.SearchRoot = TestServices.WindowHelper.WindowContent;
                    Verify.IsFalse(FocusManager.TryMoveFocus(FocusNavigationDirection.Next, options));
                }
                catch
                {
                    Verify.Fail("Exception thrown in getting focus handler");
                }
            });

            UIExecutor.Execute(() =>
            {
                rootPanel = (StackPanel)XamlReader.Load(rootPanelXaml);
                button0 = (Button)rootPanel.FindName("button0");
                button1 = (Button)rootPanel.FindName("button1");
                button2 = (Button)rootPanel.FindName("button2");
                elementList = new FrameworkElement[3] { button0, button1, button2 };
                eventOrder = new StringBuilder();
                TestServices.WindowHelper.WindowContent = rootPanel;
            });

            TestServices.WindowHelper.WaitForIdle();


            UIExecutor.Execute(() =>
            {
                rootPanel = (StackPanel)XamlReader.Load(rootPanelXaml);
                button1 = (Button)rootPanel.FindName("button1");
                button2 = (Button)rootPanel.FindName("button2");
                button0 = (Button)rootPanel.FindName("button0");
                elementList = new FrameworkElement[3] { button0, button1, button2 };
                TestServices.WindowHelper.WindowContent = rootPanel;
            });

            TestServices.WindowHelper.WaitForIdle();

            FocusHelper.EnsureFocus(button0, FocusState.Keyboard);

            using (var button1GotFocus = new EventTester<Button, RoutedEventArgs>(button1, "GotFocus"))
            {
                Log.Comment("Tab to button 1");
                TestServices.KeyboardHelper.Tab();
                button1GotFocus.Wait();
            }

            using (var focusEventOrderTester = new FocusEventOrderingTester(elementList, eventOrder))
            using (var button1LosingFocus = new EventTester<Button, LosingFocusEventArgs>(button1, "LosingFocus", button1LosingFocusHandler))
            using (var button2GettingFocus = new EventTester<Button, GettingFocusEventArgs>(button2, "GettingFocus", button2GettingFocusHandler))
            using (var button2GotFocus = new EventTester<Button, RoutedEventArgs>(button2, "GotFocus"))
            {
                UIExecutor.Execute(() =>
                {
                    Log.Comment("Calling focus on button2");
                    button2.Focus(FocusState.Programmatic);
                });
                button2GotFocus.Wait();

                UIExecutor.Execute(() =>
                {
                    Verify.AreEqual(FocusManager.GetFocusedElement(TestServices.WindowHelper.WindowContent.XamlRoot), button2);
                });

                TestServices.WindowHelper.WaitForIdle();
                Verify.AreEqual("[button1LosingFocus:1][FocusManagerLosingFocus][button2GettingFocus][FocusManagerGettingFocus][button1LostFocus][FocusManagerLostFocus][button2GotFocus][FocusManagerGotFocus]", eventOrder.ToString());
            }
        }

        [TestMethod]
        [TestProperty("Description", "Verifies that a new element can be added to the live tree in a ChangingFocus handler")]
        public void NewElementCanBeAddedToLiveTreeWhenGettingFocus()
        {
            const string rootPanelXaml =
                    @"<StackPanel Orientation='Vertical' xmlns='http://schemas.microsoft.com/winfx/2006/xaml/presentation' xmlns:x='http://schemas.microsoft.com/winfx/2006/xaml'>
                                <Button Width='50' x:Name='button1' Content='Button 1'/>
                                <Button Width='50' x:Name='button2' Content='Button 2'/>
                        </StackPanel>";

            StackPanel rootPanel = null;
            Button button1 = null;
            Button button2 = null;
            Button button3 = null;
            FrameworkElement[] elementList = null;
            StringBuilder eventOrder = null;

            var button2GettingFocusHandler = new Action<object, GettingFocusEventArgs>((source, args) =>
            {
                VerifyGettingFocusEventArgParameters(args, button1, button2, FocusState.Keyboard, FocusNavigationDirection.Next, false /*Handled*/, FocusInputDeviceKind.Keyboard, false /*Cancel*/);
                Log.Comment("Adding button");
                rootPanel.Children.Add(button3);
                args.NewFocusedElement = button3;
            });

            UIExecutor.Execute(() =>
            {
                rootPanel = (StackPanel)XamlReader.Load(rootPanelXaml);
                button1 = (Button)rootPanel.FindName("button1");
                button2 = (Button)rootPanel.FindName("button2");
                button3 = new Button() { Name = "button3", Width = 50, Height = 100, Content = "Added" };
                elementList = new FrameworkElement[3] { button1, button2, button3 };
                eventOrder = new StringBuilder();
                TestServices.WindowHelper.WindowContent = rootPanel;
            });

            TestServices.WindowHelper.WaitForIdle();

            Log.Comment("Focus button 1");
            FocusHelper.EnsureFocus(button1, FocusState.Keyboard);

            using (var focusEventOrderTester = new FocusEventOrderingTester(elementList, eventOrder))
            using (var button2GettingFocus = new EventTester<Button, GettingFocusEventArgs>(button2, "GettingFocus", button2GettingFocusHandler))
            using (var eventTester = new EventTester<Button, RoutedEventArgs>(button3, "GotFocus"))
            {
                Log.Comment("Injecting tab");
                TestServices.KeyboardHelper.Tab();
                eventTester.Wait();

                UIExecutor.Execute(() =>
                {
                    Verify.AreEqual(FocusState.Keyboard, button3.FocusState);
                    Verify.AreEqual(button3, FocusManager.GetFocusedElement(TestServices.WindowHelper.WindowContent.XamlRoot));
                });

                TestServices.WindowHelper.WaitForIdle();
                Verify.AreEqual("[button1LosingFocus:1][FocusManagerLosingFocus][button2GettingFocus][FocusManagerGettingFocus][button1LosingFocus][FocusManagerLosingFocus][button3GettingFocus][FocusManagerGettingFocus][button1LostFocus][FocusManagerLostFocus][button3GotFocus][FocusManagerGotFocus]", eventOrder.ToString());
            }
        }

        [TestMethod]
        [TestProperty("Description", "Verifies that we can use AddHandler to add changing focus event handlers")]
        public void CanAddChangingFocusHandlersWithAddHandler()
        {
            const string rootPanelXaml =
                    @"<StackPanel Orientation='Vertical' xmlns='http://schemas.microsoft.com/winfx/2006/xaml/presentation' xmlns:x='http://schemas.microsoft.com/winfx/2006/xaml'>
                                <Button Width='50' x:Name='button1' Content='Button 1'/>
                                <Button Width='50' x:Name='button2' Content='Button 2'/>
                        </StackPanel>";

            StackPanel rootPanel = null;
            Button button1 = null;
            Button button2 = null;

            FrameworkElement[] elementList = null;
            StringBuilder eventOrder = null;

            var button1LosingFocusHandler = new Action<object, LosingFocusEventArgs>((source, args) =>
            {
                VerifyLosingFocusEventArgParameters(args, button1, button2, FocusState.Keyboard, FocusNavigationDirection.Next, false /*Handled*/, FocusInputDeviceKind.Keyboard, false /*Cancel*/);
                Log.Comment("Button 1 LosingFocus Handler invoked");
            });

            var button2GettingFocusHandler = new Action<object, GettingFocusEventArgs>((source, args) =>
            {
                VerifyGettingFocusEventArgParameters(args, button1, button2, FocusState.Keyboard, FocusNavigationDirection.Next, false /*Handled*/, FocusInputDeviceKind.Keyboard, false /*Cancel*/);
                Log.Comment("Button2 GettingFocus Handler invoked");
            });

            UIExecutor.Execute(() =>
            {
                rootPanel = (StackPanel)XamlReader.Load(rootPanelXaml);
                button1 = (Button)rootPanel.FindName("button1");
                button2 = (Button)rootPanel.FindName("button2");
                elementList = new FrameworkElement[2] { button1, button2 };
                eventOrder = new StringBuilder();
                TestServices.WindowHelper.WindowContent = rootPanel;
            });

            TestServices.WindowHelper.WaitForIdle();

            Log.Comment("Focus button 1");
            FocusHelper.EnsureFocus(button1, FocusState.Keyboard);

            using (var focusEventOrderTester = new FocusEventOrderingTester(elementList, eventOrder))
            using (var button1LosingFocus = EventTester<UIElement, LosingFocusEventArgs>.FromRoutedEvent(button1, "LosingFocus", button1LosingFocusHandler))
            using (var button2GettingFocus = EventTester<UIElement, GettingFocusEventArgs>.FromRoutedEvent(button2, "GettingFocus", button2GettingFocusHandler))
            using (var button2GotFocus = new EventTester<Button, RoutedEventArgs>(button2, "GotFocus"))
            {
                Log.Comment("Injecting tab");
                TestServices.KeyboardHelper.Tab();
                button2GotFocus.Wait();
                TestServices.WindowHelper.WaitForIdle();
                Verify.AreEqual("[button1LosingFocus:1][FocusManagerLosingFocus][button2GettingFocus][FocusManagerGettingFocus][button1LostFocus][FocusManagerLostFocus][button2GotFocus][FocusManagerGotFocus]", eventOrder.ToString());
                Verify.IsTrue(button1LosingFocus.HasFired);
                Verify.IsTrue(button2GettingFocus.HasFired);
            }
        }

        [TestMethod]
        [TestProperty("Description", "Verifies that Getting and LosingFocus events do not bubble when handled")]
        public void ChangingFocusEventsDoNotBubbleWhenHandled()
        {
            const string rootPanelXaml =
                    @"<StackPanel Orientation='Vertical' xmlns='http://schemas.microsoft.com/winfx/2006/xaml/presentation' xmlns:x='http://schemas.microsoft.com/winfx/2006/xaml'>
                                <Button Width='50' x:Name='button1' Content='Button 1'/>
                                <StackPanel x:Name='stackpanel2'>
                                   <Button Width='50' x:Name='button2' Content='Button 2'/>
                                </StackPanel>
                        </StackPanel>";

            StackPanel rootPanel = null;
            StackPanel stackPanel2 = null;
            Button button1 = null;
            Button button2 = null;
            FrameworkElement[] elementList = null;
            StringBuilder eventOrder1 = null;
            StringBuilder eventOrder2 = null;
            var button2LosingFocusHandler = new Action<object, LosingFocusEventArgs>((source, args) =>
            {
                Log.Comment("Handling LosingFocus on button2");
                VerifyLosingFocusEventArgParameters(args, button2, button1, FocusState.Keyboard, FocusNavigationDirection.Previous, false /*Handled*/, FocusInputDeviceKind.Keyboard, false /*Cancel*/);
                args.Handled = true;
            });
            var button2GettingFocusHandler = new Action<object, GettingFocusEventArgs>((source, args) =>
            {
                Log.Comment("Handling GettingFocus on button2");
                VerifyGettingFocusEventArgParameters(args, button1, button2, FocusState.Keyboard, FocusNavigationDirection.Next, false /*Handled*/, FocusInputDeviceKind.Keyboard, false /*Cancel*/);
                args.Handled = true;
            });
            var stackPanel2GettingFocusHandler = new Action<object, GettingFocusEventArgs>(
            (source, args) =>
            {
                Verify.Fail("GettingFocus handler on StackPanel called");
            });
            var stackPanel2LosingFocusHandler = new Action<object, LosingFocusEventArgs>((source, args) =>
            {
                Verify.Fail("LosingFocus handler on StackPanel called");
            });

            UIExecutor.Execute(() =>
            {
                rootPanel = (StackPanel)XamlReader.Load(rootPanelXaml);
                button1 = (Button)rootPanel.FindName("button1");
                stackPanel2 = (StackPanel)rootPanel.FindName("stackpanel2");
                button2 = (Button)stackPanel2.FindName("button2");

                elementList = new FrameworkElement[3] { button1, button2, stackPanel2 };
                eventOrder1 = new StringBuilder();
                eventOrder2 = new StringBuilder();
                TestServices.WindowHelper.WindowContent = rootPanel;
            });

            TestServices.WindowHelper.WaitForIdle();

            FocusHelper.EnsureFocus(button1, FocusState.Keyboard);

            using (var focusEventOrderTester = new FocusEventOrderingTester(elementList, eventOrder1))
            using (var stackPanel2GettingFocus = new EventTester<StackPanel, GettingFocusEventArgs>(stackPanel2, "GettingFocus", stackPanel2GettingFocusHandler))
            using (var button2GettingFocus = new EventTester<Button, GettingFocusEventArgs>(button2, "GettingFocus", button2GettingFocusHandler))
            using (var button2GotFocus = new EventTester<Button, RoutedEventArgs>(button2, "GotFocus"))
            {
                Log.Comment("Focus button 2");
                TestServices.KeyboardHelper.Tab();
                button2GotFocus.Wait();

                TestServices.WindowHelper.WaitForIdle();
                Verify.AreEqual("[button1LosingFocus:1][FocusManagerLosingFocus][button2GettingFocus][stackpanel2GettingFocus:Handled][FocusManagerGettingFocus:Handled][button1LostFocus][FocusManagerLostFocus][button2GotFocus][stackpanel2GotFocus][FocusManagerGotFocus]", eventOrder1.ToString());
            }

            using (var focusEventOrderTester = new FocusEventOrderingTester(elementList, eventOrder2))
            using (var button2LosingFocus = new EventTester<Button, LosingFocusEventArgs>(button2, "LosingFocus", button2LosingFocusHandler))
            using (var StackPanel2LosingFocus = new EventTester<StackPanel, LosingFocusEventArgs>(stackPanel2, "LosingFocus", stackPanel2LosingFocusHandler))
            using (var button1GotFocus = new EventTester<Button, RoutedEventArgs>(button1, "GotFocus"))
            {
                Log.Comment("Injecting shift-tab");
                TestServices.KeyboardHelper.ShiftTab();
                button1GotFocus.Wait();
                TestServices.WindowHelper.WaitForIdle();
                Verify.AreEqual("[button2LosingFocus:1][stackpanel2LosingFocus:Handled][FocusManagerLosingFocus:Handled][button1GettingFocus][FocusManagerGettingFocus][button2LostFocus][stackpanel2LostFocus][FocusManagerLostFocus][button1GotFocus][FocusManagerGotFocus]", eventOrder2.ToString());
            }

        }

        [TestMethod]
        [TestProperty("Description", "Verifies that we can us AddHandler to add changing focus event handlers")]
        [TestProperty("IsolationLevel", "Method")]  //Isolation level set to method here as it is the only way to test FocusInputDeviceKinds.None
        [TestProperty("Hosting:Mode", "UAP")] // Blocking for WPF because default input device type is keyboard by convention there which cannot work with this test
        public void ChangingFocusEventsBubbleWhenHandledWhenUsingAddHandler()
        {
            const string rootPanelXaml =
                    @"<StackPanel Orientation='Vertical' xmlns='http://schemas.microsoft.com/winfx/2006/xaml/presentation' xmlns:x='http://schemas.microsoft.com/winfx/2006/xaml'>
                                <StackPanel x:Name='stackpanel2'>
                                    <Button Width='50' x:Name='button1' Content='Button 1'/>
                                    <Button Width='50' x:Name='button2' Content='Button 2'/>
                                </StackPanel>
                        </StackPanel>";

            StackPanel rootPanel = null;
            Button button1 = null;
            Button button2 = null;
            StackPanel stackpanel2 = null;
            StringBuilder eventOrder = null;
            FrameworkElement[] elementList = null;

            var button1LosingFocusHandler = new Action<object, LosingFocusEventArgs>((source, args) =>
            {
                VerifyLosingFocusEventArgParameters(args, button1, button2, FocusState.Pointer, FocusNavigationDirection.None, false /*Handled*/, FocusInputDeviceKind.None, false /*Cancel*/);
                Log.Comment("Button 1 LosingFocus Handler invoked");
                args.Handled = true;
            });

            var stackpanel2LosingFocusHandler = new Action<object, LosingFocusEventArgs>((source, args) =>
            {
                VerifyLosingFocusEventArgParameters(args, button1, button2, FocusState.Pointer, FocusNavigationDirection.None, true /*Handled*/, FocusInputDeviceKind.None, false /*Cancel*/);
            });

            var button2GettingFocusHandler = new Action<object, GettingFocusEventArgs>((source, args) =>
            {
                VerifyGettingFocusEventArgParameters(args, button1, button2, FocusState.Pointer, FocusNavigationDirection.None, false /*Handled*/, FocusInputDeviceKind.None, false /*Cancel*/);
                Log.Comment("Button2 GettingFocus Handler invoked");
                args.Handled = true;
            });

            var stackpanel2GettingFocusHandler = new Action<object, GettingFocusEventArgs>((source, args) =>
            {
                VerifyGettingFocusEventArgParameters(args, button1, button2, FocusState.Pointer, FocusNavigationDirection.None, true /*Handled*/, FocusInputDeviceKind.None, false /*Cancel*/);
            });

            UIExecutor.Execute(() =>
            {
                rootPanel = (StackPanel)XamlReader.Load(rootPanelXaml);
                button1 = (Button)rootPanel.FindName("button1");
                button2 = (Button)rootPanel.FindName("button2");
                stackpanel2 = (StackPanel)rootPanel.FindName("stackpanel2");

                elementList = new FrameworkElement[3] { button1, button2, stackpanel2 };
                eventOrder = new StringBuilder();

                TestServices.WindowHelper.WindowContent = rootPanel;
            });

            TestServices.WindowHelper.WaitForIdle();

            Log.Comment("Focus button 1");
            FocusHelper.EnsureFocus(button1, FocusState.Keyboard);

            using (var focusEventOrderTester = new FocusEventOrderingTester(elementList, eventOrder))
            using (var button1LosingFocus = EventTester<UIElement, LosingFocusEventArgs>.FromRoutedEvent(button1, "LosingFocus", button1LosingFocusHandler))
            using (var button2GettingFocus = EventTester<UIElement, GettingFocusEventArgs>.FromRoutedEvent(button2, "GettingFocus", button2GettingFocusHandler))
            using (var stackpanel2LosingFocus = EventTester<UIElement, LosingFocusEventArgs>.FromRoutedEvent(stackpanel2, "LosingFocus", stackpanel2LosingFocusHandler))
            using (var stackpanel2GettingFocus = EventTester<UIElement, GettingFocusEventArgs>.FromRoutedEvent(stackpanel2, "GettingFocus", stackpanel2GettingFocusHandler))
            using (var stackpanel2GotFocus = new EventTester<StackPanel, RoutedEventArgs>(stackpanel2, "GotFocus"))
            {
                UIExecutor.Execute(() =>
                {
                    Log.Comment("Changing Focus");
                    button2.Focus(FocusState.Pointer);
                });
                stackpanel2GotFocus.Wait();
                Verify.IsTrue(stackpanel2GettingFocus.HasFired);
                Verify.IsTrue(stackpanel2LosingFocus.HasFired);
                TestServices.WindowHelper.WaitForIdle();
                Verify.AreEqual("[button1LosingFocus:1][stackpanel2LosingFocus:Handled][FocusManagerLosingFocus:Handled][button2GettingFocus][stackpanel2GettingFocus:Handled][FocusManagerGettingFocus:Handled][button1LostFocus][stackpanel2LostFocus][FocusManagerLostFocus][button2GotFocus][stackpanel2GotFocus][FocusManagerGotFocus]", eventOrder.ToString());
            }
        }

        [TestMethod]
        [TestProperty("Description", "Verifies that the correct input device is received for Touch scenarios")]
        [TestProperty("TestPass:ExcludeOn", "WindowsCore")]
        [TestProperty("TestPass:MaxOSVer", WindowsOSVersion._22H2)]    // This test is currently failing on 23h2.
        public void CorrectInputDeviceOnTouchInput()
        {
            const string rootPanelXaml =
                    @"<StackPanel Orientation='Vertical' xmlns='http://schemas.microsoft.com/winfx/2006/xaml/presentation' xmlns:x='http://schemas.microsoft.com/winfx/2006/xaml'>
                            <RadioButton Width='50' x:Name='radiobutton'/>
                            <Button Width='50' x:Name='button' Content='Button 1'/>
                        </StackPanel>";

            StackPanel rootPanel = null;
            Button button = null;
            RadioButton radiobutton = null;
            FrameworkElement[] elementList = null;
            StringBuilder eventOrder = null;


            var radioButtonLosingFocusHandler = new Action<object, LosingFocusEventArgs>((source, args) =>
            {
                VerifyLosingFocusEventArgParameters(args, radiobutton, button, FocusState.Pointer, FocusNavigationDirection.None, false /*Handled*/, FocusInputDeviceKind.Touch, false /*Cancel*/);
            });

            UIExecutor.Execute(() =>
            {
                rootPanel = (StackPanel)XamlReader.Load(rootPanelXaml);
                button = (Button)rootPanel.FindName("button");
                radiobutton = (RadioButton)rootPanel.FindName("radiobutton");
                elementList = new FrameworkElement[2] { button, radiobutton };
                eventOrder = new StringBuilder();

                TestServices.WindowHelper.WindowContent = rootPanel;
            });

            TestServices.WindowHelper.WaitForIdle();

            Log.Comment("Focus radiobutton");
            FocusHelper.EnsureFocus(radiobutton, FocusState.Keyboard);

            using (var focusEventOrderTester = new FocusEventOrderingTester(elementList, eventOrder))
            using (var radioButtonLosingFocus = new EventTester<RadioButton, LosingFocusEventArgs>(radiobutton, "LosingFocus", radioButtonLosingFocusHandler))
            using (var buttonGotFocus = new EventTester<Button, RoutedEventArgs>(button, "GotFocus"))
            {
                Log.Comment("Tapping on button");
                TestServices.InputHelper.Tap(button);
                buttonGotFocus.Wait();
                TestServices.WindowHelper.WaitForIdle();
                Verify.AreEqual("[radiobuttonLosingFocus:1][FocusManagerLosingFocus][buttonGettingFocus][FocusManagerGettingFocus][radiobuttonLostFocus][FocusManagerLostFocus][buttonGotFocus][FocusManagerGotFocus]", eventOrder.ToString());
            }
        }

        [TestMethod]
        [TestProperty("WorkItem", "10112476")]
        [TestProperty("Description", "Verifies that the correct input device is received for Pen scenarios")]
        [TestProperty("TestPass:IncludeOnlyOn", "Desktop")]
        [TestProperty("TestPass:MaxOSVer", WindowsOSVersion._22H2)]    // This test is currently failing on 23h2.
        public void CorrectInputDeviceOnPenInput()
        {
            const string rootPanelXaml =
                    @"<StackPanel Orientation='Vertical' xmlns='http://schemas.microsoft.com/winfx/2006/xaml/presentation' xmlns:x='http://schemas.microsoft.com/winfx/2006/xaml'>
                            <Button Width='50' x:Name='button' Content='Button 1'/>
                            <RadioButton Width='50' x:Name='radiobutton'/>
                        </StackPanel>";

            StackPanel rootPanel = null;
            Button button = null;
            RadioButton radiobutton = null;
            FrameworkElement[] elementList = null;
            StringBuilder eventOrder = null;

            var buttonLosingFocusHandler = new Action<object, LosingFocusEventArgs>((source, args) =>
            {
                VerifyLosingFocusEventArgParameters(args, button, radiobutton, FocusState.Pointer, FocusNavigationDirection.None, false /*Handled*/, FocusInputDeviceKind.Pen, false /*Cancel*/);
            });
            UIExecutor.Execute(() =>
            {
                rootPanel = (StackPanel)XamlReader.Load(rootPanelXaml);
                button = (Button)rootPanel.FindName("button");
                radiobutton = (RadioButton)rootPanel.FindName("radiobutton");
                elementList = new FrameworkElement[2] { button, radiobutton };
                eventOrder = new StringBuilder();
                TestServices.WindowHelper.WindowContent = rootPanel;
            });

            TestServices.WindowHelper.WaitForIdle();

            Log.Comment("Focus button");
            FocusHelper.EnsureFocus(button, FocusState.Keyboard);

            using (var focusEventOrderTester = new FocusEventOrderingTester(elementList, eventOrder))
            using (var buttonLosingFocus = new EventTester<Button, LosingFocusEventArgs>(button, "LosingFocus", buttonLosingFocusHandler))
            using (var radiobuttonGotFocus = new EventTester<RadioButton, RoutedEventArgs>(radiobutton, "GotFocus"))
            {
                Log.Comment("Tapping on the radiobutton with pen");
                TestServices.InputHelper.PenTap(radiobutton);
                radiobuttonGotFocus.Wait();
                TestServices.WindowHelper.WaitForIdle();
                Verify.AreEqual("[buttonLosingFocus:1][FocusManagerLosingFocus][radiobuttonGettingFocus][FocusManagerGettingFocus][buttonLostFocus][FocusManagerLostFocus][radiobuttonGotFocus][FocusManagerGotFocus]", eventOrder.ToString());
            }
            TestServices.WindowHelper.WaitForIdle();
        }

        [TestMethod]
        [TestProperty("Description", "Verifies that the correct navigation direction is received on radiobutton")]
        public void CorrectNavigationDirectionReceivedForRadiobuttonWhenLosingFocus()
        {
            const string rootPanelXaml =
                    @"<StackPanel Orientation='Vertical' xmlns='http://schemas.microsoft.com/winfx/2006/xaml/presentation' xmlns:x='http://schemas.microsoft.com/winfx/2006/xaml'>
                            <RadioButton Width='50' x:Name='radiobutton1'/>
                            <RadioButton Width='50' x:Name='radiobutton2'/>
                        </StackPanel>";

            StackPanel rootPanel = null;
            RadioButton rb1 = null;
            RadioButton rb2 = null;
            FrameworkElement[] elementList = null;
            StringBuilder eventOrder1 = null;
            StringBuilder eventOrder2 = null;

            var rb1LosingFocusHandler = new Action<object, LosingFocusEventArgs>((source, args) =>
            {
                VerifyLosingFocusEventArgParameters(args, rb1, rb2, FocusState.Keyboard, FocusNavigationDirection.Next, false /*Handled*/, FocusInputDeviceKind.Keyboard, false /*Cancel*/);
            });

            var rb2LosingFocusHandler = new Action<object, LosingFocusEventArgs>((source, args) =>
            {
                VerifyLosingFocusEventArgParameters(args, rb2, rb1, FocusState.Keyboard, FocusNavigationDirection.Previous, false /*Handled*/, FocusInputDeviceKind.Keyboard, false /*Cancel*/);
            });

            UIExecutor.Execute(() =>
            {
                rootPanel = (StackPanel)XamlReader.Load(rootPanelXaml);
                rb1 = (RadioButton)rootPanel.FindName("radiobutton1");
                rb2 = (RadioButton)rootPanel.FindName("radiobutton2");
                elementList = new FrameworkElement[2] { rb1, rb2 };
                eventOrder1 = new StringBuilder();
                eventOrder2 = new StringBuilder();
                TestServices.WindowHelper.WindowContent = rootPanel;
            });

            TestServices.WindowHelper.WaitForIdle();

            Log.Comment("Focus radiobutton");
            FocusHelper.EnsureFocus(rb1, FocusState.Keyboard);

            using (var focusEventOrderTester = new FocusEventOrderingTester(elementList, eventOrder1))
            using (var rb1LosingFocus = new EventTester<RadioButton, LosingFocusEventArgs>(rb1, "LosingFocus", rb1LosingFocusHandler))
            using (var rb2GotFocus = new EventTester<RadioButton, RoutedEventArgs>(rb2, "GotFocus"))
            {
                Log.Comment("Injection down key");
                TestServices.KeyboardHelper.Down();
                rb2GotFocus.Wait();
                TestServices.WindowHelper.WaitForIdle();
                Verify.AreEqual("[radiobutton1LosingFocus:1][FocusManagerLosingFocus][radiobutton2GettingFocus][FocusManagerGettingFocus][radiobutton1LostFocus][FocusManagerLostFocus][radiobutton2GotFocus][FocusManagerGotFocus]", eventOrder1.ToString());
            }

            using (var focusEventOrderTester = new FocusEventOrderingTester(elementList, eventOrder2))
            using (var rb2LosingFocus = new EventTester<RadioButton, LosingFocusEventArgs>(rb2, "LosingFocus", rb2LosingFocusHandler))
            using (var rb1GotFocus = new EventTester<RadioButton, RoutedEventArgs>(rb1, "GotFocus"))
            {
                Log.Comment("Injection up key");
                TestServices.KeyboardHelper.Up();
                rb1GotFocus.Wait();
                TestServices.WindowHelper.WaitForIdle();
                Verify.AreEqual("[radiobutton2LosingFocus:1][FocusManagerLosingFocus][radiobutton1GettingFocus][FocusManagerGettingFocus][radiobutton2LostFocus][FocusManagerLostFocus][radiobutton1GotFocus][FocusManagerGotFocus]", eventOrder2.ToString());
            }
        }

        [TestMethod]
        [TestProperty("Description", "Verifies that focus redirection disengages a control")]
        public void VerifyRedirectionDisengagesAControl()
        {
            const string rootPanelXaml =
                    @"<StackPanel Orientation='Vertical' xmlns='http://schemas.microsoft.com/winfx/2006/xaml/presentation' xmlns:x='http://schemas.microsoft.com/winfx/2006/xaml'>
                            <Button Width='50' x:Name='button1' Content='Button 1'/>
                            <Button Width='50' x:Name='button2' IsFocusEngagementEnabled='true' Content='Button 2'/>
                            <Button Width='50' x:Name='button3' Content='Button 3'/>
                    </StackPanel>";

            StackPanel rootPanel = null;
            Button button1 = null;
            Button button2 = null;
            Button button3 = null;
            UIElement focusTarget = null;
            FrameworkElement[] elementList = null;
            StringBuilder eventOrder = null;

            var button2LosingFocusHandler = new Action<object, LosingFocusEventArgs>((source, args) =>
            {
                VerifyLosingFocusEventArgParameters(args, button2, focusTarget, FocusState.Keyboard, FocusNavigationDirection.None, false /*Handled*/, FocusInputDeviceKind.GameController, false /*Cancel*/);
                Log.Comment("Redirecting Focus to Button 1");
                args.NewFocusedElement = button1;
                focusTarget = button1;
            });

            var button2FocusDisengagedHandler = new Action<object, FocusDisengagedEventArgs>((source, args) =>
            {
                eventOrder.Append("[button2FocusDisengaged]");
            });

            UIExecutor.Execute(() =>
            {
                rootPanel = (StackPanel)XamlReader.Load(rootPanelXaml);
                button1 = (Button)rootPanel.FindName("button1");
                button2 = (Button)rootPanel.FindName("button2");
                button3 = (Button)rootPanel.FindName("button3");
                TestServices.WindowHelper.WindowContent = rootPanel;
                elementList = new FrameworkElement[3] { button1, button2, button3 };
                eventOrder = new StringBuilder();
                focusTarget = button3;
            });

            TestServices.WindowHelper.WaitForIdle();

            Log.Comment("Focus button 1");
            FocusHelper.EnsureFocus(button1, FocusState.Keyboard);

            using (var focusEngaged = new EventTester<Button, FocusEngagedEventArgs>(button2, "FocusEngaged"))
            {
                using (var button2GotFocus = new EventTester<Button, RoutedEventArgs>(button2, "GotFocus"))
                {
                    Log.Comment("Injecting Gamepad Down");
                    TestServices.KeyboardHelper.GamepadDpadDown();

                    button2GotFocus.Wait();
                }
                Log.Comment("Injecting Gamepad A");
                TestServices.KeyboardHelper.GamepadA();
                focusEngaged.Wait();
            }

            TestServices.WindowHelper.WaitForIdle();

            UIExecutor.Execute(() =>
            {
                Verify.IsTrue(button2.IsFocusEngaged);
            });

            using (var focusEventOrderTester = new FocusEventOrderingTester(elementList, eventOrder))
            using (var button2FocusDisengaged = new EventTester<Button, FocusDisengagedEventArgs>(button2, "FocusDisengaged", button2FocusDisengagedHandler))
            using (var button2LosingFocus = new EventTester<Button, LosingFocusEventArgs>(button2, "LosingFocus", button2LosingFocusHandler))
            using (var button1GotFocus = new EventTester<Button, RoutedEventArgs>(button1, "GotFocus"))
            {
                Log.Comment("Calling focus on button3");
                UIExecutor.Execute(() =>
                {
                    button3.Focus(FocusState.Keyboard);
                });
                button1GotFocus.Wait();

                UIExecutor.Execute(() =>
                {
                    Verify.IsFalse(button2.IsFocusEngaged);
                    Verify.AreEqual(FocusManager.GetFocusedElement(TestServices.WindowHelper.WindowContent.XamlRoot), button1);
                });

                TestServices.WindowHelper.WaitForIdle();
                Verify.AreEqual("[button2LosingFocus:1][FocusManagerLosingFocus][button2LosingFocus][FocusManagerLosingFocus][button1GettingFocus][FocusManagerGettingFocus][button2FocusDisengaged][button2LostFocus][FocusManagerLostFocus][button1GotFocus][FocusManagerGotFocus]", eventOrder.ToString());
            }
        }

        [TestMethod]
        [TestProperty("Description", "Verifies that focus cannot be cleared by redirecting to null")]
        public void FocusCannotBeClearedUsingChangingFocusEvents()
        {
            const string rootPanelXaml =
                    @"<StackPanel Orientation='Vertical' xmlns='http://schemas.microsoft.com/winfx/2006/xaml/presentation' xmlns:x='http://schemas.microsoft.com/winfx/2006/xaml'>
                                <Button Width='50' x:Name='button1' Content='Button 1'/>
                                <Button Width='50' x:Name='button2' Content='Button 2'/>
                        </StackPanel>";

            StackPanel rootPanel = null;
            Button button1 = null;
            Button button2 = null;
            FrameworkElement[] elementList = null;
            StringBuilder eventOrder = null;

            var button2LosingFocusHandler = new Action<object, LosingFocusEventArgs>((source, args) =>
            {
                Log.Comment("Attempting to clear focus");
                VerifyLosingFocusEventArgParameters(args, button2, button1, FocusState.Keyboard, FocusNavigationDirection.Previous, false /*Handled*/, FocusInputDeviceKind.Keyboard, false /*Cancel*/);
                args.NewFocusedElement = null;
            });

            UIExecutor.Execute(() =>
            {
                rootPanel = (StackPanel)XamlReader.Load(rootPanelXaml);
                button1 = (Button)rootPanel.FindName("button1");
                button2 = (Button)rootPanel.FindName("button2");
                elementList = new FrameworkElement[2] { button1, button2 };
                eventOrder = new StringBuilder();
                TestServices.WindowHelper.WindowContent = rootPanel;
            });

            TestServices.WindowHelper.WaitForIdle();

            Log.Comment("Focus button 2");
            FocusHelper.EnsureFocus(button2, FocusState.Keyboard);

            using (var focusEventOrderTester = new FocusEventOrderingTester(elementList, eventOrder))
            using (var button2LosingFocus = new EventTester<Button, LosingFocusEventArgs>(button2, "LosingFocus", button2LosingFocusHandler))
            using (var button2LostFocus = new EventTester<Button, RoutedEventArgs>(button2, "LostFocus"))
            {
                Log.Comment("Injecting shift-tab");
                TestServices.KeyboardHelper.ShiftTab();
                button2LostFocus.WaitForNoThrow(TimeSpan.FromMilliseconds(100));
                TestServices.WindowHelper.WaitForIdle();

                UIExecutor.Execute(() =>
                {
                    Verify.IsFalse(button2LostFocus.HasFired);
                    Verify.AreEqual(FocusState.Keyboard, button2.FocusState);
                    Verify.AreEqual(button2, FocusManager.GetFocusedElement(TestServices.WindowHelper.WindowContent.XamlRoot));
                    Verify.AreEqual("[button2LosingFocus:1][FocusManagerLosingFocus]", eventOrder.ToString());
                });
            }
        }

        [TestMethod]
        [TestProperty("Description", "Verifies that a SplitView pane open cannot be used to change focus when Losing Focus")]
        public void ExceptionThrownIfSplitViewPaneIsOpenedOnChangingFocus()
        {
            const string rootPanelXaml =
                    @"<StackPanel Orientation='Vertical' xmlns='http://schemas.microsoft.com/winfx/2006/xaml/presentation' xmlns:x='http://schemas.microsoft.com/winfx/2006/xaml'>
                                <SplitView x:Name='splitview'>
                                    <SplitView.Pane>
                                        <Button x:Name='button1'> Button1 </Button>
                                    </SplitView.Pane>
                                <StackPanel>
                                <Button x:Name='button2'> Button2 </Button>
                                <Button x:Name='button3'> Button3 </Button>
                                </StackPanel>
                                </SplitView>
                        </StackPanel>";

            StackPanel rootPanel = null;
            Button button1 = null;
            Button button2 = null;
            Button button3 = null;
            SplitView splitview = null;
            FrameworkElement[] elementList = null;
            StringBuilder eventOrder = null;

            var button2LosingFocusHandler = new Action<object, LosingFocusEventArgs>((source, args) =>
            {
                try
                {
                    Log.Comment("Opening Pane");
                    splitview.IsPaneOpen = true;
                    Verify.Fail("Exception was not thrown on attempted Focus change");
                }
                catch (Exception ex)
                {
                    Verify.AreEqual(ex.HResult, E_FOCUS_CHANGE_NOT_ALLOWED_DURING_FOCUS_CHANGING);
                    Log.Comment("Exception was thrown.");
                }
            });

            var splitviewPaneClosingHandler = new Action<object, SplitViewPaneClosingEventArgs>((source, args) =>
            {
                eventOrder.Append("[splitviewPaneClosing]");
            });

            var splitviewPaneClosedHandler = new Action<object, Object>((source, args) =>
            {
                eventOrder.Append("[splitviewPaneClosed]");
            });

            UIExecutor.Execute(() =>
            {
                rootPanel = (StackPanel)XamlReader.Load(rootPanelXaml);
                button1 = (Button)rootPanel.FindName("button1");
                button2 = (Button)rootPanel.FindName("button2");
                button3 = (Button)rootPanel.FindName("button3");
                splitview = (SplitView)rootPanel.FindName("splitview");
                splitview.IsPaneOpen = false;
                elementList = new FrameworkElement[3] { button1, button2, button3 };
                eventOrder = new StringBuilder();
                TestServices.WindowHelper.WindowContent = rootPanel;
            });

            TestServices.WindowHelper.WaitForIdle();

            Log.Comment("Focus button 2");
            FocusHelper.EnsureFocus(button2, FocusState.Keyboard);

            using (var focusEventOrderTester = new FocusEventOrderingTester(elementList, eventOrder))
            using (var splitViewPaneClosing = new EventTester<SplitView, SplitViewPaneClosingEventArgs>(splitview, "PaneClosing", splitviewPaneClosingHandler))
            using (var splitViewPaneClosed = new EventTester<SplitView, Object>(splitview, "PaneClosed", splitviewPaneClosedHandler))
            using (var button2LosingFocus = new EventTester<Button, LosingFocusEventArgs>(button2, "LosingFocus", button2LosingFocusHandler))
            using (var button2LostFocus = new EventTester<Button, RoutedEventArgs>(button2, "LostFocus"))
            {
                Log.Comment("Injecting tab");
                TestServices.KeyboardHelper.Tab();
                button2LostFocus.Wait();
                TestServices.WindowHelper.WaitForIdle();
                Verify.AreEqual("[button2LosingFocus:1][FocusManagerLosingFocus][button3GettingFocus][FocusManagerGettingFocus][button2LostFocus][FocusManagerLostFocus][button3GotFocus][FocusManagerGotFocus]", eventOrder.ToString());
            }

            TestServices.WindowHelper.WaitForIdle();

            UIExecutor.Execute(() =>
            {
                Verify.IsTrue(splitview.IsPaneOpen);
                Verify.AreEqual(FocusManager.GetFocusedElement(TestServices.WindowHelper.WindowContent.XamlRoot), button3);
            });
        }

        [TestMethod]
        [TestProperty("WorkItem", "10112476")]
        [TestProperty("Description", "Verifies that a SplitView pane close cannot be used to change focus when Losing Focus")]
        [TestProperty("TestPass:IncludeOnlyOn", "Desktop")]
        [TestProperty("TestPass:MaxOSVer", WindowsOSVersion._22H2)]    // This test is currently failing on 23h2.
        public void ExceptionThrownIfSplitViewPaneIsClosedOnChangingFocus()
        {
            const string rootPanelXaml =
                    @"<StackPanel Orientation='Vertical' xmlns='http://schemas.microsoft.com/winfx/2006/xaml/presentation' xmlns:x='http://schemas.microsoft.com/winfx/2006/xaml'>
                                <SplitView x:Name='splitview'>
                                    <SplitView.Pane>
                                        <StackPanel>
                                            <Button x:Name='button1' Height='50'> Button1 </Button>
                                            <Button x:Name='button3'> Button3 </Button>
                                        </StackPanel>
                                    </SplitView.Pane>
                                <Button x:Name='button2' Margin='200,200,0,0'> Button2 </Button>
                                </SplitView>
                        </StackPanel>";

            StackPanel rootPanel = null;
            Button button1 = null;
            Button button2 = null;
            Button button3 = null;
            SplitView splitview = null;
            FrameworkElement[] elementList = null;
            StringBuilder eventOrder = null;

            var button1LosingFocusHandler = new Action<object, LosingFocusEventArgs>((source, args) =>
            {
                try
                {
                    VerifyLosingFocusEventArgParameters(args, button1, button3, FocusState.Keyboard, FocusNavigationDirection.Down, false /*Handled*/, FocusInputDeviceKind.Mouse, false /*Cancel*/);
                    Log.Comment("Button 1 LosingFocus: Closing Pane");
                    splitview.IsPaneOpen = false;
                    Verify.Fail("Exception was not thrown on attempted Focus change");
                }
                catch (Exception ex)
                {
                    Verify.AreEqual(ex.HResult, E_FOCUS_CHANGE_NOT_ALLOWED_DURING_FOCUS_CHANGING);
                    Log.Comment("Exception was thrown.");
                }
            });

            var splitviewPaneClosingHandler = new Action<object, SplitViewPaneClosingEventArgs>((source, args) =>
            {
                eventOrder.Append("[splitviewPaneClosing]");
            });

            var splitviewPaneClosedHandler = new Action<object, Object>((source, args) =>
            {
                eventOrder.Append("[splitviewPaneClosed]");
            });

            UIExecutor.Execute(() =>
            {
                rootPanel = (StackPanel)XamlReader.Load(rootPanelXaml);
                button1 = (Button)rootPanel.FindName("button1");
                button2 = (Button)rootPanel.FindName("button2");
                button3 = (Button)rootPanel.FindName("button3");
                splitview = (SplitView)rootPanel.FindName("splitview");
                splitview.IsPaneOpen = false;
                elementList = new FrameworkElement[3] { button1, button2, button3 };
                eventOrder = new StringBuilder();
                TestServices.WindowHelper.WindowContent = rootPanel;
            });

            TestServices.WindowHelper.WaitForIdle();
            Log.Comment("Focus button 2");
            FocusHelper.EnsureFocus(button2, FocusState.Keyboard);

            UIExecutor.Execute(() =>
            {
                splitview.IsPaneOpen = true;
            });

            TestServices.WindowHelper.WaitForIdle();

            using (var button1Click = new EventTester<Button, RoutedEventArgs>(button1, "Click"))
            {
                Log.Comment("Focus button 1 using a pointer");
                TestServices.InputHelper.LeftMouseClick(button1);
                button1Click.Wait();
            }
            // it is ensured that all pending FocusManager events are caught so that they don't mess up the ordering of the following events
            FocusHelper.EnsureFocus(button1, FocusState.Pointer);

            using (var focusEventOrderTester = new FocusEventOrderingTester(elementList, eventOrder))
            using (var splitViewPaneClosing = new EventTester<SplitView, SplitViewPaneClosingEventArgs>(splitview, "PaneClosing", splitviewPaneClosingHandler))
            using (var splitViewPaneClosed = new EventTester<SplitView, Object>(splitview, "PaneClosed", splitviewPaneClosedHandler))
            using (var button1LosingFocus = new EventTester<Button, LosingFocusEventArgs>(button1, "LosingFocus", button1LosingFocusHandler))
            using (var button3GotFocus = new EventTester<Button, RoutedEventArgs>(button3, "GotFocus"))
            {

                UIExecutor.Execute(() =>
                {
                    Log.Comment("Trying to move focus");
                    FindNextElementOptions options = new FindNextElementOptions();
                    options.SearchRoot = TestServices.WindowHelper.WindowContent;
                    Verify.IsTrue(FocusManager.TryMoveFocus(FocusNavigationDirection.Down, options));
                });
                button3GotFocus.Wait();
                UIExecutor.Execute(() =>
                {
                    Verify.IsFalse(splitViewPaneClosed.HasFired);
                    Verify.AreEqual(button3, FocusManager.GetFocusedElement(TestServices.WindowHelper.WindowContent.XamlRoot));
                });

                TestServices.WindowHelper.WaitForIdle();
                Verify.AreEqual("[button1LosingFocus:1][FocusManagerLosingFocus][button3GettingFocus][FocusManagerGettingFocus][splitviewPaneClosing][button1LostFocus][FocusManagerLostFocus][button3GotFocus][FocusManagerGotFocus]" +
                                "[button3LosingFocus:2][FocusManagerLosingFocus][FocusManagerGettingFocus][button3LostFocus][FocusManagerLostFocus][FocusManagerGotFocus][splitviewPaneClosed]",
                    eventOrder.ToString());
            }
        }

        [TestMethod]
        [TestProperty("Description", "Verifies that a Popup open cannot be used to change focus when Losing Focus")]
        public void ExceptionThrownIfPopupOpenedInChangingFocusHandler()
        {
            const string rootPanelXaml =
                    @"<StackPanel Orientation='Vertical' xmlns='http://schemas.microsoft.com/winfx/2006/xaml/presentation' xmlns:x='http://schemas.microsoft.com/winfx/2006/xaml'>
                                <Popup x:Name='popup' IsLightDismissEnabled='true'>
                                    <Button />
                                </Popup>
                                <StackPanel>
                                  <Button x:Name='button1'> Button1 </Button>
                                  <Button x:Name='button2'> Button2 </Button>
                                </StackPanel>
                        </StackPanel>";

            StackPanel rootPanel = null;
            Button button1 = null;
            Button button2 = null;
            Microsoft.UI.Xaml.Controls.Primitives.Popup popup = null;
            FrameworkElement[] elementList = null;
            StringBuilder eventOrder = null;

            var button1LosingFocusHandler = new Action<object, LosingFocusEventArgs>((source, args) =>
            {
                try
                {
                    Log.Comment("Button 1 LosingFocus: Opening Popup");
                    popup.IsOpen = true;
                    Verify.Fail("Exception was not thrown on attempted Focus change");
                }
                catch (Exception ex)
                {
                    Verify.AreEqual(ex.HResult, E_FOCUS_CHANGE_NOT_ALLOWED_DURING_FOCUS_CHANGING);
                    Log.Comment("Exception was thrown.");
                }
            });

            var popupOpenedHandler = new Action<object, object>((source, args) =>
            {
                //We do not expect PopupOpened to be fired
                eventOrder.Append("[popupOpened]");
            });

            UIExecutor.Execute(() =>
            {
                rootPanel = (StackPanel)XamlReader.Load(rootPanelXaml);
                button1 = (Button)rootPanel.FindName("button1");
                button2 = (Button)rootPanel.FindName("button2");
                popup = (Microsoft.UI.Xaml.Controls.Primitives.Popup)rootPanel.FindName("popup");
                popup.IsOpen = false;
                elementList = new FrameworkElement[3] { button1, button2, popup };
                eventOrder = new StringBuilder();
                TestServices.WindowHelper.WindowContent = rootPanel;
            });

            TestServices.WindowHelper.WaitForIdle();
            Log.Comment("Focus button 1");
            FocusHelper.EnsureFocus(button1, FocusState.Keyboard);

            using (var focusEventOrderTester = new FocusEventOrderingTester(elementList, eventOrder))
            using (var popupOpened = new EventTester<Microsoft.UI.Xaml.Controls.Primitives.Popup, object>(popup, "Opened", popupOpenedHandler))
            using (var button1LosingFocus = new EventTester<Button, LosingFocusEventArgs>(button1, "LosingFocus", button1LosingFocusHandler))
            using (var button1LostFocus = new EventTester<Button, RoutedEventArgs>(button1, "LostFocus"))
            {
                Log.Comment("Injecting tab");
                TestServices.KeyboardHelper.Tab();
                button1LostFocus.Wait();
                TestServices.WindowHelper.WaitForIdle();
                Verify.AreEqual("[button1LosingFocus:1][FocusManagerLosingFocus][button2GettingFocus][FocusManagerGettingFocus][button1LostFocus][FocusManagerLostFocus][button2GotFocus][FocusManagerGotFocus]", eventOrder.ToString());
            }

            TestServices.WindowHelper.WaitForIdle();

            UIExecutor.Execute(() =>
            {
                Verify.IsTrue(popup.IsOpen);
                Verify.AreEqual(FocusManager.GetFocusedElement(TestServices.WindowHelper.WindowContent.XamlRoot), button2);
            });

        }

        [TestMethod]
        [TestProperty("Description", "Verifies that a Popup close cannot be used to change focus when Losing Focus")]
        [TestProperty("Ignore", "True")]
        [TestProperty("TestPass:IncludeOnlyOn", "Desktop")]
        public void ExceptionThrownIfPopupClosedInChangingFocusHandler()
        {
            const string rootPanelXaml =
                    @"<StackPanel Orientation='Vertical' xmlns='http://schemas.microsoft.com/winfx/2006/xaml/presentation' xmlns:x='http://schemas.microsoft.com/winfx/2006/xaml'>
                                <Popup x:Name='popup' IsLightDismissEnabled='true'>
                                    <Button x:Name='button3'/>
                                </Popup>
                                <StackPanel>
                                  <Button x:Name='button1'> Button1 </Button>
                                  <Button x:Name='button2'> Button2 </Button>
                                </StackPanel>
                        </StackPanel>";

            StackPanel rootPanel = null;
            Button button1 = null;
            Button button2 = null;
            Button button3 = null;
            Microsoft.UI.Xaml.Controls.Primitives.Popup popup = null;

            var button3LosingFocusHandler = new Action<object, LosingFocusEventArgs>((source, args) =>
            {
                try
                {
                    if (popup.IsOpen)
                    {
                        Log.Comment("Button 3 LosingFocus: Closing Popup");
                        popup.IsOpen = false;
                        Verify.Fail("Exception not thrown on closing SplitView");
                    }
                }
                catch (Exception ex)
                {
                    Verify.AreEqual(ex.HResult, E_FOCUS_CHANGE_NOT_ALLOWED_DURING_FOCUS_CHANGING);
                    Log.Comment("Exception was thrown, redirecting Focus to button2");

                    args.NewFocusedElement = button2;
                }
            });

            UIExecutor.Execute(() =>
            {
                rootPanel = (StackPanel)XamlReader.Load(rootPanelXaml);
                button1 = (Button)rootPanel.FindName("button1");
                button2 = (Button)rootPanel.FindName("button2");
                button3 = (Button)rootPanel.FindName("button3");
                popup = (Microsoft.UI.Xaml.Controls.Primitives.Popup)rootPanel.FindName("popup");
                popup.IsOpen = false;
                TestServices.WindowHelper.WindowContent = rootPanel;
            });

            TestServices.WindowHelper.WaitForIdle();
            Log.Comment("Focus button 1");
            FocusHelper.EnsureFocus(button1, FocusState.Keyboard);

            UIExecutor.Execute(() =>
            {
                popup.IsOpen = true;
            });

            TestServices.WindowHelper.WaitForIdle();

            Log.Comment("Focus button 3");
            FocusHelper.EnsureFocus(button3, FocusState.Keyboard);

            using (var button3LosingFocus = new EventTester<Button, LosingFocusEventArgs>(button3, "LosingFocus", button3LosingFocusHandler))
            {
                using (var button2GotFocus = new EventTester<Button, RoutedEventArgs>(button2, "GotFocus"))
                {
                    Log.Comment("Injecting tab");
                    TestServices.KeyboardHelper.Tab();
                    button2GotFocus.Wait();
                }
            }
            TestServices.WindowHelper.WaitForIdle();

            UIExecutor.Execute(() =>
            {
                Verify.AreEqual(FocusManager.GetFocusedElement(TestServices.WindowHelper.WindowContent.XamlRoot), button2);
                Verify.IsTrue(popup.IsOpen);
            });
        }

        [TestMethod]
        [TestProperty("Description", "Verifies changing focus args using AutomationPeer.SetFocus")]
        [TestProperty("TestPass:ExcludeOn", "OneCore")]
        public void VerifyChangingFocusArgsOnAutomationPeerSetFocus()
        {
            const string rootPanelXaml =
                    @"<StackPanel Orientation='Vertical' xmlns='http://schemas.microsoft.com/winfx/2006/xaml/presentation' xmlns:x='http://schemas.microsoft.com/winfx/2006/xaml'>
                                <Button Width='50' x:Name='button1' Content='Button 1'/>
                                <Button Width='50' x:Name='button2' Content='Button 2'/>
                        </StackPanel>";

            StackPanel rootPanel = null;
            Button button1 = null;
            Button button2 = null;
            Automation.Peers.AutomationPeer button2AP = null;
            FrameworkElement[] elementList = null;
            StringBuilder eventOrder = null;

            var button1LosingFocusHandler = new Action<object, LosingFocusEventArgs>((source, args) =>
            {
                VerifyLosingFocusEventArgParameters(args, button1, button2, FocusState.Programmatic, FocusNavigationDirection.None, false /*Handled*/, FocusInputDeviceKind.None, false /*Cancel*/);
            });

            var button2GettingFocusHandler = new Action<object, GettingFocusEventArgs>((source, args) =>
            {
                VerifyGettingFocusEventArgParameters(args, button1, button2, FocusState.Programmatic, FocusNavigationDirection.None, false /*Handled*/, FocusInputDeviceKind.None, false /*Cancel*/);
            });

            UIExecutor.Execute(() =>
            {
                rootPanel = (StackPanel)XamlReader.Load(rootPanelXaml);
                button1 = (Button)rootPanel.FindName("button1");
                button2 = (Button)rootPanel.FindName("button2");

                button2AP = Automation.Peers.FrameworkElementAutomationPeer.CreatePeerForElement(button2);
                elementList = new FrameworkElement[2] { button1, button2 };
                eventOrder = new StringBuilder();
                TestServices.WindowHelper.WindowContent = rootPanel;
            });

            TestServices.WindowHelper.WaitForIdle();

            XamlRoot xamlRoot = null;
            UIExecutor.Execute(() =>
            {
                xamlRoot = TestServices.WindowHelper.WindowContent.XamlRoot;
            });

            TestServices.WindowHelper.SetLastInputMethod(global::Private.Infrastructure.LastInputDeviceType.None, xamlRoot);

            Log.Comment("Focus button 1");
            FocusHelper.EnsureFocus(button1, FocusState.Keyboard);

            using (var focusEventOrderTester = new FocusEventOrderingTester(elementList, eventOrder))
            using (var button1LosingFocus = new EventTester<Button, LosingFocusEventArgs>(button1, "LosingFocus", button1LosingFocusHandler))
            using (var button2GettingFocus = new EventTester<Button, GettingFocusEventArgs>(button2, "GettingFocus", button2GettingFocusHandler))
            using (var button2GotFocus = new EventTester<Button, RoutedEventArgs>(button2, "GotFocus"))
            {
                UIExecutor.Execute(() =>
                {
                    Log.Comment("Calling SetFocus");
                    button2AP.SetFocus();
                });
                button2GotFocus.Wait();

                UIExecutor.Execute(() =>
                {
                    Verify.AreEqual(FocusManager.GetFocusedElement(TestServices.WindowHelper.WindowContent.XamlRoot), button2);
                    Log.Comment("Checking focus order");
                });

                TestServices.WindowHelper.WaitForIdle();
                Verify.AreEqual("[button1LosingFocus:1][FocusManagerLosingFocus][button2GettingFocus][FocusManagerGettingFocus][button1LostFocus][FocusManagerLostFocus][button2GotFocus][FocusManagerGotFocus]", eventOrder.ToString());
            }

            TestServices.WindowHelper.WaitForIdle();
        }

        [TestMethod]
        [TestProperty("Description", "Verifies that a flyout open cannot be used to change focus when Losing Focus")]
        [TestProperty("TestPass:ExcludeOn", "WindowsCore")] // Escape key not working
        public void ExceptionThrownIfFlyoutOpenedInChangingFocusHandler()
        {
            const string rootPanelXaml =
                    @"<StackPanel Orientation='Vertical' xmlns='http://schemas.microsoft.com/winfx/2006/xaml/presentation' xmlns:x='http://schemas.microsoft.com/winfx/2006/xaml'>
                                  <Button x:Name='button1' Content='Button1' />
                                  <Button x:Name='button2' Margin='100, 100'> Button2 </Button>
                        </StackPanel>";

            StackPanel rootPanel = null;
            Button button1 = null;
            Button button2 = null;
            Flyout flyout = null;

            var button2GettingFocusHandler = new Action<object, GettingFocusEventArgs>((source, args) =>
            {
                try
                {
                    VerifyGettingFocusEventArgParameters(args, button1, button2, FocusState.Keyboard, FocusNavigationDirection.Next, false /*Handled*/, FocusInputDeviceKind.Keyboard, false /*Cancel*/);
                    Log.Comment("Button 2 GettingFocus: Opening Popup");
                    flyout.ShowAt(button1);
                    Verify.Fail("Exception was not thrown on attempted Focus change");
                }
                catch (Exception ex)
                {
                    Verify.AreEqual(ex.HResult, E_FOCUS_CHANGE_NOT_ALLOWED_DURING_FOCUS_CHANGING);
                    Log.Comment("Exception was thrown.");
                }
            });

            UIExecutor.Execute(() =>
            {
                rootPanel = (StackPanel)XamlReader.Load(rootPanelXaml);
                button1 = (Button)rootPanel.FindName("button1");
                button2 = (Button)rootPanel.FindName("button2");
                flyout = new Flyout();
                flyout.Content = new Button { Name = "FlyoutContent", Height = 30, Width = 30, Content = "Flyout is open" };

                TestServices.WindowHelper.WindowContent = rootPanel;
            });

            TestServices.WindowHelper.WaitForIdle();
            Log.Comment("Focus button 1");
            FocusHelper.EnsureFocus(button1, FocusState.Keyboard);

            using (var flyoutOpened = new EventTester<Flyout, object>(flyout, "Opened"))
            using (var button2GettingFocus = new EventTester<Button, GettingFocusEventArgs>(button2, "GettingFocus", button2GettingFocusHandler))
            using (var button2GotFocus = new EventTester<Button, RoutedEventArgs>(button2, "GotFocus"))
            {
                Log.Comment("Injecting tab");
                TestServices.KeyboardHelper.Tab();
                button2GotFocus.Wait();

                UIExecutor.Execute(() =>
                {
                    Verify.IsTrue(button2GettingFocus.HasFired);
                    Verify.IsTrue(flyoutOpened.HasFired);
                });
            }

            using (var flyoutClose = new EventTester<Flyout, object>(flyout, "Closed"))
            {
                TestServices.KeyboardHelper.Escape();
                Log.Comment("Waiting for flyout to close");
                flyoutClose.Wait();
            }
        }

        [TestMethod]
        public void ValidateChangingFocusEventsOnlyCalledOnceWhenTabbingToAListViewHeader()
        {
            TestServices.WindowHelper.SetWindowSizeOverride(new Size(400, 800));

            StackPanel rootPanel = null;
            ListView lv = null;
            Button beforeButton = null;
            Button headerButton = null;
            Button afterButton = null;
            FrameworkElement[] elementList = null;
            StringBuilder eventOrder = null;

            const string rootPanelXaml =
                 @"<StackPanel Orientation='Vertical' xmlns='http://schemas.microsoft.com/winfx/2006/xaml/presentation' xmlns:x='http://schemas.microsoft.com/winfx/2006/xaml'>
                                  <Button x:Name='beforeButton' >Before</Button>
                                  <ListView x:Name='lv'>
                                    <ListViewItem> Item1 </ListViewItem>
                                    <ListViewItem> Item2 </ListViewItem>
                                  </ListView>
                                  <Button x:Name='afterButton'> After </Button>
                        </StackPanel>";


            var lvGettingFocusHandler = new Action<object, GettingFocusEventArgs>((source, args) =>
            {
                    VerifyGettingFocusEventArgParameters(args, beforeButton, headerButton, FocusState.Keyboard, FocusNavigationDirection.Next, false /*Handled*/, FocusInputDeviceKind.Keyboard, false /*Cancel*/);
            });

            UIExecutor.Execute(() =>
            {
                rootPanel = (StackPanel)XamlReader.Load(rootPanelXaml);
                beforeButton = (Button)rootPanel.FindName("beforeButton");
                afterButton = (Button)rootPanel.FindName("afterButton");
                headerButton = new Button { Content = "button in Header", Name = "headerButton" };
                lv = (ListView)rootPanel.FindName("lv");

                lv.Header = headerButton;
                elementList = new FrameworkElement[4] { beforeButton, afterButton, headerButton, lv };
                eventOrder = new StringBuilder();
                TestServices.WindowHelper.WindowContent = rootPanel;
            });

            TestServices.WindowHelper.WaitForIdle();

            TestServices.WindowHelper.WaitForIdle();
            Log.Comment("Focus beforeButton");
            FocusHelper.EnsureFocus(beforeButton, FocusState.Keyboard);

            using (var focusEventOrderTester = new FocusEventOrderingTester(elementList, eventOrder))
            using (var lvGettingFocus = new EventTester<ListView, GettingFocusEventArgs>(lv, "GettingFocus", lvGettingFocusHandler))
            using (var lvGotFocus = new EventTester<ListView, RoutedEventArgs>(lv, "GotFocus"))
            {
                Log.Comment("Injecting tab");
                TestServices.KeyboardHelper.Tab();
                lvGotFocus.Wait();

                TestServices.WindowHelper.WaitForIdle();
                Verify.AreEqual("[beforeButtonLosingFocus:1][FocusManagerLosingFocus][headerButtonGettingFocus][lvGettingFocus][FocusManagerGettingFocus][beforeButtonLostFocus][FocusManagerLostFocus][headerButtonGotFocus][lvGotFocus][FocusManagerGotFocus]", eventOrder.ToString());
            }


        }

        [TestMethod]
        [TestProperty("Description", "Verifies gamepad Up and Down can raise NoFocusCandidateFound")]
        public void VerifyGamepadUpAndDownCanCauseNoFocusCandidateRaise()
        {
            const string rootPanelXaml =
                    @"<StackPanel xmlns='http://schemas.microsoft.com/winfx/2006/xaml/presentation' xmlns:x='http://schemas.microsoft.com/winfx/2006/xaml'>
                            <StackPanel Orientation='Horizontal'>
                                  <Button x:Name='button0' Content='Button0' Background='Green'/>
                                  <Button x:Name='button1' Content='Button1' Background='Red'/>
                                  <Button x:Name='button2' Content='Button2' Background='Yellow'/>
                            </StackPanel>
                        </StackPanel>";

            StackPanel rootPanel = null;
            Button button0 = null;
            Button button1 = null;
            Button button2 = null;

            FrameworkElement[] elementList = null;
            StringBuilder eventOrderUp = null;
            StringBuilder eventOrderDown = null;

            var button1NoFocusCandidateFoundEventArgsUp = new Action<object, NoFocusCandidateFoundEventArgs>((source, args) =>
            {
                VerifyNoFocusCandidateFoundEventArgParameters(args, button1, FocusNavigationDirection.Up, false /*Handled*/, FocusInputDeviceKind.GameController);
            });

            var button1NoFocusCandidateFoundEventArgsDown = new Action<object, NoFocusCandidateFoundEventArgs>((source, args) =>
            {
                VerifyNoFocusCandidateFoundEventArgParameters(args, button1, FocusNavigationDirection.Down, false /*Handled*/, FocusInputDeviceKind.GameController);
            });

            UIExecutor.Execute(() =>
            {
                rootPanel = (StackPanel)XamlReader.Load(rootPanelXaml);
                button0 = (Button)rootPanel.FindName("button0");
                button1 = (Button)rootPanel.FindName("button1");
                button2 = (Button)rootPanel.FindName("button2");

                elementList = new FrameworkElement[3] { button1, button2, button0 };
                eventOrderUp = new StringBuilder();
                eventOrderDown = new StringBuilder();
                TestServices.WindowHelper.WindowContent = rootPanel;
            });

            TestServices.WindowHelper.WaitForIdle();
            Log.Comment("Focus Button1");
            FocusHelper.EnsureFocus(button1, FocusState.Keyboard);

            using (var focusEventOrderTester = new FocusEventOrderingTester(elementList, eventOrderUp))
            using (var button1NoFocusCandidateFound = new EventTester<Button, NoFocusCandidateFoundEventArgs>(button1, "NoFocusCandidateFound", button1NoFocusCandidateFoundEventArgsUp))
            {
                Log.Comment("Injecting GameController Up");
                TestServices.KeyboardHelper.GamepadDpadUp();
                button1NoFocusCandidateFound.Wait();

                UIExecutor.Execute(() =>
                {
                    Verify.AreEqual("[button1NoFocusCandidateFound]", eventOrderUp.ToString());
                });
            }

            TestServices.WindowHelper.WaitForIdle();

            using (var focusEventOrderTester = new FocusEventOrderingTester(elementList, eventOrderDown))
            using (var button1NoFocusCandidateFound = new EventTester<Button, NoFocusCandidateFoundEventArgs>(button1, "NoFocusCandidateFound", button1NoFocusCandidateFoundEventArgsDown))
            {
                Log.Comment("Injecting GameController Down");
                TestServices.KeyboardHelper.GamepadDpadDown();
                button1NoFocusCandidateFound.Wait();

                UIExecutor.Execute(() =>
                {
                    Verify.AreEqual("[button1NoFocusCandidateFound]", eventOrderDown.ToString());
                });
            }
        }

        [TestMethod]
        [TestProperty("Description", "Verifies gamepad Left and Right can raise NoFocusCandidateFound")]
        public void VerifyGamepadLeftAndRightCanCauseNoFocusCandidateRaise()
        {
            const string rootPanelXaml =
                    @"<StackPanel xmlns='http://schemas.microsoft.com/winfx/2006/xaml/presentation' xmlns:x='http://schemas.microsoft.com/winfx/2006/xaml'>
                            <StackPanel Orientation='Vertical'>
                                  <Button x:Name='button0' Content='Button0' Background='Green'/>
                                  <Button x:Name='button1' Content='Button1' Background='Red'/>
                                  <Button x:Name='button2' Content='Button2' Background='Yellow'/>
                            </StackPanel>
                        </StackPanel>";

            StackPanel rootPanel = null;
            Button button0 = null;
            Button button1 = null;
            Button button2 = null;

            FrameworkElement[] elementList = null;
            StringBuilder eventOrderLeft = null;
            StringBuilder eventOrderRight = null;

            var button1NoFocusCandidateFoundEventArgsLeft = new Action<object, NoFocusCandidateFoundEventArgs>((source, args) =>
            {
                VerifyNoFocusCandidateFoundEventArgParameters(args, button1, FocusNavigationDirection.Left, false /*Handled*/, FocusInputDeviceKind.GameController);
            });

            var button1NoFocusCandidateFoundEventArgsRight = new Action<object, NoFocusCandidateFoundEventArgs>((source, args) =>
            {
                VerifyNoFocusCandidateFoundEventArgParameters(args, button1, FocusNavigationDirection.Right, false /*Handled*/, FocusInputDeviceKind.GameController);
            });

            UIExecutor.Execute(() =>
            {
                rootPanel = (StackPanel)XamlReader.Load(rootPanelXaml);
                button0 = (Button)rootPanel.FindName("button0");
                button1 = (Button)rootPanel.FindName("button1");
                button2 = (Button)rootPanel.FindName("button2");

                elementList = new FrameworkElement[3] { button1, button2, button0 };
                eventOrderLeft = new StringBuilder();
                eventOrderRight = new StringBuilder();
                TestServices.WindowHelper.WindowContent = rootPanel;
            });

            TestServices.WindowHelper.WaitForIdle();
            Log.Comment("Focus Button1");
            FocusHelper.EnsureFocus(button1, FocusState.Keyboard);

            using (var focusEventOrderTester = new FocusEventOrderingTester(elementList, eventOrderLeft))
            using (var button1NoFocusCandidateFound = new EventTester<Button, NoFocusCandidateFoundEventArgs>(button1, "NoFocusCandidateFound", button1NoFocusCandidateFoundEventArgsLeft))
            {
                Log.Comment("Injecting GameController Left");
                TestServices.KeyboardHelper.GamepadDpadLeft();
                button1NoFocusCandidateFound.Wait();

                UIExecutor.Execute(() =>
                {
                    Verify.AreEqual("[button1NoFocusCandidateFound]", eventOrderLeft.ToString());
                });
            }

            TestServices.WindowHelper.WaitForIdle();

            using (var focusEventOrderTester = new FocusEventOrderingTester(elementList, eventOrderRight))
            using (var button1NoFocusCandidateFound = new EventTester<Button, NoFocusCandidateFoundEventArgs>(button1, "NoFocusCandidateFound", button1NoFocusCandidateFoundEventArgsRight))
            {
                Log.Comment("Injecting GameController Right");
                TestServices.KeyboardHelper.GamepadDpadRight();
                button1NoFocusCandidateFound.Wait();

                UIExecutor.Execute(() =>
                {
                    Verify.AreEqual("[button1NoFocusCandidateFound]", eventOrderRight.ToString());
                });
            }
        }


        [TestMethod]
        [TestProperty("Description", "Verifies TryMoveFocus cannot raise NoFocusCandidateFound")]
        public void VerifyTryMoveFocusDoesNotCauseNoFocusCandidateRaise()
        {
            const string rootPanelXaml =
                    @"<StackPanel xmlns='http://schemas.microsoft.com/winfx/2006/xaml/presentation' xmlns:x='http://schemas.microsoft.com/winfx/2006/xaml'>
                            <StackPanel Orientation='Horizontal'>
                                  <Button x:Name='button0' Content='Button0' Background='Green'/>
                                  <Button x:Name='button1' Content='Button1' Background='Red'/>
                                  <Button x:Name='button2' Content='Button2' Background='Yellow'/>
                            </StackPanel>
                        </StackPanel>";

            StackPanel rootPanel = null;
            Button button0 = null;
            Button button1 = null;
            Button button2 = null;

            FrameworkElement[] elementList = null;
            StringBuilder eventOrder = null;

            var button1NoFocusCandidateFoundEventArgs = new Action<object, NoFocusCandidateFoundEventArgs>((source, args) =>
            {
                Verify.Fail("NoFocusCandidateFound was invoked.");
            });

            UIExecutor.Execute(() =>
            {
                rootPanel = (StackPanel)XamlReader.Load(rootPanelXaml);
                button0 = (Button)rootPanel.FindName("button0");
                button1 = (Button)rootPanel.FindName("button1");
                button2 = (Button)rootPanel.FindName("button2");

                elementList = new FrameworkElement[3] { button1, button2, button0 };
                eventOrder = new StringBuilder();
                TestServices.WindowHelper.WindowContent = rootPanel;
            });

            TestServices.WindowHelper.WaitForIdle();
            Log.Comment("Focus Button1");
            FocusHelper.EnsureFocus(button1, FocusState.Keyboard);

            using (var focusEventOrderTester = new FocusEventOrderingTester(elementList, eventOrder))
            using (var button1NoFocusCandidateFound = new EventTester<Button, NoFocusCandidateFoundEventArgs>(button1, "NoFocusCandidateFound", button1NoFocusCandidateFoundEventArgs))
            {
                UIExecutor.Execute(() =>
                {
                    FindNextElementOptions options = new FindNextElementOptions();
                    options.SearchRoot = TestServices.WindowHelper.WindowContent;
                    Verify.IsFalse(FocusManager.TryMoveFocus(FocusNavigationDirection.Up, options));
                });

                button1NoFocusCandidateFound.WaitForNoThrow(TimeSpan.FromMilliseconds(100));

                UIExecutor.Execute(() =>
                {
                    Verify.IsFalse(button1NoFocusCandidateFound.HasFired);
                    Verify.AreEqual("", eventOrder.ToString());
                });
            }
        }

        [TestMethod]
        [TestProperty("Description", "Verifies Tabbing cannot raise NoFocusCandidateFound if focusable elements are on the page")]
        [TestProperty("Hosting:Mode", "UAP")]
        public void VerifyTabbingCanNotCauseNoFocusCandidateRaiseIfFocusableElementsAreOnThePage()
        {
            const string rootPanelXaml =
                    @"<StackPanel xmlns='http://schemas.microsoft.com/winfx/2006/xaml/presentation' xmlns:x='http://schemas.microsoft.com/winfx/2006/xaml'>
                            <StackPanel Orientation='Horizontal'>
                                  <Button x:Name='button1' Content='Button1' Background='Red'/>
                            </StackPanel>
                        </StackPanel>";

            StackPanel rootPanel = null;
            Button button1 = null;

            var button1NoFocusCandidateFoundEventArgs = new Action<object, NoFocusCandidateFoundEventArgs>((source, args) =>
            {
                Verify.Fail("NoFocusCandidateFound was fired.");
            });

            UIExecutor.Execute(() =>
            {
                rootPanel = (StackPanel)XamlReader.Load(rootPanelXaml);
                button1 = (Button)rootPanel.FindName("button1");
                TestServices.WindowHelper.WindowContent = rootPanel;
            });

            TestServices.WindowHelper.WaitForIdle();
            Log.Comment("Focus Button1");
            FocusHelper.EnsureFocus(button1, FocusState.Keyboard);

            using (var button1NoFocusCandidateFound = new EventTester<Button, NoFocusCandidateFoundEventArgs>(button1, "NoFocusCandidateFound", button1NoFocusCandidateFoundEventArgs))
            using (var button1GotFocus = new EventTester<Button, RoutedEventArgs>(button1, "GotFocus"))
            {
                Log.Comment("Injecting Tab");
                TestServices.KeyboardHelper.Tab();
                //Tab wrapping causes Button1 to lose and then regain focus
                button1GotFocus.Wait();
                Verify.IsFalse(button1NoFocusCandidateFound.HasFired);
            }
        }


        [TestMethod]
        [TestProperty("Description", "Verifies tab wrapping does not raise NoFocusCandidateFound")]
        [TestProperty("Hosting:Mode", "UAP")]   // Focus engagement bugs in lifted islands
        public void VerifyTabWrappingDoesNotCauseNoFocusCandidateRaise()
        {
            const string rootPanelXaml =
                    @"<StackPanel xmlns='http://schemas.microsoft.com/winfx/2006/xaml/presentation' xmlns:x='http://schemas.microsoft.com/winfx/2006/xaml'>
                            <StackPanel Orientation='Horizontal'>
                                  <Button x:Name='button0' Content='Button0' Background='Green' />
                                  <Button x:Name='button1' Content='Button1' Background='Red'/>
                            </StackPanel>
                        </StackPanel>";

            StackPanel rootPanel = null;
            Button button1 = null;
            Button button0 = null;
            FrameworkElement[] elementList = null;
            StringBuilder eventOrder = null;

            UIExecutor.Execute(() =>
            {
                rootPanel = (StackPanel)XamlReader.Load(rootPanelXaml);
                button1 = (Button)rootPanel.FindName("button1");
                button0 = (Button)rootPanel.FindName("button0");
                elementList = new FrameworkElement[2] { button1, button0 };
                eventOrder = new StringBuilder();
                TestServices.WindowHelper.WindowContent = rootPanel;
            });

            TestServices.WindowHelper.WaitForIdle();
            Log.Comment("Focus Button1");
            FocusHelper.EnsureFocus(button1, FocusState.Keyboard);

            var button0GettingFocusHandler = new Action<object, GettingFocusEventArgs>((source, args) =>
            {
                Verify.AreEqual(args.OldFocusedElement, null);
            });

            var button1LosingFocusHandler = new Action<object, LosingFocusEventArgs>((source, args) =>
            {
                Verify.AreEqual(args.NewFocusedElement, null);
            });

            using (var losingFocusEventTester = new EventTester<UIElement, LosingFocusEventArgs>(button1, "LosingFocus", button1LosingFocusHandler))
            using (var gettingFocusEventTester = new EventTester<UIElement, GettingFocusEventArgs>(button0, "GettingFocus", button0GettingFocusHandler))
            using (var button0GotFocus = new EventTester<Button, RoutedEventArgs>(button0, "GotFocus"))
            using (var focusEventOrderTester = new FocusEventOrderingTester(elementList, eventOrder))
            {
                Log.Comment("Injecting Tab");
                TestServices.KeyboardHelper.Tab();

                button0GotFocus.Wait();

                TestServices.WindowHelper.WaitForIdle();
                Verify.AreEqual("[button1LosingFocus:1][FocusManagerLosingFocus][FocusManagerGettingFocus]" +
                                "[FocusManagerLosingFocus:2][button0GettingFocus][FocusManagerGettingFocus]" +
                                "[button1LostFocus][FocusManagerLostFocus:1][FocusManagerGotFocus]" +
                                "[FocusManagerLostFocus:2][button0GotFocus][FocusManagerGotFocus]",
                    eventOrder.ToString());
            }
        }

        [TestMethod]
        [TestProperty("Description", "Verifies shift + tab wrapping does not start separate focus movement operation")]
        [TestProperty("Hosting:Mode", "UAP")]   // Focus engagement bugs in lifted islands
        public void VerifyShiftTabWrappingIsOneFocusMovementOperation()
        {
            const string rootPanelXaml =
                    @"<StackPanel xmlns='http://schemas.microsoft.com/winfx/2006/xaml/presentation' xmlns:x='http://schemas.microsoft.com/winfx/2006/xaml'>
                            <StackPanel Orientation='Horizontal'>
                                  <Button x:Name='button0' Content='Button0' Background='Green' />
                                  <Button x:Name='button1' Content='Button1' Background='Red'/>
                            </StackPanel>
                        </StackPanel>";

            StackPanel rootPanel = null;
            Button button1 = null;
            Button button0 = null;
            FrameworkElement[] elementList = null;
            StringBuilder eventOrder = null;

            UIExecutor.Execute(() =>
            {
                rootPanel = (StackPanel)XamlReader.Load(rootPanelXaml);
                button1 = (Button)rootPanel.FindName("button1");
                button0 = (Button)rootPanel.FindName("button0");
                elementList = new FrameworkElement[2] { button1, button0 };
                eventOrder = new StringBuilder();
                TestServices.WindowHelper.WindowContent = rootPanel;
            });
            TestServices.WindowHelper.WaitForIdle();

            Log.Comment("Focus Button0");
            FocusHelper.EnsureFocus(button0, FocusState.Keyboard);

            using (var focusEventOrderTester = new FocusEventOrderingTester(elementList, eventOrder))
            {
                Log.Comment("Injecting shift-tab");
                TestServices.KeyboardHelper.ShiftTab();

                TestServices.WindowHelper.WaitForIdle();
                Verify.AreEqual("[button0LosingFocus:1][FocusManagerLosingFocus][FocusManagerGettingFocus]" +
                    "[FocusManagerLosingFocus:2][button1GettingFocus][FocusManagerGettingFocus]" +
                    "[button0LostFocus][FocusManagerLostFocus:1][FocusManagerGotFocus]" +
                    "[FocusManagerLostFocus:2][button1GotFocus][FocusManagerGotFocus]", eventOrder.ToString());
            }
        }

        [TestMethod]
        [TestProperty("Description", "Verifies cancelling a focus change does not cause NoFocusCandidateFound to fire.")]
        public void NoFocusCandidateFoundNotRaisedWhenFocusChangeCancelled()
        {
            const string rootPanelXaml =
                    @"<StackPanel xmlns='http://schemas.microsoft.com/winfx/2006/xaml/presentation' xmlns:x='http://schemas.microsoft.com/winfx/2006/xaml'>
                            <StackPanel Orientation='Horizontal'>
                                  <Button x:Name='button0' Content='Button0' Background='Green' />
                                  <Button x:Name='button1' Content='Button1' Background='Red'/>
                            </StackPanel>
                        </StackPanel>";

            StackPanel rootPanel = null;
            Button button1 = null;
            Button button0 = null;
            FrameworkElement[] elementList = null;
            StringBuilder eventOrder = null;

            var button1GettingFocusHandler = new Action<object, GettingFocusEventArgs>((source, args) =>
            {
                VerifyGettingFocusEventArgParameters(args, button0, button1, FocusState.Keyboard, FocusNavigationDirection.Right, false /*Handled*/, FocusInputDeviceKind.GameController, false /*Cancel*/);
                Log.Comment("Cancelling Focus change.");
                args.Cancel = true;
            });

            UIExecutor.Execute(() =>
            {
                rootPanel = (StackPanel)XamlReader.Load(rootPanelXaml);
                button1 = (Button)rootPanel.FindName("button1");
                button0 = (Button)rootPanel.FindName("button0");
                elementList = new FrameworkElement[2] { button1, button0 };
                eventOrder = new StringBuilder();
                TestServices.WindowHelper.WindowContent = rootPanel;
            });

            TestServices.WindowHelper.WaitForIdle();
            Log.Comment("Focus Button0");
            FocusHelper.EnsureFocus(button0, FocusState.Keyboard);

            using (var focusEventOrderTester = new FocusEventOrderingTester(elementList, eventOrder))
            using (var button1GettingFocus = new EventTester<Button, GettingFocusEventArgs>(button1, "GettingFocus", button1GettingFocusHandler))
            {
                Log.Comment("Injecting Tab");
                TestServices.KeyboardHelper.GamepadDpadRight();

                button1GettingFocus.Wait();
                TestServices.WindowHelper.WaitForIdle();
                UIExecutor.Execute(() =>
                {
                    Verify.IsTrue(button1GettingFocus.HasFired);
                    Verify.AreEqual("[button0LosingFocus:1][FocusManagerLosingFocus][button1GettingFocus][FocusManagerGettingFocus:Canceled]", eventOrder.ToString());
                });
            }
        }

        [TestMethod]
        [TestProperty("Description", "Verifies that NoFocusCandidateFound bubbles even when handled when using AddHanlder.")]
        public void VerifyParentStillGetsHandledNoFocusCandidateFoundEventWhenUsingAddHandler()
        {
            const string rootPanelXaml =
                    @"<StackPanel xmlns='http://schemas.microsoft.com/winfx/2006/xaml/presentation' xmlns:x='http://schemas.microsoft.com/winfx/2006/xaml'>
                            <StackPanel Orientation='Horizontal' x:Name='stackPanel1'>
                                  <Button x:Name='button0' Content='Button0' Background='Green'/>
                                  <Button x:Name='button1' Content='Button1' Background='Red'/>
                                  <Button x:Name='button2' Content='Button2' Background='Yellow'/>
                            </StackPanel>
                        </StackPanel>";

            StackPanel rootPanel = null;
            Button button0 = null;
            Button button1 = null;
            Button button2 = null;
            StackPanel stackPanel1 = null;

            var button1NoFocusCandidateFoundEventArgs = new Action<object, NoFocusCandidateFoundEventArgs>((source, args) =>
            {
                Log.Comment("Button1 NoFocusCandidateFound");
                VerifyNoFocusCandidateFoundEventArgParameters(args, button1, FocusNavigationDirection.Up, false /*Handled*/, FocusInputDeviceKind.GameController);
                args.Handled = true;
            });

            var stackPanel1NoFocusCandidateFoundEventArgs = new Action<object, NoFocusCandidateFoundEventArgs>((source, args) =>
            {
                Log.Comment("StackPanel1 NoFocusCandidateFound");
                VerifyNoFocusCandidateFoundEventArgParameters(args, button1, FocusNavigationDirection.Up, true /*Handled*/, FocusInputDeviceKind.GameController);
            });

            UIExecutor.Execute(() =>
            {
                rootPanel = (StackPanel)XamlReader.Load(rootPanelXaml);
                button0 = (Button)rootPanel.FindName("button0");
                button1 = (Button)rootPanel.FindName("button1");
                button2 = (Button)rootPanel.FindName("button2");
                stackPanel1 = (StackPanel)rootPanel.FindName("stackPanel1");
                TestServices.WindowHelper.WindowContent = rootPanel;
            });

            TestServices.WindowHelper.WaitForIdle();
            Log.Comment("Focus Button1");
            FocusHelper.EnsureFocus(button1, FocusState.Keyboard);

            using (var stackPanel1NoFocusCandidateFound = EventTester<UIElement, NoFocusCandidateFoundEventArgs>.FromRoutedEvent(stackPanel1, "NoFocusCandidateFound", stackPanel1NoFocusCandidateFoundEventArgs))
            using (var button1NoFocusCandidateFound = EventTester<UIElement, NoFocusCandidateFoundEventArgs>.FromRoutedEvent(button1, "NoFocusCandidateFound", button1NoFocusCandidateFoundEventArgs))
            {
                Log.Comment("Injecting GameController Up");
                TestServices.KeyboardHelper.GamepadDpadUp();
                stackPanel1NoFocusCandidateFound.Wait();

                Verify.IsTrue(button1NoFocusCandidateFound.HasFired);
                Verify.IsTrue(stackPanel1NoFocusCandidateFound.HasFired);
            }

            TestServices.WindowHelper.WaitForIdle();

        }

        [TestMethod]
        [TestProperty("Description", "Verifies that Control.Focus can be called in a NoFocusCandidateFound event handler.")]
        public void VerifyFocusCanBeCalledInNoFocusCandidateFoundHandlder()
        {
            const string rootPanelXaml =
                    @"<StackPanel xmlns='http://schemas.microsoft.com/winfx/2006/xaml/presentation' xmlns:x='http://schemas.microsoft.com/winfx/2006/xaml'>
                            <StackPanel Orientation='Horizontal' >
                                  <Button x:Name='button0' Content='Button0' Background='Green'/>
                                  <Button x:Name='button1' Content='Button1' Background='Red'/>
                                  <Button x:Name='button2' Content='Button2' Background='Yellow'/>
                            </StackPanel>
                        </StackPanel>";

            StackPanel rootPanel = null;
            Button button0 = null;
            Button button1 = null;
            Button button2 = null;

            FrameworkElement[] elementList = null;
            StringBuilder eventOrder = null;

            var button1NoFocusCandidateFoundEventArgs = new Action<object, NoFocusCandidateFoundEventArgs>((source, args) =>
            {
                VerifyNoFocusCandidateFoundEventArgParameters(args, button1, FocusNavigationDirection.Up, false /*Handled*/, FocusInputDeviceKind.GameController);
                button2.Focus(FocusState.Pointer);
            });


            UIExecutor.Execute(() =>
            {
                rootPanel = (StackPanel)XamlReader.Load(rootPanelXaml);
                button0 = (Button)rootPanel.FindName("button0");
                button1 = (Button)rootPanel.FindName("button1");
                button2 = (Button)rootPanel.FindName("button2");

                elementList = new FrameworkElement[3] { button1, button2, button0 };
                eventOrder = new StringBuilder();
                TestServices.WindowHelper.WindowContent = rootPanel;
            });

            TestServices.WindowHelper.WaitForIdle();
            Log.Comment("Focus Button1");
            FocusHelper.EnsureFocus(button1, FocusState.Keyboard);

            using (var focusEventOrderTester = new FocusEventOrderingTester(elementList, eventOrder))
            using (var button1NoFocusCandidateFound = EventTester<UIElement, NoFocusCandidateFoundEventArgs>.FromRoutedEvent(button1, "NoFocusCandidateFound", button1NoFocusCandidateFoundEventArgs))
            using (var button2GotFocus = new EventTester<Button, RoutedEventArgs>(button2, "GotFocus"))
            {
                Log.Comment("Injecting GameController Up");
                TestServices.KeyboardHelper.GamepadDpadUp();

                button2GotFocus.Wait();

                TestServices.WindowHelper.WaitForIdle();
                Verify.AreEqual("[button1NoFocusCandidateFound][button1LosingFocus:1][FocusManagerLosingFocus][button2GettingFocus][FocusManagerGettingFocus][button1LostFocus][FocusManagerLostFocus][button2GotFocus][FocusManagerGotFocus]", eventOrder.ToString());
            }

        }

        [TestMethod]
        [TestProperty("Description", "Verifies Keyboard XYFocus can raise NoFocusCandidateFound")]
        [TestProperty("Hosting:Mode", "UAP")]  // This breaks for WPF mode. New test VerifyKeyboardOrGamePadXYFocusCanCauseNoFocusCandidateRaise has been created for gamecontroller input in WPF mode
        public void VerifyKeyboardXYFocusCanCauseNoFocusCandidateRaise()
        {
            const string rootPanelXaml =
                    @"<StackPanel xmlns='http://schemas.microsoft.com/winfx/2006/xaml/presentation' xmlns:x='http://schemas.microsoft.com/winfx/2006/xaml'>
                            <StackPanel Orientation='Vertical' XYFocusKeyboardNavigation='Enabled'>
                                  <Button x:Name='button0' Content='Button0' Background='Green'/>
                                  <Button x:Name='button1' Content='Button1' Background='Red'/>
                                  <Button x:Name='button2' Content='Button2' Background='Yellow'/>
                            </StackPanel>
                        </StackPanel>";

            StackPanel rootPanel = null;
            Button button0 = null;
            Button button1 = null;
            Button button2 = null;

            FrameworkElement[] elementList = null;
            StringBuilder eventOrderLeft = null;
            StringBuilder eventOrderRight = null;

            var button1NoFocusCandidateFoundEventArgsLeft = new Action<object, NoFocusCandidateFoundEventArgs>((source, args) =>
            {
                VerifyNoFocusCandidateFoundEventArgParameters(args, button1, FocusNavigationDirection.Left, false /*Handled*/, FocusInputDeviceKind.Keyboard);

            });

            var button1NoFocusCandidateFoundEventArgsRight = new Action<object, NoFocusCandidateFoundEventArgs>((source, args) =>
            {
                VerifyNoFocusCandidateFoundEventArgParameters(args, button1, FocusNavigationDirection.Right, false /*Handled*/, FocusInputDeviceKind.Keyboard);
            });

            UIExecutor.Execute(() =>
            {
                rootPanel = (StackPanel)XamlReader.Load(rootPanelXaml);
                button0 = (Button)rootPanel.FindName("button0");
                button1 = (Button)rootPanel.FindName("button1");
                button2 = (Button)rootPanel.FindName("button2");

                elementList = new FrameworkElement[3] { button1, button2, button0 };
                eventOrderLeft = new StringBuilder();
                eventOrderRight = new StringBuilder();
                TestServices.WindowHelper.WindowContent = rootPanel;
            });

            TestServices.WindowHelper.WaitForIdle();
            Log.Comment("Focus Button1");
            FocusHelper.EnsureFocus(button1, FocusState.Keyboard);

            using (var focusEventOrderTester = new FocusEventOrderingTester(elementList, eventOrderLeft))
            using (var button1NoFocusCandidateFound = new EventTester<Button, NoFocusCandidateFoundEventArgs>(button1, "NoFocusCandidateFound", button1NoFocusCandidateFoundEventArgsLeft))
            {
                Log.Comment("Injecting Keyboard Left");
                TestServices.KeyboardHelper.Left();
                button1NoFocusCandidateFound.Wait();

                UIExecutor.Execute(() =>
                {
                    Verify.AreEqual("[button1NoFocusCandidateFound]", eventOrderLeft.ToString());
                });
            }

            TestServices.WindowHelper.WaitForIdle();

            using (var focusEventOrderTester = new FocusEventOrderingTester(elementList, eventOrderRight))
            using (var button1NoFocusCandidateFound = new EventTester<Button, NoFocusCandidateFoundEventArgs>(button1, "NoFocusCandidateFound", button1NoFocusCandidateFoundEventArgsRight))
            {
                Log.Comment("Injecting Keyboard Right");
                TestServices.KeyboardHelper.Right();
                button1NoFocusCandidateFound.Wait();

                UIExecutor.Execute(() =>
                {
                    Verify.AreEqual("[button1NoFocusCandidateFound]", eventOrderRight.ToString());
                });
            }
        }

        [TestMethod]
        [TestProperty("Description", "Verifies Keyboard or GamePad XYFocus can raise NoFocusCandidateFound")]
        [TestProperty("Hosting:Mode", "WPF")] // In WPF host, input cannot be distinguished between Gamepad and Keyboard. This check passes because -hostingMode UAP is passed by default in test infra, but not locally
        [TestProperty("Ignore", "TRUE")]   // Focus engagement bugs in lifted islands
        public void VerifyKeyboardOrGamePadXYFocusCanCauseNoFocusCandidateRaise()
        {
            const string rootPanelXaml =
                    @"<StackPanel xmlns='http://schemas.microsoft.com/winfx/2006/xaml/presentation' xmlns:x='http://schemas.microsoft.com/winfx/2006/xaml'>
                            <StackPanel Orientation='Vertical' XYFocusKeyboardNavigation='Enabled'>
                                  <Button x:Name='button0' Content='Button0' Background='Green'/>
                                  <Button x:Name='button1' Content='Button1' Background='Red'/>
                                  <Button x:Name='button2' Content='Button2' Background='Yellow'/>
                            </StackPanel>
                        </StackPanel>";

            StackPanel rootPanel = null;
            Button button0 = null;
            Button button1 = null;
            Button button2 = null;

            FrameworkElement[] elementList = null;
            StringBuilder eventOrderLeft = null;
            StringBuilder eventOrderRight = null;

            var button1NoFocusCandidateFoundEventArgsLeft = new Action<object, NoFocusCandidateFoundEventArgs>((source, args) =>
            {
                VerifyNoFocusCandidateFoundEventArgParameters(args, button1, FocusNavigationDirection.Left, false /*Handled*/, FocusInputDeviceKind.GameController);

            });

            var button1NoFocusCandidateFoundEventArgsRight = new Action<object, NoFocusCandidateFoundEventArgs>((source, args) =>
            {
                VerifyNoFocusCandidateFoundEventArgParameters(args, button1, FocusNavigationDirection.Right, false /*Handled*/, FocusInputDeviceKind.GameController);
            });

            UIExecutor.Execute(() =>
            {
                rootPanel = (StackPanel)XamlReader.Load(rootPanelXaml);
                button0 = (Button)rootPanel.FindName("button0");
                button1 = (Button)rootPanel.FindName("button1");
                button2 = (Button)rootPanel.FindName("button2");

                elementList = new FrameworkElement[3] { button1, button2, button0 };
                eventOrderLeft = new StringBuilder();
                eventOrderRight = new StringBuilder();
                TestServices.WindowHelper.WindowContent = rootPanel;
            });

            TestServices.WindowHelper.WaitForIdle();
            Log.Comment("Focus Button1");
            FocusHelper.EnsureFocus(button1, FocusState.Keyboard);

            using (var focusEventOrderTester = new FocusEventOrderingTester(elementList, eventOrderLeft))
            using (var button1NoFocusCandidateFound = new EventTester<Button, NoFocusCandidateFoundEventArgs>(button1, "NoFocusCandidateFound", button1NoFocusCandidateFoundEventArgsLeft))
            {
                Log.Comment("Injecting Keyboard Left");
                TestServices.KeyboardHelper.Left();
                button1NoFocusCandidateFound.Wait();

                UIExecutor.Execute(() =>
                {
                    Verify.AreEqual("[button1NoFocusCandidateFound]", eventOrderLeft.ToString());
                });
            }

            TestServices.WindowHelper.WaitForIdle();

            using (var focusEventOrderTester = new FocusEventOrderingTester(elementList, eventOrderRight))
            using (var button1NoFocusCandidateFound = new EventTester<Button, NoFocusCandidateFoundEventArgs>(button1, "NoFocusCandidateFound", button1NoFocusCandidateFoundEventArgsRight))
            {
                Log.Comment("Injecting Keyboard Right");
                TestServices.KeyboardHelper.Right();
                button1NoFocusCandidateFound.Wait();

                UIExecutor.Execute(() =>
                {
                    Verify.AreEqual("[button1NoFocusCandidateFound]", eventOrderRight.ToString());
                });
            }
        }

        [TestMethod]
        [TestProperty("Description", "Verifies Keyboard XYFocus can only raise NoFocusCandidateFound once.")]
        [TestProperty("Hosting:Mode", "UAP")] // This breaks for WPF mode. New test VerifyKeyboardOrGamePadXYFocusCanCauseNoFocusCandidateRaise has been created for gamecontroller input in WPF mode
        public void VerifyKeyboardXYFocusOnlyRaisesNoFocusCandidateFoundOnce()
        {
            const string rootPanelXaml =
                    @"<StackPanel xmlns='http://schemas.microsoft.com/winfx/2006/xaml/presentation' xmlns:x='http://schemas.microsoft.com/winfx/2006/xaml'  XYFocusKeyboardNavigation='Disabled' x:Name='rootPanel'>
                            <StackPanel Orientation='Vertical' XYFocusKeyboardNavigation='Enabled'>
                            <StackPanel Orientation='Vertical' XYFocusKeyboardNavigation='Disabled'>
                            <StackPanel Orientation='Vertical' XYFocusKeyboardNavigation='Enabled'>
                                  <Button x:Name='button0' Content='Button0' Background='Green'/>
                                  <Button x:Name='button1' Content='Button1' Background='Red'/>
                                  <Button x:Name='button2' Content='Button2' Background='Yellow'/>
                            </StackPanel>
                            </StackPanel>
                            </StackPanel>
                        </StackPanel>";

            StackPanel rootPanel = null;
            Button button0 = null;
            Button button1 = null;
            Button button2 = null;

            FrameworkElement[] elementList = null;
            StringBuilder eventOrderLeft = null;
            StringBuilder eventOrderRight = null;

            var button1NoFocusCandidateFoundEventArgsLeft = new Action<object, NoFocusCandidateFoundEventArgs>((source, args) =>
            {
                VerifyNoFocusCandidateFoundEventArgParameters(args, button1, FocusNavigationDirection.Left, false /*Handled*/, FocusInputDeviceKind.Keyboard);
            });

            var button1NoFocusCandidateFoundEventArgsRight = new Action<object, NoFocusCandidateFoundEventArgs>((source, args) =>
            {
                VerifyNoFocusCandidateFoundEventArgParameters(args, button1, FocusNavigationDirection.Right, false /*Handled*/, FocusInputDeviceKind.Keyboard);
            });

            UIExecutor.Execute(() =>
            {
                rootPanel = (StackPanel)XamlReader.Load(rootPanelXaml);
                button0 = (Button)rootPanel.FindName("button0");
                button1 = (Button)rootPanel.FindName("button1");
                button2 = (Button)rootPanel.FindName("button2");

                elementList = new FrameworkElement[4] { button1, button2, button0, rootPanel };
                eventOrderLeft = new StringBuilder();
                eventOrderRight = new StringBuilder();
                TestServices.WindowHelper.WindowContent = rootPanel;
            });

            TestServices.WindowHelper.WaitForIdle();
            Log.Comment("Focus Button1");
            FocusHelper.EnsureFocus(button1, FocusState.Keyboard);

            using (var focusEventOrderTester = new FocusEventOrderingTester(elementList, eventOrderLeft))
            using (var button1NoFocusCandidateFound = new EventTester<Button, NoFocusCandidateFoundEventArgs>(button1, "NoFocusCandidateFound", button1NoFocusCandidateFoundEventArgsLeft))
            using (var rootPanelNoFocusCandidateFound = new EventTester<StackPanel, NoFocusCandidateFoundEventArgs>(rootPanel, "NoFocusCandidateFound"))
            {
                Log.Comment("Injecting Keyboard Left");
                TestServices.KeyboardHelper.Left();
                rootPanelNoFocusCandidateFound.Wait();
                TestServices.WindowHelper.WaitForIdle();
                UIExecutor.Execute(() =>
                {
                    Verify.AreEqual("[button1NoFocusCandidateFound][rootPanelNoFocusCandidateFound]", eventOrderLeft.ToString());
                });
            }
        }

        [TestMethod]
        [TestProperty("Description", "Verifies Keyboard XYFocus does not raise NoFocusCandidateFound when traversing between Enabled regions.")]
        public void VerifyKeyboardXYFocusDoesNotRaiseNoFocusCandidateFoundWhenTraversingXYFocusEnabledRegions()
        {
            const string rootPanelXaml =
                    @"<StackPanel xmlns='http://schemas.microsoft.com/winfx/2006/xaml/presentation' xmlns:x='http://schemas.microsoft.com/winfx/2006/xaml' x:Name='rootPanel'>
                            <StackPanel Orientation='Horizontal' XYFocusKeyboardNavigation='Enabled'>
                                <StackPanel Orientation='Vertical' XYFocusKeyboardNavigation='Enabled'>
                                      <Button x:Name='button0' Content='Button0' Background='Green'/>
                                      <Button x:Name='button1' Content='Button1' Background='Red'/>
                                      <Button x:Name='button2' Content='Button2' Background='Yellow'/>
                                </StackPanel>
                                <StackPanel Orientation='Vertical' XYFocusKeyboardNavigation='Enabled'>
                                     <Button x:Name='button3' Content='Button3' Background='Green'/>
                                     <Button x:Name='button4' Content='Button4' Background='Red'/>
                                     <Button x:Name='button5' Content='Button5' Background='Yellow'/>
                                </StackPanel>
                            </StackPanel>
                        </StackPanel>";

            StackPanel rootPanel = null;
            Button button0 = null;
            Button button1 = null;
            Button button2 = null;
            Button button3 = null;
            Button button4 = null;
            Button button5 = null;

            FrameworkElement[] elementList = null;

            StringBuilder eventOrder = null;

            UIExecutor.Execute(() =>
            {
                rootPanel = (StackPanel)XamlReader.Load(rootPanelXaml);
                button0 = (Button)rootPanel.FindName("button0");
                button1 = (Button)rootPanel.FindName("button1");
                button2 = (Button)rootPanel.FindName("button2");
                button3 = (Button)rootPanel.FindName("button3");
                button4 = (Button)rootPanel.FindName("button4");
                button5 = (Button)rootPanel.FindName("button5");

                elementList = new FrameworkElement[7] { button0, button1, button2, button5, button4, button5, rootPanel };
                eventOrder = new StringBuilder();
                TestServices.WindowHelper.WindowContent = rootPanel;
            });

            TestServices.WindowHelper.WaitForIdle();
            Log.Comment("Focus Button1");
            FocusHelper.EnsureFocus(button1, FocusState.Keyboard);

            using (var focusEventOrderTester = new FocusEventOrderingTester(elementList, eventOrder))
            using (var rootPanelGotFocus = new EventTester<StackPanel, RoutedEventArgs>(rootPanel, "GotFocus"))
            {
                Log.Comment("Injecting Keyboard Right");
                TestServices.KeyboardHelper.Right();

                rootPanelGotFocus.Wait();

                TestServices.WindowHelper.WaitForIdle();
                Verify.AreEqual("[button1LosingFocus:1][rootPanelLosingFocus][FocusManagerLosingFocus][button4GettingFocus][rootPanelGettingFocus][FocusManagerGettingFocus][button1LostFocus][rootPanelLostFocus][FocusManagerLostFocus][button4GotFocus][rootPanelGotFocus][FocusManagerGotFocus]", eventOrder.ToString());
            }
        }

        [TestMethod]
        [TestProperty("Description", "Verifies that NoFocusCandidateFound can fires on an engaged element.")]
        [TestProperty("Hosting:Mode", "UAP")]
        public void VerifyNoFocusCandidateFoundFiredOnEngagement()
        {
            const string rootPanelXaml =
                    @"<StackPanel Orientation='Vertical' xmlns='http://schemas.microsoft.com/winfx/2006/xaml/presentation' xmlns:x='http://schemas.microsoft.com/winfx/2006/xaml'>
                            <Button Width='50' x:Name='button0' Content='Button 0'/>
                            <UserControl x:Name='engagedControl' IsFocusEngagementEnabled='true'>
                                <StackPanel>
                                    <Button Width='50' x:Name='button1' Content='Button 1'/>
                                    <Button Width='50' x:Name='button2' Content='Button 2'/>
                                </StackPanel>
                            </UserControl>
                            <Button Width='50' x:Name='button3' Content='Button 3'/>
                    </StackPanel>";

            StackPanel rootPanel = null;
            Button button0 = null;
            Button button1 = null;
            Button button2 = null;
            Button button3 = null;
            UserControl engagedControl = null;

            FrameworkElement[] elementList = null;
            StringBuilder eventOrder = null;

            var button2NoFocusCandidateFoundHandler = new Action<object, NoFocusCandidateFoundEventArgs>((source, args) =>
            {
                VerifyNoFocusCandidateFoundEventArgParameters(args, button2, FocusNavigationDirection.Down, false /*Handled*/, FocusInputDeviceKind.GameController);
            });

            UIExecutor.Execute(() =>
            {
                rootPanel = (StackPanel)XamlReader.Load(rootPanelXaml);
                button0 = (Button)rootPanel.FindName("button0");
                button1 = (Button)rootPanel.FindName("button1");
                button2 = (Button)rootPanel.FindName("button2");
                button3 = (Button)rootPanel.FindName("button3");
                engagedControl = (UserControl)rootPanel.FindName("engagedControl");
                TestServices.WindowHelper.WindowContent = rootPanel;
                elementList = new FrameworkElement[5] {button0, button1, button2, button3, engagedControl };
                eventOrder = new StringBuilder();

            });

            TestServices.WindowHelper.WaitForIdle();

            Log.Comment("Focus button0");
            FocusHelper.EnsureFocus(button0, FocusState.Keyboard);

            using (var focusEngaged = new EventTester<UserControl, FocusEngagedEventArgs>(engagedControl, "FocusEngaged"))
            using (var button1GotFocus = new EventTester<Button, RoutedEventArgs>(button1, "GotFocus"))
            using (var button2GotFocus = new EventTester<Button, RoutedEventArgs>(button2, "GotFocus"))
            {
                Log.Comment("Injecting Gamepad Down");
                TestServices.KeyboardHelper.GamepadDpadDown();

                Log.Comment("Injecting Gamepad A");
                TestServices.KeyboardHelper.GamepadA();
                focusEngaged.Wait();
                Log.Comment("Focus engaged.");
                TestServices.KeyboardHelper.GamepadDpadDown();
                button2GotFocus.Wait();
                Log.Comment("Button2 GotFocus.");
            }

            TestServices.WindowHelper.WaitForIdle();

            UIExecutor.Execute(() =>
            {
                Verify.IsTrue(engagedControl.IsFocusEngaged);
            });

            using (var focusEventOrderTester = new FocusEventOrderingTester(elementList, eventOrder))
            using (var button2NoFocusCandidateFound = new EventTester<Button, NoFocusCandidateFoundEventArgs>(button2, "NoFocusCandidateFound", button2NoFocusCandidateFoundHandler))
            {
                Log.Comment("Injecting Gamepad Down");
                TestServices.KeyboardHelper.GamepadDpadDown();

                button2NoFocusCandidateFound.Wait();

                UIExecutor.Execute(() =>
                {
                    Verify.IsTrue(engagedControl.IsFocusEngaged);
                    Verify.AreEqual(FocusManager.GetFocusedElement(TestServices.WindowHelper.WindowContent.XamlRoot), button2);
                    Verify.AreEqual("[button2NoFocusCandidateFound][engagedControlNoFocusCandidateFound]", eventOrder.ToString());
                });
            }
        }

        [TestMethod]
        [TestProperty("Description", "Verifies that Window activation and deactivation raise Changing Focus events.")]
        [TestProperty("Hosting:Mode", "UAP")]  // To be enabled after per root xamlRoot and focus manager have been implemented
        public void VerifyWindowDeactivationAndActivationRaiseChangingFocusEvents()
        {
            const string rootPanelXaml =
                    @"<StackPanel Orientation='Vertical' xmlns='http://schemas.microsoft.com/winfx/2006/xaml/presentation' xmlns:x='http://schemas.microsoft.com/winfx/2006/xaml'>
                            <Button Width='50' x:Name='button1' Content='Button 1'/>
                            <Button Width='50' x:Name='button2' Content='Button 2'/>
                    </StackPanel>";

            StackPanel rootPanel = null;
            Button button1 = null;
            Button button2 = null;
            FrameworkElement[] elementList = null;
            StringBuilder eventOrder = null;
            XamlRoot xamlRoot = null;

            var button2GettingFocusHandler = new Action<object, GettingFocusEventArgs>((source, args) =>
            {
                VerifyGettingFocusEventArgParameters(args, null, button2, FocusState.Keyboard, FocusNavigationDirection.None, false /*Handled*/, FocusInputDeviceKind.Keyboard, false /*Cancel*/);
            });

            var button2LosingFocusHandler = new Action<object, LosingFocusEventArgs>((source, args) =>
            {
                VerifyLosingFocusEventArgParameters(args, button2, null, FocusState.Unfocused, FocusNavigationDirection.None, false /*Handled*/, FocusInputDeviceKind.Keyboard, false /*Cancel*/);
            });

            UIExecutor.Execute(() =>
            {
                rootPanel = (StackPanel)XamlReader.Load(rootPanelXaml);
                button1 = (Button)rootPanel.FindName("button1");
                button2 = (Button)rootPanel.FindName("button2");
                elementList = new FrameworkElement[2] { button1, button2};
                eventOrder = new StringBuilder();
                TestServices.WindowHelper.WindowContent = rootPanel;
            });

            TestServices.WindowHelper.WaitForIdle();

            UIExecutor.Execute(() =>
            {
                xamlRoot = rootPanel.XamlRoot;
            });

            Log.Comment("Focus button 1");
            FocusHelper.EnsureFocus(button1, FocusState.Keyboard);

            using (var button2GotFocus = new EventTester<Button, RoutedEventArgs>(button2, "GotFocus"))
            {
                Log.Comment("Injecting tab to focus button2.");
                TestServices.KeyboardHelper.Tab();
                button2GotFocus.Wait();
            }

            using (var focusEventOrderTester = new FocusEventOrderingTester(elementList, eventOrder))
            using (var gettingFocusEventTester = new EventTester<UIElement, GettingFocusEventArgs>(button2, "GettingFocus", button2GettingFocusHandler))
            using (var losingFocusEventTester = new EventTester<UIElement, LosingFocusEventArgs>(button2, "LosingFocus", button2LosingFocusHandler))
            using (var button2LostFocus = new EventTester<Button, RoutedEventArgs>(button2, "LostFocus"))
            using (var button2GotFocus = new EventTester<Button, RoutedEventArgs>(button2, "GotFocus"))
            {
                Log.Comment("Simulate app deactivate...");
                // Simulate app deactivation
                TestServices.WindowHelper.InjectWindowMessage(WM_KILLFOCUS, 0, 0, xamlRoot);
                TestServices.WindowHelper.InjectWindowMessage(WM_ACTIVATE, WA_INACTIVE, 0, xamlRoot);
                TestServices.WindowHelper.WaitForIdle();
                losingFocusEventTester.Wait();
                button2LostFocus.Wait();

                Log.Comment("Simulate app resume...");
                // Simulate app resume
                TestServices.WindowHelper.InjectWindowMessage(WM_ACTIVATE, WA_ACTIVE, 0, xamlRoot);
                TestServices.WindowHelper.InjectWindowMessage(WM_SETFOCUS, 0, 0, xamlRoot);
                TestServices.WindowHelper.WaitForIdle();
                gettingFocusEventTester.Wait();
                button2GotFocus.Wait();
                UIExecutor.Execute(() =>
                {
                    Verify.AreEqual(FocusManager.GetFocusedElement(TestServices.WindowHelper.WindowContent.XamlRoot), button2);
                });

                TestServices.WindowHelper.WaitForIdle();
                Verify.AreEqual("[button2LosingFocus:1][FocusManagerLosingFocus][FocusManagerGettingFocus][button2LostFocus][FocusManagerLostFocus][FocusManagerGotFocus]" +
                                "[FocusManagerLosingFocus:2][button2GettingFocus][FocusManagerGettingFocus][button2GotFocus][FocusManagerLostFocus][FocusManagerGotFocus]",
                    eventOrder.ToString());
            }
        }

        [TestMethod]
        [TestProperty("Description", "Verifies that Changing Focus events during Window activation and deactivation cannot be cancelled or redirected.")]
        [TestProperty("Hosting:Mode", "UAP")] // To be enabled after per root xamlRoot and focus manager have been implemented
        public void VerifyChangingFocusEventsDuringWindowDeactivationAndActivationCannotBeCancelled()
        {
            const string rootPanelXaml =
                    @"<StackPanel Orientation='Vertical' xmlns='http://schemas.microsoft.com/winfx/2006/xaml/presentation' xmlns:x='http://schemas.microsoft.com/winfx/2006/xaml'>
                            <Button Width='50' x:Name='button1' Content='Button 1'/>
                            <Button Width='50' x:Name='button2' Content='Button 2'/>
                    </StackPanel>";

            StackPanel rootPanel = null;
            Button button1 = null;
            Button button2 = null;
            FrameworkElement[] elementList = null;
            StringBuilder eventOrder = null;
            XamlRoot xamlRoot = null;

            var button2GettingFocusHandler = new Action<object, GettingFocusEventArgs>((source, args) =>
            {
                try
                {
                    VerifyGettingFocusEventArgParameters(args, null, button2, FocusState.Keyboard, FocusNavigationDirection.None, false /*Handled*/, FocusInputDeviceKind.Keyboard, false /*Cancel*/);
                    args.Cancel = true;
                    Verify.Fail("Exception was not thrown on attempted Focus change cancel.");
                }
                catch (Exception ex)
                {
                    Verify.AreEqual(ex.HResult, E_INVALID_ARG);
                    Log.Comment("Exception thrown");
                }
            });

            var button2LosingFocusHandler = new Action<object, LosingFocusEventArgs>((source, args) =>
            {
                try
                {
                    VerifyLosingFocusEventArgParameters(args, button2, null, FocusState.Unfocused, FocusNavigationDirection.None, false /*Handled*/, FocusInputDeviceKind.Keyboard, false /*Cancel*/);
                    args.NewFocusedElement = button1;
                    Verify.Fail("Exception was not thrown on attempted Focus redirection.");
                }
                catch (Exception ex)
                {
                    Verify.AreEqual(ex.HResult, E_INVALID_ARG);
                    Log.Comment("Exception thrown");
                }
            });

            UIExecutor.Execute(() =>
            {
                rootPanel = (StackPanel)XamlReader.Load(rootPanelXaml);
                button1 = (Button)rootPanel.FindName("button1");
                button2 = (Button)rootPanel.FindName("button2");
                elementList = new FrameworkElement[2] { button1, button2 };
                eventOrder = new StringBuilder();
                TestServices.WindowHelper.WindowContent = rootPanel;
            });

            TestServices.WindowHelper.WaitForIdle();

            UIExecutor.Execute(() =>
            {
                xamlRoot = rootPanel.XamlRoot;
            });

            Log.Comment("Focus button 1");
            FocusHelper.EnsureFocus(button1, FocusState.Keyboard);

            using (var button2GotFocus = new EventTester<Button, RoutedEventArgs>(button2, "GotFocus"))
            {
                Log.Comment("Injecting tab to focus button2.");
                TestServices.KeyboardHelper.Tab();
                button2GotFocus.Wait();
            }

            using (var focusEventOrderTester = new FocusEventOrderingTester(elementList, eventOrder))
            using (var gettingFocusEventTester = new EventTester<UIElement, GettingFocusEventArgs>(button2, "GettingFocus", button2GettingFocusHandler))
            using (var losingFocusEventTester = new EventTester<UIElement, LosingFocusEventArgs>(button2, "LosingFocus", button2LosingFocusHandler))
            using (var button2LostFocus = new EventTester<Button, RoutedEventArgs>(button2, "LostFocus"))
            using (var button2GotFocus = new EventTester<Button, RoutedEventArgs>(button2, "GotFocus"))
            {
                Log.Comment("Simulate app deactivate...");
                // Simulate app deactivation
                TestServices.WindowHelper.InjectWindowMessage(WM_KILLFOCUS, 0, 0, xamlRoot);
                TestServices.WindowHelper.InjectWindowMessage(WM_ACTIVATE, WA_INACTIVE, 0, xamlRoot);
                TestServices.WindowHelper.WaitForIdle();
                losingFocusEventTester.Wait();
                button2LostFocus.Wait();

                Log.Comment("Simulate app resume...");
                // Simulate app resume
                TestServices.WindowHelper.InjectWindowMessage(WM_ACTIVATE, WA_ACTIVE, 0, xamlRoot);
                TestServices.WindowHelper.InjectWindowMessage(WM_SETFOCUS, 0, 0, xamlRoot);
                TestServices.WindowHelper.WaitForIdle();
                gettingFocusEventTester.Wait();
                button2GotFocus.Wait();
                UIExecutor.Execute(() =>
                {
                    Verify.AreEqual(FocusManager.GetFocusedElement(TestServices.WindowHelper.WindowContent.XamlRoot), button2);
                    Verify.IsTrue(losingFocusEventTester.HasFired);
                    Verify.IsTrue(gettingFocusEventTester.HasFired);
                });

                TestServices.WindowHelper.WaitForIdle();
                Verify.AreEqual("[button2LosingFocus:1][FocusManagerLosingFocus][FocusManagerGettingFocus][button2LostFocus][FocusManagerLostFocus][FocusManagerGotFocus]" +
                                "[FocusManagerLosingFocus:2][button2GettingFocus][FocusManagerGettingFocus][button2GotFocus][FocusManagerLostFocus][FocusManagerGotFocus]",
                    eventOrder.ToString());
            }
        }

        [TestMethod]
        [TestProperty("Description", "Verifies that focus can change during window activation when focus moved internally")]
        [TestProperty("Hosting:Mode", "UAP")] // To be enabled after per root xamlRoot and focus manager have been implemented
        public void VerifyFocusChangeAfterInternalFocusMoveDuringWindowActivation()
        {
            const string rootPanelXaml =
                    @"<StackPanel Orientation='Vertical' xmlns='http://schemas.microsoft.com/winfx/2006/xaml/presentation' xmlns:x='http://schemas.microsoft.com/winfx/2006/xaml'>
                            <Button Width='50' x:Name='button1' Content='Button 1'/>
                            <Button Width='50' x:Name='button2' Content='Button 2'/>
                    </StackPanel>";

            StackPanel rootPanel = null;
            Button button1 = null;
            Button button2 = null;
            FrameworkElement[] elementList = null;
            StringBuilder eventOrder = null;
            XamlRoot xamlRoot = null;

            var button1GettingFocusHandler = new Action<object, GettingFocusEventArgs>((source, args) =>
            {
                VerifyGettingFocusEventArgParameters(args, null, button1, FocusState.Keyboard, FocusNavigationDirection.None, false /*Handled*/, FocusInputDeviceKind.Keyboard, false /*Cancel*/);
                button1.Visibility = Visibility.Collapsed;
            });

            UIExecutor.Execute(() =>
            {
                rootPanel = (StackPanel)XamlReader.Load(rootPanelXaml);
                button1 = (Button)rootPanel.FindName("button1");
                button2 = (Button)rootPanel.FindName("button2");
                elementList = new FrameworkElement[2] { button1, button2 };
                eventOrder = new StringBuilder();
                TestServices.WindowHelper.WindowContent = rootPanel;
            });

            TestServices.WindowHelper.WaitForIdle();

            UIExecutor.Execute(() =>
            {
                xamlRoot = rootPanel.XamlRoot;
            });

            Log.Comment("Focus button 1");
            FocusHelper.EnsureFocus(button1, FocusState.Keyboard);

             // Ensure last input device is keyboard.
            TestServices.WindowHelper.SetLastInputMethod(global::Private.Infrastructure.LastInputDeviceType.Keyboard, xamlRoot);

            using (var focusEventOrderTester = new FocusEventOrderingTester(elementList, eventOrder))
            using (var gettingFocusEventTester = new EventTester<UIElement, GettingFocusEventArgs>(button1, "GettingFocus", button1GettingFocusHandler))
            using (var button1LostFocus = new EventTester<Button, RoutedEventArgs>(button1, "LostFocus"))
            using (var button2GotFocus = new EventTester<Button, RoutedEventArgs>(button2, "GotFocus"))
            {
                Log.Comment("Simulate app deactivate...");
                // Simulate app deactivation
                TestServices.WindowHelper.InjectWindowMessage(WM_KILLFOCUS, 0, 0, xamlRoot);
                TestServices.WindowHelper.InjectWindowMessage(WM_ACTIVATE, WA_INACTIVE, 0, xamlRoot);
                TestServices.WindowHelper.WaitForIdle();
                button1LostFocus.Wait();

                Log.Comment("Simulate app resume...");
                // Simulate app resume
                TestServices.WindowHelper.InjectWindowMessage(WM_ACTIVATE, WA_ACTIVE, 0, xamlRoot);
                TestServices.WindowHelper.InjectWindowMessage(WM_SETFOCUS, 0, 0, xamlRoot);
                TestServices.WindowHelper.WaitForIdle();
                gettingFocusEventTester.Wait();
                button2GotFocus.Wait();
                UIExecutor.Execute(() =>
                {
                    Verify.AreEqual(FocusManager.GetFocusedElement(TestServices.WindowHelper.WindowContent.XamlRoot), button2);
                    Verify.IsTrue(gettingFocusEventTester.HasFired);
                });

                TestServices.WindowHelper.WaitForIdle();
                Verify.AreEqual("[button1LosingFocus:1][FocusManagerLosingFocus][FocusManagerGettingFocus][button1LostFocus][FocusManagerLostFocus][FocusManagerGotFocus]" +
                                "[FocusManagerLosingFocus:2][button1GettingFocus]" +
                                "[button1LosingFocus:3][FocusManagerLosingFocus][button2GettingFocus][FocusManagerGettingFocus]" +
                                "[FocusManagerGettingFocus:2][button1LostFocus]" +
                                "[FocusManagerLostFocus:3][button2GotFocus][FocusManagerGotFocus]",
                    eventOrder.ToString());
            }
        }

        [TestMethod]
        [TestProperty("Description", "Verifies that Changing Focus events during Window activation and deactivation cannot be cancelled or redirected after an internal focus move")]
        [TestProperty("Hosting:Mode", "UAP")] // To be enabled after per root xamlRoot and focus manager have been implemented
        public void VerifyFocusCannotBeCanceledAfterInternalFocusMoveDuringWindowActivation()
        {
            const string rootPanelXaml =
                    @"<StackPanel Orientation='Vertical' xmlns='http://schemas.microsoft.com/winfx/2006/xaml/presentation' xmlns:x='http://schemas.microsoft.com/winfx/2006/xaml'>
                            <Button Width='50' x:Name='button1' Content='Button 1'/>
                            <Button Width='50' x:Name='button2' Content='Button 2'/>
                    </StackPanel>";

            StackPanel rootPanel = null;
            Button button1 = null;
            Button button2 = null;
            FrameworkElement[] elementList = null;
            StringBuilder eventOrder = null;
            XamlRoot xamlRoot = null;

            var button1GettingFocusHandler = new Action<object, GettingFocusEventArgs>((source, args) =>
            {
                VerifyGettingFocusEventArgParameters(args, null, button1, FocusState.Keyboard, FocusNavigationDirection.None, false /*Handled*/, FocusInputDeviceKind.None, false /*Cancel*/);
                button1.Visibility = Visibility.Collapsed;
            });

            var button2GettingFocusHandler = new Action<object, GettingFocusEventArgs>((source, args) =>
            {
                try
                {
                    VerifyGettingFocusEventArgParameters(args, button1, button2, FocusState.Keyboard, FocusNavigationDirection.None, false /*Handled*/, FocusInputDeviceKind.None, false /*Cancel*/);
                    args.NewFocusedElement = button1;
                }
                catch (Exception ex)
                {
                    Verify.AreEqual(ex.HResult, E_INVALID_ARG);
                    Log.Comment("Exception thrown");
                }
            });

            UIExecutor.Execute(() =>
            {
                rootPanel = (StackPanel)XamlReader.Load(rootPanelXaml);
                button1 = (Button)rootPanel.FindName("button1");
                button2 = (Button)rootPanel.FindName("button2");
                elementList = new FrameworkElement[2] { button1, button2 };
                eventOrder = new StringBuilder();
                TestServices.WindowHelper.WindowContent = rootPanel;
            });

            TestServices.WindowHelper.WaitForIdle();

            UIExecutor.Execute(() =>
            {
                xamlRoot = rootPanel.XamlRoot;
            });

            TestServices.WindowHelper.SetLastInputMethod(global::Private.Infrastructure.LastInputDeviceType.None, xamlRoot);

            Log.Comment("Focus button 1");
            FocusHelper.EnsureFocus(button1, FocusState.Keyboard);

            using (var focusEventOrderTester = new FocusEventOrderingTester(elementList, eventOrder))
            using (var gettingFocusEventTester = new EventTester<UIElement, GettingFocusEventArgs>(button1, "GettingFocus", button1GettingFocusHandler))
            using (var button2GettingFocusEventTester = new EventTester<UIElement, GettingFocusEventArgs>(button2, "GettingFocus", button2GettingFocusHandler))
            using (var button1LostFocus = new EventTester<Button, RoutedEventArgs>(button1, "LostFocus"))
            using (var button2GotFocus = new EventTester<Button, RoutedEventArgs>(button2, "GotFocus"))
            {
                Log.Comment("Simulate app deactivate...");
                // Simulate app deactivation
                TestServices.WindowHelper.InjectWindowMessage(WM_KILLFOCUS, 0, 0, xamlRoot);
                TestServices.WindowHelper.InjectWindowMessage(WM_ACTIVATE, WA_INACTIVE, 0, xamlRoot);
                TestServices.WindowHelper.WaitForIdle();
                button1LostFocus.Wait();

                Log.Comment("Simulate app resume...");
                // Simulate app resume
                TestServices.WindowHelper.InjectWindowMessage(WM_ACTIVATE, WA_ACTIVE, 0, xamlRoot);
                TestServices.WindowHelper.InjectWindowMessage(WM_SETFOCUS, 0, 0, xamlRoot);
                TestServices.WindowHelper.WaitForIdle();
                gettingFocusEventTester.Wait();
                button2GotFocus.Wait();
                UIExecutor.Execute(() =>
                {
                    Verify.AreEqual(FocusManager.GetFocusedElement(TestServices.WindowHelper.WindowContent.XamlRoot), button2);
                    Verify.IsTrue(gettingFocusEventTester.HasFired);
                });

                TestServices.WindowHelper.WaitForIdle();
                Verify.AreEqual("[button1LosingFocus:1][FocusManagerLosingFocus][FocusManagerGettingFocus][button1LostFocus][FocusManagerLostFocus][FocusManagerGotFocus]" +
                                "[FocusManagerLosingFocus:2][button1GettingFocus]" +
                                "[button1LosingFocus:3][FocusManagerLosingFocus][button2GettingFocus][FocusManagerGettingFocus]" +
                                "[FocusManagerGettingFocus:2][button1LostFocus]" +
                                "[FocusManagerLostFocus:3][button2GotFocus][FocusManagerGotFocus]",
                    eventOrder.ToString());
            }
        }

        [TestMethod]
        [TestProperty("Description", "Verifies that changing focus events send the correct navigation direction in scrollviewers.")]
        public void ChangingFocusSendsTheCorrectNavigationDirectionInAScrollViewer()
        {
            const string rootPanelXaml =
                    @"<StackPanel Orientation='Vertical' xmlns='http://schemas.microsoft.com/winfx/2006/xaml/presentation' xmlns:x='http://schemas.microsoft.com/winfx/2006/xaml'>
                        <ScrollViewer>
                        <StackPanel>
                            <Button Width='50' x:Name='button1' Content='Button 1'/>
                            <Button Width='50' x:Name='button2' Content='Button 2'/>
                        </StackPanel>
                        </ScrollViewer>
                    </StackPanel>";

            StackPanel rootPanel = null;
            Button button1 = null;
            Button button2 = null;

            FrameworkElement[] elementList = null;
            StringBuilder eventOrder = null;

            var button2GettingFocusHandler = new Action<object, GettingFocusEventArgs>((source, args) =>
            {
                VerifyGettingFocusEventArgParameters(args, button1, button2, FocusState.Keyboard, FocusNavigationDirection.Down, false /*Handled*/, FocusInputDeviceKind.GameController, false /*Cancel*/);
            });

            var button2LosingFocusHandler = new Action<object, LosingFocusEventArgs>((source, args) =>
            {
                VerifyLosingFocusEventArgParameters(args, button2, button1, FocusState.Keyboard, FocusNavigationDirection.Up, false /*Handled*/, FocusInputDeviceKind.GameController, false /*Cancel*/);
            });

            UIExecutor.Execute(() =>
            {
                rootPanel = (StackPanel)XamlReader.Load(rootPanelXaml);
                button1 = (Button)rootPanel.FindName("button1");
                button2 = (Button)rootPanel.FindName("button2");
                elementList = new FrameworkElement[2] { button1, button2};
                eventOrder = new StringBuilder();
                TestServices.WindowHelper.WindowContent = rootPanel;
            });

            TestServices.WindowHelper.WaitForIdle();

            Log.Comment("Focus button 1");
            FocusHelper.EnsureFocus(button1, FocusState.Keyboard);

            using (var focusEventOrderTester = new FocusEventOrderingTester(elementList, eventOrder))
            using (var gettingFocusEventTester = new EventTester<UIElement, GettingFocusEventArgs>(button2, "GettingFocus", button2GettingFocusHandler))
            using (var losingFocusEventTester = new EventTester<UIElement, LosingFocusEventArgs>(button2, "LosingFocus", button2LosingFocusHandler))
            using (var button2GotFocus = new EventTester<Button, RoutedEventArgs>(button2, "GotFocus"))
            using (var button1GotFocus = new EventTester<Button, RoutedEventArgs>(button1, "GotFocus"))
            {
                Log.Comment("Injecting Gamepad Down");
                TestServices.KeyboardHelper.GamepadDpadDown();

                button2GotFocus.Wait();

                Log.Comment("Injecting Gamepad Up");
                TestServices.KeyboardHelper.GamepadDpadUp();

                button1GotFocus.Wait();
                UIExecutor.Execute(() =>
                {
                    Verify.IsTrue(gettingFocusEventTester.HasFired);
                    Verify.IsTrue(losingFocusEventTester.HasFired);
                });

                TestServices.WindowHelper.WaitForIdle();
                Verify.AreEqual("[button1LosingFocus:1][FocusManagerLosingFocus][button2GettingFocus][FocusManagerGettingFocus][button1LostFocus][FocusManagerLostFocus][button2GotFocus][FocusManagerGotFocus]" +
                                "[button2LosingFocus:2][FocusManagerLosingFocus][button1GettingFocus][FocusManagerGettingFocus][button2LostFocus][FocusManagerLostFocus][button1GotFocus][FocusManagerGotFocus]",
                    eventOrder.ToString());
            }
        }

        [TestMethod]
        [TestProperty("Description", "Verifies that changing focus events send the correct navigation direction in a splitview.")]
        public void ChangingFocusSendsTheCorrectNavigationDirectionInASplitView()
        {
            const string rootPanelXaml =
                    @"<StackPanel Orientation='Vertical' xmlns='http://schemas.microsoft.com/winfx/2006/xaml/presentation' xmlns:x='http://schemas.microsoft.com/winfx/2006/xaml'>
                                <SplitView x:Name='splitview' IsPaneOpen='true'>
                                    <SplitView.Pane>
                                        <StackPanel>
                                        <Button x:Name='button1'> Button1 </Button>
                                        <Button x:Name='button2'> Button2 </Button>
                                        </StackPanel>
                                    </SplitView.Pane>
                                <StackPanel>
                                <Button x:Name='button3'> Button3 </Button>
                                </StackPanel>
                                </SplitView>
                        </StackPanel>";

            StackPanel rootPanel = null;
            Button button1 = null;
            Button button2 = null;

            FrameworkElement[] elementList = null;
            StringBuilder eventOrder = null;

            var button2GettingFocusHandler = new Action<object, GettingFocusEventArgs>((source, args) =>
            {
                VerifyGettingFocusEventArgParameters(args, button1, button2, FocusState.Keyboard, FocusNavigationDirection.Down, false /*Handled*/, FocusInputDeviceKind.GameController, false /*Cancel*/);
            });

            var button2LosingFocusHandler = new Action<object, LosingFocusEventArgs>((source, args) =>
            {
                VerifyLosingFocusEventArgParameters(args, button2, button1, FocusState.Keyboard, FocusNavigationDirection.Up, false /*Handled*/, FocusInputDeviceKind.GameController, false /*Cancel*/);
            });

            UIExecutor.Execute(() =>
            {
                rootPanel = (StackPanel)XamlReader.Load(rootPanelXaml);
                button1 = (Button)rootPanel.FindName("button1");
                button2 = (Button)rootPanel.FindName("button2");
                elementList = new FrameworkElement[2] { button1, button2 };
                eventOrder = new StringBuilder();
                TestServices.WindowHelper.WindowContent = rootPanel;
            });

            TestServices.WindowHelper.WaitForIdle();

            Log.Comment("Focus button 1");
            FocusHelper.EnsureFocus(button1, FocusState.Keyboard);

            using (var focusEventOrderTester = new FocusEventOrderingTester(elementList, eventOrder))
            using (var gettingFocusEventTester = new EventTester<UIElement, GettingFocusEventArgs>(button2, "GettingFocus", button2GettingFocusHandler))
            using (var losingFocusEventTester = new EventTester<UIElement, LosingFocusEventArgs>(button2, "LosingFocus", button2LosingFocusHandler))
            using (var button2GotFocus = new EventTester<Button, RoutedEventArgs>(button2, "GotFocus"))
            using (var button1GotFocus = new EventTester<Button, RoutedEventArgs>(button1, "GotFocus"))
            {
                Log.Comment("Injecting Gamepad Down");
                TestServices.KeyboardHelper.GamepadDpadDown();

                button2GotFocus.Wait();

                Log.Comment("Injecting Gamepad Up");
                TestServices.KeyboardHelper.GamepadDpadUp();

                button1GotFocus.Wait();
                UIExecutor.Execute(() =>
                {
                    Verify.IsTrue(gettingFocusEventTester.HasFired);
                    Verify.IsTrue(losingFocusEventTester.HasFired);
                });

                TestServices.WindowHelper.WaitForIdle();
                Verify.AreEqual("[button1LosingFocus:1][FocusManagerLosingFocus][button2GettingFocus][FocusManagerGettingFocus][button1LostFocus][FocusManagerLostFocus][button2GotFocus][FocusManagerGotFocus]" +
                                "[button2LosingFocus:2][FocusManagerLosingFocus][button1GettingFocus][FocusManagerGettingFocus][button2LostFocus][FocusManagerLostFocus][button1GotFocus][FocusManagerGotFocus]",
                    eventOrder.ToString());
            }
        }

        [TestMethod]
        [TestProperty("Description", "Verifies that changing focus events send the correct navigation direction in a listview.")]
        public void ChangingFocusSendsTheCorrectNavigationDirectionInAListView()
        {
            const string rootPanelXaml =
                    @"<StackPanel Orientation='Vertical' xmlns='http://schemas.microsoft.com/winfx/2006/xaml/presentation' xmlns:x='http://schemas.microsoft.com/winfx/2006/xaml'>
                        <ListView>
                            <ListViewItem x:Name='button1' Content='Button 1'/>
                            <ListViewItem x:Name='button2' Content='Button 2'/>
                        </ListView>
                    </StackPanel>";

            StackPanel rootPanel = null;
            ListViewItem button1 = null;
            ListViewItem button2 = null;

            FrameworkElement[] elementList = null;
            StringBuilder eventOrder = null;

            var button2GettingFocusHandler = new Action<object, GettingFocusEventArgs>((source, args) =>
            {
                VerifyGettingFocusEventArgParameters(args, button1, button2, FocusState.Keyboard, FocusNavigationDirection.Down, false /*Handled*/, FocusInputDeviceKind.GameController, false /*Cancel*/);
            });

            var button2LosingFocusHandler = new Action<object, LosingFocusEventArgs>((source, args) =>
            {
                VerifyLosingFocusEventArgParameters(args, button2, button1, FocusState.Keyboard, FocusNavigationDirection.Up, false /*Handled*/, FocusInputDeviceKind.GameController, false /*Cancel*/);
            });

            UIExecutor.Execute(() =>
            {
                rootPanel = (StackPanel)XamlReader.Load(rootPanelXaml);
                button1 = (ListViewItem)rootPanel.FindName("button1");
                button2 = (ListViewItem)rootPanel.FindName("button2");
                elementList = new FrameworkElement[2] { button1, button2 };
                eventOrder = new StringBuilder();
                TestServices.WindowHelper.WindowContent = rootPanel;
            });

            TestServices.WindowHelper.WaitForIdle();

            Log.Comment("Focus button 1");
            FocusHelper.EnsureFocus(button1, FocusState.Keyboard);

            using (var focusEventOrderTester = new FocusEventOrderingTester(elementList, eventOrder))
            using (var gettingFocusEventTester = new EventTester<UIElement, GettingFocusEventArgs>(button2, "GettingFocus", button2GettingFocusHandler))
            using (var losingFocusEventTester = new EventTester<UIElement, LosingFocusEventArgs>(button2, "LosingFocus", button2LosingFocusHandler))
            using (var button2GotFocus = new EventTester<ListViewItem, RoutedEventArgs>(button2, "GotFocus"))
            using (var button1GotFocus = new EventTester<ListViewItem, RoutedEventArgs>(button1, "GotFocus"))
            {
                Log.Comment("Injecting Gamepad Down");
                TestServices.KeyboardHelper.GamepadDpadDown();

                button2GotFocus.Wait();

                Log.Comment("Injecting Gamepad Up");
                TestServices.KeyboardHelper.GamepadDpadUp();

                button1GotFocus.Wait();
                UIExecutor.Execute(() =>
                {
                    Verify.IsTrue(gettingFocusEventTester.HasFired);
                    Verify.IsTrue(losingFocusEventTester.HasFired);
                });

                TestServices.WindowHelper.WaitForIdle();
                Verify.AreEqual("[button1LosingFocus:1][FocusManagerLosingFocus][button2GettingFocus][FocusManagerGettingFocus][button1LostFocus][FocusManagerLostFocus][button2GotFocus][FocusManagerGotFocus]" +
                                "[button2LosingFocus:2][FocusManagerLosingFocus][button1GettingFocus][FocusManagerGettingFocus][button2LostFocus][FocusManagerLostFocus][button1GotFocus][FocusManagerGotFocus]",
                    eventOrder.ToString());
            }
        }

        [TestMethod]
        [TestProperty("Description", "Verifies focus redirection behavior for non focusable elements.")]
        public void VerifyChangingFocusRedirectionToANonFocusableElementBehavior()
        {
            const string rootPanelXaml =
                    @"<StackPanel Orientation='Vertical' xmlns='http://schemas.microsoft.com/winfx/2006/xaml/presentation' xmlns:x='http://schemas.microsoft.com/winfx/2006/xaml'>
                            <Button Width='50' x:Name='button1' Content='Button 1'/>
                            <Button Width='50' x:Name='button2' Content='Button 2'/>
                            <StackPanel x:Name='stackPanel'>
                                <Button Width='50' x:Name='button3' Content='Button 3'/>
                            </StackPanel>
                            <Button Width='50' x:Name='button4' Content='Button 4' IsTabStop='false'/>
                    </StackPanel>";

            StackPanel rootPanel = null;
            Button button1 = null;
            Button button2 = null;
            Button button3 = null;
            Button button4 = null;
            StackPanel stackPanel = null;

            FrameworkElement[] elementList = null;
            StringBuilder eventOrder1 = null;
            StringBuilder eventOrder2 = null;
            StringBuilder eventOrder3 = null;

            bool focusRedirected = false;

            var button2GettingFocusHandlerToStackPanel = new Action<object, GettingFocusEventArgs>((source, args) =>
            {
                VerifyGettingFocusEventArgParameters(args, button1, button2, FocusState.Keyboard, FocusNavigationDirection.Next, false /*Handled*/, FocusInputDeviceKind.Keyboard, false /*Cancel*/);
                Log.Comment("Redirecting Focus to StackPanel");
                args.NewFocusedElement = stackPanel;
            });

            var button2GettingFocusHandlerToButton4 = new Action<object, GettingFocusEventArgs>((source, args) =>
            {
                VerifyGettingFocusEventArgParameters(args, button1, button2, FocusState.Keyboard, FocusNavigationDirection.Next, false /*Handled*/, FocusInputDeviceKind.Keyboard, false /*Cancel*/);
                Log.Comment("Redirecting Focus to Button4");
                args.NewFocusedElement = button4;
            });

            var button1LosingFocusHandlerToStackPanel = new Action<object, LosingFocusEventArgs>((source, args) =>
            {
                if (!focusRedirected)
                {
                    VerifyLosingFocusEventArgParameters(args, button1, button2, FocusState.Keyboard, FocusNavigationDirection.Next, false /*Handled*/, FocusInputDeviceKind.Keyboard, false /*Cancel*/);
                    Log.Comment("Redirecting Focus to StackPanel");
                    args.NewFocusedElement = stackPanel;
                    focusRedirected = true;
                }
            });

            UIExecutor.Execute(() =>
            {
                rootPanel = (StackPanel)XamlReader.Load(rootPanelXaml);
                button1 = (Button)rootPanel.FindName("button1");
                button2 = (Button)rootPanel.FindName("button2");
                button3 = (Button)rootPanel.FindName("button3");
                button4 = (Button)rootPanel.FindName("button4");
                stackPanel = (StackPanel)rootPanel.FindName("stackPanel");
                elementList = new FrameworkElement[5] { button1, button2, button3, button4, stackPanel };
                eventOrder1 = new StringBuilder();
                eventOrder2 = new StringBuilder();
                eventOrder3 = new StringBuilder();
                TestServices.WindowHelper.WindowContent = rootPanel;
            });

            TestServices.WindowHelper.WaitForIdle();

            Log.Comment("Testing GettingFocus redirection to non-focusable element with focusable child.");
            Log.Comment("Focus button 1");
            FocusHelper.EnsureFocus(button1, FocusState.Keyboard);

            using (var focusEventOrderTester = new FocusEventOrderingTester(elementList, eventOrder1))
            using (var gettingFocusEventTester = new EventTester<UIElement, GettingFocusEventArgs>(button2, "GettingFocus", button2GettingFocusHandlerToStackPanel))
            using (var button3GotFocus = new EventTester<Button, RoutedEventArgs>(button3, "GotFocus"))
            {
                Log.Comment("Injecting tab");
                TestServices.KeyboardHelper.Tab();

                button3GotFocus.Wait();
                UIExecutor.Execute(() =>
                {
                    Verify.AreEqual(button3.FocusState, FocusState.Keyboard);
                    Verify.AreEqual(FocusManager.GetFocusedElement(TestServices.WindowHelper.WindowContent.XamlRoot), button3);
                });

                TestServices.WindowHelper.WaitForIdle();
                Verify.AreEqual("[button1LosingFocus:1][FocusManagerLosingFocus][button2GettingFocus][FocusManagerGettingFocus][button1LosingFocus][FocusManagerLosingFocus][button3GettingFocus][stackPanelGettingFocus][FocusManagerGettingFocus][button1LostFocus][FocusManagerLostFocus][button3GotFocus][stackPanelGotFocus][FocusManagerGotFocus]", eventOrder1.ToString());
            }

            Log.Comment("Testing LosingFocus redirection to non-focusable element with focusable child.");
            Log.Comment("Focus button 1");
            FocusHelper.EnsureFocus(button1, FocusState.Keyboard);

            using (var focusEventOrderTester = new FocusEventOrderingTester(elementList, eventOrder2))
            using (var gettingFocusEventTester = new EventTester<UIElement, LosingFocusEventArgs>(button1, "LosingFocus", button1LosingFocusHandlerToStackPanel))
            using (var button3GotFocus = new EventTester<Button, RoutedEventArgs>(button3, "GotFocus"))
            {
                Log.Comment("Injecting tab");
                TestServices.KeyboardHelper.Tab();

                button3GotFocus.Wait();
                UIExecutor.Execute(() =>
                {
                    Verify.AreEqual(button3.FocusState, FocusState.Keyboard);
                    Verify.AreEqual(FocusManager.GetFocusedElement(TestServices.WindowHelper.WindowContent.XamlRoot), button3);
                });

                TestServices.WindowHelper.WaitForIdle();
                Verify.AreEqual("[button1LosingFocus:1][FocusManagerLosingFocus][button1LosingFocus][FocusManagerLosingFocus][button3GettingFocus][stackPanelGettingFocus][FocusManagerGettingFocus][button1LostFocus][FocusManagerLostFocus][button3GotFocus][stackPanelGotFocus][FocusManagerGotFocus]", eventOrder2.ToString());
            }

            Log.Comment("Testing GetingFocus redirection to non-focusable element with no focusable child.");
            Log.Comment("Focus button 1");
            FocusHelper.EnsureFocus(button1, FocusState.Keyboard);

            using (var focusEventOrderTester = new FocusEventOrderingTester(elementList, eventOrder3))
            using (var gettingFocusEventTester = new EventTester<UIElement, GettingFocusEventArgs>(button2, "GettingFocus", button2GettingFocusHandlerToButton4))
            using (var button4GotFocus = new EventTester<Button, RoutedEventArgs>(button4, "GotFocus"))
            {
                Log.Comment("Injecting tab");
                TestServices.KeyboardHelper.Tab();

                button4GotFocus.WaitForNoThrow(TimeSpan.FromMilliseconds(100));
                TestServices.WindowHelper.WaitForIdle();
                UIExecutor.Execute(() =>
                {
                    Verify.AreEqual(button1.FocusState, FocusState.Keyboard);
                    Verify.AreEqual(FocusManager.GetFocusedElement(TestServices.WindowHelper.WindowContent.XamlRoot), button1);
                    Verify.AreEqual("[button1LosingFocus:1][FocusManagerLosingFocus][button2GettingFocus][FocusManagerGettingFocus]", eventOrder3.ToString());
                });
            }
        }

        [TestMethod]
        [TestProperty("Description", "Verifies the event ordering of OnPointerReleased and focus changing events.")]
        [TestProperty("TestPass:ExcludeOn", "WindowsCore")]
        public void VerifyFocusChangingAndOnPointerReleasedEventOrder()
        {
            const string rootPanelXaml =
                    @"<StackPanel Orientation='Horizontal' xmlns='http://schemas.microsoft.com/winfx/2006/xaml/presentation' xmlns:x='http://schemas.microsoft.com/winfx/2006/xaml'>
                                <Button Width='50' Height='50' x:Name='button1' Content='Button 1'/>
                                <Button Width='50' Height='50' x:Name='button2' Content='Button 2'/>
                        </StackPanel>";

            StackPanel rootPanel = null;
            Button button1 = null;
            Button button2 = null;

            FrameworkElement[] elementList = null;
            StringBuilder eventOrder1 = null;
            StringBuilder eventOrder2 = null;
            UIExecutor.Execute(() =>
            {
                rootPanel = (StackPanel)XamlReader.Load(rootPanelXaml);
                button1 = (Button)rootPanel.FindName("button1");
                button2 = (Button)rootPanel.FindName("button2");
                elementList = new FrameworkElement[2] { button1, button2 };
                eventOrder1 = new StringBuilder();
                eventOrder2 = new StringBuilder();
                TestServices.WindowHelper.WindowContent = rootPanel;
            });

            TestServices.WindowHelper.WaitForIdle();

            FocusHelper.EnsureFocus(button1, FocusState.Pointer);

            var button2PointerReleasedHandler = new Action<object, PointerRoutedEventArgs>((source, args) =>
            {
                FrameworkElement sourceAsFrameworkElement = (FrameworkElement)source;
                eventOrder1.Append("[" + sourceAsFrameworkElement.Name + "PointerReleased]");
            });

            var button1PointerReleasedHandler = new Action<object, PointerRoutedEventArgs>((source, args) =>
            {
                FrameworkElement sourceAsFrameworkElement = (FrameworkElement)source;
                eventOrder2.Append("[" + sourceAsFrameworkElement.Name + "PointerReleased]");
            });

            using (var focusEventOrderTester = new FocusEventOrderingTester(elementList, eventOrder1))
            using (var button2OnPointerReleased = EventTester<Button, PointerRoutedEventArgs>.FromRoutedEvent(button2, "PointerReleased", button2PointerReleasedHandler))
            using (var button2GotFocus = new EventTester<Button, RoutedEventArgs>(button2, "GotFocus"))
            {
                Log.Comment("Mouse click on button1");
                TestServices.InputHelper.LeftMouseClick(button2);
                TestServices.WindowHelper.WaitForIdle();

                button2GotFocus.Wait();
                button2OnPointerReleased.Wait();
                VerifySubstringOrder("[button1LosingFocus][button2GettingFocus]", "[button2PointerReleased]", eventOrder1.ToString());
            }

            using (var focusEventOrderTester = new FocusEventOrderingTester(elementList, eventOrder2))
            using (var button1OnPointerReleased = EventTester<Button, PointerRoutedEventArgs>.FromRoutedEvent(button1, "PointerReleased", button1PointerReleasedHandler))
            using (var button1GotFocus = new EventTester<Button, RoutedEventArgs>(button1, "GotFocus"))
            {
                Log.Comment("Tap on Button1");
                TestServices.InputHelper.Tap(button1);
                TestServices.WindowHelper.WaitForIdle();

                button1GotFocus.Wait();
                button1OnPointerReleased.Wait();
                VerifySubstringOrder("[button2LosingFocus][button1GettingFocus]", "[button1PointerReleased]", eventOrder2.ToString());
            }
        }

        [TestMethod]
        [TestProperty("Description", "Verifies the event ordering of OnPointerReleased and focus changing events.")]
        [TestProperty("TestPass:ExcludeOn", "WindowsCore")]
        public void VerifyFocusChangingAndOnPointerReleasedEventOrderForTextControls()
        {
            const string rootPanelXaml =
                    @"<StackPanel Orientation='Vertical' xmlns='http://schemas.microsoft.com/winfx/2006/xaml/presentation' xmlns:x='http://schemas.microsoft.com/winfx/2006/xaml'>
                                <RichEditBox Width='50' Height='50' x:Name='richEditBox1' />
                                <Button x:Name='btn1'>Button1</Button>
                                <RichEditBox Width='50' Height='50' x:Name='richEditBox2' />
                        </StackPanel>";

            StackPanel rootPanel = null;
            RichEditBox richEditBox1 = null;
            RichEditBox richEditBox2 = null;
            Button button1 = null;

            FrameworkElement[] elementList = null;
            StringBuilder eventOrder1 = null;
            StringBuilder eventOrder2 = null;
            UIExecutor.Execute(() =>
            {
                rootPanel = (StackPanel)XamlReader.Load(rootPanelXaml);
                richEditBox1 = (RichEditBox)rootPanel.FindName("richEditBox1");
                richEditBox2 = (RichEditBox)rootPanel.FindName("richEditBox2");
                button1 = (Button)rootPanel.FindName("btn1");
                elementList = new FrameworkElement[3] { richEditBox1, richEditBox2, button1 };
                eventOrder1 = new StringBuilder();
                eventOrder2 = new StringBuilder();
                TestServices.WindowHelper.WindowContent = rootPanel;
            });

            TestServices.WindowHelper.WaitForIdle();

            FocusHelper.EnsureFocus(richEditBox1, FocusState.Pointer);

            var richEditBox2PointerReleasedHandler = new Action<object, PointerRoutedEventArgs>((source, args) =>
            {
                FrameworkElement sourceAsFrameworkElement = (FrameworkElement)source;
                eventOrder1.Append("[" + sourceAsFrameworkElement.Name + "PointerReleased]");
            });

            var richEditBox1PointerReleasedHandler = new Action<object, PointerRoutedEventArgs>((source, args) =>
            {
                FrameworkElement sourceAsFrameworkElement = (FrameworkElement)source;
                eventOrder2.Append("[" + sourceAsFrameworkElement.Name + "PointerReleased]");
            });

            using (var focusEventOrderTester = new FocusEventOrderingTester(elementList, eventOrder1))
            using (var richEditBox2OnPointerReleased = EventTester<RichEditBox, PointerRoutedEventArgs>.FromRoutedEvent(richEditBox2, "PointerReleased", richEditBox2PointerReleasedHandler))
            using (var richEditBox2GotFocus = new EventTester<RichEditBox, RoutedEventArgs>(richEditBox2, "GotFocus"))
            {
                Log.Comment("Mouse click on richEditBox2");
                TestServices.InputHelper.LeftMouseClick(richEditBox2);
                TestServices.WindowHelper.WaitForIdle();

                richEditBox2GotFocus.Wait();
                richEditBox2OnPointerReleased.Wait();
                VerifySubstringOrder("[richEditBox1LosingFocus][richEditBox2GettingFocus]", "[richEditBox2PointerReleased]", eventOrder1.ToString());
            }

            using (var focusEventOrderTester = new FocusEventOrderingTester(elementList, eventOrder2))
            using (var richEditBox1OnPointerReleased = EventTester<RichEditBox, PointerRoutedEventArgs>.FromRoutedEvent(richEditBox1, "PointerReleased", richEditBox1PointerReleasedHandler))
            using (var richEditBox1GotFocus = new EventTester<RichEditBox, RoutedEventArgs>(richEditBox1, "GotFocus"))
            {
                Log.Comment("Tap on RichEditBox1");
                TestServices.InputHelper.Tap(richEditBox1);
                TestServices.WindowHelper.WaitForIdle();

                richEditBox1GotFocus.Wait();
                richEditBox1OnPointerReleased.Wait();
                VerifySubstringOrder("[richEditBox2LosingFocus][richEditBox1GettingFocus]", "[richEditBox1PointerReleased]", eventOrder2.ToString());
            }
        }


        [TestMethod]
        [TestProperty("Description", "Verifies the TryCancel behavior on changing focus events.")]
        [TestProperty("Hosting:Mode", "UAP")] // To be enabled after per root xamlRoot and focus manager have been implemented
        public void VerifyChangingFocusTryCancelBehavior()
        {
            const string rootPanelXaml =
                    @"<StackPanel Orientation='Vertical' xmlns='http://schemas.microsoft.com/winfx/2006/xaml/presentation' xmlns:x='http://schemas.microsoft.com/winfx/2006/xaml'>
                            <Button Width='50' x:Name='button1' Content='Button 1'/>
                            <Button Width='50' x:Name='button2' Content='Button 2'/>
                    </StackPanel>";

            StackPanel rootPanel = null;
            Button button1 = null;
            Button button2 = null;
            FrameworkElement[] elementList = null;
            StringBuilder eventOrder = null;
            XamlRoot xamlRoot = null;

            var button1GettingFocusHandlerOnTab = new Action<object, GettingFocusEventArgs>((source, args) =>
            {
                VerifyGettingFocusEventArgParameters(args, button2, button1, FocusState.Keyboard, FocusNavigationDirection.Previous, false /*Handled*/, FocusInputDeviceKind.Keyboard, false /*Cancel*/);
                Verify.IsTrue(args.TryCancel());
            });

            var button2LosingFocusHandlerOnTab = new Action<object, LosingFocusEventArgs>((source, args) =>
            {
                Log.Comment("Losing Focus on Tab.");
                VerifyLosingFocusEventArgParameters(args, button2, button1, FocusState.Keyboard, FocusNavigationDirection.Previous, false /*Handled*/, FocusInputDeviceKind.Keyboard, false /*Cancel*/);
                Verify.IsTrue(args.TryCancel());
            });

            var button2GettingFocusHandler = new Action<object, GettingFocusEventArgs>((source, args) =>
            {
                VerifyGettingFocusEventArgParameters(args, null, button2, FocusState.Keyboard, FocusNavigationDirection.None, false /*Handled*/, FocusInputDeviceKind.Keyboard, false /*Cancel*/);
                Verify.IsFalse(args.TryCancel());
            });

            var button2LosingFocusHandler = new Action<object, LosingFocusEventArgs>((source, args) =>
            {
                Log.Comment("Losing Focus on Window DeActivate.");
                VerifyLosingFocusEventArgParameters(args, button2, null, FocusState.Unfocused, FocusNavigationDirection.None, false /*Handled*/, FocusInputDeviceKind.Keyboard, false /*Cancel*/);
                Verify.IsFalse(args.TryCancel());
            });

            UIExecutor.Execute(() =>
            {
                rootPanel = (StackPanel)XamlReader.Load(rootPanelXaml);
                button1 = (Button)rootPanel.FindName("button1");
                button2 = (Button)rootPanel.FindName("button2");
                elementList = new FrameworkElement[2] { button1, button2 };
                eventOrder = new StringBuilder();
                TestServices.WindowHelper.WindowContent = rootPanel;
            });

            TestServices.WindowHelper.WaitForIdle();

            UIExecutor.Execute(() =>
            {
                xamlRoot = rootPanel.XamlRoot;
            });

            Log.Comment("Focus button 1");
            FocusHelper.EnsureFocus(button1, FocusState.Keyboard);

            using (var button2GotFocus = new EventTester<Button, RoutedEventArgs>(button2, "GotFocus"))
            {
                Log.Comment("Injecting tab to focus button2.");
                TestServices.KeyboardHelper.Tab();
                button2GotFocus.Wait();
            }

            using (var focusEventOrderTester = new FocusEventOrderingTester(elementList, eventOrder))
            using (var gettingFocusEventTester = new EventTester<UIElement, GettingFocusEventArgs>(button2, "GettingFocus", button2GettingFocusHandler))
            using (var losingFocusEventTester = new EventTester<UIElement, LosingFocusEventArgs>(button2, "LosingFocus", button2LosingFocusHandler))
            using (var button2LostFocus = new EventTester<Button, RoutedEventArgs>(button2, "LostFocus"))
            using (var button2GotFocus = new EventTester<Button, RoutedEventArgs>(button2, "GotFocus"))
            {
                Log.Comment("Simulate app deactivate...");
                // Simulate app deactivation
                TestServices.WindowHelper.InjectWindowMessage(WM_KILLFOCUS, 0, 0, xamlRoot);
                TestServices.WindowHelper.InjectWindowMessage(WM_ACTIVATE, WA_INACTIVE, 0, xamlRoot);
                TestServices.WindowHelper.WaitForIdle();
                losingFocusEventTester.Wait();
                button2LostFocus.Wait();

                Log.Comment("Simulate app resume...");
                // Simulate app resume
                TestServices.WindowHelper.InjectWindowMessage(WM_ACTIVATE, WA_ACTIVE, 0, xamlRoot);
                TestServices.WindowHelper.InjectWindowMessage(WM_SETFOCUS, 0, 0, xamlRoot);
                TestServices.WindowHelper.WaitForIdle();
                gettingFocusEventTester.Wait();
                button2GotFocus.Wait();
                UIExecutor.Execute(() =>
                {
                    Verify.AreEqual(FocusManager.GetFocusedElement(TestServices.WindowHelper.WindowContent.XamlRoot), button2);
                    Verify.IsTrue(losingFocusEventTester.HasFired);
                    Verify.IsTrue(gettingFocusEventTester.HasFired);
                });

                TestServices.WindowHelper.WaitForIdle();
                Verify.AreEqual("[button2LosingFocus:1][FocusManagerLosingFocus][FocusManagerGettingFocus][button2LostFocus][FocusManagerLostFocus][FocusManagerGotFocus]" +
                                "[FocusManagerLosingFocus:2][button2GettingFocus][FocusManagerGettingFocus][button2GotFocus][FocusManagerLostFocus][FocusManagerGotFocus]",
                    eventOrder.ToString());
            }

            TestServices.WindowHelper.WaitForIdle();

            using (var losingFocusEventTester = new EventTester<UIElement, LosingFocusEventArgs>(button2, "LosingFocus", button2LosingFocusHandlerOnTab))
            using (var button2LostFocus = new EventTester<Button, RoutedEventArgs>(button2, "LostFocus"))
            {
                Log.Comment("Injecting shift-tab");
                TestServices.KeyboardHelper.ShiftTab();
                losingFocusEventTester.Wait();

                UIExecutor.Execute(() =>
                {
                    Verify.AreEqual(FocusManager.GetFocusedElement(TestServices.WindowHelper.WindowContent.XamlRoot), button2);
                    Verify.IsFalse(button2LostFocus.HasFired);
                });
            }


            TestServices.WindowHelper.WaitForIdle();

            using (var gettingFocusEventTester = new EventTester<UIElement, GettingFocusEventArgs>(button1, "GettingFocus", button1GettingFocusHandlerOnTab))
            using (var button1GotFocus = new EventTester<Button, RoutedEventArgs>(button2, "GotFocus"))
            {
                Log.Comment("Injecting shift-tab");
                TestServices.KeyboardHelper.ShiftTab();
                gettingFocusEventTester.Wait();

                UIExecutor.Execute(() =>
                {
                    Verify.AreEqual(FocusManager.GetFocusedElement(TestServices.WindowHelper.WindowContent.XamlRoot), button2);
                    Verify.IsFalse(button1GotFocus.HasFired);
                });
                TestServices.WindowHelper.WaitForIdle();
            }
        }

        [TestMethod]
        [TestProperty("Description", "Verifies the TrySetNewFocusedElement behavior when focus is not redirectable.")]
        [TestProperty("Hosting:Mode", "UAP")] // To be enabled after per root xamlRoot and focus manager have been implemented
        public void VerifyChangingFocusTrySetNewFocusedElementBehaviorOnWindowActivation()
        {
            const string rootPanelXaml =
                    @"<StackPanel Orientation='Vertical' xmlns='http://schemas.microsoft.com/winfx/2006/xaml/presentation' xmlns:x='http://schemas.microsoft.com/winfx/2006/xaml'>
                            <Button Width='50' x:Name='button1' Content='Button 1'/>
                            <Button Width='50' x:Name='button2' Content='Button 2'/>
                            <Button Width='50' x:Name='button3' Content='Button 3'/>
                    </StackPanel>";

            StackPanel rootPanel = null;
            Button button1 = null;
            Button button2 = null;
            Button button3 = null;
            FrameworkElement[] elementList = null;
            StringBuilder eventOrder = null;
            XamlRoot xamlRoot = null;

            var button2GettingFocusHandlerOnWindowActivation = new Action<object, GettingFocusEventArgs>((source, args) =>
            {
                VerifyGettingFocusEventArgParameters(args, null, button2, FocusState.Keyboard, FocusNavigationDirection.None, false /*Handled*/, FocusInputDeviceKind.Keyboard, false /*Cancel*/);
                Verify.IsFalse(args.TrySetNewFocusedElement(button3));
            });

            var button2LosingFocusHandlerOnWindowDeactivation = new Action<object, LosingFocusEventArgs>((source, args) =>
            {
                VerifyLosingFocusEventArgParameters(args, button2, null, FocusState.Unfocused, FocusNavigationDirection.None, false /*Handled*/, FocusInputDeviceKind.Keyboard, false /*Cancel*/);
                Verify.IsFalse(args.TrySetNewFocusedElement(button3));
            });


            UIExecutor.Execute(() =>
            {
                rootPanel = (StackPanel)XamlReader.Load(rootPanelXaml);
                button1 = (Button)rootPanel.FindName("button1");
                button2 = (Button)rootPanel.FindName("button2");
                button3 = (Button)rootPanel.FindName("button3");
                elementList = new FrameworkElement[3] { button1, button2, button3 };
                eventOrder = new StringBuilder();
                TestServices.WindowHelper.WindowContent = rootPanel;
                xamlRoot = rootPanel.XamlRoot;
            });

            TestServices.WindowHelper.WaitForIdle();

            UIExecutor.Execute(() =>
            {
                xamlRoot = rootPanel.XamlRoot;
            });

            Log.Comment("Focus button 1");
            FocusHelper.EnsureFocus(button1, FocusState.Keyboard);

            using (var button2GotFocus = new EventTester<Button, RoutedEventArgs>(button2, "GotFocus"))
            {
                Log.Comment("Injecting tab to focus button2.");
                TestServices.KeyboardHelper.Tab();
                button2GotFocus.Wait();
            }

            using (var focusEventOrderTester = new FocusEventOrderingTester(elementList, eventOrder))
            using (var gettingFocusEventTester = new EventTester<UIElement, GettingFocusEventArgs>(button2, "GettingFocus", button2GettingFocusHandlerOnWindowActivation))
            using (var losingFocusEventTester = new EventTester<UIElement, LosingFocusEventArgs>(button2, "LosingFocus", button2LosingFocusHandlerOnWindowDeactivation))
            using (var button2LostFocus = new EventTester<Button, RoutedEventArgs>(button2, "LostFocus"))
            using (var button2GotFocus = new EventTester<Button, RoutedEventArgs>(button2, "GotFocus"))
            {
                Log.Comment("Simulate app deactivate...");
                TestServices.WindowHelper.InjectWindowMessage(WM_KILLFOCUS, 0, 0, xamlRoot);
                TestServices.WindowHelper.InjectWindowMessage(WM_ACTIVATE, WA_INACTIVE, 0, xamlRoot);
                TestServices.WindowHelper.WaitForIdle();
                losingFocusEventTester.Wait();
                button2LostFocus.Wait();

                Log.Comment("Simulate app resume...");
                TestServices.WindowHelper.InjectWindowMessage(WM_ACTIVATE, WA_ACTIVE, 0, xamlRoot);
                TestServices.WindowHelper.InjectWindowMessage(WM_SETFOCUS, 0, 0, xamlRoot);
                TestServices.WindowHelper.WaitForIdle();
                gettingFocusEventTester.Wait();
                button2GotFocus.Wait();
                UIExecutor.Execute(() =>
                {
                    Verify.AreEqual(FocusManager.GetFocusedElement(TestServices.WindowHelper.WindowContent.XamlRoot), button2);
                    Verify.IsTrue(losingFocusEventTester.HasFired);
                    Verify.IsTrue(gettingFocusEventTester.HasFired);
                });

                TestServices.WindowHelper.WaitForIdle();
                Verify.AreEqual("[button2LosingFocus:1][FocusManagerLosingFocus][FocusManagerGettingFocus][button2LostFocus][FocusManagerLostFocus][FocusManagerGotFocus]" +
                                "[FocusManagerLosingFocus:2][button2GettingFocus][FocusManagerGettingFocus][button2GotFocus][FocusManagerLostFocus][FocusManagerGotFocus]",
                    eventOrder.ToString());
            }
        }

        [TestMethod]
        [TestProperty("Description", "Verifies that focus can be redirected using the TrySetNewFocusedElement method")]
        public void VerifyChangingFocusTrySetNewFocusedElementBehavior()
        {
            const string rootPanelXaml =
                    @"<StackPanel Orientation='Vertical' xmlns='http://schemas.microsoft.com/winfx/2006/xaml/presentation' xmlns:x='http://schemas.microsoft.com/winfx/2006/xaml'>
                            <Button Width='50' x:Name='button1' Content='Button 1'/>
                            <Button Width='50' x:Name='button2' Content='Button 2'/>
                            <Button Width='50' x:Name='button3' Content='Button 3'/>
                    </StackPanel>";

            StackPanel rootPanel = null;
            Button button1 = null;
            Button button2 = null;
            Button button3 = null;
            FrameworkElement[] elementList = null;
            StringBuilder eventOrder = null;

            var button2GettingFocusHandler = new Action<object, GettingFocusEventArgs>((source, args) =>
            {
                VerifyGettingFocusEventArgParameters(args, button1, button2, FocusState.Keyboard, FocusNavigationDirection.Next, false /*Handled*/, FocusInputDeviceKind.Keyboard, false /*Cancel*/);
                Log.Comment("Redirecting Focus to Button 3");
                Verify.IsTrue(args.TrySetNewFocusedElement(button3));
            });

            UIExecutor.Execute(() =>
            {
                rootPanel = (StackPanel)XamlReader.Load(rootPanelXaml);
                button1 = (Button)rootPanel.FindName("button1");
                button2 = (Button)rootPanel.FindName("button2");
                button3 = (Button)rootPanel.FindName("button3");
                elementList = new FrameworkElement[3] { button1, button2, button3 };
                eventOrder = new StringBuilder();
                TestServices.WindowHelper.WindowContent = rootPanel;
            });

            TestServices.WindowHelper.WaitForIdle();

            Log.Comment("Focus button 1");
            FocusHelper.EnsureFocus(button1, FocusState.Keyboard);

            using (var focusEventOrderTester = new FocusEventOrderingTester(elementList, eventOrder))
            using (var gettingFocusEventTester = new EventTester<UIElement, GettingFocusEventArgs>(button2, "GettingFocus", button2GettingFocusHandler))
            using (var button3GotFocus = new EventTester<Button, RoutedEventArgs>(button3, "GotFocus"))
            {
                Log.Comment("Injecting tab");
                TestServices.KeyboardHelper.Tab();

                button3GotFocus.Wait();
                UIExecutor.Execute(() =>
                {
                    Verify.AreEqual(button3.FocusState, FocusState.Keyboard);
                    Verify.AreEqual(FocusManager.GetFocusedElement(TestServices.WindowHelper.WindowContent.XamlRoot), button3);
                });

                TestServices.WindowHelper.WaitForIdle();
                Verify.AreEqual("[button1LosingFocus:1][FocusManagerLosingFocus][button2GettingFocus][FocusManagerGettingFocus][button1LosingFocus][FocusManagerLosingFocus][button3GettingFocus][FocusManagerGettingFocus][button1LostFocus][FocusManagerLostFocus][button3GotFocus][FocusManagerGotFocus]", eventOrder.ToString());
            }
        }

        [TestMethod]
        [TestProperty("Description", "Verify events queing is working fine")]
        public void VerifyFocusManagerEventsSequence()
        {
            const string rootPanelXaml =
                @"<StackPanel Orientation='Vertical' xmlns='http://schemas.microsoft.com/winfx/2006/xaml/presentation' xmlns:x='http://schemas.microsoft.com/winfx/2006/xaml'>
                        <Button Width='50' x:Name='button1' Content='Button 1'/>
                        <Button Width='50' x:Name='button2' Content='Button 2'/>
                        <Button Width='50' x:Name='button3' Content='Button 3'/>
                        <Button Width='50' x:Name='button4' Content='Button 4'/>
                </StackPanel>";

            StackPanel rootPanel = null;
            Button button1 = null;
            Button button2 = null;
            Button button3 = null;

            FrameworkElement[] elementList = null;
            StringBuilder eventOrder = null;

            UIExecutor.Execute(() =>
            {
                rootPanel = (StackPanel)XamlReader.Load(rootPanelXaml);
                button1 = (Button)rootPanel.FindName("button1");
                button2 = (Button)rootPanel.FindName("button2");
                button3 = (Button)rootPanel.FindName("button3");
                elementList = new FrameworkElement[3] { button1, button2, button3 };

                eventOrder = new StringBuilder();
                TestServices.WindowHelper.WindowContent = rootPanel;
            });

            TestServices.WindowHelper.WaitForIdle();

            Log.Comment("Focus button 1");
            FocusHelper.EnsureFocus(button1, FocusState.Keyboard);

            using (var focusEventOrderTester = new FocusEventOrderingTester(elementList, eventOrder))
            {

                UIExecutor.Execute(() =>
                {
                    button2.Focus(FocusState.Keyboard);
                    button3.Focus(FocusState.Keyboard);
                });

                TestServices.WindowHelper.WaitForIdle();
                Verify.AreEqual("[button1LosingFocus:1][FocusManagerLosingFocus][button2GettingFocus][FocusManagerGettingFocus]" +
                                "[button2LosingFocus:2][FocusManagerLosingFocus][button3GettingFocus][FocusManagerGettingFocus]" +
                                "[button1LostFocus][FocusManagerLostFocus:1][button2GotFocus][FocusManagerGotFocus]" +
                                "[button2LostFocus][FocusManagerLostFocus:2][button3GotFocus][FocusManagerGotFocus]",
                                eventOrder.ToString());
            }
        }

        private void VerifyGettingFocusEventArgParameters(GettingFocusEventArgs args,
            DependencyObject oldFocusedElement,
            DependencyObject newFocusedElement,
            FocusState focusState,
            FocusNavigationDirection direction,
            bool handled,
            FocusInputDeviceKind inputDevice,
            bool cancel)
        {
            Verify.AreEqual(oldFocusedElement, args.OldFocusedElement);
            Verify.AreEqual(newFocusedElement, args.NewFocusedElement);
            Verify.AreEqual(focusState, args.FocusState);
            Verify.AreEqual(direction, args.Direction);
            Verify.AreEqual(handled, args.Handled);
            Verify.AreEqual(inputDevice, args.InputDevice);
            Verify.AreEqual(cancel, args.Cancel);
        }

        private void VerifyLosingFocusEventArgParameters(LosingFocusEventArgs args,
            DependencyObject oldFocusedElement,
            DependencyObject newFocusedElement,
            FocusState focusState,
            FocusNavigationDirection direction,
            bool handled,
            FocusInputDeviceKind inputDevice,
            bool cancel)
        {
            Verify.AreEqual(oldFocusedElement, args.OldFocusedElement);
            Verify.AreEqual(newFocusedElement, args.NewFocusedElement);
            Verify.AreEqual(focusState, args.FocusState);
            Verify.AreEqual(direction, args.Direction);
            Verify.AreEqual(handled, args.Handled);
            Verify.AreEqual(inputDevice, args.InputDevice);
            Verify.AreEqual(cancel, args.Cancel);
        }

        private void VerifyNoFocusCandidateFoundEventArgParameters(NoFocusCandidateFoundEventArgs args,
            DependencyObject originalSource,
            FocusNavigationDirection direction,
            bool handled,
            FocusInputDeviceKind inputDevice)
        {
            Verify.AreEqual(originalSource, args.OriginalSource);
            Verify.AreEqual(direction, args.Direction);
            Verify.AreEqual(handled, args.Handled);
            Verify.AreEqual(inputDevice, args.InputDevice);
        }

        //Verifies subString1 precedes substring2 in fillString (inclusive)
        private void VerifySubstringOrder(string subString1, string subString2, string fullString)
        {
            int substring1Index = fullString.IndexOf(subString1);
            int substring2Index = fullString.IndexOf(subString2);
            if(substring1Index > substring2Index)
            {
                Verify.Fail("\'" + subString1 + "\' does not precede \'" + subString2 + "\' in \'" + fullString + "\'");
            }
        }

        const long E_FOCUS_CHANGE_NOT_ALLOWED_DURING_FOCUS_CHANGING = -2144665568;
        const long E_INVALID_ARG = -2147024809;
        const uint WM_KILLFOCUS = 0x0008;
        const uint WM_ACTIVATE = 0x0006;
        const uint WM_SETFOCUS = 0x0007;
        const uint WA_INACTIVE = 0;
        const uint WA_ACTIVE = 1;
    }
}
