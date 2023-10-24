// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#include "precomp.h"
#include "ModernCollectionBasePanel.g.h"
#include "ScrollViewer.g.h"
#include "GroupStyle.g.h"
#include "SelectorItem.g.h"
#include "DispatcherTimer.g.h"
#include "IContainerRecyclingContext.g.h"
#include "XamlTraceLogging.h"
#include "IncrementalLoadingAdapter.h"

using namespace DirectUI;
using namespace DirectUISynonyms;
using namespace std::placeholders;
using xaml_controls::LayoutReference;
using xaml_controls::EstimationReference;
using namespace xaml_primitives;

// Work around disruptive max/min macros
#undef max
#undef min

//#define MCBP_DEBUG

const DOUBLE ModernCollectionBasePanel::CacheBufferPerSideInflationPixelDelta = 40;
const DOUBLE ModernCollectionBasePanel::WindowsDefaultCacheLength = 4.0;   // Total cache; not per-side
const DOUBLE ModernCollectionBasePanel::PhoneDefaultCacheLength = 2.0;   // Total cache; not per-side

// the time in ms needed for this frame to be under in order to even consider increasing
const INT ModernCollectionBasePanel::PerformCacheInflationWhenTimeAvailable = 40;

// These values should be tuned through performance tests
const UINT ModernCollectionBasePanel::QueueLengthBeforeFallback = 30;
const UINT ModernCollectionBasePanel::ValidContainersBeforeFallback = 30;

// Used to make elements bounds not overlay.
const FLOAT ModernCollectionBasePanel::EdgeOverlayDisambiguationDelta = 0.01f;

ModernCollectionBasePanel::ModernCollectionBasePanel()
    : m_windowState()
    , m_cacheManager(this)
    , m_containerManager(this)
    , m_transitionContextManager(this)
    , m_estimatedSize()
    , m_elementCountAtLastMeasure(0)
    , m_groupHeaderPlacement(xaml_primitives::GroupHeaderPlacement_Top)
    , m_originFromItemsPresenter()
    , m_bUseStickyHeaders(FALSE)
    , m_bLastItemRealized(FALSE)
    , m_currentHeaderHeight(0.0)
    , m_lastVisibleWindowClippingHeight(0.0f)
    , m_cachedPannableExtent(0.0)
    , m_enableCarouselPreload(false)
    , m_inCollectionChange(false)
    , m_layoutInProgress(false)
    , m_refreshPendingLayout(true)
{
    #ifdef DBG
    m_debugIsInTemporaryInvalidState = FALSE;
    #endif

    m_firstCacheIndexBase = -1;
    m_firstVisibleIndexBase = -1;
    m_lastVisibleIndexBase = -1;
    m_lastCacheIndexBase = -1;
    m_firstCacheGroupIndexBase = -1;
    m_firstVisibleGroupIndexBase = -1;
    m_lastVisibleGroupIndexBase = -1;
    m_lastCacheGroupIndexBase = -1;
    m_typeOfFirstVisibleElementBeforeOrientationChange = xaml_controls::ElementType_ItemContainer;
    m_indexOfFirstVisibleElementBeforeOrientationChange = -1;
    m_containerRequestedCount = 0;
    m_containerCreatedCount = 0;

    m_cacheLength = GetDefaultCacheLength();
}

ModernCollectionBasePanel::~ModernCollectionBasePanel()
{
    // Trace telemetry for container recycling effeciency metrics
    TraceLoggingWrite(
        g_hTraceProvider,
        "ContainerRecyclingLifetimeStats",
        TraceLoggingValue(m_containerRequestedCount, "ContainerRequestedCount"),
        TraceLoggingValue(m_containerCreatedCount, "ContainerCreatedCount"),
        TraceLoggingLevel(WINEVENT_LEVEL_LOG_ALWAYS),
        TelemetryPrivacyDataTag(PDT_ProductAndServicePerformance),
        TraceLoggingKeyword(MICROSOFT_KEYWORD_TELEMETRY));

    auto spObservableItemsSource = m_tpObservableItemsSource.GetSafeReference();
    if (spObservableItemsSource)
    {
        IGNOREHR(spObservableItemsSource->remove_VectorChanged(m_observableItemsSourceChangedToken));
    }

    VERIFYHR(DetachHandler(m_epGroupStyleChangedHandler, m_tpGroupStyle));

    static_cast<LayoutDataInfoProvider*>(m_spLayoutDataInfoProvider.Get())->Uninitialize();
}

_Check_return_ HRESULT ModernCollectionBasePanel::Initialize()
{
    IFC_RETURN(ModernCollectionBasePanelGenerated::Initialize());

    m_icg2 = this;

    // Bind event handlers to these methods, using a pass-through functor
    IFC_RETURN(m_epUnloadedHandler.AttachEventHandler(this, std::bind(&ModernCollectionBasePanel::OnPanelUnloaded, this, _1, _2)));

    {
        ctl::ComPtr<TrackerCollection<xaml::UIElement*>> unloadedElements;
        ctl::ComPtr<TrackerCollection<xaml::UIElement*>> unloadingElements;
        IFC_RETURN(ctl::make(&unloadedElements));
        IFC_RETURN(ctl::make(&unloadingElements));
        SetPtrValue(m_tpUnloadedElements, std::move(unloadedElements));
        SetPtrValue(m_unloadingElements, std::move(unloadingElements));
    }

    // Provides information about the data to the layout strategy.
    {
        ctl::ComPtr<LayoutDataInfoProvider> spLayoutDataInfoProvider;
        IFC_RETURN(ctl::make(static_cast<IModernCollectionBasePanel*>(this), &spLayoutDataInfoProvider));
        m_spLayoutDataInfoProvider = spLayoutDataInfoProvider;
    }

    return S_OK;
}

// Gets the default cache length
DOUBLE ModernCollectionBasePanel::GetDefaultCacheLength()
{
    return WindowsDefaultCacheLength;
}

_Check_return_ HRESULT ModernCollectionBasePanel::SetLayoutStrategyBase(_In_ const ctl::ComPtr<xaml_controls::ILayoutStrategy>& spLayoutStrategy)
{
    m_spLayoutStrategy = spLayoutStrategy;
    return m_spLayoutStrategy->SetLayoutDataInfoProvider(m_spLayoutDataInfoProvider.Get());
}

_Check_return_ HRESULT ModernCollectionBasePanel::GetLayoutStrategy(_Out_ ctl::ComPtr<xaml_controls::ILayoutStrategy>* pspLayoutStrategy)
{
    *pspLayoutStrategy = m_spLayoutStrategy;
    RRETURN(S_OK);
}

_Check_return_ HRESULT
ModernCollectionBasePanel::RegisterItemsHostImpl(_In_ IGeneratorHost* pHost)
{
    HRESULT hr = S_OK;

    ctl::ComPtr<wfc::IVector<IInspectable*>> spView;
    ctl::ComPtr<ICollectionView> spCollectionView;
    ctl::ComPtr<wfc::IObservableVector<IInspectable*>> spCollectionGroups;

    const ctl::ComPtr<IGeneratorHost> spHost = pHost;

#ifdef MCBP_DEBUG
    WCHAR szTrace[256];
    IFCEXPECT(swprintf_s(szTrace, 256, L"Registering itemshost") >= 0);
    Trace(szTrace);
#endif

    // register with the cachemanager for speeding up lookups
    IFC(m_cacheManager.RegisterItemsHost(spHost));

    IFC(spHost->get_View(&spView));

    if (IncrementalLoadingAdapter::SupportsIncrementalLoading(spView.Get()))
    {
        m_transitionContextManager.RegisterForIncrementalLoading();
    }

    // is this a grouped collection by any chance?
    IFC(spHost->get_CollectionView(&spCollectionView));

    if (m_tpObservableItemsSource)
    {
        IFC(m_tpObservableItemsSource->remove_VectorChanged(m_observableItemsSourceChangedToken));
    }

    if (spCollectionView)
    {
        IFC(spCollectionView->get_CollectionGroups(&spCollectionGroups));
        if (spCollectionGroups)
        {
            ctl::ComPtr<wfc::VectorChangedEventHandler<IInspectable*>> spItemCollectionVectorChangedHandler;
            spItemCollectionVectorChangedHandler.Attach(
                new ClassMemberEventHandler<
                ModernCollectionBasePanel,
                    IModernCollectionBasePanel,
                    wfc::VectorChangedEventHandler<IInspectable*>,
                    wfc::IObservableVector<IInspectable*>,
                    wfc::IVectorChangedEventArgs>(this, &ModernCollectionBasePanel::NotifyOfGroupsChanged));

#ifdef MCBP_DEBUG
            WCHAR szTrace[256];
            IFCEXPECT(swprintf_s(szTrace, 256, L"MCBP: Hooked up to the GroupChanged events") >= 0);
            Trace(szTrace);
#endif

            SetPtrValue(m_tpObservableItemsSource, spCollectionGroups);

            IFC(spCollectionGroups->add_VectorChanged(spItemCollectionVectorChangedHandler.Get(), &m_observableItemsSourceChangedToken));
        }
    }

    IFC(UpdateGroupStyle());

    // it's time to get some caching setup and letting our strategy know about grouping
    {
        // we might have switched sources. discard our cache and rebuild from scratch.
        m_cacheManager.ResetGroupCache();

        // re-evaluate
        auto strongCache = m_cacheManager.CacheStrongRefs(&hr); // Releases when it goes out of scope at the end of block
        IFC(hr);

        // let our strategy know what is going on
        IFC(ReevaluateGroupHeaderStrategy());
    }

    IFC(InvalidateMeasure());

Cleanup:
    RRETURN(hr);
}

// Pick up a new group style from the items host and update its event handlers
_Check_return_ HRESULT ModernCollectionBasePanel::UpdateGroupStyle()
{
    HRESULT hr = S_OK;

    ctl::ComPtr<IGroupStyle> spGroupStyle;
    ctl::ComPtr<IGeneratorHost> spItemsHost;

    IFC(m_cacheManager.GetItemsHost(&spItemsHost));
    if (spItemsHost)
    {
        BOOLEAN hidesIfEmpty = FALSE;

        IFC(spItemsHost->GetGroupStyle(nullptr, 0, &spGroupStyle));
        if (spGroupStyle)
        {
            IFC(spGroupStyle->get_HidesIfEmpty(&hidesIfEmpty));
        }
        m_cacheManager.PutHidesIfEmpty(hidesIfEmpty);
    }

    if (m_tpGroupStyle.Get() != spGroupStyle.Get())
    {
        IFC(DetachHandler(m_epGroupStyleChangedHandler, m_tpGroupStyle));

        SetPtrValue(m_tpGroupStyle, spGroupStyle);

        if (m_tpGroupStyle)
        {
            IFC(m_epGroupStyleChangedHandler.AttachEventHandler(
                m_tpGroupStyle.Cast<GroupStyle>(),
                std::bind(&ModernCollectionBasePanel::OnGroupStyleChanged, this, _1, _2)));
        }

        // We'll reset the caches, because if the group style has gone from null to valid, or vice versa, we need to start/stop
        // displaying group headers
        if ((!spGroupStyle && m_tpGroupStyle) || (spGroupStyle && !m_tpGroupStyle))
        {
            IFC(m_cacheManager.ResetGroupCache());
        }
        IFC(InvalidateMeasure());
    }

Cleanup:
    RRETURN(hr);
}

_Check_return_ HRESULT ModernCollectionBasePanel::OnGroupStyleChanged(
    _In_ xaml::IDependencyObject*,
    _In_ const CDependencyProperty* pDP)
{
    HRESULT hr = S_OK;

    switch (pDP->GetIndex())
    {
    case KnownPropertyIndex::GroupStyle_HidesIfEmpty:
        {
            // All we need here is to update the flag and invalidate if the state has changed
            BOOLEAN hidesIfEmpty = FALSE;
            if (m_tpGroupStyle)
            {
                IFC(m_tpGroupStyle->get_HidesIfEmpty(&hidesIfEmpty));
            }

            if (m_cacheManager.GetHidesIfEmpty() != hidesIfEmpty)
            {
                IFC(InvalidateMeasure());
            }

            m_cacheManager.PutHidesIfEmpty(hidesIfEmpty);
        }
        break;

    default:
        // If any other property has changed, we should start over from scratch
        IFC(Refresh());
        break;
    }

Cleanup:
    RRETURN(hr);
}

_Check_return_ HRESULT ModernCollectionBasePanel::DisconnectItemsHostImpl()
{
    HRESULT hr = S_OK;

    auto spObservableItemsSource = m_tpObservableItemsSource.GetSafeReference();
    if (spObservableItemsSource)
    {
        IFC(spObservableItemsSource->remove_VectorChanged(m_observableItemsSourceChangedToken));
    }

    // We call Refresh() here in particular for the case where Hub is retemplated to use a new panel.
    // We depend on Refresh() to unparent UIElement children, so they can be added to the new panel.
    // Refresh() cleans up other state as well to prepare the ItemsHost for using a new panel.
    IFC(Refresh());

    IFC(m_cacheManager.DisconnectItemsHost());

Cleanup:
    RRETURN(hr);
}

BOOLEAN ModernCollectionBasePanel::IsItemsHostRegistered()
{
    return m_cacheManager.IsItemsHostRegistered();
}

_Check_return_ HRESULT ModernCollectionBasePanel::SetGroupHeaderPlacement(_In_ xaml_primitives::GroupHeaderPlacement placement)
{
    m_groupHeaderPlacement = placement;
    RRETURN(ReevaluateGroupHeaderStrategy());
}

// Set the strategy's header strategy based on whether or not we're grouping.
_Check_return_ HRESULT ModernCollectionBasePanel::ReevaluateGroupHeaderStrategy()
{
    ctl::ComPtr<xaml_controls::ILayoutStrategy> spStrategy;
    xaml_controls::Orientation orientation;

    IFC_RETURN(GetLayoutStrategy(&spStrategy));
    IFC_RETURN(spStrategy->GetVirtualizationDirection(&orientation));

    // Only apiset + vertical Orientation + Headerplacement!=Left can set it to TRUE
    BOOL bOldUseStickyHeaders = m_bUseStickyHeaders;
    m_bUseStickyHeaders = FALSE;

    GroupHeaderStrategy headerStrategy = GroupHeaderStrategy::None;

    if (m_cacheManager.IsGrouping())
    {
        switch (orientation)
        {
        case xaml_controls::Orientation::Orientation_Horizontal:
            if (xaml_primitives::GroupHeaderPlacement_Left == m_groupHeaderPlacement)
            {
                headerStrategy = GroupHeaderStrategy::Inline;
            }
            else
            {
                headerStrategy = GroupHeaderStrategy::Parallel;
            }
            break;

        case xaml_controls::Orientation::Orientation_Vertical:
            if (xaml_primitives::GroupHeaderPlacement_Left == m_groupHeaderPlacement)
            {
                headerStrategy = GroupHeaderStrategy::Parallel;
            }
            else
            {
                headerStrategy = GroupHeaderStrategy::Inline;
                // get the BOOLEAN value from the public property AreStickyGroupHeadersEnabled
                IFC_RETURN(get_AreStickyGroupHeadersEnabledBase(&m_bUseStickyHeaders));
            }
            break;

        default:
            ASSERT(FALSE);
            break;
        }
    }

    IFC_RETURN(SetGroupHeaderStrategy(headerStrategy));

    // Only apiset + vertical Orientation + Headerplacement!=Left can set bOldUseStickyHeaders to TRUE
    if (bOldUseStickyHeaders && !m_bUseStickyHeaders)
    {
        IFC_RETURN(RemoveStickyHeaders());
    }
    else if (!bOldUseStickyHeaders && m_bUseStickyHeaders)
    {
        double width = 0.0;
        double height = 0.0;
        wf::Size size;

        IFC_RETURN(get_ActualWidth(&width));
        IFC_RETURN(get_ActualHeight(&height));

        size.Width = static_cast<float>(width);
        size.Height = static_cast<float>(height);

        IFC_RETURN(UpdateStickyHeaders(size));
    }

    return S_OK;
}

// determines the panning direction. This returns the stored panning direction that we were able to determine during arrange
// or none (if a new tick)
_Check_return_ HRESULT ModernCollectionBasePanel::get_PanningDirectionBaseImpl(_Out_ xaml_controls::PanelScrollingDirection* pValue)
{
    HRESULT hr = S_OK;
    XINT16 tick = 0;
    *pValue = xaml_controls::PanelScrollingDirection::PanelScrollingDirection_None;

    IFC(CoreImports::LayoutManager_GetLayoutTickForTransition(static_cast<CUIElement*>(this->GetHandle()), &tick));

    // the stored panning direction, calculated during arrange, is only valid when being queried in the same tick
    if (tick == m_windowState.currentTickNumber)
    {
        *pValue = m_windowState.m_panningDirectionBase;
    }

Cleanup:
    RRETURN(hr);
}
_Check_return_ HRESULT ModernCollectionBasePanel::put_PanningDirectionBaseImpl(_In_ xaml_controls::PanelScrollingDirection value)
{
    m_windowState.m_panningDirectionBase = value;
    RRETURN(S_OK);
}

IFACEMETHODIMP ModernCollectionBasePanel::MeasureOverride(
    _In_ wf::Size availableSize,
    _Out_ wf::Size* pReturnValue)
{
    m_layoutInProgress = true;

    auto guard = wil::scope_exit([this]()
    {
        m_layoutInProgress = false;
    });

    if (!IsItemsHostRegistered())
    {
        return S_OK;
    }

    // store the availablesize
    if (m_windowState.lastAvailableSize.Width != availableSize.Width ||
        m_windowState.lastAvailableSize.Height != availableSize.Height)
    {
        m_windowState.lastAvailableSize = availableSize;
        // available size has changed, we should recalculate
        m_windowState.validWindowCalculation = FALSE;
    }

    // Run our virtualization pass to generate, measure, and pre-arrange everything
    static const int maxVirtualizationPasses = 100;
    int virtualizationPassCount = 0;
    do
    {
        ++virtualizationPassCount;
        IFC_RETURN(RunVirtualization());
    } while (m_windowState.m_command != nullptr && virtualizationPassCount < maxVirtualizationPasses);

    // A pass may be restarted if focus work is occurring, but in general, this should only take one or two tries.
    // The bailout should never be needed, but it's there in case somebody breaks something horribly, so that we can fail more obviously
    // instead of making a developer diagnose this infinite loop.
    ASSERT(m_windowState.m_command == nullptr);
    m_windowState.m_command = nullptr;

    // Get the estimated panel extent and setup our return value
    IFC_RETURN(EstimatePanelExtent(&m_estimatedSize));
    *pReturnValue = m_estimatedSize;

    // Need to do one final step after computing the panel size
    IFC_RETURN(LayoutPreloadedItems());

    return S_OK;
}

