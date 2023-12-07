// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#include "precomp.h"
#include "SplitView.g.h"

// Core headers.
#include "SplitView.h"

// DXaml headers
#include "LineSegment.g.h"
#include "Grid.g.h"
#include "Path.g.h"
#include "PathFigure.g.h"
#include "PathGeometry.g.h"
#include "Popup.g.h"
#include "SolidColorBrush.g.h"
#include "Window.g.h"
#include "Activators.g.h"
#include "XboxUtility.h"
#include "KeyRoutedEventArgs.g.h"
#include "focusmgr.h"
#include <FrameworkUdk/BackButtonIntegration.h>
#include <XYFocus.h>
#include <LightDismissOverlayHelper.h>
#include "XamlRoot_Partial.h"
#include "ElementSoundPlayerService_Partial.h"

using namespace DirectUI;
using namespace Focus;
using namespace std::placeholders;

// Index table as follows: [DisplayMode][Placement][IsOpen]
static const wchar_t* const g_visualStateTable[4][2][2] =
{
    // Overlay
    {
        { L"Closed", L"OpenOverlayLeft" },
        { L"Closed", L"OpenOverlayRight" }
    },

    // Inline
    {
        { L"Closed", L"OpenInlineLeft" },
        { L"Closed", L"OpenInlineRight" }
    },

    // CompactOverlay
    {
        { L"ClosedCompactLeft", L"OpenCompactOverlayLeft" },
        { L"ClosedCompactRight", L"OpenCompactOverlayRight" }
    },

    // CompactInline
    {
        { L"ClosedCompactLeft", L"OpenInlineLeft" },
        { L"ClosedCompactRight", L"OpenInlineRight" }
    }
};

SplitView::~SplitView()
{
    auto xamlRoot = XamlRoot::GetForElementStatic(this);
    if (m_xamlRootChangedEventHandler && xamlRoot)
    {
        VERIFYHR(m_xamlRootChangedEventHandler.DetachEventHandler(xamlRoot.Get()));
    }

    VERIFYHR(BackButtonIntegration_UnregisterListener(this));
}

_Check_return_ HRESULT
SplitView::PrepareState()
{
    IFC_RETURN(m_loadedEventHandler.AttachEventHandler(this, std::bind(&SplitView::OnLoaded, this, _1, _2)));
    IFC_RETURN(m_unloadedEventHandler.AttachEventHandler(this, std::bind(&SplitView::OnUnloaded, this, _1, _2)));
    IFC_RETURN(m_sizeChangedEventHandler.AttachEventHandler(this, std::bind(&SplitView::OnSizeChanged, this, _1, _2)));

    return S_OK;
}

IFACEMETHODIMP
SplitView::OnApplyTemplate()
{
    IFC_RETURN(__super::OnApplyTemplate());

    m_tpPaneRoot.Clear();
    m_tpContentRoot.Clear();

    if (m_displayModeStateChangedEventHandler)
    {
        IFC_RETURN(m_displayModeStateChangedEventHandler.DetachEventHandler(m_tpDisplayModeStates.Get()));
        m_tpDisplayModeStates.Clear();
    }

    // Set window pattern on 'PaneRoot' element, if the splitview set to lightdismiss
    ctl::ComPtr<xaml::IFrameworkElement> paneRootAsFE;
    IFC_RETURN(GetTemplatePart<xaml::IFrameworkElement>(STR_LEN_PAIR(L"PaneRoot"), paneRootAsFE.ReleaseAndGetAddressOf()));
    if (paneRootAsFE)
    {
        SetPtrValue(m_tpPaneRoot, paneRootAsFE.Get());
        IFC_RETURN(paneRootAsFE.Cast<FrameworkElement>()->put_AutomationPeerFactoryIndex(static_cast<INT>(KnownTypeIndex::SplitViewPaneAutomationPeer)));
    }

    ctl::ComPtr<xaml::IFrameworkElement> lightDismissLayerAsFE;
    IFC_RETURN(GetTemplatePart<xaml::IFrameworkElement>(STR_LEN_PAIR(L"LightDismissLayer"), lightDismissLayerAsFE.ReleaseAndGetAddressOf()));
    if (lightDismissLayerAsFE)
    {
        IFC_RETURN(lightDismissLayerAsFE.Cast<FrameworkElement>()->put_AutomationPeerFactoryIndex(static_cast<INT>(KnownTypeIndex::SplitViewLightDismissAutomationPeer)));

        auto uiElementCore = static_cast<CUIElement*>(lightDismissLayerAsFE.Cast<FrameworkElement>()->GetHandle());
        uiElementCore->SetAllowsDragAndDropPassThrough(true);
    }

    ctl::ComPtr<xaml::IFrameworkElement> contentRootAsFE;
    IFC_RETURN(GetTemplatePart<xaml::IFrameworkElement>(STR_LEN_PAIR(L"ContentRoot"), contentRootAsFE.ReleaseAndGetAddressOf()));
    if (contentRootAsFE)
    {
        SetPtrValue(m_tpContentRoot, contentRootAsFE.Get());
    }

    return S_OK;
}

