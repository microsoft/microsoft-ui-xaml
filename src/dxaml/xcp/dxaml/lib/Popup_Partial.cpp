// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#include "precomp.h"
#include "Popup.g.h"
#include "Window.g.h"
#include "PopupAutomationPeer.g.h"
#include "ToolTip.g.h"
#include <FrameworkUdk/BackButtonIntegration.h>
#include "Callback.h"
#include <FocusMgr.h>
#include "XamlRoot_Partial.h"
#include "FocusObserver.h"
#include <FrameworkUdk/CoreWindowIntegration.h>
#include "FlyoutBase.g.h"

using namespace DirectUI;

// Initializes a new instance of the Popup class.
Popup::Popup()
    : m_previousXamlRootWidth(-1.0)
    , m_previousXamlRootHeight(-1.0)
    , m_dismissalTriggerFlags(DismissalTriggerFlags::None)
{
    m_defaultAutomationName = nullptr;
}

// Destructor
Popup::~Popup()
{
    if (DXamlCore::GetCurrent() != nullptr)
    {
        // Clear the caches on the current window
        ClearWindowCaches();
    }
    
    // These handlers don't require a DXamlCore on the thread.
    IGNOREHR(BackButtonIntegration_UnregisterListener(this));
    IGNOREHR(WindowPositionChanged_UnregisterListener(this));
}

void Popup::ClearWindowCaches()
{
    IGNOREHR(UnregisterForXamlRootChangedEvents());
    IGNOREHR(UnregisterForWindowActivatedEvents());
}

_Check_return_ HRESULT Popup::Initialize()
{
    IFC_RETURN(PopupGenerated::Initialize());
    return S_OK;
}

_Check_return_ HRESULT Popup::HookupWindowPositionChangedHandler(_In_ CDependencyObject* nativePopup)
{
    ASSERT(nativePopup != nullptr);

    ctl::ComPtr<DependencyObject> spPopupAsDO;
    IFC_RETURN(DXamlCore::GetCurrent()->GetPeer(nativePopup, &spPopupAsDO));

    ctl::ComPtr<DirectUI::Popup> spPopup;
    IFC_RETURN(spPopupAsDO.As(&spPopup));

    IFC_RETURN(spPopup->HookupWindowPositionChangedHandlerImpl());

    return S_OK;
}

// Window position changed events for light dismiss of windowed popups.
_Check_return_ HRESULT Popup::HookupWindowPositionChangedHandlerImpl()
{
    if (CPopup::DoesPlatformSupportWindowedPopup(DXamlCore::GetCurrent()->GetHandle())
        && DXamlCore::GetCurrent()->GetHandle()->UseWindowPosChanged())
    {
        IFC_RETURN(WindowPositionChanged_RegisterListener(this));
    }

    return S_OK;
}

_Check_return_ HRESULT Popup::CacheContentPositionInfo()
{
    // use changes in xaml root size or screen position to detect orientation/size changed for light dismiss.
    wf::Rect currentVisibleContentBounds{};
    IFC_RETURN(DXamlCore::GetCurrent()->GetVisibleContentBoundsForElement(false, true, GetHandle(), &currentVisibleContentBounds));
    m_previousWindowX = currentVisibleContentBounds.X;
    m_previousWindowY = currentVisibleContentBounds.Y;

    auto xamlRoot = XamlRoot::GetForElementStatic(this);
    if (xamlRoot)
    {
        wf::Size currentXamlRootSize{};
        IFC_RETURN(xamlRoot->get_Size(&currentXamlRootSize));
        m_previousXamlRootWidth = currentXamlRootSize.Width;
        m_previousXamlRootHeight = currentXamlRootSize.Height;
    }

    return S_OK;
}

_Check_return_ HRESULT Popup::OnBackButtonPressedImpl(_Out_ BOOLEAN* pHandled)
{
    HRESULT hr = S_OK;

    *pHandled = FALSE;

    BOOLEAN shouldDismiss;
    IFC(ShouldDismiss(DismissalTriggerFlags::BackPress, &shouldDismiss));
    if (shouldDismiss)
    {
        IFC(put_IsOpen(FALSE));
        *pHandled = TRUE;
    }

Cleanup:
    RRETURN(hr);
}

// callback function that is called when the core popup class "Close" function is called
_Check_return_ HRESULT
Popup::OnClosed(_In_ CDependencyObject* pNativePopup)
{
    ctl::ComPtr<DependencyObject> spPopupAsDO;

    ASSERT(pNativePopup != NULL);
    IFC_RETURN(DXamlCore::GetCurrent()->TryGetPeer(pNativePopup, &spPopupAsDO));

    if (spPopupAsDO)
    {
        ctl::ComPtr<IPopup> spThisPopup;

        IFC_RETURN(spPopupAsDO.As(&spThisPopup));

        IFC_RETURN(BackButtonIntegration_UnregisterListener(spThisPopup.Cast<Popup>()));
    }

    return S_OK;
}

