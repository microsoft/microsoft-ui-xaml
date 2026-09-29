// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

//  Abstract:
//      A WinRT event source implementation suitable for use by FTM objects.

#pragma once

#include "GitHelper.h"
#include "ErrorHelper.h"
#include "refcounting.h"
#include "xref_ptr.h"

namespace DirectUI
{

template <class T>
class CGITCookie
{
public:
    static _Check_return_ HRESULT Create(_In_ T* pInterface, _Outptr_ CGITCookie** ppGITCookie)
    {
        HRESULT hr = S_OK;
        CGITCookie* pGITCookie = NULL;

        pGITCookie = new CGITCookie();

        IFC(pGITCookie->m_GIT.RegisterInterfaceInGlobal(pInterface, __uuidof(T), &pGITCookie->m_dwCookie));

        *ppGITCookie = pGITCookie;
        pGITCookie = NULL;

    Cleanup:
        ReleaseInterface(pGITCookie);

        RRETURN(hr);
    }

    XUINT32 AddRef()
    {
        return InterlockedIncrement(&m_cRef);
    }

    XUINT32 Release()
    {
        XUINT32 cRef = InterlockedDecrement(&m_cRef);

        if (0 == cRef)
        {
            delete this;
        }

        return cRef;
    }

    _Check_return_ HRESULT GetInterface(_Outptr_ T** ppInterface)
    {
        return m_GIT.GetInterfaceFromGlobal(m_dwCookie, __uuidof(T), reinterpret_cast<void**>(ppInterface));
    }

    DWORD GetCookie()
    {
        return m_dwCookie;
    }

private:
    CGITCookie() :
        m_cRef(1),
        m_dwCookie(0)
    {
    }

    ~CGITCookie()
    {
        if (0 != m_dwCookie)
        {
            // Typically a failure here happens because the apartment of the original interface pointer
            // has been destroyed.
            //
            // For example, an app sets an UnhandledException handler in an STA, then the STA
            // is destroyed, then the app object deinits in the MTA and tries to release all its event handlers.
            //
            // In practice, this should not happen much since most often event handlers are free-threaded.
            // We do see it in a couple of our own ABI scenario apps, since those typically don't use
            // free-threaded handlers.
            //
            // The only thing we can really do is ignore the failure.

            IGNOREHR(m_GIT.RevokeInterfaceFromGlobal(m_dwCookie));
        }
    }

    // uncopyable
    CGITCookie(const CGITCookie&);
    CGITCookie& operator=(const CGITCookie&);

    XUINT32   m_cRef;
    DWORD     m_dwCookie;
    GITHelper m_GIT;
};

template <class T>
class CGITCookieList
{
public:
    static _Check_return_ HRESULT Create(_Outptr_ CGITCookieList** ppGITCookieList)
    {
        HRESULT hr = S_OK;
        CGITCookieList* pList = NULL;

        pList = new CGITCookieList();

        *ppGITCookieList = pList;

        RRETURN(hr);//RRETURN_REMOVAL
    }

    static _Check_return_ HRESULT Copy(_In_ CGITCookieList* pListToCopy, _Outptr_ CGITCookieList** ppGITCookieList)
    {
        HRESULT hr = S_OK;
        CGITCookieList* pList = NULL;
        XUINT32 size = pListToCopy->m_list.size();

        IFC(Create(&pList));

        for (XUINT32 i = 0; i < size; i++)
        {
            if (pListToCopy->m_list[i])
            {
                IFC(pList->Add(pListToCopy->m_list[i]));
            }
        }

        *ppGITCookieList = pList;
        pList = NULL;

    Cleanup:
        delete pList;

        RRETURN(hr);
    }

    _Check_return_ HRESULT Add(_In_ CGITCookie<T>* pGITCookie)
    {
        HRESULT hr = S_OK;

        IFC(m_list.push_back(pGITCookie));
        AddRefInterface(pGITCookie);

    Cleanup:
        RRETURN(hr);
    }

