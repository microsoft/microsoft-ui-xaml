// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#pragma once

#include "pch.h"

#include <XamlTailored.h>
#include <TreeHelper.h>
#include "AccessKeyTestHelper.h"

using namespace Microsoft::UI::Xaml::Tests::Common;
using namespace test_infra;

namespace Microsoft { namespace UI { namespace Xaml { namespace Tests {
    namespace AccessKeys {

    void AccessKeyTestHelper::BasicAccessKeyTest(SubTestEnum subTestEnum, UIElement^ fe, std::shared_ptr<Event> invocationEvent)
    {
        if ((subTestEnum == SubTestEnum::SubTest_TestAll) || (subTestEnum == SubTestEnum::SubTest_AccessKeyPressed))
        {
            LOG_OUTPUT(L"\nPress the access key and verify invoked event fired.");
            RunOnUIThread([&]()
            {
                fe->AccessKey = L"A";
                fe->ExitDisplayModeOnAccessKeyInvoked = false;
            });
            TestServices::WindowHelper->WaitForIdle();

            EnableAccessKeyMode enterAccessKeyMode;
            InjectAccessKey(L"A", fe);
            invocationEvent->WaitForDefault();
        }

        if ((subTestEnum == SubTestEnum::SubTest_TestAll) || (subTestEnum == SubTestEnum::SubTest_MultipleAccessKeysPressed))
        {
            invocationEvent->Reset();
            LOG_OUTPUT(L"\nPress the multiple keys exclude access key and verify invoked event not fired.");
            RunOnUIThread([&]()
            {
                fe->AccessKey = L"A";
                fe->ExitDisplayModeOnAccessKeyInvoked = false;
            });
            TestServices::WindowHelper->WaitForIdle();

            EnableAccessKeyMode enterAccessKeyMode;
            InjectAccessKey(L"BCD", fe, false);
            invocationEvent->WaitForNoThrow(std::chrono::milliseconds(100));
            VERIFY_IS_FALSE(invocationEvent->HasFired());
            InjectAccessKey(L"A", fe);
            invocationEvent->WaitForDefault();
        }

        if ((subTestEnum == SubTestEnum::SubTest_TestAll) || (subTestEnum == SubTestEnum::SubTest_LowerCaseAccessKeyPressed))
        {
            LOG_OUTPUT(L"\nTest lower case key press.");
            RunOnUIThread([&]()
            {
                fe->AccessKey = L"A";
                fe->ExitDisplayModeOnAccessKeyInvoked = false;
            });
            TestServices::WindowHelper->WaitForIdle();

            EnableAccessKeyMode enterAccessKeyMode;
            InjectAccessKey(L"a", fe);
            invocationEvent->WaitForDefault();
        }

        if ((subTestEnum == SubTestEnum::SubTest_TestAll) || (subTestEnum == SubTestEnum::SubTest_UpperCaseAccessKeyPressed))
        {
            LOG_OUTPUT(L"\nTest upper case key press.");
            RunOnUIThread([&]()
            {
                fe->AccessKey = L"A";
                fe->ExitDisplayModeOnAccessKeyInvoked = false;
            });
            TestServices::WindowHelper->WaitForIdle();

            EnableAccessKeyMode enterAccessKeyMode;
            InjectAccessKey(L"A", fe);
            invocationEvent->WaitForDefault();
        }

        if ((subTestEnum == SubTestEnum::SubTest_TestAll) || (subTestEnum == SubTestEnum::SubTest_DuplicateAccessKeyPressed))
        {
            LOG_OUTPUT(L"\nTest duplicate key press.");
            RunOnUIThread([&]()
            {
                fe->AccessKey = L"A";
                fe->ExitDisplayModeOnAccessKeyInvoked = false;
            });
            TestServices::WindowHelper->WaitForIdle();

            EnableAccessKeyMode enterAccessKeyMode;
            InjectAccessKey(L"A", fe);
            invocationEvent->WaitForDefault();

            // press same key again should invoke again
            InjectAccessKey(L"A", fe);
            invocationEvent->WaitForDefault();
        }

        if ((subTestEnum == SubTestEnum::SubTest_TestAll) || (subTestEnum == SubTestEnum::SubTest_TwoCharactersAccessKey))
        {
            LOG_OUTPUT(L"\nSet accesskey to two characters.");
            RunOnUIThread([&]()
            {
                fe->AccessKey = L"12";
                fe->ExitDisplayModeOnAccessKeyInvoked = false;
            });
            TestServices::WindowHelper->WaitForIdle();

            EnableAccessKeyMode enterAccessKeyMode;
            InjectAccessKey(L"$d$_1#$d$_2#$u$_1#$u$_2", fe); // 1 d -> 2 d -> 1 u -> 2 u
            invocationEvent->WaitForDefault();
        }

        if ((subTestEnum == SubTestEnum::SubTest_TestAll) || (subTestEnum == SubTestEnum::SubTest_ThreeCharactersAccessKey))
        {
            LOG_OUTPUT(L"\nSet accesskey to three characters.");
            RunOnUIThread([&]()
            {
                fe->AccessKey = L"123";
                fe->ExitDisplayModeOnAccessKeyInvoked = false;
            });
            TestServices::WindowHelper->WaitForIdle();

            EnableAccessKeyMode enterAccessKeyMode;
            InjectAccessKey(L"$d$_1#$u$_1#$d$_2#$u$_2#$d$_3#$u$_3", fe); // 1 d/u->2 d/u -> 3 d/u
            invocationEvent->WaitForDefault();
        }

        if ((subTestEnum == SubTestEnum::SubTest_TestAll) || (subTestEnum == SubTestEnum::SubTest_UnicodeCharactersAccessKey))
        {
            LOG_OUTPUT(L"Warning: Skipping unicode validation due to bugs 6652638, 6866766");
            // Disabling this test for phone/onecore - Tracked
            // Disabling on desktop as well --  Tracked
            //if (!TestServices::Utilities->IsOneCore)
            //{
            //    LOG_OUTPUT(L"\nSet accesskey to special unicode and verify key press alt+1234.");
            //    RunOnUIThread([&]()
            //    {
            //        fe->AccessKey = L"-";
            //        fe->ExitDisplayModeOnAccessKeyInvoked = false;
            //    });
            //    TestServices::WindowHelper->WaitForIdle();

            //    TestServices::KeyboardHelper->PressKeySequence(L"$d$_numlock#$u$_numlock");
            //    EnableAccessKeyMode enterAccessKeyMode;
            //    InjectAccessKey(L"$d$_alt#$d$_KeyPad1#$u$_KeyPad1#$d$_KeyPad2#$u$_KeyPad2#$d$_KeyPad3#$u$_KeyPad3#$d$_KeyPad4#$u$_KeyPad4#$u$_alt", fe);
            //    TestServices::KeyboardHelper->PressKeySequence(L"$d$_numlock#$u$_numlock");

            //    invocationEvent->WaitForDefault();
            //}
        }

        if ((subTestEnum == SubTestEnum::SubTest_TestAll) || (subTestEnum == SubTestEnum::SubTest_NullAccessKey))
        {
            LOG_OUTPUT(L"\nSet accesskey to null string, verify invoked event is not fired.");
            invocationEvent->Reset();
            RunOnUIThread([&]()
            {
                fe->AccessKey = nullptr;
                fe->ExitDisplayModeOnAccessKeyInvoked = false;
            });
            TestServices::WindowHelper->WaitForIdle();

            EnableAccessKeyMode enterAccessKeyMode;
            TestServices::KeyboardHelper->PressKeySequence(L"B");
            invocationEvent->WaitForNoThrow(std::chrono::milliseconds(100));
            VERIFY_IS_FALSE(invocationEvent->HasFired());
        }
    }

