// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#include "precomp.h"
#include <QualifierContext.h>
#include <memory>
#include <functional>

// Public methods

// Registers a callback and subscribes to all events callback is registering for 
// If the same callback is registered more than once, the most recent flags setting
// is taken.  If no flags are set, the callback is unregistered. 
_Check_return_ HRESULT QualifierContext::RegisterChangedCallback(_In_ IQualifierContextCallback* callback, _In_ QualifierFlags flags)
{

    auto comparator = [&](std::pair<IQualifierContextCallback*, QualifierFlags> a)
    { return a.first == callback; };

    auto position = std::find_if(begin(m_listeners), end(m_listeners), comparator);
    if(position != m_listeners.end())
    {

        // Remove listener : This may occur inside an OnWindowChanged call 
        if(flags == QualifierFlags::None)
        {
            m_listeners.erase(position);

            // Remove listener from pending callback list
            auto foundPending = std::find_if(begin(m_listenersPendingCallback), end(m_listenersPendingCallback), comparator);
            if(foundPending != m_listenersPendingCallback.end()) m_listenersPendingCallback.erase(foundPending);

            return S_OK;
        }

        position->second = flags;
    }
    else
    {
        m_listeners.push_back(std::make_pair(callback, flags));
    }
    
    return S_OK;
}

_Check_return_ HRESULT QualifierContext::OnWindowChanged(_In_ XUINT32 width, _In_ XUINT32 height)
{
    QualifierFlags flags = QualifierFlags::None;

    // If width changed, persist the new value, callback all listeners that registered for width changed event
    if(m_windowWidth != width)
    {
        m_windowWidth = width;
        flags = flags | QualifierFlags::Width;
    }

    // If height changed, persist the new value, callback all listeners that registered for width changed event
    if(m_windowHeight != height)
    {
        m_windowHeight = height;
        flags = flags | QualifierFlags::Height;
    }

    // If we received a spurious WM_SIZE, ignore and return
    if(flags == QualifierFlags::None) return S_OK;

    // Copy listeners to listenersPendingCallback
    m_listenersPendingCallback = m_listeners;

    // Callback all listeners
    while(!m_listenersPendingCallback.empty())
    {
        auto current = m_listenersPendingCallback.back();
        m_listenersPendingCallback.pop_back();
        CallIfRegistered(flags, current);
    }
    
    return S_OK;
};

// Private methods

void QualifierContext::CallIfRegistered(_In_ QualifierFlags flags, _In_ std::pair<IQualifierContextCallback*, _In_ QualifierFlags>& v) 
{
    // If listener has registed for event then call it 
    if((v.second & flags) != QualifierFlags::None) 
    { 
        if(v.first)
        {
            VERIFYHR(v.first->OnQualifierContextChanged());
        }
    }
};

