﻿// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

using System;
using System.Collections.Generic;
using System.Text;
using Windows.UI.Xaml.Controls;
using Windows.UI.Xaml.Markup;
using MUXControlsTestApp.Utilities;
using Windows.UI.Xaml.Media;
using Windows.UI.Xaml;
using Windows.UI.Xaml.Controls;
using Windows.UI.Xaml.Media.Imaging;
using Common;
using System.Threading;


#if USING_TAEF
using WEX.TestExecution;
using WEX.TestExecution.Markup;
using WEX.Logging.Interop;
#else
using Microsoft.VisualStudio.TestTools.UnitTesting;
using Microsoft.VisualStudio.TestTools.UnitTesting.Logging;
#endif

using TeachingTip = Microsoft.UI.Xaml.Controls.TeachingTip;
using IconSource = Microsoft.UI.Xaml.Controls.IconSource;
using SymbolIconSource = Microsoft.UI.Xaml.Controls.SymbolIconSource;
using Microsoft.UI.Private.Controls;
using Microsoft.UI.Xaml.Controls;
using Windows.UI.Xaml.Shapes;

namespace Windows.UI.Xaml.Tests.MUXControls.ApiTests
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
                blueBrush = new SolidColorBrush(Colors.Blue);
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
                var redBrush = new SolidColorBrush(Colors.Red);
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
