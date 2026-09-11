// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

using System;
using System.Collections.Generic;
using System.Text;
using Windows.UI;
using Microsoft.UI.Xaml.Controls;
using Microsoft.UI.Xaml.Markup;
using MUXControlsTestApp.Utilities;
using Microsoft.UI.Xaml.Media;
using Microsoft.UI.Xaml;
using Microsoft.UI.Xaml.Controls;
using Microsoft.UI.Xaml.Media.Imaging;
using Common;
using System.Threading;


using WEX.TestExecution;
using WEX.TestExecution.Markup;
using WEX.Logging.Interop;

using Microsoft.UI.Private.Controls;
using Microsoft.UI.Xaml.Controls;
using Microsoft.UI.Xaml.Shapes;

namespace Microsoft.UI.Xaml.Tests.MUXControls.ApiTests
{
    [TestClass]
    public class TeachingTipTests : ApiTestBase
    {
        [TestMethod]
        [TestProperty("TestPass:IncludeOnlyOn", "Desktop")]
        public void CloseDuringExpandAnimation()
        {
            VerifyCloseDuringExpandAnimation(lightDismiss: false, interruptAnimation: false);
        }

        [TestMethod]
        [TestProperty("TestPass:IncludeOnlyOn", "Desktop")]
        public void LightDismissDuringExpandAnimation()
        {
            VerifyCloseDuringExpandAnimation(lightDismiss: true, interruptAnimation: false);
        }

        [TestMethod]
        [TestProperty("TestPass:IncludeOnlyOn", "Desktop")]
        public void CloseDuringInterruptedExpandAnimation()
        {
            VerifyCloseDuringExpandAnimation(lightDismiss: false, interruptAnimation: true);
        }

        private void VerifyCloseDuringExpandAnimation(bool lightDismiss, bool interruptAnimation)
        {
            TeachingTip tip = null;
            int closingCount = 0;
            int closedCount = 0;
            var expectedReason = lightDismiss ? TeachingTipCloseReason.LightDismiss : TeachingTipCloseReason.Programmatic;

            try
            {
                RunOnUIThread.Execute(() =>
                {
                    Verify.IsTrue(new global::Windows.UI.ViewManagement.UISettings().AnimationsEnabled,
                        "This regression test requires animations to be enabled.");
                    tip = new TeachingTip { Title = "Animation regression", IsLightDismissEnabled = lightDismiss };
                    tip.Closing += (sender, args) =>
                    {
                        closingCount++;
                        Verify.AreEqual(expectedReason, args.Reason);
                    };
                    tip.Closed += (sender, args) =>
                    {
                        closedCount++;
                        Verify.AreEqual(expectedReason, args.Reason);
                    };
                    TeachingTipTestHooks.SetExpandAnimationDuration(tip, TimeSpan.FromSeconds(10));
                    TeachingTipTestHooks.SetContractAnimationDuration(tip, TimeSpan.FromMilliseconds(100));
                    Content = tip;
                    tip.IsOpen = true;
                });

                WaitForTeachingTipCondition(() =>
                    TeachingTipTestHooks.GetPopup(tip)?.IsOpen == true && TeachingTipTestHooks.GetIsExpandAnimationPlaying(tip),
                    "The popup should open while its expand animation is still running.");

                RunOnUIThread.Execute(() =>
                {
                    Verify.IsTrue(TeachingTipTestHooks.GetIsExpandAnimationPlaying(tip));
                    var popup = TeachingTipTestHooks.GetPopup(tip);
                    if (interruptAnimation)
                    {
                        // Interrupt only one animation: the expand batch must still be busy.
                        var grid = (UIElement)VisualTreeUtils.FindVisualChildByName(popup.Child, "TailOcclusionGrid");
                        Verify.IsNotNull(grid);
                        var interruptedAnimation = CompositionTarget.GetCompositorForCurrentThread().CreateVector3KeyFrameAnimation();
                        interruptedAnimation.Target = "Scale";
                        grid.StopAnimation(interruptedAnimation);
                    }

                    if (lightDismiss)
                    {
                        Microsoft.UI.Xaml.Controls.Primitives.Popup indicator = null;
                        foreach (var openPopup in VisualTreeHelper.GetOpenPopupsForXamlRoot(tip.XamlRoot))
                        {
                            if (openPopup != popup && openPopup.IsLightDismissEnabled)
                            {
                                Verify.IsNull(indicator, "There should be only one light-dismiss indicator.");
                                indicator = openPopup;
                            }
                        }
                        Verify.IsNotNull(indicator);
                        // Exercise the same Closed notification used by outside click and deactivation.
                        indicator.IsOpen = false;
                    }
                    else
                    {
                        tip.IsOpen = false;
                        tip.IsOpen = true;
                        tip.IsOpen = false;
                    }
                });

                WaitForTeachingTipCondition(() => closedCount == 1 && TeachingTipTestHooks.GetIsIdle(tip),
                    "Closing must complete without waiting for the ten-second expand animation.");
                RunOnUIThread.Execute(() =>
                {
                    Verify.IsFalse(tip.IsOpen);
                    Verify.IsFalse(TeachingTipTestHooks.GetPopup(tip).IsOpen);
                    Verify.IsNull(TeachingTipTestHooks.GetPopup(tip).Child);
                    Verify.AreEqual(1, closingCount);
                    Verify.AreEqual(1, closedCount);
                });
            }
            finally
            {
                RunOnUIThread.Execute(() => Content = null);
            }
        }

