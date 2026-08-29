// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#include "precomp.h"
#include "Activators.g.h"
#include "VisualTree.h"
#include "InputServices.h"
//------------------------------------------------------------------------
//
//  Method:   ~CManipulationEventArgs
//
//  Synopsis:
//      Destructor - releases the potential reference to the manipulation
//      container
//
//------------------------------------------------------------------------
CManipulationEventArgs::~CManipulationEventArgs()
{
    ReleaseInterface(m_pManipulationContainer);
}

//------------------------------------------------------------------------
//
//  Method:   Complete
//
//  Synopsis:
//      Completes the possible ongoing ICM manipulation.
//
//------------------------------------------------------------------------
_Check_return_ HRESULT
CManipulationEventArgs::Complete()
{
    if (m_pCore)
    {
        IFCPTR_RETURN(m_pSource);
        auto pManipulatedElement = static_cast<CUIElement*>(m_pSource);
        auto inputServices = pManipulatedElement->GetContext()->GetInputServices();
        if (inputServices)
        {
            // Stop the interaction context manager
            inputServices->StopInteraction(pManipulatedElement, false /*bCallbackForManipulationCompleted*/);
        }

        // Raising ManipulationCompleted event
        m_bRequestedComplete = TRUE;
    }
    return S_OK;
}

//------------------------------------------------------------------------
//
//  Method:   SetManipulationOriginPosition
//
//  Synopsis:
//      Set the manipulation origin position that base on the manipulation
//      container element.
//
//------------------------------------------------------------------------
_Check_return_ HRESULT
CManipulationEventArgs::SetManipulationOriginPosition(
    _In_ XFLOAT xOrigin,
    _In_ XFLOAT yOrigin)
{
    m_ptManipulation.x = xOrigin;
    m_ptManipulation.y = yOrigin;

    wf::Point pt = { xOrigin, yOrigin };

    // Get the relative point that is based on the manipulation container element
    IFC_RETURN(GetRelativePosition(m_pManipulationContainer, &pt));

    m_ptManipulation.x = pt.X;
    m_ptManipulation.y = pt.Y;

    return S_OK;
}

_Check_return_ HRESULT CManipulationEventArgs::get_Container(_In_ CUIElement** ppManipulationContainer)
{
    SetInterface(*ppManipulationContainer, m_pManipulationContainer);
    RRETURN(S_OK);
}

_Check_return_ HRESULT CManipulationEventArgs::put_Container(_In_ CUIElement* pManipulationContainer)
{
    ReplaceInterface(m_pManipulationContainer, pManipulationContainer);
    RRETURN(S_OK);
}

_Check_return_ HRESULT CManipulationInertiaStartingEventArgs::CreateFrameworkPeer(_Outptr_ IInspectable** ppPeer)
{
    RRETURN(DirectUI::OnFrameworkCreateManipulationInertiaStartingRoutedEventArgs(this, ppPeer));
}

_Check_return_ HRESULT CManipulationCompletedEventArgs::CreateFrameworkPeer(_Outptr_ IInspectable** ppPeer)
{
    RRETURN(DirectUI::OnFrameworkCreateManipulationCompletedRoutedEventArgs(this, ppPeer));
}

_Check_return_ HRESULT CManipulationDeltaEventArgs::CreateFrameworkPeer(_Outptr_ IInspectable** ppPeer)
{
    RRETURN(DirectUI::OnFrameworkCreateManipulationDeltaRoutedEventArgs(this, ppPeer));
}

_Check_return_ HRESULT CManipulationStartingEventArgs::CreateFrameworkPeer(_Outptr_ IInspectable** ppPeer)
{
    RRETURN(DirectUI::OnFrameworkCreateManipulationStartingRoutedEventArgs(this, ppPeer));
}

_Check_return_ HRESULT CManipulationStartedEventArgs::CreateFrameworkPeer(_Outptr_ IInspectable** ppPeer)
{
    RRETURN(DirectUI::OnFrameworkCreateManipulationStartedRoutedEventArgs(this, ppPeer));
}
