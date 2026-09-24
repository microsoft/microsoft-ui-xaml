using Microsoft.UI.Xaml.Controls;

namespace TableViewSampleApp;

// Table where every column is GridUnitType.Star — columns split the viewport width by their factors.
public sealed partial class StarColumnsPage : Page
{
    public StarColumnsPage()
    {
        this.InitializeComponent();

        Table.Columns.Add(SampleColumns.Text("Name", nameof(Item.Name), SampleColumns.Star(1)));
        Table.Columns.Add(SampleColumns.Text("Role", nameof(Item.Role), SampleColumns.Star(1)));
        Table.Columns.Add(SampleColumns.Text("City", nameof(Item.City), SampleColumns.Star(2)));
        Table.Columns.Add(SampleColumns.Text("Bio", nameof(Item.Bio), SampleColumns.Star(3)));

        Table.ItemsSource = Data.Make();
    }
}
