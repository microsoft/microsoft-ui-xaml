using System;
using Microsoft.UI;
using Microsoft.UI.Xaml;
using Microsoft.UI.Xaml.Controls;
using Microsoft.UI.Xaml.Data;
using Microsoft.UI.Xaml.Media;
using Windows.UI;
using Microsoft.UI.Xaml.Controls.Tabular;
// Disambiguate the real split-binary control types from the stale mock projection
// (Microsoft.UI.Xaml.Controls.TableView*) that the mock Microsoft.WinUI.dll still carries.
using TableViewTextColumn = Microsoft.UI.Xaml.Controls.Tabular.TableViewTextColumn;
using TableViewTemplateColumn = Microsoft.UI.Xaml.Controls.Tabular.TableViewTemplateColumn;
using TableViewHeadersVisibility = Microsoft.UI.Xaml.Controls.Tabular.TableViewHeadersVisibility;
using TableViewGridLinesVisibility = Microsoft.UI.Xaml.Controls.Tabular.TableViewGridLinesVisibility;
using TableViewDensity = Microsoft.UI.Xaml.Controls.Tabular.TableViewDensity;
using TableViewFrozenEdge = Microsoft.UI.Xaml.Controls.Tabular.TableViewFrozenEdge;

namespace TableViewSampleApp;

// Minimal sample: builds a TableView (real split-binary control) with one column of each kind,
// then wires the left panel so every public API can be exercised live. Data lives in Data.cs.
public sealed partial class MainWindow : Window
{
    private TableViewTextColumn _name = null!;
    private TableViewTextColumn _role = null!;
    private TableViewTextColumn _city = null!;
    private TableViewTemplateColumn _score = null!;
    private int _added;
    private bool _ready;   // guards combo SelectionChanged that fires during XAML load

    private static DataTemplate Tmpl(string key) => (DataTemplate)Application.Current.Resources[key];

    public MainWindow()
    {
        this.InitializeComponent();
        this.Title = "TableView Sample";

        BuildColumns();
        Table.EmptyTemplate = Tmpl("EmptyState");
        Table.ItemsSource = Data.Make();
        _ready = true;
    }

    private void BuildColumns()
    {
        _name = new TableViewTextColumn
        {
            Header = "Name",
            Binding = new Binding { Path = new PropertyPath(nameof(Item.Name)) },
            Width = new GridLength(1, GridUnitType.Auto),
        };
        _role = new TableViewTextColumn
        {
            Header = "Role",
            Binding = new Binding { Path = new PropertyPath(nameof(Item.Role)) },
            Width = new GridLength(1, GridUnitType.Star),
        };
        _city = new TableViewTextColumn
        {
            Header = "City",
            Binding = new Binding { Path = new PropertyPath(nameof(Item.City)) },
            Width = new GridLength(1, GridUnitType.Star),
        };
        _score = new TableViewTemplateColumn
        {
            Header = "Score",
            CellTemplate = Tmpl("ScoreCell"),
            Width = new GridLength(120, GridUnitType.Pixel),
        };

        var bio = new TableViewTemplateColumn
        {
            Header = "Bio",
            CellTemplate = Tmpl("BioCell"),
            Width = new GridLength(1, GridUnitType.Star),
        };
        var joined = new TableViewTemplateColumn
        {
            Header = "Joined",
            CellTemplate = Tmpl("JoinedCell"),
            Width = new GridLength(240, GridUnitType.Pixel),
        };
        var notes = new TableViewTemplateColumn
        {
            Header = "Notes",
            CellTemplate = Tmpl("NotesCell"),
            Width = new GridLength(160, GridUnitType.Auto),
        };
        var image = new TableViewTemplateColumn
        {
            Header = "Image",
            CellTemplate = Tmpl("ImageCell"),
            Width = new GridLength(72, GridUnitType.Pixel),
        };

        Table.Columns.Add(_name);
        Table.Columns.Add(_role);
        Table.Columns.Add(_city);
        Table.Columns.Add(_score);
        Table.Columns.Add(bio);
        Table.Columns.Add(joined);
        Table.Columns.Add(notes);
        Table.Columns.Add(image);
    }

