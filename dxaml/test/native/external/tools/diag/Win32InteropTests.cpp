// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#include "pch.h"

#include "Win32InteropTests.h"
#include "XamlDiagnosticsTestHelpers.h"
#include <CustomUserControl.h>
#include "CustomTypes.XamlTypeInfo.g.h"
#include <XamlTailored.h>
#include <TestCleanupWrapper.h>
#include "FileLoader.h"
#include <TestEvent.h>
#include "MainPage.xaml.h"
#include <CustomMetadataRegistrar.h>
#include <windows.foundation.numerics.h>
#include <microsoft.ui.interop.h>
#include "RuleTesterHelper.h"

using namespace Microsoft::UI::Xaml;
using namespace Microsoft::UI::Xaml::Media;
using namespace Microsoft::UI::Xaml::Media::Animation;
using namespace Microsoft::UI::Xaml::Tests::Common;
using namespace Microsoft::UI::Xaml::Shapes;
using namespace Microsoft::UI::Xaml::Data;
using namespace ::Tests::Tools::XamlDiagnostics;
using namespace test_infra;
using namespace ::Windows::Foundation;
using namespace ::Windows::Foundation::Collections;

namespace shared_types = ::Tests::Tools::Shared;

namespace {

    // Registers (once) a minimal window class used to host a DesktopWindowXamlSource created by a test.
    ATOM EnsureTestWindowClass()
    {
        static ATOM s_atom = 0;
        if (s_atom == 0)
        {
            WNDCLASSEXW wcex = {};
            wcex.cbSize = sizeof(WNDCLASSEXW);
            wcex.lpfnWndProc = ::DefWindowProcW;
            wcex.hInstance = ::GetModuleHandleW(nullptr);
            wcex.lpszClassName = L"XamlDiagnosticsAbandonedRootTestWindow";
            s_atom = ::RegisterClassExW(&wcex);
            VERIFY_IS_TRUE(s_atom != 0);
        }
        return s_atom;
    }

    HWND CreateTestWindow()
    {
        HWND hwnd = ::CreateWindowW(
            MAKEINTATOM(EnsureTestWindowClass()),
            L"XamlDiagnostics abandoned root test window",
            WS_OVERLAPPEDWINDOW,
            0 /* x */,
            0 /* y */,
            300 /* width */,
            300 /* height */,
            nullptr /* parent */,
            nullptr /* menu */,
            ::GetModuleHandleW(nullptr),
            nullptr /* param */);

        VERIFY_IS_NOT_NULL(hwnd);
        return hwnd;
    }

    wrl::ComPtr<IWeakReference> GetWeakReferenceTo(Platform::Object^ object)
    {
        wrl::ComPtr<IWeakReferenceSource> weakSource;
        VERIFY_SUCCEEDED(reinterpret_cast<IInspectable*>(object)->QueryInterface(IID_PPV_ARGS(&weakSource)));

        wrl::ComPtr<IWeakReference> weakReference;
        VERIFY_SUCCEEDED(weakSource->GetWeakReference(&weakReference));
        VERIFY_IS_NOT_NULL(weakReference.Get());
        return weakReference;
    }

    bool IsStillAlive(const wrl::ComPtr<IWeakReference>& weakReference)
    {
        wrl::ComPtr<IInspectable> resolved;
        VERIFY_SUCCEEDED(weakReference->Resolve(__uuidof(IInspectable), &resolved));
        return resolved != nullptr;
    }
}

namespace Microsoft { namespace UI { namespace Xaml { namespace Tests { namespace Tools {
    namespace XamlDiagnostics {

        #pragma region Test Methods

        bool Win32InteropTests::ClassSetup()
        {
            CommonTestSetupHelper::CommonTestClassSetup();

            return true;
        }

        bool Win32InteropTests::ClassCleanup()
        {
            return true;
        }

        bool Win32InteropTests::TestSetup()
        {
            // due to a known issue: Enable Shutdown XAML when running XAML Bridge (w Islands) tests,
            // we can't register custom types.
            TestServices::WindowHelper->InitializeXaml();
            return EnsureTapLoaded();
        }

