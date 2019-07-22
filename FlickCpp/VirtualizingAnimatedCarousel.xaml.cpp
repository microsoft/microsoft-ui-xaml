//
// VirtualizingAnimatedCarousel.xaml.cpp
// Implementation of the VirtualizingAnimatedCarousel class
//

#include "pch.h"
#include "VirtualizingAnimatedCarousel.xaml.h"

using namespace FlickCpp;
using namespace Microsoft::UI::Xaml::Controls;
using namespace Platform;
using namespace Windows::Foundation;
using namespace Windows::System::Threading;
using namespace Windows::UI::Composition;
using namespace Windows::UI::Core;
using namespace Windows::UI::Xaml;
using namespace Windows::UI::Xaml::Controls;
using namespace Windows::UI::Xaml::Data;
using namespace Windows::UI::Xaml::Hosting;
using namespace Windows::UI::Xaml::Input;

/*static*/ const TimeSpan VirtualizingAnimatedCarousel::s_prevNextButtonHoldPeriod = TimeSpan { 3000000 /*3e+6 ticks = 3e+8 ns = 300 ms*/ };
/*static*/ constexpr int VirtualizingAnimatedCarousel::s_continousScrollingItemSkipCount = 2;
/*static*/ const TimeSpan VirtualizingAnimatedCarousel::s_prevNextButtonContinousScrollingSelectionPeriod = TimeSpan { 1000000 /*1e+6 ticks = 1e+8 ns = 100 ms*/ };
/*static*/ const TimeSpan VirtualizingAnimatedCarousel::s_scrollViewerChangeViewDelayPeriodForDefaultItemSelectionInEvenItemCountScenario = TimeSpan { 1500000 /*1.5e+6 ticks = 1.5e+8 ns = 150 ms*/ };
/*static*/ const TimeSpan VirtualizingAnimatedCarousel::s_scrollViewerChangeViewDelayPeriodToEnsureAnimationIsShown = TimeSpan { 100000 /*1e+5 ticks = 1e+7 ns = 10 ms*/ };
/*static*/ const TimeSpan VirtualizingAnimatedCarousel::s_preserveSelectedItemAfterVirtualizingAnimatedCarouselSizeChangePeriod = TimeSpan { 2000000 /*2e+6 ticks = 2e+8 ns = 200 ms*/ };

/*static*/ double VirtualizingAnimatedCarousel::Floor(double num)
{
    return static_cast<int>(num);
}

/*static*/ double VirtualizingAnimatedCarousel::AbsoluteValue(double num)
{
    return ((num < 0) ? (-1 * num) : num);
}

VirtualizingAnimatedCarousel::VirtualizingAnimatedCarousel()
{
	InitializeComponent();

    m_dispatcher = CoreWindow::GetForCurrentThread()->Dispatcher;
    m_viewModel = ref new DemoViewModel();
    PropertyChanged(this, ref new PropertyChangedEventArgs(L"ViewModel"));
    m_carouselPrevButton = UserCarouselPrevButton;
    m_carouselNextButton = UserCarouselNextButton;
    m_scrollViewer = UserCarouselScrollViewer;
    m_repeater = UserCarouselRepeater;
    m_repeaterLayout = UserCarouselRepeaterLayout;

    // Workaround for known numerical limitation on inset clips where scrollviewer fails to clip content on right side of viewport
    ElementCompositionPreview::GetElementVisual(m_scrollViewer)->Clip = ElementCompositionPreview::GetScrollViewerManipulationPropertySet(m_scrollViewer)->Compositor->CreateInsetClip();

    m_carouselPrevButton->AddHandler(UIElement::PointerPressedEvent, ref new PointerEventHandler(this, &VirtualizingAnimatedCarousel::CarouselPrevButton_PointerPressed), true);
    m_carouselPrevButton->AddHandler(UIElement::PointerCanceledEvent, ref new PointerEventHandler(this, &VirtualizingAnimatedCarousel::CarouselPrevButton_PointerCanceled), true);
    m_carouselPrevButton->AddHandler(UIElement::PointerReleasedEvent, ref new PointerEventHandler(this, &VirtualizingAnimatedCarousel::CarouselPrevButton_PointerReleased), true);
    m_carouselPrevButton->AddHandler(UIElement::PointerExitedEvent, ref new PointerEventHandler(this, &VirtualizingAnimatedCarousel::CarouselPrevButton_PointerExited), true);
    m_carouselPrevButton->AddHandler(UIElement::PointerCaptureLostEvent, ref new PointerEventHandler(this, &VirtualizingAnimatedCarousel::CarouselPrevButton_PointerCaptureLost), true);

    m_carouselNextButton->AddHandler(UIElement::PointerPressedEvent, ref new PointerEventHandler(this, &VirtualizingAnimatedCarousel::CarouselNextButton_PointerPressed), true);
    m_carouselNextButton->AddHandler(UIElement::PointerCanceledEvent, ref new PointerEventHandler(this, &VirtualizingAnimatedCarousel::CarouselNextButton_PointerCanceled), true);
    m_carouselNextButton->AddHandler(UIElement::PointerReleasedEvent, ref new PointerEventHandler(this, &VirtualizingAnimatedCarousel::CarouselNextButton_PointerReleased), true);
    m_carouselNextButton->AddHandler(UIElement::PointerExitedEvent, ref new PointerEventHandler(this, &VirtualizingAnimatedCarousel::CarouselNextButton_PointerExited), true);
    m_carouselNextButton->AddHandler(UIElement::PointerCaptureLostEvent, ref new PointerEventHandler(this, &VirtualizingAnimatedCarousel::CarouselNextButton_PointerCaptureLost), true);
}

