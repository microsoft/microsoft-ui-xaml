// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.
using System;
using System.Text;
using WEX.Logging.Interop;
using WEX.TestExecution;
using WEX.TestExecution.Markup;

using Private.Infrastructure;
using Microsoft.UI.Xaml.Tests.Common;

using Microsoft.UI.Xaml.Controls;
using Microsoft.UI.Xaml.Documents;
using Microsoft.UI.Xaml.Input;
using Windows.Foundation;
using Microsoft.UI.Xaml.Markup;

using System.Threading.Tasks;
using System.Collections.Generic;

namespace Microsoft.UI.Xaml.Tests.Focus
{
    [TestClass]
    public class FocusTests : XamlTestsBase
    {
        private enum FocusAsyncMethod
        {
            TryFocusAsync,
            TryMoveFocusAsync
        }

        private enum FocusElementType
        {
            Button,
            StackPanel,
            TextBlock,
            RichTextBlock
        }

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
        [TestProperty("Description", "Verify that we can query the FocusState of a hyperlink")]
        public void VerifyHyperlinkFocusState()
        {
            const string rootPanelXaml =
                    @"<StackPanel Orientation='Horizontal' XYFocusKeyboardNavigation='Enabled' xmlns='http://schemas.microsoft.com/winfx/2006/xaml/presentation' xmlns:x='http://schemas.microsoft.com/winfx/2006/xaml'>
                            <Button Width='50' x:Name='button' Content='Button Group 1'/>
                            <TextBlock><Hyperlink x:Name='hyperlink'>Hyperlink</Hyperlink></TextBlock>
                    </StackPanel>";

            StackPanel rootPanel = null;
            Button button = null;
            Hyperlink hyperlink = null;

            UIExecutor.Execute(() =>
            {
                rootPanel = (StackPanel)XamlReader.Load(rootPanelXaml);
                button = (Button)rootPanel.FindName("button");
                hyperlink = (Hyperlink)rootPanel.FindName("hyperlink");

                TestServices.WindowHelper.WindowContent = rootPanel;
            });

            TestServices.WindowHelper.WaitForIdle();
            FocusHelper.EnsureFocus(button, FocusState.Keyboard);

            using (var eventTester = new EventTester<Hyperlink, RoutedEventArgs>(hyperlink, "GotFocus"))
            {
                Log.Comment("Focus hyperlink");

                UIExecutor.Execute(() =>
                {
                    hyperlink.Focus(FocusState.Keyboard);
                });

                eventTester.Wait();

                Verify.IsTrue(eventTester.HasFired);

                UIExecutor.Execute(() =>
                {
                    Verify.IsTrue(hyperlink.FocusState == FocusState.Keyboard);
                });
            }
        }

        [TestMethod]
        [TestProperty("Description", "If we have a hyperlink that is currently focused, and we call Focus with a different focus state, verify the focus state changes")]
        [TestProperty("Hosting:Mode", "UAP")] // fails in WPF mode due to final release queue is not empty cleanup issue
        public void VerifyHyperlinkFocusStateChangesEvenAfterFocusingSameHyperlink()
        {
            const string rootPanelXaml =
                    @"<StackPanel Orientation='Horizontal' XYFocusKeyboardNavigation='Enabled' xmlns='http://schemas.microsoft.com/winfx/2006/xaml/presentation' xmlns:x='http://schemas.microsoft.com/winfx/2006/xaml'>
                            <Button Width='50' x:Name='button' Content='Button Group 1'/>
                            <TextBlock><Hyperlink x:Name='hyperlink'>Hyperlink</Hyperlink></TextBlock>
                    </StackPanel>";

            StackPanel rootPanel = null;
            Button button = null;
            Hyperlink hyperlink = null;

            UIExecutor.Execute(() =>
            {
                rootPanel = (StackPanel)XamlReader.Load(rootPanelXaml);
                button = (Button)rootPanel.FindName("button");
                hyperlink = (Hyperlink)rootPanel.FindName("hyperlink");

                TestServices.WindowHelper.WindowContent = rootPanel;
            });

            TestServices.WindowHelper.WaitForIdle();
            FocusHelper.EnsureFocus(button, FocusState.Keyboard);

            using (var eventTester = new EventTester<Hyperlink, RoutedEventArgs>(hyperlink, "GotFocus"))
            {
                Log.Comment("Focus hyperlink");

                UIExecutor.Execute(() =>
                {
                    hyperlink.Focus(FocusState.Keyboard);
                });

                eventTester.Wait();

                Verify.IsTrue(eventTester.HasFired);

                UIExecutor.Execute(() =>
                {
                    Verify.IsTrue(hyperlink.FocusState == FocusState.Keyboard);
                });
            }

            TestServices.WindowHelper.WaitForIdle();

            using (var eventTester = new EventTester<Hyperlink, RoutedEventArgs>(hyperlink, "GotFocus"))
            {
                Log.Comment("Focus hyperlink with Pointer FocusState");

                UIExecutor.Execute(() =>
                {
                    hyperlink.Focus(FocusState.Pointer);
                });

                eventTester.Wait();

                Verify.IsTrue(eventTester.HasFired);

                UIExecutor.Execute(() =>
                {
                    Verify.IsTrue(hyperlink.FocusState == FocusState.Pointer);
                });
            }
        }

        [TestMethod]
        [TestProperty("Description", "If we open a popup while a hyperlink is focused, the hyperlink should regain its focus and focus state when the popup is closed")]
        [TestProperty("TestPass:ExcludeOn", "WindowsCore")] // Escape key not working
        public void FocusStateShouldBeRestoredOnHyperlinkAfterPopupClose()
        {
            const string rootPanelXaml =
                    @"<StackPanel Orientation='Horizontal' XYFocusKeyboardNavigation='Enabled' xmlns='http://schemas.microsoft.com/winfx/2006/xaml/presentation' xmlns:x='http://schemas.microsoft.com/winfx/2006/xaml'>
                            <Button Width='50' x:Name='button' Content='Button Group 1'/>
                            <TextBlock><Hyperlink x:Name='hyperlink'>Hyperlink</Hyperlink></TextBlock>
                    </StackPanel>";

            StackPanel rootPanel = null;
            Button button = null;
            Hyperlink hyperlink = null;
            Flyout flyout = null;

            UIExecutor.Execute(() =>
            {
                rootPanel = (StackPanel)XamlReader.Load(rootPanelXaml);
                button = (Button)rootPanel.FindName("button");
                hyperlink = (Hyperlink)rootPanel.FindName("hyperlink");

                flyout = new Flyout();

                TestServices.WindowHelper.WindowContent = rootPanel;
            });

            TestServices.WindowHelper.WaitForIdle();
            FocusHelper.EnsureFocus(button, FocusState.Keyboard);

            using (var flyoutTester = new EventTester<Flyout, object>(flyout, "Opened"))
            using (var eventTester = new EventTester<Hyperlink, RoutedEventArgs>(hyperlink, "GotFocus"))
            {
                Log.Comment("Focus hyperlink");

                UIExecutor.Execute(() =>
                {
                    hyperlink.Focus(FocusState.Keyboard);
                    flyout.ShowAt(button);
                });

                eventTester.Wait();

                Log.Comment("Waiting for flyout to open");
                flyoutTester.Wait();

                UIExecutor.Execute(() =>
                {
                    Verify.IsTrue(hyperlink.FocusState == FocusState.Unfocused);
                });
            }

            TestServices.WindowHelper.WaitForIdle();

            using (var eventTesterHyperlink = new EventTester<Hyperlink, RoutedEventArgs>(hyperlink, "GotFocus"))
            using (var eventTester = new EventTester<Flyout, object>(flyout, "Closed"))
            {
                TestServices.KeyboardHelper.Escape();
                Log.Comment("Waiting for flyout to close");
                eventTester.Wait();

                eventTesterHyperlink.Wait();
                Log.Comment("Hyperlink regained focus");

                UIExecutor.Execute(() =>
                {
                    Verify.IsTrue(hyperlink.FocusState == FocusState.Keyboard);
                });

            }

            TestServices.WindowHelper.WaitForIdle();
        }

        [TestMethod]
        [TestProperty("Description", "When a hyperlink is focused and the visibility of one of it's parents is changed to collapse, we should move focus to the next element")]
        [TestProperty("Hosting:Mode", "UAP")] // fails in WPF mode due to final release queue is not empty cleanup issue
        public void FocusShouldBeMovedWhenParentOfFocusedHyperlinkCollapsed()
        {
            const string rootPanelXaml =
                    @"<StackPanel Orientation='Horizontal' XYFocusKeyboardNavigation='Enabled' xmlns='http://schemas.microsoft.com/winfx/2006/xaml/presentation' xmlns:x='http://schemas.microsoft.com/winfx/2006/xaml'>
                            <Button Width='50' x:Name='button' Content='Button Group 1'/>
                            <TextBlock x:Name='tb'><Hyperlink x:Name='hyperlink'>Hyperlink</Hyperlink></TextBlock>
                    </StackPanel>";

            StackPanel rootPanel = null;
            Button button = null;
            Hyperlink hyperlink = null;
            TextBlock textBlock = null;

            UIExecutor.Execute(() =>
            {
                rootPanel = (StackPanel)XamlReader.Load(rootPanelXaml);
                button = (Button)rootPanel.FindName("button");
                hyperlink = (Hyperlink)rootPanel.FindName("hyperlink");
                textBlock = (TextBlock)rootPanel.FindName("tb");

                TestServices.WindowHelper.WindowContent = rootPanel;
            });

            TestServices.WindowHelper.WaitForIdle();
            FocusHelper.EnsureFocus(hyperlink, FocusState.Keyboard);

            using (var eventTester = new EventTester<Button, RoutedEventArgs>(button, "GotFocus"))
            {
                Log.Comment("Collapsing TextBlock");

                UIExecutor.Execute(() =>
                {
                    textBlock.Visibility = Visibility.Collapsed;
                });

                eventTester.Wait();

                UIExecutor.Execute(() =>
                {
                    Verify.IsTrue(button.FocusState == FocusState.Keyboard);
                });
            }

            TestServices.WindowHelper.WaitForIdle();
        }