_Check_return_ HRESULT Popup::OnPropertyChanged2(_In_ const PropertyChangedParams& args)
{
    IFC_RETURN(PopupGenerated::OnPropertyChanged2(args));

    switch (args.m_pDP->GetIndex())
    {
    case KnownPropertyIndex::Popup_IsOpen:
        if (args.m_pNewValue->AsBool()) //is now open
        {
            //set up back button support if necessary
            if (DXamlCore::GetCurrent()->GetHandle()->BackButtonSupported())
            {
                IFC_RETURN(BackButtonIntegration_RegisterListener(this));
            }

            //light-dismiss: register for XamlRoot.Changed events, first storing
            //the current content bounds to avoid immediately closing
            IFC_RETURN(CacheContentPositionInfo());
            IFC_RETURN(RegisterForXamlRootChangedEvents());
            IFC_RETURN(RegisterForWindowActivatedEvents());

            // If we haven't already attached event handlers for the placement target's LayoutUpdated event
            // and the child's Loaded and SizeChanged events, let's do so now.
            ctl::ComPtr<xaml::IFrameworkElement> placementTarget;
            IFC_RETURN(get_PlacementTarget(&placementTarget));

            if (placementTarget && m_placementTargetLayoutUpdatedToken.value == 0)
            {
                ctl::ComPtr<wf::IEventHandler<IInspectable*>> layoutUpdatedHandler;
                layoutUpdatedHandler.Attach(
                    new ClassMemberEventHandler<
                    Popup,
                    IPopup,
                    wf::IEventHandler<IInspectable*>,
                    IInspectable,
                    IInspectable>(this, &Popup::OnPlacementTargetLayoutUpdated, false /* subscribingToSelf */));

                IFC_RETURN(placementTarget.Cast<FrameworkElement>()->add_LayoutUpdated(layoutUpdatedHandler.Get(), &m_placementTargetLayoutUpdatedToken));
            }

            ctl::ComPtr<xaml::IUIElement> child;
            IFC_RETURN(get_Child(&child));

            if (child && m_childSizeChangedToken.value == 0)
            {
                ASSERT(m_childLoadedToken.value == 0);

                if (auto childAsFE = child.AsOrNull<FrameworkElement>())
                {
                    ctl::ComPtr<xaml::ISizeChangedEventHandler> sizeChangedHandler;
                    sizeChangedHandler.Attach(
                        new ClassMemberEventHandler<
                        Popup,
                        IPopup,
                        xaml::ISizeChangedEventHandler,
                        IInspectable,
                        xaml::ISizeChangedEventArgs>(this, &Popup::OnChildSizeChanged, false /* subscribingToSelf */));

                    IFC_RETURN(childAsFE->add_SizeChanged(sizeChangedHandler.Get(), &m_childSizeChangedToken));

                    ctl::ComPtr<xaml::IRoutedEventHandler> loadedHandler;
                    loadedHandler.Attach(
                        new ClassMemberEventHandler<
                        Popup,
                        xaml_primitives::IPopup,
                        xaml::IRoutedEventHandler,
                        IInspectable,
                        xaml::IRoutedEventArgs>(this, &Popup::OnChildLoaded, false /* subscribingToSelf */));

                    IFC_RETURN(childAsFE->add_Loaded(loadedHandler.Get(), &m_childLoadedToken));
                }
            }

            IFC_RETURN(SetPositionFromPlacement());
        }
        else //is now closed
        {
            IFC_RETURN(UnregisterForXamlRootChangedEvents());

            // We'll invalidate the calculated placement and justification when we close, since it may be different when we reopen.
            m_placementAndJustificationCalculated = false;
        }

        break;

    case KnownPropertyIndex::Popup_DesiredPlacement:
    case KnownPropertyIndex::Popup_PlacementTarget:
        {
            // Either of these invalidates the calculated placement and justification since we're intentionally moving the popup.
            m_placementAndJustificationCalculated = false;

            // If we're updating the placement target, we should attach a SizeChanged event handler
            // so we ensure we always snap to wherever the target's bounds are.
            if (args.m_pDP->GetIndex() == KnownPropertyIndex::Popup_PlacementTarget)
            {
                if (args.m_pOldValueOuterNoRef)
                {
                    ctl::ComPtr<FrameworkElement> oldPlacementTarget;
                    IFC_RETURN(ctl::do_query_interface(oldPlacementTarget, args.m_pOldValueOuterNoRef));

                    IFC_RETURN(oldPlacementTarget->remove_LayoutUpdated(m_placementTargetLayoutUpdatedToken));
                    m_placementTargetLayoutUpdatedToken.value = 0;
                }

                if (args.m_pNewValueOuterNoRef)
                {
                    ASSERT(m_placementTargetLayoutUpdatedToken.value == 0);

                    ctl::ComPtr<FrameworkElement> newPlacementTarget;
                    IFC_RETURN(ctl::do_query_interface(newPlacementTarget, args.m_pNewValueOuterNoRef));

                    ctl::ComPtr<wf::IEventHandler<IInspectable*>> layoutUpdatedHandler;
                    layoutUpdatedHandler.Attach(
                        new ClassMemberEventHandler<
                        Popup,
                        IPopup,
                        wf::IEventHandler<IInspectable*>,
                        IInspectable,
                        IInspectable>(this, &Popup::OnPlacementTargetLayoutUpdated, false /* subscribingToSelf */));

                    IFC_RETURN(newPlacementTarget->add_LayoutUpdated(layoutUpdatedHandler.Get(), &m_placementTargetLayoutUpdatedToken));
                }
            }

            // Either of these changing should cause us to reset the adjustments on the placement target's parent popup, if it has one.
            ctl::ComPtr<xaml::IFrameworkElement> placementTarget;
            IFC_RETURN(get_PlacementTarget(&placementTarget));

            // We won't have a chance to run layout again, so we'll save off the original adjustments and subtract them off in our positioning calculations.
            float xPlacementTargetAdjustment = 0.0f;
            float yPlacementTargetAdjustment = 0.0f;

            if (placementTarget)
            {
                if (auto placementTargetPopupCore = CPopup::GetClosestPopupAncestor(placementTarget.Cast<FrameworkElement>()->GetHandle()))
                {
                    xPlacementTargetAdjustment = placementTargetPopupCore->m_hAdjustment;
                    yPlacementTargetAdjustment = placementTargetPopupCore->m_vAdjustment;

                    placementTargetPopupCore->m_hAdjustment = 0.0f;
                    placementTargetPopupCore->m_vAdjustment = 0.0f;
                }
            }

            IFC_RETURN(SetPositionFromPlacement(xPlacementTargetAdjustment, yPlacementTargetAdjustment));
        }
        break;

    case KnownPropertyIndex::Popup_Child:
        // When the popup's child loads or is resized, we'll want to re-evaluate positioning,
        // so let's attach event handlers to do that.
        if (m_wrOldChild)
        {
            ctl::ComPtr<FrameworkElement> oldChild;

            // m_wrOldChild should only be set to a FrameworkElement, so we can just IFC_RETURN
            // instead of having an "if succeeded" check.
            IFC_RETURN(m_wrOldChild.As(&oldChild));

            if (oldChild)
            {
                IFC_RETURN(oldChild->remove_SizeChanged(m_childSizeChangedToken));
                IFC_RETURN(oldChild->remove_Loaded(m_childLoadedToken));
            }

            m_childSizeChangedToken.value = 0;
            m_childLoadedToken.value = 0;

            m_wrOldChild = nullptr;
        }

        if (args.m_pNewValueOuterNoRef)
        {
            ctl::ComPtr<FrameworkElement> newChild;

            if (SUCCEEDED(ctl::do_query_interface(newChild, args.m_pNewValueOuterNoRef)) && newChild)
            {
                ctl::ComPtr<xaml::ISizeChangedEventHandler> sizeChangedHandler;
                sizeChangedHandler.Attach(
                    new ClassMemberEventHandler<
                    Popup,
                    IPopup,
                    xaml::ISizeChangedEventHandler,
                    IInspectable,
                    xaml::ISizeChangedEventArgs>(this, &Popup::OnSizeChanged, false /* subscribingToSelf */));

                IFC_RETURN(newChild->add_SizeChanged(sizeChangedHandler.Get(), &m_childSizeChangedToken));

                ctl::ComPtr<xaml::IRoutedEventHandler> loadedHandler;
                loadedHandler.Attach(
                    new ClassMemberEventHandler<
                    Popup,
                    xaml_primitives::IPopup,
                    xaml::IRoutedEventHandler,
                    IInspectable,
                    xaml::IRoutedEventArgs>(this, &Popup::OnChildLoaded, false /* subscribingToSelf */));

                IFC_RETURN(newChild->add_Loaded(loadedHandler.Get(), &m_childLoadedToken));
                IFC_RETURN(newChild.AsWeak(&m_wrOldChild));
            }
        }

        IFC_RETURN(SetPositionFromPlacement());
        break;

    case KnownPropertyIndex::Popup_ShouldConstrainToRootBounds:
        {
            bool shouldConstrainToRootBounds = !!args.m_pNewValue->AsBool();
            CPopup *corePopup = static_cast<CPopup*>(GetHandle());

            // If the popup has already been unconstrained from root bounds, that can't be undone.
            if (corePopup->IsWindowed() && shouldConstrainToRootBounds)
            {
                IFC_RETURN(DirectUI::ErrorHelper::OriginateErrorUsingResourceID(E_FAIL, ERROR_POPUP_SHOULDCONSTRAINTOROOTBOUNDS_CANNOT_BE_CHANGED_AFTER_OPEN));
            }

            // To simplify implementation, ShouldConstrainToRootBounds needs to be set before the popup is ever opened.
            if (corePopup->WasEverOpened() && !shouldConstrainToRootBounds)
            {
                IFC_RETURN(DirectUI::ErrorHelper::OriginateErrorUsingResourceID(E_FAIL, ERROR_POPUP_SHOULDCONSTRAINTOROOTBOUNDS_CANNOT_BE_CHANGED_AFTER_OPEN));
            }
        }
        break;
    }

    return S_OK;
}

void Popup::NotifyOfDataContextChange(_In_ const DataContextChangedParams& args)
{
    HRESULT hr = S_OK;
    ctl::ComPtr<IUIElement> spChild;
    ctl::ComPtr<FrameworkElement> spFE;

    IFC(get_Child(&spChild));

    // Call up to base to continue the DataContext propagation
    PopupGenerated::NotifyOfDataContextChange(args);

    if (spChild != nullptr)
    {
        if (SUCCEEDED(spChild.As(&spFE)))
        {
            IFC(spFE->OnAncestorDataContextChanged(args));
        }
    }

Cleanup:
    return;
}

// Create PopupAutomationPeer to represent the Popup.
IFACEMETHODIMP Popup::OnCreateAutomationPeer(_Outptr_result_maybenull_ xaml_automation_peers::IAutomationPeer** ppAutomationPeer)
{
    HRESULT hr = S_OK;
    ctl::ComPtr<PopupAutomationPeer> spAutomationPeer;
    *ppAutomationPeer = nullptr;

    IFC(ActivationAPI::ActivateAutomationInstance(KnownTypeIndex::PopupAutomationPeer, GetHandle(), spAutomationPeer.GetAddressOf()));
    IFC(spAutomationPeer->put_Owner(this));
    *ppAutomationPeer = spAutomationPeer.Detach();

Cleanup:
    RRETURN(hr);
}

_Check_return_ HRESULT Popup::GetChildrenCount(
    _Out_ INT* pnCount)
{
    HRESULT hr = S_OK;
    ctl::ComPtr<IUIElement> spChild;

    IFC(get_Child(&spChild));

    *pnCount = spChild ? 1 :0;

Cleanup:
    RRETURN(hr);
}

_Check_return_ HRESULT Popup::GetChild(
    _In_ INT nChildIndex,
    _Outptr_ xaml::IDependencyObject** ppDO)
{
    HRESULT hr = S_OK;
    ctl::ComPtr<IUIElement> spChild;

    IFCPTR(ppDO);
    IFC(get_Child(&spChild));
    if (nChildIndex != 0 || !spChild)
    {
        IFC(E_INVALIDARG);
    }

    IFC(spChild.CopyTo(ppDO));

Cleanup:
    RRETURN(hr);
}

bool Popup::IsAssociatedWithXamlIsland() const
{
    CPopup* popup = static_cast<CPopup*>(GetHandle());
    if (popup != nullptr)
    {
        return (popup->GetAssociatedXamlIslandNoRef() != nullptr);
    }

    return false;
}

// Called on both activation and de-activation
_Check_return_
HRESULT
Popup::OnWindowActivated(
    _In_ IInspectable* sender,
    _In_ xaml::IWindowActivatedEventArgs* args)
{
    xaml::WindowActivationState state = xaml::WindowActivationState::WindowActivationState_CodeActivated;
    IFC_RETURN(args->get_WindowActivationState(&state));
    if (state == xaml::WindowActivationState::WindowActivationState_Deactivated)
    {
        IFC_RETURN(OnXamlLostFocus());
    }

    return S_OK;
}

