using System.Collections.ObjectModel;
using Windows.UI.Xaml;
using Windows.UI.Xaml.Controls;
using Windows.UI.Xaml.Input;
using Windows.UI.Xaml.Navigation;

namespace Flick
{
    public sealed partial class GroupedPage : Page
    {
        public GroupedPage()
        {
            this.InitializeComponent();
        }

        protected override async void OnNavigatedTo(NavigationEventArgs e)
        {
            base.OnNavigatedTo(e);
            
            ObservableCollection<PhotoReel> groupedPhotos = new ObservableCollection<PhotoReel>();
            groupedPhotos.Add(await FlickApi.GetPhotos("Allium", 10));
            groupedPhotos.Add(await FlickApi.GetPhotos("Daffodils", 10));
            groupedPhotos.Add(await FlickApi.GetPhotos("Geraniums", 10));
            groupedPhotos.Add(await FlickApi.GetPhotos("Roses", 10));
            groupedPhotos.Add(await FlickApi.GetPhotos("Tulips", 10));
            groupedPhotos.Add(await FlickApi.GetPhotos("Lavendar", 10));
            groupedPhotos.Add(await FlickApi.GetPhotos("Lilies", 10));
            foreach (var group in groupedPhotos)
            {
                for (int i = 0; i < group.Count; i++)
                {
                    group[i].FlexBasis = 300;
                    group[i].FlexGrow = i % 3 + 1;
                }
            }

            rootRepeater.ItemsSource = groupedPhotos;
        }

        private void OnItemGotFocus(object sender, RoutedEventArgs e)
        {
            ScrollToCenterOfViewport(sender);
        }

        private void OnItemClicked(object sender, RoutedEventArgs e)
        {
            ScrollToCenterOfViewport(sender);
        }

        private static void ScrollToCenterOfViewport(object sender)
        {
            var item = sender as FrameworkElement;
            item.StartBringIntoView(new BringIntoViewOptions() {
                HorizontalAlignmentRatio = 0.5,
                VerticalAlignmentRatio = 0.5,
                AnimationDesired = true,
            });
        }

        private void Grid_KeyDown(object sender, KeyRoutedEventArgs e)
        {
            if (e.Key == Windows.System.VirtualKey.Escape)
            {
                Frame.GoBack();
            }
        }

        private void OnStackClicked(object sender, RoutedEventArgs e)
        {
            rootRepeater.ItemTemplate = groupedStack;
        }

        private void OnGridClicked(object sender, RoutedEventArgs e)
        {
            rootRepeater.ItemTemplate = groupedGrid;
        }

        private void OnActivityClicked(object sender, RoutedEventArgs e)
        {
            rootRepeater.ItemTemplate = groupedActivity;
        }

        private void OnFlexClicked(object sender, RoutedEventArgs e)
        {
            rootRepeater.ItemTemplate = groupedFlex;
        }
        private void TextBox_TextChanged(object sender, TextChangedEventArgs e)
        {
            rootRepeater.InvalidateMeasure();
        }

    }
}