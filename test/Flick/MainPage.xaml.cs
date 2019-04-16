using System;
using System.Collections.ObjectModel;
using System.Linq;
using Windows.UI.Xaml;
using Windows.UI.Xaml.Controls;
using Windows.UI.Xaml.Input;
using Windows.UI.Xaml.Media.Imaging;

// The Blank Page item template is documented at https://go.microsoft.com/fwlink/?LinkId=402352&clcid=0x409

namespace Flick
{
    /// <summary>
    /// An empty page that can be used on its own or navigated to within a Frame.
    /// </summary>
    public sealed partial class MainPage : Page
    {
        PhotoReel Images = new PhotoReel();
        public MainPage()
        {
            this.InitializeComponent();

            this.Loaded += MainPage_Loaded;
        }

        private async void MainPage_Loaded(object sender, RoutedEventArgs e)
        {
            Images = await FlickApi.GetPhotos("tulips");
            repeater.ItemsSource = Images;
            repeater.Layout = activityLayout;
            banner.Source = new BitmapImage(new Uri(Images.Last().LargeUrl));
        }

        private void Image_Tapped(object sender, TappedRoutedEventArgs e)
        {
            Frame.Navigate(typeof(MasterDetail), new NavigateArgs() { Photos = Images, Selected = (sender as Image).DataContext as Photo });
        }

        private void OnStackLayoutClicked(object sender, RoutedEventArgs e)
        {
            repeater.Layout = stackLayout;
        }

        private void OnUniformGridLayoutClicked(object sender, RoutedEventArgs e)
        {
            repeater.Layout = uniformGridLayout;
        }

        private void OnActivityLayoutClicked(object sender, RoutedEventArgs e)
        {
            repeater.Layout = activityLayout;
        }

        private void OnAnimatedPageClicked(object sender, RoutedEventArgs e)
        {
            Frame.Navigate(typeof(MasterDetail), new NavigateArgs() { Photos = Images, Selected = Images[0] });
        }

        private void OnGroupedPageClicked(object sender, RoutedEventArgs e)
        {
            Frame.Navigate(typeof(GroupedPage), new NavigateArgs() { Photos = Images });
        }

        private void OnStoreScenarioClicked(object sender, RoutedEventArgs e)
        {
            Frame.Navigate(typeof(StoreScenario), new NavigateArgs() { Photos = Images });
        }

        private void OnCarousalClicked(object sender, RoutedEventArgs e)
        {
            Frame.Navigate(typeof(CarousalPage), new NavigateArgs() { Photos = Images, Selected = Images[0] });
        }

        private void On2DGridClicked(object sender, RoutedEventArgs e)
        {
            Frame.Navigate(typeof(UniformGrid2DPage), new NavigateArgs() { Photos = Images, Selected = Images[0] });
        }
        private void OnFlexClicked(object sender, RoutedEventArgs e)
        {
            Frame.Navigate(typeof(FlexDemo), new NavigateArgs() { Photos = Images, Selected = Images[0] });
        }

        private void AutoSuggest_SuggestionChosen(AutoSuggestBox sender, AutoSuggestBoxSuggestionChosenEventArgs args)
        {

        }

        private void AutoSuggest_TextChanged(AutoSuggestBox sender, AutoSuggestBoxTextChangedEventArgs args)
        {

        }

        private async void AutoSuggest_QuerySubmitted(AutoSuggestBox sender, AutoSuggestBoxQuerySubmittedEventArgs args)
        {
            if (args.ChosenSuggestion != null)
            {
                // User selected an item from the suggestion list, take an action on it here.
            }
            else
            {
                Images = await FlickApi.GetPhotos(args.QueryText.Replace(" ", "+"));
                repeater.ItemsSource = Images;
                banner.Source = new BitmapImage(new Uri(Images.Last().LargeUrl));
            }
        }
    }

    public class NavigateArgs
    {
        public ObservableCollection<Photo> Photos { get; set; }

        public Photo Selected { get; set; }
    }
}
