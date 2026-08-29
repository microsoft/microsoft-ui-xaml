// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#pragma once

HRESULT
XamlControls_GetDatePickerSelectionImpl(
    _In_ IInspectable *pSourceDatePicker,
    _In_ IInspectable *pPlacementTarget,
    _Outptr_ IInspectable **ppSelectionResultOperation
    );

HRESULT
XamlControls_GetTimePickerSelectionImpl(
    _In_ IInspectable *pSourceTimePicker,
    _In_ IInspectable *pPlacementTarget,
    _Outptr_ IInspectable **ppSelectionResultOperation
    );

HRESULT
XamlControls_GetListPickerSelectionImpl(
    _In_ IInspectable *pSourceComboBox,
    _Outptr_ IInspectable **ppSelectionResultOperation
    );

HRESULT
XamlControls_TestHookCreateLoopingSelectorImpl(
    _Outptr_ IInspectable **ppLoopingSelector
);

// Helpers

_Check_return_ HRESULT CopyPropertiesFromComboBoxToListPickerFlyout(
    _In_ xaml_controls::IComboBox* pSourceComboBox,
    _In_ xaml_controls::IListPickerFlyout* pDestLPF);
