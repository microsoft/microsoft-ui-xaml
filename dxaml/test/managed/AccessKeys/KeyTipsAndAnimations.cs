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
using Microsoft.UI.Xaml.Input;
using Windows.Foundation;
using System.Threading;
using System.Diagnostics;
using System.Collections.Generic;
using Microsoft.UI.Xaml.Controls.Primitives;
using Microsoft.UI.Xaml.Data;

namespace Microsoft.UI.Xaml.Tests.Foundation.Graphics.AccessKeys
{
    [TestClass]
    public class KeyTipsAndAnimationsTests : XamlTestsBase
    {
        [ClassInitialize]
        [TestProperty("BinaryUnderTest", "Microsoft.UI.Xaml.dll")]
        [TestProperty("RunAs", "UAP")]
        [TestProperty("UAP:Praid", "XamlManagedTAEFTests")]
        [TestProperty("Classification", "Integration")]
        [TestProperty("Hosting:Mode", "UAP")]  // crash reason :  test crashes at the end
        public static void Setup(TestContext context)
        {
            AssemblySetup.CommonTestClassSetup();
        }

        [ClassCleanup]
        public void ClassCleanup()
        {
            base.CommonClassCleanup();
        }

        [TestInitialize]
        public void TestSetup()
        {
            UIExecutor.Execute(() =>
            {
                // It helps test stability on phone to render a frame before the test starts, otherwise
                // we sometimes get an activation message during the test, which cases us to change the
                // FocusState.
                TestServices.WindowHelper.WindowContent = new Frame();
            });

            TestServices.WindowHelper.WaitForIdle();
        }

        // [TestMethod]
        [TestProperty("VelocityTestPass:OneCoreStrict", "Desktop")]
        public void WaitUntilAnimationIsComplete()
        {
            XamlRoot xamlRoot = null;
            UIExecutor.Execute(() =>
            {
                xamlRoot = TestServices.WindowHelper.WindowContent.XamlRoot;
            });

            using (new KeyTipHelper(xamlRoot))
            using (TestServices.KeyboardHelper.CreateKeyboardWaitKindGuard(KeyboardWaitKind.None))
            {
                Scene scene = SetupKeyTipAnimationScene();

                KeyTipHelper.VerifyKeyTipCount(0);

                Log.Comment("Pressing ALT key to show KeyTips");
                TestServices.KeyboardHelper.Alt();

                Log.Comment("Since animation is running we shouldn't have KeyTips yet");
                TestServices.WindowHelper.SynchronouslyTickUIThread(2); // Wait a bit to make sure no keytips are showing up.
                KeyTipHelper.VerifyKeyTipCount(0);

                Log.Comment("Wait for animation to complete");
                KeyTipHelper.WaitUntilAnyKeyTips();
                TestServices.Utilities.VerifyUIElementTree("AnimationCompleted");
            }
        }

        [TestMethod]
        [TestProperty("VelocityTestPass:OneCoreStrict", "Desktop")]
        public void AnimationTimeout()
        {
            XamlRoot xamlRoot = null;
            UIExecutor.Execute(() =>
            {
                xamlRoot = TestServices.WindowHelper.WindowContent.XamlRoot;
            });

            using (new KeyTipHelper(xamlRoot))
            using (TestServices.KeyboardHelper.CreateKeyboardWaitKindGuard(KeyboardWaitKind.None))
            {
                Scene scene = SetupKeyTipAnimationScene();

                UIExecutor.Execute(() =>
                {
                    scene.DoubleAnimation.Duration = new Duration(TimeSpan.FromDays(1));
                });

                KeyTipHelper.VerifyKeyTipCount(0);

                Log.Comment("Pressing ALT key to show KeyTips");
                TestServices.KeyboardHelper.Alt();

                TestServices.WindowHelper.SynchronouslyTickUIThread(2); // Wait a bit to make sure no keytips are showing up.
                KeyTipHelper.VerifyKeyTipCount(0);

                Log.Comment("KeyTip popups should show up in 1 second.  Waiting for popups to appear...");
                KeyTipHelper.Sleep(1000);
                KeyTipHelper.WaitUntilKeyTips("123 sp");

                Log.Comment("KeyTips should be visible even though animation didn't complete");

                UIExecutor.Execute(() =>
                {
                    scene.Storyboard.Stop();
                });
            }
        }