        [TestMethod]
        [TestProperty("TestPass:IncludeOnlyOn", "Desktop")]
        public void DeferredCloseDuringExpandAnimation()
        {
            TeachingTip tip = null;
            global::Windows.Foundation.Deferral deferral = null;
            TeachingTipClosingEventArgs closingArgs = null;
            int closingCount = 0;
            int closedCount = 0;
            try
            {
                RunOnUIThread.Execute(() =>
                {
                    Verify.IsTrue(new global::Windows.UI.ViewManagement.UISettings().AnimationsEnabled,
                        "This regression test requires animations to be enabled.");
                    tip = new TeachingTip { Title = "Deferred animation regression" };
                    tip.Closing += (sender, args) =>
                    {
                        closingCount++;
                        closingArgs = args;
                        deferral = args.GetDeferral();
                    };
                    tip.Closed += (sender, args) => closedCount++;
                    TeachingTipTestHooks.SetExpandAnimationDuration(tip, TimeSpan.FromSeconds(1));
                    TeachingTipTestHooks.SetContractAnimationDuration(tip, TimeSpan.FromMilliseconds(100));
                    Content = tip;
                    tip.IsOpen = true;
                });
                WaitForTeachingTipCondition(() =>
                    TeachingTipTestHooks.GetPopup(tip)?.IsOpen == true && TeachingTipTestHooks.GetIsExpandAnimationPlaying(tip),
                    "The expand animation should start.");
                RunOnUIThread.Execute(() => tip.IsOpen = false);
                WaitForTeachingTipCondition(() => deferral != null, "Closing should provide a deferral during expansion.");

                WaitForTeachingTipCondition(() => !TeachingTipTestHooks.GetIsExpandAnimationPlaying(tip),
                    "The expand batch should complete while the close is deferred.");
                RunOnUIThread.Execute(() => tip.IsOpen = true);
                WaitForTeachingTipCondition(() => !tip.IsOpen,
                    "A reopen request must be rejected while Closing is deferred.");
                RunOnUIThread.Execute(() =>
                {
                    Verify.IsFalse(TeachingTipTestHooks.GetIsIdle(tip));
                    Verify.IsTrue(TeachingTipTestHooks.GetPopup(tip).IsOpen);
                    Verify.AreEqual(1, closingCount);
                    Verify.AreEqual(0, closedCount);
                    closingArgs.Cancel = true;
                    deferral.Complete();
                    deferral = null;
                });
                WaitForTeachingTipCondition(() => tip.IsOpen && TeachingTipTestHooks.GetIsIdle(tip),
                    "Canceling the deferred close should restore the idle open state.");

                RunOnUIThread.Execute(() => tip.IsOpen = false);
                WaitForTeachingTipCondition(() => deferral != null, "A subsequent close should still work.");
                RunOnUIThread.Execute(() =>
                {
                    deferral.Complete();
                    deferral = null;
                });
                WaitForTeachingTipCondition(() => closedCount == 1 && TeachingTipTestHooks.GetIsIdle(tip),
                    "Completing the deferral should close the tip.");
                RunOnUIThread.Execute(() =>
                {
                    Verify.IsFalse(tip.IsOpen);
                    Verify.IsFalse(TeachingTipTestHooks.GetPopup(tip).IsOpen);
                    Verify.AreEqual(2, closingCount);
                });
            }
            finally
            {
                RunOnUIThread.Execute(() =>
                {
                    deferral?.Complete();
                    Content = null;
                });
            }
        }

