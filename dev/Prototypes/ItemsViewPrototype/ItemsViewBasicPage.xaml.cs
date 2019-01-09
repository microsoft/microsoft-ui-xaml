using DEPControlsTestApp.ItemsViewPrototype;
using Microsoft.UI.Xaml.Controls;
using System;
using System.Collections.Generic;
using System.Collections.ObjectModel;
using System.Diagnostics;
using System.Linq;
using System.Xml.Linq;
using Windows.UI.Xaml;
using Windows.UI.Xaml.Controls;

// The Blank Page item template is documented at https://go.microsoft.com/fwlink/?LinkId=234238

namespace DEPControlsTestApp
{
    public class DebugConverter : Windows.UI.Xaml.Data.IValueConverter
    {
        public object Convert(object value, Type targetType, object parameter, string language)
        {
            Debugger.Launch();
            return value;
        }

        public object ConvertBack(object value, Type targetType, object parameter, string language)
        {
            throw new NotImplementedException();
        }
    }
    /// <summary>
    /// An empty page that can be used on its own or navigated to within a Frame.
    /// </summary>
    public sealed partial class ItemsViewBasicPage : Page
    {
        public ItemsViewBasicPage()
        {
            // Doing this to mimic x:Bind behavior by default for the Bindings
            // on the page
            this.DataContext = this;

            var catalog = XElement.Load("SampleData.xml");
            Books = new ObservableCollection<Book>((from book in catalog.Elements("book")
                                                    select new Book()
                                                    {
                                                        ID = book.Attribute("id").Value,
                                                        Author = book.Descendants("author").First().Value,
                                                        Title = book.Descendants("title").First().Value,
                                                        Price = book.Descendants("price").First().Value,
                                                        Genre = book.Descendants("genre").First().Value,
                                                        Description = book.Descendants("description").First().Value.Trim()
                                                    }));
            ByGenre = from book in Books
                      group book by book.Genre into g
                      select g;

            this.InitializeComponent();

            var headers = new List<string> { "title", "author", "genre", "price", "id" };
            itemsView.SortFunc = (index, asc) =>
            {
                var column = headers[index];
                if (asc)
                {
                    Books = new ObservableCollection<Book>((from book in catalog.Elements("book")
                                                            orderby column == "id" ? book.Attribute("id").Value : book.Descendants(column).First().Value ascending
                                                            select new Book()
                                                            {
                                                                ID = book.Attribute("id").Value,
                                                                Author = book.Descendants("author").First().Value,
                                                                Title = book.Descendants("title").First().Value,
                                                                Price = book.Descendants("price").First().Value,
                                                                Genre = book.Descendants("genre").First().Value,
                                                                Description = book.Descendants("description").First().Value.Trim()
                                                            }));
                }
                else
                {
                    Books = new ObservableCollection<Book>((from book in catalog.Elements("book")
                                                            orderby column == "id" ? book.Attribute("id").Value : book.Descendants(column).First().Value descending
                                                            select new Book()
                                                            {
                                                                ID = book.Attribute("id").Value,
                                                                Author = book.Descendants("author").First().Value,
                                                                Title = book.Descendants("title").First().Value,
                                                                Price = book.Descendants("price").First().Value,
                                                                Genre = book.Descendants("genre").First().Value,
                                                                Description = book.Descendants("description").First().Value.Trim()
                                                            }));
                }

                ByGenre = from book in Books
                          group book by book.Genre into g
                          select g;

                if (itemsView.ItemsSource is IEnumerable<IGrouping<string, Book>>)
                {
                    itemsView.ItemsSource = ByGenre;
                }
                else
                {
                    itemsView.ItemsSource = Books;
                }
            };

            itemsView.FilterFunc = (index, filterText) =>
            {
                var column = headers[index];
                Books = new ObservableCollection<Book>((from book in catalog.Elements("book")
                                                        where book.Descendants(column).First().Value.Contains(filterText)
                                                        select new Book()
                                                        {
                                                            ID = book.Attribute("id").Value,
                                                            Author = book.Descendants("author").First().Value,
                                                            Title = book.Descendants("title").First().Value,
                                                            Price = book.Descendants("price").First().Value,
                                                            Genre = book.Descendants("genre").First().Value,
                                                            Description = book.Descendants("description").First().Value.Trim()
                                                        }));

                ByGenre = from book in Books
                          group book by book.Genre into g
                          select g;

                if (itemsView.ItemsSource is IEnumerable<IGrouping<string, Book>>)
                {
                    itemsView.ItemsSource = ByGenre;
                }
                else
                {
                    itemsView.ItemsSource = Books;
                }
            };
        }

        public ObservableCollection<Book> Books { get; private set; }

        //public IEnumerable<IGrouping<string, Book>> ByGenre { get; set; }

        public IEnumerable<IGrouping<string, Book>> ByGenre
        {
            get { return (IEnumerable<IGrouping<string, Book>>)GetValue(ByGenreProperty); }
            set { SetValue(ByGenreProperty, value); }
        }

        public static readonly DependencyProperty ByGenreProperty =
            DependencyProperty.Register("ByGenre", typeof(IEnumerable<IGrouping<string, Book>>), typeof(ItemsViewBasicPage), new PropertyMetadata(null));

        private void OnSwitchViewClicked(object sender, RoutedEventArgs e)
        {
            var view = (sender as Button).Tag as string;
            itemsView.ViewDefinition = Resources[view] as ViewDefinitionBase;

            if ((sender as Button).Content.ToString().Contains("Group"))
            {
                itemsView.ItemsSource = ByGenre;
            }
            else
            {
                itemsView.ItemsSource = Books;
            }
        }

        private void OnAddAuthorClicked(object sender, RoutedEventArgs e)
        {
            var dt = Resources["authorTemplate"] as DataTemplate;
            var view = itemsViewWithTable.ViewDefinition as TableDefinition;

            var colDef = new ItemsViewColumnDefinition()
            {
                Heading = "Author !!!",
                CellTemplate = dt
            };

            view.ColumnDefinitions.Add(colDef);
        }

        private void TableViewBase_IsGroup(object sender, IsGroupEventArgs args)
        {
            if (args.Item is IGrouping<string, Book>)
            {
                args.IsGroup = true;
            }
        }
    }
}
