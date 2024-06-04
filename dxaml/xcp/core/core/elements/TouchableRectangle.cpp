// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#include "precomp.h"

#include "RichEditGripperChild.h"

CRichEditGripper* CTouchableRectangle::GetParentGripper() const
{
    CRichEditGripperChild *pGripper = static_cast<CRichEditGripperChild*>(GetParent());
    return pGripper->m_pParentGripperNoRef;
}

_Check_return_ HRESULT
CTouchableRectangle::OnTapped(_In_ CEventArgs* pEventArgs)
{
    if (m_hasPointerCapture)
    {
        // Still in a touch session, so cache the event args to be sent after
        // pointer is released.
        ReleaseInterface(m_pPendingTapArgs);
        m_pPendingTapArgs = pEventArgs;
        pEventArgs->AddRef();
    }
    else
    {
        CRichEditGripper *pGripper = GetParentGripper();
        IFC_RETURN(pGripper->OnHitTargetTapped(pEventArgs));
    }

    return S_OK;
}

_Check_return_ HRESULT
CTouchableRectangle::OnRightTapped(_In_ CEventArgs* pEventArgs)
{
    CRichEditGripper *pGripper = GetParentGripper();

    const CInputPointEventArgs* pPointerEventArgs = static_cast<CInputPointEventArgs*>(pEventArgs);
    const bool inputIsPen = pPointerEventArgs->m_pointerDeviceType == DirectUI::PointerDeviceType::Pen;
    if (inputIsPen)
    {
        IFC_RETURN(pGripper->OnHitTargetRightTapped(pEventArgs));
    }

    return S_OK;
}

_Check_return_ HRESULT
CTouchableRectangle::OnDoubleTapped(_In_ CEventArgs* pEventArgs)
{
    if (m_hasPointerCapture)
    {
        // Still in a touch session, so cache the event args to be sent after
        // pointer is released.
        ReleaseInterface(m_pPendingDoubleTapArgs);
        m_pPendingDoubleTapArgs = pEventArgs;
        pEventArgs->AddRef();
    }
    else
    {
        CRichEditGripper *pGripper = GetParentGripper();
        IFC_RETURN(pGripper->OnHitTargetDoubleTapped(pEventArgs));
    }

    return S_OK;
}

_Check_return_ HRESULT
CTouchableRectangle::OnHolding(_In_ CEventArgs* pEventArgs)
{
    CRichEditGripper *pGripper = GetParentGripper();
    if (pGripper && pGripper->m_pPopupChild)
    {
        IFC_RETURN(pGripper->m_pPopupChild->OnGripperHeld(pEventArgs));
    }

    CPointerEventArgs  *pPointerEventArgs = static_cast<CPointerEventArgs*>(pEventArgs);
    pPointerEventArgs->m_bHandled = TRUE;

    return S_OK;
}

_Check_return_ HRESULT
CTouchableRectangle::OnPointerPressed(_In_ CEventArgs* pEventArgs)
{
    CRichEditGripper *pGripper = GetParentGripper();
    CPointerEventArgs *pPointerEventArgs = static_cast<CPointerEventArgs*>(pEventArgs);

    CRichEditGripper* const pPeerGripper = pGripper->m_pPopupChild->GetPeerNoRef();

    // Consider transferring this touch session to the peer gripper, but only
    // if it exists and is currently visible

    if (nullptr != pPeerGripper && pPeerGripper->m_pPopupChild->IsVisible())
    {

        //GetPosition() returns a point scaled for DPI which is why we use rasterzationscale on GetGlobalPoint() as well

        const XPOINTF ptThis = pGripper->GetPosition();
        const XPOINTF ptPeer = pPeerGripper->GetPosition();

        const auto rasterizationscale = VisualTree::GetForElementNoRef(this)->GetRasterizationScale();
        const auto contactX =  rasterizationscale * pPointerEventArgs->GetGlobalPoint().x;
        const auto contactY =  rasterizationscale * pPointerEventArgs->GetGlobalPoint().y;

        // Get distance (squared) from touch point to this gripper

        XFLOAT rDiffX = static_cast<XFLOAT>(contactX - ptThis.x);
        XFLOAT rDiffY = static_cast<XFLOAT>(contactY - ptThis.y);
        XFLOAT rDistSqThis = (rDiffX * rDiffX) + (rDiffY * rDiffY);

        // Get distance (squared) from touch point to the peer gripper

        rDiffX = static_cast<XFLOAT>(contactX - ptPeer.x);
        rDiffY = static_cast<XFLOAT>(contactY - ptPeer.y);
        XFLOAT rDistSqPeer = (rDiffX * rDiffX) + (rDiffY * rDiffY);

        // If the touch point is closer to the center of the peer gripper, then
        // transfer the pointer event to that gripper.

        if (rDistSqPeer < rDistSqThis)
        {
            CTouchableRectangle* const pPeer = pPeerGripper->GetHitTarget();

            // pPeer could theoretically be null if our peer gripper hasn't
            // been fully initialized yet.  I wouldn't expect it to be visible
            // in that case, but if that does happen, just let this gripper
            // capture the pointer (by not jumping to Cleanup).

            if (nullptr != pPeer)
            {
                IFC_RETURN(pPeer->OnPointerPressed(pEventArgs));
                return S_OK;
            }
        }
    }


    // Arriving here, we did not transfer the touch session, so this gripper
    // will claim it (by capturing the pointer).

    if (pPointerEventArgs->m_bHandled ||
        m_hasPointerCapture) // Ignore the tap with the other pointer
                             // while the gripper is being manipulated.
    {
        return S_OK;
    }

    bool hasPointerCapture = false;
    IFC_RETURN(CapturePointer(pPointerEventArgs->m_pPointer, &hasPointerCapture));
    m_hasPointerCapture = hasPointerCapture;

    if (m_hasPointerCapture)
    {
        m_uiCapturePointerId = pPointerEventArgs->m_pPointer->m_uiPointerId;
    }

    XPOINT ptGlobal = ConvertToIntPoint(pPointerEventArgs->GetGlobalPoint());
    m_ptGlobLast = ptGlobal;

    if (nullptr != pGripper)
    {
        const bool inputIsPen = pPointerEventArgs->m_pointerDeviceType == DirectUI::PointerDeviceType::Pen;
        if (inputIsPen && pPointerEventArgs->m_pPointer->m_bBarrelButtonPressed)
        {
            if (pGripper->m_pPopupChild)
            {
                IFC_RETURN(ReleaseTouchRectPointerCapture(nullptr, nullptr));
                IFC_RETURN(pGripper->m_pPopupChild->OnPointerPressedWithBarrelButtonDown(pEventArgs));
            }
        }
        else
        {
            IFC_RETURN(pGripper->OnManipulation(true, false, ptGlobal.x, ptGlobal.y, pPointerEventArgs->m_pPointer->m_uiPointerId));
        }
    }

    pPointerEventArgs->m_bHandled = TRUE;

    return S_OK;
}