DemoViewModel^ VirtualizingAnimatedCarousel::ViewModel::get()
{
    return m_viewModel;
}

ThreadPoolTimer^ VirtualizingAnimatedCarousel::PrevButtonContinuousScrollingPeriodicTimer::get()
{
    return m_prevButtonContinuousScrollingPeriodicTimer;
}

void VirtualizingAnimatedCarousel::PrevButtonContinuousScrollingPeriodicTimer::set(ThreadPoolTimer^ value)
{
    if (m_prevButtonContinuousScrollingPeriodicTimer != nullptr)
    {
        m_prevButtonContinuousScrollingPeriodicTimer->Cancel();
        m_prevButtonContinuousScrollingPeriodicTimer = nullptr;
    }

    m_prevButtonContinuousScrollingPeriodicTimer = value;
}

ThreadPoolTimer^ VirtualizingAnimatedCarousel::NextButtonContinuousScrollingPeriodicTimer::get()
{
    return m_nextButtonContinuousScrollingPeriodicTimer;
}

void VirtualizingAnimatedCarousel::NextButtonContinuousScrollingPeriodicTimer::set(ThreadPoolTimer^ value)
{
    if (m_nextButtonContinuousScrollingPeriodicTimer != nullptr)
    {
        m_nextButtonContinuousScrollingPeriodicTimer->Cancel();
        m_nextButtonContinuousScrollingPeriodicTimer = nullptr;
    }

    m_nextButtonContinuousScrollingPeriodicTimer = value;
}

ThreadPoolTimer^ VirtualizingAnimatedCarousel::PrevButtonHoldTimer::get()
{
    return m_prevButtonHoldTimer;
}

void VirtualizingAnimatedCarousel::PrevButtonHoldTimer::set(ThreadPoolTimer^ value)
{
    if (m_prevButtonHoldTimer != nullptr)
    {
        m_prevButtonHoldTimer->Cancel();
        m_prevButtonHoldTimer = nullptr;
    }

    m_prevButtonHoldTimer = value;
}

ThreadPoolTimer^ VirtualizingAnimatedCarousel::NextButtonHoldTimer::get()
{
    return m_nextButtonHoldTimer;
}

void VirtualizingAnimatedCarousel::NextButtonHoldTimer::set(ThreadPoolTimer^ value)
{
    if (m_nextButtonHoldTimer != nullptr)
    {
        m_nextButtonHoldTimer->Cancel();
        m_nextButtonHoldTimer = nullptr;
    }

    m_nextButtonHoldTimer = value;
}

ThreadPoolTimer^ VirtualizingAnimatedCarousel::ScrollViewerChangeViewTimer::get()
{
    return m_scrollViewerChangeViewTimer;
}

void VirtualizingAnimatedCarousel::ScrollViewerChangeViewTimer::set(ThreadPoolTimer^ value)
{
    if (m_scrollViewerChangeViewTimer != nullptr)
    {
        m_scrollViewerChangeViewTimer->Cancel();
        m_scrollViewerChangeViewTimer = nullptr;
    }

    m_scrollViewerChangeViewTimer = value;
}

ThreadPoolTimer^ VirtualizingAnimatedCarousel::PreserveSelectedItemAfterVirtualizingAnimatedCarouselSizeChangeTimer::get()
{
    return m_preserveSelectedItemAfterVirtualizingAnimatedCarouselSizeChangeTimer;
}

void VirtualizingAnimatedCarousel::PreserveSelectedItemAfterVirtualizingAnimatedCarouselSizeChangeTimer::set(ThreadPoolTimer^ value)
{
    if (m_preserveSelectedItemAfterVirtualizingAnimatedCarouselSizeChangeTimer != nullptr)
    {
        m_preserveSelectedItemAfterVirtualizingAnimatedCarouselSizeChangeTimer->Cancel();
        m_preserveSelectedItemAfterVirtualizingAnimatedCarouselSizeChangeTimer = nullptr;
    }

    m_preserveSelectedItemAfterVirtualizingAnimatedCarouselSizeChangeTimer = value;
}

void VirtualizingAnimatedCarousel::HideCarouselNextPrevButtons()
{
    m_carouselPrevButton->IsEnabled = false;
    m_carouselNextButton->IsEnabled = false;
}

void VirtualizingAnimatedCarousel::ShowCarouselNextPrevButtons()
{
    m_carouselPrevButton->IsEnabled = true;
    m_carouselNextButton->IsEnabled = true;
}

double VirtualizingAnimatedCarousel::CenterPointOfViewportInExtent()
{
    return m_scrollViewer->HorizontalOffset + m_scrollViewer->ViewportWidth / 2;
}

int VirtualizingAnimatedCarousel::GetSelectedIndexFromViewport()
{
    if ((m_repeater->ItemsSourceView == nullptr) || (m_repeater->ItemsSourceView->Count == 0))
    {
        return SelectableSnapPointForwardingRepeater::SelectedIndexValueWhenNoItemIsSelected;
    }

    int selectedIndex = static_cast<int>(Floor((CenterPointOfViewportInExtent() - m_repeaterLayout->Margin.Left + m_repeaterLayout->Spacing / 2) / (m_repeaterLayout->Spacing + m_repeaterLayout->ItemWidth)));
    selectedIndex %= m_repeater->ItemsSourceView->Count;
    return selectedIndex;
}

