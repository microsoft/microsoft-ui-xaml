// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

using System;
using System.Text;
using System.Linq;
using System.Threading.Tasks;
using System.Runtime.InteropServices;
using WEX.Logging.Interop;
using WEX.TestExecution;
using WEX.TestExecution.Markup;
using Windows.ApplicationModel.DataTransfer;

using Private.Infrastructure;

using WPFMarkup = System.Windows.Markup;

using Microsoft.UI.Xaml.Tests.Common;

using XamlControls = Microsoft.UI.Xaml.Controls;
using XamlMedia = Microsoft.UI.Xaml.Media;
using XamlMarkup = Microsoft.UI.Xaml.Markup;
using Microsoft.UI.Xaml.Controls;
using Microsoft.UI.Xaml.Documents;
using Microsoft.UI.Xaml.Input;
using Microsoft.UI.Xaml.Markup;
using Microsoft.UI.Xaml.Shapes;
using Windows.Foundation;
using System.Threading;
using System.Diagnostics;
using System.Collections.Generic;
using Microsoft.UI.Xaml.Controls.Primitives;
using Xaml = Microsoft.UI.Xaml;

using WPFControls = System.Windows.Controls;
using WPFSystem = System.Windows;
using System.Runtime.InteropServices.WindowsRuntime;
using System.Windows.Interop;

using Private.Infrastructure.Hosting.WPF;

namespace Microsoft.UI.Xaml.Tests.Hosting.Win32.WPF
{

    /*
     * Test Layout within stack panel for WPF Window
     * ----------------------------------------------
     *
     *                                      |-------------------|
     *                                      |  WPF  Button 1    |
     *                                      |-------------------|
     *
     *                                |-----------Xaml Island 1 --------|
     *                                |                                 |
     *                                |   |------------------------|    |
     *                                |   |  XAML Button 1         |    |
     *                                |   |------------------------|    |
     *                                |                                 |
     *                                |---------------------------------|
     *                                      |-------------------|
     *                                      |  WPF  Button 2    |
     *                                      |-------------------|
     *
     *                                |-----------Xaml Island 2 --------|
     *                                |                                 |
     *                                |   |------------------------|    |
     *                                |   |  XAML Button 2         |    |
     *                                |   |------------------------|    |
     *                                |                                 |
     *                                |   |------------------------|    |
     *                                |   |  XAML Button 3         |    |
     *                                |   |------------------------|    |
     *                                |                                 |
     *                                |---------------------------------|
     *
     */

    [TestClass]
    public class WPFTests : XamlTestsBase
    {
        [ClassInitialize]
        [TestProperty("BinaryUnderTest", "Microsoft.UI.Xaml.dll")]
        [TestProperty("ArtifactUnderTest", "sdk\\inc\\Microsoft.UI.Xaml.hosting.desktopwindowxamlsource.idl")]
        [TestProperty("UAP:AppXManifest", AppxManifests.WINDOWS_VERSION_CURRENT_CENTENNIAL)]
        [TestProperty("UAP:Host", "PackagedCwa")]
        [TestProperty("ThreadingModel", "STA")]
        [TestProperty("RunAs", "UAP")]
        [TestProperty("Hosting:Mode", "WPF")]
        [TestProperty("Classification", "Integration")]
        public static void Setup(TestContext context)
        {
            AssemblySetup.CommonTestClassSetup();
        }
        
        [ClassCleanup]
        public void Cleanup()
        {
            TestServices.WindowHelper.DetachMemoryManagerEvents();
            base.CommonClassCleanup();
        }

        [TestMethod]
        [TestProperty("Description", "Validates tab key input works and moves the focus")]
        [TestProperty("Ignore", "TRUE")]    // Focus engagement bugs in lifted islands
        public async Task ValidateFocusBehaviorWithMultipleIsland()
        {
            // Disable keyboard input wait as WPF is in charge here.
            using (TestServices.KeyboardHelper.CreateKeyboardWaitKindGuard(KeyboardWaitKind.None))
            {
                var host = TestServices.Win32Host as WPFHost;
                Verify.IsNotNull(host);
                Verify.IsNotNull(host.MainWindow);

                WPFControls.StackPanel container = null;
                WPFControls.Button wpfButton1 = null;
                WPFControls.Button wpfButton2 = null;
                WindowsXamlHostWrapper xamlHost1 = null;
                WindowsXamlHostWrapper xamlHost2 = null;
                WindowsXamlHostWrapper xamlHost3 = null;
                Xaml.Controls.Button xamlButton1 = null;
                Xaml.Controls.Button xamlButton2 = null;
                Xaml.Controls.Button xamlButton3 = null;
                Xaml.Controls.StackPanel xamlStackPanel = null;
                Xaml.Controls.Canvas xamlCanvas = null;
                Xaml.Controls.Button xamlButton4 = null;

                const string rootPanelXaml =
                @"<StackPanel xmlns='http://schemas.microsoft.com/winfx/2006/xaml/presentation' xmlns:x='http://schemas.microsoft.com/winfx/2006/xaml'>
                    <Button x:Name='xamlButton2' Content='XAML Button 2'/>
                    <Button x:Name='xamlButton3' Content='XAML Button 3'/>
                  </StackPanel>";

                host.MainWindow.Dispatcher.Invoke(() =>
                {
                    container = new WPFControls.StackPanel();
                    var children = container.Children;

                    // WPF Button
                    wpfButton1 = new WPFControls.Button()
                    {
                        Height = 30,
                        Width = 100,
                        Name = "wpfButton1",
                        Content = "WPF Button 1",
                    };
                    children.Add(wpfButton1);

                    // Instantiate Xaml Button and put it into the Xaml source
                    xamlButton1 = new Xaml.Controls.Button()
                    {
                        Height = 45,
                        Width = 150,
                        Name = "xamlButton1",
                        Content = new Xaml.Controls.TextBlock()
                        {
                            Text = "XAML Button 1",
                        },
                    };
                    // Xaml Source
                    xamlHost1 = new WindowsXamlHostWrapper()
                    {
                        Height = 50,
                        Width = 200,
                        Content = xamlButton1,
                    };
                    // Add Xaml source into the stackpanel children
                    xamlHost1.InsertInto(children);

                    // WPF Button
                    wpfButton2 = new WPFControls.Button()
                    {
                        Height = 30,
                        Width = 100,
                        Name = "wpfButton2",
                        Content = "WPF Button 2",
                    };
                    children.Add(wpfButton2);

                    xamlStackPanel = (StackPanel)XamlMarkup.XamlReader.Load(rootPanelXaml);
                    xamlButton2 = (Button)xamlStackPanel.FindName("xamlButton2");
                    xamlButton3 = (Button)xamlStackPanel.FindName("xamlButton3");

                    // Xaml Source
                    xamlHost2 = new WindowsXamlHostWrapper()
                    {
                        Height = 100,
                        Width = 200,
                        Content = xamlStackPanel,
                    };
                    // Add Xaml source into the stackpanel children
                    xamlHost2.InsertInto(children);

                    // Create another XAML host with a Canvas at the root so the island doesn't have a RootScrollViewer
                    xamlButton4 = new Xaml.Controls.Button()
                    {
                        Height = 45,
                        Width = 150,
                        Name = "xamlButton4",
                        Content = new Xaml.Controls.TextBlock()
                        {
                            Text = "XAML Button 4",
                        },
                    };
                    xamlCanvas = new Xaml.Controls.Canvas();
                    xamlCanvas.Children.Add(xamlButton4);

                    // Xaml Source
                    xamlHost3 = new WindowsXamlHostWrapper()
                    {
                        Height = 50,
                        Width = 200,
                        Content = xamlCanvas,
                    };
                    // Add Xaml source into the stackpanel children
                    xamlHost3.InsertInto(children);

                    host.MainWindow.Content = container;
                });
                TestServices.WindowHelper.WaitForIdle();

                using(new TestCleanupWrapper())
                {
                    // Set initial focus on Wpf button
                    await host.MainWindow.Dispatcher.InvokeAsync(async () =>
                    {
                        await wpfButton1.FocusAndWaitAsync();
                    });

                    // Verify tab moves focus to next Xaml button 1
                    TestServices.KeyboardHelper.Tab();
                    TestServices.WindowHelper.WaitForIdle();
                    host.MainWindow.Dispatcher.Invoke(() =>
                    {
                        Verify.AreEqual(xamlButton1, Xaml.Input.FocusManager.GetFocusedElement(xamlButton1.XamlRoot));
                        Log.Comment("Focus is on " + xamlButton1.Name);
                    });

                    // Verify tab moves focus to next Wpf button 2
                    TestServices.KeyboardHelper.Tab();
                    TestServices.WindowHelper.WaitForIdle();

                    host.MainWindow.Dispatcher.Invoke(() =>
                    {
                        Verify.IsTrue(wpfButton2.IsFocused);
                        Log.Comment("Focus is on " + wpfButton2.Name);
                    });

                    // Verify tab moves focus to Xaml button 2 across the Island
                    TestServices.KeyboardHelper.Tab();
                    TestServices.WindowHelper.WaitForIdle();
                    host.MainWindow.Dispatcher.Invoke(() =>
                    {
                        Verify.AreEqual(xamlButton2, Xaml.Input.FocusManager.GetFocusedElement(xamlButton2.XamlRoot));
                        Log.Comment("Focus is on " + xamlButton2.Name);
                    });

                    // Verify tab moves focus to Xaml button 3 within the Island
                    TestServices.KeyboardHelper.Tab();
                    TestServices.WindowHelper.WaitForIdle();
                    host.MainWindow.Dispatcher.Invoke(() =>
                    {
                        Verify.AreEqual(xamlButton3, Xaml.Input.FocusManager.GetFocusedElement(xamlButton3.XamlRoot));
                        Log.Comment("Focus is on " + xamlButton3.Name);
                    });

                    // Verify tab moves focus to Xaml button 4 within the next Island
                    TestServices.KeyboardHelper.Tab();
                    TestServices.WindowHelper.WaitForIdle();
                    host.MainWindow.Dispatcher.Invoke(() =>
                    {
                        Verify.AreEqual(xamlButton4, Xaml.Input.FocusManager.GetFocusedElement(xamlButton4.XamlRoot));
                        Log.Comment("Focus is on " + xamlButton4.Name);
                    });

                    // Verify tab makes focus to complete the cyle and comeback to Wpf button 1
                    TestServices.KeyboardHelper.Tab();
                    TestServices.WindowHelper.WaitForIdle();

                    host.MainWindow.Dispatcher.Invoke(() =>
                    {
                        Verify.IsTrue(wpfButton1.IsFocused);
                        Log.Comment("Focus is back on " + wpfButton1.Name);
                    });

                    // Verify shift+tab makes focus to cycle back to last Xaml button 4
                    TestServices.KeyboardHelper.ShiftTab();
                    TestServices.WindowHelper.WaitForIdle();
                    host.MainWindow.Dispatcher.Invoke(() =>
                    {
                        Verify.AreEqual(xamlButton4, Xaml.Input.FocusManager.GetFocusedElement(xamlButton4.XamlRoot));
                        Log.Comment("Focus is on " + xamlButton4.Name);
                    });

                    // Verify shift+tab makes focus to cycle back to last Xaml button 3
                    TestServices.KeyboardHelper.ShiftTab();
                    TestServices.WindowHelper.WaitForIdle();
                    host.MainWindow.Dispatcher.Invoke(() =>
                    {
                        Verify.AreEqual(xamlButton3, Xaml.Input.FocusManager.GetFocusedElement(xamlButton3.XamlRoot));
                        Log.Comment("Focus is on " + xamlButton3.Name);
                    });

                    // Verify shift+tab moves focus back to Xaml button 2 within the Island
                    TestServices.KeyboardHelper.ShiftTab();
                    TestServices.WindowHelper.WaitForIdle();

                    host.MainWindow.Dispatcher.Invoke(() =>
                    {
                        Verify.AreEqual(xamlButton2, Xaml.Input.FocusManager.GetFocusedElement(xamlButton2.XamlRoot));
                        Log.Comment("Focus is on " + xamlButton2.Name);
                    });

                    TestServices.KeyboardHelper.ShiftTab();
                    TestServices.WindowHelper.WaitForIdle();

                    host.MainWindow.Dispatcher.Invoke(() =>
                    {
                        Verify.IsTrue(wpfButton2.IsFocused);
                        Log.Comment("Focus is on " + wpfButton2.Name);
                    });

                    // Verify shift+tab moves focus back to Xaml button 1 out side the Island
                    TestServices.KeyboardHelper.ShiftTab();
                    TestServices.WindowHelper.WaitForIdle();
                    host.MainWindow.Dispatcher.Invoke(() =>
                    {
                        Verify.AreEqual(xamlButton1, Xaml.Input.FocusManager.GetFocusedElement(xamlButton1.XamlRoot));
                        Log.Comment("Focus is on " + xamlButton1.Name);
                    });

                    // Verify shift+tab moves the focus back to the starting point Wpf Button 1
                    TestServices.KeyboardHelper.ShiftTab();
                    TestServices.WindowHelper.WaitForIdle();

                    host.MainWindow.Dispatcher.Invoke(() =>
                    {
                        Verify.IsTrue(wpfButton1.IsFocused);
                        Log.Comment("Focus is back on " + wpfButton1.Name);
                    });

                    TestServices.WindowHelper.WaitForIdle();

                    // Note: We must dispose our inner xaml contents before setting WPF's content to
                    // null. Setting WPF's content to null will disconnect the HWND tree which is 
                    // not allowed for a non-disposed DesktopWindowXamlSource object.
                    xamlHost1.Dispose();
                    xamlHost2.Dispose();
                    xamlHost3.Dispose();

                    host.MainWindow.Dispatcher.Invoke(() =>
                    {
                        host.MainWindow.Content = null;
                    });
                }
            }
        }

