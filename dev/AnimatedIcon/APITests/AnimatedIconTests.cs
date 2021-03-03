// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

using MUXControlsTestApp.Utilities;

using Windows.UI.Xaml.Controls;
using Microsoft.UI.Xaml.Controls;
using Common;
using Windows.UI.Xaml.Markup;
using System.Collections.Generic;
using Windows.UI.Xaml.Media;
using System.Linq;
using System;
using System.Diagnostics;
using MUXControlsTestApp;
using Microsoft.UI.Private.Controls;
using System.Threading;
using Microsoft.UI.Xaml.Controls.AnimatedVisuals;

#if USING_TAEF
using WEX.TestExecution;
using WEX.TestExecution.Markup;
using WEX.Logging.Interop;
#else
using Microsoft.VisualStudio.TestTools.UnitTesting;
using Microsoft.VisualStudio.TestTools.UnitTesting.Logging;
#endif

namespace Windows.UI.Xaml.Tests.MUXControls.ApiTests
{
    [TestClass]
    public class AnimatedIconTests : ApiTestBase
    {
        [TestMethod]
        public void SettingStateOnParentPropagatesToChildAnimatedIcon()
        {
            AnimatedIcon animatedIcon = null;
            Grid parentGrid = null;
            RunOnUIThread.Execute(() =>
            {
                animatedIcon = new AnimatedIcon();
                parentGrid = new Grid();
                parentGrid.Children.Add(animatedIcon);

                Content = parentGrid;
                Content.UpdateLayout();
            });

            IdleSynchronizer.Wait();

            RunOnUIThread.Execute(() =>
            {
                string stateString = "Test State";
                AnimatedIcon.SetState(parentGrid, stateString);
                Verify.AreEqual(stateString, AnimatedIcon.GetState(animatedIcon));
            });
        }
        [TestMethod]
        public void SettingStateOnGrandParentPropagatesToGrandChildAnimatedIcon()
        {
            AnimatedIcon animatedIcon = null;
            Grid parentGrid = null;
            Grid grandParentGrid = null;
            RunOnUIThread.Execute(() =>
            {
                animatedIcon = new AnimatedIcon();
                parentGrid = new Grid();
                grandParentGrid = new Grid();
                parentGrid.Children.Add(animatedIcon);
                grandParentGrid.Children.Add(parentGrid);

                Content = grandParentGrid;
                Content.UpdateLayout();
            });

            IdleSynchronizer.Wait();

            RunOnUIThread.Execute(() =>
            {
                string stateString = "Test State";
                AnimatedIcon.SetState(grandParentGrid, stateString);
                Verify.AreEqual(stateString, AnimatedIcon.GetState(animatedIcon));
            });
        }

        [TestMethod]
        public void ChangingVisualTrees()
        {
            AnimatedIcon animatedIcon = null;
            Grid parentGrid = null;
            Grid grandParentGrid = null;
            Grid newParentGrid = null;
            RunOnUIThread.Execute(() =>
            {
                animatedIcon = new AnimatedIcon();
                parentGrid = new Grid();
                grandParentGrid = new Grid();
                newParentGrid = new Grid();
                parentGrid.Children.Add(animatedIcon);
                grandParentGrid.Children.Add(parentGrid);

                Content = grandParentGrid;
                Content.UpdateLayout();
            });

            IdleSynchronizer.Wait();

            RunOnUIThread.Execute(() =>
            {
                string stateString = "Test State";
                AnimatedIcon.SetState(parentGrid, stateString);
                Verify.AreEqual(stateString, AnimatedIcon.GetState(animatedIcon));

                parentGrid.Children.Clear();
                newParentGrid.Children.Add(animatedIcon);
                grandParentGrid.Children.Clear();
                grandParentGrid.Children.Add(newParentGrid);

                Content.UpdateLayout();
            });

            IdleSynchronizer.Wait();

            RunOnUIThread.Execute(() =>
            {
                string state2String = "Test State2";
                AnimatedIcon.SetState(newParentGrid, state2String);
                Verify.AreEqual(state2String, AnimatedIcon.GetState(animatedIcon));

                string badStateString = "Bad State";
                AnimatedIcon.SetState(parentGrid, badStateString);
                Verify.AreNotEqual(badStateString, AnimatedIcon.GetState(animatedIcon));
            });
        }

        [TestMethod]
        public void SettingStateOnParentDoesNotPropagateToChildNonAnimatedIcon()
        {
            AnimatedIcon animatedIcon = null;
            Grid parentGrid = null;
            Grid childGrid = null;
            RunOnUIThread.Execute(() =>
            {
                animatedIcon = new AnimatedIcon();
                parentGrid = new Grid();
                childGrid = new Grid();
                parentGrid.Children.Add(childGrid);
                childGrid.Children.Add(animatedIcon);

                Content = parentGrid;
                Content.UpdateLayout();
            });

            IdleSynchronizer.Wait();

            RunOnUIThread.Execute(() =>
            {
                string stateString = "Test State";
                AnimatedIcon.SetState(parentGrid, stateString);
                Verify.AreNotEqual(stateString, AnimatedIcon.GetState(animatedIcon));
                Verify.AreNotEqual(stateString, AnimatedIcon.GetState(childGrid));
            });
        }