Object^ VirtualizingAnimatedCarousel::GetSelectedItemFromViewport()
{
    if ((m_repeater->ItemsSourceView == nullptr) || (m_repeater->ItemsSourceView->Count == 0))
    {
        return nullptr;
    }

    Object^ selectedItem = nullptr;
    int selectedIndex = GetSelectedIndexFromViewport();
    FrameworkElement^ selectedElement = dynamic_cast<FrameworkElement^>(m_repeater->TryGetElement(selectedIndex));

    if (selectedElement != nullptr)
    {
        selectedItem = selectedElement->DataContext;
    }

    return selectedItem;
}

void VirtualizingAnimatedCarousel::DetermineIfCarouselNextPrevButtonsShouldBeHidden()
{
    if ((m_repeater->ItemsSourceView->Count > 1) && (m_repeater->ItemsSourceView->Count < m_repeaterLayout->MaxNumberOfItemsThatCanFitInViewport))
    {
        double firstItemOffset = (m_repeaterLayout->FirstSnapPointOffset - (m_repeaterLayout->ItemWidth / 2));
        double lastItemOffset = (firstItemOffset + ((m_repeater->ItemsSourceView->Count - 1) * (m_repeaterLayout->ItemWidth + m_repeaterLayout->Spacing)));
        bool hideCarouselPrevButton = (CenterPointOfViewportInExtent() <= (firstItemOffset + m_repeaterLayout->ItemWidth + (m_repeaterLayout->Spacing / 2)));
        bool hideCarouselNextButton = (CenterPointOfViewportInExtent() >= (lastItemOffset - (m_repeaterLayout->Spacing / 2)));

        if (m_carouselPrevButton->IsEnabled == hideCarouselPrevButton)
        {
            m_carouselPrevButton->IsEnabled = !m_carouselPrevButton->IsEnabled;
        }

        if (m_carouselNextButton->IsEnabled == hideCarouselNextButton)
        {
            m_carouselNextButton->IsEnabled = !m_carouselNextButton->IsEnabled;
        }
    }
}

void VirtualizingAnimatedCarousel::SetSelectedItemInViewport(int itemIndexInItemsSource)
{
    if ((itemIndexInItemsSource != SelectableSnapPointForwardingRepeater::SelectedIndexValueWhenNoItemIsSelected)
        && ((itemIndexInItemsSource < 0) || (itemIndexInItemsSource >= m_repeater->ItemsSourceView->Count)))
    {
        throw ref new InvalidArgumentException(
            "itemIndexInItemsSource must be an integer, x, where ((x == "
            + SelectableSnapPointForwardingRepeater::SelectedIndexValueWhenNoItemIsSelected
            + ") || (0 <= x < itemCount)). In this case, itemCount = "
            + m_repeater->ItemsSourceView->Count.ToString()
            + " and x = "
            + itemIndexInItemsSource.ToString()
            + ".");
    }
    else if (itemIndexInItemsSource == SelectableSnapPointForwardingRepeater::SelectedIndexValueWhenNoItemIsSelected)
    {
        m_repeater->SetSelectedItemToNone();
        ResetCarouselWithDefaultSelectionForCurrentItemCount();
    }
    else if (m_repeater->ItemsSourceView->Count == 1)
    {
        ResetCarouselWithDefaultSelectionForCurrentItemCount();
    }
    else if (m_repeater->ItemsSourceView->Count < m_repeaterLayout->MaxNumberOfItemsThatCanFitInViewport)
    {
        m_scrollViewer->ChangeView(
            ((m_repeaterLayout->FirstSnapPointOffset + (itemIndexInItemsSource * (m_repeaterLayout->ItemWidth + m_repeaterLayout->Spacing))) - (m_scrollViewer->ViewportWidth / 2)) /*horizontalOffset*/,
            nullptr /*verticalOffset*/,
            nullptr /*zoomFactor*/,
            true /*disableAnimation*/);
        DetermineIfCarouselNextPrevButtonsShouldBeHidden();
    }
    else
    {
        m_scrollViewer->ChangeView(
            ((((m_repeaterLayout->ItemWidth / 2) + ((m_repeaterLayout->ItemWidth + m_repeaterLayout->Spacing) * m_repeater->ItemsSourceView->Count * Floor(m_repeaterLayout->RepeatCount / 2.0))) - (m_scrollViewer->ViewportWidth / 2)) + (itemIndexInItemsSource * (m_repeaterLayout->ItemWidth + m_repeaterLayout->Spacing))) /*horizontalOffset*/,
            nullptr /*verticalOffset*/,
            nullptr /*zoomFactor*/,
            true /*disableAnimation*/);
        ShowCarouselNextPrevButtons();
    }
}

void VirtualizingAnimatedCarousel::ScrollViewer_ViewChanged(Object^ /*sender*/, ScrollViewerViewChangedEventArgs^ e)
{
    if (!e->IsIntermediate)
    {
        m_repeater->SelectedIndex = GetSelectedIndexFromViewport();
        m_repeater->SelectedItem = GetSelectedItemFromViewport();
    }

    DetermineIfCarouselNextPrevButtonsShouldBeHidden();
}

