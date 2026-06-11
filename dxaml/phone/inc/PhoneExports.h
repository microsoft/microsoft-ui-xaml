// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#pragma once

extern "C" {

HRESULT
XamlControlsGetDatePickerSelection(
    _In_ IInspectable* pSourceDatePicker,
    _In_ IInspectable* pPlacementTarget,
    _Outptr_ IInspectable** ppSelectionResultOperation
    );


HRESULT
XamlControlsGetTimePickerSelection(
    _In_ IInspectable* pSourceTimePicker,
    _In_ IInspectable* pPlacementTarget,
    _Outptr_ IInspectable** ppSelectionResultOperation
    );


HRESULT
XamlControlsGetListPickerSelection(
    _In_ IInspectable* pSourceComboBox,
    _Outptr_ IInspectable** ppSelectionResultOperation
    );


HRESULT
XamlControlsTestHookCreateLoopingSelector(
    _Outptr_ IInspectable** ppLoopingSelector
    );

} // extern "C"