        [TestMethod]
        [TestProperty("TestPass:IncludeOnlyOn", "Desktop")]
        public void ReopenDuringContractAnimationIsRejected()
        {
            TeachingTip tip = null;
            int closedCount = 0;
            bool closing = false;
            try
            {
                RunOnUIThread.Execute(() =>
                {
                    Verify.IsTrue(new global::Windows.UI.ViewManagement.UISettings().AnimationsEnabled,
                        "This regression test requires animations to be enabled.");
                    tip = new TeachingTip { Title = "Reopen animation regression" };
                    tip.Closing += (sender, args) => closing = true;
                    tip.Closed += (sender, args) => closedCount++;
                    TeachingTipTestHooks.SetContractAnimationDuration(tip, TimeSpan.FromSeconds(2));
                    Content = tip;
                    tip.IsOpen = true;
                });
                WaitForTeachingTipCondition(() =>
                    TeachingTipTestHooks.GetPopup(tip)?.IsOpen == true && TeachingTipTestHooks.GetIsIdle(tip),
                    "The tip should finish opening.");
                RunOnUIThread.Execute(() => tip.IsOpen = false);
                WaitForTeachingTipCondition(() => closing && !TeachingTipTestHooks.GetIsIdle(tip),
                    "The contract animation should start.");
                RunOnUIThread.Execute(() => tip.IsOpen = true);
                WaitForTeachingTipCondition(() => !tip.IsOpen, "Reopening during contraction should be rejected.");
                WaitForTeachingTipCondition(() => closedCount == 1 && TeachingTipTestHooks.GetIsIdle(tip),
                    "The original close should finish.");
                RunOnUIThread.Execute(() =>
                {
                    Verify.IsFalse(tip.IsOpen);
                    Verify.IsFalse(TeachingTipTestHooks.GetPopup(tip).IsOpen);
                });
            }
            finally
            {
                RunOnUIThread.Execute(() => Content = null);
            }
        }

        private static void WaitForTeachingTipCondition(Func<bool> condition, string message)
        {
            using (var completed = new ManualResetEvent(false))
            {
                EventHandler<object> rendering = (sender, args) =>
                {
                    if (condition())
                    {
                        completed.Set();
                    }
                };
                try
                {
                    RunOnUIThread.Execute(() => CompositionTarget.Rendering += rendering);
                    Verify.IsTrue(completed.WaitOne(TimeSpan.FromSeconds(5)), message);
                }
                finally
                {
                    RunOnUIThread.Execute(() => CompositionTarget.Rendering -= rendering);
                }
            }
        }

