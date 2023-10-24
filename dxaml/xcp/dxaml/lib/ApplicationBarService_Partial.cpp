// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

//  Abstract:
//      Represents the ApplicationBar service. One instance per DXamlCore.
// ApplicationBarService is responsible for hosting the top and bottom appbars
// and toggling all appbars in response to system events (edgy) as well as
// making sure appbars are closed in response to lightdismiss.

#include "precomp.h"
#include "ApplicationBarService.g.h"
#include "AppBarLightDismiss.g.h"
#include "Grid.g.h"
#include "Border.g.h"
#include "Popup.g.h"
#include "EdgeUIThemeTransition.g.h"
#include "SolidColorBrush.g.h"
#include "Window.g.h"
#include "AppBar.g.h"
#include "CommandBar.g.h"
#include "FlyoutBase.g.h"
#include "TransitionCollection.g.h"
#include "Page.g.h"
#include "XamlRoot.g.h"
#include "DoubleAnimationUsingKeyFrames.g.h"
#include "DiscreteDoubleKeyFrame.g.h"
#include "SplineDoubleKeyFrame.g.h"
#include "KeySpline.g.h"
#include "Storyboard.g.h"
#include <LightDismissOverlayHelper.h>

#include "focusmgr.h"

using namespace DirectUI;
using namespace DirectUISynonyms;

ApplicationBarService::ApplicationBarService()
    : m_pRootVisualWeakReference(NULL)
    , m_pEdgeGesture(NULL)
    , m_pPreviousFocusedElementWeakRef(NULL)
    , m_isOverlayVisible(false)
{
    m_DismissPressedEventToken.value = 0;
    m_DismissPointerReleasedEventToken.value = 0;
    m_DismissLayerRightTapToken.value = 0;
    m_activationToken.value = 0;
    m_EdgeGestureCompletedEventToken.value = 0;
    m_shouldTopGetFocus = TRUE;
    m_suspendLightDismissLayerState = FALSE;
    m_focusReturnState = xaml::FocusState_Unfocused;
    m_appBarsLoading = 0;

    ZeroMemory(&m_bounds, sizeof(m_bounds));
}

ApplicationBarService::~ApplicationBarService()
{
    HRESULT hr = S_OK;

    ReleaseInterface(m_pPreviousFocusedElementWeakRef);
    IFC(CleanupOpenEventHooks());

    if (m_overlayClosingCompletedHandler)
    {
        ASSERT(m_overlayClosingStoryboard);
        VERIFYHR(DetachHandler(m_overlayClosingCompletedHandler, m_overlayClosingStoryboard));
    }

Cleanup:
    VERIFYHR(hr);
}

// sets up our appbar hosting solution using a grid (that is sized to the visible size of the window)
// and two borders. Those borders are the wrappers that host the appbars.
// We use hosts so that we can apply transitions without stomping over user values.
_Check_return_
HRESULT
ApplicationBarService::Initialize() noexcept
{
    HRESULT hr = S_OK;

    ctl::ComPtr<Popup> spPopup;
    ctl::ComPtr<Grid> spGrid;
    ctl::ComPtr<AppBarLightDismiss> spAccDismissLayer;
    ctl::ComPtr<Border> spTopHost;
    ctl::ComPtr<Border> spBottomHost;
    ctl::ComPtr<EdgeUIThemeTransition> spTopHostChildTransition;
    ctl::ComPtr<EdgeUIThemeTransition> spBottomHostChildTransition;
    ctl::ComPtr<EdgeUIThemeTransition> spBottomHostTransition;
    ctl::ComPtr<SolidColorBrush> spTransparentBrush;
    ctl::ComPtr<TransitionCollection> spTransitionCollection;
    ctl::ComPtr<ITransition> spTransition;

    wfc::IVector<xaml::UIElement*>* pChildren = NULL;
    wu::Color transparentColor;
    IPointerEventHandler* pDismissLayerPointerPressedHandler = NULL;
    IPointerEventHandler* pDismissLayerPointerReleasedHandler = NULL;
    wf::ITypedEventHandler<IInspectable*, xaml::WindowActivatedEventArgs*>* pWindowActivatedHandler = NULL;
    xaml_input::IRightTappedEventHandler* pDismissLayerRightTappedHandler = NULL;

    wf::Rect bounds = {};

    IFC(ctl::make<AppBarLightDismiss>(&spAccDismissLayer));
    IFC(ctl::make<Grid>(&spGrid));
    IFC(ctl::make<Popup>(&spPopup));
    IFC(ctl::make<Border>(&spTopHost));
    IFC(ctl::make<Border>(&spBottomHost));
    IFC(ctl::make<EdgeUIThemeTransition>(&spTopHostChildTransition));
    IFC(ctl::make<EdgeUIThemeTransition>(&spBottomHostChildTransition));
    IFC(ctl::make<EdgeUIThemeTransition>(&spBottomHostTransition));

    IFC(spPopup->put_IsApplicationBarService(TRUE));

    SetPtrValue(m_tpPopupHost, spPopup);
    SetPtrValue(m_tpDismissLayer, spGrid);
    SetPtrValue(m_tpAccDismissLayer, spAccDismissLayer);

    SetPtrValue(m_tpTopBarHost, spTopHost);
    SetPtrValue(m_tpBottomBarHost, spBottomHost);
    SetPtrValue(m_tpBottomHostTransition, spBottomHostTransition);
    m_tpBottomHostTransition->SetToOnlyReactToTickAndUseIHMTiming(-1); // by default this transition will not respond

    // our popup will host the grid
    IFC(m_tpPopupHost->put_Child(m_tpDismissLayer.Get()));
    IFC(m_tpDismissLayer->get_Children(&pChildren));

    // initialize the dismiss layer to valid bounds
    IFC(DXamlCore::GetCurrent()->GetVisibleContentBoundsForElement(GetHandle(), &bounds));
    IFC(m_tpDismissLayer->put_Width(bounds.Width));
    IFC(m_tpDismissLayer->put_Height(bounds.Height));

    // the grid acts as the light dismiss layer, so it needs atleast a transparent color
    // we will be toggling the brush in order to turn on or off the shield
    IFC(ctl::make<SolidColorBrush>(&spTransparentBrush));
    transparentColor.A = 0;
    transparentColor.B = 255;
    transparentColor.G = 255;
    transparentColor.R = 255;
    SetPtrValue(m_tpTransparentBrush, spTransparentBrush);
    IFC(m_tpTransparentBrush->put_Color(transparentColor));

    // hookup pointer presses to the dismiss layer. This way we can
    // act as a true dismiss layer
    pDismissLayerPointerPressedHandler = new ClassMemberEventHandler<
        ApplicationBarService,
        xaml::IDependencyObject,
        IPointerEventHandler,
        IInspectable,
        IPointerRoutedEventArgs>(this, &ApplicationBarService::OnDismissLayerPressed);

    IFC(m_tpDismissLayer->add_PointerPressed(pDismissLayerPointerPressedHandler, &m_DismissPressedEventToken));

    // Also hookup pointer up event so we can catch and handle it. Unless we do this, controls beneath the dismiss layer
    // will get unmatched events (they will receive pointer up but not down). We do not want this.
    pDismissLayerPointerReleasedHandler = new ClassMemberEventHandler<
        ApplicationBarService,
        xaml::IDependencyObject,
        IPointerEventHandler,
        IInspectable,
        IPointerRoutedEventArgs>(this, &ApplicationBarService::OnDismissLayerPointerReleased);

    IFC(m_tpDismissLayer->add_PointerReleased(pDismissLayerPointerReleasedHandler, &m_DismissPointerReleasedEventToken));

    // Hook up right tapped event to the dismiss layer so we can toggle the appbars instead of
    // just dismissing the non-sticky ones when it gets right tapped
    pDismissLayerRightTappedHandler = new ClassMemberEventHandler<
         ApplicationBarService,
         xaml::IDependencyObject,
         xaml_input::IRightTappedEventHandler,
         IInspectable,
         IRightTappedRoutedEventArgs>(this, &ApplicationBarService::OnDismissLayerRightTapped);

     IFC(m_tpDismissLayer->add_RightTapped(pDismissLayerRightTappedHandler, &m_DismissLayerRightTapToken));

    // our dismiss layer will host in total two appbar hosts.
    // in order to easily manage the appear and disappear transitions, we'll put a transition on the
    // appbars through the childtransitions of the hosts.
    //
    // For the movement of the bottom bar though, we cannot use a transition on the bottom bar directly, since
    // it is not moving relative to its parent. It is the border (host) that is moving, so it is the one that
    // needs the transition.
    IFC(spTopHostChildTransition->put_Edge(xaml_primitives::EdgeTransitionLocation_Top));
    IFC(spBottomHostChildTransition->put_Edge(xaml_primitives::EdgeTransitionLocation_Bottom));

    // setup alignments such that the bottombar is always at the bottom of the visible area
    IFC(m_tpTopBarHost->put_VerticalAlignment(xaml::VerticalAlignment::VerticalAlignment_Top));
    IFC(m_tpTopBarHost->put_HorizontalAlignment(xaml::HorizontalAlignment::HorizontalAlignment_Stretch));
    IFC(m_tpBottomBarHost->put_VerticalAlignment(xaml::VerticalAlignment::VerticalAlignment_Bottom));
    IFC(m_tpBottomBarHost->put_HorizontalAlignment(xaml::HorizontalAlignment::HorizontalAlignment_Stretch));
    IFC(m_tpAccDismissLayer->put_VerticalAlignment(xaml::VerticalAlignment::VerticalAlignment_Stretch));
    IFC(m_tpAccDismissLayer->put_HorizontalAlignment(xaml::HorizontalAlignment::HorizontalAlignment_Stretch));

    // m_tpAccDismissLayer needs to be the last child (UpdateDismissLayer() work on that assumption)
    IFC(pChildren->Append(m_tpTopBarHost.Get()));
    IFC(pChildren->Append(m_tpBottomBarHost.Get()));
    IFC(pChildren->Append(m_tpAccDismissLayer.Get()));

    // start without shield on
    IFC(UpdateDismissLayer());

    // start probably with the popup closed
    IFC(EvaluatePopupState());

    // Add this to the list of objects not in the peer table.
    //DXamlCore::GetCurrent()->AddToReferenceTrackingList(this);

    IFC(ApplicationBarServiceGenerated::Initialize());

Cleanup:
    ReleaseInterface(pWindowActivatedHandler);
    ReleaseInterface(pDismissLayerPointerPressedHandler);
    ReleaseInterface(pDismissLayerPointerReleasedHandler);
    ReleaseInterface(pDismissLayerRightTappedHandler);
    ReleaseInterface(pChildren);
    RRETURN(hr);
}