        [TestMethod]
        [TestProperty("VelocityTestPass:OneCoreStrict", "Desktop")]
        public void InfiniteAnimation()
        {
            XamlRoot xamlRoot = null;
            UIExecutor.Execute(() =>
            {
                xamlRoot = TestServices.WindowHelper.WindowContent.XamlRoot;
            });

            using (new KeyTipHelper(xamlRoot))
            {
                Scene scene = SetupKeyTipAnimationScene();

                UIExecutor.Execute(() =>
                {
                    scene.DoubleAnimation.RepeatBehavior = XamlMedia.Animation.RepeatBehavior.Forever;
                    scene.DoubleAnimation.AutoReverse = true;
                });

                KeyTipHelper.VerifyKeyTipCount(0);

                Log.Comment("Pressing ALT key to show KeyTips");
                TestServices.KeyboardHelper.Alt();

                Log.Comment("Popups should be visible right away because the animation is infinite");
                KeyTipHelper.WaitUntilAnyKeyTips(TimeSpan.FromSeconds(0.3) /* timeout */);

                UIExecutor.Execute(() =>
                {
                    scene.Storyboard.Stop();
                });
            }
        }

        [TestMethod]
        [TestProperty("VelocityTestPass:OneCoreStrict", "Desktop")]
        public void CancelDuringAnimation()
        {
            XamlRoot xamlRoot = null;
            UIExecutor.Execute(() =>
            {
                xamlRoot = TestServices.WindowHelper.WindowContent.XamlRoot;
            });

            using (new KeyTipHelper(xamlRoot))
            using (TestServices.KeyboardHelper.CreateKeyboardWaitKindGuard(KeyboardWaitKind.None))
            {
                Scene scene = SetupKeyTipAnimationScene();

                using (var animationCompletedTester = new EventTester<XamlMedia.Animation.Storyboard, object>(scene.Storyboard, "Completed"))
                {
                    KeyTipHelper.VerifyKeyTipCount(0);

                    using (var eventHelper = EventTester<object,object>.FromStaticEvent<AccessKeyManager>("IsDisplayModeEnabledChanged"))
                    {
                        Log.Comment("Pressing ALT key to show KeyTips");
                        TestServices.KeyboardHelper.Alt();
                        eventHelper.Wait();
                    }

                    TestServices.WindowHelper.SynchronouslyTickUIThread(2);
                    KeyTipHelper.VerifyKeyTipCount(0);

                    using (var eventHelper = EventTester<object,object>.FromStaticEvent<AccessKeyManager>("IsDisplayModeEnabledChanged"))
                    {
                        Log.Comment("Pressing ALT key to exit AccessKey mode");
                        TestServices.KeyboardHelper.Alt();
                        eventHelper.Wait();
                    }

                    TestServices.WindowHelper.SynchronouslyTickUIThread(2);
                    KeyTipHelper.VerifyKeyTipCount(0);

                    Log.Comment("Wait for animation to complete");
                    animationCompletedTester.Wait();
                }
                TestServices.WindowHelper.WaitForIdle();

                KeyTipHelper.VerifyKeyTipCount(0);
            }
        }

