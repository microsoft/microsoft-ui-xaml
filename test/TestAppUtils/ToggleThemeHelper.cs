using Windows.UI.Xaml;

namespace MUXControlsTestApp
{
    class ToggleThemeHelper
    {
        public static ElementTheme CurrentTheme { get; private set; } = ElementTheme.Default;

        public static void SetThemeFromAppTheme(ApplicationTheme applicationTheme)
        {
            if(applicationTheme == ApplicationTheme.Dark)
            {
                CurrentTheme = ElementTheme.Dark;
            }
            else
            {
                CurrentTheme = ElementTheme.Light;
            }
        }

        public static void ToggleTheme()
        {
            if(CurrentTheme == ElementTheme.Dark)
            {
                CurrentTheme = ElementTheme.Light;
            }
            else
            {
                CurrentTheme = ElementTheme.Dark;
            }
        }

    }
}
