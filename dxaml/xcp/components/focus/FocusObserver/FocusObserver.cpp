// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#include "precomp.h"

#include "FocusObserver.h"

#include <CoreP.h>
#include <TreeWalker.h>

#include <DependencyObjectTraits.h>
#include <DependencyObjectTraits.g.h>
#include <DOPointerCast.h>
#include <UIElement.h>
#include <CControl.h>
#include <FocusMgr.h>
#include <Transforms.h>
#include "FocusableHelper.h"
#include <TextElement.h>
#include <CoreWindow.h>

#include "ConversionFunctions.h"

#include "RootVisual.h"
#include "NavigateFocusResult.h"
#include "NavigationFocusEventArgs.h"
#include "IFocusController.h"

using namespace DirectUI;
using namespace Focus;

namespace DirectUI
{
    _Check_return_ IActivationFactory* CreateActivationFactory_XamlSourceFocusNavigationRequest();
}

FocusObserver::FocusObserver(_In_ CCoreServices *pCoreServices, _In_ CContentRoot* contentRoot)
    : m_pCoreServicesNoRef(pCoreServices),
    m_contentRoot(contentRoot)
{
}

FocusObserver::~FocusObserver()
{
    VERIFYHR(DeInit());
}

_Check_return_ HRESULT
FocusObserver::DeInit()
{
    
    return S_OK;
}

_Check_return_ HRESULT
FocusObserver::Init(_In_opt_ xaml_hosting::IFocusController* const pFocusController)
{
    m_spFocusController = pFocusController;

    return S_OK;
}

_Check_return_ HRESULT
FocusObserver::GetOriginToComponent(
    _In_opt_ CDependencyObject* const pOldFocusedElement,
    _Out_ wf::Rect* origin)
{
    XRECTF_RB focusedElementBounds = {};

    //Transform the bounding rect of currently focused element to Component's co-ordinate space
    if (pOldFocusedElement)
    {
        if (IFocusable* focusable = CFocusableHelper::GetIFocusableForDO(pOldFocusedElement))
        {
            CDependencyObject* depObj = focusable->GetDOForIFocusable();
            IFC_RETURN(do_pointer_cast<CTextElement>(depObj)->GetContainingFrameworkElement()->GetGlobalBounds(&focusedElementBounds, true));
        }
        else
        {
            CUIElement* const pUIElement = do_pointer_cast<CUIElement>(pOldFocusedElement);
            if (pUIElement)
            {
                IFC_RETURN(pUIElement->GetGlobalBounds(&focusedElementBounds, true));
            }
        }
    }

    origin->X = focusedElementBounds.left;
    origin->Y = focusedElementBounds.top;
    origin->Width = focusedElementBounds.right - focusedElementBounds.left;
    origin->Height = focusedElementBounds.bottom - focusedElementBounds.top;

    return S_OK;
}

XRECTF_RB
FocusObserver::GetOriginFromInteraction()
{
    XRECTF_RB rectRB = {};
    
    if (m_spCurrentInteraction.Get())
    {
        wf::Rect origin = {};
        IFCFAILFAST(m_spCurrentInteraction->get_HintRect(&origin));

        rectRB = {
            origin.X,
            origin.Y,
            origin.X + origin.Width,
            origin.Y + origin.Height
        };
    }

    return rectRB;
}

_Check_return_ const FocusMovementResult
FocusObserver::NavigateFocusXY(
    _In_ CDependencyObject* pComponent,
    _In_ const FocusNavigationDirection direction,
    _In_ const XRECTF_RB& origin)
{
    XRECTF_RB rect = origin;
    Focus::XYFocusOptions xyFocusOptions = {};
    xyFocusOptions.focusHintRectangle = &rect;

    // Any value of the manifold is meaning less when Navigating or Departing into or from a component.
    // The current manifold needs to be updated from the Origin given.
    xyFocusOptions.updateManifoldsFromFocusHintRect = true;

    FocusMovement movement(xyFocusOptions, direction, pComponent);

    // We dont handle cancellation of a focus request from a host:
    //   We could support this by calling DepartFocus from the component
    //   if the component returns result.WasCanceled()
    //   We choose to not support it.
    movement.canCancel = false;

    // Do not allow DepartFocus to be called, CoreWindowsFocusAdapter will handle it.
    movement.canDepartFocus = false;

    movement.shouldCompleteAsyncOperation = true;

    return m_contentRoot->GetFocusManagerNoRef()->FindAndSetNextFocus(movement);
}

