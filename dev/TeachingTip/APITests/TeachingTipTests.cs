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

#if !BUILD_WINDOWS
using TeachingTip = Microsoft.UI.Xaml.Controls.TeachingTip;
using IconSource = Microsoft.UI.Xaml.Controls.IconSource;
using SymbolIconSource = Microsoft.UI.Xaml.Controls.SymbolIconSource;
#endif

namespace Windows.UI.Xaml.Tests.MUXControls.ApiTests
{
    [TestClass]
    public class TeachingTipTests
    {
        //[TestMethod] TODO: Re-enable once issue #643 is fixed.
        [TestProperty("TestPass:IncludeOnlyOn", "Desktop")] // TeachingTip doesn't appear to show up correctly in OneCore.
        public void TeachingTipBackgroundTest()
        {
            var loadedEvent = new AutoResetEvent(false);
            RunOnUIThread.Execute(() =>
            {
                TeachingTip teachingTip = new TeachingTip();
                teachingTip.Loaded += (object sender, RoutedEventArgs args) => { loadedEvent.Set(); };
                MUXControlsTestApp.App.TestContentRoot = teachingTip;
            });

            IdleSynchronizer.Wait();
            loadedEvent.WaitOne();

            RunOnUIThread.Execute(() =>
            {
                TeachingTip teachingTip = (TeachingTip)MUXControlsTestApp.App.TestContentRoot;
                var redBrush = new SolidColorBrush(Colors.Red);
                teachingTip.SetValue(TeachingTip.BackgroundProperty, redBrush);
                Verify.AreSame(redBrush, teachingTip.GetValue(TeachingTip.BackgroundProperty) as Brush);
                Verify.AreSame(redBrush, teachingTip.Background);

                var blueBrush = new SolidColorBrush(Colors.Blue);
                teachingTip.Background = blueBrush;
                Verify.AreSame(blueBrush, teachingTip.Background);

                var child = VisualTreeHelper.GetChild(teachingTip, 0);
                var grandChild = VisualTreeHelper.GetChild(child, 1);
                Verify.AreSame(blueBrush, ((Grid)grandChild).Background);

                teachingTip.IsLightDismissEnabled = true;
            });

            IdleSynchronizer.Wait();

            RunOnUIThread.Execute(() =>
            {
                TeachingTip teachingTip = (TeachingTip)MUXControlsTestApp.App.TestContentRoot;
                var blueBrush = new SolidColorBrush(Colors.Blue);
                Verify.AreEqual(blueBrush.Color, ((SolidColorBrush)teachingTip.Background).Color);
                var child = VisualTreeHelper.GetChild(teachingTip, 0);
                var grandChild = VisualTreeHelper.GetChild(child, 1);
                var grandChildBackgroundBrush = ((Grid)grandChild).Background;
                //If we can no longer cast the background brush to a solid color brush then changing the
                //IsLightDismissEnabled has changed the background as we expected it to.
                if(grandChildBackgroundBrush is SolidColorBrush)
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
                MUXControlsTestApp.App.TestContentRoot = teachingTip;
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
                MUXControlsTestApp.App.TestContentRoot = teachingTip;
            });

            IdleSynchronizer.Wait();
            loadedEvent.WaitOne();
        }
    }
}
