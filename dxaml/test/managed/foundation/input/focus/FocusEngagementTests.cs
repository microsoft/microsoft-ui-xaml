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

namespace Microsoft.UI.Xaml.Tests.Focus
{
    [TestClass]
    public class FocusEngagementTests : XamlTestsBase
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
        [TestProperty("Description", "Verify Focus engagement behavior when opening a flyout")]
        public void VerifyFocusEngagementOnFlyout()
        {
            StackPanel rootPanel = null;
            ListView lv = null;
            Button focusTrap = null;
            Button flyoutShow = null;
            Flyout flyout = null;

            const string rootPanelXaml =
                 @"<StackPanel
                       xmlns='http://schemas.microsoft.com/winfx/2006/xaml/presentation'
                       xmlns:x='http://schemas.microsoft.com/winfx/2006/xaml'>
                    <Button x:Name='focusTrap' >Focus trap</Button>
                    <ListView IsFocusEngagementEnabled='True'
                              SelectionMode='None'
                              IsItemClickEnabled='True'
                              x:Name='lv' >
                        <Button Content='Click me to show flyout' x:Name='flyoutShow'>
                            <Button.Flyout>
                                <Flyout LightDismissOverlayMode='On' x:Name='flyout'>
                                    <Border Background='Red'
                                            Width='300'
                                            Height='300'
                                            Padding='16'>
                                        <TextBlock TextWrapping='WrapWholeWords'
                                                   HorizontalAlignment='Center'
                                                   VerticalAlignment='Center'
                                                   Text='Lorem' />
                                    </Border>
                                </Flyout>
                            </Button.Flyout>
                        </Button>
                    </ListView>
                </StackPanel>";

            var showFlyout = new Action<object, RoutedEventArgs>((source, args) =>
            {
                flyout.ShowAt(flyoutShow);
            });


            UIExecutor.Execute(() =>
            {
                rootPanel = (StackPanel)XamlReader.Load(rootPanelXaml);

                lv = (ListView)(rootPanel.FindName("lv"));
                focusTrap = (Button)(rootPanel.FindName("focusTrap"));
                flyoutShow = (Button)(rootPanel.FindName("flyoutShow"));
                flyout = (Flyout)(rootPanel.FindName("flyout"));

                TestServices.WindowHelper.WindowContent = rootPanel;
            });

            TestServices.WindowHelper.WaitForIdle();

            FocusHelper.EnsureFocus(focusTrap, FocusState.Keyboard);

            TestServices.KeyboardHelper.GamepadDpadDown();
            TestServices.WindowHelper.WaitForIdle();


            using (var focusEngaged = new EventTester<ListView, FocusEngagedEventArgs>(lv, "FocusEngaged"))
            {
                TestServices.KeyboardHelper.GamepadA();
                focusEngaged.Wait();

                FocusHelper.EnsureFocus(flyoutShow, FocusState.Keyboard);
                UIExecutor.Execute(() =>
                {
                    Verify.IsTrue(lv.IsFocusEngaged);
                });
            }
            TestServices.WindowHelper.WaitForIdle();

            Log.Comment("Opening Flyout");
            using (var btnClicked = new EventTester<Button, RoutedEventArgs>(flyoutShow, "Click", showFlyout))
            using (var flyoutOpened = new EventTester<Flyout, object>(flyout, "Opened"))
            {
                TestServices.KeyboardHelper.GamepadA();
                flyoutOpened.Wait();
                UIExecutor.Execute(() =>
                {
                    Verify.IsTrue(lv.IsFocusEngaged);
                });
            }

            Log.Comment("Closing Flyout");
            using (var flyoutClosed = new EventTester<Flyout, object>(flyout, "Closed"))
            {
                TestServices.KeyboardHelper.GamepadB();
                flyoutClosed.Wait();
                TestServices.WindowHelper.WaitForIdle();
                UIExecutor.Execute(() =>
                {
                    Verify.IsTrue(lv.IsFocusEngaged);
                });
            }

            Log.Comment("Disengaging");
            using (var focusDisengaged = new EventTester<ListView, FocusDisengagedEventArgs>(lv, "FocusDisengaged"))
            {
                TestServices.KeyboardHelper.GamepadB();
                focusDisengaged.Wait();
                UIExecutor.Execute(() =>
                {
                    Verify.AreEqual(lv, FocusManager.GetFocusedElement(TestServices.WindowHelper.WindowContent.XamlRoot));
                });
            }

        }