void VirtualizingAnimatedCarousel::ScrollViewer_ViewChanging(Object^ /*sender*/, ScrollViewerViewChangingEventArgs^ /*e*/)
{
    DetermineIfCarouselNextPrevButtonsShouldBeHidden();
}

void VirtualizingAnimatedCarousel::ScrollViewer_LayoutUpdated(Object^ sender, Object^ e)
{
    m_selectedItemIndexPriorToVirtualizingAnimatedCarouselSizeChange = GetSelectedIndexFromViewport();
}

void VirtualizingAnimatedCarousel::ResetCarouselWithDefaultSelectionForCurrentItemCount()
{
    if ((m_repeater->ItemsSourceView == nullptr) || (m_repeater->ItemsSourceView->Count == 0))
    {
        HideCarouselNextPrevButtons();
        return;
    }
    else if (m_repeater->ItemsSourceView->Count == 1)
    {
        m_scrollViewer->ChangeView(
            0.0 /*horizontalOffset*/,
            nullptr /*verticalOffset*/,
            nullptr /*zoomFactor*/,
            true /*disableAnimation*/);
        HideCarouselNextPrevButtons();
    }
    else if (m_repeater->ItemsSourceView->Count == 2)
    {
        m_scrollViewer->ChangeView(
            static_cast<double>(m_repeaterLayout->FirstSnapPointOffset) /*horizontalOffset*/,
            nullptr /*verticalOffset*/,
            nullptr /*zoomFactor*/,
            true /*disableAnimation*/);
        DetermineIfCarouselNextPrevButtonsShouldBeHidden();
    }
    else if (m_repeater->ItemsSourceView->Count < m_repeaterLayout->MaxNumberOfItemsThatCanFitInViewport)
    {
        if ((m_repeater->ItemsSourceView->Count % 2) == 0)
        {
            // The ThreadPoolTimer and m_dispatcher->RunIdleAsync combo here are necessary to
            // fix a ScrollViewer issue where some background process cause the scrollviewer's ChangeView to be
            // interrupted which prevents the offset of the scrollviewer from being updated with the passed-in value.
            // E.g. Without the timing/dispatching here, in the 4-item scenario:
            // Expected result: Item 2/4 is selected
            // Actual result: Item 3/4 is selected
            auto weakThis = WeakReference(this);
            ThreadPoolTimer::CreateTimer(
                ref new TimerElapsedHandler(
                    [weakThis](ThreadPoolTimer^ /*timer*/)
                    {
                        if (auto thisRef = weakThis.Resolve<VirtualizingAnimatedCarousel>())
                        {
                            thisRef->m_dispatcher->RunIdleAsync(ref new IdleDispatchedHandler([weakThis](IdleDispatchedHandlerArgs^ /*args*/)
                            {
                                if (auto thisRef = weakThis.Resolve<VirtualizingAnimatedCarousel>())
                                {
                                    thisRef->m_scrollViewer->ChangeView(
                                        ((thisRef->m_scrollViewer->ExtentWidth / 2) - (thisRef->m_scrollViewer->ViewportWidth / 2) - (thisRef->m_repeaterLayout->Spacing / 2) - (thisRef->m_repeaterLayout->ItemWidth / 2)) /*horizontalOffset*/,
                                        nullptr /*verticalOffset*/,
                                        nullptr /*zoomFactor*/,
                                        true /*disableAnimation*/);
                                    thisRef->DetermineIfCarouselNextPrevButtonsShouldBeHidden();
                                }
                            }));
                        }
                    }),
                s_scrollViewerChangeViewDelayPeriodForDefaultItemSelectionInEvenItemCountScenario);
        }
        else
        {
            m_scrollViewer->ChangeView(
                ((m_scrollViewer->ExtentWidth / 2) - m_scrollViewer->ViewportWidth / 2) /*horizontalOffset*/,
                nullptr /*verticalOffset*/,
                nullptr /*zoomFactor*/,
                true /*disableAnimation*/);
            DetermineIfCarouselNextPrevButtonsShouldBeHidden();
        }
    }
    else
    {
        m_scrollViewer->ChangeView(
            (((m_repeaterLayout->ItemWidth / 2) + ((m_repeaterLayout->ItemWidth + m_repeaterLayout->Spacing) * m_repeater->ItemsSourceView->Count * Floor(m_repeaterLayout->RepeatCount / 2.0))) - (m_scrollViewer->ViewportWidth / 2)) /*horizontalOffset*/,
            nullptr /*verticalOffset*/,
            nullptr /*zoomFactor*/,
            true /*disableAnimation*/);
        ShowCarouselNextPrevButtons();
    }

    m_repeater->SelectedIndex = GetSelectedIndexFromViewport();
    Object^ selectedItem = GetSelectedItemFromViewport();
    m_repeater->SelectedItem = selectedItem;
}

void VirtualizingAnimatedCarousel::Repeater_Loaded(Object^ /*sender*/, RoutedEventArgs^ /*e*/)
{
    ResetCarouselWithDefaultSelectionForCurrentItemCount();
}