        [TestMethod]
        [TestProperty("Description", "Validates typing works when switching focus between WPF TextBox and XAML TextBox across multiple islands")]
        [TestProperty("Ignore", "TRUE")] // Programmatically moving focus between elements in different Xaml Islands does not work well.
        public async Task ValidateTextInputAfterFocusChangeWithMultipleIslands()
        {
            // Disable keyboard input wait as WPF is in charge here.
            using (TestServices.KeyboardHelper.CreateKeyboardWaitKindGuard(KeyboardWaitKind.None))
            {
                var host = TestServices.Win32Host as WPFHost;
                Verify.IsNotNull(host);
                Verify.IsNotNull(host.MainWindow);

                WPFControls.StackPanel container = null;
                WPFControls.TextBox wpfTextBox1 = null;
                WindowsXamlHostWrapper xamlHost1 = null;
                WindowsXamlHostWrapper xamlHost2 = null;
                Xaml.Controls.TextBox xamlTextBox1 = null;
                Xaml.Controls.TextBox xamlTextBox2 = null;
                Xaml.Controls.StackPanel xamlStackPanel = null;

                const string rootPanelXaml =
                @"<StackPanel xmlns='http://schemas.microsoft.com/winfx/2006/xaml/presentation' xmlns:x='http://schemas.microsoft.com/winfx/2006/xaml'>
                    <TextBox x:Name='xamlTextBox2'/>
                  </StackPanel>";

                host.MainWindow.Dispatcher.Invoke(() =>
                {
                    container = new WPFControls.StackPanel();
                    var children = container.Children;

                    // WPF TextBox
                    wpfTextBox1 = new WPFControls.TextBox()
                    {
                        Height = 30,
                        Width = 100,
                        Name = "wpfTextBox1",
                    };
                    children.Add(wpfTextBox1);

                    // Instantiate Xaml TextBox and put it into the Xaml source
                    xamlTextBox1 = new Xaml.Controls.TextBox()
                    {
                        Height = 45,
                        Width = 150,
                        Name = "xamlTextBox1",
                    };
                    // Xaml Source
                    xamlHost1 = new WindowsXamlHostWrapper()
                    {
                        Height = 50,
                        Width = 200,
                        Content = xamlTextBox1,
                    };
                    // Add Xaml source into the stackpanel children
                    xamlHost1.InsertInto(children);

                    xamlStackPanel = (StackPanel)XamlMarkup.XamlReader.Load(rootPanelXaml);
                    xamlTextBox2 = (TextBox)xamlStackPanel.FindName("xamlTextBox2");

                    // Xaml Source
                    xamlHost2 = new WindowsXamlHostWrapper()
                    {
                        Height = 100,
                        Width = 200,
                        Content = xamlStackPanel,
                    };
                    // Add Xaml source into the stackpanel children
                    xamlHost2.InsertInto(children);

                    host.MainWindow.Content = container;
                });
                TestServices.WindowHelper.WaitForIdle();

                using(xamlHost1)
                using(xamlHost2)
                using(new TestCleanupWrapper())
                {
                    // Set initial focus on Wpf TextBox
                    await host.MainWindow.Dispatcher.InvokeAsync(async () =>
                    {
                        await wpfTextBox1.FocusAndWaitAsync();
                    });

                    // Try to type on WPF TextBox
                    TestServices.KeyboardHelper.PressKeySequence("ab");
                    TestServices.WindowHelper.WaitForIdle();
                    host.MainWindow.Dispatcher.Invoke(() =>
                    {
                        Log.Comment("Typing 'ab' in WPF TextBox");
                        Verify.AreEqual(wpfTextBox1.Text, "ab");
                    });

                    FocusHelper.EnsureFocus(xamlTextBox1, FocusState.Keyboard);

                    TestServices.KeyboardHelper.PressKeySequence("12");
                    TestServices.WindowHelper.WaitForIdle();
                    host.MainWindow.Dispatcher.Invoke(() =>
                    {
                        Log.Comment("Typing '12' in XAML island1 TextBox");
                        Verify.AreEqual(xamlTextBox1.Text, "12");
                    });

                    host.MainWindow.Dispatcher.Invoke(() =>
                    {
                        wpfTextBox1.Focus();
                    });

                    TestServices.KeyboardHelper.PressKeySequence("cd");
                    TestServices.WindowHelper.WaitForIdle();
                    host.MainWindow.Dispatcher.Invoke(() =>
                    {
                        Log.Comment("Typing 'cd' in WPF TextBox");
                        Verify.AreEqual(wpfTextBox1.Text, "abcd");
                    });

                    TestServices.KeyboardHelper.Tab();
                    TestServices.WindowHelper.WaitForIdle();

                    TestServices.KeyboardHelper.PressKeySequence("34");
                    TestServices.WindowHelper.WaitForIdle();
                    host.MainWindow.Dispatcher.Invoke(() =>
                    {
                        Log.Comment("Typing '34' in XAML island1 TextBox");
                        Verify.AreEqual(xamlTextBox1.Text, "1234");
                    });

                    FocusHelper.EnsureFocus(xamlTextBox2, FocusState.Keyboard);

                    TestServices.KeyboardHelper.PressKeySequence("AB");
                    TestServices.WindowHelper.WaitForIdle();
                    host.MainWindow.Dispatcher.Invoke(() =>
                    {
                        Log.Comment("Typing 'AB' in XAML island2 TextBox");
                        Verify.AreEqual(xamlTextBox2.Text, "AB");
                    });

                    FocusHelper.EnsureFocus(xamlTextBox1, FocusState.Keyboard);

                    TestServices.KeyboardHelper.PressKeySequence("56");
                    TestServices.WindowHelper.WaitForIdle();
                    host.MainWindow.Dispatcher.Invoke(() =>
                    {
                        Log.Comment("Typing '56' in XAML island1 TextBox");
                        Verify.AreEqual(xamlTextBox1.Text, "123456");
                    });
                    TestServices.WindowHelper.WaitForIdle();

                    FocusHelper.EnsureFocus(xamlTextBox2, FocusState.Keyboard);

                    TestServices.KeyboardHelper.PressKeySequence("CD");
                    TestServices.WindowHelper.WaitForIdle();
                    host.MainWindow.Dispatcher.Invoke(() =>
                    {
                        Log.Comment("Typing 'CD' in XAML island2 TextBox");
                        Verify.AreEqual(xamlTextBox2.Text, "ABCD");
                    });

                    TestServices.WindowHelper.WaitForIdle();
                }
            }
        }

        [DllImport("user32.dll")]
        public static extern IntPtr GetDesktopWindow();

        // Disabled: DCPP: Allow ability to create Window off the UI thread
        // [TestMethod]
        // [TestProperty("Description", "Verify DesktopWindowXamlSource can not be attached to a window created on different thread.")]
        public void AttachDesktopWindowXamlSourceAndWindowOnDifferentThreads()
        {
            var host = TestServices.Win32Host as WPFHost;
            Verify.IsNotNull(host);
            Verify.IsNotNull(host.MainWindow);
            IntPtr desktopWndHandle = GetDesktopWindow();

            host.MainWindow.Dispatcher.Invoke(() =>
            {
                Exception caughtException = VerifyExtensions.Throws<Exception>(() =>
                {
                    using (Xaml.Hosting.DesktopWindowXamlSource source = new Xaml.Hosting.DesktopWindowXamlSource())
                    {
                        source.Initialize(global::Private.Infrastructure.Hosting.WPF.Interop.GetWindowIdFromWindow(desktopWndHandle));
                    }
                });
                Verify.IsTrue(caughtException.Message.Contains("https://go.microsoft.com/fwlink/?linkid=875496"));
            });

            TestServices.WindowHelper.WaitForIdle();
        }

        // Disabled: DCPP: Allow ability to create Window off the UI thread
        // [TestMethod]
        // [TestProperty("Description", "Verify two different DesktopWindowXamlSource instances can be used on different top-level windows")]
        public void AttachDesktopWindowXamlSourceToDifferentTopLevelWindow()
        {
            var host = TestServices.Win32Host as WPFHost;
            Verify.IsNotNull(host);
            Verify.IsNotNull(host.MainWindow);

            WindowsXamlHostWrapper xamlHost = null;
            host.MainWindow.Dispatcher.Invoke(() =>
            {
                var container = new WPFControls.StackPanel();
                host.MainWindow.Content = container;
                var children = container.Children;
                xamlHost = new WindowsXamlHostWrapper()
                {
                    Height = 50,
                    Width = 200,
                    Content = new Xaml.Controls.Button()
                    {
                        Height = 45,
                        Width = 150,
                        Name = "xamlButton1",
                        Content = new Xaml.Controls.TextBlock()
                        {
                            Text = "XAML Button 1",
                        },
                    },
                };
                xamlHost.InsertInto(children);
            });

            TestServices.WindowHelper.WaitForIdle();

            Xaml.Hosting.DesktopWindowXamlSource secondaryDwxs = null;
            WPFSystem.Window secondaryWindow = null;

            host.MainWindow.Dispatcher.Invoke(() =>
            {
                secondaryDwxs = new Xaml.Hosting.DesktopWindowXamlSource();

                secondaryWindow = new WPFSystem.Window();
                secondaryWindow.Show();

                var wpfInterop = new WindowInteropHelper(secondaryWindow);
                var secondaryWndHandle = wpfInterop.Handle;

                secondaryDwxs.Initialize(global::Private.Infrastructure.Hosting.WPF.Interop.GetWindowIdFromWindow(secondaryWndHandle));
            });

            TestServices.WindowHelper.WaitForIdle();

            host.MainWindow.Dispatcher.Invoke(() =>
            {
                secondaryWindow.Close();
                host.MainWindow.Activate();
                secondaryDwxs.Dispose();
                secondaryDwxs = null;
            });

            TestServices.WindowHelper.WaitForIdle();

            host.MainWindow.Dispatcher.Invoke(() =>
            {
                xamlHost.Dispose();
                xamlHost = null;
                host.MainWindow.Content = null;   
            });
        }

        [TestMethod]
        [TestProperty("Description", "Verify framework element unloaded event fired when Desktop Window Xaml Source content is set to null")]
        public async Task VerifyFrameworkElementUnloadedEventOnSetContentToNull()
        {
            var host = TestServices.Win32Host as WPFHost;
            Verify.IsNotNull(host);
            Verify.IsNotNull(host.MainWindow);

            WPFControls.StackPanel container = null;
            WindowsXamlHostWrapper xamlHost1 = null;

            Xaml.Controls.Button xamlButton = null;
            Xaml.Controls.StackPanel xamlStackPanel = null;
            const string rootPanelXaml =
                @"<StackPanel xmlns='http://schemas.microsoft.com/winfx/2006/xaml/presentation' xmlns:x='http://schemas.microsoft.com/winfx/2006/xaml'>
                    <Button x:Name='Button' Content='Button'/>
                  </StackPanel>";

            host.MainWindow.Dispatcher.Invoke(() =>
            {
                container = new WPFControls.StackPanel();
                var children = container.Children;

                xamlStackPanel = (StackPanel)XamlMarkup.XamlReader.Load(rootPanelXaml);
                xamlButton = (Button)xamlStackPanel.FindName("Button");

                // Xaml Source
                xamlHost1 = new WindowsXamlHostWrapper()
                {
                    Height = 100,
                    Width = 200,
                    Content = xamlStackPanel,
                };
                // Add Xaml source into the stackpanel children
                xamlHost1.InsertInto(children);

                host.MainWindow.Content = container;
            });
            TestServices.WindowHelper.WaitForIdle();

            AutoResetEvent xamlButtonUnloadedHasFired = new AutoResetEvent(false);

            await host.MainWindow.Dispatcher.InvokeAsync(async () =>
            {
                await xamlButton.WaitFrameworkElementUnloadedAsync();
                xamlButtonUnloadedHasFired.Set();
            });

            host.MainWindow.Dispatcher.Invoke(() =>
            {
                xamlHost1.Content = null;
            });

            Verify.IsTrue(xamlButtonUnloadedHasFired.WaitOne(TimeSpan.FromSeconds(1)));

            xamlHost1.Dispose();
        }