// registers an applicationbar so it can be toggled globally
_Check_return_
HRESULT
ApplicationBarService::RegisterApplicationBar(_In_ AppBar* pApplicationBar, _In_ DirectUI::AppBarMode mode)
{
    HRESULT hr = S_OK;
    IWeakReference* pWeakApplicationBar = NULL;
    BOOLEAN bIsOpen = FALSE;
    ctl::ComPtr<Page> spOwner;

    IFCPTR(pApplicationBar);
    IFC(ctl::as_weakref(pWeakApplicationBar, ctl::as_iinspectable(pApplicationBar)));
    IFCPTR(pWeakApplicationBar);

    // this doesn't catch multiple registrations. However, since
    // we are only registering when added to the live tree, this means we are never double registered.
    m_ApplicationBars.push_back(pWeakApplicationBar);
    AddRefInterface(pWeakApplicationBar);

    IFC(pApplicationBar->GetOwner(&spOwner));
    VisualTree* visualTree = VisualTree::GetForElementNoRef(spOwner->GetHandle());
    if (visualTree)
    {
        static_cast<CPopup*>(m_tpPopupHost.Cast<Popup>()->GetHandle())->SetAssociatedVisualTree(visualTree);
    }

    IFC(AddApplicationBarToVisualTree(pApplicationBar, mode));

    if (mode == AppBarMode_Top)
    {
        IFC(m_tpTopBarHost->ClearValueByKnownIndex(KnownPropertyIndex::Border_ChildTransitions));
    }
    else if (mode == AppBarMode_Bottom)
    {
        IFC(m_tpBottomBarHost->ClearValueByKnownIndex(KnownPropertyIndex::Border_ChildTransitions));
        IFC(m_tpBottomBarHost->ClearValueByKnownIndex(KnownPropertyIndex::UIElement_Transitions));
    }

    IFC(pApplicationBar->get_IsOpen(&bIsOpen));
    if (bIsOpen)
    {
        IFC(OpenApplicationBar(pApplicationBar, mode));
    }

    // use the application bar UIElement to determine which (instance of) Window.Activated events we should listen for.
    // Since there is always a unique instance of ApplicationBarService for each XamlRoot, we only need to do this once.
    Window* window = nullptr;
    IGNOREHR(DXamlCore::GetCurrent()->GetAssociatedWindowNoRef(pApplicationBar, &window));
    if (!m_windowActivatedHandler && window)
    {
        auto xamlRoot = XamlRoot::GetForElementStatic(pApplicationBar);
        if (xamlRoot)
        {
            IFC(ctl::AsWeak(xamlRoot.Get(), &m_weakXamlRoot));
        }

        IFC(m_windowActivatedHandler.AttachEventHandler(
            window,
            [weakXamlRoot = m_weakXamlRoot](IInspectable *sender, xaml::IWindowActivatedEventArgs *args) mutable
            {
                HRESULT hr = S_OK;
                ctl::ComPtr<IApplicationBarService> service;

                auto xamlRoot = weakXamlRoot.AsOrNull<xaml::IXamlRoot>();
                if (xamlRoot)
                {
                    IFC(xamlRoot.Cast<XamlRoot>()->TryGetApplicationBarService(service));
                    if (service)
                    {
                        IFC(service.Cast<ApplicationBarService>()->OnWindowActivated(sender, args));
                    }
                }

            Cleanup:
                RRETURN(hr);
            })
        );
    }

Cleanup:
    ReleaseInterface(pWeakApplicationBar);
    RRETURN(hr);
}

_Check_return_
HRESULT
ApplicationBarService::UnregisterApplicationBar(_In_ AppBar* pApplicationBar)
{
    HRESULT hr = S_OK;
    std::list<IWeakReference*>::iterator it;
    IAppBar* pApplicationBarInRegistration = NULL;
    ctl::ComPtr<IUIElement> spAppbarTopUIE = NULL;
    ctl::ComPtr<IUIElement> spAppbarBottomUIE = NULL;
    ctl::ComPtr<IAppBar> spAppbarTop = NULL;
    ctl::ComPtr<IAppBar> spAppbarBottom = NULL;

    IFC(m_tpTopBarHost->get_Child(spAppbarTopUIE.GetAddressOf()));
    IFC(m_tpBottomBarHost->get_Child(spAppbarBottomUIE.GetAddressOf()));
    spAppbarTop = spAppbarTopUIE ? spAppbarTopUIE.AsOrNull<IAppBar>() : NULL;
    spAppbarBottom = spAppbarBottomUIE ? spAppbarBottomUIE.AsOrNull<IAppBar>() : NULL;

    // close the appbar so that we run the correct logic around evaluating popup state
    if (spAppbarTop.Get() == pApplicationBar)
    {
        IFC(CloseApplicationBar(pApplicationBar, AppBarMode_Top));
        IFC(RemoveApplicationBarFromVisualTree(pApplicationBar, AppBarMode_Top));
    }
    if (spAppbarBottom.Get() == pApplicationBar)
    {
        IFC(CloseApplicationBar(pApplicationBar, AppBarMode_Bottom));
        IFC(RemoveApplicationBarFromVisualTree(pApplicationBar, AppBarMode_Bottom));
    }

    for (it = m_ApplicationBars.begin(); it != m_ApplicationBars.end();)
    {
        IWeakReference* pApplicationBarWeakRef = *it;
        IFC(ctl::resolve_weakref(pApplicationBarWeakRef, pApplicationBarInRegistration));
        if (pApplicationBarInRegistration != NULL && pApplicationBarInRegistration == pApplicationBar)
        {
            ReleaseInterface(pApplicationBarWeakRef);
            m_ApplicationBars.erase(it++);
            break;
        }
        else
        {
            ++it;
            // pApplicationBarInRegistration should never be NULL in the apps as we know.
            ASSERT(pApplicationBarInRegistration != NULL);
        }
        ReleaseInterface(pApplicationBarInRegistration);
    }

    // Stop listening for Window.Activated events if there are no longer any app bars registered.
    if (!m_ApplicationBars.size() && m_windowActivatedHandler)
    {
        Window* window = nullptr;
        IGNOREHR(DXamlCore::GetCurrent()->GetAssociatedWindowNoRef(pApplicationBar, &window));

        if (window)
        {
            IGNOREHR(m_windowActivatedHandler.DetachEventHandler(ctl::iinspectable_cast(window)));
        }
    }

Cleanup:
    ReleaseInterface(pApplicationBarInRegistration);
    RRETURN(hr);
}

_Check_return_
HRESULT
ApplicationBarService::ClearCaches()
{
    HRESULT hr = S_OK;
    std::list<IWeakReference*>::iterator iterator;

    for (iterator = m_ApplicationBars.begin();
        iterator != m_ApplicationBars.end();
        iterator++)
    {
        IWeakReference* pApplicationBarWeakRef = *iterator;
        ReleaseInterface(pApplicationBarWeakRef);
    }
    m_ApplicationBars.clear();

    if (m_DismissPressedEventToken.value != 0)
    {
        // we own the grid that we hook into ourselves
        IFC(m_tpDismissLayer->remove_PointerPressed(m_DismissPressedEventToken));
        m_DismissPressedEventToken.value = 0;
    }

    if (m_DismissPointerReleasedEventToken.value != 0)
    {
         // we own the grid that we hook into ourselves
        IFC(m_tpDismissLayer->remove_PointerReleased(m_DismissPointerReleasedEventToken));
        m_DismissPointerReleasedEventToken.value = 0;
    }

    if (m_DismissLayerRightTapToken.value != 0)
    {
        // we own the grid that we hook into ourselves
        IFC(m_tpDismissLayer->remove_RightTapped(m_DismissLayerRightTapToken));
        m_DismissLayerRightTapToken.value = 0;
    }
    
    CleanupWindowActivatedEventHook();
    m_weakXamlRoot.Reset();

    // Clear the popup host caches that is on the current window.
    // DxamlCore::RunMessageLoop() calls the ClearCaches() and set the current window as null(m_pWindow),
    // so the current window's caches must be cleared now.
    if (m_tpPopupHost)
    {
        m_tpPopupHost->ClearWindowCaches();
    }

Cleanup:
    ReleaseInterface(m_pRootVisualWeakReference);
    RRETURN(hr);
}

_Check_return_
HRESULT
ApplicationBarService::GetTopAndBottomAppBars(
    _Outptr_ AppBar** ppTopAppBar,
    _Outptr_ AppBar** ppBottomAppBar)
{
    HRESULT hr = S_OK;

    IFC(GetTopAndBottomAppBars(false /* openAppBarsOnly */, ppTopAppBar, ppBottomAppBar, nullptr));

Cleanup:
    RRETURN(hr);
}

// Gets Top And Bottom App Bars Only returns the ones which are open.
_Check_return_
HRESULT
ApplicationBarService::GetTopAndBottomOpenAppBars(
    _Outptr_ AppBar** ppTopAppBar,
    _Outptr_ AppBar** ppBottomAppBar,
    _Out_ BOOLEAN* pIsAnyLightDismiss)
{
    HRESULT hr = S_OK;

    IFC(GetTopAndBottomAppBars(true /* openAppBarsOnly */, ppTopAppBar, ppBottomAppBar, pIsAnyLightDismiss));

Cleanup:
    RRETURN(hr);
}

_Check_return_
HRESULT
ApplicationBarService::GetTopAndBottomAppBars(
    _In_ bool openAppBarsOnly,
    _Outptr_ AppBar** ppTopAppBar,
    _Outptr_ AppBar** ppBottomAppBar,
    _Out_opt_ BOOLEAN* pIsAnyLightDismiss)
{
    HRESULT hr = S_OK;
    ctl::ComPtr<IUIElement> spAppbarTopUIE;
    ctl::ComPtr<IUIElement> spAppbarBottomUIE;
    ctl::ComPtr<IAppBar> spAppbarTop;
    ctl::ComPtr<IAppBar> spAppbarBottom;
    BOOLEAN isTopOpen = FALSE;
    BOOLEAN isBottomOpen = FALSE;
    BOOLEAN isSticky = FALSE;
    BOOLEAN retIsLightDismiss = FALSE;
    ctl::ComPtr<AppBar> spAppBar;
    ctl::ComPtr<AppBar> spAppBarTop;
    ctl::ComPtr<AppBar> spAppBarBottom;

    *ppTopAppBar = nullptr;
    *ppBottomAppBar = nullptr;
    if (m_tpTopBarHost)
    {
        IFC(m_tpTopBarHost->get_Child(spAppbarTopUIE.GetAddressOf()));
    }
    if (m_tpBottomBarHost)
    {
        IFC(m_tpBottomBarHost->get_Child(spAppbarBottomUIE.GetAddressOf()));
    }
    spAppbarTop = spAppbarTopUIE ? spAppbarTopUIE.AsOrNull<IAppBar>() : nullptr;
    spAppbarBottom = spAppbarBottomUIE ? spAppbarBottomUIE.AsOrNull<IAppBar>() : nullptr;

    spAppBar = spAppbarTop.Cast<AppBar>();
    if (spAppBar)
    {
        if (openAppBarsOnly)
        {
            IFC(spAppBar->get_IsOpen(&isTopOpen));
        }

        if (!openAppBarsOnly || isTopOpen)
        {
            IFC(spAppBar->get_IsSticky(&isSticky));
            retIsLightDismiss = retIsLightDismiss || (!isSticky);
            spAppBarTop = spAppBar;
        }
    }
    spAppBar = spAppbarBottom.Cast<AppBar>();
    if (spAppBar)
    {
        if (openAppBarsOnly)
        {
            IFC(spAppBar->get_IsOpen(&isBottomOpen));
        }

        if (!openAppBarsOnly || isBottomOpen)
        {
            IFC(spAppBar->get_IsSticky(&isSticky));
            retIsLightDismiss = retIsLightDismiss || (!isSticky);
            spAppBarBottom = spAppBar;
        }
    }
    *ppTopAppBar = spAppBarTop.Detach();
    *ppBottomAppBar = spAppBarBottom.Detach();


    if (pIsAnyLightDismiss != nullptr)
    {
        *pIsAnyLightDismiss = retIsLightDismiss;
    }

Cleanup:
    RRETURN(hr);
}

