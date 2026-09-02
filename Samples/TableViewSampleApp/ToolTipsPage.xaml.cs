using System;
using System.Collections.Generic;
using System.Linq;
using System.Text;
using Microsoft.UI.Xaml;
using Microsoft.UI.Xaml.Controls;
using Microsoft.UI.Xaml.Controls.Tabular;
using Microsoft.UI.Xaml.Data;
using Microsoft.UI.Xaml.Automation;
using Microsoft.UI.Xaml.Automation.Peers;
using Microsoft.UI.Xaml.Media;

namespace TableViewSampleApp;

// Turns a row item into rich (non-string) tooltip content. A converter is how computed tooltips are
// authored now that the content is a binding value rather than something a handler hands back.
public sealed class BioCardConverter : IValueConverter
{
    public object Convert(object value, Type targetType, object parameter, string language)
    {
        if (value is not Item item)
        {
            return null;
        }

        // A fresh element per evaluation: a UIElement is parented by the cell's ToolTip.
        var panel = new StackPanel { Spacing = 4 };
        panel.Children.Add(new TextBlock { Text = item.Name, FontWeight = Microsoft.UI.Text.FontWeights.SemiBold });
        panel.Children.Add(new TextBlock { Text = $"{item.Role} - {item.City}", Opacity = 0.75 });
        panel.Children.Add(new Border
        {
            Height = 4,
            Width = 120,
            HorizontalAlignment = HorizontalAlignment.Left,
            Background = new SolidColorBrush(Microsoft.UI.Colors.SteelBlue),
        });

        return panel;
    }

    public object ConvertBack(object value, Type targetType, object parameter, string language) =>
        throw new NotSupportedException();
}

// Exercises per-cell tooltips: a plain string binding, a converter producing non-string content, a
// column that opts out, a cell whose own template owns a tooltip, and late attach / detach.
public sealed partial class ToolTipsPage : Page
{
    private readonly TableViewColumn _nameColumn;
    private readonly TableViewColumn _bioColumn;
    private readonly TableViewColumn _cityColumn;
    private readonly TableViewColumn _scoreColumn;

    private bool _attached;
    private int _mutations;

    public ToolTipsPage()
    {
        this.InitializeComponent();

        _nameColumn = SampleColumns.Text("Name", nameof(Item.Name), SampleColumns.Pixels(140));
        _bioColumn = SampleColumns.Text("Bio", nameof(Item.Bio), SampleColumns.Pixels(180));
        _cityColumn = SampleColumns.Text("City", nameof(Item.City), SampleColumns.Pixels(120));
        _scoreColumn = SampleColumns.Text("Score", nameof(Item.Score), SampleColumns.Pixels(70));

        Table.Columns.Add(_nameColumn);
        Table.Columns.Add(_bioColumn);
        Table.Columns.Add(_cityColumn);
        Table.Columns.Add(_scoreColumn);
        Table.Columns.Add(new TableViewTemplateColumn
        {
            Header = "Notes (app tooltip)",
            CellTemplate = (DataTemplate)Resources["AppToolTipCell"],
            Width = SampleColumns.Pixels(220),
        });

        Table.ItemsSource = Data.Make();

        AttachToolTipBindings();

        // Score deliberately never gets a binding: no tooltip should ever appear over that column.
        UpdateStatus();
    }

    private void AttachToolTipBindings()
    {
        // Same text as the cell: the peer drops the duplicate HelpText so Narrator reads it once.
        _bioColumn.CellToolTipBinding = new Binding { Path = new PropertyPath(nameof(Item.Bio)) };

        // Distinct from the cell's own text, and observable: HelpText must survive here, and
        // mutating Notes must update the tooltip with no invalidation call.
        _cityColumn.CellToolTipBinding = new Binding { Path = new PropertyPath(nameof(Item.Notes)) };

        // Non-string content, produced by a converter from the whole row item.
        _nameColumn.CellToolTipBinding = new Binding
        {
            Converter = new BioCardConverter(),
        };

        _attached = true;
        AttachLate.Content = "Detach bindings";
    }

    private void OnAttachLate(object sender, RoutedEventArgs e)
    {
        if (_attached)
        {
            // Clearing the binding retracts the tooltips.
            _bioColumn.CellToolTipBinding = null;
            _cityColumn.CellToolTipBinding = null;
            _nameColumn.CellToolTipBinding = null;
            _attached = false;
            AttachLate.Content = "Attach bindings";
        }
        else
        {
            AttachToolTipBindings();
        }

        UpdateStatus();
    }

