// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#include "precomp.h"
#include "ModernCollectionBasePanel.g.h"
#include "ScrollViewer.g.h"
#include "ScrollViewerViewChangingEventArgs.g.h"
#include "ScrollViewerViewChangedEventArgs.g.h"
#include "ItemsPresenter.g.h"
#include "VisualTreeHelper.h"
#include "host.h"
#include "BuildTreeService.g.h"
#include "BudgetManager.g.h"

using namespace DirectUI;
using namespace DirectUISynonyms;
using namespace std::placeholders;

using xaml_controls::LayoutReference;
using xaml_controls::EstimationReference;

// Work around disruptive max/min macros
#undef max
#undef min

// Uncomment to output ModernCollectionBasePanel debugging information
//#define MCBP_DEBUG

// Determine if a point is inside the window, or is before or after it in the virtualizing direction.
RelativePosition ModernCollectionBasePanel::GetReferenceDirectionFromWindow(
    _In_ wf::Rect referenceRect,
    _In_ wf::Rect window) const
{
    const float firstReferenceEdge = referenceRect.*PointFromRectInVirtualizingDirection();
    const float lastReferenceEdge = firstReferenceEdge + referenceRect.*SizeFromRectInVirtualizingDirection();
    const float firstWindowEdge = window.*PointFromRectInVirtualizingDirection();
    const float lastWindowEdge = firstWindowEdge + window.*SizeFromRectInVirtualizingDirection();

    RelativePosition result;

    if (lastReferenceEdge < firstWindowEdge)
    {
        result = RelativePosition::Before;
    }
    else if (lastWindowEdge < firstReferenceEdge)
    {
        result = RelativePosition::After;
    }
    else
    {
        result = RelativePosition::Inside;
    }

    return result;
}

// Calculates the visible window by analyzing our layout and our parent ScrollViewer's state.
_Check_return_ HRESULT ModernCollectionBasePanel::CalculateVisibleWindowFromScrollPoint(
    _In_ const wf::Point& newScrollPoint,
    _In_ FLOAT zoomFactor,
    _Out_ wf::Rect* pVisibleWindow)
{
    HRESULT hr = S_OK;

    auto spScrollViewer = m_wrScrollViewer.AsOrNull<IScrollViewer>();
    ASSERT(spScrollViewer);
    
    DOUBLE viewWidth = 0;
    DOUBLE viewHeight = 0;
    wf::Rect scrollViewport = {};
        
    IFC(spScrollViewer->get_ViewportWidth(&viewWidth));
    IFC(spScrollViewer->get_ViewportHeight(&viewHeight));

    scrollViewport.X = newScrollPoint.X;
    scrollViewport.Y = newScrollPoint.Y;
    scrollViewport.Width = static_cast<float>(viewWidth);
    scrollViewport.Height = static_cast<float>(viewHeight);

    // Build our own visible window
    IFC(CalculateVisibleWindowFromScrollData(scrollViewport, zoomFactor, pVisibleWindow));
            
    m_windowState.validWindowCalculation = true;
    
Cleanup:
    RRETURN(hr);
}

// Calculates the visible window from a set of scroll data
_Check_return_ HRESULT ModernCollectionBasePanel::CalculateVisibleWindowFromScrollData(
    _In_ const wf::Rect& scrollViewport,
    _In_ FLOAT zoomFactor,
    _Out_ wf::Rect* pVisibleWindow)
{
    HRESULT hr = S_OK;

    xaml_controls::Orientation orientation;
    wf::Rect ourVisibleWindow = {};

    IFC(m_spLayoutStrategy->GetVirtualizationDirection(&orientation));

    // Get the scroll content's viewport, projected onto our panel
    ourVisibleWindow.X = scrollViewport.X / zoomFactor;
    ourVisibleWindow.Y = scrollViewport.Y / zoomFactor;

    // Until XAML converges on whether or not to reflow and remeasure based on zoom factors,
    // I'm going to ignore the zoom factor in the non-virtualizing direction.
    // All virtualization code will see the original non-zoomed window size in the regard
    switch (orientation)
    {
    case xaml_controls::Orientation_Horizontal:
        ourVisibleWindow.Width = static_cast<FLOAT>(scrollViewport.Width) / zoomFactor;
        ourVisibleWindow.Height = static_cast<FLOAT>(m_windowState.lastAvailableSize.Height);
        break;

    case xaml_controls::Orientation_Vertical:
        ourVisibleWindow.Width = static_cast<FLOAT>(m_windowState.lastAvailableSize.Width);
        ourVisibleWindow.Height = static_cast<FLOAT>(scrollViewport.Height) / zoomFactor;
        break;
    }
    
    // ItemsPresenter may host a header / lead padding that we need to account for,
    // so in this case we subtract our layout position relative to our visual parent.
    ourVisibleWindow.X -= m_originFromItemsPresenter.X;
    ourVisibleWindow.Y -= m_originFromItemsPresenter.Y;
    
    *pVisibleWindow = ourVisibleWindow;
        
Cleanup:
    RRETURN(hr);
}

// Using the current visible window, calculate the realization window (taking into
// account buffer space, etc).
// 
// allowCache should be true during normal measure passes and false otherwise when initially setting the realization window.
// The allowCache parameter allows us to incrementally build a cache by inflating the realization window during
// measure passes.  When true, we will increase the realization window by CacheBufferPerSideInflationPixelDelta.
// When false, we will reset the cache buffers to 0.
_Check_return_ HRESULT ModernCollectionBasePanel::SetRealizationWindowFromVisibleWindow(
    _In_ BOOLEAN allowCache)
{
    HRESULT hr = S_OK;

    wf::Size bufferSize = {};
    xaml_controls::Orientation panningOrientation;
    DOUBLE visibleWindowLength = 0.0;
    DOUBLE maxCacheBufferPerSide = 0.0;
    auto spScrollViewer = m_wrScrollViewer.AsOrNull<IScrollViewer>();
    BOOLEAN barDragging = FALSE;
    BOOLEAN isLastElementRealized = FALSE;
    FLOAT panelExtent = 0.0;

    // Determine the maximum amount of cache buffer per side.
    IFC(m_spLayoutStrategy->GetVirtualizationDirection(&panningOrientation));
    switch (panningOrientation)
    {
    case xaml_controls::Orientation_Horizontal:
        visibleWindowLength = m_windowState.GetVisibleWindow().Width;
        break;

    case xaml_controls::Orientation_Vertical:
        visibleWindowLength = m_windowState.GetVisibleWindow().Height;
        break;

    default:
        ASSERT(FALSE);
        break;
    }
    maxCacheBufferPerSide = (m_cacheLength * visibleWindowLength) / 2;
    ASSERT(maxCacheBufferPerSide >= 0);

    if (spScrollViewer)
    {
        IFC(spScrollViewer.Cast<ScrollViewer>()->IsThumbDragging(&barDragging));

        if (barDragging)
        {
            IFC(ResetCacheBuffers());
        }
    }


    // Include the cache buffers if applicable.
    //
    // If we are mouse-dragging, all scroll movement is going to happen synchronously with ticks, so
    // any buffers we create is just adding wasted work that slows down our responsiveness
    if (allowCache)
    {
        BOOLEAN shouldProcessCache = TRUE;

        IFC(CanIncreaseCacheLength(&shouldProcessCache));

        // Increment the current cache buffer per side.  Cap it at the max cache buffer per side.
        if (shouldProcessCache)
        {
            m_windowState.currentCacheBufferPerSide += CacheBufferPerSideInflationPixelDelta;
            m_windowState.currentCacheBufferPerSide = std::min(m_windowState.currentCacheBufferPerSide, maxCacheBufferPerSide);

            // The realization window might change and we might need to correct for estimation errors.
            // In this case, we should also shift the window by the correction amount.
            m_windowState.allowWindowShiftOnEstimationErrorCorrection = TRUE;    
        }

        switch(panningOrientation)
        {
        case xaml_controls::Orientation_Horizontal:
            bufferSize.Width = static_cast<FLOAT>(m_windowState.currentCacheBufferPerSide);
            break;

        case xaml_controls::Orientation_Vertical:
            bufferSize.Height = static_cast<FLOAT>(m_windowState.currentCacheBufferPerSide);
            break;

        default:
            ASSERT(FALSE);  
            break;
        }
    }


    // The realization window should always be large enough to accommodate a buffer on each end
    wf::Rect realizationWindow;
    realizationWindow.Width = m_windowState.GetVisibleWindow().Width + 2 * bufferSize.Width;
    realizationWindow.Height = m_windowState.GetVisibleWindow().Height + 2 * bufferSize.Height;

    // Put a buffer above/left of the visible window, but the realization window won't go negative
    realizationWindow.X = std::max(m_windowState.GetVisibleWindow().X - bufferSize.Width, 0.0f);
    realizationWindow.Y = std::max(m_windowState.GetVisibleWindow().Y - bufferSize.Height, 0.0f);

    // The realization window should not be bigger than the panel's extent.
    // Otherwise, we might end up with disconnected containers or headers in scenarios where
    // we scroll outside of the panel (e.g. to a large header/footer/padding).
    // We check to see if the last item is realized and make sure that the realized window dimensions include the realized item
    IFC(IsLastElementRealized(&isLastElementRealized, &panelExtent));
    if (isLastElementRealized)
    {
        // if panelExtent is smaller than the visibleWindowLength (width or height depending on orientation), we adjust it to be the visibleWindowLength
        // for the case when we are inserting items, we want to make sure that all items have a legitimate chance of being inside the visible window
        // otherwise, items that are inserted before the last item such that the last item is pushed outside the current visible window are not all visible
        // and the last item will not be visible as well
        panelExtent = std::max(panelExtent, static_cast<float>(visibleWindowLength));

        realizationWindow.*SizeFromRectInVirtualizingDirection() =
            std::min(realizationWindow.*SizeFromRectInVirtualizingDirection(),
            panelExtent);

        realizationWindow.*PointFromRectInVirtualizingDirection() =
            std::min(realizationWindow.*PointFromRectInVirtualizingDirection(),
            panelExtent - realizationWindow.*SizeFromRectInVirtualizingDirection());
    }

    m_windowState.SetRealizationWindow(realizationWindow);

    // We are finished building the realization window because we are allowing cache
    // and the cache is at its max.
    m_windowState.cachePotentialReached = m_windowState.currentCacheBufferPerSide >= maxCacheBufferPerSide;

    if (!m_windowState.cachePotentialReached || !allowCache)
    {
        // make sure we are registered with budget manager so we get an opportunity to fix this
        if (!m_isRegisteredForCallbacks)
        {
            ctl::ComPtr<BuildTreeService> spBuildTree;
            IFC(DXamlCore::GetCurrent()->GetBuildTreeService(spBuildTree));
            IFC(spBuildTree->RegisterWork(this));
        }
        ASSERT(m_isRegisteredForCallbacks);
    }
    
Cleanup:
    RRETURN(hr);
}

// infinity looses the piece of information that would allow wrapping controls (like textblock, wrapgrid, image) to properly size. 
// it does however convey that 'feel free to take as much as you need though'. We have decided to keep the constraint, but 
// have frameworkelement not clip in certain situations.
BOOLEAN ModernCollectionBasePanel::WantsScrollViewerToObscureAvailableSizeBasedOnScrollBarVisibility(_In_ xaml_controls::Orientation orientation)
{
    HRESULT hr = S_OK;

    xaml_controls::Orientation virtualizingDirection;

    IFC(m_spLayoutStrategy->GetVirtualizationDirection(&virtualizingDirection));

    return virtualizingDirection == orientation;

Cleanup:
    return true;    // default behavior
}

ModernCollectionBasePanel::WindowState::WindowState()
    : m_currentRealizationWindow()
    , m_visibleWindowFromScrollState()
    , m_visibleWindowCoerced(RectUtil::CreateEmptyRect())
    , validWindowCalculation(false)
    , m_lastScrollOffset()
    , m_lastZoomFactor(1.0f)
    , m_lastScrollWasMouseLargeClick(false)
    , m_lastScrollWasForward(false)
    , currentTickNumber(-1)
    , firstVisibleItemIndexOfPreviousTick(-1)
    , m_panningDirectionBase(xaml_controls::PanelScrollingDirection::PanelScrollingDirection_None)
    , currentCacheBufferPerSide(0.0)
    , allowWindowShiftOnEstimationErrorCorrection(FALSE)
    , m_estimationCorrectionPending(false)
{
    lastAvailableSize.Width = lastAvailableSize.Height = std::numeric_limits<FLOAT>::infinity();
}

// Returns the current adjusment resulting for the possible CoercedVisibleWindow
// Note : protected by apiset, called only for Vertical Orientation
DOUBLE ModernCollectionBasePanel::WindowState::GetCurrentAdjustmentY()
{
    if (RectUtil::GetIsEmpty(m_visibleWindowCoerced))
    {
        return 0.0;
    }
    else
    {
        return m_visibleWindowFromScrollState.Y - m_visibleWindowCoerced.Y;
    }
}

void ModernCollectionBasePanel::WindowState::ApplyAdjustment(_In_ const wf::Point& correction)
{
    m_currentRealizationWindow.X += correction.X;
    m_currentRealizationWindow.Y += correction.Y;

    // If we don't currently have a command, this action constitutes initiating one
    if (RectUtil::GetIsEmpty(m_visibleWindowCoerced))
    {
        m_visibleWindowCoerced = m_visibleWindowFromScrollState;
    }

    m_visibleWindowCoerced.X += correction.X;
    m_visibleWindowCoerced.Y += correction.Y;
}

const wf::Rect& ModernCollectionBasePanel::WindowState::GetVisibleWindow() const
{
    if (RectUtil::GetIsEmpty(m_visibleWindowCoerced))
    {
        return m_visibleWindowFromScrollState;
    }
    else
    {
        return m_visibleWindowCoerced;
    }
}

void ModernCollectionBasePanel::WindowState::SetScrollStateVisibleWindow(
    _In_ const wf::Rect& newWindowFromScrollState,
    _Out_ BOOLEAN* pShouldInvalidate)
{
    // Get the current visible window, based on whether we have an override from a command
    const wf::Rect currentVisibleWindow = GetVisibleWindow();

    // If the window has changed, we'll invalidate
    if (!DoubleUtil::AreClose(currentVisibleWindow.X, newWindowFromScrollState.X) ||
        !DoubleUtil::AreClose(currentVisibleWindow.Y, newWindowFromScrollState.Y) ||
        !DoubleUtil::AreClose(currentVisibleWindow.Width, newWindowFromScrollState.Width) ||
        !DoubleUtil::AreClose(currentVisibleWindow.Height, newWindowFromScrollState.Height))
    {
        *pShouldInvalidate = TRUE;
    }
    else
    {
        *pShouldInvalidate = FALSE;
    }

    m_visibleWindowFromScrollState = newWindowFromScrollState;
}

BOOLEAN ModernCollectionBasePanel::WindowState::HasCoercedWindow() const
{
    return !DirectUI::RectUtil::GetIsEmpty(m_visibleWindowCoerced);
}

void ModernCollectionBasePanel::WindowState::ClearCoercedWindow()
{
    m_visibleWindowCoerced = RectUtil::CreateEmptyRect();
}

HRESULT ModernCollectionBasePanel::WindowState::CorrectVisibleWindowForMouseLargeClick(_In_ ModernCollectionBasePanel* panel)
{
    if (m_lastScrollWasMouseLargeClick)
    {
        auto func = [this, panel](wf::Rect* pNewVisibleWindow)
        {
            *pNewVisibleWindow = GetVisibleWindow();

            FLOAT actualHeaderOutsideVisibleWindow = 0;
            IFC_RETURN(panel->GetRealHeaderExtentOutsideVisibleWindow(&actualHeaderOutsideVisibleWindow));
            pNewVisibleWindow->Y += actualHeaderOutsideVisibleWindow * (m_lastScrollWasForward ? -1 : 1);

            return S_OK;
        };

        m_command = func;
        m_lastScrollWasMouseLargeClick = false;
        IFC_RETURN(panel->InvalidateMeasure());
    }

    return S_OK;
}

