
using Microsoft.UI.Xaml.Controls;
using System;
using System.Collections.Generic;
using Windows.UI.Xaml;
using Windows.UI.Xaml.Controls;
using Windows.UI.Xaml.Controls.Primitives;
using Windows.UI.Xaml.Hosting;
using Windows.UI.Xaml.Input;
using Windows.UI.Xaml.Navigation;
using Windows.System.Threading;

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
                var l = (Layout as VirtualizingUniformCarouselStackLayout);
                offset = (float)(Margin.Left + l.ItemWidth / 2);
                return (float)(l.ItemWidth + l.Spacing);
            }

            offset = 0;
            return 0.0f;
        }

        public bool AreHorizontalSnapPointsRegular => true;

        public bool AreVerticalSnapPointsRegular => false;

        // Number of times to repeat the count to give the 
        // illusion of infinite scrolling.
        public int RepeatCount
        {
            get { return (int)GetValue(RepeatCountProperty); }
            set
            {
                if (value < 0)
                {
                    throw new ArgumentException(String.Format("{0} must be a non-negative integer", "RepeatCount"));
                }

                SetValue(RepeatCountProperty, value);
            }
        }

        public static readonly DependencyProperty RepeatCountProperty = DependencyProperty.Register(
            "RepeatCount", typeof(int), typeof(VirtualizingUniformCarouselStackLayout), new PropertyMetadata(500));

        public event EventHandler<object> HorizontalSnapPointsChanged;
        public event EventHandler<object> VerticalSnapPointsChanged;

    }

    /// <summary>
    /// An empty page that can be used on its own or navigated to within a Frame.
    /// </summary>
    public sealed partial class AnimatedCarouselPage : Page
    {
        public enum ScrollDirection
        {
            Previous,
            Next
        }

        public AnimatedCarouselPage()
        {
            this.InitializeComponent();

            // Workaround for known numerical limitation on inset clips where scrollviewer fails to clip content on right side of viewport
            ElementCompositionPreview.GetElementVisual(sv).Clip = ElementCompositionPreview.GetScrollViewerManipulationPropertySet(sv).Compositor.CreateInsetClip();

            carouselPrevButton.AddHandler(UIElement.PointerPressedEvent, new PointerEventHandler(OnCarouselPrevButtonPointerPressed), true);
            carouselPrevButton.AddHandler(UIElement.PointerCanceledEvent, new PointerEventHandler(OnCarouselPrevButtonPointerCanceled), true);
            carouselPrevButton.AddHandler(UIElement.PointerReleasedEvent, new PointerEventHandler(OnCarouselPrevButtonPointerReleased), true);
            carouselPrevButton.AddHandler(UIElement.PointerExitedEvent, new PointerEventHandler(OnCarouselPrevButtonPointerExited), true);
            carouselPrevButton.AddHandler(UIElement.PointerCaptureLostEvent, new PointerEventHandler(OnCarouselPrevButtonPointerCaptureLost), true);

            carouselNextButton.AddHandler(UIElement.PointerPressedEvent, new PointerEventHandler(OnCarouselNextButtonPointerPressed), true);
            carouselNextButton.AddHandler(UIElement.PointerCanceledEvent, new PointerEventHandler(OnCarouselNextButtonPointerCanceled), true);
            carouselNextButton.AddHandler(UIElement.PointerReleasedEvent, new PointerEventHandler(OnCarouselNextButtonPointerReleased), true);
            carouselNextButton.AddHandler(UIElement.PointerExitedEvent, new PointerEventHandler(OnCarouselNextButtonPointerExited), true);
            carouselNextButton.AddHandler(UIElement.PointerCaptureLostEvent, new PointerEventHandler(OnCarouselNextButtonPointerCaptureLost), true);
        }

        public object SelectedItem { get; set; } = null;

        private bool IsScrolling { get; set; }

        public double ItemScaleRatio { get; set; } = 0.5;

        private ThreadPoolTimer PrevButtonContinuousScrollingPeriodicTimer { get; set; } = null;

        private ThreadPoolTimer NextButtonContinuousScrollingPeriodicTimer { get; set; } = null;

        private static TimeSpan PrevNextButtonHoldPeriod { get; } = TimeSpan.FromMilliseconds(300);

        private ThreadPoolTimer PrevButtonHoldTimer { get; set; } = null;

        private ThreadPoolTimer NextButtonHoldTimer { get; set; } = null;

        private ThreadPoolTimer ScrollViewerChangeViewTimer { get; set; } = null;

        private static int ContinousScrollingItemSkipCount { get; } = 2;

        private static TimeSpan PrevNextButtonContinousScrollingSelectionPeriod { get; } = TimeSpan.FromMilliseconds(100);

        public Visibility CarouselPrevButtonVisibility { get; set; } = Visibility.Collapsed;

        public Visibility CarouselNextButtonVisibility { get; set; } = Visibility.Collapsed;

        private void HideCarouselNextPrevButtons()
        {
            CarouselPrevButtonVisibility = CarouselNextButtonVisibility = Visibility.Collapsed;
        }

        private void ShowCarouselNextPrevButtons()
        {
            CarouselPrevButtonVisibility = CarouselNextButtonVisibility = Visibility.Visible;
        }

        protected void OnScrollViewerViewChanged(object sender, ScrollViewerViewChangedEventArgs e)
        {
            if (e.IsIntermediate)
            {
                if (!IsScrolling)
                {
                    IsScrolling = true;
                }
            }
            else
            {
                SelectedItem = GetSelectedItemFromViewport();

                textBlock.Text = "Selected Item: " + GetSelectedIndexFromViewport() ?? "null";
                IsScrolling = false;
            }
        }

        // TODO: Is this really required when we have ViewChanged with IsIntermediate = false handled ?
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

            List<Photo> subsetOfPhotos = new List<Photo>();
            for (int i = 0; i < 10; i++)
            {
                args.Photos[i].Description = "Item " + i; ;
                subsetOfPhotos.Add(args.Photos[i]);
            }

            repeater.ItemsSource = subsetOfPhotos;
            int selectedIndex = args.Photos.IndexOf(args.Selected);

            repeater.Loaded += Repeater_Loaded;
        }

        private void Repeater_Loaded(object sender, RoutedEventArgs e)
        {
            if (repeater.ItemsSourceView.Count == 0)
            {
                HideCarouselNextPrevButtons();
            }
            else if (repeater.ItemsSourceView.Count == 1)
            {
                sv.ChangeView(0, null, null, true);
                SelectedItem = GetSelectedItemFromViewport();
                HideCarouselNextPrevButtons();
            }
            else
            {
                sv.ChangeView(((layout.ItemWidth + layout.Spacing) * layout.RepeatCount), null, null, true);
                ShowCarouselNextPrevButtons();
            }
        }

        private void OnElementPrepared(Microsoft.UI.Xaml.Controls.ItemsRepeater sender, Microsoft.UI.Xaml.Controls.ItemsRepeaterElementPreparedEventArgs args)
        {
            var item = ElementCompositionPreview.GetElementVisual(args.Element);
            var svVisual = ElementCompositionPreview.GetElementVisual(sv);
            var scrollProperties = ElementCompositionPreview.GetScrollViewerManipulationPropertySet(sv);

            // Animate each item's centerpoint based on the item's distance from the center of the viewport
            // translate the position of each item horizontally closer to the center of the viewport as much as is necessary
            // in order to ensure that the Spacing property of the ItemsRepeater is still respected after the items have been scaled.
            var centerPointExpression = scrollProperties.Compositor.CreateExpressionAnimation();
            centerPointExpression.SetReferenceParameter("item", item);
            centerPointExpression.SetReferenceParameter("svVisual", svVisual);
            centerPointExpression.SetReferenceParameter("scrollProperties", scrollProperties);
            centerPointExpression.SetScalarParameter("spacing", (float)layout.Spacing);
            var centerPointExpressionString = "Vector3(((item.Size.X/2) + ((((item.Offset.X + (item.Size.X/2)) < ((svVisual.Size.X/2) - scrollProperties.Translation.X)) ? 1 : -1) * (((item.Size.X/2) * clamp((abs((item.Offset.X + (item.Size.X/2)) - ((svVisual.Size.X/2) - scrollProperties.Translation.X)) / (item.Size.X + spacing)), 0, 1)) + ((item.Size.X) * max((abs((item.Offset.X + (item.Size.X/2)) - ((svVisual.Size.X/2) - scrollProperties.Translation.X)) / (item.Size.X + spacing)) - 1, 0))) )), item.Size.Y/2, 0)";
            centerPointExpression.Expression = centerPointExpressionString;
            centerPointExpression.Target = "CenterPoint";

            // scale the item based on the distance of the item relative to the center of the viewport.            
            var scaleExpression = scrollProperties.Compositor.CreateExpressionAnimation();
            scaleExpression.SetReferenceParameter("svVisual", svVisual);
            scaleExpression.SetReferenceParameter("scrollProperties", scrollProperties);
            scaleExpression.SetReferenceParameter("item", item);
            /* TODO: Expose ItemScaleRatio (scaleRatioXY) as a DependencyProperty in the custom Carousel
             * control so the user can set it to any value */
            scaleExpression.SetScalarParameter("scaleRatioXY", (float)ItemScaleRatio);
            scaleExpression.SetScalarParameter("spacing", (float)layout.Spacing);
            var scalarScaleExpressionString = "clamp((scaleRatioXY * (1 + (1 - (abs((item.Offset.X + (item.Size.X/2)) - ((svVisual.Size.X/2) - scrollProperties.Translation.X)) / (item.Size.X + spacing))))), scaleRatioXY, 1)";
            var scaleExpressionString = string.Format("Vector3({0}, {0}, 0)", scalarScaleExpressionString);
            scaleExpression.Expression = scaleExpressionString;
            scaleExpression.Target = "Scale";

            var animationGroup = scrollProperties.Compositor.CreateAnimationGroup();
            animationGroup.Add(centerPointExpression);
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

        private void OnScrollViewerTapped(object sender, TappedRoutedEventArgs e)
        {
            if (ScrollViewerChangeViewTimer != null)
            {
                ScrollViewerChangeViewTimer.Cancel();
                ScrollViewerChangeViewTimer = null;
            }

            if (PrevButtonHoldTimer != null)
            {
                PrevButtonHoldTimer.Cancel();
                PrevButtonHoldTimer = null;
            }

            if (NextButtonHoldTimer != null)
            {
                NextButtonHoldTimer.Cancel();
                NextButtonHoldTimer = null;
            }

            if (PrevButtonContinuousScrollingPeriodicTimer != null)
            {
                PrevButtonContinuousScrollingPeriodicTimer.Cancel();
                PrevButtonContinuousScrollingPeriodicTimer = null;
            }

            if (NextButtonContinuousScrollingPeriodicTimer != null)
            {
                NextButtonContinuousScrollingPeriodicTimer.Cancel();
                NextButtonContinuousScrollingPeriodicTimer = null;
            }

            if (!IsScrolling)
            {
                var centerOfViewportOffsetInScrollViewer = CenterPointOfViewportInExtent();
                // In the nominal case, centerOfViewportOffsetInScrollViewer will be the offset of the current centerpoint in the scrollviewer's viewport;
                // however, if the "center" item is not perfectly centered (i.e. where the centerpoint falls on the item's size.x/2)
                // then set centerOfViewportOffsetInScrollViewer equal to the offset where the "centered" item would be perfectly centered.
                // This makes later calculations much simpler with respect to item animations.
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
                    // TODO: Why is this required ?? Should be removed.
                    var period = TimeSpan.FromMilliseconds(10);
                    ScrollViewerChangeViewTimer = Windows.System.Threading.ThreadPoolTimer.CreateTimer(async (source) =>
                    {
                        await Dispatcher.RunAsync(Windows.UI.Core.CoreDispatcherPriority.Normal, () =>
                        {
                            var Success = sv.ChangeView(offsetToScrollTo, null, null, false);
                        });
                    }, period);
                }
            }
        }

        private void SelectNextItem()
        {
            SelectNextItem(0 /*numberOfItemsToSkip*/);
        }
        
        private void SelectNextItem(int numberOfItemsToSkip)
        {
            // In the nominal case, centerOfViewportOffsetInScrollViewer will be the offset of the current centerpoint in the scrollviewer's viewport;
            // however, if the "center" item is not perfectly centered (i.e. where the centerpoint falls on the item's size.x/2)
            // then set centerOfViewportOffsetInScrollViewer equal to the offset where the "centered" item would be perfectly centered.
            // This makes later calculations much simpler with respect to item animations.
            var centerOfViewportOffsetInScrollViewer = sv.HorizontalOffset + sv.ViewportWidth / 2;
            centerOfViewportOffsetInScrollViewer -= (centerOfViewportOffsetInScrollViewer + layout.Spacing / 2) % (layout.Spacing + layout.ItemWidth);
            centerOfViewportOffsetInScrollViewer += layout.Spacing / 2 + layout.ItemWidth / 2;
            var newSelectedItemDistanceFromCenterPoint = layout.ItemWidth / 2 + layout.Spacing + (layout.ItemWidth * ItemScaleRatio);
            var offsetToScrollTo = sv.HorizontalOffset + newSelectedItemDistanceFromCenterPoint + (numberOfItemsToSkip * (layout.ItemWidth + layout.Spacing));
            // This odd delay is required in order to ensure that the scrollviewer animates the scroll
            // on every call to ChangeView.
            var period = TimeSpan.FromMilliseconds(10);
            ScrollViewerChangeViewTimer = Windows.System.Threading.ThreadPoolTimer.CreateTimer(async (source) =>
            {
                await Dispatcher.RunAsync(Windows.UI.Core.CoreDispatcherPriority.Normal, () =>
                {
                    var Success = sv.ChangeView(offsetToScrollTo, null, null, false);
                });
            }, period);
        }

        private void SelectPreviousItem()
        {
            SelectPreviousItem(0 /*numberOfItemsToSkip*/);
        }

        private void SelectPreviousItem(int numberOfItemsToSkip)
        {
            // In the nominal case, centerOfViewportOffsetInScrollViewer will be the offset of the current centerpoint in the scrollviewer's viewport;
            // however, if the "center" item is not perfectly centered (i.e. where the centerpoint falls on the item's size.x/2)
            // then set centerOfViewportOffsetInScrollViewer equal to the offset where the "centered" item would be perfectly centered.
            // This makes later calculations much simpler with respect to item animations.
            var centerOfViewportOffsetInScrollViewer = sv.HorizontalOffset + sv.ViewportWidth / 2;
            centerOfViewportOffsetInScrollViewer -= (centerOfViewportOffsetInScrollViewer + layout.Spacing / 2) % (layout.Spacing + layout.ItemWidth);
            centerOfViewportOffsetInScrollViewer += layout.Spacing / 2 + layout.ItemWidth / 2;
            var newSelectedItemDistanceFromCenterPoint = layout.ItemWidth / 2 + layout.Spacing + (layout.ItemWidth * ItemScaleRatio);
            var offsetToScrollTo = sv.HorizontalOffset - newSelectedItemDistanceFromCenterPoint - (numberOfItemsToSkip * (layout.ItemWidth + layout.Spacing));
            // This odd delay is required in order to ensure that the scrollviewer animates the scroll
            // on every call to ChangeView.
            var period = TimeSpan.FromMilliseconds(10);
            ScrollViewerChangeViewTimer = ThreadPoolTimer.CreateTimer(async (source) =>
            {
                await Dispatcher.RunAsync(Windows.UI.Core.CoreDispatcherPriority.Normal, () =>
                {
                    var Success = sv.ChangeView(offsetToScrollTo, null, null, false);
                });
            }, period);
        }

        private double CenterPointOfViewportInExtent()
        {
            return sv.HorizontalOffset + sv.ViewportWidth / 2;
        }

        private int GetSelectedIndexFromViewport()
        {
            int selectedItemIndex = (int)Math.Floor((CenterPointOfViewportInExtent() - layout.Margin.Left + layout.Spacing / 2) / (layout.Spacing + layout.ItemWidth));
            selectedItemIndex %= repeater.ItemsSourceView.Count;

            return selectedItemIndex;
        }

        private object GetSelectedItemFromViewport()
        {
            var selectedIndex = GetSelectedIndexFromViewport();
            var selectedElement = repeater.TryGetElement(selectedIndex) as FrameworkElement;
            var selectedItem = (selectedElement == null ? null : ((FrameworkElement)selectedElement).DataContext);
            return selectedItem;
        }