_Check_return_ HRESULT
FocusObserver::CalculateNewOrigin(_In_ FocusNavigationDirection direction, _In_ const wf::Rect& currentOrigin, _Out_ wf::Rect* newOrigin)
{
    wf::Rect windowBounds = {};
    auto* const pFocusedElement = m_contentRoot->GetVisualTreeNoRef()->GetActiveRootVisual();
    IFC_RETURN(GetOriginToComponent(pFocusedElement, &windowBounds));

    *newOrigin = currentOrigin;
    switch (direction)
    {
        case FocusNavigationDirection::Left:
        case FocusNavigationDirection::Right:
            newOrigin->X = windowBounds.X;
            newOrigin->Width = windowBounds.Width;
            break;
        case FocusNavigationDirection::Up:
        case FocusNavigationDirection::Down:
            newOrigin->Y = windowBounds.Y;
            newOrigin->Height = windowBounds.Height;
            break;
    }

    return S_OK;
}

_Check_return_ HRESULT
FocusObserver::ProcessNavigateFocusRequest(
    _In_ xaml_hosting::IXamlSourceFocusNavigationRequest* focusNavigationRequest,
    _Out_ boolean* pHandled)
{
    UpdateCurrentInteraction(focusNavigationRequest);

    xaml_hosting::XamlSourceFocusNavigationReason reason = {};
    IFC_RETURN(focusNavigationRequest->get_Reason(&reason));

    CDependencyObject* pRoot = nullptr;

    switch (m_contentRoot->GetType())
    {
    case CContentRoot::Type::XamlIslandRoot:
        // Normally islands and non-islands should be the same here and both use GetActiveRootVisual. 
        // However, in the islands case if we used GetActiveRootVisual then if the island has as its
        // root content a single control then the GetFirstFocusableElement logic below will not find it.
        // Therefore we check the RootScrollViewer (created as the parent of the root visual) so that
        // the root will be considered during the focus search. But if the island has content that is of
        // type Canvas then the RootScrollViewer won't be created (quirk/feature) so instead of doing
        // nothing we will use the root visual if the RootScrollViewer is null.
        pRoot = m_contentRoot->GetVisualTreeNoRef()->GetRootScrollViewer();
        if (!pRoot)
        {
            pRoot = m_contentRoot->GetVisualTreeNoRef()->GetActiveRootVisual();
        }
        break;
    default:
        pRoot = m_contentRoot->GetVisualTreeNoRef()->GetActiveRootVisual();
    }

    const FocusNavigationDirection direction = GetFocusNavigationDirectionFromReason(reason);

    if (reason == xaml_hosting::XamlSourceFocusNavigationReason::XamlSourceFocusNavigationReason_First ||
        reason == xaml_hosting::XamlSourceFocusNavigationReason::XamlSourceFocusNavigationReason_Last)
    {
        m_contentRoot->GetInputManager().SetLastInputDeviceType(GetInputDeviceTypeFromDirection(direction));
        const bool bReverse = (reason == xaml_hosting::XamlSourceFocusNavigationReason::XamlSourceFocusNavigationReason_Last);

        if (!pRoot)
        {
            // No content has been loaded, bail out
            return S_OK;
        }

        CDependencyObject* pCandidateElement = nullptr;
        if(bReverse)
        {
            pCandidateElement = m_contentRoot->GetFocusManagerNoRef()->GetLastFocusableElement(pRoot);
        }
        else
        {
            pCandidateElement = m_contentRoot->GetFocusManagerNoRef()->GetFirstFocusableElement(pRoot);
        }

        const bool retryWithPopupRoot = (pCandidateElement == nullptr);
        if (retryWithPopupRoot)
        {
            const auto popupRoot = m_contentRoot->GetVisualTreeNoRef()->GetPopupRoot();
            if (popupRoot != nullptr)
            {
                if (bReverse)
                {
                    pCandidateElement = m_contentRoot->GetFocusManagerNoRef()->GetLastFocusableElement(popupRoot);
                }
                else
                {
                    pCandidateElement = m_contentRoot->GetFocusManagerNoRef()->GetFirstFocusableElement(popupRoot);
                }
            }
        }

        if (pCandidateElement)
        {
            // When we move focus into XAML with tab, we first call ClearFocus to mimic desktop behavior.
            // On desktop, we call ClearFocus during a tab cycle in CJupiterWindow::AcceleratorKeyActivated, but when
            // running as a component (e.g. in c-shell) the way XAML loses focus when the user tabs away is different
            // (we call DepartFocus) and if we call ClearFocus at the same time as we do on desktop, we'll end up
            // firing the LostFocus for the previously-focused before calling GettingFocus for the newly focused element.
            // So instead, we call ClearFocus as we tab *in* to XAML content.  This preserves the focus event ordering on
            // tab cycles.
            m_contentRoot->GetFocusManagerNoRef()->ClearFocus();

            FocusMovement movement(
                pCandidateElement,
                bReverse ? FocusNavigationDirection::Previous : FocusNavigationDirection::Next,
                FocusState::Keyboard);

            // We dont handle cancellation of a focus request from a host:
            //   We could support this by calling DepartFocus from the component
            //   if the component returns result.WasCanceled()
            //   We choose to not support it.
            movement.canCancel = false;

            const FocusMovementResult result = m_contentRoot->GetFocusManagerNoRef()->SetFocusedElement(movement);
            if (result.WasMoved())
            {
                StopInteraction(pHandled);
            }
        }
        else
        {
            GUID correlationId = {};
            IFC_RETURN(focusNavigationRequest->get_CorrelationId(&correlationId));

            bool handled = false;
            IFC_RETURN(DepartFocus(direction, correlationId, &handled))
        }
    }
    else if (reason == xaml_hosting::XamlSourceFocusNavigationReason::XamlSourceFocusNavigationReason_Restore ||
             reason == xaml_hosting::XamlSourceFocusNavigationReason::XamlSourceFocusNavigationReason_Programmatic)
    {
        const auto* const pFocusedElement = m_contentRoot->GetFocusManagerNoRef()->GetFocusedElementNoRef();
        if (pFocusedElement!=nullptr)
        {
            StopInteraction(pHandled);
        }
        else if (pFocusedElement==nullptr && reason == xaml_hosting::XamlSourceFocusNavigationReason::XamlSourceFocusNavigationReason_Programmatic)
        {
            if (!pRoot)
            {
                // No content has been loaded, bail out
                return S_OK;
            }

            CDependencyObject* pCandidateElement = m_contentRoot->GetFocusManagerNoRef()->GetFirstFocusableElement(pRoot);
            if (pCandidateElement == nullptr)
            {
                pCandidateElement = pRoot;
            }
            if (pCandidateElement)
            {
                FocusMovement movement(pCandidateElement, FocusNavigationDirection::None, FocusState::Programmatic);

                // We dont handle cancellation of a focus request from a host:
                //   We could support this by calling DepartFocus from the component
                //   if the component returns result.WasCanceled()
                //   We choose to not support it.
                movement.canCancel = false;

                const FocusMovementResult result = m_contentRoot->GetFocusManagerNoRef()->SetFocusedElement(movement);
                if (result.WasMoved())
                {
                    StopInteraction(pHandled);
                }
            }
        }
    }
    else if (reason == xaml_hosting::XamlSourceFocusNavigationReason::XamlSourceFocusNavigationReason_Left  ||
             reason == xaml_hosting::XamlSourceFocusNavigationReason::XamlSourceFocusNavigationReason_Right ||
             reason == xaml_hosting::XamlSourceFocusNavigationReason::XamlSourceFocusNavigationReason_Up    ||
             reason == xaml_hosting::XamlSourceFocusNavigationReason::XamlSourceFocusNavigationReason_Down)
    {
        if (!pRoot)
        {
            // No content has been loaded, bail out
            return S_OK;
        }

        m_contentRoot->GetInputManager().SetLastInputDeviceType(GetInputDeviceTypeFromDirection(direction));

        XRECTF_RB rect = GetOriginFromInteraction();

        const FocusMovementResult result = NavigateFocusXY(pRoot, direction, rect);
        IFC_RETURN(result.GetHResult());
        if (result.WasMoved())
        {
            StopInteraction(pHandled);
        }
        else
        {
            //
            // If we could not find a target via XY then we need to depart focus again
            // But this time from an orgin inside of the component
            //
            //                             ┌────────────────────────────────┐
            //                             │        CoreWindow              │
            //                             │                                │
            //                             │                                │
            //   ┌──────────┐ Direction    ├────────────────────────────────┤
            //   │  origin  │ ─────────>   │      New Origin:               │ Depart Focus from new origin
            //   │          │              │ Calculated as the intersertion │  ─────────>
            //   │          │              │ from the direction             │
            //   └──────────┘              ├────────────────────────────────┤
            //                             │                                │
            //                             │                                │
            //                             │                                │
            //                             └────────────────────────────────┘

            wf::Rect origin = {};
            IFC_RETURN(focusNavigationRequest->get_HintRect(&origin));
            wf::Rect newOrigin = {};
            IFC_RETURN(CalculateNewOrigin(direction, origin, &newOrigin));
            GUID correlationId = {};
            IFC_RETURN(focusNavigationRequest->get_CorrelationId(&correlationId));

            bool handled = false;
            IFC_RETURN(DepartFocus(direction, newOrigin, correlationId, &handled))
        }
    }

    return S_OK;
}