        [TestMethod]
        [TestProperty("TestPass:IncludeOnlyOn", "Desktop")] // TeachingTip doesn't appear to show up correctly in OneCore.
        public void TeachingTipBackgroundTest()
        {
            TeachingTip teachingTip = null, teachingTipLightDismiss = null;
            SolidColorBrush blueBrush = null;
            Brush lightDismissBackgroundBrush = null;
            var loadedEvent = new AutoResetEvent(false);
            RunOnUIThread.Execute(() =>
            {
                Grid root = new Grid();
                teachingTip = new TeachingTip();
                teachingTip.Loaded += (object sender, RoutedEventArgs args) => { loadedEvent.Set(); };

                teachingTipLightDismiss = new TeachingTip();
                teachingTipLightDismiss.IsLightDismissEnabled = true;

                // Set LightDismiss background before show... it shouldn't take effect in the tree
                blueBrush = new SolidColorBrush(Microsoft.UI.Colors.Blue);
                teachingTipLightDismiss.Background = blueBrush;

                root.Resources.Add("TeachingTip", teachingTip);
                root.Resources.Add("TeachingTipLightDismiss", teachingTipLightDismiss);

                lightDismissBackgroundBrush = MUXControlsTestApp.App.Current.Resources["TeachingTipTransientBackground"] as Brush;
                Verify.IsNotNull(lightDismissBackgroundBrush, "lightDismissBackgroundBrush");

                teachingTip.IsOpen = true;
                teachingTipLightDismiss.IsOpen = true;

                MUXControlsTestApp.App.TestContentRoot = root;
            });

            IdleSynchronizer.Wait();
            loadedEvent.WaitOne();
            IdleSynchronizer.Wait();

            RunOnUIThread.Execute(() =>
            {
                var redBrush = new SolidColorBrush(Microsoft.UI.Colors.Red);
                teachingTip.SetValue(TeachingTip.BackgroundProperty, redBrush);
                Verify.AreSame(redBrush, teachingTip.GetValue(TeachingTip.BackgroundProperty) as Brush);
                Verify.AreSame(redBrush, teachingTip.Background);

                teachingTip.Background = blueBrush;
                Verify.AreSame(blueBrush, teachingTip.Background);

                {
                    var popup = TeachingTipTestHooks.GetPopup(teachingTip);
                    var rootGrid = popup.Child;
                    var tailOcclusionGrid = VisualTreeHelper.GetChild(rootGrid, 0);
                    var contentRootGrid = VisualTreeHelper.GetChild(tailOcclusionGrid, 0);
                    Verify.AreSame(blueBrush, ((Grid)contentRootGrid).Background, "Checking TeachingTip.Background TemplateBinding works");
                }

                {
                    var popup = TeachingTipTestHooks.GetPopup(teachingTipLightDismiss);
                    var child = popup.Child as Grid;

                    Log.Comment("Checking LightDismiss TeachingTip Background is using resource for first invocation");

                    Polygon tailPolygon = VisualTreeUtils.FindVisualChildByName(child, "TailPolygon") as Polygon;
                    Grid contentRootGrid = VisualTreeUtils.FindVisualChildByName(child, "ContentRootGrid") as Grid;
                    ContentPresenter mainContentPresenter = VisualTreeUtils.FindVisualChildByName(child, "MainContentPresenter") as ContentPresenter;
                    Border heroContentBorder = VisualTreeUtils.FindVisualChildByName(child, "HeroContentBorder") as Border;

                    VerifyLightDismissTipBackground(tailPolygon.Fill, "TailPolygon");
                    VerifyLightDismissTipBackground(contentRootGrid.Background, "ContentRootGrid");
                    VerifyLightDismissTipBackground(mainContentPresenter.Background, "MainContentPresenter");
                    VerifyLightDismissTipBackground(heroContentBorder.Background, "HeroContentBorder");

                    void VerifyLightDismissTipBackground(Brush brush, string uiPart)
                    {
                        if (lightDismissBackgroundBrush != brush)
                        {
                            if (brush is SolidColorBrush actualSolidBrush)
                            {
                                string teachingTipMessage = $"LightDismiss TeachingTip's {uiPart} Background is SolidColorBrush with color {actualSolidBrush.Color}";
                                Log.Comment(teachingTipMessage);
                                Verify.Fail(teachingTipMessage);
                            }
                            else
                            {
                                Verify.AreSame(lightDismissBackgroundBrush, brush, $"Checking LightDismiss TeachingTip's {uiPart} Background is using resource for first invocation");
                            }
                        }
                    }
                }

                teachingTip.IsLightDismissEnabled = true;
            });

            IdleSynchronizer.Wait();

            RunOnUIThread.Execute(() =>
            {
                Verify.AreEqual(blueBrush.Color, ((SolidColorBrush)teachingTip.Background).Color);

                var popup = TeachingTipTestHooks.GetPopup(teachingTip);
                var child = popup.Child as Grid;

                Polygon tailPolygon = VisualTreeUtils.FindVisualChildByName(child, "TailPolygon") as Polygon;
                Grid contentRootGrid = VisualTreeUtils.FindVisualChildByName(child, "ContentRootGrid") as Grid;
                ContentPresenter mainContentPresenter = VisualTreeUtils.FindVisualChildByName(child, "MainContentPresenter") as ContentPresenter;
                Border heroContentBorder = VisualTreeUtils.FindVisualChildByName(child, "HeroContentBorder") as Border;

                VerifyBackgroundChanged(tailPolygon.Fill, "TailPolygon");
                VerifyBackgroundChanged(contentRootGrid.Background, "ContentRootGrid");
                VerifyBackgroundChanged(mainContentPresenter.Background, "MainContentPresenter");
                VerifyBackgroundChanged(heroContentBorder.Background, "HeroContentBorder");

                void VerifyBackgroundChanged(Brush brush, string uiPart)
                {
                    // If we can no longer cast the background brush to a solid color brush then changing the
                    // IsLightDismissEnabled has changed the background as we expected it to.
                    if (brush is SolidColorBrush solidColorBrush)
                    {
                        Verify.AreNotEqual(blueBrush.Color, solidColorBrush.Color, $"TeachingTip's {uiPart} Background should have changed");
                    }
                }
            });
        }