// This method will detect the visible window, generate/recycle, measure, and pre-arrange
_Check_return_ HRESULT ModernCollectionBasePanel::RunVirtualization()
{
    HRESULT hr = S_OK;
    ctl::ComPtr<IScrollViewer> spScrollViewer = m_wrScrollViewer.AsOrNull<IScrollViewer>();

#ifdef DBG
    ASSERT(!m_debugIsInTemporaryInvalidState);
#endif

    {
        // sets up caches to make iterating over groups much faster, caches children collection, host, etc.
        auto strongCache = m_cacheManager.CacheStrongRefs(&hr); // Releases when it goes out of scope at the end of block
        IFC(hr);

        // When an initial measure comes in, we should ensure that we have a reasonable visible window set up, based on the available size given
        // If not, we'll ask for too little space in the nonvirtualizing direction, and may never get it back.
        // Window commands (ScrollIntoView) may come in before we've had a chance to get a valid window, so this is valid to run even if we have
        // a pending command.
        if (!spScrollViewer)
        {
            IFC(AttachPanelComponents());
        }
        else if (!m_windowState.validWindowCalculation)
        {
            IFC(PrimeVisibleWindow());
            IFC(SetRealizationWindowFromVisibleWindow(FALSE));
        }

        // Protected by Apiset
        // Sticky headers : TRUE only for Vertical Orientation + Headerplacement!=Left
        // MaintainViewprt : needs ItemsStackPanel AND m_itemsUpdatingScrollMode == ItemsUpdatingScrollMode_KeepItemsInView

        if (m_bUseStickyHeaders || IsMaintainViewportSupportedAndEnabled())
        {
            if (spScrollViewer)
            {
                // Update the size of the header once to that it will be correct for all header's creations
                IFC(spScrollViewer.Cast<ScrollViewer>()->get_HeaderHeight(&m_currentHeaderHeight));
            }
        }

        // make sure we are not in a state where we cannot perform layout.
        // we are not doing a FailFast if !IsLockedForLayout because
        // of bug 2863553 which breaks compat.
        if (IsLockedForLayout())
        {
            TraceGuardFailure(L"LayoutWhenLocked");
        }

        if (!m_containerManager.m_unlinkedFillers.empty())
        {
            // If we are in the middle of a group change,
            // we should stop right now instead of failing later
            IFCFAILFAST(E_FAIL);
        }

        // move unloaded elements from unloaded queue to the visual tree.
        IFC(ReloadUnloadedElements());

        // make sure that the focused container is still focused, otherwise move it to recyclequeue
        IFC(VerifyStoredFocusedContainer());

        // Ensure that any pinned containers still have a reason to be pinned. If not, unpin them.
        IFC(PrunePinnedContainers());

        // Always measure the special container (used for fixed size panels) if the strategy
        // asks for it
        IFC(MeasureSpecialElements());

        // Determine the window to show, based on either the user scrolling, or responding to a specific request
        // such as key navigation or ScrollIntoView, and then generate the appropriate anchors for the new view
        IFC(DetermineWindowAndAnchors());

        // measure all containers, regardless of window.
        // this allows us to understand how much space we currently take up, and adhere to the measure contract
        // of calling measure on all owned children
        IFC(RunGenerate());

        // No matter how hard we try, estimates are going to be wrong. That's why they're called estimates.
        // Here, we detect when they've caused us to deviate from a known constraint. Typically,
        // this means that if item/header 0 is not at pixel (0,0), or if some other element is at (0,0)
        // we'll shift our elements and window over to bring us back to where we belong.
        IFC(CorrectForEstimationErrors());

        // If ItemsUpdatingScrollMode is set to KeepLastItemInView and the viewport is at the very end
        // of the list, we need to shift the viewport to the last element which might have been recently inserted.
        IFC(TrackLastElement());

        // Measure elements in garbage section to avoid measuring during arrange pass, which can cause a layout cycle.
        IFC(MeasureElementsInGarbageSection());
    }

Cleanup:
    m_refreshPendingLayout = false;
    RRETURN(hr);
}

// Measure elements in garbage section.  Since there is no API for getting a non-special item sizes, it uses bogus index (-1) which is not special.
_Check_return_ HRESULT ModernCollectionBasePanel::MeasureElementsInGarbageSection()
{
    HRESULT hr = S_OK;
    ctl::ComPtr<wfc::IVector<xaml::UIElement*>> spChildren;
    UINT childrenCount = 0;
    wf::Size headerMeasureSize = { };
    wf::Size containerMeasureSize = { };
    wf::Rect realizationWindow = m_windowState.GetRealizationWindow();

    IFC(m_cacheManager.GetChildren(&spChildren));
    IFC(spChildren->get_Size(&childrenCount));

    IFC(m_spLayoutStrategy->GetElementMeasureSize(xaml_controls::ElementType_GroupHeader, -1, realizationWindow, &headerMeasureSize));
    IFC(m_spLayoutStrategy->GetElementMeasureSize(xaml_controls::ElementType_ItemContainer, -1, realizationWindow, &containerMeasureSize));

    for (UINT garbageIndex = m_containerManager.StartOfGarbageSection(); garbageIndex < childrenCount; ++garbageIndex)
    {
        ctl::ComPtr<IUIElement> spCurrentElement;
        wf::Size measureSize = { };
        wf::Size desiredSize = { };
        INT elementIndex = 0;

        IFC(spChildren->GetAt(garbageIndex, &spCurrentElement));

        const wf::Point position = GetGarbageElementPosition(spCurrentElement);

        xaml_controls::ElementType elementType = (GetElementIsHeader(spCurrentElement)) ? xaml_controls::ElementType_GroupHeader : xaml_controls::ElementType_ItemContainer;

        if (m_containerManager.GetIsElementPinned(elementType, spCurrentElement, &elementIndex))
        {
            ASSERT(elementIndex >= 0);

            if (elementType == xaml_controls::ElementType_GroupHeader)
            {
                IFC(m_spLayoutStrategy->GetElementMeasureSize(xaml_controls::ElementType_GroupHeader, elementIndex, realizationWindow, &measureSize));
            }
            else
            {
                IFC(m_spLayoutStrategy->GetElementMeasureSize(xaml_controls::ElementType_ItemContainer, elementIndex, realizationWindow, &measureSize));
            }
        }
        else
        {
            measureSize = (elementType == xaml_controls::ElementType_GroupHeader) ? headerMeasureSize : containerMeasureSize;
        }

        IFC(spCurrentElement->Measure(measureSize));

        IFC(spCurrentElement->get_DesiredSize(&desiredSize));

        SetBoundsForElement(spCurrentElement, RectUtil::CreateRect(position, desiredSize));

        // Protected by Apiset : TRUE only for Vertical Orientation + Headerplacement!=Left
        if (m_bUseStickyHeaders)
        {
            if (elementType == xaml_controls::ElementType_GroupHeader)
            {
                // Remove the parametric curves as the header reusing the container will have different curves
                RemoveStickyHeader(spCurrentElement);
            }
        }
    }

Cleanup:
    RRETURN(hr);
}

// If the layout specifies a need for a special group and/or item,
// ensure these items are generated, measured, and pinned.
_Check_return_ HRESULT ModernCollectionBasePanel::MeasureSpecialElements()
{
    HRESULT hr = S_OK;
    bool measureNeeded = FALSE;
    ctl::ComPtr<wfc::IVector<xaml::UIElement*>> spChildren;

    struct TypeIndexPair { xaml_controls::ElementType type; INT32 index; bool requestedByStrategy; };

    std::vector<TypeIndexPair> specialElements;

    IFC(NeedsSpecialGroup(&measureNeeded));
    if (measureNeeded)
    {
        int specialGroupIndex = -1;
        IFC(GetSpecialGroupIndex(&specialGroupIndex));
        if (specialGroupIndex >= 0 && specialGroupIndex < m_cacheManager.GetTotalGroupCount())
        {
            // C++11 uniform initialization not supported yet, so make a temp
            TypeIndexPair temp = { xaml_controls::ElementType_GroupHeader, specialGroupIndex, true };
            specialElements.emplace_back(temp);
        }
    }

    IFC(NeedsSpecialItem(&measureNeeded));
    if (measureNeeded)
    {
        int specialItemIndex = -1;
        IFC(GetSpecialItemIndex(&specialItemIndex));
        if (specialItemIndex >= 0 && specialItemIndex < m_cacheManager.GetTotalItemCount())
        {
            // C++11 uniform initialization not supported yet, so make a temp
            TypeIndexPair temp = { xaml_controls::ElementType_ItemContainer, specialItemIndex, true };
            specialElements.emplace_back(temp);
        }
    }

    if (m_enableCarouselPreload && m_cacheManager.GetTotalItemCount() > 0)
    {
        // C++11 uniform initialization not supported yet, so make a temp
        TypeIndexPair temp = { xaml_controls::ElementType_ItemContainer, 0, false };
        specialElements.emplace_back(temp);
        if (m_cacheManager.GetTotalItemCount() > 1)
        {
            temp.index = 1;
            specialElements.emplace_back(temp);
        }
        if (m_cacheManager.GetTotalItemCount() > 2)
        {
            temp.index = m_cacheManager.GetTotalItemCount() - 1;
            specialElements.emplace_back(temp);
        }
    }

    IFC(m_cacheManager.GetChildren(&spChildren));

    for (const auto& specialElementInfo : specialElements)
    {
        bool shouldEnter = false;
        wf::Size childDesiredSize;
        ctl::ComPtr<IUIElement> spSpecialElement;
        wf::Size measureSize;
        bool newElementGenerated = false;

        IFC(m_spLayoutStrategy->GetElementMeasureSize(specialElementInfo.type, specialElementInfo.index, m_windowState.GetRealizationWindow(), &measureSize));

        IFC(m_containerManager.GetElementFromPinnedElements(specialElementInfo.type, specialElementInfo.index, &spSpecialElement));

        if (!spSpecialElement)
        {
            IFC(m_containerManager.GetElementAtDataIndex(specialElementInfo.type, specialElementInfo.index, &spSpecialElement));
            if (!spSpecialElement)
            {
                // Generate, because this element hasn't been created yet
                UINT32 foundIndex = 0;
                BOOLEAN found = FALSE;

                IFC(GenerateElementAtDataIndex(specialElementInfo.type, specialElementInfo.index, &spSpecialElement));
                newElementGenerated = true;

                // items need to be in the visual tree to be measured correctly
                // however, this does not mean they have to be in the valid part.
                IFC(spChildren->IndexOf(spSpecialElement.Get(), &foundIndex, &found));
                if (!found)
                {
                    IFC(spChildren->Append(spSpecialElement.Get()));
                }
                else
                {
                    // if element is in the visual tree (comes from recycling headers) we should setup Loading transition.
                    shouldEnter = true;
                }

                IFC(PrepareElementViaItemsHost(specialElementInfo.type, specialElementInfo.index, measureSize, spSpecialElement));
            }

            IFC(m_containerManager.RegisterPinnedElement(specialElementInfo.type, specialElementInfo.index, spSpecialElement));
        }

        // items need to be in the visual tree to be measured correctly
        // however, this does not mean they have to be in the valid part.
        if (shouldEnter)
        {
            IFC(CoreImports::UIElement_SetIsEntering(static_cast<CUIElement*>(spSpecialElement.Cast<UIElement>()->GetHandle()), TRUE));
        }

        IFC(spSpecialElement->Measure(measureSize));
        IFC(spSpecialElement->get_DesiredSize(&childDesiredSize));

        if (specialElementInfo.requestedByStrategy)
        {
            IFC(RegisterSpecialElementSize(specialElementInfo.type, specialElementInfo.index, childDesiredSize));
        }

        // Protected by Apiset : TRUE only for Vertical Orientation + Headerplacement!=Left
        if (m_bUseStickyHeaders && newElementGenerated && specialElementInfo.type == xaml_controls::ElementType_GroupHeader)
        {
            // We don't have the positioning, only the size... does it really make sense here ?
            IFC(ConfigureStickyHeader(spSpecialElement, specialElementInfo.index, RectUtil::CreateRect(wf::Point(), childDesiredSize)));
        }
    }

Cleanup:
    RRETURN(hr);
}


// Measure all valid containers, regardless of window.
// This is not only required for the measure contract
// of calling measure on all owned children, but also
// allows us to understand how much space we take up.
_Check_return_ HRESULT ModernCollectionBasePanel::RunGenerate()
{
    HRESULT hr = S_OK;

    // If there is no existing element to measure, we get out.
    if ( (m_cacheManager.IsGrouping() && m_cacheManager.GetTotalLayoutGroupCount() == 0)
        || (!m_cacheManager.IsGrouping() && m_cacheManager.GetTotalItemCount() == 0))
    {
        goto Cleanup;
    }

    IFC(m_spLayoutStrategy->BeginMeasure());

    // Initializes the reference location.
    {
        LayoutReference referenceInformation = CreateDefaultLayoutReference();
        CollectionIterator iterator(m_cacheManager);

        // Set the starting location.
        if(m_viewportBehavior.isTracking)
        {
            // We need to shift all the elements to go under the realization window that accounts
            // for the virtualization shift.
            IFC(ApplyTrackedElementShift());

            // New behavior, starts from the tracked element.
            iterator.Init(m_viewportBehavior.index, m_viewportBehavior.type);

            // the idea is that the very first measure is going to be relative to itself
            referenceInformation.RelativeLocation = xaml_controls::ReferenceIdentity_Myself;
            referenceInformation.ReferenceBounds = m_viewportBehavior.elementBounds;
            referenceInformation.ReferenceIsHeader = (m_viewportBehavior.type == xaml_controls::ElementType_GroupHeader);

            IFC(Generate(iterator, referenceInformation, TRUE /* goForward */));

            // If we arrived here tracking an element, we'll generate backwards off that same point
            // This should be safe because even if tracking a header, it shouldn't have become floating
            // because the header is visible and not a candidate for recycling off the front. Akso, all of its items come after it in the order,
            // so they're also not candidates for recycling off the front
            if (iterator.MovePrevious())
            {
                referenceInformation.RelativeLocation = xaml_controls::ReferenceIdentity_AfterMe;
                IFC(Generate(iterator, referenceInformation, FALSE /* goForward */ ));
            }
        }
        else
        {
            // Old behavior, just start from the first non-floating element
            IFC(GetGenerationAnchorInformation(
                &referenceInformation,
                &iterator));
            referenceInformation.RelativeLocation = xaml_controls::ReferenceIdentity_Myself;

            IFC(Generate(iterator, referenceInformation, TRUE /* goForward */));

            // We didn't start off tracking an element, so there's no guarantee that the original reference is still alive
            // To avoid those problems, we'll just grab a new reference from the new beginning of the valid range

            IFC(GetGenerationAnchorInformation(
                &referenceInformation,
                &iterator));
            if (iterator.MovePrevious())
            {
                referenceInformation.RelativeLocation = xaml_controls::ReferenceIdentity_AfterMe;
                IFC(Generate(iterator, referenceInformation, FALSE /* goForward */ ));
            }

        }
    }

    IFC(m_spLayoutStrategy->EndMeasure());

Cleanup:
    RRETURN(hr);
}

// Given a start location and a direction, remove/recycle all valid containers and headers.
_Check_return_ HRESULT ModernCollectionBasePanel::RemoveMeasureLeftOvers(
    _In_ CollectionIterator iterator,
    _In_ BOOLEAN goForward)
{
    CollectionIterator::ElementInfo current = iterator.GetCurrent();

    // We will start from these indices when removing sentinels and leftovers.
    INT lastValidContainerIndex = m_containerManager.GetValidContainerIndexFromItemIndex(current.itemIndex);
    INT lastValidHeaderIndex = m_containerManager.GetValidHeaderIndexFromGroupIndex(current.groupIndex);

    // This is the number of elements we need to remove either at the beginning or at the end depending
    // on the measure direction (forward / backward).
    INT leftOverContainerCount = 0;
    INT leftOverHeaderCount = 0;

    if(goForward)
    {
        // We stopped at a sentinel header. lastValidHeaderIndex is already correct.
        if(current.isHeader())
        {
        }
        // We stopped at a sentinel container. lastValidContainerIndex is already correct.
        else
        {
            ++lastValidHeaderIndex;     // Skip the current's container group header.
        }

        INT validContainerCount = m_containerManager.GetValidContainerCount();
        if (validContainerCount > 0)
        {
            ASSERT(lastValidContainerIndex >= 0);
            leftOverContainerCount = validContainerCount - lastValidContainerIndex;
        }
        else
        {
            ASSERT(lastValidContainerIndex == -1);
        }

        if(m_cacheManager.IsGrouping())
        {
            leftOverHeaderCount = m_containerManager.GetValidHeaderCount() - lastValidHeaderIndex;
        }
    }
    // Going backward is not as straightforward as going forward.
    // We should not recycle/remove a header if there is at least one container realized in that group.
    else
    {
        // We stopped at a sentinel header.
        if(current.isHeader())
        {
            // We always generate the floating header while going backwards
            // So we always lay it out if we successfully iterated through all of its items
            // Therefore, the only reason to stop on a header is if it's for an empty group
            ASSERT(current.itemCountInGroup == 0);

            if (lastValidContainerIndex >= 0)
            {
                // The container index currently points at the first item of the next nonempty group
                // Decrement so this valid item doesn't get caught in the cleanup
                --lastValidContainerIndex;
            }
        }
        // We stopped at a sentinel container.
        else
        {
            // Only remove the current group if we stopped at the last container of that group.
            // Otherwise, if we haven't stopped on the group's last container, there should be
            // a floating header waiting for us. We'll lay it out after the cleanup.
            // In the meantime, let's not recycle it in the cleanup, so decrement the index
            if(m_cacheManager.IsGrouping() && current.IndexInGroup() != (current.itemCountInGroup - 1))
            {
#ifdef DBG
                ctl::ComPtr<IUIElement> spHeader;
                IFC_RETURN(m_containerManager.GetHeaderAtValidIndex(lastValidHeaderIndex, &spHeader));
                ASSERT(spHeader);
#endif
                --lastValidHeaderIndex;
            }
        }

        // +1 to convert from an index to a count.
        leftOverContainerCount = lastValidContainerIndex + 1;
        if(m_cacheManager.IsGrouping())
        {
            leftOverHeaderCount = lastValidHeaderIndex + 1;
        }
    }

    ASSERT(leftOverContainerCount >= 0 && leftOverContainerCount <= m_containerManager.GetValidContainerCount());
    ASSERT(!m_cacheManager.IsGrouping() || (leftOverHeaderCount >= 0 && leftOverHeaderCount <= m_containerManager.GetValidHeaderCount()));

    // We start with containers leftovers and sentinels.
    INT validContainerCountAfterRemovingLeftOvers = m_containerManager.GetValidContainerCount() - leftOverContainerCount;
    while (m_containerManager.GetValidContainerCount() > validContainerCountAfterRemovingLeftOvers)
    {
        INT32 validIndex = goForward ? m_containerManager.GetValidContainerCount() - 1 : 0;
        ctl::ComPtr<IUIElement> spCurrentElement;

        IFC_RETURN(m_containerManager.GetContainerAtValidIndex(validIndex, &spCurrentElement));

        if (!GetElementIsSentinel(spCurrentElement))
        {
            IFC_RETURN(m_icg2->RecycleContainer(spCurrentElement.Get()));
        }

        IFC_RETURN(m_containerManager.RemoveFromValidContainers(validIndex, FALSE /* isForItemRemoval */));
    }

    // And then we do the same for header leftovers and sentinels.
    INT validHeaderCountAfterRemovingLeftOvers = m_containerManager.GetValidHeaderCount() - leftOverHeaderCount;
    while (m_containerManager.GetValidHeaderCount() > validHeaderCountAfterRemovingLeftOvers)
    {
        INT32 validIndex = goForward ? m_containerManager.GetValidHeaderCount() - 1 : 0;
        ctl::ComPtr<IUIElement> spCurrentElement;

        IFC_RETURN(m_containerManager.GetHeaderAtValidIndex(validIndex, &spCurrentElement));

        if (!GetElementIsSentinel(spCurrentElement))
        {
            IFC_RETURN(m_icg2->RecycleHeader(spCurrentElement.Get()));
        }

        IFC_RETURN(m_containerManager.RemoveFromValidHeaders(validIndex, FALSE /* isForGroupRemoval */));
    }

    return S_OK;
}

