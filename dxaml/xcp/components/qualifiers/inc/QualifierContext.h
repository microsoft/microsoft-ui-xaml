// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#pragma once
#include <functional>
#include "QualifierFlags.h"
#include "IQualifierContextCallback.h"

// QualifierContext handles global context for qualifiers, persisting window size,
// and making available m_platform value. Listeners can register for specific context events by
// calling RegisterChangedCallback with flags set for context dimensions of interest.
class QualifierContext
{
    public:
        // Registers a callback and subscribes to all events callback is registering for
        // If the same callback is registered more than once, the most recent flags setting
        // is taken.  If no flags are set, the callback is unregistered.
        _Check_return_ HRESULT RegisterChangedCallback(_In_ IQualifierContextCallback*, _In_ QualifierFlags flags);

        HRESULT OnWindowChanged(_In_ XUINT32 width, _In_ XUINT32 height);

        XUINT32 m_windowWidth = static_cast<XUINT32>(-1);
        XUINT32 m_windowHeight = static_cast<XUINT32>(-1);

    private:
        static void CallIfRegistered(_In_ QualifierFlags flags, _In_ std::pair<IQualifierContextCallback*, QualifierFlags>& v);

        std::vector<std::pair<IQualifierContextCallback*, QualifierFlags> > m_listeners;
        std::vector<std::pair<IQualifierContextCallback*, QualifierFlags> > m_listenersPendingCallback;
};

