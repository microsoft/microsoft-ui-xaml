// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#pragma once

#include "ScrollEventArgs.g.h"

namespace DirectUI
{
    // Provides data for a Scroll event that occurs when the Thumb of a
    // ScrollBar moves.
    PARTIAL_CLASS(ScrollEventArgs)
    {
    private:
        // A value that represents the new location of the Thumb in the
        // ScrollBar.
        DOUBLE m_newValue;
        
        // The ScrollEventType enumeration value that describes the change in
        // the Thumb position that caused this event.
        xaml_primitives::ScrollEventType m_scrollEventType;
        
    public:
        // Initializes a new instance of the ScrollEventArgs class.
        ScrollEventArgs()
            : m_scrollEventType(xaml_primitives::ScrollEventType_SmallDecrement)
            , m_newValue(0.0)
        {
        }
                
        // Gets a value that represents the new location of the Thumb in the
        // ScrollBar.
        _Check_return_ HRESULT get_NewValueImpl(
            _Out_ DOUBLE* pValue)
        {
            HRESULT hr = S_OK;
            IFCPTR(pValue);
            *pValue = m_newValue;
        Cleanup:
            RRETURN(hr);
        }
        _Check_return_ HRESULT put_NewValueImpl(
            _In_ DOUBLE value)
        {
            m_newValue = value;
            RRETURN(S_OK);
        }
        
        // Gets the ScrollEventType enumeration value that describes the change
        // in the Thumb position that caused this event.
        _Check_return_ HRESULT get_ScrollEventTypeImpl(
            _Out_ xaml_primitives::ScrollEventType* pValue)
        {
            HRESULT hr = S_OK;
            IFCPTR(pValue);
            *pValue = m_scrollEventType;
        Cleanup:
            RRETURN(hr);
        }
        _Check_return_ HRESULT put_ScrollEventTypeImpl(
            _In_ xaml_primitives::ScrollEventType value)
        {
            m_scrollEventType = value;
            RRETURN(S_OK);
        }
    };
}
