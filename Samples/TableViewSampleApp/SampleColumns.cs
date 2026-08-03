using Microsoft.UI.Xaml;
using Microsoft.UI.Xaml.Data;
// Disambiguate the real split-binary column types from the stale mock projection
// (Microsoft.UI.Xaml.Controls.TableView*) that the mock Microsoft.WinUI.dll still carries.
using TableViewTextColumn = Microsoft.UI.Xaml.Controls.Tabular.TableViewTextColumn;
using TableViewTemplateColumn = Microsoft.UI.Xaml.Controls.Tabular.TableViewTemplateColumn;

namespace TableViewSampleApp;

// Small factory used by the column-width sample pages so each page only has to declare the
// header / bound property / width for its columns. Centralizes the Tabular type aliases.
internal static class SampleColumns
{
    // A text column bound one-way to an Item property, with the given width.
    public static TableViewTextColumn Text(string header, string propertyPath, GridLength width) =>
        new TableViewTextColumn
        {
            Header = header,
            Binding = new Binding { Path = new PropertyPath(propertyPath) },
            Width = width,
        };

    // A template column using a DataTemplate declared in App.xaml resources, with the given width.
    public static TableViewTemplateColumn Template(string header, string templateKey, GridLength width) =>
        new TableViewTemplateColumn
        {
            Header = header,
            CellTemplate = (DataTemplate)Application.Current.Resources[templateKey],
            Width = width,
        };

    // Convenience width factories.
    public static GridLength Auto() => new GridLength(1, GridUnitType.Auto);
    public static GridLength Star(double factor = 1) => new GridLength(factor, GridUnitType.Star);
    public static GridLength Pixels(double px) => new GridLength(px, GridUnitType.Pixel);
}