    _Check_return_ HRESULT Remove(DWORD dwCookie, _Outptr_result_maybenull_ CGITCookie<T>** ppRemovedCookie)
    {
        HRESULT hr = S_OK;
        XUINT32 size = m_list.size();
        XUINT32 i = 0;

        *ppRemovedCookie = NULL;

        for (; i < size; i++)
        {
            if (m_list[i] && dwCookie == m_list[i]->GetCookie())
            {
                CGITCookie<T>* pGITCookie = m_list[i];
                IFC(m_list.erase(i));
                // The caller releases the cookie after leaving its mutation lock.
                *ppRemovedCookie = pGITCookie;
                break;
            }
        }

        // If the cookie wasn't found, treat this as a no-op rather than a failure. This can legitimately
        // happen if the event source was already cleared before the caller got a chance to
        // explicitly unsubscribe with the token it was given from Add().

    Cleanup:
        RRETURN(hr);
    }

    XUINT32 GetSize()
    {
        return m_list.size();
    }

    CGITCookie<T>* Get(XUINT32 i)
    {
        return m_list[i];
    }

    void Clear()
    {
        XUINT32 size = m_list.size();

        for (XUINT32 i = 0; i < size; i++)
        {
            ReleaseInterface(m_list[i]);
        }

        m_list.clear();
    }

    ~CGITCookieList()
    {
        Clear();
    }

private:
    CGITCookieList()
    {
    }

    xvector<CGITCookie<T>*> m_list;

    // uncopyable
    CGITCookieList(const CGITCookieList&);
    CGITCookieList& operator=(const CGITCookieList&);
};

template <class THANDLER, class TSOURCE, class TARGS>
class CFTMEventSource
{
public:
    CFTMEventSource() :
        m_fInitialized(FALSE)
    {
        BOOL fRet;
        HRESULT hr;

        fRet = InitializeCriticalSectionAndSpinCount(&m_csAddRemove, 0x80000001);
        if (0 == fRet)
        {
            return;
        }

        fRet = InitializeCriticalSectionAndSpinCount(&m_csRaise, 0x80000001);
        if (0 == fRet)
        {
            return;
        }

        hr = EnsureHandlerState();
        if (FAILED(hr))
        {
            return;
        }

        m_fInitialized = TRUE;
    }

    ~CFTMEventSource()
    {
        m_fInitialized = FALSE;
        HandlerState* pState = m_handlerState.detach();
        ReleaseInterface(pState);
        DeleteCriticalSection(&m_csAddRemove);
        DeleteCriticalSection(&m_csRaise);
    }

    _Check_return_ HRESULT Add(_In_ THANDLER* pHandler, _Out_ EventRegistrationToken* pToken)
    {
        HRESULT hr = S_OK;
        CGITCookie<THANDLER>* pGITCookie = NULL;

        IFCEXPECT(m_fInitialized);

        IFC(CGITCookie<THANDLER>::Create(pHandler, &pGITCookie));

        {
            Lock lock(m_csAddRemove);

            CGITCookieList<THANDLER>* pList = NULL;
            IFC(GetHandlersListForAddRemove(&pList));
            IFC(pList->Add(pGITCookie));
        }

        pToken->value = pGITCookie->GetCookie();

    Cleanup:
        ReleaseInterface(pGITCookie);

        RRETURN(hr);
    }

    _Check_return_ HRESULT Remove(EventRegistrationToken token)
    {
        HRESULT hr = S_OK;
        DWORD dwCookie = static_cast<DWORD>(token.value);
        CGITCookie<THANDLER>* pRemovedCookie = NULL;

        IFCEXPECT(m_fInitialized);

        {
            Lock lock(m_csAddRemove);

            if (m_handlerState)
            {
                CGITCookieList<THANDLER>* pList = NULL;
                IFC(GetHandlersListForAddRemove(&pList));
                IFC(pList->Remove(dwCookie, &pRemovedCookie));
            }
        }

    Cleanup:
        ReleaseInterface(pRemovedCookie);
        RRETURN(hr);
    }

