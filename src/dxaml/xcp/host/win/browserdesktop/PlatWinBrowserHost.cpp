// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#include "precomp.h"
#include "corep.h"
#include <XamlBehaviorMode.h>
#include <XamlIslandRoot.h>
#include <FeatureFlags.h>
//-------------------------------------------------------------------------
//
//  Function:   CXcpBrowserHost::HandlePointerMessage
//
//  Handle WM_POINTERXXX messages
//
//-------------------------------------------------------------------------
HRESULT
CXcpBrowserHost::HandlePointerMessage(_In_ XUINT32 uMsg, _In_ MsgPacket *pMsgPack, _Inout_ InputMessage *pMsg, _In_opt_ CContentRoot* contentRoot, _Out_ bool &fHandled)
{
    HRESULT hr = S_OK;

    pMsg->m_hWindow = pMsgPack->m_hwnd;
    switch(uMsg)
    {
        case WM_POINTERDOWN:
            pMsg->m_msgID = XCP_POINTERDOWN;
            break;
        case WM_POINTERUPDATE:
            pMsg->m_msgID = XCP_POINTERUPDATE;
            break;
        case WM_POINTERUP:
            pMsg->m_msgID = XCP_POINTERUP;
            break;
        case WM_POINTERENTER:
            pMsg->m_msgID = XCP_POINTERENTER;
            break;
        case WM_POINTERLEAVE:
            pMsg->m_msgID = XCP_POINTERLEAVE;
            break;
        case WM_POINTERWHEEL:
        case WM_POINTERHWHEEL:
            pMsg->m_msgID = XCP_POINTERWHEELCHANGED;
            pMsg->m_bIsSecondaryMessage = (uMsg == WM_POINTERHWHEEL);
            break;
        case WM_POINTERCAPTURECHANGED:
            pMsg->m_msgID = XCP_POINTERCAPTURECHANGED;
            break;
        case DM_POINTERHITTEST:
            pMsg->m_msgID = XCP_DMPOINTERHITTEST;
            break;
        case WM_POINTERROUTEDAWAY:
            pMsg->m_msgID = XCP_POINTERSUSPENDED;
            break;
        default:
            break;
    }

    if (uMsg == WM_POINTERDOWN ||
        uMsg == WM_POINTERUPDATE ||
        uMsg == WM_POINTERUP ||
        uMsg == WM_POINTERENTER ||
        uMsg == WM_POINTERLEAVE ||
        uMsg == WM_POINTERWHEEL ||
        uMsg == WM_POINTERHWHEEL ||
        uMsg == WM_POINTERCAPTURECHANGED ||
        uMsg == WM_POINTERROUTEDAWAY ||
        uMsg == DM_POINTERHITTEST)
    {
        const UINT32 pointerId = GET_POINTERID_WPARAM(pMsgPack->m_wParam);

        // Xaml now gets its input via WinRT events on an InputSite. A PointerPoint will always be available.
        ASSERT(pMsgPack->m_pPointerPointNoRef != nullptr);
        IFC_RETURN(m_pcs->GetPointerInfoFromPointerPoint(
            pMsgPack->m_pPointerPointNoRef,
            &pMsg->m_pointerInfo));

        // Disable touch down visualization in the full screen case, in an attempt to keep DirectFlip enabled as much as possible.
        if (IsFullScreen() && (uMsg == WM_POINTERDOWN))
        {
            DWM_SHOWCONTACT showContacts = DWMSC_ALL;

            if (m_pcs && m_pcs->HasActiveAnimations())
            {
                showContacts = DWMSC_NONE;
            }
            else
            {
                showContacts &= ~(DWMSC_DOWN | DWMSC_HOLD);
            }

            // Show everything except the down and hold visualizations.

            // Temporarily ignoring the return value because it returns
            // an error on OneCore and prevents us from handling PointerDowns
            // MSFT: Bug 813019 is tracking a proper solution (for e.g. an APISet extension)
            IGNOREHR(DwmShowContact(pointerId,
                                showContacts));
        }

        pMsg->m_pPointerPointNoRef = pMsgPack->m_pPointerPointNoRef;
        pMsg->m_pPointerEventArgsNoRef = pMsgPack->m_pPointerEventArgsNoRef;
        pMsg->m_isNonClientPointerMessage = pMsgPack->m_isNonClientPointerMessage;

        // Set the core window handle to register InputPane handler
        pMsg->m_pointerInfo.m_pCoreWindow = pMsgPack->m_pCoreWindow;
    }

#ifdef POINTER_TRACING
    gps->DebugOutputSzNoEndl(L"[InputPointer]: PointerInfo: MsgId=0x%x PointerId=%d Type=%d frameId=%d timestamp=%d\r\n",
                                uMsg,
                                pMsg->m_pointerInfo.m_pointerId,
                                pMsg->m_pointerInfo.m_pointerInputType,
                                pMsg->m_pointerInfo.m_frameId,
                                pMsg->m_pointerInfo.m_timeStamp);
#endif // POINTER_TRACING

    RRETURN(hr);
}

//-------------------------------------------------------------------------
//
//  Function:   CXcpBrowserHost::HandleActivateMessage
//
//  Handle WM_ACTIVATE message
//
//-------------------------------------------------------------------------
HRESULT
CXcpBrowserHost::HandleActivateMessage(_In_ XUINT32 uMsg, _In_ MsgPacket *pMsgPack, _Inout_ InputMessage *pMsg, _Out_ bool &fHandled)
{
    HRESULT hr = S_OK;

    ASSERT(uMsg == WM_ACTIVATE);

    IFCPTR(pMsgPack);
    IFCPTR(pMsg);

    pMsg->m_hWindow = pMsgPack->m_hwnd;
    if ((pMsgPack->m_wParam & WA_ACTIVE) != 0 || (pMsgPack->m_wParam & WA_CLICKACTIVE) != 0)
    {
        pMsg->m_msgID = XCP_ACTIVATE;
    }
    else
    {
        pMsg->m_msgID = XCP_DEACTIVATE;
    }

Cleanup:
    RRETURN(hr);
}
