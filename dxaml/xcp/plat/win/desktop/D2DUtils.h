// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

//  Description:
//      Helper methods for D2D type conversion

#pragma once

#include <RenderTypes.h>
#include <ColorUtil.h>

inline D2D1_MATRIX_3X2_F *PALToD2DMatrix(_In_ CMILMatrix *pMatrix)
{
    return reinterpret_cast<D2D1_MATRIX_3X2_F *>(pMatrix);
}

inline const D2D1_MATRIX_3X2_F *PALToD2DMatrix(_In_ const CMILMatrix *pMatrix)
{
    return reinterpret_cast<const D2D1_MATRIX_3X2_F *>(pMatrix);
}

inline D2D1_MATRIX_4X4_F *PALToD2DMatrix4X4(_In_ CMILMatrix4x4 *pMatrix)
{
    return reinterpret_cast<D2D1_MATRIX_4X4_F *>(pMatrix);
}

inline const D2D1_MATRIX_4X4_F *PALToD2DMatrix4X4(_In_ const CMILMatrix4x4 *pMatrix)
{
    return reinterpret_cast<const D2D1_MATRIX_4X4_F *>(pMatrix);
}

inline const D3DMATRIX *PALToD3DMatrix(_In_ const CMILMatrix4x4 *pMatrix)
{
    return reinterpret_cast<const D3DMATRIX *>(pMatrix);
}

inline D2D1_POINT_2F *PALToD2DPointF(_In_ XPOINTF *pPoint)
{
    return reinterpret_cast<D2D1_POINT_2F *>(pPoint);
}

inline const D2D1_POINT_2F *PALToD2DPointF(_In_ const XPOINTF *pPoint)
{
    return reinterpret_cast<const D2D1_POINT_2F *>(pPoint);
}

inline D2D1_SIZE_F *PALToD2DSizeF(_In_ XSIZEF *pSize)
{
    return reinterpret_cast<D2D1_SIZE_F *>(pSize);
}

inline const D2D1_SIZE_F *PALToD2DSizeF(_In_ const XSIZEF *pSize)
{
    return reinterpret_cast<const D2D1_SIZE_F *>(pSize);
}

inline D2D1_RECT_F *PALToD2DRectF(_In_ XRECTF_RB *pRect)
{
    return reinterpret_cast<D2D1_RECT_F *>(pRect);
}

inline const D2D1_RECT_F *PALToD2DRectF(_In_ const XRECTF_RB *pRect)
{
    return reinterpret_cast<const D2D1_RECT_F *>(pRect);
}

inline D2D1_GRADIENT_STOP *PALToD2DGradientStops(_In_ XcpGradientStop *pGradientStop)
{
    return reinterpret_cast<D2D1_GRADIENT_STOP *>(pGradientStop);
}

inline D2D1_COLOR_F PALToD2DColor(XUINT32 color)
{
    return D2D1::ColorF(
        static_cast<XFLOAT>(MIL_COLOR_GET_RED(color)) / 255.0f,
        static_cast<XFLOAT>(MIL_COLOR_GET_GREEN(color)) / 255.0f,
        static_cast<XFLOAT>(MIL_COLOR_GET_BLUE(color)) / 255.0f,
        static_cast<XFLOAT>(MIL_COLOR_GET_ALPHA(color)) / 255.0f
        );
}

inline D2D1_MATRIX_3X2_F D2DMatrixMultiply(_In_ const D2D1_MATRIX_3X2_F *pMatrix1, _In_ const D2D1_MATRIX_3X2_F *pMatrix2)
{
    return ((*D2D1::Matrix3x2F::ReinterpretBaseType(pMatrix1)) * (*D2D1::Matrix3x2F::ReinterpretBaseType(pMatrix2)));
}

inline D2D1_MATRIX_4X4_F D2DMatrix4X4Multiply(_In_ const D2D1_MATRIX_4X4_F *pMatrix1, _In_ const D2D1_MATRIX_4X4_F *pMatrix2)
{
    return ((*D2D1::Matrix4x4F::ReinterpretBaseType(pMatrix1)) * (*D2D1::Matrix4x4F::ReinterpretBaseType(pMatrix2)));
}