// Focus the applicationbar
_Check_return_
HRESULT
ApplicationBarService::FocusApplicationBar(
    _In_ AppBar* pAppBar,
    _In_ xaml::FocusState focusState)
{
    HRESULT hr = S_OK;
    CDependencyObject* pNewTabStopCDO = NULL;

    if (m_appBarsLoading > 0)
    {
        // While an AppBar is loading another appbar can close but we do not want the number
        // to go negative.
        m_appBarsLoading--;
    }

    if (m_appBarsLoading == 0)
    {
        ctl::ComPtr<AppBar> spTopAppBar;
        ctl::ComPtr<AppBar> spBottomAppBar;
        ctl::ComPtr<DependencyObject> spNewTabStopDO;
        BOOLEAN focusUpdated = FALSE;
        BOOLEAN isAnyLightDismiss = FALSE;

        IFC(GetTopAndBottomOpenAppBars(&spTopAppBar, &spBottomAppBar, &isAnyLightDismiss));

        // Get the first focusable elements from app bars
        IFC(GetFirstFocusableElementFromAppBars(spTopAppBar.Get(), spBottomAppBar.Get(), m_shouldTopGetFocus ? AppBarTabPriority_Top : AppBarTabPriority_Bottom, FALSE /* Is backward */, &pNewTabStopCDO));

        if (pNewTabStopCDO) {
            IFC(DXamlCore::GetCurrent()->GetPeer(pNewTabStopCDO, &spNewTabStopDO));
            if (spNewTabStopDO)
            {
                IFC(DependencyObject::SetFocusedElement(spNewTabStopDO.Get(), focusState, FALSE /*animateIfBringIntoView*/, &focusUpdated));
            }
        }

        if (!focusUpdated)
        {
            IFC(pAppBar->Focus(focusState, &focusUpdated));
        }

        // If the focus was still not updated and if there is a light dismiss appbar in the play, then clear the focus,
        // if there is no sticky appbars, do nothing.
        if (!focusUpdated && isAnyLightDismiss)
        {
            CFocusManager* focusManager = VisualTree::GetFocusManagerForElement(pAppBar->GetHandle());
            focusManager->ClearFocus();
        }
    }

Cleanup:
    ReleaseInterface(pNewTabStopCDO);
    RRETURN(hr);
}

// Invoked on wui::IEdgeGesture.Completed event (aka App Edgy event)
_Check_return_
HRESULT
ApplicationBarService::OnEdgeGestureCompleted(
    _In_ wui::IEdgeGesture* pSender,
    _In_ wui::IEdgeGestureEventArgs* pArgs)
{
    HRESULT hr = S_OK;
    wui::EdgeGestureKind edgeGestureKind = wui::EdgeGestureKind_Touch;

    IFC(pArgs->get_Kind(&edgeGestureKind));
    // Depending on the manner of the edge gesture invocation determine the focus state for focus return
    switch (edgeGestureKind)
    {
        case wui::EdgeGestureKind_Touch:
        case wui::EdgeGestureKind_Mouse:
            {
                SetFocusReturnState(xaml::FocusState_Pointer);
                break;
            }
        case wui::EdgeGestureKind_Keyboard:
            {
                SetFocusReturnState(xaml::FocusState_Keyboard);
                break;
            }
    }
    IFC(ToggleApplicationBars());
    ResetFocusReturnState();

Cleanup:
    RRETURN(hr);
}


// Toggles Application Bar
// This method is invoked when the user toggles (ON/OFF) the AppBar (via touch, keyboard or mouse)
_Check_return_
HRESULT
ApplicationBarService::ToggleApplicationBars()
{
    XINT32 visibleCount = 0;
    XINT32 hiddenCount = 0;
    BOOLEAN valueToSet = TRUE;
    BOOLEAN bottomAppBarGetsOpened = FALSE;
    BOOLEAN topAppBarGetsOpened = FALSE;
    std::list<ctl::ComPtr<IAppBar>> applicationBars;

    m_shouldTopGetFocus = TRUE;
    for (auto pApplicationBarWeakRef : m_ApplicationBars)
    {
        ctl::ComPtr<IAppBar> applicationBar;
        IFC_RETURN(ctl::resolve_weakref(pApplicationBarWeakRef, applicationBar.GetAddressOf()));

        ASSERT(applicationBar);

        AppBar* const pAppBar = static_cast<AppBar *>(applicationBar.Get());
        BOOLEAN bIsOpen = FALSE;

        // We don't want to toggle CommandBars
        if (ctl::is<ICommandBar>(pAppBar))
        {
            continue;
        }

        IFC_RETURN(applicationBar->get_IsOpen(&bIsOpen));

        if (bIsOpen)
        {
            ++visibleCount;
        }
        else
        {
            ++hiddenCount;
        }
        if (pAppBar->GetMode() == AppBarMode_Bottom && bIsOpen == FALSE)
        {
            // Bottom app bar is hidden and will get opened
            bottomAppBarGetsOpened = TRUE;
        }
        else if (pAppBar->GetMode() == AppBarMode_Top && bIsOpen == FALSE)
        {
            // Top app bar is hidden and will get opened
            topAppBarGetsOpened = TRUE;
        }
        applicationBars.push_back(std::move(applicationBar));
    }

    // Show all the app bars, unless they are all currently shown in
    // which case they should be hidden. If some are shown and some
    // hidden, show them all.
    if (hiddenCount == 0 && visibleCount > 0)
    {
        valueToSet = FALSE;
    }
    if (bottomAppBarGetsOpened && topAppBarGetsOpened)
    {
        ASSERT(valueToSet);
        // Top appbar and bottom appbar is getting opened at the same time, this is the one and
        // only case where top appbar should not try to get the focus when it gets opened
        m_shouldTopGetFocus = FALSE;
    }

    for (const auto& bar : applicationBars)
    {
        if (bar)
        {
            IFC_RETURN(bar->put_IsOpen(valueToSet));
        }
    }

    return S_OK;
}

_Check_return_
HRESULT
ApplicationBarService::EvaluatePopupState()
{
    HRESULT hr = S_OK;
    ctl::ComPtr<IUIElement> spAppbarTopUIE = NULL;
    ctl::ComPtr<IUIElement> spAppbarBottomUIE = NULL;
    BOOLEAN bIsOpen = FALSE;
    BOOLEAN bHasOpenAppBars = FALSE;

    if (m_suspendLightDismissLayerState == FALSE)
    {
        IFC(m_tpTopBarHost->get_Child(spAppbarTopUIE.GetAddressOf()));
        IFC(m_tpBottomBarHost->get_Child(spAppbarBottomUIE.GetAddressOf()));

        IFC(m_tpPopupHost->get_IsOpen(&bIsOpen));
        bHasOpenAppBars = spAppbarTopUIE.Get() != NULL || spAppbarBottomUIE.Get() != NULL;

        if (m_unloadingAppbars.size() == 0 && !bHasOpenAppBars)
        {
            if (bIsOpen)
            {
                IFC(m_tpPopupHost->put_IsOpen(FALSE));
            }
        }
        else
        {
            if (!bIsOpen)
            {
                BOOLEAN bIgnored = FALSE;
                IFC(TryGetBounds(&bIgnored));

                IFC(m_tpPopupHost->put_IsOpen(TRUE));

                // when setting to true, our bar hosts will get a potential load trigger
                // this would be completely correct, since those elements are being entered.
                // however, we need the bottom barhost not to think it is in a load situation
                IFC(CoreImports::UIElement_SetIsEntering(static_cast<CUIElement*>(m_tpBottomBarHost->GetHandle()), FALSE));
            }
        }
    }

Cleanup:
    RRETURN(hr);
}

_Check_return_
HRESULT
ApplicationBarService::TryGetBounds(_Out_ BOOLEAN* boundsChanged)
{
    *boundsChanged = FALSE;

    auto xamlRoot = m_weakXamlRoot.AsOrNull<xaml::IXamlRoot>();
    if (xamlRoot && m_tpDismissLayer)
    {
        wf::Rect bounds{};
        IFC_RETURN(DXamlCore::GetCurrent()->GetVisibleContentBoundsForElement(m_tpPopupHost->GetHandle(), &bounds));

        // compare to the old bounds, in order to detect an orientation or mode change
        // specs state we should only move on orientation, however WWA has interpreted
        // this as moving on mode as well (portrait, landscape etc).
        // We actually don't need to hook into the orientation change event anymore, if
        // we just compare the dims of the visible area.
        if (bounds.Width != m_bounds.Width || bounds.Height != m_bounds.Height)
        {
            *boundsChanged = TRUE;

            // this will potentially move the lower appbar, since it is
            // vertical aligned to the bottom
            IFC_RETURN(m_tpDismissLayer->put_Width(bounds.Width));
            IFC_RETURN(m_tpDismissLayer->put_Height(bounds.Height));
        }

        // This is broken out separately to avoid setting a margin property of 0,0,0,0
        // if there is never any need.  I would expect desktop to not need a margin since
        // it has no OS chrome always overlapping the window.  On phone the margin will
        // only change due to orientation changes.
        if (bounds.X != m_bounds.X || bounds.Y != m_bounds.Y)
        {
            *boundsChanged = TRUE;

            // The visible bounds may be below chrome rendered at the top (or left) of the
            // screen so use margin to position the container of the page appbars.
            xaml::Thickness margin = {bounds.X, bounds.Y, 0, 0};
            IFC_RETURN(m_tpDismissLayer->put_Margin(margin));
        }

        m_bounds = bounds;
    }

    return S_OK;
}

_Check_return_ HRESULT
ApplicationBarService::ShouldDismissNonStickyAppBars(_Out_ BOOLEAN* shouldDismiss)
{
    // we should not dismiss app bars the first time we determine our bounds
    BOOLEAN hasBounds = !(m_bounds.X == 0 && m_bounds.Y == 0 && m_bounds.Width == 0 && m_bounds.Height == 0);
    BOOLEAN boundsChanged = FALSE;
    IFC_RETURN(TryGetBounds(&boundsChanged));

    *shouldDismiss = hasBounds && boundsChanged;
    return S_OK;
}

// either the window was resized or the IHM changed
// will react by repositioning the appbar hosts.
_Check_return_
HRESULT
ApplicationBarService::OnBoundsChanged(_In_ BOOLEAN inputPaneChange)
{
    HRESULT hr = S_OK;
    INT16 tick = 0;

    BOOLEAN shouldDismiss = FALSE;
    IFC(ShouldDismissNonStickyAppBars(&shouldDismiss));

    if (shouldDismiss && !inputPaneChange)
    {
       IFC(CloseAllNonStickyAppBars());
    }

    // bounds can change for many reasons. Only when the IHM has changed do we wish to
    // allow the transition to occur.
    if (inputPaneChange && m_tpBottomHostTransition)
    {
        IFC(CoreImports::LayoutManager_GetLayoutTickForTransition(static_cast<CUIElement*>(static_cast<Border*>(m_tpBottomBarHost.Get())->GetHandle()), (XINT16*)&tick));
        m_tpBottomHostTransition->SetToOnlyReactToTickAndUseIHMTiming(tick);
    }

Cleanup:
    RRETURN(hr);
}

_Check_return_
HRESULT ApplicationBarService::IsAppBarRegistered(_In_ AppBar* pAppBar, _Out_ BOOLEAN* pIsRegistered)
{
    HRESULT hr = S_OK;

    BOOLEAN applicationBarIsRegistered = FALSE;
    AppBar* pRegisteredAppBar = NULL;
    IAppBar* pIAppBar = NULL;

    if (pAppBar)
    {
        std::list<IWeakReference*>::iterator it;

        // this list is truly very small, so perf is not an issue here
        for (it = m_ApplicationBars.begin(); it != m_ApplicationBars.end(); )
        {
            // Resolve the weak reference before comparing with the appbar in question. Depending on the implementation of the CLR
            // same object may have different weakrefs so it is not healthy to compare weakrefs to determine whether the appbar in question
            // is registered or not.
            IFC(ctl::resolve_weakref(*it, pIAppBar));
            ASSERT(pIAppBar != NULL);
            pRegisteredAppBar = static_cast<AppBar*>(pIAppBar);
            if (pRegisteredAppBar == pAppBar)
            {
                applicationBarIsRegistered = TRUE;
                break;
            }
            ReleaseInterface(pIAppBar);
            ++it;
        }
    }

    *pIsRegistered = applicationBarIsRegistered;

Cleanup:
    ReleaseInterface(pIAppBar);
    RRETURN(hr);
}

