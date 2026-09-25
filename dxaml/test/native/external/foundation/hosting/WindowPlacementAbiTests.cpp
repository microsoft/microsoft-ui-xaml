// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#include "pch.h"
#include "WindowPlacementAbiTests.h"
#include <XamlTailored.h>
#include <Microsoft.UI.Xaml.h>
#include <microsoft.ui.xaml.coretypes2.h>
#include "TestCleanupWrapper.h"

using namespace Microsoft::UI::Xaml::Tests::Common;
using namespace test_infra;
using ::Microsoft::WRL::ComPtr;
using ::Microsoft::WRL::Wrappers::HString;
using ::Microsoft::WRL::Wrappers::HStringReference;

namespace Microsoft { namespace UI { namespace Xaml { namespace Tests {
    namespace Foundation { namespace Hosting {

        namespace
        {
            namespace abi = ::ABI::Microsoft::UI::Xaml;
            namespace foundation = ::ABI::Windows::Foundation;
            using Rect = ::ABI::Windows::Graphics::RectInt32;

            const Rect Normal = {-20, -10, 40, 30};
            const Rect WorkArea = {-100, -50, 200, 100};

            template<typename T>
            ComPtr<T> GetPlacementFactory()
            {
                ComPtr<T> factory;
                VERIFY_ARE_EQUAL(S_OK, ::RoGetActivationFactory(
                    HStringReference(L"Microsoft.UI.Xaml.WindowPlacement").Get(), IID_PPV_ARGS(&factory)));
                VERIFY_IS_NOT_NULL(factory.Get());
                return factory;
            }

            ComPtr<abi::IWindowPlacement> CreatePlacement()
            {
                auto factory = GetPlacementFactory<abi::IWindowPlacementFactory>();
                ComPtr<abi::IWindowPlacement> placement;
                VERIFY_ARE_EQUAL(S_OK, factory->CreateInstance(Normal, WorkArea, 96, &placement));
                VERIFY_IS_NOT_NULL(placement.Get());
                return placement;
            }

            void VerifyRect(const Rect& expected, const Rect& actual)
            {
                VERIFY_ARE_EQUAL(expected.X, actual.X);
                VERIFY_ARE_EQUAL(expected.Y, actual.Y);
                VERIFY_ARE_EQUAL(expected.Width, actual.Width);
                VERIFY_ARE_EQUAL(expected.Height, actual.Height);
            }

            // Supply nullable structs without relying on a language projection's boxing.
            template<typename T>
            class Reference final : public ::Microsoft::WRL::RuntimeClass<
                ::Microsoft::WRL::RuntimeClassFlags<::Microsoft::WRL::WinRt>, foundation::IReference<T>>
            {
                InspectableClass(L"WindowPlacementAbiTests.Reference", BaseTrust);
            public:
                explicit Reference(T value) : m_value(value) {}
                IFACEMETHOD(get_Value)(T* value) override
                {
                    *value = m_value;
                    return S_OK;
                }
            private:
                T m_value;
            };
        }

        bool WindowPlacementAbiTests::ClassSetup()
        {
            CommonTestSetupHelper::CommonTestClassSetup();
            return true;
        }

        bool WindowPlacementAbiTests::TestSetup()
        {
            TestServices::WindowHelper->InitializeXaml();
            return true;
        }

        bool WindowPlacementAbiTests::TestCleanup()
        {
            TestServices::WindowHelper->ShutdownXaml();
            TestServices::WindowHelper->VerifyTestCleanup();
            return true;
        }

