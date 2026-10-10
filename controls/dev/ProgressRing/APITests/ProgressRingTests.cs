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
using WEX.TestExecution;
using WEX.TestExecution.Markup;
using WEX.Logging.Interop;

namespace Microsoft.UI.Xaml.Tests.MUXControls.ApiTests
{
    [TestClass]
    public class ProgressRingTests : ApiTestBase
    {
        // Scenario: toggle IsActive on an unparented ProgressRing and read its AccessibilityView.
        // Expected: an active ring is not Raw; an inactive ring is Raw.
        // A failure means: screen readers would announce an inactive ring, or skip an active one.
        [TestMethod]
        public void VerifyAccessibilityView()
        {
            RunOnUIThread.Execute(() =>
            {
                var progressRing = new ProgressRing();
                progressRing.IsActive = true;

                Verify.AreNotEqual(AccessibilityView.Raw, AutomationProperties.GetAccessibilityView(progressRing));

                progressRing.IsActive = false;
                Verify.AreEqual(AccessibilityView.Raw, AutomationProperties.GetAccessibilityView(progressRing));
            });
        }

        // Scenario: set Value, Minimum and Maximum inside and outside the range of an unparented determinate ring.
        // Expected: defaults are Value 0 and range 0-100; Value clamps into range (150 to 100, -5 to 0) but NaN is
        //           kept; Minimum 200 raises Maximum and Value to 200; Maximum 50 lowers Minimum and Value to 50;
        //           in-range values are kept.
        // A failure means: the ring could hold an inconsistent range or out-of-range value and show wrong progress.
        [TestMethod]
        public void ValueMinimumAndMaximumAreCoercedIntoRange()
        {
            RunOnUIThread.Execute(() =>
            {
                var progressRing = new ProgressRing() { IsIndeterminate = false };
                Verify.AreEqual(0.0, progressRing.Minimum);
                Verify.AreEqual(100.0, progressRing.Maximum);
                Verify.AreEqual(0.0, progressRing.Value);

                progressRing.Value = 40;
                Verify.AreEqual(40.0, progressRing.Value, "An in-range value is kept.");

                progressRing.SetValue(ProgressRing.ValueProperty, 150.0);
                Verify.AreEqual(100.0, progressRing.Value, "A value above Maximum is coerced to Maximum.");

                progressRing.Value = -5;
                Verify.AreEqual(0.0, progressRing.Value, "A value below Minimum is coerced to Minimum.");

                progressRing.Value = 100;
                Verify.AreEqual(100.0, progressRing.Value, "Maximum itself is in range.");

                progressRing.Value = double.NaN;
                Verify.IsTrue(double.IsNaN(progressRing.Value), "NaN is not coerced.");

                progressRing.Value = 40;
                progressRing.SetValue(ProgressRing.MinimumProperty, 200.0);
                Verify.AreEqual(200.0, progressRing.Minimum);
                Verify.AreEqual(200.0, progressRing.Maximum, "Maximum is raised to a larger Minimum.");
                Verify.AreEqual(200.0, progressRing.Value, "Value is raised to the new Minimum.");

                progressRing.SetValue(ProgressRing.MaximumProperty, 50.0);
                Verify.AreEqual(50.0, progressRing.Maximum);
                Verify.AreEqual(50.0, progressRing.Minimum, "Minimum is lowered to a smaller Maximum.");
                Verify.AreEqual(50.0, progressRing.Value, "Value is lowered to the new Maximum.");

                progressRing.Minimum = 10;
                progressRing.Value = 30;
                Verify.AreEqual(10.0, progressRing.Minimum);
                Verify.AreEqual(50.0, progressRing.Maximum);
                Verify.AreEqual(30.0, progressRing.Value);
            });
        }