// Check if the focused element is on the top/bottom appbars, if it is not
// save current focused element's weak reference so we can return the focus to
// it later.

_Check_return_
HRESULT
ApplicationBarService::SaveCurrentFocusedElement(_In_ AppBar* pAppBar)
{
    ctl::ComPtr<DependencyObject> focusedElement;
    IFC_RETURN(pAppBar->GetFocusedElement(&focusedElement));

    if (focusedElement)
    {
        ReleaseInterface(m_pPreviousFocusedElementWeakRef);
        IFC_RETURN(ctl::as_weakref(m_pPreviousFocusedElementWeakRef, ctl::as_iinspectable(focusedElement.Get())));
    }

    return S_OK;
}

// Try to focus the previously saved element, if not successful try to focus the
// first focusable element in the main content.
_Check_return_
HRESULT
ApplicationBarService::FocusSavedElement(_In_ AppBar* pApplicationBar)
{
    HRESULT hr = S_OK;
    IDependencyObject* pElementToBeFocusedIDO = NULL;
    DependencyObject* pElementToBeFocusedDO = NULL;
    BOOLEAN focusUpdated = FALSE;
    xaml::FocusState focusState = xaml::FocusState_Unfocused;

    CFocusManager* focusManager = VisualTree::GetFocusManagerForElement(pApplicationBar->GetHandle());

    // Determine the focus state. Note that if the saved focus state is Unfocused, it means appbars are getting closed
    // programmatically.
    focusState = (m_focusReturnState == xaml::FocusState_Unfocused ?
                  xaml::FocusState_Programmatic : m_focusReturnState);

    // Try to focus the saved element
    if (m_pPreviousFocusedElementWeakRef)
    {
        IFC(ctl::resolve_weakref(m_pPreviousFocusedElementWeakRef, pElementToBeFocusedIDO));
        if (pElementToBeFocusedIDO)
        {
            IFC(DependencyObject::SetFocusedElement(
                static_cast<DependencyObject*>(pElementToBeFocusedIDO),
                focusState,
                FALSE /*animateIfBringIntoView*/,
                &focusUpdated));
        }
    }

    // If the focus is still not updated, clear the focus
    if (!focusUpdated)
    {
        focusManager->ClearFocus();
    }

Cleanup:
    ctl::release_interface(pElementToBeFocusedDO);
    ReleaseInterface(pElementToBeFocusedIDO);
    RRETURN(hr);
}

_Check_return_
HRESULT
ApplicationBarService::OpenApplicationBar(_In_ AppBar* pAppBar, _In_ DirectUI::AppBarMode mode)
{
    HRESULT hr = S_OK;
    BOOLEAN applicationBarIsRegistered = FALSE;

    IFC(IsAppBarRegistered(pAppBar, &applicationBarIsRegistered));

    IFC(ReevaluateIsOverlayVisible());

    if (m_isOverlayVisible)
    {
        IFC(PlayOverlayOpeningAnimation());
    }

    if (applicationBarIsRegistered)
    {
        // As per requirements, opening an AppBar should close any open FlyoutBase.
        IFC(FlyoutBase::CloseOpenFlyout());
    }

Cleanup:
    RRETURN(hr);
}

_Check_return_
HRESULT
ApplicationBarService::CloseApplicationBar(_In_ AppBar* pAppBar, _In_ DirectUI::AppBarMode mode)
{
    HRESULT hr = S_OK;
    BOOLEAN applicationBarIsRegistered = FALSE;

    IFC(IsAppBarRegistered(pAppBar, &applicationBarIsRegistered));

    if (m_isOverlayVisible)
    {
        IFC(PlayOverlayClosingAnimation());
    }

    if (applicationBarIsRegistered)
    {
        // Close any open FlyoutBase when AppBar is closing.
        // This is applicable when AppBar is closed by Win+Z or by right-click.  Other Flyouts will be
        // closed by light-dismiss prior to closing AppBar.
        IFC(FlyoutBase::CloseOpenFlyout());
    }

Cleanup:
    RRETURN(hr);
}

_Check_return_ HRESULT
ApplicationBarService::HandleApplicationBarClosedDisplayModeChange(_In_ AppBar* pAppBar, _In_ DirectUI::AppBarMode mode)
{
    HRESULT hr = S_OK;
    BOOLEAN applicationBarIsRegistered = FALSE;
    ctl::ComPtr<Border> spHost;
    ctl::ComPtr<xaml::IUIElement> spHostChild;

    IFC(IsAppBarRegistered(pAppBar, &applicationBarIsRegistered));

    if (applicationBarIsRegistered)
    {
        // The two steps to handle here are first to check if this AppBar is currently in the visual tree,
        // and then to check if this AppBar *should* currently be in the visual tree.
        // If it is and shouldn't be, we'll remove it, whereas if it isn't and should be, we'll add it.
        if (mode == AppBarMode_Top)
        {
            spHost = m_tpTopBarHost.Get();
        }
        else if (mode == AppBarMode_Bottom)
        {
            spHost = m_tpBottomBarHost.Get();
        }

        if (spHost)
        {
            bool appBarIsInVisualTree = false;

            BOOLEAN appBarIsOpen = FALSE;

            IFC(spHost->get_Child(&spHostChild));
            IFC(pAppBar->get_IsOpen(&appBarIsOpen));

            appBarIsInVisualTree = spHostChild && spHostChild.Get() == pAppBar;

            if (!appBarIsInVisualTree)
            {
                IFC(AddApplicationBarToVisualTree(pAppBar, mode));
            }

            // Regardless of whether or not we made any changes, we should modify the AppBar
            // to either add or remove loaded/unloaded transitions, depending on whether
            // the app bar is visible when closed.
            if (mode == AppBarMode_Top)
            {
                IFC(m_tpTopBarHost->ClearValueByKnownIndex(KnownPropertyIndex::Border_ChildTransitions));
            }
            else if (mode == AppBarMode_Bottom)
            {
                IFC(m_tpBottomBarHost->ClearValueByKnownIndex(KnownPropertyIndex::Border_ChildTransitions));
                IFC(m_tpBottomBarHost->ClearValueByKnownIndex(KnownPropertyIndex::UIElement_Transitions));
            }
        }
    }

Cleanup:
    RRETURN(hr);
}

_Check_return_ HRESULT
ApplicationBarService::AddApplicationBarToVisualTree(_In_ AppBar* pAppBar, _In_ DirectUI::AppBarMode mode)
{
    HRESULT hr = S_OK;
    ctl::ComPtr<Border> spHost = NULL;
    BOOLEAN applicationBarIsRegistered = FALSE;

    m_appBarsLoading++;

    IFC(IsAppBarRegistered(pAppBar, &applicationBarIsRegistered));

    // AddApplicationBarToVisualTree can be called when the appbar hasn't been registered yet.
    // It is wrong to open the appbar at that point because the mode is not yet guaranteed to be correct.
    // Nor should we be opening an appbar that isn't registered or live yet.
    // When the appbar does get registered later on, we will open at that time by looking
    // at the IsOpen property
    if (applicationBarIsRegistered)
    {
        if (mode == AppBarMode_Top)
        {
            // top
            spHost = m_tpTopBarHost.Get();
        }
        else if (mode == AppBarMode_Bottom)
        {
            // bottom
            spHost = m_tpBottomBarHost.Get();
        }

        if (spHost)
        {
            BOOLEAN bIgnored = FALSE;
            IFC(TryGetBounds(&bIgnored));

            // Set app bar focus state to be assumed when loaded. It will reset itself at the
            // appropriate time.
            pAppBar->SetOnLoadFocusState(m_focusReturnState);

            // clear the height, which was locked after closing appbar
            IFC(spHost->ClearValue(
                MetadataAPI::GetDependencyPropertyByIndex(KnownPropertyIndex::FrameworkElement_Height)));

            // Clear properties set by previous AppBar. Can't clear when the AppBar closes
            // because its unloading transition needs these properties.
            IFC(ClearAppBarOwnerPropertiesOnHost(spHost.Get()));

            // set the content, which will trigger a load transition
            IFC(spHost->put_Child(pAppBar));
            IFC(SetAppBarOwnerPropertiesOnHost(spHost.Get()));

            IFC(UpdateDismissLayer());
            IFC(EvaluatePopupState());
        }
    }

Cleanup:
    RRETURN(hr);
}

_Check_return_ HRESULT
ApplicationBarService::RemoveApplicationBarFromVisualTree(_In_ AppBar* pAppBar, _In_ DirectUI::AppBarMode mode)
{
    HRESULT hr = S_OK;
    ctl::ComPtr<IUIElement> spAppbarIUE = NULL;
    xaml::IRoutedEventHandler* pUnloadedEventHandler = NULL;
    ctl::ComPtr<Border> spHost = NULL;
    EventRegistrationToken ignoredToken;
    BOOLEAN applicationBarIsRegistered = FALSE;

    if (m_appBarsLoading > 0)
    {
        m_appBarsLoading--;
    }

    IFC(IsAppBarRegistered(pAppBar, &applicationBarIsRegistered));

    if (applicationBarIsRegistered)
    {
        if (mode == AppBarMode_Top)
        {
            IFC(m_tpTopBarHost->get_Child(spAppbarIUE.GetAddressOf()));
            spHost = m_tpTopBarHost.Get();
        }
        else if (mode == AppBarMode_Bottom)
        {
            IFC(m_tpBottomBarHost->get_Child(spAppbarIUE.GetAddressOf()));
            spHost = m_tpBottomBarHost.Get();
        }

        // if there was an appbar that we just closed, we'll need to listen to its unloading
        // transition. Otherwise we may end up closing the popup too soon.
        if (spAppbarIUE.Get() != NULL)
        {
            DOUBLE height = 0;
            ctl::ComPtr<IFrameworkElement> spAppbarIFE = NULL;
            spAppbarIFE = spAppbarIUE.AsOrNull<IFrameworkElement>();

            if (spAppbarIFE)
            {
                pUnloadedEventHandler = new ClassMemberEventHandler<
                    ApplicationBarService,
                    xaml::IDependencyObject,
                    xaml::IRoutedEventHandler,
                    IInspectable,
                    IRoutedEventArgs>(this, &ApplicationBarService::OnAppBarUnloaded);

                IFC(spAppbarIFE->add_Unloaded(pUnloadedEventHandler, &ignoredToken));

                // register that we are listening
                m_unloadingAppbars[spAppbarIFE.Get()] = ignoredToken;

                // make sure the host remains the same size after we removed the content
                // otherwise it would move to the bottom of the page itself (given its alignment)
                // and we would be unable to see the unload transition finish
                IFC(spHost->get_ActualHeight(&height));
                // set it to lock that in
                IFC(spHost->put_Height(height));
            }

            IFC(spHost->put_Child(NULL));
        }

        IFC(UpdateDismissLayer());
        IFC(EvaluatePopupState());
    }

Cleanup:
    ReleaseInterface(pUnloadedEventHandler);
    RRETURN(hr);
}