        [TestMethod]
        public void TeachingTipWithContentAndWithoutHeroContentDoesNotCrash()
        {
            var loadedEvent = new AutoResetEvent(false);
            RunOnUIThread.Execute(() =>
            {
                Grid contentGrid = new Grid();
                SymbolIconSource iconSource = new SymbolIconSource();
                iconSource.Symbol = Symbol.People;
                TeachingTip teachingTip = new TeachingTip();
                teachingTip.Content = contentGrid;
                teachingTip.IconSource = (IconSource)iconSource;
                teachingTip.Loaded += (object sender, RoutedEventArgs args) => { loadedEvent.Set(); };
                Content = teachingTip;
            });

            IdleSynchronizer.Wait();
            loadedEvent.WaitOne();
        }

        [TestMethod]
        public void TeachingTipWithContentAndWithoutIconSourceDoesNotCrash()
        {
            var loadedEvent = new AutoResetEvent(false);
            RunOnUIThread.Execute(() =>
            {
                Grid contentGrid = new Grid();
                Grid heroGrid = new Grid();
                TeachingTip teachingTip = new TeachingTip();
                teachingTip.Content = contentGrid;
                teachingTip.HeroContent = heroGrid;
                teachingTip.Loaded += (object sender, RoutedEventArgs args) => { loadedEvent.Set(); };
                Content = teachingTip;
            });

            IdleSynchronizer.Wait();
            loadedEvent.WaitOne();
        }

        [TestMethod]
        public void TeachingTipWithClearedTemplateSettingsDoesNotCrash()
        {
            // Regression test for AB#60057581: clearing the public TemplateSettingsProperty left
            // OnIconSourceChanged and UpdateSizeBasedTemplateSettings dereferencing a null peer.
            var loadedEvent = new AutoResetEvent(false);
            TeachingTip teachingTip = null;
            Border content = null;

            RunOnUIThread.Execute(() =>
            {
                content = new Border { Width = 200, Height = 100 };
                teachingTip = new TeachingTip();
                teachingTip.Content = content;
                teachingTip.IconSource = new SymbolIconSource { Symbol = Symbol.People };
                teachingTip.Loaded += (object sender, RoutedEventArgs args) => { loadedEvent.Set(); };
                Content = teachingTip;
            });

            IdleSynchronizer.Wait();
            loadedEvent.WaitOne();

            RunOnUIThread.Execute(() =>
            {
                teachingTip.ClearValue(TeachingTip.TemplateSettingsProperty);
                Verify.IsNull(teachingTip.TemplateSettings, "TemplateSettings should be null after ClearValue");

                // OnIconSourceChanged, reached directly through the property-changed callback.
                teachingTip.IconSource = new SymbolIconSource { Symbol = Symbol.Accept };
            });

            IdleSynchronizer.Wait();

            RunOnUIThread.Execute(() =>
            {
                // OnApplyTemplate -> OnIconSourceChanged, reached from the measure pass.
                teachingTip.IsOpen = true;
            });

            IdleSynchronizer.Wait();

            RunOnUIThread.Execute(() =>
            {
                // OnContentSizeChanged -> UpdateSizeBasedTemplateSettings.
                content.Width = 420;
                content.Height = 260;
            });

            IdleSynchronizer.Wait();

            RunOnUIThread.Execute(() =>
            {
                teachingTip.IsOpen = false;
            });

            IdleSynchronizer.Wait();
        }