        // Scenario: an inactive ring is resized to 40, 41 and 100 square, then to 0 wide with MinWidth cleared.
        // Expected: EllipseDiameter and EllipseOffset.Top are 5/15 at 40 (one extra pixel), 4.1/16.4 at 41 and 10/40 at
        //           100, with the other offset sides 0 and MaxSideLength equal to the size; at width 0 they are all 0.
        // A failure means: the ring's template ellipse would be sized or positioned wrong.
        [TestMethod]
        public void TemplateSettingsFollowSize()
        {
            ProgressRing progressRing = null;

            RunOnUIThread.Execute(() =>
            {
                progressRing = new ProgressRing()
                {
                    IsActive = false,
                    Width = 40,
                    Height = 40,
                    UseLayoutRounding = false,
                };
            });

            LoadInHost(progressRing);

            // Rings up to 40 wide get one extra pixel of ellipse diameter.
            VerifyTemplateSettingsAtSize(progressRing, 40, expectedDiameter: 5.0, expectedOffsetTop: 15.0);
            VerifyTemplateSettingsAtSize(progressRing, 41, expectedDiameter: 4.1, expectedOffsetTop: 16.4);
            VerifyTemplateSettingsAtSize(progressRing, 100, expectedDiameter: 10.0, expectedOffsetTop: 40.0);

            // A ring that collapses to zero width has no ellipse. The style's MinWidth is cleared so the width can reach 0.
            RunOnUIThread.Execute(() =>
            {
                progressRing.MinWidth = 0;
                progressRing.Width = 0;
                progressRing.UpdateLayout();
            });
            RunOnUIThread.WaitForTick();

            RunOnUIThread.Execute(() =>
            {
                Verify.AreEqual(0.0, progressRing.ActualWidth);
                var settings = progressRing.TemplateSettings;
                Log.Comment($"Width 0: EllipseDiameter={settings.EllipseDiameter} EllipseOffset={settings.EllipseOffset} MaxSideLength={settings.MaxSideLength}");
                Verify.AreEqual(0.0, settings.EllipseDiameter, "EllipseDiameter at width 0");
                Verify.AreEqual(0.0, settings.MaxSideLength, "MaxSideLength at width 0");
                Verify.AreEqual(new Thickness(0), settings.EllipseOffset, "EllipseOffset at width 0");
            });
        }

        // Scenario: compare GetValue/SetValue/ClearValue with the CLR property for every public DependencyProperty of
        //           ProgressRing, on an unparented ring.
        // Expected: identifiers are non-null, distinct and stable; defaults match; values set through either surface
        //           are visible through the other; ClearValue restores the default; range values stay inside the
        //           default range so coercion does not interfere; TemplateSettings is a stable non-null instance.
        // A failure means: XAML, bindings or styles using the DP identifiers would disagree with the CLR properties.
        [TestMethod]
        public void DependencyPropertyIdentifiersAndClrPropertiesAgree()
        {
            RunOnUIThread.Execute(() =>
            {
                var properties = new DependencyProperty[]
                {
                    ProgressRing.IsActiveProperty,
                    ProgressRing.IsIndeterminateProperty,
                    ProgressRing.DeterminateSourceProperty,
                    ProgressRing.IndeterminateSourceProperty,
                    ProgressRing.ValueProperty,
                    ProgressRing.MinimumProperty,
                    ProgressRing.MaximumProperty,
                };
                for (int i = 0; i < properties.Length; i++)
                {
                    Verify.IsNotNull(properties[i], "Dependency property identifier #" + i + " should not be null");
                    for (int j = 0; j < i; j++)
                    {
                        Verify.IsFalse(ReferenceEquals(properties[i], properties[j]), "Dependency property identifiers #" + j + " and #" + i + " should be distinct");
                    }
                }
                Verify.IsTrue(ReferenceEquals(ProgressRing.IsActiveProperty, ProgressRing.IsActiveProperty), "Identifiers are stable.");
                Verify.IsTrue(ReferenceEquals(ProgressRing.DeterminateSourceProperty, ProgressRing.DeterminateSourceProperty), "Identifiers are stable.");

                var progressRing = new ProgressRing();
                var source = new global::AnimatedVisuals.ProgressRingDeterminate();

                VerifyDependencyPropertyRoundTrip(progressRing, ProgressRing.IsActiveProperty, () => progressRing.IsActive, v => progressRing.IsActive = v, true, false);
                VerifyDependencyPropertyRoundTrip(progressRing, ProgressRing.IsIndeterminateProperty, () => progressRing.IsIndeterminate, v => progressRing.IsIndeterminate = v, true, false);
                VerifyDependencyPropertyRoundTrip<IAnimatedVisualSource>(progressRing, ProgressRing.DeterminateSourceProperty, () => progressRing.DeterminateSource, v => progressRing.DeterminateSource = v, null, source);
                VerifyDependencyPropertyRoundTrip<IAnimatedVisualSource>(progressRing, ProgressRing.IndeterminateSourceProperty, () => progressRing.IndeterminateSource, v => progressRing.IndeterminateSource = v, null, source);
                VerifyDependencyPropertyRoundTrip(progressRing, ProgressRing.ValueProperty, () => progressRing.Value, v => progressRing.Value = v, 0.0, 40.0);
                VerifyDependencyPropertyRoundTrip(progressRing, ProgressRing.MinimumProperty, () => progressRing.Minimum, v => progressRing.Minimum = v, 0.0, -10.0);
                VerifyDependencyPropertyRoundTrip(progressRing, ProgressRing.MaximumProperty, () => progressRing.Maximum, v => progressRing.Maximum = v, 100.0, 50.0);

                // None of the round trips above changed the other range properties.
                Verify.AreEqual(0.0, progressRing.Value);
                Verify.AreEqual(0.0, progressRing.Minimum);
                Verify.AreEqual(100.0, progressRing.Maximum);

                var templateSettings = progressRing.TemplateSettings;
                Verify.IsNotNull(templateSettings, "TemplateSettings is created with the ring.");
                Verify.IsTrue(ReferenceEquals(templateSettings, progressRing.TemplateSettings), "TemplateSettings returns the same instance.");
            });
        }