#pragma warning disable CS0628 // New protected member declared in sealed class
        protected void OnCarouselPrevButtonPointerPressed(object sender, PointerRoutedEventArgs e)
#pragma warning restore CS0628 // New protected member declared in sealed class
        {
            bool pointerCaptured = ((UIElement)sender).CapturePointer(e.Pointer);

            if (ScrollViewerChangeViewTimer != null)
            {
                ScrollViewerChangeViewTimer.Cancel();
                ScrollViewerChangeViewTimer = null;
            }

            if (PrevButtonHoldTimer != null)
            {
                PrevButtonHoldTimer.Cancel();
                PrevButtonHoldTimer = null;
            }

            if (NextButtonHoldTimer != null)
            {
                NextButtonHoldTimer.Cancel();
                NextButtonHoldTimer = null;
            }

            if (PrevButtonContinuousScrollingPeriodicTimer != null)
            {
                PrevButtonContinuousScrollingPeriodicTimer.Cancel();
                PrevButtonContinuousScrollingPeriodicTimer = null;
            }

            if (NextButtonContinuousScrollingPeriodicTimer != null)
            {
                NextButtonContinuousScrollingPeriodicTimer.Cancel();
                NextButtonContinuousScrollingPeriodicTimer = null;
            }

            if (!pointerCaptured)
            {
                return;
            }

            SelectPreviousItem();

            PrevButtonHoldTimer = ThreadPoolTimer.CreateTimer(async (source) =>
            {
                await Dispatcher.RunAsync(Windows.UI.Core.CoreDispatcherPriority.Normal, () =>
                {
                    startContinuousScrolling(ScrollDirection.Previous);
                });
            },
            PrevNextButtonHoldPeriod);
        }

        private void OnCarouselPrevButtonPointerPressEnded()
        {
            if (ScrollViewerChangeViewTimer != null)
            {
                ScrollViewerChangeViewTimer.Cancel();
                ScrollViewerChangeViewTimer = null;
            }

            if (PrevButtonHoldTimer != null)
            {
                PrevButtonHoldTimer.Cancel();
                PrevButtonHoldTimer = null;
            }

            if (PrevButtonContinuousScrollingPeriodicTimer != null)
            {
                PrevButtonContinuousScrollingPeriodicTimer.Cancel();
                PrevButtonContinuousScrollingPeriodicTimer = null;
            }
        }

