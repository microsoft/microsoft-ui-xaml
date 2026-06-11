// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.
using System;
using System.Text;
using System.Linq;
using System.Threading.Tasks;

using WEX.Logging.Interop;
using WEX.TestExecution;
using WEX.TestExecution.Markup;

using Private.Infrastructure;
using Microsoft.UI.Xaml.Tests.Common;
using Microsoft.UI.Xaml.Tests.Common.EventsListeners;

using XamlControls = Microsoft.UI.Xaml.Controls;
using XamlMedia = Microsoft.UI.Xaml.Media;
using XamlMarkup = Microsoft.UI.Xaml.Markup;
using Microsoft.UI.Xaml.Controls;
using Microsoft.UI.Xaml.Documents;
using Microsoft.UI.Xaml.Input;
using Windows.Foundation;
using System.Threading;
using System.Diagnostics;
using System.Collections.Generic;
using Microsoft.UI.Xaml.Controls.Primitives;

using System.Runtime.InteropServices.WindowsRuntime;

namespace Microsoft.UI.Xaml.Tests.Foundation.Graphics.AccessKeys
{
    [TestClass]
    public class AccessKeyTests : XamlTestsBase
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
        public void VerifyAccessKeyLifetime()
        {
            Page page1 = null;
            Page page2 = null;
            Button button2 = null;

            const string pageXaml =
                @"<Page xmlns='http://schemas.microsoft.com/winfx/2006/xaml/presentation' xmlns:x='http://schemas.microsoft.com/winfx/2006/xaml'>
                    <StackPanel Margin='10'>
                        <Button x:Name='button' AccessKey='qu'>button</Button>
                    </StackPanel>
                </Page>";

            UIExecutor.Execute(() =>
            {
                page1 = (Page)XamlMarkup.XamlReader.Load(pageXaml);
                page2 = (Page)XamlMarkup.XamlReader.Load(pageXaml);
                button2 = (Button)page2.FindName("button");

                TestServices.WindowHelper.WindowContent = page1;
            });
            TestServices.WindowHelper.WaitForIdle();
            AccessKeyHelper.TryMovingFocusToXamlForInit();


            XamlRoot xamlRoot = null;
            UIExecutor.Execute(() =>
            {
                xamlRoot = page1.XamlRoot;
            });

            using (AccessKeyHelper.CreateNoKeyTips(xamlRoot))
            using (TestServices.KeyboardHelper.CreateKeyboardWaitKindGuard(KeyboardWaitKind.None))
            {
                Log.Comment("Enter and exit AK mode");
                using (var eventTester = EventTester<object,object>.FromStaticEvent<AccessKeyManager>("IsDisplayModeEnabledChanged"))
                {
                    TestServices.KeyboardHelper.Alt();
                    eventTester.Wait();
                    TestServices.KeyboardHelper.Alt();
                    eventTester.Wait();
                }

                Log.Comment("Load a new page");
                UIExecutor.Execute(() =>
                {
                    TestServices.WindowHelper.WindowContent = page2;
                });
                TestServices.WindowHelper.WaitForIdle();

                using (var eventTester = EventTester<object,object>.FromStaticEvent<AccessKeyManager>("IsDisplayModeEnabledChanged"))
                {
                    TestServices.KeyboardHelper.Alt();
                    eventTester.Wait();
                }

                // Call LeaveImpl on button to trigger the leave-while-in-scope bug
                UIExecutor.Execute(() =>
                {
                    page1.Content = null;
                });
                TestServices.WindowHelper.WaitForIdle();

                using (var eventTester = new EventTester<Button, AccessKeyDisplayRequestedEventArgs>(button2, "AccessKeyDisplayRequested"))
                {
                    TestServices.KeyboardHelper.PressKeySequence("q");
                    eventTester.Wait();
                }

            }
        }

        [TestMethod]
        [TestProperty("TestPass:ExcludeOn", "WindowsCore")]
        [TestProperty("Ignore", "True")]
        public void VerifyAccessKeyHiddenOnMouseScroll()
        {
            using (TestServices.KeyboardHelper.CreateKeyboardWaitKindGuard(KeyboardWaitKind.None))
            {
                Page page1 = null;
                Button button = null;

                const string pageXaml =
                    @"<Page xmlns='http://schemas.microsoft.com/winfx/2006/xaml/presentation' xmlns:x='http://schemas.microsoft.com/winfx/2006/xaml'>
                        <StackPanel Margin='10'>
                            <Button x:Name='button' AccessKey='qu'>button</Button>
                        </StackPanel>
                    </Page>";

                UIExecutor.Execute(() =>
                {
                    page1 = (Page)XamlMarkup.XamlReader.Load(pageXaml);
                    button = (Button)page1.FindName("button");
                    TestServices.WindowHelper.WindowContent = page1;
                });
                TestServices.WindowHelper.WaitForIdle();

                Log.Comment("Enter AK mode");
                using (var eventTester = EventTester<object,object>.FromStaticEvent<AccessKeyManager>("IsDisplayModeEnabledChanged"))
                {
                    TestServices.KeyboardHelper.Alt();
                    eventTester.Wait();
                }

                TestServices.WindowHelper.WaitForIdle();

                Log.Comment("Scrolling mouse wheel");
                using (var dismissedEventTester = new EventTester<Button, AccessKeyDisplayDismissedEventArgs>(button, "AccessKeyDisplayDismissed"))
                {
                    TestServices.InputHelper.ScrollMouseWheel(button, -10);
                    dismissedEventTester.Wait();
                }
            }
        }