// grabs the bounds of the supplied starting element and uses that as the anchor into the strategy.
// 1. asks strategy whether we should continue generating
//    boolean Strategy.ContinueGenerating(int itemIX, int siblingIX, rect siblingBound, window)
// 2. generate or recycle container and measure
// 3. container.VirtualizationInformation.Bounds = Strategy.GetBounds(int itemIX, Size desiredSize, int siblingIX, rect siblingBounds)
// These bounds will be either desiredsize based or fixed size based (wrapgrid) or vswg like (new strategy that knows sizes upfront)
_Check_return_ HRESULT ModernCollectionBasePanel::Generate(
    _In_ CollectionIterator iterator,
    _In_ xaml_controls::LayoutReference referenceInformation,
    _In_ BOOLEAN goForward)
{
    TraceGenerateItemsBegin();

    auto guard = wil::scope_exit([]()
    {
        TraceGenerateItemsEnd();
    });

    // needed for Sticky Headers
    ctl::ComPtr<IScrollViewer> spScrollViewer = m_wrScrollViewer.AsOrNull<IScrollViewer>();

    wf::Rect windowToFill = {};

    // This flag tell us if we were tracking before Generate is called.
    // It's possible that we start tracking during the generate pass.
    // In that case, we will have to update the realization window after
    // the tracked element is placed. This will make sure it doesn't get recycled.
    BOOLEAN wereTrackingBefore = m_viewportBehavior.isTracking;

    // Keep track of whether we've generated a header that we have yet to place
    ctl::ComPtr<IUIElement> spFloatingHeader;

    const auto referenceDirection = goForward ? xaml_controls::ReferenceIdentity_BeforeMe : xaml_controls::ReferenceIdentity_AfterMe;

    if ((m_cacheManager.GetTotalItemCount() == 0 && m_cacheManager.GetTotalLayoutGroupCount() == 0) || !m_windowState.validWindowCalculation)
    {
        return S_OK;
    }

    ASSERT(m_containerManager.GetValidContainerCount() > 0 || (m_cacheManager.IsGrouping() && m_containerManager.GetValidHeaderCount() > 0));

    // When tracking, we should care about the future realization window
    // (when EndTrackingFirstVisbleElement gets called).
    if(m_viewportBehavior.isTracking)
    {
        windowToFill = GetRealizationWindowAfterViewportShift();
    }
    else
    {
        windowToFill = m_windowState.GetRealizationWindow();
    }

    // Let's make sure we have a reference rect for the previous header
    if (m_cacheManager.IsGrouping())
    {
        if (referenceInformation.ReferenceIsHeader)
        {
            // If our layout reference is already a header, just make sure it's also reflected here
            referenceInformation.HeaderBounds = referenceInformation.ReferenceBounds;
        }
        else
        {
            const auto current = iterator.GetCurrent();
            INT32 groupIndexOfReferenceHeader = -1;

            // We're trying to layout from a container
            if (goForward)
            {
                if (current.isHeader())
                {
                    // We're trying to generate a header, so the interesting header is the one for groupIndex-1
                    groupIndexOfReferenceHeader = current.groupIndex - 1;
                }
                else
                {
                    // Generating a container, so keep the header of the current group
                    groupIndexOfReferenceHeader = current.groupIndex;
                }
            }
            else
            {
                // Need to look at the header for group+1, to ensure that if the current header is big,
                // we place it far enough left to not push group+1 to the right
                groupIndexOfReferenceHeader = current.groupIndex + 1;
            }

            ctl::ComPtr<IUIElement> spReferenceHeader;
            if (m_containerManager.IsValidHeaderIndexWithinBounds(m_containerManager.GetValidHeaderIndexFromGroupIndex(groupIndexOfReferenceHeader)))
            {
                IFC_RETURN(m_containerManager.GetHeaderAtGroupIndex(groupIndexOfReferenceHeader, &spReferenceHeader));
            }

            if (spReferenceHeader)
            {
                referenceInformation.HeaderBounds = GetBoundsFromElement(spReferenceHeader);
            }
            else
            {
                // Since our reference rect belongs to a container, that container should have a header.
                // So the only way this can be hit is if we are generating backwards
                ASSERT(!goForward);
                referenceInformation.HeaderBounds = RectUtil::CreateEmptyRect();
            }
        }
    }
    else
    {
        // Not grouping, let's just clear this out to avoid any misunderstanding
        referenceInformation.HeaderBounds = RectUtil::CreateEmptyRect();
    }

    BOOLEAN continueGenerating = TRUE;
    BOOLEAN inCollection = TRUE;
    while(continueGenerating && inCollection)
    {
        const CollectionIterator::ElementInfo current = iterator.GetCurrent();
        // TODO: the blocks for headers and containers are nearly identical, refactor support methods to accept enum as type
        // allowing us to remove lots of duplicate code
        if (current.isHeader())
        {
            ASSERT(m_containerManager.GetValidHeaderCount() == 0 ||
                (m_containerManager.GetGroupIndexFromValidIndex(-1) <= current.groupIndex &&
                current.groupIndex <= m_containerManager.GetGroupIndexFromValidIndex(m_containerManager.GetValidHeaderCount())) );

            ctl::ComPtr<IUIElement> spCurrentHeader;

            // Let's compare what we have here, with what we *should* have
            IFC_RETURN(m_containerManager.GetHeaderAtGroupIndex(current.groupIndex, &spCurrentHeader));

            if (ShouldElementBeVisible(current.type, current.groupIndex))
            {
                BOOL newHeaderGenerated = FALSE;

                // Only check against the bounds of the window if we need to create a new element
                // Or...
                // If we are going backwards and are looking to finish off a nonempty group by placing its header,
                // we should always do that work even if the header is going to end up outside the window.
                // So, we should only entertain the notion of stopping if we're looking at a "solo" header for an empty group
                // Reasoning: We've already done the expensive work of generating this "floating" header,
                //            so we might as well place it in the valid range and lay it out.
                if (spCurrentHeader ||
                    (!goForward && current.itemCountInGroup > 0))
                {
                    continueGenerating = TRUE;
                }
                else
                {
                    IFC_RETURN(m_spLayoutStrategy->ShouldContinueFillingUpSpace(
                        xaml_controls::ElementType_GroupHeader,
                        m_cacheManager.DataIndexToLayoutIndex(xaml_controls::ElementType_GroupHeader, current.groupIndex),
                        referenceInformation,
                        windowToFill,
                        &continueGenerating));

                    // If we've got a floating header, we should *always* lay it out
                    ASSERT(!spFloatingHeader);
                }

                // Let's measure this thing
                if (continueGenerating)
                {
                    BOOLEAN oldBoundsAreValid = FALSE;
                    wf::Size measureSize;
                    wf::Size desiredSize = {};
                    wf::Rect bounds = {};
                    wf::Rect oldBounds = {};

                    // If we had a floating header coming in, this should be it
                    ASSERT(!spFloatingHeader || spFloatingHeader == spCurrentHeader);

                    IFC_RETURN(m_spLayoutStrategy->GetElementMeasureSize(xaml_controls::ElementType_GroupHeader, current.groupIndex, windowToFill, &measureSize));

                    // If we don't have one yet, let's go ahead and generate it now
                    if (!spCurrentHeader)
                    {
                        // now generate. Hopefully we have been able to fill the recyclequeue, but if not
                        // ICG will just create a new container for us.
                        IFC_RETURN(EnsureRecycleHeaderCandidate(goForward /* recycle candidates are at the front */, FALSE /* alwaysRecycle */));
                        if (m_containerManager.IsValidHeaderIndexWithinBounds(m_containerManager.GetValidHeaderIndexFromGroupIndex(current.groupIndex)))
                        {
                            // If we're still filling out sentinels, go ahead and recycle future elements from the end we are generating to
                            IFC_RETURN(EnsureRecycleHeaderCandidate(!goForward, FALSE /* alwaysRecycle */));
                        }
                        IFC_RETURN(m_icg2->GenerateHeaderAtGroupIndex(current.groupIndex, &spCurrentHeader));
                        IFC_RETURN(m_containerManager.PlaceInValidHeaders(current.groupIndex, spCurrentHeader));
                        IFC_RETURN(PrepareHeaderViaItemsHost(current.groupIndex, measureSize, spCurrentHeader));
                        newHeaderGenerated = TRUE;
                    }
                    else
                    {
                        oldBoundsAreValid = TRUE;
                        oldBounds = GetBoundsFromElement(spCurrentHeader);
                    }

                    IFC_RETURN(spCurrentHeader->Measure(measureSize));
                    IFC_RETURN(spCurrentHeader->get_DesiredSize(&desiredSize));
                    IFC_RETURN(m_spLayoutStrategy->GetElementBounds(
                        xaml_controls::ElementType_GroupHeader,
                        m_cacheManager.DataIndexToLayoutIndex(xaml_controls::ElementType_GroupHeader, current.groupIndex),
                        desiredSize,
                        referenceInformation,
                        windowToFill,
                        &bounds));

                    // When elements change size, we need to start tracking the first visible element if the maintain
                    // viewport behavior is enabled.
                    if(oldBoundsAreValid &&
                       (!DoubleUtil::AreClose(bounds.Width, oldBounds.Width) || !DoubleUtil::AreClose(bounds.Height, oldBounds.Height)))
                    {
                        IFC_RETURN(BeginTrackingOnMeasureChange(true /* isGroupChange */, current.groupIndex));
                    }

                    // If we are tracking due to a measure change and we are about to set
                    // new bounds for the tracked element, we will need to update the local
                    // realization window.
                    if(m_viewportBehavior.isTracking &&
                       !wereTrackingBefore &&
                       m_viewportBehavior.type == current.type &&
                       m_viewportBehavior.index == current.groupIndex)
                    {
                        m_viewportBehavior.elementBounds = bounds;
                        windowToFill = GetRealizationWindowAfterViewportShift();
                    }

                    SetBoundsForElement(spCurrentHeader, bounds);

                    referenceInformation.ReferenceIsHeader = TRUE;
                    referenceInformation.RelativeLocation = referenceDirection;
                    referenceInformation.ReferenceBounds = bounds;
                    referenceInformation.HeaderBounds = bounds;

                    // Protected by Apiset : TRUE only for Vertical Orientation + Headerplacement!=Left
                    if (m_bUseStickyHeaders && newHeaderGenerated && spScrollViewer)
                    {
                        IFC_RETURN(ConfigureStickyHeader(spCurrentHeader, current.groupIndex, bounds));
                    }

                    // We've placed the header, so no more floating header
                    spFloatingHeader.Reset();
                }
            }
            else // endif ShouldElementBeVisible
            {
                if (spCurrentHeader)
                {
                    // Looks like we have an element there, but shouldn't.
                    // Recycle it and replace it with a sentinel

                    // If somebody is trying to hide a non-empty group, we've got a problem here...
                    ASSERT(current.itemCountInGroup == 0);

                    // If we are tracking this header, we should elect a new one now.
                    if (m_viewportBehavior.isTracking)
                    {
                        ctl::ComPtr<IUIElement> spTrackedElement;
                        IFC_RETURN(GetTrackedElement(&spTrackedElement));
                        if (spCurrentHeader == spTrackedElement)
                        {
                            IFC_RETURN(ElectNewTrackedElement());
                        }
                    }

                    IFC_RETURN(m_icg2->RecycleHeader(spCurrentHeader.Get()));
                    spCurrentHeader.Reset();
                }
                // Before continuing, place a sentinel in the range so it's accounted for
                IFC_RETURN(m_containerManager.PlaceInValidHeaders(current.groupIndex, nullptr));
            }
        }
        else //container case
        {
            ASSERT(m_containerManager.GetValidContainerCount() == 0 ||
                (m_containerManager.GetItemIndexFromValidIndex(-1) <= current.itemIndex &&
                current.itemIndex <= m_containerManager.GetItemIndexFromValidIndex(m_containerManager.GetValidContainerCount())));

            ctl::ComPtr<IUIElement> spCurrentContainer;

            // Let's compare what we have here, with what we *should* have
            IFC_RETURN(m_containerManager.GetContainerAtItemIndex(current.itemIndex, &spCurrentContainer));

            if (ShouldElementBeVisible(current.type, current.itemIndex))
            {
                // Only check against the bounds of the window if we need to create a new element
                if (spCurrentContainer)
                {
                    continueGenerating = TRUE;
                }
                else
                {
                    IFC_RETURN(m_spLayoutStrategy->ShouldContinueFillingUpSpace(
                        xaml_controls::ElementType_ItemContainer,
                        current.IndexInGroup(),
                        referenceInformation,
                        windowToFill,
                        &continueGenerating));
                }

                // Let's measure it
                if (continueGenerating)
                {
                    BOOLEAN oldBoundsAreValid = FALSE;
                    wf::Size desiredSize = {};
                    wf::Size measureSize;
                    wf::Rect bounds = {};
                    wf::Rect oldBounds = {};

                    IFC_RETURN(m_spLayoutStrategy->GetElementMeasureSize(xaml_controls::ElementType_ItemContainer, current.itemIndex, windowToFill, &measureSize));

                    // If we don't have one yet, let's go ahead and generate it now
                    if (!spCurrentContainer)
                    {
                        // now generate. Hopefully we have been able to fill the recyclequeue, but if not
                        // ICG will just create a new container for us.
                        IFC_RETURN(EnsureRecycleContainerCandidate(current.itemIndex, goForward /* recycle candidates are at the front */));
                        if (m_containerManager.IsValidContainerIndexWithinBounds(m_containerManager.GetValidContainerIndexFromItemIndex(current.itemIndex)))
                        {
                            // If we're still filling out sentinels, go ahead and recycle future elements from the end we are generating to
                            IFC_RETURN(EnsureRecycleContainerCandidate(current.itemIndex, !goForward));
                        }

                        IFC_RETURN(m_icg2->GenerateContainerAtIndex(current.itemIndex, &spCurrentContainer));
                        IFC_RETURN(m_containerManager.PlaceInValidContainers(current.itemIndex, spCurrentContainer));
                        IFC_RETURN(PrepareContainerViaItemsHost(current.itemIndex, measureSize, spCurrentContainer));
                    }
                    else
                    {
                        oldBoundsAreValid = TRUE;
                        oldBounds = GetBoundsFromElement(spCurrentContainer);
                    }

                    IFC_RETURN(spCurrentContainer->Measure(measureSize));
                    IFC_RETURN(spCurrentContainer->get_DesiredSize(&desiredSize));
                    IFC_RETURN(m_spLayoutStrategy->GetElementBounds(
                        xaml_controls::ElementType_ItemContainer,
                        current.itemIndex,
                        desiredSize,
                        referenceInformation,
                        windowToFill,
                        &bounds));

                    // When elements change size, we need to start tracking the first visible element if the maintain
                    // viewport behavior is enabled.
                    if(oldBoundsAreValid &&
                       (!DoubleUtil::AreClose(bounds.Width, oldBounds.Width) || !DoubleUtil::AreClose(bounds.Height, oldBounds.Height)))
                    {
                        IFC_RETURN(BeginTrackingOnMeasureChange(false /* isGroupChange */, current.itemIndex));
                    }

                    // If we are tracking due to a measure change and we are about to set
                    // new bounds for the tracked element, we will need to update the local
                    // realization window.
                    if(m_viewportBehavior.isTracking &&
                       !wereTrackingBefore &&
                       m_viewportBehavior.type == current.type &&
                       m_viewportBehavior.index == current.itemIndex)
                    {
                        m_viewportBehavior.elementBounds = bounds;
                        windowToFill = GetRealizationWindowAfterViewportShift();
                    }

                    SetBoundsForElement(spCurrentContainer, bounds);

                    // Defer updating the layout reference until we've generated a floating header

                    // Ensure we have a header to place at the head of this group
                    if (m_cacheManager.IsGrouping() && !goForward && !spFloatingHeader)
                    {
                        IFC_RETURN(m_containerManager.GetHeaderAtGroupIndex(current.groupIndex, &spFloatingHeader));

                        if (!spFloatingHeader)
                        {
                            wf::Size measureSizeForFloatingHeader;
                            wf::Size desiredSizeForFloatingHeader;
                            wf::Rect boundsForFloatingHeader;

                            // We need to generate this header to satisfy invariants, but if we somehow think
                            // this header shouldn't be generated, somebody is mistaken
                            ASSERT(ShouldElementBeVisible(xaml_controls::ElementType_GroupHeader, current.groupIndex));

                            IFC_RETURN(m_spLayoutStrategy->GetElementMeasureSize(xaml_controls::ElementType_GroupHeader, current.groupIndex, windowToFill, &measureSizeForFloatingHeader));

                            // We must have just started creating this group. Let's preemptively generate the header
                            IFC_RETURN(EnsureRecycleHeaderCandidate(goForward /* recycle candidates are at the front */, FALSE /* always recycle */));
                            if (m_containerManager.IsValidHeaderIndexWithinBounds(m_containerManager.GetValidHeaderIndexFromGroupIndex(current.groupIndex)))
                            {
                                // If we're still filling out sentinels, go ahead and recycle future elements from the end we are generating to
                                IFC_RETURN(EnsureRecycleHeaderCandidate(!goForward, FALSE /* alwaysRecycle */));
                            }

                            IFC_RETURN(m_icg2->GenerateHeaderAtGroupIndex(current.groupIndex, &spFloatingHeader));
                            IFC_RETURN(m_containerManager.PlaceInValidHeaders(current.groupIndex, spFloatingHeader));
                            IFC_RETURN(PrepareHeaderViaItemsHost(current.groupIndex, measureSizeForFloatingHeader, spFloatingHeader));

                            IFC_RETURN(spFloatingHeader->Measure(measureSizeForFloatingHeader));
                            IFC_RETURN(spFloatingHeader->get_DesiredSize(&desiredSizeForFloatingHeader));

                            // TODO: We should be able to streamline this a bit with generalized estimations in the future
                            // but for now, we need to give it some sensible bounds
                            IFC_RETURN(m_spLayoutStrategy->GetElementBounds(
                                xaml_controls::ElementType_GroupHeader,
                                m_cacheManager.DataIndexToLayoutIndex(xaml_controls::ElementType_GroupHeader, current.groupIndex),
                                desiredSizeForFloatingHeader,
                                referenceInformation,
                                windowToFill,
                                &boundsForFloatingHeader));

                            SetBoundsForElement(spFloatingHeader, boundsForFloatingHeader);

                            // Protected by Apiset : TRUE only for Vertical Orientation + Headerplacement!=Left
                            if (m_bUseStickyHeaders && spScrollViewer)
                            {
                                IFC_RETURN(ConfigureStickyHeader(spFloatingHeader, current.groupIndex, boundsForFloatingHeader));
                            }
                        }
                    }

                    referenceInformation.ReferenceIsHeader = FALSE;
                    referenceInformation.RelativeLocation = referenceDirection;
                    referenceInformation.ReferenceBounds = bounds;
                }
                else // endif ContinueGenerating
                {
                    // We are not going to generate anymore, however we could be in a situation where
                    // we start off with the realization window at item n of a group, and dont generate
                    // even a single item because the reference's (first item) point starts before the
                    // window. In this case, we end up not measuring the floating header. Refer Bug: 2798854
                    // If there is a floating header, make sure it is always measured in the backward's pass
                    if (m_cacheManager.IsGrouping() && !goForward)
                    {
                        IFC_RETURN(m_containerManager.GetHeaderAtGroupIndex(current.groupIndex, &spFloatingHeader));
                        if (spFloatingHeader && spFloatingHeader.Cast<UIElement>()->GetHandle()->GetIsMeasureDirty())
                        {
                            // we have a floating header that has not been measured, make sure it is

                            wf::Size measureSizeForFloatingHeader;
                            wf::Size desiredSizeForFloatingHeader;
                            wf::Rect boundsForFloatingHeader;

                            IFC_RETURN(m_spLayoutStrategy->GetElementMeasureSize(xaml_controls::ElementType_GroupHeader, current.groupIndex, windowToFill, &measureSizeForFloatingHeader));
                            IFC_RETURN(spFloatingHeader->Measure(measureSizeForFloatingHeader));
                            IFC_RETURN(spFloatingHeader->get_DesiredSize(&desiredSizeForFloatingHeader));

                            IFC_RETURN(m_spLayoutStrategy->GetElementBounds(
                                xaml_controls::ElementType_GroupHeader,
                                m_cacheManager.DataIndexToLayoutIndex(xaml_controls::ElementType_GroupHeader, current.groupIndex),
                                desiredSizeForFloatingHeader,
                                referenceInformation,
                                windowToFill,
                                &boundsForFloatingHeader));

                            SetBoundsForElement(spFloatingHeader, boundsForFloatingHeader);

                            // Configure the sticky header now that the header is measured
                            if (m_bUseStickyHeaders && spScrollViewer)
                            {
                                IFC_RETURN(ConfigureStickyHeader(spFloatingHeader, current.groupIndex, boundsForFloatingHeader));
                            }
                        }
                    }
                }
            }
            else // endif ShouldElementBeVisible
            {
                if (spCurrentContainer)
                {
                    // Looks like we have an element there, but shouldn't.
                    // Recycle it and replace it with a sentinel

                    IFC_RETURN(m_icg2->RecycleContainer(spCurrentContainer.Get()));
                    spCurrentContainer.Reset();
                }

                // Before continuing, place a sentinel in the range so it's accounted for
                IFC_RETURN(m_containerManager.PlaceInValidContainers(current.itemIndex, nullptr));
           }
        }

        // Let's move to the next one and attempt to continue
        if (continueGenerating)
        {
            if (goForward)
            {
                inCollection = iterator.MoveNext();
            }
            else
            {
                inCollection = iterator.MovePrevious();
            }
        }
    }

    // Now that we've measured and/or instantiated all the sentinels, let's clean out the sentinels and leftovers
    if (inCollection)
    {
        IFC_RETURN(RemoveMeasureLeftOvers(iterator, goForward));
    }

    if (spFloatingHeader)
    {
        // Consistency check that the floating header is what we think it is
        ASSERT(m_containerManager.GetValidHeaderCount() > 0 &&
           iterator.GetCurrent().groupIndex  == m_containerManager.GetGroupIndexFromValidIndex(0));
        ASSERT(!goForward);

        IFC_RETURN(EstimateFloatingHeaderLocation(spFloatingHeader));
    }

    m_containerManager.TrimEdgeSentinels();

    // Protected by Apiset : TRUE only for Vertical Orientation + Headerplacement!=Left
    if (m_bUseStickyHeaders && goForward)
    {
        // If we are realizing forward (which we do in each measure pass),
        // we store the fact that we have hit the end of the list
        m_bLastItemRealized = !inCollection;
    }

    return S_OK;
}

