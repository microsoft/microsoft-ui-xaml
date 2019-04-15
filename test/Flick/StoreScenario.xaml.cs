using System.Collections.ObjectModel;
using Windows.UI.Xaml;
using Windows.UI.Xaml.Controls;
using Windows.UI.Xaml.Hosting;
using Windows.UI.Xaml.Input;
using Windows.UI.Xaml.Media;
using Windows.UI.Xaml.Navigation;

namespace Flick
{
    public sealed partial class StoreScenario : Page
    {
        public StoreScenario()
        {
            this.InitializeComponent();
        }

        protected override async void OnNavigatedTo(NavigationEventArgs e)
        {
            base.OnNavigatedTo(e);
            var args = e.Parameter as NavigateArgs;

            ObservableCollection<PhotoReel> groupedPhotos = new ObservableCollection<PhotoReel>();
            groupedPhotos.Add(await FlickApi.GetPhotos("tulips"));
            groupedPhotos.Add(await FlickApi.GetPhotos("daffodils"));
            groupedPhotos.Add(await FlickApi.GetPhotos("allium"));
            groupedPhotos.Add(await FlickApi.GetPhotos("roses"));
            groupedPhotos.Add(await FlickApi.GetPhotos("Lilies"));
            groupedPhotos.Add(await FlickApi.GetPhotos("Lavendar"));
            groupedPhotos.Add(await FlickApi.GetPhotos("geraniums"));

            rootRepeater.ItemsSource = groupedPhotos;
        }

        private void Grid_KeyDown(object sender, KeyRoutedEventArgs e)
        {
            if (e.Key == Windows.System.VirtualKey.Escape)
            {
                Frame.GoBack();
            }
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

        private void Repeater_ElementPrepared(Microsoft.UI.Xaml.Controls.ItemsRepeater sender, Microsoft.UI.Xaml.Controls.ItemsRepeaterElementPreparedEventArgs args)
        {
            /* Works bug disabled for perf.
            var item = ElementCompositionPreview.GetElementVisual(args.Element);

            var parentSv = VisualTreeHelper.GetParent(args.Element);
            while (!(parentSv is ScrollViewer))
            {
                parentSv = VisualTreeHelper.GetParent(parentSv);
            }

            var svVisual = ElementCompositionPreview.GetElementVisual(parentSv as ScrollViewer);
            var scrollProperties = ElementCompositionPreview.GetScrollViewerManipulationPropertySet(parentSv as ScrollViewer);

            var scaleExpresion = scrollProperties.Compositor.CreateExpressionAnimation();
            scaleExpresion.SetReferenceParameter("svVisual", svVisual);
            scaleExpresion.SetReferenceParameter("scrollProperties", scrollProperties);
            scaleExpresion.SetReferenceParameter("item", item);

            // scale the item based on the distance of the item relative to the center of the viewport.
            scaleExpresion.Expression = "1 - abs((svVisual.Size.X/2 - scrollProperties.Translation.X) - (item.Offset.X + item.Size.X/2))*(.5/(svVisual.Size.X/2))";
            item.StartAnimation("Scale.X", scaleExpresion);
            item.StartAnimation("Scale.Y", scaleExpresion);

            var centerPointExpression = scrollProperties.Compositor.CreateExpressionAnimation();
            centerPointExpression.SetReferenceParameter("item", item);
            centerPointExpression.Expression = "Vector3(item.Size.X/2, item.Size.Y/2, 0)";
            item.StartAnimation("CenterPoint", centerPointExpression);
            */
        }
    }
}