        [TestMethod]
        [TestProperty("VelocityTestPass:OneCoreStrict", "Desktop")]
        [TestProperty("HasAssociatedMasterFile", "True")]
        public void KeysPressedDuringAnimation()
        {
            XamlRoot xamlRoot = null;
            UIExecutor.Execute(() =>
            {
                xamlRoot = TestServices.WindowHelper.WindowContent.XamlRoot;
            });

            using (new KeyTipHelper(xamlRoot))
            using (TestServices.KeyboardHelper.CreateKeyboardWaitKindGuard(KeyboardWaitKind.None))
            {
                Scene scene = SetupKeyTipAnimationScene();

                using (var animationCompletedTester = new EventTester<XamlMedia.Animation.Storyboard, object>(scene.Storyboard, "Completed"))
                {
                    KeyTipHelper.VerifyKeyTipCount(0);

                    using (var eventHelper = EventTester<object,object>.FromStaticEvent<AccessKeyManager>("IsDisplayModeEnabledChanged"))
                    {
                        Log.Comment("Pressing ALT key to show KeyTips");
                        TestServices.KeyboardHelper.Alt();
                        eventHelper.Wait();
                    }

                    TestServices.WindowHelper.SynchronouslyTickUIThread(2);
                    KeyTipHelper.VerifyKeyTipCount(0);

                    using (var eventTester = new EventTester<Button, AccessKeyDisplayRequestedEventArgs>(scene.button2, "AccessKeyDisplayRequested"))
                    {
                        TestServices.KeyboardHelper.PressKeySequence("sp");
                        eventTester.Wait();

                        TestServices.WindowHelper.SynchronouslyTickUIThread(2);
                        KeyTipHelper.VerifyKeyTipCount(0);

                        TestServices.KeyboardHelper.PressKeySequence("4");
                        eventTester.Wait();

                        TestServices.WindowHelper.SynchronouslyTickUIThread(2);
                        KeyTipHelper.VerifyKeyTipCount(0);
                    }

                    Log.Comment("Wait for animation to complete");
                    animationCompletedTester.Wait();
                }
                TestServices.WindowHelper.WaitForIdle();

                TestServices.Utilities.VerifyUIElementTree("AnimationCompleted");
            }
        }

