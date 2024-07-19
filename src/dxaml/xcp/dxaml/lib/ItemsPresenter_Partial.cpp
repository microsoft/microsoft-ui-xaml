// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#include "precomp.h"
#include "ItemsPresenter.g.h"
#include "ContentControl.g.h"
#include "ScrollContentPresenter.g.h"
#include "ModernCollectionBasePanel.g.h"
#include "VirtualizingPanel.g.h"
#include "OrientedVirtualizingPanel.g.h"
#include "Canvas.g.h"
#include "VisualTreeHelper.h"

using namespace DirectUI;
using namespace DirectUISynonyms;
using namespace std::placeholders;

// Uncomment to get debug traces
//#define HEADER_DBG
//#define SCROLLING_DBG

const DOUBLE ItemsPresenter::FooterDelayLoadOffset = 200.0;

ItemsPresenter::ItemsPresenter()
    : m_bNotifyHorizontalSnapPointsChanges(FALSE)
    , m_bNotifyVerticalSnapPointsChanges(FALSE)
    , m_bNotifiedHorizontalSnapPointsChanges(FALSE)
    , m_bNotifiedVerticalSnapPointsChanges(FALSE)
    , m_bAreSnapPointsKeysHorizontal(FALSE)
    , m_leadingSnapPointKey(0)
    , m_trailingSnapPointKey(0)
    , m_leadingMarginSnapPointKey(0)
    , m_trailingMarginSnapPointKey(0)
    , m_irregularSnapPointKeysOffset(0)
    , m_regularSnapPointKey(0)
    , m_cIrregularSnapPointKeys(0)
    , m_pIrregularSnapPointKeys(NULL)
    , m_fZoomFactor(1)
    , m_bCanLoadFooter(FALSE)
    , m_lastHeaderArrangeRect()
    , m_lastFooterArrangeRect()
    , m_lastPanelArrangeRect()
    , m_HorizontalSnapPointsChangedToken()
    , m_VerticalSnapPointsChangedToken()
{
}

ItemsPresenter::~ItemsPresenter()
{
    m_tpHeader.Clear();
    m_tpFooter.Clear();

    delete [] m_pIrregularSnapPointKeys;
    m_pIrregularSnapPointKeys = NULL;

    IGNOREHR(UnhookScrollSnapPointsInfoEvents(TRUE /*isForHorizontalSnapPoints*/));
    IGNOREHR(UnhookScrollSnapPointsInfoEvents(FALSE /*isForHorizontalSnapPoints*/));
    m_tpScrollSnapPointsInfo.Clear();
}

// Supports the IManipulationDataProvider interface.
_Check_return_
HRESULT
ItemsPresenter::QueryInterfaceImpl(
    _In_ REFIID iid,
    _Outptr_ void** ppObject)
{
    if (InlineIsEqualGUID(iid, __uuidof(IManipulationDataProvider)))
    {
        *ppObject = static_cast<IManipulationDataProvider*>(this);
    }
    else if (InlineIsEqualGUID(iid, __uuidof(IScrollOwner)))
    {
        *ppObject = static_cast<IScrollOwner*>(this);
    }
    else
    {
        RRETURN(ItemsPresenterGenerated::QueryInterfaceImpl(iid, ppObject));
    }

    AddRefOuter();
    RRETURN(S_OK);
}

_Check_return_
HRESULT
ItemsPresenter::Initialize()
{
    HRESULT hr = S_OK;
    ctl::ComPtr<ContentControl> spHeader;
    ctl::ComPtr<ContentControl> spFooter;
    ctl::ComPtr<IActivationFactory> spCanvasActivationFactory;
    ctl::ComPtr<xaml_controls::ICanvasStatics> spCanvasStatics;

    IFC(ItemsPresenterGenerated::Initialize());

    IFC(ctl::make<ContentControl>(&spHeader));
    IFC(ctl::make<ContentControl>(&spFooter));

    spCanvasActivationFactory.Attach(ctl::ActivationFactoryCreator<DirectUI::CanvasFactory>::CreateActivationFactory());
    IFC(spCanvasActivationFactory.As<xaml_controls::ICanvasStatics>(&spCanvasStatics));

    // make sure that footer has lower zIndex to property display drop animation.
    IFC(spCanvasStatics->SetZIndex(spFooter.Get(), -1));

    // Setting the content alignment to stretch is necessary for content's VerticalAlignment
    // and HorizontalAlignment to work properly.

    IFC(spHeader->put_HorizontalContentAlignment(xaml::HorizontalAlignment::HorizontalAlignment_Stretch));
    IFC(spHeader->put_VerticalContentAlignment(xaml::VerticalAlignment::VerticalAlignment_Stretch));

    IFC(spFooter->put_HorizontalContentAlignment(xaml::HorizontalAlignment::HorizontalAlignment_Stretch));
    IFC(spFooter->put_VerticalContentAlignment(xaml::VerticalAlignment::VerticalAlignment_Stretch));

    IFC(spHeader->put_IsTabStop(FALSE));
    IFC(spFooter->put_IsTabStop(FALSE));

    SetPtrValue(m_tpHeader, spHeader);
    SetPtrValue(m_tpFooter, spFooter);

    // Subscribes to load/unload events.
    IFC(m_epLoadedHandler.AttachEventHandler(this, std::bind(&ItemsPresenter::OnLoaded, this, _1, _2)));
    IFC(m_epUnloadedHandler.AttachEventHandler(this, std::bind(&ItemsPresenter::OnUnloaded, this, _1, _2)));

Cleanup:
    RRETURN(hr);
}

_Check_return_ HRESULT ItemsPresenter::OnPropertyChanged2(_In_ const PropertyChangedParams& args)
{
    HRESULT hr = S_OK;

    IFC(ItemsPresenterGenerated::OnPropertyChanged2(args));

    // KnownPropertyIndex::ItemsPresenter_Padding in native and has PROP_AFFECT_MEASURE flag
    switch (args.m_pDP->GetIndex())
    {
    case KnownPropertyIndex::ItemsPresenter_Header:
        IFC(m_tpHeader->SetValueByKnownIndex(KnownPropertyIndex::ContentControl_Content, *args.m_pNewValue));
        IFC(InvalidateMeasure());
        break;
    case KnownPropertyIndex::ItemsPresenter_HeaderTemplate:
        IFC(m_tpHeader->SetValueByKnownIndex(KnownPropertyIndex::ContentControl_ContentTemplate, *args.m_pNewValue));
        IFC(InvalidateMeasure());
        break;
    case KnownPropertyIndex::ItemsPresenter_HeaderTransitions:
        IFC(m_tpHeader->SetValueByKnownIndex(KnownPropertyIndex::ContentControl_ContentTransitions, *args.m_pNewValue));
        IFC(InvalidateMeasure());
        break;
    // The footer is loaded only when it enters the viewport.
    case KnownPropertyIndex::ItemsPresenter_Footer:
        if(m_bCanLoadFooter)
        {
            IFC(m_tpFooter->SetValueByKnownIndex(KnownPropertyIndex::ContentControl_Content, *args.m_pNewValue));
            IFC(InvalidateMeasure());
        }
        break;
    case KnownPropertyIndex::ItemsPresenter_FooterTemplate:
        if(m_bCanLoadFooter)
        {
            IFC(m_tpFooter->SetValueByKnownIndex(KnownPropertyIndex::ContentControl_ContentTemplate, *args.m_pNewValue));
            IFC(InvalidateMeasure());
        }
        break;
    case KnownPropertyIndex::ItemsPresenter_FooterTransitions:
        IFC(m_tpFooter->SetValueByKnownIndex(KnownPropertyIndex::ContentControl_ContentTransitions, *args.m_pNewValue));
        IFC(InvalidateMeasure());
        break;
    case KnownPropertyIndex::ItemsPresenter_ItemsPanel:
        {
            // due to bug 913298, this code path was being called in the ApplyTemplate
            // and the Panel has not been created yet so this code path would create it and
            // then ApplyTemplate would create it again
            ctl::ComPtr<wfc::IVector<xaml::UIElement*>> spChildren;
            unsigned int childCount = 0;

            IFC(get_ChildrenInternal(&spChildren));
            IFC(spChildren->get_Size(&childCount));

            // if childCount is greater than 0, it means the Panel has been created
            if (childCount > 0)
            {
                IFC(NotifySnapPointsInfoPanelChanged());
            }
        }
        break;
    }

Cleanup:
    RRETURN(hr);
}

