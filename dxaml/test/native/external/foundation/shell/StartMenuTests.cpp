// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#include "pch.h"
#include <MUX-ETWEvents.h>
#include <XamlTailored.h>
#include <TestEvent.h>
#include <SafeEventRegistration.h>
#include "ETWWaiterProxy.h"
#include "FileLoader.h"
#include "TestCleanupWrapper.h"
#include "StartMenuTests.h"

using namespace ::Windows::UI;
using namespace Microsoft::UI::Xaml;
using namespace Microsoft::UI::Xaml::Input;
using namespace Microsoft::UI::Xaml::Controls;
using namespace Microsoft::UI::Xaml::Shapes;
using namespace Microsoft::UI::Xaml::Media;
using namespace Microsoft::UI::Xaml::Markup;
using namespace Microsoft::UI::Xaml::Tests::Common;
using namespace ::Windows::Graphics::DirectX;

using namespace test_infra;
using namespace MockDComp;
using namespace ::Windows::Storage::Streams;

namespace Microsoft { namespace UI { namespace Xaml { namespace Tests {
    namespace Foundation { namespace Shell {

        Platform::String^ StartMenuTests::GetResourcesPath() const
        {
            return GetPackageFolder() + L"resources\\native\\external\\foundation\\general\\";
        }

        bool StartMenuTests::ClassSetup()
        {
            CommonTestSetupHelper::CommonTestClassSetup();
            return true;
        }

        bool StartMenuTests::TestSetup()
        {
            test_infra::TestServices::WindowHelper->InitializeXaml();
            return true;
        }

        bool StartMenuTests::TestCleanup()
        {
            test_infra::TestServices::WindowHelper->ShutdownXaml();
            TestServices::WindowHelper->VerifyTestCleanup();
            return true;
        }

        void StartMenuTests::SetAtlasSizeHint()
        {
            TestCleanupWrapper cleanup([]()
            {
                TestServices::WindowHelper->ResetWindowContentAndWaitForIdle();
            });

            const uint32_t atlasWidthHint = 1024;
            const uint32_t atlasHeightHint = 1024;

            ETWWaiterProxy etwWaiter;
            Platform::String^ etwValidationString = GetEtwFilterString(atlasWidthHint, atlasHeightHint);

            etwWaiter.Start(
                WINDOWS_UI_XAML_ETW_PROVIDER,
                ExternalAtlasSizeOverrideInfo_value,
                etwValidationString);

            Grid^ rootGrid = safe_cast<Grid^>(LoadXamlFileOnUIThread(GetResourcesPath() + L"EmptyGrid.xaml"));
            RunOnUIThread([&]()
            {
                TestServices::WindowHelper->WindowContent = rootGrid;

                xaml::Window^ xamlWindow = xaml::Window::Current;
                xaml::IWindowPrivate^ windowPrivate = dynamic_cast<xaml::IWindowPrivate^>(xamlWindow);

                windowPrivate->SetAtlasSizeHint(atlasWidthHint, atlasHeightHint);
            });
            TestServices::WindowHelper->WaitForIdle();
            etwWaiter.WaitForDefault();

            // Need to reset the atlas size hint and force a device lost in order to avoid affecting other tests
            TestServices::Utilities->ResetAtlasSizeHint();
            TestServices::WindowHelper->SimulateDeviceLost();
            TestServices::WindowHelper->WaitForIdle();
        }

        // Test implemenation of the IAtlasRequestCallback interface:
        // Simple class that implements the atlas request callback and verifies expected callbacks.
        ref class MyCallback sealed : public xaml::IAtlasRequestCallback
        {
        public:
            MyCallback()
            {
                m_receivedCallback = false;
            }

            void SetExpectedRequest(unsigned int width, unsigned int height, DirectXPixelFormat pixelFormat)
            {
                m_receivedCallback = false;
                m_expectedWidth = width;
                m_expectedHeight = height;
                m_expectedPixelFormat = pixelFormat;
            }

            void ClearReceivedCallback()
            {
                m_receivedCallback = false;
            }

            bool GetReceivedCallback()
            {
                return m_receivedCallback;
            }

            virtual bool AtlasRequest(unsigned int width, unsigned int height, DirectXPixelFormat pixelFormat)
            {
                LOG_OUTPUT(L"Got request, width = %d, height = %d pixelformat = %d", width, height, pixelFormat);
                m_receivedCallback = true;
                VERIFY_IS_TRUE(width == m_expectedWidth);
                VERIFY_IS_TRUE(height == m_expectedHeight);
                VERIFY_IS_TRUE(pixelFormat == m_expectedPixelFormat);
                return false;
            }

        private:
            bool m_receivedCallback;                    // true if we've received a callback
            unsigned int m_expectedWidth;               // The expected width in the callback
            unsigned int m_expectedHeight;              // The expected height in the callback
            DirectXPixelFormat m_expectedPixelFormat;   // The expected pixel format in the callback
        };

        void StartMenuTests::AtlasRequest()
        {
            TestCleanupWrapper cleanup;

            MyCallback^ callback = ref new MyCallback();
            Canvas^ root;
            xaml_shapes::Ellipse^ ellipse;
            xaml::IWindowPrivate^ windowPrivate;

            RunOnUIThread([&]()
            {
                // Create a simple tree with one Ellipse that will produce a 100x100 alpha mask
                root = ref new Canvas();
                TestServices::WindowHelper->WindowContent = root;
            });
            TestServices::WindowHelper->WaitForIdle();

            RunOnUIThread([&]()
            {
                ellipse = ref new xaml_shapes::Ellipse();
                ellipse->Width = 100;
                ellipse->Height = 100;
                ellipse->Fill = ref new SolidColorBrush(Microsoft::UI::Colors::Red);
                root->Children->Append(ellipse);

                LOG_OUTPUT(L"Setting atlas callback");
                xaml::Window^ xamlWindow = xaml::Window::Current;
                windowPrivate = dynamic_cast<xaml::IWindowPrivate^>(xamlWindow);
                windowPrivate->SetAtlasRequestCallback(callback);
                callback->SetExpectedRequest(100, 100, DirectXPixelFormat::A8UIntNormalized);
            });
            TestServices::WindowHelper->WaitForIdle();

            VERIFY_IS_TRUE(callback->GetReceivedCallback());

            RunOnUIThread([&]()
            {
                // Test producing a new alpha mask at larger size
                LOG_OUTPUT(L"Changing size");
                ellipse->Width = 200;
                ellipse->Height = 200;
                callback->SetExpectedRequest(200, 200, DirectXPixelFormat::A8UIntNormalized);
            });
            TestServices::WindowHelper->WaitForIdle();
            VERIFY_IS_TRUE(callback->GetReceivedCallback());

            RunOnUIThread([&]()
            {
                // Test clearing out the callback and ensuring it doesn't get called
                LOG_OUTPUT(L"Clearing atlas callback");
                windowPrivate->SetAtlasRequestCallback(nullptr);

                LOG_OUTPUT(L"Changing size");
                ellipse->Width = 100;
                ellipse->Height = 100;
                callback->ClearReceivedCallback();
            });
            TestServices::WindowHelper->WaitForIdle();
            VERIFY_IS_FALSE(callback->GetReceivedCallback());
        }

        Platform::String^ StartMenuTests::GetEtwFilterString(unsigned int width, unsigned int height)
        {
            Platform::String^ etwFilterString =
                L"@Width=" + width + L" AND " +
                L"@Height=" + height;

            return etwFilterString;
        }

    } }
} } } }