        [TestMethod]
        [TestProperty("Description", "Verify that we get the first focusable element")]
        [TestProperty("Hosting:Mode", "UAP")]
        public void VerifyFindFirstFocusableElement()
        {
            const string rootPanelXaml =
                    @"<StackPanel xmlns='http://schemas.microsoft.com/winfx/2006/xaml/presentation' xmlns:x='http://schemas.microsoft.com/winfx/2006/xaml'>
                            <Button Width='50' x:Name='button' Content='Button 1'/>
                            <StackPanel x:Name='sp'>
                                <Button Width='50' x:Name='button2' Content='Button 2'/>
                                <Button Width='50' Content='Button 3'/>
                            </StackPanel>
                    </StackPanel>";

            StackPanel rootPanel = null;
            StackPanel sp = null;
            Button button = null;
            Button button2 = null;

            UIExecutor.Execute(() =>
            {
                rootPanel = (StackPanel)XamlReader.Load(rootPanelXaml);
                sp = (StackPanel)rootPanel.FindName("sp");
                button = (Button)rootPanel.FindName("button");
                button2 = (Button)rootPanel.FindName("button2");

                TestServices.WindowHelper.WindowContent = rootPanel;
            });

            TestServices.WindowHelper.WaitForIdle();

            UIExecutor.Execute(() =>
            {
                Log.Comment("Finding first focusable element from root");
                Verify.IsTrue(button == FocusManager.FindFirstFocusableElement(null));

                Log.Comment("Finding first focusable element within a search root");
                Verify.IsTrue(button2 == FocusManager.FindFirstFocusableElement(sp));
            });

            TestServices.WindowHelper.WaitForIdle();
        }

        [TestMethod]
        [TestProperty("Description", "Verify that we get the last focusable element")]
        [TestProperty("Hosting:Mode", "UAP")]
        public void VerifyFindLastFocusableElement()
        {
            const string rootPanelXaml =
                    @"<StackPanel xmlns='http://schemas.microsoft.com/winfx/2006/xaml/presentation' xmlns:x='http://schemas.microsoft.com/winfx/2006/xaml'>
                            <Button Width='50' x:Name='button' Content='Button 1'/>
                            <StackPanel x:Name='sp'>
                                <Button Width='50' x:Name='button2' Content='Button 2'/>
                                <Button Width='50' x:Name='button3' Content='Button 3'/>
                            </StackPanel>
                            <Button Width='50' x:Name='button4' Content='Button 4'/>
                    </StackPanel>";

            StackPanel rootPanel = null;
            StackPanel sp = null;
            Button button3 = null;
            Button button4 = null;

            UIExecutor.Execute(() =>
            {
                rootPanel = (StackPanel)XamlReader.Load(rootPanelXaml);
                sp = (StackPanel)rootPanel.FindName("sp");
                button3 = (Button)rootPanel.FindName("button3");
                button4 = (Button)rootPanel.FindName("button4");

                TestServices.WindowHelper.WindowContent = rootPanel;
            });

            TestServices.WindowHelper.WaitForIdle();

            UIExecutor.Execute(() =>
            {
                Log.Comment("Finding last focusable element from root");
                Verify.IsTrue(button4 == FocusManager.FindLastFocusableElement(null));

                Log.Comment("Finding last focusable element within a search root");
                Verify.IsTrue(button3 == FocusManager.FindLastFocusableElement(sp));
            });

            TestServices.WindowHelper.WaitForIdle();
        }

        [TestMethod]
        public void VerifyCyclingWhenTabFocusNavigationSet()
        {
            const string rootPanelXaml =
                    @"<StackPanel xmlns='http://schemas.microsoft.com/winfx/2006/xaml/presentation' xmlns:x='http://schemas.microsoft.com/winfx/2006/xaml'>
                        <Button x:Name='btn' Height='50' Content='Button0'/>
                        <StackPanel TabFocusNavigation='Cycle'>
                            <StackPanel>
                                <Button x:Name='btnA' Content='Button0'/>
                                <Button x:Name='btnB' Content='Button1'/>
                                <Button x:Name='btnC' Content='Button2'/>
                            </StackPanel>
                            <Button x:Name='btnD' Content='Button3'/>
                        </StackPanel>
                    </StackPanel>";

            StackPanel rootPanel = null;
            Button button = null;
            Button buttonA = null;
            Button buttonB = null;
            Button buttonC = null;
            Button buttonD = null;

            UIExecutor.Execute(() =>
            {
                rootPanel = (StackPanel)XamlReader.Load(rootPanelXaml);
                button =  (Button)rootPanel.FindName("btn");
                buttonA = (Button)rootPanel.FindName("btnA");
                buttonB = (Button)rootPanel.FindName("btnB");
                buttonC = (Button)rootPanel.FindName("btnC");
                buttonD = (Button)rootPanel.FindName("btnD");

                TestServices.WindowHelper.WindowContent = rootPanel;
            });

            string eventOrder = "";

            Action<object, RoutedEventArgs> action = new Action<object, RoutedEventArgs>((s, e) =>
            {
                UIExecutor.Execute(() =>
                {
                    eventOrder = eventOrder + $"[{(s as FrameworkElement).Name}]";
                });
            });

            const string successString = "[btnA][btnB][btnC][btnD][btnA][btnD][btnC][btnB][btnA][btnD]";

            TestServices.WindowHelper.WaitForIdle();
            FocusHelper.EnsureFocus(button, FocusState.Keyboard);

            using (var eventTesterA = new EventTester<Button, RoutedEventArgs>(buttonA, "GotFocus", action))
            using (var eventTesterB = new EventTester<Button, RoutedEventArgs>(buttonB, "GotFocus", action))
            using (var eventTesterC = new EventTester<Button, RoutedEventArgs>(buttonC, "GotFocus", action))
            using (var eventTesterD = new EventTester<Button, RoutedEventArgs>(buttonD, "GotFocus", action))
            {
                const int NUM_TAB = 4;

                for (int count = 0; count < NUM_TAB + 1; count++)
                {
                    TestServices.KeyboardHelper.Tab();
                    TestServices.WindowHelper.WaitForIdle();
                }

                //Go in reverse order
                for (int count = NUM_TAB - 1; count >= 0; count--)
                {
                    TestServices.KeyboardHelper.ShiftTab();
                    TestServices.WindowHelper.WaitForIdle();
                }

                TestServices.KeyboardHelper.ShiftTab();
                TestServices.WindowHelper.WaitForIdle();

                Verify.AreEqual(successString, eventOrder);
            }
        }

        [TestMethod]
        [TestProperty("Hosting:Mode", "UAP")]   // Focus engagement bugs in lifted islands
        public void VerifyCyclingWithTabIndexWhenTabFocusNavigationSet()
        {
            const string rootPanelXaml =
                    @"<StackPanel xmlns='http://schemas.microsoft.com/winfx/2006/xaml/presentation' xmlns:x='http://schemas.microsoft.com/winfx/2006/xaml'>
                        <Button x:Name='btn' Height='50' Content='Button0'/>
                        <StackPanel TabFocusNavigation='Cycle'>
                            <StackPanel>
                                <Button x:Name='btnA' TabIndex='1' Content='Button0'/>
                                <Button x:Name='btnB' TabIndex='3' Content='Button1'/>
                                <Button x:Name='btnC' TabIndex='2' Content='Button2'/>
                            </StackPanel>
                            <Button x:Name='btnD' TabIndex='0' Content='Button3'/>
                        </StackPanel>
                    </StackPanel>";

            StackPanel rootPanel = null;
            Button button = null;
            Button buttonA = null;
            Button buttonB = null;
            Button buttonC = null;
            Button buttonD = null;

            UIExecutor.Execute(() =>
            {
                rootPanel = (StackPanel)XamlReader.Load(rootPanelXaml);
                button = (Button)rootPanel.FindName("btn");
                buttonA = (Button)rootPanel.FindName("btnA");
                buttonB = (Button)rootPanel.FindName("btnB");
                buttonC = (Button)rootPanel.FindName("btnC");
                buttonD = (Button)rootPanel.FindName("btnD");

                TestServices.WindowHelper.WindowContent = rootPanel;
            });

            string eventOrder = "";

            Action<object, RoutedEventArgs> action = new Action<object, RoutedEventArgs>((s, e) =>
            {
                UIExecutor.Execute(() =>
                {
                    eventOrder = eventOrder + $"[{(s as FrameworkElement).Name}]";
                });
            });

            const string successString = "[btnD][btnA][btnC][btnB][btnD][btnB][btnC][btnA][btnD][btnB]";

            FocusHelper.EnsureFocus(button, FocusState.Keyboard);
            TestServices.WindowHelper.WaitForIdle();

            using (var eventTesterA = new EventTester<Button, RoutedEventArgs>(buttonA, "GotFocus", action))
            using (var eventTesterB = new EventTester<Button, RoutedEventArgs>(buttonB, "GotFocus", action))
            using (var eventTesterC = new EventTester<Button, RoutedEventArgs>(buttonC, "GotFocus", action))
            using (var eventTesterD = new EventTester<Button, RoutedEventArgs>(buttonD, "GotFocus", action))
            {
                const int NUM_TAB = 4;

                for (int count = 0; count < NUM_TAB + 1; count++)
                {
                    TestServices.KeyboardHelper.Tab();
                    TestServices.WindowHelper.WaitForIdle();
                }

                //Go in reverse order
                for (int count = NUM_TAB - 1; count >= 0; count--)
                {
                    TestServices.KeyboardHelper.ShiftTab();
                    TestServices.WindowHelper.WaitForIdle();
                }

                TestServices.KeyboardHelper.ShiftTab();
                TestServices.WindowHelper.WaitForIdle();

                Verify.AreEqual(successString, eventOrder);
            }
        }