// Subscribes to ScrollViewer events in order to load footer when it enters the viewport.
_Check_return_ HRESULT ItemsPresenter::OnLoaded(
    _In_ IInspectable* pSender,
    _In_ IRoutedEventArgs* pArgs )
{
    HRESULT hr = S_OK;
    ctl::ComPtr<IScrollViewer> spScrollViewer;
    ctl::ComPtr<IDependencyObject> spParentAsDO;
    ctl::ComPtr<IScrollContentPresenter> spParentAsSCP;

    // In some circumstances, Unloaded doesn't get raised. We need to make sure we don't subscribe twice
    // to the same event.
    if(m_wrScrollViewer)
    {
        ctl::ComPtr<IScrollViewer> spOldScrollViewer = m_wrScrollViewer.AsOrNull<IScrollViewer>();

        if(spOldScrollViewer)
        {
            IFC(m_epScrollViewerViewChangedHandler.DetachEventHandler(spOldScrollViewer.Get()));
        }

        m_wrScrollViewer.Reset();
    }

    IFC(VisualTreeHelper::GetParentStatic(this, &spParentAsDO));
    spParentAsSCP = spParentAsDO.AsOrNull<IScrollContentPresenter>();

    if(spParentAsSCP)
    {
        ctl::ComPtr<IScrollViewer> spScrollViewerAsII;

        IFC(spParentAsSCP.Cast<ScrollContentPresenter>()->get_ScrollOwner(&spScrollViewerAsII));
        spScrollViewer = spScrollViewerAsII.AsOrNull<IScrollViewer>();

        if(spScrollViewer)
        {
            IFC(spScrollViewer.AsWeak(&m_wrScrollViewer));
            IFC(m_epScrollViewerViewChangedHandler.AttachEventHandler(spScrollViewer.Get(), std::bind(&ItemsPresenter::OnScrollViewChanged, this, _1, _2)));

            IFC(DelayLoadFooter());
        }
    }

    // If we can't find a ScrollViewer, we won't delay load footer.
    if(!spScrollViewer)
    {
        IFC(LoadFooter(FALSE /* updateLayout */));
    }

Cleanup:
    RRETURN(hr);
}

// Unsubscribes from ScrollViewer events.
_Check_return_ HRESULT ItemsPresenter::OnUnloaded(
    _In_ IInspectable* pSender,
    _In_ IRoutedEventArgs* pArgs )
{
    HRESULT hr = S_OK;

    auto spScrollViewer = m_wrScrollViewer.AsOrNull<IScrollViewer>();
    if (spScrollViewer)
    {
        IFC(m_epScrollViewerViewChangedHandler.DetachEventHandler(spScrollViewer.Get()));
    }

    m_wrScrollViewer.Reset();

Cleanup:
    RRETURN(hr);
}

// Called when the ScrollViewer's viewport changes.
_Check_return_ HRESULT ItemsPresenter::OnScrollViewChanged(
    _In_ IInspectable* pSender,
    _In_ IScrollViewerViewChangedEventArgs* pArgs)
{
    HRESULT hr = S_OK;

    IFC(DelayLoadFooter());

Cleanup:
    RRETURN(hr);
}

// Loads the footer if it's in the viewport.
// The current viewport is retrieved from the ScrollViewer unless a future viewport is specified.
// Future viewports are used to pre-load the footer before a scroll operation.
_Check_return_ HRESULT ItemsPresenter::DelayLoadFooter()
{
    return DelayLoadFooter(nullptr, FALSE /* updateLayout */);
}

_Check_return_ HRESULT ItemsPresenter::DelayLoadFooter(
    _In_opt_ wf::Rect* pFutureViewportRect,
    _In_ BOOLEAN updateLayout)
{
    HRESULT hr = S_OK;

    if (!m_bCanLoadFooter)
    {
        auto spScrollViewer = m_wrScrollViewer.AsOrNull<IScrollViewer>();

        if(spScrollViewer)
        {
            BOOLEAN isHorizontal = false;
            DOUBLE viewportOffset = 0;
            DOUBLE viewportSize = 0;
            DOUBLE footerOffset = 0;
            DOUBLE delayLoadOffset = FooterDelayLoadOffset;
            FLOAT zoomFactor = 0.0f;

            IFC(IsHorizontal(isHorizontal));

            // Retrieves offset values depending of the panel orientation.
            if(isHorizontal)
            {
                if(pFutureViewportRect)
                {
                    viewportOffset = static_cast<DOUBLE>(pFutureViewportRect->X);
                    viewportSize = static_cast<DOUBLE>(pFutureViewportRect->Width);
                }
                else
                {
                    IFC(spScrollViewer->get_HorizontalOffset(&viewportOffset));
                    IFC(spScrollViewer->get_ViewportWidth(&viewportSize));
                }

                footerOffset = m_lastFooterArrangeRect.X;
            }
            else
            {
                if(pFutureViewportRect)
                {
                    viewportOffset = static_cast<DOUBLE>(pFutureViewportRect->Y);
                    viewportSize = static_cast<DOUBLE>(pFutureViewportRect->Height);
                }
                else
                {
                    IFC(spScrollViewer->get_VerticalOffset(&viewportOffset));
                    IFC(spScrollViewer->get_ViewportHeight(&viewportSize));
                }

                footerOffset = m_lastFooterArrangeRect.Y;
            }

            // Adjusts the viewport to account for the ScrollViewer zoom.
            IFC(spScrollViewer->get_ZoomFactor(&zoomFactor));
            viewportOffset /= zoomFactor;
            viewportSize /= zoomFactor;
            delayLoadOffset /= zoomFactor;

            // Check that the footer is in the viewport.
            if((viewportOffset + viewportSize + delayLoadOffset) > footerOffset)
            {
                IFC(LoadFooter(updateLayout));
            }
        }
    }

Cleanup:
    RRETURN(hr);
}

// Force footer to load if loading is being delayed.
_Check_return_ HRESULT ItemsPresenter::LoadFooter(
    _In_ BOOLEAN updateLayout)
{
    HRESULT hr = S_OK;


    if(!m_bCanLoadFooter)
    {
        ctl::ComPtr<IInspectable> spFooter;
        ctl::ComPtr<IDataTemplate> spFooterTemplate;
        auto spScrollViewer = m_wrScrollViewer.AsOrNull<IScrollViewer>();

        IFC(get_Footer(&spFooter));
        IFC(get_FooterTemplate(&spFooterTemplate));

        IFC(m_tpFooter->put_Content(spFooter.Get()));
        IFC(m_tpFooter->put_ContentTemplate(spFooterTemplate.Get()));

        m_bCanLoadFooter = TRUE;

        // This is called when we use ScrollIntoView to scroll to footer.
        if(updateLayout)
        {
            IFC(m_tpFooter->UpdateLayout());
        }

        // We don't need this event anymore.
        if(spScrollViewer)
        {
            IFC(m_epScrollViewerViewChangedHandler.DetachEventHandler(spScrollViewer.Get()));
        }
        IFC(m_epLoadedHandler.DetachEventHandler(ctl::iinspectable_cast(this)));
        IFC(m_epUnloadedHandler.DetachEventHandler(ctl::iinspectable_cast(this)));
    }

Cleanup:
    RRETURN(hr);
}

BOOLEAN ItemsPresenter::WantsScrollViewerToObscureAvailableSizeBasedOnScrollBarVisibility(_In_ xaml_controls::Orientation orientation)
{
    HRESULT hr = S_OK; // WARNING_IGNORES_FAILURES
    ctl::ComPtr<IPanel> spPanel;

    IFC(get_Panel(&spPanel));

    return WantsScrollViewerToObscureAvailableSizeBasedOnScrollBarVisibility(orientation, spPanel);

Cleanup:
    return true;    // default behavior
}

BOOLEAN ItemsPresenter::WantsScrollViewerToObscureAvailableSizeBasedOnScrollBarVisibility(_In_ xaml_controls::Orientation orientation,
                                                                                          _In_ ctl::ComPtr<xaml_controls::IPanel>& spPanel)
{
    return spPanel.AsOrNull<IFrameworkElement>().Cast<FrameworkElement>()->WantsScrollViewerToObscureAvailableSizeBasedOnScrollBarVisibility(orientation);
}

