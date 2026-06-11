// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

//  Abstract:
//      Provides data for the DragDelta event that occurs one or more times when
//      a user drags a Thumb control with the mouse.

#pragma once

#include "DragDeltaEventArgs.g.h"

namespace DirectUI
{
    // Provides data for the DragDelta event that occurs one or more times when
    // a user drags a Thumb control with the mouse.
    PARTIAL_CLASS(DragDeltaEventArgs)
    {
        private:
            // Gets the horizontal change in the Thumb position since the last
            // DragDelta event.
            DOUBLE m_horizontalChange;
            
            // Gets the vertical change in the Thumb position since the last
            // DragDelta event.
            DOUBLE m_verticalChange;
            
        public:
            // Initializes a new instance of the DragDeltaEventArgs class.
            DragDeltaEventArgs()
                : m_horizontalChange(0.0)
                , m_verticalChange(0.0)
            {
            }
            
            // Gets the horizontal change in the Thumb position since the last

            // DragDelta event.
            _Check_return_ HRESULT get_HorizontalChangeImpl(
                _Out_ DOUBLE* pValue)
            {
                HRESULT hr = S_OK;
                IFCPTR(pValue);
                *pValue = m_horizontalChange;
            Cleanup:
                RRETURN(hr);
            }
            _Check_return_ HRESULT put_HorizontalChangeImpl(
                _In_ DOUBLE value)
            {
                m_horizontalChange = value;
                RRETURN(S_OK);
            }
            
            // Gets the vertical change in the Thumb position since the last
            // DragDelta event.
            _Check_return_ HRESULT get_VerticalChangeImpl(
                _Out_ DOUBLE* pValue)
            {
                HRESULT hr = S_OK;
                IFCPTR(pValue);
                *pValue = m_verticalChange;
            Cleanup:
                RRETURN(hr);
            }
            _Check_return_ HRESULT put_VerticalChangeImpl(
                _In_ DOUBLE value)
            {
                m_verticalChange = value;
                RRETURN(S_OK);
            }
    };
}
