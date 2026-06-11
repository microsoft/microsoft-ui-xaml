// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#include "pch.h"
#include "TextControlsSizingTests.h"
#include <XamlTailored.h>
#include <TestEvent.h>
#include <ppltasks.h>
#include "FileLoader.h"
#include <SafeEventRegistration.h>
#include <TestCleanupWrapper.h>
#include <WUCRenderingScopeGuard.h>
#include <FocusTestHelper.h>

using namespace Microsoft::UI::Xaml::Tests::Common;
using namespace Microsoft::UI::Xaml;
using namespace Microsoft::UI::Xaml::Controls;
using namespace test_infra;
using namespace ::Windows::Foundation;
using namespace MockDComp;

namespace Microsoft { namespace UI { namespace Xaml { namespace Tests {
    namespace Foundation { namespace Text {
        bool TextControlsSizingTests::ClassSetup()
        {
            CommonTestSetupHelper::CommonTestClassSetup();
            return true;
        }

        bool TextControlsSizingTests::ClassCleanup()
        {
            return true;
        }

        bool TextControlsSizingTests::TestSetup()
        {
            test_infra::TestServices::WindowHelper->InitializeXaml();
            return true;
        }

        bool TextControlsSizingTests::TestCleanup()
        {
            test_infra::TestServices::WindowHelper->ShutdownXaml();
            TestServices::WindowHelper->VerifyTestCleanup();
            return true;
        }

        Platform::String^ TextControlsSizingTests::GetPathToFiles() const
        {
            // Get the deployment directory, and then append our test's directory to the end
            auto deploymentDir = GetTestDeploymentDir();
            return ref new Platform::String(deploymentDir + L"resources\\native\\foundation\\text\\");
        }