// Measures header, footer and the inner panel
IFACEMETHODIMP ItemsPresenter::MeasureOverride(
    _In_ wf::Size availableSize,
    _Out_ wf::Size* returnValue)
{
    ctl::ComPtr<IPanel> spPanel;
    ctl::ComPtr<IScrollInfo> spScrollInfo;
    ctl::ComPtr<IInspectable> spHeaderContent, spFooterContent;
    ctl::ComPtr<xaml::IDataTemplate> spHeaderTemplate, spFooterTemplate;
    ctl::ComPtr<wfc::IVector<xaml::UIElement*>> spChildren;

    wf::Size correctedAvailableSize = {};
    wf::Size desiredSize = {};
    wf::Size headerDesiredSize = {};
    wf::Size footerDesiredSize = {};
    BOOLEAN isHorizontal = FALSE;
    xaml::Thickness padding = {};
    UINT childCount = 0;

    *returnValue = desiredSize;

    // Ensure panel and header is initialized and hooked up.
    IFC_RETURN(get_Panel(&spPanel));

    spScrollInfo = spPanel.AsOrNull<IScrollInfo>();

    IFC_RETURN(get_Header(&spHeaderContent));
    IFC_RETURN(get_HeaderTemplate(&spHeaderTemplate));

    IFC_RETURN(get_Footer(&spFooterContent));
    IFC_RETURN(get_FooterTemplate(&spFooterTemplate));

    IFC_RETURN(get_ChildrenInternal(&spChildren));
    IFC_RETURN(spChildren->get_Size(&childCount));

    if (childCount > 0)
    {
        ctl::ComPtr<IUIElement> spFirstChild;

        if (!spHeaderContent && !spHeaderTemplate)
        {
            IFC_RETURN(m_tpHeader->put_Visibility(xaml::Visibility::Visibility_Collapsed));
        }
        else
        {
            IFC_RETURN(m_tpHeader->put_Visibility(xaml::Visibility::Visibility_Visible));
        }

        // Footers are not supported for scroll info panels (i.e. VirtualizingStackPanel), so we
        // hide them.
        if ((!spFooterContent && !spFooterTemplate) || spScrollInfo)
        {
            IFC_RETURN(m_tpFooter->put_Visibility(xaml::Visibility::Visibility_Collapsed));
        }
        else
        {
            IFC_RETURN(m_tpFooter->put_Visibility(xaml::Visibility::Visibility_Visible));
        }

        IFC_RETURN(spChildren->GetAt(0, &spFirstChild));
        if (spFirstChild.Get() != m_tpHeader.Get())
        {
            IFC_RETURN(spChildren->InsertAt(0, m_tpHeader.Get()));
            IFC_RETURN(spChildren->Append(m_tpFooter.Get()));

            IFC_RETURN(NotifySnapPointsInfoPanelChanged());
        }
    }
    else
    {
        auto coreDO = GetHandle();

        CValue headerTransitions = coreDO->CheckOnDemandProperty(KnownPropertyIndex::ItemsPresenter_HeaderTransitions);
        CValue footerTransitions = coreDO->CheckOnDemandProperty(KnownPropertyIndex::ItemsPresenter_FooterTransitions);

        IFCEXPECT_RETURN(
            !spPanel &&
            !spHeaderContent && !spHeaderTemplate &&
            !spFooterContent && !spFooterTemplate);

        return S_OK;
    }

    IFC_RETURN(get_PaddingInternal(&padding));
    IFC_RETURN(IsHorizontal(isHorizontal));

    // Correct available size for non-virtualizing direction padding.
    IFC_RETURN(DecreaseSizeForPaddingInNonVirtualizingDimension(&availableSize));

    correctedAvailableSize = availableSize;

    // Measure header with infinite size in virtualizing direction.
    // We need to ensure header is a child of ItemsPresenter
    if (isHorizontal)
    {
        correctedAvailableSize.Width = static_cast<FLOAT>(DoubleUtil::PositiveInfinity);
    }
    else
    {
        correctedAvailableSize.Height = static_cast<FLOAT>(DoubleUtil::PositiveInfinity);
    }

    // Measure header
    IFC_RETURN(m_tpHeader->Measure(correctedAvailableSize));
    IFC_RETURN(GetHeaderSize(headerDesiredSize));

    // Measure footer
    IFC_RETURN(m_tpFooter->Measure(correctedAvailableSize));
    IFC_RETURN(GetFooterSize(footerDesiredSize));

    // Measure panel

    // regarding the check for IModernCollectionBasePanel:
    // these new style virtualizing panels do not think in logical units
    // and want to be treated just like regular panels. Ofcourse they
    // do not want to be limited in the virtualizing direction, so I want to use
    // the correctedAvailableSize for it.

    if (!spScrollInfo)
    {
        ctl::ComPtr<IScrollOwner> spScrollOwner;

        if (ctl::is<IModernCollectionBasePanel>(spPanel))
        {
            // the panels will not have yet been laid out at this point so have no clue about their offset
            // Without having implemented nested virtualization completely, we have a need to understand more
            // about our situation than measure will provide. If we do not pass in the location, we will have
            // do another layoutpass to actually adjust perfectly. So the below communication increases perf
            // and fixes issues around the first tick not being perfect.
            wf::Size tempSize = availableSize;
            wf::Rect headerRect = {};
            wf::Rect panelRect = {};
            wf::Rect footerRect = {};
            wf::Point panelOrigin = {};
            xaml::Thickness margin = {};
            // Correct final size for non-virtualizing direction padding.
            IFC_RETURN(DecreaseSizeForPaddingInNonVirtualizingDimension(&tempSize));

            // Calculate arrange location for header and inner panel
            IFC_RETURN(CalculateArrangeRect(tempSize, spPanel, &headerRect, &panelRect, &footerRect));

            // the above call was correct because we already have a desiredsize for the header.
            IFC_RETURN(get_Margin(&margin));
            panelOrigin.X = panelRect.X + static_cast<float>(margin.Left);
            panelOrigin.Y = panelRect.Y + static_cast<float>(margin.Top);
            spPanel.Cast<ModernCollectionBasePanel>()->SetOriginFromItemsPresenter(panelOrigin);
        }

        IFC_RETURN(m_ScrollData.get_ScrollOwner(&spScrollOwner));
        if (spScrollOwner.Get() || ctl::is<IModernCollectionBasePanel>(spPanel))
        {
            IFC_RETURN(spPanel.Cast<Panel>()->Measure(correctedAvailableSize));
        }
        else
        {
            IFC_RETURN(spPanel.Cast<Panel>()->Measure(availableSize));
        }
    }
    else
    {
        if (ctl::is<IModernCollectionBasePanel>(spPanel))
        {
            IFC_RETURN(spPanel.Cast<Panel>()->Measure(correctedAvailableSize));
        }
        else
        {
            IFC_RETURN(spPanel.Cast<Panel>()->Measure(availableSize));
        }
    }

    IFC_RETURN(spPanel.Cast<Panel>()->get_DesiredSize(&desiredSize));

    if (isHorizontal)
    {
        // don't accumulate header and footer size for ScrollInfoPanels
        if (!spScrollInfo)
        {
            desiredSize.Width +=
                headerDesiredSize.Width +
                footerDesiredSize.Width +
                static_cast<FLOAT>(padding.Left + padding.Right);
        }
        desiredSize.Height = MAX(desiredSize.Height, MAX(headerDesiredSize.Height, footerDesiredSize.Height));
    }
    else
    {
        // don't accumulate header and footer size for ScrollInfoPanels
        if (!spScrollInfo)
        {
            desiredSize.Height +=
                headerDesiredSize.Height +
                footerDesiredSize.Height +
                static_cast<FLOAT>(padding.Top + padding.Bottom);
        }
        desiredSize.Width = MAX(desiredSize.Width, MAX(headerDesiredSize.Width, footerDesiredSize.Width));
    }

    *returnValue = desiredSize;

    // Correct return size for non-virtualizing direction padding.
    IFC_RETURN(IncreaseSizeForPaddingInNonVirtualizingDimension(returnValue));

    auto availableDimension = isHorizontal ? availableSize.Width : availableSize.Height;
    float roundedAvailableDimension = 0.0f;
    IFC_RETURN(LayoutRound(availableDimension, &roundedAvailableDimension));

    IFC_RETURN(UpdateScrollData(isHorizontal, roundedAvailableDimension));

    return S_OK;
}

// Arranges header and inner panel
IFACEMETHODIMP ItemsPresenter::ArrangeOverride(
    _In_ wf::Size finalSize,
    _Out_ wf::Size* returnValue)
{
    ctl::ComPtr<IPanel> spPanel;

    wf::Rect headerRect = {};
    wf::Rect panelRect = {};
    wf::Rect footerRect = {};

#ifdef HEADER_DBG
    WCHAR szValue[MAX_PATH];
#endif

    *returnValue = finalSize;

    IFC_RETURN(get_Panel(&spPanel));
    if (!spPanel)
    {
        return S_OK;
    }

    // Correct final size for non-virtualizing direction padding.
    IFC_RETURN(DecreaseSizeForPaddingInNonVirtualizingDimension(&finalSize));

    // Calculate arrange location for header and inner panel
    IFC_RETURN(CalculateArrangeRect(finalSize, spPanel, &headerRect, &panelRect, &footerRect));

#ifdef HEADER_DBG
    swprintf_s(szValue, MAX_PATH, L"Arrange header location rect X: %f, Y: %f, panel start location: X: %f Y: %f", rect.X, rect.Y, panelStartLocation.X, panelStartLocation.Y);
    Trace(szValue);
#endif

    // Arrange the header, panel and footer.
    IFC_RETURN(m_tpHeader->Arrange(headerRect));
    IFC_RETURN(spPanel.Cast<Panel>()->Arrange(panelRect));
    IFC_RETURN(m_tpFooter->Arrange(footerRect));

    BOOLEAN isHorizontal;
    IFC_RETURN(IsHorizontal(isHorizontal));

    const bool notifySnapPointsChanges =
        isHorizontal ?
            headerRect.X != m_lastHeaderArrangeRect.X || headerRect.Width != m_lastHeaderArrangeRect.Width ||
            footerRect.X != m_lastFooterArrangeRect.X || footerRect.Width != m_lastFooterArrangeRect.Width ||
            panelRect.X !=  m_lastPanelArrangeRect.X  || panelRect.Width != m_lastPanelArrangeRect.Width :
        /* isVertical */
            headerRect.Y != m_lastHeaderArrangeRect.Y || headerRect.Height != m_lastHeaderArrangeRect.Height ||
            footerRect.Y != m_lastFooterArrangeRect.Y || footerRect.Height != m_lastFooterArrangeRect.Height ||
            panelRect.Y  != m_lastPanelArrangeRect.Y  || panelRect.Height  != m_lastPanelArrangeRect.Height;

    m_lastHeaderArrangeRect = headerRect;
    m_lastFooterArrangeRect = footerRect;
    m_lastPanelArrangeRect = panelRect;
    IFC_RETURN(DelayLoadFooter());

    *returnValue = finalSize;

    // Correct final size for non-virtualizing direction padding.
    IFC_RETURN(IncreaseSizeForPaddingInNonVirtualizingDimension(returnValue));

    if (notifySnapPointsChanges)
    {
        // Snap point might have changed, which might require to raise an event.
        IFC_RETURN(NotifySnapPointsChanges());
    }

    return S_OK;
}