        [TestMethod]
        public void PropagatePropertiesDown()
        {
            TextBlock content = null;
            TeachingTip tip = null;
            RunOnUIThread.Execute(() =>
            {
                content = new TextBlock() {
                    Text = "Some text"
                };

                tip = new TeachingTip() {
                    Content = content,
                    FontSize = 22,
                    Foreground = new SolidColorBrush() {
                        Color = Colors.Red
                    }
                };

                Content = tip;
                Content.UpdateLayout();
                tip.IsOpen = true;
                Content.UpdateLayout();
            });

            IdleSynchronizer.Wait();

            RunOnUIThread.Execute(() =>
            {
                Verify.IsTrue(Math.Abs(22 - content.FontSize) < 1);
                var foregroundBrush = content.Foreground as SolidColorBrush;
                Verify.AreEqual(Colors.Red, foregroundBrush.Color);
            });
        }

        [TestMethod]
        public void VerifySubTitleBlockVisibilityOnInitialUnset()
        {
            TeachingTip teachingTip = null;
            RunOnUIThread.Execute(() =>
            {
                teachingTip = new TeachingTip();
                teachingTip.IsOpen = true;
                Content = teachingTip;
            });

            IdleSynchronizer.Wait();

            RunOnUIThread.Execute(() =>
            {
                Verify.AreEqual("", teachingTip.Title);
                Verify.AreEqual(Visibility.Collapsed,
                    TeachingTipTestHooks.GetTitleVisibility(teachingTip));
                Verify.AreEqual("", teachingTip.Subtitle);
                Verify.AreEqual(Visibility.Collapsed,
                    TeachingTipTestHooks.GetSubtitleVisibility(teachingTip));
            });
        }

        [TestMethod]
        public void TeachingTipHeroContentPlacementTest()
        {
            RunOnUIThread.Execute(() =>
            {
                foreach (var iPlacementMode in Enum.GetValues(typeof(TeachingTipHeroContentPlacementMode)))
                {
                    var placementMode = (TeachingTipHeroContentPlacementMode)iPlacementMode;

                    Log.Comment($"Verifying TeachingTipHeroContentPlacementMode [{placementMode}]");

                    TeachingTip teachingTip = new TeachingTip();
                    teachingTip.HeroContentPlacement = placementMode;

                    // Open the teaching tip to enter the correct visual state for the HeroContentPlacement.
                    teachingTip.IsOpen = true;

                    Content = teachingTip;
                    Content.UpdateLayout();

                    Verify.IsTrue(teachingTip.HeroContentPlacement == placementMode, $"HeroContentPlacement should have been [{placementMode}]");
                    
                    var root = VisualTreeUtils.FindVisualChildByName(teachingTip, "Container") as FrameworkElement;

                    switch (placementMode)
                    {
                        case TeachingTipHeroContentPlacementMode.Auto:
                            Verify.IsTrue(IsVisualStateActive(root, "HeroContentPlacementStates", "HeroContentTop"),
                                "The [HeroContentTop] visual state should have been active");
                            break;
                        case TeachingTipHeroContentPlacementMode.Top:
                            Verify.IsTrue(IsVisualStateActive(root, "HeroContentPlacementStates", "HeroContentTop"), 
                                "The [HeroContentTop] visual state should have been active");
                            break;
                        case TeachingTipHeroContentPlacementMode.Bottom:
                            Verify.IsTrue(IsVisualStateActive(root, "HeroContentPlacementStates", "HeroContentBottom"),
                                "The [HeroContentBottom] visual state should have been active");
                            break;
                    }
                }
            });

            bool IsVisualStateActive(FrameworkElement root, string groupName, string stateName)
            {
                foreach (var group in VisualStateManager.GetVisualStateGroups(root))
                {
                    if (group.Name == groupName)
                    {
                        return group.CurrentState != null && group.CurrentState.Name == stateName;
                    }
                }

                return false;
            }
        }
    }
}