IFACEMETHODIMP
SplitView::OnKeyDown(
    _In_ xaml_input::IKeyRoutedEventArgs* pArgs)
{
    BOOLEAN isHandled = FALSE;

    IFCPTR_RETURN(pArgs);
    IFC_RETURN(pArgs->get_Handled(&isHandled));

    if (!isHandled)
    {
        auto displayMode = xaml_controls::SplitViewDisplayMode_Overlay;
        IFC_RETURN(get_DisplayMode(&displayMode));
        if (displayMode == xaml_controls::SplitViewDisplayMode_CompactInline ||
            displayMode == xaml_controls::SplitViewDisplayMode_CompactOverlay)
        {
            wsy::VirtualKey originalKey = wsy::VirtualKey_None;
            IFC_RETURN(static_cast<KeyRoutedEventArgs*>(pArgs)->get_OriginalKey(&originalKey));

            if (XboxUtility::IsGamepadNavigationDirection(originalKey))
            {
                xaml_input::FocusNavigationDirection gamepadDirection = XboxUtility::GetNavigationDirection(originalKey);
                ctl::ComPtr<SplitView> spSplitView(this);

                CFocusManager* pFocusManager = VisualTree::GetFocusManagerForElement(GetHandle());

                Focus::XYFocusOptions xyFocusOptions;
                xyFocusOptions.searchRoot = spSplitView.Cast<DependencyObject>()->GetHandle();
                xyFocusOptions.shouldConsiderXYFocusKeyboardNavigation = true;
                xyFocusOptions.ignoreClipping = false;
                xyFocusOptions.ignoreCone = true;

                CDependencyObject* const pCandidate = pFocusManager->FindNextFocus((DirectUI::FocusNavigationDirection)gamepadDirection, xyFocusOptions);

                if (pCandidate && m_tpPaneRoot && m_tpContentRoot)
                {
                    // we got a candidate
                    BOOLEAN shouldHandle = FALSE;
                    BOOLEAN paneHasFocus = FALSE;
                    BOOLEAN contentHasFocus = FALSE;
                    ctl::ComPtr<DependencyObject> spCandidateDO = NULL;

                    IFC_RETURN(DXamlCore::GetCurrent()->GetPeer(pCandidate, &spCandidateDO));
                    // if pane has focus and candidate is from content (or)
                    // content has focus and candidate is from the pane
                    // Then handle the key and move focus. If not, do not handle and let it bubble up to auto-focus
                    IFC_RETURN(m_tpPaneRoot.Cast<FrameworkElement>()->HasFocus(&paneHasFocus));
                    if (paneHasFocus)
                    {
                        IFC_RETURN(m_tpContentRoot.Cast<FrameworkElement>()->IsAncestorOf(spCandidateDO.Get(), &shouldHandle));
                    }

                    if (!shouldHandle)
                    {
                        IFC_RETURN(m_tpContentRoot.Cast<FrameworkElement>()->HasFocus(&contentHasFocus));
                        if (contentHasFocus)
                        {
                            IFC_RETURN(m_tpPaneRoot.Cast<FrameworkElement>()->IsAncestorOf(spCandidateDO.Get(), &shouldHandle));
                        }
                    }

                    if (shouldHandle)
                    {
                        BOOLEAN focusUpdated = FALSE;
                        IFC_RETURN(SetFocusedElementWithDirection(spCandidateDO.Get(), xaml::FocusState_Keyboard, TRUE /* animateBringIntoView */, &focusUpdated, gamepadDirection));
                        IFC_RETURN(pArgs->put_Handled(focusUpdated));
                    }
                }
            }
        }
    }

    return S_OK;
}

