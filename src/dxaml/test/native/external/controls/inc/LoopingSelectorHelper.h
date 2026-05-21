// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#pragma once

#include "pch.h"

#include <XamlTailored.h>
#include <TestEvent.h>
#include <SafeEventRegistration.h>
#include <TestCleanupWrapper.h>

#include <TreeHelper.h>
#include <ControlHelper.h>
#include <ValidateTreeParams.h>

using namespace test_infra;

namespace Microsoft { namespace UI { namespace Xaml { namespace Tests { namespace Common {

    class LoopingSelectorHelper
    {
    public:
        static void PanSingleDateTimeLoopingSelector()
        {
            auto loopingSelectors = GetAllLoopingSelectors();
            VERIFY_IS_TRUE(loopingSelectors->Size > 0);

            auto loopingSelectorFirst = loopingSelectors->GetAt(0);
            DoLoopingSelectorSelectionChange(loopingSelectorFirst);
        }

        static void PanDateTimeLoopingSelector()
        {
            auto loopingSelectors = GetAllLoopingSelectors();
            VERIFY_IS_TRUE(loopingSelectors->Size == 3);

            auto loopingSelectorFirst = loopingSelectors->GetAt(0);
            DoLoopingSelectorSelectionChange(loopingSelectorFirst);
            ValidateLoopingSelectorPanel(loopingSelectorFirst);

            auto loopingSelectorSecond = loopingSelectors->GetAt(1);
            DoLoopingSelectorSelectionChange(loopingSelectorSecond);
            ValidateLoopingSelectorPanel(loopingSelectorSecond);

            auto loopingSelectorThird = loopingSelectors->GetAt(2);
            DoLoopingSelectorSelectionChange(loopingSelectorThird);
            ValidateLoopingSelectorPanel(loopingSelectorThird);
        }

        static xaml_primitives::LoopingSelector^ GetLoopingSelector(Platform::String^ namePickerHost)
        {
            xaml_primitives::LoopingSelector^ loopingSelector = nullptr;

            RunOnUIThread([&]()
            {
                auto popups = xaml_media::VisualTreeHelper::GetOpenPopupsForXamlRoot(
                    TestServices::WindowHelper->WindowContent->XamlRoot);

                for (UINT i = 0; i < popups->Size; ++i)
                {
                    auto popup = popups->GetAt(i);
                    auto pickerHost = safe_cast<xaml_controls::Border^>(TreeHelper::GetVisualChildByName(
                        safe_cast<FrameworkElement^>(popup->Child), namePickerHost));

                    if (pickerHost)
                    {
                        loopingSelector = safe_cast<xaml_primitives::LoopingSelector^>(pickerHost->Child);
                        VERIFY_IS_NOT_NULL(loopingSelector);
                        break;
                    }
                }
            });

            return loopingSelector;
        }

        static wfc::IVector<xaml_primitives::LoopingSelector^>^ GetAllLoopingSelectors()
        {
            auto result = ref new Platform::Collections::Vector<xaml_primitives::LoopingSelector^>();

            RunOnUIThread([&]()
            {
                auto popups = xaml_media::VisualTreeHelper::GetOpenPopupsForXamlRoot(
                    TestServices::WindowHelper->WindowContent->XamlRoot);

                for (UINT i = 0; i < popups->Size; ++i)
                {
                    auto popup = popups->GetAt(i);
                    auto popupChild = safe_cast<FrameworkElement^>(popup->Child);

                    TreeHelper::GetVisualChildrenByType<xaml_primitives::LoopingSelector>(popupChild, result);
                }
            });

            return result;
        }

        static void ValidateLoopingSelectorPanel(xaml_primitives::LoopingSelector^ loopingSelector)
        {
            RunOnUIThread([&]()
            {
                auto scrollViewer = safe_cast<xaml_controls::ScrollViewer^>(TreeHelper::GetVisualChildByName(
                    safe_cast<FrameworkElement^>(loopingSelector), L"ScrollViewer"));
                auto loopingSelectorPanel = safe_cast<xaml_primitives::LoopingSelectorPanel^>(scrollViewer->Content);

                VERIFY_IS_NOT_NULL(loopingSelectorPanel);
                VERIFY_IS_TRUE(loopingSelectorPanel->AreHorizontalSnapPointsRegular);
                VERIFY_IS_TRUE(loopingSelectorPanel->AreVerticalSnapPointsRegular);

                float offset = 0.0f;
                loopingSelectorPanel->GetRegularSnapPoints(xaml_controls::Orientation::Vertical, xaml_primitives::SnapPointsAlignment::Center, &offset);
                VERIFY_ARE_EQUAL(offset, 0.0f);
            });
        }