        [TestMethod]
        [TestProperty("Hosting:Mode", "UAP")]   // Focus engagement bugs in lifted islands
        public void VerifyShiftTabWhenOnceTabFocusNavigationSet()
        {
            const string rootPanelXaml =
                    @"<StackPanel xmlns='http://schemas.microsoft.com/winfx/2006/xaml/presentation' xmlns:x='http://schemas.microsoft.com/winfx/2006/xaml'>
                        <Button x:Name='btn' Height='50' Content='Button0'/>
                        <StackPanel TabFocusNavigation='Once'>
                            <StackPanel>
                                <Button x:Name='btnA' Content='Button0'/>
                                <Button x:Name='btnB' Content='Button1'/>
                                <Button x:Name='btnC' Content='Button2'/>
                            </StackPanel>
                            <Button x:Name='btnD' Content='Button3'/>
                        </StackPanel>
                    </StackPanel>";

            StackPanel rootPanel = null;
            Button button = null;
            Button buttonA = null;
            Button buttonB = null;
            Button buttonC = null;
            Button buttonD = null;

            UIExecutor.Execute(() =>
            {
                rootPanel = (StackPanel)XamlReader.Load(rootPanelXaml);
                button = (Button)rootPanel.FindName("btn");
                buttonA = (Button)rootPanel.FindName("btnA");
                buttonB = (Button)rootPanel.FindName("btnB");
                buttonC = (Button)rootPanel.FindName("btnC");
                buttonD = (Button)rootPanel.FindName("btnD");

                TestServices.WindowHelper.WindowContent = rootPanel;
            });

            TestServices.WindowHelper.WaitForIdle();

            // moving focus deliberately so that XAML Island gets the focus in the begining of the test
            FocusHelper.EnsureFocus(buttonC, FocusState.Keyboard);

            string eventOrder = "";

            Action<object, RoutedEventArgs> action = new Action<object, RoutedEventArgs>((s, e) =>
            {
                UIExecutor.Execute(() =>
                {
                    eventOrder = eventOrder + $"[{(s as FrameworkElement).Name}]";
                });
            });

            const string successString = "[btn][btnD][btn][btnD][btn][btnD]";

            using (var eventTester = new EventTester<Button, RoutedEventArgs>(button, "GotFocus", action))
            using (var eventTesterA = new EventTester<Button, RoutedEventArgs>(buttonA, "GotFocus", action))
            using (var eventTesterB = new EventTester<Button, RoutedEventArgs>(buttonB, "GotFocus", action))
            using (var eventTesterC = new EventTester<Button, RoutedEventArgs>(buttonC, "GotFocus", action))
            using (var eventTesterD = new EventTester<Button, RoutedEventArgs>(buttonD, "GotFocus", action))
            {
                FocusHelper.EnsureFocus(button, FocusState.Keyboard);
                const int NUM_TAB = 5;

                for (int count = 0; count < NUM_TAB; count++)
                {
                    TestServices.KeyboardHelper.ShiftTab();
                    TestServices.WindowHelper.WaitForIdle();
                }

                Verify.AreEqual(successString, eventOrder);
            }
        }

        [TestMethod]
        public void VerifyTabNavigationAndTabFocusNavigationIdentical()
        {
            const string rootPanelXaml =
                    @"<StackPanel xmlns='http://schemas.microsoft.com/winfx/2006/xaml/presentation' xmlns:x='http://schemas.microsoft.com/winfx/2006/xaml'>
                        <Button x:Name='btn' Height='50' Content='Button'/>
                    </StackPanel>";

            StackPanel rootPanel = null;
            Button button = null;

            UIExecutor.Execute(() =>
            {
                rootPanel = (StackPanel)XamlReader.Load(rootPanelXaml);
                button = (Button)rootPanel.FindName("btn");

                TestServices.WindowHelper.WindowContent = rootPanel;
            });

            TestServices.WindowHelper.WaitForIdle();

            UIExecutor.Execute(() =>
            {
                Verify.AreEqual(button.TabNavigation, button.TabFocusNavigation);

                button.TabNavigation = KeyboardNavigationMode.Cycle;

                Verify.AreEqual(button.TabNavigation, button.TabFocusNavigation);

                button.TabFocusNavigation = KeyboardNavigationMode.Once;

                Verify.AreEqual(button.TabNavigation, button.TabFocusNavigation);
            });
        }

        [TestMethod]
        public void VerifyClearValueWorksWithTabFocusNavigation()
        {
            const string rootPanelXaml =
                    @"<StackPanel xmlns='http://schemas.microsoft.com/winfx/2006/xaml/presentation' xmlns:x='http://schemas.microsoft.com/winfx/2006/xaml'>
                        <Button x:Name='btn' Height='50' Content='Button'/>
                    </StackPanel>";

            StackPanel rootPanel = null;
            Button button = null;

            UIExecutor.Execute(() =>
            {
                rootPanel = (StackPanel)XamlReader.Load(rootPanelXaml);
                button = (Button)rootPanel.FindName("btn");

                TestServices.WindowHelper.WindowContent = rootPanel;
            });

            TestServices.WindowHelper.WaitForIdle();

            UIExecutor.Execute(() =>
            {
                object localValue = button.ReadLocalValue(Button.TabFocusNavigationProperty);
                KeyboardNavigationMode mode = (KeyboardNavigationMode)button.GetValue(Button.TabFocusNavigationProperty);

                Verify.AreEqual(localValue, DependencyProperty.UnsetValue);
                Verify.AreEqual(mode, KeyboardNavigationMode.Local);

                button.TabFocusNavigation = KeyboardNavigationMode.Cycle;
                localValue = button.ReadLocalValue(Button.TabFocusNavigationProperty);
                mode = (KeyboardNavigationMode)button.GetValue(Button.TabFocusNavigationProperty);

                Verify.IsTrue(localValue != DependencyProperty.UnsetValue);
                Verify.AreEqual(mode, KeyboardNavigationMode.Cycle);

                button.ClearValue(Button.TabFocusNavigationProperty);

                localValue = button.ReadLocalValue(Button.TabFocusNavigationProperty);
                mode = (KeyboardNavigationMode)button.GetValue(Button.TabFocusNavigationProperty);

                Verify.AreEqual(localValue, DependencyProperty.UnsetValue);
                Verify.AreEqual(mode, KeyboardNavigationMode.Local);
            });
        }

        [TestMethod]
        [TestProperty("Description", "Verify that we get the first/last focusable correctly when tab indexes specified")]
        public void VerifyFindAndLastFocusableElementWithTabIndex()
        {
            const string rootPanelXaml =
                    @"<StackPanel xmlns='http://schemas.microsoft.com/winfx/2006/xaml/presentation' xmlns:x='http://schemas.microsoft.com/winfx/2006/xaml'>
                         <Button Width='50' x:Name='button' Content='Button 1'/>
                             <StackPanel x:Name='sp'>
                                 <Button Width='50' TabIndex='3' x:Name='button2' Content='Button 2'/>
                                 <Button Width='50' TabIndex='4' x:Name='button3' Content='Button 3'/>
                                 <Button Width='50' TabIndex='1' x:Name='button4' Content='Button 4'/>
                                 <Button Width='50' TabIndex='2' x:Name='button5' Content='Button 5'/>
                             </StackPanel>
                         <Button Width='50' x:Name='button6' Content='Button 6'/>
                     </StackPanel>";

            StackPanel rootPanel = null;
            StackPanel sp = null;
            Button button3 = null;
            Button button4 = null;

            UIExecutor.Execute(() =>
            {
                rootPanel = (StackPanel)XamlReader.Load(rootPanelXaml);
                sp = (StackPanel)rootPanel.FindName("sp");
                button3 = (Button)rootPanel.FindName("button3");
                button4 = (Button)rootPanel.FindName("button4");

                TestServices.WindowHelper.WindowContent = rootPanel;
            });

            TestServices.WindowHelper.WaitForIdle();

            UIExecutor.Execute(() =>
            {
                Verify.IsTrue(button4 == FocusManager.FindFirstFocusableElement(sp));
                Verify.IsTrue(button3 == FocusManager.FindLastFocusableElement(sp));
            });
        }

        [TestMethod]
        [TestProperty("Description", "Verify that we get the first/last focusable correctly when adding elements to the tree")]
        [TestProperty("Hosting:Mode", "UAP")]
        public void VerifyFindAndLastFocusableElementWhenElementAdded()
        {
            const string rootPanelXaml =
                    @"<StackPanel xmlns='http://schemas.microsoft.com/winfx/2006/xaml/presentation' xmlns:x='http://schemas.microsoft.com/winfx/2006/xaml'>
                        <Button Width='50' x:Name='button' Content='Button 1'/>
                        <Button Width='50' x:Name='button2' Content='Button 2'/>
                    </StackPanel>";

            StackPanel rootPanel = null;
            Button button = null;
            Button button2 = null;
            Button button3 = null;
            Button button4 = null;

            UIExecutor.Execute(() =>
            {
                rootPanel = (StackPanel)XamlReader.Load(rootPanelXaml);
                button = (Button)rootPanel.FindName("button");
                button2 = (Button)rootPanel.FindName("button2");

                TestServices.WindowHelper.WindowContent = rootPanel;
            });

            TestServices.WindowHelper.WaitForIdle();

            UIExecutor.Execute(() =>
            {
                Verify.IsTrue(button == FocusManager.FindFirstFocusableElement(null));
                Verify.IsTrue(button2 == FocusManager.FindLastFocusableElement(null));

                button3 = new Button();
                button3.Width = 50;

                button4 = new Button();
                button4.Width = 50;

                rootPanel.Children.Add(button3);
                rootPanel.Children.Insert(0, button4);

                Verify.IsTrue(button4 == FocusManager.FindFirstFocusableElement(null));
                Verify.IsTrue(button3 == FocusManager.FindLastFocusableElement(null));
            });
        }