    void AccessKeyTestHelper::InvokeAccessKeysOnMultipleButtons(std::vector<xaml_primitives::ButtonBase^> buttons)
    {
        // Set Access keys on multiple buttons, press keys one by one and verify Clicked event on all

        std::vector<SafeEventRegistrationType(xaml_primitives::ButtonBase, Click)> clickRegistrations;
        std::vector<std::shared_ptr<Event>> invokedEvents;

        std::vector<SafeEventRegistrationType(xaml_primitives::ToggleButton, Checked)> checkedRegistrations;

        RunOnUIThread([&]()
        {
            xaml::RoutedEventHandler^ clickedEventHandler = nullptr;
            clickedEventHandler = ref new xaml::RoutedEventHandler([&](Platform::Object^ sender, xaml::RoutedEventArgs^ args)
            {
                auto buttonIter = std::find(buttons.begin(), buttons.end(), dynamic_cast<xaml_primitives::ButtonBase^>(sender));
                if (buttonIter != buttons.end())
                {
                    LONGLONG buttonIndex = buttonIter - buttons.begin();
                    LOG_OUTPUT(L"Button %lld click event CB!", buttonIndex);
                    invokedEvents[buttonIter - buttons.begin()]->Set();
                }
            });

            xaml::RoutedEventHandler^ checkedEventHandler = nullptr;
            checkedEventHandler = ref new xaml::RoutedEventHandler([&](Platform::Object^ sender, xaml::RoutedEventArgs^ args)
            {
                auto buttonIter = std::find(buttons.begin(), buttons.end(), dynamic_cast<xaml_primitives::ToggleButton^>(sender));
                if (buttonIter != buttons.end())
                {
                    LONGLONG buttonIndex = buttonIter - buttons.begin();
                    LOG_OUTPUT(L"Button %lld checked event CB!", buttonIndex);
                    invokedEvents[buttonIter - buttons.begin()]->Set();
                }
            });

            for (size_t currentIndex = 0; currentIndex < buttons.size(); currentIndex++)
            {
                WCHAR wAccessKey = L'A';
                wAccessKey += static_cast<WCHAR>(currentIndex);
                std::shared_ptr<Event> spClickEvent = std::make_shared<Event>();
                xaml_primitives::ButtonBase ^button = buttons[currentIndex];
                invokedEvents.push_back(std::move(spClickEvent));

                xaml_primitives::ToggleButton ^toggleButton = dynamic_cast<xaml_primitives::ToggleButton^>(button);
                if (toggleButton == nullptr)
                {
                    auto clickRegistration = CreateSafeEventRegistration(xaml_primitives::ButtonBase, Click);
                    clickRegistration.Attach(button, clickedEventHandler);
                    clickRegistrations.push_back(std::move(clickRegistration));
                }
                else
                {
                    auto checkedRegistration = CreateSafeEventRegistration(xaml_primitives::ToggleButton, Checked);
                    checkedRegistration.Attach(toggleButton, checkedEventHandler);
                    checkedRegistrations.push_back(std::move(checkedRegistration));
                }

                WCHAR accessKeyString[2];
                StringCchPrintf(accessKeyString, 2, L"%c", wAccessKey);
                Platform::String ^accessKey = ref new Platform::String(accessKeyString);
                button->AccessKey = accessKey;
                button->ExitDisplayModeOnAccessKeyInvoked = false;
            }
        });

        TestServices::WindowHelper->WaitForIdle();

        {
            EnableAccessKeyMode enterAccessKeyMode;
            for (size_t currentIndex = 0; currentIndex < buttons.size(); currentIndex++)
            {
                WCHAR wAccessKey = L'A';
                wAccessKey += static_cast<WCHAR>(currentIndex);
                WCHAR accessKeyString[2];
                StringCchPrintf(accessKeyString, 2, L"%c", wAccessKey);
                Platform::String ^accessKey = ref new Platform::String(accessKeyString);
                InjectAccessKey(accessKey, buttons[currentIndex]);
                invokedEvents[currentIndex]->WaitForDefault();
            }
        }
    }