        //------------------------------------------------------------------------
        // Test case: Validates the original size of various TextBox controls and
        // focuses each control, temporarily adds a space to each empty control,
        // to make sure their size does not change.
        //------------------------------------------------------------------------
        void TextControlsSizingTests::ValidateInitialTextBoxSizes()
        {
            WUCRenderingScopeGuard guard(DCompRendering::WUCCompleteSynchronousCompTree, false /*resizeWindow*/);

            ::Windows::Foundation::Size size(400, 400);
            TestServices::WindowHelper->SetWindowSizeOverride(size);

            auto gotFocusEvent = std::make_shared<Event>();
            auto rootLoadedEvent = std::make_shared<Event>();
            auto rootLoadedRegistration = CreateSafeEventRegistration(xaml::FrameworkElement, Loaded);
            std::vector<SafeEventRegistrationType(xaml::FrameworkElement, GotFocus)> feGotFocusRegistrations;
            UINT stackPanelChildrenCount = 0u;
            xaml_controls::StackPanel^ stackPanel;
            xaml_controls::ScrollViewer^ rootScrollViewer = safe_cast<xaml_controls::ScrollViewer^>(LoadXamlFileOnUIThread(GetPathToFiles() + L"TextBoxSizingTests.xaml"));
            VERIFY_IS_NOT_NULL(rootScrollViewer);

            RunOnUIThread([&]()
            {
                TestServices::WindowHelper->WindowContent = rootScrollViewer;
                LOG_OUTPUT(L"Listening to root FrameworkElement.Loaded.");
                rootLoadedRegistration.Attach(
                    rootScrollViewer,
                    ref new xaml::RoutedEventHandler(
                    [rootLoadedEvent](Platform::Object^, xaml::IRoutedEventArgs^)
                {
                    LOG_OUTPUT(L"Root FE Loaded handler.");
                    rootLoadedEvent->Set();
                }));
            });

            LOG_OUTPUT(L"Waiting for root FE Loaded event.");
            rootLoadedEvent->WaitForDefault();
            TestServices::WindowHelper->WaitForIdle();

            RunOnUIThread([&]()
            {
                stackPanel = safe_cast<xaml_controls::StackPanel^>(rootScrollViewer->Content);
                stackPanelChildrenCount = stackPanel->Children->Size;

                for (UINT childIndex = 0u; childIndex < stackPanelChildrenCount; childIndex++)
                {
                    LOG_OUTPUT(L"Creating new GotFocusRegistration.");
                    auto feGotFocusRegistration = CreateSafeEventRegistration(xaml::FrameworkElement, GotFocus);

                    LOG_OUTPUT(L"Setting next FrameworkElement.");
                    xaml::FrameworkElement^ fe = safe_cast<xaml::FrameworkElement^>(stackPanel->Children->GetAt(childIndex));

                    LOG_OUTPUT(L"Listening to FrameworkElement.GotFocus.");
                    feGotFocusRegistration.Attach(
                        fe,
                        ref new xaml::RoutedEventHandler(
                        [gotFocusEvent](Platform::Object^, xaml::IRoutedEventArgs^)
                    {
                        LOG_OUTPUT(L"FrameworkElement GotFocus handler.");
                        gotFocusEvent->Set();
                    }));

                    feGotFocusRegistrations.push_back(std::move(feGotFocusRegistration));
                }
            });

            // Give focus to each text control in the StackPanel. Add and remove a space
            // character for the empty text controls.
            for (UINT childIndex = 1u; childIndex < stackPanelChildrenCount; childIndex++)
            {
                xaml_controls::Control^ control;
                xaml_controls::TextBox^ textBox;

                RunOnUIThread([&]()
                {
                    control = dynamic_cast<xaml_controls::Control^>(stackPanel->Children->GetAt(childIndex));
                    textBox = dynamic_cast<xaml_controls::TextBox^>(control);

                    gotFocusEvent->Reset();

                    LOG_OUTPUT(L"Focusing next Control.");
                    control->Focus(xaml::FocusState::Keyboard);
                });

                LOG_OUTPUT(L"Waiting for GotFocus event.");
                gotFocusEvent->WaitForDefault();
                TestServices::WindowHelper->WaitForIdle();

                RunOnUIThread([&]()
                {
                    if (textBox->Text == "")
                    {
                        LOG_OUTPUT(L"Adding space to empty text control.");
                        textBox->Text = " ";
                    }
                });

                TestServices::WindowHelper->WaitForIdle();

                RunOnUIThread([&]()
                {
                    if (textBox->Text == " ")
                    {
                        LOG_OUTPUT(L"Removing space from text control.");
                        textBox->Text = "";
                    }
                });
            }

            RunOnUIThread([&]()
            {
                gotFocusEvent->Reset();

                LOG_OUTPUT(L"Re-focusing first Control.");
                xaml_controls::Control^ control = dynamic_cast<xaml_controls::Control^>(stackPanel->Children->GetAt(0));
                control->Focus(xaml::FocusState::Programmatic);
            });

            LOG_OUTPUT(L"Waiting for GotFocus event.");
            gotFocusEvent->WaitForDefault();
            TestServices::WindowHelper->WaitForIdle();

            RunOnUIThread([&]()
            {
                LOG_OUTPUT(L"Scrolling ScrollViewer back to the top.");
                rootScrollViewer->ChangeView(nullptr /*horizontalOffset*/, 0.0 /*verticalOffset*/, nullptr /*zoomFactor*/, true /*disableAnimation*/);
            });

            TestServices::WindowHelper->WaitForIdle();

            RunOnUIThread([&]()
            {
                VERIFY_ARE_EQUAL(rootScrollViewer->VerticalOffset, 0.0);
            });

            // Waiting for the input pane to be discarded before dumping the final DComp tree.
            // Otherwise the input pane may push the test UI up and influence the snapshot.
            LOG_OUTPUT(L"Discarding input pane.");
            TestServices::WindowHelper->WaitForIdle();

            LOG_OUTPUT(L"Recording final DComp tree after focusing all controls.");
            TestServices::Utilities->VerifyMockDCompOutput(SurfaceComparison::NoComparison);
        }


