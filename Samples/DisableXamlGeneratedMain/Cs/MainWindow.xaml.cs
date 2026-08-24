using Microsoft.UI.Xaml;

namespace DisableXamlGeneratedMainCs
{
    public sealed partial class MainWindow : Window
    {
        public MainWindow()
        {
            this.InitializeComponent();

            // Surface the marker recorded by our custom entry point so that automated
            // tests can verify the app really launched through Program.Main.
            entryPointTextBlock.Text = Program.LaunchMarker;
        }
    }
}