        [TestMethod]
        [TestProperty("Description", "Verify framework element unloaded event fired when Desktop Windows Xaml Source is closed")]
        public async Task VerifyFrameworkElementUnloadedEventOnXamlHostClosed()
        {

            var host = TestServices.Win32Host as WPFHost;
            Verify.IsNotNull(host);
            Verify.IsNotNull(host.MainWindow);

            WPFControls.StackPanel container = null;
            WindowsXamlHostWrapper xamlHost1 = null;

            Xaml.Controls.StackPanel xamlStackPanel = null;

            host.MainWindow.Dispatcher.Invoke(() =>
            {
                container = new WPFControls.StackPanel();
                var children = container.Children;

                xamlStackPanel = new Xaml.Controls.StackPanel();

                // Xaml Source
                xamlHost1 = new WindowsXamlHostWrapper()
                {
                    Height = 100,
                    Width = 200,
                    Content = xamlStackPanel,
                };
                // Add Xaml source into the stackpanel children
                xamlHost1.InsertInto(children);

                host.MainWindow.Content = container;

            });
            TestServices.WindowHelper.WaitForIdle();

            AutoResetEvent xamlStackPanelUnloadedHasFired = new AutoResetEvent(false);

            await host.MainWindow.Dispatcher.InvokeAsync(async () =>
            {
                await xamlStackPanel.WaitFrameworkElementUnloadedAsync();
                xamlStackPanelUnloadedHasFired.Set();
            });

            host.MainWindow.Dispatcher.Invoke(() =>
            {
                xamlHost1.Dispose();
            });

            Verify.IsTrue(xamlStackPanelUnloadedHasFired.WaitOne(TimeSpan.FromSeconds(1)));
            TestServices.WindowHelper.WaitForIdle();
        }

        [TestMethod]
        [TestProperty("Description", "PLMHandler does not exists in WPF. Verify NULL PLMHandler access does not throw exception.")]
        public void VerifyPLMHandlerNoException()
        {
            var host = TestServices.Win32Host as WPFHost;
            Verify.IsNotNull(host);
            Verify.IsNotNull(host.MainWindow);

            SuspendingEventHandler suspendingHandler = (o, a) => { };
            EventHandler<object> resumingHandler = (o, a) => { };
            LeavingBackgroundEventHandler LeavingBackgroundHandler = (o, a) => { };
            EnteredBackgroundEventHandler EnteredBackgroundHandler = (o, a) => { };

            host.MainWindow.Dispatcher.Invoke(() =>
            {
                try
                {
                    Application.Current.Suspending += suspendingHandler;
                    Application.Current.Resuming += resumingHandler;
                    Application.Current.LeavingBackground += LeavingBackgroundHandler;
                    Application.Current.EnteredBackground += EnteredBackgroundHandler;
                }
                finally
                {
                    Application.Current.Suspending -= suspendingHandler;
                    Application.Current.Resuming -= resumingHandler;
                    Application.Current.LeavingBackground -= LeavingBackgroundHandler;
                    Application.Current.EnteredBackground -= EnteredBackgroundHandler;
                }
            });
        }

        [TestMethod]
        [TestProperty("Description", "Ensure that clicking on a TextBox always focuses the island")]
        [TestProperty("Ignore", "TRUE")] 
        public async Task ValidateTextBoxReceivesFocus()
        {
            // Disable keyboard input wait as WPF is in charge here.
            using (TestServices.KeyboardHelper.CreateKeyboardWaitKindGuard(KeyboardWaitKind.None))
            {
                var host = TestServices.Win32Host as WPFHost;

                WPFControls.StackPanel container = null;
                WPFControls.TextBox wpfTextBox = null;
                WindowsXamlHostWrapper xamlHost = null;
                Xaml.Controls.StackPanel xamlStackPanel = null;
                Xaml.Controls.TextBox xamlTextBox = null;

                const string rootPanelXaml =
                @"<StackPanel xmlns='http://schemas.microsoft.com/winfx/2006/xaml/presentation' xmlns:x='http://schemas.microsoft.com/winfx/2006/xaml'>
                    <Button x:Name='xamlButton2' Content='XAML Button 2'/>
                    <TextBox x:Name='xamlTextBox'/>
                    <Button x:Name='xamlButton3' Content='XAML Button 3'/>
                  </StackPanel>";

                host.MainWindow.Dispatcher.Invoke(() =>
                {
                    container = new WPFControls.StackPanel();
                    var children = container.Children;

                    wpfTextBox = new WPFControls.TextBox()
                    {
                        Height = 30,
                        Width = 100,
                        Name = "wpfTextBox",
                    };
                    children.Add(wpfTextBox);

                    xamlStackPanel = (StackPanel)XamlMarkup.XamlReader.Load(rootPanelXaml);
                    xamlTextBox = (TextBox)xamlStackPanel.FindName("xamlTextBox");

                    // Xaml Source
                    xamlHost = new WindowsXamlHostWrapper()
                    {
                        Height = 100,
                        Width = 200,
                        Content = xamlStackPanel,
                    };
                    // Add Xaml source into the stackpanel children
                    xamlHost.InsertInto(children);

                    host.MainWindow.Content = container;
                });
                TestServices.WindowHelper.WaitForIdle();

                // Set initial focus on wpfTextBox
                await host.MainWindow.Dispatcher.InvokeAsync(async () =>
                {
                   await wpfTextBox.FocusAndWaitAsync();
                });

                // Hard-code these coordinates because InputHelper doesn't know where the island is in screen space
                var wpfTextBoxPos = new global::Windows.Foundation.Point(515, 20);
                var xamlTextBoxPos = new global::Windows.Foundation.Point(515, 80);

                TestServices.InputHelper.ClickMouseButton(MouseButton.Left, xamlTextBoxPos);
                TestServices.WindowHelper.WaitForIdle();
                host.MainWindow.Dispatcher.Invoke(() =>
                {
                    Verify.AreEqual(xamlTextBox, Xaml.Input.FocusManager.GetFocusedElement(xamlTextBox.XamlRoot));
                    Log.Comment("Focus is on " + xamlTextBox.Name);
                });

                // Set focus on wpf button again by clicking
                TestServices.InputHelper.ClickMouseButton(MouseButton.Left, wpfTextBoxPos);
                TestServices.WindowHelper.WaitForIdle();

                TestServices.KeyboardHelper.PressKeySequence("lorem");
                TestServices.WindowHelper.WaitForIdle();

                host.MainWindow.Dispatcher.Invoke(() =>
                {
                    Verify.AreEqual("lorem", wpfTextBox.Text);
                    Verify.AreEqual("", xamlTextBox.Text);
                });

                TestServices.InputHelper.ClickMouseButton(MouseButton.Left, xamlTextBoxPos);
                TestServices.WindowHelper.WaitForIdle();

                TestServices.KeyboardHelper.PressKeySequence("ipsum");
                TestServices.WindowHelper.WaitForIdle();

                host.MainWindow.Dispatcher.Invoke(() =>
                {
                    Verify.AreEqual(xamlTextBox, Xaml.Input.FocusManager.GetFocusedElement(xamlTextBox.XamlRoot));
                    Log.Comment("Focus is on " + xamlTextBox.Name);

                    Verify.AreEqual("lorem", wpfTextBox.Text);
                    Verify.AreEqual("ipsum", xamlTextBox.Text);
                });

                TestServices.WindowHelper.WaitForIdle();

                xamlHost.Dispose();
            }
        }

        private void RunTests(string moduleName, string testName)
        {
            string strCmdLine = "TE.EXE ";
            strCmdLine += ".\\Test\\";
            strCmdLine += moduleName;
            strCmdLine += " /select:\"(@Name='*";
            strCmdLine += testName;
            strCmdLine += "*')\" ";
            strCmdLine += " /enablewttlogging /appendwttlogging /miniDumpOnCrash /miniDumpOnError /screenCaptureOnError ";
            if (Debugger.IsAttached)
            {
                strCmdLine += " /p:WaitForDebugger=1 ";
            }

            Log.Comment("Running UIA tests...");
            Log.Comment($"{strCmdLine}");
            uint exitCode = 0;
            TestServices.Utilities.RunCommandLine(strCmdLine, out exitCode);
            Log.Comment("Running UIA tests done");
            Verify.AreEqual((int)exitCode, 0);
        }

        [TestMethod]
        [TestProperty("Description", "Verify that pressing alt moves focus away from xaml island to the menu bar of WPF")]
        public async Task VerifyWPFMenuFocusedWithAltKeypress()
        {
            // Disable keyboard input wait as WPF is in charge here.
            using (TestServices.KeyboardHelper.CreateKeyboardWaitKindGuard(KeyboardWaitKind.None))
            {
                var host = TestServices.Win32Host as WPFHost;
                Verify.IsNotNull(host);
                Verify.IsNotNull(host.MainWindow);

                WPFControls.StackPanel container = null;
                WPFControls.MenuItem menuItem1 = null;
                WPFControls.Menu menu = null;
                WindowsXamlHostWrapper xamlHost1 = null;
                Xaml.Controls.Button xamlButton1 = null;

                host.MainWindow.Dispatcher.Invoke(() =>
                {
                    container = new WPFControls.StackPanel();
                    var children = container.Children;

                    menu = new WPFControls.Menu()
                    {
                        VerticalAlignment = WPFSystem.VerticalAlignment.Top,
                        Name = "mainMenu"
                    };

                    menuItem1 = new WPFControls.MenuItem()
                    {
                        Header = "item1"
                    };
                    menu.Items.Add(menuItem1);
                    children.Add(menu);

                    // Instantiate Xaml Button and put it into the Xaml source
                    xamlButton1 = new Xaml.Controls.Button()
                    {
                        Height = 45,
                        Width = 150,
                        Name = "xamlButton1",
                        Content = new Xaml.Controls.TextBlock()
                        {
                            Text = "XAML Button 1",
                        },
                    };

                    // Xaml Source
                    xamlHost1 = new WindowsXamlHostWrapper()
                    {
                        Height = 50,
                        Width = 200,
                        Content = xamlButton1,

                    };
                    // Add Xaml source into the stackpanel children
                    xamlHost1.InsertInto(children);
                    host.MainWindow.Content = container;
                });
                TestServices.WindowHelper.WaitForIdle();
                FocusHelper.EnsureFocus(xamlButton1, FocusState.Keyboard);

                // when focus is on xaml island, pressing alt should move focus to the menu bar
                TestServices.KeyboardHelper.Alt();
                TestServices.WindowHelper.WaitForIdle();

                await host.MainWindow.Dispatcher.Invoke(async () =>
                {
                    await menuItem1.WaitGotFocusAsync();
                });

                xamlHost1.Dispose();
            }
        }