        [TestMethod]
        [TestProperty("Description", "Validates Hyperlink TabIndex property affects focus order.")]
        [TestProperty("Hosting:Mode", "UAP")]   // Focus engagement bugs in lifted islands
        public void VerifyHyperlinkTabIndex()
        {
            StackPanel rootPanel = null;
            Hyperlink hyperlink1 = null;
            Hyperlink hyperlink2 = null;
            Hyperlink hyperlink3 = null;

            Hyperlink[] elementList = null;
            StringBuilder eventOrder = null;

            const string rootPanelXaml =
                @"<StackPanel xmlns='http://schemas.microsoft.com/winfx/2006/xaml/presentation' xmlns:x='http://schemas.microsoft.com/winfx/2006/xaml'>
                    <TextBlock Height='200' Width='200'>
                        <Hyperlink TabIndex='0' x:Name='hyperlink1' Foreground='LightBlue' FontSize='20'>Navigate1!</Hyperlink> \n\r
                        <Hyperlink TabIndex='2' x:Name='hyperlink2' Foreground='LightBlue' FontSize='20'>Navigate2!</Hyperlink> \n\r
                        <Hyperlink TabIndex='1' x:Name='hyperlink3' Foreground='LightBlue' FontSize='20'>Navigate3!</Hyperlink>
                    </TextBlock>
                  </StackPanel>";


            UIExecutor.Execute(() =>
            {
                rootPanel = (StackPanel)XamlReader.Load(rootPanelXaml);

                hyperlink1 = (Hyperlink)(rootPanel.FindName("hyperlink1"));
                hyperlink2 = (Hyperlink)(rootPanel.FindName("hyperlink2"));
                hyperlink3 = (Hyperlink)(rootPanel.FindName("hyperlink3"));
                elementList = new Hyperlink[3] { hyperlink1, hyperlink2, hyperlink3 };
                eventOrder = new StringBuilder();
                TestServices.WindowHelper.WindowContent = rootPanel;
            });

            TestServices.WindowHelper.WaitForIdle();

            using (var gotFocus = new EventTester<Hyperlink, RoutedEventArgs>(hyperlink1, "GotFocus"))
            {
                UIExecutor.Execute(() =>
                {
                    Verify.IsTrue(hyperlink1.Focus(FocusState.Keyboard));
                });
                gotFocus.Wait();
            }

            UIExecutor.Execute(() =>
            {
                FindNextElementOptions navigationOptions = new FindNextElementOptions();
                navigationOptions.SearchRoot = TestServices.WindowHelper.WindowContent;

                Hyperlink hy = (Hyperlink)(FocusManager.FindNextElement(FocusNavigationDirection.Next, navigationOptions));
                Verify.AreEqual(hyperlink3, hy);

                hy = (Hyperlink)(FocusManager.FindNextElement(FocusNavigationDirection.Previous, navigationOptions));
                Verify.AreEqual(hyperlink2, hy);
            });

            TestServices.WindowHelper.WaitForIdle();

            using (var focusEventOrderTester = new FocusEventOrderingTester(elementList, eventOrder))
            {
                for (int i = 0; i < 3; i++)
                {
                    TestServices.KeyboardHelper.Tab();
                    TestServices.WindowHelper.WaitForIdle();
                }
                UIExecutor.Execute(() =>
                {
                    Verify.AreEqual("[hyperlink1LostFocus][FocusManagerLostFocus:1][hyperlink3GotFocus][FocusManagerGotFocus]" +
                                    "[hyperlink3LostFocus][FocusManagerLostFocus:2][hyperlink2GotFocus][FocusManagerGotFocus]" +
                                    "[hyperlink2LostFocus][FocusManagerLostFocus:3][FocusManagerGotFocus][FocusManagerLostFocus:4][hyperlink1GotFocus][FocusManagerGotFocus]",
                        eventOrder.ToString());
                });
            }

            TestServices.WindowHelper.WaitForIdle();
        }

        [TestMethod]
        [TestProperty("Description", "Validates Hyperlink TabIndex property affects focus order in RichTextBlock.")]
        [TestProperty("Hosting:Mode", "UAP")]   // Focus engagement bugs in lifted islands
        public void VerifyHyperlinkTabIndexWithRichTextBlock()
        {
            StackPanel rootPanel = null;
            Hyperlink hyperlink1 = null;
            Hyperlink hyperlink2 = null;
            Hyperlink hyperlink3 = null;

            Hyperlink[] elementList = null;
            StringBuilder eventOrder = null;


            const string rootPanelXaml =
                @"<StackPanel xmlns='http://schemas.microsoft.com/winfx/2006/xaml/presentation'
                       xmlns:x='http://schemas.microsoft.com/winfx/2006/xaml'
                       VerticalAlignment='Center' HorizontalAlignment='Center'>
                     <RichTextBlock x:Name='txbl' Height='20' Width='200' >
                       <Paragraph><Hyperlink TabIndex='0' x:Name='hyperlink1' Foreground='LightBlue' FontSize='20'>Navigate1!</Hyperlink> \n\r
                       <Hyperlink TabIndex='2' x:Name='hyperlink2' Foreground='LightBlue'>Navigate2!</Hyperlink> \n\r
                       <Hyperlink TabIndex='1' x:Name='hyperlink3' Foreground='LightBlue'>Navigate3!</Hyperlink></Paragraph>
                     </RichTextBlock>
                     <RichTextBlockOverflow x:Name='txb2' />
                    </StackPanel>";


            UIExecutor.Execute(() =>
            {
                rootPanel = (StackPanel)XamlReader.Load(rootPanelXaml);

                hyperlink1 = (Hyperlink)(rootPanel.FindName("hyperlink1"));
                hyperlink2 = (Hyperlink)(rootPanel.FindName("hyperlink2"));
                hyperlink3 = (Hyperlink)(rootPanel.FindName("hyperlink3"));
                elementList = new Hyperlink[3] { hyperlink1, hyperlink2, hyperlink3 };
                eventOrder = new StringBuilder();
                TestServices.WindowHelper.WindowContent = rootPanel;
            });

            TestServices.WindowHelper.WaitForIdle();

            using (var gotFocus = new EventTester<Hyperlink, RoutedEventArgs>(hyperlink1, "GotFocus"))
            {
                UIExecutor.Execute(() =>
                {
                    Verify.IsTrue(hyperlink1.Focus(FocusState.Keyboard));
                });
                gotFocus.Wait();
            }

            UIExecutor.Execute(() =>
            {
                FindNextElementOptions navigationOptions = new FindNextElementOptions();
                navigationOptions.SearchRoot = TestServices.WindowHelper.WindowContent;

                Hyperlink hy = (Hyperlink)(FocusManager.FindNextElement(FocusNavigationDirection.Next, navigationOptions));
                Verify.AreEqual(hyperlink3, hy);

                hy = (Hyperlink)(FocusManager.FindNextElement(FocusNavigationDirection.Previous, navigationOptions));
                Verify.AreEqual(hyperlink2, hy);
            });

            TestServices.WindowHelper.WaitForIdle();

            using (var focusEventOrderTester = new FocusEventOrderingTester(elementList, eventOrder))
            {
                for (int i = 0; i < 3; i++)
                {
                    TestServices.KeyboardHelper.Tab();
                    TestServices.WindowHelper.WaitForIdle();
                }

                Verify.AreEqual("[hyperlink1LostFocus][FocusManagerLostFocus:1][hyperlink3GotFocus][FocusManagerGotFocus]" +
                                "[hyperlink3LostFocus][FocusManagerLostFocus:2][hyperlink2GotFocus][FocusManagerGotFocus]" +
                                "[hyperlink2LostFocus][FocusManagerLostFocus:3][FocusManagerGotFocus][FocusManagerLostFocus:4][hyperlink1GotFocus][FocusManagerGotFocus]",
                    eventOrder.ToString());
            }

            TestServices.WindowHelper.WaitForIdle();
        }

        [TestMethod]
        [TestProperty("Description", "Validates Hyperlink TabIndex property affects focus order with tab index once.")]
        [TestProperty("Hosting:Mode", "UAP")]   // Focus engagement bugs in lifted islands
        public void VerifyHyperlinkTabIndexWithTabNavigationOnce()
        {
            StackPanel rootPanel = null;
            Hyperlink hyperlink1 = null;
            Hyperlink hyperlink2 = null;
            Hyperlink hyperlink3 = null;

            Hyperlink[] elementList = null;
            StringBuilder eventOrder = null;


            const string rootPanelXaml =
                 @"<StackPanel xmlns='http://schemas.microsoft.com/winfx/2006/xaml/presentation'
                       xmlns:x='http://schemas.microsoft.com/winfx/2006/xaml'
                       VerticalAlignment='Center' HorizontalAlignment='Center'>
                     <RichTextBlock x:Name='txbl' Width='200'>
                       <Paragraph><Hyperlink TabIndex='0' x:Name='hyperlink1' Foreground='LightBlue' FontSize='20'>Navigate1!</Hyperlink></Paragraph>
                     </RichTextBlock>
                     <RichTextBlock x:Name='txb2' Width='200' TabFocusNavigation='Once'>
                       <Paragraph><Hyperlink TabIndex='2' x:Name='hyperlink3' Foreground='LightBlue'>Navigate3!</Hyperlink></Paragraph>
                       <Paragraph><Hyperlink TabIndex='1' x:Name='hyperlink2' Foreground='LightBlue'>Navigate2!</Hyperlink></Paragraph>
                     </RichTextBlock>
                   </StackPanel>";


            UIExecutor.Execute(() =>
            {
                rootPanel = (StackPanel)XamlReader.Load(rootPanelXaml);

                hyperlink1 = (Hyperlink)(rootPanel.FindName("hyperlink1"));
                hyperlink2 = (Hyperlink)(rootPanel.FindName("hyperlink2"));
                hyperlink3 = (Hyperlink)(rootPanel.FindName("hyperlink3"));
                elementList = new Hyperlink[3] { hyperlink1, hyperlink2, hyperlink3 };
                eventOrder = new StringBuilder();
                TestServices.WindowHelper.WindowContent = rootPanel;
            });

            TestServices.WindowHelper.WaitForIdle();

            using (var gotFocus = new EventTester<Hyperlink, RoutedEventArgs>(hyperlink1, "GotFocus"))
            {
                UIExecutor.Execute(() =>
                {
                    Verify.IsTrue(hyperlink1.Focus(FocusState.Keyboard));
                });
                gotFocus.Wait();
            }

            TestServices.WindowHelper.WaitForIdle();

            using (var focusEventOrderTester = new FocusEventOrderingTester(elementList, eventOrder))
            {
                for (int i = 0; i < 2; i++)
                {
                    TestServices.KeyboardHelper.Tab();
                    TestServices.WindowHelper.WaitForIdle();
                }

                Verify.AreEqual("[hyperlink1LostFocus][FocusManagerLostFocus:1][hyperlink2GotFocus][FocusManagerGotFocus]" +
                                "[hyperlink2LostFocus][FocusManagerLostFocus:2][FocusManagerGotFocus][FocusManagerLostFocus:3][hyperlink1GotFocus][FocusManagerGotFocus]",
                    eventOrder.ToString());
            }

            TestServices.WindowHelper.WaitForIdle();
        }