_Check_return_ HRESULT
SplitView::OnLoaded(_In_ IInspectable* /*pSender*/, _In_ xaml::IRoutedEventArgs* /*pArgs*/)
{
    auto splitViewCore = static_cast<CSplitView*>(GetHandle());

    // Creates the Outer Dismiss Layer when we're setting our initial state
    BOOLEAN isPaneOpen = FALSE;
    IFC_RETURN(get_IsPaneOpen(&isPaneOpen));

    if (isPaneOpen && splitViewCore->CanLightDismiss())
    {
        IFC_RETURN(SetupOuterDismissLayer());
    }

    auto xamlRoot = XamlRoot::GetForElementStatic(this);
    if (!m_xamlRootChangedEventHandler && xamlRoot)
    {
        IFC_RETURN(m_xamlRootChangedEventHandler.AttachEventHandler(xamlRoot.Get(), std::bind(&SplitView::OnXamlRootChanged, this, _1, _2)));
    }

    return S_OK;
}

_Check_return_ HRESULT
SplitView::OnUnloaded(_In_ IInspectable* /*pSender*/, _In_ xaml::IRoutedEventArgs* /*pArgs*/)
{
    return TeardownOuterDismissLayer();
}

_Check_return_ HRESULT
SplitView::OnSizeChanged(_In_ IInspectable* /*pSender*/, _In_ xaml::ISizeChangedEventArgs* pArgs )
{
    wf::Size prevSize = {};
    IFC_RETURN(pArgs->get_PreviousSize(&prevSize));

    auto splitViewCore = static_cast<CSplitView*>(GetHandle());

    // Light dismiss only if we're not setting our initial size.
    if ((prevSize.Width != 0 || prevSize.Height != 0) && splitViewCore->CanLightDismiss())
    {
        IFC_RETURN(splitViewCore->TryCloseLightDismissiblePane());
    }

    return S_OK;
}

_Check_return_ HRESULT
SplitView::OnXamlRootChanged(_In_ xaml::IXamlRoot* /*pSender*/, _In_ xaml::IXamlRootChangedEventArgs* /*pArgs*/ )
{
    auto splitViewCore = static_cast<CSplitView*>(GetHandle());
    if (splitViewCore->CanLightDismiss())
    {
        IFC_RETURN(splitViewCore->TryCloseLightDismissiblePane());
    }

    return S_OK;
}

_Check_return_ HRESULT
SplitView::OnOuterDismissElementPointerPressed(IInspectable* /*pSender*/, xaml_input::IPointerRoutedEventArgs* /*pArgs*/)
{
    auto splitViewCore = static_cast<CSplitView*>(GetHandle());
    if (splitViewCore->CanLightDismiss())
    {
        IFC_RETURN(splitViewCore->TryCloseLightDismissiblePane());
    }

    return S_OK;
}

_Check_return_ HRESULT
SplitView::OnDisplayModeStateChanged(_In_ IInspectable* /*sender*/, _In_ xaml::IVisualStateChangedEventArgs* /*args*/)
{
    // Only respond to visual state changes between opened and closed states.
    // We could get state changes between opened states if display mode is changed (such as going from Compact to Overlay)
    // while the pane is open.
    if (m_isPaneOpeningOrClosing)
    {
        m_isPaneOpeningOrClosing = false;

        BOOLEAN isPaneOpen = FALSE;
        IFC_RETURN(get_IsPaneOpen(&isPaneOpen));
        IFC_RETURN(OnPaneOpenedOrClosed(!!isPaneOpen));
    }

    return S_OK;
}

// First tab stop processing pass that attempts to keep focus within
// the open pane when the SplitView is in a light-dismissible display
// mode (Overlay & CompactOverlay).
_Check_return_ HRESULT
SplitView::ProcessTabStopOverride(
    _In_opt_ DependencyObject* pFocusedElement,
    _In_opt_ DependencyObject* pCandidateTabStopElement,
    const bool isBackward,
    const bool /*didCycleFocusAtRootVisualScope*/,
    _Outptr_ DependencyObject** ppNewTabStop,
    _Out_ BOOLEAN* pIsTabStopOverridden
    )
{
    IFC_RETURN(ProcessTabStopInternal(pFocusedElement, pCandidateTabStopElement, !isBackward, ppNewTabStop));
    *pIsTabStopOverridden = (*ppNewTabStop != nullptr);

    return S_OK;
}

// Second tab stop processing pass that handles the case where a child element
// has overridden the tab-stop candidate but that new candidate isn't a child
// of the SplitView pane. SplitView will override that new tab-stop candidate
// to make sure focus stays within the pane.
_Check_return_ HRESULT
SplitView::ProcessCandidateTabStopOverride(
    _In_opt_ DependencyObject* pFocusedElement,
    _In_ DependencyObject* /*pCandidateTabStopElement*/,
    _In_opt_ DependencyObject* pOverriddenCandidateTabStopElement,
    const bool isBackward,
    _Outptr_ DependencyObject** ppNewTabStop,
    _Out_ BOOLEAN* pIsCandidateTabStopOverridden
    )
{
    if (pOverriddenCandidateTabStopElement != nullptr)
    {
        IFC_RETURN(ProcessTabStopInternal(pFocusedElement, pOverriddenCandidateTabStopElement, !isBackward, ppNewTabStop));
        *pIsCandidateTabStopOverridden = (*ppNewTabStop != nullptr);
    }

    return S_OK;
}

