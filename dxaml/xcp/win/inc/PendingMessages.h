// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#pragma once

class CXcpDispatcher;

//------------------------------------------------------------------------
//
//  Class:  CPendingMessage
//
//  Synopsis: Pending Message with associated resource for Dispatcher
//      
//------------------------------------------------------------------------

class CPendingMessage
{
public:
    CPendingMessage(_In_ CXcpDispatcher * pDispatcher, 
                            _In_ UINT uMsg, 
                            _In_ WPARAM wParam, 
                            _In_ LPARAM lParam);
    ~CPendingMessage();

    _Check_return_ HRESULT PostMessage();

public:   
    // Message
    UINT m_uMsg;

    // Message's wParam & lParam which holds associated resource
    WPARAM m_wParam;
    LPARAM m_lParam;
    
private: 
    // Dispatcher
    CXcpDispatcher *m_pDispatcher;

    // Has message been posted to queue?
    XINT32 m_bPosted;
};

//------------------------------------------------------------------------
//
//  Class:  CPendingMessages
//
//  Synopsis: List to hold pending dispatcher messages which have associated 
//      resources. Used at Dispatcher shutdown to free resources for 
//      pending messages.
//      
//------------------------------------------------------------------------

class CPendingMessages 
{
public:
    static _Check_return_ HRESULT Create(_In_ CXcpDispatcher* pDispatcher, _Outptr_ CPendingMessages** ppPendingMessages);

    ~CPendingMessages();

    _Check_return_ HRESULT PostMessage(_In_ UINT uMsg, 
                                _In_ WPARAM wParam, 
                                _In_ LPARAM lParam);
    _Check_return_ HRESULT ReleaseResource(_In_ UINT uMsg, 
                                _In_ WPARAM wParam, 
                                _In_ LPARAM lParam);
    void ReleaseResources();

private:
    CPendingMessages(_In_ CXcpDispatcher * pDispatcher);

    _Check_return_ HRESULT FindMessage(_In_ UINT uMsg, 
                    _In_ WPARAM wParam,
                    _In_ LPARAM lParam, 
                    _Out_ CPendingMessage **ppPendingMessage);
    
private:
    wil::critical_section m_CS;

    // Dispatcher
    CXcpDispatcher *m_pDispatcher;

    // Message list
    CXcpList<CPendingMessage> m_messageList;    
};
