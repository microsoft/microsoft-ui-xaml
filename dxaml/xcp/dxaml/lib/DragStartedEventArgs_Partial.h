// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#pragma once

#include "DragStartedEventArgs.g.h"

namespace DirectUI
{
    // Provides data for the DragStarted event that occurs when a user drags a
    // Thumb control with the mouse.
    PARTIAL_CLASS(DragStartedEventArgs)
    {
        private:
            // The horizontal distance between the current mouse position and
            // the thumb coordinates.
            DOUBLE m_horizontalOffset;
            
            // The vertical distance between the current mouse position and the
            // thumb coordinates.
            DOUBLE m_verticalOffset;
            
        public:
            // Initializes a new instance of the DragStartedEventArgs class.
            DragStartedEventArgs()
                : m_horizontalOffset(0.0)
                , m_verticalOffset(0.0)
            {
            }
            
            // Gets the horizontal distance between the current mouse position
            // and the thumb coordinates.
            _Check_return_ HRESULT get_HorizontalOffsetImpl(
                _Out_ DOUBLE* pValue)
            {
                HRESULT hr = S_OK;
                IFCPTR(pValue);
                *pValue = m_horizontalOffset;
            Cleanup:
                RRETURN(hr);
            }
            _Check_return_ HRESULT put_HorizontalOffsetImpl(
                _In_ DOUBLE value)
            {
                m_horizontalOffset = value;
                RRETURN(S_OK);
            }
            
            // Gets the vertical distance between the current mouse position and
            // the thumb coordinates.
            _Check_return_ HRESULT get_VerticalOffsetImpl(
                _Out_ DOUBLE* pValue)
            {
                HRESULT hr = S_OK;
                IFCPTR(pValue);
                *pValue = m_verticalOffset;
            Cleanup:
                RRETURN(hr);
            }
            _Check_return_ HRESULT put_VerticalOffsetImpl(
                _In_ DOUBLE value)
            {
                m_verticalOffset = value;
                RRETURN(S_OK);
            }
    };
}