        void WindowPlacementAbiTests::FactoryCreatesPlacementWithDefaults()
        {
            TestCleanupWrapper cleanup;
            RunOnUIThread([&]()
            {
                auto placement = CreatePlacement();
                Rect rect{};
                VERIFY_ARE_EQUAL(S_OK, placement->get_NormalRect(&rect));
                VerifyRect(Normal, rect);
                VERIFY_ARE_EQUAL(S_OK, placement->get_WorkArea(&rect));
                VerifyRect(WorkArea, rect);
                INT dpi{};
                VERIFY_ARE_EQUAL(S_OK, placement->get_Dpi(&dpi));
                VERIFY_ARE_EQUAL(96, dpi);
                abi::WindowPlacementState state{};
                VERIFY_ARE_EQUAL(S_OK, placement->get_State(&state));
                VERIFY_ARE_EQUAL(abi::WindowPlacementState_Normal, state);
                ComPtr<foundation::IReference<Rect>> snap;
                VERIFY_ARE_EQUAL(S_OK, placement->get_SnapRect(&snap));
                VERIFY_IS_NULL(snap.Get());
                ComPtr<foundation::IReference<GUID>> desktop;
                VERIFY_ARE_EQUAL(S_OK, placement->get_VirtualDesktopId(&desktop));
                VERIFY_IS_NULL(desktop.Get());
                HString device;
                VERIFY_ARE_EQUAL(S_OK, placement->get_DisplayDeviceName(device.GetAddressOf()));
                VERIFY_ARE_EQUAL(0u, WindowsGetStringLen(device.Get()));
            });
        }

        void WindowPlacementAbiTests::InvalidDpiReturnsInvalidArgumentAndNoObject()
        {
            TestCleanupWrapper cleanup;
            RunOnUIThread([&]()
            {
                auto factory = GetPlacementFactory<abi::IWindowPlacementFactory>();
                for (INT dpi : {-1, 0, 95})
                {
                    ComPtr<abi::IWindowPlacement> placement;
                    VERIFY_ARE_EQUAL(E_INVALIDARG, factory->CreateInstance(Normal, WorkArea, dpi, &placement));
                    VERIFY_IS_NULL(placement.Get());
                }
            });
        }

        void WindowPlacementAbiTests::AllPropertiesRoundTripThroughAbi()
        {
            TestCleanupWrapper cleanup;
            RunOnUIThread([&]()
            {
                auto placement = CreatePlacement();
                const Rect normal = {10, 20, 640, 480};
                const Rect workArea = {0, 0, 1920, 1080};
                const Rect snapRect = {0, 0, 960, 1080};
                GUID desktopId{};
                VERIFY_ARE_EQUAL(S_OK, CoCreateGuid(&desktopId));
                auto snap = ::Microsoft::WRL::Make<Reference<Rect>>(snapRect);
                auto desktop = ::Microsoft::WRL::Make<Reference<GUID>>(desktopId);
                VERIFY_IS_NOT_NULL(snap.Get());
                VERIFY_IS_NOT_NULL(desktop.Get());
                const HStringReference device(L"\\\\.\\DISPLAY2");

                VERIFY_ARE_EQUAL(S_OK, placement->put_NormalRect(normal));
                VERIFY_ARE_EQUAL(S_OK, placement->put_WorkArea(workArea));
                VERIFY_ARE_EQUAL(S_OK, placement->put_Dpi(144));
                VERIFY_ARE_EQUAL(S_OK, placement->put_State(abi::WindowPlacementState_Snapped));
                VERIFY_ARE_EQUAL(S_OK, placement->put_SnapRect(snap.Get()));
                VERIFY_ARE_EQUAL(S_OK, placement->put_DisplayDeviceName(device.Get()));
                VERIFY_ARE_EQUAL(S_OK, placement->put_VirtualDesktopId(desktop.Get()));

                Rect rect{};
                VERIFY_ARE_EQUAL(S_OK, placement->get_NormalRect(&rect));
                VerifyRect(normal, rect);
                VERIFY_ARE_EQUAL(S_OK, placement->get_WorkArea(&rect));
                VerifyRect(workArea, rect);
                INT dpi{};
                VERIFY_ARE_EQUAL(S_OK, placement->get_Dpi(&dpi));
                VERIFY_ARE_EQUAL(144, dpi);
                abi::WindowPlacementState state{};
                VERIFY_ARE_EQUAL(S_OK, placement->get_State(&state));
                VERIFY_ARE_EQUAL(abi::WindowPlacementState_Snapped, state);
                ComPtr<foundation::IReference<Rect>> returnedSnap;
                VERIFY_ARE_EQUAL(S_OK, placement->get_SnapRect(&returnedSnap));
                VERIFY_IS_NOT_NULL(returnedSnap.Get());
                VERIFY_ARE_EQUAL(S_OK, returnedSnap->get_Value(&rect));
                VerifyRect(snapRect, rect);
                HString returnedDevice;
                VERIFY_ARE_EQUAL(S_OK, placement->get_DisplayDeviceName(returnedDevice.GetAddressOf()));
                INT comparison{};
                VERIFY_ARE_EQUAL(S_OK, WindowsCompareStringOrdinal(device.Get(), returnedDevice.Get(), &comparison));
                VERIFY_ARE_EQUAL(0, comparison);
                ComPtr<foundation::IReference<GUID>> returnedDesktop;
                VERIFY_ARE_EQUAL(S_OK, placement->get_VirtualDesktopId(&returnedDesktop));
                VERIFY_IS_NOT_NULL(returnedDesktop.Get());
                GUID returnedId{};
                VERIFY_ARE_EQUAL(S_OK, returnedDesktop->get_Value(&returnedId));
                VERIFY_IS_TRUE(!!InlineIsEqualGUID(desktopId, returnedId));

                VERIFY_ARE_EQUAL(S_OK, placement->put_SnapRect(nullptr));
                VERIFY_ARE_EQUAL(S_OK, placement->put_VirtualDesktopId(nullptr));
                returnedSnap.Reset();
                returnedDesktop.Reset();
                VERIFY_ARE_EQUAL(S_OK, placement->get_SnapRect(&returnedSnap));
                VERIFY_IS_NULL(returnedSnap.Get());
                VERIFY_ARE_EQUAL(S_OK, placement->get_VirtualDesktopId(&returnedDesktop));
                VERIFY_IS_NULL(returnedDesktop.Get());
            });
        }