// This method enforces which Panel can be inside which control
IFACEMETHODIMP ItemsPresenter::OnApplyTemplate()
{
    HRESULT hr = S_OK;
    KnownTypeIndex indexItemsControl = KnownTypeIndex::UnknownType;
    KnownTypeIndex indexPanel = KnownTypeIndex::UnknownType;
    ctl::ComPtr<IFrameworkElement> spItemsControlAsFE = nullptr;

    ctl::ComPtr<xaml_controls::IPanel> spIPanel;
    ctl::ComPtr<DependencyObject> spTemplatedParent;

    IFC(get_TemplatedParent(&spTemplatedParent));
    if (!spTemplatedParent)
    {
        goto Cleanup;
    }
    indexItemsControl = spTemplatedParent->GetTypeIndex();

    IFC(get_Panel(&spIPanel));
    if (!spIPanel)
    {
        goto Cleanup;
    }

    indexPanel = spIPanel.Cast<Panel>()->GetTypeIndex();

    switch(indexPanel)
    {
    case KnownTypeIndex::ItemsStackPanel:
        // enable KnownTypeIndex::ItemsStackPanel for ListBox (P2 scenario)
        if (KnownTypeIndex::ListBox == indexItemsControl)
        {
            hr = S_OK;
            break;
        }
        // We are not supporting ItemsWrapGrid for ListBox right now since WrapGrid isn't supported.
        // This is because we don't have the dev/test resources to invest to ensure this scenario works and support it,
        // but we may revisit this decision at some point.
    // Intentionally no break; Cut new panels from ComboBox and FlipView
    case KnownTypeIndex::ItemsWrapGrid:
    case KnownTypeIndex::WrapGrid:
        if (KnownTypeIndex::ItemsControl != indexItemsControl && KnownTypeIndex::ListView != indexItemsControl && KnownTypeIndex::GridView != indexItemsControl)
        {
            hr = E_FAIL;
        }
        break;
    case KnownTypeIndex::VariableSizedWrapGrid:
        if (KnownTypeIndex::ItemsControl != indexItemsControl && KnownTypeIndex::GridView != indexItemsControl)
        {
            hr = E_FAIL;
        }
        break;
    case KnownTypeIndex::CarouselPanel:
        if (KnownTypeIndex::ComboBox != indexItemsControl)
        {
            hr = E_FAIL;
        }
        break;
    case KnownTypeIndex::CalendarPanel:
        if (KnownTypeIndex::CalendarView != indexItemsControl)
        {
            hr = E_FAIL;
        }
        break;
    }

    if (E_FAIL == hr)
    {
        IFC(ErrorHelper::OriginateErrorUsingResourceID(E_FAIL, ERROR_INCORRECT_PANEL_FOR_CONTROL));
    }

    spItemsControlAsFE = spTemplatedParent.AsOrNull<IFrameworkElement>();
    if (spItemsControlAsFE)
    {
        ctl::ComPtr<DependencyObject> spItemsControlTemplatedParent = nullptr;
        IFC(spItemsControlAsFE.Cast<FrameworkElement>()->get_TemplatedParent(&spItemsControlTemplatedParent));
        if (ctl::is<IGroupItem>(spItemsControlTemplatedParent) &&
            ctl::is<IVirtualizingPanel>(spIPanel))
        {
            // We don't support virtualizing panels inside GroupItems.
            IFC(ErrorHelper::OriginateErrorUsingResourceID(E_FAIL, ERROR_INCORRECT_PANEL_FOR_GROUPITEM));
        }
    }

Cleanup:
    RRETURN(hr);
}

// Itemspresenter is the lynchpin in deciding whether we want to be in a non clipping subtree.
// Basically, when he decided to not want infinity, even though we were in a scrolling configuration,
// we want the measures underneath it to not constrain the desiredsize to the availablesize. That will allow
// wrapping uielements (like textblock) to size correctly, and still allow containers that want to be bigger
// to look good.
// Itemspresenter will set the value on himself (used during layout) and on the panel. Listview will set it on the
// items (so that individual measure on them will work).
bool ItemsPresenter::EvaluateAndSetNonClippingBehavior(bool isNotBeingPassedInfinity)
{
    bool result = false;
    ctl::ComPtr<IPanel> spPanel;

    if (SUCCEEDED(get_Panel(&spPanel)))
    {
        ctl::ComPtr<IItemsStackPanel> spIsp;
        spIsp = spPanel.AsOrNull<IItemsStackPanel>();

        if (spIsp)
        {
            // we only want the behavior in an isp at the moment

            // first set it to ourselves
            static_cast<CItemsPresenter*>(GetHandle())->SetIsNonClippingSubtree(isNotBeingPassedInfinity);

            // in the case of a modernpanel, lets have this itemspresenter not clip to the available size
            // in overconstrained scenarios
            // we're just passing through the setting to the panel here in an effort to make itemspresenter behave like a
            // 'dumb' passthrough. The real logic was performed when measuring the scrollcontentpresenter

            // we pass it along - the reason we want these individual elements also to have the correct value is because
            // they could potentially be measured independently
            static_cast<CItemsStackPanel*>(static_cast<DirectUI::Panel*>(spPanel.Get())->GetHandle())->SetIsNonClippingSubtree(isNotBeingPassedInfinity);

            result = isNotBeingPassedInfinity;
        }
    }

    return result;
}

// Calculates arrange location for header, footer and inner panel
_Check_return_ HRESULT ItemsPresenter::CalculateArrangeRect(
    _In_ wf::Size availableSize,
    _In_ ctl::ComPtr<IPanel>& spPanel,
    _Out_ wf::Rect* pHeaderArrangeRect,
    _Out_ wf::Rect* pPanelArrangeRect,
    _Out_ wf::Rect* pFooterArrangeRect)
{
    HRESULT hr = S_OK;
    wf::Size headerDesiredSize = {};
    wf::Size panelDesiredSize = {};
    wf::Size footerDesiredSize = {};

    wf::Size startingSize = {};
    wf::Size sizeOfFirstVisibleChild = {};

    xaml::Thickness padding = {};

    DOUBLE offset = 0.0;
    DOUBLE innerPanelOffset = 0.0;
    DOUBLE offsetForTrailingPadding = 0;
    DOUBLE firstVisibleItemSize = 0;

    BOOLEAN isHorizontal = FALSE;
    BOOLEAN isPixelBasedPanel = FALSE;

    // a virtualizing panel thinks in logical units and is expected to be arranged
    // at the size of the itemspresenter (so it can fill it up completely)
    // this is not the case for normal elements (pixel based).
    // Below you will see we subtract the rect.Y and X positions from the final size:
    // Given that we have a padding of 100 and a pixelsbased panel with width of 2000, the
    // measured size of the total will be 2100. That will be the finalsize that we get to arrange with.
    // If we were to arrange the panel with a width of 2100, we would give it more size than it originally
    // desired. Thus we calculate back to 2000 and arrange with that.
    // This flag also lets us _only_ do this to pixel based panels
    isPixelBasedPanel = ctl::is<IModernCollectionBasePanel>(spPanel) || ctl::is<IStackPanel>(spPanel);

    // current logic is to only decrease padding size in the non virtualizing dimension
    // however, in pixel based scenarios we need more correction
    // This flag lets us do an extra correction in virtualizing direction
    IFC(IsHorizontal(isHorizontal));

    if (isHorizontal)
    {
        IFC(get_HorizontalOffset(&offset));
    }
    else
    {
        IFC(get_VerticalOffset(&offset));
    }

    innerPanelOffset = DoubleUtil::Max(offset - 2, 0);
    IFC(GetInnerPanelOffset(isHorizontal, &innerPanelOffset));

    IFC(GetHeaderSize(headerDesiredSize));
    IFC(spPanel.Cast<Panel>()->get_DesiredSize(&panelDesiredSize));
    IFC(GetFooterSize(footerDesiredSize));

    // Allow header as much as possible in their non-virtualizing dimension
    // so that HorizontalAlignment can be used properly on them.
    pHeaderArrangeRect->Height = isHorizontal ? MAX(headerDesiredSize.Height, availableSize.Height) : headerDesiredSize.Height;
    pHeaderArrangeRect->Width = isHorizontal ? headerDesiredSize.Width : MAX(headerDesiredSize.Width, availableSize.Width);

    // Allow footer as much as possible in their non-virtualizing dimension
    // so that HorizontalAlignment can be used properly on them.
    pFooterArrangeRect->Height = isHorizontal ? MAX(footerDesiredSize.Height, availableSize.Height) : footerDesiredSize.Height;
    pFooterArrangeRect->Width = isHorizontal ? footerDesiredSize.Width : MAX(footerDesiredSize.Width, availableSize.Width);

    IFC(get_PaddingInternal(&padding));

    if(isHorizontal)
    {
        pHeaderArrangeRect->X = static_cast<FLOAT>(padding.Left);
        pHeaderArrangeRect->Y = static_cast<FLOAT>(padding.Top);

        pPanelArrangeRect->X = pHeaderArrangeRect->X + headerDesiredSize.Width;
        pPanelArrangeRect->Y = pHeaderArrangeRect->Y;
    }
    else
    {
        pHeaderArrangeRect->Y = static_cast<FLOAT>(padding.Top);
        pHeaderArrangeRect->X = static_cast<FLOAT>(padding.Left);

        pPanelArrangeRect->Y = pHeaderArrangeRect->Y + headerDesiredSize.Height;
        pPanelArrangeRect->X = pHeaderArrangeRect->X;
    }

    // Note: footer location is only relevant for pixel based panels.
    if(isHorizontal)
    {
        pFooterArrangeRect->X = MAX(pPanelArrangeRect->X + panelDesiredSize.Width, availableSize.Width - pFooterArrangeRect->Width - static_cast<FLOAT>(padding.Right));
        pFooterArrangeRect->Y = pHeaderArrangeRect->Y;
    }
    else
    {
        pFooterArrangeRect->Y = MAX(pPanelArrangeRect->Y + panelDesiredSize.Height, availableSize.Height - pFooterArrangeRect->Height - static_cast<FLOAT>(padding.Bottom));
        pFooterArrangeRect->X = pHeaderArrangeRect->X;
    }


    // Fix header arrange location using starting size determined using inner panel's alignments
    pHeaderArrangeRect->X += startingSize.Width;
    pHeaderArrangeRect->Y += startingSize.Height;

    // Fix header and panel start location to accommodate trailing padding
    // This is when 3 > offset - inneroffset > 2
    if (DoubleUtil::GreaterThan(offset - innerPanelOffset, 2))
    {
        IFC(GetSizeOfContainer(sizeOfFirstVisibleChild));
        firstVisibleItemSize = isHorizontal ? sizeOfFirstVisibleChild.Width : sizeOfFirstVisibleChild.Height;
        offsetForTrailingPadding = -1 * DoubleUtil::Min(1, offset - innerPanelOffset - 2) * firstVisibleItemSize;
    }

    // see comment above on the modern panels.
    pPanelArrangeRect->Height = isPixelBasedPanel && !isHorizontal ? pFooterArrangeRect->Y - pPanelArrangeRect->Y : availableSize.Height;
    pPanelArrangeRect->Width = isPixelBasedPanel && isHorizontal ? pFooterArrangeRect->X - pPanelArrangeRect->X : availableSize.Width;

Cleanup:
    RRETURN(hr);
}

