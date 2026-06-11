// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#include "precomp.h"
#include "RichEditGripper.h"
#include "TextBoxBase.h"
#include "TextBoxView.h"
#include "HoldingEventArgs.h"
#include "RichEditGripperChild.h"

#include <textserv.h>

//------------------------------------------------------------------------
//
//  Method:   CRichEditGripper::Create
//
//  Synopsis: Bare DO initialization. Most of the initialization is done in
//  InitializeGripper method.
//
//------------------------------------------------------------------------
_Check_return_ HRESULT CRichEditGripper::Create(
    _Outptr_ CRichEditGripper **ppObject,
    _In_ CREATEPARAMETERS *pCreate)
{
    HRESULT hr = S_OK;
    CRichEditGripper *pObj = NULL;
    CDependencyObject *pPopupChild = nullptr;
    if (pCreate->m_value.GetType() == valueString)
    {
        IFC(E_NOTIMPL);
    }
    else
    {
        pObj = new CRichEditGripper();

        IFC(CRichEditGripperChild::Create(&pPopupChild, pCreate));

        pObj->m_pPopupChild.attach(static_cast<CRichEditGripperChild*>(pPopupChild));
        pObj->m_pPopupChild->m_pParentGripperNoRef = pObj;

        *ppObject = pObj;
        pObj = NULL;
        pPopupChild = NULL;
    }

Cleanup:
    ReleaseInterface(pObj); // Follow the standard pattern
    ReleaseInterface(pPopupChild);
    return hr;
}

_Check_return_ HRESULT CRichEditGripper::InitializeGripper(
    _In_ ITextServices *pTextServices,
    _In_ RichEditGripperCommon::GripperType gripperType,
    _In_ ITextHost2 *pTextHost,
    _In_ CTextBoxBase *pTextBoxBase)
{
    if (!m_pPopupChild)
    {
        IFC_RETURN(E_FAIL);
    }

    IFC_RETURN(m_pPopupChild->InitializeGripper(pTextServices,
        gripperType,
        pTextHost,
        pTextBoxBase));

    return S_OK;
}

//------------------------------------------------------------------------
//
//  Synopsis:
//      Updates the position of the gripper. Coordinate is relative to
//      world. Also changes the gripper pole height, unless the lineHeight
//      is POLE_HEIGHT_UNCHANGED
//
//      Note: this method does not force the coordinate property changes if
//      the new coordinates matches the previous ones.
//
//      Note: it is assumed that the Show method will be called immediately
//      following any call to this method.
//
//------------------------------------------------------------------------
_Check_return_ HRESULT CRichEditGripper::UpdateCenterWorldCoordinate(
    _In_ const XPOINTF& centerWorldCoordinate,
    _In_ XFLOAT lineHeight)
{
    IFC_RETURN(m_pPopupChild->UpdateCenterWorldCoordinate(centerWorldCoordinate, lineHeight));
    return S_OK;
}

XRECT_WH CRichEditGripper::GetGripperScreenRect()
{
    return m_pPopupChild->GetGripperScreenRect();

}

_Check_return_ HRESULT CRichEditGripper::UpdateThemeColor()
{
    IFC_RETURN(m_pPopupChild->UpdateThemeColor());
    return S_OK;
}

_Check_return_ XFLOAT CRichEditGripper::GetActualOffsetX()
{
    return m_pPopupChild->GetActualOffsetX();
}

_Check_return_ XFLOAT CRichEditGripper::GetActualOffsetY()
{
    return m_pPopupChild->GetActualOffsetY();
}

_Check_return_ HRESULT CRichEditGripper::ShowPole(RichEditGripperCommon::PoleMode ePoleMode)
{
    IFC_RETURN(m_pPopupChild->ShowPole(ePoleMode));
    return S_OK;
}

_Check_return_ HRESULT CRichEditGripper::Show(bool bForSelection)
{
    IFC_RETURN(m_pPopupChild->Show(bForSelection));
    return S_OK;
}

_Check_return_ HRESULT CRichEditGripper::Hide()
{
    IFC_RETURN(m_pPopupChild->Hide());
    return S_OK;
}

_Check_return_ HRESULT CRichEditGripper::OnHitTargetTapped(_In_ CEventArgs* pEventArgs)
{
    IFC_RETURN(m_pPopupChild->OnHitTargetTapped(pEventArgs));
    return S_OK;
}

_Check_return_ HRESULT CRichEditGripper::OnHitTargetRightTapped(_In_ CEventArgs* pEventArgs)
{
    IFC_RETURN(m_pPopupChild->OnHitTargetRightTapped(pEventArgs));
    return S_OK;
}

_Check_return_ HRESULT CRichEditGripper::OnHitTargetDoubleTapped(_In_ CEventArgs* pEventArgs)
{
    IFC_RETURN(m_pPopupChild->OnHitTargetDoubleTapped(pEventArgs));
    return S_OK;
}

_Check_return_ HRESULT CRichEditGripper::OnManipulation(
    bool begin,
    bool end,
    XINT32 x0,
    XINT32 y0,
    INT32 pointerId)
{
    IFC_RETURN(m_pPopupChild->OnManipulation(
        begin,
        end,
        x0,
        y0,
        pointerId));

    return S_OK;
}

void CRichEditGripper::SetPopup(_In_ CPopup *pPopup)
{
    m_pPopup.attach(pPopup);
    VERIFYHR(m_pPopup->SetValueByKnownIndex(KnownPropertyIndex::Popup_Child, m_pPopupChild.get()));
}

XPOINTF CRichEditGripper::GetPosition()
{
    return m_pPopupChild->GetPosition();
}

CTouchableRectangle* CRichEditGripper::GetHitTarget()
{
    return m_pPopupChild->GetHitTarget();
}

_Check_return_ bool CRichEditGripper::HasTextSelection()
{
    if (m_pPopupChild)
    {
        return m_pPopupChild->HasTextSelection();
    }

    return false;
}