_Check_return_ HRESULT
ApplicationBarService::UpdateDismissLayer()
{
    ctl::ComPtr<IUIElement> spAppbarIUE = NULL;
    ctl::ComPtr<IAppBar> spAppbar = NULL;
    BOOLEAN bActivated = FALSE;
    BOOLEAN bIsOpen = FALSE;
    BOOLEAN bIsSticky = FALSE;
    ctl::ComPtr<wfc::IVector<xaml::UIElement*>> spChildren;
    UINT nChildren;
    ctl::ComPtr<xaml::IUIElement> spLastChild;

    if (m_suspendLightDismissLayerState == FALSE)
    {
        IFC_RETURN(m_tpTopBarHost->get_Child(spAppbarIUE.ReleaseAndGetAddressOf()));
        spAppbar = spAppbarIUE ? spAppbarIUE.AsOrNull<IAppBar>() : NULL;
        if (spAppbar)
        {
            IFC_RETURN(spAppbar->get_IsOpen(&bIsOpen));
            IFC_RETURN(spAppbar->get_IsSticky(&bIsSticky));
        }
        if (bIsOpen && !bIsSticky)
        {
            bActivated = TRUE;
        }
        else
        {
            IFC_RETURN(m_tpBottomBarHost->get_Child(spAppbarIUE.ReleaseAndGetAddressOf()));
            spAppbar = spAppbarIUE ? spAppbarIUE.AsOrNull<IAppBar>() : NULL;
            if (spAppbar)
            {
                IFC_RETURN(spAppbar->get_IsOpen(&bIsOpen));
                IFC_RETURN(spAppbar->get_IsSticky(&bIsSticky));
            }
            if (bIsOpen && !bIsSticky)
            {
                bActivated = TRUE;
            }
        }


        // the dismisslayer is actually the grid that is the first child of m_tpPopupHost
        // it has a transparent background (not NULL) and thus will be able to catch input.
        // by setting a transparent background or clearing the value, we control whether
        // it will react to the pointerevents.
        // if the dismisslayer is not activated we want to remove its AP from the UIA tree
        IFC_RETURN(m_tpDismissLayer->get_Children(&spChildren));
        IFC_RETURN(spChildren.Get()->get_Size(&nChildren));
        bool accDismissLayerIsInChildren = false;
        if (nChildren > 0)
        {
            // If the accDismissLayer is in the tree, it will be the last child of m_tpDismissLayer.
            IFC_RETURN(spChildren.Get()->GetAt(nChildren - 1, &spLastChild));
            accDismissLayerIsInChildren = (m_tpAccDismissLayer.Get() == spLastChild.Get());
        }

        if (bActivated)
        {
            IFC_RETURN(m_tpDismissLayer->put_Background(m_tpTransparentBrush.Get()));
            if (!accDismissLayerIsInChildren)
            {
                IFC_RETURN(spChildren.Get()->Append(m_tpAccDismissLayer.Get()));
            }
        }
        else
        {
            IFC_RETURN(m_tpDismissLayer->ClearValue(
                MetadataAPI::GetDependencyPropertyByIndex(KnownPropertyIndex::Panel_Background)));

            if (accDismissLayerIsInChildren)
            {
                IFC_RETURN(spChildren.Get()->RemoveAtEnd());
            }
        }
    }

    RRETURN(S_OK);
}


// called when we lose window activation or when we are dismissing
_Check_return_ HRESULT
ApplicationBarService::CloseAllNonStickyAppBars()
{
    bool isAnyAppBarClosed = false;
    IFC_RETURN(CloseAllNonStickyAppBars(&isAnyAppBarClosed));

    return S_OK;
}

_Check_return_ HRESULT
ApplicationBarService::CloseAllNonStickyAppBars(_Out_ bool* isAnyAppBarClosed)
{
    HRESULT hr = S_OK;
    *isAnyAppBarClosed = false;

    IAppBar* pApplicationBar = NULL;
    IWeakReference* pApplicationBarWeakRef = NULL;

    std::list<IWeakReference*>::iterator iterator;
    for (iterator = m_ApplicationBars.begin();
         iterator != m_ApplicationBars.end();
         ++iterator)
    {
        pApplicationBarWeakRef = *iterator;
        IFC(ctl::resolve_weakref(pApplicationBarWeakRef, pApplicationBar));
        if (pApplicationBar != NULL)
        {
            BOOLEAN bIsOpen = FALSE;
            BOOLEAN bIsSticky = FALSE;

            IFC(pApplicationBar->get_IsOpen(&bIsOpen));
            IFC(pApplicationBar->get_IsSticky(&bIsSticky));

            if (bIsOpen && !bIsSticky)
            {
                IFC(pApplicationBar->put_IsOpen(FALSE));
                *isAnyAppBarClosed = true;
            }

            ReleaseInterface(pApplicationBar);
        }
    }

Cleanup:
    RRETURN(hr);
}

// Toggle the app bars when the dismiss layer is right tapped
_Check_return_
HRESULT
ApplicationBarService::OnDismissLayerRightTapped(
    _In_ IInspectable* pSender,
    _In_ xaml_input::IRightTappedRoutedEventArgs* pArgs)
{
    HRESULT hr = S_OK;
    BOOLEAN isHandled = FALSE;
    mui::PointerDeviceType pointerDeviceType = mui::PointerDeviceType_Touch;

    IFC(pArgs->get_PointerDeviceType(&pointerDeviceType));
    if (pointerDeviceType != mui::PointerDeviceType_Mouse)
    {
        goto Cleanup;
    }

    IFC(pArgs->get_Handled(&isHandled));
    if (!isHandled)
    {
        // Set focus state for returning the focus
        SetFocusReturnState(xaml::FocusState_Pointer);
        IFC(ToggleApplicationBars());
        ResetFocusReturnState();
        IFC(pArgs->put_Handled(TRUE));
    }

Cleanup:
    RRETURN(hr);
}

// Handle the left mouse button press on the dismiss layer
_Check_return_
HRESULT
ApplicationBarService::OnDismissLayerPressed(
    _In_ IInspectable* pSender,
    _In_ xaml_input::IPointerRoutedEventArgs* pArgs)
{
    HRESULT hr = S_OK;
    BOOLEAN isLeftButtonPressed = FALSE;
    ixp::IPointerPoint* pPointerPoint = NULL;
    ixp::IPointerPointProperties* pPointerProperties = NULL;
    IUIElement* pIUIElement = NULL;
    BOOLEAN isHandled = FALSE;

    IFC(pArgs->get_Handled(&isHandled));
    if (!isHandled)
    {
        pIUIElement = ctl::query_interface<xaml::IUIElement>(pSender);
        IFC(pArgs->GetCurrentPoint(pIUIElement, &pPointerPoint));
        IFC(pPointerPoint->get_Properties(&pPointerProperties));
        IFC(pPointerProperties->get_IsLeftButtonPressed(&isLeftButtonPressed));
        if(isLeftButtonPressed)
        {
            // We will close the dismiss layer on pointer pressed. So until then we close the appbars but do not alter
            // popup or light dismiss layer state.
            m_suspendLightDismissLayerState = TRUE;

            // Set focus state for returning the focus
            SetFocusReturnState(xaml::FocusState_Pointer);
            // We handle this event for only left mouse button, as our logic for right mouse button is
            // different and handled at ApplicationBarService::OnDismissLayerRightTapped
            IFC(CloseAllNonStickyAppBars());
            ResetFocusReturnState();
            IFC(pArgs->put_Handled(TRUE));
        }
    }

Cleanup:
    ReleaseInterface(pPointerPoint);
    ReleaseInterface(pPointerProperties);
    ReleaseInterface(pIUIElement);
    RRETURN(hr);
}

// Handle the pointer released on dismiss layer. This function will be
// be called if m_suspendLightDismissLayerState is true.
_Check_return_
HRESULT
ApplicationBarService::OnDismissLayerPointerReleased(
    _In_ IInspectable* pSender,
    _In_ xaml_input::IPointerRoutedEventArgs* pArgs)
{
    HRESULT hr = S_OK;
    BOOLEAN isHandled = FALSE;

    IFC(pArgs->get_Handled(&isHandled));

    // No need to run this logic if we are releasing the right mouse button so we also
    // check if our suspension flag is set, which is never set on right mouse pressed.
    if (!isHandled && m_suspendLightDismissLayerState)
    {
        m_suspendLightDismissLayerState = FALSE;
        IFC(UpdateDismissLayer());
        IFC(EvaluatePopupState());
        IFC(pArgs->put_Handled(TRUE));
    }

Cleanup:
    RRETURN(hr);
}

// Called on both activation and de-activation
_Check_return_
HRESULT
ApplicationBarService::OnWindowActivated(
    _In_ IInspectable* pSender,
    _In_ xaml::IWindowActivatedEventArgs* pArgs)
{
    HRESULT hr = S_OK;
    xaml::WindowActivationState state = xaml::WindowActivationState::WindowActivationState_CodeActivated;

    IFC(pArgs->get_WindowActivationState(&state));
    if (state == xaml::WindowActivationState::WindowActivationState_Deactivated)
    {
        // Set focus state for returning the focus
        SetFocusReturnState(xaml::FocusState_Programmatic);
        IFC(CloseAllNonStickyAppBars());
        ResetFocusReturnState();
    }

Cleanup:
    RRETURN(hr);
}

_Check_return_
HRESULT
ApplicationBarService::OnAppBarUnloaded(
    _In_ IInspectable* pSender,
    _In_ xaml::IRoutedEventArgs* pArgs)
{
    HRESULT hr = S_OK;
    ctl::ComPtr<IFrameworkElement> spAppbarIFE = NULL;
    EventRegistrationToken token;
    std::map<xaml::IFrameworkElement*, EventRegistrationToken>::iterator it;

    spAppbarIFE.Attach(ctl::query_interface<IFrameworkElement>(pSender));
    it = m_unloadingAppbars.find(spAppbarIFE.Get());

    if (it != m_unloadingAppbars.end())
    {
        token = it->second;

        IFC(spAppbarIFE->remove_Unloaded(token));

        m_unloadingAppbars.erase(it);
    }

    IFC(EvaluatePopupState());

Cleanup:
    RRETURN(hr);
}

_Check_return_ HRESULT ApplicationBarService::CleanupOpenEventHooks()
{
    HRESULT hr = S_OK;
    IUIElement* pRootVisual = NULL;

    if (m_pEdgeGesture)
    {
        IFC(m_pEdgeGesture->remove_Completed(m_EdgeGestureCompletedEventToken));
        m_EdgeGestureCompletedEventToken.value = 0;
    }

    IFC(ctl::resolve_weakref(m_pRootVisualWeakReference, pRootVisual));

    if (pRootVisual)
    {
        IFC(pRootVisual->remove_KeyDown(m_KeyPressedEventToken));
    }

    CleanupWindowActivatedEventHook();

Cleanup:
    ReleaseInterface(pRootVisual);
    ReleaseInterface(m_pEdgeGesture);
    ReleaseInterface(m_pRootVisualWeakReference);
    RRETURN(hr);
}

_Check_return_ HRESULT ApplicationBarService::CleanupWindowActivatedEventHook()
{
    auto xamlRoot = m_weakXamlRoot.AsOrNull<xaml::IXamlRoot>();
    if (m_windowActivatedHandler && xamlRoot)
    {
        ctl::ComPtr<xaml::IUIElement> contentElement;
        IFC_RETURN(xamlRoot->get_Content(&contentElement));
        if (contentElement)
        {
            Window* window = nullptr;
            IFC_RETURN(DXamlCore::GetCurrent()->GetAssociatedWindowNoRef(contentElement.Cast<UIElement>(), &window));
            if (window)
            {
                IFC_RETURN(m_windowActivatedHandler.DetachEventHandler(ctl::iinspectable_cast(window)));
            }
        }
    }

    return S_OK;
}

// Get the currently pressed keyboard modifiers.
_Check_return_
HRESULT
ApplicationBarService::GetKeyboardModifiers(
    _Out_ wsy::VirtualKeyModifiers* pModifierKeys)
{
    HRESULT hr = S_OK;

    IFC(CoreImports::Input_GetKeyboardModifiers(pModifierKeys));

Cleanup:
    RRETURN(hr);
}

