// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

//  Abstract:
//      Represents the base class for all range controls such as the ScrollBar
//      and ProgressBar.

#pragma once

#include "RangeBase.g.h"

namespace DirectUI
{
    class RangeBaseAutomationPeer;

    // Represents the base class for all range controls.
    PARTIAL_CLASS(RangeBase)
    {
        public:
            IFACEMETHOD(put_Minimum)(_In_ DOUBLE value) override;
            IFACEMETHOD(put_Maximum)(_In_ DOUBLE value) override;
            IFACEMETHOD(put_Value)(_In_ DOUBLE value) override;
            IFACEMETHOD(put_SmallChange)(_In_ DOUBLE value) override;
            IFACEMETHOD(put_LargeChange)(_In_ DOUBLE value) override;

            _Check_return_ HRESULT OnMinimumChangedImpl(_In_ DOUBLE oldMinimum, _In_ DOUBLE newMinimum)
            {
                RRETURN(S_OK);
            }

            _Check_return_ HRESULT OnMaximumChangedImpl(_In_ DOUBLE oldMaximum, _In_ DOUBLE newMaximum)
            {
                RRETURN(S_OK);
            }

            _Check_return_ HRESULT OnValueChangedImpl(_In_ DOUBLE oldValue, _In_ DOUBLE newValue);

        protected:
            _Check_return_ HRESULT GetDefaultValue2(
                _In_ const CDependencyProperty* pDP,
                _Out_ CValue* pValue) override;

            _Check_return_ HRESULT OnPropertyChanged2(_In_ const PropertyChangedParams& args) override;

            // Override OnCreateAutomationPeer()
            IFACEMETHOD(OnCreateAutomationPeer)(_Outptr_ xaml_automation_peers::IAutomationPeer** returnValue);

        private:
            // Makes sure the DOUBLE value is not Double.NaN, Double.PositiveInfinity, or Double.NegativeInfinity.
            // If the value passed is one of these illegal values, throws an ArgumentException with a custom message.
            _Check_return_ HRESULT EnsureValidDoubleValue(_In_ DOUBLE value);

            _Check_return_ HRESULT HandlePropertyChanged(
                _In_ const PropertyChangedParams& args,
                HRESULT (RangeBaseAutomationPeer::*OnChanged)(const CValue&, const CValue&),
                HRESULT (RangeBaseGenerated::*OnChangedProtected)(DOUBLE, DOUBLE)
            );
    };
}
