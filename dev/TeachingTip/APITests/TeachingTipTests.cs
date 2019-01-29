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
#endif

namespace Windows.UI.Xaml.Tests.MUXControls.ApiTests
{
    [TestClass]
    public class TeachingTipTests
    {
        [TestMethod]
        public void TeachingTipBackgroundTest()
        {
            var resetEvent = new AutoResetEvent(false);
            RunOnUIThread.Execute(() =>
            {
                TeachingTip teachingTip = new TeachingTip();
                teachingTip.Loaded += (object sender, RoutedEventArgs args) => { resetEvent.Set(); };
                MUXControlsTestApp.App.TestContentRoot = teachingTip;
            });

            IdleSynchronizer.Wait();
            resetEvent.WaitOne();

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
                Verify.AreNotEqual(blueBrush, teachingTip.Background);
                var child = VisualTreeHelper.GetChild(teachingTip, 0);
                var grandChild = VisualTreeHelper.GetChild(child, 1);
                Verify.AreNotEqual(blueBrush, ((Grid)grandChild).Background);
            });
        }
    }
}
