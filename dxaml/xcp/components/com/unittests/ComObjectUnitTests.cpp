// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#include "precomp.h"
#include "ComObjectUnitTests.h"

#include <ComBase.h>
#include <ComObject.h>
#include <ComPtr.h>
#include <memory>
#include <string>

using namespace ctl;

namespace Windows { namespace UI { namespace Xaml { namespace Tests { namespace Com {

    class NamedInspectable : public ComBase
    {
    public:
        static constexpr WCHAR RuntimeName[] = L"ComObjectUnitTests.NamedInspectable";
        INSPECTABLE_CLASS(RuntimeName);
    };

    class EmptyNameInspectable : public ComBase
    {
    public:
        static constexpr WCHAR RuntimeName[] = L"";
        INSPECTABLE_CLASS(RuntimeName);
    };

    class EmbeddedNullNameInspectable : public ComBase
    {
    public:
        static constexpr WCHAR RuntimeName[] = L"ComObjectUnitTests.\u03A9\0Suffix";
        INSPECTABLE_CLASS(RuntimeName);
    };

    class OuterInspectable : public ComBase
    {
    public:
        static constexpr WCHAR RuntimeName[] = L"ComObjectUnitTests.OuterInspectable";
        INSPECTABLE_CLASS(RuntimeName);
    };

    class IidResultInspectable : public ComBase
    {
    public:
        HRESULT Result = S_FALSE;
        ULONG* LastCount = nullptr;
        IID** LastIids = nullptr;

        HRESULT GetIidsImpl(_Out_ ULONG* iidCount, _Outptr_ IID** iids)
        {
            LastCount = iidCount;
            LastIids = iids;
            if (SUCCEEDED(Result))
            {
                *iidCount = 0;
                *iids = nullptr;
            }
            return Result;
        }
    };

    static void ValidateIidResults(IInspectable* instance, IidResultInspectable* implementation)
    {
        ULONG count = 7;
        IID sentinel = IID_IUnknown;
        IID* iids = &sentinel;
        VERIFY_ARE_EQUAL(S_FALSE, instance->GetIids(&count, &iids));
        VERIFY_ARE_EQUAL(0ul, count);
        VERIFY_IS_NULL(iids);
        VERIFY_IS_TRUE(implementation->LastCount == &count);
        VERIFY_IS_TRUE(implementation->LastIids == &iids);

        implementation->Result = E_FAIL;
        count = 7;
        iids = &sentinel;
        VERIFY_ARE_EQUAL(E_FAIL, instance->GetIids(&count, &iids));
        VERIFY_ARE_EQUAL(7ul, count);
        VERIFY_IS_TRUE(iids == &sentinel);
        VERIFY_IS_TRUE(implementation->LastCount == &count);
        VERIFY_IS_TRUE(implementation->LastIids == &iids);
    }

    template<typename T>
    static void ValidateRuntimeClassName()
    {
        ctl::ComPtr<T> instance;
        THROW_IF_FAILED(ComObject<T>::CreateInstance(instance.ReleaseAndGetAddressOf()));

        #pragma warning(suppress: 6387) // Exercise the invalid output pointer.
        VERIFY_ARE_EQUAL(E_INVALIDARG, instance->GetRuntimeClassName(nullptr));

        wrl_wrappers::HString retainedName;
        {
            wrl_wrappers::HString name;
            THROW_IF_FAILED(instance->GetRuntimeClassName(name.GetAddressOf()));
            THROW_IF_FAILED(name.CopyTo(retainedName.GetAddressOf()));
        }

        wrl_wrappers::HString secondName;
        THROW_IF_FAILED(instance->GetRuntimeClassName(secondName.GetAddressOf()));
        instance.Reset();

        const std::wstring expectedName(T::RuntimeName, _countof(T::RuntimeName) - 1);
        UINT32 length = 0;
        const WCHAR* text = retainedName.GetRawBuffer(&length);
        VERIFY_ARE_EQUAL(expectedName, std::wstring(text, length));
        text = secondName.GetRawBuffer(&length);
        VERIFY_ARE_EQUAL(expectedName, std::wstring(text, length));

        if constexpr (_countof(T::RuntimeName) == 1)
        {
            VERIFY_IS_NULL(retainedName.Get());
            VERIFY_IS_NULL(secondName.Get());
        }
    }

    void ComObjectUnitTests::CanInstantiateComObject()
    {
        ctl::ComPtr<ComBase> comInstance;
        THROW_IF_FAILED(ComObject<ComBase>::CreateInstance(&comInstance));
        VERIFY_IS_NOT_NULL(comInstance);

        ctl::ComPtr<SupportErrorInfo> supportErrorInfoInstance;
        THROW_IF_FAILED(ComObject<SupportErrorInfo>::CreateInstance(&supportErrorInfoInstance));
        VERIFY_IS_NOT_NULL(supportErrorInfoInstance);
    }