// Clear our understanding of our parent ScrollViewer.
_Check_return_ HRESULT ModernCollectionBasePanel::DetachPanelComponents()
{
    HRESULT hr = S_OK;
    
    auto sv = m_wrScrollViewer.AsOrNull<IScrollViewer>();
    if (sv)
    {
        IFC(m_epScrollViewerViewChangingHandler.DetachEventHandler(sv.Get()));
        IFC(m_epScrollViewerViewChangedHandler.DetachEventHandler(sv.Get()));        
        IFC(m_epScrollViewerSizeChangedHandler.DetachEventHandler(sv.Get()));
    }

    IFC(RemoveStickyHeaders());

    m_wrScrollViewer.Reset();
    m_wrItemsPresenter.Reset();
        
Cleanup:
    RRETURN(hr);
}

// Called when our ScrollViewer's has changed. Update our current visible window.        
_Check_return_ HRESULT ModernCollectionBasePanel::OnScrollViewChanged(
    _In_ IInspectable* pSender,
    _In_ IScrollViewerViewChangedEventArgs* pArgs)
{
    HRESULT hr = S_OK;
    
    auto spScrollViewer = m_wrScrollViewer.AsOrNull<IScrollViewer>();
    if (spScrollViewer)
    {
        DOUBLE horizontalOffset = 0.0f;
        DOUBLE verticalOffset = 0.0f;
        FLOAT zoomFactor = m_windowState.m_lastZoomFactor;
        
        IFC(spScrollViewer->get_HorizontalOffset(&horizontalOffset));
        IFC(spScrollViewer->get_VerticalOffset(&verticalOffset));

        if (!spScrollViewer.Cast<ScrollViewer>()->IsInDirectManipulationZoom())
        {
            wf::Rect visibleWindow = {};
            BOOLEAN shouldInvalidate = FALSE;

            IFC(spScrollViewer->get_ZoomFactor(&zoomFactor));

            m_windowState.m_lastScrollOffset.X = static_cast<FLOAT>(horizontalOffset);
            m_windowState.m_lastScrollOffset.Y = static_cast<FLOAT>(verticalOffset);
            m_windowState.m_lastZoomFactor = zoomFactor;

            IFC(CalculateVisibleWindowFromScrollPoint(m_windowState.m_lastScrollOffset, m_windowState.m_lastZoomFactor, &visibleWindow));
            IFC(BeginTrackingOnViewportSizeChange(m_windowState.GetVisibleWindow(), visibleWindow));
            m_windowState.SetScrollStateVisibleWindow(visibleWindow, &shouldInvalidate);
            IFC(SetRealizationWindowFromVisibleWindow(FALSE /* allowCache */));
            if (shouldInvalidate)
            {
                // If the visible window starts before or at offset 0 in the virtualizing direction (e.g. looking at the header),
                // then we consider that the intent is to show the beginning of the list and realize starting from the first visible element.
                if (visibleWindow.*PointFromRectInVirtualizingDirection() > 0.0f)
                {
                    m_windowState.allowWindowShiftOnEstimationErrorCorrection = TRUE;
                }

                IFC(InvalidateMeasure());
            }
            else
            {
                // Protected by Apiset : TRUE only for Vertical Orientation + Headerplacement!=Left
                if (m_bUseStickyHeaders)
                {
                    wf::Size finalSize = { visibleWindow.Width, visibleWindow.Height };
                    IFC(UpdateStickyHeaders(finalSize));
                }
            }

            // Make sure that we don't setup transitions during manipulation.
            boolean isIntermediate;
            IFC(pArgs->get_IsIntermediate(&isIntermediate));
            IFC(put_IsIgnoringTransitions(isIntermediate));
        }
    }

Cleanup:
    RRETURN(hr);
}

// Called when our ScrollViewer's view changes. Update our current visible window.
_Check_return_ HRESULT ModernCollectionBasePanel::OnScrollViewChanging(
    _In_ IInspectable* pSender,
    _In_ IScrollViewerViewChangingEventArgs* pArgs)
{
    HRESULT hr = S_OK;
    auto spScrollViewer = m_wrScrollViewer.AsOrNull<IScrollViewer>();

    if (spScrollViewer)
    {
        ctl::ComPtr<IScrollViewerView> spIView;
        DOUBLE horizontalOffset = 0.0f;
        DOUBLE verticalOffset = 0.0f;
        FLOAT zoomFactor = m_windowState.m_lastZoomFactor;

        ScrollViewerViewChangingEventArgs* pArgsNoRef = static_cast<ScrollViewerViewChangingEventArgs*>(pArgs);
        IFC(pArgsNoRef->get_NextView(&spIView));

        IFC(spIView->get_HorizontalOffset(&horizontalOffset));
        IFC(spIView->get_VerticalOffset(&verticalOffset));

        if (!spScrollViewer.Cast<ScrollViewer>()->IsInDirectManipulationZoom())
        {
            wf::Rect visibleWindow = {};
            BOOLEAN shouldInvalidate = FALSE;
            DOUBLE viewportHeight = 0.0f;
            DOUBLE scrolledBy = 0.0f;
            bool scrolledByAViewport = false;

            m_windowState.m_lastScrollWasMouseLargeClick = false;

            IFC(spScrollViewer->get_ViewportHeight(&viewportHeight));

            // Did we scroll by a viewport and what direction did we scroll
            scrolledBy = DoubleUtil::Abs(m_windowState.m_lastScrollOffset.Y - verticalOffset);
            scrolledByAViewport = !!DoubleUtil::AreWithinTolerance(scrolledBy, viewportHeight, 1.0);

            // capture the scroll direction - forward or backward
            m_windowState.m_lastScrollWasForward = m_windowState.m_lastScrollOffset.Y < verticalOffset;
            

            IFC(spIView->get_ZoomFactor(&zoomFactor));

            m_windowState.m_lastScrollOffset.X = static_cast<FLOAT>(horizontalOffset);
            m_windowState.m_lastScrollOffset.Y = static_cast<FLOAT>(verticalOffset);
            m_windowState.m_lastZoomFactor = zoomFactor;

            IFC(CalculateVisibleWindowFromScrollPoint(m_windowState.m_lastScrollOffset, m_windowState.m_lastZoomFactor, &visibleWindow));
            IFC(BeginTrackingOnViewportSizeChange(m_windowState.GetVisibleWindow(), visibleWindow));
            UpdateViewportOffsetDelta(m_windowState.GetVisibleWindow(), visibleWindow);
            m_windowState.SetScrollStateVisibleWindow(visibleWindow, &shouldInvalidate);
            IFC(SetRealizationWindowFromVisibleWindow(FALSE /* allowCache */));
            if (shouldInvalidate)
            {
                // If the visible window starts before or at offset 0 in the virtualizing direction (e.g. looking at the header),
                // then we consider that the intent is to show the beginning of the list and realize starting from the first visible element.
                if (visibleWindow.*PointFromRectInVirtualizingDirection() > 0.0f)
                {
                    m_windowState.allowWindowShiftOnEstimationErrorCorrection = TRUE;
                }
                IFC(put_IsIgnoringTransitions(TRUE));
                IFC(InvalidateMeasure());

                // What we are trying to identify is that if this view change
                // was initiated by the user clicking on the vertical scrollbar 
                // large increase or decrease buttons. We need to do correction to
                // the visible window in this particular case when using sticky headers.
                // This is because the sticky header is occluding part of the visible 
                // window - hence when the user clicks on these buttons we end up 
                // skipping items if we do not account for the occluded part of the 
                // visible window.
                // Unfortunately - there is no easy way to figure out if it was a 
                // large increase/decrease which caused the view change without 
                // tightly coupling with the ScrollViewer's scollbar - instead 
                // we use the following check to get the same result.

                // Note: PageUp/Down and ScrollItemIntoView will not come into this
                // code path since the visible window is already set and shouldInvalidate 
                // will be false causing us to skip this code block.

                // we are using sticky headers (and)
                // we scrolled by a viewport [large increase/decrease causes that] (and)
                // we are not in direct manipulation (and)
                // we are not thumb dragging 
                if (m_bUseStickyHeaders && 
                    scrolledByAViewport &&
                    !spScrollViewer.Cast<ScrollViewer>()->IsInDirectManipulation())
                {
                    BOOLEAN isThumbDragging = FALSE;

                    IFC(spScrollViewer.Cast<ScrollViewer>()->IsThumbDragging(&isThumbDragging))
                    if (!isThumbDragging)
                    {
                        // we need to adjust the visible window and account
                        // for the part occluded by the sticky header 
                        m_windowState.m_lastScrollWasMouseLargeClick = true;
                    }
                }
            }
        }
    }

Cleanup:
    RRETURN(hr);
}


_Check_return_ HRESULT ModernCollectionBasePanel::OnScrollViewerSizeChanged(
    _In_ IInspectable *pSender,
    _In_ xaml::ISizeChangedEventArgs *pArgs)
{
    HRESULT hr = S_OK;
    auto spScrollViewer = m_wrScrollViewer.AsOrNull<IScrollViewer>();

    if (spScrollViewer)
    {
        wf::Rect ourVisibleWindow = { };
        IFC(CalculateVisibleWindowFromScrollPoint(m_windowState.m_lastScrollOffset, m_windowState.m_lastZoomFactor, &ourVisibleWindow));
        IFC(BeginTrackingOnViewportSizeChange(m_windowState.GetVisibleWindow(), ourVisibleWindow));
        m_windowState.SetScrollStateVisibleWindow(ourVisibleWindow);
        IFC(SetRealizationWindowFromVisibleWindow(FALSE /* allowCache */));

        IFC(InvalidateMeasure());
    }

Cleanup:
    RRETURN(hr);
}


// Walk up the parent chain and find our ScrollViewer.
// Attach its OnScrollViewChanging and ScrollViewChanged to our OnScrollViewChange method.
// Also attach to our ItemsPresenter.
_Check_return_ HRESULT ModernCollectionBasePanel::AttachPanelComponents()
{
    HRESULT hr = S_OK;

    // Walk up the parent chain and tally up all the scroll viewers
    ctl::ComPtr<IDependencyObject> spCurrentAsDO = this;
    ctl::ComPtr<IDependencyObject> spParentAsDO;

    while (spCurrentAsDO)
    {
        IFC(VisualTreeHelper::GetParentStatic(spCurrentAsDO.Get(), &spParentAsDO));

        if (spParentAsDO)
        {
            ctl::ComPtr<IScrollViewer> spNodeScrollViewer = spParentAsDO.AsOrNull<IScrollViewer>();
            if (spNodeScrollViewer)
            {
                // Bind event handlers to the correct methods, using a pass-through functor
                IFC(m_epScrollViewerViewChangingHandler.AttachEventHandler(spNodeScrollViewer.Cast<ScrollViewer>(), std::bind(&ModernCollectionBasePanel::OnScrollViewChanging, this, _1, _2)));
                IFC(m_epScrollViewerViewChangedHandler.AttachEventHandler(spNodeScrollViewer.Cast<ScrollViewer>(), std::bind(&ModernCollectionBasePanel::OnScrollViewChanged, this, _1, _2)));
                IFC(m_epScrollViewerSizeChangedHandler.AttachEventHandler(spNodeScrollViewer.Cast<ScrollViewer>(), std::bind(&ModernCollectionBasePanel::OnScrollViewerSizeChanged, this, _1, _2)));
            
                IFC(spNodeScrollViewer.AsWeak(&m_wrScrollViewer));
                
                break;
            }
            else if(ctl::is<IItemsPresenter>(spParentAsDO))
            {
                IFC(spParentAsDO.AsWeak(&m_wrItemsPresenter));
            }
        }
        spCurrentAsDO = spParentAsDO;
    }
    
    // At this point we won't have gotten any ViewChanging or ViewChanged events, so we need to get a window ourselves.
    IFC(PrimeVisibleWindow());
    IFC(SetRealizationWindowFromVisibleWindow(FALSE));
    IFC(InvalidateMeasure());

#if DBG
    IFC(VisualTreeHelper::GetParentStatic(this, &spParentAsDO));
    if (spParentAsDO)
    {
        ctl::ComPtr<IItemsPresenter> spParentAsItemsPresenter;

        spParentAsItemsPresenter = spParentAsDO.AsOrNull<IItemsPresenter>();
        if (!spParentAsItemsPresenter)
        {
            ctl::ComPtr<IScrollContentPresenter> spParentAsScrollContentPresenter;

            spParentAsScrollContentPresenter = spParentAsDO.AsOrNull<IScrollContentPresenter>();
            if (!spParentAsScrollContentPresenter)
            {
                // Unsupported case.  Right now ModernPanel only supports:
                //  1) ItemsControl, which is detected by checking if the parent is an ItemsPresenter
                //  2) Directly parenting under a ScrollViewer, as in the Hub case.
                //     If parent is not SCP in the Hub case, our transform and window detection code may be broken.
                ASSERT(FALSE);
            }
        }
    }
#endif

Cleanup:
    RRETURN(hr);
}

// Before we get any ScrollViewChanging or ScrollViewChanged events, we need to proactively get a window
// (instead of waiting for an event to tell us of a window).
// This method gets a window from the ScrollViewer's offset, if possible. If a ScrollViewer isn't
// available, we go with a default window.
_Check_return_ HRESULT ModernCollectionBasePanel::PrimeVisibleWindow(bool allowTrackingOnViewportSizeChange /* = false */)
{
    wf::Rect ourVisibleWindow = { };

    auto spScrollViewer = m_wrScrollViewer.AsOrNull<IScrollViewer>();
    if (spScrollViewer)
    {
        DOUBLE horizontalOffset = 0.0;
        DOUBLE verticalOffset = 0.0;
        FLOAT zoomFactor = 1.0f;
        
        IFC_RETURN(spScrollViewer->get_HorizontalOffset(&horizontalOffset));
        IFC_RETURN(spScrollViewer->get_VerticalOffset(&verticalOffset));
        IFC_RETURN(spScrollViewer.Cast<ScrollViewer>()->get_ZoomFactor(&zoomFactor));
        
        m_windowState.m_lastScrollOffset.X = static_cast<FLOAT>(horizontalOffset);
        m_windowState.m_lastScrollOffset.Y = static_cast<FLOAT>(verticalOffset);
        m_windowState.m_lastZoomFactor = zoomFactor;
        
        IFC_RETURN(CalculateVisibleWindowFromScrollPoint(m_windowState.m_lastScrollOffset, zoomFactor, &ourVisibleWindow));
    }
    else
    {
        
        // Haven't yet been attached to ScrollViewers. Should just bail out and use a predetermined window.
        // We'll default to 0 pixels size in the virtualization direction, and give it the panel's available size
        // in the non-virtualizing direction
        xaml_controls::Orientation panningOrientation;
        IFC_RETURN(m_spLayoutStrategy->GetVirtualizationDirection(&panningOrientation));

        switch(panningOrientation)
        {
        case xaml_controls::Orientation_Horizontal:
            ourVisibleWindow.Width = 0;
            ourVisibleWindow.Height = m_windowState.lastAvailableSize.Height;
            break;

        case xaml_controls::Orientation_Vertical:
            ourVisibleWindow.Width = m_windowState.lastAvailableSize.Width;
            ourVisibleWindow.Height = 0;
            break;

        default:
            ASSERT(FALSE);
            ourVisibleWindow.Width = 0;
            ourVisibleWindow.Height = 0;
            break;
        }
        
        m_windowState.validWindowCalculation = false;
    }

    if (allowTrackingOnViewportSizeChange)
    {
        IFC_RETURN(BeginTrackingOnViewportSizeChange(m_windowState.GetVisibleWindow(), ourVisibleWindow));
    }

    m_windowState.SetScrollStateVisibleWindow(ourVisibleWindow);

    return S_OK;
}

// Resets the current cache buffer to 0
_Check_return_ HRESULT ModernCollectionBasePanel::ResetCacheBuffers()
{
    HRESULT hr = S_OK;

    m_windowState.currentCacheBufferPerSide = 0.0;

    m_windowState.cachePotentialReached = false;

    // make sure we are registered with budget manager so we get an opportunity to fix this
    if (!m_isRegisteredForCallbacks)
    {
        ctl::ComPtr<BuildTreeService> spBuildTree;
        IFC(DXamlCore::GetCurrent()->GetBuildTreeService(spBuildTree));
        IFC(spBuildTree->RegisterWork(this));
    }
    ASSERT(m_isRegisteredForCallbacks);

Cleanup:
    RRETURN(hr);
}

