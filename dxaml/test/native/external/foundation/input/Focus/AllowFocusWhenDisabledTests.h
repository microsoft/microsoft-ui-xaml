// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#pragma once

//  Copyright (c) Microsoft Corporation.  All rights reserved.

#pragma once

#include <Versioning.h>
#include <XamlTailored.h>
#include <TestEvent.h>
#include <SafeEventRegistration.h>
#include <TestCleanupWrapper.h>

using namespace Microsoft::UI::Xaml::Tests::Common;
using namespace test_infra;

namespace Microsoft { namespace UI { namespace Xaml { namespace Tests {
    namespace Focus {
        class AllowFocusWhenDisabledIntegrationTests : public WEX::TestClass<AllowFocusWhenDisabledIntegrationTests>
        {
        public:
            BEGIN_TEST_CLASS(AllowFocusWhenDisabledIntegrationTests)
                TEST_CLASS_PROPERTY(L"BinaryUnderTest", L"Microsoft.UI.Xaml.dll")
                TEST_CLASS_PROPERTY(L"RunAs", L"UAP")
                TEST_CLASS_PROPERTY(L"Classification", L"Integration")
                TEST_CLASS_PROPERTY(L"VelocityTestPass:OneCoreStrict", L"Desktop")
            END_TEST_CLASS()

            TEST_CLASS_SETUP(ClassSetup)
            TEST_METHOD_SETUP(TestSetup)
            TEST_METHOD_CLEANUP(TestCleanup)

            BEGIN_TEST_METHOD(ControlReceivesAllowFocusWhenDisabledThroughInheritance)
                TEST_METHOD_PROPERTY(L"Description", L"Verifies that when a parent element has AllowFocusWhenDisabled false, it propagates to its children.")
                TEST_METHOD_PROPERTY(L"Hosting:Mode", L"UAP")   // Intermittent test timeout
            END_TEST_METHOD()

            BEGIN_TEST_METHOD(AllowFocusWhenDisabledPropertyDefault)
                TEST_METHOD_PROPERTY(L"Description", L"Verifies that default value of AlowFocusOnInteraction is set to false.")
            END_TEST_METHOD()

            BEGIN_TEST_METHOD(MenuFlyoutCorrectlyPropagatesProperty)
                TEST_METHOD_PROPERTY(L"Description", L"A Flyout is special in that it creates an internal class and forwards its values to that class. Verify that AllowFocusWhenDisabled is propagated correctly")
                TEST_METHOD_PROPERTY(L"TestPass:IncludeOnlyOn", L"Desktop")
            END_TEST_METHOD()

            BEGIN_TEST_METHOD(SupportAllowFocusWhenDisabledOnTextBox)
                TEST_METHOD_PROPERTY(L"Description", L"Check visual state when focusing on TextBox with AllowFocusWhenDisabled property is set")
                TEST_METHOD_PROPERTY(L"Hosting:Mode", L"WPF")
            END_TEST_METHOD()

            BEGIN_TEST_METHOD(SupportAllowFocusWhenDisabledOnTextBoxWithGamePad)
                TEST_METHOD_PROPERTY(L"Description", L"Check visual state when focusing on TextBox using GamePad with AllowFocusWhenDisabled property is set")
                TEST_METHOD_PROPERTY(L"Hosting:Mode", L"WPF")
            END_TEST_METHOD()

            BEGIN_TEST_METHOD(SupportAllowFocusWhenDisabledOnToggleSwitch)
                TEST_METHOD_PROPERTY(L"Description", L"Check visual state when focusing on disabled ToggleSwitch control with AllowFocusWhenDisabled property is set")
            END_TEST_METHOD()

            BEGIN_TEST_METHOD(SupportAllowFocusWhenDisabledOnButton)
                TEST_METHOD_PROPERTY(L"Description", L"Check visual state when focusing on disabled Button control with AllowFocusWhenDisabled property is set")
            END_TEST_METHOD()

            BEGIN_TEST_METHOD(FocusStayOnSameControlAfterReEnable)
                TEST_METHOD_PROPERTY(L"Description", L"Check focus does not move forward to next control after reenable the disabled control with AllowFocusWhenDisabled property is set")
            END_TEST_METHOD()

            BEGIN_TEST_METHOD(SupportAllowFocusWhenDisabledOnButtonWithCustomizedTemplate)
                TEST_METHOD_PROPERTY(L"Description", L"Check visual state when focusing on disabled Button control with customized template")
            END_TEST_METHOD()

            BEGIN_TEST_METHOD(SupportAllowFocusWhenDisabledOnToggleButton)
                TEST_METHOD_PROPERTY(L"Description", L"Check visual state when focusing on disabled ToggleButton control with AllowFocusWhenDisabled property is set")
            END_TEST_METHOD()

            BEGIN_TEST_METHOD(SupportAllowFocusWhenDisabledOnSlider)
                TEST_METHOD_PROPERTY(L"Description", L"Check visual state when focusing on Slider control with AllowFocusWhenDisabled property is set")
                TEST_METHOD_PROPERTY(L"Hosting:Mode", L"UAP")   // [DCPP-test] WPF tests are failing with AnimationIdle timeout during test cleanup
            END_TEST_METHOD()

