// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

using System;
using System.Threading;
using Common;
using Microsoft.UI.Xaml.Controls;
using MUXControlsTestApp.Utilities;
using Microsoft.UI.Xaml.Automation.Peers;
using Microsoft.UI.Xaml.Automation.Provider;
using Microsoft.UI.Xaml.Automation.Peers;
using Microsoft.UI.Xaml.Media;
using Microsoft.UI.Xaml.Automation;
using Microsoft.UI.Xaml.Shapes;
using Microsoft.UI.Xaml.Markup;
using Microsoft.UI.Xaml.Controls;
using WEX.TestExecution;
using WEX.TestExecution.Markup;
using WEX.Logging.Interop;

namespace Microsoft.UI.Xaml.Tests.MUXControls.ApiTests
{
    [TestClass]
    public class ProgressBarTests : ApiTestBase
    {
        // Scenario: load two ProgressBars from XAML, one inside a Grid whose resources set ProgressBarTrackHeight to 3,
        //           and one without the override.
        // Expected: the template's track Rectangle is 3 high in the first bar and keeps the default height of 1 in the
        //           second.
        // A failure means: ProgressBarTrackHeight could not be overridden per scope, or the override would leak.
        [TestMethod]
        public void ResourceOverridablity()
        {
            RunOnUIThread.Execute(() =>
            {
                var root = (StackPanel)XamlReader.Load(
    @"<StackPanel xmlns='http://schemas.microsoft.com/winfx/2006/xaml/presentation' 
                             xmlns:x='http://schemas.microsoft.com/winfx/2006/xaml'
                             xmlns:primitives='using:Microsoft.UI.Xaml.Controls.Primitives'
                             xmlns:controls='using:Microsoft.UI.Xaml.Controls'> 
                             <Grid>
                                <Grid.Resources>
                                    <x:Double x:Key='ProgressBarTrackHeight'>3</x:Double>
                                </Grid.Resources>
                                <controls:ProgressBar/>
                            </Grid>
                            <controls:ProgressBar/>
                       </StackPanel>");

                Content = root;
                Content.UpdateLayout();

                var grid = VisualTreeHelper.GetChild(root, 0);
                var progressBar1 = VisualTreeHelper.GetChild(grid, 0);
                var templateRoot1 = VisualTreeHelper.GetChild(progressBar1, 0);
                var Border11 = VisualTreeHelper.GetChild(templateRoot1, 0);
                var Border12 = VisualTreeHelper.GetChild(Border11, 0);
                var grid1 = VisualTreeHelper.GetChild(Border12, 0);
                var rect1 = VisualTreeHelper.GetChild(grid1, 0) as Rectangle;
                Verify.AreEqual(3, rect1.Height);

                var progressBar2 = VisualTreeHelper.GetChild(root, 1);
                var templateRoot2 = VisualTreeHelper.GetChild(progressBar2, 0);
                var Border21 = VisualTreeHelper.GetChild(templateRoot2, 0);
                var Border22 = VisualTreeHelper.GetChild(Border21, 0);
                var grid2 = VisualTreeHelper.GetChild(Border22, 0);
                var rect2 = VisualTreeHelper.GetChild(grid2, 0) as Rectangle;
                Verify.AreEqual(1, rect2.Height);
            });
        }

        // Scenario: a ProgressBar named "Bar" goes through combinations of IsIndeterminate, ShowPaused and ShowError; a
        //           peer is also constructed directly.
        // Expected: class name and control type ProgressBar; a determinate, normal bar is named "Bar"; indeterminate,
        //           paused and error each add a status prefix; error wins over paused, paused over indeterminate; the
        //           direct peer matches.
        // A failure means: screen readers would announce the wrong or no status for a busy, paused or failed bar.
        [TestMethod]
        public void AutomationPeerNameReflectsStatusPrecedence()
        {
            ProgressBar progressBar = null;

            RunOnUIThread.Execute(() =>
            {
                progressBar = CreateProgressBar(200);
                AutomationProperties.SetName(progressBar, "Bar");
            });

            LoadInHost(progressBar);

            RunOnUIThread.Execute(() =>
            {
                var peer = FrameworkElementAutomationPeer.CreatePeerForElement(progressBar);
                Verify.IsNotNull(peer);
                Verify.AreEqual("Microsoft.UI.Xaml.Controls.ProgressBar", peer.GetClassName());
                Verify.AreEqual(AutomationControlType.ProgressBar, peer.GetAutomationControlType());
                Verify.AreEqual("Bar", peer.GetName(), "A determinate, normal ProgressBar reports only its name.");

                progressBar.IsIndeterminate = true;
                var busyName = peer.GetName();
                VerifyNameHasStatusPrefix(busyName, "Bar", "indeterminate");

                progressBar.ShowPaused = true;
                var pausedName = peer.GetName();
                VerifyNameHasStatusPrefix(pausedName, "Bar", "paused");
                Verify.AreNotEqual(busyName, pausedName, "Paused status takes precedence over indeterminate status.");

                progressBar.ShowError = true;
                var errorName = peer.GetName();
                VerifyNameHasStatusPrefix(errorName, "Bar", "error");
                Verify.AreNotEqual(pausedName, errorName, "Error status takes precedence over paused status.");
                Verify.AreNotEqual(busyName, errorName, "Error status takes precedence over indeterminate status.");

                progressBar.ShowPaused = false;
                Verify.AreEqual(errorName, peer.GetName(), "Error status is reported regardless of paused state.");

                progressBar.ShowError = false;
                progressBar.IsIndeterminate = false;
                progressBar.ShowPaused = true;
                Verify.AreEqual(pausedName, peer.GetName(), "Paused status does not depend on indeterminate state.");

                progressBar.ShowPaused = false;
                Verify.AreEqual("Bar", peer.GetName());

                var explicitPeer = new ProgressBarAutomationPeer(progressBar);
                Verify.AreEqual("Microsoft.UI.Xaml.Controls.ProgressBar", explicitPeer.GetClassName());
                Verify.AreEqual("Bar", explicitPeer.GetName());
            });
        }

        // Scenario: a determinate bar (range 10-60, value 35) is read through RangeValue, updated, set through
        //           SetValue(250), then made indeterminate and determinate again.
        // Expected: Minimum 10, Maximum 60, Value 35 then 50, NaN SmallChange/LargeChange; after SetValue(250) Value is
        //           in range and reported; there is no RangeValue pattern while indeterminate, and it returns when
        //           determinate.
        // A failure means: assistive technology would read wrong values, or a value for an indeterminate bar.
        [TestMethod]
        public void AutomationPeerExposesRangeValueOnlyWhenDeterminate()
        {
            ProgressBar progressBar = null;

            RunOnUIThread.Execute(() =>
            {
                progressBar = CreateProgressBar(200);
                progressBar.Minimum = 10;
                progressBar.Maximum = 60;
                progressBar.Value = 35;
            });

            LoadInHost(progressBar);

            RunOnUIThread.Execute(() =>
            {
                var peer = FrameworkElementAutomationPeer.CreatePeerForElement(progressBar);
                var provider = peer.GetPattern(PatternInterface.RangeValue) as IRangeValueProvider;
                Verify.IsNotNull(provider, "A determinate ProgressBar exposes the RangeValue pattern.");
                Verify.AreEqual(10.0, provider.Minimum);
                Verify.AreEqual(60.0, provider.Maximum);
                Verify.AreEqual(35.0, provider.Value);
                Verify.IsTrue(double.IsNaN(provider.SmallChange), "ProgressBar has no small change.");
                Verify.IsTrue(double.IsNaN(provider.LargeChange), "ProgressBar has no large change.");

                progressBar.Value = 50;
                Verify.AreEqual(50.0, provider.Value, "The provider reflects the current value.");

                // SetValue is exercised only for behavior that is not in question: the value stays within range
                // and the provider keeps reporting the control's actual value.
                provider.SetValue(250);
                Verify.IsTrue(progressBar.Value >= progressBar.Minimum && progressBar.Value <= progressBar.Maximum, "Value stays within range.");
                Verify.AreEqual(progressBar.Value, provider.Value);

                progressBar.IsIndeterminate = true;
                Verify.IsNull(peer.GetPattern(PatternInterface.RangeValue), "An indeterminate ProgressBar has no RangeValue pattern.");

                progressBar.IsIndeterminate = false;
                Verify.IsNotNull(peer.GetPattern(PatternInterface.RangeValue), "The RangeValue pattern returns when determinate again.");
            });
        }

        // Scenario: a 20 high bar with padding (5,2,7,3) is resized to widths 180, 181, 280 and 281.
        // Expected: EllipseDiameter/EllipseOffset are 4/4 up to 180, 5/7 up to 280 and 6/9 above; animation positions
        //           scale with the width (e.g. EllipseAnimationEndPosition = width/3); ClipRect excludes the padding
        //           (5, 2, width-12, 15).
        // A failure means: the indeterminate animation would be sized or placed wrong, or draw into the padding.
        [TestMethod]
        public void TemplateSettingsFollowWidthThresholds()
        {
            ProgressBar progressBar = null;

            RunOnUIThread.Execute(() =>
            {
                progressBar = CreateProgressBar(180);
                progressBar.Height = 20;
                progressBar.Padding = new Thickness(5, 2, 7, 3);
            });

            LoadInHost(progressBar);

            // Small ellipses up to 180, medium up to 280, large above.
            VerifyTemplateSettingsAtWidth(progressBar, 180, expectedDiameter: 4, expectedOffset: 4);
            VerifyTemplateSettingsAtWidth(progressBar, 181, expectedDiameter: 5, expectedOffset: 7);
            VerifyTemplateSettingsAtWidth(progressBar, 280, expectedDiameter: 5, expectedOffset: 7);
            VerifyTemplateSettingsAtWidth(progressBar, 281, expectedDiameter: 6, expectedOffset: 9);
        }

        // Scenario: a 200 wide bar changes range, Value, IsIndeterminate, ShowPaused, ShowError and Visibility.
        // Expected: the determinate indicator is (Value-Min)/(Max-Min) of 200 (50, 150, 0 for an empty range, 100 for
        //           70 in 20-120); indeterminate shows 80/120; paused or error widen the second to 200
        //           (IndeterminatePaused/IndeterminateError, Error when collapsed); states return to Indeterminate,
        //           then Determinate.
        // A failure means: the bar would draw the wrong amount of progress or the wrong state visuals.
        [TestMethod]
        public void IndicatorWidthsAndStatesFollowRangeAndStatus()
        {
            ProgressBar progressBar = null;
            Rectangle determinateIndicator = null;
            Rectangle indeterminateIndicator = null;
            Rectangle indeterminateIndicator2 = null;

            RunOnUIThread.Execute(() =>
            {
                progressBar = CreateProgressBar(200);
                progressBar.Value = 25;
            });

            LoadInHost(progressBar);

            RunOnUIThread.Execute(() =>
            {
                determinateIndicator = (Rectangle)progressBar.FindVisualChildByName("DeterminateProgressBarIndicator");
                indeterminateIndicator = (Rectangle)progressBar.FindVisualChildByName("IndeterminateProgressBarIndicator");
                indeterminateIndicator2 = (Rectangle)progressBar.FindVisualChildByName("IndeterminateProgressBarIndicator2");

                Verify.AreEqual(50.0, determinateIndicator.Width, "25% of a 200 wide bar.");
                VerifyState(progressBar, "Determinate");

                progressBar.Value = 75;
                Verify.AreEqual(150.0, determinateIndicator.Width);
                Verify.AreEqual(-100.0, progressBar.TemplateSettings.IndicatorLengthDelta, "Delta is the negated growth of the indicator.");

                progressBar.Minimum = 50;
                progressBar.Maximum = 50;
                Verify.AreEqual(50.0, progressBar.Value);
                Verify.AreEqual(0.0, determinateIndicator.Width, "An empty range has no determinate indicator.");

                progressBar.Minimum = 0;
                progressBar.Maximum = 100;
                progressBar.Value = 25;
                Verify.AreEqual(50.0, determinateIndicator.Width);

                // A non-zero Minimum is subtracted from Value: (70 - 20) / (120 - 20) of 200 = 100.
                progressBar.Maximum = 120;
                progressBar.Minimum = 20;
                progressBar.Value = 70;
                Verify.AreEqual(100.0, determinateIndicator.Width, "Indicator width is relative to Minimum.");

                progressBar.Minimum = 0;
                progressBar.Maximum = 100;
                progressBar.Value = 25;
                Verify.AreEqual(50.0, determinateIndicator.Width);

                progressBar.SetValue(ProgressBar.IsIndeterminateProperty, true);
                Verify.AreEqual(0.0, determinateIndicator.Width, "Indeterminate hides the determinate indicator.");
                Verify.AreEqual(80.0, indeterminateIndicator.Width, "First indeterminate indicator is 40% of the bar.");
                Verify.AreEqual(120.0, indeterminateIndicator2.Width, "Second indeterminate indicator is 60% of the bar.");
                VerifyState(progressBar, "Indeterminate");

                progressBar.SetValue(ProgressBar.ShowPausedProperty, true);
                Verify.AreEqual(200.0, indeterminateIndicator2.Width, "Paused indeterminate shows a full-width indicator.");
                VerifyState(progressBar, "IndeterminatePaused");

                progressBar.SetValue(ProgressBar.ShowPausedProperty, false);
                progressBar.SetValue(ProgressBar.ShowErrorProperty, true);
                Verify.AreEqual(200.0, indeterminateIndicator2.Width, "Error indeterminate shows a full-width indicator.");
                VerifyState(progressBar, "IndeterminateError");

                progressBar.Visibility = Visibility.Collapsed;
                VerifyState(progressBar, "Error");

                // Clearing the error recomputes the indeterminate indicators even while collapsed:
                // the second indicator returns from full width to 60% of the bar.
                progressBar.SetValue(ProgressBar.ShowErrorProperty, false);
                Verify.AreEqual(0.0, determinateIndicator.Width, "Still indeterminate: no determinate indicator.");
                Verify.AreEqual(80.0, indeterminateIndicator.Width);
                Verify.AreEqual(120.0, indeterminateIndicator2.Width, "Without error/paused the second indicator is 60% of the bar.");

                progressBar.Visibility = Visibility.Visible;
                VerifyState(progressBar, "Indeterminate");

                progressBar.IsIndeterminate = false;
            });

            // Updating -> Determinate has a VisualTransition storyboard, so the group's CurrentState
            // reports the target state only once that transition completes.
            WaitForState(progressBar, "Determinate");
        }

        // Scenario: compare GetValue/SetValue/ClearValue with the CLR property for every public DependencyProperty of
        //           ProgressBar, on an unparented bar.
        // Expected: identifiers are non-null, distinct (also from RangeBase.ValueProperty) and stable; defaults match;
        //           values set through either surface are visible through the other; ClearValue restores the default;
        //           TemplateSettings is a stable non-null instance.
        // A failure means: XAML, bindings or styles using the DP identifiers would disagree with the CLR properties.
        [TestMethod]
        public void DependencyPropertyIdentifiersAndClrPropertiesAgree()
        {
            RunOnUIThread.Execute(() =>
            {
                var properties = new DependencyProperty[]
                {
                    ProgressBar.IsIndeterminateProperty,
                    ProgressBar.ShowErrorProperty,
                    ProgressBar.ShowPausedProperty,
                    Microsoft.UI.Xaml.Controls.Primitives.RangeBase.ValueProperty,
                };
                for (int i = 0; i < properties.Length; i++)
                {
                    Verify.IsNotNull(properties[i], "Dependency property identifier #" + i + " should not be null");
                    for (int j = 0; j < i; j++)
                    {
                        Verify.IsFalse(ReferenceEquals(properties[i], properties[j]), "Dependency property identifiers #" + j + " and #" + i + " should be distinct");
                    }
                }
                Verify.IsTrue(ReferenceEquals(ProgressBar.ShowErrorProperty, ProgressBar.ShowErrorProperty), "Identifiers are stable.");

                var progressBar = new ProgressBar();
                VerifyDependencyPropertyRoundTrip(progressBar, ProgressBar.IsIndeterminateProperty, () => progressBar.IsIndeterminate, v => progressBar.IsIndeterminate = v, false, true);
                VerifyDependencyPropertyRoundTrip(progressBar, ProgressBar.ShowErrorProperty, () => progressBar.ShowError, v => progressBar.ShowError = v, false, true);
                VerifyDependencyPropertyRoundTrip(progressBar, ProgressBar.ShowPausedProperty, () => progressBar.ShowPaused, v => progressBar.ShowPaused = v, false, true);

                var templateSettings = progressBar.TemplateSettings;
                Verify.IsNotNull(templateSettings, "TemplateSettings is created with the bar.");
                Verify.IsTrue(ReferenceEquals(templateSettings, progressBar.TemplateSettings), "TemplateSettings returns the same instance.");
            });
        }

        private static ProgressBar CreateProgressBar(double width)
        {
            return new ProgressBar()
            {
                Width = width,
                Padding = new Thickness(0),
                BorderThickness = new Thickness(0),
                UseLayoutRounding = false,
                HorizontalAlignment = HorizontalAlignment.Left,
                VerticalAlignment = VerticalAlignment.Top,
            };
        }

        private void LoadInHost(FrameworkElement element)
        {
            var loaded = new AutoResetEvent(false);
            RunOnUIThread.Execute(() =>
            {
                element.Loaded += (sender, args) => loaded.Set();
                Content = new Grid() { UseLayoutRounding = false, Children = { element } };
                Content.UpdateLayout();
            });
            TestUtilities.WaitForEvent(loaded);
            IdleSynchronizer.Wait();
        }

        private static void VerifyNameHasStatusPrefix(string name, string baseName, string status)
        {
            Log.Comment($"Automation name for {status}: '{name}'");
            Verify.IsTrue(name.EndsWith(baseName), $"The {status} name should end with the element's name.");
            Verify.IsTrue(name.Length > baseName.Length, $"The {status} name should include a status prefix.");
        }

        private static void VerifyState(ProgressBar progressBar, string expectedState)
        {
            var states = VisualStateHelper.GetCurrentVisualStateNameString(progressBar);
            Log.Comment($"Current visual states: {states}");
            Verify.IsTrue(VisualStateHelper.ContainsVisualState(progressBar, expectedState), $"Expected visual state {expectedState}, actual: {states}");
        }

        private static void WaitForState(ProgressBar progressBar, string expectedState)
        {
            var stopwatch = global::System.Diagnostics.Stopwatch.StartNew();
            string states = null;
            while (stopwatch.ElapsedMilliseconds < ApiTestBase.DefaultWaitTimeInMS)
            {
                bool reached = false;
                RunOnUIThread.Execute(() =>
                {
                    states = VisualStateHelper.GetCurrentVisualStateNameString(progressBar);
                    reached = VisualStateHelper.ContainsVisualState(progressBar, expectedState);
                });

                if (reached)
                {
                    Log.Comment($"Reached visual state {expectedState}");
                    return;
                }

                Thread.Sleep(50);
            }

            Verify.Fail($"Expected visual state {expectedState}, actual: {states}");
        }

        private static void VerifyTemplateSettingsAtWidth(ProgressBar progressBar, double width, double expectedDiameter, double expectedOffset)
        {
            RunOnUIThread.Execute(() =>
            {
                progressBar.Width = width;
                progressBar.UpdateLayout();
            });
            RunOnUIThread.WaitForTick();

            RunOnUIThread.Execute(() =>
            {
                Verify.AreEqual(width, progressBar.ActualWidth);
                var settings = progressBar.TemplateSettings;
                Log.Comment($"Width {width}: EllipseDiameter={settings.EllipseDiameter} EllipseOffset={settings.EllipseOffset}");
                Verify.AreEqual(expectedDiameter, settings.EllipseDiameter, $"EllipseDiameter at width {width}");
                Verify.AreEqual(expectedOffset, settings.EllipseOffset, $"EllipseOffset at width {width}");
                VerifyClose(width / 3.0, settings.EllipseAnimationEndPosition, "EllipseAnimationEndPosition");
                VerifyClose(width * 2.0 / 3.0, settings.EllipseAnimationWellPosition, "EllipseAnimationWellPosition");
                VerifyClose(width * -0.4, settings.ContainerAnimationStartPosition, "ContainerAnimationStartPosition");
                VerifyClose(width * 1.2, settings.ContainerAnimationEndPosition, "ContainerAnimationEndPosition");
                VerifyClose(width * -0.9, settings.Container2AnimationStartPosition, "Container2AnimationStartPosition");
                VerifyClose(width * 0.6 * 1.66, settings.Container2AnimationEndPosition, "Container2AnimationEndPosition");
                VerifyClose(0.0, settings.ContainerAnimationMidPosition, "ContainerAnimationMidPosition");

                // The clip excludes the padding (5, 2, 7, 3) of the 20 high bar.
                var clip = settings.ClipRect.Rect;
                VerifyClose(5.0, clip.X, "ClipRect.X");
                VerifyClose(2.0, clip.Y, "ClipRect.Y");
                VerifyClose(width - 12.0, clip.Width, "ClipRect.Width");
                VerifyClose(15.0, clip.Height, "ClipRect.Height");
            });
        }

        private static void VerifyClose(double expected, double actual, string message)
        {
            Verify.IsLessThan(Math.Abs(expected - actual), 0.01, $"{message}: expected {expected}, actual {actual}");
        }

        private static void VerifyDependencyPropertyRoundTrip<T>(DependencyObject owner, DependencyProperty property, Func<T> getClrValue, Action<T> setClrValue, T defaultValue, T newValue)
        {
            Verify.AreEqual(defaultValue, getClrValue(), "CLR default");
            Verify.AreEqual(defaultValue, (T)owner.GetValue(property), "GetValue default");

            owner.SetValue(property, newValue);
            Verify.AreEqual(newValue, getClrValue(), "A value set through the DP should be visible through the CLR property");
            owner.ClearValue(property);
            Verify.AreEqual(defaultValue, getClrValue(), "ClearValue should restore the default");

            setClrValue(newValue);
            Verify.AreEqual(newValue, (T)owner.GetValue(property), "A value set through the CLR property should be visible through the DP");
            owner.ClearValue(property);
            Verify.AreEqual(defaultValue, getClrValue(), "ClearValue should restore the default");
            Verify.AreEqual(defaultValue, (T)owner.GetValue(property), "ClearValue should restore the default");
        }
    }
}
