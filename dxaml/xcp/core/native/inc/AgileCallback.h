// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#pragma once

#include <WRLHelper.h>

template<typename TDelegateInterface, typename TCallbackObject, typename TArg1, typename TArg2>
Microsoft::WRL::ComPtr<typename Microsoft::WRL::Details::ArgTraitsHelper<TDelegateInterface>::Interface>
DispatcherCallback(
    _In_ TCallbackObject *pObject,
    _In_ HRESULT (TCallbackObject::* pMethod)(TArg1*, TArg2*)) noexcept
{
    static_assert(__is_base_of(IUnknown, TDelegateInterface) && !__is_base_of(IInspectable, TDelegateInterface), "Delegates objects must be 'IUnknown' base and not 'IInspectable'");
    static_assert(Microsoft::WRL::Details::ArgTraitsHelper<TDelegateInterface>::Traits::args == 2, "Number of arguments on object method doesn't match number of arguments on Delegate::Invoke");
    static_assert(Microsoft::WRL::Details::IsSame<TArg1*, typename Microsoft::WRL::Details::ArgTraitsHelper<TDelegateInterface>::Traits::Arg1Type>::value, "Argument 1 from object method doesn't match Invoke argument 1");
    static_assert(Microsoft::WRL::Details::IsSame<TArg2*, typename Microsoft::WRL::Details::ArgTraitsHelper<TDelegateInterface>::Traits::Arg2Type>::value, "Argument 2 from object method doesn't match Invoke argument 2");

    typedef HRESULT (TCallbackObject::* TCallbackMethod)(TArg1*, TArg2*);

    struct ComObject : wrl::RuntimeClass<wrl::RuntimeClassFlags<wrl::Delegate>, TDelegateInterface, wrl::FtmBase>
    {
        ComObject(TCallbackObject *pDO, TCallbackMethod pMethod) noexcept
            : m_wpDO(xref::get_weakref(pDO)), m_pMethod(pMethod)
        {
            ctl::ComPtr<msy::IDispatcherQueueStatics> spDispatcherQueueStatics;
            IFCFAILFAST(wf::GetActivationFactory(wrl_wrappers::HStringReference(RuntimeClass_Microsoft_UI_Dispatching_DispatcherQueue).Get(),
                spDispatcherQueueStatics.ReleaseAndGetAddressOf()));
            IFCFAILFAST(spDispatcherQueueStatics->GetForCurrentThread(&m_spDispatcherQueue));
            if (!m_spDispatcherQueue)
            {
                IFCFAILFAST(E_UNEXPECTED);
            }
        }

        STDMETHOD(Invoke)(TArg1* arg1, TArg2* arg2)
        {
            ctl::ComPtr<TArg1> spArg1(arg1);
            ctl::ComPtr<TArg2> spArg2(arg2);
            xref::weakref_ptr<TCallbackObject> wpDO(m_wpDO);
            TCallbackMethod pMethod(m_pMethod);
            auto callback = WRLHelper::MakeAgileCallback<msy::IDispatcherQueueHandler>([wpDO, pMethod, spArg1, spArg2]() -> HRESULT
            {
                xref_ptr<TCallbackObject> spDO = wpDO.lock();
                if (spDO)
                {
                    return ((spDO.get())->*pMethod)(spArg1.Get(), spArg2.Get());
                }
                else
                {
                    return S_OK;
                }
            });

            boolean enqueued;
            IFC_RETURN(m_spDispatcherQueue->TryEnqueue(
                    callback.Get(),
                    &enqueued));
            IFCEXPECT_RETURN(enqueued);
            return S_OK;
        }

    private:
        ctl::ComPtr<msy::IDispatcherQueue> m_spDispatcherQueue;
        xref::weakref_ptr<TCallbackObject> m_wpDO;
        TCallbackMethod m_pMethod;
    };

    return Microsoft::WRL::Make<ComObject>(pObject, pMethod);
}