// This method works in two stages:
// Stage 1: Determine if the new window is disconnected from either our headers or containers
//          and figure out the closest valid elements from which to estimate
// Stage 2: Calculate new anchor indices by estimating how many groups/items it would take for
//          us to get from the old realized elements to the new window
_Check_return_ HRESULT ModernCollectionBasePanel::DetectAndHandleDisconnectedView()
{
    HRESULT hr = S_OK;

    BOOLEAN windowDisconnectedFromContainers = FALSE;
    INT32 disconnectedReferenceItemIndex = -1;
    wf::Rect disconnectedReferenceItemRect;
    RelativePosition referenceItemPositionFromWindow;

    BOOLEAN windowDisconnectedFromHeaders = FALSE;
    INT32 disconnectedReferenceGroupIndex = -1;
    wf::Rect disconnectedReferenceGroupRect;
    RelativePosition referenceGroupPositionFromWindow;

    const wf::Rect windowToFill = m_windowState.GetRealizationWindow();

    // If we are tracking an element, the viewport will shift to follow it
    // and, consequentially, we can't end up with a disconnected view.
    if(m_viewportBehavior.isTracking)
    {
#if DBG
        wf::Rect futureRealizationWindow = GetRealizationWindowAfterViewportShift();
        referenceItemPositionFromWindow = GetReferenceDirectionFromWindow(
            m_viewportBehavior.elementBounds,
            futureRealizationWindow);

        ASSERT(referenceItemPositionFromWindow == RelativePosition::Inside);
#endif
        if (m_viewportBehavior.type == xaml_controls::ElementType_GroupHeader &&
            m_containerManager.GetValidContainerCount() > 0)
        {
            int startItemIndex;
            IFC(m_cacheManager.GetGroupInformationFromGroupIndex(
                m_viewportBehavior.index,
                &startItemIndex,
                nullptr /* pItemCountInGroup */));

            // We are tracking a group header, which means we are going to use it as an anchor during layout.
            // While generating forward or backward from our header anchor, it's very common
            // to iterate over the realized containers block. Thus, it's important that there is
            // no gap between our anchor header and the contiguous block of containers.
            // If we detect such gap, we will call RemoveMeasureLeftOvers to clear it.
            // If we don't, we would break the contiguousness invariant and crash during
            // the Generate run.
            
            const int firstValidContainerItemIndex = m_containerManager.GetItemIndexFromValidIndex(0);
            if (firstValidContainerItemIndex > startItemIndex)
            {
                // There is a gap after the anchor that we need to clear.
                CollectionIterator gapRemovalIterator(m_cacheManager);
                gapRemovalIterator.Init(firstValidContainerItemIndex, xaml_controls::ElementType_ItemContainer);
                IFC(RemoveMeasureLeftOvers(gapRemovalIterator, true /* goForward */));
            }
            else
            {
                const int lastValidContainerItemIndex = m_containerManager.GetItemIndexFromValidIndex(m_containerManager.GetValidContainerCount() - 1);
                if (lastValidContainerItemIndex < startItemIndex - 1)
                {
                    // There is a gap before the anchor that we need to clear.
                    CollectionIterator gapRemovalIterator(m_cacheManager);
                    gapRemovalIterator.Init(lastValidContainerItemIndex, xaml_controls::ElementType_ItemContainer);
                    IFC(RemoveMeasureLeftOvers(gapRemovalIterator, false /* goForward */));
                }
            }
        }

        goto Cleanup;
    }

    // Let's see if any of the items are still in view
    if (m_containerManager.GetValidContainerCount() > 0)
    {
        UIElement::VirtualizationInformation* pFirstContainerInfo;
        UIElement::VirtualizationInformation* pLastContainerInfo;
        ctl::ComPtr<IUIElement> spContainer;
        wf::Rect tempRect;

        IFC(m_containerManager.GetContainerAtValidIndex(0, &spContainer));
        pFirstContainerInfo = GetVirtualizationInformationFromElement(spContainer);

        IFC(m_containerManager.GetContainerAtValidIndex(m_containerManager.GetValidContainerCount()-1, &spContainer));
        pLastContainerInfo = GetVirtualizationInformationFromElement(spContainer);

        tempRect = pFirstContainerInfo->GetBounds();

        referenceItemPositionFromWindow = GetReferenceDirectionFromWindow(tempRect, windowToFill);
        if (referenceItemPositionFromWindow == RelativePosition::After)
        {
            // Our first item is now beyond the window
            windowDisconnectedFromContainers = TRUE;
            disconnectedReferenceItemIndex = m_containerManager.GetItemIndexFromValidIndex(0);
            disconnectedReferenceItemRect = tempRect;
        }

        if (!windowDisconnectedFromContainers && referenceItemPositionFromWindow == RelativePosition::Before)
        {
            // Check the last item
            tempRect = pLastContainerInfo->GetBounds();
            referenceItemPositionFromWindow = GetReferenceDirectionFromWindow(tempRect, windowToFill);
            if (referenceItemPositionFromWindow == RelativePosition::Before)
            {
                // Our last item is before the window
                windowDisconnectedFromContainers = TRUE;
                disconnectedReferenceItemIndex = m_containerManager.GetItemIndexFromValidIndex(m_containerManager.GetValidContainerCount()-1);
                disconnectedReferenceItemRect = tempRect;
            }
        }
    }
    else
    {
        if (m_cacheManager.GetTotalItemCount() > 0)
        {
            // We haven't realized any item containers, but they exist in the collection. Mark them as disconnected
            windowDisconnectedFromContainers = TRUE;

            wf::Point locationOfFirstElement;
            IFC(m_spLayoutStrategy->GetPositionOfFirstElement(&locationOfFirstElement));

            disconnectedReferenceItemIndex = 0;
            disconnectedReferenceItemRect.X = locationOfFirstElement.X;
            disconnectedReferenceItemRect.Y = locationOfFirstElement.Y;
            disconnectedReferenceItemRect.Width = 0;
            disconnectedReferenceItemRect.Height = 0;
            referenceItemPositionFromWindow = RelativePosition::Before;
        }
    }

    // Now let's see if our headers are disconnected
    if (m_containerManager.GetValidHeaderCount() > 0)
    {
        UIElement::VirtualizationInformation* pFirstHeaderInfo;
        UIElement::VirtualizationInformation* pLastHeaderInfo;
        ctl::ComPtr<IUIElement> spHeader;
        wf::Rect tempRect;

        IFC(m_containerManager.GetHeaderAtValidIndex(0, &spHeader));
        pFirstHeaderInfo = GetVirtualizationInformationFromElement(spHeader);

        IFC(m_containerManager.GetHeaderAtValidIndex(m_containerManager.GetValidHeaderCount()-1, &spHeader));
        pLastHeaderInfo = GetVirtualizationInformationFromElement(spHeader);

        tempRect = pFirstHeaderInfo->GetBounds();

        referenceGroupPositionFromWindow = GetReferenceDirectionFromWindow(tempRect, windowToFill);
        if (referenceGroupPositionFromWindow == RelativePosition::After)
        {
            // Our first item is now beyond the window
            windowDisconnectedFromHeaders = TRUE;
            disconnectedReferenceGroupIndex = m_containerManager.GetGroupIndexFromValidIndex(0);
            disconnectedReferenceGroupRect = tempRect;
        }

        if (!windowDisconnectedFromHeaders && referenceGroupPositionFromWindow == RelativePosition::Before)
        {
            // Check the last item
            tempRect = pLastHeaderInfo->GetBounds();
            referenceGroupPositionFromWindow = GetReferenceDirectionFromWindow(tempRect, windowToFill);
            if (referenceGroupPositionFromWindow == RelativePosition::Before)
            {
                // Our last item is before the window
                windowDisconnectedFromHeaders = TRUE;
                disconnectedReferenceGroupIndex = m_containerManager.GetGroupIndexFromValidIndex(m_containerManager.GetValidHeaderCount()-1);
                disconnectedReferenceGroupRect = tempRect;
            }
        }
    }
    else
    {
        if (m_cacheManager.GetTotalLayoutGroupCount() > 0)
        {
            // We haven't realized any headers, but groups exist in the collection.
            windowDisconnectedFromHeaders = TRUE;

            wf::Point locationOfFirstElement;
            IFC(m_spLayoutStrategy->GetPositionOfFirstElement(&locationOfFirstElement));

            disconnectedReferenceGroupIndex = 0;
            disconnectedReferenceGroupRect.X = locationOfFirstElement.X;
            disconnectedReferenceGroupRect.Y = locationOfFirstElement.Y;
            disconnectedReferenceGroupRect.Width = 0;
            disconnectedReferenceGroupRect.Height = 0;
            referenceGroupPositionFromWindow = RelativePosition::Before;
        }
    }

    // Now that we know if we've disconnected from our headers and/or items, let's decide what and where to anchor
    if (m_cacheManager.IsGrouping())
    {
        if (windowDisconnectedFromHeaders && windowDisconnectedFromContainers)
        {
            // Nothing overlaps the new window. Throw it all away and get an anchor group
            INT32 itemIndexAtReferenceHeader;
            INT32 itemCountInReferenceHeader;
            IFC(m_cacheManager.GetGroupInformationFromGroupIndex(disconnectedReferenceGroupIndex, &itemIndexAtReferenceHeader, &itemCountInReferenceHeader));

            EstimationReference headerRef = CreateDefaultEstimationReference(xaml_controls::ElementType_GroupHeader);
            headerRef.ElementIndex = disconnectedReferenceGroupIndex;
            headerRef.ElementBounds = disconnectedReferenceGroupRect;

            EstimationReference containerRef = CreateDefaultEstimationReference(xaml_controls::ElementType_ItemContainer);
            containerRef.ElementIndex = disconnectedReferenceItemIndex;
            containerRef.ElementBounds = disconnectedReferenceItemRect;

            IFC(RecycleAllContainersAndHeaders());

            IFC(GenerateAnchorsForWindow(windowToFill, &headerRef, &containerRef));
        }
        else if(windowDisconnectedFromHeaders)
        {
            if (m_containerManager.GetValidContainerCount() > 0)
            {
                // Only the headers are disconnected, but the containers still overlap the new window.
                // This could happen if we are moving around in a gigantic group.
                // There's nothing to do here. Just keep walking the containers.
                // GenerateForward/Backward will handle headers as they encounter the need
            }
            else
            {
                // We have no valid containers, but our headers went disconnected
                // We need to re-anchor a header
                INT32 itemIndexAtReferenceHeader;
                INT32 itemCountInReferenceHeader;
                IFC(m_cacheManager.GetGroupInformationFromGroupIndex(disconnectedReferenceGroupIndex, &itemIndexAtReferenceHeader, &itemCountInReferenceHeader));

                EstimationReference headerRef = CreateDefaultEstimationReference(xaml_controls::ElementType_GroupHeader);
                headerRef.ElementIndex = disconnectedReferenceGroupIndex;
                headerRef.ElementBounds = disconnectedReferenceGroupRect;

                IFC(RecycleAllContainersAndHeaders());

                IFC(GenerateAnchorsForWindow(windowToFill, &headerRef, nullptr));
            }
        }
        else if(windowDisconnectedFromContainers)
        {
            ASSERT(m_containerManager.GetValidHeaderCount() > 0);

            if (m_containerManager.GetValidContainerCount() > 0)
            {
                // The containers are all useless to us, but we have headers hitting the window.
                // The headers may be floating, but they may have unrealized items that are now relevant
                // Recycle all the existing containers and see if there is a nonempty group in the window
                // with which we can generate an appropriate anchor item index, so we don't have to generate
                // a ridiculous distance back to the window

                ctl::ComPtr<IUIElement> spHeader;
                EstimationReference containerRef = CreateDefaultEstimationReference(xaml_controls::ElementType_ItemContainer);
                containerRef.ElementIndex = disconnectedReferenceItemIndex;
                containerRef.ElementBounds = disconnectedReferenceItemRect;

                INT32 groupIndex;
                INT32 itemIndexInGroup;
                INT32 itemCountInGroup;
                IFC(m_cacheManager.GetGroupInformationFromItemIndex(disconnectedReferenceItemIndex, &groupIndex, &itemIndexInGroup, &itemCountInGroup));
            
                const INT validHeaderIndex = m_containerManager.GetValidHeaderIndexFromGroupIndex(groupIndex);
                ASSERT(m_containerManager.IsValidHeaderIndexWithinBounds(validHeaderIndex));
                IFC(m_containerManager.GetHeaderAtValidIndex(validHeaderIndex, &spHeader));

                EstimationReference headerRef = CreateDefaultEstimationReference(xaml_controls::ElementType_GroupHeader);
                headerRef.ElementIndex = groupIndex;
                headerRef.ElementBounds = GetBoundsFromElement(spHeader);

                IFC(RecycleAllContainersAndHeaders());

                IFC(GenerateAnchorsForWindow(windowToFill, &headerRef, &containerRef));
            }
            else
            {
                // We have no containers at all, but the headers aren't disconnected
                // Some of the headers may be far outside the window, so lets get the best candidate
                INT32 validHeaderIndex = 0;
                ctl::ComPtr<IUIElement> spCandidateHeader;
                wf::Rect candidateHeaderBounds;
                RelativePosition candidatePosition;
                BOOLEAN foundCandidate = FALSE;

                // Since the headers aren't disconnected, we know the first header must be either before or inside the window
                // So let's walk the headers until we reach the window or reach the end of our valid collection
                while (!foundCandidate && validHeaderIndex < m_containerManager.GetValidHeaderCount())
                {
                    IFC(m_containerManager.GetHeaderAtValidIndex(validHeaderIndex, &spCandidateHeader));
                    candidateHeaderBounds = GetBoundsFromElement(spCandidateHeader);
                    candidatePosition = GetReferenceDirectionFromWindow(candidateHeaderBounds, windowToFill);

                    switch(candidatePosition)
                    {
                    case RelativePosition::Inside:
                        // Bingo!
                        foundCandidate = TRUE;
                        break;

                    case RelativePosition::After:
                        // We went from a header that was before the window, to one that was after the window.
                        // The headers "straddle" the window. So, let's back up and use the one before the window,
                        // because that's the group whose items, if any, will hit the window
                        --validHeaderIndex;
                        foundCandidate = TRUE;
                        break;

                    case RelativePosition::Before:
                        // Haven't found it yet, keep looking
                        ++validHeaderIndex;
                        break;
                    }
                }

                if (!foundCandidate)
                {
                    // If we got here, we're wrong because the last header was before the window
                    // Therefore, headers should have shown up as disconnected
                    ASSERT(FALSE);
                    goto Cleanup;
                }

                // Now, we're only interested in recycling and anchoring if any of the "interesting" groups are nonempty
                INT32 firstItemIndexInGroup;
                INT32 itemCountInGroup;
                INT32 groupIndex;
                foundCandidate = FALSE;

                while (!foundCandidate && validHeaderIndex < m_containerManager.GetValidHeaderCount() && candidatePosition != RelativePosition::After)
                {
                    IFC(m_containerManager.GetHeaderAtValidIndex(validHeaderIndex, &spCandidateHeader));
                    if (spCandidateHeader)
                    {
                        candidateHeaderBounds = GetBoundsFromElement(spCandidateHeader);
                        candidatePosition = GetReferenceDirectionFromWindow(candidateHeaderBounds, windowToFill);

                        if (candidatePosition != RelativePosition::After)
                        {
                            // See if this is a nonempty group from which to anchor an item
                            groupIndex = m_containerManager.GetGroupIndexFromValidIndex(validHeaderIndex);
                            IFC(m_cacheManager.GetGroupInformationFromGroupIndex(groupIndex, &firstItemIndexInGroup, &itemCountInGroup));
                            if (itemCountInGroup > 0)
                            {
                                foundCandidate = TRUE;
                            }
                            else
                            {
                                // Keep looking...
                                ++validHeaderIndex;
                            }
                        }
                    }
                    else
                    {
                        // The header is not realized. Keep looking...
                        ++validHeaderIndex;
                    }
                }

                // We found a nonempty group that wasn't after the window, nor was it more than one header in front of the window
                // Looks like we finally found a good place to generate an anchor item
                if (foundCandidate)
                {
                    EstimationReference headerRef = CreateDefaultEstimationReference(xaml_controls::ElementType_GroupHeader);
                    headerRef.ElementIndex = m_containerManager.GetGroupIndexFromValidIndex(validHeaderIndex);
                    headerRef.ElementBounds = candidateHeaderBounds;
                    
                    IFC(RecycleAllContainersAndHeaders());

                    IFC(GenerateAnchorsForWindow(windowToFill, &headerRef, nullptr));
                }
            }
        }
        else
        {
            // Nothing is disconnected. Looks like there's nothing to do here
        }
    }
    else
    {
        if (windowDisconnectedFromContainers)
        {
            EstimationReference containerRef = CreateDefaultEstimationReference(xaml_controls::ElementType_ItemContainer);
            containerRef.ElementIndex = disconnectedReferenceItemIndex;
            containerRef.ElementBounds = disconnectedReferenceItemRect;

            IFC(RecycleAllContainersAndHeaders());

            IFC(GenerateAnchorsForWindow(windowToFill, nullptr, &containerRef));
        }
    }

Cleanup:
    RRETURN(hr);
}

