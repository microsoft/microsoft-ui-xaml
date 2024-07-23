// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#pragma once

#include "ComObjectBase.h"

namespace DirectUI { class DependencyObject; }

namespace ctl
{
    template <typename T> DirectUI::DependencyObject* CreateComObjectInstanceNoInit(_In_ IInspectable* const pOuter);

    template <class TBASE>
    class ComObject final :
        public ComObjectBase,
        public TBASE
    {
        friend DirectUI::DependencyObject* CreateComObjectInstanceNoInit<TBASE>(_In_ IInspectable* const pOuter);

    private:
        ComObject(_In_ IInspectable* pOuter) : ComObjectBase(pOuter)
        {
        }

    public:
        // IInspectable (non-delegating) implementation
        IFACEMETHODIMP NonDelegatingQueryInterface(REFIID iid, void **ppValue) final
        {
            return NonDelegatingQueryInterfaceBase(iid, ppValue);
        }

        // IInspectable (delegating) implementation
        IFACEMETHODIMP QueryInterface(REFIID iid, void **ppValue) final
        {
            return QueryInterfaceBase(iid, ppValue);
        }

        HRESULT QueryInterfaceImplBase(_In_ REFIID iid, _Outptr_ void** ppObject) final
        {
            return TBASE::QueryInterfaceImpl(iid, ppObject);
        }

        INonDelegatingUnknown* AsNonDelegatingInspectable() final
        {
            return (m_pControllingUnknown != nullptr) ? this : nullptr;
        }

        IFACEMETHODIMP_(ULONG) NonDelegatingAddRef() final
        {
            return TBASE::AddRefImpl();
        }

        IFACEMETHODIMP_(ULONG) NonDelegatingRelease() final
        {
            return TBASE::ReleaseImpl();
        }

        IFACEMETHODIMP NonDelegatingGetRuntimeClassName(_Out_ HSTRING *pClassName) final
        {
            return TBASE::GetRuntimeClassNameImpl(pClassName);
        }

        IFACEMETHODIMP NonDelegatingGetTrustLevel(_Out_ TrustLevel *trustLvl) final
        {
            *trustLvl = BaseTrust; // Every XAML object currently is BaseTrust
            return S_OK;
        }

        IFACEMETHODIMP NonDelegatingGetIids(_Out_ ULONG *iidCount, _Outptr_ IID **iids) final
        {
            return TBASE::GetIidsImpl(iidCount, iids);
        }

        IFACEMETHODIMP_(ULONG) AddRef() final
        {
            if (m_pControllingUnknown)
            {
                return m_pControllingUnknown->AddRef();
            }
            else
            {
                return TBASE::AddRefImpl();
            }
        }

        IFACEMETHODIMP_(ULONG) Release() final
        {
            if (m_pControllingUnknown)
            {
                return m_pControllingUnknown->Release();
            }
            else
            {
                return TBASE::ReleaseImpl();
            }
        }

        IFACEMETHODIMP GetRuntimeClassName(_Out_ HSTRING *pClassName) final
        {
            if (m_pControllingUnknown)
            {
                return m_pControllingUnknown->GetRuntimeClassName(pClassName);
            }
            else
            {
                return TBASE::GetRuntimeClassNameImpl(pClassName);
            }
        }

        IFACEMETHODIMP GetTrustLevel(_Out_ TrustLevel *trustLvl) final
        {
            if (m_pControllingUnknown)
            {
                return m_pControllingUnknown->GetTrustLevel(trustLvl);
            }
            else
            {
                *trustLvl = BaseTrust; // Every XAML object is BaseTrust currently
                return S_OK;
            }
        }

        IFACEMETHODIMP GetIids(_Out_ ULONG *iidCount, _Outptr_result_buffer_all_maybenull_(*iidCount) IID **iids) final
        {
            if (m_pControllingUnknown)
            {
                return m_pControllingUnknown->GetIids(iidCount, iids);
            }
            else
            {
                return TBASE::GetIidsImpl(iidCount, iids);
            }
        }

    public:

        static _Check_return_ HRESULT CreateInstance(_Outptr_ IInspectable **ppNewInstance)
        {
            return CreateInstance(NULL, ppNewInstance);
        }

        static _Check_return_ HRESULT CreateInstance(_In_ IInspectable* pOuter, _Outptr_ IInspectable **ppNewInstance)
        {
            HRESULT hr = S_OK;
            ComObject* pNewInstance = new ComObject<TBASE>(pOuter);
            IFC(ctl::ComObjectBase::CreateInstanceBase(pNewInstance));
            *ppNewInstance = static_cast<IInspectable *>(static_cast<ComBase *>(pNewInstance));
            pNewInstance = NULL;

        Cleanup:
            ReleaseInterface(pNewInstance);
            RRETURN(hr);
        }

        template <class T>
        static _Check_return_ HRESULT CreateInstance(_Outptr_ T **ppNewInstance, bool fDisableLeakCheck = false)
        {
            return CreateInstance(NULL, ppNewInstance, fDisableLeakCheck);
        }

        template <class T>
        static _Check_return_ HRESULT CreateInstance(_In_ IInspectable* pOuter, _Outptr_ T **ppNewInstance, bool fDisableLeakCheck = false)
        {
            HRESULT hr = S_OK;
            ComObject<TBASE>* pNewInstance = new ComObject<TBASE>(pOuter);
            IFC(ctl::ComObjectBase::CreateInstanceBase(pNewInstance));

#if XCP_MONITOR && DBG
            if (fDisableLeakCheck)
            {
                // TODO: Figure out why this crashes.
                ::XcpDebugSetLeakDetectionFlag(pNewInstance, true);
                pNewInstance->DisableLeakCheck();
            }
#endif

            *ppNewInstance = static_cast<T *>(pNewInstance);
            pNewInstance = NULL;

        Cleanup:
            ReleaseInterface(pNewInstance);
            RRETURN(hr);
        }
    };

    template <typename T>
    DirectUI::DependencyObject* CreateComObjectInstanceNoInit(_In_ IInspectable* const pOuter)
    {
        return static_cast<DirectUI::DependencyObject*>(new ComObject<T>(pOuter));
    }
}