// Ensure that any pinned containers still have a reason to be pinned. If not, unpin them.
_Check_return_ HRESULT ModernCollectionBasePanel::PrunePinnedContainers()
{
    ctl::ComPtr<IGeneratorHost> spIHost;
    int specialItemIndex = -1;

    // We currently have 2 reasons for keeping a container pinned:
    // 1- The container is the special item.
    // 2- The GeneratorHost tells us we can't recycle that container.
    // Verify that either still hold. If not, unpin.

    IFC_RETURN(GetSpecialItemIndex(&specialItemIndex));

    IFC_RETURN(m_cacheManager.GetItemsHost(&spIHost));

    const auto totalItemCount = m_cacheManager.GetTotalItemCount();
    const auto enableCarouselPreload = m_enableCarouselPreload;

    IFC_RETURN(m_containerManager.FilterPinnedContainers(
        [&spIHost, specialItemIndex, totalItemCount, enableCarouselPreload] (INT index, IUIElement* pElement, BOOLEAN* pKeep)
        {
            HRESULT hr2 = S_OK;

            if (specialItemIndex == index)
            {
                *pKeep = TRUE;
            }
            else if (enableCarouselPreload &&
                (index == 0 || index == 1 || index == totalItemCount - 1))
            {
                *pKeep = TRUE;
            }
            else
            {
                BOOLEAN canRecycle = FALSE;
                hr2 = (spIHost->CanRecycleContainer(static_cast<UIElement*>(pElement), &canRecycle));
                *pKeep = !canRecycle;
            }

            return hr2;
        }));

    return S_OK;
}

// Recycle containers if our recycle queue is empty and we have containers to recycle.
_Check_return_ HRESULT ModernCollectionBasePanel::EnsureRecycleContainerCandidate(_In_ INT32 index, _In_ BOOLEAN fromFront)
{
    HRESULT hr = S_OK;

    BOOLEAN recycleQueueHasCandidate = FALSE;

    const INT32 lastValidContainerIndex = m_containerManager.GetValidContainerCount() -1;

    // is the recycle queue filled?
    ctl::ComPtr<IContainerRecyclingContext> spRecyclingContext;
    IFC(m_cacheManager.GetContainerRecyclingContext(index, &spRecyclingContext));

    IFC(m_icg2->FindRecyclingCandidate(index, &recycleQueueHasCandidate));

    if (!recycleQueueHasCandidate && lastValidContainerIndex >= 0)
    {
        ctl::ComPtr<IUIElement> spCandidate;
        ctl::ComPtr<IUIElement> spFocusedContainer;
        ctl::ComPtr<IGeneratorHost> spIHost;

        // Index are valid in collections and therefore are navigated either incrementing from 0 or decrementing from lastValidContainerIndex
        INT32 indexFirstRecycleCandidateInValidContainers = fromFront ? 0 : lastValidContainerIndex;
        INT32 indexRecycleCandidateInValidContainers = indexFirstRecycleCandidateInValidContainers;
        INT32 increment = fromFront ? 1 : -1;
        // In order to handle index changes when we remove a container, we will keep track of positions in the iteration
        // Positions are always counted incrementing from 0
        INT32 focusedPosition = -1;
        INT32 foundPosition = -1;
        INT32 currentPosition = 0;
        INT32 fallbackPosition = -1;

        BOOL found = FALSE;
        BOOL areDisjoint = TRUE;
        // ah, there is no candidate for recycling. let's make it happen

        IFC(m_cacheManager.GetItemsHost(&spIHost));

        while (areDisjoint && (!found) && (currentPosition < ValidContainersBeforeFallback) && m_containerManager.IsValidContainerIndexWithinBounds(indexRecycleCandidateInValidContainers))
        {
            IFC(m_containerManager.GetContainerAtValidIndex(indexRecycleCandidateInValidContainers, &spCandidate));

            areDisjoint =
                GetElementIsSentinel(spCandidate) == false && // If the container hasn't been realized yet, it's considered non disjoint.
                RectUtil::AreDisjoint(GetVirtualizationInformationFromElement(spCandidate)->GetBounds(), m_viewportBehavior.isTracking ? GetRealizationWindowAfterViewportShift() : m_windowState.GetRealizationWindow());

            // if !areDisjoint, we reached the start of the virtualization window and will not find a candidate anymore
            if (areDisjoint)
            {
                // focused logic here
                // we have to check whether this container is focused. if so, we move it to some separate storage
                // and try the next container
                BOOLEAN hasFocus = FALSE;
                BOOLEAN isRecyclable = FALSE;

                IFC(m_cacheManager.IsFocusedChild(spCandidate.Get(), &hasFocus));

                IFC(spIHost->CanRecycleContainer(spCandidate.Cast<UIElement>(), &isRecyclable));

                auto isPinned = m_containerManager.GetIsContainerPinned(spCandidate);

                if (hasFocus)
                {
                    // yep, this one has focus. let's not use it.
                    // potentially tough situation: what if the focused container is the only item in a group
                    // and we move it out of the validcontainers, and then find that the next container (in the next)
                    // group won't be recycled? Now we're left with a header that has not been cleaned up.
                    // So we store the focused container and _only_ move it if we find that we want to cleanup
                    // the next container

                    spFocusedContainer = spCandidate;
                    focusedPosition = currentPosition;
                }
                else if(!isRecyclable &&
                        !isPinned)
                {
                    // pin containers which are not pinned yet but should
                    IFC(m_containerManager.RegisterPinnedContainer(
                        m_containerManager.GetItemIndexFromValidIndex(indexRecycleCandidateInValidContainers),
                        spCandidate));
                    isPinned = true;
                }

                if (isPinned ||
                    hasFocus)
                {
                    // for non-eligible candidates try the next one.
                    ++currentPosition;
                    indexRecycleCandidateInValidContainers += increment;
                }
                else
                {
                    // The container at the current position is not in the realization window and does not have focus
                    // Therefore it is a real candidate

                    if (spRecyclingContext)
                    {
                        // We have to check that the candidate is compatible with the context (typically its DataTemplate)
                        BOOLEAN isCompatible = TRUE;

                        IFC(spRecyclingContext->IsCompatible(spCandidate.Get(), &isCompatible));
                        if (isCompatible)
                        {
                            found = TRUE;
                            foundPosition = currentPosition;
                        }
                        else
                        {
                            // Just store the current position in case we need to fallback to a container with a wrong template
                            if (fallbackPosition == -1)
                            {
                                fallbackPosition = currentPosition;
                            }

                            // And try next container
                            indexRecycleCandidateInValidContainers += increment;
                            currentPosition++;
                        }
                    }
                    else
                    {
                        // No other constraint, let's take the current candidate
                        foundPosition = currentPosition;
                        found = TRUE;
                    }
                }
            }
        }

        if ((!found) && spRecyclingContext)
        {
            // The queue has no valid container and we don't have one either
            UINT queueLength;
            IFC(m_icg2->GetQueueLength(&queueLength));
            // We won't take a "wrong DataTemplate" container from the queue in order to keep chances
            // for next search to succeed
            // But if we have many containers outside the virtualization Windows, let's recycle one
            if ((queueLength < QueueLengthBeforeFallback) && (fallbackPosition != -1) && (currentPosition >= ValidContainersBeforeFallback))
            {
                found = TRUE;
                foundPosition = fallbackPosition;
            }
            // Else we will create a new container or it will be taken from the queue (but with a template change)
        }

        if (found)
        {
            // We need to recycle containers beetween positions 0 and foundPosition (included)
            // except the possible pinned/focused element which is handled differenty

            // If we are starting from front, then we won't have to increment the
            // current index because removing an item will slide others
            // But if we are starting from end, then removing an item will not change the indexes
            INT32 increment2 = fromFront ? 0 : -1;
            INT32 focusedIndexInValidContainers = -1;
            BOOLEAN recycled = FALSE;

            indexRecycleCandidateInValidContainers = indexFirstRecycleCandidateInValidContainers;

            for (int i = 0; i <= foundPosition; i++, indexRecycleCandidateInValidContainers += increment2)
            {
                // If this is the focused element, just skip it
                if (i == focusedPosition)
                {
                    focusedIndexInValidContainers = indexRecycleCandidateInValidContainers;

                    if (fromFront)
                    {
                        // We don't remove the focused container yet, therefore we have to
                        // increment the index
                        indexRecycleCandidateInValidContainers++;
                    }

                    continue;
                }
                IFC(m_containerManager.GetContainerAtValidIndex(indexRecycleCandidateInValidContainers, &spCandidate));

                // we want to guarantee that there are no headers after this container, since
                // that would break 'edge' assumptions: we need to be able to trust that when we
                // get the last header, it can be treated as the 'edge' and generation can start
                // from that edge. So there should not be a header after a group that has a few
                // containers left to be recycled.
                INT32 groupIndex = 0;
                INT32 indexInGroup = 0;
                INT32 countInGroup = 0;

                bool groupsAreRecycled = FALSE;

                if (m_cacheManager.IsGrouping())
                {
                    // do this before recycling the container
                    IFC(m_cacheManager.GetGroupInformationFromItemIndex(
                        m_containerManager.GetItemIndexFromValidIndex(indexRecycleCandidateInValidContainers),
                        &groupIndex,
                        &indexInGroup,
                        &countInGroup));
                }

                // If we have just passed the focused container, this is the time to move out that container as well.
                // Note that "positions" are always counted up
                if ((spFocusedContainer) && (i == focusedPosition + 1))
                {
                    // this assumes we have cleaned up before
                    // this will make sure that the focused container is 'pinned' and won't be recycled, but still
                    // is not in the valid container range
                    IFC(m_containerManager.SetFocusedContainer(
                        m_containerManager.GetItemIndexFromValidIndex(focusedIndexInValidContainers),
                        spFocusedContainer));

                    IFC(m_containerManager.RemoveFromValidContainers(focusedIndexInValidContainers, FALSE /* isForItemRemoval */, index));

                    // If we are moving forward, we need to adjust the current index as we have just removed an item
                    if (fromFront)
                    {
                        indexRecycleCandidateInValidContainers--;
                    }
                }

                // A header won't recycle if it has containers, so we need to recycle the container first;
                // this means we need to be absolutely sure that we recycle the headers after this.
                // Note that TryRecycleContainer will no-op if the container is pinned.
                IFC(m_icg2->TryRecycleContainer(spCandidate.Get(), &recycled));
                IFC(m_containerManager.RemoveFromValidContainers(indexRecycleCandidateInValidContainers, FALSE /* isForItemRemoval */, index));

                if (m_cacheManager.IsGrouping())
                {
                    // now make sure that headers are cleaned up appropriately
                    if (fromFront)
                    {
                        // recycling any container means we need to make sure
                        // that any previous groups are recycled.
                        // if we removed the last container in a group, we will also recycle that header
                        INT32 firstGroupIndexGoal = indexInGroup == countInGroup -1 ? groupIndex + 1 : groupIndex;
                        INT32 firstCurrentGroupIndex = m_containerManager.GetGroupIndexFromValidIndex(0);
                        groupsAreRecycled = firstCurrentGroupIndex >= firstGroupIndexGoal;

                        while(!groupsAreRecycled)
                        {
                            // we specify that we always want to recycle, whether there is a recycle queue or not
                            // since this is about keeping our contiguousness promises
#ifdef DBG
                            INT32 validHeaderCount = m_containerManager.GetValidHeaderCount();
#endif
                            IFC(EnsureRecycleHeaderCandidate(TRUE, TRUE /* always recycle */));

#ifdef DBG
                            // Make sure we recycled
                            ASSERT(validHeaderCount > m_containerManager.GetValidHeaderCount());
#endif

                            if (firstCurrentGroupIndex < m_containerManager.GetGroupIndexFromValidIndex(0))
                            {
                                // we actually were able to recycle that header
                                firstCurrentGroupIndex = m_containerManager.GetGroupIndexFromValidIndex(0);
                                groupsAreRecycled = firstCurrentGroupIndex >= firstGroupIndexGoal;
                            }
                            else
                            {
                                break;
                            }
                        }
                    }
                    else
                    {
                        INT32 lastGroupIndexGoal = indexInGroup == 0 ? groupIndex -1 : groupIndex;
                        INT32 lastCurrentGroupIndex = m_containerManager.GetGroupIndexFromValidIndex(m_containerManager.GetValidHeaderCount()-1);
                        groupsAreRecycled = lastCurrentGroupIndex <= lastGroupIndexGoal;
                        while (!groupsAreRecycled)
                        {
                            // we specify that we always want to recycle, whether there is a recycle queue or not
                            // since this is about keeping our contiguousness promises
#ifdef DBG
                            INT32 validHeaderCount = m_containerManager.GetValidHeaderCount();
#endif
                            IFC(EnsureRecycleHeaderCandidate(FALSE, TRUE /* always recycle */));

#ifdef DBG
                            // Make sure we recycled
                            ASSERT(validHeaderCount > m_containerManager.GetValidHeaderCount());
#endif

                            if (lastCurrentGroupIndex > m_containerManager.GetGroupIndexFromValidIndex(m_containerManager.GetValidHeaderCount()-1))
                            {
                                // we actually were able to recycle that header
                                lastCurrentGroupIndex = m_containerManager.GetGroupIndexFromValidIndex(m_containerManager.GetValidHeaderCount()-1);
                                groupsAreRecycled = lastCurrentGroupIndex <= lastGroupIndexGoal;
                            }
                            else
                            {
                                break;
                            }
                        }
                    }

                    ASSERT(groupsAreRecycled);  // This should _never_ fire. if it does, we're in trouble
                }
            }
            if (recycled && spRecyclingContext)
            {
                // The last recycled container is the one we were looking for, and we have been able to recycle it
                IFC(spRecyclingContext->put_SelectedContainer(spCandidate.Get()));
            }
        }
    }

Cleanup:
    RRETURN(hr);
}

// Recycle headers if our recycle queue is empty and we have headers to recycle.
// alwaysRecycle: even if we have elements in the recyclequeue, still recycle.
_Check_return_ HRESULT ModernCollectionBasePanel::EnsureRecycleHeaderCandidate(_In_ BOOLEAN fromFront, _In_ BOOLEAN alwaysRecycle)
{
    // only ever recycle empty headers (no containers left)

    HRESULT hr = S_OK;

    BOOLEAN doRecycle = TRUE;

    // is the recycle queue filled? we might not care
    if (!alwaysRecycle)
    {
        // if we are not forced to recycle, then only recycle if the queue is empty.
        // otherwise during generation we will just take one out of the queue
        IFC(m_icg2->GetHeaderRecycleQueueEmpty(&doRecycle));
    }

    if (doRecycle && m_containerManager.GetValidHeaderCount() > 0)
    {
        INT32 indexRecycleCandidateInValidHeaders = fromFront ? 0 : m_containerManager.GetValidHeaderCount() - 1;

        // ah, there is no candidate for recycling. let's make it happen

        // can only recycle a header that is representing a group that has no valid containers
        // so we check the container at the edge and determine that it is not in this group.
        // that basically means that this group is empty and thus can be recycled.
        INT32 groupIndex = -1;

        if (m_containerManager.GetValidContainerCount() > 0)
        {
            IFC(m_cacheManager.GetGroupInformationFromItemIndex(
                m_containerManager.GetItemIndexFromValidIndex(fromFront ? 0 : m_containerManager.GetValidContainerCount() -1),
                &groupIndex,
                nullptr,
                nullptr));
        }

        if (groupIndex != m_containerManager.GetGroupIndexFromValidIndex(indexRecycleCandidateInValidHeaders))
        {
            ctl::ComPtr<IUIElement> spCandidate;

            // the first (or last) container is not in the group that we are considering recycling.
            IFC(m_containerManager.GetHeaderAtValidIndex(indexRecycleCandidateInValidHeaders, &spCandidate));

            // UGLY! We really need to generalize this notion of generalized pinning and recycling
            // But for now, this special case stays...
            while(!spCandidate)
            {
                IFC(m_containerManager.RemoveFromValidHeaders(indexRecycleCandidateInValidHeaders, FALSE /* isForGroupRemoval */));
                indexRecycleCandidateInValidHeaders = fromFront ? 0 : m_containerManager.GetValidHeaderCount() - 1;
                IFC(m_containerManager.GetHeaderAtValidIndex(indexRecycleCandidateInValidHeaders, &spCandidate));
            }

            if (alwaysRecycle || RectUtil::AreDisjoint(
                GetBoundsFromElement(spCandidate),
                m_viewportBehavior.isTracking ? GetRealizationWindowAfterViewportShift() : m_windowState.GetRealizationWindow()))
            {
                if (m_viewportBehavior.isTracking &&
                    m_viewportBehavior.type == xaml_controls::ElementType::ElementType_GroupHeader &&
                    m_containerManager.GetValidHeaderIndexFromGroupIndex(m_viewportBehavior.index) == indexRecycleCandidateInValidHeaders)
                {
                    // We are about to recycle a tracked header.
                    // Let's cancel tracking because we don't want GetTrackedElement to later fail.
                    m_viewportBehavior.isTracking = false;
                }

                // no intersection, so free to recycle
                IFC(m_icg2->RecycleHeader(spCandidate.Get()));
                IFC(m_containerManager.RemoveFromValidHeaders(indexRecycleCandidateInValidHeaders, FALSE /* isForGroupRemoval */));
            }
        }
    }

Cleanup:
    RRETURN(hr);
}

