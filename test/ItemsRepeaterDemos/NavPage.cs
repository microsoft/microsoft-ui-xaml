using Windows.UI.Xaml.Controls;
using Windows.UI.Xaml.Input;

namespace ItemsRepeaterDemos
{
    public class NavPage: Page
    {
        protected override void OnKeyDown(KeyRoutedEventArgs e)
        {
            if (e.Key == Windows.System.VirtualKey.Escape)
            {
                if (Frame.CanGoBack)
                {
                    Frame.GoBack();
                }
            }
        }
    }
}
