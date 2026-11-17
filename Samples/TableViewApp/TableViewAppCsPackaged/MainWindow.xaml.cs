using System.Collections.Generic;
using Microsoft.UI.Xaml;
using Microsoft.UI.Xaml.Controls.Tabular;
using Microsoft.UI.Xaml.Markup;

namespace TableViewAppCsPackaged
{
    public class Person
    {
        public string Name { get; set; } = string.Empty;
        public int Age { get; set; }
    }

    public sealed partial class MainWindow : Window
    {
        // Bound from markup via x:Bind, so it must be populated before InitializeComponent.
        public List<Person> People { get; } = new List<Person>
        {
            new Person { Name = "Alice", Age = 30 },
            new Person { Name = "Bob", Age = 25 },
            new Person { Name = "Carol", Age = 40 },
        };

        public MainWindow()
        {
            this.InitializeComponent();

            // Second instance built entirely from code, to cover the non-markup activation path.
            var codeTable = new TableView { Height = 160, ItemsSource = People };
            codeTable.Columns.Add(MakeColumn("Name", "Name"));
            codeTable.Columns.Add(MakeColumn("Age", "Age"));
            CodeTableHost.Children.Add(codeTable);

            StatusText.Text = $"Both tables are bound to the same {People.Count} items.";
        }

        // A text column is a TableViewTemplateColumn whose cell is a TextBlock bound to the property.
        private static TableViewTemplateColumn MakeColumn(string header, string path) => new TableViewTemplateColumn
        {
            Header = header,
            SortMemberPath = path,
            CellTemplate = (DataTemplate)XamlReader.Load(
                "<DataTemplate xmlns='http://schemas.microsoft.com/winfx/2006/xaml/presentation'>" +
                $"<TextBlock Text='{{Binding {path}}}' VerticalAlignment='Center' Padding='8,4,8,4' TextTrimming='CharacterEllipsis' />" +
                "</DataTemplate>"),
        };

        private void ToggleTheme_Click(object sender, RoutedEventArgs e)
        {
            RootGrid.RequestedTheme = RootGrid.RequestedTheme == ElementTheme.Dark
                ? ElementTheme.Light
                : ElementTheme.Dark;
        }
    }
}