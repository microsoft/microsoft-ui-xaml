// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

//  Abstract:
//      Provides data for the DragCompleted event that occurs when a user
//      completes a drag operation with the mouse of a Thumb control

#pragma once

#include "DragCompletedEventArgs.g.h"

namespace DirectUI
{
    // Provides data for the DragCompleted event that occurs when a user
    // completes a drag operation with the mouse of a Thumb control
    PARTIAL_CLASS(DragCompletedEventArgs)
    {
        private:
            // Gets the horizontal distance between the current mouse position
            // and the thumb coordinates.
            DOUBLE m_horizontalChange;
            
            // Gets the vertical distance between the current mouse position and
            // the thumb coordinates.
            DOUBLE m_verticalChange;
            
            // Gets a value that indicates whether the drag operation was
            // canceled.
            BOOLEAN m_canceled;
            
        public:
            // Initializes a new instance of the DragCompletedEventArgs class.
            DragCompletedEventArgs()
                : m_horizontalChange(0.0)
                , m_verticalChange(0.0)
                , m_canceled(FALSE)
            {
            }
            
            // Gets the horizontal distance between the current mouse position
            // and the thumb coordinates.
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
            
            // Gets the vertical distance between the current mouse position and
            // the thumb coordinates.
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
            
            // Gets a value that indicates whether the drag operation was
            // canceled.
            _Check_return_ HRESULT get_CanceledImpl(
                _Out_ BOOLEAN* pValue)
            {
                HRESULT hr = S_OK;
                IFCPTR(pValue);
                *pValue = m_canceled;
            Cleanup:
                RRETURN(hr);
            }
            _Check_return_ HRESULT put_CanceledImpl(
                _In_ BOOLEAN value)
            {
                m_canceled = value;
                RRETURN(S_OK);
            }
    };
}