_Check_return_ HRESULT
SplitView::ProcessTabStopInternal(
    _In_opt_ DependencyObject* pFocusedElement,
    _In_opt_ DependencyObject* pCandidateTabStopElement,
    bool isForward,
    _Outptr_result_maybenull_ DependencyObject** ppNewTabStop
    )
{
    xref_ptr<CDependencyObject> spNewTabStop;

    *ppNewTabStop = nullptr;

    spNewTabStop.attach(static_cast<CSplitView*>(GetHandle())->ProcessTabStop(
        isForward,
        pFocusedElement ? pFocusedElement->GetHandle() : nullptr,
        pCandidateTabStopElement ? pCandidateTabStopElement->GetHandle() : nullptr));

    // Check to see if we overrode the tab stop candidate.
    if (spNewTabStop)
    {
        IFC_RETURN(DXamlCore::GetCurrent()->GetPeer(spNewTabStop, ppNewTabStop));
    }

    return S_OK;
}

_Check_return_ HRESULT
SplitView::GetFirstFocusableElementOverride(_Outptr_result_maybenull_ DependencyObject** firstFocusable)
{
    *firstFocusable = nullptr;

    // If the splitview is open and light-dismissible, then always send focus to the pane.
    auto splitViewCore = static_cast<CSplitView*>(GetHandle());
    if (splitViewCore->CanLightDismiss())
    {
        xref_ptr<CDependencyObject> element;
        splitViewCore->GetFirstFocusableElementFromPane(element.ReleaseAndGetAddressOf());
        if (element)
        {
            IFC_RETURN(DXamlCore::GetCurrent()->GetPeer(element, firstFocusable));
        }
    }
    else
    {
        // Fallback to the default behavior.
        IFC_RETURN(__super::GetFirstFocusableElementOverride(firstFocusable));
    }

    return S_OK;
}

_Check_return_ HRESULT
SplitView::GetLastFocusableElementOverride(_Outptr_ DependencyObject** lastFocusable)
{
    *lastFocusable = nullptr;

    // If the splitview is open and light-dismissible, then always send focus to the pane.
    auto splitViewCore = static_cast<CSplitView*>(GetHandle());
    if (splitViewCore->CanLightDismiss())
    {
        xref_ptr<CDependencyObject> element;
        splitViewCore->GetLastFocusableElementFromPane(element.ReleaseAndGetAddressOf());
        if (element)
        {
            IFC_RETURN(DXamlCore::GetCurrent()->GetPeer(element, lastFocusable));
        }
    }
    else
    {
        // Fallback to the default behavior.
        IFC_RETURN(__super::GetLastFocusableElementOverride(lastFocusable));
    }

    return S_OK;
}

_Check_return_ HRESULT
SplitView::OnBackButtonPressedImpl(_Out_ BOOLEAN* pHandled)
{
    IFCPTR_RETURN(pHandled);

    bool canLightDismiss = static_cast<CSplitView*>(GetHandle())->CanLightDismiss();
    if (canLightDismiss)
    {
        IFC_RETURN(put_IsPaneOpen(FALSE));
        *pHandled = TRUE;
    }

    return S_OK;
}

_Check_return_ HRESULT SplitView::OnPropertyChanged2(_In_ const PropertyChangedParams& args)
{
    IFC_RETURN(SplitViewGenerated::OnPropertyChanged2(args));

    switch (args.m_pDP->GetIndex())
    {
        case KnownPropertyIndex::SplitView_IsPaneOpen:
            IFC_RETURN(OnIsPaneOpenChanged(!!args.m_pNewValue->AsBool()));
            break;

        case KnownPropertyIndex::SplitView_DisplayMode:
            IFC_RETURN(OnDisplayModeChanged());
            break;
    }

    return S_OK;
}

