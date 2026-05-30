// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#include "precomp.h"
#include "Activators.g.h"
#include "ContextRequestedEventArgs.h"

//------------------------------------------------------------------------
//
//  Method:   GetPosition(static)
//
//  Synopsis:
//      Static method that translates the global coordinates in a 
//  CInputPointEventArgs into coordinates relative to the given CUIElement.
//
//      pObject is the CInputPointEventArgs pointer.
//      ppargs[0] is the CUIElement pointer.
//
//------------------------------------------------------------------------

_Check_return_ 
HRESULT 
CInputPointEventArgs::GetPosition(
    _In_opt_ CUIElement* pRelativeTo, 
    _Out_ wf::Point*  pRelativePoint)
{
    wf::Point point;

    // Copy the global coordinate into it.
    point.X = m_ptGlobal.x;
    point.Y = m_ptGlobal.y;

    // Convert the point from the global to the relative point
    IFC_RETURN(ConvertGlobalPointToRelativePoint(pRelativeTo, &point));

    // Give ownership of the point to the result
    *pRelativePoint = point;

    return S_OK;
}

//------------------------------------------------------------------------
//
//  Method:   GetRelativePosition
//
//  Synopsis:
//      Get the relative position from the specified relative element
//
//------------------------------------------------------------------------
_Check_return_ 
HRESULT 
CInputPointEventArgs::GetRelativePosition(
    _In_opt_ CUIElement* pRelativeTo, 
    _Inout_ wf::Point* pRelativePoint)
{
    return ConvertGlobalPointToRelativePoint(pRelativeTo, pRelativePoint);
}

//------------------------------------------------------------------------
//
//  Method:   ConvertGlobalPointToRelativePoint
//
//  Synopsis:
//      Convert the global pointer to the specified element's position
//
//------------------------------------------------------------------------
_Check_return_ HRESULT 
CInputPointEventArgs::ConvertGlobalPointToRelativePoint(
    _In_opt_ CUIElement* pRelativeTo, 
    _Inout_ wf::Point* pRelativePoint)
{
    if(NULL != pRelativeTo 
        && pRelativeTo->GetTypeIndex() == KnownTypeIndex::Popup
        && !pRelativeTo->IsActive())
    {
        // for popup not in the live tree, we can use its 
        // child for this operation since popup does not
        // have visuals of its own and has just one child. 
        // static_cast safe as we have already verified it to be a popup.
        CPopup* pPopup = static_cast<CPopup*>(pRelativeTo);
        pRelativeTo = pPopup->m_pChild;
    }

    if(NULL != pRelativeTo && !pRelativeTo->IsActive())
    {
        // We doesn't return a fail for inactive relativeTo element. 
        // Return the zero point as (0,0) if relativeTo element is out of tree. 
        pRelativePoint->X = 0;
        pRelativePoint->Y = 0;
    }
    else
    {
        IFC_RETURN(TransformPointToElement(pRelativeTo, pRelativePoint));
    }

    return S_OK;
}


HRESULT CInputPointEventArgs::TransformPointToElement(_In_opt_ CUIElement *pUIElement, _Inout_ wf::Point* ppt)
{
    HRESULT hr = S_OK;
    ITransformer* pTransformer = NULL;
    wf::Point pt = *ppt;
    CAggregateTransformer* pAT = NULL;
    CUIElement *pTargetElement = pUIElement;

    // In the event that the target element is NULL then the point should be transformed to the
    // browser control root. This means taking in to account the hidden root visual so it's
    // nessecary to transform to the actual control root.
    if (pTargetElement == NULL)
    {
        pTargetElement = static_cast<CUIElement*>(m_pCore->GetMainRootVisual());
    }

    if (pTargetElement)
    {
        // Get the transform from the root visual to the element
        IFC(pTargetElement->TransformToRoot(&pTransformer));

        // Transfrom the point
        XPOINTF ptInternal = { pt.X, pt.Y };
        IFC(pTransformer->ReverseTransform(&ptInternal, &ptInternal, 1));
        pt.X = ptInternal.x;
        pt.Y = ptInternal.y;
    }

    *ppt = pt;

Cleanup:
    ReleaseInterface(pTransformer);
    ReleaseInterface(pAT);
    RRETURN(hr);
}

_Check_return_ HRESULT CTappedEventArgs::CreateFrameworkPeer(_Outptr_ IInspectable** ppPeer)
{
    RRETURN(DirectUI::OnFrameworkCreateTappedRoutedEventArgs(this, ppPeer));
}

_Check_return_ HRESULT CDoubleTappedEventArgs::CreateFrameworkPeer(_Outptr_ IInspectable** ppPeer)
{
    RRETURN(DirectUI::OnFrameworkCreateDoubleTappedRoutedEventArgs(this, ppPeer));
}

_Check_return_ HRESULT CContextRequestedEventArgs::CreateFrameworkPeer(_Outptr_ IInspectable** ppPeer)
{
    RRETURN(DirectUI::OnFrameworkCreateContextRequestedEventArgs(this, ppPeer));
}

_Check_return_ HRESULT CRightTappedEventArgs::CreateFrameworkPeer(_Outptr_ IInspectable** ppPeer)
{
    RRETURN(DirectUI::OnFrameworkCreateRightTappedRoutedEventArgs(this, ppPeer));
}