        [TestMethod]
        [TestProperty("Description", "Verify that TryMoveAsync can be awaited")]
        public async Task VerifyTryMoveFocusAsync()
        {
            string expectedString = "[element1LosingFocus:1][FocusManagerLosingFocus][element2GettingFocus][FocusManagerGettingFocus][element1LostFocus][FocusManagerLostFocus][element2GotFocus][FocusManagerGotFocus]";
            await FocusAsyncValidation(FocusAsyncMethod.TryMoveFocusAsync, FocusElementType.Button, false, expectedString);
        }

        [TestMethod]
        [TestProperty("Description", "Verify that TryMoveAsync can be awaited, StackPanel variant")]
        public async Task VerifyTryMoveFocusAsyncForStackPanel()
        {
            string expectedString = "[element1LosingFocus:1][FocusManagerLosingFocus][element2GettingFocus][FocusManagerGettingFocus][element1LostFocus][FocusManagerLostFocus][element2GotFocus][FocusManagerGotFocus]";
            await FocusAsyncValidation(FocusAsyncMethod.TryMoveFocusAsync, FocusElementType.StackPanel, false, expectedString);
        }

        [TestMethod]
        [TestProperty("Description", "Verify that TryMoveAsync can be awaited, TextBlock variant")]
        public async Task VerifyTryMoveFocusAsyncForTextBlock()
        {
            string expectedString = "[element1LosingFocus:1][FocusManagerLosingFocus][element2GettingFocus][FocusManagerGettingFocus][element1LostFocus][FocusManagerLostFocus][element2GotFocus][FocusManagerGotFocus]";
            await FocusAsyncValidation(FocusAsyncMethod.TryMoveFocusAsync, FocusElementType.TextBlock, false, expectedString);
        }

        [TestMethod]
        [TestProperty("Description", "Verify that TryMoveAsync can be awaited, RichTextBlock variant")]
        public async Task VerifyTryMoveFocusAsyncForRichTextBlock()
        {
            string expectedString = "[element1LosingFocus:1][FocusManagerLosingFocus][element2GettingFocus][FocusManagerGettingFocus][element1LostFocus][FocusManagerLostFocus][element2GotFocus][FocusManagerGotFocus]";
            await FocusAsyncValidation(FocusAsyncMethod.TryMoveFocusAsync, FocusElementType.RichTextBlock, false, expectedString);
        }

        [TestMethod]
        [TestProperty("Description", "Verify that we successfully await the operation, but the Succeeded value is false")]
        public async Task VerifyTryMoveFocusAsyncUnsuccessful()
        {
            string expectedString = "[element1LosingFocus:1][FocusManagerLosingFocus][element2GettingFocus][FocusManagerGettingFocus:Canceled]";
            await FocusAsyncValidation(FocusAsyncMethod.TryMoveFocusAsync, FocusElementType.Button, true, expectedString);
        }

        [TestMethod]
        [TestProperty("Description", "Verify that we successfully await the operation, but the Succeeded value is false, StackPanel variant")]
        public async Task VerifyTryMoveFocusAsyncUnsuccessfulForStackPanel()
        {
            string expectedString = "[element1LosingFocus:1][FocusManagerLosingFocus][element2GettingFocus][FocusManagerGettingFocus:Canceled]";
            await FocusAsyncValidation(FocusAsyncMethod.TryMoveFocusAsync, FocusElementType.StackPanel, true, expectedString);
        }

        [TestMethod]
        [TestProperty("Description", "Verify that we successfully await the operation, but the Succeeded value is false, TextBlock variant")]
        public async Task VerifyTryMoveFocusAsyncUnsuccessfulForTextBlock()
        {
            string expectedString = "[element1LosingFocus:1][FocusManagerLosingFocus][element2GettingFocus][FocusManagerGettingFocus:Canceled]";
            await FocusAsyncValidation(FocusAsyncMethod.TryMoveFocusAsync, FocusElementType.TextBlock, true, expectedString);
        }

        [TestMethod]
        [TestProperty("Description", "Verify that we successfully await the operation, but the Succeeded value is false, RichTextBlock variant")]
        public async Task VerifyTryMoveFocusAsyncUnsuccessfulForRichTextBlock()
        {
            string expectedString = "[element1LosingFocus:1][FocusManagerLosingFocus][element2GettingFocus][FocusManagerGettingFocus:Canceled]";
            await FocusAsyncValidation(FocusAsyncMethod.TryMoveFocusAsync, FocusElementType.RichTextBlock, true, expectedString);
        }

        [TestMethod]
        [TestProperty("Description", "Verify that TryAsync can be awaited")]
        [TestProperty("Hosting:Mode", "UAP")] // fails in WPF mode due to final release queue is not empty cleanup issue
        public async Task VerifyTryFocusAsync()
        {
            string expectedString = "[element1LosingFocus:1][FocusManagerLosingFocus][element2GettingFocus][FocusManagerGettingFocus][element1LostFocus][FocusManagerLostFocus][element2GotFocus][FocusManagerGotFocus]";
            await FocusAsyncValidation(FocusAsyncMethod.TryFocusAsync, FocusElementType.Button, false, expectedString);
        }

        [TestMethod]
        [TestProperty("Description", "Verify that TryAsync can be awaited, StackPanel variant")]
        [TestProperty("Hosting:Mode", "UAP")] // fails in WPF mode due to final release queue is not empty cleanup issue
        public async Task VerifyTryFocusAsyncForStackPanel()
        {
            string expectedString = "[element1LosingFocus:1][FocusManagerLosingFocus][element2GettingFocus][FocusManagerGettingFocus][element1LostFocus][FocusManagerLostFocus][element2GotFocus][FocusManagerGotFocus]";
            await FocusAsyncValidation(FocusAsyncMethod.TryFocusAsync, FocusElementType.StackPanel, false, expectedString);
        }

        [TestMethod]
        [TestProperty("Description", "Verify that TryAsync can be awaited, TextBlock variant")]
        [TestProperty("Hosting:Mode", "UAP")] // fails in WPF mode due to final release queue is not empty cleanup issue
        public async Task VerifyTryFocusAsyncForTextBlock()
        {
            string expectedString = "[element1LosingFocus:1][FocusManagerLosingFocus][element2GettingFocus][FocusManagerGettingFocus][element1LostFocus][FocusManagerLostFocus][element2GotFocus][FocusManagerGotFocus]";
            await FocusAsyncValidation(FocusAsyncMethod.TryFocusAsync, FocusElementType.TextBlock, false, expectedString);
        }

        [TestMethod]
        [TestProperty("Description", "Verify that TryAsync can be awaited, RichTextBlock")]
        [TestProperty("Hosting:Mode", "UAP")] // fails in WPF mode due to final release queue is not empty cleanup issue
        public async Task VerifyTryFocusAsyncForRichTextBlock()
        {
            string expectedString = "[element1LosingFocus:1][FocusManagerLosingFocus][element2GettingFocus][FocusManagerGettingFocus][element1LostFocus][FocusManagerLostFocus][element2GotFocus][FocusManagerGotFocus]";
            await FocusAsyncValidation(FocusAsyncMethod.TryFocusAsync, FocusElementType.RichTextBlock, false, expectedString);
        }

        [TestMethod]
        [TestProperty("Description", "Verify that we successfully await the operation, but the Succeeded value is false")]
        [TestProperty("Hosting:Mode", "UAP")] // fails in WPF mode due to final release queue is not empty cleanup issue
        public async Task VerifyTryFocusAsyncUnsuccessful()
        {
            string expectedString = "[element1LosingFocus:1][FocusManagerLosingFocus][element2GettingFocus][FocusManagerGettingFocus:Canceled]";
            await FocusAsyncValidation(FocusAsyncMethod.TryFocusAsync, FocusElementType.Button, true, expectedString);
        }

        [TestMethod]
        [TestProperty("Description", "Verify that we successfully await the operation, but the Succeeded value is false, StackPanel variant")]
        [TestProperty("Hosting:Mode", "UAP")] // fails in WPF mode due to final release queue is not empty cleanup issue
        public async Task VerifyTryFocusAsyncUnsuccessfulForStackPanel()
        {
            string expectedString = "[element1LosingFocus:1][FocusManagerLosingFocus][element2GettingFocus][FocusManagerGettingFocus:Canceled]";
            await FocusAsyncValidation(FocusAsyncMethod.TryFocusAsync, FocusElementType.StackPanel, true, expectedString);
        }

        [TestMethod]
        [TestProperty("Description", "Verify that we successfully await the operation, but the Succeeded value is false, TextBlock variant")]
        [TestProperty("Hosting:Mode", "UAP")] // fails in WPF mode due to final release queue is not empty cleanup issue
        public async Task VerifyTryFocusAsyncUnsuccessfulForTextBlock()
        {
            string expectedString = "[element1LosingFocus:1][FocusManagerLosingFocus][element2GettingFocus][FocusManagerGettingFocus:Canceled]";
            await FocusAsyncValidation(FocusAsyncMethod.TryFocusAsync, FocusElementType.TextBlock, true, expectedString);
        }

        [TestMethod]
        [TestProperty("Description", "Verify that we successfully await the operation, but the Succeeded value is false, RichTextBlock variant")]
        [TestProperty("Hosting:Mode", "UAP")] // fails in WPF mode due to final release queue is not empty cleanup issue
        public async Task VerifyTryFocusAsyncUnsuccessfulForRichTextBlock()
        {
            string expectedString = "[element1LosingFocus:1][FocusManagerLosingFocus][element2GettingFocus][FocusManagerGettingFocus:Canceled]";
            await FocusAsyncValidation(FocusAsyncMethod.TryFocusAsync, FocusElementType.RichTextBlock, true, expectedString);
        }