        [TestMethod]
        [TestProperty("Description", "Verify Focus engagement behavior when opening a MenuFlyout")]
        [TestProperty("Hosting:Mode", "UAP")]
        public void VerifyFocusEngagementOnMenuFlyout()
        {
            StackPanel rootPanel = null;
            ListView lv = null;
            Button focusTrap = null;
            Button flyoutShow = null;
            MenuFlyout flyout = null;

            const string rootPanelXaml =
                @"<StackPanel
                       xmlns='http://schemas.microsoft.com/winfx/2006/xaml/presentation'
                       xmlns:x='http://schemas.microsoft.com/winfx/2006/xaml'>
                    <Button x:Name='focusTrap' >Focus trap</Button>
                    <ListView IsFocusEngagementEnabled='True'
                              SelectionMode='None'
                              IsItemClickEnabled='True'
                              x:Name='lv' >
                        <Button Content='Click me to show flyout' x:Name='flyoutShow'>
                            <Button.Flyout>
                            <MenuFlyout x:Name='flyout'>
                                <MenuFlyoutItem x:Name='itemA'>itemA</MenuFlyoutItem>
                                <MenuFlyoutItem x:Name='itemB'>itemB</MenuFlyoutItem>
                            </MenuFlyout>
                            </Button.Flyout>
                        </Button>
                    </ListView>
                </StackPanel>";


            var showFlyout = new Action<object, RoutedEventArgs>((source, args) =>
            {
                Log.Comment("Showing flyout");
                flyout.ShowAt(flyoutShow);
            });


            UIExecutor.Execute(() =>
            {
                rootPanel = (StackPanel)XamlReader.Load(rootPanelXaml);

                lv = (ListView)(rootPanel.FindName("lv"));
                focusTrap = (Button)(rootPanel.FindName("focusTrap"));
                flyoutShow = (Button)(rootPanel.FindName("flyoutShow"));
                flyout = (MenuFlyout)(rootPanel.FindName("flyout"));

                TestServices.WindowHelper.WindowContent = rootPanel;
            });

            TestServices.WindowHelper.WaitForIdle();

            FocusHelper.EnsureFocus(focusTrap, FocusState.Keyboard);

            TestServices.KeyboardHelper.GamepadDpadDown();
            TestServices.WindowHelper.WaitForIdle();


            using (var focusEngaged = new EventTester<ListView, FocusEngagedEventArgs>(lv, "FocusEngaged"))
            {
                TestServices.KeyboardHelper.GamepadA();
                focusEngaged.Wait();

                FocusHelper.EnsureFocus(flyoutShow, FocusState.Keyboard);
                UIExecutor.Execute(() =>
                {
                    Verify.IsTrue(lv.IsFocusEngaged);
                });
            }
            TestServices.WindowHelper.WaitForIdle();

            Log.Comment("Opening Flyout");
            using (var btnClicked = new EventTester<Button, RoutedEventArgs>(flyoutShow, "Click", showFlyout))
            using (var flyoutOpened = new EventTester<MenuFlyout, object>(flyout, "Opened"))
            {
                TestServices.KeyboardHelper.GamepadA();
                flyoutOpened.Wait();
                UIExecutor.Execute(() =>
                {
                    Verify.IsTrue(lv.IsFocusEngaged);
                });
            }

            Log.Comment("Closing Flyout");
            using (var flyoutClosed = new EventTester<MenuFlyout, object>(flyout, "Closed"))
            {
                TestServices.KeyboardHelper.GamepadB();
                flyoutClosed.Wait();
                TestServices.WindowHelper.WaitForIdle();
                UIExecutor.Execute(() =>
                {
                    Verify.IsTrue(lv.IsFocusEngaged);
                });
            }

            Log.Comment("Disengaging");
            using (var focusDisengaged = new EventTester<ListView, FocusDisengagedEventArgs>(lv, "FocusDisengaged"))
            {
                TestServices.KeyboardHelper.GamepadB();
                focusDisengaged.Wait();
                UIExecutor.Execute(() =>
                {
                    Verify.AreEqual(lv, FocusManager.GetFocusedElement(TestServices.WindowHelper.WindowContent.XamlRoot));
                });
            }

        }
    }
}