        [TestMethod]
        [TestProperty("Description", "Validates DWXS ProcessKeyboardAccelerator API")]
        [TestProperty("Ignore", "TRUE")] // TODO: Re-enable when the accelerator processing 
                                         // for islands in WPF is aligned with Xaml Desktop apps
        public async Task ValidateDWXSProcessKeyboardAcceleratorAPI()
        {
            var host = TestServices.Win32Host as WPFHost;
            Verify.IsNotNull(host);
            Verify.IsNotNull(host.MainWindow);

            WPFControls.StackPanel container = null;
            WPFControls.Button wpfButton1 = null;

            WindowsXamlHostWrapper xamlHost = null;
            Xaml.Controls.StackPanel xamlStackPanel = null;
            Xaml.Controls.Button xamlButton1 = null;
            Xaml.Input.KeyboardAccelerator ctrlAAccelerator = null;

            const string rootPanelXaml =
                @"<StackPanel xmlns='http://schemas.microsoft.com/winfx/2006/xaml/presentation' xmlns:x='http://schemas.microsoft.com/winfx/2006/xaml'>
                    <Button x:Name='xamlButton1' Content='XAML Button 1'>
                        <Button.KeyboardAccelerators>
                            <KeyboardAccelerator x:Name='keyboardAccelerator' Modifiers='Control' Key='A' />
                        </Button.KeyboardAccelerators>
                    </Button>
                </StackPanel>";

            host.MainWindow.Dispatcher.Invoke(() =>
            {
                container = new WPFControls.StackPanel();
                var children = container.Children;

                // WPF Button
                wpfButton1 = new WPFControls.Button()
                {
                    Height = 30,
                    Width = 100,
                    Name = "wpfButton1",
                    Content = "WPF Button 1",
                };
                children.Add(wpfButton1);

                xamlStackPanel = (Xaml.Controls.StackPanel)XamlMarkup.XamlReader.Load(rootPanelXaml);
                xamlButton1 = (Xaml.Controls.Button)xamlStackPanel.FindName("xamlButton1");
                ctrlAAccelerator = (Xaml.Input.KeyboardAccelerator)xamlStackPanel.FindName("keyboardAccelerator");

                // Xaml Source
                xamlHost = new WindowsXamlHostWrapper()
                {
                    Height = 100,
                    Width = 200,
                    Content = xamlStackPanel,
                };
                // Add Xaml source into the stackpanel children
                xamlHost.InsertInto(children);

                host.MainWindow.Content = container;
            });
            TestServices.WindowHelper.WaitForIdle();

            // Set initial focus on Wpf button
            await host.MainWindow.Dispatcher.InvokeAsync(async () =>
            {
                await wpfButton1.FocusAndWaitAsync();
            });

            var keyboardAcceleratorInvokedHandler = new Action<object, KeyboardAcceleratorInvokedEventArgs>((source, args) =>
            {
                VerifyKeyboardAcceleratorInvokedEventArgs(
                    source,
                    args,
                    global::Windows.System.VirtualKey.A,
                    global::Windows.System.VirtualKeyModifiers.Control,
                    xamlButton1,
                    handled:false);
            });

            using (TestServices.KeyboardHelper.CreateKeyboardWaitKindGuard(KeyboardWaitKind.None))
            using (new TestCleanupWrapper())
            using (xamlHost)
            {
                using (var keyboardAcceleratorInvoked = new EventTester<KeyboardAccelerator, KeyboardAcceleratorInvokedEventArgs>(ctrlAAccelerator, "Invoked", keyboardAcceleratorInvokedHandler))
                using (var buttonClickTester = new EventTester<Button, RoutedEventArgs>(xamlButton1, "Click"))
                {
                    Log.Comment("Ctrl+A");
                    TestServices.KeyboardHelper.PressKeySequence("$d$_ctrlscan#$d$_a#$u$_a#$u$_ctrlscan");
                    TestServices.WindowHelper.WaitForIdle();

                    Log.Comment("Validating keyboardAcceleratorInvoked action invoked.");
                    await keyboardAcceleratorInvoked.VerifyEventRaised();
                    Log.Comment("Validating button automation action invoked.");
                    await buttonClickTester.VerifyEventRaised();
                }
                // Verify that invalid accelerators do not invoke the valid accelerator defined on a control
                using (var keyboardAcceleratorInvoked = new EventTester<KeyboardAccelerator, KeyboardAcceleratorInvokedEventArgs>(ctrlAAccelerator, "Invoked", keyboardAcceleratorInvokedHandler))
                using (var buttonClickTester = new EventTester<Button, RoutedEventArgs>(xamlButton1, "Click"))
                {
                    Log.Comment("Ctrl+Shift+A");
                    TestServices.KeyboardHelper.PressKeySequence("$d$_ctrlscan#$d$_lshift#$d$_a#$u$_a#$u$_lshift#$u$_ctrlscan");
                    TestServices.WindowHelper.WaitForIdle();

                    Log.Comment("Ctrl+B");
                    TestServices.KeyboardHelper.PressKeySequence("$d$_ctrlscan#$d$_b#$u$_b#$u$_ctrlscan");
                    TestServices.WindowHelper.WaitForIdle();

                    await keyboardAcceleratorInvoked.VerifyEventNotRaised();
                    Log.Comment("Validating button automation action not invoked.");
                    await buttonClickTester.VerifyEventNotRaised();
                }
                Log.Comment("Shift");
                TestServices.KeyboardHelper.PressKeySequence("$d$_lshift#$u$_lshift");
                TestServices.WindowHelper.WaitForIdle();
            }
            TestServices.WindowHelper.WaitForIdle();
        }// End of Test Case

        [TestMethod]
        [TestProperty("Description", "Validates that each XAML island maintains its own AdaptiveTriggers qualifiers.")]
        public void ValidateAdaptiveTriggersFunctionPerHost()
        {
            var host = TestServices.Win32Host as WPFHost;
            Verify.IsNotNull(host);
            Verify.IsNotNull(host.MainWindow);

            WindowsXamlHostWrapper xamlHost1 = null;
            WindowsXamlHostWrapper xamlHost2 = null;
            UserControl userControl1 = null;
            UserControl userControl2 = null;
            Rectangle variableWidthRectangle1 = null;
            Rectangle variableWidthRectangle2 = null;

            AutoResetEvent rectangle1SizeChangedEvent = new AutoResetEvent(false);
            AutoResetEvent rectangle2SizeChangedEvent = new AutoResetEvent(false);

            host.MainWindow.Dispatcher.Invoke(() =>
            {
                var container = new WPFControls.StackPanel();
                var children = container.Children;

                xamlHost1 = new WindowsXamlHostWrapper()
                {
                    Height = 50,
                    Width = 400,
                };

                xamlHost1.InsertInto(children);

                xamlHost2 = new WindowsXamlHostWrapper()
                {
                    Height = 50,
                    Width = 300,
                };

                xamlHost2.InsertInto(children);
                host.MainWindow.Content = container;

                string userControlXaml =
                    @"<UserControl xmlns='http://schemas.microsoft.com/winfx/2006/xaml/presentation' xmlns:x='http://schemas.microsoft.com/winfx/2006/xaml'>
                        <Grid>
                            <VisualStateManager.VisualStateGroups>
                                <VisualStateGroup>
                                    <VisualState>
                                        <VisualState.StateTriggers>
                                            <AdaptiveTrigger MinWindowWidth='0' />
                                        </VisualState.StateTriggers>
                                    </VisualState>
                                    <VisualState>
                                        <VisualState.StateTriggers>
                                            <AdaptiveTrigger MinWindowWidth='250' />
                                        </VisualState.StateTriggers>
                                        <VisualState.Setters>
                                            <Setter Target='VariableWidthRectangle.Width' Value='200' />
                                        </VisualState.Setters>
                                    </VisualState>
                                </VisualStateGroup>
                            </VisualStateManager.VisualStateGroups>
                            <Grid.ColumnDefinitions>
                                <ColumnDefinition Width='Auto' />
                                <ColumnDefinition />
                            </Grid.ColumnDefinitions>

                            <Rectangle x:Name='VariableWidthRectangle' Fill='{0}' Width='10' />
                            <Rectangle Fill='{1}' Grid.Column='1' />
                        </Grid>
                    </UserControl>";

                userControl1 = (UserControl)XamlReader.Load(string.Format(userControlXaml, "Red", "Blue"));
                userControl2 = (UserControl)XamlReader.Load(string.Format(userControlXaml, "Green", "Yellow"));

                // Giving these names helps with debugging.
                userControl1.Name = "userControl1";
                userControl2.Name = "userControl2";

                variableWidthRectangle1 = (Rectangle)userControl1.FindName("VariableWidthRectangle");
                variableWidthRectangle2 = (Rectangle)userControl2.FindName("VariableWidthRectangle");

                variableWidthRectangle1.SizeChanged += (args, sender) => { rectangle1SizeChangedEvent.Set(); };
                variableWidthRectangle2.SizeChanged += (args, sender) => { rectangle2SizeChangedEvent.Set(); };

                xamlHost1.Content = userControl1;
                xamlHost2.Content = userControl2;
            });

            rectangle1SizeChangedEvent.WaitOne();
            rectangle2SizeChangedEvent.WaitOne();
            TestServices.WindowHelper.WaitForIdle();

            TestServices.WindowHelper.WaitForIdle();

            host.MainWindow.Dispatcher.Invoke(() =>
            {
                Verify.AreEqual(variableWidthRectangle1.Width, 200);
                Verify.AreEqual(variableWidthRectangle2.Width, 200);

                xamlHost1.Width = 200;
            });

            rectangle1SizeChangedEvent.WaitOne();
            TestServices.WindowHelper.WaitForIdle();

            host.MainWindow.Dispatcher.Invoke(() =>
            {
                Verify.AreEqual(variableWidthRectangle1.Width, 10);
                Verify.AreEqual(variableWidthRectangle2.Width, 200);

                xamlHost2.Width = 150;
            });

            rectangle2SizeChangedEvent.WaitOne();
            TestServices.WindowHelper.WaitForIdle();

            host.MainWindow.Dispatcher.Invoke(() =>
            {
                Verify.AreEqual(variableWidthRectangle1.Width, 10);
                Verify.AreEqual(variableWidthRectangle2.Width, 10);

                xamlHost1.Width = 400;
            });

            rectangle1SizeChangedEvent.WaitOne();
            TestServices.WindowHelper.WaitForIdle();

            host.MainWindow.Dispatcher.Invoke(() =>
            {
                Verify.AreEqual(variableWidthRectangle1.Width, 200);
                Verify.AreEqual(variableWidthRectangle2.Width, 10);

                xamlHost2.Width = 300;
            });

            rectangle2SizeChangedEvent.WaitOne();
            TestServices.WindowHelper.WaitForIdle();

            host.MainWindow.Dispatcher.Invoke(() =>
            {
                Verify.AreEqual(variableWidthRectangle1.Width, 200);
                Verify.AreEqual(variableWidthRectangle2.Width, 200);

                // Now we flip the hosts' content and ensure that everything continues to work.
                xamlHost1.Content = null;
                xamlHost2.Content = userControl1;
                xamlHost1.Content = userControl2;
            });

            TestServices.WindowHelper.WaitForIdle();

            rectangle1SizeChangedEvent.Reset();
            rectangle2SizeChangedEvent.Reset();

            host.MainWindow.Dispatcher.Invoke(() =>
            {
                Verify.AreEqual(variableWidthRectangle1.Width, 200);
                Verify.AreEqual(variableWidthRectangle2.Width, 200);

                xamlHost2.Width = 150;
            });

            rectangle1SizeChangedEvent.WaitOne();
            TestServices.WindowHelper.WaitForIdle();

            host.MainWindow.Dispatcher.Invoke(() =>
            {
                Verify.AreEqual(variableWidthRectangle1.Width, 10);
                Verify.AreEqual(variableWidthRectangle2.Width, 200);

                xamlHost1.Width = 200;
            });

            rectangle2SizeChangedEvent.WaitOne();
            TestServices.WindowHelper.WaitForIdle();

            host.MainWindow.Dispatcher.Invoke(() =>
            {
                Verify.AreEqual(variableWidthRectangle1.Width, 10);
                Verify.AreEqual(variableWidthRectangle2.Width, 10);
            });

            xamlHost1.Dispose();
            xamlHost2.Dispose();
        }

        #region Helpers

        void VerifyKeyboardAcceleratorInvokedEventArgs(
            object sender,
            KeyboardAcceleratorInvokedEventArgs args,
            global::Windows.System.VirtualKey key,
            global::Windows.System.VirtualKeyModifiers modifiers,
            DependencyObject element,
            bool handled)
        {
            KeyboardAccelerator senderAsKA = sender as KeyboardAccelerator;
            Verify.IsNotNull(senderAsKA);
            Verify.AreEqual(senderAsKA.Key, key);
            Verify.AreEqual(senderAsKA.Modifiers, modifiers);
            Verify.AreEqual(args.Handled, handled);
            Verify.AreEqual(args.Element, element);
        }

        #endregion