void VirtualizingAnimatedCarousel::Repeater_ElementPrepared(ItemsRepeater^ /*sender*/, ItemsRepeaterElementPreparedEventArgs^ e)
{
    Visual^ item = ElementCompositionPreview::GetElementVisual(e->Element);
    Visual^ svVisual = ElementCompositionPreview::GetElementVisual(m_scrollViewer);
    CompositionPropertySet^ scrollProperties = ElementCompositionPreview::GetScrollViewerManipulationPropertySet(m_scrollViewer);

    // Animate each item's centerpoint based on the item's distance from the center of the viewport
    // translate the position of each item horizontally closer to the center of the viewport as much as is necessary
    // in order to ensure that the Spacing property of the ItemsRepeater is still respected after the items have been scaled.
    ExpressionAnimation^ centerPointExpression = scrollProperties->Compositor->CreateExpressionAnimation();
    centerPointExpression->SetReferenceParameter("item", item);
    centerPointExpression->SetReferenceParameter("svVisual", svVisual);
    centerPointExpression->SetReferenceParameter("scrollProperties", scrollProperties);
    centerPointExpression->SetScalarParameter("spacing", static_cast<float>(m_repeaterLayout->Spacing));
    String^ centerPointExpressionString = "Vector3(((item.Size.X/2) + ((((item.Offset.X + (item.Size.X/2)) < ((svVisual.Size.X/2) - scrollProperties.Translation.X)) ? 1 : -1) * (((item.Size.X/2) * clamp((abs((item.Offset.X + (item.Size.X/2)) - ((svVisual.Size.X/2) - scrollProperties.Translation.X)) / (item.Size.X + spacing)), 0, 1)) + ((item.Size.X) * max((abs((item.Offset.X + (item.Size.X/2)) - ((svVisual.Size.X/2) - scrollProperties.Translation.X)) / (item.Size.X + spacing)) - 1, 0))) )), item.Size.Y/2, 0)";
    centerPointExpression->Expression = centerPointExpressionString;
    centerPointExpression->Target = "CenterPoint";

    // scale the item based on the distance of the item relative to the center of the viewport.            
    ExpressionAnimation^ scaleExpression = scrollProperties->Compositor->CreateExpressionAnimation();
    scaleExpression->SetReferenceParameter("svVisual", svVisual);
    scaleExpression->SetReferenceParameter("scrollProperties", scrollProperties);
    scaleExpression->SetReferenceParameter("item", item);
    scaleExpression->SetScalarParameter("scaleRatioXY", static_cast<float>(m_repeaterLayout->ItemScaleRatio));
    scaleExpression->SetScalarParameter("spacing", static_cast<float>(m_repeaterLayout->Spacing));
    String^ scalarScaleExpressionString = "clamp((scaleRatioXY * (1 + (1 - (abs((item.Offset.X + (item.Size.X/2)) - ((svVisual.Size.X/2) - scrollProperties.Translation.X)) / (item.Size.X + spacing))))), scaleRatioXY, 1)";
    String^ scaleExpressionString = ("Vector3(" + scalarScaleExpressionString + ", " + scalarScaleExpressionString + ", 0)");
    scaleExpression->Expression = scaleExpressionString;
    scaleExpression->Target = "Scale";

    CompositionAnimationGroup^ animationGroup = scrollProperties->Compositor->CreateAnimationGroup();
    animationGroup->Add(centerPointExpression);
    animationGroup->Add(scaleExpression);

    item->StartAnimationGroup(animationGroup);
}

void VirtualizingAnimatedCarousel::CancelAllCarouselPrevButtonTimers()
{
    if (ScrollViewerChangeViewTimer != nullptr)
    {
        ScrollViewerChangeViewTimer->Cancel();
        ScrollViewerChangeViewTimer = nullptr;
    }

    if (PrevButtonHoldTimer != nullptr)
    {
        PrevButtonHoldTimer->Cancel();
        PrevButtonHoldTimer = nullptr;
    }

    if (PrevButtonContinuousScrollingPeriodicTimer != nullptr)
    {
        PrevButtonContinuousScrollingPeriodicTimer->Cancel();
        PrevButtonContinuousScrollingPeriodicTimer = nullptr;
    }
}

void VirtualizingAnimatedCarousel::CancelAllCarouselNextButtonTimers()
{
    if (ScrollViewerChangeViewTimer != nullptr)
    {
        ScrollViewerChangeViewTimer->Cancel();
        ScrollViewerChangeViewTimer = nullptr;
    }

    if (NextButtonHoldTimer != nullptr)
    {
        NextButtonHoldTimer->Cancel();
        NextButtonHoldTimer = nullptr;
    }

    if (NextButtonContinuousScrollingPeriodicTimer != nullptr)
    {
        NextButtonContinuousScrollingPeriodicTimer->Cancel();
        NextButtonContinuousScrollingPeriodicTimer = nullptr;
    }
}

void VirtualizingAnimatedCarousel::CancelAllCarouselScrollRelatedTimers()
{
    CancelAllCarouselPrevButtonTimers();
    CancelAllCarouselNextButtonTimers();
}