        void WindowPlacementAbiTests::CapturedPlacementIsAnIndependentCopy()
        {
            TestCleanupWrapper cleanup;
            RunOnUIThread([&]()
            {
                auto window = ref new xaml::Window();
                auto closeWindow = wil::scope_exit([&]() { window->Close(); });
                window->Content = ref new xaml::Controls::Grid();
                ComPtr<IInspectable> inspectable(reinterpret_cast<IInspectable*>(window));
                ComPtr<abi::IWindow12> window12;
                VERIFY_SUCCEEDED(inspectable.As(&window12));
                ComPtr<IInspectable> optionsInstance;
                VERIFY_ARE_EQUAL(S_OK, RoActivateInstance(
                    HStringReference(L"Microsoft.UI.Xaml.WindowShowOptions").Get(), &optionsInstance));
                ComPtr<abi::IWindowShowOptions> options;
                VERIFY_SUCCEEDED(optionsInstance.As(&options));
                VERIFY_ARE_EQUAL(S_OK, options->put_DoNotActivate(true));
                VERIFY_ARE_EQUAL(S_OK, window12->ShowWithOptions(options.Get()));

                ComPtr<abi::IWindowPlacement> captured;
                ::boolean hasPlacement = false;
                VERIFY_ARE_EQUAL(S_OK, window12->TryGetPlacement(&captured, &hasPlacement));
                VERIFY_IS_TRUE(!!hasPlacement);
                VERIFY_IS_NOT_NULL(captured.Get());
                Rect original{};
                INT originalDpi{};
                VERIFY_ARE_EQUAL(S_OK, captured->get_NormalRect(&original));
                VERIFY_ARE_EQUAL(S_OK, captured->get_Dpi(&originalDpi));
                VERIFY_IS_TRUE(original.Width > 0 && original.Height > 0);
                VERIFY_IS_TRUE(originalDpi >= 96);

                // Editing a returned snapshot must not edit the window or its cached placement.
                const Rect edited = {0, 0, 1, 1};
                VERIFY_ARE_EQUAL(S_OK, captured->put_NormalRect(edited));
                VERIFY_ARE_EQUAL(S_OK, captured->put_Dpi(0));
                ComPtr<abi::IWindowPlacement> recaptured;
                hasPlacement = false;
                VERIFY_ARE_EQUAL(S_OK, window12->TryGetPlacement(&recaptured, &hasPlacement));
                VERIFY_IS_TRUE(!!hasPlacement);
                VERIFY_IS_NOT_NULL(recaptured.Get());
                Rect actual{};
                INT actualDpi{};
                VERIFY_ARE_EQUAL(S_OK, recaptured->get_NormalRect(&actual));
                VerifyRect(original, actual);
                VERIFY_ARE_EQUAL(S_OK, recaptured->get_Dpi(&actualDpi));
                VERIFY_ARE_EQUAL(originalDpi, actualDpi);
                VERIFY_ARE_EQUAL(S_OK, captured->get_NormalRect(&actual));
                VerifyRect(edited, actual);
                VERIFY_ARE_EQUAL(S_OK, captured->get_Dpi(&actualDpi));
                VERIFY_ARE_EQUAL(0, actualDpi);
            });
        }

