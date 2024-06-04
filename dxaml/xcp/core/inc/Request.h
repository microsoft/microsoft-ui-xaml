// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#pragma once

// The set of fixed token values.
#define REQUEST_CLR -1      // All CLR-attached event listeners get this token value.
#define REQUEST_INVALID -2  // Arbitrary marker to indicate a REQUEST does not have a valid token value.
#define REQUEST_INTERNAL -3 // All event listeners attached by internal code get this token value.

class CDependencyObject;

struct REQUEST
{
    CDependencyObject   *m_pListener;
    EventHandle          m_hEvent;
    CDependencyObject   *m_pObject;
    INTERNAL_EVENT_HANDLER m_pfnInternalEventDelegate;
    XINT32               m_iToken;          // Token value representing this REQUEST for use in Add/Remove events.
    XUINT8               m_bFired ;
    XUINT8               m_bAdded ;
    XUINT8               m_bHandledEventsToo;

    REQUEST()
    {
        m_pListener = NULL;
        m_pObject = NULL;
        m_pfnInternalEventDelegate = NULL;
        m_iToken = REQUEST_INVALID;
        m_bFired = FALSE;
        m_bAdded = FALSE;
        m_bHandledEventsToo = FALSE; // This flag is for invoking event handler even though event is handled
    }

    ~REQUEST()
    {
        Release();
    }

    void Release()
    {
        CDependencyObject *pDOTemp = NULL;

        ReleaseInterface(m_pListener);

        // Releasing the DO could cause re-entrance and could cause in double releasing of DO.
        // make a copy and release.
        pDOTemp = m_pObject;
        m_pObject = NULL;
        ReleaseInterface(pDOTemp);
        m_iToken = REQUEST_INVALID;
    }

    //= operator
    REQUEST& operator=(const REQUEST& req)
    {
        if (this != &req)
        {
            memcpy(this, static_cast<void*>(const_cast<REQUEST*>(&req)), sizeof(REQUEST));

            if (m_pListener)
            {
                AddRefInterface(m_pListener);
            }

            if (m_pObject)
            {
                AddRefInterface(m_pObject);
            }
        }
        return *this;
    }
};