        [TestMethod]
        public void VerifyHyperlinkCanHaveScopeOwner()
        {
            const string rootPanelXaml =
                    @"<StackPanel xmlns='http://schemas.microsoft.com/winfx/2006/xaml/presentation' xmlns:x='http://schemas.microsoft.com/winfx/2006/xaml'>
                        <StackPanel IsAccessKeyScope='True'>
                            <TextBlock><Hyperlink x:Name='hyperlink' AccessKey='H'>hyperlink</Hyperlink></TextBlock>
                        </StackPanel>
                        <StackPanel AccessKey='1' x:Name='scope2' IsAccessKeyScope='True'>
                            <Button Content='Button' AccessKey='B' />
                        </StackPanel>
                    </StackPanel>";

            StackPanel rootPanel = null;
            StackPanel scope = null;
            Hyperlink hyperlink = null;

            UIExecutor.Execute(() =>
            {
                rootPanel = (StackPanel)XamlMarkup.XamlReader.Load(rootPanelXaml);
                hyperlink = (Hyperlink)rootPanel.FindName("hyperlink");
                scope = (StackPanel)rootPanel.FindName("scope2");

                hyperlink.AccessKeyScopeOwner = scope;

                TestServices.WindowHelper.WindowContent = rootPanel;
            });

            TestServices.WindowHelper.WaitForIdle();

            XamlRoot xamlRoot = null;
            UIExecutor.Execute(() =>
            {
                xamlRoot = rootPanel.XamlRoot;
            });

            using (new AccessKeyHelper(xamlRoot, false))
            using (TestServices.KeyboardHelper.CreateKeyboardWaitKindGuard(KeyboardWaitKind.None))
            {
                Log.Comment("Enter AK mode");
                using (var eventTester = EventTester<object,object>.FromStaticEvent<AccessKeyManager>("IsDisplayModeEnabledChanged"))
                {
                    TestServices.KeyboardHelper.Alt();
                    eventTester.Wait();
                }

                using (var eventTester = new EventTester<DependencyObject, AccessKeyInvokedEventArgs>(hyperlink, "AccessKeyInvoked"))
                {
                    Log.Comment("Press 1");
                    TestServices.KeyboardHelper.PressKeySequence("1");
                    TestServices.WindowHelper.WaitForIdle();

                    Log.Comment("Press H");
                    TestServices.KeyboardHelper.PressKeySequence("H");
                    eventTester.Wait();
                }
            }
        }

        [TestMethod]
        public void VerifyTextElementCanBeScopeOwner()
        {
            const string rootPanelXaml =
                    @"<StackPanel xmlns='http://schemas.microsoft.com/winfx/2006/xaml/presentation' xmlns:x='http://schemas.microsoft.com/winfx/2006/xaml'>
                        <RichTextBlock>
                            <Paragraph IsAccessKeyScope='True' AccessKey='p'>
                                This is paragraph one with a
                                <Hyperlink x:Name='hyperlink' AccessKey='h'>hyperlink</Hyperlink>
                            </Paragraph>
                            <Paragraph IsAccessKeyScope='True' AccessKey='q'>
                                This is paragraph two with a
                                <Hyperlink x:Name='hyperlink2' AccessKey='h'>hyperlink</Hyperlink>
                            </Paragraph>
                        </RichTextBlock>
                    </StackPanel>";

            StackPanel rootPanel = null;
            Hyperlink hyperlink = null;
            Hyperlink hyperlink2 = null;

            UIExecutor.Execute(() =>
            {
                rootPanel = (StackPanel)XamlMarkup.XamlReader.Load(rootPanelXaml);
                hyperlink = (Hyperlink)rootPanel.FindName("hyperlink");
                hyperlink2 = (Hyperlink)rootPanel.FindName("hyperlink2");

                TestServices.WindowHelper.WindowContent = rootPanel;
            });

            TestServices.WindowHelper.WaitForIdle();

            XamlRoot xamlRoot = null;
            UIExecutor.Execute(() =>
            {
                xamlRoot = rootPanel.XamlRoot;
            });

            using (new AccessKeyHelper(xamlRoot, true))
            using (TestServices.KeyboardHelper.CreateKeyboardWaitKindGuard(KeyboardWaitKind.None))
            {
                Log.Comment("Enter AK mode");
                using (var eventTester = EventTester<object,object>.FromStaticEvent<AccessKeyManager>("IsDisplayModeEnabledChanged"))
                {
                    TestServices.KeyboardHelper.Alt();
                    eventTester.Wait();
                }

                TestServices.WindowHelper.WaitForIdle();


                using (var eventTester2 = new EventTester<DependencyObject, AccessKeyInvokedEventArgs>(hyperlink2, "AccessKeyInvoked"))
                using (var eventTester = new EventTester<DependencyObject, AccessKeyInvokedEventArgs>(hyperlink, "AccessKeyInvoked"))
                {
                    Log.Comment("Press p");
                    TestServices.KeyboardHelper.PressKeySequence("p");
                    TestServices.WindowHelper.WaitForIdle();

                    Log.Comment("Press h");
                    TestServices.KeyboardHelper.PressKeySequence("h");
                    eventTester.Wait();
                    eventTester2.WaitForNoThrow(TimeSpan.FromMilliseconds(100));
                }
            }
        }