// Given a window to fill with containers and headers, estimate
// the location of the anchor container and header (if any),
// call into ICG2 to get the container and header, set both of
// their bounds, and add to valid containers and headers.
_Check_return_ HRESULT ModernCollectionBasePanel::GenerateAnchorsForWindow(
    _In_ const wf::Rect& windowToFill,
    _In_opt_ const EstimationReference* headerReference,
    _In_opt_ const EstimationReference* containerReference)
{
    HRESULT hr = S_OK;

    // Ideally, we'll make these params optional on the strategy and pass the pointers through, but for now, we'll just have to
    // create default objects in lieu of nullptrs until codegen supports this behavior. But we'll do the translation
    // here to avoid inflicting this pain everywhere else
    EstimationReference resolvedHeaderReference = headerReference ? *headerReference : CreateDefaultEstimationReference(xaml_controls::ElementType_GroupHeader);
    EstimationReference resolvedContainerReference = containerReference ? *containerReference : CreateDefaultEstimationReference(xaml_controls::ElementType_ItemContainer);

    resolvedHeaderReference.ElementIndex = m_cacheManager.DataIndexToLayoutIndex(xaml_controls::ElementType_GroupHeader, resolvedHeaderReference.ElementIndex);

    // We should not be tracking any particular element in this case.
    if (m_viewportBehavior.isTracking)
    {
        m_viewportBehavior.Reset();
    }

    if (m_cacheManager.IsGrouping())
    {
        wf::Size desiredSize;
        wf::Rect headerRect;
        wf::Size measureSizeForHeader;
        INT32 targetLayoutGroupIndex;
        ctl::ComPtr<IUIElement> spNewHeader;
        LayoutReference refInfo = CreateDefaultLayoutReference();

        IFC(m_spLayoutStrategy->EstimateElementIndex(
            xaml_controls::ElementType_GroupHeader,
            resolvedHeaderReference,
            resolvedContainerReference,
            windowToFill,
            &headerRect,
            &targetLayoutGroupIndex));

        ASSERT(m_containerManager.GetValidHeaderCount() == 0);
        ASSERT(m_cacheManager.GetTotalLayoutGroupCount() > 0);

        // Constrain the index
        targetLayoutGroupIndex = std::max(0, targetLayoutGroupIndex);
        targetLayoutGroupIndex = std::min(targetLayoutGroupIndex, m_cacheManager.GetTotalLayoutGroupCount() - 1);

        INT32 targetGroupIndex = m_cacheManager.LayoutIndexToDataIndex(xaml_controls::ElementType_GroupHeader, targetLayoutGroupIndex);
        IFC(m_spLayoutStrategy->GetElementMeasureSize(xaml_controls::ElementType_GroupHeader, targetGroupIndex, windowToFill, &measureSizeForHeader));

        // Generate the anchor header
        IFC(m_icg2->GenerateHeaderAtGroupIndex(targetGroupIndex, &spNewHeader));
        IFC(m_containerManager.PlaceInValidHeaders(targetGroupIndex, spNewHeader));
        IFC(PrepareHeaderViaItemsHost(targetGroupIndex, measureSizeForHeader, spNewHeader));

        // Store its desired position
        UIElement::VirtualizationInformation* pHeaderInfo = GetVirtualizationInformationFromElement(spNewHeader);

        // Now, if we have a nonempty group, let's generate a suitable anchor item container
        INT32 firstItemInAnchorGroup;
        INT32 itemCountInAnchorGroup;
        IFC(m_cacheManager.GetGroupInformationFromGroupIndex(targetGroupIndex, &firstItemInAnchorGroup, &itemCountInAnchorGroup));
        if (itemCountInAnchorGroup > 0)
        {
            INT32 targetItemIndex;
            ctl::ComPtr<IUIElement> spNewContainer;
            wf::Rect containerRect;
            wf::Size measureSizeForContainer;

            EstimationReference headerEstimationInfo = CreateDefaultEstimationReference(xaml_controls::ElementType_GroupHeader);
            headerEstimationInfo.ElementIndex = m_cacheManager.DataIndexToLayoutIndex(xaml_controls::ElementType_GroupHeader, targetGroupIndex);
            headerEstimationInfo.ElementBounds = headerRect;

            IFC(m_spLayoutStrategy->EstimateElementIndex(
                xaml_controls::ElementType_ItemContainer,
                headerEstimationInfo,
                CreateDefaultEstimationReference(xaml_controls::ElementType_ItemContainer),
                windowToFill,
                &containerRect,
                &targetItemIndex));

            // Constrain the index
            targetItemIndex = std::max(firstItemInAnchorGroup, targetItemIndex);
            targetItemIndex = std::min(targetItemIndex, firstItemInAnchorGroup + itemCountInAnchorGroup - 1);

            IFC(m_spLayoutStrategy->GetElementMeasureSize(xaml_controls::ElementType_ItemContainer, targetItemIndex, windowToFill, &measureSizeForContainer));

            // Generate the anchor container
            IFC(m_icg2->GenerateContainerAtIndex(targetItemIndex, &spNewContainer));
            IFC(m_containerManager.PlaceInValidContainers(targetItemIndex, spNewContainer));
            IFC(PrepareContainerViaItemsHost(targetItemIndex, measureSizeForContainer, spNewContainer));

            // Measure it so we can get accurate ideas for placement
            IFC(spNewContainer->Measure(measureSizeForContainer));
            IFC(spNewContainer->get_DesiredSize(&desiredSize));

            refInfo.ReferenceBounds = containerRect;
            refInfo.ReferenceIsHeader = FALSE;
            refInfo.RelativeLocation = xaml_controls::ReferenceIdentity_Myself;
            IFC(m_spLayoutStrategy->GetElementBounds(
                xaml_controls::ElementType_ItemContainer,
                targetItemIndex, 
                desiredSize, 
                refInfo, 
                windowToFill, 
                &containerRect));

            // Store its desired position
            SetBoundsForElement(spNewContainer, containerRect);
            
            pHeaderInfo->SetBounds(headerRect);
        }
        else
        {
            pHeaderInfo->SetBounds(headerRect);
        }
        // Protected by Apiset : TRUE only for Vertical Orientation + Headerplacement!=Left
        if (m_bUseStickyHeaders)
        {
            IFC(ConfigureStickyHeader(spNewHeader, targetGroupIndex, headerRect));
        }
    }
    else
    {
        wf::Rect containerRect;
        wf::Size measureSize;
        wf::Size desiredSize;
        INT32 targetItemIndex;
        ctl::ComPtr<IUIElement> spNewContainer;
        LayoutReference refInfo = CreateDefaultLayoutReference();

        IFC(m_spLayoutStrategy->EstimateElementIndex(
            xaml_controls::ElementType_ItemContainer,
            CreateDefaultEstimationReference(xaml_controls::ElementType_GroupHeader),
            resolvedContainerReference,
            windowToFill,
            &containerRect,
            &targetItemIndex));

        ASSERT(m_containerManager.GetValidContainerCount() == 0);
        ASSERT(m_cacheManager.GetTotalItemCount() > 0);

        // Constrain the index
        targetItemIndex = std::max(0, targetItemIndex);
        targetItemIndex = std::min(targetItemIndex, m_cacheManager.GetTotalItemCount() - 1);

        IFC(m_spLayoutStrategy->GetElementMeasureSize(xaml_controls::ElementType_ItemContainer, targetItemIndex, windowToFill, &measureSize));

        // Generate the anchor item
        IFC(m_icg2->GenerateContainerAtIndex(targetItemIndex, &spNewContainer));
        IFC(m_containerManager.PlaceInValidContainers(targetItemIndex, spNewContainer));
        IFC(PrepareContainerViaItemsHost(targetItemIndex, measureSize, spNewContainer)); 

        // Measure it so we can get accurate ideas for placement
        IFC(spNewContainer->Measure(measureSize));
        IFC(spNewContainer->get_DesiredSize(&desiredSize));

        refInfo.ReferenceBounds = containerRect;
        refInfo.ReferenceIsHeader = FALSE;
        refInfo.RelativeLocation = xaml_controls::ReferenceIdentity_Myself;
        IFC(m_spLayoutStrategy->GetElementBounds(
            xaml_controls::ElementType_ItemContainer,
            targetItemIndex, 
            desiredSize, 
            refInfo, 
            windowToFill, 
            &containerRect));

        // Store its desired position
        SetBoundsForElement(spNewContainer, containerRect);
    }

Cleanup:
    RRETURN(hr);
}

// Given a group index and its reference, estimate its position,
// call into ICG2 to get a header for the group, place the header
// in the valid headers, and sets the header's bounds to the estimated
// position.
_Check_return_ HRESULT ModernCollectionBasePanel::GenerateAnchorForGroup(
    _In_ const INT32 groupIndex,
    _In_opt_ const EstimationReference* headerReference,
    _In_opt_ const EstimationReference* containerReference)
{
    HRESULT hr = S_OK;

    ASSERT(m_cacheManager.IsGrouping());

    ctl::ComPtr<IUIElement> spNewHeader;
    wf::Rect headerRect;
    wf::Size measureSize;

    // Ideally, we'll make these params optional on the strategy and pass the pointers through, but for now, we'll just have to
    // create default objects in lieu of nullptrs until codegen supports this behavior. But we'll do the translation
    // here to avoid inflicting this pain everywhere else
    EstimationReference resolvedHeaderReference = headerReference ? *headerReference : CreateDefaultEstimationReference(xaml_controls::ElementType_GroupHeader);
    EstimationReference resolvedContainerReference = containerReference ? *containerReference : CreateDefaultEstimationReference(xaml_controls::ElementType_ItemContainer);

    resolvedHeaderReference.ElementIndex = m_cacheManager.DataIndexToLayoutIndex(xaml_controls::ElementType_GroupHeader, resolvedHeaderReference.ElementIndex);

    IFC(m_spLayoutStrategy->GetElementMeasureSize(xaml_controls::ElementType_GroupHeader, groupIndex, m_windowState.GetRealizationWindow(), &measureSize));

    IFC(m_spLayoutStrategy->EstimateElementBounds(
        xaml_controls::ElementType_GroupHeader,
        m_cacheManager.DataIndexToLayoutIndex(xaml_controls::ElementType_GroupHeader, groupIndex),
        resolvedHeaderReference,
        resolvedContainerReference,
        m_windowState.GetRealizationWindow(),
        &headerRect));

    ASSERT(m_containerManager.GetValidHeaderCount() == 0);
    ASSERT(m_cacheManager.GetTotalGroupCount() > 0);

    // Generate the anchor header
    IFC(m_icg2->GenerateHeaderAtGroupIndex(groupIndex, &spNewHeader));
    IFC(m_containerManager.PlaceInValidHeaders(groupIndex, spNewHeader));
    IFC(PrepareHeaderViaItemsHost(groupIndex, measureSize, spNewHeader));

    // Store its desired position
    SetBoundsForElement(spNewHeader, headerRect);

    // Validate that the anchor didn't overestimate and land outside the valid bounds
    IFC(CorrectForEstimationErrors(false /* adjustWindow */));

    // Protected by Apiset : TRUE only for Vertical Orientation + Headerplacement!=Left
    if (m_bUseStickyHeaders)
    {
        IFC(ConfigureStickyHeader(spNewHeader, groupIndex, headerRect));
    }

Cleanup:
    RRETURN(hr);
}

// Given an item index and its references, estimate its position,
// call into ICG2 to get a container, place the container
// in the valid containers, and sets the container's bounds to the estimated
// position.        
_Check_return_ HRESULT ModernCollectionBasePanel::GenerateAnchorForItem(
    _In_ const INT32 itemIndex,
    _In_opt_ const EstimationReference* headerReference,
    _In_opt_ const EstimationReference* containerReference)
{
    HRESULT hr = S_OK;

    ctl::ComPtr<IUIElement> spNewContainer;
    wf::Rect containerRect;
    wf::Size measureSize;

    // Ideally, we'll make these params optional on the strategy and pass the pointers through, but for now, we'll just have to
    // create default objects in lieu of nullptrs until codegen supports this behavior. But we'll do the translation
    // here to avoid inflicting this pain everywhere else
    EstimationReference resolvedHeaderReference = headerReference ? *headerReference : CreateDefaultEstimationReference(xaml_controls::ElementType_GroupHeader);
    EstimationReference resolvedContainerReference = containerReference ? *containerReference : CreateDefaultEstimationReference(xaml_controls::ElementType_ItemContainer);

    resolvedHeaderReference.ElementIndex = m_cacheManager.DataIndexToLayoutIndex(xaml_controls::ElementType_GroupHeader, resolvedHeaderReference.ElementIndex);

    IFC(m_spLayoutStrategy->GetElementMeasureSize(xaml_controls::ElementType_ItemContainer, itemIndex, m_windowState.GetRealizationWindow(), &measureSize));

    IFC(m_spLayoutStrategy->EstimateElementBounds(
        xaml_controls::ElementType_ItemContainer,
        itemIndex,
        resolvedHeaderReference,
        resolvedContainerReference,
        m_windowState.GetRealizationWindow(),
        &containerRect));

    ASSERT(m_containerManager.GetValidContainerCount() == 0);
    ASSERT(m_cacheManager.GetTotalItemCount() > 0);

    // Generate the anchor container
    IFC(m_icg2->GenerateContainerAtIndex(itemIndex, &spNewContainer));
    IFC(m_containerManager.PlaceInValidContainers(itemIndex, spNewContainer));
    IFC(PrepareContainerViaItemsHost(itemIndex, measureSize, spNewContainer));

    // Store its desired position
    SetBoundsForElement(spNewContainer, containerRect);

    // Validate that the anchor didn't overestimate and land outside the valid bounds
    IFC(CorrectForEstimationErrors(false /* adjustWindow */));

Cleanup:
    RRETURN(hr);
}

