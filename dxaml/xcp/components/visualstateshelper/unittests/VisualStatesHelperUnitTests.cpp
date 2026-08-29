// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#include "precomp.h"
#include "VisualStatesHelperUnitTests.h"

#include <XamlLogging.h>

namespace Windows { namespace UI { namespace Xaml { namespace Tests { namespace Enterprise { namespace VisualStatesHelper {


    static void ValidateStates(std::vector<const wchar_t*> validVisualStates, const wchar_t* firstState, ...)
    {
        unsigned int size = static_cast<unsigned int>(validVisualStates.size());
        va_list vl;

        // The following warning is for passing a non-trivially-constructible argument to a variable method.  This parameter is not
        // used so this should not matter.  It just needs to be the last required argument to this function before the optional ones
        // begin.  The warning is telling us that it will not construct/destruct the parameter.  But it is already constructed so
        // that does not matter.
#pragma warning (suppress: 4840)
        va_start(vl, validVisualStates);

        for (unsigned int i = 0; i < size; ++i)
        {
            VERIFY_IS_TRUE(std::find(validVisualStates.begin(), validVisualStates.end(), va_arg(vl, const wchar_t*)) != validVisualStates.end());
        }

        va_end(vl);
    }

    //
    // Test Cases
    //
    void VisualStatesHelperUnitTests::ValidateGetValidVisualStatesListViewBaseItem()
    {
        std::vector<const wchar_t*> validVisualStates;

        // unfocused normal
        {
            DirectUI::Components::VisualStatesHelper::ListViewBaseItemVisualStatesCriteria criteria;

            criteria.isEnabled = true;

            VERIFY_SUCCEEDED(DirectUI::Components::VisualStatesHelper::GetValidVisualStatesListViewBaseItem(criteria, validVisualStates));

            ValidateStates(validVisualStates, L"Unfocused", L"Normal", L"MultiSelectDisabled", L"SelectionIndicatorDisabled", L"Enabled", L"NoReorderHint", L"NotDragging");
        }

        // unfocused pointerover
        {
            DirectUI::Components::VisualStatesHelper::ListViewBaseItemVisualStatesCriteria criteria;

            criteria.isEnabled = true;
            criteria.isPointerOver = true;

            VERIFY_SUCCEEDED(DirectUI::Components::VisualStatesHelper::GetValidVisualStatesListViewBaseItem(criteria, validVisualStates));

            ValidateStates(validVisualStates, L"Unfocused", L"PointerOver", L"MultiSelectDisabled", L"SelectionIndicatorDisabled", L"Enabled", L"NoReorderHint", L"NotDragging");
        }

        // unfocused pressed and pointerover
        {
            DirectUI::Components::VisualStatesHelper::ListViewBaseItemVisualStatesCriteria criteria;

            criteria.isEnabled = true;
            criteria.isPressed = true;
            criteria.isPointerOver = true;

            VERIFY_SUCCEEDED(DirectUI::Components::VisualStatesHelper::GetValidVisualStatesListViewBaseItem(criteria, validVisualStates));

            ValidateStates(validVisualStates, L"Unfocused", L"Pressed", L"MultiSelectDisabled", L"SelectionIndicatorDisabled", L"Enabled", L"NoReorderHint", L"NotDragging");
        }

        // unfocused pressed and no pointerover (for touch)
        {
            DirectUI::Components::VisualStatesHelper::ListViewBaseItemVisualStatesCriteria criteria;

            criteria.isEnabled = true;
            criteria.isPressed = true;

            VERIFY_SUCCEEDED(DirectUI::Components::VisualStatesHelper::GetValidVisualStatesListViewBaseItem(criteria, validVisualStates));

            ValidateStates(validVisualStates, L"Unfocused", L"Pressed", L"MultiSelectDisabled", L"SelectionIndicatorDisabled", L"Enabled", L"NoReorderHint", L"NotDragging");
        }

        // unfocused selected
        {
            DirectUI::Components::VisualStatesHelper::ListViewBaseItemVisualStatesCriteria criteria;

            criteria.isEnabled = true;
            criteria.isSelected = true;

            VERIFY_SUCCEEDED(DirectUI::Components::VisualStatesHelper::GetValidVisualStatesListViewBaseItem(criteria, validVisualStates));

            ValidateStates(validVisualStates, L"Unfocused", L"Selected", L"MultiSelectDisabled", L"SelectionIndicatorDisabled", L"Enabled", L"NoReorderHint", L"NotDragging");
        }

        // unfocused pointerover selected
        {
            DirectUI::Components::VisualStatesHelper::ListViewBaseItemVisualStatesCriteria criteria;

            criteria.isEnabled = true;
            criteria.isSelected = true;
            criteria.isPointerOver = true;

            VERIFY_SUCCEEDED(DirectUI::Components::VisualStatesHelper::GetValidVisualStatesListViewBaseItem(criteria, validVisualStates));

            ValidateStates(validVisualStates, L"Unfocused", L"PointerOverSelected", L"MultiSelectDisabled", L"SelectionIndicatorDisabled", L"Enabled", L"NoReorderHint", L"NotDragging");
        }

        // unfocused pressed selected
        {
            DirectUI::Components::VisualStatesHelper::ListViewBaseItemVisualStatesCriteria criteria;

            criteria.isEnabled = true;
            criteria.isSelected = true;
            criteria.isPressed = true;
            criteria.isPointerOver = true;

            VERIFY_SUCCEEDED(DirectUI::Components::VisualStatesHelper::GetValidVisualStatesListViewBaseItem(criteria, validVisualStates));

            ValidateStates(validVisualStates, L"Unfocused", L"PressedSelected", L"MultiSelectDisabled", L"SelectionIndicatorDisabled", L"Enabled", L"NoReorderHint", L"NotDragging");
        }

        // unfocused disabled
        {
            DirectUI::Components::VisualStatesHelper::ListViewBaseItemVisualStatesCriteria criteria;

            VERIFY_SUCCEEDED(DirectUI::Components::VisualStatesHelper::GetValidVisualStatesListViewBaseItem(criteria, validVisualStates));

            ValidateStates(validVisualStates, L"Unfocused", L"Normal", L"MultiSelectDisabled", L"SelectionIndicatorDisabled", L"Disabled", L"NoReorderHint", L"NotDragging");
        }

        // unfocused disabled selected
        {
            DirectUI::Components::VisualStatesHelper::ListViewBaseItemVisualStatesCriteria criteria;

            criteria.isSelected = true;

            VERIFY_SUCCEEDED(DirectUI::Components::VisualStatesHelper::GetValidVisualStatesListViewBaseItem(criteria, validVisualStates));

            ValidateStates(validVisualStates, L"Unfocused", L"Selected", L"MultiSelectDisabled", L"SelectionIndicatorDisabled", L"Disabled", L"NoReorderHint", L"NotDragging");
        }

        // focused normal
        {
            DirectUI::Components::VisualStatesHelper::ListViewBaseItemVisualStatesCriteria criteria;

            criteria.isEnabled = true;
            criteria.focusState = xaml::FocusState_Keyboard;

            VERIFY_SUCCEEDED(DirectUI::Components::VisualStatesHelper::GetValidVisualStatesListViewBaseItem(criteria, validVisualStates));

            ValidateStates(validVisualStates, L"Focused", L"Normal", L"MultiSelectDisabled", L"SelectionIndicatorDisabled", L"Enabled", L"NoReorderHint", L"NotDragging");
        }

        // focused normal
        {
            DirectUI::Components::VisualStatesHelper::ListViewBaseItemVisualStatesCriteria criteria;

            criteria.isEnabled = true;
            criteria.focusState = xaml::FocusState_Programmatic;

            VERIFY_SUCCEEDED(DirectUI::Components::VisualStatesHelper::GetValidVisualStatesListViewBaseItem(criteria, validVisualStates));

            ValidateStates(validVisualStates, L"Focused", L"Normal", L"MultiSelectDisabled", L"SelectionIndicatorDisabled", L"Enabled", L"NoReorderHint", L"NotDragging");
        }

        // pointerfocused normal
        {
            DirectUI::Components::VisualStatesHelper::ListViewBaseItemVisualStatesCriteria criteria;

            criteria.isEnabled = true;
            criteria.focusState = xaml::FocusState_Pointer;

            VERIFY_SUCCEEDED(DirectUI::Components::VisualStatesHelper::GetValidVisualStatesListViewBaseItem(criteria, validVisualStates));

            ValidateStates(validVisualStates, L"PointerFocused", L"Normal", L"MultiSelectDisabled", L"SelectionIndicatorDisabled", L"Enabled", L"NoReorderHint", L"NotDragging");
        }

        // focused selected
        {
            DirectUI::Components::VisualStatesHelper::ListViewBaseItemVisualStatesCriteria criteria;

            criteria.isEnabled = true;
            criteria.isSelected = true;
            criteria.focusState = xaml::FocusState_Programmatic;

            VERIFY_SUCCEEDED(DirectUI::Components::VisualStatesHelper::GetValidVisualStatesListViewBaseItem(criteria, validVisualStates));

            ValidateStates(validVisualStates, L"Focused", L"Selected", L"MultiSelectDisabled", L"SelectionIndicatorDisabled", L"Enabled", L"NoReorderHint", L"NotDragging");
        }

        // multiselect unfocused normal
        {
            DirectUI::Components::VisualStatesHelper::ListViewBaseItemVisualStatesCriteria criteria;

            criteria.isEnabled = true;
            criteria.isMultiSelect = true;

            VERIFY_SUCCEEDED(DirectUI::Components::VisualStatesHelper::GetValidVisualStatesListViewBaseItem(criteria, validVisualStates));

            ValidateStates(validVisualStates, L"Unfocused", L"Normal", L"MultiSelectEnabled", L"SelectionIndicatorDisabled", L"Enabled", L"NoReorderHint", L"NotDragging");
        }

        // multiselect unfocused selected
        {
            DirectUI::Components::VisualStatesHelper::ListViewBaseItemVisualStatesCriteria criteria;

            criteria.isEnabled = true;
            criteria.isSelected = true;
            criteria.isMultiSelect = true;

            VERIFY_SUCCEEDED(DirectUI::Components::VisualStatesHelper::GetValidVisualStatesListViewBaseItem(criteria, validVisualStates));

            ValidateStates(validVisualStates, L"Unfocused", L"Selected", L"MultiSelectEnabled", L"SelectionIndicatorDisabled", L"SelectionIndicatorDisabled", L"Enabled", L"NoReorderHint", L"NotDragging");

        }

        // disabled multiselect unfocused normal
        {
            DirectUI::Components::VisualStatesHelper::ListViewBaseItemVisualStatesCriteria criteria;

            criteria.isMultiSelect = true;

            VERIFY_SUCCEEDED(DirectUI::Components::VisualStatesHelper::GetValidVisualStatesListViewBaseItem(criteria, validVisualStates));

            ValidateStates(validVisualStates, L"Unfocused", L"Normal", L"MultiSelectEnabled", L"SelectionIndicatorDisabled", L"Disabled", L"NoReorderHint", L"NotDragging");
        }

        // disabled multiselect unfocused selected
        {
            DirectUI::Components::VisualStatesHelper::ListViewBaseItemVisualStatesCriteria criteria;

            criteria.isSelected = true;
            criteria.isMultiSelect = true;

            VERIFY_SUCCEEDED(DirectUI::Components::VisualStatesHelper::GetValidVisualStatesListViewBaseItem(criteria, validVisualStates));

            ValidateStates(validVisualStates, L"Unfocused", L"Selected", L"MultiSelectEnabled", L"SelectionIndicatorDisabled", L"Disabled", L"NoReorderHint", L"NotDragging");
        }

        // indicator-select unfocused normal
        {
            DirectUI::Components::VisualStatesHelper::ListViewBaseItemVisualStatesCriteria criteria;

            criteria.isEnabled = true;
            criteria.isIndicatorSelect = true;

            VERIFY_SUCCEEDED(DirectUI::Components::VisualStatesHelper::GetValidVisualStatesListViewBaseItem(criteria, validVisualStates));

            ValidateStates(validVisualStates, L"Unfocused", L"Normal", L"MultiSelectDisabled", L"SelectionIndicatorEnabled", L"Enabled", L"NoReorderHint", L"NotDragging");
        }

        // indicator-select unfocused selected
        {
            DirectUI::Components::VisualStatesHelper::ListViewBaseItemVisualStatesCriteria criteria;

            criteria.isEnabled = true;
            criteria.isSelected = true;
            criteria.isIndicatorSelect = true;

            VERIFY_SUCCEEDED(DirectUI::Components::VisualStatesHelper::GetValidVisualStatesListViewBaseItem(criteria, validVisualStates));

            ValidateStates(validVisualStates, L"Unfocused", L"Selected", L"MultiSelectDisabled", L"SelectionIndicatorEnabled", L"Enabled", L"NoReorderHint", L"NotDragging");
        }

        // disabled indicator-select unfocused normal
        {
            DirectUI::Components::VisualStatesHelper::ListViewBaseItemVisualStatesCriteria criteria;

            criteria.isIndicatorSelect = true;

            VERIFY_SUCCEEDED(DirectUI::Components::VisualStatesHelper::GetValidVisualStatesListViewBaseItem(criteria, validVisualStates));

            ValidateStates(validVisualStates, L"Unfocused", L"Normal", L"MultiSelectDisabled", L"Disabled", L"NoReorderHint", L"NotDragging", L"SelectionIndicatorEnabled");
        }

        // disabled indicator-select unfocused selected
        {
            DirectUI::Components::VisualStatesHelper::ListViewBaseItemVisualStatesCriteria criteria;

            criteria.isSelected = true;
            criteria.isIndicatorSelect = true;

            VERIFY_SUCCEEDED(DirectUI::Components::VisualStatesHelper::GetValidVisualStatesListViewBaseItem(criteria, validVisualStates));

            ValidateStates(validVisualStates, L"Unfocused", L"Selected", L"MultiSelectDisabled", L"Disabled", L"NoReorderHint", L"NotDragging", L"SelectionIndicatorEnabled");
        }

        // enabled selected dragging primary
        {
            DirectUI::Components::VisualStatesHelper::ListViewBaseItemVisualStatesCriteria criteria;

            criteria.isEnabled = true;
            criteria.isSelected = true;
            criteria.isDragging = true;
            criteria.isItemDragPrimary = true;

            VERIFY_SUCCEEDED(DirectUI::Components::VisualStatesHelper::GetValidVisualStatesListViewBaseItem(criteria, validVisualStates));

            ValidateStates(validVisualStates, L"Unfocused", L"PressedSelected", L"MultiSelectDisabled", L"SelectionIndicatorDisabled", L"Enabled", L"NoReorderHint", L"NotDragging");
        }

        // enabled dragging primary
        {
            DirectUI::Components::VisualStatesHelper::ListViewBaseItemVisualStatesCriteria criteria;

            criteria.isEnabled = true;
            criteria.isDragging = true;
            criteria.isItemDragPrimary = true;

            VERIFY_SUCCEEDED(DirectUI::Components::VisualStatesHelper::GetValidVisualStatesListViewBaseItem(criteria, validVisualStates));

            ValidateStates(validVisualStates, L"Unfocused", L"Pressed", L"MultiSelectDisabled", L"Enabled", L"NoReorderHint", L"NotDragging", L"SelectionIndicatorDisabled");
        }

        // enabled selection indicator
        {
            DirectUI::Components::VisualStatesHelper::ListViewBaseItemVisualStatesCriteria criteria;

            criteria.isIndicatorSelect = true;

            VERIFY_SUCCEEDED(DirectUI::Components::VisualStatesHelper::GetValidVisualStatesListViewBaseItem(criteria, validVisualStates));

            ValidateStates(validVisualStates, L"Unfocused", L"Normal", L"MultiSelectDisabled", L"Disabled", L"NoReorderHint", L"NotDragging", L"SelectionIndicatorEnabled");
        }
    }

} } } } } }