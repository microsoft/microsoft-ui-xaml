using Microsoft.UI.Xaml.Controls;

namespace TableViewSampleApp;

// Table mixing Auto, Pixel and Star column widths (plus a Template column).
public sealed partial class MixedColumnsPage : Page
{
    public MixedColumnsPage()
    {
        this.InitializeComponent();

        Table.Columns.Add(SampleColumns.Text("Name", nameof(Item.Name), SampleColumns.Auto()));
        Table.Columns.Add(SampleColumns.Text("Role", nameof(Item.Role), SampleColumns.Pixels(120)));
        Table.Columns.Add(SampleColumns.Template("Score", "ScoreCell", SampleColumns.Pixels(120)));
        Table.Columns.Add(SampleColumns.Text("City", nameof(Item.City), SampleColumns.Star(1)));
        Table.Columns.Add(SampleColumns.Text("Bio", nameof(Item.Bio), SampleColumns.Star(2)));

        Table.ItemsSource = Data.Make();
    }
}
