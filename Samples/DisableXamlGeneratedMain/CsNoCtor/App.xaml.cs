using Microsoft.UI.Xaml;
using System;

namespace DisableXamlGeneratedMainNoCtorCs
{
    public partial class App : Application
    {
        // This sample intentionally has NO parameterless constructor. The developer
        // supplies their own entry point (see Program.cs) and constructs the App with
        // this parameterized constructor. Because DISABLE_XAML_GENERATED_MAIN is defined,
        // the XamlCompiler must NOT emit the parameterless constructor call in the
        // generated XamlGeneratedMain() helper.
        public App(int launchId)
        {
            LaunchMarker = $"CustomMain:{launchId}";

            this.InitializeComponent();
        }

        // Recorded by the parameterized constructor above and displayed by MainWindow so
        // that automated tests can verify that the app was launched by the developer's
        // own entry point using this constructor.
        public static string LaunchMarker { get; private set; } = string.Empty;

        protected override void OnLaunched(Microsoft.UI.Xaml.LaunchActivatedEventArgs args)
        {
            m_window = new MainWindow();
            m_window.Activate();
        }

        private Window m_window;
    }
}
