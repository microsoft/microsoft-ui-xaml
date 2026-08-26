using System.Collections.Generic;
using Microsoft.UI.Xaml;
using Microsoft.UI.Xaml.Controls.Tabular;

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
            codeTable.Columns.Add(new TableViewTextColumn
            {
                Header = "Name",
                Binding = new Microsoft.UI.Xaml.Data.Binding { Path = new PropertyPath("Name") }
            });
            codeTable.Columns.Add(new TableViewTextColumn
            {
                Header = "Age",
                Binding = new Microsoft.UI.Xaml.Data.Binding { Path = new PropertyPath("Age") }
            });
            CodeTableHost.Children.Add(codeTable);

            StatusText.Text = $"markup TableView: {MarkupTable != null}; code TableView: {CodeTableHost.Children.Count}; items: {People.Count}.";
        }

        private void ToggleTheme_Click(object sender, RoutedEventArgs e)
        {
            RootGrid.RequestedTheme = RootGrid.RequestedTheme == ElementTheme.Dark
                ? ElementTheme.Light
                : ElementTheme.Dark;
        }
    }
}