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

        // Unattended runs: a trigger file in the app's data directory opens the matching page and
        // runs it. autorun-livecheck runs the live-shaping self-check; autorun-perf the 100k probe.
        var liveTrigger = System.IO.Path.Combine(HierarchyPage.DataDirectory, LiveShapingSelfCheckPage.TriggerFileName);
        var perfTrigger = System.IO.Path.Combine(HierarchyPage.DataDirectory, HierarchyPage.PerfTriggerFileName);
        if (System.IO.File.Exists(liveTrigger))
        {
            System.IO.File.Delete(liveTrigger);
            LiveShapingSelfCheckPage.AutoRun = true;
            Nav.SelectedItem = Nav.MenuItems.OfType<NavigationViewItem>().First(i => (i.Tag as string) == "livecheck");
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
            "livecheck" => typeof(LiveShapingSelfCheckPage),
            _ => typeof(PlaygroundPage),
        };

        if (ContentFrame.CurrentSourcePageType != pageType)
        {
            ContentFrame.Navigate(pageType);
        }
    }
}
