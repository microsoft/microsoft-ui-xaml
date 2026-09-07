using System.Collections.Generic;
using System.Collections.ObjectModel;
using System.Linq;
using System.Text;
using Microsoft.UI.Xaml.Controls;
using Microsoft.UI.Xaml.Controls.Tabular;
// tabular-namespace TableView aliases: disambiguate from the (stale-mock) base Microsoft.UI.Xaml.Controls.TableView projection
using TableView = Microsoft.UI.Xaml.Controls.Tabular.TableView;
using TableViewSelectionChangedEventArgs = Microsoft.UI.Xaml.Controls.SelectionChangedEventArgs;
using TableViewSelectionMode = Microsoft.UI.Xaml.Controls.Tabular.TableViewSelectionMode;
using TableViewSampleApp.Data;
using TableViewSampleApp.Models;

namespace TableViewSampleApp.Pages;

public sealed partial class SelectionPage : Page
{
    private int _changeCount;

    public SelectionPage()
    {
        People = PersonData.Take(50);
        InitializeComponent();
        PeopleTable.HeadersVisibility = TableViewHeadersVisibility.Column;
        PeopleTable.ItemsSource = People;
        Loaded += (_, _) => { UpdateModeDescription((ModeCombo?.SelectedItem as ComboBoxItem)?.Content as string); RefreshReadout(); };
    }

    public ObservableCollection<Person> People { get; }

    private void OnModeChanged(object sender, SelectionChangedEventArgs e)
    {
        if (PeopleTable is null || ModeCombo.SelectedItem is not ComboBoxItem item) return;
        var label = item.Content as string;
        PeopleTable.SelectionMode = label switch
        {
            "None" => TableViewSelectionMode.None,
            "Single" => TableViewSelectionMode.Single,
            "Multiple" => TableViewSelectionMode.Single,
            "Extended" => TableViewSelectionMode.Single,
            _ => TableViewSelectionMode.Single,
        };
        UpdateModeDescription(label);
        RefreshReadout();
    }

    private void UpdateModeDescription(string? mode)
    {
        if (ModeDescriptionText is null) return;
        ModeDescriptionText.Text = mode switch
        {
            "None" => "Rows cannot be selected.",
            "Single" => "Click a row to select exactly one item.",
            "Multiple" => "Use the API buttons to build a multiple selection, including ranges.",
            "Extended" => "Use extended range selection gestures or the API buttons to build a multiple selection.",
            _ => "Use the API buttons to build a multiple selection, including ranges.",
        };
    }

    private void OnSelectFirstClick(object sender, Microsoft.UI.Xaml.RoutedEventArgs e) { if (People.Count > 0) PeopleTable.Select(0); }
    private void OnSelectLastClick(object sender, Microsoft.UI.Xaml.RoutedEventArgs e) { if (People.Count > 0) PeopleTable.Select(People.Count - 1); }
    // Select all is not available: selection is single-item this release (None or Single).
    private void OnClearClick(object sender, Microsoft.UI.Xaml.RoutedEventArgs e) => PeopleTable.DeselectAll();

    private void OnInvertClick(object sender, Microsoft.UI.Xaml.RoutedEventArgs e)
    {
        var selected = new System.Collections.Generic.HashSet<int>(PeopleTable.SelectedIndex);
        for (int i = 0; i < People.Count; i++)
        {
            if (selected.Contains(i)) PeopleTable.Deselect(i); else PeopleTable.Select(i);
        }
    }

    private void OnTableSelectionChanged(TableView sender, TableViewSelectionChangedEventArgs args)
    {
        _changeCount++;
        RefreshReadout();
    }

    private void RefreshReadout()
    {
        if (PeopleTable is null || SelectedCountText is null) return;
        SelectedCountText.Text = (PeopleTable.SelectedItem is null ? 0 : 1).ToString();
        SelectedIndexText.Text = PeopleTable.SelectedIndex.ToString();
        ChangeCountText.Text = _changeCount.ToString();
        SelectedIndicesText.Text = PeopleTable.SelectedIndex < 0 ? "(none)" : PeopleTable.SelectedIndex.ToString();
        var items = PeopleTable.SelectedItem is null ? new List<object>() : new List<object> { PeopleTable.SelectedItem };
        if (items.Count == 0) { SelectedItemsText.Text = "(none)"; return; }
        var preview = items.Cast<Person>().Take(5).Select(p => $"{p.FirstName} {p.LastName}");
        var sb = new StringBuilder(string.Join(", ", preview));
        if (items.Count > 5) sb.Append(", … (+").Append(items.Count - 5).Append(" more)");
        SelectedItemsText.Text = sb.ToString();
    }
}