// Common focus lost handler when host is an island or a CoreWindow.  In the  island case, this is called
// when the island's focus is lost.  In the CoreWindow case, this is called when CoreWindow's activation is lost.
_Check_return_
HRESULT
Popup::OnXamlLostFocus()
{
    ctl::ComPtr<IUIElement> spChild;
    IFC_RETURN(get_Child(&spChild));

    // We don't want to dismiss open light dismiss popups that do not have children. This is to keep compatability of Win 8
    // applications.
    BOOLEAN shouldDismiss = false;
    IFC_RETURN(ShouldDismiss(DismissalTriggerFlags::WindowDeactivated, &shouldDismiss));
    if (shouldDismiss && spChild)
    {
        // Dismiss the popup if window gets deactivated and the currently
        // active component is not within the popup.  If the popup is
        // associated with a flyout, we'll close that instead to allow
        // anything like closing animations and cleanup to execute first.
        if (auto flyout = CPopup::GetClosestFlyoutAncestor(GetHandle()))
        {
            IFC_RETURN(FlyoutBase::HideFlyout(flyout));
        }
        else
        {
            IFC_RETURN(CoreImports::Popup_Close(GetHandle()));
        }
    }

    return S_OK;
}


//-----------------------------------------------------------------------------
//
//  OnDisconnectVisualChildren
//
//  During a DisconnectVisualChildrenRecursive tree walk, clear Child property as well.
//
//-----------------------------------------------------------------------------


IFACEMETHODIMP
Popup::OnDisconnectVisualChildren()
{
    HRESULT hr = S_OK;

    IFC(ClearValue(
        MetadataAPI::GetDependencyPropertyByIndex(KnownPropertyIndex::Popup_Child)));

    IFC(PopupGenerated::OnDisconnectVisualChildren());

Cleanup:
    RRETURN(hr);
}

_Check_return_ HRESULT Popup::GetSavedFocusState(
    _Out_ xaml::FocusState* pFocusState)
{
    HRESULT hr = S_OK;
    FocusState focusStateNative = FocusState::Unfocused;

    *pFocusState = xaml::FocusState_Unfocused;
    IFC(CoreImports::Popup_GetSavedFocusState(GetHandle(), &focusStateNative));
    *pFocusState = static_cast<xaml::FocusState>(focusStateNative);

Cleanup:
    RRETURN(hr);
}

_Check_return_ HRESULT
Popup::GetNextTabStopOverride(
    _Outptr_ DependencyObject** ppNextTabStop)
{
    RRETURN(GetTabStop(TRUE/*isForward*/, ppNextTabStop));
}

_Check_return_ HRESULT
Popup::GetPreviousTabStopOverride(
    _Outptr_ DependencyObject** ppPreviousTabStop)
{
    RRETURN(GetTabStop(FALSE/*isForward*/, ppPreviousTabStop));
}

_Check_return_ HRESULT
Popup::GetTabStop(
    _In_ BOOLEAN isForward,
    _Outptr_ DependencyObject** ppTabStop)
{
    HRESULT hr = S_OK;
    bool isLightDismiss = false;
    bool canHaveFocusableChildren = false;
    bool isFocusable = false;
    BOOLEAN isTabStop = FALSE;
    ctl::ComPtr<IUIElement> spChild;
    ctl::ComPtr<xaml::IUIElement> spChildAsUIElement;
    CDependencyObject* pChildAsNativeDONoRef = NULL;
    CDependencyObject* pTabStopNativeDO = NULL;

    IFC(CoreImports::Popup_GetIsLightDismiss(GetHandle(), &isLightDismiss));
    if (isLightDismiss)
    {
        IFC(get_Child(&spChild));
        if (spChild.Get())
        {
            pChildAsNativeDONoRef = spChild.Cast<UIElement>()->GetHandle();
            if (pChildAsNativeDONoRef)
            {
                IFC(CoreImports::FocusManager_CanHaveFocusableChildren(
                    pChildAsNativeDONoRef,
                    &canHaveFocusableChildren));

                if (canHaveFocusableChildren)
                {
                    if (isForward)
                    {
                        IFC(CoreImports::FocusManager_GetFirstFocusableElement(pChildAsNativeDONoRef, &pTabStopNativeDO));
                    }
                    else
                    {
                        IFC(CoreImports::FocusManager_GetLastFocusableElement(pChildAsNativeDONoRef, &pTabStopNativeDO));
                    }
                }
                else
                {
                    spChildAsUIElement = spChild.AsOrNull<xaml::IUIElement>();
                    if (spChildAsUIElement.Get())
                    {
                        IFC(spChildAsUIElement.Get()->get_IsTabStop(&isTabStop));
                    }
                    if (isTabStop)
                    {
                        IFC(CoreImports::UIElement_IsFocusable(static_cast<CUIElement*>(pChildAsNativeDONoRef), &isFocusable));
                        if (isFocusable)
                        {
                            pTabStopNativeDO = pChildAsNativeDONoRef;
                        }
                    }
                }
            }
        }
    }

    if (pTabStopNativeDO)
    {
        IFC(DXamlCore::GetCurrent()->GetPeer(pTabStopNativeDO, ppTabStop));
    }
    else
    {
        IFC(DXamlCore::GetCurrent()->GetPeer(GetHandle(), ppTabStop));
    }

Cleanup:
    ReleaseInterface(pTabStopNativeDO);

    RRETURN(hr);
}

// Programmatic call to light-dismiss.
// If it was opened as a light-dismiss Popup, the Popup will close and restore
// focus to the control that had focus when the Popup was opened using focusStateAfterClosing.
// If it was opened as a normal Popup, the Popup will simply close.
_Check_return_ HRESULT
Popup::LightDismiss(
    _In_ xaml::FocusState focusStateAfterClosing)
{
    HRESULT hr = S_OK;

#if DBG
    BOOLEAN shouldDismiss;
    ASSERT(focusStateAfterClosing != xaml::FocusState_Unfocused);
    IFC(ShouldDismiss(DismissalTriggerFlags::CoreLightDismiss, &shouldDismiss));
    ASSERT(shouldDismiss);
#endif

    IFC(CoreImports::Popup_SetFocusStateAfterClosing(GetHandle(), static_cast<FocusState>(focusStateAfterClosing)));

    IFC(Close());

Cleanup:
    RRETURN(hr);
}

_Check_return_ HRESULT
Popup::OnXamlRootChanged(
    _In_ xaml::IXamlRoot* pSender,
    _In_ xaml::IXamlRootChangedEventArgs* pArgs)
{
    wf::Size currentXamlRootSize{};
    IFC_RETURN(pSender->get_Size(&currentXamlRootSize));
    if (currentXamlRootSize.Width != m_previousXamlRootWidth ||
        currentXamlRootSize.Height != m_previousXamlRootHeight)
    {
        BOOLEAN shouldDismiss;
        IFC_RETURN(ShouldDismiss(DismissalTriggerFlags::WindowSizeChange, &shouldDismiss));
        if (shouldDismiss)
        {
            IFC_RETURN(Close());
        }
    }

    m_previousXamlRootWidth = currentXamlRootSize.Width;
    m_previousXamlRootHeight = currentXamlRootSize.Height;

    return S_OK;
}

STDMETHODIMP Popup::OnCoreWindowPositionChanged()
{
    if (!IsAssociatedWithXamlIsland())
    {
        OnHostWindowPositionChangedImpl();
    }

    return S_OK;
}

/*static*/ void Popup::OnHostWindowPositionChanged(_In_ CDependencyObject* nativePopup)
{
    ctl::ComPtr<DependencyObject> popupAsDO;
    IFCFAILFAST(DXamlCore::GetCurrent()->GetPeer(nativePopup, &popupAsDO));

    ctl::ComPtr<DirectUI::Popup> popup;
    IFCFAILFAST(popupAsDO.As(&popup));

    popup->OnHostWindowPositionChangedImpl();
}

void Popup::OnHostWindowPositionChangedImpl()
{
    CPopup *pPopup = static_cast<CPopup*>(GetHandle());
    if (pPopup && pPopup->IsWindowed())
    {
        auto toolTipOwner = m_wrOwner.AsOrNull<xaml_controls::IToolTip>();

        if (toolTipOwner && !toolTipOwner.Cast<ToolTip>()->IsOpenAsAutomaticToolTip())
        {
            toolTipOwner.Cast<ToolTip>()->RepositionPopup();
        }
        else
        {
            bool shouldClose = true;
            Window* window = nullptr;
            IFCFAILFAST(DXamlCore::GetCurrent()->GetAssociatedWindowNoRef(this, &window));
            if (window && window->HasBounds())
            {
                shouldClose = false;
                wf::Rect currentWindowBounds = { };
                IFCFAILFAST(DXamlCore::GetCurrent()->GetVisibleContentBoundsForElement(false, true, GetHandle(), &currentWindowBounds));
                if (currentWindowBounds.X != m_previousWindowX ||
                    currentWindowBounds.Y != m_previousWindowY)
                {
                    shouldClose = true;
                    m_previousWindowX = currentWindowBounds.X;
                    m_previousWindowY = currentWindowBounds.Y;
                }
            }

            if (shouldClose)
            {
                // Keep CPopup alive to prevent CPopup::Close from crashing after releasing itself.
                // This is a temporary fix to reduce risk near ship date. Ideally CPopupRoot::RemoveFromOpenPopupList
                // should call UnpegManagedPeer before AsyncRelease, so CPopup is released on the next tick.
                xref_ptr<CPopup> spPopup(pPopup);

                IFCFAILFAST(Close());
            }
        }
    }
}

