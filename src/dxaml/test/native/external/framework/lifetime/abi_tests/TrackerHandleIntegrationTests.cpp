// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#include "pch.h"
#include "TrackerHandleIntegrationTests.h"
#include <XamlTailored.h>
#include <TestCleanupWrapper.h>
#include <wrl.h>
#include <wrl/wrappers/corewrappers.h>
#include "TrackerHelperBase.h"
#include <wil/result_macros.h>

#undef GetCurrentTime

#include <asyncinfo.h>
#include <Microsoft.UI.Xaml.hosting.referencetracker.h>
#include <Microsoft.UI.Xaml.h>

using namespace Platform;
using namespace ::Windows::Foundation;
using namespace ABI::Microsoft::UI::Xaml;
using namespace Microsoft::UI::Xaml::Tests::Common;
using namespace test_infra;
using namespace WEX::Logging;
using namespace WEX::Common;
using namespace Microsoft;

namespace Microsoft { namespace UI { namespace Xaml { namespace Tests {
    namespace Framework { namespace Lifetime {

bool TrackerHandleIntegrationTests::ClassSetup()
{
    CommonTestSetupHelper::CommonTestClassSetup();
    return true;
}

bool TrackerHandleIntegrationTests::TestSetup()
{
    TestServices::WindowHelper->InitializeXaml();
    return true;
}

bool TrackerHandleIntegrationTests::TestCleanup()
{
    TestServices::WindowHelper->ShutdownXaml();
    TestServices::WindowHelper->VerifyTestCleanup();
    return true;
}

MIDL_INTERFACE("2994B434-AD9B-4AD1-8382-2D42DEE74634")
ICustomDO : public IInspectable
{

};

class CustomDO
    : public TrackerHelperBase<
    ICustomDO,
    WRL::ComposableBase<ABI::Microsoft::UI::Xaml::IDependencyObjectFactory>>
{

public:
    STDMETHOD(RuntimeClassInitialize)();

    ~CustomDO();

    _Check_return_ HRESULT SetElement(_In_opt_ ABI::Microsoft::UI::Xaml::IUIElement* pElement);
    _Check_return_ HRESULT GetElement(_COM_Outptr_result_maybenull_ ABI::Microsoft::UI::Xaml::IUIElement** ppElement);

private:
    TrackerHandle m_uielement = nullptr;
};

STDMETHODIMP CustomDO::RuntimeClassInitialize()
{
    WRL::ComPtr<ABI::Microsoft::UI::Xaml::IDependencyObjectFactory> spInnerFactory;
    RETURN_IF_FAILED(::Windows::Foundation::GetActivationFactory(
        WRL::Wrappers::HStringReference(RuntimeClass_Microsoft_UI_Xaml_DependencyObject).Get(),
        &spInnerFactory));

    WRL::ComPtr<IInspectable> spInnerInspectable;
    WRL::ComPtr<ABI::Microsoft::UI::Xaml::IDependencyObject> spInnerInstance;
    RETURN_IF_FAILED(spInnerFactory->CreateInstance(
        static_cast<ICustomDO*>(this),
        &spInnerInspectable,
        &spInnerInstance));

    RETURN_IF_FAILED(SetComposableBasePointers(
        spInnerInspectable.Get(),
        spInnerFactory.Get()));
    return S_OK;
}

CustomDO::~CustomDO()
{
    if (m_uielement)
    {
        VERIFY_SUCCEEDED(DeleteTrackerHandle(m_uielement));
    }
}

_Check_return_ HRESULT CustomDO::SetElement(_In_opt_ ABI::Microsoft::UI::Xaml::IUIElement* pElement)
{
    if (!m_uielement)
    {
        RETURN_IF_FAILED(CreateTrackerHandle(&m_uielement));
    }
    RETURN_IF_FAILED(SetTrackerValue(m_uielement, pElement));
    return S_OK;
}

_Check_return_ HRESULT CustomDO::GetElement(_COM_Outptr_result_maybenull_ ABI::Microsoft::UI::Xaml::IUIElement** ppElement)
{
    *ppElement = nullptr;
    if (m_uielement)
    {
        auto succeeded = false;
        WRL::ComPtr<IUnknown> unk;

        succeeded = TryGetTrackerValue(m_uielement, &unk);
        RETURN_HR_IF(E_FAIL, !succeeded);
        RETURN_IF_FAILED(unk.CopyTo(ppElement));
    }
    return S_OK;
}


// Validates basic TrackerHandle API functionality
void TrackerHandleIntegrationTests::BasicApiTest()
{
    TestCleanupWrapper cleanup;
    WRL::ComPtr<CustomDO> obj;
    RunOnUIThread([&]()
    {
        VERIFY_SUCCEEDED(WRL::Details::MakeAndInitialize<CustomDO>(&obj));

        auto buttonAsUIElement = safe_cast<Microsoft::UI::Xaml::UIElement^>(ref new Microsoft::UI::Xaml::Controls::Button);
        auto abi_element = reinterpret_cast<ABI::Microsoft::UI::Xaml::IUIElement*>(buttonAsUIElement);
        VERIFY_SUCCEEDED(obj->SetElement(abi_element));
    });

    RunOnUIThread([&]()
    {
        // Let the original button go out of scope, and verify that the TrackerHandle kept it alive
        WRL::ComPtr<ABI::Microsoft::UI::Xaml::IUIElement> spUI;
        VERIFY_SUCCEEDED(obj->GetElement(&spUI));
        VERIFY_IS_NOT_NULL(spUI);

        // Get a WeakReference to the element, clear the TrackerHandle value, and verify that the underlying object is gone
        WRL::WeakRef spWeak;
        VERIFY_SUCCEEDED(spUI.AsWeak(&spWeak));
        VERIFY_IS_NOT_NULL(spWeak);

        // Resolve the weakref and verify the underlying object is still present
        spUI.Reset();
        VERIFY_SUCCEEDED(spWeak.As(&spUI));
        VERIFY_IS_NOT_NULL(spUI);

        // Now, clear the underlying tracker value and verify that the weakref no longer resolves
        spUI.Reset();
        VERIFY_SUCCEEDED(obj->SetElement(nullptr));
        VERIFY_SUCCEEDED(spWeak.As(&spUI));
        VERIFY_IS_NULL(spUI);
    });

    // Destroy the custom DO by releasing its final ref off-thread, and verify that it correctly
    // gets cleaned up in the final release queue
    obj.Reset();
}

} } } } } }