// Returns whether the last element of the data source was realized.
// If that's the case, it will also return the panel's extent.
_Check_return_ HRESULT ModernCollectionBasePanel::IsLastElementRealized(
    _Out_ BOOLEAN* pResult,
    _Out_ FLOAT* pPanelExtent)
{
    HRESULT hr = S_OK;
    BOOLEAN checkIfLastContainerIsRealized = !m_cacheManager.IsGrouping();
    FLOAT panelExtent = 0.0;

    *pResult = FALSE;

    // Note: we don't care much about the case where there is no items or groups in
    // the collection, so we will just return FALSE in that case.

    if (m_cacheManager.IsGrouping() && m_containerManager.GetValidHeaderCount() > 0)
    {
        ctl::ComPtr<IUIElement> spHeader;

        const INT32 lastGroupIndex = m_containerManager.GetGroupIndexFromValidIndex(m_containerManager.GetValidHeaderCount() - 1);
        IFC(m_containerManager.GetHeaderAtValidIndex(m_containerManager.GetValidHeaderCount() - 1, &spHeader));

        if (m_cacheManager.DataIndexToLayoutIndex(xaml_controls::ElementType_GroupHeader, lastGroupIndex) == m_cacheManager.GetTotalLayoutGroupCount() - 1 &&
            spHeader)
        {
            INT32 itemsInGroup = 0;
            wf::Rect bounds = {};
            IFC(m_cacheManager.GetGroupInformationFromGroupIndex(lastGroupIndex, nullptr, &itemsInGroup));

            if (itemsInGroup > 0)
            {
                checkIfLastContainerIsRealized = TRUE;
            }
            else
            {
                *pResult = TRUE;
            }

            bounds = GetBoundsFromElement(spHeader);
            panelExtent = bounds.*PointFromRectInVirtualizingDirection() + bounds.*SizeFromRectInVirtualizingDirection();
        }
    }
    
    if(checkIfLastContainerIsRealized && m_containerManager.GetValidContainerCount() > 0)
    {
        ctl::ComPtr<IUIElement> spContainer;
        wf::Rect bounds = {};

        const INT32 lastItemIndex = m_containerManager.GetItemIndexFromValidIndex(m_containerManager.GetValidContainerCount() - 1);
        IFC(m_containerManager.GetContainerAtValidIndex(m_containerManager.GetValidContainerCount() - 1, &spContainer));

        if (lastItemIndex == m_cacheManager.GetTotalItemCount() - 1 &&
            spContainer)
        {
            *pResult = TRUE;
        }

        bounds = GetBoundsFromElement(spContainer);
        panelExtent = std::max(panelExtent, bounds.*PointFromRectInVirtualizingDirection() + bounds.*SizeFromRectInVirtualizingDirection());
    }

    if(*pResult)
    {
        *pPanelExtent = panelExtent;
    }
    else
    {
        *pPanelExtent = 0.0;
    }

Cleanup:
    RRETURN(hr);
}

// Estimate the extent of our panel
// Since the last realized header and container represent the extent realized so far, we can
// get more accurate results by estimating the remaining unrealized extent and tacking it on to the end of the realized extent
_Check_return_ HRESULT ModernCollectionBasePanel::EstimatePanelExtent(_Out_ wf::Size* pReturnValue)
{
    HRESULT hr = S_OK;
    const wf::Rect windowConstraint = m_windowState.GetRealizationWindow();

    wf::Size extent = {};
    if (m_cacheManager.IsGrouping())
    {
        // Get the last header and/or container, then ask the layout strategy to estimate the remaining unrealized extent
        if (m_containerManager.GetValidHeaderCount() > 0)
        {
            ctl::ComPtr<IUIElement> spChild;
            EstimationReference headerEstimationRef = CreateDefaultEstimationReference(xaml_controls::ElementType_GroupHeader);

            IFC(m_containerManager.GetHeaderAtValidIndex(m_containerManager.GetValidHeaderCount()-1, &spChild));

            INT lastGroupIndex = m_containerManager.GetGroupIndexFromValidIndex(m_containerManager.GetValidHeaderCount()-1);

            INT firstItemIndexInLastGroup;
            INT itemsInLastGroup;
            IFC(m_cacheManager.GetGroupInformationFromGroupIndex(lastGroupIndex, &firstItemIndexInLastGroup, &itemsInLastGroup));

            headerEstimationRef.ElementIndex = m_cacheManager.DataIndexToLayoutIndex(xaml_controls::ElementType_GroupHeader, lastGroupIndex);
            headerEstimationRef.ElementBounds = GetBoundsFromElement(spChild);
            
            if (m_containerManager.GetValidContainerCount() > 0)
            {
                EstimationReference containerEstimationRef = CreateDefaultEstimationReference(xaml_controls::ElementType_ItemContainer);

                IFC(m_containerManager.GetContainerAtValidIndex(m_containerManager.GetValidContainerCount()-1, &spChild));
                INT lastItemIndex = m_containerManager.GetItemIndexFromValidIndex(m_containerManager.GetValidContainerCount() - 1);

                containerEstimationRef.ElementIndex = lastItemIndex;
                containerEstimationRef.ElementBounds = GetBoundsFromElement(spChild);

                IFC(m_spLayoutStrategy->EstimatePanelExtent(
                    headerEstimationRef,
                    containerEstimationRef,
                    windowConstraint,
                    &extent));
            }
            else
            {
                IFC(m_spLayoutStrategy->EstimatePanelExtent(
                    headerEstimationRef,
                    CreateDefaultEstimationReference(xaml_controls::ElementType_ItemContainer),
                    windowConstraint,
                    &extent));
            }
        }
        else
        {
            // If there are any groups in the collection we should have generated at least one header
            ASSERT(m_cacheManager.GetTotalLayoutGroupCount() == 0);
            extent.Width = 0;
            extent.Height = 0;
        }
    }
    else
    {
        // No headers, so just work with the last container
        if (m_containerManager.GetValidContainerCount() > 0)
        {
            ctl::ComPtr<IUIElement> spChild;

            IFC(m_containerManager.GetContainerAtValidIndex(m_containerManager.GetValidContainerCount()-1, &spChild));
            INT lastContainerItemIndex = m_containerManager.GetItemIndexFromValidIndex(m_containerManager.GetValidContainerCount() -1);
            
            EstimationReference containerEstimationRef = CreateDefaultEstimationReference(xaml_controls::ElementType_ItemContainer);
            containerEstimationRef.ElementIndex = lastContainerItemIndex;
            containerEstimationRef.ElementBounds = GetBoundsFromElement(spChild);

            IFC(m_spLayoutStrategy->EstimatePanelExtent(
                CreateDefaultEstimationReference(xaml_controls::ElementType_GroupHeader),
                containerEstimationRef,
                windowConstraint,
                &extent));
        }
        else
        {
            // If there are any items in the collection, we should have generated at least one container
            ASSERT(m_cacheManager.GetTotalItemCount() == 0);
            extent.Width = 0;
            extent.Height = 0;
        }
    }

    m_elementCountAtLastMeasure = m_cacheManager.IsGrouping() ? m_cacheManager.GetTotalLayoutGroupCount() : m_cacheManager.GetTotalItemCount();
    *pReturnValue = extent;

Cleanup:
    RRETURN(hr);
}

// Estimates are inevitably wrong, so... when they are wrong, we need to adjust our containers' placement
// on the panel so users can scroll exactly to the first/last elements, no more, no less. So, when we start
// getting close to the edge, we need to see if we are wrong and perform the shift.
_Check_return_ HRESULT ModernCollectionBasePanel::CorrectForEstimationErrors(bool adjustWindow /*= true*/)
{
    HRESULT hr = S_OK;
    float neededCorrection = 0.0f;

    if (m_cacheManager.IsGrouping() && m_containerManager.GetValidHeaderCount() > 0)
    {
        ctl::ComPtr<IUIElement> spElement;
        IFC(m_containerManager.GetHeaderAtValidIndex(0, &spElement));
        wf::Rect firstHeaderRect = GetBoundsFromElement(spElement);
        INT32 realizedGroupIndex = m_containerManager.GetGroupIndexFromValidIndex(0);
        INT32 realizedLayoutGroupIndex = m_cacheManager.DataIndexToLayoutIndex(xaml_controls::ElementType_GroupHeader, realizedGroupIndex);

        // Let's see who our first header is, and where its bounds are.
        // We only need to correct if realized the first header or if we realized before the
        // expected location of the first element.
        if (realizedLayoutGroupIndex == 0)
        {
            neededCorrection = - (firstHeaderRect.*PointFromRectInVirtualizingDirection());
        }
        else
        {
            wf::Point positionOfFirstElement;
            IFC(m_spLayoutStrategy->GetPositionOfFirstElement(&positionOfFirstElement));
            if (firstHeaderRect.*PointFromRectInVirtualizingDirection() < positionOfFirstElement.*PointFromPointInVirtualizingDirection() ||
                m_windowState.m_estimationCorrectionPending)
            {
                EstimationReference headerReference = CreateDefaultEstimationReference(xaml_controls::ElementType_GroupHeader);
                headerReference.ElementIndex = realizedLayoutGroupIndex;
                headerReference.ElementBounds = firstHeaderRect;
            
                EstimationReference containerReference = CreateDefaultEstimationReference(xaml_controls::ElementType_ItemContainer);            
                if (m_containerManager.GetValidContainerCount() > 0)
                {
                    IFC(m_containerManager.GetContainerAtValidIndex(0, &spElement));
                    containerReference.ElementIndex = m_containerManager.GetItemIndexFromValidIndex(0);
                    containerReference.ElementBounds = GetBoundsFromElement(spElement);
                }

                wf::Rect estimatedBoundsOfFirstUnrealizedHeader;
                // Ask the strategy how far we are off
                IFC(m_spLayoutStrategy->EstimateElementBounds(
                    xaml_controls::ElementType_GroupHeader,
                    0 /* elementIndex */,
                    headerReference,
                    containerReference,
                    m_windowState.GetRealizationWindow(),
                    &estimatedBoundsOfFirstUnrealizedHeader));

                neededCorrection = std::max(
                    -(firstHeaderRect.*PointFromRectInVirtualizingDirection()),
                    -(estimatedBoundsOfFirstUnrealizedHeader.*PointFromRectInVirtualizingDirection()));

                m_windowState.m_estimationCorrectionPending = false;
            }
        }
    }
    else if (m_containerManager.GetValidContainerCount() > 0)
    {
        // This shouldn't be possible if we're grouping
        ASSERT(!m_cacheManager.IsGrouping());

        // We only have containers, so find out where the first one is
        ctl::ComPtr<IUIElement> spContainer;
        INT32 realizedItemIndex = m_containerManager.GetItemIndexFromValidIndex(0);
        
        IFC(m_containerManager.GetContainerAtItemIndex(realizedItemIndex, &spContainer));
        wf::Rect firstContainerRect = GetBoundsFromElement(spContainer);

        if (realizedItemIndex == 0)
        {
            neededCorrection = -(firstContainerRect.*PointFromRectInVirtualizingDirection());
        }
        else
        {
            wf::Point positionOfFirstElement;
            IFC(m_spLayoutStrategy->GetPositionOfFirstElement(&positionOfFirstElement));

            if (firstContainerRect.*PointFromRectInVirtualizingDirection() < positionOfFirstElement.*PointFromPointInVirtualizingDirection() 
                || m_windowState.m_estimationCorrectionPending)
            {
                EstimationReference headerReference = CreateDefaultEstimationReference(xaml_controls::ElementType_GroupHeader);
                EstimationReference containerReference = CreateDefaultEstimationReference(xaml_controls::ElementType_ItemContainer);
                containerReference.ElementIndex = realizedItemIndex;
                containerReference.ElementBounds = firstContainerRect;

                wf::Rect estimatedBoundsOfFirstUnrealizedContainer;
                // Ask the strategy how far we are off
                IFC(m_spLayoutStrategy->EstimateElementBounds(
                    xaml_controls::ElementType_ItemContainer,
                    0 /* elementIndex */,
                    headerReference,
                    containerReference,
                    m_windowState.GetRealizationWindow(),
                    &estimatedBoundsOfFirstUnrealizedContainer));

                neededCorrection = std::max(
                    -(firstContainerRect.*PointFromRectInVirtualizingDirection()),
                    -(estimatedBoundsOfFirstUnrealizedContainer.*PointFromRectInVirtualizingDirection()));

                m_windowState.m_estimationCorrectionPending = false;
            }
        }
    }

    // If the needed correction is non-zero, run through all our elements and apply the correction to their bounds
    if (!DoubleUtil::IsZero(neededCorrection))
    {
        wf::Point neededCorrectionPoint = {};
        neededCorrectionPoint.*PointFromPointInVirtualizingDirection() = neededCorrection;

        for (INT32 validHeaderIndex = 0; validHeaderIndex < m_containerManager.GetValidHeaderCount(); ++validHeaderIndex)
        {
            ctl::ComPtr<IUIElement> spHeader;
            IFC(m_containerManager.GetHeaderAtValidIndex(validHeaderIndex, &spHeader));
            if (spHeader)
            {
                UIElement::VirtualizationInformation* pVirtualizationInfo = GetVirtualizationInformationFromElement(spHeader);
                wf::Rect bounds = pVirtualizationInfo->GetBounds();
                bounds.X += neededCorrectionPoint.X;
                bounds.Y += neededCorrectionPoint.Y;
                pVirtualizationInfo->SetBounds(bounds);
            }
        }
        for (INT32 validContainerIndex = 0; validContainerIndex < m_containerManager.GetValidContainerCount(); ++validContainerIndex)
        {
            ctl::ComPtr<IUIElement> spContainer;
            IFC(m_containerManager.GetContainerAtValidIndex(validContainerIndex, &spContainer));
            if (spContainer)
            {
                UIElement::VirtualizationInformation* pVirtualizationInfo = GetVirtualizationInformationFromElement(spContainer);
                wf::Rect bounds = pVirtualizationInfo->GetBounds();
                bounds.X += neededCorrectionPoint.X;
                bounds.Y += neededCorrectionPoint.Y;
                pVirtualizationInfo->SetBounds(bounds);
            }
        }

        // Apply this adjustment to the tracked bounds as well
        if (m_viewportBehavior.isTracking)
        {
            m_viewportBehavior.elementBounds.X += neededCorrectionPoint.X;
            m_viewportBehavior.elementBounds.Y += neededCorrectionPoint.Y;
        }

        if (adjustWindow)
        {
            if (m_windowState.allowWindowShiftOnEstimationErrorCorrection)
            {
                // Adjust our viewing rect
                m_windowState.ApplyAdjustment(neededCorrectionPoint);
            }
            else if (std::fabs(neededCorrectionPoint.X) > 1 || std::fabs(neededCorrectionPoint.Y) > 1)
            {
                // We've shuffled items to the side, but are not moving our window. This could leave a gap.
                // So, to eliminate it, we'll run another pass to make sure we continue generating to the edge
                if (!m_windowState.m_command)
                {
                    // Just create a no-op command that returns the currently visible window
                    auto func = [this](wf::Rect* pNewVisibleWindow)
                    {
                        *pNewVisibleWindow = this->m_windowState.GetVisibleWindow();
                        m_windowState.allowWindowShiftOnEstimationErrorCorrection = FALSE;
                        RRETURN(S_OK);
                    };
                    m_windowState.m_command = func;
                }
            }
        }
    }

    m_windowState.allowWindowShiftOnEstimationErrorCorrection = FALSE;

Cleanup:
    RRETURN(hr);
}

// Scrolls our ScrollViewer to the given area (synchronous).
_Check_return_ HRESULT ModernCollectionBasePanel::SetScrollViewerOffsetsTo(_In_ const wf::Rect& targetRect, _In_ bool animate, _Out_ bool* pIssued)
{
    HRESULT hr = S_OK;
    
    auto spScrollViewer = m_wrScrollViewer.AsOrNull<IScrollViewer>();

    *pIssued = false;

    if (spScrollViewer)
    {
        BOOLEAN handled = FALSE;
        float horizontalOffset = targetRect.X;
        float verticalOffset = targetRect.Y;

        ctl::ComPtr<IInspectable> spHorizontalOffset;
        ctl::ComPtr<IInspectable> spVerticalOffset;
        ctl::ComPtr<wf::IReference<DOUBLE>> spHorizontalOffsetReference;
        ctl::ComPtr<wf::IReference<DOUBLE>> spVerticalOffsetReference;
        xaml_controls::Orientation orientation;
        
        IFC(m_spLayoutStrategy->GetVirtualizationDirection(&orientation));
        
        // Account for our offset from the ItemsPresenter (i.e. listview.header)
        horizontalOffset += m_originFromItemsPresenter.X;
        verticalOffset += m_originFromItemsPresenter.Y;

        // Adjust for zoom factor
        // We do this after adjust for the layout slot, because the layout slot (list.header) is in our non-zoomed coordinates
        // but the ScrollViewer deals with zoomed coordinates

        // Until XAML converges on whether or not to reflow and remeasure based on zoom factors,
        // I'm going to ignore the zoom factor in the non-virtualizing direction.
        // All virtualization code will see the original non-zoomed window size in the regard
        switch (orientation)
        {
        case xaml_controls::Orientation_Horizontal:
            IFC(PropertyValue::CreateFromDouble(horizontalOffset * m_windowState.m_lastZoomFactor, &spHorizontalOffset));
            IFC(spHorizontalOffset.As(&spHorizontalOffsetReference));
            break;

        case xaml_controls::Orientation_Vertical:
            IFC(PropertyValue::CreateFromDouble(verticalOffset * m_windowState.m_lastZoomFactor, &spVerticalOffset));
            IFC(spVerticalOffset.As(&spVerticalOffsetReference));
            break;
        }

        IFC(spScrollViewer->ChangeViewWithOptionalAnimation(
            spHorizontalOffsetReference.Get(),
            spVerticalOffsetReference.Get(),
            nullptr  /*pZoomFactor*/,
            !animate /*disableAnimation*/,
            &handled));

        *pIssued = true;
    }
    
Cleanup:
    RRETURN(hr);
}