    void ComObjectUnitTests::RuntimeClassNamePreservesOwnership()
    {
        ValidateRuntimeClassName<NamedInspectable>();
    }

    void ComObjectUnitTests::GetIidsPreservesOwnership()
    {
        ctl::ComPtr<ComBase> instance;
        THROW_IF_FAILED(ComObject<ComBase>::CreateInstance(instance.ReleaseAndGetAddressOf()));

        ULONG count = 0;
        IID* iids = nullptr;
        THROW_IF_FAILED(instance->GetIids(&count, &iids));
        std::unique_ptr<IID, decltype(&CoTaskMemFree)> first(iids, CoTaskMemFree);
        VERIFY_ARE_EQUAL(2ul, count);

        THROW_IF_FAILED(instance->GetIids(&count, &iids));
        std::unique_ptr<IID, decltype(&CoTaskMemFree)> second(iids, CoTaskMemFree);
        VERIFY_ARE_EQUAL(2ul, count);
        VERIFY_IS_TRUE(first.get() != second.get());
        instance.Reset();

        VERIFY_IS_TRUE(first.get()[0] == IID_IUnknown);
        VERIFY_IS_TRUE(first.get()[1] == IID_IInspectable);
        VERIFY_IS_TRUE(second.get()[0] == IID_IUnknown);
        VERIFY_IS_TRUE(second.get()[1] == IID_IInspectable);
    }

    void ComObjectUnitTests::GetIidsForwardsResults()
    {
        ctl::ComPtr<IidResultInspectable> instance;
        THROW_IF_FAILED(ComObject<IidResultInspectable>::CreateInstance(instance.ReleaseAndGetAddressOf()));
        ValidateIidResults(instance.Get(), instance.Get());
    }

    void ComObjectUnitTests::AggregatedGetIidsUsesOuter()
    {
        ctl::ComPtr<IidResultInspectable> outer;
        THROW_IF_FAILED(ComObject<IidResultInspectable>::CreateInstance(outer.ReleaseAndGetAddressOf()));

        ComObject<ComBase>* inner = nullptr;
        THROW_IF_FAILED(ComObject<ComBase>::CreateInstance(outer.Get(), &inner));
        auto releaseInner = [](ComObject<ComBase>* value) { value->NonDelegatingRelease(); };
        std::unique_ptr<ComObject<ComBase>, decltype(releaseInner)> innerOwner(inner, releaseInner);
        ValidateIidResults(inner, outer.Get());

        ULONG count = 0;
        IID* iids = nullptr;
        THROW_IF_FAILED(inner->NonDelegatingGetIids(&count, &iids));
        std::unique_ptr<IID, decltype(&CoTaskMemFree)> ownedIids(iids, CoTaskMemFree);
        VERIFY_ARE_EQUAL(2ul, count);
        VERIFY_IS_TRUE(iids[0] == IID_IUnknown);
        VERIFY_IS_TRUE(iids[1] == IID_IInspectable);
    }

    void ComObjectUnitTests::RuntimeClassNameHandlesEmptyAndEmbeddedNulls()
    {
        ValidateRuntimeClassName<EmptyNameInspectable>();
        ValidateRuntimeClassName<EmbeddedNullNameInspectable>();
    }

    void ComObjectUnitTests::AggregatedRuntimeClassNameUsesOuter()
    {
        ctl::ComPtr<OuterInspectable> outer;
        THROW_IF_FAILED(ComObject<OuterInspectable>::CreateInstance(outer.ReleaseAndGetAddressOf()));

        ComObject<NamedInspectable>* inner = nullptr;
        THROW_IF_FAILED(ComObject<NamedInspectable>::CreateInstance(outer.Get(), &inner));
        auto releaseInner = [](ComObject<NamedInspectable>* value) { value->NonDelegatingRelease(); };
        std::unique_ptr<ComObject<NamedInspectable>, decltype(releaseInner)> innerOwner(inner, releaseInner);

        wrl_wrappers::HString delegatedName;
        THROW_IF_FAILED(inner->GetRuntimeClassName(delegatedName.GetAddressOf()));
        VERIFY_ARE_EQUAL(std::wstring(OuterInspectable::RuntimeName), std::wstring(delegatedName.GetRawBuffer(nullptr)));

        wrl_wrappers::HString nonDelegatingName;
        THROW_IF_FAILED(inner->NonDelegatingGetRuntimeClassName(nonDelegatingName.GetAddressOf()));
        VERIFY_ARE_EQUAL(std::wstring(NamedInspectable::RuntimeName), std::wstring(nonDelegatingName.GetRawBuffer(nullptr)));
    }

} } } } }