        private async Task FocusAsyncValidation(FocusAsyncMethod method, FocusElementType elementType, bool shouldCancel, string expectedString)
        {
            string rootPanelXaml = null;

            switch (elementType)
            {
                case FocusElementType.Button:
                    rootPanelXaml =
                    @"<StackPanel xmlns='http://schemas.microsoft.com/winfx/2006/xaml/presentation' xmlns:x='http://schemas.microsoft.com/winfx/2006/xaml'>
                            <Button Width='50' x:Name='element1' Content='Button 1'/>
                            <StackPanel x:Name='sp'>
                                <Button Width='50' x:Name='element2' Content='Button 2'/>
                                <Button Width='50' Content='Button 3'/>
                            </StackPanel>
                            <Button Width='50' Content='Button 4'/>
                    </StackPanel>";
                    break;
                case FocusElementType.StackPanel:
                    rootPanelXaml =
                    @"<StackPanel xmlns='http://schemas.microsoft.com/winfx/2006/xaml/presentation' xmlns:x='http://schemas.microsoft.com/winfx/2006/xaml'>
                            <Button Width='50' x:Name='element1' Content='Button 1'/>
                            <StackPanel x:Name='sp'>
                                <StackPanel x:Name='element2' HorizontalAlignment = 'Left' Width='50' Height='20' Background='Red' IsTabStop='True' UseSystemFocusVisuals='True'/>
                                <Button Width='50' Content='Button 3'/>
                            </StackPanel>
                            <Button Width='50' Content='Button 4'/>
                    </StackPanel>";
                    break;
                case FocusElementType.TextBlock:
                    rootPanelXaml =
                    @"<StackPanel xmlns='http://schemas.microsoft.com/winfx/2006/xaml/presentation' xmlns:x='http://schemas.microsoft.com/winfx/2006/xaml'>
                            <Button Width='50' x:Name='element1' Content='Button 1'/>
                            <StackPanel x:Name='sp'>
                                <TextBlock x:Name='element2' Text='TextBlock' HorizontalAlignment = 'Left' Width='50' IsTabStop='True' UseSystemFocusVisuals='True'/>
                                <Button Width='50' Content='Button 3'/>
                            </StackPanel>
                            <Button Width='50' Content='Button 4'/>
                    </StackPanel>";
                    break;
                case FocusElementType.RichTextBlock:
                    rootPanelXaml =
                    @"<StackPanel xmlns='http://schemas.microsoft.com/winfx/2006/xaml/presentation' xmlns:x='http://schemas.microsoft.com/winfx/2006/xaml'>
                            <Button Width='50' x:Name='element1' Content='Button 1'/>
                            <StackPanel x:Name='sp'>
                                <RichTextBlock x:Name='element2' HorizontalAlignment = 'Left' Width='50' IsTabStop='True' UseSystemFocusVisuals='True'>
                                  <Paragraph>RichTextBlock</Paragraph>
                                </RichTextBlock>
                                <Button Width='50' Content='Button 3'/>
                            </StackPanel>
                            <Button Width='50' Content='Button 4'/>
                    </StackPanel>";
                    break;
            }

            StackPanel rootPanel = null;
            FrameworkElement element1 = null;
            FrameworkElement element2 = null;
            FrameworkElement[] elementList = null;
            StringBuilder eventOrder = null;

            UIExecutor.Execute(() =>
            {
                rootPanel = (StackPanel)XamlReader.Load(rootPanelXaml);
                element1 = (FrameworkElement)rootPanel.FindName("element1");
                element2 = (FrameworkElement)rootPanel.FindName("element2");
                eventOrder = new StringBuilder();
                elementList = new FrameworkElement[2] { element1, element2 };

                TestServices.WindowHelper.WindowContent = rootPanel;
            });
            TestServices.WindowHelper.WaitForIdle();

            FocusHelper.EnsureFocus(element1, FocusState.Keyboard);

            var element2GettingFocusHandler = new Action<object, GettingFocusEventArgs>((source, args) =>
            {
                args.Cancel = shouldCancel;
            });

            FrameworkElement targetElement = shouldCancel ? element1 : element2;

            using (var focusEventOrderTester = new FocusEventOrderingTester(elementList, eventOrder))
            using (var element2GettingFocus = new EventTester<UIElement, GettingFocusEventArgs>(element2, "GettingFocus", element2GettingFocusHandler))
            {
                await UIExecutor.ExecuteAsync(async () =>
                {
                    FocusMovementResult result = null;

                    if (method == FocusAsyncMethod.TryFocusAsync)
                    {
                        Log.Comment("calling TryFocusAsync");
                        result = await FocusManager.TryFocusAsync(element2, FocusState.Keyboard);
                    }
                    else if (method == FocusAsyncMethod.TryMoveFocusAsync)
                    {
                        Log.Comment("TryMoveFocusAsync in the down direction");
                        FindNextElementOptions options = new FindNextElementOptions();
                        options.SearchRoot = TestServices.WindowHelper.WindowContent;
                        result = await FocusManager.TryMoveFocusAsync(FocusNavigationDirection.Down, options);
                    }

                    Verify.AreEqual(result.Succeeded, !shouldCancel);
                    Verify.AreEqual(targetElement, FocusManager.GetFocusedElement(TestServices.WindowHelper.WindowContent.XamlRoot));
                });

                TestServices.WindowHelper.WaitForIdle();
                Verify.AreEqual(expectedString, eventOrder.ToString());
            }
        }

        [TestMethod]
        [TestProperty("Description", "Verify that TryAsync fails when trying to focus a non-focusable element")]
        [TestProperty("Hosting:Mode", "UAP")] // fails in WPF mode due to final release queue is not empty cleanup issue
        public async Task VerifyTryFocusAsyncFailsOnNonFocusableElements()
        {
            const string rootPanelXaml =
                    @"<StackPanel xmlns='http://schemas.microsoft.com/winfx/2006/xaml/presentation' xmlns:x='http://schemas.microsoft.com/winfx/2006/xaml'>
                            <Button Width='50' x:Name='button' Content='Button 1'/>
                            <StackPanel x:Name='sp'>
                                <Button Width='50' x:Name='button2' Content='Button 2'/>
                                <Button Width='50' Content='Button 3'/>
                            </StackPanel>
                            <Button Width='50' Content='Button 4'/>
                    </StackPanel>";

            StackPanel rootPanel = null;
            Button button = null;
            Button button2 = null;

            UIExecutor.Execute(() =>
            {
                rootPanel = (StackPanel)XamlReader.Load(rootPanelXaml);
                button = (Button)rootPanel.FindName("button");
                button2 = (Button)rootPanel.FindName("button2");

                TestServices.WindowHelper.WindowContent = rootPanel;
            });
            TestServices.WindowHelper.WaitForIdle();

            FocusHelper.EnsureFocus(button2, FocusState.Keyboard);

            await UIExecutor.ExecuteAsync(async () =>
            {
                Log.Comment("calling TryFocusAsync");
                FocusMovementResult result = await FocusManager.TryFocusAsync(rootPanel, FocusState.Keyboard);

                Verify.AreEqual(result.Succeeded, false);
                Verify.AreEqual(button2, FocusManager.GetFocusedElement(TestServices.WindowHelper.WindowContent.XamlRoot));
            });
        }

        [TestMethod]
        [TestProperty("Description", "Verifies execution of code in KeyDown handler if TryFocusAsync can be completed synchronously")]
        public void VerifyAsyncTaskCompletesSynchronouslyOnSameThread()
        {
            const string rootPanelXaml =
                    @"<StackPanel xmlns='http://schemas.microsoft.com/winfx/2006/xaml/presentation' xmlns:x='http://schemas.microsoft.com/winfx/2006/xaml'>
                            <Button Width='50' x:Name='button' Content='Button 1'/>
                            <StackPanel x:Name='sp'>
                                <Button Width='50' x:Name='button2' Content='Button 2'/>
                                <Button Width='50' Content='Button 3'/>
                            </StackPanel>
                            <Button Width='50' Content='Button 4'/>
                    </StackPanel>";

            StackPanel rootPanel = null;
            Button button = null;
            Button button2 = null;

            UIExecutor.Execute(() =>
            {
                rootPanel = (StackPanel)XamlReader.Load(rootPanelXaml);
                button = (Button)rootPanel.FindName("button");
                button2 = (Button)rootPanel.FindName("button2");

                TestServices.WindowHelper.WindowContent = rootPanel;
            });
            TestServices.WindowHelper.WaitForIdle();

            FocusHelper.EnsureFocus(button, FocusState.Keyboard);

            var buttonKeyDownEventHandler = new Action<object, KeyRoutedEventArgs>((source, args) =>
            {
                UIExecutor.Execute(async () =>
                {
                    Log.Comment("calling TryFocusAsync");
                    FocusMovementResult result = await FocusManager.TryFocusAsync(button2, FocusState.Keyboard);
                    args.Handled = result.Succeeded;
                });
            });

            var rootPanelKeyDownEventHandler = new Action<object, KeyRoutedEventArgs>((source, args) =>
            {
                UIExecutor.Execute(() =>
                {
                    Verify.AreEqual(args.Handled, true);
                });
            });

            using (var buttonKeyDownEventTester = new EventTester<UIElement, KeyRoutedEventArgs>(button, "KeyDown", buttonKeyDownEventHandler))
            using (var rootPanelKeyDownEventTester = EventTester<UIElement, KeyRoutedEventArgs>.FromRoutedEvent(rootPanel, "KeyDown", rootPanelKeyDownEventHandler))
            {
                TestServices.KeyboardHelper.Up();

                UIExecutor.Execute(() =>
                {
                    Verify.AreEqual(button2, FocusManager.GetFocusedElement(TestServices.WindowHelper.WindowContent.XamlRoot));
                });

                rootPanelKeyDownEventTester.Wait();
            }
        }