#pragma warning disable CS0628 // New protected member declared in sealed class
        protected void OnCarouselPrevButtonPointerCanceled(object sender, PointerRoutedEventArgs e)
#pragma warning restore CS0628 // New protected member declared in sealed class
        {
            OnCarouselPrevButtonPointerPressEnded();
        }

#pragma warning disable CS0628 // New protected member declared in sealed class
        protected void OnCarouselPrevButtonPointerReleased(object sender, PointerRoutedEventArgs e)
#pragma warning restore CS0628 // New protected member declared in sealed class
        {
            OnCarouselPrevButtonPointerPressEnded();
        }

#pragma warning disable CS0628 // New protected member declared in sealed class
        protected void OnCarouselPrevButtonPointerExited(object sender, PointerRoutedEventArgs e)
#pragma warning restore CS0628 // New protected member declared in sealed class
        {
            OnCarouselPrevButtonPointerPressEnded();
        }

#pragma warning disable CS0628 // New protected member declared in sealed class
        protected void OnCarouselPrevButtonPointerCaptureLost(object sender, PointerRoutedEventArgs e)
#pragma warning restore CS0628 // New protected member declared in sealed class
        {
            OnCarouselPrevButtonPointerPressEnded();
        }

#pragma warning disable CS0628 // New protected member declared in sealed class
        protected void OnCarouselNextButtonPointerPressed(object sender, PointerRoutedEventArgs e)