        void WindowPlacementAbiTests::DetachedLoadOfMissingIdReturnsSuccessAndNull()
        {
            TestCleanupWrapper cleanup;
            RunOnUIThread([&]()
            {
                auto statics = GetPlacementFactory<abi::IWindowPlacementStatics>();
                GUID id{};
                VERIFY_ARE_EQUAL(S_OK, CoCreateGuid(&id));
                WCHAR idText[39]{};
                VERIFY_ARE_EQUAL(39, StringFromGUID2(id, idText, ARRAYSIZE(idText)));
                ComPtr<abi::IWindowPlacement> placement;
                VERIFY_ARE_EQUAL(S_OK, statics->LoadForPersistPlacementId(
                    HStringReference(idText).Get(), &placement));
                VERIFY_IS_NULL(placement.Get());
            });
        }

        void WindowPlacementAbiTests::DetachedLoadOfEmptyOrNullIdReturnsInvalidArgumentAndNoObject()
        {
            TestCleanupWrapper cleanup;
            RunOnUIThread([&]()
            {
                auto statics = GetPlacementFactory<abi::IWindowPlacementStatics>();
                // WinRT canonicalizes an empty string to a null HSTRING, so pass both forms to keep
                // the test honest about which inputs actually reach the callee.
                const HStringReference emptyId(L"");
                const HSTRING ids[] = {static_cast<HSTRING>(nullptr), emptyId.Get()};
                for (HSTRING id : ids)
                {
                    // The generated argument check rejects a null id before it zeroes the
                    // out-parameter, so start from null rather than a sentinel: the contract this
                    // asserts is "no object is produced", not "the callee clears what you passed in".
                    ComPtr<abi::IWindowPlacement> placement;
                    VERIFY_ARE_EQUAL(E_INVALIDARG, statics->LoadForPersistPlacementId(id, placement.GetAddressOf()));
                    VERIFY_IS_NULL(placement.Get());
                }

                // A non-null id does reach the callee, which must clear the out-parameter before it
                // does any work. Use a sentinel here to prove that clearing actually happens.
                GUID unused{};
                VERIFY_ARE_EQUAL(S_OK, CoCreateGuid(&unused));
                WCHAR unusedText[39]{};
                VERIFY_ARE_EQUAL(39, StringFromGUID2(unused, unusedText, ARRAYSIZE(unusedText)));
                auto sentinel = reinterpret_cast<abi::IWindowPlacement*>(static_cast<INT_PTR>(-1));
                VERIFY_ARE_EQUAL(S_OK, statics->LoadForPersistPlacementId(
                    HStringReference(unusedText).Get(), &sentinel));
                VERIFY_IS_NULL(sentinel);
            });
        }

    } }
} } } }