    // ---- Name column width / min / max ----
    private void Width_Changed(object sender, SelectionChangedEventArgs e)
    {
        if (!_ready) return;
        _name.Width = WidthCombo.SelectedIndex switch
        {
            1 => new GridLength(200, GridUnitType.Pixel),
            2 => new GridLength(1, GridUnitType.Star),
            3 => new GridLength(2, GridUnitType.Star),
            _ => new GridLength(1, GridUnitType.Auto),
        };
    }

    private void MinW_Toggle(object sender, RoutedEventArgs e)
        => _name.MinWidth = (MinW.IsChecked == true) ? 120 : 20;

    private void MaxW_Toggle(object sender, RoutedEventArgs e)
        => _name.MaxWidth = (MaxW.IsChecked == true) ? 160 : double.PositiveInfinity;

    // ---- Columns ----
    private void CityVis_Toggle(object sender, RoutedEventArgs e)
        => _city.Visibility = (CityVis.IsChecked == true) ? Visibility.Visible : Visibility.Collapsed;

    private void AddCol_Click(object sender, RoutedEventArgs e)
    {
        _added++;
        Table.Columns.Add(new TableViewTextColumn
        {
            Header = $"Extra {_added}",
            Binding = new Binding { Path = new PropertyPath(nameof(Item.Role)) },
            Width = new GridLength(1, GridUnitType.Auto),
        });
    }

    private void RemoveCol_Click(object sender, RoutedEventArgs e)
    {
        if (Table.Columns.Count > 1)
        {
            Table.Columns.RemoveAt(Table.Columns.Count - 1);
        }
    }

    // ---- Headers / gridlines / density ----
    private void Headers_Changed(object sender, SelectionChangedEventArgs e)
    {
        if (!_ready) return;
        Table.HeadersVisibility = HeadersCombo.SelectedIndex == 1
            ? TableViewHeadersVisibility.None
            : TableViewHeadersVisibility.Column;
    }

    private void GridLines_Changed(object sender, SelectionChangedEventArgs e)
    {
        if (!_ready) return;
        Table.GridLinesVisibility = GridLinesCombo.SelectedIndex switch
        {
            1 => TableViewGridLinesVisibility.Horizontal,
            2 => TableViewGridLinesVisibility.Vertical,
            3 => TableViewGridLinesVisibility.None,
            _ => TableViewGridLinesVisibility.All,
        };
    }

    private void Density_Changed(object sender, SelectionChangedEventArgs e)
    {
        if (!_ready) return;
        Table.Density = DensityCombo.SelectedIndex switch
        {
            0 => TableViewDensity.Compact,
            2 => TableViewDensity.Comfortable,
            _ => TableViewDensity.Standard,
        };
    }

    // ---- Row banding ----
    private void RowBg_Click(object sender, RoutedEventArgs e)
        => Table.RowBackground = new SolidColorBrush(Color.FromArgb(24, 0, 120, 215));

    private void AltBg_Click(object sender, RoutedEventArgs e)
        => Table.AlternatingRowBackground = new SolidColorBrush(Color.FromArgb(20, 128, 128, 128));

    private void ClearBg_Click(object sender, RoutedEventArgs e)
    {
        Table.RowBackground = null;
        Table.AlternatingRowBackground = null;
    }

    // ---- Frozen / read-only / header template / empty ----
    private void Freeze_Toggle(object sender, RoutedEventArgs e)
        => _name.FrozenEdge = (FreezeToggle.IsChecked == true) ? TableViewFrozenEdge.Leading : TableViewFrozenEdge.None;

    private void ReadOnly_Toggle(object sender, RoutedEventArgs e)
        => Table.IsReadOnly = ReadOnlyToggle.IsChecked == true;

    private void HeaderTmpl_Toggle(object sender, RoutedEventArgs e)
        => _score.HeaderTemplate = (HeaderTmplToggle.IsChecked == true) ? Tmpl("StarHeader") : null;

    private void Empty_Toggle(object sender, RoutedEventArgs e)
    {
        if (EmptyToggle.IsChecked == true)
        {
            Table.ItemsSource = null;
            EmptyToggle.Content = "Load data";
        }
        else
        {
            Table.ItemsSource = Data.Make();
            EmptyToggle.Content = "Clear data";
        }
    }
}
