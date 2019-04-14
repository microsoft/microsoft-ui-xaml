using System;
using System.Collections.Generic;
using System.Collections.ObjectModel;
using System.Diagnostics;
using System.IO;
using System.Linq;
using System.Net;
using System.Runtime.InteropServices.WindowsRuntime;
using System.Xml.Linq;
using Windows.Foundation;
using Windows.Foundation.Collections;
using Windows.UI.Xaml;
using Windows.UI.Xaml.Controls;
using Windows.UI.Xaml.Controls.Primitives;
using Windows.UI.Xaml.Data;
using Windows.UI.Xaml.Input;
using Windows.UI.Xaml.Media;
using Windows.UI.Xaml.Media.Imaging;
using Windows.UI.Xaml.Navigation;

// The Blank Page item template is documented at https://go.microsoft.com/fwlink/?LinkId=402352&clcid=0x409

namespace Flick
{
    /// <summary>
    /// An empty page that can be used on its own or navigated to within a Frame.
    /// </summary>
    public sealed partial class MainPage : Page
    {
        ObservableCollection<Photo> Images = new ObservableCollection<Photo>();
        public MainPage()
        {
            this.InitializeComponent();

            this.Loaded += MainPage_Loaded;
        }

        private void MainPage_Loaded(object sender, RoutedEventArgs e)
        {
            // https://www.flickr.com/services/api/flickr.photos.search.html
            var url = "https://api.flickr.com/services/rest/?method=flickr.photos.search&api_key=dc318dbb02bf7cd2ab3daca1ae7d93c8&tags=supercar&styles=depthoffield&safe_search=1&per_page=100&page=1";
            LoadFeed(url);
        }

        private async void LoadFeed(string url)
        {
            Windows.Web.Http.HttpClient httpClient = new Windows.Web.Http.HttpClient();
            Uri requestUri = new Uri(url);
            Windows.Web.Http.HttpResponseMessage httpResponse = new Windows.Web.Http.HttpResponseMessage();
            string httpResponseBody = "";

            try
            {
                //Send the GET request
                httpResponse = await httpClient.GetAsync(requestUri);
                httpResponse.EnsureSuccessStatusCode();
                httpResponseBody = await httpResponse.Content.ReadAsStringAsync();
            }
            catch (Exception ex)
            {
                httpResponseBody = "Error: " + ex.HResult.ToString("X") + " Message: " + ex.Message;
            }

            XElement root = XElement.Parse(httpResponseBody);
            var entries = root.Descendants()
                          .Where(x => x.Name.LocalName == "photo")
                          .ToList();

            Console.WriteLine("There are {0} nodes...", entries.Count());
            Images.Clear();
            foreach (XElement v in entries)
            {
                Images.Add(Photo.Parse(v));
            }
            repeater.ItemsSource = Images;
            repeater.Layout = activityLayout;// gridLayout;
            banner.Source = new BitmapImage(new Uri(Images.Last().LargeUrl));
        }

        private void Image_Tapped(object sender, TappedRoutedEventArgs e)
        {
            Frame.Navigate(typeof(MasterDetail), new NavigateArgs() { Photos = Images, Selected = (sender as Image).DataContext as Photo });
        }

        private void AppBarButton_Click(object sender, RoutedEventArgs e)
        {
            repeater.Layout = stackLayout;
        }

        private void AppBarButton_Click_1(object sender, RoutedEventArgs e)
        {
            repeater.Layout = uniformGridLayout;
        }

        private void AppBarButton_Click_2(object sender, RoutedEventArgs e)
        {
            repeater.Layout = activityLayout;
        }

        private void AppBarButton_Click_3(object sender, RoutedEventArgs e)
        {
            Frame.Navigate(typeof(GroupedPage), new NavigateArgs() { Photos = Images });
        }

        private void AppBarButton_Click_4(object sender, RoutedEventArgs e)
        {
            Frame.Navigate(typeof(StoreScenario), new NavigateArgs() { Photos = Images });
        }

        private void AutoSuggest_SuggestionChosen(AutoSuggestBox sender, AutoSuggestBoxSuggestionChosenEventArgs args)
        {

        }

        private void AutoSuggest_TextChanged(AutoSuggestBox sender, AutoSuggestBoxTextChangedEventArgs args)
        {

        }

        private void AutoSuggest_QuerySubmitted(AutoSuggestBox sender, AutoSuggestBoxQuerySubmittedEventArgs args)
        {
            if (args.ChosenSuggestion != null)
            {
                // User selected an item from the suggestion list, take an action on it here.
            }
            else
            {
                // Use args.QueryText to determine what to do.
                var url = "https://api.flickr.com/services/rest/?method=flickr.photos.search&api_key=dc318dbb02bf7cd2ab3daca1ae7d93c8&tags={0}&styles=depthoffield&safe_search=1&per_page=100&page=1";
                LoadFeed(string.Format(url, args.QueryText.Replace(" ", "+")));
            }
        }
    }

    public class NavigateArgs
    {
        public ObservableCollection<Photo> Photos { get; set; }

        public Photo Selected { get; set; }
    }
}