            BEGIN_TEST_METHOD(SupportAllowFocusWhenDisabledOnRepeatButton)
                TEST_METHOD_PROPERTY(L"Description", L"Check visual state when focusing on RepeatButton control with AllowFocusWhenDisabled property is set")
            END_TEST_METHOD()

            BEGIN_TEST_METHOD(SupportAllowFocusWhenDisabledOnHyperlinkButton)
                TEST_METHOD_PROPERTY(L"Description", L"Check visual state when focusing on HyperlinkButton control with AllowFocusWhenDisabled property is set")
            END_TEST_METHOD()

            BEGIN_TEST_METHOD(SupportAllowFocusWhenDisabledOnAutoSuggestBox)
                TEST_METHOD_PROPERTY(L"Description", L"Check visual state when focusing on AutoSuggestBox control with AllowFocusWhenDisabled property is set")
                TEST_METHOD_PROPERTY(L"Hosting:Mode", L"UAP")   // Input pane can't initialize with lifted islands
            END_TEST_METHOD()

            BEGIN_TEST_METHOD(AllowUiaFocusWhenElementDisabled)
                TEST_METHOD_PROPERTY(L"Description", L"When an element is disabled and AllowFocusWhenDisabled is true, it should be focusable via UIA")
            END_TEST_METHOD()

        private:
            template <typename TClassUnderTest>
            void TestAllowFocusWhenDisabledVisualStates()
            {
                TestCleanupWrapper cleanup;

                TClassUnderTest^ control = nullptr;
                auto controlGotFocusEvent = std::make_shared<Event>();
                auto controlGotFocusRegistration = CreateSafeEventRegistration(TClassUnderTest, GotFocus);

                //create root panel and control under test
                RunOnUIThread([&]()
                {
                    auto rootPanel = ref new xaml_controls::StackPanel();
                    TestServices::WindowHelper->WindowContent = rootPanel;

                    // Add a placeholder first focusable element
                    rootPanel->Children->Append(ref new xaml_controls::Button());

                    control = ref new TClassUnderTest();
                    rootPanel->Children->Append(control);

                    controlGotFocusRegistration.Attach(
                        control,
                        [controlGotFocusEvent]()
                    {
                        LOG_OUTPUT(L"Control GotFocus.");
                        controlGotFocusEvent->Set();
                    });

                });
                TestServices::WindowHelper->WaitForIdle();

                // Set control's AllowFocusWhenDisabled to true and disable the control
                RunOnUIThread([&]()
                {
                    LOG_OUTPUT(L"Disable control and set AllowFocusWhenDisabled to true.");
                    control->AllowFocusWhenDisabled = true;
                    control->IsEnabled = false;
                });
                TestServices::WindowHelper->WaitForIdle();
                controlGotFocusEvent->Reset();
                // Focus on the control
                RunOnUIThread([&]()
                {
                    LOG_OUTPUT(L"Setting focus to control.");
                    control->Focus(FocusState::Keyboard);
                });
                TestServices::WindowHelper->WaitForIdle();
                controlGotFocusEvent->WaitForDefault();

                RunOnUIThread([&]()
                {
                    bool disabledStateFound = false;
                    auto templateRoot = safe_cast<FrameworkElement^>(xaml_media::VisualTreeHelper::GetChild(control, 0));
                    auto groups = xaml::VisualStateManager::GetVisualStateGroups(templateRoot);
                    VERIFY_IS_TRUE(groups->Size > 0);

                    // enumerate current state of all VSG and check if contains "Disable"
                    for (unsigned int i = 0; i < groups->Size; ++i)
                    {
                        auto currentVisualStateGroup = groups->GetAt(i);

                        LOG_OUTPUT(L"VSG %s current state:%s", currentVisualStateGroup->Name->Data(), currentVisualStateGroup->CurrentState->Name->Data());
                        if (currentVisualStateGroup->CurrentState->Name == L"Disabled")
                        {
                            disabledStateFound = true;
                        }
                    }

                    VERIFY_IS_TRUE(disabledStateFound);
                });
                TestServices::WindowHelper->WaitForIdle();

                RunOnUIThread([&]()
                {
                    LOG_OUTPUT(L"Setting to control enabled.");
                    control->IsEnabled = true;
                });
                TestServices::WindowHelper->WaitForIdle();

                RunOnUIThread([&]()
                {
                    bool normalStateFound = false;
                    auto templateRoot = safe_cast<FrameworkElement^>(xaml_media::VisualTreeHelper::GetChild(control, 0));
                    auto groups = xaml::VisualStateManager::GetVisualStateGroups(templateRoot);
                    VERIFY_IS_TRUE(groups->Size > 0);

                    // enumerate current state of all VSG and check if contains "Disable"
                    for (unsigned int i = 0; i < groups->Size; ++i)
                    {
                        auto currentVisualStateGroup = groups->GetAt(i);

                        LOG_OUTPUT(L"VSG %s current state:%s", currentVisualStateGroup->Name->Data(), currentVisualStateGroup->CurrentState->Name->Data());
                        if (currentVisualStateGroup->CurrentState->Name == L"Normal")
                        {
                            normalStateFound = true;
                            break;
                        }
                    }

                    VERIFY_IS_TRUE(normalStateFound);
                });
                TestServices::WindowHelper->WaitForIdle();
            }

            void SupportAllowFocusWhenDisabledOnTextBoxHelper(bool useGamePad);
            Platform::String^ GetResourcesPath() const;
        };
    }
}}}}
