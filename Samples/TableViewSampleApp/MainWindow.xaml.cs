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

#if HIERARCHY_API_KEYBY
        // Unattended perf run: an autorun-perf file in the app's data directory opens the hierarchy
        // page and runs the 100k probe.
        var perfTrigger = System.IO.Path.Combine(HierarchyPage.DataDirectory, HierarchyPage.PerfTriggerFileName);
        if (System.IO.File.Exists(perfTrigger))
        {
            System.IO.File.Delete(perfTrigger);
            HierarchyPage.AutoPerf = true;
            Nav.SelectedItem = Nav.MenuItems.OfType<NavigationViewItem>().First(i => (i.Tag as string) == "hierarchy");
        }
#else
        // The hierarchy demo page targets the KeyBy API and is not built for other hierarchy APIs.
        Nav.MenuItems.Remove(Nav.MenuItems.OfType<NavigationViewItem>().First(i => (i.Tag as string) == "hierarchy"));
#endif

        // Unattended hierarchy perf run: the trigger file's content carries the options
        // (e.g. "n=50000 shape=all key=int runs=3 variant=foo out=C:\perf compare=1"); empty runs
        // the full matrix. See HierarchyPerfPage for the outputs.
        var hierarchyPerfTrigger = System.IO.Path.Combine(HierarchyPerfPage.DataDirectory, HierarchyPerfPage.TriggerFileName);
        if (System.IO.File.Exists(hierarchyPerfTrigger))
        {
            HierarchyPerfPage.AutoRunArgs = System.IO.File.ReadAllText(hierarchyPerfTrigger);
            System.IO.File.Delete(hierarchyPerfTrigger);
            Nav.SelectedItem = Nav.MenuItems.OfType<NavigationViewItem>().First(i => (i.Tag as string) == "hierarchyperf");
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
#if HIERARCHY_API_KEYBY
            "hierarchy" => typeof(HierarchyPage),
#endif
            "hierarchyperf" => typeof(HierarchyPerfPage),
            _ => typeof(PlaygroundPage),
        };

        if (ContentFrame.CurrentSourcePageType != pageType)
        {
            ContentFrame.Navigate(pageType);
        }
    }
}