_Check_return_ HRESULT
SplitView::ChangeVisualState(_In_ bool useTransitions)
{
    BOOLEAN ignored = FALSE;

    // DisplayModeStates
    {
        auto displayMode = xaml_controls::SplitViewDisplayMode_Overlay;
        IFC_RETURN(get_DisplayMode(&displayMode));

        auto placement = xaml_controls::SplitViewPanePlacement_Left;
        IFC_RETURN(get_PanePlacement(&placement));

        BOOLEAN isPaneOpen = FALSE;
        IFC_RETURN(get_IsPaneOpen(&isPaneOpen));

        // Look up the visual state based on display mode, placement and, ispaneopen state.
        auto visualStateName = g_visualStateTable[static_cast<size_t>(displayMode)][static_cast<size_t>(placement)][static_cast<size_t>(isPaneOpen)];
        IFC_RETURN(GoToState(useTransitions, visualStateName, &ignored));
    }

    // OverlayVisibilityStates
    {
        bool isOverlayVisible = false;
        IFC_RETURN(LightDismissOverlayHelper::ResolveIsOverlayVisibleForControl(this, &isOverlayVisible));
        IFC_RETURN(GoToState(useTransitions, isOverlayVisible ? L"OverlayVisible" : L"OverlayNotVisible", &ignored));
    }

    return S_OK;
}

_Check_return_ HRESULT
SplitView::OnIsPaneOpenChanged(bool isOpen)
{
    IFC_RETURN(RegisterForDisplayModeStatesChangedEvent());

    m_isPaneOpeningOrClosing = true;

    IFC_RETURN(UpdateVisualState());

    if (isOpen)
    {
        // Raise the PaneOpening event.
        PaneOpeningEventSourceType* eventSource = nullptr;
        IFC_RETURN(GetPaneOpeningEventSourceNoRef(&eventSource));
        IFC_RETURN(eventSource->Raise(this, nullptr /*args*/));

        // Call into the core object to set focus to the pane on opening.
        IFC_RETURN(static_cast<CSplitView*>(GetHandle())->OnPaneOpening());

        if (DXamlCore::GetCurrent()->GetHandle()->BackButtonSupported())
        {
            IFC_RETURN(BackButtonIntegration_RegisterListener(this));
        }
        IFC_RETURN(SetupOuterDismissLayer());
    }
    else
    {
        // Call into the core object to raise the PaneClosing event, which is raised asynchronously.
        // This will also restore focus to whichever element had focus when the pane opened.
        IFC_RETURN(static_cast<CSplitView*>(GetHandle())->OnPaneClosing());

        IFC_RETURN(BackButtonIntegration_UnregisterListener(this));
        IFC_RETURN(TeardownOuterDismissLayer());
    }

    // If the display modes states changing event was not registered, then
    // do the opened/closed work here instead. This could be the case if
    // the SplitView has been re-templated to remove the 'DisplayModeStates'
    // state group.
    if (!m_displayModeStateChangedEventHandler)
    {
        IFC_RETURN(OnPaneOpenedOrClosed(isOpen));
    }

    return S_OK;
}

_Check_return_ HRESULT
SplitView::OnDisplayModeChanged()
{
    if (static_cast<CSplitView*>(GetHandle())->CanLightDismiss())
    {
        IFC_RETURN(SetupOuterDismissLayer());
    }
    else
    {
        IFC_RETURN(TeardownOuterDismissLayer());
    }

    return S_OK;
}

