// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

//  Abstract:
//      The OrientedSize structure is used to abstract the growth direction from
//      the layout algorithms of custom panels.

#pragma once

namespace DirectUI
{
    // The OrientedSize structure is used to abstract the growth direction from
    // the layout algorithms of custom panels.  When the growth direction is
    // oriented horizontally (ex: the next element is arranged on the side of
    // the previous element), then the Width grows directly with the placement
    // of elements and Height grows indirectly with the size of the largest
    // element in the row.  When the orientation is reversed, so is the
    // directional growth with respect to Width and Height.
    struct OrientedSize
    {
        private:
            // The orientation of the size.
            xaml_controls::Orientation m_orientation;
            
            // The size dimension that grows directly with layout orientation.
            XDOUBLE m_direct;
            
            // The size dimension that grows indirectly with layout orientation.
            XDOUBLE m_indirect;
            
        public:
            // Initializes a new instance of the OrientedSize structure.
            OrientedSize()
                : m_orientation(xaml_controls::Orientation_Vertical)
                , m_direct(0.0)
                , m_indirect(0.0)
            {
            }
            
            // Gets the orientation of the size.
            xaml_controls::Orientation get_Orientation()
            {
                return m_orientation;
            }
            
            // Sets the orientation of the size.
            void put_Orientation(
                _In_ xaml_controls::Orientation orientation)
            {
                m_orientation = orientation;
            }
            
            // Gets the size dimension that grows directly with layout
            // orientation.
            XDOUBLE get_Direct()
            {
                return m_direct;
            }
            
            // Sets the size dimension that grows directly with layout
            // orientation.
            void put_Direct(
                _In_ XDOUBLE direct)
            {
                m_direct = direct;
            }
            
            // Gets the size dimension that grows indirectly with layout
            // orientation.
            XDOUBLE get_Indirect()
            {
                return m_indirect;
            }
            
            // Sets the size dimension that grows indirectly with layout
            // orientation.
            void put_Indirect(
                _In_ XDOUBLE indirect)
            {
                m_indirect = indirect;
            }
            
            // Gets the width of the size.
            XDOUBLE get_Width()
            {
                return m_orientation == xaml_controls::Orientation_Horizontal ?
                    m_direct :
                    m_indirect;
            }
            
            // Sets the width of the size.
            void put_Width(
                _In_ XDOUBLE value)
            {
                if (m_orientation == xaml_controls::Orientation_Horizontal)
                {
                    m_direct = value;
                }
                else
                {
                    m_indirect = value;
                }
            }
            
            // Gets the height of the size.
            XDOUBLE get_Height()
            {
                return m_orientation == xaml_controls::Orientation_Horizontal ?
                    m_indirect :
                    m_direct;
            }
            
            // Sets the height of the size.
            void put_Height(
                _In_ XDOUBLE value)
            {
                if (m_orientation == xaml_controls::Orientation_Horizontal)
                {
                    m_indirect = value;
                }
                else
                {
                    m_direct = value;
                }
            }
            
            // Helper to convert the OrientedSize into a regular Size value.
            _Check_return_ HRESULT AsUnorientedSize(
                _In_ wf::Size* pSize)
            {
                HRESULT hr = S_OK;
                IFCPTR(pSize);
                
                pSize->Width = (float)get_Width();
                pSize->Height = (float)get_Height();
                
            Cleanup:
                RRETURN(hr);
            }
    };
}
