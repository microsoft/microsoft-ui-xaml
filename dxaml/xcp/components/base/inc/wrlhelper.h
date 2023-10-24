// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#pragma once

namespace WRLHelper 
{
    // Creates an agile callback function. This means that the completion callback can
    // be called on the async thread rather than the originating thread. It is important
    // to remember that the default behavior for the callback template is to create
    // a proxied callback which must execute on the originating thread.
    template<typename TDelegateInterface, typename TCallback>
    ::Microsoft::WRL::ComPtr<TDelegateInterface> MakeAgileCallback(TCallback callback) throw()
    {
        return ::Microsoft::WRL::Callback <
            ::Microsoft::WRL::Implements <
            ::Microsoft::WRL::RuntimeClassFlags<::Microsoft::WRL::ClassicCom>,
            TDelegateInterface,
            ::Microsoft::WRL::FtmBase >> (callback);
    }

    // Creates an regular callback function. This means that the completion callback is
    // called on the originating thread. 
    template<typename TDelegateInterface, typename TCallback>
    ::Microsoft::WRL::ComPtr<TDelegateInterface> MakeCallback(TCallback callback) throw()
    {
        return ::Microsoft::WRL::Callback <
            ::Microsoft::WRL::Implements <
            ::Microsoft::WRL::RuntimeClassFlags<::Microsoft::WRL::ClassicCom>,
            TDelegateInterface >> (callback);
    }

    // Creates an regular callback function. This means that the completion callback is
    // called on the originating thread. 
    template<typename TDelegateInterface, typename TCallbackObject>
    ::Microsoft::WRL::ComPtr<TDelegateInterface> MakeCallback2(TCallbackObject* object, HRESULT (TCallbackObject::* callback)()) throw()
    {
        return ::Microsoft::WRL::Callback <
            ::Microsoft::WRL::Implements <
            ::Microsoft::WRL::RuntimeClassFlags<::Microsoft::WRL::ClassicCom>,
            TDelegateInterface >> (object, callback);
    }
}
