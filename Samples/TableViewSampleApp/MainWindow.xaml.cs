using System;
using System.Linq;
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

        // Unattended runs: a trigger file in the app's data directory opens the matching page and runs it.
        // The self-check wins when both exist; the perf trigger is then left in place for the next launch.
        var dir = HierarchySelfCheckPage.DataDirectory;
        var checkTrigger = System.IO.Path.Combine(dir, HierarchySelfCheckPage.TriggerFileName);
        var perfTrigger = System.IO.Path.Combine(dir, HierarchyPage.PerfTriggerFileName);
        if (System.IO.File.Exists(checkTrigger))
        {
            System.IO.File.Delete(checkTrigger);
            HierarchySelfCheckPage.AutoRun = true;
            Nav.SelectedItem = Nav.MenuItems.OfType<NavigationViewItem>().First(i => (i.Tag as string) == "selfcheck");
        }
        else if (System.IO.File.Exists(perfTrigger))
        {
            System.IO.File.Delete(perfTrigger);
            HierarchyPage.AutoPerf = true;
            Nav.SelectedItem = Nav.MenuItems.OfType<NavigationViewItem>().First(i => (i.Tag as string) == "hierarchy");
        }
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
            "tooltips" => typeof(ToolTipsPage),
            "shaping" => typeof(ShapingPage),
            "hierarchy" => typeof(HierarchyPage),
            "selfcheck" => typeof(HierarchySelfCheckPage),
            _ => typeof(PlaygroundPage),
        };

        if (ContentFrame.CurrentSourcePageType != pageType)
        {
            ContentFrame.Navigate(pageType);
        }
    }
}
