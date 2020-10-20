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
                    var child = popup.Child;
                    var grandChild = VisualTreeHelper.GetChild(child, 0);
                    Verify.AreSame(blueBrush, ((Grid)grandChild).Background, "Checking TeachingTip.Background TemplateBinding works");
                }

                {
                    var popup = TeachingTipTestHooks.GetPopup(teachingTipLightDismiss);
                    var child = popup.Child;
                    var grandChild = VisualTreeHelper.GetChild(child, 0);
                    var actualBrush = ((Grid)grandChild).Background;
                    Log.Comment("Checking LightDismiss TeachingTip Background is using resource for first invocation");
                    if (lightDismissBackgroundBrush != actualBrush)
                    {
                        if (actualBrush is SolidColorBrush actualSolidBrush)
                        {
                            string teachingTipMessage = $"LightDismiss TeachingTip Background is SolidColorBrush with color {actualSolidBrush.Color}";
                            Log.Comment(teachingTipMessage);
                            Verify.Fail(teachingTipMessage);
                        }
                        else
                        {
                            Verify.AreSame(lightDismissBackgroundBrush, actualBrush, "Checking LightDismiss TeachingTip Background is using resource for first invocation");
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
                var grandChild = VisualTreeHelper.GetChild(child, 0);
                var grandChildBackgroundBrush = ((Grid)grandChild).Background;
                //If we can no longer cast the background brush to a solid color brush then changing the
                //IsLightDismissEnabled has changed the background as we expected it to.
                if (grandChildBackgroundBrush is SolidColorBrush)
                {
                    Verify.AreNotEqual(blueBrush.Color, ((SolidColorBrush)grandChildBackgroundBrush).Color);
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