_Check_return_ HRESULT
SplitView::SetupOuterDismissLayer() noexcept
{
    // If we're not in a light-dismissible state, then we can bail out of
    // setting up this dismiss layer.
    if (!static_cast<CSplitView*>(GetHandle())->CanLightDismiss())
    {
        return S_OK;
    }

    if (!IsInLiveTree())
    {
        return S_OK;
    }

    // To detect input outside of the SplitView's bounds, we create
    // a popup layer that hosts 4 polygonal elements arranged around
    // the SplitView.  The Top and Bottom elements are defined by 6
    // points, while the Left and Right elements are defined by 4 points.
    // We make them polygons rather than rectangles to account for any
    // transforms that may be applied to the SplitView (such as skew).
    //
    // Pointer handlers are attached to the elements and dismiss the
    // pane if activated.
    //
    // We try to create only the elements that are needed, for example,
    // a full screen SplitView does not need any, so we bail early.
    // Some apps that have nested SplitViews might not be full screen.
    // A notable example is Spartan where their SplitView is stacked
    // underneath the title/menu bar.  In that case, only the top
    // element should be needed.
    //
    //    X-------------------------------X
    //    |                               |
    //    |              Top              |
    //    |                               |
    //    X-------X---------------X-------X
    //    |       |               |       |
    //    | Left  |   SplitView   | Right |
    //    |       |               |       |
    //    X-------X---------------X-------X
    //    |                               |
    //    |             Bottom            |
    //    |                               |
    //    X-------------------------------X
    //
    // TODO: At better solution would likely involve some coordination
    // with the input manager, however at the time of writing this, that
    // was a riskier option.  We ultimately went with this solution because
    // it is relatively low risk, but we should revisit this in some future MQ.
    // Task 2386326 has been opened to track improvements in this area.

    wf::Rect windowBounds = {};
    IFC_RETURN(DXamlCore::GetCurrent()->GetContentBoundsForElement(GetHandle(), &windowBounds));

    double actualWidth = 0.0;
    double actualHeight = 0.0;
    IFC_RETURN(get_ActualWidth(&actualWidth));
    IFC_RETURN(get_ActualHeight(&actualHeight));

    ctl::ComPtr<xaml_media::IGeneralTransform> transformToVisual;
    IFC_RETURN(TransformToVisual(nullptr, &transformToVisual));

    // Transform all 4 corners of the SplitView's bounds.
    wf::Point topLeftCorner = {};
    wf::Point topRightCorner = {};
    wf::Point bottomRightCorner = {};
    wf::Point bottomLeftCorner = {};

    auto flowDirection = xaml::FlowDirection_LeftToRight;
    IFC_RETURN(get_FlowDirection(&flowDirection));
    if (flowDirection == xaml::FlowDirection_LeftToRight)
    {
        IFC_RETURN(transformToVisual->TransformPoint({ 0.f, 0.f }, &topLeftCorner));
        IFC_RETURN(transformToVisual->TransformPoint({ static_cast<float>(actualWidth), 0.f }, &topRightCorner));
        IFC_RETURN(transformToVisual->TransformPoint({ static_cast<float>(actualWidth), static_cast<float>(actualHeight) }, &bottomRightCorner));
        IFC_RETURN(transformToVisual->TransformPoint({ 0.f, static_cast<float>(actualHeight) }, &bottomLeftCorner));
    }
    else
    {
        IFC_RETURN(transformToVisual->TransformPoint({ static_cast<float>(actualWidth), 0.f }, &topLeftCorner));
        IFC_RETURN(transformToVisual->TransformPoint({ 0.f, 0.f }, &topRightCorner));
        IFC_RETURN(transformToVisual->TransformPoint({ 0.f, static_cast<float>(actualHeight) }, &bottomRightCorner));
        IFC_RETURN(transformToVisual->TransformPoint({ static_cast<float>(actualWidth), static_cast<float>(actualHeight) }, &bottomLeftCorner));
    }

    // Determine which elements we need based on the translated corner points.
    bool needsTop = topLeftCorner.Y > 0 || topRightCorner.Y > 0;
    bool needsBottom = bottomLeftCorner.Y < windowBounds.Height || bottomRightCorner.Y < windowBounds.Height;
    bool needsLeft = topLeftCorner.X > 0 || bottomLeftCorner.X > 0;
    bool needsRight = topRightCorner.X < windowBounds.Width || bottomRightCorner.X < windowBounds.Width;

    // If the SplitView doesn't need any of the 4 elements, then it takes up the entire
    /// window, so we can bail early.
    if (!needsTop && !needsBottom && !needsLeft && !needsRight)
    {
        return S_OK;
    }

    if (!m_dismissHostElement)
    {
        ctl::ComPtr<Grid> grid;
        IFC_RETURN(ctl::make<Grid>(&grid));
        IFC_RETURN(grid.As(&m_dismissHostElement));

        IFC_RETURN(m_dismissLayerPointerPressedEventHandler.AttachEventHandler(
            grid.Get(),
            std::bind(&SplitView::OnOuterDismissElementPointerPressed, this, _1, _2)
            ));
    }

    if (!m_outerDismissLayerPopup)
    {
        ctl::ComPtr<Popup> popup;
        IFC_RETURN(ctl::make(&popup));
        IFC_RETURN(popup->put_Child(m_dismissHostElement.Cast<Grid>()));
        IFC_RETURN(popup.As(&m_outerDismissLayerPopup));
    }

    static_cast<CPopup*>(m_outerDismissLayerPopup.Cast<DirectUI::Popup>()->GetHandle())->SetAssociatedIsland(VisualTree::GetXamlIslandRootForElement(GetHandle()));

    IFC_RETURN(m_outerDismissLayerPopup->put_IsOpen(TRUE));

    if (needsTop)
    {
        if (m_topDismissElement)
        {
            IFC_RETURN(m_topDismissElement.Cast<Path>()->put_Visibility(xaml::Visibility_Visible));
        }
        else
        {
            IFC_RETURN(CreatePolygonalPath(m_dismissHostElement.Get(), 6, &m_topDismissElement));
        }

        wf::Point points[] = { { 0.f, 0.f },{ windowBounds.Width, 0.f },{ windowBounds.Width, topRightCorner.Y }, topRightCorner, topLeftCorner,{ 0.f, topLeftCorner.Y } };
        IFC_RETURN(UpdatePolygonalPath(m_topDismissElement.Get(), ARRAYSIZE(points), points));
    }
    else if (m_topDismissElement)
    {
        IFC_RETURN(m_topDismissElement.Cast<Path>()->put_Visibility(xaml::Visibility_Collapsed));
    }

    if (needsBottom)
    {
        if (m_bottomDismissElement)
        {
            IFC_RETURN(m_bottomDismissElement.Cast<Path>()->put_Visibility(xaml::Visibility_Visible));
        }
        else
        {
            IFC_RETURN(CreatePolygonalPath(m_dismissHostElement.Get(), 6, &m_bottomDismissElement));
        }

        wf::Point points[] = { { 0.f, bottomLeftCorner.Y }, bottomLeftCorner, bottomRightCorner,{ windowBounds.Width, bottomRightCorner.Y },{ windowBounds.Width, windowBounds.Height },{ 0.f, windowBounds.Height } };
        IFC_RETURN(UpdatePolygonalPath(m_bottomDismissElement.Get(), ARRAYSIZE(points), points));
    }
    else if (m_bottomDismissElement)
    {
        IFC_RETURN(m_bottomDismissElement.Cast<Path>()->put_Visibility(xaml::Visibility_Collapsed));
    }

    if (needsLeft)
    {
        if (m_leftDismissElement)
        {
            IFC_RETURN(m_leftDismissElement.Cast<Path>()->put_Visibility(xaml::Visibility_Visible));
        }
        else
        {
            IFC_RETURN(CreatePolygonalPath(m_dismissHostElement.Get(), 4, &m_leftDismissElement));
        }

        wf::Point points[] = { { 0.f, topLeftCorner.Y }, topLeftCorner, bottomLeftCorner,{ 0.f, bottomLeftCorner.Y } };
        IFC_RETURN(UpdatePolygonalPath(m_leftDismissElement.Get(), ARRAYSIZE(points), points));
    }
    else if (m_leftDismissElement)
    {
        IFC_RETURN(m_leftDismissElement.Cast<Path>()->put_Visibility(xaml::Visibility_Collapsed));
    }

    if (needsRight)
    {
        if (m_rightDismissElement)
        {
            IFC_RETURN(m_rightDismissElement.Cast<Path>()->put_Visibility(xaml::Visibility_Visible));
        }
        else
        {
            IFC_RETURN(CreatePolygonalPath(m_dismissHostElement.Get(), 4, &m_rightDismissElement));
        }

        wf::Point points[] = { topRightCorner,{ windowBounds.Width, topRightCorner.Y },{ windowBounds.Width, bottomRightCorner.Y }, bottomRightCorner };
        IFC_RETURN(UpdatePolygonalPath(m_rightDismissElement.Get(), ARRAYSIZE(points), points));
    }
    else if (m_rightDismissElement)
    {
        IFC_RETURN(m_rightDismissElement.Cast<Path>()->put_Visibility(xaml::Visibility_Collapsed));
    }

    return S_OK;
}