void VirtualizingAnimatedCarousel::ScrollViewer_Tapped(Object^ /*sender*/, TappedRoutedEventArgs^ e)
{
    CancelAllCarouselScrollRelatedTimers();
    double centerOfViewportOffsetInScrollViewer = CenterPointOfViewportInExtent();
    // In the nominal case, centerOfViewportOffsetInScrollViewer will be the offset of the current centerpoint in the scrollviewer's viewport;
    // however, if the "center" item is not perfectly centered (i.e-> where the centerpoint falls on the item's size.x/2)
    // then set centerOfViewportOffsetInScrollViewer equal to the offset where the "centered" item would be perfectly centered.
    // This makes later calculations much simpler with respect to item animations.
    centerOfViewportOffsetInScrollViewer -= (static_cast<int>((centerOfViewportOffsetInScrollViewer + m_repeaterLayout->Spacing / 2) - m_repeaterLayout->Margin.Left) % static_cast<int>(m_repeaterLayout->Spacing + m_repeaterLayout->ItemWidth));
    centerOfViewportOffsetInScrollViewer += (m_repeaterLayout->Spacing / 2 + m_repeaterLayout->ItemWidth / 2);

    double tapPositionOffsetInScrollViewer = e->GetPosition(m_scrollViewer).X + m_scrollViewer->HorizontalOffset;
    double tapPositionDistanceFromSVCenterPoint = AbsoluteValue(tapPositionOffsetInScrollViewer - centerOfViewportOffsetInScrollViewer);
    double offsetToScrollTo;

    if (tapPositionDistanceFromSVCenterPoint <= (m_repeaterLayout->ItemWidth / 2 + m_repeaterLayout->Spacing / 2))
    {
        offsetToScrollTo = centerOfViewportOffsetInScrollViewer - m_scrollViewer->ViewportWidth / 2;
    }
    else
    {
        tapPositionDistanceFromSVCenterPoint -= m_repeaterLayout->ItemWidth / 2 + m_repeaterLayout->Spacing / 2;
        int tappedItemIndexDifferenceFromCenter = static_cast<int>(Floor(tapPositionDistanceFromSVCenterPoint / (m_repeaterLayout->ItemWidth * m_repeaterLayout->ItemScaleRatio + m_repeaterLayout->Spacing))) + 1;
        offsetToScrollTo = m_scrollViewer->HorizontalOffset + (((tapPositionOffsetInScrollViewer < centerOfViewportOffsetInScrollViewer) ? -1 : 1) * (tappedItemIndexDifferenceFromCenter * (m_repeaterLayout->ItemWidth + m_repeaterLayout->Spacing)));
    }

    if (offsetToScrollTo != m_scrollViewer->HorizontalOffset)
    {
        // This odd delay is required in order to ensure that the scrollviewer animates the scroll
        // on every call to ChangeView.
        auto weakThis = WeakReference(this);
        ScrollViewerChangeViewTimer = ThreadPoolTimer::CreateTimer(
            ref new TimerElapsedHandler(
                [weakThis, offsetToScrollTo](ThreadPoolTimer^ /*timer*/)
                {
                    if (auto thisRef = weakThis.Resolve<VirtualizingAnimatedCarousel>())
                    {
                        thisRef->m_dispatcher->RunAsync(
                            CoreDispatcherPriority::Normal,
                            ref new DispatchedHandler([weakThis, offsetToScrollTo]()
                            {
                                if (auto thisRef = weakThis.Resolve<VirtualizingAnimatedCarousel>())
                                {
                                    thisRef->m_scrollViewer->ChangeView(
                                        offsetToScrollTo /*horizontalOffset*/,
                                        nullptr /*verticalOffset*/,
                                        nullptr /*zoomFactor*/,
                                        false /*disableAnimation*/);
                                }
                            }));
                    }
                }),
            s_scrollViewerChangeViewDelayPeriodToEnsureAnimationIsShown);
    }
}

void VirtualizingAnimatedCarousel::SelectNextItem(int numberOfItemsToSkip)
{
    // In the nominal case, centerOfViewportOffsetInScrollViewer will be the offset of the current centerpoint in the scrollviewer's viewport;
    // however, if the "center" item is not perfectly centered (i.e. where the centerpoint falls on the item's size.x/2)
    // then set centerOfViewportOffsetInScrollViewer equal to the offset where the "centered" item would be perfectly centered.
    // This makes later calculations much simpler with respect to item animations.
    double centerOfViewportOffsetInScrollViewer = CenterPointOfViewportInExtent();
    centerOfViewportOffsetInScrollViewer -= (static_cast<int>((centerOfViewportOffsetInScrollViewer + m_repeaterLayout->Spacing / 2) - m_repeaterLayout->Margin.Left) % static_cast<int>(m_repeaterLayout->Spacing + m_repeaterLayout->ItemWidth));
    centerOfViewportOffsetInScrollViewer += (m_repeaterLayout->Spacing / 2 + m_repeaterLayout->ItemWidth / 2);
    double newSelectedItemDistanceFromCenterPoint = (m_repeaterLayout->ItemWidth + m_repeaterLayout->Spacing) * (1 + numberOfItemsToSkip);
    double offsetToScrollTo = (centerOfViewportOffsetInScrollViewer + newSelectedItemDistanceFromCenterPoint - (m_scrollViewer->ViewportWidth / 2));

    // This odd delay is required in order to ensure that the scrollviewer animates the scroll
    // on every call to ChangeView.
    auto weakThis = WeakReference(this);
    ScrollViewerChangeViewTimer = ThreadPoolTimer::CreateTimer(
        ref new TimerElapsedHandler(
            [weakThis, offsetToScrollTo](ThreadPoolTimer^ /*timer*/)
            {
                if (auto thisRef = weakThis.Resolve<VirtualizingAnimatedCarousel>())
                {
                    thisRef->m_dispatcher->RunAsync(
                        CoreDispatcherPriority::Normal,
                        ref new DispatchedHandler([weakThis, offsetToScrollTo]()
                    {
                        if (auto thisRef = weakThis.Resolve<VirtualizingAnimatedCarousel>())
                        {
                            thisRef->m_scrollViewer->ChangeView(
                                offsetToScrollTo /*horizontalOffset*/,
                                nullptr /*verticalOffset*/,
                                nullptr /*zoomFactor*/,
                                false /*disableAnimation*/);
                        }
                    }));
                }
            }),
        s_scrollViewerChangeViewDelayPeriodToEnsureAnimationIsShown);
}

