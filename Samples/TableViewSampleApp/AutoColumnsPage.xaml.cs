using Microsoft.UI.Xaml.Controls;

namespace TableViewSampleApp;

// Table where every column is GridUnitType.Auto — each column sizes to the widest realized cell.
public sealed partial class AutoColumnsPage : Page
{
    public AutoColumnsPage()
    {
        this.InitializeComponent();

        Table.Columns.Add(SampleColumns.Text("Name", nameof(Item.Name), SampleColumns.Auto()));
        Table.Columns.Add(SampleColumns.Text("Role", nameof(Item.Role), SampleColumns.Auto()));
        Table.Columns.Add(SampleColumns.Text("City", nameof(Item.City), SampleColumns.Auto()));
        Table.Columns.Add(SampleColumns.Text("Score", nameof(Item.Score), SampleColumns.Auto()));
        // Editable multi-line TextBox: type a longer line to grow the Auto column width, add lines
        // (Enter) to grow the Auto row height -- exercises both column and row resizing live.
        Table.Columns.Add(SampleColumns.Template("Notes (edit)", "AutoEditCell", SampleColumns.Auto()));

        Table.ItemsSource = Data.Make();
    }
}