        [TestMethod]
        [TestProperty("Description", "Verifies execution of code in KeyDown handler if TryFocusAsync can be completed synchronously")]
        public async void ElementMovesToCorrectElementWhenFocusedElementCollapsed()
        {
            const string rootPanelXaml =
                    @"<StackPanel xmlns='http://schemas.microsoft.com/winfx/2006/xaml/presentation' xmlns:x='http://schemas.microsoft.com/winfx/2006/xaml'>
                            <Button Content='Button 1'/>
                            <StackPanel>
                                <Button x:Name='collapsedButton' Content='Button 2'/>
                                <Button x:Name='notCollapsedButton' Content='Button 3'/>
                            </StackPanel>
                            <Button Content='Button 4'/>
                    </StackPanel>";

            StackPanel rootPanel = null;
            Button collapsedButton = null;
            Button notCollapsedButton = null;

            UIExecutor.Execute(() =>
            {
                rootPanel = (StackPanel)XamlReader.Load(rootPanelXaml);
                collapsedButton = (Button)rootPanel.FindName("collapsedButton");
                notCollapsedButton = (Button)rootPanel.FindName("notCollapsedButton");

                TestServices.WindowHelper.WindowContent = rootPanel;
            });
            TestServices.WindowHelper.WaitForIdle();

            FocusHelper.EnsureFocus(collapsedButton, FocusState.Keyboard);

            using (var notCollapsedButtonGotFocus = new EventTester<Button, RoutedEventArgs>(notCollapsedButton, "GotFocus"))
            {
                UIExecutor.Execute(() =>
                {
                    collapsedButton.Visibility = Visibility.Collapsed;
                });

                await notCollapsedButtonGotFocus.VerifyEventRaised();
            }
        }

        [TestMethod]
        [TestProperty("Description", "Verifies that moving focus back (using Shift+Tab) into ContentControl (with disabled tab stop) brings focus to the correct element contained inside it. ")]
        public void FocusMovesToNextCorrectElementWhenFocusCandidateElementCollapsed()
        {
            const string rootPanelXaml =
                    @"<StackPanel xmlns='http://schemas.microsoft.com/winfx/2006/xaml/presentation' xmlns:x='http://schemas.microsoft.com/winfx/2006/xaml'>
                            <StackPanel>
                                <ContentControl IsTabStop='false'>
                                    <StackPanel>
                                        <Button x:Name='innerBtn1' Content='Inner Button 1'/>
                                        <Button x:Name='innerBtn2' Content='Inner Button 2'/>
                                    </StackPanel>
                                </ContentControl>
                                <Button x:Name='outerBtn' Content='Outside Button'/>
                            </StackPanel>
                    </StackPanel>";

            StackPanel rootPanel = null;
            Button innerBtn2 = null;
            Button outerBtn = null;

            UIExecutor.Execute(() =>
            {
                rootPanel = (StackPanel)XamlReader.Load(rootPanelXaml);
                innerBtn2 = (Button)rootPanel.FindName("innerBtn2");
                outerBtn = (Button)rootPanel.FindName("outerBtn");

                TestServices.WindowHelper.WindowContent = rootPanel;
            });
            TestServices.WindowHelper.WaitForIdle();

            FocusHelper.EnsureFocus(outerBtn, FocusState.Keyboard);
            using (var innerBtn2GotFocus = new EventTester<Button, RoutedEventArgs>(innerBtn2, "GotFocus"))
            {
                TestServices.KeyboardHelper.ShiftTab();
                innerBtn2GotFocus.Wait();

                UIExecutor.Execute(() =>
                {
                    Verify.AreEqual(innerBtn2, FocusManager.GetFocusedElement(TestServices.WindowHelper.WindowContent.XamlRoot));
                });
            }
        }

        [TestMethod]
        [TestProperty("Description", "Verify that we can [can't] focus focusable [non-focusable] stackpanel UIElement")]
        public void VerifyFocusBehaviorWithFocusDisabledUIElement()
        {
            const string rootPanelXaml =
                    @"<StackPanel xmlns='http://schemas.microsoft.com/winfx/2006/xaml/presentation' xmlns:x='http://schemas.microsoft.com/winfx/2006/xaml'>
                            <Button Width='50' x:Name='outerSPBtn' Content='Button-1'/>
                            <StackPanel x:Name='innerSP'>
                                <Button Width='50' x:Name='innerSPBtn' Content='Button-2'/>
                            </StackPanel>
                    </StackPanel>";

            StackPanel rootPanel = null;
            StackPanel innerSP = null;
            Button outerSPBtn = null;
            Button innerSPBtn = null;

            UIExecutor.Execute(() =>
            {
                rootPanel = (StackPanel)XamlReader.Load(rootPanelXaml);
                innerSP = (StackPanel)rootPanel.FindName("innerSP");
                outerSPBtn = (Button)rootPanel.FindName("outerSPBtn");
                innerSPBtn = (Button)rootPanel.FindName("innerSPBtn");

                TestServices.WindowHelper.WindowContent = rootPanel;
            });
            TestServices.WindowHelper.WaitForIdle();

            UIExecutor.Execute(() =>
            {
                Log.Comment("Focus outer stackpanel button");
                Verify.IsTrue(outerSPBtn.Focus(FocusState.Programmatic));

                // Default IsTabStop is false for UIElements
                Log.Comment("Focus non focusable inner stackpanel and inner button should get focused");
                Verify.IsTrue(innerSP.Focus(FocusState.Programmatic));
                Verify.AreEqual(innerSPBtn, FocusManager.GetFocusedElement(TestServices.WindowHelper.WindowContent.XamlRoot));

                Log.Comment("Make inner stackpanel focusable by setting IsTabStop = true");
                innerSP.IsTabStop = true;

                Log.Comment("Focus inner stackpanel");
                Verify.IsTrue(innerSP.Focus(FocusState.Programmatic));
                Verify.AreEqual(innerSP, FocusManager.GetFocusedElement(TestServices.WindowHelper.WindowContent.XamlRoot));
            });

            TestServices.WindowHelper.WaitForIdle();
        }

        [TestMethod]
        [TestProperty("Description", "Verify that we can[can't] focus focusable[non-focusable] stackpanel with Tab/ Shift+Tab")]
        [TestProperty("Hosting:Mode", "UAP")]   // Focus engagement bugs in lifted islands
        public void VerifyFocusBehaviorWithTabOnFocusDisabledUIElement()
        {
            const string rootPanelXaml =
                    @"<StackPanel xmlns='http://schemas.microsoft.com/winfx/2006/xaml/presentation' xmlns:x='http://schemas.microsoft.com/winfx/2006/xaml'>
                            <StackPanel x:Name='innerSP1' IsTabStop='True'/>
                            <StackPanel x:Name='innerSP2' IsTabStop='False'/>
                            <StackPanel x:Name='innerSP3' IsTabStop='True'/>
                    </StackPanel>";

            StackPanel rootPanel = null;
            StackPanel innerSP1 = null;
            StackPanel innerSP2 = null;
            StackPanel innerSP3 = null;

            UIExecutor.Execute(() =>
            {
                rootPanel = (StackPanel)XamlReader.Load(rootPanelXaml);
                innerSP1 = (StackPanel)rootPanel.FindName("innerSP1");
                innerSP2 = (StackPanel)rootPanel.FindName("innerSP2");
                innerSP3 = (StackPanel)rootPanel.FindName("innerSP3");

                TestServices.WindowHelper.WindowContent = rootPanel;
            });
            TestServices.WindowHelper.WaitForIdle();
            FocusHelper.EnsureFocus(innerSP1, FocusState.Keyboard);

            Log.Comment("Hit Tab to focus non-focusable innerSP2 and next available InnerSP3 should get focused");
            TestServices.KeyboardHelper.Tab();
            TestServices.WindowHelper.WaitForIdle();
            UIExecutor.Execute(() =>
            {
                Verify.AreEqual(innerSP3, FocusManager.GetFocusedElement(TestServices.WindowHelper.WindowContent.XamlRoot));
                Log.Comment("Make innerSP2 focusable by setting IsTabStop = true");
                innerSP2.IsTabStop = true;
            });

            Log.Comment("Hit Shift+Tab to focus innerSP2");
            TestServices.KeyboardHelper.ShiftTab();
            TestServices.WindowHelper.WaitForIdle();
            UIExecutor.Execute(() =>
            {
                Verify.AreEqual(innerSP2, FocusManager.GetFocusedElement(TestServices.WindowHelper.WindowContent.XamlRoot));
                innerSP3.IsTabStop = false;
            });

            Log.Comment("Hit Tab to focus non-focusable innerSP3 and focus should wrap back around to innerSP1");
            TestServices.KeyboardHelper.Tab();
            TestServices.WindowHelper.WaitForIdle();
            UIExecutor.Execute(() =>
            {
                Verify.AreEqual(innerSP1, FocusManager.GetFocusedElement(TestServices.WindowHelper.WindowContent.XamlRoot));
            });

            TestServices.WindowHelper.WaitForIdle();
        }