        [TestMethod]
        [TestProperty("TestPass:ExcludeOn", "WindowsCore")]
        [TestProperty("Ignore", "TRUE")] // Move windowed popups to lifted input
        [TestProperty("HasAssociatedMasterFile", "True")]
        public void MenuFlyout()
        {
            XamlRoot xamlRoot = null;
            UIExecutor.Execute(() =>
            {
                xamlRoot = TestServices.WindowHelper.WindowContent.XamlRoot;
            });

            using (new KeyTipHelper(xamlRoot))
            using (TestServices.KeyboardHelper.CreateKeyboardWaitKindGuard(KeyboardWaitKind.None))
            {
                Page root;
                Button button1 = null;
                MenuFlyoutItem itemA = null;

                UIExecutor.Execute(() =>
                {
                    root = (Page)XamlMarkup.XamlReader.Load(@"
                    <Page xmlns='http://schemas.microsoft.com/winfx/2006/xaml/presentation' xmlns:x='http://schemas.microsoft.com/winfx/2006/xaml'>
                        <StackPanel Margin='20'>
                            <Button AccessKey='1' KeyTipPlacementMode='Right' Content='button1' IsAccessKeyScope='True' x:Name='button1'>
                                <Button.Flyout>
                                    <MenuFlyout>
                                        <MenuFlyoutItem AccessKey='a' KeyTipPlacementMode='Right' x:Name='itemA'>itemA</MenuFlyoutItem>
                                        <MenuFlyoutItem AccessKey='b' KeyTipPlacementMode='Right' x:Name='itemB'>itemB</MenuFlyoutItem>
                                    </MenuFlyout>
                                </Button.Flyout>
                            </Button>
                            <Button AccessKey='2' Content='button2' KeyTipPlacementMode='Right' />
                        </StackPanel>
                    </Page>");
                    button1 = (Button)root.FindName("button1");
                    itemA = (MenuFlyoutItem)root.FindName("itemA");
                    TestServices.WindowHelper.WindowContent = root;
                });
                TestServices.WindowHelper.WaitForIdle();

                KeyTipHelper.VerifyKeyTipCount(0);

                Log.Comment("Pressing ALT key to show KeyTips");
                TestServices.KeyboardHelper.Alt();

                KeyTipHelper.WaitUntilAnyKeyTips();

                TestServices.KeyboardHelper.PressKeySequence("1");

                Log.Comment("There should be no KeyTip popups until animation is done.");
                KeyTipHelper.WaitUntilKeyTips("");

                Log.Comment("KeyTips should appear in 1 second");
                KeyTipHelper.Sleep(1000);
                KeyTipHelper.WaitUntilKeyTips("a b");

                if (TestServices.Utilities.IsDesktop)
                {
                    TestServices.Utilities.VerifyUIElementTree("Windowed");
                }
                else
                {
                    TestServices.Utilities.VerifyUIElementTree("Unwindowed");
                }

                Log.Comment("Pressing ESC to back out one scope and hide MenuFlyout");
                TestServices.KeyboardHelper.Escape();

                Log.Comment("KeyTip popups for outer scope should show up soon");
                KeyTipHelper.WaitUntilKeyTips("1 2");

                Log.Comment("Pressing ESC to exit AK mode completely");
                TestServices.KeyboardHelper.Escape();

                KeyTipHelper.WaitUntilKeyTips("");
            }
        }

        class Scene
        {
            public XamlMedia.Animation.Storyboard Storyboard;
            public XamlMedia.Animation.DoubleAnimation DoubleAnimation;
            public Button button1;
            public Button button2;
        }

        Scene SetupKeyTipAnimationScene()
        {
            Scene scene = new Scene();

            Page root;
            XamlMedia.TranslateTransform translateTransform = null;
            XamlMedia.Animation.Storyboard storyboard = null;
            XamlMedia.Animation.DoubleAnimation doubleAnimation = null;

            UIExecutor.Execute(() =>
            {
                root = (Page)XamlMarkup.XamlReader.Load(@"
                    <Page xmlns='http://schemas.microsoft.com/winfx/2006/xaml/presentation' xmlns:x='http://schemas.microsoft.com/winfx/2006/xaml'>
                        <StackPanel>
                            <StackPanel.RenderTransform>
                                <TranslateTransform x:Name='translateTransform' />
                            </StackPanel.RenderTransform>
                            <Button AccessKey='123' KeyTipPlacementMode='Right' x:Name='button1'>button 1</Button>
                            <StackPanel AccessKey='sp' IsAccessKeyScope='True' KeyTipPlacementMode='Right'>
                                <Button AccessKey='456' KeyTipPlacementMode='Bottom' x:Name='button2'>button 2</Button>
                            </StackPanel>
                        </StackPanel >
                    </Page>");

                translateTransform = (XamlMedia.TranslateTransform)root.FindName("translateTransform");
                scene.button1 = (Button)root.FindName("button1");
                scene.button2 = (Button)root.FindName("button2");
                TestServices.WindowHelper.WindowContent = root;
            });
            TestServices.WindowHelper.WaitForIdle();

            var storyboardCreatedEvent = new AutoResetEvent(false);

            UIExecutor.Execute(() =>
            {
                // Start the animation
                storyboard = new XamlMedia.Animation.Storyboard();
                doubleAnimation = new XamlMedia.Animation.DoubleAnimation();
                doubleAnimation.From = 50.0;
                doubleAnimation.To = 100.0;
                doubleAnimation.Duration = new Duration(TimeSpan.FromMilliseconds(500));
                XamlMedia.Animation.Storyboard.SetTarget(storyboard, translateTransform);
                XamlMedia.Animation.Storyboard.SetTargetProperty(storyboard, "Y");
                storyboard.Children.Add(doubleAnimation);
                storyboard.Begin();
                storyboardCreatedEvent.Set();
            });

            Log.Comment("Wait for animation to start");
            TestServices.WindowHelper.SynchronouslyTickUIThread(2);

            storyboardCreatedEvent.WaitOne();
            scene.Storyboard = storyboard;
            scene.DoubleAnimation = doubleAnimation;
            return scene;
        }
    }
}