// When generating or measuring backward, this method is used when we need to set the bounds for
// a header of a non-empty group and the first container of that group haven't been realized yet.
_Check_return_ HRESULT ModernCollectionBasePanel::EstimateFloatingHeaderLocation(_In_ ctl::ComPtr<IUIElement>& spFloatingHeader)
{
    HRESULT hr = S_OK;

    if (spFloatingHeader)
    {
        // We feel it is important to first generate a header for a group and then generate containers.
        // But this means that when we generated the header, we did not yet have container for itemInGroup 0.
        // So we have placed it wrongly. When generating container for itemInGroup 0, we will have known exactly how to place and
        // there is code that will do that. If we did not end up resetting spFloatingHeader, it means that the realizationwindow
        // stopped us from generating that container, and thus we end up here.

        // Let's estimate where to place this header for now, based on the closest available item
        ctl::ComPtr<IUIElement> spReferenceContainer;
        wf::Rect headerBounds;
        UIElement::VirtualizationInformation* p_containerVirtualizationInfo = nullptr;
        UIElement::VirtualizationInformation* p_headerVirtualizationInfo = nullptr;

        ASSERT(m_containerManager.GetValidHeaderCount() > 0 && m_containerManager.GetValidContainerCount() > 0);

        p_headerVirtualizationInfo = GetVirtualizationInformationFromElement(spFloatingHeader);
        ASSERT(p_headerVirtualizationInfo->GetIsRealized());
        headerBounds = p_headerVirtualizationInfo->GetBounds();

        IFC(m_containerManager.GetContainerAtValidIndex(0, &spReferenceContainer));
        p_containerVirtualizationInfo = GetVirtualizationInformationFromElement(spReferenceContainer);

        INT32 itemIndex = m_containerManager.GetItemIndexFromValidIndex(0);
        INT32 indexInGroup = -1;
        INT32 groupIndexOfItem = -1;
        INT32 itemCountInGroup = -1;
        IFC(m_cacheManager.GetGroupInformationFromItemIndex(itemIndex, &groupIndexOfItem, &indexInGroup, &itemCountInGroup));

        ASSERT(groupIndexOfItem == m_containerManager.GetGroupIndexFromValidIndex(0)); // Make sure we are placing the header we think we should be
        ASSERT(indexInGroup != 0);              // And that we didn't already have our chance to do so

        // we need to find the distance the header would have to the first container that we _are_ showing
        // TODO: this should probably take into account the item's location in the group. For now, we just go with average group sizes
        EstimationReference headerRef = CreateDefaultEstimationReference(xaml_controls::ElementType_GroupHeader);
        headerRef.ElementIndex = m_cacheManager.DataIndexToLayoutIndex(xaml_controls::ElementType_GroupHeader, groupIndexOfItem);
        headerRef.ElementBounds = p_headerVirtualizationInfo->GetBounds();

        EstimationReference containerRef = CreateDefaultEstimationReference(xaml_controls::ElementType_ItemContainer);
        containerRef.ElementIndex = itemIndex;
        containerRef.ElementBounds = p_containerVirtualizationInfo->GetBounds();

        IFC(m_spLayoutStrategy->EstimateElementBounds(
            xaml_controls::ElementType_GroupHeader,
            headerRef.ElementIndex,
            headerRef,
            containerRef,
            m_windowState.GetRealizationWindow(),
            &headerBounds));

        // since we called a grouping method, the non-virtualizingdirection has been nulled out probably.
        p_headerVirtualizationInfo->SetBounds(headerBounds);
    }

Cleanup:
    RRETURN(hr);
}

IFACEMETHODIMP ModernCollectionBasePanel::ArrangeOverride(
    _In_ wf::Size finalSize,
    _Out_ wf::Size* returnValue)
{
    ctl::ComPtr<wfc::IVector<xaml::UIElement*>> spChildren;
    UINT childrenCount = 0;
    wf::Rect adjustedVisibleWindow = {};
    BOOLEAN isIgnoring = FALSE;
    bool forceScrollViewerUpdate = false; // Header's size change may require the SV to be updated

    auto spScrollViewer = m_wrScrollViewer.AsOrNull<IScrollViewer>();

    auto guard = wil::scope_exit([this]()
    {
        VERIFYHR(put_IsIgnoringTransitions(FALSE));
    });

    if (!IsItemsHostRegistered())
    {
        return S_OK;
    }

    if (spScrollViewer)
    {
        IFC_RETURN(get_IsIgnoringTransitions(&isIgnoring));
        if (!isIgnoring && spScrollViewer.Cast<ScrollViewer>()->IsInManipulation())
        {
            // make sure not to setup transitions during DM
            IFC_RETURN(put_IsIgnoringTransitions(TRUE));
            isIgnoring = TRUE;
        }

        if (IsMaintainViewportSupportedAndEnabled())
        {
            DOUBLE newHeaderHeight = 0.0;
            IFC_RETURN(spScrollViewer.Cast<ScrollViewer>()->get_HeaderHeight(&newHeaderHeight));
            if (!DoubleUtil::AreClose(m_currentHeaderHeight, newHeaderHeight))
            {
                // Store the new value (which will be used to update sticky headers curves)
                m_currentHeaderHeight = newHeaderHeight;
                // And force the update of the ScrollViewer if we are not at the top
                // of the list
                forceScrollViewerUpdate = (m_windowState.GetVisibleWindow().Y > 0.0);
            }
        }
    }

    // At this point we won't have gotten any ViewChanging or ViewChanged events, so we need to get a window ourselves.
    IFC_RETURN(PrimeVisibleWindow(true /* allowTrackingOnViewportSizeChange */));
    IFC_RETURN(SetRealizationWindowFromVisibleWindow(FALSE));

    INT oldFirstVisibleIndexBase = -1;
    INT oldLastVisibleIndexBase = -1;

    IFC_RETURN(get_FirstVisibleIndexBase(&oldFirstVisibleIndexBase));
    IFC_RETURN(get_LastVisibleIndexBase(&oldLastVisibleIndexBase));

    IFC_RETURN(EndTrackingFirstVisibleElement());

    // Allow us to avoid ambiguity due to edge overlay between elements.
    adjustedVisibleWindow = m_windowState.GetVisibleWindow();
    IFC_RETURN(CoerceWindowToExtent(adjustedVisibleWindow));

    adjustedVisibleWindow.*PointFromRectInVirtualizingDirection() += EdgeOverlayDisambiguationDelta;
    adjustedVisibleWindow.*SizeFromRectInVirtualizingDirection() -= EdgeOverlayDisambiguationDelta * 2;
    if (adjustedVisibleWindow.*SizeFromRectInVirtualizingDirection() < 0)
    {
        adjustedVisibleWindow.*SizeFromRectInVirtualizingDirection() = 0;
    }
    // make sure that the non-virtualizing direction is not taken into account
    adjustedVisibleWindow.*PointFromRectInNonVirtualizingDirection() = 0;

    // going to do 3 walks: headers, containers and garbage
    // not checking for sentinels because they should have been removed during measure
    IFC_RETURN(ArrangeElements(xaml_controls::ElementType_GroupHeader, finalSize, adjustedVisibleWindow));

    // Protected by Apiset : TRUE only for Vertical Orientation
    if (m_bUseStickyHeaders)
    {
        adjustedVisibleWindow.Y += m_lastVisibleWindowClippingHeight;
        adjustedVisibleWindow.Height -= m_lastVisibleWindowClippingHeight;
    }

    IFC_RETURN(ArrangeElements(xaml_controls::ElementType_ItemContainer, finalSize, adjustedVisibleWindow));

    IFC_RETURN(m_cacheManager.GetChildren(&spChildren));
    IFC_RETURN(spChildren->get_Size(&childrenCount));

    for (UINT garbageIndex = m_containerManager.StartOfGarbageSection(); garbageIndex < childrenCount; ++garbageIndex)
    {
        ctl::ComPtr<IUIElement> spCurrentElement;
        wf::Rect arrangedBounds;
        IFC_RETURN(spChildren->GetAt(garbageIndex, &spCurrentElement));

        // Gets the arrange bounds for the element.
        IFC_RETURN(m_spLayoutStrategy->GetElementArrangeBounds(
            GetElementIsHeader(spCurrentElement) ?
                xaml_controls::ElementType_GroupHeader :
                xaml_controls::ElementType_ItemContainer,
            -1,
            GetBoundsFromElement(spCurrentElement),
            adjustedVisibleWindow,
            finalSize,
            &arrangedBounds));

        // make sure transitions do not play.
        IFC_RETURN(CoreImports::UIElement_SetIsEntering(spCurrentElement.Cast<UIElement>()->GetHandle(), FALSE));

        IFC_RETURN(CoreImports::UIElement_SetCurrentTransitionLocation(
            spCurrentElement.Cast<UIElement>()->GetHandle(),
            arrangedBounds.X,
            arrangedBounds.Y,
            arrangedBounds.Width,
            arrangedBounds.Height));

        IFC_RETURN(spCurrentElement->Arrange(arrangedBounds));
    }

    // use these values to update the panning direction
    XINT16 tick = 0;
    IFC_RETURN(CoreImports::LayoutManager_GetLayoutTickForTransition(this->GetHandle(), &tick));
    if (tick != m_windowState.currentTickNumber)
    {
        INT32 previousVisibleIndex = -1;
        // aha, the tick has increased, we can now set values
        IFC_RETURN(get_FirstVisibleIndexBase(&previousVisibleIndex));
        m_windowState.firstVisibleItemIndexOfPreviousTick = previousVisibleIndex;
        m_windowState.currentTickNumber = tick;

        IFC_RETURN(put_PanningDirectionBase(xaml_controls::PanelScrollingDirection::PanelScrollingDirection_None));
    }

    INT newFirstVisibleIndex = -1;
    INT newLastVisibleIndex = -1;

    IFC_RETURN(get_FirstVisibleIndexBase(&newFirstVisibleIndex));
    IFC_RETURN(get_LastVisibleIndexBase(&newLastVisibleIndex));

    // compare them
    if (m_windowState.firstVisibleItemIndexOfPreviousTick > -1 && m_windowState.firstVisibleItemIndexOfPreviousTick != newFirstVisibleIndex)
    {
        IFC_RETURN(put_PanningDirectionBase(
            m_windowState.firstVisibleItemIndexOfPreviousTick < newFirstVisibleIndex ?
            xaml_controls::PanelScrollingDirection::PanelScrollingDirection_Forward : xaml_controls::PanelScrollingDirection::PanelScrollingDirection_Backward));
    }

    if (oldFirstVisibleIndexBase != newFirstVisibleIndex ||
        oldLastVisibleIndexBase != newLastVisibleIndex)
    {
        VisibleIndicesUpdatedEventSourceType* pEventSource = nullptr;

        IFC_RETURN(GetVisibleIndicesUpdatedEventSourceNoRef(&pEventSource));
        IFC_RETURN(pEventSource->Raise(ctl::as_iinspectable(this), nullptr));
    }


    if (m_bUseStickyHeaders)
    {
        int firstVisibleGroupIndex = -1;
        IFC_RETURN(get_FirstVisibleGroupIndexBase(&firstVisibleGroupIndex));
        if (firstVisibleGroupIndex != -1)
        {
            const INT firstVisibleHeaderValidIndex = m_containerManager.GetValidHeaderIndexFromGroupIndex(firstVisibleGroupIndex);

            // walk backwards from first visible group index.
            for (auto validHeaderIndex = firstVisibleHeaderValidIndex;
                validHeaderIndex >= 0;
                --validHeaderIndex)
            {

                int groupStartItemIndex = -1;
                int groupItemCount = -1;

                ASSERT(m_containerManager.IsValidHeaderIndexWithinBounds(validHeaderIndex));
                const int groupDataIndex = m_containerManager.GetGroupIndexFromValidIndex(validHeaderIndex);
                IFC_RETURN(m_cacheManager.GetGroupInformationFromGroupIndex(groupDataIndex, &groupStartItemIndex, &groupItemCount));

                bool isGamepadFocusCandidate = true;
                // empty group headers are always candidates
                if (groupItemCount != 0)
                {
                    if (groupDataIndex == firstVisibleGroupIndex)
                    {
                        // if the group header and its first item are in the visible viewport. The
                        // header is a candidate. If the first item is not visible, we should not be able
                        // to focus on the header.
                        isGamepadFocusCandidate = (groupStartItemIndex >= newFirstVisibleIndex);
                    }
                    else
                    {
                        // non-empty headers above the first visible group are not candidates
                        isGamepadFocusCandidate = false;
                    }
                }

                if (!isGamepadFocusCandidate)
                {
                    // not a focus candidate
                    ctl::ComPtr<IUIElement> spHeader;
                    IFC_RETURN(m_containerManager.GetHeaderAtValidIndex(validHeaderIndex, &spHeader));
                    IFC_RETURN(spHeader.Cast<UIElement>()->put_IsGamepadFocusCandidate(isGamepadFocusCandidate));
                }
            }
        }
    }

    IFC_RETURN(UpdateSnapPoints());

    // We should have processed our command by now.
    ASSERT(m_windowState.m_command == nullptr);

    // At this point, we have executed our window management command, and should be safe to remove it.
    m_windowState.m_command = nullptr;

    // If we have a coerced window, let's send the new offsets to the ScrollViewer
    // This will trigger viewChanged/Changing events. If the new window coincides
    // with the desired coerced window, no measure invalidation should happen in our handlers.
    // If for some reason, it is different because the ScrollViewer couldn't honor our request,
    // the measure invalidate will occur and will get another chance to perform layout at
    // the final window location.
    if (m_windowState.HasCoercedWindow())
    {
        bool issued = false;
        IFC_RETURN(SetScrollViewerOffsetsTo(m_windowState.GetCoercedVisibleWindow(), false /*animate*/, &issued));

        // Clear out the coerced window so our next layout pass can start from the raw scroll state
        // only clear out the coerced window if we were succesful scrolling to the offset
        if (issued)
        {
            m_windowState.ClearCoercedWindow();
        }
    }
    else if (forceScrollViewerUpdate)
    {
        bool handled = false;
        // Force the ScrollViewer to display the correct window
        IFC_RETURN(SetScrollViewerOffsetsTo(m_windowState.GetVisibleWindow(), false /*animate*/, &handled));
    }

    m_windowState.CorrectVisibleWindowForMouseLargeClick(this);

    IFC_RETURN(TraceVirtualizationData());

    // Alignment is being taken into account on the core. That means that the DesiredSize that you
    // returned in Measure is taken as an input and compared to the AlignmentProperties. If you have
    // alignment of stretch in the non-virtualizing direction, you get that final size and you have to
    // deal with it. If you have any other alignment, you will get your desiredsize.
    // In our case, we just happen to start our elements from an imaginary (0,0) point. So in the case of
    // stretch, where we get more than needed, we could place the elements somewhere in between. However,
    // we do not do that, since we feel adding childrenalignment is not needed to position children:
    // you can tweak the alignment of the panel itself. PM felt that background on the panel is not too
    // widely used.

    // Returning a bigger size than actually used is absolutely possible and is needed to trigger the core
    // to setup layout clipping. So we should never lie to layout.

    // the desiredsize is the output of GetPanelExtent. Instead of rerunning that command (which does a loop), just read
    // it back.
    wf::Size result;
    IFC_RETURN(get_DesiredSize(&result));

    // and then account for stretch: if we need more size than stretch can give us, happily return that. Otherwise act as though
    // we took up that space and be happy with our implicit children alignment of top-left.
    result.Width = std::max(result.Width, finalSize.Width);
    result.Height = std::max(result.Height, finalSize.Height);

    *returnValue = result;

    // inform the host that virtualization is finished
    if (IsItemsHostRegistered())
    {
        ctl::ComPtr<IGeneratorHost> spItemsHost;

        IFC_RETURN(m_cacheManager.GetItemsHost(&spItemsHost));
        IFC_RETURN(spItemsHost->VirtualizationFinished());
    }

    // We track the unloading elements for the duration of one layout pass only.
    // The reason for that is that we currently only need them for viewport tracking
    // in order to adjust theme transitions.
    IFC_RETURN(m_unloadingElements->Clear());

    guard.release();

    IFC_RETURN(put_IsIgnoringTransitions(FALSE));

    return S_OK;
}

// returns an array of indices of all pinned elements by type
_Check_return_ HRESULT ModernCollectionBasePanel::GetPinnedElementsIndexVector(
    _In_ xaml_controls::ElementType type,
    _Out_ std::vector<unsigned int>* pReturnValue)
{
    RRETURN(m_containerManager.GetPinnedElementsIndexVector(type, pReturnValue));
}

