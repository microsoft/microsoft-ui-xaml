// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#include "pch.h"
#include <XamlTailored.h>
#include <TestEvent.h>
#include <SafeEventRegistration.h>
#include <ppltasks.h>
#include "ActivationIntegrationTests.h"

using namespace Platform;
using namespace Concurrency;
using namespace Microsoft::WRL;
using namespace ::Windows::Foundation;
using namespace ::Windows::Foundation::Collections;
using namespace Microsoft::UI::Xaml;
using namespace Microsoft::UI::Xaml::Controls;
using namespace Microsoft::UI::Xaml::Markup;
using namespace Microsoft::UI::Xaml::Data;
using namespace Microsoft::UI::Xaml::Media;
using namespace Microsoft::UI::Xaml::Media::Animation;

using namespace Microsoft::UI::Xaml::Tests::Common;

using namespace test_infra;
using namespace std;

namespace Microsoft { namespace UI { namespace Xaml { namespace Tests {
    namespace Framework { namespace Activation {

        bool ActivationIntegrationTests::ClassSetup()
        {
            CommonTestSetupHelper::CommonTestClassSetup();
            return true;
        }

        void ActivationIntegrationTests::CreateAFloatCollection()
        {
            RunOnUIThread([&]()
            {
                typedef HRESULT(WINAPI *FpnDllGetActivationFactory)(HSTRING, IActivationFactory**);

                HMODULE hModXAML;
                VERIFY_IS_TRUE(GetModuleHandleEx(GET_MODULE_HANDLE_EX_FLAG_UNCHANGED_REFCOUNT, L"Microsoft.UI.Xaml.dll", &hModXAML) == TRUE, L"Unable to get the handle for Microsoft.UI.Xaml.dll.");

                FpnDllGetActivationFactory pfnGetActivationFactory = (FpnDllGetActivationFactory)GetProcAddress(hModXAML, "DllGetActivationFactory");
                VERIFY_IS_NOT_NULL(pfnGetActivationFactory);

                ComPtr<IActivationFactory> spActivationFactory;
                VERIFY_SUCCEEDED(pfnGetActivationFactory(Platform::StringReference(L"Microsoft.UI.Xaml.Media.FloatCollection").GetHSTRING(), &spActivationFactory));

                ComPtr<IInspectable> spFloatCollection;
                VERIFY_SUCCEEDED(spActivationFactory->ActivateInstance(&spFloatCollection));

                VERIFY_IS_NOT_NULL(spFloatCollection.Get());
            });
        }

        void ActivationIntegrationTests::CreateInputEventArgs()
        {
            RunOnUIThread([&]()
            {
                auto args = ref new xaml_input::ManipulationStartingRoutedEventArgs();
                args->Pivot = ref new xaml_input::ManipulationPivot();

                VERIFY_IS_NOT_NULL(ref new xaml_input::ManipulationStartedRoutedEventArgs());
                VERIFY_IS_NOT_NULL(ref new xaml_input::ManipulationCompletedRoutedEventArgs());
                VERIFY_IS_NOT_NULL(ref new xaml_input::TappedRoutedEventArgs());
                VERIFY_IS_NOT_NULL(ref new xaml_input::DoubleTappedRoutedEventArgs());
                VERIFY_IS_NOT_NULL(ref new xaml_input::HoldingRoutedEventArgs());
            });
        }

} } } } } }