        bool Win32InteropTests::TestCleanup()
        {
            test_infra::TestServices::WindowHelper->ShutdownXaml();
            test_infra::TestServices::WindowHelper->VerifyTestCleanup();

            return true;
        }

        void Win32InteropTests::VerifyMutationEventsUsingDesktopWindowXamlSource()
        {
            auto xamlText = ref new Platform::String(
                L"<Grid x:Name='root' xmlns='http://schemas.microsoft.com/winfx/2006/xaml/presentation' xmlns:x='http://schemas.microsoft.com/winfx/2006/xaml' Background='Red' Height='45' Width='45'>"
                L"</Grid>");
            wrl::ComPtr<VisualTreeServiceCallback> callback;
            auto cleanup = m_connectionHelper->Advise(xamlText, callback);
            m_connectionHelper->SetLogAllHandles(true);
            // We should just have the DesktopWindowXamlSource
            VERIFY_ARE_EQUAL(callback->GetNumberOfRoots(), 1u);
            auto desktopWindowXamlSource = callback->GetElementByHandle(callback->GetRoots()[0]);
            VERIFY_IS_TRUE(wcscmp(L"Microsoft.UI.Xaml.Hosting.DesktopWindowXamlSource", desktopWindowXamlSource.Type) == 0);

            auto children = callback->GetChildren(desktopWindowXamlSource.Handle);
            VERIFY_ARE_EQUAL(children.size(), 2u);

            auto root = callback->GetElementByName(L"root");
            auto rootObject = ih_cast<xaml_controls::Grid>(root.Handle);

            InstanceHandle addedButtonHandle = 0, addedEllipseHandle = 0;
            RunOnUIThread([&]() {
                auto button = ref new xaml_controls::Button();
                auto ellipse = ref new xaml_shapes::Ellipse();

                addedButtonHandle = ih_cast(button);
                addedEllipseHandle = ih_cast(ellipse);
                rootObject->Children->Append(button);
                rootObject->Children->Append(ellipse);
            });

            TestServices::WindowHelper->WaitForIdle();

            children = callback->GetChildren(root.Handle);
            VERIFY_ARE_EQUAL(2u, children.size());
            VERIFY_ARE_EQUAL(addedButtonHandle, children[0]);
            VERIFY_ARE_EQUAL(addedEllipseHandle, children[1]);

            LOG_OUTPUT(L"Validate we don't add a root for an unconnected DesktopWindowXamlSource");
            xaml_hosting::DesktopWindowXamlSource^ source;
            RunOnUIThread([&]() {
                source = ref new xaml_hosting::DesktopWindowXamlSource();
                source->Content = ref new xaml_shapes::Rectangle();
            });
            TestServices::WindowHelper->WaitForIdle();

            VERIFY_ARE_EQUAL(callback->GetNumberOfRoots(), 1u);
            RunOnUIThread([&]() {
                delete source;
                source = nullptr;
            });
        }