_Check_return_ HRESULT ModernCollectionBasePanel::ProcessOrientationChange()
{
    HRESULT hr = S_OK;
    ctl::ComPtr<IGeneratorHost> spIHost;

    IFC(m_cacheManager.GetItemsHost(&spIHost));

    // if the orientation changes but we are not even hooked up yet, we can fully ignore
    // it, since it will have no effect (measure will just automatically do the correct thing)
    if (spIHost)
    {
        // when orientation is changed, we expect the available size to change as well, so 
        // at the beginning of measure we will invalidate the validWindowCalculation probably
        m_windowState.validWindowCalculation = FALSE;

        // remove any commands, because they are not going to make sense anymore
        m_windowState.m_command = nullptr;

        // we want to keep the current item in the same location
        // but only at the next measure will we have a correct available size
        // so we need to create a command and defer this just like a regular ScrollItemIntoView
        {
            auto func = [this](wf::Rect* pNewVisibleWindow)
            {
                RRETURN(DoProcessOrientation(pNewVisibleWindow));
            };

            m_windowState.m_command = func;
        }
    } 

    // there is magic happening in the layout where some components will change 
    // dimensions to infinite. But since only we are dirty, in the first pass of layout
    // we will not get that code executed properly, ending up with infinity/infinity.
    // which is a terrible situation to be in for a virtualizing panel (it will de-virtualize everything).
    // Ultimately this will fix itself because the desiredsize of this panel will change which will change 
    // the desiredsize of the itemspresenter, which will invalidate the scrollcontentpresenter.

    // The solution is to simply be proactive and immediately invalidate the scrollcontentpresenter so that we
    // are assured of a great measure with valid values next time around.
        

    if (spIHost)
    {
        ctl::ComPtr<IDependencyObject> spHostAsDO = spIHost.AsOrNull<IDependencyObject>();
        ctl::ComPtr<IDependencyObject> spCurrentAsDO = this;

        while (spCurrentAsDO)
        {
            ctl::ComPtr<IDependencyObject> spParentAsDO;
            IFC(VisualTreeHelper::GetParentStatic(spCurrentAsDO.Get(), &spParentAsDO));

            if (spParentAsDO)
            {
                // anything in the tree is going to be a uielement.

                IFC(spParentAsDO.AsOrNull<IUIElement>()->InvalidateMeasure());
                
                // is this our itemscontrol?
                if (spParentAsDO == spHostAsDO)
                {
                    break;
                }
            }
            spCurrentAsDO = spParentAsDO;
        }
    }

    IFC(InvalidateMeasure());

Cleanup:
    RRETURN(hr);
}

// Triggers a synchronous layout if a command exists.
_Check_return_ HRESULT ModernCollectionBasePanel::FlushWindowStateCommand()
{
    if (m_windowState.m_command)
    {
        IFC_RETURN(InvalidateMeasure());
        IFC_RETURN(UpdateLayout());
    }
    return S_OK;
}

_Check_return_ HRESULT ModernCollectionBasePanel::ScrollRectIntoView(
    _In_ wf::Rect targetRect, 
    _In_ BOOLEAN forceSynchronous)
{
    HRESULT hr = S_OK;

    // If a command already exists, flush it and force a synchronous layout.
    IFC(FlushWindowStateCommand());

    {
        // This is a pretty simple command, so we'll just throw it in a lambda.
        // If this starts getting complicated, spin it off into a method
        auto func = [this, targetRect] (wf::Rect* pNewVisibleWindow)
        {
            // Place the window around the rect
            *pNewVisibleWindow = targetRect;
            
            // Cancels tracking if we were tracking.
            m_viewportBehavior.Reset();

            // this move can bring us into an edge that needs correction
            m_windowState.allowWindowShiftOnEstimationErrorCorrection = TRUE;
            RRETURN(S_OK);
        };
        m_windowState.m_command = func;
    }

    IFC(InvalidateMeasure());
    if (forceSynchronous)
    {
        IFC(UpdateLayout());
    }

Cleanup:
    RRETURN(hr);
}

_Check_return_ HRESULT ModernCollectionBasePanel::ScrollItemIntoView(
    _In_ UINT index,
    _In_ xaml_controls::ScrollIntoViewAlignment alignment,
    _In_ DOUBLE offset,
    _In_ BOOLEAN forceSynchronous,
    _In_ BOOLEAN animate)
{
    // If a command already exists, flush it and force a synchronous layout.
    IFC_RETURN(FlushWindowStateCommand());

    if (animate && m_containerManager.IsItemConnected(index))
    {
        ASSERT(m_windowState.m_command == nullptr);
        ASSERT(offset == 0);
        IFC_RETURN(DoScrollItemIntoView(index, alignment, 0 /*offset*/, TRUE /*animate*/, nullptr /*pComputedWindow*/));
    }
    else
    {
        {
            // We can't correctly prepare a new container outside of the visual tree.
            // The template won't get applied and we end up raising CCC with a null
            // args.ItemContainer.ContentTemplateRoot

            if (IsInLiveTree() &&
                IsItemsHostRegistered() &&
                !m_containerManager.HasFocusedContainer() && 
                !m_layoutInProgress)
            {
                // if nothing is focused, and index is connected, set that as focused container
                // if not connected, create a container.
                // we also do not want to do this during layout because we could end up preparing containers while 
                // layout is in the middle of preparing containers.
                ctl::ComPtr<IUIElement> spContainer;
                if (m_containerManager.IsItemConnected(index))
                {
                    IFC_RETURN(m_containerManager.GetContainerAtItemIndex(index, &spContainer));
                }
                else
                {
                    wf::Size measureSize;
                    HRESULT hr;

                    auto strongCache = m_cacheManager.CacheStrongRefs(&hr); // Releases when it goes out of scope at the end of method
                    IFC_RETURN(hr);

                    // try on both sides
                    IFC_RETURN(EnsureRecycleContainerCandidate(index, false /*fromFront*/));
                    IFC_RETURN(EnsureRecycleContainerCandidate(index, true /*fromFront*/));
                    if (m_containerManager.HasFocusedContainer())
                    {
                        // EnsureRecycleContainerCandidate could set the stored focused 
                        // container based on who actually has focus. We don't care what
                        // currently has focus - we want to set focus on the scrolled item.
                        IFC_RETURN(ReleaseStoredFocusedContainer());
                    }

                    IFC_RETURN(m_icg2->GenerateContainerAtIndex(index, &spContainer));
                    IFC_RETURN(m_containerManager.PlaceInGarbageRegionIfNotInChildren(spContainer));

                    IFC_RETURN(m_spLayoutStrategy->GetElementMeasureSize(
                        xaml_controls::ElementType_ItemContainer,
                        index,
                        m_windowState.GetRealizationWindow(),
                        &measureSize));
                    IFC_RETURN(PrepareContainerViaItemsHost(index, measureSize, spContainer));
                }

                IFC_RETURN(m_containerManager.SetFocusedContainer(index, spContainer));
                m_containerManager.m_wasLastScrollIntoViewForHeader = false;
            }

            if (m_containerManager.m_isScrollIntoViewInProgress)
            {
                // We got a ScrollIntoView while we are processing a ScrollIntoView.
                // This is not a scenario we support, let's leave a trace to help
                // us debug apps that do this.
                TraceGuardFailure(L"ScrollItemIntoViewReentrancy");
            }

            // We need to drop a command breadcrumb and run the virtualization pass to create the element
            auto func = [this, index, alignment, offset](wf::Rect* pNewVisibleWindow)
            {
                m_containerManager.m_isScrollIntoViewInProgress = true;
                IFC_RETURN(DoScrollItemIntoView(index, alignment, offset, FALSE /*animate*/, pNewVisibleWindow));
                m_containerManager.m_isScrollIntoViewPending = false;
                // Verify that it is still valid to keep the stored focused container.
                // It can be cleared if the container is in the valid range or it is still focused
                IFC_RETURN(VerifyStoredFocusedContainer());
                m_containerManager.m_isScrollIntoViewInProgress = false;
                return S_OK;
            };

            m_windowState.m_command = func;
            m_containerManager.m_isScrollIntoViewPending = true;
        }

        IFC_RETURN(InvalidateMeasure());
        if (forceSynchronous)
        {
            IFC_RETURN(UpdateLayout());
        }
    }

    return S_OK;
}

// Provide a focus candidate. If a scroll into view happens, the panel will 
// keep that as a focus candidate. The listview can ask for this candidate 
// when it needs to focus.
_Check_return_ HRESULT ModernCollectionBasePanel::GetFocusCandidate(
    _Out_ INT* index,
    _Out_ INT* groupIndex,
    _Out_ ctl::ComPtr<xaml::IUIElement>* pContainer)
{
    *index = -1;
    *groupIndex = -1;
    *pContainer = nullptr;

    if (m_containerManager.m_wasLastScrollIntoViewForHeader)
    {
        IFC_RETURN(m_containerManager.GetFocusedHeaderContainer(groupIndex, pContainer));
    }
    else
    {
        IFC_RETURN(m_containerManager.GetFocusedContainer(index, pContainer));
    }

    return S_OK;
}

_Check_return_ HRESULT ModernCollectionBasePanel::ScrollGroupHeaderIntoView(
    _In_ UINT index,
    _In_ xaml_controls::ScrollIntoViewAlignment alignment,
    _In_ DOUBLE offset,
    _In_ BOOLEAN forceSynchronous,
    _In_ BOOLEAN animate,
    _In_ UINT neighboringItemIndex) // Valid when 'animate' is True
{
    // If a command already exists, flush it and force a synchronous layout.
    IFC_RETURN(FlushWindowStateCommand());
    
    // Animations are only supported when moving to a realized group header and all items in between are also realized.
    if (animate && m_containerManager.IsGroupHeaderConnected(index, neighboringItemIndex))
    {
        ASSERT(m_windowState.m_command == nullptr);
        ASSERT(offset == 0);
        IFC_RETURN(DoScrollGroupHeaderIntoView(index, alignment, 0 /*offset*/, TRUE /*animate*/, nullptr /*pComputedWindow*/));
    }
    else
    {
        {
            // We can't correctly prepare a new container outside of the visual tree.
            // The template won't get applied and we end up raising CCC with a null
            // args.ItemContainer.ContentTemplateRoot

            if (IsInLiveTree() &&
                IsItemsHostRegistered() &&
                !m_containerManager.HasFocusedContainer() &&
                !m_layoutInProgress)
            {
                HRESULT hr;
                // Releases when it goes out of scope at the end of method
                auto strongCache = m_cacheManager.CacheStrongRefs(&hr);
                IFC_RETURN(hr);

                // if nothing is focused, and index is connected, set that as focused container
                // if not connected, create a container 
                ctl::ComPtr<IUIElement> spContainer;
                index = static_cast<UINT>(m_cacheManager.DataIndexToLayoutIndex(xaml_controls::ElementType_GroupHeader, index));
                index = static_cast<UINT>(m_cacheManager.LayoutIndexToDataIndex(xaml_controls::ElementType_GroupHeader, index));
                if (m_containerManager.IsGroupHeaderRealized(index))
                {
                    IFC_RETURN(m_containerManager.GetHeaderAtGroupIndex(index, &spContainer));
                }
                else
                {
                    IFC_RETURN(m_icg2->GenerateHeaderAtGroupIndex(index, &spContainer));
                    IFC_RETURN(m_containerManager.PlaceInGarbageRegionIfNotInChildren(spContainer));
                }

                IFC_RETURN(m_containerManager.SetFocusedHeaderContainer(index, spContainer));
                m_containerManager.m_wasLastScrollIntoViewForHeader = true;
            }

            if (m_containerManager.m_isScrollIntoViewInProgress)
            {
                // We got a ScrollIntoView while we are processing a ScrollIntoView.
                // This is not a scenario we support, let's leave a trace to help
                // us debug apps that do this.
                TraceGuardFailure(L"ScrollGroupHeaderIntoViewReentrancy");
            }

            // We need to drop a command breadcrumb and run the virtualization pass to create the element
            auto func = [this, index, alignment, offset](wf::Rect* pNewVisibleWindow)
            {
                m_containerManager.m_isScrollIntoViewInProgress = true;
                IFC_RETURN(DoScrollGroupHeaderIntoView(index, alignment, offset, FALSE /*animate*/, pNewVisibleWindow));
                // Reset the stored focused header once it is in view.
                IFC_RETURN(m_containerManager.ResetFocusedHeaderContainer(TRUE));
                m_containerManager.m_isScrollIntoViewInProgress = false;
                return S_OK;
            };

            m_windowState.m_command = func;
        }

        IFC_RETURN(InvalidateMeasure());
        if (forceSynchronous)
        {
            IFC_RETURN(UpdateLayout());
        }
    }

    return S_OK;
}

// Determine the window to show, based on either the user scrolling, or responding to a specific request
// such as key navigation or ScrollIntoView, and then generate the appropriate anchors for the new view
_Check_return_ HRESULT ModernCollectionBasePanel::DetermineWindowAndAnchors()
{
    wf::Rect newVisibleWindow = { -1.0f, -1.0f, -1.0f, -1.0f };
    if (m_windowState.m_command)
    {
        // Copy over the command on the stack in case it gets
        // overwritten during the call. This may prevent proper
        // execution of the rest of the original command.
        auto command = std::move(m_windowState.m_command);
        // If we have a pending window command, let's execute it
        IFC_RETURN(command(&newVisibleWindow));
    }
    else
    {
        IFC_RETURN(BeginTrackingOnRefresh(&newVisibleWindow));
    }

    if (!RectUtil::GetIsEmpty(newVisibleWindow))
    {
        m_windowState.SetCoercedVisibleWindow(newVisibleWindow);

        // We should let ItemsPresenter know about this new window in case we
        // need to load the footer.
        auto spItemsPresenter = m_wrItemsPresenter.AsOrNull<IItemsPresenter>();
        auto spScrollViewer = m_wrScrollViewer.AsOrNull<IScrollViewer>();

        if(spItemsPresenter && spScrollViewer)
        {
            DOUBLE viewportWidth = 0.0;
            DOUBLE viewportHeight = 0.0;

            IFC_RETURN(spScrollViewer->get_ViewportWidth(&viewportWidth));
            IFC_RETURN(spScrollViewer->get_ViewportHeight(&viewportHeight));

            wf::Rect futureViewportRect = {
                    /* X = */ (newVisibleWindow.X + m_originFromItemsPresenter.X) * m_windowState.m_lastZoomFactor,
                    /* Y = */ (newVisibleWindow.Y + m_originFromItemsPresenter.Y) * m_windowState.m_lastZoomFactor,
                    /* Width = */ static_cast<FLOAT>(viewportWidth),
                    /* Height = */ static_cast<FLOAT>(viewportHeight)
                };

            IFC_RETURN(spItemsPresenter.Cast<ItemsPresenter>()->DelayLoadFooter(&futureViewportRect, FALSE /* updateLayout */));
        }
    }
   
    IFC_RETURN(SetRealizationWindowFromVisibleWindow(TRUE));    
    IFC_RETURN(DetectAndHandleDisconnectedView());

    return S_OK;
}

_Check_return_ HRESULT ModernCollectionBasePanel::DoProcessOrientation(_Out_ wf::Rect* pComputedWindow)
{
    HRESULT hr = S_OK;

    IFC(BeginTrackingOnOrientationChange(pComputedWindow));

    // also, since we are not 'tracking' the first visible container, we should reset the cache
    IFC(ResetCacheBuffers());

Cleanup:
    RRETURN(hr);
}

