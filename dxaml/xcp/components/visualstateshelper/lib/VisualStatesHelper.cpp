// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#include "precomp.h"
#include "VisualStatesHelper.h"

using namespace DirectUI::Components;

_Check_return_ HRESULT VisualStatesHelper::GetValidVisualStatesListViewBaseItem(
    _In_ ListViewBaseItemVisualStatesCriteria& criteria,
    _Out_ std::vector<const wchar_t*>& validVisualStates)
{
    unsigned int index = 0;
    const unsigned int expectedVisualStatesSize = 7;

    validVisualStates.clear();

    validVisualStates.resize(expectedVisualStatesSize);

    // Focus States
    if (xaml::FocusState_Unfocused != criteria.focusState && criteria.isEnabled)
    {
        if (xaml::FocusState_Pointer == criteria.focusState)
        {
            validVisualStates[index] = L"PointerFocused";
        }
        else
        {
            validVisualStates[index] = L"Focused";
        }
    }
    else
    {
        validVisualStates[index] = L"Unfocused";
    }

    ++index;

    // Multi-Select States
    if (criteria.isMultiSelect)
    {
        validVisualStates[index] = L"MultiSelectEnabled";
    }
    else
    {
        validVisualStates[index] = L"MultiSelectDisabled";
    }

    ++index;

    // Indicator-Select States
    if (criteria.isIndicatorSelect)
    {
        validVisualStates[index] = L"SelectionIndicatorEnabled";
    }
    else
    {
        validVisualStates[index] = L"SelectionIndicatorDisabled";
    }

    ++index;

    // Enabled and Selection States
    if (criteria.isEnabled)
    {
        validVisualStates[index++] = L"Enabled";
            
        if (criteria.isDraggedOver)
        {
            if (criteria.isSelected)
            {
                validVisualStates[index] = L"PointerOverSelected";
            }
            else
            {
                validVisualStates[index] = L"PointerOver";
            }
        }
        else if (criteria.isSelected)
        {
            if (criteria.isPressed)
            {
                validVisualStates[index] = L"PressedSelected";
            }
            else if (criteria.isPointerOver)
            {
                validVisualStates[index] = L"PointerOverSelected";
            }
            else
            {
                if (criteria.isDragging && criteria.isItemDragPrimary && !criteria.isDragVisualCaptured)
                {
                    // Retain press till drag visual is captured
                    validVisualStates[index] = L"PressedSelected";
                }
                else
                {
                    validVisualStates[index] = L"Selected";
                }
            }
        }
        else if (criteria.isPointerOver)
        {
            if (criteria.isPressed)
            {
                validVisualStates[index] = L"Pressed";
            }
            else
            {
                validVisualStates[index] = L"PointerOver";
            }
        }
        else if (criteria.isPressed)
        {
            validVisualStates[index] = L"Pressed";
        }
        else
        {
            if (criteria.isDragging && criteria.isItemDragPrimary && !criteria.isDragVisualCaptured)
            {
                // Retain press till drag visual is captured
                validVisualStates[index] = L"Pressed";
            }
            else
            {
                validVisualStates[index] = L"Normal";
            }
        }
    }
    else
    {
        validVisualStates[index++] = L"Disabled";

        if (criteria.isSelected)
        {
            validVisualStates[index] = L"Selected";
        }
        else
        {
            validVisualStates[index] = L"Normal";
        }
    }

    ++index;

    validVisualStates[index] = L"NoReorderHint";

    ++index;

    // Drag & Reorder States
    if ((criteria.isDragging || criteria.isHolding) && criteria.isInsideListView)
    {
        // to go into the DragOver state, an item must be DraggedOver, enabled, not selected and not the primary dragged item
        // selected items should go into MultipleDraggingSecondary
        // primary dragged items should go to DraggedPlaceholder state for the duration of the drag
        if (criteria.isDraggedOver && criteria.isEnabled && !criteria.isItemDragPrimary && !criteria.isSelected)
        {
            validVisualStates[index] = L"DragOver";
        }
        else if (criteria.dragItemsCount > 1)
        {
            if (criteria.isItemDragPrimary)
            {
                if (criteria.isDragVisualCaptured)
                {
                    if (criteria.canReorder)
                    {
                        validVisualStates[index] = L"ReorderedPlaceholder";
                    }
                    else
                    {
                        validVisualStates[index] = L"DraggedPlaceholder";
                    }
                }
                else
                {
                    if (criteria.canReorder)
                    {
                        validVisualStates[index] = L"MultipleReorderingPrimary";
                    }
                    else
                    {
                        validVisualStates[index] = L"MultipleDraggingPrimary";
                    }
                }
            }
            else
            {
                if (criteria.isSelected)
                {
                    if (criteria.canReorder)
                    {
                        validVisualStates[index] = L"ReorderingTarget";
                    }
                    else
                    {
                        validVisualStates[index] = L"MultipleDraggingSecondary";
                    }
                }
                else
                {
                    if (criteria.canReorder)
                    {
                        validVisualStates[index] = L"ReorderingTarget";
                    }
                    else
                    {
                        validVisualStates[index] = L"DraggingTarget";
                    }
                }
            }
        }
        else
        {
            // Single drag
            if (criteria.isItemDragPrimary)
            {
                if (criteria.isDragVisualCaptured)
                {
                    if (criteria.canReorder)
                    {
                        validVisualStates[index] = L"ReorderedPlaceholder";
                    }
                    else
                    {
                        validVisualStates[index] = L"DraggedPlaceholder";
                    }
                }
                else
                {
                    if (criteria.canReorder)
                    {
                        validVisualStates[index] = L"Reordering";
                    }
                    else
                    {
                        validVisualStates[index] = L"Dragging";
                    }
                }
            }
            else
            {
                if (criteria.canReorder)
                {
                    validVisualStates[index] = L"ReorderingTarget";
                }
                else
                {
                    validVisualStates[index] = L"DraggingTarget";
                }
            }
        }
    }
    else
    {
        // No drag
        validVisualStates[index] = L"NotDragging";
    }

    return S_OK;
}