// Returns true if the physical orientation is horizontal.
_Check_return_ HRESULT ItemsPresenter::IsHorizontal(
    _Out_ BOOLEAN& isHorizontal)
{
    HRESULT hr = S_OK;
    xaml_controls::Orientation orientation = xaml_controls::Orientation_Horizontal;

    IFC(get_PhysicalOrientation(&orientation));
    isHorizontal = orientation == xaml_controls::Orientation_Horizontal;

Cleanup:
    RRETURN(hr);
}

// Update scroll data after Measure.
// Fixes offset, viewport and extent based on padding and header values.
_Check_return_ HRESULT ItemsPresenter::UpdateScrollData(
    _In_ BOOLEAN isHorizontal,
    _In_ DOUBLE availableDimension)
{
    HRESULT hr = S_OK;
    ctl::ComPtr<IPanel> spPanel;
    ctl::ComPtr<IScrollInfo> spIScrollInfo;
    DOUBLE extentX = 0;
    DOUBLE extentY = 0;
    DOUBLE offsetX = 0;
    DOUBLE offsetY = 0;
    DOUBLE innerPanelScrollableX = 0;
    DOUBLE innerPanelScrollableY = 0;
    DOUBLE viewportX = 0;
    DOUBLE viewportY = 0;
    wf::Size extent = {};
    wf::Size viewport = {};
    wf::Size sizeOfFirstVisibleChild = {};
    wf::Size panelsDesiredSize = {};
    ScrollVector offset = {};
    DOUBLE trailingPaddingSize = 0;
    DOUBLE leadingPaddingSize = 0;
    DOUBLE headerAndPaddingSize = 0;
    DOUBLE outOfViewheaderAndPaddingSize = 0;
    DOUBLE inViewHeaderAndPaddingSize = 0;
    DOUBLE trailingPaddingOffset = 0;

    IFC(get_Panel(&spPanel));
    spIScrollInfo = spPanel.AsOrNull<IScrollInfo>();
    if (!spIScrollInfo)
    {
        goto Cleanup;
    }

    m_ScrollData.m_MinOffset.X = 0.0;
    m_ScrollData.m_MinOffset.Y = 0.0;

    IFC(GetSentinelItemSize(LeadingPaddingSentinelIndex, isHorizontal, leadingPaddingSize));
    IFC(GetHeaderAndLeadingPaddingPixelSize(isHorizontal, headerAndPaddingSize));

    if (leadingPaddingSize == 0)
    {
        if (isHorizontal)
        {
            m_ScrollData.m_MinOffset.X++;
        }
        else
        {
            m_ScrollData.m_MinOffset.Y++;
        }
    }

    if (headerAndPaddingSize == 0)
    {
        if (isHorizontal)
        {
            m_ScrollData.m_MinOffset.X++;
        }
        else
        {
            m_ScrollData.m_MinOffset.Y++;
        }
    }

    if (isHorizontal)
    {
        IFC(spIScrollInfo->get_VerticalOffset(&offsetY));
        offsetX = MAX(m_ScrollData.m_MinOffset.X, m_ScrollData.get_OffsetX());
    }
    else
    {
        IFC(spIScrollInfo->get_HorizontalOffset(&offsetX));
        offsetY = MAX(m_ScrollData.m_MinOffset.Y, m_ScrollData.get_OffsetY());
    }

    IFC(spPanel.Cast<Panel>()->get_DesiredSize(&panelsDesiredSize));

    IFC(GetSizeOfContainer(sizeOfFirstVisibleChild));
    IFC(GetSentinelItemSize(TrailingPaddingSentinelIndex, isHorizontal, trailingPaddingSize));

    IFC(spIScrollInfo->get_ExtentWidth(&extentX));
    IFC(spIScrollInfo->get_ExtentHeight(&extentY));
    IFC(spIScrollInfo->get_ViewportWidth(&viewportX));
    IFC(spIScrollInfo->get_ViewportHeight(&viewportY));
    IFC(GetInnerPanelsScrollableDimension(TRUE, &innerPanelScrollableX));
    IFC(GetInnerPanelsScrollableDimension(FALSE, &innerPanelScrollableY));

    if (isHorizontal)
    {
        extentX += 2; // 1 for padding and 1 for header

        if (DoubleUtil::GreaterThan(m_ScrollData.get_OffsetX() - innerPanelScrollableX, 2))
        {
            // Logical offset to allow trailing padding to be scrolled into view.
            trailingPaddingOffset = sizeOfFirstVisibleChild.Width == 0 ? 0
                                    : (trailingPaddingSize / sizeOfFirstVisibleChild.Width);
            // Correct offset to fit trailing padding
            offsetX = DoubleUtil::Min(m_ScrollData.get_OffsetX(), innerPanelScrollableX + 2 + trailingPaddingOffset);

            // Fix viewport in virtualizing direction if we have trailing padding in view.
            if (trailingPaddingSize > 0)
            {
                viewportX = extentX - trailingPaddingOffset;
            }
        }


        // panel is not scrollable.
        if (innerPanelScrollableX == 0)
        {
            DOUBLE availableDimensionForHeaderAndPadding = availableDimension - panelsDesiredSize.Width;
            ASSERT(availableDimensionForHeaderAndPadding >= 0, L"Unexpected available dimension for header and padding.");

            // case1. padding and header are in view too
            if (availableDimensionForHeaderAndPadding - (headerAndPaddingSize + trailingPaddingSize) >= 0)
            {
            viewportX += 2 - m_ScrollData.m_MinOffset.X;
            offsetX = m_ScrollData.m_MinOffset.X;
            }
            // case2. padding and header are not in view.
            // in this case our maxOffset is 2 or less.
            else
            {
                DOUBLE maxOffset = 0.0;
                DOUBLE maxPixelOffset = headerAndPaddingSize + trailingPaddingSize - availableDimensionForHeaderAndPadding;

                IFC(TranslatePixelDeltaToOffset(/*currentOffset*/0, /*isHorizontal*/TRUE, maxPixelOffset, maxOffset));
                ASSERT(maxPixelOffset == 0, L"Unexpected remaining pixel offset after translation.");

                viewportX = extentX - maxOffset;
                offsetX = MIN(offsetX, maxOffset);
                ASSERT(offsetX >= m_ScrollData.m_MinOffset.X, L"Invalid offset after coercion.");
            }
        }
        else
        {
            // Fix viewport in virtualizing direction if we have leading padding or header in view.
            if (DoubleUtil::LessThan(offsetX, 2) && availableDimension > 0)
            {
                IFC(GetHeaderAndLeadingPaddingPixelSize(isHorizontal, offsetX, outOfViewheaderAndPaddingSize));
                inViewHeaderAndPaddingSize = headerAndPaddingSize - outOfViewheaderAndPaddingSize;

                viewportX = viewportX * (availableDimension - inViewHeaderAndPaddingSize) / availableDimension;
                viewportX += 2 - offsetX;
            }
        }
    }
    else
    {
        extentY += 2; // 1 for padding and 1 for header

        if (DoubleUtil::GreaterThan(m_ScrollData.get_OffsetY() - innerPanelScrollableY, 2))
        {
            // Logical offset to allow trailing padding to be scrolled into view.
            trailingPaddingOffset = sizeOfFirstVisibleChild.Height == 0 ? 0
                                    : (trailingPaddingSize / sizeOfFirstVisibleChild.Height);
            // Correct offset to fit trailing padding
            offsetY = DoubleUtil::Min(m_ScrollData.get_OffsetY(), innerPanelScrollableY + 2 + trailingPaddingOffset);

            // Fix viewport in virtualizing direction if we have trailing padding in view.
            if (trailingPaddingSize > 0)
            {
                viewportY = extentY - trailingPaddingOffset;
            }
        }


        // panel is not scrollable.
        if (innerPanelScrollableY == 0)
        {
            DOUBLE availableDimensionForHeaderAndPadding = availableDimension - panelsDesiredSize.Height;
            ASSERT(availableDimensionForHeaderAndPadding >= 0, L"Unexpected available dimension for header and padding.");

            // case1. padding and header are in view too
            if (availableDimensionForHeaderAndPadding - (headerAndPaddingSize + trailingPaddingSize) >= 0)
            {
            viewportY += 2 - m_ScrollData.m_MinOffset.Y;
            offsetY = m_ScrollData.m_MinOffset.Y;
        }
            // case2. padding and header are not in view.
            // in this case our maxOffset is 2 or less.
            else
            {
                DOUBLE maxOffset = 0.0;
                DOUBLE maxPixelOffset = headerAndPaddingSize + trailingPaddingSize - availableDimensionForHeaderAndPadding;

                IFC(TranslatePixelDeltaToOffset(/*currentOffset*/0, /*isHorizontal*/FALSE, maxPixelOffset, maxOffset));
                ASSERT(maxPixelOffset == 0, L"Unexpected remaining pixel offset after translation.");

                viewportY = extentY - maxOffset;
                offsetY = MIN(offsetY, maxOffset);
                ASSERT(offsetY >= m_ScrollData.m_MinOffset.Y, L"Invalid offset after coercion.");
            }
        }
        else
        {
            // Fix viewport in virtualizing direction if we have leading padding or header in view.
            if (DoubleUtil::LessThan(offsetY, 2) && availableDimension > 0)
            {
                IFC(GetHeaderAndLeadingPaddingPixelSize(isHorizontal, offsetY, outOfViewheaderAndPaddingSize));
                inViewHeaderAndPaddingSize = headerAndPaddingSize - outOfViewheaderAndPaddingSize;

                viewportY = viewportY * (availableDimension - inViewHeaderAndPaddingSize) / availableDimension;
                viewportY += 2 - offsetY;
            }
        }

    }

    offset.X = offsetX;
    offset.Y = offsetY;
    extent.Height = static_cast<FLOAT>(extentY);
    extent.Width= static_cast<FLOAT>(extentX);
    viewport.Height = static_cast<FLOAT>(viewportY);
    viewport.Width = static_cast<FLOAT>(viewportX);

    IFC(SetAndVerifyScrollingData(viewport, extent, offset));

Cleanup:
    RRETURN(hr);
}

