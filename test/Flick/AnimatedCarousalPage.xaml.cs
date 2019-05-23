
using Microsoft.UI.Xaml.Controls;
using System;
using System.Numerics;
using System.Collections.Generic;
using Windows.UI.Xaml;
using Windows.UI.Xaml.Controls;
using Windows.UI.Xaml.Controls.Primitives;
using Windows.UI.Xaml.Hosting;
using Windows.UI.Xaml.Input;
using Windows.UI.Xaml.Media.Imaging;
using Windows.UI.Xaml.Navigation;
using System.Diagnostics;

// The Blank Page item template is documented at https://go.microsoft.com/fwlink/?LinkId=234238

namespace Flick
{

    public class SnappPointForwardingRepeater : ItemsRepeater, IScrollSnapPointsInfo
    {
        public SnappPointForwardingRepeater()
        {
        }

        public IReadOnlyList<float> GetIrregularSnapPoints(Orientation orientation, SnapPointsAlignment alignment)
        {
            return null;
        }

        public float GetRegularSnapPoints(Orientation orientation, SnapPointsAlignment alignment, out float offset)
        {
            if (alignment == SnapPointsAlignment.Center && orientation == Orientation.Horizontal)
            {
                var l = (Layout as VirtualizingUniformCarousalStackLayout);
                offset = (float)(Margin.Left + (l.ItemWidth / 2));
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

            // Workaround for known numerical limitation on inset clips where scrollviewer fails to clip content on right side of viewport
            ElementCompositionPreview.GetElementVisual(sv).Clip = ElementCompositionPreview.GetScrollViewerManipulationPropertySet(sv).Compositor.CreateInsetClip();
        }

        private object selectedItem = null;

        public object SelectedItem
        {
            get
            {
                return selectedItem;
            }
            set
            {
                selectedItem = value;
            }
        }

        private bool IsScrolling { get; set; }

        public double ItemScaleRatio { get; set; } = 0.5;

        protected void OnScrollViewerViewChanged(object sender, ScrollViewerViewChangedEventArgs e)
        {
            var svCenterPoint = sv.HorizontalOffset + sv.ViewportWidth / 2;
            int selectedItemIndex = (int)Math.Floor((svCenterPoint + layout.Spacing / 2) / (layout.Spacing + layout.ItemWidth));
            selectedItemIndex %= ((System.Collections.Generic.IReadOnlyList<object>)repeater.ItemsSource).Count;
            var selectedUIElement = repeater.TryGetElement(selectedItemIndex);
            SelectedItem = (selectedUIElement == null ? null : ((UserControl)selectedUIElement).DataContext);

            if (e.IsIntermediate)
            {
                if (!IsScrolling)
                {
                    IsScrolling = true;
                }
            }
            else
            {
                textBlock.Text = "Selected Item: " + (SelectedItem == null ? "null" : selectedItemIndex.ToString());
                IsScrolling = false;
            }
        }

        protected void OnScrollViewerViewChanging(object sender, ScrollViewerViewChangingEventArgs e)
        {
            if (!IsScrolling)
            {
                IsScrolling = true;
            }
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
            sv.ChangeView(((layout.ItemWidth + layout.Spacing) * 500), null, null, true);
        }

        private void OnElementPrepared(Microsoft.UI.Xaml.Controls.ItemsRepeater sender, Microsoft.UI.Xaml.Controls.ItemsRepeaterElementPreparedEventArgs args)
        {
            var item = ElementCompositionPreview.GetElementVisual(args.Element);
            var svVisual = ElementCompositionPreview.GetElementVisual(sv);
            var scrollProperties = ElementCompositionPreview.GetScrollViewerManipulationPropertySet(sv);
            var animationGroup = scrollProperties.Compositor.CreateAnimationGroup();

            // Animate each item's centerpoint based on the item's distance from the center of the viewport
            // translate the position of each item horizontally closer to the center of the viewport as much as is necessary
            // in order to ensure that the Spacing property of the ItemsRepeater is still respected after the items have been scaled.
            var centerPointExpressionString = "Vector3(((item.Size.X/2) + ((((item.Offset.X + (item.Size.X/2)) < ((svVisual.Size.X/2) - scrollProperties.Translation.X)) ? 1 : -1) * (((item.Size.X/2) * clamp((abs((item.Offset.X + (item.Size.X/2)) - ((svVisual.Size.X/2) - scrollProperties.Translation.X)) / (item.Size.X + spacing)), 0, 1)) + ((item.Size.X) * max((abs((item.Offset.X + (item.Size.X/2)) - ((svVisual.Size.X/2) - scrollProperties.Translation.X)) / (item.Size.X + spacing)) - 1, 0))) )), item.Size.Y/2, 0)";
            var centerPointExpression = scrollProperties.Compositor.CreateExpressionAnimation();
            centerPointExpression.SetReferenceParameter("item", item);
            centerPointExpression.SetReferenceParameter("svVisual", svVisual);
            centerPointExpression.SetReferenceParameter("scrollProperties", scrollProperties);
            centerPointExpression.SetScalarParameter("spacing", (float)layout.Spacing);
            centerPointExpression.Expression = centerPointExpressionString;
            centerPointExpression.Target = "CenterPoint";
            animationGroup.Add(centerPointExpression);

            // scale the item based on the distance of the item relative to the center of the viewport.
            var scalarScaleExpressionString = "clamp((scaleRatioXY * (1 + (1 - (abs((item.Offset.X + (item.Size.X/2)) - ((svVisual.Size.X/2) - scrollProperties.Translation.X)) / (item.Size.X + spacing))))), scaleRatioXY, 1)";
            var scaleExpressionString = string.Format("Vector3({0}, {0}, 0)", scalarScaleExpressionString);

            var scaleExpression = scrollProperties.Compositor.CreateExpressionAnimation();
            scaleExpression.SetReferenceParameter("svVisual", svVisual);
            scaleExpression.SetReferenceParameter("scrollProperties", scrollProperties);
            scaleExpression.SetReferenceParameter("item", item);
            /* TODO: Expose ItemScaleRatio (scaleRatioXY) as a DependencyProperty in the custom Carousel
             * control so the user can set it to any value */
            scaleExpression.SetScalarParameter("scaleRatioXY", (float)ItemScaleRatio);
            scaleExpression.SetScalarParameter("spacing", (float)layout.Spacing);
            scaleExpression.Expression = scaleExpressionString;
            scaleExpression.Target = "Scale";
            animationGroup.Add(scaleExpression);

            item.StartAnimationGroup(animationGroup);
        }

        // ScrollToCenterOfViewport is currently not being used due to it
        // not being able to correctly animate an item to center if said item
        // has been animated to a different location from its original layout location
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

        private void Sv_Tapped(object sender, TappedRoutedEventArgs e)
        {
            if (!IsScrolling)
            {
                // In the nominal case, centerOfViewportOffsetInScrollViewer will be the offset of the current centerpoint in the scrollviewer's viewport;
                // however, if the "center" item is not perfectly centered (i.e. where the centerpoint falls on the item's size.x/2)
                // then set centerOfViewportOffsetInScrollViewer equal to the offset where the "centered" item would be perfectly centered.
                // This makes later calculations much simpler with respect to item animations.
                var centerOfViewportOffsetInScrollViewer = sv.HorizontalOffset + sv.ViewportWidth / 2;
                centerOfViewportOffsetInScrollViewer -= (centerOfViewportOffsetInScrollViewer + layout.Spacing / 2) % (layout.Spacing + layout.ItemWidth);
                centerOfViewportOffsetInScrollViewer += layout.Spacing / 2 + layout.ItemWidth / 2;
                var tapPositionOffsetInScrollViewer = e.GetPosition(sv).X + sv.HorizontalOffset;
                var tapPositionDistanceFromSVCenterPoint = Math.Abs(tapPositionOffsetInScrollViewer - centerOfViewportOffsetInScrollViewer);
                double offsetToScrollTo;

                if (tapPositionDistanceFromSVCenterPoint <= (layout.ItemWidth / 2 + layout.Spacing / 2))
                {
                    offsetToScrollTo = centerOfViewportOffsetInScrollViewer - sv.ViewportWidth / 2;
                }
                else
                {
                    tapPositionDistanceFromSVCenterPoint -= layout.ItemWidth / 2 + layout.Spacing / 2;
                    var tappedItemIndexDifferenceFromCenter = (int)Math.Floor(tapPositionDistanceFromSVCenterPoint / (layout.ItemWidth * ItemScaleRatio + layout.Spacing)) + 1;
                    offsetToScrollTo = sv.HorizontalOffset + (((tapPositionOffsetInScrollViewer < centerOfViewportOffsetInScrollViewer) ? -1 : 1) * (tappedItemIndexDifferenceFromCenter * (layout.ItemWidth + layout.Spacing)));
                }

                if (offsetToScrollTo != sv.HorizontalOffset)
                {
                    // This odd delay is required in order to ensure that the scrollviewer animates the scroll
                    // on every call to ChangeView.
                    var period = TimeSpan.FromMilliseconds(10);
                    Windows.System.Threading.ThreadPoolTimer.CreateTimer(async (source) =>
                    {
                        await Dispatcher.RunAsync(Windows.UI.Core.CoreDispatcherPriority.Normal, () =>
                        {
                            var Succes = sv.ChangeView(offsetToScrollTo, null, null, false);
                        });
                    }, period);
                }
            }
        }
    }
}