    void AccessKeyTestHelper::InjectAccessKey
    (
        Platform::String ^ strToType,
        xaml::UIElement^ element,
        bool expectingAccessKeyInvoked,
        bool handleAccessKeyInvokedEvent
    )
    {
        auto accessKeyInvokedRegistration = CreateSafeEventRegistration(Microsoft::UI::Xaml::UIElement, AccessKeyInvoked);
        std::shared_ptr<Event> accessKeyInvokedEvent = std::make_shared<Event>();

        RunOnUIThread([&]()
        {
            accessKeyInvokedRegistration.Attach(
                element,
                ref new wf::TypedEventHandler<UIElement^, xaml_input::AccessKeyInvokedEventArgs^>(
                    [=](UIElement^, xaml_input::AccessKeyInvokedEventArgs^ args)
            {
                LOG_OUTPUT(L"AccessKeyInvoked...");
                accessKeyInvokedEvent->Set();
                if (handleAccessKeyInvokedEvent)
                {
                    args->Handled = true;
                }
            }));
        });

        LOG_OUTPUT(L"Injecting access key %ws", strToType->Data());
        TestServices::KeyboardHelper->PressKeySequence(strToType);
        TestServices::WindowHelper->WaitForIdle();

        if (expectingAccessKeyInvoked)
        {
            accessKeyInvokedEvent->WaitForDefault();
            VERIFY_IS_TRUE(accessKeyInvokedEvent->HasFired());
        }
        else
        {
            accessKeyInvokedEvent->WaitForNoThrow(std::chrono::milliseconds(100));
            VERIFY_IS_FALSE(accessKeyInvokedEvent->HasFired());
        }

        if (accessKeyInvokedEvent->HasFired())
        {
            LOG_OUTPUT(L"AccessKeyInvokedEvent fired!");
        }
        else
        {
            LOG_OUTPUT(L"AccessKeyInvokedEvent did not fire!");
        }

    }

