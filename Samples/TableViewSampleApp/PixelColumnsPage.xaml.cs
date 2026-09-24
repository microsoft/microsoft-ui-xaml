using Microsoft.UI.Xaml.Controls;

namespace TableViewSampleApp;

// Table where every column has a fixed GridUnitType.Pixel width (content-independent).
public sealed partial class PixelColumnsPage : Page
{
    public PixelColumnsPage()
    {
        this.InitializeComponent();

        Table.Columns.Add(SampleColumns.Text("Name", nameof(Item.Name), SampleColumns.Pixels(220)));
        Table.Columns.Add(SampleColumns.Text("Role", nameof(Item.Role), SampleColumns.Pixels(120)));
        Table.Columns.Add(SampleColumns.Text("City", nameof(Item.City), SampleColumns.Pixels(160)));
        Table.Columns.Add(SampleColumns.Template("Score", "ScoreCell", SampleColumns.Pixels(140)));
        Table.Columns.Add(SampleColumns.Text("Notes", nameof(Item.Notes), SampleColumns.Pixels(200)));

        Table.ItemsSource = Data.Make();
    }
}
