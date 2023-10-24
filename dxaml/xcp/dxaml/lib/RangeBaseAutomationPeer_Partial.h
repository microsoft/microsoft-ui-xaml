// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#pragma once

#include "RangeBaseAutomationPeer.g.h"
#include "Clock.h"

namespace DirectUI
{
    // Represents the RangeBaseAutomationPeer
    PARTIAL_CLASS(RangeBaseAutomationPeer)
    {
        public:
            // Initializes a new instance of the ToggleButtonAutomationPeer class.
            RangeBaseAutomationPeer();
            ~RangeBaseAutomationPeer() override;

            IFACEMETHOD(GetPatternCore)(_In_ xaml_automation_peers::PatternInterface patternInterface, _Outptr_ IInspectable** ppReturnValue);

            _Check_return_ HRESULT SetValueImpl(_In_ DOUBLE value);
            _Check_return_ HRESULT get_ValueImpl(_Out_ DOUBLE* pValue);
            _Check_return_ HRESULT get_IsReadOnlyImpl(_Out_ BOOLEAN* pValue);
            _Check_return_ HRESULT get_MaximumImpl(_Out_ DOUBLE* pValue);
            _Check_return_ HRESULT get_MinimumImpl(_Out_ DOUBLE* pValue);
            _Check_return_ HRESULT get_LargeChangeImpl(_Out_ DOUBLE* pValue);
            _Check_return_ HRESULT get_SmallChangeImpl(_Out_ DOUBLE* pValue);

            _Check_return_ HRESULT RaiseMinimumPropertyChangedEvent(
                _In_ const CValue& oldValue, 
                _In_ const CValue& newValue);
            _Check_return_ HRESULT RaiseMaximumPropertyChangedEvent(
                _In_ const CValue& oldValue,
                _In_ const CValue& newValue);
            _Check_return_ HRESULT RaiseValuePropertyChangedEvent(
                _In_ const CValue& oldValue,
                _In_ const CValue& newValue);

            _Check_return_ HRESULT EnableValueChangedEventThrottling(_In_ bool value);

    private:
        Jupiter::HighResolutionClock::time_point m_timePointOfLastValuePropertyChangedEvent;
        bool m_isEnableValueChangedEventThrottling;

    };
}