        [TestMethod]
        [TestProperty("Description", "Ensure Dragging from XamlIslandRoot to another Island works")]
        [TestProperty("Ignore", "TRUE")] // This test is hard coding screen positions and is clicking in the wrong locations
        public void DragFromXamlIslandToXamlIsland()
        {
            // Disable keyboard input wait as WPF is in charge here.
            using (TestServices.KeyboardHelper.CreateKeyboardWaitKindGuard(KeyboardWaitKind.None))
            {
                //This is the general host of all the elements
                var host = TestServices.Win32Host as WPFHost;
                Verify.IsNotNull(host);
                Verify.IsNotNull(host.MainWindow);

                WPFControls.StackPanel container = null;
                WindowsXamlHostWrapper xamlHost1 = null;//This is the first XamlIslandRoot
                WindowsXamlHostWrapper xamlHost2 = null;//This is the second XamlIslandRoot
                WindowsXamlHostWrapper xamlHost3 = null;//This Island is just for occupying the position on the top
                Xaml.Shapes.Rectangle xamlRect = null; ///The element to be dragged
                XamlControls.StackPanel xamlStackPanel1 = null;
                XamlControls.StackPanel xamlStackPanel2 = null;
                XamlControls.StackPanel xamlStackPanel3 = null;
                XamlControls.Canvas xamlSourceCanvas = null;
                XamlControls.Canvas xamlTargetCanvas = null;
                string dragSucceedText = "Draging action performed";

                const string xamlIslandContent1 =
                @"<StackPanel xmlns='http://schemas.microsoft.com/winfx/2006/xaml/presentation' xmlns:x='http://schemas.microsoft.com/winfx/2006/xaml' CanDrag='true' AllowDrop='true' BorderBrush='Red' BorderThickness='4'>
                      <Canvas x:Name='xamlTargetCanvas' AllowDrop='true' Width='150' Height='150' Background='Green'/>
                  </StackPanel>";

                const string xamlIslandContent2 =
                @"<StackPanel xmlns='http://schemas.microsoft.com/winfx/2006/xaml/presentation' xmlns:x='http://schemas.microsoft.com/winfx/2006/xaml' CanDrag='true'>
                      <Canvas x:Name='XamlSourceCanvas' AllowDrop='true' CanDrag='true' Width='180' Height='180' Background='Blue'>
                          <Rectangle x:Name='Rectangle' Width='120' Height='120' CanDrag='true' AllowDrop='true' Fill='Red'/>
                      </Canvas>
                      <TextBlock x:Name='Text' Margin='8,8,0,0'> This is a XamlIslandRoot </TextBlock>
                  </StackPanel>";

                const string xamlIslandContent3 =
                @"<StackPanel xmlns='http://schemas.microsoft.com/winfx/2006/xaml/presentation' xmlns:x='http://schemas.microsoft.com/winfx/2006/xaml' CanDrag='true' AllowDrop='true' BorderBrush='Red' BorderThickness='4'>
                      <Canvas x:Name='xamlTargetCanvas' AllowDrop='true' Width='150' Height='150' Background='Green'>
                          <TextBlock x:Name='Text' Margin='8,8,0,0' AllowDrop='true'>This is another XamlIslandRoot</TextBlock>
                      </Canvas>
                  </StackPanel>";

                var dragEnterEvent = new AutoResetEvent(false);
                var islandDropEvent = new AutoResetEvent(false);
                var pointerPressedEvent = new AutoResetEvent(false);
                var dragStartingEvent = new AutoResetEvent(false);
                var dropCompletedEvent = new AutoResetEvent(false);
                var dropResult = DataPackageOperation.None;

                //Xaml Elements events handlers:
                DragEventHandler dragEnterEventAction = (sender, args) =>
                {
                    WEX.Logging.Interop.Log.Comment("Set AcceptedOperation in DragEnter handler.");
                    args.AcceptedOperation = DataPackageOperation.Copy;
                    dragEnterEvent.Set();
                };

                DragEventHandler islandDropEventAction = async (sender, args) =>
                {
                    try
                    {
                        if (args.DataView.Contains(StandardDataFormats.Text))
                        {
                            var text = await args.DataView.GetTextAsync();
                            Verify.IsTrue((string)text == dragSucceedText);
                            WEX.Logging.Interop.Log.Comment("XamlIslandRoot received data: " + text);
                            XamlControls.Canvas senderElement = (XamlControls.Canvas)sender;
                            XamlControls.TextBlock textBlock = (XamlControls.TextBlock)senderElement.Children[0];
                            textBlock.Text = text;

                            global::Windows.Foundation.Point position = args.GetPosition(xamlTargetCanvas);
                            WEX.Logging.Interop.Log.Comment("Element drop position: " + position.X + " " + position.Y);
                        }
                        else
                        {
                            WEX.Logging.Interop.Log.Comment("No text data received");
                        }
                    }
                    catch (Exception e)
                    {
                        WEX.Logging.Interop.Log.Comment("Data receive exception: " + e.Message);
                    }

                    WEX.Logging.Interop.Log.Comment("Set AcceptedOperation in Island Drop handler.");
                    args.AcceptedOperation = DataPackageOperation.Copy;
                    islandDropEvent.Set();
                };

                PointerEventHandler pointerPressedEventAction = (sender, args) =>
                {
                    WEX.Logging.Interop.Log.Comment("Pointer pressed on source element");

                    pointerPressedEvent.Set();
                };

                global::Windows.Foundation.TypedEventHandler<Microsoft.UI.Xaml.UIElement, DragStartingEventArgs> dragStartingEventAction = (sender, args) =>
                {
                    args.Data.SetText(dragSucceedText);
                    args.DragUI.SetContentFromDataPackage();
                    global::Windows.Foundation.Point position = args.GetPosition(xamlSourceCanvas);
                    WEX.Logging.Interop.Log.Comment("Drag starting on source element, position: " + position.X + " " + position.Y);
                    dragStartingEvent.Set();
                    args.Data.RequestedOperation = DataPackageOperation.Copy;
                };

                global::Windows.Foundation.TypedEventHandler<Microsoft.UI.Xaml.UIElement, DropCompletedEventArgs> dropCompletedEventAction = (sender, args) =>
                {
                    WEX.Logging.Interop.Log.Comment("Drag completed on source element");
                    dropResult = args.DropResult;
                    dropCompletedEvent.Set();
                };

                host.MainWindow.Dispatcher.Invoke(() =>
                {
                    container = new WPFControls.StackPanel();
                    container.AllowDrop = true;
                    var children = container.Children;

                    //XamlIslandRoots elements:
                    xamlStackPanel1 = (StackPanel)XamlMarkup.XamlReader.Load(xamlIslandContent1);
                    xamlStackPanel2 = (StackPanel)XamlMarkup.XamlReader.Load(xamlIslandContent2);
                    xamlStackPanel3 = (StackPanel)XamlMarkup.XamlReader.Load(xamlIslandContent3);
                    xamlRect = (Xaml.Shapes.Rectangle)xamlStackPanel2.FindName("Rectangle");
                    xamlSourceCanvas = (Xaml.Controls.Canvas)xamlStackPanel2.FindName("XamlSourceCanvas");
                    xamlTargetCanvas = (Xaml.Controls.Canvas)xamlStackPanel3.FindName("xamlTargetCanvas");

                    // Xaml Source, these are the xaml islands

                    //Having trouble finding the elements on the
                    //top area of the mainWindow, here using an Island
                    //to occupy the position
                    xamlHost1 = new WindowsXamlHostWrapper()
                    {
                        Height = 200,
                        Width = 200,
                        Content = xamlStackPanel1,
                    };
                    xamlHost1.InsertInto(children);

                    xamlHost2 = new WindowsXamlHostWrapper()
                    {
                        Height = 200,
                        Width = 200,
                        Content = xamlStackPanel2,
                    };
                    xamlHost2.InsertInto(children);

                    xamlHost3 = new WindowsXamlHostWrapper()
                    {
                        Height = 200,
                        Width = 200,
                        Content = xamlStackPanel3,
                    };
                    xamlHost3.InsertInto(children);

                    host.MainWindow.Content = container;

                    //Attach event handlers:
                    xamlRect.PointerPressed += pointerPressedEventAction;
                    xamlRect.DragStarting += dragStartingEventAction;
                    xamlRect.DropCompleted += dropCompletedEventAction;
                    xamlTargetCanvas.DragEnter += dragEnterEventAction;
                    xamlTargetCanvas.Drop += islandDropEventAction;
                });
                TestServices.WindowHelper.WaitForIdle();

                // Let's force the InputHelper to initialize before doing the real test
                WEX.Logging.Interop.Log.Comment("Tapping xamlIslandRoot rectangle.");
                TestServices.InputHelper.Tap(xamlRect);
                TestServices.WindowHelper.WaitForIdle();

                //DragBetweenElements does not work well for XamlIslandRoot, thus, use MouseDrag here and
                //provide some offset to help locate the rectangle to be dragged.
                WEX.Logging.Interop.Log.Comment("Dragging XamlIslandRoot item to another XamlIslandRoot.");
                TestServices.InputHelper.MouseDrag(xamlRect, 420, 220, xamlTargetCanvas, 444, 455, MouseButton.Left);
                TestServices.WindowHelper.WaitForIdle();
                //The drag action is too fast and the drop event could not receive the data in time,
                //we sleep for while
                Sleep(3000);

                TestServices.InputHelper.MouseButtonUp(xamlTargetCanvas, 444, 455, MouseButton.Left);
                TestServices.WindowHelper.WaitForIdle();

                dropCompletedEvent.WaitOne();

                Verify.IsTrue(dropResult == DataPackageOperation.Copy, "Drop result is set correctly.");

                xamlHost1.Dispose();
                xamlHost2.Dispose();
                xamlHost3.Dispose();
            }
        }

        [TestMethod]
        [TestProperty("Description", "Ensure Dragging from a Win32 window to a XamlIslandRoot works")]
        [TestProperty("Ignore", "TRUE")] // TODO 29717290: Re-enable once failure has been fixed.
        public void DragFromXamlIslandToWPFWindow()
        {
            // Disable keyboard input wait as WPF is in charge here.
            using (TestServices.KeyboardHelper.CreateKeyboardWaitKindGuard(KeyboardWaitKind.None))
            {
                //This is the general host of all the elements
                var host = TestServices.Win32Host as WPFHost;
                Verify.IsNotNull(host);
                Verify.IsNotNull(host.MainWindow);

                WPFControls.StackPanel container = null;
                WindowsXamlHostWrapper xamlHost1 = null;//This is the first XamlIslandRoot
                WindowsXamlHostWrapper xamlHost2 = null;//This island is just for occupying the position on the top
                WPFControls.Canvas windowTargetCanvas = null;
                WPFControls.TextBlock windowText = null;
                WPFControls.Border windowTargetBorder = null;
                Xaml.Shapes.Rectangle xamlRect = null; ///The element to be dragged
                XamlControls.StackPanel xamlStackPanel1 = null;
                XamlControls.StackPanel xamlStackPanel2 = null;
                XamlControls.Canvas xamlSourceCanvas = null;

                string dragSucceedText = "Draging action performed";

                const string xamlIslandContent1 =
                @"<StackPanel xmlns='http://schemas.microsoft.com/winfx/2006/xaml/presentation' xmlns:x='http://schemas.microsoft.com/winfx/2006/xaml' CanDrag='true' AllowDrop='true' BorderBrush='Red' BorderThickness='4'>
                      <Canvas x:Name='xamlTargetCanvas' AllowDrop='true' Width='150' Height='150' Background='Green'/>
                  </StackPanel>";

                const string xamlIslandContent2 =
                @"<StackPanel xmlns='http://schemas.microsoft.com/winfx/2006/xaml/presentation' xmlns:x='http://schemas.microsoft.com/winfx/2006/xaml' CanDrag='true'>
                      <Canvas x:Name='XamlSourceCanvas' AllowDrop='true' CanDrag='true' Width='180' Height='180' Background='Blue'>
                          <Rectangle x:Name='Rectangle' Width='200' Height='200' CanDrag='true' AllowDrop='true' Fill='Red'/>
                      </Canvas>
                      <TextBlock x:Name='Text' Margin='8,8,0,0'> This is a XamlIslandRoot </TextBlock>
                  </StackPanel>";

                var dragEnterEvent = new AutoResetEvent(false);
                var windowDropEvent = new AutoResetEvent(false);
                var pointerPressedEvent = new AutoResetEvent(false);
                var dragStartingEvent = new AutoResetEvent(false);
                var dropCompletedEvent = new AutoResetEvent(false);
                var dropResult = DataPackageOperation.None;

                //Xaml Elements events handlers:
                WPFSystem.DragEventHandler dragEnterEventAction = (sender, args) =>
                {
                    WEX.Logging.Interop.Log.Comment("Set AcceptedOperation in DragEnter handler.");
                    args.Effects = WPFSystem.DragDropEffects.Copy;
                    dragEnterEvent.Set();
                };

                PointerEventHandler pointerPressedEventAction = (sender, args) =>
                {
                    WEX.Logging.Interop.Log.Comment("Pointer pressed on source element");

                    pointerPressedEvent.Set();
                };

                global::Windows.Foundation.TypedEventHandler<Microsoft.UI.Xaml.UIElement, DragStartingEventArgs> dragStartingEventAction = (sender, args) =>
                {
                    args.Data.SetText(dragSucceedText);
                    args.DragUI.SetContentFromDataPackage();
                    global::Windows.Foundation.Point position = args.GetPosition(xamlSourceCanvas);
                    WEX.Logging.Interop.Log.Comment("Drag starting on source element, position: " + position.X + " " + position.Y);
                    dragStartingEvent.Set();
                    args.Data.RequestedOperation = DataPackageOperation.Copy;
                };

                global::Windows.Foundation.TypedEventHandler<Microsoft.UI.Xaml.UIElement, DropCompletedEventArgs> dropCompletedEventAction = (sender, args) =>
                {
                    WEX.Logging.Interop.Log.Comment("Drag completed on source element");
                    dropResult = args.DropResult;
                    dropCompletedEvent.Set();
                };

                //Windows window event handler:
                WPFSystem.DragEventHandler windowDropEventAction = (sender, args) =>
                {
                    try
                    {
                        if (args.Data.GetDataPresent(WPFSystem.DataFormats.Text))
                        {
                            var text = args.Data.GetData(WPFSystem.DataFormats.Text);
                            Verify.IsTrue((string)text == dragSucceedText);
                            WEX.Logging.Interop.Log.Comment("WPF window received data: " + text);
                            WPFControls.Canvas senderElement = (WPFControls.Canvas)sender;
                            WPFControls.TextBlock textBlock = (WPFControls.TextBlock)senderElement.Children[1];
                            textBlock.Text = (string)text;

                            WPFSystem.Point position = args.GetPosition(windowTargetCanvas);
                            WEX.Logging.Interop.Log.Comment("Element drop position: " + position.X + " " + position.Y);
                        }
                        else
                        {
                            WEX.Logging.Interop.Log.Comment("No text data received");
                        }
                    }
                    catch (Exception e)
                    {
                        WEX.Logging.Interop.Log.Comment("Data receive exception: " + e.Message);
                    }
                    WEX.Logging.Interop.Log.Comment("Set AcceptedOperation in Window Drop handler.");
                    args.Effects = WPFSystem.DragDropEffects.Copy;
                    windowDropEvent.Set();
                };

                host.MainWindow.Dispatcher.Invoke(() =>
                {
                    container = new WPFControls.StackPanel();
                    container.AllowDrop = true;
                    var children = container.Children;
                    //Target Canvas for Windows window
                    windowTargetCanvas = new WPFControls.Canvas();
                    windowTargetCanvas.Width = 200;
                    windowTargetCanvas.Height = 200;
                    windowTargetCanvas.AllowDrop = true;
                    windowTargetCanvas.Background = WPFSystem.Media.Brushes.Black;

                    windowTargetBorder = new WPFControls.Border();
                    windowTargetBorder.AllowDrop = true;
                    windowTargetBorder.Width = 200;
                    windowTargetBorder.Height = 200;
                    windowTargetBorder.BorderThickness = new WPFSystem.Thickness(5);
                    windowTargetBorder.BorderBrush = WPFSystem.Media.Brushes.Blue;
                    windowTargetCanvas.Children.Add(windowTargetBorder);
                    windowText = new WPFControls.TextBlock();
                    windowText.Margin = new WPFSystem.Thickness(10);
                    windowText.Background = WPFSystem.Media.Brushes.White;
                    windowText.AllowDrop = true;
                    windowText.Text = "This is a WPF Canvas";
                    windowTargetCanvas.Children.Add(windowText);

                    //XamlIslandRoots elements:
                    xamlStackPanel1 = (StackPanel)XamlMarkup.XamlReader.Load(xamlIslandContent1);
                    xamlStackPanel2 = (StackPanel)XamlMarkup.XamlReader.Load(xamlIslandContent2);
                    xamlRect = (Xaml.Shapes.Rectangle)xamlStackPanel2.FindName("Rectangle");
                    xamlSourceCanvas = (Xaml.Controls.Canvas)xamlStackPanel2.FindName("XamlSourceCanvas");

                    // Xaml Source, these are the xaml islands

                    //Having trouble finding the elements on the
                    //top area of the mainWindow, here using an Island
                    //to occupy the position
                    xamlHost1 = new WindowsXamlHostWrapper()
                    {
                        Height = 200,
                        Width = 200,
                        Content = xamlStackPanel1,
                    };
                    xamlHost1.InsertInto(children);

                    xamlHost2 = new WindowsXamlHostWrapper()
                    {
                        Height = 200,
                        Width = 200,
                        Content = xamlStackPanel2,
                    };
                    xamlHost2.InsertInto(children);

                    children.Add(windowTargetCanvas);

                    host.MainWindow.Content = container;

                    //Attach event handlers:
                    xamlRect.PointerPressed += pointerPressedEventAction;
                    xamlRect.DragStarting += dragStartingEventAction;
                    xamlRect.DropCompleted += dropCompletedEventAction;
                    windowTargetCanvas.DragEnter += dragEnterEventAction;
                    windowTargetCanvas.Drop += windowDropEventAction;
                });
                TestServices.WindowHelper.WaitForIdle();

                // Let's force the InputHelper to initialize before doing the real test
                WEX.Logging.Interop.Log.Comment("Tapping xamlIslandRoot rectangle.");
                TestServices.InputHelper.Tap(xamlRect);
                TestServices.WindowHelper.WaitForIdle();

                //DragBetweenElements does not work well for XamlIslandRoot, thus, use MouseDrag here and
                //provide some offset to help locate the rectangle to be dragged.Note that MouseDrag
                //only accept Xaml types, thus, we use the source canvas as a base to locate the wpf
                //target element
                WEX.Logging.Interop.Log.Comment("Dragging XamlIslandRoot item to a wpf canvas.");
                TestServices.InputHelper.MouseDrag(xamlRect, 420, 220, xamlSourceCanvas, 444, 455, MouseButton.Left);
                TestServices.WindowHelper.WaitForIdle();
                //The drag action is too fast and the drop event could not receive the data in time,
                //we sleep for while
                Sleep(3000);

                TestServices.InputHelper.MouseButtonUp(xamlSourceCanvas, 444, 455, MouseButton.Left);
                TestServices.WindowHelper.WaitForIdle();

                dropCompletedEvent.WaitOne();

                Verify.IsTrue(dropResult == DataPackageOperation.Copy, "Drop result is set correctly.");

                xamlHost1.Dispose();
                xamlHost2.Dispose();
            }
        }


