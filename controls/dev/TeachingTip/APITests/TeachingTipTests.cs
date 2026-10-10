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

                // Reposition once the tail is measured (see TargetedTipIsPositionedAtPreferredCornerPlacement, concern C9) so the
                // template settings are computed from settled sizes.
                RunOnUIThread.Execute(() => tip.PlacementMargin = new Thickness(1));
                IdleSynchronizer.Wait();

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
                    Rect tipRect = GetTipRect(tip);
                    var expected = GetExpectedUntargetedOffset(tip.XamlRoot.Size, new Size(tipRect.Width, tipRect.Height), placement, margin);

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
            if (AnimationsAreDisabled())
            {
                Log.Warning("Test is disabled when animations are turned off: the tip becomes idle as soon as it opens.");
                return;
            }

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
        // Repositioning an open tip
        // ------------------------------------------------------------------------------------------------

        [TestMethod]
        public void ChangingPreferredPlacementRepositionsOpenUntargetedTip()
        {
            var tip = CreateUntargetedTip();
            OpenTip(tip);

            RunOnUIThread.Execute(() =>
            {
                Rect initial = GetTipRect(tip);
                var tipSize = new Size(initial.Width, initial.Height);
                var bottom = GetExpectedUntargetedOffset(tip.XamlRoot.Size, tipSize, TeachingTipPlacementMode.Auto, new Thickness(0));
                VerifyOffset(bottom.x, initial.X, "Initial HorizontalOffset");
                VerifyOffset(bottom.y, initial.Y, "Initial VerticalOffset");

                // Changing PreferredPlacement while the tip is open repositions it synchronously (TeachingTip::OnPropertyChanged ->
                // PositionPopup). Only the position is checked: the effective placement of an open tip is not re-evaluated (concern C7).
                foreach (var placement in new[] { TeachingTipPlacementMode.Top, TeachingTipPlacementMode.Left, TeachingTipPlacementMode.Center, TeachingTipPlacementMode.RightBottom })
                {
                    tip.PreferredPlacement = placement;
                    var expected = GetExpectedUntargetedOffset(tip.XamlRoot.Size, tipSize, placement, new Thickness(0));
                    Rect actual = GetTipRect(tip);
                    VerifyOffset(expected.x, actual.X, $"{placement} HorizontalOffset after changing PreferredPlacement while open");
                    VerifyOffset(expected.y, actual.Y, $"{placement} VerticalOffset after changing PreferredPlacement while open");
                    Verify.IsTrue(tip.IsOpen, "The tip stays open");
                }
            });
        }

        [TestMethod]
        [TestProperty("IsolationLevel", "Method")] // Resizes the test app window, so it runs in its own app instance.
        public void OpenTipsRepositionWhenWindowIsResized()
        {
            Microsoft.UI.Windowing.AppWindow appWindow = null;
            Microsoft.UI.Windowing.OverlappedPresenter presenter = null;
            global::Windows.Graphics.SizeInt32 maximizedSize = default;
            RunOnUIThread.Execute(() =>
            {
                appWindow = MUXControlsTestApp.App.CurrentWindow.AppWindow;
                presenter = appWindow.Presenter as Microsoft.UI.Windowing.OverlappedPresenter;
                maximizedSize = appWindow.Size;
            });
            Verify.IsNotNull(presenter, "The test window should use an OverlappedPresenter");

            try
            {
                ResizeTestWindow(appWindow, presenter, maximizedSize, 0.9);

                Button target = null;
                TeachingTip targeted = null;
                TeachingTip untargeted = null;
                RunOnUIThread.Execute(() =>
                {
                    target = new Button
                    {
                        Content = "Target",
                        Width = TargetSize,
                        Height = TargetSize,
                        HorizontalAlignment = HorizontalAlignment.Center,
                        VerticalAlignment = VerticalAlignment.Center,
                    };
                    targeted = new TeachingTip { Title = "Targeted", Content = new Border { Width = 200, Height = 150 }, Target = target, PreferredPlacement = TeachingTipPlacementMode.Bottom };
                    untargeted = new TeachingTip { Title = "Untargeted", Content = new Border { Width = 200, Height = 150 } };
                    var root = new Grid();
                    root.Children.Add(target);
                    root.Children.Add(targeted);
                    root.Children.Add(untargeted);
                    Content = root;
                    Content.UpdateLayout();
                });
                OpenTip(targeted);
                OpenTip(untargeted);

                Point targetedBefore = default;
                Point untargetedBefore = default;
                RunOnUIThread.Execute(() =>
                {
                    VerifyEffectivePlacement(targeted, TeachingTipPlacementMode.Bottom);
                    targetedBefore = GetPopupOffset(targeted);
                    untargetedBefore = GetPopupOffset(untargeted);
                });

                ResizeTestWindow(appWindow, presenter, maximizedSize, 0.75);

                // XamlRoot.Changed repositions open tips on a later rendering tick and there is no public completion event, so wait
                // for both popups to move; the exact positions are then checked once.
                Verify.IsTrue(
                    WaitForUIState(() => GetPopupOffset(targeted) != targetedBefore && GetPopupOffset(untargeted) != untargetedBefore),
                    "Both open tips should be repositioned after the window is resized");

                RunOnUIThread.Execute(() =>
                {
                    // The targeted tip is still centered below its target, which moved with the window.
                    Rect targetBounds = GetBoundsInRoot(target);
                    Rect targetedRect = GetTipRect(targeted);
                    VerifyOffset(targetBounds.X + targetBounds.Width / 2 - targetedRect.Width / 2, targetedRect.X, "Targeted HorizontalOffset after resize");
                    VerifyOffset(targetBounds.Bottom, targetedRect.Y, "Targeted VerticalOffset after resize");

                    // The untargeted tip is at the bottom center of the resized window.
                    Rect untargetedRect = GetTipRect(untargeted);
                    var expected = GetExpectedUntargetedOffset(untargeted.XamlRoot.Size, new Size(untargetedRect.Width, untargetedRect.Height), TeachingTipPlacementMode.Auto, new Thickness(0));
                    VerifyOffset(expected.x, untargetedRect.X, "Untargeted HorizontalOffset after resize");
                    VerifyOffset(expected.y, untargetedRect.Y, "Untargeted VerticalOffset after resize");

                    Verify.IsTrue(targeted.IsOpen && untargeted.IsOpen, "Resizing the window does not close the tips");
                });
            }
            finally
            {
                RunOnUIThread.Execute(() => presenter.Maximize());
                Verify.IsTrue(
                    WaitForUIState(() => presenter.State == Microsoft.UI.Windowing.OverlappedPresenterState.Maximized),
                    "The test window should be maximized again");
                Log.Comment("Test window restored to Maximized");
            }
        }

        // ------------------------------------------------------------------------------------------------
        // Animation, shadow and elevation test hooks
        // ------------------------------------------------------------------------------------------------

        [TestMethod]
        public void CustomEasingFunctionsDriveOpenAndClose()
        {
            var tip = CreateUntargetedTip();
            int closedCount = 0;
            var closedRaised = new ManualResetEvent(false);
            RunOnUIThread.Execute(() =>
            {
                // The hooks replace the easing functions and rebuild the expand and contract animations around them.
                var compositor = CompositionTarget.GetCompositorForCurrentThread();
                TeachingTipTestHooks.SetExpandEasingFunction(tip, compositor.CreateLinearEasingFunction());
                TeachingTipTestHooks.SetContractEasingFunction(tip, compositor.CreateLinearEasingFunction());
                tip.Closed += (s, a) => { closedCount++; closedRaised.Set(); };
            });

            // The rebuilt animations must still run to completion: the tip becomes idle both open and closed.
            OpenTip(tip);
            CloseTip(tip);
            Verify.IsTrue(closedRaised.WaitOne(DefaultWaitTimeInMS), "Closed should be raised");
            RunOnUIThread.Execute(() => Verify.AreEqual(1, closedCount, "Closed count"));
        }

        [TestMethod]
        public void AnimationDurationHooksApplyToExistingAnimations()
        {
            if (AnimationsAreDisabled())
            {
                Log.Warning("Test is disabled when animations are turned off: the tip opens and closes without its expand/contract animations.");
                return;
            }

            var tip = CreateUntargetedTip();

            // The first open and close create the expand and contract animations; the hooks must update those existing animations.
            OpenTip(tip);
            CloseTip(tip);
            var duration = TimeSpan.FromMilliseconds(1500);
            var minimum = TimeSpan.FromMilliseconds(1000);
            RunOnUIThread.Execute(() =>
            {
                TeachingTipTestHooks.SetExpandAnimationDuration(tip, duration);
                TeachingTipTestHooks.SetContractAnimationDuration(tip, duration);
            });

            // The tip becomes idle only when its animation completes, so it cannot finish opening or closing sooner than the new
            // duration (the defaults are 300ms and 200ms). Only this lower bound is asserted, so machine speed cannot fail the test.
            var stopwatch = Stopwatch.StartNew();
            RunOnUIThread.Execute(() => tip.IsOpen = true);
            Verify.IsTrue(WaitForTipState(tip, isOpen: true), "TeachingTip should open and become idle");
            TimeSpan openTime = stopwatch.Elapsed;
            Verify.IsTrue(openTime >= minimum, $"Opening took {openTime.TotalMilliseconds}ms; the {duration.TotalMilliseconds}ms expand animation should apply");

            stopwatch.Restart();
            RunOnUIThread.Execute(() => tip.IsOpen = false);
            Verify.IsTrue(WaitForTipState(tip, isOpen: false), "TeachingTip should close and become idle");
            TimeSpan closeTime = stopwatch.Elapsed;
            Verify.IsTrue(closeTime >= minimum, $"Closing took {closeTime.TotalMilliseconds}ms; the {duration.TotalMilliseconds}ms contract animation should apply");
        }

        [TestMethod]
        public void TipShadowHookRemovesAndRestoresThemeShadow()
        {
            var tip = CreateUntargetedTip();
            OpenTip(tip);

            RunOnUIThread.Execute(() =>
            {
                var contentRootGrid = GetContentRootGrid(tip);
                Verify.IsTrue(contentRootGrid.Shadow is ThemeShadow, "An open tip has a ThemeShadow by default");

                TeachingTipTestHooks.SetTipShouldHaveShadow(tip, false);
                Verify.IsNull(contentRootGrid.Shadow, "Turning the shadow off removes it");

                TeachingTipTestHooks.SetTipShouldHaveShadow(tip, true);
                Verify.IsTrue(contentRootGrid.Shadow is ThemeShadow, "Turning the shadow back on restores a ThemeShadow");
                Verify.AreEqual(32f, contentRootGrid.Translation.Z, "The shadowed content is raised to the default content elevation");
            });
        }

        [TestMethod]
        public void ContentElevationHookRaisesContentRootGrid()
        {
            var tip = CreateUntargetedTip();
            OpenTip(tip);

            RunOnUIThread.Execute(() =>
            {
                var contentRootGrid = GetContentRootGrid(tip);
                var before = contentRootGrid.Translation;
                TeachingTipTestHooks.SetContentElevation(tip, 12);
                var after = contentRootGrid.Translation;
                Verify.AreEqual(12f, after.Z, "Content elevation");
                Verify.AreEqual(before.X, after.X, "Translation.X is unchanged");
                Verify.AreEqual(before.Y, after.Y, "Translation.Y is unchanged");
            });

            // The new elevation is kept when the tip is closed and opened again.
            CloseTip(tip);
            OpenTip(tip);
            RunOnUIThread.Execute(() => Verify.AreEqual(12f, GetContentRootGrid(tip).Translation.Z, "Content elevation after reopening"));
        }

        [TestMethod]
        public void TailElevationHookRaisesTailPolygon()
        {
            var tip = CreateTargetedTip(TeachingTipPlacementMode.Bottom);
            UseWindowBoundsAroundTarget(tip, AmpleSpace, AmpleSpace, AmpleSpace, AmpleSpace);
            OpenTip(tip);

            RunOnUIThread.Execute(() =>
            {
                var tailPolygon = GetTailPolygon(tip);
                var before = tailPolygon.Translation;
                TeachingTipTestHooks.SetTailElevation(tip, 12);
                var after = tailPolygon.Translation;
                Verify.AreEqual(12f, after.Z, "Tail elevation");
                Verify.AreEqual(before.X, after.X, "Translation.X is unchanged");
                Verify.AreEqual(before.Y, after.Y, "Translation.Y is unchanged");
            });
        }

        // ------------------------------------------------------------------------------------------------
        // Test hook getters and events
        // ------------------------------------------------------------------------------------------------

        [TestMethod]
        public void TitleAndSubtitleVisibilityHooksNeedTheTemplate()
        {
            TeachingTip tip = null;
            RunOnUIThread.Execute(() =>
            {
                tip = new TeachingTip { Title = "Title", Subtitle = "Subtitle" };

                // Without a template there are no title and subtitle parts, so both report Collapsed despite the non-empty text.
                Verify.AreEqual(Visibility.Collapsed, TeachingTipTestHooks.GetTitleVisibility(tip), "Title visibility before the template is applied");
                Verify.AreEqual(Visibility.Collapsed, TeachingTipTestHooks.GetSubtitleVisibility(tip), "Subtitle visibility before the template is applied");

                Content = tip;
                Content.UpdateLayout();
            });
            IdleSynchronizer.Wait();

            RunOnUIThread.Execute(() =>
            {
                // Once the template is applied the same hooks report the parts, which are visible for non-empty text.
                Verify.AreEqual(Visibility.Visible, TeachingTipTestHooks.GetTitleVisibility(tip), "Title visibility after the template is applied");
                Verify.AreEqual(Visibility.Visible, TeachingTipTestHooks.GetSubtitleVisibility(tip), "Subtitle visibility after the template is applied");
            });
        }

        [TestMethod]
        public void TestHookGettersReturnDefaultsForNullTip()
        {
            RunOnUIThread.Execute(() =>
            {
                Verify.IsTrue(TeachingTipTestHooks.GetIsIdle(null), "GetIsIdle");
                Verify.AreEqual(TeachingTipPlacementMode.Auto, TeachingTipTestHooks.GetEffectivePlacement(null), "GetEffectivePlacement");
                Verify.AreEqual(TeachingTipHeroContentPlacementMode.Auto, TeachingTipTestHooks.GetEffectiveHeroContentPlacement(null), "GetEffectiveHeroContentPlacement");
                Verify.AreEqual(0.0, TeachingTipTestHooks.GetVerticalOffset(null), "GetVerticalOffset");
                Verify.AreEqual(0.0, TeachingTipTestHooks.GetHorizontalOffset(null), "GetHorizontalOffset");
                Verify.AreEqual(Visibility.Collapsed, TeachingTipTestHooks.GetTitleVisibility(null), "GetTitleVisibility");
                Verify.AreEqual(Visibility.Collapsed, TeachingTipTestHooks.GetSubtitleVisibility(null), "GetSubtitleVisibility");
                Verify.IsNull(TeachingTipTestHooks.GetPopup(null), "GetPopup");
            });
        }

        [TestMethod]
        public void TestHookEventsStopNotifyingAfterUnsubscribe()
        {
            var tip = CreateTargetedTip(TeachingTipPlacementMode.Bottom, t => t.HeroContent = new Border { Width = 200, Height = 40 });
            UseWindowBoundsAroundTarget(tip, AmpleSpace, AmpleSpace, AmpleSpace, AmpleSpace);

            var notifications = new List<(string name, object sender)>();
            var handlers = new Dictionary<string, TypedEventHandler<TeachingTip, object>>();
            foreach (var name in new[] { "OpenedStatusChanged", "IdleStatusChanged", "EffectivePlacementChanged", "EffectiveHeroContentPlacementChanged", "OffsetChanged", "TitleVisibilityChanged", "SubtitleVisibilityChanged" })
            {
                handlers[name] = (s, a) => { lock (notifications) { notifications.Add((name, s)); } };
            }

            void Subscribe()
            {
                TeachingTipTestHooks.OpenedStatusChanged += handlers["OpenedStatusChanged"];
                TeachingTipTestHooks.IdleStatusChanged += handlers["IdleStatusChanged"];
                TeachingTipTestHooks.EffectivePlacementChanged += handlers["EffectivePlacementChanged"];
                TeachingTipTestHooks.EffectiveHeroContentPlacementChanged += handlers["EffectiveHeroContentPlacementChanged"];
                TeachingTipTestHooks.OffsetChanged += handlers["OffsetChanged"];
                TeachingTipTestHooks.TitleVisibilityChanged += handlers["TitleVisibilityChanged"];
                TeachingTipTestHooks.SubtitleVisibilityChanged += handlers["SubtitleVisibilityChanged"];
            }

            // Each handler is the event's only subscriber, so removing it also removes the native subscription.
            void Unsubscribe()
            {
                TeachingTipTestHooks.OpenedStatusChanged -= handlers["OpenedStatusChanged"];
                TeachingTipTestHooks.IdleStatusChanged -= handlers["IdleStatusChanged"];
                TeachingTipTestHooks.EffectivePlacementChanged -= handlers["EffectivePlacementChanged"];
                TeachingTipTestHooks.EffectiveHeroContentPlacementChanged -= handlers["EffectiveHeroContentPlacementChanged"];
                TeachingTipTestHooks.OffsetChanged -= handlers["OffsetChanged"];
                TeachingTipTestHooks.TitleVisibilityChanged -= handlers["TitleVisibilityChanged"];
                TeachingTipTestHooks.SubtitleVisibilityChanged -= handlers["SubtitleVisibilityChanged"];
            }

            // Changing the title and subtitle, then opening a Bottom tip with Auto hero placement and closing it, changes every
            // state that the hook events report on.
            void ChangeEveryReportedState(string round)
            {
                RunOnUIThread.Execute(() =>
                {
                    tip.Title = "Title " + round;
                    tip.Subtitle = "Subtitle " + round;
                });
                OpenTip(tip);
                CloseTip(tip);
            }

            bool subscribed = false;
            try
            {
                RunOnUIThread.Execute(() => { Subscribe(); subscribed = true; });
                ChangeEveryReportedState("subscribed");

                if (PlatformConfiguration.IsDebugBuildConfiguration())
                {
                    // TeachingTip raises the hook events only in debug (DBG) builds.
                    List<string> raisedForTip;
                    lock (notifications)
                    {
                        raisedForTip = notifications.Where(n => ReferenceEquals(n.sender, tip)).Select(n => n.name).Distinct().ToList();
                    }
                    foreach (var name in handlers.Keys)
                    {
                        Verify.IsTrue(raisedForTip.Contains(name), $"{name} should notify its subscriber with the tip as sender");
                    }
                }
                else
                {
                    Log.Comment("Release build: TeachingTip compiles the hook notifications out, so only unsubscription is verified.");
                }

                RunOnUIThread.Execute(() => { Unsubscribe(); subscribed = false; });
                lock (notifications)
                {
                    notifications.Clear();
                }

                ChangeEveryReportedState("unsubscribed");
                lock (notifications)
                {
                    Verify.AreEqual(0, notifications.Count, "Removed hook handlers must not be invoked: " + string.Join(",", notifications.Select(n => n.name)));
                }
            }
            finally
            {
                if (subscribed)
                {
                    RunOnUIThread.Execute(Unsubscribe);
                }
            }
        }

        // ------------------------------------------------------------------------------------------------
        // Events
        // ------------------------------------------------------------------------------------------------

        [TestMethod]
        public void RemovedEventHandlersAreNotInvoked()
        {
            var tip = CreateUntargetedTip(t =>
            {
                t.ActionButtonContent = "Action";
                t.CloseButtonContent = "Close";
            });
            OpenTip(tip);

            var calls = new List<string>();
            var closedRaised = new ManualResetEvent(false);
            RunOnUIThread.Execute(() =>
            {
                // Each removed handler is its event's only handler, so removing it also removes the native subscription.
                TypedEventHandler<TeachingTip, object> removedActionButtonClick = (s, a) => calls.Add("removed ActionButtonClick");
                TypedEventHandler<TeachingTip, object> removedCloseButtonClick = (s, a) => calls.Add("removed CloseButtonClick");
                TypedEventHandler<TeachingTip, TeachingTipClosingEventArgs> removedClosing = (s, a) => calls.Add("removed Closing");
                TypedEventHandler<TeachingTip, TeachingTipClosedEventArgs> removedClosed = (s, a) => calls.Add("removed Closed");
                tip.ActionButtonClick += removedActionButtonClick;
                tip.ActionButtonClick -= removedActionButtonClick;
                tip.CloseButtonClick += removedCloseButtonClick;
                tip.CloseButtonClick -= removedCloseButtonClick;
                tip.Closing += removedClosing;
                tip.Closing -= removedClosing;
                tip.Closed += removedClosed;
                tip.Closed -= removedClosed;

                // Handlers added afterwards show that the events are still raised.
                tip.ActionButtonClick += (s, a) => calls.Add("ActionButtonClick");
                tip.CloseButtonClick += (s, a) => calls.Add("CloseButtonClick");
                tip.Closing += (s, a) => calls.Add("Closing");
                tip.Closed += (s, a) => { calls.Add("Closed"); closedRaised.Set(); };

                InvokeButton(GetTemplateButton(tip, "ActionButton"));
                InvokeButton(GetTemplateButton(tip, "CloseButton"));
            });

            Verify.IsTrue(WaitForTipState(tip, isOpen: false), "The close button should close the tip");
            Verify.IsTrue(closedRaised.WaitOne(DefaultWaitTimeInMS), "Closed should be raised");
            RunOnUIThread.Execute(() =>
            {
                Verify.AreEqual("ActionButtonClick,CloseButtonClick,Closing,Closed", string.Join(",", calls));
            });
        }

