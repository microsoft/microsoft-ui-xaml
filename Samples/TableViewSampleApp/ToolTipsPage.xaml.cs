using System;
using System.Collections.Generic;
using System.Linq;
using System.Text;
using Microsoft.UI.Xaml;
using Microsoft.UI.Xaml.Controls;
using Microsoft.UI.Xaml.Data;
using Microsoft.UI.Xaml.Automation;
using Microsoft.UI.Xaml.Automation.Peers;
using Microsoft.UI.Xaml.Media;
using Windows.Foundation;
// Disambiguate the real split-binary types from the stale mock projection
// (Microsoft.UI.Xaml.Controls.TableView*) that the mock Microsoft.WinUI.dll still carries.
using TableView = Microsoft.UI.Xaml.Controls.Tabular.TableView;
using TableViewColumn = Microsoft.UI.Xaml.Controls.Tabular.TableViewColumn;
using TableViewTemplateColumn = Microsoft.UI.Xaml.Controls.Tabular.TableViewTemplateColumn;
using TableViewCellToolTipRequestedEventArgs = Microsoft.UI.Xaml.Controls.Tabular.TableViewCellToolTipRequestedEventArgs;

namespace TableViewSampleApp;

// Exercises the opt-in per-cell tooltip feature end to end: string content, non-string content with
// a UIA override, cells that deliberately get no tooltip, a cell whose own template owns a tooltip,
// late attach/detach with InvalidateCellToolTips, re-entrant invalidation from inside the handler,
// and a throwing handler (which must be contained, not fatal).
public sealed partial class ToolTipsPage : Page
{
    private readonly TableViewColumn _nameColumn;
    private readonly TableViewColumn _bioColumn;
    private readonly TableViewColumn _scoreColumn;

    private TypedEventHandler<TableView, TableViewCellToolTipRequestedEventArgs>? _handler;
    private bool _attached;
    private bool _enabled = true;
    private bool _throwInHandler;
    private int _peerCells;
    private bool _reentrant;
    private int _requestCount;
    private int _mutations;
    private bool _inHandler;
    private readonly HashSet<string> _itemsSeen = new();

    public ToolTipsPage()
    {
        this.InitializeComponent();

        _nameColumn = SampleColumns.Text("Name", nameof(Item.Name), SampleColumns.Pixels(140));
        _bioColumn = SampleColumns.Text("Bio", nameof(Item.Bio), SampleColumns.Pixels(180));
        _scoreColumn = SampleColumns.Text("Score", nameof(Item.Score), SampleColumns.Pixels(70));

        Table.Columns.Add(_nameColumn);
        Table.Columns.Add(_bioColumn);
        Table.Columns.Add(_scoreColumn);
        Table.Columns.Add(new TableViewTemplateColumn
        {
            Header = "Notes (app tooltip)",
            CellTemplate = (DataTemplate)Resources["AppToolTipCell"],
            Width = SampleColumns.Pixels(220),
        });

        Table.ItemsSource = Data.Make();

        _handler = OnCellToolTipRequested;
        Table.CellToolTipRequested += _handler;
        _attached = true;

        UpdateStatus();
    }

    private void OnCellToolTipRequested(TableView sender, TableViewCellToolTipRequestedEventArgs args)
    {
        _requestCount++;

        if (_throwInHandler)
        {
            throw new InvalidOperationException("Deliberate handler fault - the control must contain this.");
        }

        // Re-entrancy: the API documents that this is safe to call from a handler. If the coalescing
        // guard regressed, this recurses without bound instead of queueing one more pass.
        if (_reentrant && !_inHandler)
        {
            _inHandler = true;
            try
            {
                sender.InvalidateCellToolTips();
            }
            finally
            {
                _inHandler = false;
            }
        }

        if (!_enabled || args.Item is not Item item)
        {
            return;
        }

        // Recycling proof: a recycled row must re-raise for its NEW item, so this set grows past the
        // ~28 rows realized on the first screen once the table is scrolled.
        _itemsSeen.Add(item.Name + "|" + item.Score);

        if (args.Column == _bioColumn)
        {
            // Plain string content: the control publishes it as UIA HelpText too.
            args.Content = item.Bio + (_mutations > 0 ? $" [rev {_mutations}]" : string.Empty);
        }
        else if (args.Column == _nameColumn)
        {
            // Non-string content: UIA cannot read it, so AutomationHelpText supplies the spoken text.
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

            args.Content = panel;
            args.AutomationHelpText = $"{item.Name}, {item.Role}, {item.City}";
        }
        // Score: deliberately left alone - no tooltip should ever appear over that column.
    }

    private void OnToggleEnabled(object sender, RoutedEventArgs e)
    {
        _enabled = !_enabled;
        EnableToolTips.Content = _enabled ? "Tooltips: ON" : "Tooltips: OFF";
        Table.InvalidateCellToolTips();
        UpdateStatus();
    }