        [TestMethod]
        [TestProperty("TestPass:MaxOSVer", WindowsOSVersion._22H2)]    // This test is currently failing on 23h2.
        public void ValidateMenuFlyoutClosesWhenXamlFocusLost()
        {
            // Disable keyboard input wait as WPF is in charge here.
            using (TestServices.KeyboardHelper.CreateKeyboardWaitKindGuard(KeyboardWaitKind.None))
            {
                var host = TestServices.Win32Host as WPFHost;
                Verify.IsNotNull(host);
                Verify.IsNotNull(host.MainWindow);

                WPFControls.StackPanel container = null;
                WPFControls.Button wpfButton1 = null;
                WPFControls.Button wpfButton2 = null;
                WindowsXamlHostWrapper xamlHost = null;
                Xaml.Controls.Button xamlButton1 = null;
                Xaml.Controls.Button xamlButton2 = null;
                Xaml.Controls.StackPanel xamlStackPanel = null;
                MenuFlyout menuFlyout = null;

                const string rootPanelXaml =
                @"<StackPanel xmlns='http://schemas.microsoft.com/winfx/2006/xaml/presentation' xmlns:x='http://schemas.microsoft.com/winfx/2006/xaml'>
                    <Button x:Name='xamlButton1' Content='XAML Button 1'/>
                    <Button x:Name='xamlButton2' Content='XAML Button 2'/>
                  </StackPanel>";

                host.MainWindow.Dispatcher.Invoke(() =>
                {
                    container = new WPFControls.StackPanel();
                    container.HorizontalAlignment = WPFSystem.HorizontalAlignment.Left;
                    container.Margin = new WPFSystem.Thickness(10.0);
                    var children = container.Children;

                    // WPF Button
                    wpfButton1 = new WPFControls.Button()
                    {
                        Height = 30,
                        Width = 100,
                        Name = "wpfButton1",
                        Content = "WPF Button 1",
                    };
                    children.Add(wpfButton1);

                    xamlStackPanel = (StackPanel)XamlMarkup.XamlReader.Load(rootPanelXaml);
                    xamlButton1 = (Button)xamlStackPanel.FindName("xamlButton1");
                    xamlButton2 = (Button)xamlStackPanel.FindName("xamlButton2");

                    xamlButton1.Click += (s, args) => {
                        menuFlyout = new MenuFlyout();
                        menuFlyout.Items.Add(new MenuFlyoutItem() { Text = "Item 1" });
                        menuFlyout.Items.Add(new MenuFlyoutItem() { Text = "Item 2" });
                        menuFlyout.Items.Add(new MenuFlyoutItem() { Text = "Item 3" });
                        menuFlyout.ShowAt((UIElement)s, new Point(0, 0));
                    };

                    // Xaml Source
                    xamlHost = new WindowsXamlHostWrapper()
                    {
                        Height = 100,
                        Width = 200,
                        Content = xamlStackPanel,
                    };
                    // Add Xaml source into the stackpanel children
                    xamlHost.InsertInto(children);

                    // WPF Button
                    wpfButton2 = new WPFControls.Button()
                    {
                        Height = 30,
                        Width = 100,
                        Name = "wpfButton2",
                        Content = "WPF Button 2",
                    };
                    children.Add(wpfButton2);

                    host.MainWindow.Content = container;
                });
                TestServices.WindowHelper.WaitForIdle();

                using(xamlHost)
                using(new TestCleanupWrapper())
                {
                    // Hard-code these coordinates because InputHelper doesn't know where the island is in screen space
                    var xamlButton1Pos = new global::Windows.Foundation.Point(60, 54);
                    var wpfButton1Pos = new global::Windows.Foundation.Point(110, 23);

                    TestServices.InputHelper.ClickMouseButton(MouseButton.Left, xamlButton1Pos);
                    TestServices.WindowHelper.WaitForIdle();

                    host.MainWindow.Dispatcher.Invoke(() =>
                    {
                        Verify.IsTrue(menuFlyout.IsOpen);
                    });

                    TestServices.InputHelper.ClickMouseButton(MouseButton.Left, wpfButton1Pos);
                    TestServices.WindowHelper.WaitForIdle();

                    host.MainWindow.Dispatcher.Invoke(() =>
                    {
                        Verify.IsFalse(menuFlyout.IsOpen);
                    });
                }

            }
        }


        [TestMethod]
        public void ValidateWebBrowserWorksWhenXamlIslandLoaded()
        {
            // Disable keyboard input wait as WPF is in charge here.
            using (TestServices.KeyboardHelper.CreateKeyboardWaitKindGuard(KeyboardWaitKind.None))
            using (new TestCleanupWrapper())
            {
                var host = TestServices.Win32Host as WPFHost;
                Verify.IsNotNull(host);
                Verify.IsNotNull(host.MainWindow);

                WPFControls.StackPanel container = null;
                WPFControls.WebBrowser wpfWebBrowser = null;
                WindowsXamlHostWrapper xamlHost = null;
                Xaml.Controls.StackPanel xamlStackPanel = null;

                Log.Comment("First, load a WPF WebBrowser");
                var htmlLoadedEvent = new AutoResetEvent(false);
                host.MainWindow.Dispatcher.Invoke(() =>
                {
                    container = new WPFControls.StackPanel();
                    container.HorizontalAlignment = WPFSystem.HorizontalAlignment.Left;
                    container.Margin = new WPFSystem.Thickness(10.0);
                    var children = container.Children;

                    wpfWebBrowser = new WPFControls.WebBrowser()
                    {
                        Height = 100,
                        Width = 100,
                    };
                    children.Add(wpfWebBrowser);

                    host.MainWindow.Content = container;

                    wpfWebBrowser.NavigateToString("<html><body>this is the HTML content</body></html>");
                    wpfWebBrowser.LoadCompleted += new WPFSystem.Navigation.LoadCompletedEventHandler((s, a) => {
                        htmlLoadedEvent.Set();
                    });
                });
                htmlLoadedEvent.WaitOne();
                TestServices.WindowHelper.WaitForIdle();

                Log.Comment("Now, loading XAML");

                host.MainWindow.Dispatcher.Invoke(() =>
                {
                    const string rootPanelXaml =
                    @"<StackPanel xmlns='http://schemas.microsoft.com/winfx/2006/xaml/presentation' xmlns:x='http://schemas.microsoft.com/winfx/2006/xaml'>
                        <Button x:Name='xamlButton1' Content='XAML Button 1'/>
                        <Button x:Name='xamlButton2' Content='XAML Button 2'/>
                    </StackPanel>";
                    xamlStackPanel = (StackPanel)XamlMarkup.XamlReader.Load(rootPanelXaml);

                    // Xaml Source
                    xamlHost = new WindowsXamlHostWrapper()
                    {
                        Height = 100,
                        Width = 200,
                        Content = xamlStackPanel,
                    };
                    // Add Xaml source into the stackpanel children
                    xamlHost.InsertInto(container.Children);
                });
                TestServices.WindowHelper.WaitForIdle();

                host.MainWindow.Dispatcher.Invoke(() =>
                {
                    wpfWebBrowser.NavigateToString("<html><body>this is the second HTML navigation target</body></html>");
                });
                htmlLoadedEvent.WaitOne();
                TestServices.WindowHelper.WaitForIdle();

                xamlHost.Dispose();
            }
        }


