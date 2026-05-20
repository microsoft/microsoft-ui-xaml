// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#pragma once

#include <XamlTailored.h>
#include <TestEvent.h>
#include <SafeEventRegistration.h>
#include <TestCleanupWrapper.h>

namespace Microsoft { namespace UI { namespace Xaml { namespace Tests { namespace Generic {

    enum class ToggleAction
    {
        Tap,
        Space,
        Enter,
    };

    template<typename TClassUnderTest>
    class ToggleButtonTests
    {
    public:
        static void CanToggle()
        {
            LOG_OUTPUT(L"Validating can toggle using Tap.");
            CanToggle(ToggleAction::Tap);

            LOG_OUTPUT(L"Validating can toggle using Space.");
            CanToggle(ToggleAction::Space);

            LOG_OUTPUT(L"Validating can toggle using Enter.");
            CanToggle(ToggleAction::Enter);
        }

        static void CanToggle(ToggleAction action)
        {
            TestCleanupWrapper cleanup;
            TClassUnderTest^ toggleButton = nullptr;

            Event checkedEvent;
            Event uncheckedEvent;
            Event indeterminateEvent;

            auto checkedRegistration = CreateSafeEventRegistration(TClassUnderTest, Checked);
            auto uncheckedRegistration = CreateSafeEventRegistration(TClassUnderTest, Unchecked);
            auto indeterminateRegistration = CreateSafeEventRegistration(TClassUnderTest, Indeterminate);

            RunOnUIThread([&]()
            {
                toggleButton = ref new TClassUnderTest();

                WEX::Common::Throw::If(toggleButton->IsThreeState, E_FAIL, L"Should not be 3-state by default.");
                WEX::Common::Throw::If(toggleButton->IsChecked == nullptr, E_FAIL, L"Should not be null by default.");
                WEX::Common::Throw::If(toggleButton->IsChecked->Value, E_FAIL, L"Should not be toggled by default.");

                toggleButton->HorizontalAlignment = xaml::HorizontalAlignment::Center;
                toggleButton->VerticalAlignment = xaml::VerticalAlignment::Center;

                checkedRegistration.Attach(toggleButton, [&]() { checkedEvent.Set(); });
                uncheckedRegistration.Attach(toggleButton, [&]() { uncheckedEvent.Set(); });
                indeterminateRegistration.Attach(toggleButton, [&]() { indeterminateEvent.Set(); });

                auto root = ref new xaml_controls::Grid();
                root->Children->Append(toggleButton);
                TestServices::WindowHelper->WindowContent = root;
            });
            TestServices::WindowHelper->WaitForIdle();

            RunOnUIThread([&]()
            {
                toggleButton->Focus(xaml::FocusState::Keyboard);
            });

            DoToggleAction(action, toggleButton);
            checkedEvent.WaitForDefault();
            RunOnUIThread([&]
            {
                VERIFY_IS_TRUE(toggleButton->IsChecked->Value);
            });

            DoToggleAction(action, toggleButton);
            uncheckedEvent.WaitForDefault();
            RunOnUIThread([&]
            {
                VERIFY_IS_FALSE(toggleButton->IsChecked->Value);
            });

            // Test a 3-state toggle button.
            RunOnUIThread([&]()
            {
                toggleButton->IsThreeState = true;
            });

            // It should be unchecked by now so toggle 2 times to go to the
            // indeterminate state.
            DoToggleAction(action, toggleButton);
            DoToggleAction(action, toggleButton);
            indeterminateEvent.WaitForDefault();
            RunOnUIThread([&]
            {
                VERIFY_IS_NULL(toggleButton->IsChecked);
            });
        }

    private:
        static void DoToggleAction(ToggleAction action, TClassUnderTest^ toggleButton)
        {
            switch (action)
            {
            case ToggleAction::Tap:
                TestServices::InputHelper->Tap(toggleButton);
                break;

            case ToggleAction::Space:
                TestServices::KeyboardHelper->Space();
                break;

            case ToggleAction::Enter:
                TestServices::KeyboardHelper->Enter();
                break;
            }
            TestServices::WindowHelper->WaitForIdle();
        }

    }; // class ToggleButtonTests

} } } } } // namespace Microsoft::UI::Xaml::Tests::Generic