// Arrange all elements of a given type
_Check_return_ HRESULT DirectUI::ModernCollectionBasePanel::ArrangeElements(
    _In_ xaml_controls::ElementType type,
    _In_ const wf::Size& finalSize,
    _In_ const wf::Rect& adjustedVisibleWindow)
{
    INT32 firstCacheIndex = -1;
    INT32 firstVisibleIndex = -1;
    INT32 lastVisibleIndex = -1;
    INT32 lastCacheIndex = -1;

    // While we arrange group headers, we need to account for sticky headers
    // when deciding what group header is visible first. We also calculate
    // how much height sticky headers are currently taking away from the
    // visible window. Using this value, we make sure the first visible item
    // we pick is not completely clipped away by sticky headers.
    // This will allow us to get an accurate value for the FirstVisibleIndex and
    // FirstVisibleGroupIndex properties.
    FLOAT heightOfActiveSickyHeader = 0.0f;
    FLOAT offsetOfFirstHeaderAfterStickyHeader = std::numeric_limits<FLOAT>::infinity();

    const auto elementCount = m_containerManager.GetValidElementCount(type);

    // for item containers (where LiveReorder is only supported), we get the items host
    // to query for the bounds if an arrange is happening during a LiveReorder
    ctl::ComPtr<IGeneratorHost> spItemsHost;
    if (type == xaml_controls::ElementType_ItemContainer &&
        IsItemsHostRegistered())
    {
        IFC_RETURN(m_cacheManager.GetItemsHost(&spItemsHost));
    }

    // Clear out old arrange rects
    BOOLEAN needsElementRects = FALSE;
    IFC_RETURN(m_spLayoutStrategy->HasIrregularSnapPoints(type, &needsElementRects));

    m_arrangeRects[type].clear();
    if (needsElementRects)
    {
        m_arrangeRects[type].reserve(elementCount);
    }

    for (INT32 validElementIndex = 0; validElementIndex < elementCount; ++validElementIndex)
    {
        ctl::ComPtr<IUIElement> spCurrentElement;

        IFC_RETURN(m_containerManager.GetElementAtValidIndex(type, validElementIndex, &spCurrentElement));
        if (spCurrentElement)
        {
            wf::Rect arrangedBounds;
            auto dataIndex = m_containerManager.GetDataIndexFromValidIndex(type, validElementIndex);
            auto layoutIndex = m_cacheManager.DataIndexToLayoutIndex(type, dataIndex);

            IFC_RETURN(m_spLayoutStrategy->GetElementArrangeBounds(
                type,
                layoutIndex,
                GetBoundsFromElement(spCurrentElement),
                adjustedVisibleWindow,
                finalSize,
                &arrangedBounds));

            auto insideVisibleWindow = !RectUtil::AreDisjoint(arrangedBounds, adjustedVisibleWindow);

            // in the case of a first arrange, our window will have dimensions of 0. In this case though,
            // insideVisibileWindow will be false for the first container that comes after a header. We'll
            // come back to it soon when a next arrange comes in with actual window dimensions, but transitions won't
            // be able to be setup any more after that since arrange won't be dirty anymore for that container.
            // So when we see a window without proper dimensions, we are just going to assume we are inside the Visible Window
            if (adjustedVisibleWindow.*SizeFromRectInVirtualizingDirection() < 5.0)
            {
                insideVisibleWindow = true;
            }

            if (firstCacheIndex == -1)
            {
                firstCacheIndex = dataIndex;
            }
            if (firstVisibleIndex == -1 && insideVisibleWindow)
            {
                firstVisibleIndex = dataIndex;
            }
            if (lastVisibleIndex < dataIndex && insideVisibleWindow)
            {
                lastVisibleIndex = dataIndex;
            }
            if (lastCacheIndex < dataIndex)
            {
                lastCacheIndex = dataIndex;
            }


            if (!insideVisibleWindow)
            {
                // We don't play transitions only if the container isn't in the visible window
                // and didn't use to in the previous layout pass either.
                CUIElement* container = spCurrentElement.Cast<UIElement>()->GetHandle();
                const auto currentOffset = container->GetLayoutStorage()->m_offset;
                const auto currentSize = container->GetLayoutStorage()->m_size;
                const bool wasInsideVisibleWindow =
                    currentSize.width != 0.0f &&
                    currentSize.height != 0.0f &&
                    !RectUtil::AreDisjoint(
                        adjustedVisibleWindow,
                        wf::Rect{ currentOffset.x, currentOffset.y, currentSize.width, currentSize.height });

                if (wasInsideVisibleWindow == false)
                {
                    // Make sure transitions do not play
                    IFC_RETURN(CoreImports::UIElement_SetIsEntering(spCurrentElement.Cast<UIElement>()->GetHandle(), FALSE));
                    IFC_RETURN(CoreImports::UIElement_CancelTransition(spCurrentElement.Cast<UIElement>()->GetHandle()));

                    IFC_RETURN(CoreImports::UIElement_SetCurrentTransitionLocation(
                        spCurrentElement.Cast<UIElement>()->GetHandle(),
                        arrangedBounds.X,
                        arrangedBounds.Y,
                        arrangedBounds.Width,
                        arrangedBounds.Height));
                }
            }

            // query the host for arrange bounds
            // during LiveReorder, containers already have a bound to go to
            if (spItemsHost)
            {
                IFC_RETURN(spItemsHost->OverrideContainerArrangeBounds(dataIndex, arrangedBounds, &arrangedBounds));
            }
            IFC_RETURN(spCurrentElement->Arrange(arrangedBounds));

            // After arranging the element, if it is a sticky header go ahead and set bounds on the
            // sticky header wrapper - which will position the LTE if required. Note that this has to be done
            // after arrange since SetDesiredBounds looks up the ActualOffsetX from the element which is only valid
            // after arrange.
            if (type == xaml_controls::ElementType_GroupHeader && m_bUseStickyHeaders)
            {
                if (arrangedBounds.Y <= adjustedVisibleWindow.Y)
                {
                    firstVisibleIndex = dataIndex;
                    heightOfActiveSickyHeader = arrangedBounds.Height;
                }
                else if (offsetOfFirstHeaderAfterStickyHeader == std::numeric_limits<FLOAT>::infinity())
                {
                    offsetOfFirstHeaderAfterStickyHeader = arrangedBounds.Y;
                }

                if (GetVirtualizationInformationFromElement(spCurrentElement)->GetIsSticky())
                {
                    IFC_RETURN(GetVirtualizationInformationFromElement(spCurrentElement)->m_stickyHeaderWrapper->SetDesiredBounds(arrangedBounds));

                    // Start by considering all sticky headers as valid candidates for auto-focus. At the end of arrange
                    // we look at the first visible header and decide if we want to set the property to FALSE.
                    ctl::ComPtr<UIElement> stickyHeader = spCurrentElement.Cast<UIElement>();
                    IFC_RETURN(stickyHeader->put_IsGamepadFocusCandidate(TRUE));
                }
            }

            if (needsElementRects)
            {
                BOOLEAN hasSnapPointOnElement = FALSE;
                IFC_RETURN(m_spLayoutStrategy->HasSnapPointOnElement(type, layoutIndex, &hasSnapPointOnElement));
                if (hasSnapPointOnElement)
                {
                    ASSERT(m_arrangeRects[type].size() < m_arrangeRects[type].capacity());
                    m_arrangeRects[type].push_back(arrangedBounds);
                }
            }
        }
    }

    // Now that all the headers have been arranged, let's make sure their sticky curves get updated
    if (type == xaml_controls::ElementType_GroupHeader && m_bUseStickyHeaders)
    {
        IFC_RETURN(UpdateStickyHeaders(finalSize));

        // A stickied header may affect the set of visible group headers
        // May happen when, for example, looking at the middle of a large group.
        // The sticky header will be the first and last visible group header in that case.
        if (lastVisibleIndex == -1 && firstVisibleIndex != -1)
        {
            lastVisibleIndex = firstVisibleIndex;
        }

        // The sticky header may obscure an item, so we need to account for that when determining which items are in the visible window
        m_lastVisibleWindowClippingHeight = std::min(heightOfActiveSickyHeader, offsetOfFirstHeaderAfterStickyHeader - m_windowState.GetVisibleWindow().Y);
    }

    switch (type)
    {
    case xaml_controls::ElementType_GroupHeader:
        IFC_RETURN(put_FirstCacheGroupIndexBase(firstCacheIndex));
        IFC_RETURN(put_FirstVisibleGroupIndexBase(firstVisibleIndex));
        IFC_RETURN(put_LastVisibleGroupIndexBase(lastVisibleIndex));
        IFC_RETURN(put_LastCacheGroupIndexBase(lastCacheIndex));
        break;

    case xaml_controls::ElementType_ItemContainer:
        IFC_RETURN(put_FirstCacheIndexBase(firstCacheIndex));
        IFC_RETURN(put_FirstVisibleIndexBase(firstVisibleIndex));
        IFC_RETURN(put_LastVisibleIndexBase(lastVisibleIndex));
        IFC_RETURN(put_LastCacheIndexBase(lastCacheIndex));
        break;
    }

    return S_OK;
}

// Output data desired by ETW tracing
_Check_return_ HRESULT ModernCollectionBasePanel::TraceVirtualizationData()
{
    HRESULT hr = S_OK;

    // Put these checks into separate variables to deal with a prefast error for
    // warning 6287: "redundant code: the left and right sub-expressions are identical"
    bool eventEnabledVirtualizedCollectionUpdatedInfo = EventEnabledVirtualizedCollectionUpdatedInfo();
    bool eventEnabledVirtualizedCollectionBoundsInfo = EventEnabledVirtualizedCollectionBoundsInfo();

    if (eventEnabledVirtualizedCollectionUpdatedInfo || eventEnabledVirtualizedCollectionBoundsInfo)
    {
        auto spScrollViewer = m_wrScrollViewer.AsOrNull<xaml_controls::IScrollViewer>();
        if (spScrollViewer)
        {
            wf::Rect contentBounds = { std::numeric_limits<FLOAT>::infinity(), std::numeric_limits<FLOAT>::infinity(), 0, 0 };
            wf::Rect realizedItemBounds = { std::numeric_limits<FLOAT>::infinity(), std::numeric_limits<FLOAT>::infinity(), 0, 0 };
            bool hasPlaceholders = false;

            DOUBLE viewportWidth = 0;
            DOUBLE viewportHeight = 0;
            DOUBLE extentWidth = 0;
            DOUBLE extentHeight = 0;

            IFC(spScrollViewer->get_ViewportWidth(&viewportWidth));
            IFC(spScrollViewer->get_ViewportHeight(&viewportHeight));
            IFC(spScrollViewer->get_ExtentWidth(&extentWidth));
            IFC(spScrollViewer->get_ExtentHeight(&extentHeight));

            for (INT typeIndex = 0; typeIndex < ElementType_Count; ++typeIndex)
            {
                const xaml_controls::ElementType type = static_cast<xaml_controls::ElementType>(typeIndex);
                for (INT validIndex = 0; validIndex < m_containerManager.GetValidElementCount(type); ++validIndex)
                {
                    ctl::ComPtr<IUIElement> spElement;
                    IFC(m_containerManager.GetElementAtValidIndex(type, validIndex, &spElement));
                    if (spElement)
                    {
                        auto bounds = GetBoundsFromElement(spElement);

                        if (eventEnabledVirtualizedCollectionUpdatedInfo)
                        {
                            contentBounds.X = std::min(bounds.X, contentBounds.X);
                            contentBounds.Y = std::min(bounds.Y, contentBounds.Y);
                            contentBounds.Width = std::max(bounds.X + bounds.Width - contentBounds.X, contentBounds.Width);
                            contentBounds.Height = std::max(bounds.Y + bounds.Height - contentBounds.Y, contentBounds.Height);
                        }

                        if (type == xaml_controls::ElementType_ItemContainer && eventEnabledVirtualizedCollectionBoundsInfo)
                        {
                            auto spSelectorItem = spElement.AsOrNull<xaml_primitives::ISelectorItem>();
                            bool isPlaceholder = spSelectorItem && spSelectorItem.Cast<SelectorItem>()->GetIsUIPlaceholder();

                            hasPlaceholders = isPlaceholder || hasPlaceholders;
                            if (!isPlaceholder)
                            {
                                realizedItemBounds.X = std::min(bounds.X, realizedItemBounds.X);
                                realizedItemBounds.Y = std::min(bounds.Y, realizedItemBounds.Y);
                                realizedItemBounds.Width = std::max(bounds.X + bounds.Width - realizedItemBounds.X, realizedItemBounds.Width);
                                realizedItemBounds.Height = std::max(bounds.Y + bounds.Height - realizedItemBounds.Y, realizedItemBounds.Height);
                            }
                        }
                    }
                }
            }

            TraceVirtualizedCollectionUpdatedInfo(
                (UINT64)spScrollViewer.Cast<ScrollViewer>()->GetHandle(),
                DoubleUtil::IsPositiveInfinity(contentBounds.X) ? 0 : contentBounds.X,
                DoubleUtil::IsPositiveInfinity(contentBounds.Y) ? 0 : contentBounds.Y,
                contentBounds.Width,
                contentBounds.Height,
                static_cast<FLOAT>(viewportWidth),
                static_cast<FLOAT>(viewportHeight),
                static_cast<FLOAT>(extentWidth),
                static_cast<FLOAT>(extentHeight),
                hasPlaceholders
                );

            if (hasPlaceholders)
            {
                TraceVirtualizedCollectionBoundsInfo(
                    (UINT64)spScrollViewer.Cast<ScrollViewer>()->GetHandle(),
                    DoubleUtil::IsPositiveInfinity(realizedItemBounds.X) ? 0 : realizedItemBounds.X,
                    DoubleUtil::IsPositiveInfinity(realizedItemBounds.Y) ? 0 : realizedItemBounds.Y,
                    realizedItemBounds.Width,
                    realizedItemBounds.Height
                    );
            }
        }
    }

Cleanup:
    RRETURN(hr);
}

// Creates a default layout reference.
xaml_controls::LayoutReference
ModernCollectionBasePanel::CreateDefaultLayoutReference() const
{
    return {};
}

// Creates a default (aka invalid) estimation reference.
xaml_controls::EstimationReference
ModernCollectionBasePanel::CreateDefaultEstimationReference(
    _In_ xaml_controls::ElementType elementType) const
{
    EstimationReference reference = {};
    reference.ElementType = elementType;
    reference.ElementIndex = -1;
    reference.ElementBounds = {};
    return reference;
}

_Check_return_ HRESULT DirectUI::ModernCollectionBasePanel::OnPanelUnloaded(
    _In_ IInspectable* pSender,
    _In_ IRoutedEventArgs* pArgs )
{
    HRESULT hr = S_OK;

    IFC(DetachPanelComponents());

    // invalidate, so that we are assured of getting a measure when we are put back into the tree
    IFC(InvalidateMeasure());

Cleanup:
    RRETURN(hr);
}

// this base panel implementation is hidden from IDL
_Check_return_ HRESULT DirectUI::ModernCollectionBasePanel::QueryInterfaceImpl( _In_ REFIID iid, _Outptr_ void** ppObject )
{
    if (InlineIsEqualGUID(iid, __uuidof(IModernCollectionBasePanel)))
    {
        *ppObject = static_cast<IModernCollectionBasePanel*>(this);
    }
    else
    {
        RRETURN(ModernCollectionBasePanelGenerated::QueryInterfaceImpl(iid, ppObject));
    }

    AddRefOuter();
    RRETURN(S_OK);
}

// Scroll by one visible viewport length,
// forcing virtualization to run.
// toIncreasingIndex - Governs the direction of scroll.
_Check_return_ HRESULT ModernCollectionBasePanel::SynchronousScrollByPage(
    _In_ BOOLEAN toIncreasingIndex)
{
    HRESULT hr = S_OK;
    xaml_controls::Orientation orientation;
    wf::Rect targetRect = m_windowState.GetVisibleWindow();
    const float pageMovementFactor = toIncreasingIndex ? 1.0f : -1.0f;

    IFC(m_spLayoutStrategy->GetVirtualizationDirection(&orientation));
    if (orientation == xaml_controls::Orientation_Horizontal)
    {
        targetRect.X += pageMovementFactor * targetRect.Width;
    }
    else
    {
        targetRect.Y += pageMovementFactor * targetRect.Height;
    }

    IFC(ScrollRectIntoView(targetRect, TRUE /* forceSynchronous */));

    // When using sticky headers, adjust for the sticky header
    // overlapping over items, but moving the rect by the amount the sticky header
    // overlaps the content. This amount is the same as the amount by which the real
    // header is outside the visible window. Notice that in the case of sticky headers
    // we perfor two scroll rect into view calls because in order to calculate the adjustment
    // amount, we first need to scroll by a page, then find the sticky header and then
    // calculate.
    // true only for vertical
    if (m_bUseStickyHeaders)
    {
        FLOAT actualHeaderOutsideVisibleWindow = 0;
        IFC(GetRealHeaderExtentOutsideVisibleWindow(&actualHeaderOutsideVisibleWindow));
        targetRect.Y -= actualHeaderOutsideVisibleWindow *pageMovementFactor;
        IFC(ScrollRectIntoView(targetRect, TRUE /* forceSynchronous */));
    }

Cleanup:
    RRETURN(hr);
}

// Calculates the last visible index, from either direction.
// This is usually the last fully visible index, but if
// there is no fully visible index, this returns the last
// partly visible index.
// fromLowerIndex - direction the search starts from. I.E., with a horizontal orientation,
// fromLowerIndex = TRUE means the rightmost visible container.
_Check_return_ HRESULT ModernCollectionBasePanel::GetLastVisibleContainer(
    _In_ BOOLEAN fromLowerIndex,
    _In_ BOOLEAN searchForItems,
    _Out_ UINT* pLastVisibleItemIndex,
    _Out_opt_ BOOLEAN* pFound)
{
    HRESULT hr = S_OK;

    // Find the last/first visible index by walking each realized item from the appropriate end of the realized containers.
    INT32 idxOfFirstContainer = 0;
    INT32 idxOfLastContainer = 0;
    INT32 startingIndex = 0;
    const INT32 step = fromLowerIndex ? -1 : 1;
    INT32 firstPartlyVisibleValidIndex = 0;
    INT32 targetValidIndex = 0;
    BOOLEAN foundSomething = FALSE;
    BOOLEAN foundAtLeastOnePartlyVisibleContainer = FALSE;
    xaml_controls::Orientation virtDirection;
    wf::Rect visibleWindow = m_windowState.GetVisibleWindow();

    // adjust the visible window for sticky headers
    // this makes sure we have a more accurate index when
    // scrolling back up.
    if (m_bUseStickyHeaders)
    {
        visibleWindow.Y += m_lastVisibleWindowClippingHeight;
        visibleWindow.Height -= m_lastVisibleWindowClippingHeight;
    }

    *pLastVisibleItemIndex = 0;

    if (pFound)
    {
        *pFound = FALSE;
    }

    IFC(m_spLayoutStrategy->GetVirtualizationDirection(&virtDirection));

    idxOfLastContainer = ((searchForItems) ? m_containerManager.GetValidContainerCount() : m_containerManager.GetValidHeaderCount()) - 1;
    startingIndex = fromLowerIndex ? idxOfLastContainer : idxOfFirstContainer;

    for (INT32 currentValidIndex = startingIndex;
         currentValidIndex >= idxOfFirstContainer && currentValidIndex <= idxOfLastContainer;
         currentValidIndex += step)
    {
        ctl::ComPtr<IUIElement> spChild;
        wf::Rect childBounds = {};

        if (searchForItems)
        {
            IFC(m_containerManager.GetContainerAtValidIndex(currentValidIndex, &spChild));
        }
        else
        {
            IFC(m_containerManager.GetHeaderAtValidIndex(currentValidIndex, &spChild));
        }

        childBounds = GetBoundsFromElement(spChild);

        if (RectUtil::AreDisjoint(childBounds, visibleWindow))
        {
            // Container is completely disjoint from the visible window- go to the next container.
            continue;
        }

        // To handle containers taller than the nonvirtualizing direction, we constrain
        // the nonvirtualizing container bounds to the nonvirtualizing extent.
        if (virtDirection == xaml_controls::Orientation_Horizontal)
        {
            childBounds.Y = std::max(visibleWindow.Y, std::min(childBounds.Y, 0.0f));
            childBounds.Height = std::min(childBounds.Height, visibleWindow.Height - childBounds.Y);
        }
        else
        {
            childBounds.X = std::max(visibleWindow.X, std::min(childBounds.X, 0.0f));
            childBounds.Width = std::min(childBounds.Width, visibleWindow.Width - childBounds.X);
        }

        if (RectUtil::ContainsFully(visibleWindow, childBounds))
        {
            targetValidIndex = currentValidIndex;
            foundSomething = TRUE;
            break;
        }
        else if (foundAtLeastOnePartlyVisibleContainer)
        {
            if (RectUtil::AreDisjoint(visibleWindow, childBounds))
            {
                // We've been seeing partly visible containers, but no full intersections.
                // However, we've seen a completely invisible container. This means we're
                // at a set of two consecutive items, both of which are partly visible.
                // So, return the last partly visible container.
                foundSomething = TRUE;
                targetValidIndex = currentValidIndex;
                break;
            }
            else
            {
                // Continue searching.
            }
        }
        else
        {
            // Haven't found any partly visible containers yet.
            if (!RectUtil::AreDisjoint(visibleWindow, childBounds))
            {
                foundAtLeastOnePartlyVisibleContainer = TRUE;
                firstPartlyVisibleValidIndex = currentValidIndex;
            }
            else
            {
                // Continue searching.
            }
        }
    }

    if (foundSomething ||
        foundAtLeastOnePartlyVisibleContainer)
    {
        UINT lastVisibleIndex = (foundSomething) ? targetValidIndex : firstPartlyVisibleValidIndex;

        if (searchForItems)
        {
            ASSERT(m_containerManager.IsValidContainerIndexWithinBounds(lastVisibleIndex));
            *pLastVisibleItemIndex = m_containerManager.GetItemIndexFromValidIndex(lastVisibleIndex);
        }
        else
        {
            ASSERT(m_containerManager.IsValidHeaderIndexWithinBounds(lastVisibleIndex));
            *pLastVisibleItemIndex = m_containerManager.GetGroupIndexFromValidIndex(lastVisibleIndex);
        }

        if (pFound)
        {
            *pFound = TRUE;
        }
    }
    else
    {
        // Nothing was found in the view, so default to either first or last container in the entire collection.
        if (fromLowerIndex)
        {
            if (searchForItems)
            {
                *pLastVisibleItemIndex = m_cacheManager.GetTotalItemCount() - 1;
            }
            else
            {
                *pLastVisibleItemIndex = m_cacheManager.GetTotalGroupCount() - 1;
            }
        }
    }

Cleanup:
    RRETURN(hr);
}