_Check_return_ HRESULT ItemsPresenter::SetAndVerifyScrollingData(
    _In_ wf::Size viewport,
    _In_ wf::Size extent,
    _In_ ScrollVector offset)
{
    HRESULT hr = S_OK;

    // Detect changes to the viewport, extent, and offset
    BOOLEAN viewportChanged = !DoubleUtil::AreClose(viewport.Height, m_ScrollData.m_viewport.Height)
        || !DoubleUtil::AreClose(viewport.Width, m_ScrollData.m_viewport.Width);

    BOOLEAN extentChanged = !DoubleUtil::AreClose(extent.Height, m_ScrollData.m_extent.Height)
        || !DoubleUtil::AreClose(extent.Width, m_ScrollData.m_extent.Width);

    BOOLEAN offsetChanged = !DoubleUtil::AreClose(offset.X, m_ScrollData.m_ComputedOffset.X)
        || !DoubleUtil::AreClose(offset.Y, m_ScrollData.m_ComputedOffset.Y);

    // Update data and fire scroll change notifications
    IFC(m_ScrollData.put_Offset(offset));
    if (viewportChanged || extentChanged || offsetChanged)
    {
        m_ScrollData.m_viewport = viewport;
        m_ScrollData.m_extent = extent;
        m_ScrollData.m_ComputedOffset = offset;

#ifdef SCROLLING_DBG
        WCHAR szValue[MAX_PATH];

        swprintf_s(szValue, MAX_PATH, L"Updating ScrollViewer with following values: Viewport = {%f, %f}, Extent = {%f, %f}, Offset = {%f, %f}\n", viewport.Width, viewport.Height, extent.Width, extent.Height, offset.X, offset.Y);
        Trace(szValue);
#endif

        IFC(OnScrollChange());
    }

Cleanup:
    RRETURN(hr);
}

// Invalidates scroll info on owner SV.
_Check_return_ HRESULT ItemsPresenter::OnScrollChange()
{
    HRESULT hr = S_OK;
    ctl::ComPtr<IScrollOwner> spScrollOwner;

    IFC(m_ScrollData.get_ScrollOwner(&spScrollOwner));
    if (spScrollOwner)
    {
        IFC(spScrollOwner->InvalidateScrollInfoImpl());
    }

Cleanup:
    RRETURN(hr);
}

// Ensures inner panel is hooked up.
_Check_return_
HRESULT
ItemsPresenter::get_Panel(_Outptr_ xaml_controls::IPanel** ppPanel)
{
    HRESULT hr = S_OK;

    ctl::ComPtr<wfc::IVector<xaml::UIElement*>> spChildren;
    ctl::ComPtr<IUIElement> spChild;
    ctl::ComPtr<IPanel> spChildAsPanel;
    BOOLEAN bTemplateApplied = FALSE;
    UINT childCount = 0;

    IFC(get_ChildrenInternal(&spChildren));
    IFC(spChildren->get_Size(&childCount));
    if (childCount == 0)
    {
        IFC(InvokeApplyTemplate(&bTemplateApplied));
        IFC(spChildren->get_Size(&childCount));
    }

    // if we just have created new panel then we need to set it's scroll owner.
    if (bTemplateApplied)
    {
        ctl::ComPtr<IScrollOwner> spScrollOwner;
        IFC(m_ScrollData.get_ScrollOwner(&spScrollOwner));

        // This call will keep current owner and update inner panel owner to this ItemsPresenter.
        IFC(put_ScrollOwnerImpl(spScrollOwner.Get()));
    }

    if (childCount == 1)
    {
        // If we have header in the visual tree then template could not be applied
        IFC(spChildren->GetAt(0, &spChild));
    }
    if (childCount == 3)
    {
        // We have a header, panel and then footer.
        IFC(spChildren->GetAt(1, &spChild));
    }

    IFC(spChild.As(&spChildAsPanel));
    IFC(spChildAsPanel.CopyTo(ppPanel));

Cleanup:
    RRETURN(hr);
}

// Returns header desired size.
_Check_return_ HRESULT ItemsPresenter::GetHeaderSize(
    _Out_ wf::Size& headerSize)
{
    HRESULT hr = S_OK;

    IFC(m_tpHeader->get_DesiredSize(&headerSize));

Cleanup:
    RRETURN(hr);
}

// Returns panel desired size.
_Check_return_ HRESULT ItemsPresenter::GetPanelSize(
    _Out_ wf::Size& panelSize)
{
    HRESULT hr = S_OK;
    ctl::ComPtr<IPanel> spPanel;

    IFC(get_Panel(&spPanel));
    IFC(spPanel.Cast<Panel>()->get_DesiredSize(&panelSize));

Cleanup:
    RRETURN(hr);
}

// Returns footer desired size.
_Check_return_ HRESULT ItemsPresenter::GetFooterSize(
    _Out_ wf::Size& footerSize)
{
    HRESULT hr = S_OK;

    IFC(m_tpFooter->get_DesiredSize(&footerSize));

Cleanup:
    RRETURN(hr);
}

// Returns leading padding + header size in pixels for virtualizing direction.
_Check_return_ HRESULT ItemsPresenter::GetHeaderAndLeadingPaddingPixelSize(
    _In_ BOOLEAN isHorizontal,
    _Out_ DOUBLE& size)
{
    HRESULT hr = S_OK;
    wf::Size headerSize = {};
    xaml::Thickness padding = {};

    size = 0;
    IFC(GetHeaderSize(headerSize));
    IFC(get_PaddingInternal(&padding));

    if (isHorizontal)
    {
        size = headerSize.Width + padding.Left;
    }
    else
    {
        size = headerSize.Height + padding.Top;
    }

Cleanup:
    RRETURN(hr);
}

