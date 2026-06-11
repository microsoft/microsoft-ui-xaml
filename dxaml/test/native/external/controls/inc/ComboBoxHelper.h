// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#pragma once

#include <XamlTailored.h>
#include <TestEvent.h>
#include <SafeEventRegistration.h>
#include <CommonInputHelper.h>
#include <TreeHelper.h>
#include <ControlHelper.h>
#include <SelectorHelper.h>

using namespace test_infra;

namespace Microsoft { namespace UI { namespace Xaml { namespace Tests { namespace Common {

    class ComboBoxHelper
    {
    public:
        enum class OpenMethod
        {
            Mouse,
            Touch,
            Keyboard,
            Gamepad,
            Programmatic
        };

        enum class CloseMethod
        {
            Mouse,
            Touch,
            Keyboard,
            Gamepad,
            Programmatic
        };

        static void FocusComboBoxIfNecessary(xaml_controls::ComboBox^ comboBox)
        {
            auto comboBoxGotFocusEvent = std::make_shared<Event>();
            auto gotFocusRegistration = CreateSafeEventRegistration(xaml_controls::ComboBox, GotFocus);
            gotFocusRegistration.Attach(comboBox, [&]()
            {
                LOG_OUTPUT(L"[ComboBox]: Got Focus Event Fired.");
                comboBoxGotFocusEvent->Set();
            });

            bool alreadyHasFocus = false;
            RunOnUIThread([&]()
            {
                alreadyHasFocus = comboBox->FocusState != xaml::FocusState::Unfocused;
                comboBox->Focus(xaml::FocusState::Keyboard);
            });

            if (!alreadyHasFocus)
            {
                comboBoxGotFocusEvent->WaitForDefault();
            }
        }

        static void OpenComboBox(xaml_controls::ComboBox^ comboBox, OpenMethod openMethod)
        {
            auto comboBoxOpenedEvent = std::make_shared<Event>();
            auto openedRegistration = CreateSafeEventRegistration(xaml_controls::ComboBox, DropDownOpened);
            openedRegistration.Attach(comboBox, [&](){ comboBoxOpenedEvent->Set(); });

            if (openMethod == OpenMethod::Mouse)
            {
                TestServices::InputHelper->LeftMouseClick(comboBox);
            }
            else if (openMethod == OpenMethod::Touch)
            {
                TestServices::InputHelper->Tap(comboBox);
            }
            else if (openMethod == OpenMethod::Keyboard)
            {
                FocusComboBoxIfNecessary(comboBox);
                TestServices::KeyboardHelper->PressKeySequence(L" ");
            }
            else if (openMethod == OpenMethod::Gamepad)
            {
                FocusComboBoxIfNecessary(comboBox);
                CommonInputHelper::Accept(InputDevice::Gamepad);
            }
            else if (openMethod == OpenMethod::Programmatic)
            {
                RunOnUIThread([&]()
                {
                    comboBox->IsDropDownOpen = true;
                });
            }
            TestServices::WindowHelper->WaitForIdle();

            comboBoxOpenedEvent->WaitForDefault();
        }

        static void CloseComboBox(xaml_controls::ComboBox^ comboBox)
        {
            CloseComboBox(comboBox, CloseMethod::Programmatic);
        }

        static void CloseComboBox(xaml_controls::ComboBox^ comboBox, CloseMethod closeMethod)
        {
            auto dropDownClosedEvent = std::make_shared<Event>();
            auto dropDownClosedRegistration = CreateSafeEventRegistration(xaml_controls::ComboBox, DropDownClosed);
            dropDownClosedRegistration.Attach(comboBox, [&](){ dropDownClosedEvent->Set(); });

            if (closeMethod == CloseMethod::Touch || closeMethod == CloseMethod::Mouse)
            {
                wf::Rect dropdownBounds = GetBoundsOfOpenDropdown(comboBox);
                int outsideBuffer = 10; // Tap at least this far away from the dropdown in order to ensure that it closes.
                auto closeTapPoint = wf::Point(dropdownBounds.X + dropdownBounds.Width + outsideBuffer, dropdownBounds.Y + dropdownBounds.Height + outsideBuffer);

                if (closeMethod == CloseMethod::Touch)
                {
                    TestServices::InputHelper->Tap(closeTapPoint);
                }
                else
                {
                    TestServices::InputHelper->LeftMouseClick(closeTapPoint);
                }
            }
            else if (closeMethod == CloseMethod::Keyboard)
            {
                CommonInputHelper::Cancel(InputDevice::Keyboard);
            }
            else if (closeMethod == CloseMethod::Gamepad)
            {
                CommonInputHelper::Cancel(InputDevice::Gamepad);
            }
            else if (closeMethod == CloseMethod::Programmatic)
            {
                RunOnUIThread([&]()
                {
                    comboBox->IsDropDownOpen = false;
                });
            }

            dropDownClosedEvent->WaitForDefault();
            test_infra::TestServices::WindowHelper->WaitForIdle();

            dropDownClosedRegistration.Detach();
        }

        static wf::Rect GetBoundsOfOpenDropdown(xaml::DependencyObject^ element)
        {
            wf::Rect dropdownBounds = {};

            RunOnUIThread([&]()
            {
                auto dropdownScrollViewer = safe_cast<xaml_controls::ScrollViewer^>(TreeHelper::GetVisualChildByNameFromOpenPopups(L"ScrollViewer", element));
                WEX::Common::Throw::IfFalse(dropdownScrollViewer != nullptr, E_FAIL, L"DropDown not found.");
                dropdownBounds = ControlHelper::GetBounds(dropdownScrollViewer);

                LOG_OUTPUT(L"dropdownBounds: (%f, %f, %f, %f)", dropdownBounds.X, dropdownBounds.Y, dropdownBounds.Width, dropdownBounds.Height);

            });
            test_infra::TestServices::WindowHelper->WaitForIdle();

            return dropdownBounds;
        }

        // Verify the selected Index on the ComboBox.
        static void VerifySelectedIndex(xaml_controls::ComboBox^ comboBox, int expected)
        {
            SelectorHelper::VerifySelectedIndex(comboBox, expected);
        }

        // Uses touch to select the ComboBoxItem with the specified index.
        // Note: The function does not currently scroll the popup, so it won't work if the item to be selected
        // is not immediately visible.
        static void SelectItemWithTap(xaml_controls::ComboBox^ comboBox, int index)
        {
            OpenComboBox(comboBox, OpenMethod::Touch);
            TestServices::WindowHelper->WaitForIdle();

            Event selectionChangedEvent;
            auto selectionChangedRegistration = CreateSafeEventRegistration(xaml_controls::ComboBox, SelectionChanged);
            selectionChangedRegistration.Attach(comboBox, [&](){ selectionChangedEvent.Set(); });

            xaml_controls::ComboBoxItem^ comboBoxItemToSelect;
            RunOnUIThread([&]()
            {
                comboBoxItemToSelect = safe_cast<xaml_controls::ComboBoxItem^>(comboBox->ContainerFromIndex(index));
                THROW_IF_NULL(comboBoxItemToSelect);
            });

            TestServices::InputHelper->Tap(comboBoxItemToSelect);
            selectionChangedEvent.WaitForDefault();
        }
    };

} } } } }