/*static*/ void Popup::OnIslandLostFocus(_In_ CDependencyObject* nativePopup)
{
    ctl::ComPtr<DependencyObject> popupAsDO;
    if (SUCCEEDED(DXamlCore::GetCurrent()->GetPeer(nativePopup, &popupAsDO)))
    {
        ctl::ComPtr<DirectUI::Popup> popup;
        IFCFAILFAST(popupAsDO.As(&popup));

        popup->OnIslandLostFocusImpl();
    }
}

void Popup::OnIslandLostFocusImpl()
{
    IGNOREHR(OnXamlLostFocus());
}

_Check_return_ HRESULT
Popup::SetDismissalTriggerFlags(_In_ UINT32 flags)
{
    HRESULT hr = S_OK;

    if (flags != m_dismissalTriggerFlags)
    {
        BOOLEAN isLightDismissEnabled = FALSE;

        IFC(get_IsLightDismissEnabled(&isLightDismissEnabled));

        if ((DismissalTriggerFlags::CoreLightDismiss & flags) && !isLightDismissEnabled)
        {
            IFC(put_IsLightDismissEnabled(TRUE));
        }
        else if (!(DismissalTriggerFlags::CoreLightDismiss & flags) && isLightDismissEnabled)
        {
            IFC(put_IsLightDismissEnabled(FALSE));
        }

        m_dismissalTriggerFlags = flags;
    }

Cleanup:
    RRETURN(hr);

}

_Check_return_ HRESULT Popup::Close()
{
    CValue valueFalse;
    valueFalse.SetBool(false);

    //  Don't use put_IsOpen(FALSE) to close the popup, because app may have
    //  set IsOpen to a binding, and put_IsOpen will clear that binding. Instead,
    //  set the core property directly.
    IFC_RETURN(SetValueCore(
        SetValueParams(
            MetadataAPI::GetDependencyPropertyByIndex(KnownPropertyIndex::Popup_IsOpen),
            valueFalse),
        /* fSetEffectiveValueOnly */ true));

    return S_OK;
}

// Based on a combination of any special lightDismissFlags and the state of the core IsLightDismissEnabled property
// determine whether the supplied reason should result in dismissing the popup
_Check_return_ HRESULT Popup::ShouldDismiss(
    _In_ DismissalTriggerFlags reason,
    _Out_ BOOLEAN* returnValue)
{
    *returnValue = false;

    // No special flags have been set. Defer to IsLightDismissEnabled
    if (m_dismissalTriggerFlags == DismissalTriggerFlags::None)
    {
        IFC_RETURN(get_IsLightDismissEnabled(returnValue));
    }
    else
    {
        *returnValue = ((reason & m_dismissalTriggerFlags) != 0);
    }

    // During a drag and drop operation, the current window might lose
    // focus. If that happens, we don't want to close the popup because it could
    // be the source or the target of the dnd operation.
    if (*returnValue && reason == DismissalTriggerFlags::WindowDeactivated)
    {
        *returnValue = !DXamlCore::IsWinRTDndOperationInProgress();
    }

    return S_OK;
}

_Check_return_ HRESULT Popup::RegisterForXamlRootChangedEvents()
{
    auto xamlRoot = XamlRoot::GetForElementStatic(this);
    if (!m_xamlRootChangedEventHandler && xamlRoot)
    {
        ctl::WeakRefPtr weakInstance;
        IFC_RETURN(ctl::AsWeak(this, &weakInstance));
        IFC_RETURN(m_xamlRootChangedEventHandler.AttachEventHandler(
            xamlRoot.Get(),
            [weakInstance](xaml::IXamlRoot *sender, xaml::IXamlRootChangedEventArgs *args) mutable
            {
                ctl::ComPtr<IPopup> instance;
                instance = weakInstance.AsOrNull<IPopup>();

                if (instance)
                {
                    IFC_RETURN(instance.Cast<Popup>()->OnXamlRootChanged(sender, args));
                }

                return S_OK;
            })
        );
    }

    return S_OK;
}

_Check_return_ HRESULT Popup::UnregisterForXamlRootChangedEvents()
{
    // this can be called as part of destructors / unload- avoid creating a new XamlRoot
    auto xamlRoot = XamlRoot::GetForElementStatic(this, false /*createIfNotExist*/);
    if (m_xamlRootChangedEventHandler && xamlRoot)
    {
        IGNOREHR(m_xamlRootChangedEventHandler.DetachEventHandler(xamlRoot.Get()));
    }

    return S_OK;
}

_Check_return_ HRESULT Popup::RegisterForWindowActivatedEvents()
{
    if (IsAssociatedWithXamlIsland())
    {
        // If the popup is associated with an island, we don't need to attach to Window activation.
        // This is because the only action taken by a window (de)activation state change is to dismiss
        // the popup via OnXamlLostFocus. The island codepath will call OnXamlLostFocus for us.
        return S_OK;
    }

    // Determine the window that this UIElement belongs to and subscribe to Activated events if valid
    Window* window = nullptr;
    IFC_RETURN(DXamlCore::GetCurrent()->GetAssociatedWindowNoRef(this, &window));
    if (!m_windowActivatedHandler && window)
    {
        ctl::WeakRefPtr weakThis;
        IFC_RETURN(ctl::AsWeak(this, &weakThis));
        IFC_RETURN(m_windowActivatedHandler.AttachEventHandler(
            window,
            [weakThis](IInspectable *sender, xaml::IWindowActivatedEventArgs *args) mutable
            {
                ctl::ComPtr<IPopup> strongThis;
                strongThis = weakThis.AsOrNull<IPopup>();

                if (strongThis)
                {
                    IFC_RETURN(strongThis.Cast<Popup>()->OnWindowActivated(sender, args));
                }

                return S_OK;
            })
        );
    }

    return S_OK;
}

_Check_return_ HRESULT Popup::UnregisterForWindowActivatedEvents()
{
    Window* window = nullptr;
    IFC_RETURN(DXamlCore::GetCurrent()->GetAssociatedWindowNoRef(this, &window));
    if (m_windowActivatedHandler && window)
    {
        IGNOREHR(m_windowActivatedHandler.DetachEventHandler(ctl::iinspectable_cast(window)));
    }

    return S_OK;
}

_Check_return_ HRESULT Popup::SetDefaultAutomationName(
    _In_ HSTRING automationName)
{
    return WindowsDuplicateString(automationName, &m_defaultAutomationName);
}

_Check_return_ HRESULT Popup::GetPlainText(
    _Out_ HSTRING* strPlainText)
{
    *strPlainText = nullptr;

    if (m_defaultAutomationName)
    {
        IFC_RETURN(WindowsDuplicateString(m_defaultAutomationName, strPlainText));
    }

    return S_OK;
}

_Check_return_ HRESULT Popup::TruncateAutomationName(
    _Inout_opt_ HSTRING *stringToTruncate)
{
    if (*stringToTruncate == nullptr)
    {
        return S_OK;
    }

    HSTRING originalString = *stringToTruncate;
    UINT32 sourceStringLength;
    LPCWSTR sourceStringRawBuffer = WindowsGetStringRawBuffer(*stringToTruncate, &sourceStringLength);

    // First we'll figure out whether we need to truncate at all.
    // We'll iterate through the raw buffer and verify if we have
    // a newline character, or more than twenty words.
    int wordCount = 0;
    bool isInSpace = true;
    bool shouldTruncate = false;
    UINT32 truncatePosition = 0;
    UINT32 lastWordEndPosition = 0;

    for (UINT32 i = 0; i < sourceStringLength; i++)
    {
        if (sourceStringRawBuffer[i] == '\n' || sourceStringRawBuffer[i] == '\r')
        {
            shouldTruncate = true;
            truncatePosition = i;
            break;
        }

        if (isInSpace && !isspace(sourceStringRawBuffer[i]))
        {
            isInSpace = false;
            wordCount++;

            if (wordCount > 20)
            {
                shouldTruncate = true;
                truncatePosition = lastWordEndPosition;
                break;
            }
        }
        else if (!isInSpace && isspace(sourceStringRawBuffer[i]))
        {
            isInSpace = true;
            lastWordEndPosition = i;
        }
    }

    if (shouldTruncate)
    {
        wrl_wrappers::HString truncatedString;
        wrl_wrappers::HString ellipsisString;

        truncatedString.Set(sourceStringRawBuffer, sourceStringLength);
        truncatedString.Substring(0, truncatePosition, truncatedString);
        ellipsisString.Set(STR_LEN_PAIR(L"..."));
        truncatedString.Concat(ellipsisString, truncatedString);

        truncatedString.CopyTo(stringToTruncate);
    }
    else
    {
        // If we don't need to truncate the string, then we'll just copy it.
        IFC_RETURN(WindowsDuplicateString(originalString, stringToTruncate));
    }

    // Now we can delete the original string.
    IFC_RETURN(WindowsDeleteString(originalString));

    return S_OK;
}

