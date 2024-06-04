// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#include "precomp.h"

#include <XcpWindow.h>

//------------------------------------------------------------------------
//
//  Method: CPendingMessage constructor
//
//------------------------------------------------------------------------

CPendingMessage::CPendingMessage(
    _In_ CXcpDispatcher * pDispatcher,
    _In_ UINT uMsg,
    _In_ WPARAM wParam,
    _In_ LPARAM lParam)
{
    XCP_STRONG(&m_pDispatcher);
    m_pDispatcher = pDispatcher;
    m_pDispatcher->AddRef();

    m_uMsg = uMsg;
    m_wParam = wParam;
    m_lParam = lParam;
    m_bPosted = FALSE;
}

//------------------------------------------------------------------------
//
//  Method: CPendingMessage destructor
//
//------------------------------------------------------------------------

CPendingMessage::~CPendingMessage()
{
    // If message was posted, free corresponding resources
    if (m_bPosted)
    {
        m_pDispatcher->ReleaseMessageResources(
                        m_uMsg,
                        m_lParam);
    }

    ReleaseInterface(m_pDispatcher);
}

//------------------------------------------------------------------------
//
//  Method: CPendingMessage PostMessage
//
//  Synopsis: Post message to dispatcher's queue
//------------------------------------------------------------------------

_Check_return_
HRESULT CPendingMessage::PostMessage()
{
    HRESULT hr = S_OK;

    // Posted already?
    ASSERT(!m_bPosted);

    IFCEXPECT(SUCCEEDED(m_pDispatcher->QueueDeferredInvoke(m_uMsg, m_wParam, m_lParam)));

    m_bPosted = TRUE;

Cleanup:
    return hr;
}

//------------------------------------------------------------------------
//
//  Method: CPendingMessages factory method
//
//------------------------------------------------------------------------

_Check_return_ HRESULT CPendingMessages::Create(_In_ CXcpDispatcher* pDispatcher, _Outptr_ CPendingMessages** ppPendingMessages)
{
    HRESULT hr = S_OK;

    CPendingMessages* pMessages = NULL;

    pMessages = new CPendingMessages(pDispatcher);

    *ppPendingMessages = pMessages;

    RRETURN(hr);//RRETURN_REMOVAL
}

//------------------------------------------------------------------------
//
//  Method: CPendingMessages constructor
//
//------------------------------------------------------------------------

CPendingMessages::CPendingMessages(
    _In_ CXcpDispatcher * pDispatcher)
{
    XCP_STRONG(&m_pDispatcher);
    m_pDispatcher = pDispatcher;
    m_pDispatcher->AddRef();
}

//------------------------------------------------------------------------
//
//  Method: CPendingMessages destructor
//
//------------------------------------------------------------------------

CPendingMessages::~CPendingMessages()
{
    ReleaseInterface(m_pDispatcher);
}

//------------------------------------------------------------------------
//
//  Method: CPendingMessages PostMessage
//
//  Synopsis: Post message with associated resource to dispatcher's queue
//      and add message to list of pending messages.
//
//      Code to release the resource must be added to
//      CXcpDispatcher::ReleaseMessageResources.
//------------------------------------------------------------------------

_Check_return_
HRESULT CPendingMessages::PostMessage(
    _In_ UINT uMsg,
    _In_ WPARAM wParam,
    _In_ LPARAM lParam)
{
    HRESULT hr = S_OK;
    CPendingMessage *pPendingMessage = NULL;
    XINT32 bAddedToList = FALSE;

    // Add to list of pending messages
    pPendingMessage =
            new CPendingMessage(m_pDispatcher, uMsg, wParam, lParam);

    // Scope the lock
    {
        auto Lock = m_CS.lock();

        IFC(m_messageList.Add(pPendingMessage));

        bAddedToList = TRUE;

        // Post message to dispatcher's queue. This must be done
        // before unlocking the message queue as otherwise it
        // could be removed and cleaned up if a ReleaseResources
        // call is made from another thread.
        IFC(pPendingMessage->PostMessage());
    }

Cleanup:
    if (FAILED(hr))
    {
        if (bAddedToList)
        {
            auto Lock = m_CS.lock();

            IGNOREHR(m_messageList.Remove(pPendingMessage, FALSE));
        }

        delete pPendingMessage;
    }

    return hr;
}

//------------------------------------------------------------------------
//
//  Method: CPendingMessages ReleaseResource
//
//  Synopsis: Release resource associated with message
//------------------------------------------------------------------------

_Check_return_
HRESULT CPendingMessages::ReleaseResource(
    _In_ UINT uMsg,
    _In_ WPARAM wParam,
    _In_ LPARAM lParam)
{
    HRESULT hr = S_OK;
    CPendingMessage *pPendingMessage = NULL;

    // Scope the lock
    {
        auto Lock = m_CS.lock();

        // Find message whose resources are to be released
        IFC_NOTRACE(FindMessage(uMsg, wParam, lParam, &pPendingMessage));

        // Resource is freed in message's destructor, when
        // message is removed from list
        IFC(m_messageList.Remove(pPendingMessage));
    }

Cleanup:
    return hr;
}

//------------------------------------------------------------------------
//
//  Method: CPendingMessages::ReleaseResources
//
//  Synopsis: Release resources for all messages
//------------------------------------------------------------------------

void CPendingMessages::ReleaseResources()
{
    auto Lock = m_CS.lock();

    // Resources are released in each message's destructor
    m_messageList.Clean();
}

//------------------------------------------------------------------------
//
//  Method: CPendingMessages FindMessage
//
//  Synopsis: Find a message in the list
//------------------------------------------------------------------------

_Check_return_ HRESULT CPendingMessages::FindMessage(
    _In_ UINT uMsg,
    _In_ WPARAM wParam,
    _In_ LPARAM lParam,
    _Out_ CPendingMessage **ppPendingMessage)
{
    HRESULT hr = S_OK;
    CXcpList<CPendingMessage>::XCPListNode *pNode;

    *ppPendingMessage = NULL;

    // Scope the lock
    {
        auto Lock = m_CS.lock();

        pNode = m_messageList.GetHead();
        IFCCHECK_NOTRACE(pNode);

        // Search for message
        while (pNode != NULL)
        {
            CPendingMessage * pPendingMessage = pNode->m_pData;
            ASSERT(pPendingMessage);

            if ((pPendingMessage->m_uMsg == uMsg)
                && (pPendingMessage->m_wParam == wParam)
                && (pPendingMessage->m_lParam == lParam))
            {
                // Found
                *ppPendingMessage = pPendingMessage;
                goto Cleanup;
            }

            pNode = pNode->m_pNext;
        }
    }

    // Search failed
    IFC_NOTRACE(E_FAIL);

Cleanup:
    return hr;
}