    void AccessKeyTestHelper::InjectAccessKey
    (
        Platform::String ^ strToType,
        xaml_docs::TextElement^ element,
        bool expectingAccessKeyInvoked,
        bool handleAccessKeyInvokedEvent
    )
    {
        auto accessKeyInvokedRegistration = CreateSafeEventRegistration(xaml_docs::TextElement, AccessKeyInvoked);
        std::shared_ptr<Event> accessKeyInvokedEvent = std::make_shared<Event>();

        RunOnUIThread([&]()
        {
            accessKeyInvokedRegistration.Attach(
                element,
                ref new wf::TypedEventHandler<xaml_docs::TextElement^, xaml_input::AccessKeyInvokedEventArgs^>(
                    [=](xaml_docs::TextElement^, xaml_input::AccessKeyInvokedEventArgs^ args)
            {
                LOG_OUTPUT(L"AccessKeyInvoked...");
                accessKeyInvokedEvent->Set();
                if (handleAccessKeyInvokedEvent)
                {
                    args->Handled = true;
                }
            }));
        });

        LOG_OUTPUT(L"Injecting access key %ws", strToType->Data());
        TestServices::KeyboardHelper->PressKeySequence(strToType);
        TestServices::WindowHelper->WaitForIdle();

        if (expectingAccessKeyInvoked)
        {
            accessKeyInvokedEvent->WaitForDefault();
            VERIFY_IS_TRUE(accessKeyInvokedEvent->HasFired());
        }
        else
        {
            accessKeyInvokedEvent->WaitForNoThrow(std::chrono::milliseconds(100));
            VERIFY_IS_FALSE(accessKeyInvokedEvent->HasFired());
        }

        if (accessKeyInvokedEvent->HasFired())
        {
            LOG_OUTPUT(L"AccessKeyInvokedEvent fired!");
        }
        else
        {
            LOG_OUTPUT(L"AccessKeyInvokedEvent did not fire!");
        }

    }

    void AccessKeyTestHelper::TryMovingFocusToXamlForInit()
    {
        RunOnUIThread([&]()
        {
            xaml::UIElement^ element = safe_cast<xaml::UIElement^>(
                xaml_input::FocusManager::FindFirstFocusableElement(TestServices::WindowHelper->WindowContent));
            if (element)
            {
                element->Focus(FocusState::Programmatic);
            }
        });
        TestServices::WindowHelper->WaitForIdle();
    }

    Microsoft::UI::Xaml::Input::IAccessKeyManagerStatics^ AccessKeyTestHelper::GetAccessKeyManagerStatics()
    {
        static Platform::Object^ accessKeyManagerStaticsObject;
        if (!accessKeyManagerStaticsObject)
        {
            VERIFY_SUCCEEDED(wf::GetActivationFactory(wrl_wrappers::HStringReference(L"Microsoft.UI.Xaml.Input.AccessKeyManager").Get(),
                reinterpret_cast<IInspectable**>(&accessKeyManagerStaticsObject)));
        }
        return safe_cast<Microsoft::UI::Xaml::Input::IAccessKeyManagerStatics^>(accessKeyManagerStaticsObject);
    }

}}}}}