        // Scenario: an active determinate ring using the built-in determinate animation gets new Foreground/Background
        //           brushes and brush colors.
        // Expected: smoke only: no exception, same built-in source, still loaded and determinate. Colors are not
        //           observable through the API, and a suspected product bug means a determinate ring ignores
        //           Foreground/Background.
        // A failure means: recoloring a determinate ring would throw, or replace or unload its animation.
        [TestMethod]
        public void ColorChangesOnADeterminateRingKeepTheBuiltInSource()
        {
            ProgressRing progressRing = null;

            RunOnUIThread.Execute(() =>
            {
                progressRing = new ProgressRing() { IsActive = true, IsIndeterminate = false, Value = 50 };
            });

            LoadInHost(progressRing);

            RunOnUIThread.Execute(() =>
            {
                var player = GetLottiePlayer(progressRing);
                var builtInSource = player.Source;
                Verify.IsNotNull(builtInSource, "A determinate ring without DeterminateSource uses a built-in animation.");
                Verify.IsNull(progressRing.DeterminateSource);
                Verify.IsTrue(player.IsAnimatedVisualLoaded);

                var foreground = new SolidColorBrush(Colors.Red);
                progressRing.Foreground = foreground;
                foreground.Color = Colors.Blue;
                var background = new SolidColorBrush(Colors.Green);
                progressRing.Background = background;
                background.Color = Colors.Yellow;
                progressRing.Foreground = new LinearGradientBrush();
                progressRing.Background = new LinearGradientBrush();

                Verify.AreSame(builtInSource, player.Source, "Color changes must not replace the built-in determinate animation.");
                Verify.IsFalse(progressRing.IsIndeterminate);
                Verify.IsTrue(player.IsAnimatedVisualLoaded);
            });
        }