        [TestMethod]
        public void CanSetStateOnAnimatedIconDirectlyWithoutPropagationToChild()
        {
            AnimatedIcon animatedIcon = null;
            RunOnUIThread.Execute(() =>
            {
                animatedIcon = new AnimatedIcon();

                Content = animatedIcon;
                Content.UpdateLayout();
            });

            IdleSynchronizer.Wait();

            RunOnUIThread.Execute(() =>
            {
                string stateString = "Test State";
                AnimatedIcon.SetState(animatedIcon, stateString);
                Verify.AreEqual(stateString, AnimatedIcon.GetState(animatedIcon));
                Verify.AreNotEqual(stateString, AnimatedIcon.GetState(VisualTreeHelper.GetChild(animatedIcon, 0)));
            });
        }

        [TestMethod]
        public void ForegroundInheritsFromParent()
        {
            AnimatedIcon animatedIcon = null;
            ContentControl parentContentControl = null;
            RunOnUIThread.Execute(() =>
            {
                animatedIcon = new AnimatedIcon();
                parentContentControl = new ContentControl();
                parentContentControl.Content = animatedIcon;

                Content = parentContentControl;
                Content.UpdateLayout();
            });

            IdleSynchronizer.Wait();

            RunOnUIThread.Execute(() =>
            {
                var foregroundBrush = new SolidColorBrush(Colors.Red);
                parentContentControl.Foreground = foregroundBrush;
                Verify.AreEqual(foregroundBrush, animatedIcon.Foreground);

                var newForegroundBrush = new SolidColorBrush(Colors.Blue);
                animatedIcon.Foreground = newForegroundBrush;
                Verify.AreEqual(newForegroundBrush, animatedIcon.Foreground);
            });
        }

        [TestMethod]
        public void CanChangeSourceAfterState()
        {
            AnimatedIcon animatedIcon = null;
            Grid parentGrid = null;
            RunOnUIThread.Execute(() =>
            {
                animatedIcon = new AnimatedIcon();
                parentGrid = new Grid();
                parentGrid.Children.Add(animatedIcon);

                Content = parentGrid;
                Content.UpdateLayout();
            });

            IdleSynchronizer.Wait();

            RunOnUIThread.Execute(() =>
            {
                animatedIcon.Source = new AnimatedChevronDownSmallVisualSource();
                AnimatedIcon.SetState(parentGrid, "Normal");
                animatedIcon.Source = new AnimatedSettingsVisualSource();
                AnimatedIcon.SetState(parentGrid, "PointerOver");
                animatedIcon.Source = null;
                AnimatedIcon.SetState(parentGrid, "");
            });
        }

