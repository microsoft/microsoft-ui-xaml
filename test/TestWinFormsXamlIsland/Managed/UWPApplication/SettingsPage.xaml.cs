using System;
using System.Linq;
using System.Reflection;
using Windows.UI.Xaml;
using Windows.UI.Xaml.Controls;

namespace ManagedUWP
{
    public sealed partial class SettingsPage : Page
    {
        public static TEnum GetEnum<TEnum>(string text) where TEnum : struct
        {
            if (!typeof(TEnum).GetTypeInfo().IsEnum)
            {
                throw new InvalidOperationException("Generic parameter 'TEnum' must be an enum.");
            }
            return (TEnum)Enum.Parse(typeof(TEnum), text);
        }

        public SettingsPage()
        {
            this.InitializeComponent();
            this.Loaded += OnSettingsPageLoaded;
        }

        private void OnSettingsPageLoaded(object sender, RoutedEventArgs e)
        {
            var currentTheme = (this.XamlRoot.Content as Control).RequestedTheme.ToString();
            if (currentTheme == "Default")
            {
                currentTheme = Application.Current.RequestedTheme.ToString();
            }
            ThemePanel.Children.Cast<RadioButton>().FirstOrDefault(c => c?.Tag?.ToString() == currentTheme).IsChecked = true;
        }

        private void OnThemeRadioButtonChecked(object sender, RoutedEventArgs e)
        {
            var selectedTheme = ((RadioButton)sender)?.Tag?.ToString();
            if (selectedTheme != null)
            {
                // BUG: Setting a value to the Application object is no longer working.
                //Application.Current.RequestedTheme = GetEnum<ApplicationTheme>(selectedTheme);
                (this.XamlRoot.Content as Control).RequestedTheme = GetEnum<ElementTheme>(selectedTheme);
            }
        }
    }
}
