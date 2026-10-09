// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

using MUXControlsTestApp.Utilities;

using Microsoft.UI.Xaml.Controls;
using Common;
using Microsoft.UI.Xaml.Markup;
using System.Collections.Generic;
using Microsoft.UI.Xaml.Media;
using System.Linq;
using System;
using System.Diagnostics;
using System.Globalization;
using System.Numerics;
using MUXControlsTestApp;
using Microsoft.UI.Composition;
using Microsoft.UI.Private.Controls;
using Microsoft.UI.Xaml.Hosting;
using System.Threading;
using Microsoft.UI.Xaml.Controls.AnimatedVisuals;
using Microsoft.UI.Xaml.XamlTypeInfo;
using Windows.UI.ViewManagement;
using Color = Windows.UI.Color;
using Point = Windows.Foundation.Point;
using Size = Windows.Foundation.Size;

using WEX.TestExecution;
using WEX.TestExecution.Markup;
using WEX.Logging.Interop;

namespace Microsoft.UI.Xaml.Tests.MUXControls.ApiTests
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
                AnimatedIcon.SetState(parentGrid, "Initial State");

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
                AnimatedIcon.SetState(grandParentGrid, "Initial State");

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
                AnimatedIcon.SetState(parentGrid, "Initial State");

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
                AnimatedIcon.SetState(newParentGrid, "Initial State");

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
                AnimatedIcon.SetState(parentGrid, "Initial State");

                Content = parentGrid;
                Content.UpdateLayout();
            });

            IdleSynchronizer.Wait();

            RunOnUIThread.Execute(() =>
            {
                string stateString = "Test State";
                AnimatedIcon.SetState(parentGrid, stateString);
                Verify.AreNotEqual(stateString, AnimatedIcon.GetState(childGrid));
                Verify.AreEqual(stateString, AnimatedIcon.GetState(animatedIcon));
            });
        }

        [TestMethod]
        public void AddingAnimatedIconToGridWithoutAStateDoesNotPropogateState()
        {
            // This is not actually a desired behavior.  Ideally we would be able to set
            // the AnimatedIcon.State property on any ancestor of an animated icon at any
            // time and that would reach the icon. However this is challenging to do
            // efficiently so instead we require that the parent have an AnimatedIcon.State
            // value when the icon is loaded.
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
                Verify.AreNotEqual(stateString, AnimatedIcon.GetState(animatedIcon));
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

        // Scenario: Loads an icon under a Grid with AnimatedIcon.State "Initial State", then in one UI-thread pass sets
        // Source to ChevronDownSmall, Settings and finally null, changing the Grid's State to "Normal", "PointerOver"
        // and "" in between.
        // Expected: each Source change immediately replaces the hosted composition visual (two different non-null
        // visuals, then none); the icon ends with State "" and no recorded transition segment.
        // Failure means: setting Source does not synchronously swap or remove the animated visual, parent State changes
        // do not reach the icon, or a transition is recorded although no source is set.
        [TestMethod]
        public void CanChangeSourceAfterState()
        {
            var animatedIcon = CreateIconInParentGridWithState("Initial State", out Grid parentGrid);

            // Swap sources while the parent's state changes are still pending, all within one UI-thread pass.
            ChangeSourcesAndParentStatesInOnePass(animatedIcon, parentGrid,
                out Visual chevronVisual, out Visual settingsVisual, out Visual finalVisual);
            IdleSynchronizer.Wait();

            // Every Source change synchronously replaces the composition child visual hosted by the icon.
            Verify.IsNotNull(chevronVisual);
            Verify.IsNotNull(settingsVisual);
            Verify.IsFalse(ReferenceEquals(chevronVisual, settingsVisual));
            Verify.IsNull(finalVisual);

            // The parent's states reached the icon, but with no source left there was no transition to record.
            Verify.AreEqual("", GetStateOf(animatedIcon));
            Verify.AreEqual(" |  | ", DescribeLastSegment(animatedIcon));
        }

        // Scenario: Loads an icon with the mock source MockIAnimatedIconSource2, whose markers are deliberately
        // incomplete, and steps State through a, b, c, d, e, f, b, "0.12345" and "Failure".
        // Expected: each transition resolves its markers in the documented fallback order: "XToY_Start"/"XToY_End",
        // then "XToY", then "Y", then any marker ending in "ToY_End", then Y parsed as a number, else 0.0 (e.g. "fTob |
        // | aTob_End", "0.12345ToFailure |  | 0.0").
        // Failure means: AnimatedIcon's marker lookup and fallback order for state transitions changed or is broken.
        [TestMethod]
        public void TransitionFallbackLogic()
        {
            var animatedIcon = CreateLoadedIcon(new MockIAnimatedIconSource2());

            using (var recorder = new TransitionRecorder(animatedIcon))
            {
                SetStateAndWaitForTransitions(recorder, animatedIcon, "a", 1);

                SetStateAndWaitForTransitions(recorder, animatedIcon, "b", 2);
                Verify.AreEqual("aTob | aTob_Start | aTob_End", DescribeLastSegment(animatedIcon));

                // bToc_End is undefined in MockIAnimatedIconSource2
                SetStateAndWaitForTransitions(recorder, animatedIcon, "c", 3);
                Verify.AreEqual("bToc | bToc_Start | ", DescribeLastSegment(animatedIcon));

                // cTod_Start is undefined in MockIAnimatedIconSource2
                SetStateAndWaitForTransitions(recorder, animatedIcon, "d", 4);
                Verify.AreEqual("cTod |  | cTod_End", DescribeLastSegment(animatedIcon));

                // dToe_Start and dToe_End are undefined in MockIAnimatedIconSource2, the first backup is dToe
                SetStateAndWaitForTransitions(recorder, animatedIcon, "e", 5);
                Verify.AreEqual("dToe |  | dToe", DescribeLastSegment(animatedIcon));

                // eTof_Start, eTof_End, and eTof are undefined in MockIAnimatedIconSource2, the second backup is f
                SetStateAndWaitForTransitions(recorder, animatedIcon, "f", 6);
                Verify.AreEqual("eTof |  | f", DescribeLastSegment(animatedIcon));

                // fTob_Start, fTob_End, fTob and b are all undefined in MockIAnimatedIconSource2, the third backup is any
                // marker which ends with the string "Tob_End"
                SetStateAndWaitForTransitions(recorder, animatedIcon, "b", 7);
                Verify.AreEqual("fTob |  | aTob_End", DescribeLastSegment(animatedIcon));

                // bTo0.12345_Start, bTo0.12345_End, bTo0.12345, and 0.12345  are all undefined in MockIAnimatedIconSource2, and
                // there are no markers which end with the string "To0.12345_End" so finally we attempt to interpret the state as
                // a float to get the position to animate to.
                SetStateAndWaitForTransitions(recorder, animatedIcon, "0.12345", 8);
                Verify.AreEqual("bTo0.12345 |  | 0.12345", DescribeLastSegment(animatedIcon));

                // 0.12345ToFailure_Start, 0.12345ToFailure_End, 0.12345ToFailure, and Failure are all undefined in MockIAnimatedIconSource2, and
                // there are no markers which end with the string "ToFailure_End" and Failure is not a float, so we have failed to find a marker.
                SetStateAndWaitForTransitions(recorder, animatedIcon, "Failure", 9);
                Verify.AreEqual("0.12345ToFailure |  | 0.0", DescribeLastSegment(animatedIcon));
            }
        }

        // Scenario: Places an icon with no Source in a StackPanel, then sets Source to
        // AnimatedChevronDownSmallVisualSource.
        // Expected: the height is 0 without a source; afterwards the icon has a real height (over 10), is as wide as
        // the StackPanel and stays square because the 48x48 visual is scaled uniformly.
        // Failure means: setting Source does not invalidate layout, or measure does not scale the visual uniformly to
        // the available width.
        [TestMethod]
        public void ChangingSourcePropertyChangesRenderSize()
        {
            var icon = CreateIconInStackPanel(out StackPanel stackPanel);

            // Icon height will be zero if the source property is not set.
            Verify.IsTrue(Math.Abs(GetActualHeight(icon)) < 0.1);

            SetSource(icon, new AnimatedChevronDownSmallVisualSource());
            IdleSynchronizer.Wait();

            // Icon will have a height if the AnimatedIcon also updated the visual tree to rerender.
            double height = GetActualHeight(icon);
            Verify.IsTrue(Math.Abs(height) > 10);

            // The StackPanel offers unbounded height, so the square 48x48 visual is scaled uniformly to the full width.
            Verify.IsLessThan(Math.Abs(GetActualWidth(stackPanel) - GetActualWidth(icon)), 1.0);
            Verify.IsLessThan(Math.Abs(GetActualWidth(icon) - height), 1.0);
        }

        // Scenario: Measures an icon showing the 48x48 AnimatedBackVisualSource against several constraints, including
        // infinite width and/or height.
        // Expected: the desired size keeps the 1:1 aspect ratio and fits the tighter side: 60x60 for 100x60, 32x32 for
        // 32x96, 24x24 for infinity x 24, 36x36 for 36 x infinity, and the natural 48x48 when both sides are infinite.
        // Failure means: MeasureOverride no longer scales the visual uniformly or mishandles unbounded constraints.
        [TestMethod]
        public void MeasureScalesVisualUniformlyToAvailableSize()
        {
            var icon = CreateIconInCanvas(new AnimatedBackVisualSource());

            // The generated visual is 48x48; the desired size keeps that aspect ratio and fits the tighter dimension.
            // Constraints are multiples of 4 DIPs so layout rounding is exact at 100/125/150/175/200% display scale.
            Verify.AreEqual(new Size(60, 60), MeasureIcon(icon, new Size(100, 60)));
            Verify.AreEqual(new Size(32, 32), MeasureIcon(icon, new Size(32, 96)));
            Verify.AreEqual(new Size(24, 24), MeasureIcon(icon, new Size(double.PositiveInfinity, 24)));
            Verify.AreEqual(new Size(36, 36), MeasureIcon(icon, new Size(36, double.PositiveInfinity)));
            Verify.AreEqual(new Size(48, 48), MeasureIcon(icon, new Size(double.PositiveInfinity, double.PositiveInfinity)));
        }

        // Scenario: Measures an icon that is not in the live tree and whose source creates a visual with a natural size
        // of 0x0, once with 100x100 and once with 0x0 available.
        // Expected: the desired size is 0x0 in both cases.
        // Failure means: MeasureOverride mishandles a zero-size visual (for example dividing by zero and reporting a
        // NaN, infinite or non-zero size).
        [TestMethod]
        public void MeasureReturnsZeroForZeroSizeVisual()
        {
            // Measured outside the live tree so that only MeasureOverride runs for the zero-size visual.
            var icon = CreateDetachedIcon(new ZeroSizeAnimatedVisualSource());

            Verify.AreEqual(new Size(0, 0), MeasureIcon(icon, new Size(100, 100)));
            Verify.AreEqual(new Size(0, 0), MeasureIcon(icon, new Size(0, 0)));
        }

        // Scenario: Arranges three icons showing the 48x48 back-arrow visual in a Canvas at 96x48 (wide), 96x96 (large)
        // and 48x96 (tall).
        // Expected: the root visual stays 48x48, is scaled by the smaller ratio and centered: wide -> offset (24,0)
        // scale 1; large -> offset (0,0) scale 2; tall -> offset (0,24) scale 1.
        // Failure means: ArrangeOverride computes the wrong scale or centering offset for the animated visual.
        [TestMethod]
        public void ArrangeCentersAndScalesAnimatedVisual()
        {
            var canvas = CreateLoadedCanvas();
            var wideIcon = AddBackIconToCanvas(canvas, 96, 48);
            var largeIcon = AddBackIconToCanvas(canvas, 96, 96);
            var tallIcon = AddBackIconToCanvas(canvas, 48, 96);
            IdleSynchronizer.Wait();

            // Uniform scale by the smaller ratio, centered in the arranged rect.
            Verify.AreEqual(new Vector3(24, 0, 0), GetRootVisualOffset(wideIcon));
            Verify.AreEqual(new Vector3(1, 1, 1), GetRootVisualScale(wideIcon));
            Verify.AreEqual(new Vector2(48, 48), GetRootVisualSize(wideIcon));

            Verify.AreEqual(new Vector3(0, 0, 0), GetRootVisualOffset(largeIcon));
            Verify.AreEqual(new Vector3(2, 2, 1), GetRootVisualScale(largeIcon));
            Verify.AreEqual(new Vector2(48, 48), GetRootVisualSize(largeIcon));

            Verify.AreEqual(new Vector3(0, 24, 0), GetRootVisualOffset(tallIcon));
            Verify.AreEqual(new Vector3(1, 1, 1), GetRootVisualScale(tallIcon));
            Verify.AreEqual(new Vector2(48, 48), GetRootVisualSize(tallIcon));
        }

        // Scenario: Loads an icon with no Source and a SymbolIconSource(Accept) FallbackIconSource.
        // Expected: no animated visual is hosted; the icon's root panel has 2 children: the collapsed internal path and
        // a SymbolIcon showing Accept.
        // Failure means: the FallbackIconSource is not displayed when there is no source, or the internal path is
        // visible.
        [TestMethod]
        public void FallbackIconIsShownWhenSourceIsNull()
        {
            var icon = CreateLoadedIcon(null, fallbackSymbol: Symbol.Accept);

            Verify.IsNull(GetAnimatedVisualRoot(icon));
            // Child 0 is the collapsed PathIcon path; the fallback icon is appended after it.
            Verify.AreEqual(2, GetRootPanelChildCount(icon));
            Verify.AreEqual(Visibility.Collapsed, GetPathVisibility(icon));
            Verify.AreEqual(Symbol.Accept, GetFallbackSymbol(icon));
        }

        // Scenario: Loads an icon whose source returns null from TryCreateAnimatedVisual, with a SymbolIconSource(Back)
        // fallback.
        // Expected: no animated visual is hosted and the Back SymbolIcon is added as the second child of the root
        // panel.
        // Failure means: AnimatedIcon does not fall back to FallbackIconSource when the source fails to create a
        // visual.
        [TestMethod]
        public void FallbackIconIsShownWhenSourceCreatesNoVisual()
        {
            var icon = CreateLoadedIcon(new NullVisualAnimatedVisualSource(), fallbackSymbol: Symbol.Back);

            Verify.IsNull(GetAnimatedVisualRoot(icon));
            Verify.AreEqual(2, GetRootPanelChildCount(icon));
            Verify.AreEqual(Symbol.Back, GetFallbackSymbol(icon));
        }

        // Scenario: Loads an icon with no Source showing an Accept SymbolIcon fallback, then sets FallbackIconSource to
        // a FontIconSource with glyph U+E700.
        // Expected: the root panel still has exactly 2 children and the fallback is now a FontIcon with glyph U+E700.
        // Failure means: changing FallbackIconSource does not replace the old fallback element (it is kept, duplicated
        // or not updated).
        [TestMethod]
        public void ChangingFallbackIconSourceReplacesFallbackIcon()
        {
            var icon = CreateLoadedIcon(null, fallbackSymbol: Symbol.Accept);
            Verify.AreEqual(Symbol.Accept, GetFallbackSymbol(icon));

            SetFallbackGlyph(icon, "\uE700");

            // The previous fallback icon is replaced, not accumulated.
            Verify.AreEqual(2, GetRootPanelChildCount(icon));
            Verify.AreEqual("\uE700", GetFallbackGlyph(icon));
        }

        // Scenario: Loads an icon with no Source and an Accept fallback, sets Source to AnimatedBackVisualSource, then
        // sets Source back to null.
        // Expected: with a source the visual is hosted and the fallback is removed (1 child); back at null the visual
        // is gone and the Accept fallback is shown again (2 children).
        // Failure means: switching Source between a real visual and null does not swap correctly between the animated
        // visual and the fallback icon.
        [TestMethod]
        public void SettingSourceReplacesFallbackIconWithVisual()
        {
            var icon = CreateLoadedIcon(null, fallbackSymbol: Symbol.Accept);
            Verify.AreEqual(Symbol.Accept, GetFallbackSymbol(icon));

            SetSource(icon, new AnimatedBackVisualSource());
            Verify.IsNotNull(GetAnimatedVisualRoot(icon));
            Verify.AreEqual(1, GetRootPanelChildCount(icon), "The fallback icon must be removed once a visual is available.");

            SetSource(icon, null);
            Verify.IsNull(GetAnimatedVisualRoot(icon));
            Verify.AreEqual(2, GetRootPanelChildCount(icon));
            Verify.AreEqual(Symbol.Accept, GetFallbackSymbol(icon));
        }

        // Scenario: Loads an icon that has both a working source and an Accept fallback, then changes the fallback to
        // Back.
        // Expected: the animated visual is hosted and the root panel keeps only 1 child before and after the fallback
        // change.
        // Failure means: a fallback icon is added even though the animated visual is displayed.
        [TestMethod]
        public void FallbackIconIsNotShownWhenVisualIsDisplayed()
        {
            var icon = CreateLoadedIcon(new AnimatedBackVisualSource(), fallbackSymbol: Symbol.Accept);
            Verify.IsNotNull(GetAnimatedVisualRoot(icon));
            Verify.AreEqual(1, GetRootPanelChildCount(icon));

            SetFallbackSymbol(icon, Symbol.Back);
            Verify.AreEqual(1, GetRootPanelChildCount(icon));
        }

        // Scenario: Loads an icon, then toggles FlowDirection between LeftToRight and RightToLeft and
        // MirroredWhenRightToLeft between true and false.
        // Expected: RenderTransformOrigin is (0.5,0.5); the render transform's ScaleX is -1 (undoing the RTL mirror)
        // only for RightToLeft with MirroredWhenRightToLeft false, and 1 otherwise.
        // Failure means: the icon is wrongly mirrored or not mirrored in right-to-left layouts, or changes to
        // MirroredWhenRightToLeft or FlowDirection are ignored.
        [TestMethod]
        public void RightToLeftFlowDirectionIsCounteredUnlessMirrored()
        {
            var icon = CreateLoadedIcon(new AnimatedBackVisualSource());
            Verify.AreEqual(new Point(0.5, 0.5), GetRenderTransformOrigin(icon));
            Verify.AreEqual(1.0, GetMirrorScaleX(icon));

            // XAML mirrors RightToLeft content; the icon undoes that unless MirroredWhenRightToLeft is set.
            SetFlowDirection(icon, FlowDirection.RightToLeft);
            Verify.AreEqual(-1.0, GetMirrorScaleX(icon));

            SetMirroredWhenRightToLeft(icon, true);
            Verify.AreEqual(1.0, GetMirrorScaleX(icon));

            SetMirroredWhenRightToLeft(icon, false);
            Verify.AreEqual(-1.0, GetMirrorScaleX(icon));

            SetFlowDirection(icon, FlowDirection.LeftToRight);
            Verify.AreEqual(1.0, GetMirrorScaleX(icon));
        }

        // Scenario: Loads an icon with no Source showing a Back fallback and sets FlowDirection to RightToLeft.
        // Expected: the icon's render transform ScaleX stays 1.0, so no counter-mirroring is applied.
        // Failure means: the right-to-left counter-mirror meant for the animated visual is also applied when only the
        // fallback icon is shown.
        [TestMethod]
        public void FallbackIconIsNotCounterMirrored()
        {
            var icon = CreateLoadedIcon(null, fallbackSymbol: Symbol.Back);

            SetFlowDirection(icon, FlowDirection.RightToLeft);
            Verify.AreEqual(1.0, GetMirrorScaleX(icon));
        }

        // Scenario: Creates an icon with a red Foreground and a recording source, then sets Source, loads it, changes
        // the brush to blue, replaces it with a green brush, changes the old brush, and switches to a gradient brush.
        // Expected: the source receives SetColorProperty("Foreground", ...) for red, blue and green, one call per
        // change; changes to a replaced brush or after switching to a gradient brush cause no further calls.
        // Failure means: Foreground colors are not forwarded to the animated visual source, or the icon keeps listening
        // to brushes it no longer uses.
        [TestMethod]
        public void ForegroundColorIsForwardedToSource()
        {
            var source = new ControlledAnimatedVisualSource(new Dictionary<string, double>());
            var icon = CreateDetachedIconWithForeground(Colors.Red, out SolidColorBrush firstBrush);

            // Creating the visual pushes the current foreground color into the source.
            SetSource(icon, source);
            Verify.AreEqual("Foreground=" + Colors.Red, DescribeLastColorProperty(source));

            LoadIcon(icon);
            int calls = source.ColorPropertyCalls.Count;

            SetBrushColor(firstBrush, Colors.Blue);
            Verify.AreEqual(calls + 1, source.ColorPropertyCalls.Count);
            Verify.AreEqual("Foreground=" + Colors.Blue, DescribeLastColorProperty(source));

            var secondBrush = SetSolidForeground(icon, Colors.Green);
            Verify.AreEqual(calls + 2, source.ColorPropertyCalls.Count);
            Verify.AreEqual("Foreground=" + Colors.Green, DescribeLastColorProperty(source));

            // The replaced brush is no longer observed.
            SetBrushColor(firstBrush, Colors.Yellow);
            Verify.AreEqual(calls + 2, source.ColorPropertyCalls.Count);

            // A non-solid brush has no single color to forward, and stops observation of the previous brush.
            SetGradientForeground(icon);
            SetBrushColor(secondBrush, Colors.Purple);
            Verify.AreEqual(calls + 2, source.ColorPropertyCalls.Count);
            Verify.AreEqual("Foreground=" + Colors.Green, DescribeLastColorProperty(source));
        }

        // Scenario: Uses a source with chosen markers (aTob_Start 0.1, aTob_End 0.2, bToc_Start 0.3, cTod_End 0.4, dToe
        // 0.5, f 0.6) and zero duration, and steps State through a, b, c, d, e, f, b, "0.75" and "Failure".
        // Expected: each step records the expected segment and Progress lands on the resolved value: 0, 0.2, 0.3, 0.4,
        // 0.5, 0.6, 0.2, 0.75, then 0 for the unresolvable state.
        // Failure means: the marker fallback picks the wrong marker, or the visual's Progress is not moved to the
        // resolved marker position.
        [TestMethod]
        public void TransitionMovesProgressToResolvedMarker()
        {
            var markers = new Dictionary<string, double>
            {
                ["aTob_Start"] = 0.1,
                ["aTob_End"] = 0.2,
                ["bToc_Start"] = 0.3,
                ["cTod_End"] = 0.4,
                ["dToe"] = 0.5,
                ["f"] = 0.6,
            };
            // Zero duration makes every segment jump straight to its end so the final Progress is deterministic.
            var icon = CreateLoadedIcon(new ControlledAnimatedVisualSource(markers), durationMultiplier: 0);

            using (var recorder = new TransitionRecorder(icon))
            {
                SetStateAndWaitForTransitions(recorder, icon, "a", 1);
                Verify.AreEqual("Toa |  | 0.0", DescribeLastSegment(icon));
                Verify.IsLessThan(Math.Abs(0.0f - ReadProgress(icon)), c_progressTolerance);

                SetStateAndWaitForTransitions(recorder, icon, "b", 2);
                Verify.AreEqual("aTob | aTob_Start | aTob_End", DescribeLastSegment(icon));
                Verify.IsLessThan(Math.Abs(0.2f - ReadProgress(icon)), c_progressTolerance);

                SetStateAndWaitForTransitions(recorder, icon, "c", 3);
                Verify.AreEqual("bToc | bToc_Start | ", DescribeLastSegment(icon));
                Verify.IsLessThan(Math.Abs(0.3f - ReadProgress(icon)), c_progressTolerance);

                SetStateAndWaitForTransitions(recorder, icon, "d", 4);
                Verify.AreEqual("cTod |  | cTod_End", DescribeLastSegment(icon));
                Verify.IsLessThan(Math.Abs(0.4f - ReadProgress(icon)), c_progressTolerance);

                SetStateAndWaitForTransitions(recorder, icon, "e", 5);
                Verify.AreEqual("dToe |  | dToe", DescribeLastSegment(icon));
                Verify.IsLessThan(Math.Abs(0.5f - ReadProgress(icon)), c_progressTolerance);

                SetStateAndWaitForTransitions(recorder, icon, "f", 6);
                Verify.AreEqual("eTof |  | f", DescribeLastSegment(icon));
                Verify.IsLessThan(Math.Abs(0.6f - ReadProgress(icon)), c_progressTolerance);

                SetStateAndWaitForTransitions(recorder, icon, "b", 7);
                Verify.AreEqual("fTob |  | aTob_End", DescribeLastSegment(icon));
                Verify.IsLessThan(Math.Abs(0.2f - ReadProgress(icon)), c_progressTolerance);

                SetStateAndWaitForTransitions(recorder, icon, "0.75", 8);
                Verify.AreEqual("bTo0.75 |  | 0.75", DescribeLastSegment(icon));
                Verify.IsLessThan(Math.Abs(0.75f - ReadProgress(icon)), c_progressTolerance);

                SetStateAndWaitForTransitions(recorder, icon, "Failure", 9);
                Verify.AreEqual("0.75ToFailure |  | 0.0", DescribeLastSegment(icon));
                Verify.IsLessThan(Math.Abs(0.0f - ReadProgress(icon)), c_progressTolerance);
            }
        }

        // Scenario: Loads an icon whose source has no markers and sets State "a", removes the source and sets "b", then
        // sets a source with marker c=0.5 and sets State "c".
        // Expected: no transition is recorded for "a" or "b"; the transition for "c" is bToc ending at marker "c" with
        // Progress 0.5.
        // Failure means: transitions are recorded without markers or without a source, or the previous state is not
        // tracked while no transition can play.
        [TestMethod]
        public void StateChangeWithoutMarkersRecordsNoTransition()
        {
            var icon = CreateLoadedIcon(new ControlledAnimatedVisualSource(null), durationMultiplier: 0);

            using (var recorder = new TransitionRecorder(icon))
            {
                SetStateAndWaitForLayout(icon, "a");
                Verify.AreEqual(0, recorder.Segments.Length);
                Verify.AreEqual(" |  | ", DescribeLastSegment(icon));

                SetSource(icon, null);
                SetStateAndWaitForLayout(icon, "b");
                Verify.AreEqual(0, recorder.Segments.Length);
                Verify.AreEqual(" |  | ", DescribeLastSegment(icon));

                // The unresolved states were still tracked, so the next transition starts from "b".
                SetSource(icon, new ControlledAnimatedVisualSource(new Dictionary<string, double> { ["c"] = 0.5 }));
                SetStateAndWaitForTransitions(recorder, icon, "c", 1);
                Verify.AreEqual("bToc |  | c", DescribeLastSegment(icon));
                Verify.IsLessThan(Math.Abs(0.5f - ReadProgress(icon)), c_progressTolerance);
            }
        }

        // Scenario: Loads an icon in State "Normal" (zero duration) and sets State "Pressed" then "PointerOver" before
        // the next layout pass.
        // Expected: only one new transition, NormalToPointerOver with its _Start and _End markers, is recorded; the
        // intermediate "Pressed" is skipped.
        // Failure means: AnimatedIcon plays intermediate states set within one layout pass instead of going straight to
        // the last one.
        [TestMethod]
        public void StateChangesWithinOneLayoutPassTransitionOnlyToLastState()
        {
            var icon = CreateLoadedIcon(new AnimatedBackVisualSource(), durationMultiplier: 0);

            using (var recorder = new TransitionRecorder(icon))
            {
                SetStateAndWaitForTransitions(recorder, icon, "Normal", 1);

                SetStatesInOneLayoutPass(icon, "Pressed", "PointerOver");
                IdleSynchronizer.Wait();

                Verify.AreEqual("ToNormal, NormalToPointerOver", string.Join(", ", recorder.Segments));
                Verify.AreEqual("NormalToPointerOver | NormalToPointerOver_Start | NormalToPointerOver_End", DescribeLastSegment(icon));
            }
        }

        // Scenario: Loads an icon with its own State "Own" inside a Grid whose State is "Parent", then changes the
        // Grid's State to "Changed".
        // Expected: the icon keeps "Own" when it is loaded, and then follows the ancestor's change to "Changed".
        // Failure means: loading overwrites an icon's own State with its ancestor's, or later ancestor State changes no
        // longer reach the icon.
        [TestMethod]
        public void LocalStateIsKeptWhenLoadedUnderAncestorWithState()
        {
            var icon = CreateIconWithStateUnderParentWithState("Own", "Parent", out Grid parentGrid);
            Verify.AreEqual("Own", GetStateOf(icon));

            // The icon still follows later changes of its ancestor's state.
            SetStateOf(parentGrid, "Changed");
            Verify.AreEqual("Changed", GetStateOf(icon));
        }

        // Scenario: With the QueueOne queue behavior, starts the slowed-down (about 12 s) NormalToPointerOver
        // transition and sets State "Pressed" while it plays.
        // Expected: NormalToPointerOver keeps playing (still 2 recorded segments); once it finishes,
        // PointerOverToPressed plays as the 3rd segment.
        // Failure means: QueueOne interrupts the playing transition or never plays the queued state.
        // Note: logs SKIPPED and returns when Windows animation effects are disabled.
        [TestMethod]
        public void QueueOneDefersStateChangeUntilTransitionCompletes()
        {
            if (!AreAnimationsEnabledOrLogSkip())
            {
                return;
            }

            var icon = CreateIconPlayingNormalToPointerOver(AnimatedIconAnimationQueueBehavior.QueueOne, out var recorder);
            using (recorder)
            {
                SetStateAndWaitForLayout(icon, "Pressed");
                Verify.AreEqual("NormalToPointerOver", GetLastSegment(icon));
                Verify.AreEqual(2, recorder.Segments.Length);

                recorder.WaitForCount(3, c_longTransitionTimeoutMs);
                Verify.AreEqual("PointerOverToPressed", GetLastSegment(icon));
            }
        }

        // Scenario: With QueueOne and the default queue length of 4, sets four states while the slowed-down
        // NormalToPointerOver transition plays, then sets a fifth.
        // Expected: the four states are queued with no new segment; the fifth immediately plays the oldest queued
        // transition, PointerOverToPressed.
        // Failure means: the queue length limit is not honored (states are dropped, played too early, or never advance
        // when the queue is full).
        // Note: logs SKIPPED and returns when Windows animation effects are disabled.
        [TestMethod]
        public void QueueOneAdvancesImmediatelyWhenQueueIsFull()
        {
            if (!AreAnimationsEnabledOrLogSkip())
            {
                return;
            }

            var icon = CreateIconPlayingNormalToPointerOver(AnimatedIconAnimationQueueBehavior.QueueOne, out var recorder);
            using (recorder)
            {
                // The default queue holds four states while a transition plays.
                foreach (var state in new[] { "Pressed", "Normal", "PointerOver", "Pressed" })
                {
                    SetStateAndWaitForLayout(icon, state);
                    Verify.AreEqual("NormalToPointerOver", GetLastSegment(icon));
                    Verify.AreEqual(2, recorder.Segments.Length);
                }

                // A fifth state immediately plays the oldest queued transition.
                SetStateAndWaitForLayout(icon, "Normal");
                Verify.AreEqual("PointerOverToPressed", GetLastSegment(icon));
                Verify.AreEqual(3, recorder.Segments.Length);
            }
        }

        // Scenario: With the Cut queue behavior, sets State "Pressed" while the slowed-down NormalToPointerOver
        // transition is playing.
        // Expected: PointerOverToPressed starts immediately as the 3rd recorded segment.
        // Failure means: Cut no longer interrupts the playing transition.
        // Note: logs SKIPPED and returns when Windows animation effects are disabled.
        [TestMethod]
        public void CutInterruptsPlayingTransition()
        {
            if (!AreAnimationsEnabledOrLogSkip())
            {
                return;
            }

            var icon = CreateIconPlayingNormalToPointerOver(AnimatedIconAnimationQueueBehavior.Cut, out var recorder);
            using (recorder)
            {
                SetStateAndWaitForLayout(icon, "Pressed");
                Verify.AreEqual("PointerOverToPressed", GetLastSegment(icon));
                Verify.AreEqual(3, recorder.Segments.Length);
            }
        }

        // Scenario: With SpeedUpQueueOne and a speed-up multiplier of 1000000, sets State "Pressed" while the
        // slowed-down NormalToPointerOver transition plays.
        // Expected: the rest of NormalToPointerOver finishes within the same layout pass and PointerOverToPressed
        // plays; segments are "ToNormal, NormalToPointerOver, PointerOverToPressed".
        // Failure means: SpeedUpQueueOne does not speed up the playing transition and then play the queued state.
        // Note: logs SKIPPED and returns when Windows animation effects are disabled.
        [TestMethod]
        public void SpeedUpQueueOneFinishesPlayingTransitionBeforeQueuedState()
        {
            if (!AreAnimationsEnabledOrLogSkip())
            {
                return;
            }

            // With an extreme speed-up the rest of the playing transition completes within the same layout pass.
            var icon = CreateIconPlayingNormalToPointerOver(AnimatedIconAnimationQueueBehavior.SpeedUpQueueOne, out var recorder, speedUpMultiplier: 1000000f);
            using (recorder)
            {
                SetStateAndWaitForLayout(icon, "Pressed");
                Verify.AreEqual("PointerOverToPressed", GetLastSegment(icon));
                Verify.AreEqual("ToNormal, NormalToPointerOver, PointerOverToPressed", string.Join(", ", recorder.Segments));
            }
        }

        // Scenario: With SpeedUpQueueOne, a speed-up multiplier of 1 and a queue length of 1, sets "Pressed" and then
        // "Normal" while the slowed-down NormalToPointerOver transition plays.
        // Expected: "Pressed" is queued (no new segment); "Normal" overflows the queue so PointerOverToPressed plays at
        // once, and PressedToNormal follows when it completes.
        // Failure means: SpeedUpQueueOne ignores the queue length or loses the queued state.
        // Note: logs SKIPPED and returns when Windows animation effects are disabled.
        [TestMethod]
        public void SpeedUpQueueOneAdvancesImmediatelyWhenQueueIsFull()
        {
            if (!AreAnimationsEnabledOrLogSkip())
            {
                return;
            }

            var icon = CreateIconPlayingNormalToPointerOver(AnimatedIconAnimationQueueBehavior.SpeedUpQueueOne, out var recorder, speedUpMultiplier: 1f, queueLength: 1);
            using (recorder)
            {
                // A speed-up multiplier of 1 keeps the remainder of NormalToPointerOver playing, so the queue is still full below.
                SetStateAndWaitForLayout(icon, "Pressed");
                Verify.AreEqual("NormalToPointerOver", GetLastSegment(icon));
                Verify.AreEqual(2, recorder.Segments.Length);

                // The queue already holds one state, so it plays now and the new state is queued behind it.
                SetStateAndWaitForLayout(icon, "Normal");
                Verify.AreEqual("PointerOverToPressed", GetLastSegment(icon));
                Verify.AreEqual(3, recorder.Segments.Length);

                recorder.WaitForCount(4, c_longTransitionTimeoutMs);
                Verify.AreEqual("PressedToNormal", GetLastSegment(icon));
            }
        }

        // Scenario: With SpeedUpQueueOne (7x speed-up, durations stretched 80x), queues "Pressed" and "Normal" while
        // NormalToPointerOver plays, then sets "PointerOver" once the queue has drained.
        // Expected: queued transitions play in order; PointerOverToPressed is sped up (under 6 s) because another state
        // waits, while the last queued PressedToNormal plays at normal speed (over 12 s) before NormalToPointerOver.
        // Failure means: queued transitions play out of order, are not sped up while more states are waiting, or the
        // last one is wrongly sped up.
        // Note: logs SKIPPED and returns when Windows animation effects are disabled.
        [TestMethod]
        public void SpeedUpQueueOnePlaysQueuedStatesInOrderAfterSpeedingUp()
        {
            if (!AreAnimationsEnabledOrLogSkip())
            {
                return;
            }

            var icon = CreateIconPlayingNormalToPointerOver(AnimatedIconAnimationQueueBehavior.SpeedUpQueueOne, out var recorder);
            using (recorder)
            {
                SetStateAndWaitForLayout(icon, "Pressed");
                SetStateAndWaitForLayout(icon, "Normal");
                Verify.AreEqual("NormalToPointerOver", GetLastSegment(icon));
                Verify.AreEqual(2, recorder.Segments.Length);

                // The playing transition is sped up, so it finishes well before its unmodified duration (about 12s).
                recorder.WaitForCount(3, c_speedUpTimeoutMs);

                // Two states were queued, so PointerOverToPressed (about 12s at 1x) also plays at the 7x speed-up (about 1.7s).
                recorder.WaitForCount(4, c_speedUpTimeoutMs);
                Verify.IsLessThan(recorder.MillisecondsBetween(2, 3), c_spedUpTransitionMaxMs);

                // The last queued state plays at 1x: PressedToNormal (about 25s) has to finish before a newly set state plays.
                SetStateAndWaitForLayout(icon, "PointerOver");
                recorder.WaitForCount(5, c_normalSpeedPressedToNormalTimeoutMs);
                Verify.IsGreaterThan(recorder.MillisecondsBetween(3, 4), c_normalSpeedPressedToNormalMinMs);
                Verify.AreEqual("ToNormal, NormalToPointerOver, PointerOverToPressed, PressedToNormal, NormalToPointerOver", string.Join(", ", recorder.Segments));
            }
        }

        // Scenario: Uses a timed source whose bToc segment lasts exactly 20 ms (200000 ticks) and queues c and d while
        // the 5.12 s aTob transition plays.
        // Expected: bToc is animated, so d plays only after it completes; segments are recorded as "Toa, aTob, bToc,
        // cTod".
        // Failure means: the 20 ms threshold changed and a segment of exactly 20 ms is completed instantly instead of
        // being animated.
        // Note: logs SKIPPED and returns when Windows animation effects are disabled.
        [TestMethod]
        public void TransitionOfExactly20msIsAnimated()
        {
            if (!AreAnimationsEnabledOrLogSkip())
            {
                return;
            }

            // bToc lasts 204800000 ticks * (1024 / 2^20) = 200000 ticks, exactly 20 ms: the shortest segment that is animated.
            var icon = CreateLoadedIcon(CreateQueueBoundarySource(1024));
            using (var recorder = new TransitionRecorder(icon))
            {
                PlayAToBThenQueueCAndD(recorder, icon);
                Verify.AreEqual(2, recorder.Segments.Length);

                // bToc is animated, so the queued d is only played once bToc's animation completes.
                recorder.WaitForCount(4, c_longTransitionTimeoutMs);
                Verify.AreEqual("Toa, aTob, bToc, cTod", string.Join(", ", recorder.Segments));
            }
        }

        // Scenario: Same setup as TransitionOfExactly20msIsAnimated, but bToc lasts 199804 ticks, just under 20 ms.
        // Expected: bToc completes synchronously, so the queued d plays from within it and segments are recorded as
        // "Toa, aTob, cTod, bToc".
        // Failure means: segments shorter than 20 ms are animated instead of being completed immediately.
        // Note: logs SKIPPED and returns when Windows animation effects are disabled.
        [TestMethod]
        public void TransitionShorterThan20msCompletesImmediately()
        {
            if (!AreAnimationsEnabledOrLogSkip())
            {
                return;
            }

            // bToc lasts 204800000 ticks * (1023 / 2^20) = 199804.6875, truncated to 199804 ticks: just under 20 ms.
            var icon = CreateLoadedIcon(CreateQueueBoundarySource(1023));
            using (var recorder = new TransitionRecorder(icon))
            {
                PlayAToBThenQueueCAndD(recorder, icon);
                Verify.AreEqual(2, recorder.Segments.Length);

                // bToc completes synchronously, so the queued d is played (and recorded) from within bToc's transition.
                recorder.WaitForCount(4, c_longTransitionTimeoutMs);
                Verify.AreEqual("Toa, aTob, cTod, bToc", string.Join(", ", recorder.Segments));
            }
        }

        // Scenario: With a 100 s timed source, moves to State "x" (marker 0.9) and then "y", whose transition xToy runs
        // from xToy_Start 0.1 to xToy_End 0.3.
        // Expected: Progress is 0.9 at "x"; just after xToy starts, Progress is between 0.099 and 0.15 (at the start
        // marker), not near 0.9.
        // Failure means: transitions do not jump to their _Start marker before animating.
        // Note: logs SKIPPED and returns when Windows animation effects are disabled.
        [TestMethod]
        public void TransitionAnimationStartsAtStartMarker()
        {
            if (!AreAnimationsEnabledOrLogSkip())
            {
                return;
            }

            var markers = new Dictionary<string, double> { ["x"] = 0.9, ["xToy_Start"] = 0.1, ["xToy_End"] = 0.3 };
            var icon = CreateLoadedIcon(new TimedAnimatedVisualSource(markers, TimeSpan.FromSeconds(100)));
            using (var recorder = new TransitionRecorder(icon))
            {
                SetStateAndWaitForTransitions(recorder, icon, "x", 1);
                Verify.IsLessThan(Math.Abs(0.9f - ReadProgress(icon)), c_progressTolerance);

                // xToy plays from 0.1 to 0.3 over 20 s, so just after it starts Progress is at the start marker, not near 0.9.
                SetStateAndWaitForTransitions(recorder, icon, "y", 2);
                Verify.AreEqual("xToy | xToy_Start | xToy_End", DescribeLastSegment(icon));
                float progress = ReadProgress(icon);
                Verify.IsGreaterThan(progress, 0.099f);
                Verify.IsLessThan(progress, 0.15f);
            }
        }

        // Scenario: With a 100 s timed source, moves to State "x" (marker 0.9) and then to the numeric State "0.5".
        // Expected: the segment is "xTo0.5 |  | 0.5" and just after it starts Progress is still between 0.85 and
        // 0.9001, i.e. it animates from the current position.
        // Failure means: a numeric state jumps to some other start position instead of animating from the current
        // Progress.
        // Note: logs SKIPPED and returns when Windows animation effects are disabled.
        [TestMethod]
        public void NumericStateAnimatesFromCurrentProgress()
        {
            if (!AreAnimationsEnabledOrLogSkip())
            {
                return;
            }

            var markers = new Dictionary<string, double> { ["x"] = 0.9 };
            var icon = CreateLoadedIcon(new TimedAnimatedVisualSource(markers, TimeSpan.FromSeconds(100)));
            using (var recorder = new TransitionRecorder(icon))
            {
                SetStateAndWaitForTransitions(recorder, icon, "x", 1);
                Verify.IsLessThan(Math.Abs(0.9f - ReadProgress(icon)), c_progressTolerance);

                // A numeric state has no start marker: Progress animates from its current 0.9 toward 0.5 over
                // 100 s (the previous segment length defaults to 1), so just after it starts Progress is still near 0.9.
                SetStateAndWaitForTransitions(recorder, icon, "0.5", 2);
                Verify.AreEqual("xTo0.5 |  | 0.5", DescribeLastSegment(icon));
                float progress = ReadProgress(icon);
                Verify.IsGreaterThan(progress, 0.85f);
                Verify.IsLessThan(progress, 0.9001f);
            }
        }
        // Scenario: Checks the generated AnimatedAcceptVisualSource: its Markers table, an animated visual it creates,
        // and how it applies theme colors.
        // Expected: Markers match the expected table exactly; the visual is 48x48, lasts 26666666 ticks (about 2.67 s),
        // has content, accepts SetColorProperty and is closed by Dispose; SetColorProperty("Foreground") recolors its
        // brushes while an unknown property name is ignored.
        // Failure means: the generated AnimatedAcceptVisualSource changed or regressed (markers, size, duration or
        // Foreground theming), so controls that use it may animate or color incorrectly.
        [TestMethod]
        public void AnimatedAcceptVisualSourceContract()
        {
            var source = new AnimatedAcceptVisualSource();
            Verify.AreEqual(DescribeMarkers(ExpectedMarkers.Accept), DescribeMarkers(source.Markers));
            Verify.AreEqual(DescribeExpectedAnimatedVisual(26666666), DescribeAnimatedVisual(source));
            Verify.AreEqual("Foreground applied=True; other name ignored=True", DescribeThemeForeground(source));
        }

        // Scenario: Checks the generated AnimatedBackVisualSource: its Markers table, an animated visual it creates,
        // and how it applies theme colors.
        // Expected: Markers match the expected table exactly; the visual is 48x48, lasts 13333333 ticks (about 1.33 s),
        // has content, accepts SetColorProperty and is closed by Dispose; SetColorProperty("Foreground") recolors its
        // brushes while an unknown property name is ignored.
        // Failure means: the generated AnimatedBackVisualSource changed or regressed (markers, size, duration or
        // Foreground theming), so controls that use it may animate or color incorrectly.
        [TestMethod]
        public void AnimatedBackVisualSourceContract()
        {
            var source = new AnimatedBackVisualSource();
            Verify.AreEqual(DescribeMarkers(ExpectedMarkers.Back), DescribeMarkers(source.Markers));
            Verify.AreEqual(DescribeExpectedAnimatedVisual(13333333), DescribeAnimatedVisual(source));
            Verify.AreEqual("Foreground applied=True; other name ignored=True", DescribeThemeForeground(source));
        }

        // Scenario: Checks the generated AnimatedChevronDownSmallVisualSource: its Markers table, an animated visual it
        // creates, and how it applies theme colors.
        // Expected: Markers match the expected table exactly; the visual is 48x48, lasts 4833333 ticks (about 0.48 s),
        // has content, accepts SetColorProperty and is closed by Dispose; SetColorProperty("Foreground") recolors its
        // brushes while an unknown property name is ignored.
        // Failure means: the generated AnimatedChevronDownSmallVisualSource changed or regressed (markers, size,
        // duration or Foreground theming), so controls that use it may animate or color incorrectly.
        [TestMethod]
        public void AnimatedChevronDownSmallVisualSourceContract()
        {
            var source = new AnimatedChevronDownSmallVisualSource();
            Verify.AreEqual(DescribeMarkers(ExpectedMarkers.ChevronDownSmall), DescribeMarkers(source.Markers));
            Verify.AreEqual(DescribeExpectedAnimatedVisual(4833333), DescribeAnimatedVisual(source));
            Verify.AreEqual("Foreground applied=True; other name ignored=True", DescribeThemeForeground(source));
        }

        // Scenario: Checks the generated AnimatedChevronRightDownSmallVisualSource: its Markers table, an animated
        // visual it creates, and how it applies theme colors.
        // Expected: Markers match the expected table exactly; the visual is 48x48, lasts 36666666 ticks (about 3.67 s),
        // has content, accepts SetColorProperty and is closed by Dispose; SetColorProperty("Foreground") recolors its
        // brushes while an unknown property name is ignored.
        // Failure means: the generated AnimatedChevronRightDownSmallVisualSource changed or regressed (markers, size,
        // duration or Foreground theming), so controls that use it may animate or color incorrectly.
        [TestMethod]
        public void AnimatedChevronRightDownSmallVisualSourceContract()
        {
            var source = new AnimatedChevronRightDownSmallVisualSource();
            Verify.AreEqual(DescribeMarkers(ExpectedMarkers.ChevronRightDownSmall), DescribeMarkers(source.Markers));
            Verify.AreEqual(DescribeExpectedAnimatedVisual(36666666), DescribeAnimatedVisual(source));
            Verify.AreEqual("Foreground applied=True; other name ignored=True", DescribeThemeForeground(source));
        }

        // Scenario: Checks the generated AnimatedChevronUpDownSmallVisualSource: its Markers table, an animated visual
        // it creates, and how it applies theme colors.
        // Expected: Markers match the expected table exactly; the visual is 48x48, lasts 43333333 ticks (about 4.33 s),
        // has content, accepts SetColorProperty and is closed by Dispose; SetColorProperty("Foreground") recolors its
        // brushes while an unknown property name is ignored.
        // Failure means: the generated AnimatedChevronUpDownSmallVisualSource changed or regressed (markers, size,
        // duration or Foreground theming), so controls that use it may animate or color incorrectly.
        [TestMethod]
        public void AnimatedChevronUpDownSmallVisualSourceContract()
        {
            var source = new AnimatedChevronUpDownSmallVisualSource();
            Verify.AreEqual(DescribeMarkers(ExpectedMarkers.ChevronUpDownSmall), DescribeMarkers(source.Markers));
            Verify.AreEqual(DescribeExpectedAnimatedVisual(43333333), DescribeAnimatedVisual(source));
            Verify.AreEqual("Foreground applied=True; other name ignored=True", DescribeThemeForeground(source));
        }

        // Scenario: Checks the generated AnimatedFindVisualSource: its Markers table, an animated visual it creates,
        // and how it applies theme colors.
        // Expected: Markers match the expected table exactly; the visual is 48x48, lasts 13333333 ticks (about 1.33 s),
        // has content, accepts SetColorProperty and is closed by Dispose; SetColorProperty("Foreground") recolors its
        // brushes while an unknown property name is ignored.
        // Failure means: the generated AnimatedFindVisualSource changed or regressed (markers, size, duration or
        // Foreground theming), so controls that use it may animate or color incorrectly.
        [TestMethod]
        public void AnimatedFindVisualSourceContract()
        {
            var source = new AnimatedFindVisualSource();
            Verify.AreEqual(DescribeMarkers(ExpectedMarkers.Find), DescribeMarkers(source.Markers));
            Verify.AreEqual(DescribeExpectedAnimatedVisual(13333333), DescribeAnimatedVisual(source));
            Verify.AreEqual("Foreground applied=True; other name ignored=True", DescribeThemeForeground(source));
        }

        // Scenario: Checks the generated AnimatedGlobalNavigationButtonVisualSource: its Markers table, an animated
        // visual it creates, and how it applies theme colors.
        // Expected: Markers match the expected table exactly; the visual is 48x48, lasts 13333333 ticks (about 1.33 s),
        // has content, accepts SetColorProperty and is closed by Dispose; SetColorProperty("Foreground") recolors its
        // brushes while an unknown property name is ignored.
        // Failure means: the generated AnimatedGlobalNavigationButtonVisualSource changed or regressed (markers, size,
        // duration or Foreground theming), so controls that use it may animate or color incorrectly.
        [TestMethod]
        public void AnimatedGlobalNavigationButtonVisualSourceContract()
        {
            var source = new AnimatedGlobalNavigationButtonVisualSource();
            Verify.AreEqual(DescribeMarkers(ExpectedMarkers.GlobalNavigationButton), DescribeMarkers(source.Markers));
            Verify.AreEqual(DescribeExpectedAnimatedVisual(13333333), DescribeAnimatedVisual(source));
            Verify.AreEqual("Foreground applied=True; other name ignored=True", DescribeThemeForeground(source));
        }

        // Scenario: Checks the generated AnimatedSettingsVisualSource: its Markers table, an animated visual it
        // creates, and how it applies theme colors.
        // Expected: Markers match the expected table exactly; the visual is 48x48, lasts 20000000 ticks (about 2 s),
        // has content, accepts SetColorProperty and is closed by Dispose; SetColorProperty("Foreground") recolors its
        // brushes while an unknown property name is ignored.
        // Failure means: the generated AnimatedSettingsVisualSource changed or regressed (markers, size, duration or
        // Foreground theming), so controls that use it may animate or color incorrectly.
        [TestMethod]
        public void AnimatedSettingsVisualSourceContract()
        {
            var source = new AnimatedSettingsVisualSource();
            Verify.AreEqual(DescribeMarkers(ExpectedMarkers.Settings), DescribeMarkers(source.Markers));
            Verify.AreEqual(DescribeExpectedAnimatedVisual(20000000), DescribeAnimatedVisual(source));
            Verify.AreEqual("Foreground applied=True; other name ignored=True", DescribeThemeForeground(source));
        }

        // Scenario: Loads an icon with AnimatedChevronRightDownSmallVisualSource (zero duration) and sets State
        // "NormalOff" then "NormalOn".
        // Expected: "NormalOff" resolves to a marker ending in "ToNormalOff_End"; "NormalOn" plays NormalOffToNormalOn
        // between its _Start and _End markers and Progress ends at NormalOffToNormalOn_End.
        // Failure means: the generated chevron's On/Off markers no longer work with AnimatedIcon's state transitions.
        [TestMethod]
        public void AnimatedChevronRightDownSmallTransitionsBetweenOnAndOffStates()
        {
            var icon = CreateLoadedIcon(new AnimatedChevronRightDownSmallVisualSource(), durationMultiplier: 0);

            using (var recorder = new TransitionRecorder(icon))
            {
                // No marker is named "ToNormalOff" exactly, so the icon cuts to the end of a transition into NormalOff.
                SetStateAndWaitForTransitions(recorder, icon, "NormalOff", 1);
                Verify.IsTrue(DescribeLastSegment(icon).StartsWith("ToNormalOff |  | "));
                Verify.IsTrue(DescribeLastSegment(icon).EndsWith("ToNormalOff_End"));

                SetStateAndWaitForTransitions(recorder, icon, "NormalOn", 2);
                Verify.AreEqual("NormalOffToNormalOn | NormalOffToNormalOn_Start | NormalOffToNormalOn_End", DescribeLastSegment(icon));
                Verify.IsLessThan(Math.Abs((float)ExpectedMarkers.ChevronRightDownSmall["NormalOffToNormalOn_End"] - ReadProgress(icon)), c_progressTolerance);
            }
        }

        // Scenario: Reads the AnimatedIcon State, Source, FallbackIconSource and MirroredWhenRightToLeft property
        // identifiers, constructs an AnimatedIcon and an AnimatedIconSource, then reads them again.
        // Expected: the 4 identifiers are non-null and distinct, unchanged after construction, and distinct from
        // AnimatedIconSource's 3 same-named identifiers (7 distinct in total).
        // Failure means: property registration is missing, is redone when an instance is constructed, or is shared
        // incorrectly between AnimatedIcon and AnimatedIconSource.
        [TestMethod]
        public void DependencyPropertyIdentifiersAreRegisteredDistinctAndStable()
        {
            var identifiers = ReadIconPropertyIds().ToArray();

            // Constructing instances runs EnsureProperties again; it must keep the registered identifiers.
            CreateAnimatedIconAndIconSource();
            var identifiersReadAgain = ReadIconPropertyIds().ToArray();
            var iconSourceIdentifiers = ReadIconSourcePropertyIds();

            Verify.AreEqual(4, CountDistinctNonNull(identifiers, new DependencyProperty[0]));
            Verify.AreEqual(identifiers[0], identifiersReadAgain[0]);
            Verify.AreEqual(identifiers[1], identifiersReadAgain[1]);
            Verify.AreEqual(identifiers[2], identifiersReadAgain[2]);
            Verify.AreEqual(identifiers[3], identifiersReadAgain[3]);

            // AnimatedIconSource registers its own Source, FallbackIconSource and MirroredWhenRightToLeft properties.
            Verify.AreEqual(7, CountDistinctNonNull(identifiers, iconSourceIdentifiers));
        }

        // Scenario: Reads the registered default values of the four AnimatedIcon properties and the values of a newly
        // created icon.
        // Expected: defaults are State "", Source null, FallbackIconSource null and MirroredWhenRightToLeft false; a
        // new icon has no local values and returns those defaults.
        // Failure means: the registered metadata defaults changed (for example State null instead of ""), which can
        // break code that reads them.
        [TestMethod]
        public void DependencyPropertyDefaultsMatchRegisteredMetadata()
        {
            var ids = ReadIconPropertyIds();
            var defaults = GetMetadataDefaultValues(ids.ToArray(), typeof(AnimatedIcon));

            // State defaults to an empty string rather than null; OnLoaded unboxes ancestor values as strings.
            Verify.AreEqual("", defaults[0]);
            Verify.IsNull(defaults[1]);
            Verify.IsNull(defaults[2]);
            Verify.AreEqual(false, defaults[3]);

            var icon = CreateBareIcon();
            Verify.AreEqual(4, CountUnsetLocalValues(icon, ids.ToArray()));
            Verify.AreEqual("", GetValueOf(icon, ids.State));
            Verify.IsNull(GetValueOf(icon, ids.Source));
            Verify.IsNull(GetValueOf(icon, ids.FallbackIconSource));
            Verify.AreEqual(false, GetValueOf(icon, ids.MirroredWhenRightToLeft));
        }

        // Scenario: Sets the four AnimatedIcon properties through SetValue and reads them through the CLR properties,
        // then the reverse, then clears them; finally clears MirroredWhenRightToLeft (true) on a loaded RightToLeft
        // icon.
        // Expected: both access paths see the same values; ClearValue restores the defaults and leaves no local values;
        // clearing MirroredWhenRightToLeft brings back the -1 counter-mirror ScaleX.
        // Failure means: CLR properties and dependency properties use different storage, or ClearValue does not restore
        // defaults or run the property-changed handling.
        [TestMethod]
        public void ClrPropertiesAndDependencyPropertiesShareStorage()
        {
            var ids = ReadIconPropertyIds();
            var icon = CreateDetachedIcon(null);
            var source = new AnimatedBackVisualSource();
            var fallback = CreateSymbolIconSource(Symbol.Accept);

            // Values set through the identifiers are what the CLR properties return.
            SetValueOf(icon, ids.Source, source);
            SetValueOf(icon, ids.FallbackIconSource, fallback);
            SetValueOf(icon, ids.MirroredWhenRightToLeft, true);
            SetValueOf(icon, ids.State, "Pressed");
            Verify.AreEqual(source, GetSourceOf(icon));
            Verify.AreEqual(fallback, GetFallbackIconSourceOf(icon));
            Verify.AreEqual(true, GetMirroredWhenRightToLeftOf(icon));
            Verify.AreEqual("Pressed", GetStateOf(icon));

            // Values set through the CLR properties are stored under the identifiers.
            var otherSource = new AnimatedSettingsVisualSource();
            var otherFallback = CreateSymbolIconSource(Symbol.Back);
            SetClrProperties(icon, otherSource, otherFallback, false, "Normal");
            Verify.AreEqual(otherSource, GetValueOf(icon, ids.Source));
            Verify.AreEqual(otherFallback, GetValueOf(icon, ids.FallbackIconSource));
            Verify.AreEqual(false, GetValueOf(icon, ids.MirroredWhenRightToLeft));
            Verify.AreEqual("Normal", GetValueOf(icon, ids.State));

            // Clearing the local values restores the registered defaults.
            SetMirroredWhenRightToLeft(icon, true);
            ClearValuesOf(icon, ids.ToArray());
            Verify.AreEqual(4, CountUnsetLocalValues(icon, ids.ToArray()));
            Verify.IsNull(GetSourceOf(icon));
            Verify.IsNull(GetFallbackIconSourceOf(icon));
            Verify.AreEqual(false, GetMirroredWhenRightToLeftOf(icon));
            Verify.AreEqual("", GetStateOf(icon));

            // ClearValue raises the change callback like a set: the right-to-left counter-mirroring comes back.
            var loadedIcon = CreateLoadedIcon(new AnimatedBackVisualSource());
            SetFlowDirection(loadedIcon, FlowDirection.RightToLeft);
            SetValueOf(loadedIcon, ids.MirroredWhenRightToLeft, true);
            Verify.AreEqual(1.0, GetMirrorScaleX(loadedIcon));
            ClearValuesOf(loadedIcon, ids.MirroredWhenRightToLeft);
            Verify.AreEqual(-1.0, GetMirrorScaleX(loadedIcon));
        }

        // Scenario: Sets the attached AnimatedIcon.State on a SolidColorBrush (not a UIElement) and on Borders,
        // including a null value.
        // Expected: the values read back through GetState and GetValue ("Pressed", "PointerOver"); an untouched Border
        // returns "", and setting null reads back as "".
        // Failure means: the attached State cannot be stored on objects that are not icons (or fails for
        // non-UIElements), or its default/null handling changed.
        [TestMethod]
        public void StateAttachedPropertyIsStoredOnAnyDependencyObject()
        {
            var ids = ReadIconPropertyIds();
            var brush = CreateSolidColorBrush();
            var border = CreateBorder();
            var untouchedBorder = CreateBorder();

            // A brush is a DependencyObject but not a UIElement; the State change callback ignores non-icons.
            SetStateOf(brush, "Pressed");
            SetValueOf(border, ids.State, "PointerOver");
            Verify.AreEqual("Pressed", GetStateOf(brush));
            Verify.AreEqual("Pressed", GetValueOf(brush, ids.State));
            Verify.AreEqual("PointerOver", GetStateOf(border));

            Verify.AreEqual("", GetStateOf(untouchedBorder));
            Verify.AreEqual("", GetValueOf(untouchedBorder, ids.State));

            // A null string is marshaled as an empty HSTRING.
            SetStateOf(border, null);
            Verify.AreEqual("", GetStateOf(border));
        }

        // Scenario: Loads XAML with a Grid setting AnimatedIcon.State="Pressed" around an AnimatedIcon with
        // MirroredWhenRightToLeft="True", a SymbolIconSource(Accept) fallback and an AnimatedBackVisualSource as
        // content.
        // Expected: the Grid and the icon both have State "Pressed", Source is an AnimatedBackVisualSource, mirroring
        // is true, the fallback symbol is Accept and the visual is displayed with no fallback child.
        // Failure means: XAML parsing of AnimatedIcon (content property, attached State or property values) or State
        // inheritance from an ancestor on load is broken.
        [TestMethod]
        public void XamlMarkupSetsAnimatedIconPropertiesAndAttachedState()
        {
            var grid = (Grid)LoadMarkupAsContent(c_iconUnderGridWithAttachedStateMarkup);
            var icon = GetFirstChildIcon(grid);

            Verify.AreEqual("Pressed", GetStateOf(grid));
            // The icon has no State of its own, so it copies the ancestor's State when it is loaded.
            Verify.AreEqual("Pressed", GetStateOf(icon));
            // The animated visual source is the content property value.
            Verify.AreEqual(typeof(AnimatedBackVisualSource).FullName, GetSourceTypeName(icon));
            Verify.AreEqual(true, GetMirroredWhenRightToLeftOf(icon));
            Verify.AreEqual(Symbol.Accept, GetFallbackIconSourceSymbol(icon));
            Verify.IsNotNull(GetAnimatedVisualRoot(icon));
            Verify.AreEqual(1, GetRootPanelChildCount(icon));
        }

        // Scenario: Loads a templated ContentControl whose AnimatedIcon gets MirroredWhenRightToLeft from a Style
        // setter and AnimatedIcon.State from a "Pressed" VisualState setter, then goes to Pressed and back to Normal.
        // Expected: mirroring is true without being a local value; State is "" at first, "Pressed" in the Pressed state
        // and "" again in Normal.
        // Failure means: Style or VisualState setters targeting AnimatedIcon properties (the pattern used by Button,
        // CheckBox and ComboBox templates) do not resolve or apply.
        [TestMethod]
        public void StyleAndVisualStateSettersResolveAnimatedIconProperties()
        {
            var ids = ReadIconPropertyIds();
            var control = (Control)LoadMarkupAsContent(c_templatedControlWithIconStateSetterMarkup);
            var icon = FindTemplateIcon(control);

            // The style setter's "True" is converted to the Boolean property type and is not a local value.
            Verify.AreEqual(true, GetMirroredWhenRightToLeftOf(icon));
            Verify.AreEqual(false, HasLocalValue(icon, ids.MirroredWhenRightToLeft));
            Verify.AreEqual("", GetStateOf(icon));

            // The same Target="Name.(AnimatedIcon.State)" setter pattern is used by Button, CheckBox and ComboBox templates.
            Verify.AreEqual(true, GoToStateAndWaitForSetters(control, "Pressed"));
            Verify.AreEqual("Pressed", GetStateOf(icon));
            Verify.AreEqual(true, GoToStateAndWaitForSetters(control, "Normal"));
            Verify.AreEqual("", GetStateOf(icon));
        }

        // Scenario: Gets the AnimatedIcon type from the controls XAML metadata provider, inspects its members,
        // activates an instance and sets and gets values through the metadata members.
        // Expected: the full name matches, the content property is Source, the four members are writable dependency
        // properties, an unknown member is absent, and member get/set reaches the real properties.
        // Failure means: the generated XAML type information for AnimatedIcon is wrong, which breaks XAML parsing or
        // binding of AnimatedIcon.
        [TestMethod]
        public void XamlMetadataProviderDescribesAnimatedIconMembers()
        {
            var type = GetControlsXamlType(c_animatedIconTypeName);

            Verify.AreEqual(c_animatedIconTypeName, GetFullNameOf(type));
            Verify.AreEqual("Source", GetContentPropertyName(type));
            Verify.AreEqual(
                "Source=DP,RW; FallbackIconSource=DP,RW; MirroredWhenRightToLeft=DP,RW; State=DP,RW",
                DescribeMembers(type, new[] { "Source", "FallbackIconSource", "MirroredWhenRightToLeft", "State" }));
            Verify.AreEqual(false, HasMember(type, "NotAMember"));

            var instance = ActivateXamlType(type);
            Verify.AreEqual(typeof(AnimatedIcon), instance.GetType());
            var icon = (AnimatedIcon)instance;

            // Member values go through the registered dependency properties.
            SetMemberValue(type, "MirroredWhenRightToLeft", icon, true);
            SetMemberValue(type, "State", icon, "Pressed");
            Verify.AreEqual(true, GetMirroredWhenRightToLeftOf(icon));
            Verify.AreEqual("Pressed", GetStateOf(icon));

            var source = new AnimatedFindVisualSource();
            SetSource(icon, source);
            Verify.AreEqual(source, GetMemberValue(type, "Source", icon));
        }

        // Scenario: Gets the activation factories for AnimatedIcon, AnimatedIconSource and AnimatedIconTestHooks and
        // asks each factory object for its runtime class name.
        // Expected: each factory reports its own full class name.
        // Failure means: the generated activation factory for one of these classes is broken or registered under the
        // wrong name.
        [TestMethod]
        public void ActivationFactoriesReportRuntimeClassNames()
        {
            Verify.AreEqual(c_animatedIconTypeName, GetActivationFactoryRuntimeClassName(c_animatedIconTypeName));
            Verify.AreEqual(c_animatedIconSourceTypeName, GetActivationFactoryRuntimeClassName(c_animatedIconSourceTypeName));
            Verify.AreEqual(c_animatedIconTestHooksTypeName, GetActivationFactoryRuntimeClassName(c_animatedIconTestHooksTypeName));
        }

        private const int c_transitionTimeoutMs = 10000;
        private const int c_longTransitionTimeoutMs = 30000;
        private const int c_speedUpTimeoutMs = 8000;
        // At an 80x duration multiplier: PointerOverToPressed is about 12s at 1x and 1.7s at the 7x speed-up;
        // PressedToNormal is about 25s at 1x and 3.6s at 7x.
        private const long c_spedUpTransitionMaxMs = 6000;
        private const long c_normalSpeedPressedToNormalMinMs = 12000;
        private const int c_normalSpeedPressedToNormalTimeoutMs = 45000;
        private const float c_progressTolerance = 0.0001f;
        // Stretches the 0.11-progress NormalToPointerOver segment of AnimatedBackVisualSource (1.33s) to about 12s.
        private const float c_slowDurationMultiplier = 80f;

        private const string c_animatedIconTypeName = "Microsoft.UI.Xaml.Controls.AnimatedIcon";
        private const string c_animatedIconSourceTypeName = "Microsoft.UI.Xaml.Controls.AnimatedIconSource";
        private const string c_animatedIconTestHooksTypeName = "Microsoft.UI.Private.Controls.AnimatedIconTestHooks";

        // The attached State is set on the Grid the way control templates set it on a ContentPresenter.
        private const string c_iconUnderGridWithAttachedStateMarkup =
            @"<Grid xmlns='http://schemas.microsoft.com/winfx/2006/xaml/presentation'
                    xmlns:controls='using:Microsoft.UI.Xaml.Controls'
                    xmlns:visuals='using:Microsoft.UI.Xaml.Controls.AnimatedVisuals'
                    controls:AnimatedIcon.State='Pressed'>
                <controls:AnimatedIcon MirroredWhenRightToLeft='True'>
                    <controls:AnimatedIcon.FallbackIconSource>
                        <controls:SymbolIconSource Symbol='Accept'/>
                    </controls:AnimatedIcon.FallbackIconSource>
                    <visuals:AnimatedBackVisualSource/>
                </controls:AnimatedIcon>
            </Grid>";

        private const string c_templatedControlWithIconStateSetterMarkup =
            @"<ContentControl xmlns='http://schemas.microsoft.com/winfx/2006/xaml/presentation'
                              xmlns:x='http://schemas.microsoft.com/winfx/2006/xaml'
                              xmlns:controls='using:Microsoft.UI.Xaml.Controls'>
                <ContentControl.Template>
                    <ControlTemplate TargetType='ContentControl'>
                        <Grid>
                            <VisualStateManager.VisualStateGroups>
                                <VisualStateGroup x:Name='CommonStates'>
                                    <VisualState x:Name='Normal'/>
                                    <VisualState x:Name='Pressed'>
                                        <VisualState.Setters>
                                            <Setter Target='Icon.(controls:AnimatedIcon.State)' Value='Pressed'/>
                                        </VisualState.Setters>
                                    </VisualState>
                                </VisualStateGroup>
                            </VisualStateManager.VisualStateGroups>
                            <controls:AnimatedIcon x:Name='Icon'>
                                <controls:AnimatedIcon.Style>
                                    <Style TargetType='controls:AnimatedIcon'>
                                        <Setter Property='MirroredWhenRightToLeft' Value='True'/>
                                    </Style>
                                </controls:AnimatedIcon.Style>
                            </controls:AnimatedIcon>
                        </Grid>
                    </ControlTemplate>
                </ContentControl.Template>
            </ContentControl>";

        // Records the LastAnimationSegmentChanged notifications of one icon, which TransitionStates raises synchronously.
        private sealed class TransitionRecorder : IDisposable
        {
            private readonly List<string> _segments = new List<string>();
            private readonly List<long> _segmentTimesMs = new List<long>();
            private readonly Stopwatch _clock = Stopwatch.StartNew();
            private readonly AutoResetEvent _segmentChanged = new AutoResetEvent(false);
            private string _iconName;

            public TransitionRecorder(AnimatedIcon icon)
            {
                RunOnUIThread.Execute(() =>
                {
                    _iconName = icon.Name;
                    AnimatedIconTestHooks.LastAnimationSegmentChanged += OnLastAnimationSegmentChanged;
                });
            }

            public string[] Segments
            {
                get { lock (_segments) { return _segments.ToArray(); } }
            }

            public void WaitForCount(int count, int timeoutMs = c_transitionTimeoutMs)
            {
                var stopwatch = Stopwatch.StartNew();
                while (Segments.Length < count && stopwatch.ElapsedMilliseconds < timeoutMs)
                {
                    _segmentChanged.WaitOne(TimeSpan.FromMilliseconds(Math.Max(1, timeoutMs - stopwatch.ElapsedMilliseconds)));
                }
                Verify.AreEqual(count, Segments.Length, "Transitions so far: " + string.Join(", ", Segments));
            }

            // Time between the starts of two recorded transitions, i.e. how long the earlier one played.
            public long MillisecondsBetween(int fromIndex, int toIndex)
            {
                lock (_segments) { return _segmentTimesMs[toIndex] - _segmentTimesMs[fromIndex]; }
            }

            public void Dispose()
            {
                RunOnUIThread.Execute(() => AnimatedIconTestHooks.LastAnimationSegmentChanged -= OnLastAnimationSegmentChanged);
            }

            private void OnLastAnimationSegmentChanged(AnimatedIcon sender, object args)
            {
                if (sender.Name == _iconName)
                {
                    lock (_segments)
                    {
                        _segments.Add(AnimatedIconTestHooks.GetLastAnimationSegment(sender));
                        _segmentTimesMs.Add(_clock.ElapsedMilliseconds);
                    }
                    _segmentChanged.Set();
                }
            }
        }

        private AnimatedIcon CreateLoadedIcon(IAnimatedVisualSource2 source, float durationMultiplier = 1f, Symbol? fallbackSymbol = null)
        {
            AnimatedIcon icon = null;
            RunOnUIThread.Execute(() =>
            {
                icon = new AnimatedIcon { Name = "AnimatedIcon_" + Guid.NewGuid().ToString("N") };
                if (fallbackSymbol.HasValue)
                {
                    icon.FallbackIconSource = new SymbolIconSource { Symbol = fallbackSymbol.Value };
                }
                icon.Source = source;
                if (durationMultiplier != 1f)
                {
                    AnimatedIconTestHooks.SetDurationMultiplier(icon, durationMultiplier);
                }
                Content = icon;
                Content.UpdateLayout();
            });
            IdleSynchronizer.Wait();
            return icon;
        }

        private AnimatedIcon CreateIconInParentGridWithState(string parentState, out Grid parentGrid)
        {
            AnimatedIcon icon = null;
            Grid grid = null;
            RunOnUIThread.Execute(() =>
            {
                icon = new AnimatedIcon();
                grid = new Grid { Children = { icon } };
                AnimatedIcon.SetState(grid, parentState);
                Content = grid;
                Content.UpdateLayout();
            });
            IdleSynchronizer.Wait();
            parentGrid = grid;
            return icon;
        }

        private static void ChangeSourcesAndParentStatesInOnePass(AnimatedIcon icon, Grid parentGrid,
            out Visual chevronVisual, out Visual settingsVisual, out Visual finalVisual)
        {
            Visual chevron = null;
            Visual settings = null;
            Visual final = null;
            RunOnUIThread.Execute(() =>
            {
                icon.Source = new AnimatedChevronDownSmallVisualSource();
                chevron = FindAnimatedVisualRoot(icon);
                AnimatedIcon.SetState(parentGrid, "Normal");
                icon.Source = new AnimatedSettingsVisualSource();
                settings = FindAnimatedVisualRoot(icon);
                AnimatedIcon.SetState(parentGrid, "PointerOver");
                icon.Source = null;
                final = FindAnimatedVisualRoot(icon);
                AnimatedIcon.SetState(parentGrid, "");
            });
            chevronVisual = chevron;
            settingsVisual = settings;
            finalVisual = final;
        }

        private AnimatedIcon CreateIconInStackPanel(out StackPanel stackPanel)
        {
            AnimatedIcon icon = null;
            StackPanel panel = null;
            RunOnUIThread.Execute(() =>
            {
                icon = new AnimatedIcon();
                panel = new StackPanel { Children = { icon } };
                Content = panel;
                Content.UpdateLayout();
            });
            IdleSynchronizer.Wait();
            stackPanel = panel;
            return icon;
        }

        private AnimatedIcon CreateIconInCanvas(IAnimatedVisualSource2 source)
        {
            AnimatedIcon icon = null;
            RunOnUIThread.Execute(() =>
            {
                icon = new AnimatedIcon { Source = source };
                Content = new Canvas { Children = { icon } };
                Content.UpdateLayout();
            });
            IdleSynchronizer.Wait();
            return icon;
        }

        private static AnimatedIcon CreateDetachedIcon(IAnimatedVisualSource2 source)
        {
            AnimatedIcon icon = null;
            RunOnUIThread.Execute(() => icon = new AnimatedIcon { Source = source });
            return icon;
        }

        private static AnimatedIcon CreateDetachedIconWithForeground(Color color, out SolidColorBrush brush)
        {
            AnimatedIcon icon = null;
            SolidColorBrush foreground = null;
            RunOnUIThread.Execute(() =>
            {
                foreground = new SolidColorBrush(color);
                icon = new AnimatedIcon { Foreground = foreground };
            });
            brush = foreground;
            return icon;
        }

        private AnimatedIcon CreateIconWithStateUnderParentWithState(string iconState, string parentState, out Grid parentGrid)
        {
            AnimatedIcon icon = null;
            Grid grid = null;
            RunOnUIThread.Execute(() =>
            {
                icon = new AnimatedIcon();
                AnimatedIcon.SetState(icon, iconState);
                grid = new Grid { Children = { icon } };
                AnimatedIcon.SetState(grid, parentState);
                Content = grid;
                Content.UpdateLayout();
            });
            IdleSynchronizer.Wait();
            parentGrid = grid;
            return icon;
        }

        private Canvas CreateLoadedCanvas()
        {
            Canvas canvas = null;
            RunOnUIThread.Execute(() =>
            {
                canvas = new Canvas();
                Content = canvas;
                Content.UpdateLayout();
            });
            return canvas;
        }

        private static AnimatedIcon AddBackIconToCanvas(Canvas canvas, double width, double height)
        {
            AnimatedIcon icon = null;
            RunOnUIThread.Execute(() =>
            {
                icon = new AnimatedIcon { Source = new AnimatedBackVisualSource(), Width = width, Height = height };
                canvas.Children.Add(icon);
                canvas.UpdateLayout();
            });
            return icon;
        }

        private void LoadIcon(AnimatedIcon icon)
        {
            RunOnUIThread.Execute(() =>
            {
                Content = icon;
                Content.UpdateLayout();
            });
            IdleSynchronizer.Wait();
        }

        // Without system animations every segment is applied instantly and nothing is ever playing, so the
        // queue-behavior tests cannot observe a playing transition. Matches the repo's log-and-skip convention.
        private static bool AreAnimationsEnabledOrLogSkip()
        {
            if (new UISettings().AnimationsEnabled)
            {
                return true;
            }
            Log.Warning("SKIPPED: this AnimatedIcon queue-behavior test is disabled when Windows animation effects are turned off; it was not validated on this machine.");
            return false;
        }
        // The visual duration and marker values are binary fractions, so every float product in PlaySegment is exact.
        private static readonly TimeSpan c_boundaryVisualDuration = TimeSpan.FromTicks(204800000);

        // aTob lasts 5.12 s; bToc lasts bTocLengthIn2Pow20ths / 2^20 of the visual duration; cTod has zero length.
        private static TimedAnimatedVisualSource CreateQueueBoundarySource(int bTocLengthIn2Pow20ths)
        {
            var markers = new Dictionary<string, double>
            {
                ["aTob_Start"] = 0.0,
                ["aTob_End"] = 0.25,
                ["bToc_Start"] = 0.5,
                ["bToc_End"] = 0.5 + bTocLengthIn2Pow20ths / 1048576.0,
                ["cTod_Start"] = 0.75,
                ["cTod_End"] = 0.75,
            };
            return new TimedAnimatedVisualSource(markers, c_boundaryVisualDuration);
        }

        // Queues c and d (QueueOne) while the 5.12 s aTob transition plays.
        private static void PlayAToBThenQueueCAndD(TransitionRecorder recorder, AnimatedIcon icon)
        {
            SetStateAndWaitForTransitions(recorder, icon, "a", 1);
            SetStateAndWaitForTransitions(recorder, icon, "b", 2);
            SetStateAndWaitForLayout(icon, "c");
            SetStateAndWaitForLayout(icon, "d");
        }
        // Leaves the icon playing the slowed NormalToPointerOver transition of AnimatedBackVisualSource.
        private AnimatedIcon CreateIconPlayingNormalToPointerOver(
            AnimatedIconAnimationQueueBehavior behavior,
            out TransitionRecorder recorder,
            float speedUpMultiplier = 7f,
            int queueLength = 4)
        {

            var icon = CreateLoadedIcon(new AnimatedBackVisualSource(), c_slowDurationMultiplier);
            RunOnUIThread.Execute(() =>
            {
                AnimatedIconTestHooks.SetAnimationQueueBehavior(icon, behavior);
                AnimatedIconTestHooks.SetSpeedUpMultiplier(icon, speedUpMultiplier);
                AnimatedIconTestHooks.SetQueueLength(icon, queueLength);
            });

            recorder = new TransitionRecorder(icon);
            SetStateAndWaitForTransitions(recorder, icon, "Normal", 1);
            SetStateAndWaitForTransitions(recorder, icon, "PointerOver", 2);
            Verify.AreEqual("NormalToPointerOver", GetLastSegment(icon));
            return icon;
        }

        private static void SetStateAndWaitForTransitions(TransitionRecorder recorder, AnimatedIcon icon, string state, int expectedCount)
        {
            RunOnUIThread.Execute(() => AnimatedIcon.SetState(icon, state));
            recorder.WaitForCount(expectedCount);
        }

        // AnimatedIcon applies a new state on the first layout pass after it is set.
        private static void SetStateAndWaitForLayout(AnimatedIcon icon, string state)
        {
            SetStatesInOneLayoutPass(icon, state);
        }

        private static void SetStatesInOneLayoutPass(AnimatedIcon icon, params string[] states)
        {
            var layoutUpdated = new AutoResetEvent(false);
            EventHandler<object> onLayoutUpdated = (s, e) => layoutUpdated.Set();
            RunOnUIThread.Execute(() =>
            {
                icon.LayoutUpdated += onLayoutUpdated;
                foreach (var state in states)
                {
                    AnimatedIcon.SetState(icon, state);
                }
            });
            Verify.IsTrue(layoutUpdated.WaitOne(DefaultWaitTimeInMS), "Waiting for LayoutUpdated");
            RunOnUIThread.Execute(() => icon.LayoutUpdated -= onLayoutUpdated);
        }

        private static void SetStateOf(DependencyObject element, string state)
        {
            RunOnUIThread.Execute(() => AnimatedIcon.SetState(element, state));
        }

        private static string GetStateOf(DependencyObject element)
        {
            string state = null;
            RunOnUIThread.Execute(() => state = AnimatedIcon.GetState(element));
            return state;
        }

        private static string GetLastSegment(AnimatedIcon icon)
        {
            string segment = null;
            RunOnUIThread.Execute(() => segment = AnimatedIconTestHooks.GetLastAnimationSegment(icon));
            return segment;
        }

        // "<transition> | <start marker> | <end marker>" as recorded by the test hooks.
        private static string DescribeLastSegment(AnimatedIcon icon)
        {
            string description = null;
            RunOnUIThread.Execute(() =>
            {
                description = AnimatedIconTestHooks.GetLastAnimationSegment(icon) + " | " +
                    AnimatedIconTestHooks.GetLastAnimationSegmentStart(icon) + " | " +
                    AnimatedIconTestHooks.GetLastAnimationSegmentEnd(icon);
            });
            return description;
        }

        // Progress is driven by an expression animation, so it is read through a composition spy.
        private static float ReadProgress(AnimatedIcon icon)
        {
            CompositionPropertySet properties = null;
            RunOnUIThread.Execute(() =>
            {
                properties = FindAnimatedVisualRoot(icon).Properties;
                CompositionPropertySpy.StartSpyingScalarProperty(properties, "Progress", float.NaN);
            });
            CompositionPropertySpy.SynchronouslyTickUIThread(10);
            RunOnUIThread.Execute(() => CompositionPropertySpy.StopSpyingProperty(properties, "Progress"));
            CompositionPropertySpy.SynchronouslyTickUIThread(10);

            float progress = float.NaN;
            var status = CompositionGetValueStatus.NotFound;
            RunOnUIThread.Execute(() => status = CompositionPropertySpy.TryGetScalar(properties, "Progress", out progress));
            Verify.AreEqual(CompositionGetValueStatus.Succeeded, status);
            return progress;
        }

        private static void SetSource(AnimatedIcon icon, IAnimatedVisualSource2 source)
        {
            RunOnUIThread.Execute(() => icon.Source = source);
        }

        private static void SetFallbackSymbol(AnimatedIcon icon, Symbol symbol)
        {
            RunOnUIThread.Execute(() => icon.FallbackIconSource = new SymbolIconSource { Symbol = symbol });
        }

        private static void SetFallbackGlyph(AnimatedIcon icon, string glyph)
        {
            RunOnUIThread.Execute(() => icon.FallbackIconSource = new FontIconSource { Glyph = glyph });
        }

        private static void SetFlowDirection(AnimatedIcon icon, FlowDirection flowDirection)
        {
            RunOnUIThread.Execute(() => icon.FlowDirection = flowDirection);
        }

        private static void SetMirroredWhenRightToLeft(AnimatedIcon icon, bool mirrored)
        {
            RunOnUIThread.Execute(() => icon.MirroredWhenRightToLeft = mirrored);
        }

        private static SolidColorBrush SetSolidForeground(AnimatedIcon icon, Color color)
        {
            SolidColorBrush brush = null;
            RunOnUIThread.Execute(() =>
            {
                brush = new SolidColorBrush(color);
                icon.Foreground = brush;
            });
            return brush;
        }

        private static void SetGradientForeground(AnimatedIcon icon)
        {
            RunOnUIThread.Execute(() => icon.Foreground = new LinearGradientBrush());
        }

        private static void SetBrushColor(SolidColorBrush brush, Color color)
        {
            RunOnUIThread.Execute(() => brush.Color = color);
        }

        private static string DescribeLastColorProperty(ControlledAnimatedVisualSource source)
        {
            string description = null;
            RunOnUIThread.Execute(() =>
            {
                var last = source.ColorPropertyCalls.Last();
                description = last.Key + "=" + last.Value;
            });
            return description;
        }

        private static Size MeasureIcon(AnimatedIcon icon, Size available)
        {
            var desired = new Size(double.NaN, double.NaN);
            RunOnUIThread.Execute(() =>
            {
                icon.Measure(available);
                desired = icon.DesiredSize;
            });
            return desired;
        }

        private static double GetActualWidth(FrameworkElement element)
        {
            double width = double.NaN;
            RunOnUIThread.Execute(() => width = element.ActualWidth);
            return width;
        }

        private static double GetActualHeight(FrameworkElement element)
        {
            double height = double.NaN;
            RunOnUIThread.Execute(() => height = element.ActualHeight);
            return height;
        }

        private static Visual GetAnimatedVisualRoot(AnimatedIcon icon)
        {
            Visual root = null;
            RunOnUIThread.Execute(() => root = FindAnimatedVisualRoot(icon));
            return root;
        }

        private static Vector3 GetRootVisualOffset(AnimatedIcon icon)
        {
            var offset = new Vector3(float.NaN);
            RunOnUIThread.Execute(() => offset = FindAnimatedVisualRoot(icon).Offset);
            return offset;
        }

        private static Vector3 GetRootVisualScale(AnimatedIcon icon)
        {
            var scale = new Vector3(float.NaN);
            RunOnUIThread.Execute(() => scale = FindAnimatedVisualRoot(icon).Scale);
            return scale;
        }

        private static Vector2 GetRootVisualSize(AnimatedIcon icon)
        {
            var size = new Vector2(float.NaN);
            RunOnUIThread.Execute(() => size = FindAnimatedVisualRoot(icon).Size);
            return size;
        }

        private static int GetRootPanelChildCount(AnimatedIcon icon)
        {
            int count = -1;
            RunOnUIThread.Execute(() => count = FindRootPanel(icon).Children.Count);
            return count;
        }

        // Child 0 of the root panel is the PathIcon path that AnimatedIcon keeps collapsed.
        private static Visibility GetPathVisibility(AnimatedIcon icon)
        {
            var visibility = Visibility.Visible;
            RunOnUIThread.Execute(() => visibility = FindRootPanel(icon).Children[0].Visibility);
            return visibility;
        }

        // The fallback icon is appended as child 1 of the root panel.
        private static Symbol? GetFallbackSymbol(AnimatedIcon icon)
        {
            Symbol? symbol = null;
            RunOnUIThread.Execute(() => symbol = (FindRootPanel(icon).Children[1] as SymbolIcon)?.Symbol);
            return symbol;
        }

        private static string GetFallbackGlyph(AnimatedIcon icon)
        {
            string glyph = null;
            RunOnUIThread.Execute(() => glyph = (FindRootPanel(icon).Children[1] as FontIcon)?.Glyph);
            return glyph;
        }

        private static double GetMirrorScaleX(AnimatedIcon icon)
        {
            double scaleX = double.NaN;
            RunOnUIThread.Execute(() => scaleX = ((ScaleTransform)icon.RenderTransform).ScaleX);
            return scaleX;
        }

        private static Point GetRenderTransformOrigin(AnimatedIcon icon)
        {
            var origin = new Point(double.NaN, double.NaN);
            RunOnUIThread.Execute(() => origin = icon.RenderTransformOrigin);
            return origin;
        }

        private static Panel FindRootPanel(AnimatedIcon icon)
        {
            return (Panel)VisualTreeHelper.GetChild(icon, 0);
        }

        private static Visual FindAnimatedVisualRoot(AnimatedIcon icon)
        {
            return ElementCompositionPreview.GetElementChildVisual(FindRootPanel(icon));
        }

        private static string DescribeMarkers(IReadOnlyDictionary<string, double> markers)
        {
            return string.Join("; ", markers
                .OrderBy(marker => marker.Key, StringComparer.Ordinal)
                .Select(marker => marker.Key + "=" + marker.Value.ToString("R", CultureInfo.InvariantCulture)));
        }

        private static string DescribeExpectedAnimatedVisual(long durationTicks)
        {
            return "Diagnostics=null; Size=48x48; DurationTicks=" + durationTicks + "; RootHasChildren=True; SetColorPropertyDoesNotThrow=True; RootClosedByDispose=True";
        }

        // Creates the animated visual and describes the contract observable through IAnimatedVisual, then closes it.
        private static string DescribeAnimatedVisual(IAnimatedVisualSource2 source)
        {
            string description = null;
            RunOnUIThread.Execute(() =>
            {
                var compositor = Microsoft.UI.Xaml.Media.CompositionTarget.GetCompositorForCurrentThread();
                var visual = source.TryCreateAnimatedVisual(compositor, out object diagnostics);
                var root = visual.RootVisual as ContainerVisual;
                description = "Diagnostics=" + (diagnostics == null ? "null" : diagnostics.ToString()) +
                    "; Size=" + visual.Size.X + "x" + visual.Size.Y +
                    "; DurationTicks=" + visual.Duration.Ticks +
                    "; RootHasChildren=" + (root != null && root.Children.Count > 0);

                // Only checks that the calls are accepted; DescribeThemeForeground observes the resulting theme color.
                source.SetColorProperty("Foreground", Colors.Red);
                source.SetColorProperty("NotAThemeProperty", Colors.Red);
                description += "; SetColorPropertyDoesNotThrow=True";

                visual.Dispose();
                description += "; RootClosedByDispose=" + IsClosed(root);
            });
            return description;
        }

        private static readonly Color c_themeTestColor = new Color { A = 0xFF, R = 0x12, G = 0x34, B = 0x56 };
        private static readonly Color c_otherTestColor = new Color { A = 0xFF, R = 0x65, G = 0x43, B = 0x21 };

        // Hosts a new animated visual and reads the colors its brushes render after SetColorProperty("Foreground")
        // and after SetColorProperty with an unknown theme property name.
        private string DescribeThemeForeground(IAnimatedVisualSource2 source)
        {
            IAnimatedVisual visual = null;
            Border host = null;
            CompositionColorBrush[] brushes = null;
            RunOnUIThread.Execute(() =>
            {
                var compositor = Microsoft.UI.Xaml.Media.CompositionTarget.GetCompositorForCurrentThread();
                visual = source.TryCreateAnimatedVisual(compositor, out object diagnostics);
                host = new Border { Width = 48, Height = 48 };
                Content = host;
                Content.UpdateLayout();
                ElementCompositionPreview.SetElementChildVisual(host, visual.RootVisual);
                brushes = FindColorBrushes(visual.RootVisual);
                source.SetColorProperty("Foreground", c_themeTestColor);
            });
            var afterForeground = ReadBrushColors(brushes);

            RunOnUIThread.Execute(() => source.SetColorProperty("NotAThemeProperty", c_otherTestColor));
            var afterOtherName = ReadBrushColors(brushes);

            RunOnUIThread.Execute(() =>
            {
                ElementCompositionPreview.SetElementChildVisual(host, null);
                visual.Dispose();
            });
            return "Foreground applied=" + afterForeground.Contains(c_themeTestColor) +
                "; other name ignored=" + (afterOtherName.Contains(c_themeTestColor) && !afterOtherName.Contains(c_otherTestColor));
        }

        private static CompositionColorBrush[] FindColorBrushes(Visual root)
        {
            var brushes = new List<CompositionColorBrush>();
            var visuals = new Stack<Visual>();
            var shapes = new Stack<CompositionShape>();
            visuals.Push(root);
            while (visuals.Count > 0)
            {
                var visual = visuals.Pop();
                if (visual is ShapeVisual shapeVisual)
                {
                    foreach (var shape in shapeVisual.Shapes) { shapes.Push(shape); }
                }
                if (visual is ContainerVisual container)
                {
                    foreach (var child in container.Children) { visuals.Push(child); }
                }
            }
            while (shapes.Count > 0)
            {
                var shape = shapes.Pop();
                if (shape is CompositionContainerShape containerShape)
                {
                    foreach (var child in containerShape.Shapes) { shapes.Push(child); }
                }
                else if (shape is CompositionSpriteShape sprite)
                {
                    foreach (var brush in new[] { sprite.FillBrush, sprite.StrokeBrush })
                    {
                        if (brush is CompositionColorBrush colorBrush && !brushes.Contains(colorBrush)) { brushes.Add(colorBrush); }
                    }
                }
            }
            return brushes.ToArray();
        }

        // A brush's Color is driven by an expression bound to the theme property set, so it is read through a spy
        // property set in the same way as CompositionPropertySpy reads scalars.
        private static Color[] ReadBrushColors(CompositionColorBrush[] brushes)
        {
            CompositionPropertySet spy = null;
            RunOnUIThread.Execute(() =>
            {
                spy = Microsoft.UI.Xaml.Media.CompositionTarget.GetCompositorForCurrentThread().CreatePropertySet();
                for (int i = 0; i < brushes.Length; i++)
                {
                    spy.InsertColor("Color" + i, Colors.Transparent);
                    var expression = spy.Compositor.CreateExpressionAnimation("brush.Color");
                    expression.SetReferenceParameter("brush", brushes[i]);
                    spy.StartAnimation("Color" + i, expression);
                }
            });
            CompositionPropertySpy.SynchronouslyTickUIThread(10);
            RunOnUIThread.Execute(() =>
            {
                for (int i = 0; i < brushes.Length; i++) { spy.StopAnimation("Color" + i); }
            });
            CompositionPropertySpy.SynchronouslyTickUIThread(10);

            var colors = new List<Color>();
            RunOnUIThread.Execute(() =>
            {
                for (int i = 0; i < brushes.Length; i++)
                {
                    if (spy.TryGetColor("Color" + i, out Color color) == CompositionGetValueStatus.Succeeded) { colors.Add(color); }
                }
            });
            return colors.ToArray();
        }

        // The AnimatedIcon dependency property identifiers, read from the public statics on the UI thread.
        private sealed class IconPropertyIds
        {
            public DependencyProperty State;
            public DependencyProperty Source;
            public DependencyProperty FallbackIconSource;
            public DependencyProperty MirroredWhenRightToLeft;

            public DependencyProperty[] ToArray()
            {
                return new[] { State, Source, FallbackIconSource, MirroredWhenRightToLeft };
            }
        }

        private static IconPropertyIds ReadIconPropertyIds()
        {
            var ids = new IconPropertyIds();
            RunOnUIThread.Execute(() =>
            {
                ids.State = AnimatedIcon.StateProperty;
                ids.Source = AnimatedIcon.SourceProperty;
                ids.FallbackIconSource = AnimatedIcon.FallbackIconSourceProperty;
                ids.MirroredWhenRightToLeft = AnimatedIcon.MirroredWhenRightToLeftProperty;
            });
            return ids;
        }

        // Source, FallbackIconSource, MirroredWhenRightToLeft.
        private static DependencyProperty[] ReadIconSourcePropertyIds()
        {
            DependencyProperty[] ids = null;
            RunOnUIThread.Execute(() =>
            {
                ids = new[] { AnimatedIconSource.SourceProperty, AnimatedIconSource.FallbackIconSourceProperty, AnimatedIconSource.MirroredWhenRightToLeftProperty };
            });
            return ids;
        }

        // No property is set, so every AnimatedIcon property is at its registered default.
        private static AnimatedIcon CreateBareIcon()
        {
            AnimatedIcon icon = null;
            RunOnUIThread.Execute(() => icon = new AnimatedIcon());
            return icon;
        }

        private static void CreateAnimatedIconAndIconSource()
        {
            object icon = null;
            object iconSource = null;
            RunOnUIThread.Execute(() =>
            {
                icon = new AnimatedIcon();
                iconSource = new AnimatedIconSource();
            });
            Verify.IsNotNull(icon);
            Verify.IsNotNull(iconSource);
        }

        private static int CountDistinctNonNull(DependencyProperty[] first, DependencyProperty[] second)
        {
            var distinct = new List<DependencyProperty>();
            foreach (var property in first.Concat(second))
            {
                if (property != null && !distinct.Any(known => ReferenceEquals(known, property)))
                {
                    distinct.Add(property);
                }
            }
            return distinct.Count;
        }

        private static object[] GetMetadataDefaultValues(DependencyProperty[] properties, Type ownerType)
        {
            object[] defaults = null;
            RunOnUIThread.Execute(() =>
            {
                defaults = properties.Select(property => property.GetMetadata(ownerType).DefaultValue).ToArray();
            });
            return defaults;
        }

        private static int CountUnsetLocalValues(DependencyObject element, DependencyProperty[] properties)
        {
            int count = -1;
            RunOnUIThread.Execute(() =>
            {
                count = properties.Count(property => element.ReadLocalValue(property) == DependencyProperty.UnsetValue);
            });
            return count;
        }

        private static bool HasLocalValue(DependencyObject element, DependencyProperty property)
        {
            return CountUnsetLocalValues(element, new[] { property }) == 0;
        }

        private static object GetValueOf(DependencyObject element, DependencyProperty property)
        {
            object value = null;
            RunOnUIThread.Execute(() => value = element.GetValue(property));
            return value;
        }

        private static void SetValueOf(DependencyObject element, DependencyProperty property, object value)
        {
            RunOnUIThread.Execute(() => element.SetValue(property, value));
        }

        private static void ClearValuesOf(DependencyObject element, params DependencyProperty[] properties)
        {
            RunOnUIThread.Execute(() =>
            {
                foreach (var property in properties)
                {
                    element.ClearValue(property);
                }
            });
        }

        private static IAnimatedVisualSource2 GetSourceOf(AnimatedIcon icon)
        {
            IAnimatedVisualSource2 source = null;
            RunOnUIThread.Execute(() => source = icon.Source);
            return source;
        }

        private static IconSource GetFallbackIconSourceOf(AnimatedIcon icon)
        {
            IconSource fallback = null;
            RunOnUIThread.Execute(() => fallback = icon.FallbackIconSource);
            return fallback;
        }

        private static bool GetMirroredWhenRightToLeftOf(AnimatedIcon icon)
        {
            bool mirrored = false;
            RunOnUIThread.Execute(() => mirrored = icon.MirroredWhenRightToLeft);
            return mirrored;
        }

        private static void SetClrProperties(AnimatedIcon icon, IAnimatedVisualSource2 source, IconSource fallback, bool mirrored, string state)
        {
            RunOnUIThread.Execute(() =>
            {
                icon.Source = source;
                icon.FallbackIconSource = fallback;
                icon.MirroredWhenRightToLeft = mirrored;
                AnimatedIcon.SetState(icon, state);
            });
        }

        private static SymbolIconSource CreateSymbolIconSource(Symbol symbol)
        {
            SymbolIconSource iconSource = null;
            RunOnUIThread.Execute(() => iconSource = new SymbolIconSource { Symbol = symbol });
            return iconSource;
        }

        private static SolidColorBrush CreateSolidColorBrush()
        {
            SolidColorBrush brush = null;
            RunOnUIThread.Execute(() => brush = new SolidColorBrush(Colors.Red));
            return brush;
        }

        private static Border CreateBorder()
        {
            Border border = null;
            RunOnUIThread.Execute(() => border = new Border());
            return border;
        }

        private FrameworkElement LoadMarkupAsContent(string markup)
        {
            FrameworkElement root = null;
            RunOnUIThread.Execute(() =>
            {
                root = (FrameworkElement)XamlReader.Load(markup);
                Content = root;
                Content.UpdateLayout();
            });
            IdleSynchronizer.Wait();
            return root;
        }

        private static AnimatedIcon GetFirstChildIcon(Panel panel)
        {
            AnimatedIcon icon = null;
            RunOnUIThread.Execute(() => icon = (AnimatedIcon)panel.Children[0]);
            return icon;
        }

        // The template root is a Grid whose first child is the named AnimatedIcon.
        private static AnimatedIcon FindTemplateIcon(Control control)
        {
            AnimatedIcon icon = null;
            RunOnUIThread.Execute(() => icon = (AnimatedIcon)((Panel)VisualTreeHelper.GetChild(control, 0)).Children[0]);
            return icon;
        }

        private static string GetSourceTypeName(AnimatedIcon icon)
        {
            string name = null;
            RunOnUIThread.Execute(() => name = icon.Source?.GetType().FullName);
            return name;
        }

        private static Symbol? GetFallbackIconSourceSymbol(AnimatedIcon icon)
        {
            Symbol? symbol = null;
            RunOnUIThread.Execute(() => symbol = (icon.FallbackIconSource as SymbolIconSource)?.Symbol);
            return symbol;
        }

        // VisualStateManager applies the setters of the new state after GoToState returns,
        // so the setter values are read once the UI thread is idle.
        private static bool GoToStateAndWaitForSetters(Control control, string stateName)
        {
            bool changed = false;
            RunOnUIThread.Execute(() => changed = VisualStateManager.GoToState(control, stateName, false));
            IdleSynchronizer.Wait();
            return changed;
        }

        private static IXamlType GetControlsXamlType(string typeName)
        {
            IXamlType type = null;
            RunOnUIThread.Execute(() => type = new XamlControlsXamlMetaDataProvider().GetXamlType(typeName));
            Verify.IsNotNull(type, "XAML type " + typeName);
            return type;
        }

        private static string GetFullNameOf(IXamlType type)
        {
            string name = null;
            RunOnUIThread.Execute(() => name = type.FullName);
            return name;
        }

        private static string GetContentPropertyName(IXamlType type)
        {
            string name = null;
            RunOnUIThread.Execute(() => name = type.ContentProperty?.Name);
            return name;
        }

        // "Name=DP,RW" for a writable dependency-property member, "Name=missing" when the type has no such member.
        private static string DescribeMembers(IXamlType type, string[] memberNames)
        {
            string description = null;
            RunOnUIThread.Execute(() =>
            {
                description = string.Join("; ", memberNames.Select(name =>
                {
                    var member = type.GetMember(name);
                    return member == null ? name + "=missing" :
                        member.Name + "=" + (member.IsDependencyProperty ? "DP" : "CLR") + "," + (member.IsReadOnly ? "RO" : "RW");
                }));
            });
            return description;
        }

        private static bool HasMember(IXamlType type, string memberName)
        {
            bool hasMember = true;
            RunOnUIThread.Execute(() => hasMember = type.GetMember(memberName) != null);
            return hasMember;
        }

        private static object ActivateXamlType(IXamlType type)
        {
            object instance = null;
            RunOnUIThread.Execute(() => instance = type.ActivateInstance());
            Verify.IsNotNull(instance, "Activated " + type.FullName);
            return instance;
        }

        private static void SetMemberValue(IXamlType type, string memberName, object instance, object value)
        {
            RunOnUIThread.Execute(() => type.GetMember(memberName).SetValue(instance, value));
        }

        private static object GetMemberValue(IXamlType type, string memberName, object instance)
        {
            object value = null;
            RunOnUIThread.Execute(() => value = type.GetMember(memberName).GetValue(instance));
            return value;
        }

        // Asks the activation factory object itself (IInspectable::GetRuntimeClassName), not an instance.
        private static string GetActivationFactoryRuntimeClassName(string typeName)
        {
            string name = null;
            RunOnUIThread.Execute(() => name = new global::WinRT.IInspectable(global::WinRT.ActivationFactory.Get(typeName)).GetRuntimeClassName(false));
            return name;
        }

        private static bool IsClosed(Visual visual)
        {
            const int RO_E_CLOSED = unchecked((int)0x80000013);
            try
            {
                var unused = visual.Size;
                return false;
            }
            catch (Exception e) when (e is ObjectDisposedException || e.HResult == RO_E_CLOSED)
            {
                return true;
            }
        }
    }
}