_Check_return_ HRESULT
FocusObserver::DepartFocus(_In_ FocusNavigationDirection direction, _In_ GUID correlationId, _Inout_ bool* pHandled)
{
    wf::Rect origin = {};
    auto* const pFocusedElement = m_contentRoot->GetFocusManagerNoRef()->GetFocusedElementNoRef();
    IFC_RETURN(GetOriginToComponent(pFocusedElement, &origin));

    IFC_RETURN(DepartFocus(direction, origin, correlationId, pHandled));

    return S_OK;
}

_Check_return_ HRESULT
FocusObserver::DepartFocus(_In_ FocusNavigationDirection direction, _In_ const wf::Rect& origin, _In_ GUID correlationId, _Inout_ bool* pHandled)
{
    if (*pHandled || m_spFocusController == nullptr)
    {
        return S_OK;
    }

    const auto reason = GetFocusNavigationReasonFromDirection(direction);
    if (reason == XAML_SOURCE_FOCUS_NAVIGATION_REASON_NOT_SUPPORTED)
    {
        // Do nothing if we dont support this navigation
        return S_OK;
    }

    IFC_RETURN(StartInteraction(reason, origin, correlationId));

    IFC_RETURN(m_spFocusController->DepartFocus(m_spCurrentInteraction.Get()));
    *pHandled = TRUE;

    return S_OK;
}

_Check_return_ HRESULT
FocusObserver::StartInteraction(
    _In_ xaml_hosting::XamlSourceFocusNavigationReason reason,
    _In_ const wf::Rect& origin,
    _In_ GUID correlationId)
{
    wrl::ComPtr<IActivationFactory> requestActivationFactory;
    requestActivationFactory.Attach(DirectUI::CreateActivationFactory_XamlSourceFocusNavigationRequest());
    wrl::ComPtr<xaml_hosting::IXamlSourceFocusNavigationRequestFactory> requestFactory;
    IFC_RETURN(requestActivationFactory.As(&requestFactory));

    IFC_RETURN(requestFactory->CreateInstanceWithHintRectAndCorrelationId(
        reason,
        origin,
        correlationId,
        &m_spCurrentInteraction));

    return S_OK;
}

void FocusObserver::StopInteraction(boolean* pHandled)
{
    ASSERT(m_spCurrentInteraction);
    m_spCurrentInteraction.Reset();
    *pHandled = true;        
}

wuc::CoreWindowActivationMode FocusObserver::GetActivationMode() const
{
    return wuc::CoreWindowActivationMode::CoreWindowActivationMode_None;
}