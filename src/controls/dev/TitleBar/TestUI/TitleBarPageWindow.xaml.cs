using Microsoft.UI.Xaml;
using Microsoft.UI.Xaml.Controls;
using Microsoft.UI.Xaml.Markup;
using System;

namespace MUXControlsTestApp
{
    public sealed partial class TitleBarPageWindow : Window
    {
        private int backRequestedCount = 0;
        private int paneToggleRequestedCount = 0;
        public TitleBarPageWindow()
        {
            this.InitializeComponent();

            // C# code to set AppTitleBar uielement as titlebar
            this.ExtendsContentIntoTitleBar = true;
            this.SetTitleBar(this.WindowingTitleBar);

        }

        private void WindowingTitleBar_BackRequested(TitleBar sender, object args)
        {
            BackRequestedCountTextBox.Text = (backRequestedCount++).ToString();
        }

        private void IsBackButtonVisibleCheckBox_CheckedChanged(object sender, RoutedEventArgs e)
        {
            if (WindowingTitleBar != null)
            {
                WindowingTitleBar.IsBackButtonVisible = IsBackButtonVisibleCheckBox.IsChecked.Value;
            }
        }

        private void IsBackEnabledCheckBox_CheckedChanged(object sender, RoutedEventArgs e)
        {
            if (WindowingTitleBar != null)
            {
                WindowingTitleBar.IsBackEnabled = IsBackEnabledCheckBox.IsChecked.Value;
            }
        }

        private void WindowingTitleBar_PaneToggleRequested(TitleBar sender, object args)
        {
            PaneToggleButtonRequestedCountTextBox.Text = (paneToggleRequestedCount++).ToString();
        }

        private void IsPaneToggleButtonVisibleCheckbox_CheckedChanged(object sender, RoutedEventArgs e)
        {
            if (WindowingTitleBar != null)
            {
                WindowingTitleBar.IsPaneToggleButtonVisible = IsPaneToggleButtonVisibleCheckbox.IsChecked.Value;
            }
        }

        private void SetIconCheckBox_CheckedChanged(object sender, RoutedEventArgs e)
        {
            if (WindowingTitleBar != null)
            {
                if (SetIconCheckBox.IsChecked.Value)
                {
                    var icon = new Microsoft.UI.Xaml.Controls.SymbolIconSource();
                    icon.Symbol = Symbol.Mail;
                    WindowingTitleBar.IconSource = icon;
                }
                else
                {
                    WindowingTitleBar.IconSource = null;
                }
            }
        }

        private void CustomContentCheckBox_CheckedChanged(object sender, RoutedEventArgs e)
        {
            if (WindowingTitleBar != null)
            {
                if (CustomContentCheckBox.IsChecked.Value)
                {
                    string xaml =
                    @"<Grid xmlns='http://schemas.microsoft.com/winfx/2006/xaml/presentation'>
                         <Grid.ColumnDefinitions>
                            <ColumnDefinition Width='Auto'/>
                            <ColumnDefinition Width='*' />
                         </Grid.ColumnDefinitions >

                         <Button Content='Left'/>
                         <Button Grid.Column='1' Content='Right' HorizontalAlignment='Right'/>
                    </Grid>";

                    var element = XamlReader.Load(xaml);
                    WindowingTitleBar.Content = element;
                }
                else
                {
                    WindowingTitleBar.Content = null;
                }
            }
        }

        private void HeaderContentCheckBox_CheckedChanged(object sender, RoutedEventArgs e)
        {
            if (WindowingTitleBar != null)
            {
                if (HeaderContentCheckBox.IsChecked.Value)
                {
                    var button = new Button();
                    button.Content = "Header";
                    WindowingTitleBar.Header = button;
                }
                else
                {
                    WindowingTitleBar.Header = null;
                }
            }
        }

        private void FooterContentCheckBox_CheckedChanged(object sender, RoutedEventArgs e)
        {
            if (WindowingTitleBar != null)
            {
                if (FooterContentCheckBox.IsChecked.Value)
                {
                    var button = new Button();
                    button.Content = "Footer";
                    WindowingTitleBar.Footer = button;
                }
                else
                {
                    WindowingTitleBar.Footer = null;
                }
            }
        }

        private void SetSubtitleButton_Click(object sender, RoutedEventArgs e)
        {
            if (WindowingTitleBar != null)
            {
                WindowingTitleBar.Subtitle = SubtitleTextBox.Text;
            }
        }

        private void TitleButton_Click(object sender, RoutedEventArgs e)
        {
            if (WindowingTitleBar != null)
            {
                WindowingTitleBar.Title = TitleTextBox.Text;
            }
        }


    }
}