        [TestMethod]
        [TestProperty("Description", "Validates access keys on pivot control.")]
        [TestProperty("Hosting:Mode", "UAP")]
        public void VerifyAccessKeysWorkOnPivotControl()
        {
            const string rootPanelXaml =
                @"<StackPanel xmlns='http://schemas.microsoft.com/winfx/2006/xaml/presentation' xmlns:x='http://schemas.microsoft.com/winfx/2006/xaml'>
                    <Pivot x:Name='rootPivot' Title='PIVOT WITH ACCESS KEYS' >
                        <Pivot.RightHeader>
                            <CommandBar ClosedDisplayMode='Compact'>
                                <AppBarButton x:Name='Back' Icon='Back' Label='Previous'  />
                                <AppBarButton x:Name='Forward' Icon='Forward' Label='Next'   />
                            </CommandBar>
                        </Pivot.RightHeader>
                        <PivotItem x:Name = 'pivotItem1' Header='Pivot Item P' AccessKey='p'>
                            <TextBlock Text='Content of pivot item 1.' />
                        </PivotItem>
                        <PivotItem x:Name = 'pivotItem2' Header='Pivot Item Q' AccessKey='q'>
                            <TextBlock Text='Content of pivot item 2.' />
                        </PivotItem>
                        <PivotItem x:Name = 'pivotItem3' Header='Pivot Item R' AccessKey='r'>
                            <TextBlock Text='Content of pivot item 3.' />
                        </PivotItem>
                    </Pivot>
                </StackPanel>";

            StackPanel rootPanel = null;
            Pivot pivot = null;

            UIExecutor.Execute(() =>
            {
                rootPanel = (StackPanel)XamlMarkup.XamlReader.Load(rootPanelXaml);
                pivot = (Pivot)rootPanel.FindName("rootPivot");

                TestServices.WindowHelper.WindowContent = rootPanel;
            });
            TestServices.WindowHelper.WaitForIdle();

            XamlRoot xamlRoot = null;
            UIExecutor.Execute(() =>
            {
                xamlRoot = rootPanel.XamlRoot;
            });

            using (new AccessKeyHelper(xamlRoot, true))
            using (TestServices.KeyboardHelper.CreateKeyboardWaitKindGuard(KeyboardWaitKind.None))
            {
                var selectionChangedEventHandler_P = new Action<object, SelectionChangedEventArgs>((source, args) =>
                {
                    verifySelectedPivotItemChanged(source, 0);
                });

                var selectionChangedEventHandler_Q = new Action<object, SelectionChangedEventArgs>((source, args) =>
                {
                    verifySelectedPivotItemChanged(source, 1);
                });

                var selectionChangedEventHandler_R = new Action<object, SelectionChangedEventArgs>((source, args) =>
                {
                    verifySelectedPivotItemChanged(source, 2);
                });

                Log.Comment("Verifying HotKeys");
                using (var eventTesterR = new EventTester<Pivot, SelectionChangedEventArgs>(pivot, "SelectionChanged", selectionChangedEventHandler_R))
                {
                    Log.Comment("Press r");
                    TestServices.KeyboardHelper.PressKeySequence("$d$_alt#$d$_r#$u$_r#$u$_alt");
                    eventTesterR.Wait();
                    TestServices.WindowHelper.WaitForIdle();
                }

                using (var eventTesterP = new EventTester<Pivot, SelectionChangedEventArgs>(pivot, "SelectionChanged", selectionChangedEventHandler_P))
                {
                    Log.Comment("Press p");
                    TestServices.KeyboardHelper.PressKeySequence("$d$_alt#$d$_p#$u$_p#$u$_alt");
                    eventTesterP.Wait();
                    TestServices.WindowHelper.WaitForIdle();
                }

                Log.Comment("Enter AK mode");
                using (var eventTester = EventTester<object, object>.FromStaticEvent<AccessKeyManager>("IsDisplayModeEnabledChanged"))
                {
                    TestServices.KeyboardHelper.Alt();
                    eventTester.Wait();
                }
                TestServices.WindowHelper.WaitForIdle();

                using (var eventTesterQ = new EventTester<Pivot, SelectionChangedEventArgs>(pivot, "SelectionChanged", selectionChangedEventHandler_Q))
                {
                    Log.Comment("Press q");
                    TestServices.KeyboardHelper.PressKeySequence("$d$_q#$u$_q");
                    eventTesterQ.Wait();
                    TestServices.WindowHelper.WaitForIdle();
                }
            }
        }