// This method actually performs the heavy lifting of ScrollItemIntoView.
_Check_return_ HRESULT ModernCollectionBasePanel::DoScrollItemIntoView(
    _In_ INT32 itemIndex,
    _In_ xaml_controls::ScrollIntoViewAlignment alignment,
    _In_ DOUBLE offset,
    _In_ BOOLEAN animate,
    _Out_opt_ wf::Rect* pComputedWindow)
{
    HRESULT hr = S_OK;

    ctl::ComPtr<IUIElement> spContainer;
    wf::Rect containerBounds;
    const wf::Rect windowToFill = m_windowState.GetVisibleWindow();
    const wf::Size infiniteBounds = {std::numeric_limits<float>::infinity(), std::numeric_limits<float>::infinity()};
    wf::Rect computedWindow = RectUtil::CreateEmptyRect();
    wf::Rect headerBounds; // needed for sticky headers
    bool needReEstimation = false;

    // there could have been a change in the collection since the original scrollitemintoview command
    // was issued. Let's see if itemindex is still valid
    if (itemIndex < 0 || itemIndex >= m_cacheManager.GetTotalItemCount())
    {
        // When an animation is requested, the target item is expected to be connected so this branch should not be entered.
        ASSERT(!animate);

        // collection has changed, this command is no longer executable
        // we don't know how to calculate a window, so an empty rect will keep the current window unchanged
        goto Cleanup;
    }

    // If the item is already in the valid collection, just adjust its position
    if (m_containerManager.IsItemConnected(static_cast<UINT>(itemIndex)))
    {
        IFC(m_containerManager.GetContainerAtItemIndex(itemIndex, &spContainer));

        DOUBLE deltaY = 0.0;

        // This item's container is already generated, so let's just make sure it is visible at the desired alignment
        containerBounds = GetBoundsFromElement(spContainer);

        // Protected by Apiset : TRUE only for Vertical Orientation + Headerplacement!=Left
        if (m_bUseStickyHeaders)
        {
            // with sticky headers, we need more: the item has to be below its group's header too
            INT32 indexOfGroup = 0;
            ctl::ComPtr<IUIElement> spHeader;
            IFC(m_cacheManager.GetGroupInformationFromItemIndex(itemIndex, &indexOfGroup, nullptr, nullptr));
            INT validIndex = m_containerManager.GetValidHeaderIndexFromGroupIndex(indexOfGroup);
            IFC(m_containerManager.GetHeaderAtValidIndex(validIndex, &spHeader));
            headerBounds = GetBoundsFromElement(spHeader);
            deltaY = headerBounds.Height;
        }

        // If the container is completely inside the screen, and they request default alignment, we're done!
        if (alignment == xaml_controls::ScrollIntoViewAlignment_Default &&
            m_windowState.GetVisibleWindow().X <= containerBounds.X &&
            containerBounds.X + containerBounds.Width <= m_windowState.GetVisibleWindow().X + m_windowState.GetVisibleWindow().Width &&
            (m_windowState.GetVisibleWindow().Y + deltaY) <= containerBounds.Y &&
            containerBounds.Y + containerBounds.Height <= m_windowState.GetVisibleWindow().Y + m_windowState.GetVisibleWindow().Height)
        {
            // Nothing to do here, apparently.
            goto Cleanup;
        }
        
        if (!animate)
        {
            // Since the window may change, reset the cache buffers to only show what's going to be visible first.
            // The cache will rebuild asynchronously around the new visible window.
            IFC(ResetCacheBuffers());
        }
    }
    else
    {
        ASSERT(!animate);
        ASSERT(0 <= itemIndex && itemIndex < m_cacheManager.GetTotalItemCount());

        INT32 anchorItemIndexInGroup = 0;
        if (m_cacheManager.IsGrouping())
        {
            // See if our target group is already created.
            // If so, we'll just use its info as our reference data
            ctl::ComPtr<IUIElement> spGroupAnchor;

            INT32 targetGroupIndex;
            INT32 itemCountInTargetGroup;
            IFC(m_cacheManager.GetGroupInformationFromItemIndex(itemIndex, &targetGroupIndex, &anchorItemIndexInGroup, &itemCountInTargetGroup));
    
            EstimationReference headerEstimationInfo = CreateDefaultEstimationReference(xaml_controls::ElementType_GroupHeader);
            if (m_containerManager.GetValidHeaderCount() > 0)
            {
                INT32 lowGroupBound = m_containerManager.GetGroupIndexFromValidIndex(0);
                INT32 highGroupBound = m_containerManager.GetGroupIndexFromValidIndex(m_containerManager.GetValidHeaderCount() - 1);
                if (targetGroupIndex < lowGroupBound)
                {
                    headerEstimationInfo.ElementIndex = lowGroupBound;
                }
                else if(targetGroupIndex > highGroupBound)
                {
                    headerEstimationInfo.ElementIndex = highGroupBound;
                }
                else
                {
                    headerEstimationInfo.ElementIndex = targetGroupIndex;
                }

                ctl::ComPtr<IUIElement> spReferenceHeader;
                IFC(m_containerManager.GetHeaderAtGroupIndex(headerEstimationInfo.ElementIndex, &spReferenceHeader));

                // If our reference header turned out to be a sentinel, give up and use the first generated header
                if (!spReferenceHeader)
                {
                    headerEstimationInfo.ElementIndex = lowGroupBound;
                    IFC(m_containerManager.GetHeaderAtGroupIndex(lowGroupBound, &spReferenceHeader));
                }

                headerEstimationInfo.ElementBounds = GetBoundsFromElement(spReferenceHeader);
            }
            else
            {
                headerEstimationInfo.ElementIndex = 0;
                headerEstimationInfo.ElementBounds = {};
                
                // We re-estimate if there are no valid containers. Even if there is one valid
                // container we will not re-estimate. If we decide to be more agressive we can move this
                // outside the else clause.
                needReEstimation = true;
            }

            // Now we're ready to recycle and bring in the relevant group
            IFC(RecycleAllContainersAndHeaders());

            IFC(GenerateAnchorForGroup(targetGroupIndex, &headerEstimationInfo, nullptr));

            // Now, get that group anchor, and use it to estimate the location of the item
            IFC(m_containerManager.GetHeaderAtGroupIndex(targetGroupIndex, &spGroupAnchor));

            // Measure it so we get the correct alignment
            {
                wf::Size desiredSize;
                LayoutReference refInfo = CreateDefaultLayoutReference();
                refInfo.ReferenceIsHeader = TRUE;
                refInfo.RelativeLocation = xaml_controls::ReferenceIdentity_Myself;
                refInfo.ReferenceBounds = GetBoundsFromElement(spGroupAnchor);

                IFC(spGroupAnchor->Measure(infiniteBounds));
                IFC(spGroupAnchor->get_DesiredSize(&desiredSize));
                IFC(m_spLayoutStrategy->GetElementBounds(
                    xaml_controls::ElementType_GroupHeader,
                    m_cacheManager.DataIndexToLayoutIndex(xaml_controls::ElementType_GroupHeader, targetGroupIndex),
                    desiredSize, 
                    refInfo,
                    windowToFill,
                    &headerBounds));

                SetBoundsForElement(spGroupAnchor, headerBounds);
            }

            headerEstimationInfo.ElementIndex = targetGroupIndex;
            headerEstimationInfo.ElementBounds = GetBoundsFromElement(spGroupAnchor);

            IFC(GenerateAnchorForItem(itemIndex, &headerEstimationInfo, nullptr));
        }
        else // not grouping
        {
            anchorItemIndexInGroup = itemIndex;
            EstimationReference containerInfo = CreateDefaultEstimationReference(xaml_controls::ElementType_ItemContainer);
            if (m_containerManager.GetValidContainerCount() > 0)
            {
                INT32 firstItemIndex = m_containerManager.GetItemIndexFromValidIndex(0);
                INT32 lastItemIndex = m_containerManager.GetItemIndexFromValidIndex(m_containerManager.GetValidContainerCount()-1);
                INT32 referenceItemIndex;
                if (itemIndex < firstItemIndex)
                {
                    referenceItemIndex = firstItemIndex;
                }
                else if (lastItemIndex < itemIndex)
                {
                    referenceItemIndex = lastItemIndex;
                }
                else
                {
                    // The item we're trying to scroll must have been a sentinel.
                    // Rather than give up and hunt all over for a non-sentinel, just bail and use the the first realized item as the reference
                    referenceItemIndex = firstItemIndex;
                }
                ctl::ComPtr<IUIElement> spReferenceContainer;
                IFC(m_containerManager.GetContainerAtItemIndex(referenceItemIndex, &spReferenceContainer));

                containerInfo.ElementIndex = referenceItemIndex;
                containerInfo.ElementBounds = GetBoundsFromElement(spReferenceContainer);
            }
            else
            {
                containerInfo.ElementIndex = 0;
                containerInfo.ElementBounds = {};
                
                // We re-estimate if there are no valid containers. Even if there is one valid
                // container we will not re-estimate. If we decide to be more agressive we can move this
                // outside the else clause.
                needReEstimation = true;
            }

            IFC(RecycleAllContainersAndHeaders());
            
            IFC(GenerateAnchorForItem(itemIndex, nullptr, &containerInfo));
        }

        ASSERT(m_containerManager.IsValidContainerIndexWithinBounds(m_containerManager.GetValidContainerIndexFromItemIndex(itemIndex)));

        IFC(m_containerManager.GetContainerAtItemIndex(itemIndex, &spContainer));
        containerBounds = GetBoundsFromElement(spContainer);

        // Let's measure it to ensure proper alignment
        {
            wf::Size desiredSize;
            LayoutReference refInfo = CreateDefaultLayoutReference();
            refInfo.ReferenceIsHeader = FALSE;
            refInfo.RelativeLocation = xaml_controls::ReferenceIdentity_Myself;
            refInfo.ReferenceBounds = containerBounds;

            IFC(spContainer->Measure(infiniteBounds));
            IFC(spContainer->get_DesiredSize(&desiredSize));
            IFC(m_spLayoutStrategy->GetElementBounds(
                xaml_controls::ElementType_ItemContainer,
                itemIndex,
                desiredSize,
                refInfo,
                windowToFill,
                &containerBounds));

            SetBoundsForElement(spContainer, containerBounds);
        }

        // We performed a scrollinto view that could cause the estimation to be incorrect.
        // For example, we could have done a scroll into view right after setting the itemsource,
        // in which case we would have realized just the first item and then based that estimate
        // to estimate the anchor. In this case just using the first item size for estimation leads
        // to a pretty bad estimate. Setting this flag will cause us to re-evaluate the estimates
        // and correct in the next measure after we have generated more items - which means we have
        // seen more containers and hence have a better estimate. See bug 1405633 for more details
        // on this scenario.
        m_windowState.m_estimationCorrectionPending = needReEstimation;
    }

    // Protected by Apiset : TRUE only for Vertical Orientation + Headerplacement!=Left
    if (m_bUseStickyHeaders)
    {
        // Enlarge the window to include the header's height
        containerBounds.Y -= headerBounds.Height;
        containerBounds.Height += headerBounds.Height;
    }

    // Place the window around the anchor
    IFC(PlaceWindowAroundElementRect(m_windowState.GetVisibleWindow(), containerBounds, alignment, offset, &computedWindow));

    if (animate)
    {
        // Issue the animation request.
        bool issued = false;
        IFC(SetScrollViewerOffsetsTo(computedWindow, true /*animate*/, &issued));
    }
    else
    {
        // this move can bring us into an edge that needs correction
        m_windowState.allowWindowShiftOnEstimationErrorCorrection = TRUE;
    }

    // Cancels tracking if we were tracking.
    m_viewportBehavior.Reset();

Cleanup:
    if (pComputedWindow)
    {
        *pComputedWindow = computedWindow;
    }
    RRETURN(hr);
}

// This method actually performs the heavy lifting of ScrollGroupHeaderIntoView.
_Check_return_ HRESULT ModernCollectionBasePanel::DoScrollGroupHeaderIntoView(
    _In_ INT32 groupIndex,
    _In_ xaml_controls::ScrollIntoViewAlignment alignment,
    _In_ DOUBLE offset,
    _In_ BOOLEAN animate,
    _Out_opt_ wf::Rect* pComputedWindow)
{
    HRESULT hr = S_OK;

    ctl::ComPtr<IUIElement> spHeader;
    wf::Rect headerBounds;
    const wf::Rect windowToFill = m_windowState.GetVisibleWindow();
    const wf::Size infiniteBounds = {std::numeric_limits<float>::infinity(), std::numeric_limits<float>::infinity()};
    BOOLEAN headerConnected = FALSE;
    INT32 referenceGroupIndex = -1;
    bool needReEstimation = false;

    wf::Rect computedWindow = RectUtil::CreateEmptyRect();

    // deal with scrolling to an empty group
    groupIndex = m_cacheManager.DataIndexToLayoutIndex(xaml_controls::ElementType_GroupHeader, groupIndex);
    groupIndex = m_cacheManager.LayoutIndexToDataIndex(xaml_controls::ElementType_GroupHeader, groupIndex);

    // there could have been a change in the collection since the original scrollgroupintoview command
    // was issued. Let's see if groupindex is still valid
    if (groupIndex < 0 || groupIndex >= m_cacheManager.GetTotalGroupCount())
    {
        // When an animation is requested, the target group is expected to be connected so this branch should not be entered.
        ASSERT(!animate);

        // collection has changed, this command is no longer executable
        // we don't know how to calculate a window, so an empty rect will keep the current window unchanged
        goto Cleanup;
    }

    // Find out if we've already realized this header, and if not, who the closest header is
    if (m_containerManager.GetValidHeaderCount() > 0)
    {
        INT32 firstValidGroupIndex = m_containerManager.GetGroupIndexFromValidIndex(0);
        INT32 lastValidGroupIndex = m_containerManager.GetGroupIndexFromValidIndex(m_containerManager.GetValidHeaderCount()-1);

        if (groupIndex < firstValidGroupIndex)
        {
            headerConnected = FALSE;
            referenceGroupIndex = firstValidGroupIndex;
        }
        else if (lastValidGroupIndex < groupIndex)
        {
            headerConnected = FALSE;
            referenceGroupIndex = lastValidGroupIndex;
        }
        if (firstValidGroupIndex <= groupIndex && groupIndex <= lastValidGroupIndex)
        {
            IFC(m_containerManager.GetHeaderAtGroupIndex(groupIndex, &spHeader));
            if (spHeader)
            {
                headerConnected = TRUE;
                referenceGroupIndex = groupIndex;
            }
            else
            {
                headerConnected = FALSE;
                referenceGroupIndex = firstValidGroupIndex;
            }
        }
    }
    else
    {
        headerConnected = FALSE;
        referenceGroupIndex = 0;
    }

    // If the header is already in the valid collection, just adjust its position
    if (headerConnected)
    {
        // This group's header is already generated, so let's just make sure it is visible at the desired alignment
        headerBounds = GetBoundsFromElement(spHeader);

        // If the header is completely inside the screen, and they request default alignment, we're done!
        if (alignment == xaml_controls::ScrollIntoViewAlignment_Default &&
            m_windowState.GetVisibleWindow().X <= headerBounds.X &&
            headerBounds.X + headerBounds.Width <= m_windowState.GetVisibleWindow().X + m_windowState.GetVisibleWindow().Width &&
            m_windowState.GetVisibleWindow().Y <= headerBounds.Y &&
            headerBounds.Y + headerBounds.Height <= m_windowState.GetVisibleWindow().Y + m_windowState.GetVisibleWindow().Height)
        {
            // Nothing to do here, apparently.
            goto Cleanup;
        }

        if (!animate)
        {
            // Since the window may change, reset the cache buffers to only show what's going to be visible first.
            // The cache will rebuild asynchronously around the new visible window.
            IFC(ResetCacheBuffers());
        }
    }
    else
    {
        ASSERT(!animate);
        // Get a reference to estimate from and generate an anchor
        ASSERT(0 <= groupIndex && groupIndex < m_cacheManager.GetTotalGroupCount() || m_cacheManager.GetTotalGroupCount() == 0);
        ASSERT(m_cacheManager.IsGrouping());
        
        EstimationReference headerInfo = CreateDefaultEstimationReference(xaml_controls::ElementType_GroupHeader);
        if (m_containerManager.GetValidHeaderCount() > 0)
        {
            ctl::ComPtr<IUIElement> spReferenceHeader;

            ASSERT(m_containerManager.IsValidHeaderIndexWithinBounds(m_containerManager.GetValidHeaderIndexFromGroupIndex(referenceGroupIndex)));
            IFC(m_containerManager.GetHeaderAtValidIndex(m_containerManager.GetValidHeaderIndexFromGroupIndex(referenceGroupIndex), &spReferenceHeader));

            headerInfo.ElementIndex = referenceGroupIndex;
            headerInfo.ElementBounds = GetBoundsFromElement(spReferenceHeader);
        }
        else
        {
            headerInfo.ElementIndex = 0;
            headerInfo.ElementBounds = {};

            // we are disconnected and we need to estimate. In this case let's re-estimate
            // position in the panel once we have more items realized.
            needReEstimation = true;
        }

        IFC(RecycleAllContainersAndHeaders());

        IFC(GenerateAnchorForGroup(groupIndex, &headerInfo, nullptr));

        // Now that we've generated the anchor, let's grab it and measure it so we can accurately place the window
        ASSERT(m_containerManager.IsValidHeaderIndexWithinBounds(m_containerManager.GetValidHeaderIndexFromGroupIndex(groupIndex)));

        IFC(m_containerManager.GetHeaderAtGroupIndex(groupIndex, &spHeader));
        headerBounds = GetBoundsFromElement(spHeader);

        {
            wf::Size desiredSize;
            LayoutReference refInfo = CreateDefaultLayoutReference();
            refInfo.ReferenceIsHeader = TRUE;
            refInfo.RelativeLocation = xaml_controls::ReferenceIdentity_Myself;
            refInfo.ReferenceBounds = headerBounds;

            IFC(spHeader->Measure(infiniteBounds));
            IFC(spHeader->get_DesiredSize(&desiredSize));
            IFC(m_spLayoutStrategy->GetElementBounds(
                xaml_controls::ElementType_GroupHeader,
                m_cacheManager.DataIndexToLayoutIndex(xaml_controls::ElementType_GroupHeader, groupIndex),
                desiredSize,
                refInfo,
                windowToFill,
                &headerBounds));

            SetBoundsForElement(spHeader, headerBounds);
        }

        m_windowState.m_estimationCorrectionPending = needReEstimation;
    }

    // Place the window around the anchor
    IFC(PlaceWindowAroundElementRect(m_windowState.GetVisibleWindow(), headerBounds, alignment, offset, &computedWindow));

    if (animate)
    {
        // Issue the animation request.
        bool issued = false;
        IFC(SetScrollViewerOffsetsTo(computedWindow, true /*animate*/, &issued));
    }
    else
    {
        // this move can bring us into an edge that needs correction
        m_windowState.allowWindowShiftOnEstimationErrorCorrection = TRUE;
    }

    // Cancels tracking if we were tracking.
    m_viewportBehavior.Reset();

Cleanup:
    if (pComputedWindow)
    {
        *pComputedWindow = computedWindow;
    }
    RRETURN(hr);
}

