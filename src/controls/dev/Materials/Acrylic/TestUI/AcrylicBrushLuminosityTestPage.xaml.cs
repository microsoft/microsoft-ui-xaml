using Microsoft.UI.Xaml.Media;
using Windows.Foundation.Metadata;

// The Blank Page item template is documented at https://go.microsoft.com/fwlink/?LinkId=234238

namespace MUXControlsTestApp
{
    /// <summary>
    /// An empty page that can be used on its own or navigated to within a Frame.
    /// </summary>
    public sealed partial class AcrylicBrushLuminosityTestPage : TestPage
    {
        public AcrylicBrushLuminosityTestPage()
        {
            this.InitializeComponent();

            ((AcrylicBrush)Resources["AcrylicBlackWithLuminosityOpacity"]).TintLuminosityOpacity = 0.2;
            ((AcrylicBrush)Resources["AcrylicDarkGreyWithLuminosityOpacity"]).TintLuminosityOpacity = 0.2;
        }
    }
}
