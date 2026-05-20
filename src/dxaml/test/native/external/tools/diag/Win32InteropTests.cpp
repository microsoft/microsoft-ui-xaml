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