        // Scenario: a ring named "Ring" (range 10-20, value 15) toggles IsActive and IsIndeterminate while its peer is
        //           read; SetValue(250) is called on the provider.
        // Expected: class name ProgressRing, control type ProgressBar, a localized type; the name is "Ring" except
        //           active indeterminate (busy prefix); no RangeValue while indeterminate; determinate RangeValue is
        //           10/20/15 with NaN Small/LargeChange, and SetValue keeps Value in range.
        // A failure means: screen readers would announce the wrong role, busy state or progress for the ring.
        [TestMethod]
        public void AutomationPeerReflectsActiveAndIndeterminateState()
        {
            ProgressRing progressRing = null;

            RunOnUIThread.Execute(() =>
            {
                progressRing = new ProgressRing()
                {
                    IsActive = false,
                    IsIndeterminate = true,
                    Minimum = 10,
                    Maximum = 20,
                    Value = 15,
                };
                AutomationProperties.SetName(progressRing, "Ring");
            });

            LoadInHost(progressRing);

            RunOnUIThread.Execute(() =>
            {
                var peer = FrameworkElementAutomationPeer.CreatePeerForElement(progressRing);
                Verify.IsNotNull(peer);
                Verify.AreEqual("Microsoft.UI.Xaml.Controls.ProgressRing", peer.GetClassName());
                Verify.AreEqual(AutomationControlType.ProgressBar, peer.GetAutomationControlType());
                Verify.IsFalse(string.IsNullOrEmpty(peer.GetLocalizedControlType()), "ProgressRing provides a localized control type.");

                Verify.AreEqual("Ring", peer.GetName(), "An inactive ring reports only its name.");
                Verify.IsNull(peer.GetPattern(PatternInterface.RangeValue), "An indeterminate ring has no RangeValue pattern.");

                progressRing.IsActive = true;
                var busyName = peer.GetName();
                Log.Comment($"Active indeterminate name: '{busyName}'");
                Verify.IsTrue(busyName.EndsWith(" Ring"), "The busy status is separated from the name by a space.");
                Verify.IsTrue(busyName.Length > " Ring".Length, "An active indeterminate ring reports a busy status.");
                Verify.IsNull(peer.GetPattern(PatternInterface.RangeValue));

                progressRing.IsIndeterminate = false;
                Verify.AreEqual("Ring", peer.GetName(), "An active determinate ring reports only its name.");
                var provider = peer.GetPattern(PatternInterface.RangeValue) as IRangeValueProvider;
                Verify.IsNotNull(provider, "A determinate ring exposes the RangeValue pattern.");
                Verify.AreEqual(10.0, provider.Minimum);
                Verify.AreEqual(20.0, provider.Maximum);
                Verify.AreEqual(15.0, provider.Value);
                Verify.IsTrue(double.IsNaN(provider.SmallChange), "ProgressRing has no small change.");
                Verify.IsTrue(double.IsNaN(provider.LargeChange), "ProgressRing has no large change.");

                // SetValue is exercised only for behavior that is not in question: the value stays within range
                // and the provider keeps reporting the control's actual value.
                provider.SetValue(250);
                Verify.IsTrue(progressRing.Value >= progressRing.Minimum && progressRing.Value <= progressRing.Maximum, "Value stays within range.");
                Verify.AreEqual(progressRing.Value, provider.Value);

                progressRing.IsActive = false;
                Verify.AreEqual("Ring", peer.GetName());

                var explicitPeer = new ProgressRingAutomationPeer(progressRing);
                Verify.AreEqual("Microsoft.UI.Xaml.Controls.ProgressRing", explicitPeer.GetClassName());
            });
        }

        // Scenario: three rings use custom IndeterminateSource/DeterminateSource that are cleared and set again; an
        //           active ring with both toggles IsIndeterminate. Brush changes are smoke only.
        // Expected: the LottiePlayer uses the custom source for the current state; clearing one falls back to a
        //           non-null built-in source and setting it again restores it; with DeterminateSource cleared the
        //           active ring uses a built-in one.
        // A failure means: custom ring animations would not show, or the wrong animation would show after a change.
        [TestMethod]
        public void CustomSourcesAreSelectedByStateAndClearingRestoresDefaults()
        {
            ProgressRing indeterminateRing = null;
            ProgressRing determinateRing = null;
            ProgressRing switchingRing = null;
            IAnimatedVisualSource customIndeterminate = null;
            IAnimatedVisualSource customDeterminate = null;

            RunOnUIThread.Execute(() =>
            {
                customIndeterminate = new global::AnimatedVisuals.ProgressRingDeterminate();
                customDeterminate = new global::AnimatedVisuals.ProgressRingDeterminate();

                indeterminateRing = new ProgressRing() { IsActive = false, IsIndeterminate = true, IndeterminateSource = customIndeterminate };
                determinateRing = new ProgressRing() { IsActive = false, IsIndeterminate = false, DeterminateSource = customDeterminate };
                switchingRing = new ProgressRing() { IsActive = false, IsIndeterminate = true, IndeterminateSource = customIndeterminate, DeterminateSource = customDeterminate };
            });

            LoadInHost(() => new StackPanel() { Children = { indeterminateRing, determinateRing, switchingRing } }, indeterminateRing);

            RunOnUIThread.Execute(() =>
            {
                var indeterminatePlayer = GetLottiePlayer(indeterminateRing);
                var determinatePlayer = GetLottiePlayer(determinateRing);
                var switchingPlayer = GetLottiePlayer(switchingRing);

                Verify.AreSame(customIndeterminate, indeterminatePlayer.Source, "A custom IndeterminateSource is used while indeterminate.");
                Verify.AreSame(customDeterminate, determinatePlayer.Source, "A custom DeterminateSource is used while determinate.");

                // Smoke only (must not throw): brush and brush-color changes run the ProgressRing color handlers.
                // The Lottie theme colors they write are not observable from the API, so nothing is asserted here.
                var foreground = new SolidColorBrush(Colors.Red);
                indeterminateRing.Foreground = foreground;
                foreground.Color = Colors.Blue;
                var background = new SolidColorBrush(Colors.Green);
                indeterminateRing.Background = background;
                background.Color = Colors.Yellow;

                // Clearing the custom sources falls back to the built-in animations.
                indeterminateRing.IndeterminateSource = null;
                var defaultIndeterminate = indeterminatePlayer.Source;
                Verify.IsNotNull(defaultIndeterminate, "Clearing IndeterminateSource restores the default animation.");
                Verify.AreNotSame(customIndeterminate, defaultIndeterminate);

                determinateRing.DeterminateSource = null;
                var defaultDeterminate = determinatePlayer.Source;
                Verify.IsNotNull(defaultDeterminate, "Clearing DeterminateSource restores the default animation.");
                Verify.AreNotSame(customDeterminate, defaultDeterminate);

                // Smoke only (must not throw): recolor the default animations, including a non-SolidColorBrush
                // (theme color fallback).
                foreground.Color = Colors.Purple;
                indeterminateRing.Foreground = new LinearGradientBrush();
                indeterminateRing.Background = new LinearGradientBrush();

                // Setting a custom source again replaces the default.
                indeterminateRing.IndeterminateSource = customIndeterminate;
                Verify.AreSame(customIndeterminate, indeterminatePlayer.Source);

                // While active, IsIndeterminate selects which custom source the player uses.
                switchingRing.IsActive = true;
                Verify.AreSame(customIndeterminate, switchingPlayer.Source, "Active indeterminate uses IndeterminateSource.");
                switchingRing.IsIndeterminate = false;
                Verify.AreSame(customDeterminate, switchingPlayer.Source, "Active determinate uses DeterminateSource.");
                switchingRing.IsIndeterminate = true;
                Verify.AreSame(customIndeterminate, switchingPlayer.Source, "Switching back uses IndeterminateSource again.");

                switchingRing.DeterminateSource = null;
                switchingRing.IsIndeterminate = false;
                var switchedDefault = switchingPlayer.Source;
                Verify.IsNotNull(switchedDefault, "Without a DeterminateSource the default determinate animation is used.");
                Verify.AreNotSame(customDeterminate, switchedDefault);
                Verify.AreNotSame(customIndeterminate, switchedDefault);

                switchingRing.IsActive = false;
            });
        }