        [TestMethod]
        public void TransitionFallbackLogic()
        {
            AnimatedIcon animatedIcon = null;
            var layoutUpdatedEvent = new AutoResetEvent(false);

            RunOnUIThread.Execute(() =>
            {
                animatedIcon = new AnimatedIcon();
                animatedIcon.Source = new MockIAnimatedIconSource2();

                Content = animatedIcon;
                Content.UpdateLayout();
            });

            IdleSynchronizer.Wait();
            
            RunOnUIThread.Execute(() =>
            {
                animatedIcon.LayoutUpdated += AnimatedIcon_LayoutUpdated;
                AnimatedIcon.SetState(animatedIcon, "a");
                Content.UpdateLayout();
            });

            IdleSynchronizer.Wait();
            layoutUpdatedEvent.WaitOne();

            RunOnUIThread.Execute(() =>
            {
                layoutUpdatedEvent.Reset();
                AnimatedIcon.SetState(animatedIcon, "b");
                Content.UpdateLayout();
            });
            
            IdleSynchronizer.Wait();
            layoutUpdatedEvent.WaitOne();

            RunOnUIThread.Execute(() =>
            {
                Verify.AreEqual("aTob_Start", AnimatedIconTestHooks.GetLastAnimationSegmentStart(animatedIcon));
                Verify.AreEqual("aTob_End", AnimatedIconTestHooks.GetLastAnimationSegmentEnd(animatedIcon));

                layoutUpdatedEvent.Reset();
                AnimatedIcon.SetState(animatedIcon, "c");
                Content.UpdateLayout();
            });

            IdleSynchronizer.Wait();
            layoutUpdatedEvent.WaitOne();

            RunOnUIThread.Execute(() =>
            {
                Verify.AreEqual("bToc_Start", AnimatedIconTestHooks.GetLastAnimationSegmentStart(animatedIcon));
                // bToc_End is undefined in MockIAnimatedIconSource2
                Verify.AreEqual("", AnimatedIconTestHooks.GetLastAnimationSegmentEnd(animatedIcon));

                layoutUpdatedEvent.Reset();
                AnimatedIcon.SetState(animatedIcon, "d");
                Content.UpdateLayout();
            });

            IdleSynchronizer.Wait();
            layoutUpdatedEvent.WaitOne();

            RunOnUIThread.Execute(() =>
            {
                // cTod_Start is undefined in MockIAnimatedIconSource2
                Verify.AreEqual("", AnimatedIconTestHooks.GetLastAnimationSegmentStart(animatedIcon));
                Verify.AreEqual("cTod_End", AnimatedIconTestHooks.GetLastAnimationSegmentEnd(animatedIcon));

                layoutUpdatedEvent.Reset();
                AnimatedIcon.SetState(animatedIcon, "e");
                Content.UpdateLayout();
            });

            IdleSynchronizer.Wait();
            layoutUpdatedEvent.WaitOne();

            RunOnUIThread.Execute(() =>
            {
                // dToe_Start and dToe_End are undefined in MockIAnimatedIconSource2, the first backup is dToe
                Verify.AreEqual("", AnimatedIconTestHooks.GetLastAnimationSegmentStart(animatedIcon));
                Verify.AreEqual("dToe", AnimatedIconTestHooks.GetLastAnimationSegmentEnd(animatedIcon));

                layoutUpdatedEvent.Reset();
                AnimatedIcon.SetState(animatedIcon, "f");
                Content.UpdateLayout();
            });

            IdleSynchronizer.Wait();
            layoutUpdatedEvent.WaitOne();

            RunOnUIThread.Execute(() =>
            {
                // eTof_Start, eTof_End, and eTof are undefined in MockIAnimatedIconSource2, the second backup is f
                Verify.AreEqual("", AnimatedIconTestHooks.GetLastAnimationSegmentStart(animatedIcon));
                Verify.AreEqual("f", AnimatedIconTestHooks.GetLastAnimationSegmentEnd(animatedIcon));

                layoutUpdatedEvent.Reset();
                AnimatedIcon.SetState(animatedIcon, "b");
                Content.UpdateLayout();
            });

            IdleSynchronizer.Wait();
            layoutUpdatedEvent.WaitOne();

            RunOnUIThread.Execute(() =>
            {
                // fTob_Start, fTob_End, fTob and b are all undefined in MockIAnimatedIconSource2, the third backup is any
                // marker which ends with the string "Tob_End"
                Verify.AreEqual("", AnimatedIconTestHooks.GetLastAnimationSegmentStart(animatedIcon));
                Verify.AreEqual("aTob_End", AnimatedIconTestHooks.GetLastAnimationSegmentEnd(animatedIcon));

                layoutUpdatedEvent.Reset();
                AnimatedIcon.SetState(animatedIcon, "0.12345");
                Content.UpdateLayout();
            });

            IdleSynchronizer.Wait();
            layoutUpdatedEvent.WaitOne();

            RunOnUIThread.Execute(() =>
            {
                // bTo0.12345_Start, bTo0.12345_End, bTo0.12345, and 0.12345  are all undefined in MockIAnimatedIconSource2, and
                // there are no markers which end with the string "To0.12345_End" so finally we attempt to interpret the state as
                // a float to get the position to animate to.
                Verify.AreEqual("", AnimatedIconTestHooks.GetLastAnimationSegmentStart(animatedIcon));
                Verify.AreEqual("0.12345", AnimatedIconTestHooks.GetLastAnimationSegmentEnd(animatedIcon));

                layoutUpdatedEvent.Reset();
                AnimatedIcon.SetState(animatedIcon, "Failure");
                Content.UpdateLayout();
            });

            IdleSynchronizer.Wait();
            layoutUpdatedEvent.WaitOne();

            RunOnUIThread.Execute(() =>
            {
                // 0.12345ToFailure_Start, 0.12345ToFailure_End, 0.12345ToFailure, and Failure are all undefined in MockIAnimatedIconSource2, and
                // there are no markers which end with the string "ToFailure_End" and Failure is not a float, so we have failed to find a marker.
                Verify.AreEqual("", AnimatedIconTestHooks.GetLastAnimationSegmentStart(animatedIcon));
                Verify.AreEqual("0.0", AnimatedIconTestHooks.GetLastAnimationSegmentEnd(animatedIcon));
            });

            void AnimatedIcon_LayoutUpdated(object sender, object e)
            {
                layoutUpdatedEvent.Set();
            }
        }
    }
}