// Get the closest element information to the point.
_Check_return_ HRESULT ModernCollectionBasePanel::GetClosestElementInfo(
    _In_ wf::Point position,
    _Out_ xaml_primitives::ElementInfo* returnValue)
{
    HRESULT hr = S_OK;

    // Basic idea:
    // 1- Find a reference container so we can anchor the search.
    // 1a- If grouping, try to find group containing point by walking group headers. Then, grab the first realized container within the group.
    // 1b- If not grouping, grab the first realized container
    // TODO: Optimize which realized container we use, invoking estimation APIs.
    // -- jump to layout strategy -- Given: Index of item within group + point relative to either first item in group or to header.
    // 2- Do math to figure out which column the point is in.
    // 3- Knowing column and row height, figure out closest index.

    INT32 referenceItemIndex = -1;
    INT32 referenceItemIndexInGroup = -1;
    INT32 referenceGroupIndex = 0;
    wf::Rect referenceItemBounds = {0, 0, 0, 0};

    INT32 targetItemIndex = -1;
    BOOLEAN groupWasEmpty = FALSE;
    xaml_controls::IndexSearchHint searchHint = xaml_controls::IndexSearchHint_NoHint;
    xaml_controls::IndexSearchHint previousHint = xaml_controls::IndexSearchHint_NoHint;

    returnValue->m_childIndex = -1;

    IFC(FindValidContainerInGroupForPoint(position, &groupWasEmpty, &referenceGroupIndex, &referenceItemIndex, &referenceItemIndexInGroup, &referenceItemBounds));

    if (!groupWasEmpty)
    {
        INT32 sizeOfGroup = 0;

        if (m_cacheManager.IsGrouping())
        {
            IFC(m_cacheManager.GetGroupInformationFromGroupIndex(referenceGroupIndex, nullptr /* pStartItemIndex */, &sizeOfGroup));
        }
        else
        {
            sizeOfGroup = m_cacheManager.GetTotalItemCount();
        }

        do
        {
            ctl::ComPtr<IUIElement> spContainer;
            INT32 validContainerIndex = m_containerManager.GetValidContainerIndexFromItemIndex(referenceItemIndex);

            if (m_containerManager.IsValidContainerIndexWithinBounds(validContainerIndex))
            {
                // it was within the bounds. We can try to get a container
                IFC(m_containerManager.GetContainerAtValidIndex(validContainerIndex, &spContainer));
            }
            else
            {
                // that is not within the bounds.
                // this can happen when we have just inserted a couple of elements at the beginning of the collection. There wouldn't have been
                // sentinels, and we wouldn't be getting a valid range.
            }

            // if spcontainer is null, we could be dealing with a sentinel or we've fallen outside of the valid range
            if (spContainer)
            {
                EstimationReference containerReference = CreateDefaultEstimationReference(xaml_controls::ElementType_ItemContainer);
                containerReference.ElementIndex = referenceItemIndex;
                containerReference.ElementBounds = GetBoundsFromElement(spContainer);

                xaml_controls::ElementType targetElementType;

                IFC(m_spLayoutStrategy->EstimateIndexFromPoint(
                    false /* requestingInsertionIndex */,
                    position,
                    containerReference,
                    m_windowState.GetRealizationWindow(),
                    &searchHint,
                    &targetElementType,
                    &targetItemIndex));

                ASSERT(targetElementType == xaml_controls::ElementType_ItemContainer);

                if (previousHint == xaml_controls::IndexSearchHint_NoHint)
                {
                    // initialize
                    previousHint = searchHint;
                }
            }
            else
            {
                // if we hit a sentinel or we're outside of the valid range, give up.

                // set back previous values based on the direction we are going.
                if (searchHint == xaml_controls::IndexSearchHint_SearchBackwards)
                {
                    referenceItemIndex++;
                    referenceItemIndexInGroup++;
                }
                else
                {
                    referenceItemIndex--;
                    referenceItemIndexInGroup--;
                }
                searchHint = xaml_controls::IndexSearchHint_Exact;
            }

            if (searchHint == xaml_controls::IndexSearchHint_SearchBackwards)
            {
                ASSERT(referenceItemIndex == targetItemIndex);

                if (referenceItemIndex == 0 ||
                    referenceItemIndexInGroup == 0 ||
                    !m_containerManager.IsValidContainerIndexWithinBounds(m_containerManager.GetValidContainerIndexFromItemIndex(referenceItemIndex - 1)))
                {
                    searchHint = xaml_controls::IndexSearchHint_Exact;
                }
                else
                {
                    referenceItemIndex--;
                    referenceItemIndexInGroup--;
                }

                if (previousHint == xaml_controls::IndexSearchHint_SearchForwards)
                {
                    // we hit a gap. This can happen when items are being deleted.
                    // Let's give up
                    searchHint = xaml_controls::IndexSearchHint_Exact;
                }
            }

            if (searchHint == xaml_controls::IndexSearchHint_SearchForwards)
            {
                ASSERT(referenceItemIndex == targetItemIndex);

                if (referenceItemIndexInGroup == sizeOfGroup - 1 ||
                    !m_containerManager.IsValidContainerIndexWithinBounds(m_containerManager.GetValidContainerIndexFromItemIndex(referenceItemIndex + 1)))
                {
                    searchHint = xaml_controls::IndexSearchHint_Exact;
                }
                else if (previousHint == xaml_controls::IndexSearchHint_SearchBackwards)
                {
                    // we hit a gap. This can happen when items are being deleted.
                    // Let's give up
                    searchHint = xaml_controls::IndexSearchHint_Exact;
                }
                else
                {
                    referenceItemIndex++;
                    referenceItemIndexInGroup++;
                }
            }

        } while (searchHint == xaml_controls::IndexSearchHint_SearchBackwards || searchHint == xaml_controls::IndexSearchHint_SearchForwards);

        // We don't support inexact EstimateIndexFromPoint calls yet.
        ASSERT(searchHint == xaml_controls::IndexSearchHint_Exact);

        // We get the target as a group index. Add the group start index to convert to global index.
        returnValue->m_childIndex = targetItemIndex;
    }
    else
    {
        if (m_cacheManager.IsGrouping())
        {
            returnValue->m_childIndex = referenceGroupIndex;
            returnValue->m_childIsHeader = TRUE;
        }
        else
        {
            // Means our collection was empty. Return -1.
            //Item with 0 index kind of valid and will be returned when we have count of 1.
            returnValue->m_childIndex = -1;
        }
    }

Cleanup:
    RRETURN(hr);
}

// Get the index where an item should be inserted if it were dropped at
// the given position. This will be used by live reordering.
_Check_return_ HRESULT ModernCollectionBasePanel::GetInsertionIndex(
    _In_ wf::Point position,
    _Out_ INT* pReturnValue)
{
    HRESULT hr = S_OK;

    // Basic idea:
    // 1- Find a reference container so we can anchor the search.
    // 1a- If grouping, try to find group containing point by walking group headers. Then, grab the first realized container within the group.
    // 1b- If not grouping, grab the first realized container
    // TODO: Optimize which realized container we use, invoking estimation APIs.
    // -- jump to layout strategy -- Given: Index of item within group + point relative to either first item in group or to header.
    // 2- Do math to figure out which column the point is in.
    // 3- Knowing column and row height, figure out insertion index.

    INT32 referenceItemIndex = -1;
    INT32 referenceItemIndexInGroup = -1;
    wf::Rect referenceItemBounds;
    INT32 targetItemIndex = -1;
    BOOLEAN groupWasEmpty = FALSE;
    INT32 groupIndex = 0;
    xaml_controls::IndexSearchHint searchHint = xaml_controls::IndexSearchHint_NoHint;
    xaml_controls::IndexSearchHint previousHint = xaml_controls::IndexSearchHint_NoHint;

    *pReturnValue = 0;

    IFC(FindValidContainerInGroupForPoint(position, &groupWasEmpty, &groupIndex, &referenceItemIndex, &referenceItemIndexInGroup, &referenceItemBounds));

    if (!groupWasEmpty)
    {
        INT32 sizeOfGroup = 0;

        if (m_cacheManager.IsGrouping())
        {
            IFC(m_cacheManager.GetGroupInformationFromGroupIndex(groupIndex, nullptr, &sizeOfGroup));
        }
        else
        {
            sizeOfGroup = m_cacheManager.GetTotalItemCount();
        }

        do
        {
            ctl::ComPtr<IUIElement> spContainer;
            INT32 validContainerIndex = m_containerManager.GetValidContainerIndexFromItemIndex(referenceItemIndex);

            if (m_containerManager.IsValidContainerIndexWithinBounds(validContainerIndex))
            {
                // it was within the bounds. We can try to get a container
                IFC(m_containerManager.GetContainerAtValidIndex(validContainerIndex, &spContainer));
            }
            else
            {
                // that is not within the bounds.
                // this can happen when we have just inserted a couple of elements at the beginning of the collection. There wouldn't have been
                // sentinels, and we wouldn't be getting a valid range.
            }

            // if spcontainer is null, we could be dealing with a sentinel or we've fallen outside of the valid range
            if (spContainer)
            {
                EstimationReference containerReference = CreateDefaultEstimationReference(xaml_controls::ElementType_ItemContainer);
                containerReference.ElementIndex = referenceItemIndex;
                containerReference.ElementBounds = GetBoundsFromElement(spContainer);

                xaml_controls::ElementType targetElementType;

                IFC(m_spLayoutStrategy->EstimateIndexFromPoint(
                    true /* requestingInsertionIndex */,
                    position,
                    containerReference,
                    m_windowState.GetRealizationWindow(),
                    &searchHint,
                    &targetElementType,
                    &targetItemIndex));

                ASSERT(targetElementType == xaml_controls::ElementType_ItemContainer);

                if (previousHint == xaml_controls::IndexSearchHint_NoHint)
                {
                    // initialize
                    previousHint = searchHint;
                }
            }
            else
            {
                // if we hit a sentinel or we're outside of the valid range, give up.

                // set back previous values based on the direction we are going.
                if (searchHint == xaml_controls::IndexSearchHint_SearchBackwards)
                {
                    referenceItemIndex++;
                    referenceItemIndexInGroup++;
                }
                else
                {
                    referenceItemIndex--;
                    referenceItemIndexInGroup--;
                }
                searchHint = xaml_controls::IndexSearchHint_Exact;
            }

            if (searchHint == xaml_controls::IndexSearchHint_SearchBackwards)
            {
                if (referenceItemIndex == 0)
                {
                    searchHint = xaml_controls::IndexSearchHint_Exact;
                }
                else
                {
                    referenceItemIndex--;
                    referenceItemIndexInGroup--;
                }

                if (previousHint == xaml_controls::IndexSearchHint_SearchForwards)
                {
                    // we hit a gap. This can happen when items are being deleted.
                    // Let's give up
                    searchHint = xaml_controls::IndexSearchHint_Exact;
                }
            }
            else if (searchHint == xaml_controls::IndexSearchHint_SearchForwards)
            {
                if (referenceItemIndex == m_cacheManager.GetTotalItemCount() - 1)
                {
                    searchHint = xaml_controls::IndexSearchHint_Exact;
                    // we allow insertion at the end of the list
                    ++targetItemIndex;
                }
                else if (previousHint == xaml_controls::IndexSearchHint_SearchBackwards)
                {
                    // we hit a gap. This can happen when items are being deleted.
                    // Let's give up
                    searchHint = xaml_controls::IndexSearchHint_Exact;
                }
                else
                {
                    referenceItemIndex++;
                    referenceItemIndexInGroup++;
                }
            }

        } while (searchHint == xaml_controls::IndexSearchHint_SearchBackwards || searchHint == xaml_controls::IndexSearchHint_SearchForwards);

        // Note: allow index to be == size of collection, because it's valid to insert at end of collection!
        targetItemIndex = std::min(m_cacheManager.GetTotalItemCount(), targetItemIndex);

        // We don't support inexact EstimateInsertIndexFromPoint calls yet.
        ASSERT(searchHint == xaml_controls::IndexSearchHint_Exact);

        // The target item index is already a global index.
        *pReturnValue = targetItemIndex;
    }
    else
    {
        if (m_cacheManager.IsGrouping())
        {
            // We shouldn't get here since we don't currently try to get insertion indexes with grouping enabled.
            // A possible algorithm would be to try to find the nearest group with a non-zero count, and determine
            // insertion index based on that.
            IFC(E_NOTIMPL);
        }
        else
        {
            // Means our collection was empty. Return 0.
            *pReturnValue = 0;
        }
    }

Cleanup:
    RRETURN(hr);
}

// Get the indexes where an item should be inserted if it were dropped at
// the given position
_Check_return_ HRESULT ModernCollectionBasePanel::GetInsertionIndexesImpl(
    _In_ wf::Point position,
    _Out_ INT* pFirst,
    _Out_ INT* pSecond)
{
    int insertionIndex = -1;
    BOOLEAN isLeftBoundary = FALSE;
    BOOLEAN isRightBoundary = FALSE;
    BOOLEAN isTopBoundary = FALSE;
    BOOLEAN isBottomBoundary = FALSE;
    BOOLEAN firstCheck = FALSE;
    BOOLEAN secondCheck = FALSE;
    xaml_controls::Orientation orientation = xaml_controls::Orientation_Vertical;

    IFCPTR_RETURN(pFirst);
    IFCPTR_RETURN(pSecond);

    *pFirst = -1;
    *pSecond = -1;

    IFC_RETURN(GetInsertionIndex(position, &insertionIndex));
    IFC_RETURN(IsLayoutBoundary(insertionIndex, &isLeftBoundary, &isTopBoundary, &isRightBoundary, &isBottomBoundary));

    *pFirst = insertionIndex - 1;
    *pSecond = insertionIndex;

    IFC_RETURN(m_spLayoutStrategy->GetVirtualizationDirection(&orientation));
    if (orientation == xaml_controls::Orientation_Vertical)
    {
        firstCheck = isTopBoundary;
        secondCheck = isBottomBoundary;
    }
    else
    {
        firstCheck = isLeftBoundary;
        secondCheck = isRightBoundary;
    }

    // make sure we're not at the edges of the panel
    if (firstCheck)
    {
        *pFirst = -1;
    }
    else if (secondCheck)
    {
        *pSecond = -1;
    }

    return S_OK;
}

// Finds an arbitrary realized item from within the group under the given point.
// Non-grouped scenarios are treated as one large group.
_Check_return_ HRESULT ModernCollectionBasePanel::FindValidContainerInGroupForPoint(
        _In_ wf::Point point,
        _Out_ BOOLEAN* pGroupWasEmpty,
        _Out_ INT32* pGroupIndex,
        _Out_ INT32* pReferenceItemIndex,
        _Out_ INT32* pReferenceItemIndexInGroup,
        _Out_ wf::Rect* pReferenceItemBounds)
{
    HRESULT hr = S_OK;

    *pGroupWasEmpty = FALSE;
    *pGroupIndex = -1;
    *pReferenceItemIndex = -1;
    *pReferenceItemIndexInGroup = -1;
    *pReferenceItemBounds = {};

    ctl::ComPtr<IUIElement> spReferenceContainer;
    INT32 referenceItemIndex = -1;
    INT32 referenceItemIndexInGroup = -1;
    INT32 groupIndex = -1;
    BOOLEAN groupWasEmpty = FALSE;

    if (m_cacheManager.IsGrouping())
    {
        // Find the group we're in. Walk from left to right until we notice a group header to the right of/below our point.
        INT32 foundHeaderIndex = 0;
        const INT32 validHeaderCount = m_containerManager.GetValidHeaderCount();
        xaml_controls::Orientation orientation;

        ASSERT(validHeaderCount > 0);

        IFC(m_spLayoutStrategy->GetVirtualizationDirection(&orientation));

        // we start iterating from 2nd group if it exist. If not the loop will be ended right away.
        for (INT32 headerIndex = 1; headerIndex < validHeaderCount; headerIndex++)
        {
            ctl::ComPtr<IUIElement> spHeader;
            IFC(m_containerManager.GetHeaderAtValidIndex(headerIndex, &spHeader));

            // It's possible that a header doesn't exist, for example if it's hidden (HidesIfEmpty==true).
            // Skip over such headers.
            if (spHeader)
            {
                wf::Rect headerBounds = GetBoundsFromElement(spHeader);
                if ((orientation == xaml_controls::Orientation_Horizontal) ? (point.X < headerBounds.X) : (point.Y < headerBounds.Y))
                {
                    break;
                }
                // store current candidate. We will use this value in case all following groups are all hidden or if the next
                // candidate boundary is beyond the manipulation point.
                foundHeaderIndex = headerIndex;
            }
        }

        // by this point we should have found the index of valid group.
        groupIndex = m_containerManager.GetGroupIndexFromValidIndex(foundHeaderIndex);

        // Get any realized container in the group. There has to be at least one, unless the group is empty.
        INT32 startItemIndex = 0;
        INT32 itemCountInGroup = 0;

        IFC(m_cacheManager.GetGroupInformationFromGroupIndex(groupIndex, &startItemIndex, &itemCountInGroup));

        if (itemCountInGroup > 0)
        {
            INT32 validContainerSearchStart = m_containerManager.GetValidContainerIndexFromItemIndex(startItemIndex);
            INT32 validContainerSearchEnd = m_containerManager.GetValidContainerIndexFromItemIndex(startItemIndex + itemCountInGroup);

            groupWasEmpty = FALSE;

            validContainerSearchStart = std::max(0, validContainerSearchStart);
            validContainerSearchEnd = std::min(m_containerManager.GetValidContainerCount(), validContainerSearchEnd);

            // We had a header, we should have at least one valid container in that header's group.
            ASSERT(validContainerSearchStart < validContainerSearchEnd);
            IFC(m_containerManager.GetContainerAtValidIndex(validContainerSearchStart, &spReferenceContainer));
            referenceItemIndex = m_containerManager.GetItemIndexFromValidIndex(validContainerSearchStart);
            referenceItemIndexInGroup = referenceItemIndex - startItemIndex;
        }
        else
        {
            // Our group was empty! Let caller know.
            groupWasEmpty = TRUE;
        }

    }
    else if (m_containerManager.GetValidContainerCount() > 0)
    {
        // Not grouping. Grab any old element for now (could optimize to try to get closer to input point).
        referenceItemIndex = m_containerManager.GetItemIndexFromValidIndex(0);
        referenceItemIndexInGroup = referenceItemIndex;
        groupIndex = 0;
        groupWasEmpty = m_cacheManager.GetTotalItemCount() == 0;
        if (!groupWasEmpty)
        {
            IFC(m_containerManager.GetContainerAtValidIndex(0, &spReferenceContainer));
        }
    }

    if (spReferenceContainer)
    {
        *pReferenceItemBounds = GetBoundsFromElement(spReferenceContainer);
    }

    *pReferenceItemIndex = referenceItemIndex;
    *pReferenceItemIndexInGroup = referenceItemIndexInGroup;
    *pGroupWasEmpty = groupWasEmpty;
    *pGroupIndex = groupIndex;

Cleanup:
    RRETURN(hr);
}