_Check_return_ HRESULT
SplitView::TeardownOuterDismissLayer()
{
    if (m_outerDismissLayerPopup)
    {
        IFC_RETURN(m_outerDismissLayerPopup->put_IsOpen(FALSE));
    }

    return S_OK;
}

_Check_return_ HRESULT SplitView::RegisterForDisplayModeStatesChangedEvent()
{
    if (!m_displayModeStateChangedEventHandler)
    {
        ctl::ComPtr<xaml::IVisualStateGroup> displayModeStates;
        IFC_RETURN(GetTemplatePart<xaml::IVisualStateGroup>(STR_LEN_PAIR(L"DisplayModeStates"), displayModeStates.ReleaseAndGetAddressOf()));
        if (displayModeStates)
        {
            IFC_RETURN(m_displayModeStateChangedEventHandler.AttachEventHandler(displayModeStates.Get(), std::bind(&SplitView::OnDisplayModeStateChanged, this, _1, _2)));
            SetPtrValue(m_tpDisplayModeStates, displayModeStates);
        }
    }

    return S_OK;
}

_Check_return_ HRESULT SplitView::OnPaneOpenedOrClosed(bool isPaneOpen)
{
    if (isPaneOpen)
    {
        // Raise the PaneOpened event.
        PaneOpenedEventSourceType* eventSource = nullptr;
        IFC_RETURN(GetPaneOpenedEventSourceNoRef(&eventSource));
        IFC_RETURN(eventSource->Raise(this, nullptr /*args*/));

        IFC_RETURN(DirectUI::ElementSoundPlayerService::RequestInteractionSoundForElementStatic(xaml::ElementSoundKind_Show, this));
    }
    else
    {
        // Call into the core object to raise the PaneClosed event, which is raised asynchronously.
        IFC_RETURN(static_cast<CSplitView*>(GetHandle())->OnPaneClosed());

        IFC_RETURN(DirectUI::ElementSoundPlayerService::RequestInteractionSoundForElementStatic(xaml::ElementSoundKind_Hide, this));
    }

    return S_OK;
}