        [TestMethod]
        [TestProperty("Description", "Verify focus event order on UIElement")]
        public void VerifyFocusEventOrderOnUIElement()
        {
            string expectedString = "[innerSP1LosingFocus:1][FocusManagerLosingFocus][innerSP3GettingFocus][FocusManagerGettingFocus][innerSP1LostFocus][FocusManagerLostFocus][innerSP3GotFocus][FocusManagerGotFocus]";

            string rootPanelXaml = @"<StackPanel xmlns='http://schemas.microsoft.com/winfx/2006/xaml/presentation' xmlns:x='http://schemas.microsoft.com/winfx/2006/xaml'>
                            <StackPanel x:Name='innerSP1' IsTabStop='True'/>
                            <StackPanel x:Name='innerSP2' IsTabStop='False'/>
                            <StackPanel x:Name='innerSP3' IsTabStop='True'/>
                    </StackPanel>";

            StackPanel rootPanel = null;
            StackPanel innerSP1 = null;
            StackPanel innerSP2 = null;
            StackPanel innerSP3 = null;
            FrameworkElement[] elementList = null;
            StringBuilder eventOrder = null;

            UIExecutor.Execute(() =>
            {
                rootPanel = (StackPanel)XamlReader.Load(rootPanelXaml);
                innerSP1 = (StackPanel)rootPanel.FindName("innerSP1");
                innerSP2 = (StackPanel)rootPanel.FindName("innerSP2");
                innerSP3 = (StackPanel)rootPanel.FindName("innerSP3");
                eventOrder = new StringBuilder();
                elementList = new FrameworkElement[2] { innerSP1, innerSP3 };

                TestServices.WindowHelper.WindowContent = rootPanel;
            });
            TestServices.WindowHelper.WaitForIdle();
            FocusHelper.EnsureFocus(innerSP1, FocusState.Keyboard);

            UIExecutor.Execute(() =>
            {
                innerSP2.Focus(FocusState.Keyboard);
                // As innerSP2 is not focusable, Focus should not move..
                Verify.AreEqual(innerSP1, FocusManager.GetFocusedElement(TestServices.WindowHelper.WindowContent.XamlRoot));
            });

            using (var focusEventOrderTester = new FocusEventOrderingTester(elementList, eventOrder))
            {
                UIExecutor.Execute(() =>
                {
                    innerSP3.Focus(FocusState.Keyboard);
                    Verify.AreEqual(innerSP3, FocusManager.GetFocusedElement(TestServices.WindowHelper.WindowContent.XamlRoot));
                });

                TestServices.WindowHelper.WaitForIdle();
                Verify.AreEqual(expectedString, eventOrder.ToString());
            }

            TestServices.WindowHelper.WaitForIdle();
        }

        [TestMethod]
        [TestProperty("Description", "Validates TryMoveFocus and FindNextElement with FindElementOptions with invalid SearchRoot fails in islands/ desktop")]
        [TestProperty("Hosting:Mode", "WPF")]
        public void ValidateFocusApisWithInvalidSearchRootFailInWin32()
        {
            const string rootPanelXaml =
                    @"<StackPanel xmlns='http://schemas.microsoft.com/winfx/2006/xaml/presentation' xmlns:x='http://schemas.microsoft.com/winfx/2006/xaml'>
                        <StackPanel Orientation='Horizontal'>
                            <StackPanel Orientation='Vertical'>
                                <Button x:Name='buttonB' Content='B' Height='30' Width='30' Margin='0,120,0,0'/>
                            </StackPanel>
                            <StackPanel Orientation='Vertical'>
                                <Button x:Name='buttonA' Content='A' Height='90' Width='90'/>
                                <Button x:Name='buttonD' Content='D' Height='30' Width='30' Margin='30,90,0,0'/>
                            </StackPanel>
                            <StackPanel Orientation='Vertical'>
                                <Button x:Name='buttonC' Content='C' Height='30' Width='30' Margin='90,90,0,0'/>
                            </StackPanel>
                        </StackPanel>
                    </StackPanel>";

            StackPanel rootPanel = null;

            Button buttonA = null;
            Button buttonB = null;
            Button buttonC = null;
            Button buttonD = null;
            FindNextElementOptions navigationOptions = null;

            UIExecutor.Execute(() =>
            {
                rootPanel = (StackPanel)XamlReader.Load(rootPanelXaml);
                navigationOptions = new FindNextElementOptions();
                navigationOptions.SearchRoot = null;
                buttonA = (Button)rootPanel.FindName("buttonA");
                buttonB = (Button)rootPanel.FindName("buttonB");
                buttonC = (Button)rootPanel.FindName("buttonC");
                buttonD = (Button)rootPanel.FindName("buttonD");

                TestServices.WindowHelper.WindowContent = rootPanel;
            });

            TestServices.WindowHelper.WaitForIdle();
            FocusHelper.EnsureFocus(buttonA, FocusState.Keyboard);

            UIExecutor.Execute(() =>
            {
                Log.Comment("Trying Projection");
                navigationOptions.XYFocusNavigationStrategyOverride = XYFocusNavigationStrategyOverride.Projection;

                Log.Comment("Verifying TryMoveFocus fails in desktop with invalide SearchRoot in FindNextElementOptions");
                try
                {
                    FocusManager.TryMoveFocus(FocusNavigationDirection.Down, navigationOptions);
                    Verify.Fail("Exception was not thrown");
                }
                catch { }

                Log.Comment("Verifying FindNextElement fails in desktop with invalide SearchRoot in FindNextElementOptions ");
                try
                {
                    FocusManager.FindNextElement(FocusNavigationDirection.Next, navigationOptions);
                    Verify.Fail("Exception was not thrown");
                }
                catch { }
            });
            TestServices.WindowHelper.WaitForIdle();
        }

        [TestMethod]
        [TestProperty("Description", "Validates legacy api TryMoveFocus/ FindNextElement fails with an exception in islands/ desktop mode")]
        [TestProperty("Hosting:Mode", "WPF")]
        public void ValidateFocusApisWithoutFindNextElementOptionsFailInWin32()
        {
            const string rootPanelXaml =
                    @"<StackPanel xmlns='http://schemas.microsoft.com/winfx/2006/xaml/presentation' xmlns:x='http://schemas.microsoft.com/winfx/2006/xaml'>
                        <StackPanel Orientation='Horizontal'>
                            <StackPanel Orientation='Vertical'>
                                <Button x:Name='buttonB' Content='B' Height='30' Width='30' Margin='0,120,0,0'/>
                            </StackPanel>
                            <StackPanel Orientation='Vertical'>
                                <Button x:Name='buttonA' Content='A' Height='90' Width='90'/>
                                <Button x:Name='buttonD' Content='D' Height='30' Width='30' Margin='30,90,0,0'/>
                            </StackPanel>
                            <StackPanel Orientation='Vertical'>
                                <Button x:Name='buttonC' Content='C' Height='30' Width='30' Margin='90,90,0,0'/>
                            </StackPanel>
                        </StackPanel>
                    </StackPanel>";

            StackPanel rootPanel = null;

            Button buttonA = null;
            Button buttonB = null;
            Button buttonC = null;
            Button buttonD = null;
            Rect hint = new Rect();

            UIExecutor.Execute(() =>
            {
                rootPanel = (StackPanel)XamlReader.Load(rootPanelXaml);
                buttonA = (Button)rootPanel.FindName("buttonA");
                buttonB = (Button)rootPanel.FindName("buttonB");
                buttonC = (Button)rootPanel.FindName("buttonC");
                buttonD = (Button)rootPanel.FindName("buttonD");

                TestServices.WindowHelper.WindowContent = rootPanel;
            });

            TestServices.WindowHelper.WaitForIdle();
            FocusHelper.EnsureFocus(buttonA, FocusState.Keyboard);

            UIExecutor.Execute(() =>
            {
                Log.Comment("Verifying TryMoveFocus without FindNextElementOptions fails in desktop");
                try
                {
                    FocusManager.TryMoveFocus(FocusNavigationDirection.Down);
                    Verify.Fail("Exception was not thrown");
                }
                catch { }

                Log.Comment("Verifying FindNextElement without FindNextElementOptions fails in desktop");
                try
                {
                    FocusManager.FindNextElement(FocusNavigationDirection.Next);
                    Verify.Fail("Exception was not thrown");
                }
                catch { }

                Log.Comment("Verifying FindNextFocusableElement without FindNextElementOptions fails in desktop");
                try
                {
                    FocusManager.FindNextFocusableElement(FocusNavigationDirection.Next);
                    Verify.Fail("Exception was not thrown");
                }
                catch { }

                Log.Comment("Verifying FindNextFocusableElement without FindNextElementOptions fails in desktop");
                try
                {
                    FocusManager.FindNextFocusableElement(FocusNavigationDirection.Down, hint);
                    Verify.Fail("Exception was not thrown");
                }
                catch { }
            });
            TestServices.WindowHelper.WaitForIdle();
        }

        [TestMethod]
        [TestProperty("Description", "Make sure we can tab across a Control that overrides GetChildrenInTabFocusOrder")]
        [TestProperty("Hosting:Mode", "WPF")]
        public void VerifyTabOrderCorrectWhenUsingGetChildrenInTabFocusOrder()
        {
            const string rootPanelXaml =
                @"<StackPanel xmlns='http://schemas.microsoft.com/winfx/2006/xaml/presentation' xmlns:x='http://schemas.microsoft.com/winfx/2006/xaml'>
                    <StackPanel x:Name='innerPanel'>
                        <Button x:Name='buttonA'>buttonA</Button>
                    </StackPanel>
                    <Button x:Name='buttonC'>buttonC</Button>
                </StackPanel>";

            StackPanel rootPanel = null;
            StackPanel innerPanel = null;
            Button buttonA = null;
            Button buttonC = null;
            CustomControl customControl = null;

            UIExecutor.Execute(() =>
            {
                rootPanel = (StackPanel)XamlReader.Load(rootPanelXaml);
                innerPanel = (StackPanel)rootPanel.FindName("innerPanel");
                buttonA = (Button)rootPanel.FindName("buttonA");
                buttonC = (Button)rootPanel.FindName("buttonC");

                customControl = new CustomControl();
                customControl.Content = new Button() { Content="Ghost button"};
                customControl.IsTabStop = false;
                innerPanel.Children.Add(customControl);

                TestServices.WindowHelper.WindowContent = rootPanel;
            });
            TestServices.WindowHelper.WaitForIdle();

            FocusHelper.EnsureFocus(buttonA, FocusState.Keyboard);

            Log.Comment("Try to tab forward across the CustomControl");
            using (var buttonCGotFocus = new EventTester<Button, RoutedEventArgs>(buttonC, "GotFocus"))
            {
                TestServices.KeyboardHelper.Tab();
                buttonCGotFocus.Wait();
            }

            Log.Comment("Try to tab backward across the CustomControl");
            using (var buttonAGotFocus = new EventTester<Button, RoutedEventArgs>(buttonA, "GotFocus"))
            {
                TestServices.KeyboardHelper.ShiftTab();
                buttonAGotFocus.Wait();
            }
        }

    }

    public partial class CustomControl : ContentControl
    {
        protected override IEnumerable<DependencyObject> GetChildrenInTabFocusOrder()
        {
            Log.Comment("Return empty list from GetChildrenInTabFocusOrder");
            List<DependencyObject> list = new List<DependencyObject>();
            return list;
        }

    }
}
