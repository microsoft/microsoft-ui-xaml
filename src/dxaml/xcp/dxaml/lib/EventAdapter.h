// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

//  Abstract:
//      Defines a class template EventAdapter. This is used when an "outer"
//      object is wrapping an "inner" object, and the outer wants to forward
//      an event from the inner, and the outer event will have a different
//      signature than the inner event.
//
//      The adapter implements the handler interface required by the inner
//      event. The adapter references an outer event handler and a wrapper
//      instance. When the inner event is raised, the adapter invokes a
//      callback function, passing the inner sender and params, the outer
//      handler, and the wrapper instance. The callback function would
//      then invoke the outer handler, re-packaging the sender and params
//      as needed.
//
//      The adapter aggregates the FTM. This ensures that the adapter is
//      "transparent" with regard to the threading policy of the event
//      and the outer handler: the adapter does not add any marshaling of
//      its own.

#pragma once

#include "GitHelper.h"

namespace DirectUI
{

template <class T_INNER_HANDLER,
          class T_OUTER_HANDLER>
class EventAdapterBase : public T_INNER_HANDLER
{
public:
    HRESULT STDMETHODCALLTYPE QueryInterface(
        REFIID riid,
        __RPC__deref_out  void **ppvObject) override
    {
        *ppvObject = NULL;

        if (InlineIsEqualGUID(riid, IID_IUnknown))
        {
            *ppvObject = static_cast<IUnknown*>(this);
        }
        else if (InlineIsEqualGUID(riid, __uuidof(T_INNER_HANDLER)))
        {
            *ppvObject = static_cast<T_INNER_HANDLER*>(this);
        }
        else if (InlineIsEqualGUID(riid, IID_IMarshal))
        {
            return m_pUnkFTM->QueryInterface(riid, ppvObject);
        }
        else
        {
            return E_NOINTERFACE;
        }

        AddRef();
        return S_OK;
    }

    ULONG STDMETHODCALLTYPE AddRef( void) override
    {
        return InterlockedIncrement(&m_cRef);
    }

    ULONG STDMETHODCALLTYPE Release( void) override
    {
        LONG cRef = InterlockedDecrement(&m_cRef);

        if (0 == cRef)
        {
            delete this;
        }

        return cRef;
    }

protected:
    _Check_return_ HRESULT GetOuterHandler(_Outptr_ T_OUTER_HANDLER** ppHandler)
    {
        return m_GIT.GetInterfaceFromGlobal(m_dwOuterHandlerCookie, __uuidof(T_OUTER_HANDLER), reinterpret_cast<void**>(ppHandler));
    }

    EventAdapterBase(
        _In_ T_OUTER_HANDLER* pOuterHandler,
        _Out_ HRESULT* phr) :
        m_cRef(1),
        m_pUnkFTM(NULL),
        m_dwOuterHandlerCookie(0)
    {
        HRESULT hr = S_OK;

        IFC(CoCreateFreeThreadedMarshaler(this, &m_pUnkFTM));

        IFC(m_GIT.RegisterInterfaceInGlobal(pOuterHandler, __uuidof(T_OUTER_HANDLER), &m_dwOuterHandlerCookie));

    Cleanup:
        *phr = hr;
    }

    ~EventAdapterBase()
    {
        ReleaseInterface(m_pUnkFTM);

        // Note: failures here can be ignored - typically they indicate the apartment the
        // interface pointer belongs to has gone away.
        if (0 != m_dwOuterHandlerCookie)
        {
            IGNOREHR(m_GIT.RevokeInterfaceFromGlobal(m_dwOuterHandlerCookie));
        }
    }

private:
    ULONG     m_cRef;
    IUnknown* m_pUnkFTM;
    DWORD     m_dwOuterHandlerCookie;
    GITHelper m_GIT;
};

template <class T_INNER_HANDLER,
          class T_OUTER_HANDLER,
          class T_INNER_ARG_1,
          class T_INNER_ARG_2>
class EventAdapter : public EventAdapterBase<T_INNER_HANDLER, T_OUTER_HANDLER>
{
    typedef _Check_return_ HRESULT (InvokeCallbackFn)(
        _In_ T_INNER_ARG_1* pInnerArg1,
        _In_ T_INNER_ARG_2* pInnerArg2,
        _In_ T_OUTER_HANDLER* pOuterHandler);

public:
    static _Check_return_ HRESULT Create(
        _In_ T_OUTER_HANDLER* pOuterHandler,
        _In_ InvokeCallbackFn* pfnInvokeCallback,
        _Outptr_ EventAdapter** ppAdapter)
    {
        HRESULT hr = S_OK;
        EventAdapter* pAdapter = NULL;

        pAdapter = new EventAdapter(pOuterHandler, pfnInvokeCallback, &hr);
        IFC(hr);

        *ppAdapter = pAdapter;
        pAdapter = NULL;

    Cleanup:
        ReleaseInterface(pAdapter);

        RRETURN(hr);
    }

    IFACEMETHODIMP Invoke(_In_ T_INNER_ARG_1* pInnerArg1, _In_ T_INNER_ARG_2* pInnerArg2) override
    {
        HRESULT hr = S_OK;
        T_OUTER_HANDLER* pOuterHandler = NULL;

        IFC(this->GetOuterHandler(&pOuterHandler));

        IFC(m_pfnInvokeCallback(pInnerArg1, pInnerArg2, pOuterHandler));

    Cleanup:
        ReleaseInterface(pOuterHandler);

        RRETURN(hr);
    }

private:
    EventAdapter(
        _In_ T_OUTER_HANDLER* pOuterHandler,
        _In_ InvokeCallbackFn* pfnInvokeCallback,
        _Out_ HRESULT* phr) :
        EventAdapterBase<T_INNER_HANDLER, T_OUTER_HANDLER>(pOuterHandler, phr),
        m_pfnInvokeCallback(pfnInvokeCallback)
    {
    }

    InvokeCallbackFn* m_pfnInvokeCallback;
};

}
