using Microsoft.UI.Xaml.Controls;
using System;
using System.Collections.Generic;
using Windows.UI.Xaml;
using Windows.UI.Xaml.Controls;
using Windows.UI.Xaml.Controls.Primitives;
using Windows.UI.Xaml.Hosting;
using Windows.UI.Xaml.Input;
using Windows.UI.Xaml.Media.Imaging;
using Windows.UI.Xaml.Navigation;

// The Blank Page item template is documented at https://go.microsoft.com/fwlink/?LinkId=234238

namespace Flick
{

    public class SnappPointForwardingRepeater : ItemsRepeater, IScrollSnapPointsInfo
    {
        public IReadOnlyList<float> GetIrregularSnapPoints(Orientation orientation, SnapPointsAlignment alignment)
        {
            return null;
        }

        public float GetRegularSnapPoints(Orientation orientation, SnapPointsAlignment alignment, out float offset)
        {
            if (alignment == SnapPointsAlignment.Center && orientation == Orientation.Horizontal)
            {
                var l = (Layout as VirtualizingUniformCarousalStackLayout);
                offset = (float)(l.ItemWidth / 2 + l.Spacing);
                return (float)(l.ItemWidth + l.Spacing);
            }

            offset = 0;
            return 0.0f;
        }

        public bool AreHorizontalSnapPointsRegular => true;

        public bool AreVerticalSnapPointsRegular => false;

        public event EventHandler<object> HorizontalSnapPointsChanged;
        public event EventHandler<object> VerticalSnapPointsChanged;
    }
    /// <summary>
    /// An empty page that can be used on its own or navigated to within a Frame.
    /// </summary>
    public sealed partial class AnimatedCarousalPage : Page
    {
        public AnimatedCarousalPage()
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
                args.Photos[i].Description = "Item " + i; ;
                subsetOf7Photos.Add(args.Photos[i]);
            }

            repeater.ItemsSource = subsetOf7Photos;
            int selectedIndex = args.Photos.IndexOf(args.Selected);

            repeater.Loaded += Repeater_Loaded;
        }

        private void Repeater_Loaded(object sender, RoutedEventArgs e)
        {
            sv.ChangeView((layout.ItemWidth + layout.Spacing) * 500, null, null, true);
           // sv.HorizontalSnapPointsType = SnapPointsType.Mandatory;
           // sv.HorizontalSnapPointsAlignment = SnapPointsAlignment.Center;
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
            scaleExpresion.Expression = "1 - abs((svVisual.Size.X/2 - scrollProperties.Translation.X) - (item.Offset.X + item.Size.X/2))*(.75/(svVisual.Size.X/2))";
            item.StartAnimation("Scale.X", scaleExpresion);
            item.StartAnimation("Scale.Y", scaleExpresion);

            var centerPointExpression = scrollProperties.Compositor.CreateExpressionAnimation();
            centerPointExpression.SetReferenceParameter("item", item);
            centerPointExpression.Expression = "Vector3(item.Size.X/2, item.Size.Y/2, 0)";
            item.StartAnimation("CenterPoint", centerPointExpression);
        }

        private void OnItemGotFocus(object sender, RoutedEventArgs e)
        {
            ScrollToCenterOfViewport(sender);
        }

        private void OnItemClicked(object sender, RoutedEventArgs e)
        {
            ScrollToCenterOfViewport(sender);
            //sv.ChangeView((layout.ItemWidth + layout.Spacing) * 500, null, null);
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