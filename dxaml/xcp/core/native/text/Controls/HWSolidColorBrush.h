// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

//  Abstract:
//      PC based implementation of ID2D1SolidColorBrush.

#pragma once

//---------------------------------------------------------------------------
//
//  HWSolidColorBrush
//
//  PC based implementation of ID2D1SolidColorBrush.
//
//---------------------------------------------------------------------------
class HWSolidColorBrush final : public ID2D1SolidColorBrush
{
public:

    //
    // ID2D1SolidColorBrush interface
    //

    virtual HRESULT __stdcall QueryInterface(
        REFIID riid,
        _Outptr_ void **ppvObject
        );

    virtual ULONG __stdcall AddRef();

    virtual ULONG __stdcall Release();

    virtual void __stdcall GetFactory(
        _Outptr_ ID2D1Factory **factory
        ) const;

    virtual void __stdcall SetOpacity(
        FLOAT opacity
        );

    virtual void __stdcall SetTransform(
        _In_ CONST D2D1_MATRIX_3X2_F *transform
        );

    virtual FLOAT __stdcall GetOpacity() const;

    virtual void __stdcall GetTransform(
        _Out_ D2D1_MATRIX_3X2_F *transform
        ) const;

    virtual void __stdcall SetColor(
        _In_ CONST D2D1_COLOR_F *color
        );

    virtual D2D1_COLOR_F __stdcall GetColor() const;

    //
    // HWSolidColorBrush implementation
    //

    // Initializes a new instance of the HWSolidColorBrush class.
    HWSolidColorBrush(
        _In_ CONST D2D1_COLOR_F *color
        );

    // Gets color as ARGB value.
    UINT32 GetArgb() const;

private:
    // Count of references to this instance.
    ULONG m_referenceCount;

    // Color of the solid color brush.
    D2D1_COLOR_F m_color;

    // Transform applied to this brush.
    CMILMatrix m_transform;

    // Degree of opacity of this brush.
    FLOAT m_opacity;

    // Release resources associated with the HWSolidColorBrush.
    virtual ~HWSolidColorBrush();
};

// Gets color as ARGB value.
inline UINT32 HWSolidColorBrush::GetArgb() const
{
    BYTE a = (UINT8)(m_color.a * 0xFF);
    BYTE r = (UINT8)(m_color.r * 0xFF);
    BYTE g = (UINT8)(m_color.g * 0xFF);
    BYTE b = (UINT8)(m_color.b * 0xFF);

    return (((UINT32)a) << 24) |
           (((UINT32)r) << 16) |
           (((UINT32)g) <<  8) |
           (((UINT32)b) <<  0);
}
