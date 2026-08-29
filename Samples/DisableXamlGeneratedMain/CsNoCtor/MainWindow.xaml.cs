using Microsoft.UI.Xaml;

namespace DisableXamlGeneratedMainNoCtorCs
{
    public sealed partial class MainWindow : Window
    {
        public MainWindow()
        {
            this.InitializeComponent();

            // Surface the marker recorded by App's parameterized constructor so that
            // automated tests can verify the app really launched through Program.Main
            // and the custom App(int) constructor.
            entryPointTextBlock.Text = App.LaunchMarker;
        }
    }
}
