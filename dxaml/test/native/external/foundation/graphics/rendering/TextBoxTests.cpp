// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#include "pch.h"
#include "TextBoxTests.h"
#include <XamlTailored.h>
#include <TestEvent.h>
#include "FileLoader.h"
#include "TestCleanupWrapper.h"
#include "SafeEventRegistration.h"
#include <WUCRenderingScopeGuard.h>

using namespace ::Windows::UI;
using namespace Microsoft::UI::Xaml;
using namespace Microsoft::UI::Xaml::Tests::Common;

using namespace test_infra;
using namespace MockDComp;
using namespace ::Windows::Storage::Streams;

namespace Microsoft { namespace UI { namespace Xaml { namespace Tests {
    namespace Foundation { namespace Graphics {

        Platform::String^ TextBoxTests::GetResourcesPath() const
        {
            return GetPackageFolder() + L"resources\\native\\external\\foundation\\graphics\\rendering\\";
        }

        bool TextBoxTests::ClassSetup()
        {
            CommonTestSetupHelper::CommonTestClassSetup();
            return true;
        }

        bool TextBoxTests::TestSetup()
        {
            test_infra::TestServices::WindowHelper->InitializeXaml();
            return true;
        }

        bool TextBoxTests::TestCleanup()
        {
            test_infra::TestServices::WindowHelper->ShutdownXaml();
            TestServices::WindowHelper->VerifyTestCleanup();
            return true;
        }

        //------------------------------------------------------------------------
        // Test case: Renders a TextBox
        //------------------------------------------------------------------------
        void TextBoxTests::RenderTextBoxWith18Zoom()
        {
            WUCRenderingScopeGuard guard(DCompRendering::WUCCompleteSynchronousCompTree, false /*resizeWindow*/);
            TestServices::WindowHelper->SetWindowSizeOverrideWithWindowScale(wf::Size(400, 300), 1.8f);

            auto loadedRegistration = CreateSafeEventRegistration(xaml_controls::Canvas, Loaded);
            auto loadedEvent = std::make_shared<Event>();
            auto renderedEvent = std::make_shared<Event>();

            RunOnUIThread([&] ()
            {
                xaml_controls::Canvas ^canvas = ref new xaml_controls::Canvas();

                xaml_controls::TextBox ^textBox = ref new xaml_controls::TextBox();
                VERIFY_IS_NOT_NULL(textBox);
                textBox->Width = 100;

                canvas->Children->Append(textBox);

                TestServices::WindowHelper->WindowContent = canvas;

                loadedRegistration.Attach(canvas, ref new xaml::RoutedEventHandler([loadedEvent](Platform::Object^, xaml::RoutedEventArgs^)
                {
                    loadedEvent->Set();
                }));
            });

            loadedEvent->WaitForDefault();
            TestServices::WindowHelper->WaitForIdle();

            TestServices::Utilities->VerifyMockDCompOutput(MockDComp::SurfaceComparison::NoComparison);
        }

        void TextBoxTests::StartEndHorizontalTextAlignment()
        {
            TestCleanupWrapper cleanup;

            xaml_controls::TextBox^ textBox = nullptr;
            xaml_controls::Panel^ root =
                safe_cast<xaml_controls::Panel^>(LoadXamlFileOnUIThread(GetResourcesPath() + L"TextBoxTests.xaml"));

            RunOnUIThread([&]()
            {
                TestServices::WindowHelper->WindowContent = root;

                textBox = safe_cast<xaml_controls::TextBox^>(root->FindName(L"myTextBox"));
                VERIFY_IS_NOT_NULL(textBox);

                // HorizontalTextAlignment::Start should be the same as TextAlignment::Left
                textBox->HorizontalTextAlignment = TextAlignment::Start;
                VERIFY_IS_TRUE(textBox->TextAlignment == TextAlignment::Left);

                // HorizontalTextAlignment::End should be the same as TextAlignment::Right
                textBox->HorizontalTextAlignment = TextAlignment::End;
                VERIFY_IS_TRUE(textBox->TextAlignment == TextAlignment::Right);
            });
            TestServices::WindowHelper->WaitForIdle();
        }

        void TextBoxTests::VerifyForegroundAlphaOnlyChangeTakesEffect()
        {
            WUCRenderingScopeGuard guard(DCompRendering::WUCCompleteSynchronousCompTree, false /*resizeWindow*/);
            TestServices::WindowHelper->SetWindowSizeOverride(wf::Size(400, 300));

            auto loadedRegistration = CreateSafeEventRegistration(xaml_controls::Canvas, Loaded);
            auto loadedEvent = std::make_shared<Event>();

            xaml_controls::TextBox^ textBox = nullptr;
            RunOnUIThread([&] ()
            {
                xaml_controls::Canvas^ canvas = ref new xaml_controls::Canvas();
                canvas->Width = 400;
                canvas->Height = 100;

                auto button = ref new xaml_controls::Button();
                canvas->Children->Append(button);

                textBox = ref new xaml_controls::TextBox();
                textBox->Foreground = ref new Media::SolidColorBrush(ColorHelper::FromArgb(0xAA, 0xBB, 0xCC, 0xDD));
                textBox->Text = L"TextBox text";

                canvas->Children->Append(textBox);

                TestServices::WindowHelper->WindowContent = canvas;

                loadedRegistration.Attach(canvas, ref new xaml::RoutedEventHandler([=](Platform::Object^, xaml::RoutedEventArgs^)
                {
                    loadedEvent->Set();
                    button->Focus(FocusState::Programmatic);
                }));
            });

            loadedEvent->WaitForDefault();
            TestServices::WindowHelper->WaitForIdle();

            RunOnUIThread([&] ()
            {
                textBox->Foreground = ref new Media::SolidColorBrush(ColorHelper::FromArgb(0X20, 0xBB, 0xCC, 0xDD));
            });
            TestServices::WindowHelper->WaitForIdle();
            TestServices::Utilities->VerifyMockDCompOutput(MockDComp::SurfaceComparison::NoComparison);
        }

    } }
} } } }