// Helper method to place a window around an element's rect
_Check_return_ HRESULT ModernCollectionBasePanel::PlaceWindowAroundElementRect(
    _In_ const wf::Rect& sourceWindowRect,
    _In_ const wf::Rect& elementRectToAlignTo,
    _In_ xaml_controls::ScrollIntoViewAlignment alignment,
    _In_ DOUBLE offset,
    _Out_ wf::Rect* pResult)
{
    HRESULT hr = S_OK;

    wf::Rect result = sourceWindowRect;

    xaml_controls::Orientation orientation;
    IFC(m_spLayoutStrategy->GetVirtualizationDirection(&orientation));

    switch(orientation)
    {
    case xaml_controls::Orientation_Horizontal:
        switch(alignment)
        {
        case xaml_controls::ScrollIntoViewAlignment_Leading:
            result.X = elementRectToAlignTo.X;
            break;

        case xaml_controls::ScrollIntoViewAlignment_Default:
        default:
            // First see if the right edge needs to be brought in.
            // If not, or if this forces the left edge outside the window, bring it in
            if (sourceWindowRect.X + sourceWindowRect.Width < elementRectToAlignTo.X + elementRectToAlignTo.Width)
            {
                result.X = elementRectToAlignTo.X + elementRectToAlignTo.Width - sourceWindowRect.Width;
            }
            if (elementRectToAlignTo.X < result.X)
            {
                result.X = elementRectToAlignTo.X;
            }
            break;
        }

        result.X += static_cast<FLOAT>(offset);
        break;

    case xaml_controls::Orientation_Vertical:
        switch(alignment)
        {
        case xaml_controls::ScrollIntoViewAlignment_Leading:
            result.Y = elementRectToAlignTo.Y;
            break;

        case xaml_controls::ScrollIntoViewAlignment_Default:
        default:
            // First see if the bottom edge needs to be brought in.
            // If not, or if this forces the top edge outside the window, bring it in
            if (sourceWindowRect.Y + sourceWindowRect.Height < elementRectToAlignTo.Y + elementRectToAlignTo.Height)
            {
                result.Y = elementRectToAlignTo.Y + elementRectToAlignTo.Height - sourceWindowRect.Height;
            }
            if (elementRectToAlignTo.Y < result.Y)
            {
                result.Y = elementRectToAlignTo.Y;
            }
            break;
        }

        result.Y += static_cast<FLOAT>(offset);
        break;
    }

    *pResult = result;

Cleanup:
    RRETURN(hr);
}

// Recycles alls containers (and headers if grouping) and removes them from the cache manager.
_Check_return_ HRESULT
ModernCollectionBasePanel::RecycleAllContainersAndHeaders()
{
    HRESULT hr = S_OK;

    ctl::ComPtr<IGeneratorHost> spHost;
    IFC(m_cacheManager.GetItemsHost(&spHost));

    // Ensure we protect focused and pinned elements
    for (int typeIndex = 0; typeIndex < ElementType_Count; ++typeIndex)
    {
        const xaml_controls::ElementType type = static_cast<xaml_controls::ElementType>(typeIndex);
        const int validElementCount = m_containerManager.GetValidElementCount(type);

        for (int validIndex = 0; validIndex < validElementCount; ++validIndex)
        {
            const int dataIndex = m_containerManager.GetDataIndexFromValidIndex(type, validIndex);
            ctl::ComPtr<IUIElement> spElement;

            // If it's already pinned/focused, nothing to do
            if (!m_containerManager.GetIsIndexPinned(type, dataIndex) && !m_containerManager.GetIsIndexFocused(type, dataIndex))
            {
                IFC(m_containerManager.GetElementAtDataIndex(type, dataIndex, &spElement));
                if (spElement)
                {
                    // See if this element needs to be stored as focused
                    BOOLEAN hasFocus = FALSE;
                    IFC(m_cacheManager.IsFocusedChild(spElement.Get(), &hasFocus));

                    if (hasFocus && typeIndex == xaml_controls::ElementType_ItemContainer)
                    {
                        if (m_containerManager.HasFocusedContainer())
                        {
                            // this item is focused, but there is a stored focused container. There 
                            // was a scroll into view which stored the focused container but that was not
                            // focused, so dump the stored focused container. 
                            IFC(ReleaseStoredFocusedContainer());
                        }

                        IFC(m_containerManager.SetFocusedContainer(dataIndex, spElement));
                    }
                    else
                    {
                        BOOLEAN canRecycle = TRUE;
                        IFC(spHost->CanRecycleContainer(spElement.Cast<UIElement>(), &canRecycle));
                        if (!canRecycle)
                        {
                            IFC(m_containerManager.RegisterPinnedElement(type, dataIndex, spElement));
                        }
                    }
                }
            }
        }
    }

    if (m_cacheManager.IsGrouping())
    {
        IFC(m_icg2->RecycleAllHeaders());
        IFC(m_containerManager.RemoveAllValidHeaders());
    }

    IFC(m_icg2->RecycleAllContainers());
    IFC(m_containerManager.RemoveAllValidContainers());

    IFC(ResetCacheBuffers());

    // reset all the caches that we know for sure are going to be invalid now
    m_firstCacheIndexBase = -1;
    m_firstVisibleIndexBase = -1;
    m_lastVisibleIndexBase = -1;
    m_lastCacheIndexBase = -1;
    m_firstCacheGroupIndexBase = -1;
    m_firstVisibleGroupIndexBase = -1;
    m_lastVisibleGroupIndexBase = -1;
    m_lastCacheGroupIndexBase = -1;

Cleanup:
    RRETURN(hr);
}

// determines whether we are in a position to increase the cache
_Check_return_ HRESULT ModernCollectionBasePanel::CanIncreaseCacheLength(_Out_ BOOLEAN *pCanIncreaseCache)
{
    HRESULT hr = S_OK;
    
    auto spScrollViewer = m_wrScrollViewer.AsOrNull<IScrollViewer>();

    BOOLEAN shouldProcessCache = TRUE;

    // 1. Make sure we are not panning
    if (shouldProcessCache && spScrollViewer)
    {
        if (spScrollViewer.Cast<ScrollViewer>()->IsInManipulation())
        {
            shouldProcessCache = FALSE;
        }
        else
        {
            BOOLEAN barDragging = FALSE;
            IFC(spScrollViewer.Cast<ScrollViewer>()->IsThumbDragging(&barDragging));
            shouldProcessCache = !barDragging;
        }
    }

    // 2. Make sure we are not working on showing content
    if (shouldProcessCache)
    {
        ctl::ComPtr<IGeneratorHost> spHost;
        ctl::ComPtr<ITreeBuilder> spHostAsTreeBuilder;

        // Make sure queue of items to prepare in visible window is empty.
        IFC(m_cacheManager.GetItemsHost(&spHost));

        // we do that by looking at our host, the listview, which is the one driving
        // content setting
        spHostAsTreeBuilder = spHost.AsOrNull<ITreeBuilder>();
        if (spHostAsTreeBuilder)
        {
            BOOLEAN registeredForCallbacks = FALSE;
            IFC(spHostAsTreeBuilder->get_IsRegisteredForCallbacks(&registeredForCallbacks));

            shouldProcessCache = !registeredForCallbacks;
        }
    }
        
    // 3. Make sure we are not bogged down
    if (shouldProcessCache)
    {
        ctl::ComPtr<BudgetManager> spBudget;
        INT timeElapsedInMS = 0;

        IFC(DXamlCore::GetCurrent()->GetBudgetManager(spBudget));
        IFC(spBudget->GetElapsedMilliSecondsSinceLastUITick(&timeElapsedInMS));

        shouldProcessCache = timeElapsedInMS < PerformCacheInflationWhenTimeAvailable;
    }

    // 4. Go hardcore, we don't even want to process cache if there is _any_ input in the Message Queue!
    if (shouldProcessCache)
    {
        CCoreServices* pCore = nullptr;

        pCore = DXamlCore::GetCurrent()->GetHandle();
        if(pCore)
        {
            ITickableFrameScheduler *pFrameScheduler = pCore->GetBrowserHost()->GetFrameScheduler();
            if (pFrameScheduler)
            {
                // building up cache is regarded as a task that we only do when idle. 
                // if this is a high priority frame, it was requested by an invalidation outside of the
                // regular tick, presumably because input has occurred that has invalidated layout.
                // In that case, we do not want to build up cache.
                // But the ITreeBuilder implementation of the panel will have returned that it has more
                // work to do, so hopefully we will get a new chance soon.
                shouldProcessCache = !pFrameScheduler->IsHighPriority();
            }
        }
    }

    *pCanIncreaseCache = shouldProcessCache;

Cleanup:
    RRETURN(hr);
}

_Check_return_ HRESULT ModernCollectionBasePanel::IsBuildTreeSuspendedImpl(_Out_ BOOLEAN* pReturnValue)
{
    *pReturnValue = GetHandle()->IsCollapsed() || !GetHandle()->AreAllAncestorsVisible();
    return S_OK;
}

// the async version of doWork that is being called by NWDrawTree
_Check_return_ HRESULT ModernCollectionBasePanel::BuildTreeImpl(_Out_ BOOLEAN *pWorkLeft)
{
    HRESULT hr = S_OK;

    if (static_cast<CUIElement*>(GetHandle())->GetIsMeasureDirty())
    {
        // If we are measure dirty, we take ourselves out of the build tree service queue. 
        // During the next measure we will re-evaluate and register work if cache needs to be expanded.
        // Consider the case where the panel is collapsed while having work in the build tree 
        // service queue. We will come here repeatedly and say we have work left and invalidate measure
        // but measure will not do anything since we are collapsed, and we keep ticking.
        // This stops that from happening. 
        *pWorkLeft = false;
    }
    else
    {
        if (!m_windowState.cachePotentialReached)
        {
            // great, we are given an opportunity to work on the caches. First figure out if we want to.
            BOOLEAN shouldProcessCache = TRUE;

            IFC(CanIncreaseCacheLength(&shouldProcessCache));

            if (shouldProcessCache)
            {
                // he needs to be invalidated so he can actually increase the cache.
                // note how we only do this if we did absolutely no work
                IFC(InvalidateMeasure());

                // since after buildtree from the core we will call updatelayout, 
                // the panel will get an opportunity to increase the cache. 
                // The panel will have logic to either do the work or not.
                // Notice how we only did this invalidate if we did not do any other work for this
                // panel.
            }
        }

        // notice the approach here: 
        // We check whether we can build up cache right now
        //   If so: invalidate
        //   If not: do nothing
        // Regardless, we will return that we have work left for as long as work hasn't been finished,
        // and we are in the live tree.

        *pWorkLeft = !m_windowState.cachePotentialReached && IsInLiveTree();
    }

Cleanup:
#ifdef MCBP_DEBUG
    IGNOREHR(gps->DebugTrace(XCP_TRACE_OUTPUT_MSG /*traceType*/, L"MCBP_DEBUG[0x%p]: BuildTreeImpl. exit workLeft=%d", this, *pWorkLeft));
#endif
    RRETURN(hr);
}

_Check_return_ HRESULT
ModernCollectionBasePanel::CoerceWindowToExtent(_Inout_ wf::Rect& window)
{
    auto scrollViewer = m_wrScrollViewer.AsOrNull<IScrollViewer>();
    if (scrollViewer)
    {
        double extentWidth;
        double extentHeight;
        float zoomFactor;
        IFC_RETURN(scrollViewer->get_ExtentWidth(&extentWidth));
        IFC_RETURN(scrollViewer->get_ExtentHeight(&extentHeight));
        IFC_RETURN(scrollViewer->get_ZoomFactor(&zoomFactor));

        const wf::Rect extent = {
            -m_originFromItemsPresenter.X,
            -m_originFromItemsPresenter.Y,
            static_cast<float>(extentWidth / zoomFactor),
            static_cast<float>(extentHeight / zoomFactor)
        };

        // The window's size in the virtualizing direction cannot be bigger than the extent.
        window.*SizeFromRectInVirtualizingDirection() = std::min(window.*SizeFromRectInVirtualizingDirection(), extent.*SizeFromRectInVirtualizingDirection());

        // The window can't go beyond the beginning of the extent.
        if (window.*PointFromRectInVirtualizingDirection() < extent.*PointFromRectInVirtualizingDirection())
        {
            window.*PointFromRectInVirtualizingDirection() = extent.*PointFromRectInVirtualizingDirection();
        }
        else
        {
            // The window can't go beyond the end of the extent.
            const float bottomDelta =
                (window.*PointFromRectInVirtualizingDirection() + window.*SizeFromRectInVirtualizingDirection()) -
                (extent.*PointFromRectInVirtualizingDirection() + extent.*SizeFromRectInVirtualizingDirection());
            if (bottomDelta > 0.0f)
            {
                window.*PointFromRectInVirtualizingDirection() -= bottomDelta;
            }
        }
    }
    return S_OK;
}