//    AppBar Focus behavior
//
//    If the AppBar is closed:
//
//        If ClosedDisplayMode is Hidden:
//
//            - The AppBars never get keyboard focus.
//
//        If ClosedDisplayMode is Minimal:
//
//            - Only the "..." button can be focused.
//            - When tabbing, the focus order is:
//                - Top AppBar "..." button;
//                - Page content; and then
//                - Bottom AppBar "..." button.
//
//        If ClosedDisplayMode is Compact:
//
//            - All of the AppBar's content can be focused.
//            - When tabbing, the focus order is:
//                - Top AppBar content;
//                - Page content; and then
//                - Bottom AppBar content.
//
//    If the AppBar is open:
//
//        If ClosedDisplayMode is Hidden and IsSticky is set to false:
//
//            - Tabbing iterates through the Top AppBar's content,
//                then the Bottom AppBar's content, never going to the page content.
//
//        Otherwise:
//
//            - Tabbing iterates through the Top AppBar's content,
//                then the page content, then the Bottom AppBar's content.
//
_Check_return_
HRESULT
ApplicationBarService::ProcessTabStopOverride(
    _In_opt_ DependencyObject* pFocusedElement,
    _In_opt_ DependencyObject* pCandidateTabStopElement,
    _In_ BOOLEAN isBackward,
    _Outptr_result_maybenull_ DependencyObject** ppNewTabStop,
    _Out_ BOOLEAN* pIsTabStopOverridden
     )
{
    HRESULT hr = S_OK;
    AppBar* pTopAppBar = nullptr;
    AppBar* pBottomAppBar = nullptr;
    BOOLEAN isAnyAppBarLightDismiss = FALSE;
    BOOLEAN isTopAppBarOpen = FALSE;
    BOOLEAN isBottomAppBarOpen = FALSE;
    bool isAnyAppBarVisibleWhenClosed = false;
    bool isTopAppBarVisibleWhenClosed = false;
    bool isBottomAppBarVisibleWhenClosed = false;
    bool isInTopAppBar = false;
    bool isInBottomAppBar = false;
    UIElement* pFocusedElementUI = nullptr;
    BOOLEAN shouldExitAppBar = FALSE;
    CDependencyObject* pNewTabStop = nullptr;

    IFCCATASTROPHIC(ppNewTabStop);
    IFCCATASTROPHIC(pIsTabStopOverridden);

    *ppNewTabStop = NULL;
    *pIsTabStopOverridden = FALSE;

    IFC(GetTopAndBottomAppBars(false /* openAppBarsOnly */, &pTopAppBar, &pBottomAppBar, &isAnyAppBarLightDismiss));

    if (pTopAppBar != nullptr)
    {
        auto closedDisplayMode = xaml_controls::AppBarClosedDisplayMode_Hidden;
        IFC(pTopAppBar->get_ClosedDisplayMode(&closedDisplayMode));
        isTopAppBarVisibleWhenClosed = (closedDisplayMode != xaml_controls::AppBarClosedDisplayMode_Hidden);

        IFC(pTopAppBar->get_IsOpen(&isTopAppBarOpen));
    }

    if (pBottomAppBar != nullptr)
    {
        auto closedDisplayMode = xaml_controls::AppBarClosedDisplayMode_Hidden;
        IFC(pBottomAppBar->get_ClosedDisplayMode(&closedDisplayMode));
        isBottomAppBarVisibleWhenClosed = (closedDisplayMode != xaml_controls::AppBarClosedDisplayMode_Hidden);

        IFC(pBottomAppBar->get_IsOpen(&isBottomAppBarOpen));
    }

    isAnyAppBarVisibleWhenClosed = isTopAppBarVisibleWhenClosed || isBottomAppBarVisibleWhenClosed;

    // If no AppBar exists that is currently visible, then we don't suggest a focus element.
    if ((pTopAppBar == nullptr || (!isTopAppBarOpen && !isTopAppBarVisibleWhenClosed)) &&
        (pBottomAppBar == nullptr || (!isBottomAppBarOpen && !isBottomAppBarVisibleWhenClosed)))
    {
        goto Cleanup;
    }
    else if (pFocusedElement == nullptr)
    {
        // Get the first focusable elements from app bars
        IFC(GetFirstFocusableElementFromAppBars(pTopAppBar, pBottomAppBar, AppBarTabPriority_Bottom, isBackward, &pNewTabStop));
    }
    else if (pFocusedElement != nullptr)
    {
        // Determine whether the focused element is in one of the appbars
        pFocusedElementUI = static_cast<UIElement *>(ctl::query_interface<IUIElement>(pFocusedElement));
        CFocusManager* focusManager = VisualTree::GetFocusManagerForElement(pFocusedElement->GetHandle());

        if (pTopAppBar)
        {
            IFC(pTopAppBar->ContainsElement(pFocusedElementUI, &isInTopAppBar));
        }
        if (pBottomAppBar && !isInTopAppBar)
        {
            IFC(pBottomAppBar->ContainsElement(pFocusedElementUI, &isInBottomAppBar));
        }
        ASSERT(!isInTopAppBar || !isInBottomAppBar);
        if (!isBackward)
        {
            if (isInTopAppBar)
            {
                IFC(GetShouldExitAppBar(pTopAppBar, pFocusedElement->GetHandle(), isBackward, &shouldExitAppBar));
                if (shouldExitAppBar)
                {
                    // We've reached the end of the top app bar.
                    // If either of the app bars is light dismiss and none are visible when closed, then focus goes to the first element of the bottom app bar.
                    // If all app bars are sticky or at least one is visible when closed, then focus goes to the page content.
                    if (isAnyAppBarLightDismiss && !isAnyAppBarVisibleWhenClosed)
                    {
                        IFC(GetFirstFocusableElementFromAppBars(pTopAppBar, pBottomAppBar, AppBarTabPriority_Bottom, isBackward, &pNewTabStop));
                    }
                    else
                    {
                        pNewTabStop = focusManager->GetFirstFocusableElementFromRoot(isBackward);
                        AddRefInterface(pNewTabStop);
                    }
                }
                // If we should not exit the appbar, new tab stop returns null and regular focus manager logic is followed.
            }
            else if (isInBottomAppBar)
            {
                IFC(GetShouldExitAppBar(pBottomAppBar, pFocusedElement->GetHandle(), isBackward, &shouldExitAppBar));
                if (shouldExitAppBar)
                {
                    // We've reached the end of the bottom app bar.  First try to go to the top app bar.
                    // If we can't and either of the app bars is light dismiss, and none are visible when closed, then focus goes to the first element of the bottom app bar.
                    // If all app bars are sticky or at least one is visible when closed, then focus goes to the page content.
                    IFC(GetFirstFocusableElementFromAppBars(pTopAppBar, NULL, AppBarTabPriority_Top, isBackward, &pNewTabStop));
                    if (pNewTabStop == nullptr)
                    {
                        if (isAnyAppBarLightDismiss && !isAnyAppBarVisibleWhenClosed)
                        {
                            IFC(GetFirstFocusableElementFromAppBars(pTopAppBar, pBottomAppBar, AppBarTabPriority_Top, isBackward, &pNewTabStop));
                        }
                        else
                        {
                            pNewTabStop = focusManager->GetFirstFocusableElementFromRoot(isBackward);
                            AddRefInterface(pNewTabStop);
                        }
                    }
                }
                // If we should not exit the appbar, new tab stop returns null and regular focus manager logic is followed.
            }
            else
            {
                // The Focused element is not in the app bars. We need to try to find a focusable element from app bars.
                const BOOLEAN shouldEnterAppBar = GetShouldEnterAppBar(pFocusedElement->GetHandle(), isBackward);
                if (shouldEnterAppBar)
                {
                    IFC(GetFirstFocusableElementFromAppBars(pTopAppBar, pBottomAppBar, AppBarTabPriority_Bottom, isBackward, &pNewTabStop));
                }
                // If we should not enter the appbar, new tab stop returns null and regular focus manager logic is followed.
            }
        }
        else
        {
            if (isInTopAppBar)
            {
                IFC(GetShouldExitAppBar(pTopAppBar, pFocusedElement->GetHandle(), isBackward, &shouldExitAppBar));
                if (shouldExitAppBar)
                {
                    // We've reached the beginning of the top app bar while shift-tabbing.  First try to go to the last item of the bottom app bar.
                    // If we can't and either of the app bars is light dismiss, and none are visible when closed, then focus goes to the last element of the top app bar.
                    // If all app bars are sticky or at least one is visible when closed, then focus goes to the page content.
                    IFC(GetFirstFocusableElementFromAppBars(NULL, pBottomAppBar, AppBarTabPriority_Bottom, isBackward, &pNewTabStop));
                    if (pNewTabStop == nullptr)
                    {
                        if (isAnyAppBarLightDismiss && !isAnyAppBarVisibleWhenClosed)
                        {
                            IFC(GetFirstFocusableElementFromAppBars(pTopAppBar, pBottomAppBar, AppBarTabPriority_Bottom, isBackward, &pNewTabStop));
                        }
                        else
                        {
                            pNewTabStop = focusManager->GetFirstFocusableElementFromRoot(isBackward);
                            AddRefInterface(pNewTabStop);
                        }
                    }
                }
                // If we should not exit the appbar, new tab stop returns null and regular focus manager logic is followed.
            }
            else if (isInBottomAppBar)
            {
                // We've reached the beginning of the bottom app bar while shift-tabbing.
                // If either of the app bars is light dismiss and none are visible when closed, then focus goes to the last element of the top app bar.
                // If all app bars are sticky or at least one is visible when closed, then focus goes to the page content.
                IFC(GetShouldExitAppBar(pBottomAppBar, pFocusedElement->GetHandle(), isBackward, &shouldExitAppBar));
                if (shouldExitAppBar)
                {
                    if (isAnyAppBarLightDismiss && !isAnyAppBarVisibleWhenClosed)
                    {
                        IFC(GetFirstFocusableElementFromAppBars(pTopAppBar, pBottomAppBar, AppBarTabPriority_Top, isBackward, &pNewTabStop));
                    }
                    else
                    {
                        pNewTabStop = focusManager->GetFirstFocusableElementFromRoot(isBackward);
                        AddRefInterface(pNewTabStop);
                    }
                }
                // If we should not exit the appbar, new tab stop returns null and regular focus manager logic is followed.
            }
            else
            {
                const BOOLEAN shouldEnterAppBar = GetShouldEnterAppBar(pFocusedElement->GetHandle(), isBackward);
                if (shouldEnterAppBar)
                {
                    // Shift-tabbing from the beginning of the main content, we need to go to top app bar if it is open.
                    // If it is not open we need to go to the bottom appbar
                    IFC(GetFirstFocusableElementFromAppBars(pTopAppBar, pBottomAppBar, AppBarTabPriority_Top, isBackward, &pNewTabStop));
                }
                // If we should not enter the appbar, new tab stop returns null and regular focus manager logic is followed.
            }
        }
    }
    if (pNewTabStop)
    {
        IFC(DXamlCore::GetCurrent()->GetPeer(pNewTabStop, ppNewTabStop));
        *pIsTabStopOverridden = TRUE;
    }

Cleanup:
     ctl::release_interface(pTopAppBar);
     ctl::release_interface(pBottomAppBar);
     ctl::release_interface(pFocusedElementUI);
     ReleaseInterface(pNewTabStop);
     RRETURN(hr);
}

