// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

using System;
using System.Collections.Generic;
using System.Diagnostics;
using System.Linq;
using System.Text;
using System.Windows.Input;
using Windows.Foundation;
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
using Microsoft.UI.Xaml.Automation;
using Microsoft.UI.Xaml.Automation.Peers;
using Microsoft.UI.Xaml.Automation.Provider;
using Microsoft.UI.Xaml.Controls;
using Microsoft.UI.Xaml.Shapes;

namespace Microsoft.UI.Xaml.Tests.MUXControls.ApiTests
{
    [TestClass]
    public class TeachingTipTests : ApiTestBase
    {
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
            TeachingTip teachingTip = null;
            RunOnUIThread.Execute(() =>
            {
                Grid contentGrid = new Grid();
                SymbolIconSource iconSource = new SymbolIconSource();
                iconSource.Symbol = Symbol.People;
                teachingTip = new TeachingTip();
                teachingTip.Content = contentGrid;
                teachingTip.IconSource = (IconSource)iconSource;
                teachingTip.Loaded += (object sender, RoutedEventArgs args) => { loadedEvent.Set(); };
                Content = teachingTip;
            });

            IdleSynchronizer.Wait();
            Verify.IsTrue(loadedEvent.WaitOne(DefaultWaitTimeInMS), "TeachingTip should load");

            RunOnUIThread.Execute(() =>
            {
                // The icon must still be presented when there is no hero content.
                Verify.AreEqual("Icon", GetCurrentStateName(teachingTip, "IconStates"));
                Verify.IsNotNull(teachingTip.TemplateSettings.IconElement, "IconElement should be created from IconSource");
            });
        }