#if MUX_PRERELEASE
        [TestMethod]
        public void OpenedIsRaisedOnceTheTipHasOpened()
        {
            var tip = CreateUntargetedTip();
            var observations = new List<string>();
            var openedRaised = new ManualResetEvent(false);
            TypedEventHandler<TeachingTip, TeachingTipOpenedEventArgs> handler = (s, a) =>
            {
                // Opened follows the expand animation, so the tip is already idle with its popup open.
                var popup = TeachingTipTestHooks.GetPopup(s);
                observations.Add($"sender={ReferenceEquals(s, tip)},args={a != null},idle={TeachingTipTestHooks.GetIsIdle(s)},popupOpen={popup != null && popup.IsOpen}");
                openedRaised.Set();
            };

            RunOnUIThread.Execute(() =>
            {
                tip.Opened += handler;
                tip.IsOpen = true;
            });
            Verify.IsTrue(openedRaised.WaitOne(DefaultWaitTimeInMS), "Opened should be raised");
            Verify.IsTrue(WaitForTipState(tip, isOpen: true), "TeachingTip should open and become idle");
            RunOnUIThread.Execute(() =>
            {
                Verify.AreEqual("sender=True,args=True,idle=True,popupOpen=True", string.Join(";", observations), "Opened is raised exactly once per open");
                tip.Opened -= handler;
            });
            CloseTip(tip);

            var laterHandlerRaised = new ManualResetEvent(false);
            RunOnUIThread.Execute(() => tip.Opened += (s, a) => laterHandlerRaised.Set());
            OpenTip(tip);
            Verify.IsTrue(laterHandlerRaised.WaitOne(DefaultWaitTimeInMS), "Opened should be raised again when the tip reopens");
            RunOnUIThread.Execute(() => Verify.AreEqual(1, observations.Count, "A removed Opened handler must not be invoked"));
        }