        ref class TextBox2 sealed : public TextBox
        {
         public:
             TextBox2::TextBox2(float measureHeight)
             {
                 m_measureHeight = measureHeight;
             }

         protected:
             Size MeasureOverride(Size availableSize) override
             {
                 Size result = __super::MeasureOverride(availableSize);
                 VERIFY_ARE_EQUAL(m_measureHeight, floor(result.Height));
                 m_measureOverrideCount++;
                 return result;
             }
          public:
            int GetMeasureOverrideCount()
            {
                return m_measureOverrideCount;
            }
          private:
            int m_measureOverrideCount = 0;
            float m_measureHeight = 0.0f;
        };

        void TextControlsSizingTests::ValidateInitialTextBoxMeasureOverride()
        {
            TestCleanupWrapper cleanup([]()
            {
                TestServices::WindowHelper->ResetWindowContentAndWaitForIdle();
            });

            Grid^ rootGrid = nullptr;
            TextBox2^ tb = nullptr;

            RunOnUIThread([&]()
            {
                rootGrid = ref new Grid;
                rootGrid->Width = 400;
                TestServices::WindowHelper->WindowContent = rootGrid;
            });
            TestServices::WindowHelper->WaitForIdle();

            RunOnUIThread([&]()
            {
                auto stackPanel = dynamic_cast<StackPanel^> (xaml_markup::XamlReader::Load(
                    L"<StackPanel xmlns='http://schemas.microsoft.com/winfx/2006/xaml/presentation' xmlns:x='http://schemas.microsoft.com/winfx/2006/xaml'>"
                    L"  <StackPanel.Resources>"
                    L"    <x:Double x:Key='ControlContentThemeFontSize'>15</x:Double>"
                    L"    <Thickness x:Key='TextControlThemePadding'>10,3,6,5</Thickness>"
                    L"  </StackPanel.Resources>"
                    L"</StackPanel>"));

                tb = ref new TextBox2(249.0f);
                tb->Text = "Video provides a powerful way to help you prove your point. When you click Online Video, you can paste in the embed code for the video you want to add.Video provides a powerful way to help you prove your point. When you click Online Video, you can paste in the embed code for the video you want to add.Video provides a powerful way to help you prove your point. When you click Online Video, you can paste in the embed code for the video you want to add.Video provides a powerful way to help you prove your point. When you click Online Video, you can paste in the embed code for the video you want to add.";
                tb->AcceptsReturn = true;
                tb->TextWrapping = TextWrapping::Wrap;

                stackPanel->Children->Append(tb);
                rootGrid->Children->Append(stackPanel);
            });

            TestServices::WindowHelper->WaitForIdle();
            VERIFY_ARE_EQUAL(1, tb->GetMeasureOverrideCount());
        }

        void TextControlsSizingTests::ValidateAutoGrowTextBoxMeasureOverride()
        {
            TestCleanupWrapper cleanup([]()
            {
                TestServices::WindowHelper->ResetWindowContentAndWaitForIdle();
            });

            Grid^ rootGrid = nullptr;
            TextBox2^ tb = nullptr;

            RunOnUIThread([&]()
            {
                rootGrid = ref new Grid;
                rootGrid->Width = 400;
                TestServices::WindowHelper->WindowContent = rootGrid;
            });
            TestServices::WindowHelper->WaitForIdle();

            RunOnUIThread([&]()
            {
                tb = ref new TextBox2(32.0f);
                tb->Text = "abcdef.";
                tb->TextWrapping = TextWrapping::Wrap;
                tb->VerticalAlignment = VerticalAlignment::Center;
                tb->HorizontalAlignment = HorizontalAlignment::Center;

                rootGrid->Children->Append(tb);
            });

            TestServices::WindowHelper->WaitForIdle();
            FocusTestHelper::EnsureFocus(tb, FocusState::Programmatic);
            TestServices::KeyboardHelper->PressKeySequence(L"x");
            TestServices::WindowHelper->WaitForIdle();
        }
    } }
} } } }