        [TestMethod]
        [TestProperty("Description", "In a multi-level flyout menu, press access keys quickly to navigate the whole menu.  It often crashes, and rarely actually traverses the whole menu.")]
        [TestProperty("Hosting:Mode", "UAP")]
        public void MenuFlyoutMultiLevel()
        {
            Page root = null;
            Button button1 = null;
            UIExecutor.Execute(() =>
            {
                root = (Page)XamlMarkup.XamlReader.Load(@"
                <Page xmlns='http://schemas.microsoft.com/winfx/2006/xaml/presentation' xmlns:x='http://schemas.microsoft.com/winfx/2006/xaml'>
                    <StackPanel Margin='20'>
                        <Button AccessKey='1' KeyTipPlacementMode='Right' Content='b1' IsAccessKeyScope='True' x:Name='button1'>
                            <Button.Flyout>
                                <MenuFlyout>
                                    <MenuFlyoutItem AccessKey='a' KeyTipPlacementMode='Right' x:Name='itemA'>itemA</MenuFlyoutItem>
                                    <MenuFlyoutItem AccessKey='b' KeyTipPlacementMode='Right' x:Name='itemB'>itemB</MenuFlyoutItem>
                                    <MenuFlyoutSubItem AccessKey='s' KeyTipPlacementMode='Right' IsAccessKeyScope='True' Text='sub'>
                                        <MenuFlyoutItem AccessKey='a' KeyTipPlacementMode='Right' x:Name='level1_itemA'>itemA</MenuFlyoutItem>
                                        <MenuFlyoutItem AccessKey='b' KeyTipPlacementMode='Right' x:Name='level1_itemB'>itemB</MenuFlyoutItem>
                                        <MenuFlyoutSubItem AccessKey='s' KeyTipPlacementMode='Right' IsAccessKeyScope='True' Text='sub'>
                                            <MenuFlyoutItem AccessKey='a' KeyTipPlacementMode='Right' x:Name='level2_itemA'>itemA</MenuFlyoutItem>
                                            <MenuFlyoutItem AccessKey='b' KeyTipPlacementMode='Right' x:Name='level2_itemB'>itemB</MenuFlyoutItem>
                                            <MenuFlyoutSubItem AccessKey='s' KeyTipPlacementMode='Right' IsAccessKeyScope='True' Text='sub'>
                                                <MenuFlyoutItem AccessKey='a' KeyTipPlacementMode='Right' x:Name='level3_itemA'>itemA</MenuFlyoutItem>
                                                <MenuFlyoutItem AccessKey='b' KeyTipPlacementMode='Right' x:Name='level3_itemB'>itemB</MenuFlyoutItem>
                                            </MenuFlyoutSubItem>
                                        </MenuFlyoutSubItem>
                                    </MenuFlyoutSubItem>
                                </MenuFlyout>
                            </Button.Flyout>
                        </Button>
                        <Button AccessKey='2' Content='b2' KeyTipPlacementMode='Right' />
                    </StackPanel>
                </Page>");
                button1 = (Button)root.FindName("button1");
                TestServices.WindowHelper.WindowContent = root;
            });
            TestServices.WindowHelper.WaitForIdle();
            AccessKeyHelper.TryMovingFocusToXamlForInit();

            XamlRoot xamlRoot = null;
            UIExecutor.Execute(() =>
            {
                xamlRoot = root.XamlRoot;
            });

            using (AccessKeyHelper.CreateNoKeyTips(xamlRoot)) // Let's make sure this isn't KeyTip's fault...
            using (TestServices.KeyboardHelper.CreateKeyboardWaitKindGuard(KeyboardWaitKind.None))
            {
                Log.Comment("I'm going to press some keys, please don't crash!");

                TestServices.KeyboardHelper.Alt();
                TestServices.KeyboardHelper.PressKeySequence("1");
                TestServices.KeyboardHelper.PressKeySequence("s");
                TestServices.KeyboardHelper.PressKeySequence("s");
                TestServices.KeyboardHelper.PressKeySequence("s");

                TestServices.WindowHelper.WaitForIdle();

                Log.Comment("Now back out with escapes...");

                TestServices.KeyboardHelper.Escape();
                TestServices.KeyboardHelper.Escape();
                TestServices.KeyboardHelper.Escape();
                TestServices.KeyboardHelper.Escape();
                TestServices.KeyboardHelper.Escape();
            }
        }

        [TestMethod]
        public void MenuFlyout()
        {
            Page root = null;
            Button button1 = null;
            MenuFlyoutItem itemA = null;

            UIExecutor.Execute(() =>
            {
                root = (Page)XamlMarkup.XamlReader.Load(@"
                <Page xmlns='http://schemas.microsoft.com/winfx/2006/xaml/presentation' xmlns:x='http://schemas.microsoft.com/winfx/2006/xaml'>
                    <StackPanel Margin='20'>
                        <Button AccessKey='1' Content='button1' IsAccessKeyScope='True' x:Name='button1'>
                            <Button.Flyout>
                                <MenuFlyout>
                                    <MenuFlyoutItem AccessKey='a' x:Name='itemA'>itemA</MenuFlyoutItem>
                                </MenuFlyout>
                            </Button.Flyout>
                        </Button>
                    </StackPanel>
                </Page>");
                button1 = (Button)root.FindName("button1");
                itemA = (MenuFlyoutItem)root.FindName("itemA");
                TestServices.WindowHelper.WindowContent = root;
            });
            TestServices.WindowHelper.WaitForIdle();
            AccessKeyHelper.TryMovingFocusToXamlForInit();

            XamlRoot xamlRoot = null;
            UIExecutor.Execute(() =>
            {
                xamlRoot = root.XamlRoot;
            });

            using (AccessKeyHelper.CreateNoKeyTips(xamlRoot))
            {
                using (var button1DisplayRequested = new EventTester<Button, AccessKeyDisplayRequestedEventArgs>(button1, "AccessKeyDisplayRequested"))
                using (var itemADisplayRequested = new EventTester<MenuFlyoutItem, AccessKeyDisplayRequestedEventArgs>(itemA, "AccessKeyDisplayRequested"))
                {
                    TestServices.KeyboardHelper.Alt();
                    button1DisplayRequested.Wait();

                    Verify.AreEqual(1, button1DisplayRequested.ExecuteCount);
                    Verify.AreEqual(0, itemADisplayRequested.ExecuteCount);

                    TestServices.KeyboardHelper.PressKeySequence("1");
                    itemADisplayRequested.Wait();

                    Verify.AreEqual(1, button1DisplayRequested.ExecuteCount);
                    Verify.AreEqual(1, itemADisplayRequested.ExecuteCount);

                    TestServices.KeyboardHelper.Escape();
                    button1DisplayRequested.Wait();

                    TestServices.WindowHelper.SynchronouslyTickUIThread(2);

                    Verify.AreEqual(2, button1DisplayRequested.ExecuteCount);
                    Verify.AreEqual(1, itemADisplayRequested.ExecuteCount);
                }
            }
        }

        [TestMethod]
        [TestProperty("Hosting:Mode", "WPF")]
        public void CommandBarFlyout()
        {
            // This is similar to File Explorer's Context Menus.
            // We have an island with a dummy Grid.  The real UI is a CommandBarFlyout, which is shown programatically.
            // To make the CommandBarFlyout's primary and secondary commands all show up, we set the AccessKeyScopeOwner for all the items
            // to the dummy Grid.  This test ensures the user can press Escape to back all the way out the CommandBarFlyout.
            Grid root = null;
            CommandBarFlyout commandBarFlyout = null;
            AutoResetEvent cbfOpened = new AutoResetEvent(false);
            AppBarButton appBarButton1 = null;
            AppBarButton appBarButtonA = null;

            UIExecutor.Execute(() =>
            {
                root = new Grid();
                TestServices.WindowHelper.WindowContent = root;

                root.IsAccessKeyScope = true;
                commandBarFlyout = new CommandBarFlyout();
                
                commandBarFlyout.PrimaryCommands.Add(appBarButton1 = new AppBarButton() { Label = "item 1", AccessKey = "1", AccessKeyScopeOwner=root });
                commandBarFlyout.PrimaryCommands.Add(new AppBarButton() { Label = "item 2", AccessKey = "2", AccessKeyScopeOwner = root });
                commandBarFlyout.PrimaryCommands.Add(new AppBarButton() { Label = "item 3", AccessKey = "3", AccessKeyScopeOwner = root });
                commandBarFlyout.SecondaryCommands.Add(appBarButtonA = new AppBarButton() { Label = "item a", AccessKey = "a", AccessKeyScopeOwner = root });
                commandBarFlyout.SecondaryCommands.Add(new AppBarButton() { Label = "item b", AccessKey = "b", AccessKeyScopeOwner = root });
            });
            TestServices.WindowHelper.WaitForIdle();
            AccessKeyHelper.TryMovingFocusToXamlForInit();

            TestServices.WindowHelper.WaitForIdle();

            XamlRoot xamlRoot = null;
            UIExecutor.Execute(() => {xamlRoot = root.XamlRoot;});

            using (AccessKeyHelper.CreateWithKeyTips(xamlRoot))
            using (var isDisplayModeEnabledChanged = EventTester<object,object>.FromStaticEvent<AccessKeyManager>("IsDisplayModeEnabledChanged"))
            using (var appBarButton1DisplayRequested = new EventTester<AppBarButton, AccessKeyDisplayRequestedEventArgs>(appBarButton1, "AccessKeyDisplayRequested"))
            using (var appBarButton1DisplayDismissed = new EventTester<AppBarButton, AccessKeyDisplayDismissedEventArgs>(appBarButton1, "AccessKeyDisplayDismissed"))
            using (var appBarButtonADisplayRequested = new EventTester<AppBarButton, AccessKeyDisplayRequestedEventArgs>(appBarButtonA, "AccessKeyDisplayRequested"))
            using (var appBarButtonADisplayDismissed = new EventTester<AppBarButton, AccessKeyDisplayDismissedEventArgs>(appBarButtonA, "AccessKeyDisplayDismissed"))
            {
                UIExecutor.Execute(() =>
                {                    
                    commandBarFlyout.XamlRoot = xamlRoot;
                    commandBarFlyout.Opened += (s,e) => { cbfOpened.Set(); };
                    var options = new FlyoutShowOptions()
                    {
                        Position = new Point (100, 100)
                    };
                    commandBarFlyout.ShowAt(root, options);
                });
                TestServices.WindowHelper.WaitForIdle();
                cbfOpened.WaitOne();

                UIExecutor.Execute(() =>
                {
                    ((AppBarButton)commandBarFlyout.PrimaryCommands[0]).Focus(FocusState.Keyboard);
                });
                TestServices.WindowHelper.WaitForIdle();

                Log.Comment("Enter AccessKeys EnterDisplayMode programatically.");
                UIExecutor.Execute(() =>
                {
                    AccessKeyManager.EnterDisplayMode(xamlRoot);
                });

                isDisplayModeEnabledChanged.Wait();
                appBarButton1DisplayRequested.Wait();
                appBarButtonADisplayRequested.Wait();

                Verify.AreEqual(1, appBarButton1DisplayRequested.ExecuteCount);
                Verify.AreEqual(0, appBarButton1DisplayDismissed.ExecuteCount);
                Verify.AreEqual(1, appBarButtonADisplayRequested.ExecuteCount);
                Verify.AreEqual(0, appBarButtonADisplayDismissed.ExecuteCount);

                Log.Comment("Press Escape.  We expect to exit DisplayMode completely.");
                TestServices.KeyboardHelper.Escape();

                isDisplayModeEnabledChanged.Wait();
                appBarButton1DisplayDismissed.Wait();
                appBarButtonADisplayDismissed.Wait();

                Verify.AreEqual(1, appBarButton1DisplayRequested.ExecuteCount);
                Verify.AreEqual(1, appBarButton1DisplayDismissed.ExecuteCount);
                Verify.AreEqual(1, appBarButtonADisplayRequested.ExecuteCount);
                Verify.AreEqual(1, appBarButtonADisplayDismissed.ExecuteCount);
            }
        }

        [TestMethod]
        public void VerifyAccessKeyNotInvokedIfParentIsCollapsed()
        {
            StackPanel rootPanel = null;
            Button button = null;
            Button button2 = null;
            Hyperlink hyperlink = null;
            StackPanel parentPanel = null;

            const string rootPanelXaml =
                @"<StackPanel xmlns='http://schemas.microsoft.com/winfx/2006/xaml/presentation' xmlns:x='http://schemas.microsoft.com/winfx/2006/xaml'>
                        <StackPanel Margin='10' x:Name='parentPanel' Visibility='Visible'>
                            <Button x:Name='button' AccessKey='A'>button</Button>
                            <TextBlock><Hyperlink x:Name='hyperlink' AccessKey='H'>hyperlink</Hyperlink></TextBlock>
                        </StackPanel>
                        <Button x:Name='button2' AccessKey='B'>FocusTrap</Button>
                    </StackPanel>";

            UIExecutor.Execute(() =>
            {
                rootPanel = (StackPanel)XamlMarkup.XamlReader.Load(rootPanelXaml);
                button = (Button)rootPanel.FindName("button");
                button2 = (Button)rootPanel.FindName("button2");
                hyperlink = (Hyperlink)rootPanel.FindName("hyperlink");
                parentPanel = (StackPanel)rootPanel.FindName("parentPanel");
                TestServices.WindowHelper.WindowContent = rootPanel;
            });
            TestServices.WindowHelper.WaitForIdle();

            XamlRoot xamlRoot = null;
            UIExecutor.Execute(() =>
            {
                xamlRoot = rootPanel.XamlRoot;
            });

            using (AccessKeyHelper.CreateWithKeyTips(xamlRoot))
            using (TestServices.KeyboardHelper.CreateKeyboardWaitKindGuard(KeyboardWaitKind.None))
            {
                var verifyAKNotInvokedHandler = new Action<object, AccessKeyInvokedEventArgs>((source, args) =>
                {
                    Verify.Fail("AccessKey invoked when parent was collapsed");
                });

                FocusHelper.EnsureFocus(button2, FocusState.Keyboard);

                Log.Comment("Enter AK mode");
                using (var akDisplayRequested = new EventTester<Button, AccessKeyDisplayRequestedEventArgs>(button2, "AccessKeyDisplayRequested"))
                {
                    TestServices.KeyboardHelper.Alt();
                    TestServices.WindowHelper.WaitForIdle();
                    akDisplayRequested.Wait();
                }

                using (var eventTester = new EventTester<DependencyObject, AccessKeyInvokedEventArgs>(button, "AccessKeyInvoked"))
                {
                    Log.Comment("Press A");
                    TestServices.KeyboardHelper.PressKeySequence("a");
                    eventTester.Wait();
                }

                using (var eventTester = EventTester<object, object>.FromStaticEvent<AccessKeyManager>("IsDisplayModeEnabledChanged"))
                {
                    TestServices.KeyboardHelper.Alt();
                    eventTester.Wait();
                }

                using (var eventTester = new EventTester<DependencyObject, AccessKeyInvokedEventArgs>(hyperlink, "AccessKeyInvoked"))
                {
                    Log.Comment("Press H");
                    TestServices.KeyboardHelper.PressKeySequence("h");
                    eventTester.Wait();
                }

                TestServices.WindowHelper.WaitForIdle();
                UIExecutor.Execute(() =>
                {
                    parentPanel.Visibility = Visibility.Collapsed;
                });
                TestServices.WindowHelper.WaitForIdle();

                Log.Comment("Enter AK mode");
                using (var akDisplayRequested = new EventTester<Button, AccessKeyDisplayRequestedEventArgs>(button2, "AccessKeyDisplayRequested"))
                {
                    TestServices.KeyboardHelper.Alt();
                    akDisplayRequested.Wait();
                }

                using (var eventTester = new EventTester<DependencyObject, AccessKeyInvokedEventArgs>(button, "AccessKeyInvoked", verifyAKNotInvokedHandler))
                {
                    Log.Comment("Press A");
                    TestServices.KeyboardHelper.PressKeySequence("a");
                    TestServices.WindowHelper.WaitForIdle();
                    eventTester.WaitForNoThrow(TimeSpan.FromMilliseconds(150));
                    Verify.IsFalse(eventTester.HasFired);
                }

                using (var eventTester = new EventTester<DependencyObject, AccessKeyInvokedEventArgs>(hyperlink, "AccessKeyInvoked", verifyAKNotInvokedHandler))
                {
                    Log.Comment("Press H");
                    TestServices.KeyboardHelper.PressKeySequence("h");
                    TestServices.WindowHelper.WaitForIdle();
                    eventTester.WaitForNoThrow(TimeSpan.FromMilliseconds(150));
                    Verify.IsFalse(eventTester.HasFired);
                }

                TestServices.KeyboardHelper.Escape();
                TestServices.WindowHelper.WaitForIdle();
            }
        }

        [TestMethod]
        [TestProperty("Ignore", "True")] // Crash in KeyTipManager::CreatePopup when running test AccessKeyTests.VerifyAccessKeysAddedWhenInAKModeAreShown
        [TestProperty("HasAssociatedMasterFile", "True")]
        public void VerifyAccessKeysAddedWhenInAKModeAreShown()
        {
            StackPanel rootPanel = null;
            Button button = null;
            Button button2 = null;
            Hyperlink hyperlink = null;

            const string rootPanelXaml =
                @"<StackPanel xmlns='http://schemas.microsoft.com/winfx/2006/xaml/presentation' xmlns:x='http://schemas.microsoft.com/winfx/2006/xaml'>
                        <Button x:Name='button' AccessKey='A' KeyTipPlacementMode='Right'>button</Button>
                        <TextBlock><Hyperlink x:Name='hyperlink'>hyperlink</Hyperlink></TextBlock>
                    </StackPanel>";

            UIExecutor.Execute(() =>
            {
                rootPanel = (StackPanel)XamlMarkup.XamlReader.Load(rootPanelXaml);
                button = (Button)rootPanel.FindName("button");
                hyperlink = (Hyperlink)rootPanel.FindName("hyperlink");
                button2 = new Button();
                button2.Content = "Button2";
                button2.AccessKey = "B";
                TestServices.WindowHelper.WindowContent = rootPanel;
            });
            TestServices.WindowHelper.WaitForIdle();
            AccessKeyHelper.TryMovingFocusToXamlForInit();

            XamlRoot xamlRoot = null;
            UIExecutor.Execute(() =>
            {
                xamlRoot = rootPanel.XamlRoot;
            });

            using (AccessKeyHelper.CreateWithKeyTips(xamlRoot))
            using (TestServices.KeyboardHelper.CreateKeyboardWaitKindGuard(KeyboardWaitKind.None))
            {
                FocusHelper.EnsureFocus(button, FocusState.Keyboard);

                Log.Comment("Enter AK mode");
                using (var akDisplayRequested = new EventTester<Button, AccessKeyDisplayRequestedEventArgs>(button, "AccessKeyDisplayRequested"))
                {
                    TestServices.KeyboardHelper.Alt();
                    TestServices.WindowHelper.WaitForIdle();
                    akDisplayRequested.Wait();
                }

                UIExecutor.Execute(() =>
                {
                    rootPanel.Children.Add(button2);
                    hyperlink.AccessKey = "C";
                });
                TestServices.WindowHelper.WaitForIdle();


                TestServices.Utilities.VerifyUIElementTree();
                TestServices.KeyboardHelper.Escape();
                TestServices.WindowHelper.WaitForIdle();
            }
        }

        [TestMethod]
        [TestProperty("HasAssociatedMasterFile", "True")]
        [TestProperty("Ignore", "True")] // Unreliable tests in WPF netcore 5: KeyTips tests
        public void VerifyAccessKeysWorkWhenDynamicSelectorsAreAddedWhileInAKMode()
        {
            string dataTemplate =
            " <DataTemplate xmlns='http://schemas.microsoft.com/winfx/2006/xaml/presentation'> " +
            "     <TextBlock Text='Text' AccessKey='{Binding}' />  " +
            " </DataTemplate>";

            AccessKeysWorkWhenDynamicSelectorsAreAddedWhileInAKModeHelper(dataTemplate);
        }

        [TestMethod]
        [TestProperty("HasAssociatedMasterFile", "True")]
        [TestProperty("Ignore", "True")] // Unreliable tests in WPF netcore 5: KeyTips tests
        public void VerifyTextElementAccessKeysWorkWhenDynamicSelectorsAreAddedWhileInAKMode()
        {
            string dataTemplate =
            " <DataTemplate xmlns='http://schemas.microsoft.com/winfx/2006/xaml/presentation'> " +
            "  <TextBlock><Hyperlink AccessKey='{Binding}'>hyperlink</Hyperlink></TextBlock> " +
            " </DataTemplate>";

            AccessKeysWorkWhenDynamicSelectorsAreAddedWhileInAKModeHelper(dataTemplate);
        }

        [TestMethod]
        public void VerifyOnlyCorrectlyScopedAccessKeysAreAddedWhenInAKMode()
        {
            StackPanel rootPanel = null;
            Button button = null;
            Button button2 = null;
            Button button3 = null;

            const string rootPanelXaml =
                @"<StackPanel xmlns='http://schemas.microsoft.com/winfx/2006/xaml/presentation' xmlns:x='http://schemas.microsoft.com/winfx/2006/xaml'>
                        <Button x:Name='button' AccessKey='A' KeyTipPlacementMode='Right' IsAccessKeyScope='True'>button</Button>
                    </StackPanel>";

            UIExecutor.Execute(() =>
            {
                rootPanel = (StackPanel)XamlMarkup.XamlReader.Load(rootPanelXaml);
                button = (Button)rootPanel.FindName("button");

                button2 = new Button();
                button2.Content = "Button2";
                button2.AccessKey = "B";
                button2.AccessKeyScopeOwner = button;

                button3 = new Button();
                button3.Content = "Button3";
                button3.AccessKey = "C";

                TestServices.WindowHelper.WindowContent = rootPanel;
            });
            TestServices.WindowHelper.WaitForIdle();
            AccessKeyHelper.TryMovingFocusToXamlForInit();

            XamlRoot xamlRoot = null;
            UIExecutor.Execute(() =>
            {
                xamlRoot = rootPanel.XamlRoot;
            });

            using (AccessKeyHelper.CreateWithKeyTips(xamlRoot))
            using (TestServices.KeyboardHelper.CreateKeyboardWaitKindGuard(KeyboardWaitKind.None))
            {
                var verifyAKNotInvokedHandler = new Action<object, AccessKeyInvokedEventArgs>((source, args) =>
                {
                    Verify.Fail("AccessKey invoked when parent was collapsed");
                });

                var button1ClickedHandler = new Action<object, RoutedEventArgs>((source, args) =>
                {
                    rootPanel.Children.Add(button2);
                    rootPanel.Children.Add(button3);
                });

                FocusHelper.EnsureFocus(button, FocusState.Keyboard);

                Log.Comment("Enter AK mode");
                using (var akDisplayRequested = new EventTester<Button, AccessKeyDisplayRequestedEventArgs>(button, "AccessKeyDisplayRequested"))
                {
                    TestServices.KeyboardHelper.Alt();
                    TestServices.WindowHelper.WaitForIdle();
                    akDisplayRequested.Wait();
                }

                using (var buttonClicked = new EventTester<Button, RoutedEventArgs>(button, "Click", button1ClickedHandler))
                {
                    Log.Comment("Press A");
                    TestServices.KeyboardHelper.PressKeySequence("a");
                    TestServices.WindowHelper.WaitForIdle();
                    buttonClicked.Wait();
                }

                using (var eventTester = new EventTester<DependencyObject, AccessKeyInvokedEventArgs>(button3, "AccessKeyInvoked", verifyAKNotInvokedHandler))
                {
                    Log.Comment("Press C");
                    TestServices.KeyboardHelper.PressKeySequence("c");
                    eventTester.WaitForNoThrow(TimeSpan.FromMilliseconds(150));
                    Verify.IsFalse(eventTester.HasFired);
                }

                using (var eventTester = new EventTester<DependencyObject, AccessKeyInvokedEventArgs>(button2, "AccessKeyInvoked"))
                {
                    Log.Comment("Press B");
                    TestServices.KeyboardHelper.PressKeySequence("b");
                    eventTester.Wait();
                }
            }
        }

        #region Helpers
        public static void verifySelectedPivotItemChanged(
            object sender,
            int key)
        {
            Pivot senderAsPivot = sender as Pivot;
            Verify.IsTrue(senderAsPivot != null);
            Verify.AreEqual(senderAsPivot.SelectedIndex, key);
        }

        private void AccessKeysWorkWhenDynamicSelectorsAreAddedWhileInAKModeHelper(string dataTemplate)
        {
            StackPanel rootPanel = null;
            Button button = null;
            ListView listView = null;

            const string rootPanelXaml =
                @"<StackPanel xmlns='http://schemas.microsoft.com/winfx/2006/xaml/presentation' xmlns:x='http://schemas.microsoft.com/winfx/2006/xaml'>
                        <Button x:Name='button' AccessKey='O' KeyTipPlacementMode='Right' IsAccessKeyScope='True'>button</Button>
                            </StackPanel>";

            UIExecutor.Execute(() =>
            {
                rootPanel = (StackPanel)XamlMarkup.XamlReader.Load(rootPanelXaml);
                button = (Button)rootPanel.FindName("button");
                listView = new ListView();
                TestServices.WindowHelper.WindowContent = rootPanel;

                listView.ItemsSource = new List<string>(new string[] { "1", "2", "3" });

                DataTemplate dt = (DataTemplate)Microsoft.UI.Xaml.Markup.XamlReader.Load(dataTemplate);

                listView.ItemTemplate = dt;

            });
            TestServices.WindowHelper.WaitForIdle();

            XamlRoot xamlRoot = null;
            UIExecutor.Execute(() =>
            {
                xamlRoot = rootPanel.XamlRoot;
            });

            using (new KeyTipHelper(xamlRoot))
            using (AccessKeyHelper.CreateWithKeyTips(xamlRoot))
            {
                FocusHelper.EnsureFocus(button, FocusState.Keyboard);

                Log.Comment("Enter AK mode");
                using (TestServices.KeyboardHelper.CreateKeyboardWaitKindGuard(KeyboardWaitKind.None))
                using (var akDisplayRequested = new EventTester<Button, AccessKeyDisplayRequestedEventArgs>(button, "AccessKeyDisplayRequested"))
                {
                    TestServices.KeyboardHelper.Alt();
                    TestServices.WindowHelper.WaitForIdle();
                    akDisplayRequested.Wait();
                }

                UIExecutor.Execute(() =>
                {
                    rootPanel.Children.Add(listView);
                });
                TestServices.WindowHelper.WaitForIdle();
                KeyTipHelper.WaitUntilKeyTipCountIs(4);
                TestServices.WindowHelper.WaitForIdle();

                TestServices.Utilities.VerifyUIElementTree();
                TestServices.KeyboardHelper.Escape();
                TestServices.WindowHelper.WaitForIdle();
            }
        }
        #endregion
    }
}

