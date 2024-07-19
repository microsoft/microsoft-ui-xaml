// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#pragma once

namespace DirectUI { namespace Components { namespace VisualStatesHelper {

    struct ListViewBaseItemVisualStatesCriteria
    {
        BOOLEAN isEnabled;
        BOOLEAN isSelected;
        BOOLEAN isPressed;
        BOOLEAN isPointerOver;
        BOOLEAN isMultiSelect;
        BOOLEAN isIndicatorSelect;
        BOOLEAN isDragging;
        BOOLEAN isItemDragPrimary;
        BOOLEAN isInsideListView;
        BOOLEAN isDragVisualCaptured;
        BOOLEAN isHolding;
        BOOLEAN canDrag;
        BOOLEAN canReorder;
        BOOLEAN isDraggedOver;

        unsigned int dragItemsCount;

        xaml::FocusState focusState;

        ListViewBaseItemVisualStatesCriteria()
        {
            isEnabled = FALSE;
            isSelected = FALSE;
            isPressed = FALSE;
            isPointerOver = FALSE;
            isMultiSelect = FALSE;
            isIndicatorSelect = FALSE;
            isDragging = FALSE;
            isItemDragPrimary = FALSE;
            isInsideListView = FALSE;
            isDragVisualCaptured = FALSE;
            isHolding = FALSE;
            canDrag = FALSE;
            canReorder = FALSE;
            isDraggedOver = FALSE;

            dragItemsCount = 0;

            focusState = xaml::FocusState::FocusState_Unfocused;
        }
    };

    _Check_return_ HRESULT GetValidVisualStatesListViewBaseItem(
        _In_ ListViewBaseItemVisualStatesCriteria& criteria,
        _Out_ std::vector<const wchar_t*>& validVisualStates);

} } }