_Check_return_ HRESULT
CTouchableRectangle::OnPointerMoved(_In_ CEventArgs* pEventArgs)
{
    auto const pPointerEventArgs = static_cast<CPointerEventArgs* const>(pEventArgs);
    if (pPointerEventArgs->m_bHandled || !m_hasPointerCapture)
    {
        return S_OK;
    }

    m_ptGlobLast = ConvertToIntPoint(pPointerEventArgs->GetGlobalPoint());

    CRichEditGripper* const pGripper = GetParentGripper();
    if (pGripper)
    {
        IFC_RETURN(pGripper->OnManipulation(false, false, m_ptGlobLast.x, m_ptGlobLast.y, pPointerEventArgs->m_pPointer->m_uiPointerId));
    }

    pPointerEventArgs->m_bHandled = TRUE;

    return S_OK;
}

_Check_return_ HRESULT CTouchableRectangle::OnPointerReleased(_In_ CEventArgs* pEventArgs)
{
    auto guard = wil::scope_exit([this]()
    {
        ReleaseInterface(m_pPendingDoubleTapArgs);
        ReleaseInterface(m_pPendingTapArgs);
    });

    auto const pPointerEventArgs = static_cast<CPointerEventArgs* const>(pEventArgs);

    if (pPointerEventArgs->m_bHandled)
    {
        return S_OK;
    }

    bool bCaptureReleased = false;

    if (pPointerEventArgs->m_pPointer->m_uiPointerId == m_uiCapturePointerId)
    {
        // Checks if the pointer is captured, then releases it if so
        // If the pointer is no longer captured (because gripper was hidden)
        // then we should not call OnManipulation
        IFC_RETURN(ReleaseTouchRectPointerCapture(&bCaptureReleased, nullptr));
    }

    XPOINT ptGlobal = ConvertToIntPoint(pPointerEventArgs->GetGlobalPoint());

    IFC_RETURN(DoFinalManipulation(ptGlobal, bCaptureReleased, pPointerEventArgs->m_pPointer->m_uiPointerId));

    pPointerEventArgs->m_bHandled = TRUE;

    if (m_pPendingDoubleTapArgs)
    {
        IFC_RETURN(OnDoubleTapped(m_pPendingDoubleTapArgs));
    }

    if (m_pPendingTapArgs)
    {
        IFC_RETURN(OnTapped(m_pPendingTapArgs));
    }

    return S_OK;
}

//-------------------------------------------------------------------------
//
//  Synopsis:
//      Helper method called from OnPointerReleased or from
//      CRichEditGripper::OnHitTargetHolding.
//
//-------------------------------------------------------------------------
_Check_return_ HRESULT CTouchableRectangle::DoFinalManipulation(_In_ XPOINT ptGlobal, _In_ bool captureReleased, _In_ INT32 pointerId)
{
    CRichEditGripper* const pGripper = GetParentGripper();
    m_ptGlobLast = ptGlobal;

    if (pGripper && captureReleased)
    {
        // This is the final OnManipulation call for this touch session, so pass TRUE for bEnd.
        IFC_RETURN(pGripper->OnManipulation(false, true, ptGlobal.x, ptGlobal.y, pointerId));
    }

    return S_OK;
}

XPOINT CTouchableRectangle::ConvertToIntPoint(_In_ const XPOINTF& coordinate)
{
    XPOINT pixels;
    pixels.x = static_cast<XINT32>(coordinate.x);
    pixels.y = static_cast<XINT32>(coordinate.y);
    return pixels;
}

_Check_return_ HRESULT CTouchableRectangle::ReleaseTouchRectPointerCapture(_Out_opt_ bool *pbReleased, _Out_opt_ XINT32 *pPointerId)
{
    if (pbReleased)
    {
        *pbReleased = FALSE;
    }

    if (m_hasPointerCapture)
    {
        if (pbReleased)
        {
            *pbReleased = TRUE;
        }

        m_hasPointerCapture = false;
        IFC_RETURN(ReleasePointerCaptures());

        if (pPointerId)
        {
            *pPointerId = m_uiCapturePointerId;
        }

        m_uiCapturePointerId = -1;
    }

    return S_OK;
}

_Check_return_ HRESULT CTouchableRectangle::GetPointerCaptured(_Out_opt_ bool *pbCaptured)
{
    if (pbCaptured)
    {
        *pbCaptured = m_hasPointerCapture;
    }

    return S_OK;
}