        [TestMethod]
        public void HostWindowResize()
        {
            // Disable keyboard input wait as WPF is in charge here.
            using (TestServices.KeyboardHelper.CreateKeyboardWaitKindGuard(KeyboardWaitKind.None))
            {
                var host = TestServices.Win32Host as WPFHost;
                Verify.IsNotNull(host);
                Verify.IsNotNull(host.MainWindow);

                WPFControls.StackPanel container = null;
                WPFControls.Button wpfButton1 = null;
                WPFControls.Button wpfButton2 = null;
                WindowsXamlHostWrapper xamlHost = null;
                Xaml.Controls.StackPanel xamlStackPanel = null;

                const string rootPanelXaml =
                @"<StackPanel xmlns='http://schemas.microsoft.com/winfx/2006/xaml/presentation' xmlns:x='http://schemas.microsoft.com/winfx/2006/xaml'>
                    <Button x:Name='xamlButton1' Content='XAML Button 1'/>
                    <Button x:Name='xamlButton2' Content='XAML Button 2'/>
                  </StackPanel>";

                host.MainWindow.Dispatcher.Invoke(() =>
                {
                    container = new WPFControls.StackPanel();
                    container.HorizontalAlignment = WPFSystem.HorizontalAlignment.Left;
                    container.Margin = new WPFSystem.Thickness(10.0);
                    var children = container.Children;

                    // WPF Button
                    wpfButton1 = new WPFControls.Button()
                    {
                        Height = 30,
                        Width = 100,
                        Name = "wpfButton1",
                        Content = "WPF Button 1",
                    };
                    children.Add(wpfButton1);

                    xamlStackPanel = (StackPanel)XamlMarkup.XamlReader.Load(rootPanelXaml);
                    xamlStackPanel.Children.Add(new MyControl());

                    // Xaml Source
                    xamlHost = new WindowsXamlHostWrapper()
                    {
                        Height = 100,
                        Width = 200,
                        Content = xamlStackPanel,
                    };
                    // Add Xaml source into the stackpanel children
                    xamlHost.InsertInto(children);

                    // WPF Button
                    wpfButton2 = new WPFControls.Button()
                    {
                        Height = 30,
                        Width = 100,
                        Name = "wpfButton2",
                        Content = "WPF Button 2",
                    };
                    children.Add(wpfButton2);

                    host.MainWindow.Content = container;
                });
                TestServices.WindowHelper.WaitForIdle();

                using(xamlHost)
                using(new TestCleanupWrapper())
                {
                    TestServices.WindowHelper.WaitForIdle();
                }
            }
        }

        // Test Layout
        // -----------
        //  
        //                                     +-------------------+
        //                                     |  WPF  Button 1    |
        //                                     +-------------------+
        //  
        //                                +---------Xaml-Island-----------+
        //                                |                               |
        //                                |       Empty Island            |
        //                                +-------------------------------+
        //
        //                                     +-------------------+
        //                                     |  WPF  Button 2    |
        //                                     +-------------------+
        //  
        
        [TestMethod]
        [TestProperty("Description", "Validates focus in empty island scenario")]
        public async Task ValidateFocusBehaviorWithEmptyIsland()
        {
            // Disable keyboard input wait as WPF is in charge here.
            using (TestServices.KeyboardHelper.CreateKeyboardWaitKindGuard(KeyboardWaitKind.None))
            {
                var host = TestServices.Win32Host as WPFHost;
                Verify.IsNotNull(host);
                Verify.IsNotNull(host.MainWindow);

                WPFControls.StackPanel container = null;
                WPFControls.Button wpfButton1 = null;
                WPFControls.Button wpfButton2 = null;
                WindowsXamlHostWrapper xamlHost1 = null;

                host.MainWindow.Dispatcher.Invoke(() =>
                {
                    container = new WPFControls.StackPanel();
                    var children = container.Children;

                    // WPF Button1
                    wpfButton1 = new WPFControls.Button()
                    {
                        Height = 30,
                        Width = 100,
                        Name = "wpfButton1",
                        Content = "WPF Button 1",
                    };
                    children.Add(wpfButton1);
                    
                    // Xaml Source: Empty Island
                    xamlHost1 = new WindowsXamlHostWrapper()
                    {
                        Height = 50,
                        Width = 200,
                    };
                    // Add Xaml source into the stackpanel children
                    xamlHost1.InsertInto(children);

                    // WPF Button2
                    wpfButton2 = new WPFControls.Button()
                    {
                        Height = 30,
                        Width = 100,
                        Name = "wpfButton2",
                        Content = "WPF Button 2",
                    };
                    children.Add(wpfButton2);

                    host.MainWindow.Content = container;
                });
                TestServices.WindowHelper.WaitForIdle();

                using (new TestCleanupWrapper())
                {
                    // Set initial focus on Wpf button
                    await host.MainWindow.Dispatcher.InvokeAsync(async () =>
                    {
                        await wpfButton1.FocusAndWaitAsync();
                        Verify.IsTrue(wpfButton1.IsFocused);
                        Log.Comment("Focus is on " + wpfButton1.Name);
                    });

                    // Verify tab moves focus to next WPF button2
                    TestServices.KeyboardHelper.Tab();
                    TestServices.WindowHelper.WaitForIdle();
                    host.MainWindow.Dispatcher.Invoke(() =>
                    {
                        Verify.IsTrue(wpfButton2.IsFocused);
                        Log.Comment("Focus is on " + wpfButton2.Name);
                    });

                    // Note: We must dispose our inner xaml content before setting WPF's content to
                    // null. Setting WPF's content to null will disconnect the HWND tree which is 
                    // not allowed for a non-disposed DesktopWindowXamlSource object.
                    xamlHost1.Dispose();

                    host.MainWindow.Dispatcher.Invoke(() =>
                    {
                        host.MainWindow.Content = null;
                    });
                }
            }
        }

        // Test Layout
        // -----------
        //  
        //                                                                 +-------------------+
        //                                                                 |  WPF  Button 1    |
        //                                                                 +-------------------+
        //  
        //                                +---------Xaml-Island-----------+                      +---------Xaml-Island-----------+
        //                                |                               |      ........        |                               |
        //                                |         Empty Island          |  Multiple Islands 7  |         Empty Islands         |
        //                                +-------------------------------+                      +-------------------------------+
        //                                               1                                                      2
        //                                                                 +-------------------+
        //                                                                 |  WPF  Button 2    |
        //                                                                 +-------------------+
        //  

        [TestMethod]
        [TestProperty("Description", "Validates focus in empty island scenario")]
        public async Task ValidateFocusBehaviorWithMultipleEmptyIsland()
        {
            // Disable keyboard input wait as WPF is in charge here.
            using (TestServices.KeyboardHelper.CreateKeyboardWaitKindGuard(KeyboardWaitKind.None))
            {
                var host = TestServices.Win32Host as WPFHost;
                Verify.IsNotNull(host);
                Verify.IsNotNull(host.MainWindow);

                WPFControls.StackPanel container = null;
                WPFControls.Button wpfButton1 = null;
                WPFControls.Button wpfButton2 = null;
                WindowsXamlHostWrapper[] xamlHostN = { null, null, null, null, null, null, null};

                host.MainWindow.Dispatcher.Invoke(() =>
                {
                    container = new WPFControls.StackPanel();
                    var children = container.Children;

                    // WPF Button1
                    wpfButton1 = new WPFControls.Button()
                    {
                        Height = 30,
                        Width = 100,
                        Name = "wpfButton1",
                        Content = "WPF Button 1",
                    };
                    children.Add(wpfButton1);

                    // Xaml Source: Empty Island
                    for(int i = 0; i<7; i++)
                    {
                        xamlHostN[i] = new WindowsXamlHostWrapper()
                        {
                            Height = 50,
                            Width = 200,
                        };
                    }
                    // Add Xaml source into the stackpanel children
                    foreach (WindowsXamlHostWrapper xamlHost in xamlHostN)
                    {
                        xamlHost.InsertInto(children);
                    }

                    // WPF Button2
                    wpfButton2 = new WPFControls.Button()
                    {
                        Height = 30,
                        Width = 100,
                        Name = "wpfButton2",
                        Content = "WPF Button 2",
                    };
                    children.Add(wpfButton2);

                    host.MainWindow.Content = container;
                });
                TestServices.WindowHelper.WaitForIdle();

                using (new TestCleanupWrapper())
                {
                    // Set initial focus on Wpf button
                    await host.MainWindow.Dispatcher.InvokeAsync(async () =>
                    {
                        await wpfButton1.FocusAndWaitAsync();
                        Verify.IsTrue(wpfButton1.IsFocused);
                        Log.Comment("Focus is on " + wpfButton1.Name);
                    });

                    // Verify tab moves focus to next WPF button2
                    TestServices.KeyboardHelper.Tab();
                    TestServices.WindowHelper.WaitForIdle();
                    host.MainWindow.Dispatcher.Invoke(() =>
                    {
                        Verify.IsTrue(wpfButton2.IsFocused);
                        Log.Comment("Focus is on " + wpfButton2.Name);
                    });

                    // Verify tab cycle back focus to again WPF button1
                    TestServices.KeyboardHelper.Tab();
                    TestServices.WindowHelper.WaitForIdle();
                    host.MainWindow.Dispatcher.Invoke(() =>
                    {
                        Verify.IsTrue(wpfButton1.IsFocused);
                        Log.Comment("Focus is on " + wpfButton1.Name);
                    });

                    // Note: We must dispose our inner xaml contents before setting WPF's content to
                    // null. Setting WPF's content to null will disconnect the HWND tree which is 
                    // not allowed for a non-disposed DesktopWindowXamlSource object.
                    foreach (WindowsXamlHostWrapper xamlHost in xamlHostN)
                    {
                        xamlHost.Dispose();
                    }

                    host.MainWindow.Dispatcher.Invoke(() =>
                    {
                        host.MainWindow.Content = null;
                    });
                }
            }
        }

        // Test Layout
        // -----------
        //  
        //                                                                 +-------------------+
        //                                                                 |  WPF  Button 1    |
        //                                                                 +-------------------+
        //  
        //                                +---------Xaml-Island-----------+                      +---------Xaml-Island-----------+
        //                                |                               |      ........        |                               |
        //                                |    stackpanel with button     |  Multiple Islands 2  |    stackpanel with button     |
        //                                +-------------------------------+                      +-------------------------------+
        //                                               1                                                      2
        //                                                                 +-------------------+
        //                                                                 |  WPF  Button 2    |
        //                                                                 +-------------------+
        //  

        // Move elements across the islands
        [TestMethod]
        [TestProperty("Description", "Validates entire stackpanel can be moved from an Island to another")]
        public void MoveStackPanelAcrossIsland()
        {
            // Disable keyboard input wait as WPF is in charge here.
            using (TestServices.KeyboardHelper.CreateKeyboardWaitKindGuard(KeyboardWaitKind.None))
            {
                var host = TestServices.Win32Host as WPFHost;
                Verify.IsNotNull(host);
                Verify.IsNotNull(host.MainWindow);

                WPFControls.StackPanel container = null;
                WPFControls.Button wpfButton1 = null;
                WPFControls.Button wpfButton2 = null;
                WindowsXamlHostWrapper xamlHost1 = null, xamlHost2 = null;
                Xaml.Controls.StackPanel xamlStackPanel = null;

                const string rootPanelXaml =
                @"<StackPanel xmlns='http://schemas.microsoft.com/winfx/2006/xaml/presentation' xmlns:x='http://schemas.microsoft.com/winfx/2006/xaml'>
                    <Button x:Name='xamlButton1' Content='XAML Button 1' />
                </StackPanel>";

                host.MainWindow.Dispatcher.Invoke(() =>
                {
                    container = new WPFControls.StackPanel();
                    var children = container.Children;

                    // WPF Button1
                    wpfButton1 = new WPFControls.Button()
                    {
                        Height = 30,
                        Width = 100,
                        Name = "wpfButton1",
                        Content = "WPF Button 1",
                    };
                    children.Add(wpfButton1);

                    xamlStackPanel = (Xaml.Controls.StackPanel)XamlMarkup.XamlReader.Load(rootPanelXaml);

                    // Xaml Source: 1st Island
                    xamlHost1 = new WindowsXamlHostWrapper()
                    {
                        Height = 50,
                        Width = 200,
                        Content = xamlStackPanel,
                    };
                    
                    // Add Xaml source into the stackpanel children
                    xamlHost1.InsertInto(children);

                    // Xaml Source: 2nd Island
                    xamlHost2 = new WindowsXamlHostWrapper()
                    {
                        Height = 50,
                        Width = 200,
                    };
                    xamlHost2.InsertInto(children);

                    // WPF Button2
                    wpfButton2 = new WPFControls.Button()
                    {
                        Height = 30,
                        Width = 100,
                        Name = "wpfButton2",
                        Content = "WPF Button 2",
                    };
                    children.Add(wpfButton2);

                    host.MainWindow.Content = container;
                });
                TestServices.WindowHelper.WaitForIdle();

                host.MainWindow.Dispatcher.Invoke(() =>
                {
                    // Move StackPanel form 1st island to 2nd
                    xamlHost1.Content = null;
                    xamlHost2.Content = xamlStackPanel;
                });
                TestServices.WindowHelper.WaitForIdle();

                // Note: We must dispose our inner xaml contents before setting WPF's content to
                // null. Setting WPF's content to null will disconnect the HWND tree which is 
                // not allowed for a non-disposed DesktopWindowXamlSource object.
                xamlHost1.Dispose();
                xamlHost2.Dispose();
                
                using (new TestCleanupWrapper())
                {
                    host.MainWindow.Dispatcher.Invoke(() =>
                    {
                        host.MainWindow.Content = null;
                    });
                }
            }
        }