// Gets the focusable element from the appbars depending on parameters.
// tabPriority -> Determines which appbar to look first
// startFromBottom -> Determines whether to get the first or the last focusable element
_Check_return_
HRESULT
ApplicationBarService::GetFirstFocusableElementFromAppBars(
    _In_opt_ AppBar* pTopAppBar,
    _In_opt_ AppBar* pBottomAppBar,
    _In_ AppBarTabPriority tabPriority,
    _In_ BOOLEAN startFromEnd,
    _Outptr_result_maybenull_ CDependencyObject **ppNewTabStop)
{
    HRESULT hr = S_OK;
    CDependencyObject* pNewTabStop = NULL;

    IFCCATASTROPHIC(ppNewTabStop);
    *ppNewTabStop = NULL;
    if (tabPriority == AppBarTabPriority_Bottom)
    {
        // Bottom AppBar is priority
        // Getting the first element
        if (pBottomAppBar != NULL)
        {
            if (!startFromEnd)
            {
                // FIRST focusable element from BOTTOM Appbar
                IFC(CoreImports::FocusManager_GetFirstFocusableElement(pBottomAppBar->GetHandle(), &pNewTabStop));
            }
            else
            {
                // LAST focusable element from BOTTOM Appbar
                IFC(CoreImports::FocusManager_GetLastFocusableElement(pBottomAppBar->GetHandle(), &pNewTabStop));
            }
        }
        if (pTopAppBar != NULL && pNewTabStop == NULL)
        {
            if (!startFromEnd)
            {
                // FIRST focusable element from TOP Appbar
                IFC(CoreImports::FocusManager_GetFirstFocusableElement(pTopAppBar->GetHandle(), &pNewTabStop));
            }
            else
            {
                // LAST focusable element from TOP Appbar
                IFC(CoreImports::FocusManager_GetLastFocusableElement(pTopAppBar->GetHandle(), &pNewTabStop));
            }
        }
    }
    else if (tabPriority == AppBarTabPriority_Top)
    {
        //Top AppBar is priority
        // Getting the first element
        if (pTopAppBar != NULL)
        {
            if (!startFromEnd)
            {
                // FIRST focusable element from TOP Appbar
                IFC(CoreImports::FocusManager_GetFirstFocusableElement(pTopAppBar->GetHandle(), &pNewTabStop));
            }
            else
            {
                // LAST focusable element from TOP Appbar
                IFC(CoreImports::FocusManager_GetLastFocusableElement(pTopAppBar->GetHandle(), &pNewTabStop));
            }
        }
        if (pBottomAppBar != NULL && pNewTabStop == NULL)
        {
            if (!startFromEnd)
            {
                // FIRST focusable element from BOTTOM Appbar
                IFC(CoreImports::FocusManager_GetFirstFocusableElement(pBottomAppBar->GetHandle(), &pNewTabStop));
            }
            else
            {
                // LAST focusable element from BOTTOM Appbar
                IFC(CoreImports::FocusManager_GetLastFocusableElement(pBottomAppBar->GetHandle(), &pNewTabStop));
            }
        }
    }
    *ppNewTabStop = pNewTabStop;
    pNewTabStop = NULL;

Cleanup:
    ReleaseInterface(pNewTabStop);
    RRETURN(hr);
}

// This method decides if focus should enter an appbar
// We should enter an appbar in these conditions;
//      . Shift tab from the first focusable element of the root
//      . Tab from the last focusable element of the root
bool ApplicationBarService::GetShouldEnterAppBar (
    _In_ CDependencyObject *pFocusedElement,
    _In_ BOOLEAN shiftPressed)
{
    CFocusManager* focusManager = VisualTree::GetFocusManagerForElement(pFocusedElement);
    CDependencyObject* pFocusableElementFromRoot = focusManager->GetFirstFocusableElementFromRoot(!shiftPressed);
    return pFocusableElementFromRoot == pFocusedElement;
}

// This method decides if focus should leave the appbar
// We should exit the appbar in these conditions;
//      . Shift tab from the first focusable element of the appbar
//      . Tab from the last focusable element of the appbar
_Check_return_
HRESULT
ApplicationBarService::GetShouldExitAppBar (
    _In_ AppBar* pAppBar,
    _In_ CDependencyObject* pFocusedElement,
    _In_ BOOLEAN shiftPressed,
    _Out_ BOOLEAN* pShouldExitAppBar)
{
    HRESULT hr = S_OK;
    CDependencyObject* pFocusableElement = NULL;
    CDependencyObject* pAppBarCDO = NULL;
    BOOLEAN shouldExit = FALSE;

    *pShouldExitAppBar = FALSE;
    pAppBarCDO = pAppBar->GetHandle();
    if (shiftPressed)
    {
        IFC(CoreImports::FocusManager_GetFirstFocusableElement(pAppBarCDO, &pFocusableElement));
        if (pFocusedElement == pFocusableElement)
        {
            shouldExit = TRUE;
        }
    }
    else
    {
        IFC(CoreImports::FocusManager_GetLastFocusableElement(pAppBarCDO, &pFocusableElement));
        if (pFocusedElement == pFocusableElement)
        {
            shouldExit = TRUE;
        }
    }
    *pShouldExitAppBar = shouldExit;

Cleanup:
    ReleaseInterface(pFocusableElement);
    RRETURN(hr);
}

_Check_return_
HRESULT
ApplicationBarService::GetAppBarStatus (
    _Out_ bool* pIsTopOpen,
    _Out_ bool* pIsTopSticky,
    _Out_ XFLOAT* pWidthTop,
    _Out_ XFLOAT* pHeightTop,
    _Out_ bool* pIsBottomOpen,
    _Out_ bool* pIsBottomSticky,
    _Out_ XFLOAT* pWidthBottom,
    _Out_ XFLOAT* pHeightBottom)
{
    HRESULT hr = S_OK;
    ctl::ComPtr<IUIElement> spTopAppBarUIE = NULL;
    ctl::ComPtr<IAppBar> spTopAppBar = NULL;
    AppBar* pTopAppBar = NULL;
    BOOLEAN isTopAppBarOpen = FALSE;
    BOOLEAN isTopAppBarSticky = FALSE;
    DOUBLE widthTop = 0;
    DOUBLE heightTop = 0;
    ctl::ComPtr<IUIElement> spBottomAppBarUIE = NULL;
    ctl::ComPtr<IAppBar> spBottomAppBar = NULL;
    AppBar* pBottomAppBar = NULL;
    BOOLEAN isBottomAppBarOpen = FALSE;
    BOOLEAN isBottomAppBarSticky = FALSE;
    DOUBLE widthBottom = 0;
    DOUBLE heightBottom = 0;

    IFCPTR(pIsTopOpen);
    IFCPTR(pIsTopSticky);
    IFCPTR(pWidthTop);
    IFCPTR(pHeightTop);
    IFCPTR(pIsBottomOpen);
    IFCPTR(pIsBottomSticky);
    IFCPTR(pWidthBottom);
    IFCPTR(pHeightBottom);

    *pIsTopOpen = FALSE;
    *pIsTopSticky = FALSE;
    *pWidthTop = 0.0f;
    *pHeightTop = 0.0f;
    *pIsBottomOpen = FALSE;
    *pIsBottomSticky = FALSE;
    *pWidthBottom = 0.0f;
    *pHeightBottom = 0.0f;

    if (m_tpTopBarHost)
    {
        IFC(m_tpTopBarHost->get_Child(spTopAppBarUIE.GetAddressOf()));

        spTopAppBar = spTopAppBarUIE ? spTopAppBarUIE.AsOrNull<IAppBar>() : NULL;
        pTopAppBar = spTopAppBar.Cast<AppBar>();

        if (pTopAppBar)
        {
            ctl::ComPtr<IFrameworkElement> spTopAppBarIFE = NULL;

            IFC(pTopAppBar->get_IsOpen(&isTopAppBarOpen));
            *pIsTopOpen = !!isTopAppBarOpen;

            IFC(pTopAppBar->get_IsSticky(&isTopAppBarSticky));
            *pIsTopSticky = !!isTopAppBarSticky;

            IFC(spTopAppBarUIE.As<IFrameworkElement>(&spTopAppBarIFE));

            IFC(spTopAppBarIFE->get_ActualWidth(&widthTop));
            IFC(spTopAppBarIFE->get_ActualHeight(&heightTop));

            *pWidthTop = static_cast<XFLOAT>(widthTop);
            *pHeightTop = static_cast<XFLOAT>(heightTop);
        }
    }

    if (m_tpBottomBarHost)
    {
        IFC(m_tpBottomBarHost->get_Child(spBottomAppBarUIE.GetAddressOf()));

        spBottomAppBar = spBottomAppBarUIE ? spBottomAppBarUIE.AsOrNull<IAppBar>() : NULL;
        pBottomAppBar = spBottomAppBar.Cast<AppBar>();

        if (pBottomAppBar)
        {
            ctl::ComPtr<IFrameworkElement> spBottomAppBarIFE = NULL;

            IFC(pBottomAppBar->get_IsOpen(&isBottomAppBarOpen));
            *pIsBottomOpen = !!isBottomAppBarOpen;

            IFC(pBottomAppBar->get_IsSticky(&isBottomAppBarSticky));
            *pIsBottomSticky = !!isBottomAppBarSticky;

            IFC(spBottomAppBarUIE.As<IFrameworkElement>(&spBottomAppBarIFE));

            IFC(spBottomAppBarIFE->get_ActualWidth(&widthBottom));
            IFC(spBottomAppBarIFE->get_ActualHeight(&heightBottom));

            *pWidthBottom = static_cast<XFLOAT>(widthBottom);
            *pHeightBottom = static_cast<XFLOAT>(heightBottom);
        }
    }

Cleanup:
    RRETURN(hr);
}

_Check_return_
HRESULT
ApplicationBarService::ProcessToggleApplicationBarsFromMouseRightTapped()
{
    HRESULT hr = S_OK;

    SetFocusReturnState(xaml::FocusState_Pointer);
    IFC(ToggleApplicationBars());
    ResetFocusReturnState();

Cleanup:
    RRETURN(hr);
}

// Propagate owner's FlowDirection and Language values to AppBarHost
_Check_return_
HRESULT
ApplicationBarService::SetAppBarOwnerPropertiesOnHost(
    _In_ Border* pAppBarHost)
{
    HRESULT hr = S_OK;
    xaml::FlowDirection flowDirection = xaml::FlowDirection_LeftToRight;
    wrl_wrappers::HString strLanguage;
    ctl::ComPtr<IUIElement> spAppBarIUE;
    ctl::ComPtr<IAppBar> spAppBar;
    ctl::ComPtr<Page> spOwner;

    IFC(pAppBarHost->get_Child(&spAppBarIUE));
    spAppBar = spAppBarIUE.AsOrNull<IAppBar>();
    if (spAppBar)
    {
        // Get AppBar's owner
        IFC(spAppBar.Cast<AppBar>()->GetOwner(&spOwner));
        if (spOwner)
        {
            // Propagate owner's FlowDirection and Language
            IFC(spOwner->get_FlowDirection(&flowDirection));
            IFC(pAppBarHost->put_FlowDirection(flowDirection));

            IFC(spOwner->get_Language(strLanguage.GetAddressOf()));
            IFC(pAppBarHost->put_Language(strLanguage.Get()));
        }
    }

Cleanup:
    RRETURN(hr);
}

// Clear owner's FlowDirection and Language values that were set on AppBarHost
_Check_return_
HRESULT
ApplicationBarService::ClearAppBarOwnerPropertiesOnHost(
    _In_ Border* pAppBarHost)
{
    HRESULT hr = S_OK;

    // Clear FlowDirection and Language properties
    IFC(pAppBarHost->ClearValue(
        MetadataAPI::GetDependencyPropertyByIndex(KnownPropertyIndex::FrameworkElement_FlowDirection)));

    IFC(pAppBarHost->ClearValue(
        MetadataAPI::GetDependencyPropertyByIndex(KnownPropertyIndex::FrameworkElement_Language)));

Cleanup:
    RRETURN(hr);
}

