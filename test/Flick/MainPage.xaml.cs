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
    }

    public class NavigateArgs
    {
        public ObservableCollection<Photo> Photos { get; set; }

        public Photo Selected { get; set; }
    }

    // <photo id="47482260852" owner="65635049@N08" secret="27c7ff6465" server="7839" farm="8" title="Chantilly Arts &amp; Elegance 2016 - Bugatti Veyron 16.4 Super Sport WRC" ispublic="1" isfriend="0" isfamily="0" />
    public class Photo
    {
        public string Id { get; set; }

        public string Owner { get; set; }

        public string Secret { get; set; }

        public string Server { get; set; }

        public string Farm { get; set; }

        public string Title { get; set; }

        public string UrlMedium
        {
            get
            {
                //  https://www.flickr.com/services/api/misc.urls.html
                return string.Format("https://farm{0}.staticflickr.com/{1}/{2}_{3}_m.jpg", Farm, Server, Id, Secret);
            }
        }

        public string LargeUrl
        {
            get
            {
                //  https://www.flickr.com/services/api/misc.urls.html
                return string.Format("https://farm{0}.staticflickr.com/{1}/{2}_{3}_b.jpg", Farm, Server, Id, Secret);
            }
        }

        public static Photo Parse(XElement element)
        {
            Photo photo = new Photo();
            foreach (var attr in element.Attributes())
            {
                switch (attr.Name.ToString())
                {
                    case "id":
                        photo.Id = attr.Value;
                        break;
                    case "owner":
                        photo.Owner = attr.Value;
                        break;
                    case "secret":
                        photo.Secret = attr.Value;
                        break;
                    case "farm":
                        photo.Farm = attr.Value;
                        break;
                    case "title":
                        photo.Title = attr.Value;
                        break;
                    case "server":
                        photo.Server = attr.Value;
                        break;
                    default:
                        break;
                }
            }

            return photo;
        }


    }
}
