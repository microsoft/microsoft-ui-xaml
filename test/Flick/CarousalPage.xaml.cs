using System;
using System.Collections.Generic;
using Windows.UI.Xaml;
using Windows.UI.Xaml.Controls;
using Windows.UI.Xaml.Hosting;
using Windows.UI.Xaml.Input;
using Windows.UI.Xaml.Media.Imaging;
using Windows.UI.Xaml.Navigation;

// The Blank Page item template is documented at https://go.microsoft.com/fwlink/?LinkId=234238

namespace Flick
{
    /// <summary>
    /// An empty page that can be used on its own or navigated to within a Frame.
    /// </summary>
    public sealed partial class CarouselPage : Page
    {
        int m_selectedIndex = 0;
        public CarouselPage()
        {
            this.InitializeComponent();
        }

        protected override void OnNavigatedTo(NavigationEventArgs e)
        {
            base.OnNavigatedTo(e);
            var args = e.Parameter as NavigateArgs;

            List<Photo> subsetOf7Photos = new List<Photo>();
            for (int i = 0; i < 10; i++)
            {
                subsetOf7Photos.Add(args.Photos[i]);
            }

            repeater.ItemsSource = subsetOf7Photos;
            int selectedIndex = args.Photos.IndexOf(args.Selected);
            banner.Source = new BitmapImage(new Uri(args.Selected.LargeUrl));
            m_selectedIndex = 0; // Can set initial item here. The one in args might not exist because we filted the list.
            repeater.Loaded += Repeater_Loaded;
        }

        private void Repeater_Loaded(object sender, RoutedEventArgs e)
        {
            sv.ChangeView((layout.ItemWidth + layout.Spacing) * 500 + m_selectedIndex*layout.ItemWidth, null, null, true);
        }

        private void SetBanner(object sender)
        {
            var selected = (sender as FrameworkElement).DataContext as Photo;
            banner.Source = new BitmapImage(new Uri(selected.LargeUrl));
            bannerEnter.Begin();
        }

        private void OnItemGotFocus(object sender, RoutedEventArgs e)
        {
            SetBanner(sender);
            ScrollToCenterOfViewport(sender);
        }

        private void OnItemClicked(object sender, RoutedEventArgs e)
        {
            SetBanner(sender);
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
    }
}