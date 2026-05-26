// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#pragma once

XAML_ABI_NAMESPACE_BEGIN namespace Microsoft { namespace UI { namespace Xaml { namespace Controls
{
    // A helper class for use by DatePickerFlyout and TimePickerFlyout
    class DateTimePickerFlyoutHelper
    {
    public:
        // Calculates the point at which to place the opened Flyout.
        static _Check_return_ HRESULT CalculatePlacementPosition(_In_ IFrameworkElement* pTargetElement, _In_ IControl* pFlyoutPresenter, _Out_ wf::Point* pPoint);

        // Some of the Calendars are RTL (Hebrew, Um Al Qura), and FlowDirection is changed to RTL for the Grid.
        // then column 0 of the Grid is on the right side, and firstPickerAsControl is on the right side too.
        // In this situation, we need to invert the direction when handling left/right KEY down
        static _Check_return_ HRESULT ShouldInvertKeyDirection(_In_ IFrameworkElement* contentPanel, _Out_ BOOLEAN* invert);

        static bool ShouldFirstToThirdDirection(wsy::VirtualKey key, BOOLEAN invert);
        static bool ShouldThirdToFirstDirection(wsy::VirtualKey key, BOOLEAN invert);

        static _Check_return_ HRESULT OnKeyDownImpl(_In_ xaml_input::IKeyRoutedEventArgs* pEventArgs,
            _In_ xaml_controls::IControl* tpFirstPickerAsControl,
            _In_ xaml_controls::IControl* tpSecondPickerAsControl,
            _In_ xaml_controls::IControl* tpThirdPickerAsControl,
            _In_ IFrameworkElement* tpContentPanel);

    };
}}}} XAML_ABI_NAMESPACE_END