#pragma warning restore CS0628 // New protected member declared in sealed class
        {
            bool pointerCaptured = ((UIElement)sender).CapturePointer(e.Pointer);

            if (ScrollViewerChangeViewTimer != null)
            {
                ScrollViewerChangeViewTimer.Cancel();
                ScrollViewerChangeViewTimer = null;
            }

            if (NextButtonHoldTimer != null)
            {
                NextButtonHoldTimer.Cancel();
                NextButtonHoldTimer = null;
            }

            if (PrevButtonHoldTimer != null)
            {
                PrevButtonHoldTimer.Cancel();
                PrevButtonHoldTimer = null;
            }

            if (PrevButtonContinuousScrollingPeriodicTimer != null)
            {
                PrevButtonContinuousScrollingPeriodicTimer.Cancel();
                PrevButtonContinuousScrollingPeriodicTimer = null;
            }

            if (NextButtonContinuousScrollingPeriodicTimer != null)
            {
                NextButtonContinuousScrollingPeriodicTimer.Cancel();
                NextButtonContinuousScrollingPeriodicTimer = null;
            }

            if (!pointerCaptured)
            {
                return;
            }

            SelectNextItem();

            PrevButtonHoldTimer = ThreadPoolTimer.CreateTimer(async (source) =>
            {
                await Dispatcher.RunAsync(Windows.UI.Core.CoreDispatcherPriority.Normal, () =>
                {
                    startContinuousScrolling(ScrollDirection.Next);
                });
            },
            PrevNextButtonHoldPeriod);
        }

        private void OnCarouselNextButtonPointerPressEnded()
        {
            if (ScrollViewerChangeViewTimer != null)
            {
                ScrollViewerChangeViewTimer.Cancel();
                ScrollViewerChangeViewTimer = null;
            }

            if (NextButtonHoldTimer != null)
            {
                NextButtonHoldTimer.Cancel();
                NextButtonHoldTimer = null;
            }

            if (NextButtonContinuousScrollingPeriodicTimer != null)
            {
                NextButtonContinuousScrollingPeriodicTimer.Cancel();
                NextButtonContinuousScrollingPeriodicTimer = null;
            }
        }