        private void LoadInHost(FrameworkElement element)
        {
            LoadInHost(() => element, element);
        }

        private void LoadInHost(Func<FrameworkElement> createRoot, FrameworkElement elementToWaitFor)
        {
            var loaded = new AutoResetEvent(false);
            RunOnUIThread.Execute(() =>
            {
                elementToWaitFor.Loaded += (sender, args) => loaded.Set();
                var root = createRoot();
                Content = root as Panel ?? new Grid() { Children = { root } };
                Content.UpdateLayout();
            });
            TestUtilities.WaitForEvent(loaded);
            IdleSynchronizer.Wait();
        }

        private static AnimatedVisualPlayer GetLottiePlayer(ProgressRing progressRing)
        {
            var player = progressRing.FindVisualChildByName("LottiePlayer") as AnimatedVisualPlayer;
            Verify.IsNotNull(player, "The ProgressRing template should contain the LottiePlayer.");
            return player;
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

        private static void VerifyTemplateSettingsAtSize(ProgressRing progressRing, double size, double expectedDiameter, double expectedOffsetTop)
        {
            RunOnUIThread.Execute(() =>
            {
                progressRing.Width = size;
                progressRing.Height = size;
                progressRing.UpdateLayout();
            });
            RunOnUIThread.WaitForTick();

            RunOnUIThread.Execute(() =>
            {
                Verify.AreEqual(size, progressRing.ActualWidth);
                var settings = progressRing.TemplateSettings;
                Log.Comment($"Size {size}: EllipseDiameter={settings.EllipseDiameter} EllipseOffset={settings.EllipseOffset} MaxSideLength={settings.MaxSideLength}");
                Verify.IsLessThan(Math.Abs(expectedDiameter - settings.EllipseDiameter), 0.001, $"EllipseDiameter at size {size}");
                Verify.IsLessThan(Math.Abs(expectedOffsetTop - settings.EllipseOffset.Top), 0.001, $"EllipseOffset.Top at size {size}");
                Verify.AreEqual(0.0, settings.EllipseOffset.Left);
                Verify.AreEqual(0.0, settings.EllipseOffset.Right);
                Verify.AreEqual(0.0, settings.EllipseOffset.Bottom);
                Verify.AreEqual(size, settings.MaxSideLength, $"MaxSideLength at size {size}");
            });
        }
    }
}