        // Regression test: a DesktopWindowXamlSource that an app simply drops on the floor, without
        // calling Close() first, must still be released once the app's last reference goes away.
        //
        // The mirror tree that XamlDiagnostics builds is only torn down when the root raises a Remove
        // mutation, and the only thing that raises it for an abandoned source is the source's own
        // destructor. If the mirror tree holds a strong reference to the root, the destructor can never
        // run, so the Remove never arrives and both the root and everything under it leak for the
        // lifetime of the process. Roots are therefore held weakly once they've been handed to the
        // callback.
        void Win32InteropTests::VerifyAbandonedDesktopWindowXamlSourceRootIsReleased()
        {
            auto xamlText = ref new Platform::String(
                L"<Grid x:Name='root' xmlns='http://schemas.microsoft.com/winfx/2006/xaml/presentation' xmlns:x='http://schemas.microsoft.com/winfx/2006/xaml' Background='Red' Height='45' Width='45'>"
                L"</Grid>");
            wrl::ComPtr<VisualTreeServiceCallback> callback;
            auto cleanup = m_connectionHelper->Advise(xamlText, callback);
            m_connectionHelper->SetLogAllHandles(true);

            // The DesktopWindowXamlSource the test app itself is hosted in.
            VERIFY_ARE_EQUAL(1u, callback->GetNumberOfRoots());
            const InstanceHandle appRootHandle = callback->GetRoots()[0];

            HWND hwnd = nullptr;
            xaml_hosting::DesktopWindowXamlSource^ source;
            wrl::ComPtr<IWeakReference> weakSource;
            TestCleanupWrapper hostCleanup([&]() {
                RunOnUIThread([&]() {
                    // Also close a retained source when the assertion fails on an unfixed runtime.
                    if (weakSource)
                    {
                        wrl::ComPtr<IInspectable> retained;
                        VERIFY_SUCCEEDED(weakSource->Resolve(__uuidof(IInspectable), &retained));
                        if (retained)
                        {
                            wrl::ComPtr<ABI::Windows::Foundation::IClosable> closable;
                            VERIFY_SUCCEEDED(retained.As(&closable));
                            VERIFY_SUCCEEDED(closable->Close());
                        }
                    }
                    else if (source)
                    {
                        delete source;
                    }
                    source = nullptr;
                    weakSource.Reset();
                    if (hwnd)
                    {
                        VERIFY_IS_TRUE(::DestroyWindow(hwnd));
                        hwnd = nullptr;
                    }
                });
            });

            LOG_OUTPUT(L"Creating a second, connected DesktopWindowXamlSource.");
            RunOnUIThread([&]() {
                hwnd = CreateTestWindow();

                ABI::Microsoft::UI::WindowId abiWindowId = {};
                VERIFY_SUCCEEDED(ABI::Microsoft::UI::GetWindowIdFromWindow(hwnd, &abiWindowId));

                source = ref new xaml_hosting::DesktopWindowXamlSource();
                source->Initialize(Microsoft::UI::WindowId{ abiWindowId.Value });
                source->Content = ref new xaml_shapes::Rectangle();

                weakSource = GetWeakReferenceTo(source);
            });
            TestServices::WindowHelper->WaitForIdle();

            LOG_OUTPUT(L"A connected source should show up as a second root.");
            VERIFY_ARE_EQUAL(2u, callback->GetNumberOfRoots());

            InstanceHandle abandonedRootHandle = 0;
            for (const auto handle : callback->GetRoots())
            {
                if (handle != appRootHandle)
                {
                    abandonedRootHandle = handle;
                    break;
                }
            }
            VERIFY_IS_TRUE(abandonedRootHandle != 0);

            LOG_OUTPUT(L"Dropping the app's reference without calling Close().");
            RunOnUIThread([&]() {
                // Deliberately not 'delete source' - that would call Close() and take the well behaved
                // path, which works with or without the fix. The point of this test is the app that
                // just lets the source go.
                source = nullptr;
            });
            TestServices::WindowHelper->WaitForIdle();

            LOG_OUTPUT(L"The source should be gone, and its root should have been removed.");
            RunOnUIThread([&]() {
                VERIFY_IS_FALSE(IsStillAlive(weakSource));
            });
            VERIFY_ARE_EQUAL(1u, callback->GetNumberOfRoots());
            VERIFY_ARE_EQUAL(appRootHandle, callback->GetRoots()[0]);

            LOG_OUTPUT(L"The stale handle should no longer resolve.");
            RunOnUIThread([&]() {
                wrl::ComPtr<IInspectable> stale;
                VERIFY_FAILED(m_tap->GetIInspectableFromHandle(abandonedRootHandle, &stale));
                VERIFY_IS_NULL(stale.Get());
            });
        }

