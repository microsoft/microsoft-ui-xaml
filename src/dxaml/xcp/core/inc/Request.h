// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#pragma once

struct REQUEST
{
    EventHandle          m_hEvent;
    INTERNAL_EVENT_HANDLER m_pfnInternalEventDelegate = nullptr;
    XUINT8               m_bFired = FALSE;
    XUINT8               m_bActive = FALSE;
    XUINT8               m_bCanFireWhenInactive = FALSE;
    XUINT8               m_bHandledEventsToo; // This flag is for invoking event handler even though event is handled 

    REQUEST()
        : m_bHandledEventsToo(FALSE)
    {
    }

    REQUEST(EventHandle hEvent, bool handledEventsToo)
        : m_hEvent(hEvent)
        , m_bHandledEventsToo(handledEventsToo)
    {
    }
};