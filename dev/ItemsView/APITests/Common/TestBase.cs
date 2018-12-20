using DEPControlsTestApp.Utilities;
using System.Threading;
using WEX.TestExecution;
using WEX.TestExecution.Markup;
using Windows.UI.Xaml;
using Windows.UI.Xaml.Controls;

namespace DEPControls.ApiTests.ItemsViewTests
{
    class TestsBase
    {
        private Border _host;

        public const int DefaultWaitTimeInMS = 5000;

        // Set this content instead of using Window.Current.Content
        // because the latter requires you to tick the UI thread
        // before a layout pass can happen while you can directly call
        // UpdateLayout after the former, which is faster and less
        // sensitive to timing issues.
        public UIElement Content
        {
            get { return _host.Child; }
            set { _host.Child = value; }
        }

        [TestInitialize]
        public void Setup()
        {
            var hostLoaded = new ManualResetEvent(false);
            RunOnUIThread.Execute(() =>
            {
                _host = new Border();
                _host.Loaded += delegate { hostLoaded.Set(); };
                DEPControlsTestApp.App.TestContentRoot = _host;
            });
            Verify.IsTrue(hostLoaded.WaitOne(DefaultWaitTimeInMS));
        }

        [TestCleanup]
        public void Cleanup()
        {
            // Put things back the way we found them.
            RunOnUIThread.Execute(() =>
            {
                DEPControlsTestApp.App.TestContentRoot = null;
            });
        }
    }
}