        // Move elements from stackpanel across the islands.
        [TestMethod]
        [TestProperty("Description", "Validates button can be moved from an Island to another")]
        public void MoveButtonAcrossIsland()
        {
            // Disable keyboard input wait as WPF is in charge here.
            using (TestServices.KeyboardHelper.CreateKeyboardWaitKindGuard(KeyboardWaitKind.None))
            {
                var host = TestServices.Win32Host as WPFHost;
                Verify.IsNotNull(host);
                Verify.IsNotNull(host.MainWindow);

                WPFControls.StackPanel container = null;
                WPFControls.Button wpfButton1 = null;
                WPFControls.Button wpfButton2 = null;
                WindowsXamlHostWrapper xamlHost1 = null, xamlHost2 = null;
                Xaml.Controls.StackPanel xamlStackPanel = null;
                Xaml.Controls.StackPanel xamlStackPanel2 = null;
                Xaml.Controls.Button xamlButton1 = null;

                const string rootPanelXaml =
                @"<StackPanel xmlns='http://schemas.microsoft.com/winfx/2006/xaml/presentation' xmlns:x='http://schemas.microsoft.com/winfx/2006/xaml'>
                    <Button x:Name='xamlButton1' Content='XAML Button 1' />
                </StackPanel>";

                const string rootPanelXaml2 =
                @"<StackPanel xmlns='http://schemas.microsoft.com/winfx/2006/xaml/presentation' xmlns:x='http://schemas.microsoft.com/winfx/2006/xaml'>
                    <Button x:Name='xamlButton2' Content='XAML Button 2' />
                </StackPanel>";

                host.MainWindow.Dispatcher.Invoke(() =>
                {
                    container = new WPFControls.StackPanel();
                    var children = container.Children;

                    // WPF Button1
                    wpfButton1 = new WPFControls.Button()
                    {
                        Height = 30,
                        Width = 100,
                        Name = "wpfButton1",
                        Content = "WPF Button 1",
                    };
                    children.Add(wpfButton1);

                    xamlStackPanel = (Xaml.Controls.StackPanel)XamlMarkup.XamlReader.Load(rootPanelXaml);
                    xamlStackPanel2 = (Xaml.Controls.StackPanel)XamlMarkup.XamlReader.Load(rootPanelXaml2);
                    xamlButton1 = (Xaml.Controls.Button)xamlStackPanel.FindName("xamlButton1");

                    // Xaml Source: 1st Island
                    xamlHost1 = new WindowsXamlHostWrapper()
                    {
                        Height = 50,
                        Width = 200,
                        Content = xamlStackPanel,
                    };

                    // Add Xaml source into the stackpanel children
                    xamlHost1.InsertInto(children);

                    // Xaml Source: 2nd Island
                    xamlHost2 = new WindowsXamlHostWrapper()
                    {
                        Height = 50,
                        Width = 200,
                        Content = xamlStackPanel2,
                    };
                    xamlHost2.InsertInto(children);

                    // WPF Button2
                    wpfButton2 = new WPFControls.Button()
                    {
                        Height = 30,
                        Width = 100,
                        Name = "wpfButton2",
                        Content = "WPF Button 2",
                    };
                    children.Add(wpfButton2);

                    host.MainWindow.Content = container;
                });
                TestServices.WindowHelper.WaitForIdle();

                host.MainWindow.Dispatcher.Invoke(() =>
                {
                    var childrenSPIsland1 = xamlStackPanel.Children;
                    var childrenSPIsland2 = xamlStackPanel2.Children;

                    // Move StackPanel form 1st island to 2nd
                    childrenSPIsland1.Remove(xamlButton1);
                    childrenSPIsland2.Add(xamlButton1); 
                });
                TestServices.WindowHelper.WaitForIdle();

                // Note: We must dispose our inner xaml contents before setting WPF's content to
                // null. Setting WPF's content to null will disconnect the HWND tree which is 
                // not allowed for a non-disposed DesktopWindowXamlSource object.
                xamlHost1.Dispose();
                xamlHost2.Dispose();

                using (new TestCleanupWrapper())
                {
                    host.MainWindow.Dispatcher.Invoke(() =>
                    {
                        host.MainWindow.Content = null;
                    });
                }
            }
        }

        // Share brush across the islands
        [TestMethod]
        [TestProperty("Description", "Validates objects(e.g. brush) can be shared across Islands")]
        public void ShareBrushAcrossIsland()
        {
            // Disable keyboard input wait as WPF is in charge here.
            using (TestServices.KeyboardHelper.CreateKeyboardWaitKindGuard(KeyboardWaitKind.None))
            {
                var host = TestServices.Win32Host as WPFHost;
                Verify.IsNotNull(host);
                Verify.IsNotNull(host.MainWindow);

                WPFControls.StackPanel container = null;
                WPFControls.Button wpfButton1 = null;
                WPFControls.Button wpfButton2 = null;
                Xaml.Controls.Button xamlButton2 = null;
                WindowsXamlHostWrapper xamlHost1 = null, xamlHost2 = null;
                Xaml.Controls.StackPanel xamlStackPanel = null;
                Microsoft.UI.Xaml.Media.SolidColorBrush sharedBrush = null;                

                const string rootPanelXaml =
                @"<StackPanel xmlns='http://schemas.microsoft.com/winfx/2006/xaml/presentation' xmlns:x='http://schemas.microsoft.com/winfx/2006/xaml'>
                    <Button x:Name='xamlButton1' Content='XAML Button 1' />
                </StackPanel>";

                host.MainWindow.Dispatcher.Invoke(() =>
                {
                    container = new WPFControls.StackPanel();
                    var children = container.Children;
                    
                    sharedBrush = new Microsoft.UI.Xaml.Media.SolidColorBrush(Colors.Black);
                    Verify.IsNotNull(sharedBrush, "Brush is not valid");

                    // WPF Button1
                    wpfButton1 = new WPFControls.Button()
                    {
                        Height = 30,
                        Width = 100,
                        Name = "wpfButton1",
                        Content = "WPF Button 1",
                    };
                    children.Add(wpfButton1);

                    xamlStackPanel = (Xaml.Controls.StackPanel)XamlMarkup.XamlReader.Load(rootPanelXaml);
                    xamlStackPanel.Background = sharedBrush;

                    // Xaml Source: 1st Island
                    xamlHost1 = new WindowsXamlHostWrapper()
                    {
                        Height = 50,
                        Width = 200,
                        Content = xamlStackPanel,
                    };
                    
                    // Add Xaml source into the stackpanel children
                    xamlHost1.InsertInto(children);

                    // Instantiate Xaml Button and put it into the Xaml source
                    xamlButton2 = new Xaml.Controls.Button()
                    {
                        Height = 45,
                        Width = 150,
                        Name = "xamlButton2",
                        Content = new Xaml.Controls.TextBlock()
                        {
                            Text = "XAML Button 2",
                        },
                        Background = sharedBrush,
                    };
                    // Xaml Source
                    xamlHost2 = new WindowsXamlHostWrapper()
                    {
                        Height = 50,
                        Width = 200,
                        Content = xamlButton2,
                    };
                    // Add Xaml source into the stackpanel children
                    xamlHost2.InsertInto(children);

                    // WPF Button2
                    wpfButton2 = new WPFControls.Button()
                    {
                        Height = 30,
                        Width = 100,
                        Name = "wpfButton2",
                        Content = "WPF Button 2",
                        //Background = sharedBrush,
                    };
                    children.Add(wpfButton2);

                    host.MainWindow.Content = container;
                });
                TestServices.WindowHelper.WaitForIdle();

                host.MainWindow.Dispatcher.Invoke(() =>
                {
                    // Move StackPanel form 1st island to 2nd
                    xamlHost1.Content = null;
                    xamlHost2.Content = xamlStackPanel;
                });
                TestServices.WindowHelper.WaitForIdle();

                // Note: We must dispose our inner xaml contents before setting WPF's content to
                // null. Setting WPF's content to null will disconnect the HWND tree which is 
                // not allowed for a non-disposed DesktopWindowXamlSource object.
                xamlHost1.Dispose();
                xamlHost2.Dispose();

                using (new TestCleanupWrapper())
                {
                    host.MainWindow.Dispatcher.Invoke(() =>
                    {
                        host.MainWindow.Content = null;
                    });
                }
            }
        }

        /*
         * Test Layout 
         *
         *                                |-----------Xaml Island  ---------|
         *                                |                                 |
         *                                |   |------------------------|    |
         *                                |   |  XAML Button 1         |    |
         *                                |   |------------------------|    |
         *                                |                                 |
         *                                |   |------------------------|    |
         *                                |   |  XAML Button 2         |    |
         *                                |   |------------------------|    |                                 |
         *                                |---------------------------------|
         *
         */
        [TestMethod]
        [TestProperty("Ignore", "TRUE")] // Test fails when run in loop mode.
        [TestProperty("Description", "Validates tab key input works and moves the focus")]
        public void ValidateFocusBehaviorWithIsland()
        {
            // Disable keyboard input wait as WPF is in charge here.
            using (TestServices.KeyboardHelper.CreateKeyboardWaitKindGuard(KeyboardWaitKind.None))
            {
                var host = TestServices.Win32Host as WPFHost;
                Verify.IsNotNull(host);
                Verify.IsNotNull(host.MainWindow);

                WPFControls.StackPanel container = null;
                WindowsXamlHostWrapper xamlHost1 = null;
                Xaml.Controls.Button xamlButton1 = null;
                Xaml.Controls.Button xamlButton2 = null;
                Xaml.Controls.StackPanel xamlStackPanel = null;

                const string rootPanelXaml =
                @"<StackPanel xmlns='http://schemas.microsoft.com/winfx/2006/xaml/presentation' xmlns:x='http://schemas.microsoft.com/winfx/2006/xaml'>
                    <Button x:Name='xamlButton1' Content='XAML Button 1'/>
                    <Button x:Name='xamlButton2' Content='XAML Button 2'/>
                  </StackPanel>";

                host.MainWindow.Dispatcher.Invoke(() =>
                {
                    container = new WPFControls.StackPanel();
                    var children = container.Children;

                    xamlStackPanel = (StackPanel)XamlMarkup.XamlReader.Load(rootPanelXaml);
                    xamlButton1 = (Button)xamlStackPanel.FindName("xamlButton1");
                    xamlButton2 = (Button)xamlStackPanel.FindName("xamlButton2");

                    // Xaml Source
                    xamlHost1 = new WindowsXamlHostWrapper()
                    {
                        Height = 100,
                        Width = 200,
                        Content = xamlStackPanel,
                    };
                    // Add Xaml source into the stackpanel children
                    xamlHost1.InsertInto(children);

                    host.MainWindow.Content = container;
                });
                TestServices.WindowHelper.WaitForIdle();

                using (xamlHost1)
                using (new TestCleanupWrapper())
                {
                    // Set initial focus on xamlButton1 
                    FocusHelper.EnsureFocus(xamlButton1, FocusState.Keyboard);

                    // Verify tab moves focus to next xamlButton2
                    TestServices.KeyboardHelper.Tab();
                    TestServices.WindowHelper.WaitForIdle();
                    host.MainWindow.Dispatcher.Invoke(() =>
                    {
                        Verify.AreEqual(xamlButton2, Xaml.Input.FocusManager.GetFocusedElement(xamlButton2.XamlRoot));
                        Log.Comment("Focus is on " + xamlButton2.Name);
                    });

                    // Verify tab moves focus back to xamlButton1
                    TestServices.KeyboardHelper.Tab();
                    TestServices.WindowHelper.WaitForIdle();

                    host.MainWindow.Dispatcher.Invoke(() =>
                    {
                        Verify.AreEqual(xamlButton1, Xaml.Input.FocusManager.GetFocusedElement(xamlButton1.XamlRoot));
                        Log.Comment("Focus is on " + xamlButton1.Name);
                    });

                    // Verify shift+tab moves focus to XamlButton2 across the Island
                    TestServices.KeyboardHelper.ShiftTab();
                    TestServices.WindowHelper.WaitForIdle();
                    host.MainWindow.Dispatcher.Invoke(() =>
                    {
                        Verify.AreEqual(xamlButton2, Xaml.Input.FocusManager.GetFocusedElement(xamlButton2.XamlRoot));
                        Log.Comment("Focus is on " + xamlButton2.Name);
                    });

                    TestServices.WindowHelper.WaitForIdle();

                    host.MainWindow.Dispatcher.Invoke(() =>
                    {
                        host.MainWindow.Content = null;
                    });
                }
            }
        }

        public static void Sleep(int milliseconds)
        {
            new AutoResetEvent(false).WaitOne(milliseconds);
        }
    }// End of Test Class

    partial class MyControl : Control
    {
        protected override Size MeasureOverride(Size availableSize)
        {
            Log.Comment("MyControl MeasureOverride");
            var myParent = Microsoft.UI.Xaml.Media.VisualTreeHelper.GetParent(this);
            Verify.IsTrue(myParent is StackPanel);
            return base.MeasureOverride(availableSize);
        }
    }
}// End of Namespace
