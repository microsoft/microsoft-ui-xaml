// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#include "pch.h"
#include "ActivationABITests.h"
#include <XamlTailored.h>
#include <TestCleanupWrapper.h>
#include <wrl/wrappers/corewrappers.h>
#include <wil/result_macros.h>

#undef GetCurrentTime

#include <asyncinfo.h>
#include <Microsoft.UI.Xaml.h>

using namespace Platform;
using namespace ::Windows::Foundation;
using namespace ABI::Microsoft::UI::Xaml;
using namespace ABI::Microsoft::UI::Xaml::Controls;
using namespace Microsoft::UI::Xaml::Tests::Common;
using namespace test_infra;
using namespace WEX::Logging;
using namespace WEX::Common;
using namespace Microsoft;

namespace Microsoft { namespace UI { namespace Xaml { namespace Tests {
    namespace Framework { namespace Activation {

bool ActivationABITests::ClassSetup()
{
    CommonTestSetupHelper::CommonTestClassSetup();
    return true;
}

bool ActivationABITests::TestSetup()
{
    TestServices::WindowHelper->InitializeXaml();
    return true;
}

bool ActivationABITests::TestCleanup()
{
    TestServices::WindowHelper->ShutdownXaml();
    TestServices::WindowHelper->VerifyTestCleanup();
    return true;
}

void ActivationABITests::PointerReturnedForStyleSelectorIsCorrect()
{
    RunOnUIThread([&]()
    {
        typedef HRESULT(WINAPI *FpnDllGetActivationFactory)(HSTRING, IActivationFactory**);

        HMODULE hModXAML;
        VERIFY_IS_TRUE(GetModuleHandleEx(GET_MODULE_HANDLE_EX_FLAG_UNCHANGED_REFCOUNT, L"Microsoft.UI.Xaml.dll", &hModXAML) == TRUE, L"Unable to get the handle for Microsoft.UI.Xaml.dll.");

        FpnDllGetActivationFactory pfnGetActivationFactory = (FpnDllGetActivationFactory)GetProcAddress(hModXAML, "DllGetActivationFactory");
        VERIFY_IS_NOT_NULL(pfnGetActivationFactory);

        Microsoft::WRL::ComPtr<IActivationFactory> spActivationFactory;
        VERIFY_SUCCEEDED(pfnGetActivationFactory(WRL::Wrappers::HStringReference(RuntimeClass_Microsoft_UI_Xaml_Controls_StyleSelector).Get(), &spActivationFactory));

        Microsoft::WRL::ComPtr<IStyleSelectorFactory> spStyleSelectorFactory;
        VERIFY_SUCCEEDED(spActivationFactory->QueryInterface(IID_PPV_ARGS(&spStyleSelectorFactory)));

        Microsoft::WRL::ComPtr<IStyleSelector> spStyleSelector;
        VERIFY_SUCCEEDED(spStyleSelectorFactory->CreateInstance(nullptr /* pOuter */, nullptr /* ppInner */, &spStyleSelector));

        Microsoft::WRL::ComPtr<IStyleSelector> spStyleSelectorAsIStyleSelector;
        VERIFY_SUCCEEDED(spStyleSelector->QueryInterface(IID_PPV_ARGS(&spStyleSelectorAsIStyleSelector)));

        // We should have received back an IStyleSelector the first time, so the QI should not change the pointer.
        // If it did, it means we returned some other vtable from the CreateInstance call which is not expected.
        // Games, such as Supermarket Management 2 HD, have taken a dependency on us returning the right type (without
        // the need to QI us).
        VERIFY_ARE_EQUAL(spStyleSelector.Get(), spStyleSelectorAsIStyleSelector.Get());
    });
}

} } } } } }
