using System;
using Microsoft.UI.Xaml;

namespace ChartAppCsPackaged
{
    public partial class App : Application
    {
        private readonly int _primaryThreadId = Environment.CurrentManagedThreadId;
        public App()
        {
            this.InitializeComponent();
        }

        protected override void OnLaunched(Microsoft.UI.Xaml.LaunchActivatedEventArgs args)
        {
            if (Environment.CurrentManagedThreadId != _primaryThreadId)
            {
                return;
            }

            m_window ??= new ChartsSample.MainWindow("C# | Packaged");
            m_window.Activate();
        }

        private Window m_window;
    }
}