// Gets a series of BOOLEAN values indicating whether a given index is
// positioned on the leftmost, topmost, rightmost, or bottommost
// edges of the layout.  This can be useful for both determining whether
// to tilt items at the edges of rows or columns as well as providing
// data for portal animations.
_Check_return_ HRESULT ModernCollectionBasePanel::IsLayoutBoundary(
    _In_ INT index,
    _Out_ BOOLEAN* isLeftBoundary,
    _Out_ BOOLEAN* isTopBoundary,
    _Out_ BOOLEAN* isRightBoundary,
    _Out_ BOOLEAN* isBottomBoundary)
{
    HRESULT hr = S_OK;

    // 1- Find group index, index in group, etc.
    // 2- Ask strategy if it's a boundary.

    *isLeftBoundary = FALSE;
    *isTopBoundary = FALSE;
    *isRightBoundary = FALSE;
    *isBottomBoundary = FALSE;

    // Don't support grouping yet. At present, we don't call into this private API if we're grouping.
    ASSERT(!m_cacheManager.IsGrouping());

    IFC(m_spLayoutStrategy->IsIndexLayoutBoundary(
            xaml_controls::ElementType_ItemContainer,
            index,
            m_windowState.GetRealizationWindow(),
            isLeftBoundary,
            isTopBoundary,
            isRightBoundary,
            isBottomBoundary));

Cleanup:
    RRETURN(hr);
}

_Check_return_ HRESULT ModernCollectionBasePanel::GetItemsBounds(
                _Out_ wf::Rect* returnValue)
{
    *returnValue = m_windowState.GetVisibleWindow();
    RRETURN(S_OK);

}

// is used by GenerateForward and Backward to determine how to generate.
// forward generation: needs an anchor at the left side of the valid ranges
// backward generation: needs an anchor on the left side of the valid ranges
//  Will always be the first container, unless there are no containers at all,
//  in which case we will fallback to using headers.
_Check_return_ HRESULT ModernCollectionBasePanel::GetGenerationAnchorInformation(
    _Out_ LayoutReference* pReferenceInformation,
    _Out_ CollectionIterator* pIterator)
{
    HRESULT hr = S_OK;

    // determine a reference
    // this can work because we have the following recycle rules:
    // 1. a header that has a container in it is not recycled
    // 2. when we recycle the last container from a group, we also recycle the header
    // 3. an empty header is not recycled, unless a container in a previous group needs
    //    to be recycled.
    // However, in the meantime inserts might have occurred on a group that have not resulted
    // in sentinels because they were at the edge. In this scenario we are in quite the pickle:
    // if that group lies outside of the realization window, then adding sentinels to it will result
    // in generation of containers that are outside the realization window which means that they will
    // be eligible for recycling again. The approach thus is to recognize that situation and special casing it.

    LayoutReference referenceInformation = CreateDefaultLayoutReference();
    ctl::ComPtr<IUIElement> spEdgeElement;
    ctl::ComPtr<IUIElement> spEdgeHeader;

    INT32 groupIndexOfEdgeItem = -1;
    INT32 indexInGroupOfEdgeItem = -1;
    INT32 itemIndexOfEdgeItem = -1;
    BOOLEAN useContainerAsEdge = FALSE;

    if (m_containerManager.GetValidContainerCount() > 0)
    {
        itemIndexOfEdgeItem = m_containerManager.GetItemIndexFromValidIndex(0);
        if (m_cacheManager.IsGrouping())
        {
            IFC(m_cacheManager.GetGroupInformationFromItemIndex(
                itemIndexOfEdgeItem,
                &groupIndexOfEdgeItem,
                &indexInGroupOfEdgeItem,
                nullptr));

            IFC(m_containerManager.GetHeaderAtGroupIndex(groupIndexOfEdgeItem, &spEdgeHeader));
        }
    }

    useContainerAsEdge =
        // if we are not grouping, use a container
        !m_cacheManager.IsGrouping()
        // if the first valid container is not representing the first item in the group, we have more containers to generate
        || indexInGroupOfEdgeItem > 0;

    if (useContainerAsEdge)
    {
        // we use a container if the container we found is floating. Whether we inserted new items or not, that
        // is still the best approach.
        IFC(m_containerManager.GetContainerAtValidIndex(0, &spEdgeElement));
        referenceInformation.ReferenceIsHeader = FALSE;
        pIterator->Init(itemIndexOfEdgeItem, xaml_controls::ElementType_ItemContainer);

        // get the headerbounds of this container
        if (spEdgeHeader)
        {
            referenceInformation.HeaderBounds = GetBoundsFromElement(spEdgeHeader);
        }
    }
    else
    {
        // we might have to use a header. We should do that when:
        // 1. there are no containers at all
        // 2. the index in group is 0, use its header (it is not floating)

        // but what if items have been inserted before the first container. Those sentinels will not have been
        // created, and creating them on the fly is fraught with danger.

        // reaching this else block means that:
        // a. we are grouping
        // b. we may have an edge item that is at index 0

        if (spEdgeHeader)
        {
            // if we already determined a header, we know it mcatches the item
            spEdgeElement = spEdgeHeader;
            pIterator->Init(groupIndexOfEdgeItem, xaml_controls::ElementType_GroupHeader);
        }
        else
        {
            // we have no header, per definition this means that we have no containers
            ASSERT(m_containerManager.GetValidContainerCount() == 0);

            // in that case, we can just take the first header
            IFC(m_containerManager.GetHeaderAtValidIndex(0, &spEdgeElement));
            pIterator->Init(m_containerManager.GetGroupIndexFromValidIndex(0), xaml_controls::ElementType_GroupHeader);
        }

        referenceInformation.ReferenceIsHeader = TRUE;

    }

    referenceInformation.ReferenceBounds = GetBoundsFromElement(spEdgeElement);
    referenceInformation.RelativeLocation = xaml_controls::ReferenceIdentity_AfterMe;

    *pReferenceInformation = referenceInformation;

Cleanup:
    RRETURN(hr);
}

_Check_return_ HRESULT ModernCollectionBasePanel::LayoutPreloadedItems()
{
    HRESULT hr = S_OK;

    if (m_enableCarouselPreload)
    {
        ASSERT(!m_cacheManager.IsGrouping());

        // If we're preloading items for faux carouseling, let's ensure they are in the correct location if not already in the valid range
        // We'll prioritize in the following order:
        // Item 0, Item 1, Last-1
        const auto itemCount = m_cacheManager.GetTotalItemCount();
        const auto windowToFill = m_windowState.GetRealizationWindow();

        if (itemCount > 0)
        {
            // Ensure item 0 is placed
            INT32 itemIndex = 0;
            wf::Rect firstContainerBounds;

            if (!m_containerManager.IsItemIndexWithinValidRange(itemIndex))
            {
                ctl::ComPtr<IUIElement> spFirstContainer;
                IFC(m_containerManager.GetContainerFromPinnedContainers(itemIndex, &spFirstContainer));
                ASSERT(spFirstContainer);

                wf::Point positionOfFirstItem;
                IFC(m_spLayoutStrategy->GetPositionOfFirstElement(&positionOfFirstItem));

                firstContainerBounds = RectUtil::CreateRect(positionOfFirstItem, RectUtil::GetSize(GetBoundsFromElement(spFirstContainer)));
                SetBoundsForElement(spFirstContainer, firstContainerBounds);
            }
            else
            {
                // This item already exists in the valid range, just get its bounds so we can place Item 1
                ctl::ComPtr<IUIElement> spFirstContainer;
                IFC(m_containerManager.GetContainerAtItemIndex(itemIndex, &spFirstContainer));
                firstContainerBounds = GetBoundsFromElement(spFirstContainer);
            }

            if (itemCount > 1)
            {
                // Use the bounds from item 0 to place item 1
                itemIndex = 1;
                if (!m_containerManager.IsItemIndexWithinValidRange(itemIndex))
                {
                    ctl::ComPtr<IUIElement> spSecondContainer;
                    IFC(m_containerManager.GetContainerFromPinnedContainers(itemIndex, &spSecondContainer));

                    LayoutReference referenceInfo = CreateDefaultLayoutReference();
                    referenceInfo.ReferenceIsHeader = false;
                    referenceInfo.ReferenceBounds = firstContainerBounds;
                    referenceInfo.RelativeLocation = xaml_controls::ReferenceIdentity_BeforeMe;

                    wf::Rect bounds;
                    IFC(m_spLayoutStrategy->GetElementBounds(
                        xaml_controls::ElementType_ItemContainer,
                        itemIndex,
                        RectUtil::GetSize(GetBoundsFromElement(spSecondContainer)),
                        referenceInfo,
                        windowToFill,
                        &bounds));

                    SetBoundsForElement(spSecondContainer, bounds);
                }
            }
        }

        // Now, see if we need to place the last item
        // Only need to do this if we have more than two items, as items 0 and 1 were just placed
        if (itemCount > 2)
        {
            INT32 itemIndex = itemCount - 1;
            wf::Rect lastContainerBounds;

            if (!m_containerManager.IsItemIndexWithinValidRange(itemIndex))
            {
                ctl::ComPtr<IUIElement> spLastContainer;
                IFC(m_containerManager.GetContainerFromPinnedContainers(itemIndex, &spLastContainer));
                ASSERT(spLastContainer);

                lastContainerBounds = GetBoundsFromElement(spLastContainer);
                LayoutReference referenceInfo = CreateDefaultLayoutReference();
                referenceInfo.ReferenceIsHeader = false;
                referenceInfo.ReferenceBounds = lastContainerBounds;
                referenceInfo.RelativeLocation = xaml_controls::ReferenceIdentity_Myself;

                // Go through a call to the strategy to ensure this container's location in the non-virtualizing direction is correct
                IFC(m_spLayoutStrategy->GetElementBounds(
                    xaml_controls::ElementType_ItemContainer,
                    itemIndex,
                    RectUtil::GetSize(lastContainerBounds),
                    referenceInfo,
                    windowToFill,
                    &lastContainerBounds));

                // Correct the virtualizing position with respect to the panel's estimated size
                lastContainerBounds.*PointFromRectInVirtualizingDirection() = m_estimatedSize.*SizeInVirtualizingDirection() - lastContainerBounds.*SizeFromRectInVirtualizingDirection();
                SetBoundsForElement(spLastContainer, lastContainerBounds);
            }
        }
    }

Cleanup:
    RRETURN(hr);
}


// Return whether an element should be generated/placed/shown. Normally, this is always true
// but can return false if we're hiding empty groups or collapsing groups
bool ModernCollectionBasePanel::ShouldElementBeVisible(_In_ xaml_controls::ElementType type, _In_ INT32 index) const
{
    bool shouldBeVisible = false;

    switch (type)
    {
    case xaml_controls::ElementType_GroupHeader:
        {
            if (m_cacheManager.IsGrouping())
            {
                if (m_cacheManager.GetHidesIfEmpty())
                {
                    INT32 itemsInGroup = 0;
                    VERIFYHR(m_cacheManager.GetGroupInformationFromGroupIndex(index, nullptr, &itemsInGroup));

                    shouldBeVisible = (itemsInGroup > 0);
                }
                else
                {
                    shouldBeVisible = true;
                }
            }
            else
            {
                // Why would we even call this? If we're not grouping, we shouldn't be displaying headers
                ASSERT(0);
                shouldBeVisible = false;
            }
        }
        break;

    case xaml_controls::ElementType_ItemContainer:
        shouldBeVisible = true;
        break;

    default:
        ASSERT(0);
    }

    return shouldBeVisible;
}

float wf::Point::* ModernCollectionBasePanel::PointFromPointInVirtualizingDirection () const
{
    xaml_controls::Orientation layoutOrientation = xaml_controls::Orientation_Horizontal;

    if(SUCCEEDED(m_spLayoutStrategy->GetVirtualizationDirection(&layoutOrientation)))
    {
        switch(layoutOrientation)
        {
        case xaml_controls::Orientation_Horizontal:
            return &wf::Point::X;
        case xaml_controls::Orientation_Vertical:
            return &wf::Point::Y;
        }
    }

    return nullptr;
}

float wf::Size::* ModernCollectionBasePanel::PointFromSizeInVirtualizingDirection() const
{
    xaml_controls::Orientation layoutOrientation = xaml_controls::Orientation_Horizontal;

    if (SUCCEEDED(m_spLayoutStrategy->GetVirtualizationDirection(&layoutOrientation)))
    {
        switch (layoutOrientation)
        {
        case xaml_controls::Orientation_Horizontal:
            return &wf::Size::Width;
        case xaml_controls::Orientation_Vertical:
            return &wf::Size::Height;
        }
    }

    return nullptr;
}

float wf::Size::* ModernCollectionBasePanel::SizeInVirtualizingDirection() const
{
    xaml_controls::Orientation layoutOrientation = xaml_controls::Orientation_Horizontal;

    if (SUCCEEDED(m_spLayoutStrategy->GetVirtualizationDirection(&layoutOrientation)))
    {
        switch (layoutOrientation)
        {
        case xaml_controls::Orientation_Horizontal:
            return &wf::Size::Width;
        case xaml_controls::Orientation_Vertical:
            return &wf::Size::Height;
        }
    }

    return nullptr;
}

float wf::Size::* ModernCollectionBasePanel::SizeInNonVirtualizingDirection() const
{
    xaml_controls::Orientation layoutOrientation = xaml_controls::Orientation_Horizontal;

    if (SUCCEEDED(m_spLayoutStrategy->GetVirtualizationDirection(&layoutOrientation)))
    {
        switch (layoutOrientation)
        {
        case xaml_controls::Orientation_Horizontal:
            return &wf::Size::Height;
        case xaml_controls::Orientation_Vertical:
            return &wf::Size::Width;
        }
    }

    return nullptr;
}


float wf::Rect::* ModernCollectionBasePanel::PointFromRectInVirtualizingDirection () const
{
    xaml_controls::Orientation layoutOrientation = xaml_controls::Orientation_Horizontal;

    if(SUCCEEDED(m_spLayoutStrategy->GetVirtualizationDirection(&layoutOrientation)))
    {
        switch(layoutOrientation)
        {
        case xaml_controls::Orientation_Horizontal:
            return &wf::Rect::X;
        case xaml_controls::Orientation_Vertical:
            return &wf::Rect::Y;
        }
    }

    return nullptr;
}

float wf::Rect::* ModernCollectionBasePanel::PointFromRectInNonVirtualizingDirection () const
{
    xaml_controls::Orientation layoutOrientation = xaml_controls::Orientation_Horizontal;

    if(SUCCEEDED(m_spLayoutStrategy->GetVirtualizationDirection(&layoutOrientation)))
    {
        switch(layoutOrientation)
        {
        case xaml_controls::Orientation_Horizontal:
            return &wf::Rect::Y;
        case xaml_controls::Orientation_Vertical:
            return &wf::Rect::X;
        }
    }

    return nullptr;
}

float wf::Rect::* ModernCollectionBasePanel::SizeFromRectInVirtualizingDirection () const
{
    xaml_controls::Orientation layoutOrientation = xaml_controls::Orientation_Horizontal;

    if(SUCCEEDED(m_spLayoutStrategy->GetVirtualizationDirection(&layoutOrientation)))
    {
        switch(layoutOrientation)
        {
        case xaml_controls::Orientation_Horizontal:
            return &wf::Rect::Width;
        case xaml_controls::Orientation_Vertical:
            return &wf::Rect::Height;
        }
    }

    return nullptr;
}

// Gets the cache length.
_Check_return_ HRESULT
ModernCollectionBasePanel::get_CacheLengthBase(
    _Out_ DOUBLE* pValue)
{
    HRESULT hr = S_OK;

    IFCPTR(pValue);
    *pValue = m_cacheLength;

Cleanup:
    RRETURN(hr);
}

// Sets the cache length.
_Check_return_ HRESULT
ModernCollectionBasePanel::put_CacheLengthBase(
    _In_ DOUBLE value)
{
    HRESULT hr = S_OK;

    if (value < 0)
    {
        IFC(E_INVALIDARG);
    }

    m_cacheLength = value;

    IFC(ResetCacheBuffers());

Cleanup:
    RRETURN(hr);
}

wf::Point ModernCollectionBasePanel::GetGarbageElementPosition(_In_ const ctl::ComPtr<IUIElement>& spElement) const
{
    wf::Point result = GetOffScreenPosition();

    // If we performing carouselling preload, don't throw the wrapped elements off into lala-land
    if (m_enableCarouselPreload)
    {
        INT32 elementIndex = -1;

        xaml_controls::ElementType elementType = (GetElementIsHeader(spElement)) ? xaml_controls::ElementType_GroupHeader : xaml_controls::ElementType_ItemContainer;

        if (m_containerManager.GetIsElementPinned(elementType, spElement, &elementIndex))
        {
            ASSERT(elementIndex != -1);
            if (elementIndex == 0 || elementIndex == 1 || elementIndex == m_cacheManager.GetTotalItemCount() - 1)
            {
                result = RectUtil::GetPoint(GetBoundsFromElement(spElement));
            }
        }
    }

    return result;
}

_Check_return_ HRESULT ModernCollectionBasePanel::GetViewportSize(_Out_ wf::Size* pSize)
{
    HRESULT hr = S_OK;

    pSize->Width = 0.0;
    pSize->Height = 0.0;
    auto pScrollViewer = m_wrScrollViewer.AsOrNull<xaml_controls::IScrollViewer>();
    if (pScrollViewer)
    {
        DOUBLE viewportHeight = 0.0;
        DOUBLE viewportWidth = 0.0;

        IFC(pScrollViewer->get_ViewportHeight(&viewportHeight));
        IFC(pScrollViewer->get_ViewportWidth(&viewportWidth));
        pSize->Width = static_cast<float>(viewportWidth);
        pSize->Height = static_cast<float>(viewportHeight);
    }

Cleanup:
    return hr;
}

_Check_return_ HRESULT ModernCollectionBasePanel::OnPropertyChanged2(_In_ const PropertyChangedParams& args)
{
    IFC_RETURN(ModernCollectionBasePanelGenerated::OnPropertyChanged2(args));

    switch (args.m_pDP->GetIndex())
    {
        case KnownPropertyIndex::ModernCollectionBasePanel_AreStickyGroupHeadersEnabledBase:
        {
            // Call ReevaluateGroupHeaderStrategy when AreStickyGroupHeadersEnabled changes
            IFC_RETURN(ReevaluateGroupHeaderStrategy());
            break;
        }
    }

    return S_OK;
}