_Check_return_ HRESULT Popup::GetShouldUIAPeerExposeWindowPattern(
    _Out_ bool *shouldExposeWindowPattern)
{
    BOOLEAN isLightDismissEnabled = FALSE;
    BOOLEAN isContentDialog = FALSE;
    BOOLEAN isSubMenu = FALSE;

    *shouldExposeWindowPattern = false;

    IFC_RETURN(get_IsLightDismissEnabled(&isLightDismissEnabled));
    IFC_RETURN(get_IsContentDialog(&isContentDialog));
    IFC_RETURN(get_IsSubMenu(&isSubMenu));

    *shouldExposeWindowPattern = isLightDismissEnabled || isContentDialog || isSubMenu;
    return S_OK;
}

_Check_return_ HRESULT Popup::SetOwner(_In_opt_ xaml::IDependencyObject* const owner)
{
    m_wrOwner = nullptr;

    if (owner)
    {
        IFC_RETURN(ctl::AsWeak(owner, &m_wrOwner));
    }

    return S_OK;
}

bool Popup::IsWindowed()
{
    CPopup *corePopup = static_cast<CPopup*>(GetHandle());
    return !!corePopup->IsWindowed();
}

_Check_return_ HRESULT Popup::get_IsConstrainedToRootBoundsImpl(_Out_ BOOLEAN* pValue)
{
    // If we haven't been windowed yet, then we won't know for sure whether we'll constrain ourselves to root bounds.
    // However, we want apps to be able to rely on the value of this property at any time, so in those cases,
    // we'll predict whether we will be constrained on the basis of whether ShouldConstrainToRootBounds
    // is set to false and of whether the current platform supports windowed popups.
    // If both of those are the case, then we'll return false, as we know that the popup won't be constrained
    // to root bounds if nothing changes.
    if (IsWindowed())
    {
        *pValue = FALSE;
    }
    else
    {
        BOOLEAN shouldConstrainToRootBounds;
        IFC_RETURN(get_ShouldConstrainToRootBounds(&shouldConstrainToRootBounds));

        *pValue = !CPopup::DoesPlatformSupportWindowedPopup(DXamlCore::GetCurrent()->GetHandle()) ||
            shouldConstrainToRootBounds;
    }

    return S_OK;
}

_Check_return_ HRESULT Popup::QueryInterfaceImpl(_In_ REFIID iid, _Outptr_ void** ppObject)
{
    if (InlineIsEqualGUID(iid, __uuidof(ABI::Microsoft::Internal::FrameworkUdk::ICoreWindowPositionChangedListener)))
    {
        *ppObject = static_cast<ABI::Microsoft::Internal::FrameworkUdk::ICoreWindowPositionChangedListener*>(this);
    }
    else
    {
        return PopupGenerated::QueryInterfaceImpl(iid, ppObject);
    }

    AddRefOuter();
    return S_OK;
}