_Check_return_ HRESULT
ApplicationBarService::ReevaluateIsOverlayVisible()
{
    ASSERT(m_tpPopupHost);

    ctl::ComPtr<AppBar> topAppBar;
    ctl::ComPtr<AppBar> bottomAppBar;
    ctl::ComPtr<Page> ownerPage;

    BOOLEAN isAnyLightDismiss = FALSE;
    IFC_RETURN(GetTopAndBottomOpenAppBars(&topAppBar, &bottomAppBar, &isAnyLightDismiss));

    // Evaluate the overlay visibility based on the setting of both the top
    // and bottom open app bars.  The most 'visible' mode from either takes
    // precedence.
    bool isOverlayVisible = false;
    {
        if (topAppBar)
        {
            IFC_RETURN(LightDismissOverlayHelper::ResolveIsOverlayVisibleForControl(topAppBar.Get(), &isOverlayVisible));

            IFC_RETURN(topAppBar->GetOwner(&ownerPage));
        }

        if (bottomAppBar)
        {
            if (!isOverlayVisible)
            {
                IFC_RETURN(LightDismissOverlayHelper::ResolveIsOverlayVisibleForControl(bottomAppBar.Get(), &isOverlayVisible));
            }

            if (!ownerPage)
            {
                IFC_RETURN(bottomAppBar->GetOwner(&ownerPage));
            }
        }
    }

    if (isOverlayVisible != m_isOverlayVisible)
    {
        m_isOverlayVisible = isOverlayVisible;

        if (m_isOverlayVisible)
        {
            // If we get in here, then we have an app bar set on some page.
            ASSERT(ownerPage);
            ASSERT(m_tpDismissLayer);

            // The dismiss layer is the popup host's child.  Update it's
            // requested theme to match that of our appbars' owner page.
            // This effectively sets the overlay's theme because the popup
            // host's overlay synchronizes its theme with the popup's child.
            auto requestedTheme = xaml::ElementTheme_Default;
            IFC_RETURN(ownerPage->get_RequestedTheme(&requestedTheme));
            IFC_RETURN(m_tpDismissLayer->put_RequestedTheme(requestedTheme));

            // Normally, the overlay is only shown for light-dismiss popups, but for controls that roll their own
            // light-dismiss logic (and therefore configure their popup's to not be light-dismiss) we still want
            // to re-use the popup's overlay code.
            IFC_RETURN(m_tpPopupHost.Cast<Popup>()->put_DisableOverlayIsLightDismissCheck(TRUE));
            IFC_RETURN(m_tpPopupHost->put_LightDismissOverlayMode(xaml_controls::LightDismissOverlayMode_On));

            // Set the appropriate brush resource to use for the overlay.
            xstring_ptr themeBrush;
            IFC_RETURN(xstring_ptr::CloneBuffer(L"AppBarLightDismissOverlayBackground", &themeBrush));
            IFC_RETURN(static_cast<CPopup*>(m_tpPopupHost->GetHandle())->SetOverlayThemeBrush(themeBrush));

            if (!m_overlayOpeningStoryboard || !m_overlayClosingStoryboard)
            {
                IFC_RETURN(CreateOverlayAnimations());
            }

            // Retarget the animations to the new overlay element.
            IFC_RETURN(UpdateTargetForOverlayAnimations());
        }
        else
        {
            IFC_RETURN(m_tpPopupHost->put_LightDismissOverlayMode(xaml_controls::LightDismissOverlayMode_Off));
        }
    }

    return S_OK;
}

_Check_return_ HRESULT
ApplicationBarService::CreateOverlayAnimations()
{
    // This method creates animations that match the default opening/closing
    // visual transitions for AppBar/CommandBar.
    auto createAnimation = [](
        INT64 durationMs,
        double startValue,
        double endValue,
        wf::Point controlPoint1,
        wf::Point controlPoint2,
        _In_ CDependencyObject* target,
        _Out_ xaml_animation::IStoryboard** storyboard) -> HRESULT
    {
        ctl::ComPtr<DoubleAnimationUsingKeyFrames> doubleAnimation;
        IFC_RETURN(ctl::make(&doubleAnimation));

        IFC_RETURN(StoryboardFactory::SetTargetPropertyStatic(doubleAnimation.Get(), wrl_wrappers::HStringReference(L"Opacity").Get()));

        ctl::ComPtr<wfc::IVector<xaml_animation::DoubleKeyFrame*>> keyFrames;
        IFC_RETURN(doubleAnimation->get_KeyFrames(&keyFrames));

        ctl::ComPtr<DiscreteDoubleKeyFrame> firstKeyFrame;
        IFC_RETURN(ctl::make(&firstKeyFrame));

        xaml_animation::KeyTime keyTime = {};
        keyTime.TimeSpan.Duration = 0;
        IFC_RETURN(firstKeyFrame->put_KeyTime(keyTime));
        IFC_RETURN(firstKeyFrame->put_Value(startValue));
        IFC_RETURN(keyFrames->Append(firstKeyFrame.Get()));

        ctl::ComPtr<KeySpline> keySpline;
        IFC_RETURN(ctl::make(&keySpline));
        IFC_RETURN(keySpline->put_ControlPoint1(controlPoint1));
        IFC_RETURN(keySpline->put_ControlPoint2(controlPoint2));

        ctl::ComPtr<SplineDoubleKeyFrame> secondKeyFrame;
        IFC_RETURN(ctl::make(&secondKeyFrame));

        keyTime.TimeSpan.Duration = durationMs * 10000;     // Converting milliseconds to nano seconds.
        IFC_RETURN(secondKeyFrame->put_KeyTime(keyTime));
        IFC_RETURN(secondKeyFrame->put_Value(endValue));
        IFC_RETURN(secondKeyFrame->put_KeySpline(keySpline.Get()));
        IFC_RETURN(keyFrames->Append(secondKeyFrame.Get()));

        ctl::ComPtr<Storyboard> localStoryboard;
        IFC_RETURN(ctl::make(&localStoryboard));

        IFC_RETURN(CoreImports::Storyboard_SetTarget(static_cast<CTimeline*>(localStoryboard->GetHandle()), target));

        ctl::ComPtr<wfc::IVector<xaml_animation::Timeline*>> children;
        IFC_RETURN(localStoryboard->get_Children(&children));
        IFC_RETURN(children->Append(doubleAnimation.Get()));

        IFC_RETURN(localStoryboard.CopyTo(storyboard));

        return S_OK;
    };

    ctl::ComPtr<xaml_animation::IStoryboard> storyboard;

    IFC_RETURN(createAnimation(
        s_OpeningDurationMs,
        0.0,
        1.0,
        { 0.1f, 0.9f },
        { 0.2f, 1.0f },
        m_tpDismissLayer->GetHandle(),
        storyboard.ReleaseAndGetAddressOf()));
    SetPtrValue(m_overlayOpeningStoryboard, storyboard.Get());

    IFC_RETURN(createAnimation(
        s_ClosingDurationMs,
        1.0,
        0.0,
        { 0.2f, 0.0f },
        { 0.0f, 1.0f },
        m_tpDismissLayer->GetHandle(),
        storyboard.ReleaseAndGetAddressOf()));
    SetPtrValue(m_overlayClosingStoryboard, storyboard.Get());

    IFC_RETURN(m_overlayClosingCompletedHandler.AttachEventHandler(
        m_overlayClosingStoryboard.Cast<Storyboard>(),
        [this](_In_ IInspectable* /*sender*/, _In_ IInspectable* /*args*/)
        {
            // We wait until the closing animation completes before re-evaluating our popup's
            // overlay state to make sure the fade animation plays before getting rid of
            // the overlay element.
            return ReevaluateIsOverlayVisible();
        }));

    return S_OK;
}

_Check_return_ HRESULT
ApplicationBarService::UpdateTargetForOverlayAnimations()
{
    ASSERT(m_tpPopupHost);
    ASSERT(m_isOverlayVisible);

    ctl::ComPtr<xaml::IFrameworkElement> overlayElement;
    IFC_RETURN(m_tpPopupHost->get_OverlayElement(&overlayElement));

    if (m_overlayOpeningStoryboard)
    {
        IFC_RETURN(m_overlayOpeningStoryboard->Stop());

        IFC_RETURN(CoreImports::Storyboard_SetTarget(
            static_cast<CTimeline*>(m_overlayOpeningStoryboard.Cast<Storyboard>()->GetHandle()),
            overlayElement.Cast<FrameworkElement>()->GetHandle())
            );
    }

    if (m_overlayClosingStoryboard)
    {
        IFC_RETURN(m_overlayClosingStoryboard->Stop());

        IFC_RETURN(CoreImports::Storyboard_SetTarget(
            static_cast<CTimeline*>(m_overlayClosingStoryboard.Cast<Storyboard>()->GetHandle()),
            overlayElement.Cast<FrameworkElement>()->GetHandle())
            );
    }

    return S_OK;
}

_Check_return_ HRESULT
ApplicationBarService::PlayOverlayOpeningAnimation()
{
    ASSERT(m_isOverlayVisible);

    if (m_overlayClosingStoryboard)
    {
        IFC_RETURN(m_overlayClosingStoryboard->Stop());
    }

    if (m_overlayOpeningStoryboard)
    {
        IFC_RETURN(m_overlayOpeningStoryboard->Begin());
    }

    return S_OK;
}

_Check_return_ HRESULT
ApplicationBarService::PlayOverlayClosingAnimation()
{
    ASSERT(m_isOverlayVisible);

    if (m_overlayOpeningStoryboard)
    {
        IFC_RETURN(m_overlayOpeningStoryboard->Stop());
    }

    if (m_overlayClosingStoryboard)
    {
        IFC_RETURN(m_overlayClosingStoryboard->Begin());
    }

    return S_OK;
}

_Check_return_
HRESULT
ApplicationBarServiceStatics::GetAppBarStatus(
    _In_ CDependencyObject* object,
    _Out_ bool* pIsTopOpen,
    _Out_ bool* pIsTopSticky,
    _Out_ XFLOAT* pTopWidth,
    _Out_ XFLOAT* pTopHeight,
    _Out_ bool* pIsBottomOpen,
    _Out_ bool* pIsBottomSticky,
    _Out_ XFLOAT* pBottomWidth,
    _Out_ XFLOAT* pBottomHeight)
{
    ctl::ComPtr<IApplicationBarService> applicationBarService;
    ctl::ComPtr<DependencyObject> objectDo;

    *pIsTopOpen = false;
    *pIsTopSticky = false;
    *pTopWidth = 0;
    *pTopHeight = 0;
    *pIsBottomOpen = false;
    *pIsBottomSticky = false;
    *pBottomWidth = 0;
    *pBottomHeight = 0;

    IFC_RETURN(DXamlCore::GetCurrent()->GetPeer(object, &objectDo));
    if (objectDo)
    {
        if (const auto xamlRoot = XamlRoot::GetImplementationForElementStatic(objectDo.Get()))
        {
            IFC_RETURN(xamlRoot->TryGetApplicationBarService(applicationBarService));
            if (applicationBarService)
            {
                IFC_RETURN(applicationBarService->GetAppBarStatus(pIsTopOpen, pIsTopSticky, pTopWidth,
                            pTopHeight, pIsBottomOpen, pIsBottomSticky, pBottomWidth, pBottomHeight));
            }
        }
    }

    return S_OK;
}

_Check_return_
HRESULT
ApplicationBarServiceStatics::ProcessToggleApplicationBarsFromMouseRightTapped(_In_ IInspectable* xamlRootInspectable)
{
    IFCEXPECT_RETURN(xamlRootInspectable);
    ctl::ComPtr<IInspectable> xamlRootIns{xamlRootInspectable};

    ctl::ComPtr<XamlRoot> xamlRoot;
    IFC_RETURN(xamlRootIns.As(&xamlRoot));

    ctl::ComPtr<IApplicationBarService> applicationBarService;
    IFC_RETURN(xamlRoot->TryGetApplicationBarService(applicationBarService));
    if (applicationBarService)
    {
        IFC_RETURN(applicationBarService->ProcessToggleApplicationBarsFromMouseRightTapped());
    }
    return S_OK;
}