void VirtualizingAnimatedCarousel::SelectNextItem()
{
    SelectNextItem(0 /*numberOfItemsToSkip*/);
}

void VirtualizingAnimatedCarousel::SelectPreviousItem(int numberOfItemsToSkip)
{
    // In the nominal case, centerOfViewportOffsetInScrollViewer will be the offset of the current centerpoint in the scrollviewer's viewport;
    // however, if the "center" item is not perfectly centered (i.e. where the centerpoint falls on the item's size.x/2)
    // then set centerOfViewportOffsetInScrollViewer equal to the offset where the "centered" item would be perfectly centered.
    // This makes later calculations much simpler with respect to item animations.
    double centerOfViewportOffsetInScrollViewer = CenterPointOfViewportInExtent();
    centerOfViewportOffsetInScrollViewer -= (static_cast<int>((centerOfViewportOffsetInScrollViewer + m_repeaterLayout->Spacing / 2) - m_repeaterLayout->Margin.Left) % static_cast<int>(m_repeaterLayout->Spacing + m_repeaterLayout->ItemWidth));
    centerOfViewportOffsetInScrollViewer += (m_repeaterLayout->Spacing / 2 + m_repeaterLayout->ItemWidth / 2);
    double newSelectedItemDistanceFromCenterPoint = (m_repeaterLayout->ItemWidth + m_repeaterLayout->Spacing) * (1 + numberOfItemsToSkip);
    double offsetToScrollTo = (centerOfViewportOffsetInScrollViewer - newSelectedItemDistanceFromCenterPoint - (m_scrollViewer->ViewportWidth / 2));

    // This odd delay is required in order to ensure that the scrollviewer animates the scroll
    // on every call to ChangeView.
    auto weakThis = WeakReference(this);
    ScrollViewerChangeViewTimer = ThreadPoolTimer::CreateTimer(
        ref new TimerElapsedHandler(
            [weakThis, offsetToScrollTo](ThreadPoolTimer^ /*timer*/)
            {
                if (auto thisRef = weakThis.Resolve<VirtualizingAnimatedCarousel>())
                {
                    thisRef->m_dispatcher->RunAsync(
                        CoreDispatcherPriority::Normal,
                        ref new DispatchedHandler([weakThis, offsetToScrollTo]()
                    {
                        if (auto thisRef = weakThis.Resolve<VirtualizingAnimatedCarousel>())
                        {
                            thisRef->m_scrollViewer->ChangeView(
                                offsetToScrollTo /*horizontalOffset*/,
                                nullptr /*verticalOffset*/,
                                nullptr /*zoomFactor*/,
                                false /*disableAnimation*/);
                        }
                    }));
                }
            }),
        s_scrollViewerChangeViewDelayPeriodToEnsureAnimationIsShown);
}

void VirtualizingAnimatedCarousel::SelectPreviousItem()
{
    SelectPreviousItem(0 /*numberOfItemsToSkip*/);
}

void VirtualizingAnimatedCarousel::StartContinuousScrolling(ScrollDirection scrollDirection)
{
    CancelAllCarouselScrollRelatedTimers();

    if (scrollDirection == ScrollDirection::Previous)
    {
        auto weakThis = WeakReference(this);
        PrevButtonContinuousScrollingPeriodicTimer = ThreadPoolTimer::CreatePeriodicTimer(
            ref new TimerElapsedHandler(
                [weakThis](ThreadPoolTimer^ /*timer*/)
                {
                    if (auto thisRef = weakThis.Resolve<VirtualizingAnimatedCarousel>())
                    {
                        thisRef->m_dispatcher->RunAsync(
                            CoreDispatcherPriority::Normal,
                            ref new DispatchedHandler([weakThis]()
                            {
                                if (auto thisRef = weakThis.Resolve<VirtualizingAnimatedCarousel>())
                                {
                                    thisRef->SelectPreviousItem(thisRef->s_continousScrollingItemSkipCount);
                                }
                            }));
                    }
                }),
            s_prevNextButtonContinousScrollingSelectionPeriod);
    }
    else
    {
        auto weakThis = WeakReference(this);
        NextButtonContinuousScrollingPeriodicTimer = ThreadPoolTimer::CreatePeriodicTimer(
            ref new TimerElapsedHandler(
                [weakThis](ThreadPoolTimer^ /*timer*/)
                {
                    if (auto thisRef = weakThis.Resolve<VirtualizingAnimatedCarousel>())
                    {
                        thisRef->m_dispatcher->RunAsync(
                            CoreDispatcherPriority::Normal,
                            ref new DispatchedHandler([weakThis]()
                        {
                            if (auto thisRef = weakThis.Resolve<VirtualizingAnimatedCarousel>())
                            {
                                thisRef->SelectNextItem(thisRef->s_continousScrollingItemSkipCount);
                            }
                        }));
                    }
                }),
            s_prevNextButtonContinousScrollingSelectionPeriod);
    }
}