#pragma warning disable CS0628 // New protected member declared in sealed class
        protected void OnCarouselNextButtonPointerCanceled(object sender, PointerRoutedEventArgs e)
#pragma warning restore CS0628 // New protected member declared in sealed class
        {
            OnCarouselNextButtonPointerPressEnded();
        }

#pragma warning disable CS0628 // New protected member declared in sealed class
        protected void OnCarouselNextButtonPointerReleased(object sender, PointerRoutedEventArgs e)
#pragma warning restore CS0628 // New protected member declared in sealed class
        {
            OnCarouselNextButtonPointerPressEnded();
        }

#pragma warning disable CS0628 // New protected member declared in sealed class
        protected void OnCarouselNextButtonPointerExited(object sender, PointerRoutedEventArgs e)
#pragma warning restore CS0628 // New protected member declared in sealed class
        {
            OnCarouselNextButtonPointerPressEnded();
        }

#pragma warning disable CS0628 // New protected member declared in sealed class
        protected void OnCarouselNextButtonPointerCaptureLost(object sender, PointerRoutedEventArgs e)
#pragma warning restore CS0628 // New protected member declared in sealed class
        {
            OnCarouselNextButtonPointerPressEnded();
        }

        private void startContinuousScrolling(ScrollDirection scrollDirection)
        {
            if ((scrollDirection == ScrollDirection.Previous && PrevButtonContinuousScrollingPeriodicTimer != null)
                || (scrollDirection == ScrollDirection.Next && NextButtonContinuousScrollingPeriodicTimer != null)
                || (scrollDirection == ScrollDirection.Previous && NextButtonContinuousScrollingPeriodicTimer != null)
                || (scrollDirection == ScrollDirection.Next && PrevButtonContinuousScrollingPeriodicTimer != null))
            {
                return;
            }

            if (scrollDirection == ScrollDirection.Previous)
            {
                PrevButtonContinuousScrollingPeriodicTimer = ThreadPoolTimer.CreatePeriodicTimer(async (source) =>
                {
                    await Dispatcher.RunAsync(Windows.UI.Core.CoreDispatcherPriority.Normal,
                        () =>
                        {
                            SelectPreviousItem(ContinousScrollingItemSkipCount);
                        });
                },
                PrevNextButtonContinousScrollingSelectionPeriod,
                async (source) =>
                {
                    await Dispatcher.RunAsync(Windows.UI.Core.CoreDispatcherPriority.Normal,
                        () =>
                        {
                            if (PrevButtonContinuousScrollingPeriodicTimer != null)
                            {
                                PrevButtonContinuousScrollingPeriodicTimer.Cancel();
                                PrevButtonContinuousScrollingPeriodicTimer = null;
                            }
                        });
                });
            }
            else
            {
                NextButtonContinuousScrollingPeriodicTimer = ThreadPoolTimer.CreatePeriodicTimer(async (source) =>
                {
                    await Dispatcher.RunAsync(Windows.UI.Core.CoreDispatcherPriority.Normal,
                        () =>
                        {
                            SelectNextItem(ContinousScrollingItemSkipCount);
                        });
                },
                PrevNextButtonContinousScrollingSelectionPeriod,
                async (source) =>
                {
                    await Dispatcher.RunAsync(Windows.UI.Core.CoreDispatcherPriority.Normal,
                        () =>
                        {
                            if (PrevButtonContinuousScrollingPeriodicTimer != null)
                            {
                                PrevButtonContinuousScrollingPeriodicTimer.Cancel();
                                PrevButtonContinuousScrollingPeriodicTimer = null;
                            }
                        });
                });
            }
        }
    }
}