        [TestMethod]
        public void TeachingTipWithContentAndWithoutIconSourceDoesNotCrash()
        {
            var loadedEvent = new AutoResetEvent(false);
            TeachingTip teachingTip = null;
            RunOnUIThread.Execute(() =>
            {
                Grid contentGrid = new Grid();
                Grid heroGrid = new Grid();
                teachingTip = new TeachingTip();
                teachingTip.Content = contentGrid;
                teachingTip.HeroContent = heroGrid;
                teachingTip.Loaded += (object sender, RoutedEventArgs args) => { loadedEvent.Set(); };
                Content = teachingTip;
            });

            IdleSynchronizer.Wait();
            Verify.IsTrue(loadedEvent.WaitOne(DefaultWaitTimeInMS), "TeachingTip should load");

            RunOnUIThread.Execute(() =>
            {
                Verify.AreEqual("NoIcon", GetCurrentStateName(teachingTip, "IconStates"));
                Verify.IsNull(teachingTip.TemplateSettings.IconElement, "No IconElement should be created without an IconSource");

                // Removing the icon after it was shown must return to the NoIcon presentation.
                teachingTip.IconSource = new SymbolIconSource { Symbol = Symbol.People };
                Verify.AreEqual("Icon", GetCurrentStateName(teachingTip, "IconStates"));
                teachingTip.IconSource = null;
                Verify.AreEqual("NoIcon", GetCurrentStateName(teachingTip, "IconStates"));
                Verify.IsNull(teachingTip.TemplateSettings.IconElement, "IconElement should be cleared with the IconSource");
            });
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

                // Positive control: the hooks above report Collapsed when the template part is missing, so
                // also prove that the same parts become Visible for non-empty text and collapse again independently.
                teachingTip.Title = "Title";
                teachingTip.Subtitle = "Subtitle";
                Verify.AreEqual(Visibility.Visible, TeachingTipTestHooks.GetTitleVisibility(teachingTip));
                Verify.AreEqual(Visibility.Visible, TeachingTipTestHooks.GetSubtitleVisibility(teachingTip));

                teachingTip.Title = "";
                Verify.AreEqual(Visibility.Collapsed, TeachingTipTestHooks.GetTitleVisibility(teachingTip));
                Verify.AreEqual(Visibility.Visible, TeachingTipTestHooks.GetSubtitleVisibility(teachingTip));

                teachingTip.Subtitle = "";
                Verify.AreEqual(Visibility.Collapsed, TeachingTipTestHooks.GetSubtitleVisibility(teachingTip));
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

        // ------------------------------------------------------------------------------------------------
        // Targeted placement
        // ------------------------------------------------------------------------------------------------

        [TestMethod]
        public void TargetedTipIsPositionedAtPreferredSidePlacement()
        {
            foreach (var placement in new[] { TeachingTipPlacementMode.Top, TeachingTipPlacementMode.Bottom, TeachingTipPlacementMode.Left, TeachingTipPlacementMode.Right, TeachingTipPlacementMode.Center })
            {
                Log.Comment($"PreferredPlacement = {placement}");
                var tip = CreateTargetedTip(placement);
                Rect target = UseWindowBoundsAroundTarget(tip, AmpleSpace, AmpleSpace, AmpleSpace, AmpleSpace);
                OpenTip(tip);

                RunOnUIThread.Execute(() =>
                {
                    VerifyEffectivePlacement(tip, placement);
                    Rect tipRect = GetTipRect(tip);
                    double centerX = target.X + target.Width / 2;
                    double centerY = target.Y + target.Height / 2;

                    // Side placements center the tip on the target's axis and put its edge against the target.
                    (double x, double y) expected = placement switch
                    {
                        TeachingTipPlacementMode.Top => (centerX - tipRect.Width / 2, target.Y - tipRect.Height),
                        TeachingTipPlacementMode.Bottom => (centerX - tipRect.Width / 2, target.Bottom),
                        TeachingTipPlacementMode.Left => (target.X - tipRect.Width, centerY - tipRect.Height / 2),
                        TeachingTipPlacementMode.Right => (target.Right, centerY - tipRect.Height / 2),
                        _ /* Center */ => (centerX - tipRect.Width / 2, centerY - tipRect.Height),
                    };
                    VerifyOffset(expected.x, tipRect.X, $"{placement} HorizontalOffset");
                    VerifyOffset(expected.y, tipRect.Y, $"{placement} VerticalOffset");
                });
            }
        }

        [TestMethod]
        public void TargetedTipIsPositionedAtPreferredCornerPlacement()
        {
            var corners = new[]
            {
                TeachingTipPlacementMode.TopRight, TeachingTipPlacementMode.TopLeft,
                TeachingTipPlacementMode.BottomRight, TeachingTipPlacementMode.BottomLeft,
                TeachingTipPlacementMode.LeftTop, TeachingTipPlacementMode.LeftBottom,
                TeachingTipPlacementMode.RightTop, TeachingTipPlacementMode.RightBottom,
            };

            foreach (var placement in corners)
            {
                Log.Comment($"PreferredPlacement = {placement}");
                var tip = CreateTargetedTip(placement);
                Rect target = UseWindowBoundsAroundTarget(tip, AmpleSpace, AmpleSpace, AmpleSpace, AmpleSpace);
                OpenTip(tip);

                // The first positioning can run before the tail polygon has been measured, which leaves the tip offset by
                // half a tail (product concern C9). Setting PlacementMargin repositions the open tip with the settled layout
                // (TeachingTip::OnPlacementMarginChanged), so the formula is checked against stable sizes.
                const double margin = 1;
                RunOnUIThread.Execute(() => tip.PlacementMargin = new Thickness(margin));
                IdleSynchronizer.Wait();

                RunOnUIThread.Execute(() =>
                {
                    VerifyEffectivePlacement(tip, placement);
                    Rect tipRect = GetTipRect(tip);
                    double centerX = target.X + target.Width / 2;
                    double centerY = target.Y + target.Height / 2;

                    // The edge facing the target is exact (plus the margin). Along the other axis the tail is a fixed distance
                    // from one end of the tip (tail margin columns plus half the tail, as TeachingTip::MinimumTipEdgeToTailCenter
                    // computes it), so the tip is offset from the target center by that distance.
                    double tailCenter = GetMinimumTipEdgeToTailCenter(tip);
                    double topY = target.Y - tipRect.Height - margin;
                    double bottomY = target.Bottom + margin;
                    double leftX = target.X - tipRect.Width - margin;
                    double rightX = target.Right + margin;
                    (double x, double y) expected = placement switch
                    {
                        TeachingTipPlacementMode.TopRight => (centerX - tailCenter, topY),
                        TeachingTipPlacementMode.TopLeft => (centerX - tipRect.Width + tailCenter, topY),
                        TeachingTipPlacementMode.BottomRight => (centerX - tailCenter, bottomY),
                        TeachingTipPlacementMode.BottomLeft => (centerX - tipRect.Width + tailCenter, bottomY),
                        TeachingTipPlacementMode.LeftTop => (leftX, centerY - tipRect.Height + tailCenter),
                        TeachingTipPlacementMode.LeftBottom => (leftX, centerY - tailCenter),
                        TeachingTipPlacementMode.RightTop => (rightX, centerY - tipRect.Height + tailCenter),
                        _ /* RightBottom */ => (rightX, centerY - tailCenter),
                    };
                    VerifyOffset(expected.x, tipRect.X, $"{placement} HorizontalOffset (tail center distance {tailCenter})");
                    VerifyOffset(expected.y, tipRect.Y, $"{placement} VerticalOffset (tail center distance {tailCenter})");
                });
            }
        }

        [TestMethod]
        public void TailPlacementUpdatesTopHighlightMargins()
        {
            foreach (var placement in AllPlacements)
            {
                Log.Comment($"PreferredPlacement = {placement}");
                var tip = CreateTargetedTip(placement);
                UseWindowBoundsAroundTarget(tip, AmpleSpace, AmpleSpace, AmpleSpace, AmpleSpace);
                OpenTip(tip);

                RunOnUIThread.Execute(() =>
                {
                    VerifyEffectivePlacement(tip, placement);
                    VerifyHighlightMarginsForTail(tip, placement);
                });
            }
        }

        [TestMethod]
        public void PlacementMarginMovesOpenTargetedTipAwayFromTarget()
        {
            var margin = new Thickness(11, 13, 17, 19);
            foreach (var placement in new[] { TeachingTipPlacementMode.Top, TeachingTipPlacementMode.Bottom, TeachingTipPlacementMode.Left, TeachingTipPlacementMode.Right })
            {
                Log.Comment($"PreferredPlacement = {placement}");
                var tip = CreateTargetedTip(placement);
                UseWindowBoundsAroundTarget(tip, AmpleSpace, AmpleSpace, AmpleSpace, AmpleSpace);
                OpenTip(tip);

                Rect before = default;
                RunOnUIThread.Execute(() =>
                {
                    before = GetTipRect(tip);
                    tip.PlacementMargin = margin;
                });
                IdleSynchronizer.Wait();

                RunOnUIThread.Execute(() =>
                {
                    // Only the margin on the side facing the target applies, and the placement itself is kept.
                    VerifyEffectivePlacement(tip, placement);
                    Rect after = GetTipRect(tip);
                    (double dx, double dy) expected = placement switch
                    {
                        TeachingTipPlacementMode.Top => (0, -margin.Top),
                        TeachingTipPlacementMode.Bottom => (0, margin.Bottom),
                        TeachingTipPlacementMode.Left => (-margin.Left, 0),
                        _ /* Right */ => (margin.Right, 0),
                    };
                    VerifyOffset(before.X + expected.dx, after.X, $"{placement} HorizontalOffset after PlacementMargin change");
                    VerifyOffset(before.Y + expected.dy, after.Y, $"{placement} VerticalOffset after PlacementMargin change");
                });
            }
        }

        [TestMethod]
        public void PreferredPlacementIsMirroredForRightToLeftFlowDirection()
        {
            var expectedPlacements = new Dictionary<TeachingTipPlacementMode, TeachingTipPlacementMode>
            {
                { TeachingTipPlacementMode.Auto, TeachingTipPlacementMode.Top },
                { TeachingTipPlacementMode.Top, TeachingTipPlacementMode.Top },
                { TeachingTipPlacementMode.Bottom, TeachingTipPlacementMode.Bottom },
                { TeachingTipPlacementMode.Center, TeachingTipPlacementMode.Center },
                { TeachingTipPlacementMode.Left, TeachingTipPlacementMode.Right },
                { TeachingTipPlacementMode.Right, TeachingTipPlacementMode.Left },
                { TeachingTipPlacementMode.TopLeft, TeachingTipPlacementMode.TopRight },
                { TeachingTipPlacementMode.TopRight, TeachingTipPlacementMode.TopLeft },
                { TeachingTipPlacementMode.BottomLeft, TeachingTipPlacementMode.BottomRight },
                { TeachingTipPlacementMode.BottomRight, TeachingTipPlacementMode.BottomLeft },
                { TeachingTipPlacementMode.LeftTop, TeachingTipPlacementMode.RightTop },
                { TeachingTipPlacementMode.LeftBottom, TeachingTipPlacementMode.RightBottom },
                { TeachingTipPlacementMode.RightTop, TeachingTipPlacementMode.LeftTop },
                { TeachingTipPlacementMode.RightBottom, TeachingTipPlacementMode.LeftBottom },
            };

            foreach (var entry in expectedPlacements)
            {
                Log.Comment($"RightToLeft PreferredPlacement = {entry.Key}, expected effective placement = {entry.Value}");
                var tip = CreateTargetedTip(entry.Key, t => t.FlowDirection = FlowDirection.RightToLeft);
                UseWindowBoundsAroundTarget(tip, AmpleSpace, AmpleSpace, AmpleSpace, AmpleSpace);
                OpenTip(tip);

                RunOnUIThread.Execute(() =>
                {
                    Verify.AreEqual(entry.Value, TeachingTipTestHooks.GetEffectivePlacement(tip), $"Effective placement for RightToLeft {entry.Key}");
                });
            }
        }

        // ------------------------------------------------------------------------------------------------
        // Placement fallback
        // ------------------------------------------------------------------------------------------------

        [TestMethod]
        public void PlacementFallsBackToOppositeSideWhenPreferredSideLacksSpace()
        {
            // Horizontal preferences fall back to the other horizontal side before trying vertical ones.
            VerifyEffectivePlacementWithinWindow(TeachingTipPlacementMode.Left, TeachingTipPlacementMode.Right, NarrowSpace, AmpleSpace, AmpleSpace, AmpleSpace);
            VerifyEffectivePlacementWithinWindow(TeachingTipPlacementMode.Right, TeachingTipPlacementMode.Left, AmpleSpace, AmpleSpace, NarrowSpace, AmpleSpace);
            VerifyEffectivePlacementWithinWindow(TeachingTipPlacementMode.Bottom, TeachingTipPlacementMode.Top, AmpleSpace, AmpleSpace, AmpleSpace, NarrowSpace);
            VerifyEffectivePlacementWithinWindow(TeachingTipPlacementMode.Top, TeachingTipPlacementMode.Bottom, AmpleSpace, NarrowSpace, AmpleSpace, AmpleSpace);
        }

        [TestMethod]
        public void PlacementAvoidsSidesWhereTargetIsOutsideWindow()
        {
            // The target's left 30px (more than half of it) is outside the window: placements centered horizontally on the
            // target and Left placements are unavailable, so a Top preference ends up on the Right.
            VerifyEffectivePlacementWithinWindow(TeachingTipPlacementMode.Top, TeachingTipPlacementMode.Right, -30, AmpleSpace, AmpleSpace, AmpleSpace);

            // The target's top 10px is outside the window: Top placements are unavailable.
            VerifyEffectivePlacementWithinWindow(TeachingTipPlacementMode.Top, TeachingTipPlacementMode.Bottom, AmpleSpace, -10, AmpleSpace, AmpleSpace);
        }

        [TestMethod]
        public void HeroContentPlacementExcludesTailPlacementsAgainstHeroContent()
        {
            // Hero content at the bottom: a tail on the bottom edge (Top* placements) would touch it.
            VerifyEffectivePlacementWithinWindow(TeachingTipPlacementMode.Top, TeachingTipPlacementMode.Bottom, AmpleSpace, AmpleSpace, AmpleSpace, AmpleSpace,
                t =>
                {
                    t.HeroContent = new Border { Width = 200, Height = 40 };
                    t.HeroContentPlacement = TeachingTipHeroContentPlacementMode.Bottom;
                });

            // Hero content at the top: a tail on the top edge (Bottom* placements) would touch it.
            VerifyEffectivePlacementWithinWindow(TeachingTipPlacementMode.Bottom, TeachingTipPlacementMode.Top, AmpleSpace, AmpleSpace, AmpleSpace, AmpleSpace,
                t =>
                {
                    t.HeroContent = new Border { Width = 200, Height = 40 };
                    t.HeroContentPlacement = TeachingTipHeroContentPlacementMode.Top;
                });

            // Hero content taller than the rest of the tip: a side tail would touch it, so Left/Right are unavailable.
            VerifyEffectivePlacementWithinWindow(TeachingTipPlacementMode.Left, TeachingTipPlacementMode.Top, AmpleSpace, AmpleSpace, AmpleSpace, AmpleSpace,
                t =>
                {
                    t.Content = new Border { Width = 200, Height = 20 };
                    t.HeroContent = new Border { Width = 200, Height = 250 };
                });
        }

        [TestMethod]
        public void HeroContentFollowsTailSideWhenPlacementIsAuto()
        {
            foreach (var placement in AllPlacements)
            {
                var tip = CreateTargetedTip(placement, t => t.HeroContent = new Border { Width = 200, Height = 40 });
                UseWindowBoundsAroundTarget(tip, AmpleSpace, AmpleSpace, AmpleSpace, AmpleSpace);
                OpenTip(tip);

                RunOnUIThread.Execute(() =>
                {
                    VerifyEffectivePlacement(tip, placement);

                    // With HeroContentPlacement=Auto the hero content moves away from the tail, so Bottom tails put it at the bottom.
                    bool tailOnTopEdge = placement == TeachingTipPlacementMode.Bottom || placement == TeachingTipPlacementMode.BottomLeft ||
                        placement == TeachingTipPlacementMode.BottomRight || placement == TeachingTipPlacementMode.LeftBottom ||
                        placement == TeachingTipPlacementMode.RightBottom;
                    var expected = tailOnTopEdge ? TeachingTipHeroContentPlacementMode.Bottom : TeachingTipHeroContentPlacementMode.Top;
                    Verify.AreEqual(expected, TeachingTipTestHooks.GetEffectiveHeroContentPlacement(tip), $"Effective hero placement for {placement}");
                    Verify.AreEqual(tailOnTopEdge ? "HeroContentBottom" : "HeroContentTop", GetCurrentStateName(tip, "HeroContentPlacementStates"), $"Hero state for {placement}");
                });
            }
        }

        [TestMethod]
        public void UnconstrainedTipUsesPreferredPlacementWithoutSpaceChecks()
        {
            // Out-of-root tips can extend past the window, so the preferred placement is used as-is even without room for it.
            var tip = CreateTargetedTip(TeachingTipPlacementMode.Bottom, t => t.ShouldConstrainToRootBounds = false);
            UseWindowBoundsAroundTarget(tip, AmpleSpace, AmpleSpace, AmpleSpace, NarrowSpace);
            OpenTip(tip);

            RunOnUIThread.Execute(() =>
            {
                Verify.AreEqual(TeachingTipPlacementMode.Bottom, TeachingTipTestHooks.GetEffectivePlacement(tip));
                Verify.IsFalse(TeachingTipTestHooks.GetPopup(tip).ShouldConstrainToRootBounds, "Popup should not be constrained to root bounds");
            });
        }

        [TestMethod]
        public void ChangingShouldConstrainToRootBoundsAppliesToNextOpen()
        {
            var tip = CreateUntargetedTip();
            OpenTip(tip);

            object firstPopup = null;
            RunOnUIThread.Execute(() =>
            {
                var popup = TeachingTipTestHooks.GetPopup(tip);
                firstPopup = popup;
                Verify.IsTrue(popup.ShouldConstrainToRootBounds);

                // The popup cannot change this setting while open; the open popup must be left untouched.
                tip.ShouldConstrainToRootBounds = false;
                Verify.AreSame(popup, TeachingTipTestHooks.GetPopup(tip));
                Verify.IsTrue(popup.ShouldConstrainToRootBounds);
                Verify.IsTrue(popup.IsOpen);
            });

            CloseTip(tip);
            OpenTip(tip);

            RunOnUIThread.Execute(() =>
            {
                var popup = TeachingTipTestHooks.GetPopup(tip);
                Verify.AreNotSame(firstPopup, popup, "A new popup should be created for the new setting");
                Verify.IsFalse(popup.ShouldConstrainToRootBounds);
                Verify.IsTrue(popup.IsOpen);
            });
        }

        // ------------------------------------------------------------------------------------------------
        // Tail visibility
        // ------------------------------------------------------------------------------------------------

        [TestMethod]
        public void CollapsedTailVisibilityHidesTailWithoutMovingTargetedTip()
        {
            var tip = CreateTargetedTip(TeachingTipPlacementMode.Bottom);
            UseWindowBoundsAroundTarget(tip, AmpleSpace, AmpleSpace, AmpleSpace, AmpleSpace);
            OpenTip(tip);

            Rect before = default;
            RunOnUIThread.Execute(() =>
            {
                VerifyEffectivePlacement(tip, TeachingTipPlacementMode.Bottom);
                Verify.AreEqual(Visibility.Visible, GetTailPolygon(tip).Visibility);
                before = GetTipRect(tip);

                tip.TailVisibility = TeachingTipTailVisibility.Collapsed;
            });
            IdleSynchronizer.Wait();

            RunOnUIThread.Execute(() =>
            {
                Verify.AreEqual("Untargeted", GetCurrentStateName(tip, "PlacementStates"));
                Verify.AreEqual(Visibility.Collapsed, GetTailPolygon(tip).Visibility);
                Verify.AreEqual(TeachingTipPlacementMode.Bottom, TeachingTipTestHooks.GetEffectivePlacement(tip), "Hiding the tail must not change the placement");
                Rect after = GetTipRect(tip);
                VerifyOffset(before.X, after.X, "HorizontalOffset after collapsing the tail");
                VerifyOffset(before.Y, after.Y, "VerticalOffset after collapsing the tail");
                // Without a tail the top edge is highlighted as for an untargeted tip.
                VerifyThickness(new Thickness(0), tip.TemplateSettings.TopRightHighlightMargin, "TopRightHighlightMargin without tail");

                tip.TailVisibility = TeachingTipTailVisibility.Auto;
            });
            IdleSynchronizer.Wait();

            RunOnUIThread.Execute(() =>
            {
                VerifyEffectivePlacement(tip, TeachingTipPlacementMode.Bottom);
                Verify.AreEqual(Visibility.Visible, GetTailPolygon(tip).Visibility);
            });
        }

        [TestMethod]
        public void VisibleTailVisibilityShowsTailForUntargetedTip()
        {
            var defaultTip = CreateUntargetedTip();
            OpenTip(defaultTip);
            RunOnUIThread.Execute(() =>
            {
                Verify.AreEqual("Untargeted", GetCurrentStateName(defaultTip, "PlacementStates"), "Untargeted tips have no tail by default");
                Verify.AreEqual(Visibility.Collapsed, GetTailPolygon(defaultTip).Visibility);
            });

            var tip = CreateUntargetedTip(t => t.TailVisibility = TeachingTipTailVisibility.Visible);
            OpenTip(tip);
            RunOnUIThread.Execute(() =>
            {
                // An untargeted tip is placed at the bottom of the window, so a forced tail uses the Bottom presentation.
                Verify.AreEqual("Bottom", GetCurrentStateName(tip, "PlacementStates"));
                Verify.AreEqual(Visibility.Visible, GetTailPolygon(tip).Visibility);
            });
        }

        // ------------------------------------------------------------------------------------------------
        // Untargeted placement
        // ------------------------------------------------------------------------------------------------

        [TestMethod]
        public void UntargetedTipIsPositionedRelativeToWindowEdges()
        {
            var margin = new Thickness(3, 5, 7, 11);
            const double windowEdgeMargin = 24;

            foreach (var placement in AllPlacements.Concat(new[] { TeachingTipPlacementMode.Auto }))
            {
                Log.Comment($"PreferredPlacement = {placement}");
                var tip = CreateUntargetedTip(t =>
                {
                    t.PreferredPlacement = placement;
                    t.PlacementMargin = margin;
                });
                OpenTip(tip);

                RunOnUIThread.Execute(() =>
                {
                    Size window = tip.XamlRoot.Size;
                    Rect tipRect = GetTipRect(tip);

                    double nearX = windowEdgeMargin + margin.Left;
                    double nearY = windowEdgeMargin + margin.Top;
                    double farX = window.Width - (tipRect.Width + windowEdgeMargin + margin.Right);
                    double farY = window.Height - (tipRect.Height + windowEdgeMargin + margin.Bottom);
                    double centerX = window.Width / 2 - tipRect.Width / 2 + margin.Left - margin.Right;
                    double centerY = window.Height / 2 - tipRect.Height / 2 + margin.Top - margin.Bottom;

                    (double x, double y) expected = placement switch
                    {
                        TeachingTipPlacementMode.Top => (centerX, nearY),
                        TeachingTipPlacementMode.Left => (nearX, centerY),
                        TeachingTipPlacementMode.Right => (farX, centerY),
                        TeachingTipPlacementMode.TopRight => (farX, nearY),
                        TeachingTipPlacementMode.TopLeft => (nearX, nearY),
                        TeachingTipPlacementMode.BottomRight => (farX, farY),
                        TeachingTipPlacementMode.BottomLeft => (nearX, farY),
                        TeachingTipPlacementMode.LeftTop => (nearX, nearY),
                        TeachingTipPlacementMode.LeftBottom => (nearX, farY),
                        TeachingTipPlacementMode.RightTop => (farX, nearY),
                        TeachingTipPlacementMode.RightBottom => (farX, farY),
                        TeachingTipPlacementMode.Center => (centerX, centerY),
                        _ /* Bottom, Auto */ => (centerX, farY),
                    };

                    Verify.AreEqual("Untargeted", GetCurrentStateName(tip, "PlacementStates"));
                    VerifyOffset(expected.x, tipRect.X, $"{placement} HorizontalOffset");
                    VerifyOffset(expected.y, tipRect.Y, $"{placement} VerticalOffset");
                });
            }
        }

        [TestMethod]
        public void UntargetedTipThatDoesNotFitRaisesClosingAndClosedWithoutOpening()
        {
            var tip = CreateUntargetedTip();

            // Open once so the tip has been measured, then shrink the (simulated) window below the tip's size.
            OpenTip(tip);
            CloseTip(tip);

            var events = new List<string>();
            var closedEvent = new ManualResetEvent(false);
            RunOnUIThread.Execute(() =>
            {
                tip.Closing += (s, a) => events.Add("Closing:" + a.Reason);
                tip.Closed += (s, a) => { events.Add("Closed:" + a.Reason); closedEvent.Set(); };
                TeachingTipTestHooks.SetUseTestWindowBounds(tip, true);
                TeachingTipTestHooks.SetTestWindowBounds(tip, new Rect(0, 0, 10, 10));
                tip.IsOpen = true;
            });

            Verify.IsTrue(closedEvent.WaitOne(DefaultWaitTimeInMS), "Closed should be raised for a tip that does not fit");
            Verify.IsTrue(WaitForTipState(tip, isOpen: false), "TeachingTip should settle closed");

            RunOnUIThread.Execute(() =>
            {
                Verify.AreEqual("Closing:Programmatic,Closed:Programmatic", string.Join(",", events));
                Verify.IsFalse(tip.IsOpen, "IsOpen should be reset when the tip does not fit");
                Verify.IsFalse(TeachingTipTestHooks.GetPopup(tip).IsOpen, "Popup should not open when the tip does not fit");
            });
        }

        // ------------------------------------------------------------------------------------------------
        // Buttons, commands and styles
        // ------------------------------------------------------------------------------------------------

        [TestMethod]
        public void ActionButtonClickRaisesEventWithoutClosingTip()
        {
            var tip = CreateUntargetedTip(t => t.ActionButtonContent = "Action");
            OpenTip(tip);

            int clicks = 0;
            int removedHandlerClicks = 0;
            int closingCount = 0;
            object clickSender = null;
            TypedEventHandler<TeachingTip, object> removedHandler = (s, a) => removedHandlerClicks++;
            RunOnUIThread.Execute(() =>
            {
                tip.ActionButtonClick += (s, a) => { clicks++; clickSender = s; };
                tip.ActionButtonClick += removedHandler;
                tip.ActionButtonClick -= removedHandler;
                tip.Closing += (s, a) => closingCount++;

                InvokeButton(GetTemplateButton(tip, "ActionButton"));
            });
            IdleSynchronizer.Wait();

            RunOnUIThread.Execute(() =>
            {
                Verify.AreEqual(1, clicks, "ActionButtonClick count");
                Verify.AreSame(tip, clickSender, "ActionButtonClick sender");
                Verify.AreEqual(0, removedHandlerClicks, "A removed ActionButtonClick handler must not be invoked");
                Verify.AreEqual(0, closingCount, "The action button must not close the tip");
                Verify.IsTrue(tip.IsOpen);
            });
        }

        [TestMethod]
        public void ActionButtonCommandExecutesWithParameter()
        {
            var command = new RecordingCommand();
            var tip = CreateUntargetedTip(t =>
            {
                t.ActionButtonContent = "Action";
                t.ActionButtonCommand = command;
                t.ActionButtonCommandParameter = "ActionParameter";
            });
            OpenTip(tip);

            RunOnUIThread.Execute(() =>
            {
                Verify.AreSame(command, tip.ActionButtonCommand);
                Verify.AreEqual("ActionParameter", tip.ActionButtonCommandParameter as string);
                InvokeButton(GetTemplateButton(tip, "ActionButton"));
            });
            IdleSynchronizer.Wait();

            RunOnUIThread.Execute(() =>
            {
                Verify.AreEqual(1, command.ExecutedParameters.Count, "ActionButtonCommand execution count");
                Verify.AreEqual("ActionParameter", command.ExecutedParameters[0] as string);
                Verify.IsTrue(tip.IsOpen);
            });
        }

        [TestMethod]
        public void ActionButtonIsDisabledWhenCommandCannotExecute()
        {
            var command = new RecordingCommand { CanExecuteResult = false };
            var tip = CreateUntargetedTip(t =>
            {
                t.ActionButtonContent = "Action";
                t.ActionButtonCommand = command;
            });
            OpenTip(tip);

            RunOnUIThread.Execute(() =>
            {
                Verify.IsFalse(GetTemplateButton(tip, "ActionButton").IsEnabled);
            });
        }

        [TestMethod]
        public void CloseButtonCommandExecutesWithParameterAndClosesTip()
        {
            var command = new RecordingCommand();
            var tip = CreateUntargetedTip(t =>
            {
                t.CloseButtonContent = "Close";
                t.CloseButtonCommand = command;
                t.CloseButtonCommandParameter = "CloseParameter";
            });
            OpenTip(tip);

            var events = new List<string>();
            var closedRaised = new ManualResetEvent(false);
            int removedHandlerCalls = 0;
            TypedEventHandler<TeachingTip, object> removedHandler = (s, a) => removedHandlerCalls++;
            RunOnUIThread.Execute(() =>
            {
                Verify.AreSame(command, tip.CloseButtonCommand);
                Verify.AreEqual("CloseParameter", tip.CloseButtonCommandParameter as string);

                tip.CloseButtonClick += (s, a) => events.Add("CloseButtonClick");
                tip.CloseButtonClick += removedHandler;
                tip.CloseButtonClick -= removedHandler;
                tip.Closing += (s, a) => events.Add("Closing:" + a.Reason);
                tip.Closed += (s, a) => { events.Add("Closed:" + a.Reason); closedRaised.Set(); };

                InvokeButton(GetTemplateButton(tip, "CloseButton"));
            });

            Verify.IsTrue(WaitForTipState(tip, isOpen: false), "The close button should close the tip");
            // Popup.Closed (and so TeachingTip.Closed) is raised asynchronously after the popup closes.
            Verify.IsTrue(closedRaised.WaitOne(DefaultWaitTimeInMS), "Closed should be raised");

            RunOnUIThread.Execute(() =>
            {
                Verify.AreEqual(1, command.ExecutedParameters.Count, "CloseButtonCommand execution count");
                Verify.AreEqual("CloseParameter", command.ExecutedParameters[0] as string);
                Verify.AreEqual("CloseButtonClick,Closing:CloseButton,Closed:CloseButton", string.Join(",", events));
                Verify.AreEqual(0, removedHandlerCalls, "A removed CloseButtonClick handler must not be invoked");
                Verify.IsFalse(tip.IsOpen);
            });
        }

        [TestMethod]
        public void ButtonStylesAreAppliedToTemplateButtons()
        {
            Style actionStyle = null;
            Style closeStyle = null;
            RunOnUIThread.Execute(() =>
            {
                actionStyle = new Style(typeof(Button));
                actionStyle.Setters.Add(new Setter(FrameworkElement.TagProperty, "ActionStyled"));
                closeStyle = new Style(typeof(Button));
                closeStyle.Setters.Add(new Setter(FrameworkElement.TagProperty, "CloseStyled"));
            });

            var tip = CreateUntargetedTip(t =>
            {
                t.ActionButtonContent = "Action";
                t.CloseButtonContent = "Close";
                t.ActionButtonStyle = actionStyle;
                t.CloseButtonStyle = closeStyle;
            });
            OpenTip(tip);

            RunOnUIThread.Execute(() =>
            {
                Verify.AreSame(actionStyle, tip.ActionButtonStyle);
                Verify.AreSame(closeStyle, tip.CloseButtonStyle);

                var actionButton = GetTemplateButton(tip, "ActionButton");
                var closeButton = GetTemplateButton(tip, "CloseButton");
                Verify.AreSame(actionStyle, actionButton.Style);
                Verify.AreSame(closeStyle, closeButton.Style);
                Verify.AreEqual("ActionStyled", actionButton.Tag as string);
                Verify.AreEqual("CloseStyled", closeButton.Tag as string);
            });
        }

        [TestMethod]
        public void ButtonStatesReflectButtonContentAndLightDismiss()
        {
            var cases = new (object action, object close, bool lightDismiss, string buttons, string closeLocation)[]
            {
                (null, null, false, "NoButtonsVisible", "HeaderCloseButton"),
                (null, null, true, "NoButtonsVisible", "FooterCloseButton"),
                ("Action", null, false, "ActionButtonVisible", "HeaderCloseButton"),
                ("Action", null, true, "ActionButtonVisible", "FooterCloseButton"),
                (null, "Close", false, "CloseButtonVisible", "FooterCloseButton"),
                ("Action", "Close", false, "BothButtonsVisible", "FooterCloseButton"),
            };

            var tip = CreateUntargetedTip();
            RunOnUIThread.Execute(() =>
            {
                foreach (var c in cases)
                {
                    tip.ActionButtonContent = c.action;
                    tip.CloseButtonContent = c.close;
                    tip.IsLightDismissEnabled = c.lightDismiss;

                    string description = $"action={c.action ?? "null"}, close={c.close ?? "null"}, lightDismiss={c.lightDismiss}";
                    Verify.AreEqual(c.buttons, GetCurrentStateName(tip, "ButtonsStates"), description);
                    Verify.AreEqual(c.closeLocation, GetCurrentStateName(tip, "CloseButtonLocations"), description);
                }
            });
        }

        [TestMethod]
        public void ContentStatesTrackContentPresence()
        {
            var tip = CreateUntargetedTip(t => t.Content = null);
            RunOnUIThread.Execute(() =>
            {
                tip.Content = new Border { Width = 50, Height = 50 };
                Verify.AreEqual("Content", GetCurrentStateName(tip, "ContentStates"));

                tip.Content = null;
                Verify.AreEqual("NoContent", GetCurrentStateName(tip, "ContentStates"));

                tip.Content = "Text content";
                Verify.AreEqual("Content", GetCurrentStateName(tip, "ContentStates"));
            });
        }

        // ------------------------------------------------------------------------------------------------
        // Closing: deferral
        // ------------------------------------------------------------------------------------------------

        [TestMethod]
        public void ClosingDeferralHoldsTipOpenUntilCompleted()
        {
            var tip = CreateUntargetedTip();
            OpenTip(tip);

            Deferral deferral = null;
            var closedReasons = new List<TeachingTipCloseReason>();
            var closingRaised = new ManualResetEvent(false);
            var closedRaised = new ManualResetEvent(false);
            RunOnUIThread.Execute(() =>
            {
                tip.Closing += (s, a) => { deferral = a.GetDeferral(); closingRaised.Set(); };
                tip.Closed += (s, a) => { closedReasons.Add(a.Reason); closedRaised.Set(); };
                tip.IsOpen = false;
            });

            Verify.IsTrue(closingRaised.WaitOne(DefaultWaitTimeInMS), "Closing should be raised");
            IdleSynchronizer.Wait();

            RunOnUIThread.Execute(() =>
            {
                Verify.IsFalse(tip.IsOpen);
                Verify.IsTrue(TeachingTipTestHooks.GetPopup(tip).IsOpen, "An outstanding deferral must keep the popup open");
                Verify.AreEqual(0, closedReasons.Count, "Closed must wait for the deferral");
                Verify.IsFalse(TeachingTipTestHooks.GetIsIdle(tip), "The tip is still closing");

                deferral.Complete();
            });

            Verify.IsTrue(WaitForTipState(tip, isOpen: false), "Completing the deferral should close the tip");
            Verify.IsTrue(closedRaised.WaitOne(DefaultWaitTimeInMS), "Closed should be raised");
            RunOnUIThread.Execute(() =>
            {
                Verify.AreEqual(1, closedReasons.Count);
                Verify.AreEqual(TeachingTipCloseReason.Programmatic, closedReasons[0]);
            });
        }

        [TestMethod]
        public void ClosingWaitsForAllDeferrals()
        {
            var tip = CreateUntargetedTip();
            OpenTip(tip);

            Deferral first = null;
            Deferral second = null;
            int closedCount = 0;
            var closingRaised = new ManualResetEvent(false);
            var closedRaised = new ManualResetEvent(false);
            RunOnUIThread.Execute(() =>
            {
                tip.Closing += (s, a) => { first = a.GetDeferral(); second = a.GetDeferral(); closingRaised.Set(); };
                tip.Closed += (s, a) => { closedCount++; closedRaised.Set(); };
                tip.IsOpen = false;
            });

            Verify.IsTrue(closingRaised.WaitOne(DefaultWaitTimeInMS), "Closing should be raised");
            RunOnUIThread.Execute(() => first.Complete());
            IdleSynchronizer.Wait();

            RunOnUIThread.Execute(() =>
            {
                Verify.IsTrue(TeachingTipTestHooks.GetPopup(tip).IsOpen, "One outstanding deferral must keep the popup open");
                Verify.AreEqual(0, closedCount);
                second.Complete();
            });

            Verify.IsTrue(WaitForTipState(tip, isOpen: false), "Completing the last deferral should close the tip");
            Verify.IsTrue(closedRaised.WaitOne(DefaultWaitTimeInMS), "Closed should be raised");
            RunOnUIThread.Execute(() => Verify.AreEqual(1, closedCount));
        }

        // ------------------------------------------------------------------------------------------------
        // Automation
        // ------------------------------------------------------------------------------------------------

        [TestMethod]
        public void AutomationPeerReportsWindowStateOfTip()
        {
            var tip = CreateUntargetedTip();
            IWindowProvider window = null;
            RunOnUIThread.Execute(() =>
            {
                window = GetWindowProvider(tip);
                Verify.IsFalse(window.IsTopmost, "A closed tip is not topmost");
                Verify.IsFalse(window.IsModal, "A tip without light dismiss is not modal");
                Verify.IsFalse(window.Maximizable);
                Verify.IsFalse(window.Minimizable);
                Verify.AreEqual(WindowVisualState.Normal, window.VisualState);
                Verify.IsTrue(window.WaitForInputIdle(0));

                tip.IsLightDismissEnabled = true;
                Verify.IsTrue(window.IsModal, "A light-dismiss tip is modal");
                tip.IsLightDismissEnabled = false;
                Verify.IsFalse(window.IsModal);
            });

            OpenTip(tip);

            RunOnUIThread.Execute(() =>
            {
                Verify.IsTrue(window.IsTopmost, "An open tip is topmost");
                Verify.AreEqual(WindowInteractionState.ReadyForUserInteraction, window.InteractionState);

                // Window visual state is not supported: requests are ignored.
                window.SetVisualState(WindowVisualState.Minimized);
                Verify.AreEqual(WindowVisualState.Normal, window.VisualState);
                Verify.IsTrue(tip.IsOpen);
            });
        }

        [TestMethod]
        public void AutomationPeerCloseClosesTip()
        {
            var tip = CreateUntargetedTip();
            OpenTip(tip);

            var closedReasons = new List<TeachingTipCloseReason>();
            var closedRaised = new ManualResetEvent(false);
            RunOnUIThread.Execute(() =>
            {
                tip.Closed += (s, a) => { closedReasons.Add(a.Reason); closedRaised.Set(); };
                GetWindowProvider(tip).Close();
                Verify.IsFalse(tip.IsOpen, "IWindowProvider.Close should set IsOpen to false");
            });

            Verify.IsTrue(WaitForTipState(tip, isOpen: false), "The tip should close");
            Verify.IsTrue(closedRaised.WaitOne(DefaultWaitTimeInMS), "Closed should be raised");
            RunOnUIThread.Execute(() =>
            {
                Verify.AreEqual(1, closedReasons.Count);
                Verify.AreEqual(TeachingTipCloseReason.Programmatic, closedReasons[0]);
                Verify.IsFalse(GetWindowProvider(tip).IsTopmost);
            });
        }

        [TestMethod]
        public void AutomationPeerReportsClosingWhileCloseIsDeferred()
        {
            var tip = CreateUntargetedTip();
            OpenTip(tip);

            Deferral deferral = null;
            var closingRaised = new ManualResetEvent(false);
            RunOnUIThread.Execute(() =>
            {
                tip.Closing += (s, a) => { deferral = a.GetDeferral(); closingRaised.Set(); };
                tip.IsOpen = false;
            });
            Verify.IsTrue(closingRaised.WaitOne(DefaultWaitTimeInMS), "Closing should be raised");
            IdleSynchronizer.Wait();

            RunOnUIThread.Execute(() =>
            {
                Verify.AreEqual(WindowInteractionState.Closing, GetWindowProvider(tip).InteractionState);
                deferral.Complete();
            });

            Verify.IsTrue(WaitForTipState(tip, isOpen: false), "The tip should close");
            RunOnUIThread.Execute(() =>
            {
                // Not asserted: the value reported for a closed, idle tip is under review.
                Log.Comment($"InteractionState for a closed idle tip: {GetWindowProvider(tip).InteractionState}");
            });
        }

        [TestMethod]
        public void AutomationIdAndNameAreForwardedToPopup()
        {
            var tip = CreateUntargetedTip(t => t.Title = "First title");
            OpenTip(tip);

            RunOnUIThread.Execute(() =>
            {
                var popup = TeachingTipTestHooks.GetPopup(tip);
                Verify.AreEqual("First title", AutomationProperties.GetName(popup), "Popup name falls back to the title");

                tip.Title = "Second title";
                Verify.AreEqual("Second title", AutomationProperties.GetName(popup), "Popup name tracks the title");

                AutomationProperties.SetName(tip, "Explicit name");
                Verify.AreEqual("Explicit name", AutomationProperties.GetName(popup), "An explicit name wins over the title");
                tip.Title = "Third title";
                Verify.AreEqual("Explicit name", AutomationProperties.GetName(popup));

                AutomationProperties.SetAutomationId(tip, "TipAutomationId");
                Verify.AreEqual("TipAutomationId", AutomationProperties.GetAutomationId(popup), "Popup AutomationId tracks the tip");
            });
        }

        // ------------------------------------------------------------------------------------------------
        // Tail center point, placement boundaries and opening state
        // ------------------------------------------------------------------------------------------------

        [TestMethod]
        public void TailPlacementSetsScaleCenterPointAtTail()
        {
            // The expand/contract animations scale the tip around TailOcclusionGrid.CenterPoint, which must sit at the tail.
            foreach (var placement in AllPlacements)
            {
                Log.Comment($"PreferredPlacement = {placement}");
                var tip = CreateTargetedTip(placement);
                UseWindowBoundsAroundTarget(tip, AmpleSpace, AmpleSpace, AmpleSpace, AmpleSpace);
                OpenTip(tip);

                RunOnUIThread.Execute(() =>
                {
                    VerifyEffectivePlacement(tip, placement);
                    var grid = GetTailOcclusionGrid(tip);
                    double width = grid.ActualWidth;
                    double height = grid.ActualHeight;
                    var columns = grid.ColumnDefinitions;
                    var rows = grid.RowDefinitions;
                    double firstColumn = columns[0].ActualWidth;
                    double secondColumn = columns[1].ActualWidth;
                    double nextToLastColumn = columns[columns.Count - 2].ActualWidth;
                    double lastColumn = columns[columns.Count - 1].ActualWidth;
                    double firstRow = rows[0].ActualHeight;
                    double secondRow = rows[1].ActualHeight;
                    double nextToLastRow = rows[rows.Count - 2].ActualHeight;
                    double lastRow = rows[rows.Count - 1].ActualHeight;

                    (double x, double y) expected = placement switch
                    {
                        TeachingTipPlacementMode.Top => (width / 2, height - lastRow),
                        TeachingTipPlacementMode.Bottom => (width / 2, firstRow),
                        TeachingTipPlacementMode.Left => (width - lastColumn, height / 2),
                        TeachingTipPlacementMode.Right => (firstColumn, height / 2),
                        TeachingTipPlacementMode.TopRight => (firstColumn + secondColumn + 1, height - lastRow),
                        TeachingTipPlacementMode.TopLeft => (width - (nextToLastColumn + lastColumn + 1), height - lastRow),
                        TeachingTipPlacementMode.BottomRight => (firstColumn + secondColumn + 1, firstRow),
                        TeachingTipPlacementMode.BottomLeft => (width - (nextToLastColumn + lastColumn + 1), firstRow),
                        TeachingTipPlacementMode.LeftTop => (width - lastColumn, height - (nextToLastRow + lastRow + 1)),
                        TeachingTipPlacementMode.LeftBottom => (width - lastColumn, firstRow + secondRow + 1),
                        TeachingTipPlacementMode.RightTop => (firstColumn, height - (nextToLastRow + lastRow + 1)),
                        TeachingTipPlacementMode.RightBottom => (firstColumn, firstRow + secondRow + 1),
                        _ /* Center */ => (width / 2, height - lastRow),
                    };
                    VerifyOffset(expected.x, grid.CenterPoint.X, $"{placement} CenterPoint.X");
                    VerifyOffset(expected.y, grid.CenterPoint.Y, $"{placement} CenterPoint.Y");
                });
            }

            // Without a tail the tip scales around its center.
            var untargeted = CreateUntargetedTip();
            OpenTip(untargeted);
            RunOnUIThread.Execute(() =>
            {
                var grid = GetTailOcclusionGrid(untargeted);
                VerifyOffset(grid.ActualWidth / 2, grid.CenterPoint.X, "Untargeted CenterPoint.X");
                VerifyOffset(grid.ActualHeight / 2, grid.CenterPoint.Y, "Untargeted CenterPoint.Y");
            });
        }

        [TestMethod]
        public void TargetFlushWithWindowEdgeKeepsThatSideAvailable()
        {
            // A target that touches a window edge is not clipped by it. An out-of-root tip whose screen has room beyond that
            // edge can therefore still use that side. The test hooks turn off the out-of-root "always Top" shortcut and
            // supply deterministic window and screen bounds.
            VerifyFlushEdgePlacement(TeachingTipPlacementMode.Left, t => new Rect(t.X, t.Y - AmpleSpace, t.Width + AmpleSpace, t.Height + 2 * AmpleSpace),
                (t, w) => t.X - w.X);
            VerifyFlushEdgePlacement(TeachingTipPlacementMode.Right, t => new Rect(t.X, t.Y - AmpleSpace, t.Width, t.Height + 2 * AmpleSpace),
                (t, w) => (w.X + w.Width) - (t.X + t.Width));
            VerifyFlushEdgePlacement(TeachingTipPlacementMode.Top, t => new Rect(t.X - AmpleSpace, t.Y, t.Width + 2 * AmpleSpace, t.Height + AmpleSpace),
                (t, w) => t.Y - w.Y);
        }

        [TestMethod]
        public void UntargetedTipExactlyAsTallAsWindowDoesNotOpen()
        {
            VerifyUntargetedExactFit(constrainToRootBounds: true);
        }

        [TestMethod]
        public void UntargetedOutOfRootTipExactlyAsTallAsScreenDoesNotOpen()
        {
            VerifyUntargetedExactFit(constrainToRootBounds: false);
        }

        [TestMethod]
        public void BottomCornerPreferenceFallsBackToBottomBeforeTop()
        {
            // Measure the tip and its tail position with ample space.
            var probe = CreateTargetedTip(TeachingTipPlacementMode.BottomLeft);
            UseWindowBoundsAroundTarget(probe, AmpleSpace, AmpleSpace, AmpleSpace, AmpleSpace);
            OpenTip(probe);
            double tipWidth = 0;
            double tailCenter = 0;
            RunOnUIThread.Execute(() =>
            {
                tipWidth = GetTailOcclusionGrid(probe).ActualWidth;
                tailCenter = GetMinimumTipEdgeToTailCenter(probe);
            });

            // Half the tip width beside the target leaves room for a centered Bottom or Top tip but not for the corner placement.
            double side = tipWidth / 2;
            Verify.IsTrue(tipWidth - tailCenter > side + TargetSize / 2, $"Precondition: the corner placement must not fit (width {tipWidth}, tail {tailCenter})");
            VerifyEffectivePlacementWithinWindow(TeachingTipPlacementMode.BottomLeft, TeachingTipPlacementMode.Bottom, side, AmpleSpace, AmpleSpace, AmpleSpace);
            VerifyEffectivePlacementWithinWindow(TeachingTipPlacementMode.BottomRight, TeachingTipPlacementMode.Bottom, AmpleSpace, AmpleSpace, side, AmpleSpace);
        }

        [TestMethod]
        public void AutomationPeerReportsRunningWhileTipIsOpening()
        {
            var tip = CreateUntargetedTip();
            IWindowProvider window = null;
            RunOnUIThread.Execute(() =>
            {
                window = GetWindowProvider(tip);
                // Slow the expand animation so that the opening phase (open but not yet idle) can be observed.
                TeachingTipTestHooks.SetExpandAnimationDuration(tip, TimeSpan.FromSeconds(2));
                tip.IsOpen = true;
            });

            // The popup opens on a later composition tick; there is no event for the start of the animation, so poll.
            bool opening = false;
            WindowInteractionState openingState = WindowInteractionState.NotResponding;
            var stopwatch = Stopwatch.StartNew();
            while (!opening && stopwatch.ElapsedMilliseconds < 5000)
            {
                RunOnUIThread.Execute(() =>
                {
                    var popup = TeachingTipTestHooks.GetPopup(tip);
                    if (popup != null && popup.IsOpen && !TeachingTipTestHooks.GetIsIdle(tip))
                    {
                        opening = true;
                        openingState = window.InteractionState;
                    }
                });
                if (!opening)
                {
                    Thread.Sleep(20);
                }
            }

            Verify.IsTrue(opening, "The tip should be observed while it is opening (popup open, not idle)");
            Verify.AreEqual(WindowInteractionState.Running, openingState, "InteractionState while opening");

            Verify.IsTrue(WaitForTipState(tip, isOpen: true), "The tip should finish opening");
            RunOnUIThread.Execute(() => Verify.AreEqual(WindowInteractionState.ReadyForUserInteraction, window.InteractionState));
        }

        // ------------------------------------------------------------------------------------------------
        // Helpers
        // ------------------------------------------------------------------------------------------------

        private const double TargetSize = 40;
        private const double AmpleSpace = 2000;
        private const double NarrowSpace = 5;
        private const double OffsetTolerance = 0.5;

        private static readonly TeachingTipPlacementMode[] AllPlacements = new[]
        {
            TeachingTipPlacementMode.Top, TeachingTipPlacementMode.Bottom, TeachingTipPlacementMode.Left, TeachingTipPlacementMode.Right,
            TeachingTipPlacementMode.TopRight, TeachingTipPlacementMode.TopLeft, TeachingTipPlacementMode.BottomRight, TeachingTipPlacementMode.BottomLeft,
            TeachingTipPlacementMode.LeftTop, TeachingTipPlacementMode.LeftBottom, TeachingTipPlacementMode.RightTop, TeachingTipPlacementMode.RightBottom,
            TeachingTipPlacementMode.Center,
        };

        private sealed class RecordingCommand : ICommand
        {
            public bool CanExecuteResult { get; set; } = true;
            public List<object> ExecutedParameters { get; } = new List<object>();
            public event EventHandler CanExecuteChanged { add { } remove { } }
            public bool CanExecute(object parameter) => CanExecuteResult;
            public void Execute(object parameter) => ExecutedParameters.Add(parameter);
        }

        // Creates a tip targeting a centered button. 'configure' runs before the tip enters the tree.
        private TeachingTip CreateTargetedTip(TeachingTipPlacementMode preferredPlacement, Action<TeachingTip> configure = null)
        {
            TeachingTip tip = null;
            RunOnUIThread.Execute(() =>
            {
                var target = new Button
                {
                    Content = "Target",
                    Width = TargetSize,
                    Height = TargetSize,
                    HorizontalAlignment = HorizontalAlignment.Center,
                    VerticalAlignment = VerticalAlignment.Center,
                };
                tip = new TeachingTip
                {
                    Title = "Title",
                    Content = new Border { Width = 200, Height = 150 },
                    Target = target,
                    PreferredPlacement = preferredPlacement,
                };
                configure?.Invoke(tip);

                var root = new Grid();
                root.Children.Add(target);
                root.Children.Add(tip);
                Content = root;
                Content.UpdateLayout();
            });
            return tip;
        }

        private TeachingTip CreateUntargetedTip(Action<TeachingTip> configure = null)
        {
            TeachingTip tip = null;
            RunOnUIThread.Execute(() =>
            {
                tip = new TeachingTip
                {
                    Title = "Title",
                    Content = new Border { Width = 200, Height = 150 },
                };
                configure?.Invoke(tip);
                Content = tip;
                Content.UpdateLayout();
            });
            return tip;
        }

        // Simulates the window (in core-window space) around the tip's target so placement decisions do not depend
        // on the size or position of the test app window. Negative values put part of the target outside the window.
        private static Rect UseWindowBoundsAroundTarget(TeachingTip tip, double left, double top, double right, double bottom)
        {
            Rect targetBounds = default;
            RunOnUIThread.Execute(() =>
            {
                targetBounds = GetBoundsInRoot(tip.Target);
                TeachingTipTestHooks.SetUseTestWindowBounds(tip, true);
                TeachingTipTestHooks.SetTestWindowBounds(tip, new Rect(
                    targetBounds.X - left,
                    targetBounds.Y - top,
                    targetBounds.Width + left + right,
                    targetBounds.Height + top + bottom));
            });
            return targetBounds;
        }

        private void VerifyEffectivePlacementWithinWindow(
            TeachingTipPlacementMode preferred,
            TeachingTipPlacementMode expected,
            double left, double top, double right, double bottom,
            Action<TeachingTip> configure = null)
        {
            Log.Comment($"PreferredPlacement = {preferred}, space around target (l,t,r,b) = ({left},{top},{right},{bottom})");
            var tip = CreateTargetedTip(preferred, configure);
            UseWindowBoundsAroundTarget(tip, left, top, right, bottom);
            OpenTip(tip);
            RunOnUIThread.Execute(() =>
            {
                Verify.AreEqual(expected, TeachingTipTestHooks.GetEffectivePlacement(tip), $"Effective placement for preferred {preferred}");
            });
        }

        private static Rect GetBoundsInRoot(FrameworkElement element)
        {
            return element.TransformToVisual(null).TransformBounds(new Rect(0, 0, element.ActualWidth, element.ActualHeight));
        }

        private static void OpenTip(TeachingTip tip)
        {
            RunOnUIThread.Execute(() => tip.IsOpen = true);
            Verify.IsTrue(WaitForTipState(tip, isOpen: true), "TeachingTip should open and become idle");
            IdleSynchronizer.Wait();
        }

        private static void CloseTip(TeachingTip tip)
        {
            RunOnUIThread.Execute(() => tip.IsOpen = false);
            Verify.IsTrue(WaitForTipState(tip, isOpen: false), "TeachingTip should close and become idle");
            IdleSynchronizer.Wait();
        }

        // IsOpen changes are applied on a later composition-rendering tick and are followed by an expand/contract
        // animation. There is no public completion event (TeachingTipTestHooks.IdleStatusChanged is DBG-only), so this
        // polls the observable state; the timeout is only an outer safety bound.
        private static bool WaitForTipState(TeachingTip tip, bool isOpen)
        {
            var stopwatch = Stopwatch.StartNew();
            bool reached = false;
            while (true)
            {
                IdleSynchronizer.Wait();
                RunOnUIThread.Execute(() =>
                {
                    var popup = TeachingTipTestHooks.GetPopup(tip);
                    reached = TeachingTipTestHooks.GetIsIdle(tip) && popup != null && popup.IsOpen == isOpen && tip.IsOpen == isOpen;
                });
                if (reached || stopwatch.ElapsedMilliseconds > 10000)
                {
                    break;
                }
                Thread.Sleep(50);
            }
            return reached;
        }

        // Size and position of the open tip (excluding its tail) in core-window space.
        private static Rect GetTipRect(TeachingTip tip)
        {
            var popup = TeachingTipTestHooks.GetPopup(tip);
            var tailOcclusionGrid = VisualTreeUtils.FindVisualChildByName(popup.Child, "TailOcclusionGrid");
            Verify.IsNotNull(tailOcclusionGrid, "TailOcclusionGrid");
            return new Rect(popup.HorizontalOffset, popup.VerticalOffset, tailOcclusionGrid.ActualWidth, tailOcclusionGrid.ActualHeight);
        }

        private static Polygon GetTailPolygon(TeachingTip tip)
        {
            var tailPolygon = VisualTreeUtils.FindVisualChildByName(TeachingTipTestHooks.GetPopup(tip).Child, "TailPolygon") as Polygon;
            Verify.IsNotNull(tailPolygon, "TailPolygon");
            return tailPolygon;
        }

        private static Button GetTemplateButton(TeachingTip tip, string name)
        {
            var button = VisualTreeUtils.FindVisualChildByName(TeachingTipTestHooks.GetPopup(tip).Child, name) as Button;
            Verify.IsNotNull(button, name);
            return button;
        }

        private static void InvokeButton(Button button)
        {
            var peer = FrameworkElementAutomationPeer.CreatePeerForElement(button);
            var invokeProvider = peer.GetPattern(PatternInterface.Invoke) as IInvokeProvider;
            Verify.IsNotNull(invokeProvider, "IInvokeProvider");
            invokeProvider.Invoke();
        }

        private static IWindowProvider GetWindowProvider(TeachingTip tip)
        {
            var peer = FrameworkElementAutomationPeer.CreatePeerForElement(tip);
            Verify.IsNotNull(peer, "TeachingTip automation peer");
            Log.Comment($"GetPattern(PatternInterface.Window) returns {(peer.GetPattern(PatternInterface.Window) == null ? "null" : "a provider")}");
            var window = peer as IWindowProvider;
            Verify.IsNotNull(window, "TeachingTipAutomationPeer should implement IWindowProvider");
            return window;
        }

        private static string GetCurrentStateName(TeachingTip tip, string groupName)
        {
            var container = VisualTreeUtils.FindVisualChildByName(tip, "Container");
            Verify.IsNotNull(container, "TeachingTip template root 'Container'");
            var group = VisualStateManager.GetVisualStateGroups(container).FirstOrDefault(g => g.Name == groupName);
            Verify.IsNotNull(group, "VisualStateGroup " + groupName);
            return group.CurrentState?.Name;
        }

        private static void VerifyEffectivePlacement(TeachingTip tip, TeachingTipPlacementMode placement)
        {
            Verify.AreEqual(placement, TeachingTipTestHooks.GetEffectivePlacement(tip), "Effective placement");
            Verify.AreEqual(placement.ToString(), GetCurrentStateName(tip, "PlacementStates"), "PlacementStates");
        }

        // The top edge highlight is split around a tail on the top edge (Bottom* placements); otherwise only the corner
        // radius and the side of the tail adjust it.
        private static void VerifyHighlightMarginsForTail(TeachingTip tip, TeachingTipPlacementMode placement)
        {
            Thickness topRight = tip.TemplateSettings.TopRightHighlightMargin;
            Thickness topLeft = tip.TemplateSettings.TopLeftHighlightMargin;
            CornerRadius radius = tip.CornerRadius;
            string what = $"{placement} highlight margins";

            switch (placement)
            {
                case TeachingTipPlacementMode.Top:
                case TeachingTipPlacementMode.TopLeft:
                case TeachingTipPlacementMode.TopRight:
                case TeachingTipPlacementMode.Center:
                    VerifyThickness(new Thickness(0), topRight, what + " (TopRight)");
                    VerifyThickness(new Thickness(radius.TopLeft - 1, 1, radius.TopRight - 1, 0), topLeft, what + " (TopLeft)");
                    break;
                case TeachingTipPlacementMode.Left:
                case TeachingTipPlacementMode.LeftTop:
                case TeachingTipPlacementMode.LeftBottom:
                    VerifyThickness(new Thickness(0), topRight, what + " (TopRight)");
                    VerifyThickness(new Thickness(radius.TopLeft - 1, 1, radius.TopRight - 2, 0), topLeft, what + " (TopLeft)");
                    break;
                case TeachingTipPlacementMode.Right:
                case TeachingTipPlacementMode.RightTop:
                case TeachingTipPlacementMode.RightBottom:
                    VerifyThickness(new Thickness(0), topRight, what + " (TopRight)");
                    VerifyThickness(new Thickness(radius.TopLeft - 2, 1, radius.TopRight - 1, 0), topLeft, what + " (TopLeft)");
                    break;
                default:
                    // Bottom, BottomLeft, BottomRight: the highlight stops at the tail from both corners.
                    VerifyOffset(radius.TopLeft - 1, topLeft.Left, what + " TopLeft.Left");
                    VerifyOffset(0, topLeft.Top, what + " TopLeft.Top");
                    VerifyOffset(0, topRight.Top, what + " TopRight.Top");
                    VerifyOffset(radius.TopRight - 1, topRight.Right, what + " TopRight.Right");
                    Verify.IsTrue(topRight.Left > 0 && topLeft.Right > 0, what + ": both highlight segments end at the tail");
                    if (placement == TeachingTipPlacementMode.Bottom)
                    {
                        VerifyOffset(topLeft.Right, topRight.Left, what + ": centered tail splits the highlight evenly");
                    }
                    else if (placement == TeachingTipPlacementMode.BottomLeft)
                    {
                        Verify.IsTrue(topRight.Left > topLeft.Right, what + ": tail near the right edge");
                    }
                    else
                    {
                        Verify.IsTrue(topRight.Left < topLeft.Right, what + ": tail near the left edge");
                    }
                    break;
            }
        }

        private static void VerifyOffset(double expected, double actual, string what)
        {
            Verify.IsTrue(Math.Abs(expected - actual) <= OffsetTolerance, $"{what}: expected {expected}, actual {actual}");
        }

        private static void VerifyThickness(Thickness expected, Thickness actual, string what)
        {
            Verify.IsTrue(
                Math.Abs(expected.Left - actual.Left) <= OffsetTolerance &&
                Math.Abs(expected.Top - actual.Top) <= OffsetTolerance &&
                Math.Abs(expected.Right - actual.Right) <= OffsetTolerance &&
                Math.Abs(expected.Bottom - actual.Bottom) <= OffsetTolerance,
                $"{what}: expected {expected}, actual {actual}");
        }

        private void VerifyFlushEdgePlacement(TeachingTipPlacementMode placement, Func<Rect, Rect> windowFromTarget, Func<Rect, Rect, double> edgeSpace)
        {
            Log.Comment($"PreferredPlacement = {placement} with the target flush against that window edge");
            var tip = CreateTargetedTip(placement, t => t.ShouldConstrainToRootBounds = false);
            RunOnUIThread.Execute(() =>
            {
                Rect target = GetBoundsInRoot(tip.Target);
                Rect window = windowFromTarget(target);
                Verify.AreEqual(0.0, edgeSpace(target, window), "Precondition: the target is exactly flush with the window edge");
                TeachingTipTestHooks.SetReturnTopForOutOfWindowPlacement(tip, false);
                TeachingTipTestHooks.SetUseTestWindowBounds(tip, true);
                TeachingTipTestHooks.SetTestWindowBounds(tip, window);
                TeachingTipTestHooks.SetUseTestScreenBounds(tip, true);
                TeachingTipTestHooks.SetTestScreenBounds(tip, new Rect(target.X - AmpleSpace, target.Y - AmpleSpace, target.Width + 2 * AmpleSpace, target.Height + 2 * AmpleSpace));
            });
            OpenTip(tip);
            RunOnUIThread.Execute(() => Verify.AreEqual(placement, TeachingTipTestHooks.GetEffectivePlacement(tip), $"Effective placement for a target flush with the {placement} edge"));
        }

        private void VerifyUntargetedExactFit(bool constrainToRootBounds)
        {
            var tip = CreateUntargetedTip(t => t.ShouldConstrainToRootBounds = constrainToRootBounds);
            OpenTip(tip);

            double tipWidth = 0;
            double tipHeight = 0;
            int closedCount = 0;
            var events = new List<string>();
            var closedRaised = new ManualResetEvent(false);
            RunOnUIThread.Execute(() =>
            {
                var grid = GetTailOcclusionGrid(tip);
                tipWidth = grid.ActualWidth;
                tipHeight = grid.ActualHeight;
                tip.Closing += (s, a) => events.Add("Closing:" + a.Reason);
                tip.Closed += (s, a) => { closedCount++; events.Add("Closed:" + a.Reason); closedRaised.Set(); };
            });
            Verify.AreEqual((double)(float)tipHeight, tipHeight, "Precondition: the tip height must be exactly representable in bounds");

            // Changing HeroContentPlacement makes the open tip re-evaluate its placement against the current bounds
            // (TeachingTip::OnHeroContentPlacementChanged). One pixel more than the tip's height still fits.
            RunOnUIThread.Execute(() =>
            {
                UseFitBounds(tip, constrainToRootBounds, new Rect(0, 0, tipWidth + 100, tipHeight + 1));
                tip.HeroContentPlacement = TeachingTipHeroContentPlacementMode.Top;
            });
            IdleSynchronizer.Wait();
            Verify.IsTrue(WaitForTipState(tip, isOpen: true), "A tip one pixel shorter than the available space should stay open");
            RunOnUIThread.Execute(() => Verify.AreEqual(0, closedCount, "A tip that fits must not close"));

            // Exactly the tip's height does not fit: the tip closes.
            RunOnUIThread.Execute(() =>
            {
                UseFitBounds(tip, constrainToRootBounds, new Rect(0, 0, tipWidth + 100, tipHeight));
                tip.HeroContentPlacement = TeachingTipHeroContentPlacementMode.Bottom;
            });

            Verify.IsTrue(closedRaised.WaitOne(DefaultWaitTimeInMS), "Closed should be raised for a tip exactly as tall as the available space");
            Verify.IsTrue(WaitForTipState(tip, isOpen: false), "TeachingTip should settle closed");
            RunOnUIThread.Execute(() =>
            {
                Verify.AreEqual("Closing:Programmatic,Closed:Programmatic", string.Join(",", events));
                Verify.IsFalse(tip.IsOpen);
                Verify.IsFalse(TeachingTipTestHooks.GetPopup(tip).IsOpen);
            });
        }

        // Constrained tips fit inside the (simulated) window; out-of-root tips fit inside the (simulated) screen once the
        // out-of-root "always fits" shortcut is turned off.
        private static void UseFitBounds(TeachingTip tip, bool constrainToRootBounds, Rect bounds)
        {
            if (constrainToRootBounds)
            {
                TeachingTipTestHooks.SetUseTestWindowBounds(tip, true);
                TeachingTipTestHooks.SetTestWindowBounds(tip, bounds);
            }
            else
            {
                TeachingTipTestHooks.SetReturnTopForOutOfWindowPlacement(tip, false);
                TeachingTipTestHooks.SetUseTestScreenBounds(tip, true);
                TeachingTipTestHooks.SetTestScreenBounds(tip, bounds);
            }
        }

        private static Grid GetTailOcclusionGrid(TeachingTip tip)
        {
            var grid = VisualTreeUtils.FindVisualChildByName(TeachingTipTestHooks.GetPopup(tip).Child, "TailOcclusionGrid") as Grid;
            Verify.IsNotNull(grid, "TailOcclusionGrid");
            return grid;
        }

        // Distance from the tip edge to the center of the tail, from the template's tail margin columns and tail size.
        private static double GetMinimumTipEdgeToTailCenter(TeachingTip tip)
        {
            var popupChild = TeachingTipTestHooks.GetPopup(tip).Child;
            var tailOcclusionGrid = VisualTreeUtils.FindVisualChildByName(popupChild, "TailOcclusionGrid") as Grid;
            Verify.IsNotNull(tailOcclusionGrid, "TailOcclusionGrid");
            Verify.IsTrue(tailOcclusionGrid.ColumnDefinitions.Count > 1, "TailOcclusionGrid tail margin columns");
            var tailPolygon = GetTailPolygon(tip);
            return tailOcclusionGrid.ColumnDefinitions[0].ActualWidth + tailOcclusionGrid.ColumnDefinitions[1].ActualWidth +
                Math.Max(tailPolygon.ActualHeight, tailPolygon.ActualWidth) / 2;
        }
    }
}