void VirtualizingAnimatedCarousel::CarouselPrevButton_PointerPressed(Object^ sender, PointerRoutedEventArgs^ e)
{
    if (!(safe_cast<UIElement^>(sender)->CapturePointer(e->Pointer)))
    {
        return;
    }

    CancelAllCarouselScrollRelatedTimers();
    SelectPreviousItem();

    auto weakThis = WeakReference(this);
    PrevButtonHoldTimer = ThreadPoolTimer::CreateTimer(
        ref new TimerElapsedHandler(
            [weakThis](ThreadPoolTimer^ /*timer*/)
            {
                if (auto thisRef = weakThis.Resolve<VirtualizingAnimatedCarousel>())
                {
                    thisRef->m_dispatcher->RunAsync(
                        CoreDispatcherPriority::Normal,
                        ref new DispatchedHandler([weakThis]()
                    {
                        if (auto thisRef = weakThis.Resolve<VirtualizingAnimatedCarousel>())
                        {
                            thisRef->StartContinuousScrolling(VirtualizingAnimatedCarousel::ScrollDirection::Previous);
                        }
                    }));
                }
            }),
        s_prevNextButtonHoldPeriod);
}

void VirtualizingAnimatedCarousel::CarouselPrevButton_PointerCanceled(Object^ /*sender*/, PointerRoutedEventArgs^ /*e*/)
{
    CancelAllCarouselScrollRelatedTimers();
}

void VirtualizingAnimatedCarousel::CarouselPrevButton_PointerReleased(Object^ /*sender*/, PointerRoutedEventArgs^ /*e*/)
{
    CancelAllCarouselScrollRelatedTimers();
}

void VirtualizingAnimatedCarousel::CarouselPrevButton_PointerExited(Object^ /*sender*/, PointerRoutedEventArgs^ /*e*/)
{
    CancelAllCarouselScrollRelatedTimers();
}

void VirtualizingAnimatedCarousel::CarouselPrevButton_PointerCaptureLost(Object^ /*sender*/, PointerRoutedEventArgs^ /*e*/)
{
    CancelAllCarouselScrollRelatedTimers();
}

void VirtualizingAnimatedCarousel::CarouselNextButton_PointerPressed(Object^ sender, PointerRoutedEventArgs^ e)
{
    if (!(safe_cast<UIElement^>(sender)->CapturePointer(e->Pointer)))
    {
        return;
    }

    CancelAllCarouselScrollRelatedTimers();
    SelectNextItem();

    auto weakThis = WeakReference(this);
    NextButtonHoldTimer = ThreadPoolTimer::CreateTimer(
        ref new TimerElapsedHandler(
            [weakThis](ThreadPoolTimer^ /*timer*/)
            {
                if (auto thisRef = weakThis.Resolve<VirtualizingAnimatedCarousel>())
                {
                    thisRef->m_dispatcher->RunAsync(
                        CoreDispatcherPriority::Normal,
                        ref new DispatchedHandler([weakThis]()
                    {
                        if (auto thisRef = weakThis.Resolve<VirtualizingAnimatedCarousel>())
                        {
                            thisRef->StartContinuousScrolling(VirtualizingAnimatedCarousel::ScrollDirection::Next);
                        }
                    }));
                }
            }),
        s_prevNextButtonHoldPeriod);
}

void VirtualizingAnimatedCarousel::CarouselNextButton_PointerCanceled(Object^ /*sender*/, PointerRoutedEventArgs^ /*e*/)
{
    CancelAllCarouselScrollRelatedTimers();
}

void VirtualizingAnimatedCarousel::CarouselNextButton_PointerReleased(Object^ /*sender*/, PointerRoutedEventArgs^ /*e*/)
{
    CancelAllCarouselScrollRelatedTimers();
}

void VirtualizingAnimatedCarousel::CarouselNextButton_PointerExited(Object^ /*sender*/, PointerRoutedEventArgs^ /*e*/)
{
    CancelAllCarouselScrollRelatedTimers();
}

void VirtualizingAnimatedCarousel::CarouselNextButton_PointerCaptureLost(Object^ /*sender*/, PointerRoutedEventArgs^ /*e*/)
{
    CancelAllCarouselScrollRelatedTimers();
}

void VirtualizingAnimatedCarousel::VirtualizingAnimatedCarousel_SizeChanged(Object^ /*sender*/, SizeChangedEventArgs^ /*e*/)
{
    if (PreserveSelectedItemAfterVirtualizingAnimatedCarouselSizeChangeTimer != nullptr)
    {
        return;
    }

    SetSelectedItemInViewport(m_selectedItemIndexPriorToVirtualizingAnimatedCarouselSizeChange);

    auto weakThis = WeakReference(this);
    PreserveSelectedItemAfterVirtualizingAnimatedCarouselSizeChangeTimer = ThreadPoolTimer::CreateTimer(
        ref new TimerElapsedHandler(
            [weakThis](ThreadPoolTimer^ /*timer*/)
            {
                if (auto thisRef = weakThis.Resolve<VirtualizingAnimatedCarousel>())
                {
                    thisRef->m_dispatcher->RunIdleAsync(
                        ref new IdleDispatchedHandler([weakThis](IdleDispatchedHandlerArgs^ /*args*/)
                    {
                        if (auto thisRef = weakThis.Resolve<VirtualizingAnimatedCarousel>())
                        {
                            thisRef->PreserveSelectedItemAfterVirtualizingAnimatedCarouselSizeChangeTimer = nullptr;
                        }
                    }));
                }
            }),
        s_preserveSelectedItemAfterVirtualizingAnimatedCarouselSizeChangePeriod);
}
