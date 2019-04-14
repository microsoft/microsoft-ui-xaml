using System;
using System.Collections.Generic;
using System.Collections.ObjectModel;
using System.IO;
using System.Linq;
using System.Runtime.InteropServices.WindowsRuntime;
using Windows.Foundation;
using Windows.Foundation.Collections;
using Windows.UI.Xaml;
using Windows.UI.Xaml.Controls;
using Windows.UI.Xaml.Controls.Primitives;
using Windows.UI.Xaml.Data;
using Windows.UI.Xaml.Hosting;
using Windows.UI.Xaml.Input;
using Windows.UI.Xaml.Media;
using Windows.UI.Xaml.Media.Imaging;
using Windows.UI.Xaml.Navigation;

namespace Flick
{
    public sealed partial class GroupedPage : Page
    {
        public GroupedPage()
        {
            this.InitializeComponent();
        }

        protected override void OnNavigatedTo(NavigationEventArgs e)
        {
            base.OnNavigatedTo(e);
            var args = e.Parameter as NavigateArgs;

            ObservableCollection<PhotoReel> groupedPhotos = new ObservableCollection<PhotoReel>();
            for (int i = 0; i < 10; i++)
            {
                PhotoReel reel = new PhotoReel(args.Photos) { Name = "Group " + i };
                groupedPhotos.Add(reel);
            }

            rootRepeater.ItemsSource = groupedPhotos;

            //var anchor = repeater.GetOrCreateElement(selectedIndex);
            //(anchor as UserControl).Focus(FocusState.Keyboard);
            //UpdateLayout();
            //ScrollToCenterOfViewport(anchor);
        }

        private void OnElementPrepared(Microsoft.UI.Xaml.Controls.ItemsRepeater sender, Microsoft.UI.Xaml.Controls.ItemsRepeaterElementPreparedEventArgs args)
        {
            var item = ElementCompositionPreview.GetElementVisual(args.Element);
            var svVisual = ElementCompositionPreview.GetElementVisual(sv);
            var scrollProperties = ElementCompositionPreview.GetScrollViewerManipulationPropertySet(sv);

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
        }

        private void SetBanner(object sender)
        {
           // var selected = (sender as FrameworkElement).DataContext as Photo;
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