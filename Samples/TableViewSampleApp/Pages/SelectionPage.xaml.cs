using System.Collections.ObjectModel;
using System.Globalization;
using Microsoft.UI.Xaml.Controls;
using Microsoft.UI.Xaml.Controls.Tabular;
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
            "Single" => "Click a row or use the buttons to select exactly one item.",
            _ => "Click a row or use the buttons to select exactly one item.",
        };
    }

    private void OnSelectFirstClick(object sender, Microsoft.UI.Xaml.RoutedEventArgs e) { if (People.Count > 0) PeopleTable.Select(0); }
    private void OnSelectLastClick(object sender, Microsoft.UI.Xaml.RoutedEventArgs e) { if (People.Count > 0) PeopleTable.Select(People.Count - 1); }
    private void OnClearClick(object sender, Microsoft.UI.Xaml.RoutedEventArgs e) => PeopleTable.DeselectAll();

    private void OnTableSelectionChanged(TableView sender, TableViewSelectionChangedEventArgs args)
    {
        _changeCount++;
        RefreshReadout();
    }

    private void RefreshReadout()
    {
        if (PeopleTable is null || SelectedCountText is null) return;
        SelectedCountText.Text = (PeopleTable.SelectedItem is null ? 0 : 1).ToString(CultureInfo.InvariantCulture);
        SelectedIndexText.Text = PeopleTable.SelectedIndex.ToString(CultureInfo.InvariantCulture);
        ChangeCountText.Text = _changeCount.ToString(CultureInfo.InvariantCulture);
        SelectedItemText.Text = PeopleTable.SelectedItem is Person person
            ? $"{person.FirstName} {person.LastName}"
            : "(none)";
    }
}
