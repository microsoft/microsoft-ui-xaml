using System;
using Microsoft.UI.Xaml;
using Microsoft.UI.Xaml.Controls;

namespace TableViewSampleApp;

// Navigation shell: a NavigationView + Frame hosting the interactive playground plus one page per
// column-width configuration (all Auto / all Star / all Pixel / mixed).
public sealed partial class MainWindow : Window
{
    public MainWindow()
    {
        this.InitializeComponent();
        this.Title = "TableView Sample";

        // Select the first item, which navigates the frame to the playground via SelectionChanged.
        Nav.SelectedItem = Nav.MenuItems[0];
    }

    private void Nav_SelectionChanged(NavigationView sender, NavigationViewSelectionChangedEventArgs args)
    {
        if (ContentFrame is null || args.SelectedItem is not NavigationViewItem item)
        {
            return;
        }

        Type pageType = (item.Tag as string) switch
        {
            "auto" => typeof(AutoColumnsPage),
            "star" => typeof(StarColumnsPage),
            "pixel" => typeof(PixelColumnsPage),
            "mixed" => typeof(MixedColumnsPage),
            "interactive" => typeof(InteractiveCellsPage),
            "selection" => typeof(SelectionPage),
            _ => typeof(PlaygroundPage),
        };

        if (ContentFrame.CurrentSourcePageType != pageType)
        {
            ContentFrame.Navigate(pageType);
        }
    }
}
