// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#pragma once

#ifdef DBG
#define BOXER_BUFFER_SET_STORED_TYPE(type) m_stored_type = (type);
#define BOXER_BUFFER_CHECK_STORED_TYPE(type) ASSERT(m_stored_type == (type));
#else
#define BOXER_BUFFER_SET_STORED_TYPE(type) ;
#define BOXER_BUFFER_CHECK_STORED_TYPE(type) ;
#endif

namespace DirectUI
{
    struct XMATRIX
    {
        FLOAT m11;
        FLOAT m12;
        FLOAT m21;
        FLOAT m22;
        FLOAT offsetX;
        FLOAT offsetY;
    };

    struct XMATRIX3D
    {
        XFLOAT m11;
        XFLOAT m12;
        XFLOAT m13;
        XFLOAT m14;
        XFLOAT m21;
        XFLOAT m22;
        XFLOAT m23;
        XFLOAT m24;
        XFLOAT m31;
        XFLOAT m32;
        XFLOAT m33;
        XFLOAT m34;
        XFLOAT offsetX;
        XFLOAT offsetY;
        XFLOAT offsetZ;
        XFLOAT m44;
    };

    // Used to store values on the stack of a boxing method caller.
    class BoxerBuffer
    {
    private:

#if DBG
        enum class stored_type
        {
            none,
            point,
            rect,
            thickness,
            cornerradius,
            size,
            gridlength,
            matrix,
            matrix3d
        };

        stored_type m_stored_type = stored_type::none;
#endif

    public:
        BoxerBuffer() = default;

        void SetPoint(
            float x,
            float y)
        {
            m_data.m_point.x = x;
            m_data.m_point.y = y;

            BOXER_BUFFER_SET_STORED_TYPE(stored_type::point);
        }

        const XPOINTF* GetPoint() const
        {
            BOXER_BUFFER_CHECK_STORED_TYPE(stored_type::point);
            return &m_data.m_point;
        }

        void SetRect(
            float x,
            float y,
            float width,
            float height)
        {
            m_data.m_rect.X = x;
            m_data.m_rect.Y = y;
            m_data.m_rect.Width = width;
            m_data.m_rect.Height = height;

            BOXER_BUFFER_SET_STORED_TYPE(stored_type::rect);
        }

        const XRECTF* GetRect() const
        {
            BOXER_BUFFER_CHECK_STORED_TYPE(stored_type::rect);
            return &m_data.m_rect;
        }

        void SetThickness(
            float left,
            float top,
            float right,
            float bottom)
        {
            m_data.m_thickness.left = left;
            m_data.m_thickness.top = top;
            m_data.m_thickness.right = right;
            m_data.m_thickness.bottom = bottom;

            BOXER_BUFFER_SET_STORED_TYPE(stored_type::thickness);
        }

        const XTHICKNESS* GetThickness() const
        {
            BOXER_BUFFER_CHECK_STORED_TYPE(stored_type::thickness);
            return &m_data.m_thickness;
        }

        void SetCornerRadius(
            float bottomLeft,
            float bottomRight,
            float topLeft,
            float topRight)
        {
            m_data.m_cornerradius.bottomLeft = bottomLeft;
            m_data.m_cornerradius.bottomRight = bottomRight;
            m_data.m_cornerradius.topLeft = topLeft;
            m_data.m_cornerradius.topRight = topRight;

            BOXER_BUFFER_SET_STORED_TYPE(stored_type::cornerradius);
        }

        const XCORNERRADIUS* GetCornerRadius() const
        {
            BOXER_BUFFER_CHECK_STORED_TYPE(stored_type::cornerradius);
            return &m_data.m_cornerradius;
        }

        void SetSize(
            float width,
            float height)
        {
            m_data.m_size.width = width;
            m_data.m_size.height = height;

            BOXER_BUFFER_SET_STORED_TYPE(stored_type::size);
        }

        const XSIZEF* GetSize() const
        {
            BOXER_BUFFER_CHECK_STORED_TYPE(stored_type::size);
            return &m_data.m_size;
        }

        void SetGridLength(
            DirectUI::GridUnitType type,
            float value)
        {
            m_data.m_gridlength.type = type;
            m_data.m_gridlength.value = value;

            BOXER_BUFFER_SET_STORED_TYPE(stored_type::gridlength);
        }

        const XGRIDLENGTH* GetGridLength() const
        {
            BOXER_BUFFER_CHECK_STORED_TYPE(stored_type::gridlength);
            return &m_data.m_gridlength;
        }

        void SetMatrix(
            float m11,
            float m12,
            float m21,
            float m22,
            float offsetX,
            float offsetY)
        {
            m_data.m_matrix.m11 = m11;
            m_data.m_matrix.m12 = m12;
            m_data.m_matrix.m21 = m21;
            m_data.m_matrix.m22 = m22;
            m_data.m_matrix.offsetX = offsetX;
            m_data.m_matrix.offsetY = offsetY;

            BOXER_BUFFER_SET_STORED_TYPE(stored_type::matrix);
        }

        const XMATRIX* GetMatrix() const
        {
            BOXER_BUFFER_CHECK_STORED_TYPE(stored_type::matrix);
            return &m_data.m_matrix;
        }

        void SetMatrix3D(
            float m11,
            float m12,
            float m13,
            float m14,
            float m21,
            float m22,
            float m23,
            float m24,
            float m31,
            float m32,
            float m33,
            float m34,
            float offsetX,
            float offsetY,
            float offsetZ,
            float m44)
        {
            m_data.m_matrix3d.m11 = m11;
            m_data.m_matrix3d.m12 = m12;
            m_data.m_matrix3d.m13 = m13;
            m_data.m_matrix3d.m14 = m14;
            m_data.m_matrix3d.m21 = m21;
            m_data.m_matrix3d.m22 = m22;
            m_data.m_matrix3d.m23 = m23;
            m_data.m_matrix3d.m24 = m24;
            m_data.m_matrix3d.m31 = m31;
            m_data.m_matrix3d.m32 = m32;
            m_data.m_matrix3d.m33 = m33;
            m_data.m_matrix3d.m34 = m34;
            m_data.m_matrix3d.offsetX = offsetX;
            m_data.m_matrix3d.offsetY = offsetY;
            m_data.m_matrix3d.offsetZ = offsetZ;
            m_data.m_matrix3d.m44 = m44;

            BOXER_BUFFER_SET_STORED_TYPE(stored_type::matrix3d);
        }

        const XMATRIX3D* GetMatrix3D() const
        {
            BOXER_BUFFER_CHECK_STORED_TYPE(stored_type::matrix3d);
            return &m_data.m_matrix3d;
        }

    private:
        union
        {
            XPOINTF       m_point;
            XRECTF        m_rect;
            XTHICKNESS    m_thickness;
            XCORNERRADIUS m_cornerradius;
            XSIZEF        m_size;
            XGRIDLENGTH   m_gridlength;
            XMATRIX       m_matrix;
            XMATRIX3D     m_matrix3d;
        } m_data = {};
    };
}