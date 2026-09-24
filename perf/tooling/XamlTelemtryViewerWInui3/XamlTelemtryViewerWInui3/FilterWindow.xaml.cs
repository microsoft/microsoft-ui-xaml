using Microsoft.UI.Xaml;
using Microsoft.UI.Xaml.Controls;
using XamlTelemtryViewerWInui3.Models;
using XamlTelemtryViewerWInui3.ViewModels;

namespace XamlTelemtryViewerWInui3
{
    public sealed partial class FilterWindow : Window
    {
        public FilterWindowViewModel? ViewModel { get; private set; }
        private Window? _parentWindow;

        public FilterWindow(EventFilter filter, Window? parentWindow = null)
        {
            this.InitializeComponent();
            this.Title = "Filter Editor";
            _parentWindow = parentWindow;
            
            ViewModel = new FilterWindowViewModel(filter);
            
            // Set DataContext on the root content
            if (this.Content is FrameworkElement fe)
            {
                fe.DataContext = ViewModel;
            }
            
            var appWindow = this.AppWindow;
            appWindow.ResizeClient(new Windows.Graphics.SizeInt32(1200, 700));
            
            // When parent window closes, close this window too
            if (_parentWindow != null)
            {
                _parentWindow.Closed += (_, _) =>
                {
                    this.Close();
                };
            }
        }

        private void ParseQueryTextButton_Click(object sender, RoutedEventArgs e)
        {
            try
            {
                if (ViewModel?.ParseQueryTextCommand != null)
                {
                    ViewModel.ParseQueryTextCommand.Execute(null);
                }
            }
            catch (System.Exception)
            {
                // Silently handle parse errors - user will see them in the UI
            }
        }

        private void RefreshTextButton_Click(object sender, RoutedEventArgs e)
        {
            try
            {
                if (ViewModel?.RefreshTextCommand != null)
                {
                    ViewModel.RefreshTextCommand.Execute(null);
                }
            }
            catch (System.Exception)
            {
                // Silently handle refresh errors
            }
        }

        private void ApplyButton_Click(object sender, RoutedEventArgs e)
        {
            try
            {
                if (ViewModel?.ApplyCommand != null)
                {
                    ViewModel.ApplyCommand.Execute(null);
                }
            }
            catch (System.Exception)
            {
                // Silently handle apply errors
            }
        }

    }
}
