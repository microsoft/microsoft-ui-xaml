// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#pragma once

#include <DXamlServices.h>
#include <ComObject.h>

class CDependencyObject;

namespace DirectUI
{
    class DependencyObject;
}

namespace ctl
{
    template <typename tobject>
    struct IsCoreDependencyObject
    {
        static const bool value = std::is_base_of<::CDependencyObject, tobject>::value;
    };

    template <typename tobject>
    struct IsDependencyObject
    {
        static const bool value = std::is_base_of<::DirectUI::DependencyObject, tobject>::value;
    };

    // This variant of make will only work if tobject is a DependencyObject
    template <typename tobject>
    _Check_return_ typename std::enable_if<IsDependencyObject<tobject>::value, HRESULT>::type make(ctl::Internal::ComPtrRef<ComPtr<tobject>> ppNewInstance)
    {
        ctl::ComPtr<::DirectUI::DependencyObject> spInstance;
        IFC_RETURN(::DirectUI::DXamlServices::IsDXamlCoreInitialized() ? S_OK : RPC_E_WRONG_THREAD);
        IFC_RETURN(::DirectUI::DXamlServices::ActivatePeer(tobject::GetTypeIndexStatic(), &spInstance));
        auto ptr = ppNewInstance.ReleaseAndGetAddressOf();
        *ptr = static_cast<tobject*>(spInstance.Detach());
        return S_OK;
    }

    template <typename tobject>
    _Check_return_ typename std::enable_if<!IsDependencyObject<tobject>::value && IsComObject<tobject>::value, HRESULT>::type make(ctl::Internal::ComPtrRef<ComPtr<tobject>> ppNewInstance)
    {
        ctl::ComPtr<tobject> inst;
        IFC_RETURN(ComObject<tobject>::CreateInstance(inst.ReleaseAndGetAddressOf()));
        auto ptr = ppNewInstance.ReleaseAndGetAddressOf();
        *ptr = inst.Detach();
        return S_OK;
    }

    template <typename tobject>
    _Check_return_ typename std::enable_if<!IsDependencyObject<tobject>::value && IsComObject<tobject>::value, HRESULT>::type make_ignoreleak(ctl::Internal::ComPtrRef<ComPtr<tobject>> ppNewInstance)
    {
        ctl::ComPtr<tobject> inst;
        IFC_RETURN(ComObject<tobject>::CreateInstance(inst.ReleaseAndGetAddressOf(), /* fDisableLeakCheck */ TRUE));
        auto ptr = ppNewInstance.ReleaseAndGetAddressOf();
        *ptr = inst.Detach();
        return S_OK;
    }

    template <typename tobject>
    _Check_return_ typename std::enable_if<!IsComObject<tobject>::value, HRESULT>::type make(ctl::Internal::ComPtrRef<ComPtr<tobject>> ppNewInstance)
    {
        HRESULT hr = S_OK;
        tobject *pResult = NULL;

        static_assert(!ctl::IsEventPtrCompatible<tobject>::value && !ctl::IsWeakEventPtrCompatible<tobject>::value,
            "Use (Weak)EventPtr to Attach/Detach event handlers, do not create them by hand");

        pResult = new tobject();

        auto ptr = ppNewInstance.ReleaseAndGetAddressOf();
        *ptr = pResult;

        RRETURN(hr);//RRETURN_REMOVAL
    }

    template <typename tobject, typename t0>
    _Check_return_ HRESULT make(t0&& arg0, ctl::Internal::ComPtrRef<ComPtr<tobject>> ppNewInstance)
    {
        HRESULT hr = S_OK;

        IFC(make<tobject>(ppNewInstance));
        IFC((*ppNewInstance)->Initialize(std::forward<t0>(arg0)));

    Cleanup:

        RRETURN(hr);
    }

    template <typename tobject, typename t0, typename t1>
    _Check_return_ HRESULT make(t0&& arg0, t1&& arg1, ctl::Internal::ComPtrRef<ComPtr<tobject>> ppNewInstance)
    {
        HRESULT hr = S_OK;

        IFC(make<tobject>(ppNewInstance));
        IFC((*ppNewInstance)->Initialize(std::forward<t0>(arg0), std::forward<t1>(arg1)));

    Cleanup:

        RRETURN(hr);
    }

    template <typename tobject, typename t0, typename t1, typename t2>
    _Check_return_ HRESULT make(t0&& arg0, t1&& arg1, t2&& arg2, ctl::Internal::ComPtrRef<ComPtr<tobject>> ppNewInstance)
    {
        HRESULT hr = S_OK;

        IFC(make<tobject>(ppNewInstance));
        IFC((*ppNewInstance)->Initialize(std::forward<t0>(arg0), std::forward<t1>(arg1), std::forward<t2>(arg2)));

    Cleanup:

        RRETURN(hr);
    }

     template <typename tobject, typename t0, typename t1, typename t2, typename t3>
    _Check_return_ HRESULT make(t0&& arg0, t1&& arg1, t2&& arg2, t3&& arg3, ctl::Internal::ComPtrRef<ComPtr<tobject>> ppNewInstance)
    {
        HRESULT hr = S_OK;

        IFC(make<tobject>(ppNewInstance));
        IFC((*ppNewInstance)->Initialize(std::forward<t0>(arg0), std::forward<t1>(arg1), std::forward<t2>(arg2), std::forward<t3>(arg3)));

    Cleanup:

        RRETURN(hr);
    }

    template <typename tobject, typename t0>
    _Check_return_ HRESULT make_ignoreleak(t0&& arg0, ctl::Internal::ComPtrRef<ComPtr<tobject>> ppNewInstance)
    {
        HRESULT hr = S_OK;

        IFC(make_ignoreleak<tobject>(ppNewInstance));
        IFC((*ppNewInstance)->Initialize(std::forward<t0>(arg0)));

    Cleanup:

        RRETURN(hr);
    }

    // Thread-safe version of ctl::make, intended for use when initializing global static ComPtrs (e.g. StaticStore's members).
    // Note the reversed ordering of arguments compared to the standard ctl::make!
    template<typename tobject, typename... targs>
    _Check_return_ HRESULT make_threadsafe(_In_ ctl::Internal::ComPtrRef<ctl::ComPtr<tobject>> object, _In_ targs&&... args)
    {
        // Use a double-checked lock-free pattern to ensure the object is initialized, and retrieve it
        // If multiple threads see a 'nullptr' value for the object, then they all get to try to create
        // a non-null value for it. The first one to complete the InterlockedCompareExchangePointer call
        // gets to keep its created value; all other threads will discard their created values.
        if (!(*object))
        {
            ctl::ComPtr<tobject> tempObject;
            IFC_RETURN(ctl::make(std::forward<targs>(args)..., &tempObject));

            auto oldObject = (tobject*)InterlockedCompareExchangePointer(
                (void**)object.GetAddressOf(),
                (void*)tempObject.Get(),
                nullptr);

            if (!oldObject)
            {
                // This invocation won the race, so detach the temp smart pointer 'tempObject'.
                // Otherwise, the winning value will be automatically cleaned up when the
                // temp smart pointer 'tempObject' goes out of scope.
                // Essentially, if this invocation won (as indicated by a null value for 'oldObject', 
                // then 'tempObject' has transferred ownership of the created object to 'object'.
                tempObject.Detach();
            }
            else
            {
                // A non-null prior value indicates that this invocation lost the race.
                // Nothing needs to be done as the created object will be automatically
                // cleaned up by the smart pointer.
            }
        }

        return S_OK;
    }
}