_Check_return_ HRESULT Popup::SetPositionFromPlacement(float xPlacementTargetAdjustment, float yPlacementTargetAdjustment)
{
    CPopup* corePopup = static_cast<CPopup*>(GetHandle());
    XFLOAT originalCalculatedHOffset = corePopup->m_calculatedHOffset;
    XFLOAT originalCalculatedVOffset = corePopup->m_calculatedVOffset;

    BOOLEAN isOpen;
    ctl::ComPtr<xaml::IUIElement> child;
    ctl::ComPtr<xaml::IFrameworkElement> placementTarget;
    xaml_primitives::PopupPlacementMode desiredPlacement;

    IFC_RETURN(get_IsOpen(&isOpen));
    IFC_RETURN(get_Child(&child));
    IFC_RETURN(get_PlacementTarget(&placementTarget));
    IFC_RETURN(get_DesiredPlacement(&desiredPlacement));

    CPopup* placementTargetPopupCore = nullptr;
    bool placementTargetPopupMoved = false;
    std::vector<std::wstring> warningInfo;

    if (placementTarget)
    {
        placementTargetPopupCore = CPopup::GetClosestPopupAncestor(placementTarget.Cast<FrameworkElement>()->GetHandle());
    }

    if (corePopup->StoreLayoutCycleWarningContexts())
    {
        // The layout cycle iteration count has almost reached its 250 limit. Store some Popup internal fields in the warningInfo array
        // at this beginning of the method, and then again at the end of the method below to see the potential deltas in the crash dump.
        GetLayoutCycleWarningContext(corePopup, placementTargetPopupCore, L"Entry", warningInfo);
    }

    xaml_primitives::PopupPlacementMode actualPlacement = desiredPlacement;

    // If we aren't open or have no child, then we have no position to set.  Otherwise, if both
    // PlacementTarget and Placement have been set, then we need to calculate
    // the popup's position from those values.
    if (isOpen && child && placementTarget && desiredPlacement != xaml_primitives::PopupPlacementMode_Auto)
    {
        // Since we'll need to know momentarily whether or not the popup is windowed,
        // let's make it windowed now if it's going to be.
        IFC_RETURN(corePopup->SetIsWindowedIfNeeded());

        // This isn't a flyout if the above values are set, but we can reuse the same logic
        // to get the available window rect.
        wf::Rect availableRect{};
        IFC_RETURN(FlyoutBase::CalculateAvailableWindowRect(
            this,
            placementTarget.Cast<FrameworkElement>(),
            false /* hasTargetPosition */,
            wf::Point{},
            false /* isFull */,
            &availableRect));

        xaml::FlowDirection flowDirection;
        IFC_RETURN(get_FlowDirection(&flowDirection));

        wfn::Vector2 childSizeVector;
        IFC_RETURN(child.Cast<UIElement>()->get_ActualSize(&childSizeVector));

        wf::Size childSize = { childSizeVector.X, childSizeVector.Y };

        // If the child hasn't been sized yet, there's nothing for us to do.
        if (childSize.Width > 0 && childSize.Height > 0)
        {
            wf::Point position = { 0, 0 };

            XRECTF_RB targetBoundsCore;
            IFC_RETURN(static_cast<CFrameworkElement*>(placementTarget.Cast<FrameworkElement>()->GetHandle())->GetGlobalBoundsLogical(&targetBoundsCore, TRUE /* ignoreClipping */));

            wf::Rect targetBounds =
            {
                targetBoundsCore.left,
                targetBoundsCore.top,
                targetBoundsCore.right - targetBoundsCore.left,
                targetBoundsCore.bottom - targetBoundsCore.top
            };

            targetBounds.X -= xPlacementTargetAdjustment;
            targetBounds.Y -= yPlacementTargetAdjustment;

            wf::Point originalPosition;
            bool tryMovingParentPlacementPopup = false;

            Popup::MajorPlacementMode majorPlacement;
            Popup::PreferredJustification justification;

            if (m_placementAndJustificationCalculated)
            {
                majorPlacement = m_calculatedMajorPlacement;
                justification = m_calculatedJustification;

                SetPositionFromMajorPlacementAndJustification(
                    &position,
                    childSize,
                    targetBounds,
                    flowDirection,
                    m_calculatedMajorPlacement,
                    m_calculatedJustification);

                originalPosition = position;

                // We'll check whether we're out of bounds.  We don't actually want to update the placement and justification,
                // so we'll save those values off.
                Popup::MajorPlacementMode dummyPlacement = majorPlacement;
                Popup::PreferredJustification dummyJustification = justification;
                bool valueChanged = false;

                FlipMajorPlacementAndJustificationIfOutOfBounds(
                    &dummyPlacement,
                    &dummyJustification,
                    position,
                    childSize,
                    availableRect,
                    &valueChanged);

                // If so, then if the placement target is itself in a popup that's placed at an absolute position,
                // we'll try moving that popup to bring this popup positioned relative to it into view.
                tryMovingParentPlacementPopup = valueChanged;
            }
            else
            {
                majorPlacement = GetMajorPlacementFromPlacement(desiredPlacement, flowDirection);
                justification = GetJustificationFromPlacement(desiredPlacement, flowDirection);

                // Ensure that the major placement mode and the justification are on orthogonal axes.
                switch (majorPlacement)
                {
                case Popup::MajorPlacementMode::Top:
                case Popup::MajorPlacementMode::Bottom:
                    ASSERT(justification == Popup::PreferredJustification::HorizontalCenter ||
                        justification == Popup::PreferredJustification::Left ||
                        justification == Popup::PreferredJustification::Right);
                    break;
                case Popup::MajorPlacementMode::Left:
                case Popup::MajorPlacementMode::Right:
                    ASSERT(justification == Popup::PreferredJustification::VerticalCenter ||
                        justification == Popup::PreferredJustification::Top ||
                        justification == Popup::PreferredJustification::Bottom);
                    break;
                }

                SetPositionFromMajorPlacementAndJustification(
                    &position,
                    childSize,
                    targetBounds,
                    flowDirection,
                    majorPlacement,
                    justification);

                originalPosition = position;

                // If the popup is out of bounds in the x- or y-direction, change which side we're showing it on
                // to put it back into bounds.
                bool valueChanged = false;

                FlipMajorPlacementAndJustificationIfOutOfBounds(
                    &majorPlacement,
                    &justification,
                    position,
                    childSize,
                    availableRect,
                    &valueChanged);

                if (valueChanged)
                {
                    SetPositionFromMajorPlacementAndJustification(
                        &position,
                        childSize,
                        targetBounds,
                        flowDirection,
                        majorPlacement,
                        justification);

                    // Now we'll check whether we're still out of bounds after flipping sides.
                    FlipMajorPlacementAndJustificationIfOutOfBounds(
                        &majorPlacement,
                        &justification,
                        position,
                        childSize,
                        availableRect,
                        &valueChanged);

                    // If so, we'll try one last thing - if the placement target is itself in a popup that's placed at an
                    // absolute position, we'll try moving that popup to bring this popup positioned relative to it into view.
                    tryMovingParentPlacementPopup = valueChanged;
                }

                m_calculatedMajorPlacement = majorPlacement;
                m_calculatedJustification = justification;

                m_placementAndJustificationCalculated = true;
            }

            actualPlacement = GetPlacementFromMajorPlacementAndJustification(majorPlacement, justification, flowDirection);

            if (tryMovingParentPlacementPopup)
            {
                if (placementTargetPopupCore)
                {
                    ctl::ComPtr<DependencyObject> placementTargetPopupAsDO;
                    IFC_RETURN(DXamlCore::GetCurrent()->GetPeer(placementTargetPopupCore, &placementTargetPopupAsDO));

                    ctl::ComPtr<Popup> placementTargetPopup;
                    IFC_RETURN(placementTargetPopupAsDO.As(&placementTargetPopup));

                    BOOLEAN placementTargetPopupIsOpen;
                    xaml_primitives::PopupPlacementMode placementTargetPopupDesiredPlacement;

                    IFC_RETURN(placementTargetPopup->get_IsOpen(&placementTargetPopupIsOpen));
                    IFC_RETURN(placementTargetPopup->get_DesiredPlacement(&placementTargetPopupDesiredPlacement));

                    if (placementTargetPopupIsOpen && placementTargetPopupDesiredPlacement == xaml_primitives::PopupPlacementMode_Auto)
                    {
                        float xAdjustment = 0.0f;
                        float yAdjustment = 0.0f;

                        if (majorPlacement == Popup::MajorPlacementMode::Left ||
                            majorPlacement == Popup::MajorPlacementMode::Right)
                        {
                            if (originalPosition.X < availableRect.X)
                            {
                                xAdjustment = availableRect.X - originalPosition.X;
                            }
                            else if (originalPosition.X + childSize.Width > availableRect.X + availableRect.Width)
                            {
                                xAdjustment = availableRect.X + availableRect.Width - (originalPosition.X + childSize.Width);
                            }
                        }

                        if (majorPlacement == Popup::MajorPlacementMode::Top ||
                            majorPlacement == Popup::MajorPlacementMode::Bottom)
                        {
                            if (originalPosition.Y < availableRect.Y)
                            {
                                yAdjustment = availableRect.Y - originalPosition.Y;
                            }
                            else if (originalPosition.Y + childSize.Height > availableRect.Y + availableRect.Height)
                            {
                                yAdjustment = availableRect.Y + availableRect.Height - (originalPosition.Y + childSize.Height);
                            }
                        }

                        const float epsilon = 0.1f;

                        if (std::abs(xAdjustment) > epsilon)
                        {
                            placementTargetPopupCore->m_hAdjustment += xAdjustment;
                            targetBounds.X += xAdjustment;
                            placementTargetPopupMoved = true;
                        }

                        if (std::abs(yAdjustment) > epsilon)
                        {
                            placementTargetPopupCore->m_vAdjustment += yAdjustment;
                            targetBounds.Y += yAdjustment;
                            placementTargetPopupMoved = true;
                        }
                    }
                }

                SetPositionFromMajorPlacementAndJustification(
                    &position,
                    childSize,
                    targetBounds,
                    flowDirection,
                    majorPlacement,
                    justification);
            }

            // We need to subtract off the layout offset between this element and the root,
            // since the placement position should override the layout position.
            ctl::ComPtr<xaml_media::IGeneralTransform> transformToRoot;
            wf::Point topLeftCorner;

            IFC_RETURN(TransformToVisual(nullptr, &transformToRoot));
            IFC_RETURN(transformToRoot->TransformPoint({ 0, 0 }, &topLeftCorner));

            corePopup->m_calculatedHOffset = position.X - topLeftCorner.X;
            corePopup->m_calculatedVOffset = position.Y - topLeftCorner.Y;
        }
        else
        {
            corePopup->m_calculatedHOffset = 0.0f;
            corePopup->m_calculatedVOffset = 0.0f;

            if (placementTargetPopupCore)
            {
                placementTargetPopupCore->m_hAdjustment = 0.0f;
                placementTargetPopupCore->m_vAdjustment = 0.0f;
            }
        }
    }
    else
    {
        // If we aren't open or aren't calculating the popup position from its placement and target,
        // then we'll revert these values back to zero to allow normal layout to resume.
        corePopup->m_calculatedHOffset = 0.0f;
        corePopup->m_calculatedVOffset = 0.0f;

        if (placementTargetPopupCore)
        {
            placementTargetPopupCore->m_hAdjustment = 0.0f;
            placementTargetPopupCore->m_vAdjustment = 0.0f;
        }
    }

    // If we're open and the placement we arrived upon was different than the previously used one,
    // then we'll report to the app the fact that it changed.
    if (isOpen)
    {
        xaml_primitives::PopupPlacementMode currentActualPlacement;
        IFC_RETURN(GetValueByKnownIndex(KnownPropertyIndex::Popup_ActualPlacement, &currentActualPlacement));

        if (actualPlacement != currentActualPlacement)
        {
            IFC_RETURN(SetValueByKnownIndex(KnownPropertyIndex::Popup_ActualPlacement, actualPlacement));

            if (ShouldRaiseEvent(static_cast<KnownEventIndex>(KnownEventIndex::Popup_ActualPlacementChanged)))
            {
                ctl::ComPtr<EventArgs> eventArgs;
                IFC_RETURN(ctl::make(&eventArgs));

                ActualPlacementChangedEventSourceType* eventSource;
                IFC_RETURN(PopupGenerated::GetActualPlacementChangedEventSourceNoRef(&eventSource));

                IFC_RETURN(eventSource->Raise(ctl::as_iinspectable(this), ctl::as_iinspectable(eventArgs.Get())));
            }
        }
    }

    CPopupRoot* popupRoot = nullptr;

    // If we updated the popup's position, then we need to invalidate arrange for the popup root
    // to have it pick that up.
    if (!DoubleUtil::AreClose(corePopup->m_calculatedHOffset, originalCalculatedHOffset) ||
        !DoubleUtil::AreClose(corePopup->m_calculatedVOffset, originalCalculatedVOffset) ||
        placementTargetPopupMoved)
    {
        IFC_RETURN(corePopup->GetContext()->GetAdjustedPopupRootForElement(corePopup, &popupRoot));
        if (popupRoot)
        {
            popupRoot->InvalidateArrange();
        }
    }

    if (corePopup->StoreLayoutCycleWarningContexts())
    {
        // Append the warningInfo array with the internal fields on method exit, indicating whether an InvalidateArrange call was performed or not.
        GetLayoutCycleWarningContext(corePopup, placementTargetPopupCore, popupRoot ? L"ExitWithInvalidateArrange" : L"Exit", warningInfo);

        // Record the current stack trace and WarningContext with the internal fields at method entry and exit. All this data will be exposed in a
        // stowed exception in the imminent crash dump.
        corePopup->StoreLayoutCycleWarningContext(warningInfo);
    }

    return S_OK;
}