        static void DoLoopingSelectorSelectionChange(xaml_primitives::LoopingSelector^ loopingSelector)
        {
            std::shared_ptr<Event> selectionChangedEvent = std::make_shared<Event>();
            auto selectionChangedRegistration = CreateSafeEventRegistration(xaml_primitives::LoopingSelector, SelectionChanged);

            RunOnUIThread([&]()
            {
                selectionChangedRegistration.Attach(
                    loopingSelector,
                    ref new xaml_controls::SelectionChangedEventHandler(
                    [selectionChangedEvent, loopingSelector](Platform::Object^ sender, xaml_controls::SelectionChangedEventArgs^ args)
                {
                    LOG_OUTPUT(L"DoLoopingSelectorSelectionChange: SelectionChanged event fired. new selection index=%d", loopingSelector->SelectedIndex);

                    auto selectedItem = loopingSelector->SelectedItem;
                    VERIFY_IS_NOT_NULL(selectedItem);

                    auto items = loopingSelector->Items;
                    LOG_OUTPUT(L"VerifyProperties: items size==%d ", items->Size);
                    auto selectedItemFromIndex = items->GetAt(loopingSelector->SelectedIndex);
                    VERIFY_IS_NOT_NULL(selectedItemFromIndex);
                    VERIFY_ARE_EQUAL(selectedItem, selectedItemFromIndex);

                    selectionChangedEvent->Set();
                }));
            });

            TestServices::WindowHelper->WaitForIdle();

            // These values were changed to work around DCPP Test: InputManagerXaml.dll InjectPressAndDrag does not work correctly on 64 bit OS
            test_infra::TestServices::InputHelper->PanFromCenter(loopingSelector, 0 /*relX*/, -100 /*relY*/, 10.0 /*velocityFactor*/);
            selectionChangedEvent->WaitForDefault();
        }

        enum class SelectionMode
        {
            Keyboard,
            UpDownButtons
        };

        static void SelectItemByIndex(xaml_primitives::LoopingSelector^ loopingSelector, int indexOfItemToSelect, SelectionMode selectionMode)
        {
            auto selectionChangedEvent = std::make_shared<Event>();
            auto selectionChangedRegistration = CreateSafeEventRegistration(xaml_primitives::LoopingSelector, SelectionChanged);
            selectionChangedRegistration.Attach(loopingSelector, [&](){ selectionChangedEvent->Set(); });

            int selectedIndex = -1;
            RunOnUIThread([&]()
            {
                selectedIndex = loopingSelector->SelectedIndex;
            });

            if (selectionMode == SelectionMode::UpDownButtons)
            {
                TestServices::InputHelper->MoveMouse(loopingSelector);
                TestServices::WindowHelper->WaitForIdle();

                xaml_primitives::ButtonBase^ upButton;
                xaml_primitives::ButtonBase^ downButton;
                RunOnUIThread([&]()
                {
                    upButton = safe_cast<xaml_primitives::ButtonBase^>(TreeHelper::GetVisualChildByName(loopingSelector, L"UpButton"));
                    downButton = safe_cast<xaml_primitives::ButtonBase^>(TreeHelper::GetVisualChildByName(loopingSelector, L"DownButton"));
                    THROW_IF_NULL(upButton);
                    THROW_IF_NULL(downButton);
                });

                int delta = indexOfItemToSelect - selectedIndex;
                xaml_primitives::ButtonBase^ buttonToTap = delta > 0 ? downButton : upButton;

                for (int i = 0; i < std::abs(delta); i++)
                {
                    TestServices::WindowHelper->WaitForIdle();
                    TestServices::InputHelper->LeftMouseClick(buttonToTap);
                    selectionChangedEvent->WaitForDefault();
                }
            }
            else if (selectionMode == SelectionMode::Keyboard)
            {
                ControlHelper::EnsureFocused(loopingSelector);

                auto upKeySequence = ref new Platform::String(L"$d$_up#$u$_up");
                auto downKeySequence = ref new Platform::String(L"$d$_down#$u$_down");

                int delta = indexOfItemToSelect - selectedIndex;
                auto keySequenceToUse = delta > 0 ? downKeySequence : upKeySequence;

                for (int i = 0; i < std::abs(delta); i++)
                {
                    TestServices::WindowHelper->WaitForIdle();
                    TestServices::KeyboardHelper->PressKeySequence(keySequenceToUse);
                    selectionChangedEvent->WaitForDefault();
                }
            }

            RunOnUIThread([&]()
            {
                VERIFY_ARE_EQUAL(indexOfItemToSelect, loopingSelector->SelectedIndex);
            });
        }

        // Given a particular value to look for, return any LoopingSelectorItems in the child collection that have the same value.
        // Assumes that CreateTestLoopingSelector creates ints.
        template <typename TCollection>
        static xaml_primitives::LoopingSelectorItem^ FindLoopingSelectorItemByInt(TCollection^ childCollection, int value)
        {
            for (unsigned int i = 0; i < childCollection->Size; i++)
            {
                auto child = safe_cast<xaml_primitives::LoopingSelectorItem^>(childCollection->GetAt(i));

                int childValue = safe_cast<int>(child->Content);

                if (childValue == value)
                {
                    return child;
                }
            }
            return nullptr;
        }
    };

}}}}}