// Returns leading padding + header size in pixels for virtualizing direction for given logical offset.
_Check_return_ HRESULT ItemsPresenter::GetHeaderAndLeadingPaddingPixelSize(
    _In_ BOOLEAN isHorizontal,
    _In_ DOUBLE logicalOffset,
    _Out_ DOUBLE& size)
{
    HRESULT hr = S_OK;
    wf::Size headerSize = {};
    xaml::Thickness padding = {};

    size = 0;
    if (DoubleUtil::GreaterThan(logicalOffset, 2))
    {
        goto Cleanup;
    }

    if (DoubleUtil::GreaterThan(logicalOffset, 1))
    {
        IFC(GetHeaderSize(headerSize));

        if (isHorizontal)
        {
            size += (logicalOffset - 1) * headerSize.Width;
        }
        else
        {
            size += (logicalOffset - 1) * headerSize.Height;
        }

        logicalOffset = 1;
    }

    IFC(get_PaddingInternal(&padding));
    if (isHorizontal)
    {
        size += logicalOffset * padding.Left;
    }
    else
    {
        size += logicalOffset * padding.Top;
    }

Cleanup:
    RRETURN(hr);
}

// Gets size of leading/trailing padding, header/footer or panel.
// (see the SpecialSentinelIndices enumeration)
_Check_return_ HRESULT ItemsPresenter::GetSentinelItemSize(
    _In_ INT index,
    _In_ BOOLEAN isHorizontal,
    _Out_ DOUBLE& size)
{
    HRESULT hr = S_OK;
    xaml::Thickness padding = {};

    size = 0;
    IFC(get_PaddingInternal(&padding));

    switch(index)
    {
        case PanelSentinelIndex: // panel
        {
            wf::Size panelSize = {};
            IFC(GetPanelSize(panelSize));

            if(isHorizontal)
            {
                size = panelSize.Width;
            }
            else
            {
                size = panelSize.Height;
            }
        }
        break;
        case FooterSentinelIndex: // footer
        {
            wf::Size footerSize = {};
            IFC(GetFooterSize(footerSize));

            if(isHorizontal)
            {
                size = footerSize.Width;
            }
            else
            {
                size = footerSize.Height;
            }

            break;
        }
        case TrailingPaddingSentinelIndex: // Trailing padding
        {
            if (isHorizontal)
            {
                size = padding.Right;
            }
            else
            {
                size = padding.Bottom;
            }

            break;
        }
        case LeadingPaddingSentinelIndex: // Leading padding
        {
            if (isHorizontal)
            {
                size = padding.Left;
            }
            else
            {
                size = padding.Top;
            }

            break;
        }
        case HeaderSentinelIndex: // header
        {
            wf::Size headerSize = {};
            IFC(GetHeaderSize(headerSize));

            if (isHorizontal)
            {
                size = headerSize.Width;
            }
            else
            {
                size = headerSize.Height;
            }

            break;
        }
    }

Cleanup:
    RRETURN(hr);
}

_Check_return_ HRESULT ItemsPresenter::GetZoomFactor(
    _Out_ FLOAT* pZoomFactor)
{
    HRESULT hr = S_OK;
    ctl::ComPtr<IScrollOwner> spScrollOwner;

    IFC(m_ScrollData.get_ScrollOwner(&spScrollOwner));
    if (spScrollOwner)
    {
        IFC(spScrollOwner->get_ZoomFactorImpl(pZoomFactor));
    }

Cleanup:
    RRETURN(hr);
}

// Calculates starting offset for header's arrange
// given available size, header size and inner panel's Verical/Horizontal children alignments
_Check_return_ HRESULT ItemsPresenter::ComputeAlignmentSize(
    _In_ wf::Size availableSize,
    _In_ wf::Size headerSize,
    _Out_ wf::Size* startingSize)
{
    HRESULT hr = S_OK;
    ctl::ComPtr<IPanel> spPanel;
    ctl::ComPtr<IWrapGrid> spPanelAsWG;
    ctl::ComPtr<IVariableSizedWrapGrid> spPanelAsVSWG;
    xaml::HorizontalAlignment horizontalAlignment = xaml::HorizontalAlignment::HorizontalAlignment_Left;
    xaml::VerticalAlignment verticalAlignment = xaml::VerticalAlignment::VerticalAlignment_Top;
    DOUBLE startingOffset = 0;
    BOOLEAN isHorizontal = FALSE;

    IFC(get_Panel(&spPanel));
    spPanelAsWG = spPanel.AsOrNull<IWrapGrid>();

    if (!spPanelAsWG)
    {
        spPanelAsVSWG = spPanel.AsOrNull<IVariableSizedWrapGrid>();
        if (!spPanelAsVSWG)
        {
            goto Cleanup;
        }
    }

    IFC(IsHorizontal(isHorizontal));

    if (!isHorizontal)
    {
        if (spPanelAsWG)
        {
            IFC(spPanelAsWG->get_HorizontalChildrenAlignment(&horizontalAlignment));
        }
        else
        {
            IFC(spPanelAsVSWG->get_HorizontalChildrenAlignment(&horizontalAlignment));
        }
        IFC(ComputeStartingOffset(horizontalAlignment,
                                availableSize.Width,
                                headerSize.Width,
                                &startingOffset));
        startingSize->Width = static_cast<FLOAT>(startingOffset);
    }
    else
    {
        if (spPanelAsWG)
        {
            IFC(spPanelAsWG->get_VerticalChildrenAlignment(&verticalAlignment));
        }
        else
        {
            IFC(spPanelAsVSWG->get_VerticalChildrenAlignment(&verticalAlignment));
        }
        IFC(ComputeStartingOffset(verticalAlignment,
                                availableSize.Height,
                                headerSize.Height,
                                &startingOffset));
        startingSize->Height = static_cast<FLOAT>(startingOffset);
    }

Cleanup:
    RRETURN(hr);
}

// Calculates starting offset for header's arrange
// given available size, header size and alignment
_Check_return_ HRESULT ItemsPresenter::ComputeStartingOffset(
    _In_ INT alignment,
    _In_ DOUBLE availableSize,
    _In_ DOUBLE requiredSize,
    _Out_ DOUBLE* pStartingOffset)
{
    HRESULT hr = S_OK;

    *pStartingOffset = 0;

    if (alignment == xaml::VerticalAlignment_Center)
    {
        *pStartingOffset = DoubleUtil::Max((availableSize - requiredSize) / 2.0, 0.0);
    }
    else if (alignment == xaml::VerticalAlignment_Bottom) // or HorizontalAlignment_Right
    {
        *pStartingOffset = DoubleUtil::Max(availableSize - requiredSize, 0.0);
    }

    RRETURN(hr);
}

// Returns scroll offset for inner panel
_Check_return_ HRESULT ItemsPresenter::GetInnerPanelOffset(
    _In_ BOOLEAN isHorizontal,
    _Out_ DOUBLE* pOffset)
{
    HRESULT hr = S_OK;
    ctl::ComPtr<IPanel> spPanel;
    ctl::ComPtr<IScrollInfo> spIScrollInfo;

    *pOffset = 0.0;

    IFC(get_Panel(&spPanel));
    spIScrollInfo = spPanel.AsOrNull<IScrollInfo>();

    if (!spIScrollInfo)
    {
        goto Cleanup;
    }

    if (isHorizontal)
    {
        IFC(spIScrollInfo->get_HorizontalOffset(pOffset));
    }
    else
    {
        IFC(spIScrollInfo->get_VerticalOffset(pOffset));
    }

Cleanup:
    RRETURN(hr);
}

// Returns maximum offset that can be set on inner panel
_Check_return_ HRESULT ItemsPresenter::GetInnerPanelsScrollableDimension(
    _In_ BOOLEAN isHorizontal,
    _Out_ DOUBLE* pValue)
{
    HRESULT hr = S_OK;
    ctl::ComPtr<IPanel> spPanel;
    ctl::ComPtr<IScrollInfo> spIScrollInfo;
    DOUBLE extent = 0.0;
    DOUBLE viewport = 0.0;

    IFC(get_Panel(&spPanel));
    spIScrollInfo = spPanel.AsOrNull<IScrollInfo>();

    if (!spIScrollInfo)
    {
        goto Cleanup;
    }

    if (isHorizontal)
    {
        IFC(spIScrollInfo->get_ExtentWidth(&extent));
        IFC(spIScrollInfo->get_ViewportWidth(&viewport));
    }
    else
    {
        IFC(spIScrollInfo->get_ExtentHeight(&extent));
        IFC(spIScrollInfo->get_ViewportHeight(&viewport));
    }

    *pValue = DoubleUtil::Max(0.0, extent - viewport);

Cleanup:
    RRETURN(hr);
}