/* static */ void Popup::SetPositionFromMajorPlacementAndJustification(
    _Inout_ wf::Point* position,
    wf::Size const& childSize,
    wf::Rect const& targetBounds,
    xaml::FlowDirection flowDirection,
    Popup::MajorPlacementMode majorPlacement,
    Popup::PreferredJustification justification)
{
    switch (majorPlacement)
    {
    case Popup::MajorPlacementMode::Top:
        position->Y = targetBounds.Y - childSize.Height;
        break;
    case Popup::MajorPlacementMode::Bottom:
        position->Y = targetBounds.Y + targetBounds.Height;
        break;
    case Popup::MajorPlacementMode::Left:
        position->X = targetBounds.X - childSize.Width;
        break;
    case Popup::MajorPlacementMode::Right:
        position->X = targetBounds.X + targetBounds.Width;
        break;
    }

    switch (justification)
    {
    case Popup::PreferredJustification::HorizontalCenter:
        position->X = targetBounds.X + targetBounds.Width / 2 - childSize.Width / 2;
        break;
    case Popup::PreferredJustification::Left:
        position->X = targetBounds.X;
        break;
    case Popup::PreferredJustification::Right:
        position->X = targetBounds.X + targetBounds.Width - childSize.Width;
        break;
    case Popup::PreferredJustification::VerticalCenter:
        position->Y = targetBounds.Y + targetBounds.Height / 2 - childSize.Height / 2;
        break;
    case Popup::PreferredJustification::Top:
        position->Y = targetBounds.Y;
        break;
    case Popup::PreferredJustification::Bottom:
        position->Y = targetBounds.Y + targetBounds.Height - childSize.Height;
        break;
    }

    // If the flow direction is RTL, then we need the position to be the top-right corner of the popup,
    // rather than the top-left corner.
    if (flowDirection == xaml::FlowDirection_RightToLeft)
    {
        position->X += childSize.Width;
    }
}

/* static */ void Popup::FlipMajorPlacementAndJustificationIfOutOfBounds(
    _Inout_ Popup::MajorPlacementMode* majorPlacement,
    _Inout_ Popup::PreferredJustification* justification,
    wf::Point const& position,
    wf::Size const& childSize,
    wf::Rect const& availableRect,
    _Out_ bool* valueChanged)
{
    *valueChanged = false;

    if (position.X + childSize.Width > availableRect.X + availableRect.Width)
    {
        // If we were positioned on the right, we're now positioned on the left.
        if (*majorPlacement == Popup::MajorPlacementMode::Right)
        {
            *majorPlacement = Popup::MajorPlacementMode::Left;
            *valueChanged = true;
        }

        // If we were justified left, we're now justified right.
        if (*justification == Popup::PreferredJustification::Left)
        {
            *justification = Popup::PreferredJustification::Right;
            *valueChanged = true;
        }
    }
    else if (position.X < availableRect.X)
    {
        // If we were positioned on the left, we're now positioned on the right.
        if (*majorPlacement == Popup::MajorPlacementMode::Left)
        {
            *majorPlacement = Popup::MajorPlacementMode::Right;
            *valueChanged = true;
        }

        // If we were justified right, we're now justified left.
        if (*justification == Popup::PreferredJustification::Right)
        {
            *justification = Popup::PreferredJustification::Left;
            *valueChanged = true;
        }
    }

    if (position.Y < availableRect.Y)
    {
        // If we were positioned on the top, we're now positioned on the bottom.
        if (*majorPlacement == Popup::MajorPlacementMode::Top)
        {
            *majorPlacement = Popup::MajorPlacementMode::Bottom;
            *valueChanged = true;
        }

        // If we were justified bottom, we're now justified top.
        if (*justification == Popup::PreferredJustification::Bottom)
        {
            *justification = Popup::PreferredJustification::Top;
            *valueChanged = true;
        }
    }
    else if (position.Y + childSize.Height > availableRect.Y + availableRect.Height)
    {
        // If we were positioned on the bottom, we're now positioned on the top.
        if (*majorPlacement == Popup::MajorPlacementMode::Bottom)
        {
            *majorPlacement = Popup::MajorPlacementMode::Top;
            *valueChanged = true;
        }

        // If we were justified top, we're now justified bottom.
        if (*justification == Popup::PreferredJustification::Top)
        {
            *justification = Popup::PreferredJustification::Bottom;
            *valueChanged = true;
        }
    }
}

/*static */ Popup::MajorPlacementMode Popup::GetMajorPlacementFromPlacement(
    xaml_primitives::PopupPlacementMode placement,
    xaml::FlowDirection flowDirection)
{
    switch (placement)
    {
    case xaml_primitives::PopupPlacementMode_Top:
    case xaml_primitives::PopupPlacementMode_TopEdgeAlignedLeft:
    case xaml_primitives::PopupPlacementMode_TopEdgeAlignedRight:
        return Popup::MajorPlacementMode::Top;
    case xaml_primitives::PopupPlacementMode_Bottom:
    case xaml_primitives::PopupPlacementMode_BottomEdgeAlignedLeft:
    case xaml_primitives::PopupPlacementMode_BottomEdgeAlignedRight:
        return Popup::MajorPlacementMode::Bottom;
    case xaml_primitives::PopupPlacementMode_Left:
    case xaml_primitives::PopupPlacementMode_LeftEdgeAlignedTop:
    case xaml_primitives::PopupPlacementMode_LeftEdgeAlignedBottom:
        if (flowDirection == xaml::FlowDirection_LeftToRight)
        {
            return Popup::MajorPlacementMode::Left;
        }
        else
        {
            return Popup::MajorPlacementMode::Right;
        }
    case xaml_primitives::PopupPlacementMode_Right:
    case xaml_primitives::PopupPlacementMode_RightEdgeAlignedTop:
    case xaml_primitives::PopupPlacementMode_RightEdgeAlignedBottom:
        if (flowDirection == xaml::FlowDirection_LeftToRight)
        {
            return Popup::MajorPlacementMode::Right;
        }
        else
        {
            return Popup::MajorPlacementMode::Left;
        }
    default:
        ASSERT(FALSE, L"Unsupported PopupPlacementMode");
        return Popup::MajorPlacementMode::Auto;
    }
}

/* static */ Popup::PreferredJustification Popup::GetJustificationFromPlacement(
    xaml_primitives::PopupPlacementMode placement,
    xaml::FlowDirection flowDirection)
{
    switch (placement)
    {
    case xaml_primitives::PopupPlacementMode_Top:
    case xaml_primitives::PopupPlacementMode_Bottom:
        return Popup::PreferredJustification::HorizontalCenter;
    case xaml_primitives::PopupPlacementMode_Left:
    case xaml_primitives::PopupPlacementMode_Right:
        return Popup::PreferredJustification::VerticalCenter;
    case xaml_primitives::PopupPlacementMode_TopEdgeAlignedLeft:
    case xaml_primitives::PopupPlacementMode_BottomEdgeAlignedLeft:
        if (flowDirection == xaml::FlowDirection_LeftToRight)
        {
            return Popup::PreferredJustification::Left;
        }
        else
        {
            return Popup::PreferredJustification::Right;
        }
    case xaml_primitives::PopupPlacementMode_TopEdgeAlignedRight:
    case xaml_primitives::PopupPlacementMode_BottomEdgeAlignedRight:
        if (flowDirection == xaml::FlowDirection_LeftToRight)
        {
            return Popup::PreferredJustification::Right;
        }
        else
        {
            return Popup::PreferredJustification::Left;
        }
    case xaml_primitives::PopupPlacementMode_LeftEdgeAlignedTop:
    case xaml_primitives::PopupPlacementMode_RightEdgeAlignedTop:
        return Popup::PreferredJustification::Top;
    case xaml_primitives::PopupPlacementMode_LeftEdgeAlignedBottom:
    case xaml_primitives::PopupPlacementMode_RightEdgeAlignedBottom:
        return Popup::PreferredJustification::Bottom;
    default:
        ASSERT(FALSE, L"Unsupported PopupPlacementMode");
        return Popup::PreferredJustification::Auto;
    }
}