_Check_return_ HRESULT
SplitView::CreatePolygonalPath(_In_ xaml_controls::IGrid* hostElement, size_t numPoints, _Outptr_ xaml_shapes::IPath** ppPath)
{
    ctl::ComPtr<Path> path;
    IFC_RETURN(ctl::make(&path));

    ctl::ComPtr<wfc::IVector<xaml::UIElement*>> children;
    IFC_RETURN(static_cast<Grid*>(hostElement)->get_Children(&children));
    IFC_RETURN(children->Append(path.Get()));

    // Set a transparent brush to make sure it's hit-testable.
    ctl::ComPtr<SolidColorBrush> transparentBrush;
    IFC_RETURN(ctl::make(&transparentBrush));
    IFC_RETURN(transparentBrush->put_Color({ 0, 0, 0, 0 }));
    IFC_RETURN(path->put_Fill(transparentBrush.Get()));

    // Create the path geometry into which we'll add our rectangle figures.
    ctl::ComPtr<PathGeometry> pathGeometry;
    IFC_RETURN(ctl::make(&pathGeometry));
    IFC_RETURN(path->put_Data(pathGeometry.Get()));

    ctl::ComPtr<wfc::IVector<xaml_media::PathFigure*>> figures;
    IFC_RETURN(pathGeometry->get_Figures(&figures));

    ctl::ComPtr<PathFigure> figure;
    IFC_RETURN(ctl::make(&figure));

    IFC_RETURN(figures->Append(figure.Get()));

    IFC_RETURN(figure->put_IsClosed(TRUE));

    ctl::ComPtr<wfc::IVector<xaml_media::PathSegment*>> segments;
    IFC_RETURN(figure->get_Segments(&segments));

    // The number of segments we have is equal to the number of points we have minus 1
    // because this is a closed figure.
    for (size_t i = 0; i < (numPoints - 1); ++i)
    {
        ctl::ComPtr<LineSegment> segment;
        IFC_RETURN(ctl::make(&segment));
        IFC_RETURN(segments->Append(segment.Get()));
    }

    auto uiElementCore = static_cast<CUIElement*>(path->GetHandle());
    uiElementCore->SetAllowsDragAndDropPassThrough(true);

    IFC_RETURN(path.CopyTo(ppPath));

    return S_OK;
}

_Check_return_ HRESULT
SplitView::UpdatePolygonalPath(
    _In_ xaml_shapes::IPath* path,
    size_t numPoints,
    _In_ wf::Point* points
    )
{
    ctl::ComPtr<xaml_media::IGeometry> geometry;
    IFC_RETURN(path->get_Data(&geometry));

    ctl::ComPtr<PathGeometry> pathGeometry;
    IFC_RETURN(geometry.As(&pathGeometry));

    ctl::ComPtr<wfc::IVector<xaml_media::PathFigure*>> figures;
    IFC_RETURN(pathGeometry->get_Figures(&figures));

    ctl::ComPtr<xaml_media::IPathFigure> figure;
    IFC_RETURN(figures->GetAt(0, &figure));
    ASSERT(figure);

    IFC_RETURN(figure->put_StartPoint(points[0]));

    ctl::ComPtr<wfc::IVector<xaml_media::PathSegment*>> segments;
    IFC_RETURN(figure->get_Segments(&segments));

    for (size_t i = 1; i < numPoints; ++i)
    {
        ctl::ComPtr<xaml_media::IPathSegment> segment;

        // Subtract 1 because the first point in our array is set as the figure's
        // start point.
        IFC_RETURN(segments->GetAt(static_cast<UINT>(i - 1), &segment));
        ASSERT(ctl::is<xaml_media::ILineSegment>(segment.Get()));

        IFC_RETURN(segment.AsOrNull<xaml_media::ILineSegment>()->put_Point(points[i]));
    }

    return S_OK;
}