_Check_return_ HRESULT ItemsPresenter::DecreaseSizeForPaddingInNonVirtualizingDimension(
    _Inout_ wf::Size* pSize)
{
    HRESULT hr = S_OK;
    xaml::Thickness padding = {};
    BOOLEAN isHorizontal = FALSE;

    IFC(IsHorizontal(isHorizontal));
    IFC(get_PaddingInternal(&padding));

    // Correct available size for non-virtualizing direction padding.
    if (isHorizontal)
    {
        pSize->Height -= static_cast<FLOAT>(padding.Top + padding.Bottom);
    }
    else
    {
        pSize->Width -= static_cast<FLOAT>(padding.Left + padding.Right);
    }

Cleanup:
    RRETURN(hr);
}

_Check_return_ HRESULT ItemsPresenter::IncreaseSizeForPaddingInNonVirtualizingDimension(
    _Inout_ wf::Size* pSize)
{
    HRESULT hr = S_OK;
    xaml::Thickness padding = {};
    BOOLEAN isHorizontal = FALSE;

    IFC(IsHorizontal(isHorizontal));
    IFC(get_PaddingInternal(&padding));

    // Correct available size for non-virtualizing direction padding.
    if (isHorizontal)
    {
        pSize->Height += static_cast<FLOAT>(padding.Top + padding.Bottom);
    }
    else
    {
        pSize->Width += static_cast<FLOAT>(padding.Left + padding.Right);
    }

Cleanup:
    RRETURN(hr);
}

_Check_return_
 HRESULT
 ItemsPresenter::ScrollIntoView(
    _In_ UINT index,
    _In_ BOOLEAN isGroupItemIndex,
    _In_ BOOLEAN isHeader,
    _In_ BOOLEAN forceSynchronous,
    _In_ DOUBLE pixelOffsetHint,
    _In_ xaml_controls::ScrollIntoViewAlignment alignment)
{
    HRESULT hr = S_OK;
    ctl::ComPtr<IPanel> spPanel;
    ctl::ComPtr<IVirtualizingPanel> spVirtualizingPanel;
    BOOLEAN isHorizontal = FALSE;
    index += isHeader ? 0 : 2;

    DOUBLE logicalOffset = index;

    IFC(IsHorizontal(isHorizontal));

    // Correct available size for non-virtualizing direction padding.
    if (isHorizontal)
    {
        if (alignment == xaml_controls::ScrollIntoViewAlignment_Default && pixelOffsetHint > 0 && m_ScrollData.get_OffsetX() < 2)
        {
            IFC(ComputeLogicalOffset(isHorizontal, pixelOffsetHint, logicalOffset));
        }

        if (logicalOffset < 2)
        {
            IFC(SetHorizontalOffsetImpl(logicalOffset));
            // to avoid using pixelOffsetHint in subsequential calls to ScrollIntoView we should have updated layout.
            // it is expensive to do but there is no way we can control 2(or more) public call to ScrollIntoView API in 1 measure cycle.
            IFC(UpdateLayout());
        }
        else
        {
            IFC(get_Panel(&spPanel));
            IFC(spPanel.As(&spVirtualizingPanel));
            IFC(spVirtualizingPanel.Cast<VirtualizingPanel>()->ScrollIntoView(index - 2, isGroupItemIndex, alignment));
            if (forceSynchronous)
            {
                IFC(UpdateLayout());
            }
        }
    }
    else
    {
        if (alignment == xaml_controls::ScrollIntoViewAlignment_Default && pixelOffsetHint > 0 && m_ScrollData.get_OffsetY() < 2)
        {
            IFC(ComputeLogicalOffset(isHorizontal, pixelOffsetHint, logicalOffset));
        }

        if (logicalOffset < 2)
        {
            IFC(SetVerticalOffsetImpl(logicalOffset));
            // to avoid using pixelOffsetHint in subsequential calls to ScrollIntoView we should have updated layout.
            // it is expensive to do but there is no way we can control 2(or more) public call to ScrollIntoView API in 1 measure cycle.
            IFC(UpdateLayout());
        }
        else
        {
            IFC(get_Panel(&spPanel));
            IFC(spPanel.As(&spVirtualizingPanel));
            IFC(spVirtualizingPanel.Cast<VirtualizingPanel>()->ScrollIntoView(index - 2, isGroupItemIndex, alignment));
            if (forceSynchronous)
            {
                IFC(UpdateLayout());
            }
        }
    }

Cleanup:
    RRETURN(hr);
}

_Check_return_
HRESULT
ItemsPresenter::get_PaddingInternal(_Out_ xaml::Thickness* pValue)
{
    HRESULT hr = S_OK;
    ctl::ComPtr<IPanel> spPanel;

    IFC(get_Padding(pValue));

    IFC(get_Panel(&spPanel));

    if (!ctl::is<IModernCollectionBasePanel>(spPanel))
    {
        // The Win8 logical virtualizing panels had issues with padding on the end of their scrollable extent
        // So, preserve their workaround. Win8 also disabled this for StackPanel, and it's not worth quirking right now.
        BOOLEAN isHorizontal = FALSE;

        IFC(IsHorizontal(isHorizontal));

        if (isHorizontal)
        {
            pValue->Left = MAX(0, pValue->Left);
            pValue->Right = 0;
        }
        else
        {
            pValue->Top = MAX(0, pValue->Top);
            pValue->Bottom = 0;
        }
    }

Cleanup:
    RRETURN(hr);
}

// Called when an ItemsControl's ContentTemplate changes.
// Allows a hook for us to clean up the old ItemsPresenter before layout is run with the
// new template and new ItemsPresenter.
// If Header is a UIElement, we must remove its association with m_tpHeader here.
// Otherwise, applying the new template and measuring the new ItemsPresenter will throw.
_Check_return_ HRESULT
ItemsPresenter::Dispose(
    _In_ CItemsPresenter* pNativeItemsPresenter)
{
    ctl::ComPtr<DependencyObject> spPeer = nullptr;
    // ItemPresenter peer may already have been released when the ControlTemplate was replaced.
    IFC_RETURN(DXamlCore::GetCurrent()->TryGetPeer(pNativeItemsPresenter, &spPeer));

    if (spPeer)
    {
        auto pItemsPresenter = static_cast<ItemsPresenter*>(spPeer.Get());

        IFC_RETURN(pItemsPresenter->m_tpHeader->put_Content(nullptr));
        IFC_RETURN(pItemsPresenter->m_tpHeader->put_ContentTemplate(nullptr));
        IFC_RETURN(pItemsPresenter->m_tpHeader->ClearValueByKnownIndex(KnownPropertyIndex::ContentControl_ContentTransitions));

        IFC_RETURN(pItemsPresenter->m_tpFooter->put_Content(nullptr));
        IFC_RETURN(pItemsPresenter->m_tpFooter->put_ContentTemplate(nullptr));
        IFC_RETURN(pItemsPresenter->m_tpFooter->ClearValueByKnownIndex(KnownPropertyIndex::ContentControl_ContentTransitions));
    }

    return S_OK;
}

_Check_return_ HRESULT ItemsPresenter::get_HeaderContainer(_Outptr_ ContentControl** ppHeaderContainer)
{
    RRETURN(m_tpHeader.CopyTo(ppHeaderContainer));
}

_Check_return_ HRESULT ItemsPresenter::get_FooterContainer(_Outptr_ ContentControl** ppFooterContainer)
{
    RRETURN(m_tpFooter.CopyTo(ppFooterContainer));
}

// Returns the first visible part (header, panel or footer) of this ItemsPresenter.
_Check_return_ HRESULT
ItemsPresenter::GetFirstVisiblePart(
    _Out_ ItemsPresenterParts* pPart,
    _Out_ DOUBLE* pOffset)
{
    HRESULT hr = S_OK;

    auto spScrollViewer = m_wrScrollViewer.AsOrNull<IScrollViewer>();

    // Default return value.
    *pPart = ItemsPresenterParts_Header;
    *pOffset = 0.0;

    if (spScrollViewer)
    {
        BOOLEAN isHorizontal = FALSE;
        DOUBLE viewportOffset = 0.0;
        DOUBLE headerOffset = 0.0;
        DOUBLE panelOffset = 0.0;
        DOUBLE footerOffset = 0.0;

        IFC(IsHorizontal(isHorizontal));

        // Retrieves offset values depending of the panel orientation.
        if (isHorizontal)
        {
            IFC(spScrollViewer->get_HorizontalOffset(&viewportOffset));
            headerOffset = m_lastHeaderArrangeRect.X;
            panelOffset = m_lastHeaderArrangeRect.X + m_lastHeaderArrangeRect.Width;
            footerOffset = m_lastFooterArrangeRect.X;
        }
        else
        {
            IFC(spScrollViewer->get_VerticalOffset(&viewportOffset));
            headerOffset = m_lastHeaderArrangeRect.Y;
            panelOffset = m_lastHeaderArrangeRect.Y + m_lastHeaderArrangeRect.Height;
            footerOffset = m_lastFooterArrangeRect.Y;
        }

        if (viewportOffset < panelOffset)
        {
            // pPart is already set to ItemsPresenterPart_Header.
            *pOffset = viewportOffset - headerOffset;
        }
        else if (viewportOffset < footerOffset)
        {
            *pPart = ItemsPresenterParts_Panel;
            *pOffset = viewportOffset - panelOffset;
        }
        else
        {
            *pPart = ItemsPresenterParts_Footer;
            *pOffset = viewportOffset - footerOffset;
        }
    }

Cleanup:
    RRETURN(hr);
}