/* static */ xaml_primitives::PopupPlacementMode Popup::GetPlacementFromMajorPlacementAndJustification(
    Popup::MajorPlacementMode majorPlacement,
    Popup::PreferredJustification justification,
    xaml::FlowDirection flowDirection)
{
    switch (majorPlacement)
    {
    case Popup::MajorPlacementMode::Top:
        switch (justification)
        {
        case Popup::PreferredJustification::HorizontalCenter:
            return xaml_primitives::PopupPlacementMode_Top;
        case Popup::PreferredJustification::Left:
            if (flowDirection == xaml::FlowDirection_LeftToRight)
            {
                return xaml_primitives::PopupPlacementMode_TopEdgeAlignedLeft;
            }
            else
            {
                return xaml_primitives::PopupPlacementMode_TopEdgeAlignedRight;
            }
        case Popup::PreferredJustification::Right:
            if (flowDirection == xaml::FlowDirection_LeftToRight)
            {
                return xaml_primitives::PopupPlacementMode_TopEdgeAlignedRight;
            }
            else
            {
                return xaml_primitives::PopupPlacementMode_TopEdgeAlignedLeft;
            }
        default:
            ASSERT(FALSE, L"Invalid MajorPlacementMode/PreferredJustification combination");
            return xaml_primitives::PopupPlacementMode_Auto;
        }
    case Popup::MajorPlacementMode::Bottom:
        switch (justification)
        {
        case Popup::PreferredJustification::HorizontalCenter:
            return xaml_primitives::PopupPlacementMode_Bottom;
        case Popup::PreferredJustification::Left:
            if (flowDirection == xaml::FlowDirection_LeftToRight)
            {
                return xaml_primitives::PopupPlacementMode_BottomEdgeAlignedLeft;
            }
            else
            {
                return xaml_primitives::PopupPlacementMode_BottomEdgeAlignedRight;
            }
        case Popup::PreferredJustification::Right:
            if (flowDirection == xaml::FlowDirection_LeftToRight)
            {
                return xaml_primitives::PopupPlacementMode_BottomEdgeAlignedRight;
            }
            else
            {
                return xaml_primitives::PopupPlacementMode_BottomEdgeAlignedLeft;
            }
        default:
            ASSERT(FALSE, L"Invalid MajorPlacementMode/PreferredJustification combination");
            return xaml_primitives::PopupPlacementMode_Auto;
        }
    case Popup::MajorPlacementMode::Left:
        if (flowDirection == xaml::FlowDirection_LeftToRight)
        {
            switch (justification)
            {
            case Popup::PreferredJustification::VerticalCenter:
                return xaml_primitives::PopupPlacementMode_Left;
            case Popup::PreferredJustification::Top:
                return xaml_primitives::PopupPlacementMode_LeftEdgeAlignedTop;
            case Popup::PreferredJustification::Bottom:
                return xaml_primitives::PopupPlacementMode_LeftEdgeAlignedBottom;
            default:
                ASSERT(FALSE, L"Invalid MajorPlacementMode/PreferredJustification combination");
                return xaml_primitives::PopupPlacementMode_Auto;
            }
        }
        else
        {
            switch (justification)
            {
            case Popup::PreferredJustification::VerticalCenter:
                return xaml_primitives::PopupPlacementMode_Right;
            case Popup::PreferredJustification::Top:
                return xaml_primitives::PopupPlacementMode_RightEdgeAlignedTop;
            case Popup::PreferredJustification::Bottom:
                return xaml_primitives::PopupPlacementMode_RightEdgeAlignedBottom;
            default:
                ASSERT(FALSE, L"Invalid MajorPlacementMode/PreferredJustification combination");
                return xaml_primitives::PopupPlacementMode_Auto;
            }
        }
    case Popup::MajorPlacementMode::Right:
        if (flowDirection == xaml::FlowDirection_LeftToRight)
        {
            switch (justification)
            {
            case Popup::PreferredJustification::VerticalCenter:
                return xaml_primitives::PopupPlacementMode_Right;
            case Popup::PreferredJustification::Top:
                return xaml_primitives::PopupPlacementMode_RightEdgeAlignedTop;
            case Popup::PreferredJustification::Bottom:
                return xaml_primitives::PopupPlacementMode_RightEdgeAlignedBottom;
            default:
                ASSERT(FALSE, L"Invalid MajorPlacementMode/PreferredJustification combination");
                return xaml_primitives::PopupPlacementMode_Auto;
            }
        }
        else
        {
            switch (justification)
            {
            case Popup::PreferredJustification::VerticalCenter:
                return xaml_primitives::PopupPlacementMode_Left;
            case Popup::PreferredJustification::Top:
                return xaml_primitives::PopupPlacementMode_LeftEdgeAlignedTop;
            case Popup::PreferredJustification::Bottom:
                return xaml_primitives::PopupPlacementMode_LeftEdgeAlignedBottom;
            default:
                ASSERT(FALSE, L"Invalid MajorPlacementMode/PreferredJustification combination");
                return xaml_primitives::PopupPlacementMode_Auto;
            }
        }

    default:
        ASSERT(FALSE, L"Invalid MajorPlacementMode");
        return xaml_primitives::PopupPlacementMode_Auto;
    }
}

_Check_return_ HRESULT Popup::OnPlacementTargetLayoutUpdated(
    _In_ IInspectable* /* sender */,
    _In_ IInspectable* /* args */)
{
    IFC_RETURN(SetPositionFromPlacement());
    return S_OK;
}

_Check_return_ HRESULT Popup::OnChildSizeChanged(
    _In_ IInspectable* /* sender */,
    _In_ xaml::ISizeChangedEventArgs* /* args */)
{
    IFC_RETURN(SetPositionFromPlacement());
    return S_OK;
}

_Check_return_ HRESULT Popup::OnChildLoaded(
    _In_ IInspectable* /* sender */,
    _In_ xaml::IRoutedEventArgs* /* args */)
{
    IFC_RETURN(SetPositionFromPlacement());
    return S_OK;
}

// Fills a string vector with some Popup internal field values for storage in a WarningContext when the layout iterations get close to the 250 limit
// in order to ease layout cycles' debugging (that involve a Popup).
void Popup::GetLayoutCycleWarningContext(
    _In_ CPopup* corePopup,
    _In_opt_ CPopup* placementTargetPopupCore,
    _In_ const std::wstring& prefix,
    _Inout_ std::vector<std::wstring>& warningInfo) const
{
    ASSERT(corePopup && corePopup->StoreLayoutCycleWarningContexts());

    std::wstring placementAndJustificationCalculated = prefix;
    placementAndJustificationCalculated.append(L" - m_placementAndJustificationCalculated: ");
    placementAndJustificationCalculated.append(std::to_wstring(m_placementAndJustificationCalculated));
    warningInfo.push_back(std::move(placementAndJustificationCalculated));

    std::wstring calculatedMajorPlacement = prefix;
    calculatedMajorPlacement.append(L" - m_calculatedMajorPlacement: ");
    calculatedMajorPlacement.append(std::to_wstring(static_cast<int>(m_calculatedMajorPlacement)));
    warningInfo.push_back(std::move(calculatedMajorPlacement));

    std::wstring calculatedJustification = prefix;
    calculatedJustification.append(L" - m_calculatedJustification: ");
    calculatedJustification.append(std::to_wstring(static_cast<int>(m_calculatedJustification)));
    warningInfo.push_back(std::move(calculatedJustification));

    std::wstring corePopupPrefix = prefix;
    corePopupPrefix.append(L" - corePopup");

    GetLayoutCycleWarningContext(corePopup, corePopupPrefix, warningInfo);

    if (placementTargetPopupCore)
    {
        std::wstring placementTargetPopupCorePrefix = prefix;
        placementTargetPopupCorePrefix.append(L" - placementTargetPopupCore");

        GetLayoutCycleWarningContext(placementTargetPopupCore, placementTargetPopupCorePrefix, warningInfo);
    }
}

/*static*/ void Popup::GetLayoutCycleWarningContext(
    _In_ CPopup* corePopup,
    _In_ const std::wstring& prefix,
    _Inout_ std::vector<std::wstring>& warningInfo)
{
    ASSERT(corePopup && corePopup->StoreLayoutCycleWarningContexts());

    std::wstring calculatedHOffset = prefix;
    calculatedHOffset.append(L" - m_calculatedHOffset: ");
    calculatedHOffset.append(std::to_wstring(corePopup->m_calculatedHOffset));
    warningInfo.push_back(std::move(calculatedHOffset));

    std::wstring calculatedVOffset = prefix;
    calculatedVOffset.append(L" - m_calculatedVOffset: ");
    calculatedVOffset.append(std::to_wstring(corePopup->m_calculatedVOffset));
    warningInfo.push_back(std::move(calculatedVOffset));

    std::wstring eHOffset = prefix;
    eHOffset.append(L" - m_eHOffset: ");
    eHOffset.append(std::to_wstring(corePopup->m_eHOffset));
    warningInfo.push_back(std::move(eHOffset));

    std::wstring eVOffset = prefix;
    eVOffset.append(L" - m_eVOffset: ");
    eVOffset.append(std::to_wstring(corePopup->m_eVOffset));
    warningInfo.push_back(std::move(eVOffset));

    std::wstring hAdjustment = prefix;
    hAdjustment.append(L" - m_hAdjustment: ");
    hAdjustment.append(std::to_wstring(corePopup->m_hAdjustment));
    warningInfo.push_back(std::move(hAdjustment));

    std::wstring vAdjustment = prefix;
    vAdjustment.append(L" - m_vAdjustment: ");
    vAdjustment.append(std::to_wstring(corePopup->m_vAdjustment));
    warningInfo.push_back(std::move(vAdjustment));
}

// Microsoft::UI::Composition::ICompositionSupportsSystemBackdrop implementation
IFACEMETHODIMP Popup::get_SystemBackdrop(_Outptr_result_maybenull_ ABI::Windows::UI::Composition::ICompositionBrush** systemBackdropBrush)
{
    CPopup* corePopup = static_cast<CPopup*>(GetHandle());
    IFC_RETURN(corePopup->GetSystemBackdrop(systemBackdropBrush));
    return S_OK;
}

IFACEMETHODIMP Popup::put_SystemBackdrop(_In_opt_ ABI::Windows::UI::Composition::ICompositionBrush* systemBackdropBrush)
{
    CPopup* corePopup = static_cast<CPopup*>(GetHandle());
    IFC_RETURN(corePopup->SetSystemBackdrop(systemBackdropBrush));
    return S_OK;
}