    private void OnToggleThrow(object sender, RoutedEventArgs e)
    {
        _throwInHandler = !_throwInHandler;
        ThrowInHandler.Content = _throwInHandler ? "Throwing handler: ON" : "Throwing handler: OFF";
        Table.InvalidateCellToolTips();
        UpdateStatus();
    }

    private void OnToggleReentrant(object sender, RoutedEventArgs e)
    {
        _reentrant = !_reentrant;
        ReentrantInvalidate.Content = _reentrant ? "Reentrant invalidate: ON" : "Reentrant invalidate: OFF";
        Table.InvalidateCellToolTips();
        UpdateStatus();
    }

    private void OnAttachLate(object sender, RoutedEventArgs e)
    {
        if (_attached)
        {
            Table.CellToolTipRequested -= _handler;
            _attached = false;
            AttachLate.Content = "Attach handler";
        }
        else
        {
            _handler ??= OnCellToolTipRequested;
            Table.CellToolTipRequested += _handler;
            _attached = true;
            AttachLate.Content = "Detach handler";
        }

        // Rows are already realized, so the pass only reruns because of this call.
        Table.InvalidateCellToolTips();
        UpdateStatus();
    }

    private void OnMutateData(object sender, RoutedEventArgs e)
    {
        _mutations++;
        Table.InvalidateCellToolTips();
        UpdateStatus();
    }

    private void UpdateStatus()
    {
        Status.Text = $"handler {(_attached ? "attached" : "detached")} - requests raised: {_requestCount}"
            + $" - data revision: {_mutations} - distinct items seen: {_itemsSeen.Count}";
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
        // Scrolling is what recycles rows - the path rounds 1-3 found the UAF and the stale-content
        // defects on. Nothing else in this page exercises it.
        // The template has several ScrollViewers (the header band scrolls too); pick the one that
        // can actually scroll vertically, or this silently tests nothing.
        var scrollers = Descendants(Table).OfType<ScrollViewer>().ToList();
        var scroller = scrollers.OrderByDescending(s => s.ScrollableHeight).FirstOrDefault();
        if (scroller is null || scroller.ScrollableHeight <= 0)
        {
            Status.Text = $"SCROLL: no vertically scrollable ScrollViewer (found {scrollers.Count})";
            return;
        }

        var before = scroller.VerticalOffset;
        _ = scroller.ChangeView(null, before + 900, null, true);

        // Report the offset so a scroll that silently does nothing cannot look like a passing test.
        DispatcherQueue.TryEnqueue(() =>
        {
            Status.Text = $"SCROLL: {before:F0} -> {scroller.VerticalOffset:F0}"
                + $" (max {scroller.ScrollableHeight:F0}) - requests {_requestCount}"
                + $" - distinct items {_itemsSeen.Count}";
        });
    }

    // Reads the live cell wrappers and reports, per column, who owns each cell's tooltip. This is
    // the only way to check the ownership rules without UIA (whose tree walks are unreliable here).
    private void OnCheckOwnership(object sender, RoutedEventArgs e)
    {
        var perColumn = new Dictionary<string, (int cells, int controlTips, int appTipsInContent, int helpTexts)>();

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
            }

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
              .Append($"appTip={kv.Value.appTipsInContent} help={kv.Value.helpTexts}   ");
        }

        var dup = CountDuplicateSpokenHelpText();
        sb.Append($"UIA-cells={_peerCells} UIA-DUPhelp={dup}");

        Status.Text = sb.Length > 0 ? sb.ToString() : "no cells found";
    }

    // Counts cells whose spoken HelpText merely repeats the value already in their spoken Name, i.e.
    // the ones Narrator would read twice. Walks the automation peers rather than the attached
    // property: the control publishes HelpText eagerly and suppresses the redundant case at query
    // time, so only the peer reports what is actually announced.
    private int CountDuplicateSpokenHelpText()
    {
        var duplicates = 0;
        _peerCells = 0;

        void Visit(AutomationPeer peer)
        {
            if (peer.GetAutomationControlType() == AutomationControlType.DataItem)
            {
                _peerCells++;
                var help = peer.GetHelpText();
                var name = peer.GetName();
                if (!string.IsNullOrEmpty(help) && !string.IsNullOrEmpty(name) && name.EndsWith(help, StringComparison.Ordinal))
                {
                    duplicates++;
                }
            }

            foreach (var child in peer.GetChildren() ?? new List<AutomationPeer>())
            {
                Visit(child);
            }
        }

        var root = FrameworkElementAutomationPeer.CreatePeerForElement(Table);
        if (root is not null)
        {
            Visit(root);
        }

        return duplicates;
    }
}