    private void OnMutateData(object sender, RoutedEventArgs e)
    {
        // No invalidation API, and none needed: Item raises PropertyChanged for Notes.
        _mutations++;
        if (Table.ItemsSource is IEnumerable<Item> items)
        {
            foreach (var item in items.Take(40))
            {
                item.Notes = $"{item.Notes.Split(" [")[0]} [rev {_mutations}]";
            }
        }

        UpdateStatus();
    }

    private void UpdateStatus()
    {
        Status.Text = $"tooltip bindings {(_attached ? "attached" : "detached")} - data revision {_mutations}"
            + " - Bio (dup text), City (shows Notes), Name (rich content), Score (none), Notes (app-owned)";
    }

    private static IEnumerable<DependencyObject> Descendants(DependencyObject root)
    {
        int count = VisualTreeHelper.GetChildrenCount(root);
        for (int i = 0; i < count; i++)
        {
            var child = VisualTreeHelper.GetChild(root, i);
            yield return child;
            foreach (var d in Descendants(child))
            {
                yield return d;
            }
        }
    }

    private void OnScroll(object sender, RoutedEventArgs e)
    {
        // Scrolling recycles rows. With bindings there is no per-cell work on this path.
        var scrollers = Descendants(Table).OfType<ScrollViewer>().ToList();
        var scroller = scrollers.OrderByDescending(s => s.ScrollableHeight).FirstOrDefault();
        if (scroller is null || scroller.ScrollableHeight <= 0)
        {
            Status.Text = $"SCROLL: no vertically scrollable ScrollViewer (found {scrollers.Count})";
            return;
        }

        var before = scroller.VerticalOffset;
        _ = scroller.ChangeView(null, before + 900, null, true);

        DispatcherQueue.TryEnqueue(() =>
        {
            Status.Text = $"SCROLL: {before:F0} -> {scroller.VerticalOffset:F0} (max {scroller.ScrollableHeight:F0})";
        });
    }

    // Reads the live cell wrappers and reports, per column, who owns each cell's tooltip. This is
    // the only way to check the ownership rules without UIA (whose tree walks are unreliable here).
    private void OnCheckOwnership(object sender, RoutedEventArgs e)
    {
        var perColumn = new Dictionary<string, (int cells, int controlTips, int appTipsInContent, int helpTexts, int dupHelpText)>();

        foreach (var border in Descendants(Table).OfType<Border>())
        {
            if (border.Tag is not TableViewColumn column)
            {
                continue;
            }

            var header = column.Header?.ToString() ?? "?";
            perColumn.TryGetValue(header, out var acc);

            acc.cells++;

            if (ToolTipService.GetToolTip(border) is not null)
            {
                acc.controlTips++;
            }

            var help = AutomationProperties.GetHelpText(border);
            if (!string.IsNullOrEmpty(help))
            {
                acc.helpTexts++;

                // The peer suppresses duplicate HelpText at query time, so a match here is expected.
                var cellText = Descendants(border).OfType<TextBlock>().FirstOrDefault()?.Text;
                if (!string.IsNullOrEmpty(cellText) && cellText == help)
                {
                    acc.dupHelpText++;
                }
            }

            // What Narrator actually reads comes from TableViewCellAutomationPeer, which the row
            // peer creates - not from a peer for the Border. Counted in OnCheckUiaHelp instead.

            // An app tooltip declared inside the cell template lives on the content, not the wrapper.
            foreach (var inner in Descendants(border))
            {
                if (inner is FrameworkElement fe && ToolTipService.GetToolTip(fe) is not null)
                {
                    acc.appTipsInContent++;
                    break;
                }
            }

            perColumn[header] = acc;
        }

        var sb = new StringBuilder();
        foreach (var kv in perColumn.OrderBy(k => k.Key))
        {
            sb.Append($"[{kv.Key}] cells={kv.Value.cells} tip={kv.Value.controlTips} ")
              .Append($"appTip={kv.Value.appTipsInContent} help={kv.Value.helpTexts} ")
              .Append($"DUPhelp={kv.Value.dupHelpText}   ");
        }

        Status.Text = sb.Length > 0 ? sb.ToString() : "no cells found";
    }

    // Walks the real UIA peer tree: TableView peer -> row peers -> TableViewCellAutomationPeer.
    // GetHelpText() here is exactly what Narrator reads, so it proves the peer's suppression.
    private void OnCheckUiaHelp(object sender, RoutedEventArgs e)
    {
        var tablePeer = FrameworkElementAutomationPeer.CreatePeerForElement(Table);
        if (tablePeer is null)
        {
            Status.Text = "UIA: no TableView peer";
            return;
        }

        var columnNames = new[] { "Name", "Bio", "City", "Score", "Notes" };
        var withHelp = new int[columnNames.Length];
        var totals = new int[columnNames.Length];
        var firstText = new string[columnNames.Length];
        int rows = 0;

        // Row peers are not necessarily direct children of the table peer, so find the peers whose
        // own children are TableViewCell peers.
        static IEnumerable<AutomationPeer> Walk(AutomationPeer peer, int depth)
        {
            yield return peer;
            if (depth > 8)
            {
                yield break;
            }

            var children = peer.GetChildren();
            if (children is null)
            {
                yield break;
            }

            foreach (var c in children)
            {
                foreach (var d in Walk(c, depth + 1))
                {
                    yield return d;
                }
            }
        }

        foreach (var candidate in Walk(tablePeer, 0))
        {
            var cells = candidate.GetChildren();
            if (cells is null || cells.Count == 0 || cells[0].GetClassName() != "TableViewCell")
            {
                continue;
            }

            rows++;
            for (int i = 0; i < cells.Count && i < columnNames.Length; i++)
            {
                totals[i]++;
                var help = cells[i].GetHelpText();
                if (!string.IsNullOrEmpty(help))
                {
                    withHelp[i]++;
                    firstText[i] ??= help;
                }
            }
        }

        var sb2 = new StringBuilder($"UIA rows={rows}   ");
        for (int i = 0; i < columnNames.Length; i++)
        {
            sb2.Append($"[{columnNames[i]}] uiaHelp={withHelp[i]}/{totals[i]}");
            if (firstText[i] is not null)
            {
                var t = firstText[i];
                sb2.Append($" \"{(t.Length > 18 ? t[..18] : t)}\"");
            }
            sb2.Append("   ");
        }

        Status.Text = sb2.ToString();
    }
}