#endif

        [TestMethod]
        public void ClosingCancelCanBeSetAndClearedInHandler()
        {
            var tip = CreateUntargetedTip();
            OpenTip(tip);

            var cancelValues = new List<bool>();
            var closedReasons = new List<TeachingTipCloseReason>();
            var closedRaised = new ManualResetEvent(false);
            RunOnUIThread.Execute(() =>
            {
                tip.Closing += (s, a) =>
                {
                    cancelValues.Add(a.Cancel);
                    a.Cancel = true;
                    cancelValues.Add(a.Cancel);
                    a.Cancel = false;
                    cancelValues.Add(a.Cancel);
                };
                tip.Closed += (s, a) => { closedReasons.Add(a.Reason); closedRaised.Set(); };
                tip.IsOpen = false;
            });

            // Cancel ends up false, so the close completes normally. (Leaving Cancel set is not covered: see concern C5.)
            Verify.IsTrue(closedRaised.WaitOne(DefaultWaitTimeInMS), "Closed should be raised");
            Verify.IsTrue(WaitForTipState(tip, isOpen: false), "The tip should close");
            RunOnUIThread.Execute(() =>
            {
                Verify.AreEqual("False,True,False", string.Join(",", cancelValues), "Cancel values read back in the Closing handler");
                Verify.AreEqual(1, closedReasons.Count, "Closed count");
                Verify.AreEqual(TeachingTipCloseReason.Programmatic, closedReasons[0]);
            });
        }

        // ------------------------------------------------------------------------------------------------
        // Automation peer, dependency properties and activation factories
        // ------------------------------------------------------------------------------------------------

        [TestMethod]
        public void AutomationPeerCanBeCreatedForTip()
        {
            var tip = CreateUntargetedTip();
            RunOnUIThread.Execute(() =>
            {
                var peer = new TeachingTipAutomationPeer(tip);
                Verify.AreSame(tip, peer.Owner, "Owner");
                Verify.AreEqual("Microsoft.UI.Xaml.Controls.TeachingTip", peer.GetClassName(), "ClassName");
                Verify.IsTrue(peer is IWindowProvider, "The peer implements IWindowProvider");
                Verify.AreEqual(AutomationControlType.Pane, peer.GetAutomationControlType(), "A tip without light dismiss is a pane");

                tip.IsLightDismissEnabled = true;
                Verify.AreEqual(AutomationControlType.Window, peer.GetAutomationControlType(), "A light-dismiss tip is a window");
            });
        }

        [TestMethod]
        public void DependencyPropertyIdentifiersMatchProperties()
        {
            TeachingTip tip = null;
            RunOnUIThread.Execute(() =>
            {
                tip = new TeachingTip();
                Content = tip;
                Content.UpdateLayout();
            });

            RunOnUIThread.Execute(() =>
            {
                var identifiers = new[]
                {
                    TeachingTip.IsOpenProperty, TeachingTip.TargetProperty, TeachingTip.TailVisibilityProperty, TeachingTip.TitleProperty,
                    TeachingTip.SubtitleProperty, TeachingTip.ActionButtonContentProperty, TeachingTip.ActionButtonStyleProperty,
                    TeachingTip.ActionButtonCommandProperty, TeachingTip.ActionButtonCommandParameterProperty, TeachingTip.CloseButtonContentProperty,
                    TeachingTip.CloseButtonStyleProperty, TeachingTip.CloseButtonCommandProperty, TeachingTip.CloseButtonCommandParameterProperty,
                    TeachingTip.PlacementMarginProperty, TeachingTip.ShouldConstrainToRootBoundsProperty, TeachingTip.IsLightDismissEnabledProperty,
                    TeachingTip.PreferredPlacementProperty, TeachingTip.HeroContentPlacementProperty, TeachingTip.HeroContentProperty,
                    TeachingTip.IconSourceProperty, TeachingTip.TemplateSettingsProperty,
                };
                Verify.IsTrue(identifiers.All(p => p != null), "Every TeachingTip property identifier is available");
                Verify.AreEqual(identifiers.Length, identifiers.Distinct().Count(), "Every TeachingTip property has its own identifier");

                // IsOpen is only checked for its default: opening a tip is covered by the placement and lifecycle tests.
                Verify.AreEqual(false, (bool)tip.GetValue(TeachingTip.IsOpenProperty), "IsOpen default value");

                // Defaults come from TeachingTip.idl (MUX_DEFAULT_VALUE) or are the type's default.
                VerifyDependencyProperty(tip, TeachingTip.TitleProperty, "Title", "", "New title", () => tip.Title, v => tip.Title = v);
                VerifyDependencyProperty(tip, TeachingTip.SubtitleProperty, "Subtitle", "", "New subtitle", () => tip.Subtitle, v => tip.Subtitle = v);
                VerifyDependencyProperty(tip, TeachingTip.TargetProperty, "Target", (FrameworkElement)null, new Button(), () => tip.Target, v => tip.Target = v);
                VerifyDependencyProperty(tip, TeachingTip.TailVisibilityProperty, "TailVisibility", TeachingTipTailVisibility.Auto, TeachingTipTailVisibility.Collapsed, () => tip.TailVisibility, v => tip.TailVisibility = v);
                VerifyDependencyProperty<object>(tip, TeachingTip.ActionButtonContentProperty, "ActionButtonContent", null, "Action", () => tip.ActionButtonContent, v => tip.ActionButtonContent = v);
                // The default TeachingTip style sets both button styles to DefaultButtonStyle, so ClearValue returns to that style value.
                var defaultButtonStyle = MUXControlsTestApp.App.Current.Resources["DefaultButtonStyle"] as Style;
                Verify.IsNotNull(defaultButtonStyle, "DefaultButtonStyle resource");
                VerifyDependencyProperty(tip, TeachingTip.ActionButtonStyleProperty, "ActionButtonStyle", defaultButtonStyle, new Style(typeof(Button)), () => tip.ActionButtonStyle, v => tip.ActionButtonStyle = v);
                VerifyDependencyProperty<ICommand>(tip, TeachingTip.ActionButtonCommandProperty, "ActionButtonCommand", null, new RecordingCommand(), () => tip.ActionButtonCommand, v => tip.ActionButtonCommand = v);
                VerifyDependencyProperty<object>(tip, TeachingTip.ActionButtonCommandParameterProperty, "ActionButtonCommandParameter", null, "ActionParameter", () => tip.ActionButtonCommandParameter, v => tip.ActionButtonCommandParameter = v);
                VerifyDependencyProperty<object>(tip, TeachingTip.CloseButtonContentProperty, "CloseButtonContent", null, "Close", () => tip.CloseButtonContent, v => tip.CloseButtonContent = v);
                VerifyDependencyProperty(tip, TeachingTip.CloseButtonStyleProperty, "CloseButtonStyle", defaultButtonStyle, new Style(typeof(Button)), () => tip.CloseButtonStyle, v => tip.CloseButtonStyle = v);
                VerifyDependencyProperty<ICommand>(tip, TeachingTip.CloseButtonCommandProperty, "CloseButtonCommand", null, new RecordingCommand(), () => tip.CloseButtonCommand, v => tip.CloseButtonCommand = v);
                VerifyDependencyProperty<object>(tip, TeachingTip.CloseButtonCommandParameterProperty, "CloseButtonCommandParameter", null, "CloseParameter", () => tip.CloseButtonCommandParameter, v => tip.CloseButtonCommandParameter = v);
                VerifyDependencyProperty(tip, TeachingTip.PlacementMarginProperty, "PlacementMargin", new Thickness(0), new Thickness(1, 2, 3, 4), () => tip.PlacementMargin, v => tip.PlacementMargin = v);
                VerifyDependencyProperty(tip, TeachingTip.ShouldConstrainToRootBoundsProperty, "ShouldConstrainToRootBounds", true, false, () => tip.ShouldConstrainToRootBounds, v => tip.ShouldConstrainToRootBounds = v);
                VerifyDependencyProperty(tip, TeachingTip.IsLightDismissEnabledProperty, "IsLightDismissEnabled", false, true, () => tip.IsLightDismissEnabled, v => tip.IsLightDismissEnabled = v);
                VerifyDependencyProperty(tip, TeachingTip.PreferredPlacementProperty, "PreferredPlacement", TeachingTipPlacementMode.Auto, TeachingTipPlacementMode.LeftBottom, () => tip.PreferredPlacement, v => tip.PreferredPlacement = v);
                VerifyDependencyProperty(tip, TeachingTip.HeroContentPlacementProperty, "HeroContentPlacement", TeachingTipHeroContentPlacementMode.Auto, TeachingTipHeroContentPlacementMode.Bottom, () => tip.HeroContentPlacement, v => tip.HeroContentPlacement = v);
                VerifyDependencyProperty(tip, TeachingTip.HeroContentProperty, "HeroContent", (UIElement)null, new Border(), () => tip.HeroContent, v => tip.HeroContent = v);
                VerifyDependencyProperty(tip, TeachingTip.IconSourceProperty, "IconSource", (IconSource)null, new SymbolIconSource { Symbol = Symbol.People }, () => tip.IconSource, v => tip.IconSource = v);

                // The tip creates its own TemplateSettings, so that object (not null) is the identifier's initial value.
                var ownSettings = tip.TemplateSettings;
                Verify.IsNotNull(ownSettings, "A tip creates its TemplateSettings");
                Verify.AreSame(ownSettings, tip.GetValue(TeachingTip.TemplateSettingsProperty), "TemplateSettings through its identifier");
                var otherSettings = new TeachingTipTemplateSettings();
                tip.SetValue(TeachingTip.TemplateSettingsProperty, otherSettings);
                Verify.AreSame(otherSettings, tip.TemplateSettings, "TemplateSettings set through its identifier");
                tip.SetValue(TeachingTip.TemplateSettingsProperty, ownSettings);
            });
        }

        [TestMethod]
        public void TemplateSettingsPropertyIdentifiersMatchProperties()
        {
            RunOnUIThread.Execute(() =>
            {
                var settings = new TeachingTipTemplateSettings();
                var identifiers = new[]
                {
                    TeachingTipTemplateSettings.TopRightHighlightMarginProperty,
                    TeachingTipTemplateSettings.TopLeftHighlightMarginProperty,
                    TeachingTipTemplateSettings.IconElementProperty,
                };
                Verify.IsTrue(identifiers.All(p => p != null), "Every TeachingTipTemplateSettings property identifier is available");
                Verify.AreEqual(identifiers.Length, identifiers.Distinct().Count(), "Every TeachingTipTemplateSettings property has its own identifier");

                VerifyDependencyProperty(settings, TeachingTipTemplateSettings.TopRightHighlightMarginProperty, "TopRightHighlightMargin", new Thickness(0), new Thickness(1, 2, 3, 4), () => settings.TopRightHighlightMargin, v => settings.TopRightHighlightMargin = v);
                VerifyDependencyProperty(settings, TeachingTipTemplateSettings.TopLeftHighlightMarginProperty, "TopLeftHighlightMargin", new Thickness(0), new Thickness(5, 6, 7, 8), () => settings.TopLeftHighlightMargin, v => settings.TopLeftHighlightMargin = v);
                VerifyDependencyProperty(settings, TeachingTipTemplateSettings.IconElementProperty, "IconElement", (IconElement)null, new SymbolIcon(Symbol.People), () => settings.IconElement, v => settings.IconElement = v);
            });
        }

        [TestMethod]
        public void ActivationFactoriesReportClassNamesAndRejectDefaultActivation()
        {
            var classNames = new[]
            {
                "Microsoft.UI.Xaml.Controls.TeachingTip",
                "Microsoft.UI.Xaml.Controls.TeachingTipTemplateSettings",
                "Microsoft.UI.Xaml.Automation.Peers.TeachingTipAutomationPeer",
                "Microsoft.UI.Private.Controls.TeachingTipTestHooks",
            };

            RunOnUIThread.Execute(() =>
            {
                foreach (var className in classNames)
                {
                    var factory = global::WinRT.ActivationFactory.Get(className);
                    Verify.IsNotNull(factory, className + " activation factory");
                    Verify.AreEqual(className, new global::WinRT.IInspectable(factory).GetRuntimeClassName(), className + " activation factory runtime class name");

                    // Composable classes (TeachingTip, TeachingTipTemplateSettings, TeachingTipAutomationPeer) and the static-only
                    // TeachingTipTestHooks are not default-activatable: IActivationFactory.ActivateInstance returns E_NOTIMPL.
                    int hresult = 0;
                    try
                    {
                        IntPtr instance = new global::ABI.WinRT.Interop.IActivationFactory(factory).ActivateInstance();
                        if (instance != IntPtr.Zero)
                        {
                            global::System.Runtime.InteropServices.Marshal.Release(instance);
                        }
                    }
                    catch (Exception e)
                    {
                        hresult = e.HResult;
                    }
                    Verify.AreEqual(E_NOTIMPL, hresult, className + " ActivateInstance HRESULT");
                }
            });
        }

        [TestMethod]
        public void EventArgsReportRuntimeClassNames()
        {
            var tip = CreateUntargetedTip();
            var classNames = new List<string>();
            var closedRaised = new ManualResetEvent(false);
            RunOnUIThread.Execute(() =>
            {
#if MUX_PRERELEASE
                tip.Opened += (s, a) => classNames.Add(GetRuntimeClassName(a));
#endif
                tip.Closing += (s, a) => classNames.Add(GetRuntimeClassName(a));
                tip.Closed += (s, a) => { classNames.Add(GetRuntimeClassName(a)); closedRaised.Set(); };
            });

            OpenTip(tip);
            CloseTip(tip);
            Verify.IsTrue(closedRaised.WaitOne(DefaultWaitTimeInMS), "Closed should be raised");

            var expected = new List<string>();
#if MUX_PRERELEASE
            expected.Add("Microsoft.UI.Xaml.Controls.TeachingTipOpenedEventArgs");
#endif
            expected.Add("Microsoft.UI.Xaml.Controls.TeachingTipClosingEventArgs");
            expected.Add("Microsoft.UI.Xaml.Controls.TeachingTipClosedEventArgs");
            RunOnUIThread.Execute(() => Verify.AreEqual(string.Join(",", expected), string.Join(",", classNames)));
        }

        // ------------------------------------------------------------------------------------------------
        // Custom template without the optional tail and button parts
        // ------------------------------------------------------------------------------------------------

        [TestMethod]
        [TestProperty("IsolationLevel", "Method")] // A custom template reaches missing-part fallbacks; isolated so a crash cannot affect other tests.
        public void UntargetedTipWithoutTailPartsOpensAndCloses()
        {
            var windowBounds = new Rect(0, 0, 800, 600);
            var tip = CreateUntargetedTip(t =>
            {
                t.Template = LoadTemplateWithoutTailParts();
                TeachingTipTestHooks.SetUseTestWindowBounds(t, true);
                TeachingTipTestHooks.SetTestWindowBounds(t, windowBounds);
            });

            var closedReasons = new List<TeachingTipCloseReason>();
            var closedRaised = new ManualResetEvent(false);
            RunOnUIThread.Execute(() => tip.Closed += (s, a) => { closedReasons.Add(a.Reason); closedRaised.Set(); });

            // Opening plays the expand animation on TailEdgeBorder and ContentRootGrid; OpenTip waits for it to finish.
            OpenTip(tip);
            RunOnUIThread.Execute(() =>
            {
                var popup = TeachingTipTestHooks.GetPopup(tip);
                Verify.AreEqual("TemplateRoot", (popup.Child as FrameworkElement)?.Name, "The popup shows the custom template's root");

                // An open tip is positioned when its TailOcclusionGrid is sized, which never happens without that part (observation
                // C10, not asserted). Changing PlacementMargin repositions the open tip synchronously (TeachingTip::OnPlacementMarginChanged).
                Log.Comment($"Offset after opening without TailOcclusionGrid: ({popup.HorizontalOffset}, {popup.VerticalOffset})");
                var margin = new Thickness(1);
                tip.PlacementMargin = margin;

                // Without a TailOcclusionGrid the tip is positioned as if it had no size: centered, 24px above the bottom window edge.
                var expected = GetExpectedUntargetedOffset(new Size(windowBounds.Width, windowBounds.Height), new Size(0, 0), TeachingTipPlacementMode.Auto, margin);
                VerifyOffset(expected.x, popup.HorizontalOffset, "HorizontalOffset of a tip without TailOcclusionGrid");
                VerifyOffset(expected.y, popup.VerticalOffset, "VerticalOffset of a tip without TailOcclusionGrid");
            });

            // Closing plays the contract animation on the same parts.
            CloseTip(tip);
            Verify.IsTrue(closedRaised.WaitOne(DefaultWaitTimeInMS), "Closed should be raised");
            RunOnUIThread.Execute(() =>
            {
                Verify.AreEqual(1, closedReasons.Count, "Closed count");
                Verify.AreEqual(TeachingTipCloseReason.Programmatic, closedReasons[0]);
            });
        }

        [TestMethod]
        [TestProperty("IsolationLevel", "Method")] // A custom template reaches missing-part fallbacks; isolated so a crash cannot affect other tests.
        public void TargetedTipWithoutTailPartsIsPositionedAsZeroSized()
        {
            foreach (var placement in new[] { TeachingTipPlacementMode.Top, TeachingTipPlacementMode.TopRight })
            {
                Log.Comment($"PreferredPlacement = {placement}");
                var tip = CreateTargetedTip(placement, t => t.Template = LoadTemplateWithoutTailParts());
                Rect target = UseWindowBoundsAroundTarget(tip, AmpleSpace, AmpleSpace, AmpleSpace, AmpleSpace);
                OpenTip(tip);

                RunOnUIThread.Execute(() =>
                {
                    // The open tip is only positioned once something repositions it (observation C10); PlacementMargin does so synchronously.
                    var popup = TeachingTipTestHooks.GetPopup(tip);
                    Log.Comment($"Offset after opening without TailOcclusionGrid: ({popup.HorizontalOffset}, {popup.VerticalOffset})");
                    const double margin = 1;
                    tip.PlacementMargin = new Thickness(margin);

                    Verify.AreEqual(placement, TeachingTipTestHooks.GetEffectivePlacement(tip), "Effective placement");

                    // Without TailOcclusionGrid and TailPolygon the tip size and tail distances fall back to 0, so the popup's
                    // origin is the target's top center, moved up by the margin, for both Top and TopRight.
                    VerifyOffset(target.X + target.Width / 2, popup.HorizontalOffset, $"{placement} HorizontalOffset");
                    VerifyOffset(target.Y - margin, popup.VerticalOffset, $"{placement} VerticalOffset");
                });

                CloseTip(tip);
            }
        }

        [TestMethod]
        [TestProperty("IsolationLevel", "Method")] // A custom template reaches missing-part fallbacks; isolated so a crash cannot affect other tests.
        public void TailGridWithOneColumnHasNoTailEdgeMargin()
        {
            var tip = CreateTargetedTip(TeachingTipPlacementMode.BottomRight, t => t.Template = LoadTemplateWithOneColumnTailGrid());
            Rect target = UseWindowBoundsAroundTarget(tip, AmpleSpace, AmpleSpace, AmpleSpace, AmpleSpace);
            OpenTip(tip);

            // Reposition once the open tip is measured so the offsets and template settings use settled sizes.
            const double margin = 1;
            RunOnUIThread.Execute(() => tip.PlacementMargin = new Thickness(margin));
            IdleSynchronizer.Wait();

            RunOnUIThread.Execute(() =>
            {
                Verify.AreEqual(TeachingTipPlacementMode.BottomRight, TeachingTipTestHooks.GetEffectivePlacement(tip), "Effective placement");
                Verify.AreEqual(1, GetTailOcclusionGrid(tip).ColumnDefinitions.Count, "Precondition: the TailOcclusionGrid has a single column");
                var tailPolygon = GetTailPolygon(tip);
                double tailLongSide = Math.Max(tailPolygon.ActualHeight, tailPolygon.ActualWidth) - 2 * TailOcclusionAmount;
                Verify.IsTrue(tailLongSide > 0, "Precondition: the tail polygon is measured");

                // With fewer than two tail-margin columns the tail edge margin and the tip-edge-to-tail-center distance are 0
                // (TeachingTip::MinimumTipEdgeToTailEdgeMargin and MinimumTipEdgeToTailCenter), so the BottomRight highlight
                // starts right after the tail and the popup's left edge is at the target's horizontal center.
                VerifyOffset(tailLongSide - 1, tip.TemplateSettings.TopRightHighlightMargin.Left, "BottomRight TopRightHighlightMargin.Left with no tail edge margin");
                var popup = TeachingTipTestHooks.GetPopup(tip);
                VerifyOffset(target.X + target.Width / 2, popup.HorizontalOffset, "BottomRight HorizontalOffset with no tail-center distance");
                VerifyOffset(target.Bottom + margin, popup.VerticalOffset, "BottomRight VerticalOffset");
            });

            CloseTip(tip);
        }

        [TestMethod]
        public void TopPlacementNeedsRoomForContentAndTailShortSide()
        {
            var tip = CreateTargetedTip(TeachingTipPlacementMode.Top);
            Rect target = UseWindowBoundsAroundTarget(tip, AmpleSpace, AmpleSpace, AmpleSpace, AmpleSpace);
            OpenTip(tip);

            double required = 0;
            RunOnUIThread.Execute(() =>
            {
                VerifyEffectivePlacement(tip, TeachingTipPlacementMode.Top);
                var tailPolygon = GetTailPolygon(tip);
                double tailShortSide = Math.Min(tailPolygon.ActualHeight, tailPolygon.ActualWidth) - TailOcclusionAmount;
                Verify.IsTrue(tailShortSide > 0, "Precondition: the tail polygon is measured");

                // TeachingTip::DetermineEffectivePlacementTargeted keeps Top only if the TailOcclusionGrid height plus the tail's short
                // side fits between the target and the top of the window.
                required = GetTailOcclusionGrid(tip).ActualHeight + tailShortSide;
            });
            Verify.AreEqual((double)(float)(target.Y - required), target.Y - required, "Precondition: the boundary is exactly representable in the (float) test window bounds");

            // Exactly enough room above the target: Top is kept. Changing HeroContentPlacement makes the open tip re-run the
            // placement algorithm synchronously against the current bounds (TeachingTip::OnHeroContentPlacementChanged).
            UseWindowBoundsAroundTarget(tip, AmpleSpace, required, AmpleSpace, AmpleSpace);
            RunOnUIThread.Execute(() =>
            {
                tip.HeroContentPlacement = TeachingTipHeroContentPlacementMode.Top;
                Verify.AreEqual(TeachingTipPlacementMode.Top, TeachingTipTestHooks.GetEffectivePlacement(tip), $"Top with exactly {required}px above the target");
            });

            // One pixel less: Top no longer fits and the tip falls back to Bottom.
            UseWindowBoundsAroundTarget(tip, AmpleSpace, required - 1, AmpleSpace, AmpleSpace);
            RunOnUIThread.Execute(() =>
            {
                tip.HeroContentPlacement = TeachingTipHeroContentPlacementMode.Bottom;
                Verify.AreEqual(TeachingTipPlacementMode.Bottom, TeachingTipTestHooks.GetEffectivePlacement(tip), $"Fallback with {required - 1}px above the target");
            });
        }

        [TestMethod]
        [TestProperty("IsolationLevel", "Method")] // A hook that dereferenced the null tip would crash the test app.
        public void TestHookSettersIgnoreNullTip()
        {
            RunOnUIThread.Execute(() =>
            {
                // Every setter must ignore a null tip, even when its other argument is valid.
                var compositor = CompositionTarget.GetCompositorForCurrentThread();
                TeachingTipTestHooks.SetExpandEasingFunction(null, compositor.CreateLinearEasingFunction());
                TeachingTipTestHooks.SetContractEasingFunction(null, compositor.CreateLinearEasingFunction());
                TeachingTipTestHooks.SetTipShouldHaveShadow(null, false);
                TeachingTipTestHooks.SetContentElevation(null, 12);
                TeachingTipTestHooks.SetTailElevation(null, 12);
                TeachingTipTestHooks.SetUseTestWindowBounds(null, true);
                TeachingTipTestHooks.SetTestWindowBounds(null, new Rect(0, 0, 10, 10));
                TeachingTipTestHooks.SetUseTestScreenBounds(null, true);
                TeachingTipTestHooks.SetTestScreenBounds(null, new Rect(0, 0, 10, 10));
                TeachingTipTestHooks.SetTipFollowsTarget(null, true);
                TeachingTipTestHooks.SetReturnTopForOutOfWindowPlacement(null, false);
                TeachingTipTestHooks.SetExpandAnimationDuration(null, TimeSpan.FromSeconds(1));
                TeachingTipTestHooks.SetContractAnimationDuration(null, TimeSpan.FromSeconds(1));
            });

            // Positive control: the same hooks still apply to a real tip, which then opens and closes normally.
            var tip = CreateUntargetedTip();
            RunOnUIThread.Execute(() =>
            {
                var compositor = CompositionTarget.GetCompositorForCurrentThread();
                TeachingTipTestHooks.SetExpandEasingFunction(tip, compositor.CreateLinearEasingFunction());
                TeachingTipTestHooks.SetContractEasingFunction(tip, compositor.CreateLinearEasingFunction());
            });
            OpenTip(tip);
            CloseTip(tip);
        }

        // ------------------------------------------------------------------------------------------------
        // Helpers
        // ------------------------------------------------------------------------------------------------

        private const double TargetSize = 40;
        private const double AmpleSpace = 2000;
        private const double NarrowSpace = 5;
        private const double OffsetTolerance = 0.5;
        private const double UntargetedTipWindowEdgeMargin = 24;
        private const double TailOcclusionAmount = 2; // TeachingTip::s_tailOcclusionAmount
        private const int E_NOTIMPL = unchecked((int)0x80004001);

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
                    {
                        // Bottom, BottomLeft, BottomRight: the highlight stops at the tail from both corners. The expected values follow
                        // the TeachingTip.h helpers: the tail's edge margin is the second tail-margin column plus the 2px tail occlusion,
                        // and the tail's long/short sides are the TailPolygon's sides minus the occlusion.
                        double width = GetContentRootGrid(tip).ActualWidth;
                        var tailOcclusionGrid = GetTailOcclusionGrid(tip);
                        var tailPolygon = GetTailPolygon(tip);
                        double tailEdgeMargin = tailOcclusionGrid.ColumnDefinitions[1].ActualWidth + TailOcclusionAmount;
                        double tailLongSide = Math.Max(tailPolygon.ActualHeight, tailPolygon.ActualWidth) - 2 * TailOcclusionAmount;
                        double tailShortSide = Math.Min(tailPolygon.ActualHeight, tailPolygon.ActualWidth) - TailOcclusionAmount;
                        Verify.IsTrue(tailShortSide > 0 && tailEdgeMargin > TailOcclusionAmount, $"{what}: precondition: tail polygon and margin columns are measured");

                        (Thickness right, Thickness left) expected = placement switch
                        {
                            TeachingTipPlacementMode.Bottom => (
                                new Thickness(width / 2 + tailShortSide - 1, 0, radius.TopRight - 1, 0),
                                new Thickness(radius.TopLeft - 1, 0, width / 2 + tailShortSide - 1, 0)),
                            TeachingTipPlacementMode.BottomRight => (
                                new Thickness(tailEdgeMargin + tailLongSide - 1, 0, radius.TopRight - 1, 0),
                                new Thickness(radius.TopLeft - 1, 0, width - (tailEdgeMargin + 1), 0)),
                            _ /* BottomLeft */ => (
                                new Thickness(width - (tailEdgeMargin + 1), 0, radius.TopRight - 1, 0),
                                new Thickness(radius.TopLeft - 1, 0, tailEdgeMargin + tailLongSide - 1, 0)),
                        };
                        VerifyThickness(expected.right, topRight, what + " (TopRight)");
                        VerifyThickness(expected.left, topLeft, what + " (TopLeft)");
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

        // Expected popup offsets of an untargeted tip (TeachingTip::PositionUntargetedPopup) in a window at the origin: the tip keeps
        // 24px from the window edges, and the placement margin shifts it further.
        private static (double x, double y) GetExpectedUntargetedOffset(Size window, Size tip, TeachingTipPlacementMode placement, Thickness margin)
        {
            double nearX = UntargetedTipWindowEdgeMargin + margin.Left;
            double nearY = UntargetedTipWindowEdgeMargin + margin.Top;
            double farX = window.Width - (tip.Width + UntargetedTipWindowEdgeMargin + margin.Right);
            double farY = window.Height - (tip.Height + UntargetedTipWindowEdgeMargin + margin.Bottom);
            double centerX = window.Width / 2 - tip.Width / 2 + margin.Left - margin.Right;
            double centerY = window.Height / 2 - tip.Height / 2 + margin.Top - margin.Bottom;

            return placement switch
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
        }

        // TeachingTip skips its expand/contract animations when the system animation setting is off (SharedHelpers::IsAnimationsEnabled),
        // so tests that observe those animations cannot run there (precedent: ScrollViewTests.VerifyVisualStates).
        private static bool AnimationsAreDisabled()
        {
            return !new global::Windows.UI.ViewManagement.UISettings().AnimationsEnabled;
        }

        // Waits for a UI-thread condition that has no completion event; the timeout is only an outer safety bound.
        private static bool WaitForUIState(Func<bool> condition)
        {
            var stopwatch = Stopwatch.StartNew();
            bool reached = false;
            while (true)
            {
                IdleSynchronizer.Wait();
                RunOnUIThread.Execute(() => reached = condition());
                if (reached || stopwatch.ElapsedMilliseconds > 10000)
                {
                    break;
                }
                Thread.Sleep(50);
            }
            return reached;
        }

        private static Point GetPopupOffset(TeachingTip tip)
        {
            var popup = TeachingTipTestHooks.GetPopup(tip);
            return new Point(popup.HorizontalOffset, popup.VerticalOffset);
        }

        // Restores the test window and sizes it to a fraction of its maximized size, then waits until the XAML content has that width.
        private static void ResizeTestWindow(Microsoft.UI.Windowing.AppWindow appWindow, Microsoft.UI.Windowing.OverlappedPresenter presenter, global::Windows.Graphics.SizeInt32 maximizedSize, double fraction)
        {
            var size = new global::Windows.Graphics.SizeInt32((int)(maximizedSize.Width * fraction), (int)(maximizedSize.Height * fraction));
            RunOnUIThread.Execute(() =>
            {
                if (presenter.State != Microsoft.UI.Windowing.OverlappedPresenterState.Restored)
                {
                    presenter.Restore();
                }
                appWindow.Resize(size);
            });

            Verify.IsTrue(WaitForUIState(() =>
            {
                var xamlRoot = MUXControlsTestApp.App.CurrentWindow.Content.XamlRoot;
                return appWindow.Size.Width == size.Width &&
                    Math.Abs(xamlRoot.Size.Width * xamlRoot.RasterizationScale - appWindow.ClientSize.Width) < 2;
            }), $"The test window should be resized to {size.Width}x{size.Height}");
        }

        private static Grid GetContentRootGrid(TeachingTip tip)
        {
            var grid = VisualTreeUtils.FindVisualChildByName(TeachingTipTestHooks.GetPopup(tip).Child, "ContentRootGrid") as Grid;
            Verify.IsNotNull(grid, "ContentRootGrid");
            return grid;
        }

        // Checks that a dependency property identifier is the one behind its property: a value set through either one is visible
        // through the other, and ClearValue restores the default.
        private static void VerifyDependencyProperty<T>(DependencyObject owner, DependencyProperty property, string name, T defaultValue, T newValue, Func<T> get, Action<T> set)
        {
            Verify.IsNotNull(property, name + "Property");
            Verify.AreEqual(defaultValue, (T)owner.GetValue(property), name + " default value");

            owner.SetValue(property, newValue);
            Verify.AreEqual(newValue, get(), name + " set through its identifier");

            owner.ClearValue(property);
            Verify.AreEqual(defaultValue, get(), name + " after ClearValue");

            set(newValue);
            Verify.AreEqual(newValue, (T)owner.GetValue(property), name + " set through the property");
            owner.ClearValue(property);
        }

        private static string GetRuntimeClassName(object projectedObject)
        {
            return new global::WinRT.IInspectable(((global::WinRT.IWinRTObject)projectedObject).NativeObject).GetRuntimeClassName();
        }

        // A template with only the parts TeachingTip requires (Container and ContentRootGrid, see concern C6) plus a TailEdgeBorder,
        // which no shipped template has. TailOcclusionGrid, TailPolygon and the buttons are deliberately missing.
        private static ControlTemplate LoadTemplateWithoutTailParts()
        {
            return (ControlTemplate)XamlReader.Load(
                @"<ControlTemplate xmlns='http://schemas.microsoft.com/winfx/2006/xaml/presentation'
                                   xmlns:x='http://schemas.microsoft.com/winfx/2006/xaml'
                                   xmlns:controls='using:Microsoft.UI.Xaml.Controls'
                                   TargetType='controls:TeachingTip'>
                    <Border x:Name='Container'>
                        <Grid x:Name='TemplateRoot'>
                            <Grid x:Name='TailEdgeBorder' Width='20' Height='10' />
                            <Grid x:Name='ContentRootGrid' Width='160' Height='80'>
                                <ContentPresenter Content='{TemplateBinding Content}' />
                            </Grid>
                        </Grid>
                    </Border>
                </ControlTemplate>");
        }

        // Required parts plus a TailOcclusionGrid with a single column (shipped templates have five) and a TailPolygon.
        private static ControlTemplate LoadTemplateWithOneColumnTailGrid()
        {
            return (ControlTemplate)XamlReader.Load(
                @"<ControlTemplate xmlns='http://schemas.microsoft.com/winfx/2006/xaml/presentation'
                                   xmlns:x='http://schemas.microsoft.com/winfx/2006/xaml'
                                   xmlns:controls='using:Microsoft.UI.Xaml.Controls'
                                   TargetType='controls:TeachingTip'>
                    <Border x:Name='Container'>
                        <Grid x:Name='TemplateRoot'>
                            <Grid x:Name='TailOcclusionGrid'>
                                <Grid.ColumnDefinitions>
                                    <ColumnDefinition Width='Auto' />
                                </Grid.ColumnDefinitions>
                                <Grid x:Name='ContentRootGrid' Width='160' Height='80'>
                                    <ContentPresenter Content='{TemplateBinding Content}' />
                                </Grid>
                                <Polygon x:Name='TailPolygon' Width='20' Height='10' Points='0,10 10,0 20,10' Fill='Gray' />
                            </Grid>
                        </Grid>
                    </Border>
                </ControlTemplate>");
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
