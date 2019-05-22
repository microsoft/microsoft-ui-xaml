
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

        private bool isScrolling = false;

        public bool IsScrolling
        {
            get
            {
                return isScrolling;
            }
        }

        private double itemScaleRatio = 0.5;

        public double ItemScaleRatio
        {
            get
            {
                return itemScaleRatio;
            }
            set
            {
                itemScaleRatio = value;
            }
        }

        protected void OnScrollViewerViewChanged(object sender, ScrollViewerViewChangedEventArgs e)
        {
            var svCenterPoint = sv.HorizontalOffset + sv.ViewportWidth / 2;
            int selectedItemIndex = (int)Math.Floor((svCenterPoint + layout.Spacing / 2) / (layout.Spacing + layout.ItemWidth));
            selectedItemIndex %= ((System.Collections.Generic.IReadOnlyList<object>)repeater.ItemsSource).Count;
            var selectedUIElement = repeater.TryGetElement(selectedItemIndex);
            SelectedItem = (selectedUIElement == null ? null : ((UserControl)selectedUIElement).DataContext);

            if (e.IsIntermediate)
            {
                if (!isScrolling)
                {
                    isScrolling = true;
                }
            }
            else
            {
                textBlock.Text = "Selected Item: " + (SelectedItem == null ? "null" : selectedItemIndex.ToString());
                isScrolling = false;
            }
        }

        protected void OnScrollViewerViewChanging(object sender, ScrollViewerViewChangingEventArgs e)
        {
            if (!isScrolling)
            {
                isScrolling = true;
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
           // sv.HorizontalSnapPointsType = SnapPointsType.Mandatory;
           // sv.HorizontalSnapPointsAlignment = SnapPointsAlignment.Center;
        }

        private void OnElementPrepared(Microsoft.UI.Xaml.Controls.ItemsRepeater sender, Microsoft.UI.Xaml.Controls.ItemsRepeaterElementPreparedEventArgs args)
        {
            var item = ElementCompositionPreview.GetElementVisual(args.Element);
            //var item = ElementCompositionPreview.GetElementVisual(((Border)args.Element).Child);

            var svVisual = ElementCompositionPreview.GetElementVisual(sv);
            var scrollProperties = ElementCompositionPreview.GetScrollViewerManipulationPropertySet(sv);
            var animationGroup = scrollProperties.Compositor.CreateAnimationGroup();

            var centerPointExpression = scrollProperties.Compositor.CreateExpressionAnimation();
            centerPointExpression.SetReferenceParameter("item", item);
            centerPointExpression.SetReferenceParameter("svVisual", svVisual);
            centerPointExpression.SetReferenceParameter("scrollProperties", scrollProperties);
            centerPointExpression.SetScalarParameter("spacing", (float)layout.Spacing);
            //centerPointExpression.Expression = "Vector3(((item.Size.X/2) + ((((item.Offset.X + (item.Size.X/2)) < ((svVisual.Size.X/2) - scrollProperties.Translation.X)) ? 1 : -1) * ((item.Size.X/2) * clamp((abs((item.Offset.X + (item.Size.X/2)) - ((svVisual.Size.X/2) - scrollProperties.Translation.X)) / (item.Size.X + spacing)), 0, 1)))), item.Size.Y/2, 0)";
            //item.StartAnimation("CenterPoint", centerPointExpression);
            //centerPointExpression.Expression = "Vector3(item.Size.X/2, item.Size.Y/2, 0)";
            centerPointExpression.Expression = "Vector3(((item.Size.X/2) + ((((item.Offset.X + (item.Size.X/2)) < ((svVisual.Size.X/2) - scrollProperties.Translation.X)) ? 1 : -1) * (((item.Size.X/2) * clamp((abs((item.Offset.X + (item.Size.X/2)) - ((svVisual.Size.X/2) - scrollProperties.Translation.X)) / (item.Size.X + spacing)), 0, 1)) + ((item.Size.X) * max((abs((item.Offset.X + (item.Size.X/2)) - ((svVisual.Size.X/2) - scrollProperties.Translation.X)) / (item.Size.X + spacing)) - 1, 0))) )), item.Size.Y/2, 0)";
            centerPointExpression.Target = "CenterPoint";
            animationGroup.Add(centerPointExpression);

            var scaleXExpression = scrollProperties.Compositor.CreateExpressionAnimation();
            scaleXExpression.SetReferenceParameter("svVisual", svVisual);
            scaleXExpression.SetReferenceParameter("scrollProperties", scrollProperties);
            scaleXExpression.SetReferenceParameter("item", item);

            /* TODO: Expose ItemScaleRatio (scaleRatioXY) as a DependencyProperty in the custom Carousel
             * control so the user can set it to any value */
            scaleXExpression.SetScalarParameter("scaleRatioXY", (float)ItemScaleRatio);
            scaleXExpression.SetScalarParameter("spacing", (float)layout.Spacing);

            // scale the item based on the distance of the item relative to the center of the viewport.
            //scaleExpression.Expression = "1 - abs((svVisual.Size.X/2 - scrollProperties.Translation.X) - (item.Offset.X + item.Size.X/2))*(.75/(svVisual.Size.X/2))";
            var scaleExpressionString = "clamp((scaleRatioXY * (1 + (1 - (abs((item.Offset.X + (item.Size.X/2)) - ((svVisual.Size.X/2) - scrollProperties.Translation.X)) / (item.Size.X + spacing))))), scaleRatioXY, 1)";
            scaleXExpression.Expression = scaleExpressionString;

            // item.StartAnimation("Scale.X", scaleXExpression);
            scaleXExpression.Target = "Scale.X";
            animationGroup.Add(scaleXExpression);

            var scaleYExpression = scrollProperties.Compositor.CreateExpressionAnimation();
            scaleYExpression.SetReferenceParameter("svVisual", svVisual);
            scaleYExpression.SetReferenceParameter("scrollProperties", scrollProperties);
            scaleYExpression.SetReferenceParameter("item", item);

            /* TODO: Expose ItemScaleRatio (scaleRatioXY) as a DependencyProperty in the custom Carousel
             * control so the user can set it to any value */
            scaleYExpression.SetScalarParameter("scaleRatioXY", (float)ItemScaleRatio);
            scaleYExpression.SetScalarParameter("spacing", (float)layout.Spacing);

            // scale the item based on the distance of the item relative to the center of the viewport.
            scaleYExpression.Expression = scaleExpressionString;

            // item.StartAnimation("Scale.Y", scaleXExpression);
            scaleYExpression.Target = "Scale.Y";
            animationGroup.Add(scaleYExpression);

            /* TODO: Create an ExpressionAnimation to be applied to each item's Offset property that will 
             * allow each item to maintain the fixed Layout.Spacing even when a Scale animation has been applied */
            var offsetExpression = scrollProperties.Compositor.CreateExpressionAnimation();
            offsetExpression.SetReferenceParameter("svVisual", svVisual);
            offsetExpression.SetReferenceParameter("scrollProperties", scrollProperties);
            offsetExpression.SetReferenceParameter("item", item);
            offsetExpression.SetScalarParameter("itemScaleRatio", (float)ItemScaleRatio);
            offsetExpression.SetScalarParameter("spacing", (float)layout.Spacing);
            //offsetExpression.Expression = "Vector3(0,200,0)";
            //offsetExpression.Expression = "Vector3(((((item.Offset.X + (item.Size.X/2)) < ((svVisual.Size.X/2) - scrollProperties.Translation.X)) ? 1 : -1) * (item.Size.X * (1 - clamp((scaleRatioXY * (1 + (1 - (abs((item.Offset.X + (item.Size.X/2)) - ((svVisual.Size.X/2) - scrollProperties.Translation.X)) / (item.Size.X + spacing))))), scaleRatioXY, 1)) / 2)), 0, 0)";
            //offsetExpression.Expression = "Vector3(((abs((item.Offset.X + (item.Size.X/2)) - ((svVisual.Size.X/2) - scrollProperties.Translation.X)) > (item.Size.X + spacing)) ? ((((item.Offset.X + (item.Size.X/2)) < ((svVisual.Size.X/2) - scrollProperties.Translation.X)) ? 1 : -1) * min((((item.Size.X * (1 - itemScaleRatio)) * ((abs((item.Offset.X + (item.Size.X/2)) - ((svVisual.Size.X/2) - scrollProperties.Translation.X))) / (item.Size.X + spacing))) - (item.Size.X + spacing)), 0)) : 0), 0, 0)";
            //item.StartAnimation("Offset", offsetExpression);
            offsetExpression.Target = "Translation";
            //offsetExpression.Target = "Offset.Y";
            //args.Element.StartAnimation(offsetExpression);
            //animationGroup.Add(offsetExpression);

            item.StartAnimationGroup(animationGroup);
        }

        private void OnItemGotFocus(object sender, RoutedEventArgs e)
        {
            //ScrollToCenterOfViewport(sender);
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

        private void Sv_Tapped(object sender, TappedRoutedEventArgs e)
        {
            if (!isScrolling)
            {
                // Get x position of the scrollviewer's center point if the currently selected item.Offset.X + item.Size.X/2 was the center point
                var svCenterPoint = sv.HorizontalOffset + sv.ViewportWidth / 2;
                svCenterPoint -= (svCenterPoint + layout.Spacing / 2) % (layout.Spacing + layout.ItemWidth);
                svCenterPoint += layout.Spacing / 2 + layout.ItemWidth / 2;
                var tapPositionInSV = e.GetPosition(sv).X + sv.HorizontalOffset;
                var tapPositionDistanceFromSVCenterPoint = Math.Abs(tapPositionInSV - svCenterPoint);
                int tappedItemIndex;

                if (tapPositionDistanceFromSVCenterPoint <= (layout.ItemWidth / 2 + layout.Spacing / 2))
                {
                    var svOffsetForGivenCenterPoint = svCenterPoint - sv.ViewportWidth / 2;

                    if (svOffsetForGivenCenterPoint != sv.HorizontalOffset)
                    {
                        sv.ChangeView(svOffsetForGivenCenterPoint , null, null, false);
                    }
                }
                else
                {
                    tapPositionDistanceFromSVCenterPoint -= layout.ItemWidth / 2 + layout.Spacing / 2;
                    var tappedItemIndexDifferenceFromCenter = (int)Math.Floor(tapPositionDistanceFromSVCenterPoint / (layout.ItemWidth * ItemScaleRatio + layout.Spacing)) + 1;
                    //var distanceToScroll = -1 * ((sv.ViewportWidth / 2) - (tappedItemIndexDifferenceFromCenter * (layout.ItemWidth + layout.Spacing)));
                    //sv.ChangeView(distanceToScroll, null, null, false);
                    var centerItemIndex = (int)Math.Floor((svCenterPoint + layout.Spacing / 2) / (layout.Spacing + layout.ItemWidth));
                    var offsetToScroll = sv.HorizontalOffset + (((tapPositionInSV < svCenterPoint) ? -1 : 1) * (tappedItemIndexDifferenceFromCenter * (layout.ItemWidth + layout.Spacing)));
                    sv.ChangeView(offsetToScroll, null, null, false);
                    //tappedItemIndex = (tapPositionInSV > svCenterPoint ? (centerItemIndex + tappedItemIndexDifferenceFromCenter) : (centerItemIndex - tappedItemIndexDifferenceFromCenter));
                    //tappedItemIndex %= ((System.Collections.Generic.IReadOnlyList<object>)repeater.ItemsSource).Count;
                }

                //var tappedUIElement = repeater.TryGetElement(tappedItemIndex);

                //if (tappedUIElement != null)
                //{
                //    ScrollToCenterOfViewport(tappedUIElement);
                //}
            }
        }
    }
}