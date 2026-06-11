// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

//  Abstract:
//      Templates used to help with the implementation of COM objects

#pragma once

namespace ctl
{
    // Common class to implemente IUnknown
    template<class TINTERFACE>
    class __declspec(novtable) implements: public TINTERFACE
    {
    protected:
        // This class is marked novtable, so must not be instantiated directly.
        implements(): m_cRef(1) {}

        explicit implements(_In_ ULONG initialRefCount)
            : m_cRef(initialRefCount)
        {}

    public:
        virtual ~implements()
        {}

        public:
        IFACEMETHODIMP QueryInterface(_In_ REFIID riid, _Outptr_ void** ppvObject) override
        {
            IUnknown *pResult = NULL;

            if (InlineIsEqualGUID(riid, IID_IUnknown))
            {
                pResult = static_cast<IUnknown*>(this);
            }
            else if (InlineIsEqualGUID(riid, __uuidof(TINTERFACE)))
            {
                pResult = static_cast<TINTERFACE*>(this);
            }
            // Any other interface needs to be handled by the derived class
            else
            {
                IFC_NOTRACE_RETURN(E_NOINTERFACE);
            }

            pResult->AddRef();
            *ppvObject = (void*)pResult;

            return S_OK;
        }

        IFACEMETHODIMP_(ULONG) AddRef() override
        {
            return InterlockedIncrement(&m_cRef);
        }

        IFACEMETHODIMP_(ULONG) Release() override
        {
            ULONG result = InterlockedDecrement(&m_cRef);
            if (result == 0)
            {
                delete this;
            }

            return result;
        }

    private:
        ULONG m_cRef;
    };

    template <class TINTERFACE>
    class implements_inspectable:
        public implements<TINTERFACE>
    {

    public:

        STDMETHODIMP QueryInterface(_In_ REFIID iid, _Outptr_ void ** ppvObject) override
        {
            IUnknown *pResult = NULL;

            if (InlineIsEqualGUID(iid, __uuidof(IInspectable)))
            {
                pResult = static_cast<IInspectable*>(this);
            }
            else
            {
                return implements<TINTERFACE>::QueryInterface(iid, ppvObject);
            }

            pResult->AddRef();
            *ppvObject = (void*)pResult;

            return S_OK;
        }

        STDMETHODIMP GetRuntimeClassName(_Out_ HSTRING *pClassName) override
        {
            *pClassName = nullptr;
            return S_OK;
        }

        STDMETHODIMP GetTrustLevel(_Out_ TrustLevel *trustLvl) override
        {
            return E_NOTIMPL;
        }

        STDMETHODIMP GetIids(_Out_ ULONG *iidCount, _Outptr_ IID **iids) override
        {
            return E_NOTIMPL;
        }
    };
}
