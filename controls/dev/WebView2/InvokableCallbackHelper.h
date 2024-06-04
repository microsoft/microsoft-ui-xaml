// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#pragma once

namespace details
{
    // Used to produce a delegate with the Invoke() method signature that matches that of TDelegateInterface.
    template<typename TDelegateInterface> struct CallbackTraits; // to be specialized

    template<typename TDelegateInterface, typename ...TArgs>
    struct CallbackTraits<HRESULT(STDMETHODCALLTYPE TDelegateInterface::*)(TArgs...)>
    {

        template<typename TDelegateInterface>
        struct InvokeCallback : winrt::implements<InvokeCallback<TDelegateInterface>, TDelegateInterface>
        {
        private:
            std::function<HRESULT STDMETHODCALLTYPE(TArgs...)> m_invokeFunction;

        public:

            InvokeCallback() = delete;

            InvokeCallback(std::function<HRESULT STDMETHODCALLTYPE(TArgs...)>&& func_obj) : m_invokeFunction(func_obj) {}

            HRESULT STDMETHODCALLTYPE Invoke(TArgs... args) override
            {
                return m_invokeFunction(args...);
            }
        };
    };
}

// This callback type provides a winrt-style implementation of the COM delegate interface/ABI that is non-reliant on WRL,
// but can be used as a direct replacement for 'WRL::Callback's.
template<typename TDelegateInterface, typename TLambda>
auto InvokableCallback(TLambda&& callback)
{
    using InvokeSignature = decltype(&TDelegateInterface::Invoke);
    using CallbackType = typename details::CallbackTraits<InvokeSignature>::template InvokeCallback<TDelegateInterface>;
    return winrt::make<CallbackType>(std::forward<TLambda>(callback));
}