        // Gets and sets the SystemBackdrop brush property for a connected DesktopWindowXamlSource using null and non-null composition brushes.
        void Win32InteropTests::UseDesktopWindowXamlSourceSystemBackdrop()
        {
            const auto xamlText = ref new Platform::String(L"<Grid xmlns='http://schemas.microsoft.com/winfx/2006/xaml/presentation' xmlns:x='http://schemas.microsoft.com/winfx/2006/xaml' Background='Red' Height='100' Width='100'/>");
            wrl::ComPtr<VisualTreeServiceCallback> callback;
            auto cleanup = m_connectionHelper->Advise(xamlText, callback);
            m_connectionHelper->SetLogAllHandles(true);
            VERIFY_ARE_EQUAL(callback->GetNumberOfRoots(), 1u);
            auto desktopWindowXamlSource = callback->GetElementByHandle(callback->GetRoots()[0]);
            VERIFY_IS_TRUE(wcscmp(L"Windows.UI.Xaml.Hosting.DesktopWindowXamlSource", desktopWindowXamlSource.Type) == 0);

            RunOnUIThread([&]()
            {
                LOG_OUTPUT(L"Accessing connected DesktopWindowXamlSource instance");
                auto source = ih_cast<xaml_hosting::DesktopWindowXamlSource>(desktopWindowXamlSource.Handle);

                LOG_OUTPUT(L"Accessing ICompositionSupportsSystemBackdrop implementation");
                auto compositionSupportsSystemBackdrop = safe_cast<::Microsoft::UI::Composition::ICompositionSupportsSystemBackdrop^>(source);
                VERIFY_IS_NOT_NULL(compositionSupportsSystemBackdrop);

                LOG_OUTPUT(L"Setting SystemBackdrop to null");
                compositionSupportsSystemBackdrop->SystemBackdrop = nullptr;

                LOG_OUTPUT(L"Accessing null SystemBackdrop");
                auto compositionBrushRead = compositionSupportsSystemBackdrop->SystemBackdrop;
                VERIFY_IS_NULL(compositionBrushRead);

                // Note: We don't have access to the correct Windows.UI.Composition.Compositor to create a brush
                // which can actually draw. But we can create a compositor which will hopefully be enough for this
                // test. (This might fail at runtime if the Compositor gets checked during setting the SystemBackdrop
                // property.)
                LOG_OUTPUT(L"Creating CompositionColorBrush instance");
                auto compositor = ref new ::Windows::UI::Composition::Compositor();
                auto compositionColorBrush = compositor->CreateColorBrush(Microsoft::UI::ColorHelper::FromArgb(0xAA, 0xFF, 0x00, 0x00));

                LOG_OUTPUT(L"Setting SystemBackdrop to non-null CompositionColorBrush");
                compositionSupportsSystemBackdrop->SystemBackdrop = compositionColorBrush;

                LOG_OUTPUT(L"Accessing non-null SystemBackdrop");
                compositionBrushRead = compositionSupportsSystemBackdrop->SystemBackdrop;
                VERIFY_IS_NOT_NULL(compositionBrushRead);
                VERIFY_ARE_EQUAL(compositionColorBrush, compositionBrushRead);

                LOG_OUTPUT(L"Resetting SystemBackdrop to null");
                compositionSupportsSystemBackdrop->SystemBackdrop = nullptr;

                LOG_OUTPUT(L"Accessing null SystemBackdrop");
                compositionBrushRead = compositionSupportsSystemBackdrop->SystemBackdrop;
                VERIFY_IS_NULL(compositionBrushRead);
            });
            TestServices::WindowHelper->WaitForIdle();
        }

        // Sets disconnected DesktopWindowXamlSource's SystemBackdrop property and expects an error back.
        void Win32InteropTests::SetDisconnectedDesktopWindowXamlSourceSystemBackdrop()
        {
            RunOnUIThread([&]()
            {
                LOG_OUTPUT(L"Creating disconnected DesktopWindowXamlSource instance");
                auto source = ref new xaml_hosting::DesktopWindowXamlSource();

                LOG_OUTPUT(L"Accessing ICompositionSupportsSystemBackdrop implementation");
                auto compositionSupportsSystemBackdrop = safe_cast<::Microsoft::UI::Composition::ICompositionSupportsSystemBackdrop^>(source);
                VERIFY_IS_NOT_NULL(compositionSupportsSystemBackdrop);

                LOG_OUTPUT(L"Attempting to set SystemBackdrop to null");
                bool exceptionThrown = false;
                try
                {
                    compositionSupportsSystemBackdrop->SystemBackdrop = nullptr;
                }
                catch (Platform::Exception^ e)
                {
                    LOG_OUTPUT(L"Exception caught");
                    VERIFY_IS_TRUE(e->HResult == HRESULT_FROM_WIN32(ERROR_INVALID_OPERATION));
                    exceptionThrown = true;
                }
                VERIFY_IS_TRUE(exceptionThrown);
            });
            TestServices::WindowHelper->WaitForIdle();
        }
    }
} } } } }