    _Check_return_ HRESULT Raise(_In_ TSOURCE* pSource, _In_ TARGS* pArgs)
    {
        HRESULT hr = S_OK;
        Lock lock(m_csRaise);
        XUINT32 size = 0;
        THANDLER* pHandler = NULL;
        CGITCookie<THANDLER>* pGITCookie = NULL;
        xref_ptr<HandlerState> state;

        IFCEXPECT(m_fInitialized);

        {
            Lock lock2(m_csAddRemove);
            state = m_handlerState;
            if (state)
            {
                state->m_fInRaise = TRUE;
                size = state->m_pHandlers->GetSize();
            }
        }

        for (XUINT32 i = 0; i < size; i++)
        {
            {
                Lock lock2(m_csAddRemove);
                // A nested raise publishes its changes before the outer raise resumes.
                // Keep the entry-size limit, but account for a list that has since shrunk.
                if (i >= state->m_pHandlers->GetSize())
                {
                    break;
                }

                pGITCookie = state->m_pHandlers->Get(i);
                AddRefInterface(pGITCookie);
            }

            if (pGITCookie)
            {
                HRESULT getResult = pGITCookie->GetInterface(&pHandler);
                ReleaseInterface(pGITCookie);
                IFC(getResult);
                HRESULT invokeResult = pHandler->Invoke(pSource, pArgs);
                if (FAILED(invokeResult))
                {
                    IGNOREHR(ErrorHelper::ReportUnhandledError(invokeResult));
                }
                ReleaseInterface(pHandler);
            }
        }

    Cleanup:
        ReleaseInterface(pHandler);
        ReleaseInterface(pGITCookie);

        if (state)
        {
            CGITCookieList<THANDLER>* pRetiredHandlers = NULL;
            {
                Lock lock2(m_csAddRemove);
                if (state->m_pHandlersCopy)
                {
                    pRetiredHandlers = state->m_pHandlers;
                    state->m_pHandlers = state->m_pHandlersCopy;
                    state->m_pHandlersCopy = NULL;
                }
                // Route reentrant Add/Remove to the published list before releasing delegates.
                state->m_fInRaise = FALSE;
            }

            delete pRetiredHandlers;
        }

        // The retained state is released before m_csRaise, including on failure.
        RRETURN(hr);
    }

    void Clear()
    {
        if (!m_fInitialized)
        {
            return;
        }

        Lock raiseLock(m_csRaise);
        xref_ptr<HandlerState> retiredState;
        {
            Lock addRemoveLock(m_csAddRemove);
            // Active raises keep their shared state; new raises and registrations cannot see it.
            retiredState.attach(m_handlerState.detach());
        }
    }

private:
    struct HandlerState final : CXcpObjectBase<>
    {
        ~HandlerState() override
        {
            delete m_pHandlers;
            delete m_pHandlersCopy;
        }

        bool m_fInRaise = FALSE;
        CGITCookieList<THANDLER>* m_pHandlers = NULL;
        CGITCookieList<THANDLER>* m_pHandlersCopy = NULL;
    };

    // state
    bool m_fInitialized;
    xref_ptr<HandlerState> m_handlerState;

    // locks
    CRITICAL_SECTION m_csAddRemove;
    CRITICAL_SECTION m_csRaise;

    _Check_return_ HRESULT EnsureHandlerState()
    {
        if (!m_handlerState)
        {
            xref_ptr<HandlerState> state;
            state.attach(new HandlerState());
            IFC_RETURN(CGITCookieList<THANDLER>::Create(&state->m_pHandlers));
            m_handlerState.attach(state.detach());
        }

        return S_OK;
    }

    // Called under m_csAddRemove.
    _Check_return_ HRESULT GetHandlersListForAddRemove(_Outptr_ CGITCookieList<THANDLER>** ppList)
    {
        IFC_RETURN(EnsureHandlerState());

        if (m_handlerState->m_fInRaise)
        {
            if (!m_handlerState->m_pHandlersCopy)
            {
                IFC_RETURN(CGITCookieList<THANDLER>::Copy(m_handlerState->m_pHandlers, &m_handlerState->m_pHandlersCopy));
            }
            *ppList = m_handlerState->m_pHandlersCopy;
        }
        else
        {
            *ppList = m_handlerState->m_pHandlers;
        }

        return S_OK;
    }

    class Lock
    {
    public:
        Lock(CRITICAL_SECTION& cs) :
            m_cs(cs)
        {
            EnterCriticalSection(&m_cs);
        }

        ~Lock()
        {
            LeaveCriticalSection(&m_cs);
        }

    private:
        CRITICAL_SECTION& m_cs;
    };
};


}
