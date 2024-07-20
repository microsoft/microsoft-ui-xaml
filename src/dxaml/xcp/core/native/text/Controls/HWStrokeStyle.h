// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

//  Abstract:
//      PC based implementation of ID2D1StrokeStyle.

#pragma once

//---------------------------------------------------------------------------
//
//  HWStrokeStyle
//
//  PC based implementation of ID2D1StrokeStyle.
//
//---------------------------------------------------------------------------
class HWStrokeStyle final : public ID2D1StrokeStyle
{
public:

    //
    // ID2D1StrokeStyle interface
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

    virtual D2D1_CAP_STYLE __stdcall GetStartCap() const;

    virtual D2D1_CAP_STYLE __stdcall GetEndCap() const;

    virtual D2D1_CAP_STYLE __stdcall GetDashCap() const;

    virtual FLOAT __stdcall GetMiterLimit() const;

    virtual D2D1_LINE_JOIN __stdcall GetLineJoin() const;

    virtual FLOAT __stdcall  GetDashOffset() const;

    virtual D2D1_DASH_STYLE __stdcall GetDashStyle() const;

    virtual UINT32 __stdcall GetDashesCount() const;

    virtual void __stdcall GetDashes(
        _Out_writes_(dashesCount) FLOAT *dashes,
        UINT dashesCount
        ) const;

    //
    // HWStrokeStyle implementation
    //

    // Initializes a new instance of the HWStrokeStyle class.
    HWStrokeStyle(
        _In_ CONST D2D1_STROKE_STYLE_PROPERTIES *strokeStyleProperties
        );

    // Initializes the dashes array.
    HRESULT SetDashes(
        _In_reads_opt_(dashesCount) CONST FLOAT *dashes,
        UINT dashesCount
        );

private:
    // Count of references to this instance.
    ULONG m_referenceCount;

    // Stroke style properties.
    D2D1_STROKE_STYLE_PROPERTIES m_styleProperties;

    // Dashes array.
    xvector<FLOAT> m_dashes;

    // Release resources associated with the HWStrokeStyle.
    virtual ~HWStrokeStyle();
};
