using System.Collections.Generic;
using Microsoft.UI.Xaml;
using Microsoft.UI.Xaml.Controls;
using Microsoft.UI.Xaml.Controls.Tabular;

namespace TableViewSampleApp;

// Every column hosts an interactive Expander cell; the width-mode combo applies the SAME width kind
// (Auto / Star / Pixel) to ALL columns so Auto and Star sizing can be compared with live content.
public sealed partial class InteractiveCellsPage : Page
{
    private readonly List<TableViewColumn> _columns = new();
    private bool _ready;   // guards the combo SelectionChanged that fires during XAML load

    public InteractiveCellsPage()
    {
        this.InitializeComponent();

        _columns.Add(SampleColumns.Template("Name", "ExpandNameCell", SampleColumns.Auto()));
        _columns.Add(SampleColumns.Template("Role", "ExpandRoleCell", SampleColumns.Auto()));
        _columns.Add(SampleColumns.Template("Bio", "ExpandBioCell", SampleColumns.Auto()));
        _columns.Add(SampleColumns.Template("Image", "ExpandImageCell", SampleColumns.Auto()));
        _columns.Add(SampleColumns.Template("Width", "GrowWidthCell", SampleColumns.Auto()));

        foreach (var column in _columns)
        {
            Table.Columns.Add(column);
        }

        Table.ItemsSource = Data.Make();
        _ready = true;
    }

    private void Width_Changed(object sender, SelectionChangedEventArgs e)
    {
        if (!_ready) return;

        GridLength width = WidthCombo.SelectedIndex switch
        {
            1 => SampleColumns.Star(1),
            2 => SampleColumns.Pixels(200),
            _ => SampleColumns.Auto(),
        };

        foreach (var column in _columns)
        {
            column.Width = width;
        }
    }
}
