// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#include "precomp.h"
#include "HWStrokeStyle.h"

//---------------------------------------------------------------------------
//
//  Initializes a new instance of the HWStrokeStyle class.
//
//---------------------------------------------------------------------------
HWStrokeStyle::HWStrokeStyle(
    _In_ CONST D2D1_STROKE_STYLE_PROPERTIES *strokeStyleProperties
    )
    : m_referenceCount(1)
    , m_styleProperties(*strokeStyleProperties)
{
}

//---------------------------------------------------------------------------
//
//  Release resources associated with the HWStrokeStyle.
//
//---------------------------------------------------------------------------
HWStrokeStyle::~HWStrokeStyle()
{
    m_dashes.clear();
}

HRESULT HWStrokeStyle::QueryInterface(
    REFIID riid,
    _Outptr_ void **ppvObject
    )
{
    ASSERT(FALSE);
    *ppvObject = NULL;
    return E_NOTIMPL;
}

ULONG HWStrokeStyle::AddRef()
{
    return ++m_referenceCount;
}

ULONG HWStrokeStyle::Release()
{
    ASSERT(m_referenceCount > 0);

    XUINT32 referenceCount = --m_referenceCount;
    if (0 == m_referenceCount)
    {
        delete this;
    }
    return referenceCount;
}

void HWStrokeStyle::GetFactory(
    _Outptr_ ID2D1Factory **factory 
    ) const
{
    ASSERT(FALSE);
    *factory = NULL;
}

D2D1_CAP_STYLE HWStrokeStyle::GetStartCap() const
{
    return m_styleProperties.startCap;
}

D2D1_CAP_STYLE HWStrokeStyle::GetEndCap() const
{
    return m_styleProperties.endCap;
}

D2D1_CAP_STYLE HWStrokeStyle::GetDashCap() const
{
    return m_styleProperties.dashCap;
}

FLOAT HWStrokeStyle::GetMiterLimit() const
{
    return m_styleProperties.miterLimit;
}

D2D1_LINE_JOIN HWStrokeStyle::GetLineJoin() const
{
    return m_styleProperties.lineJoin;
}

FLOAT HWStrokeStyle::GetDashOffset() const
{
    return m_styleProperties.dashOffset;
}

D2D1_DASH_STYLE HWStrokeStyle::GetDashStyle() const
{
    return m_styleProperties.dashStyle;
}

UINT32 HWStrokeStyle::GetDashesCount() const
{
    return m_dashes.size();
}

void HWStrokeStyle::GetDashes(
    _Out_writes_(dashesCount) FLOAT *dashes,
    UINT dashesCount
    ) const
{
    ASSERT(dashesCount > 0);
    ASSERT(dashesCount <= m_dashes.size());
    for (UINT i = 0; i < dashesCount; i++)
    {
        dashes[i] = m_dashes[i];
    }
}

//---------------------------------------------------------------------------
//
//  Initializes the dashes array.
//
//---------------------------------------------------------------------------
HRESULT HWStrokeStyle::SetDashes(
    _In_reads_opt_(dashesCount) CONST FLOAT *dashes,
    UINT dashesCount
    )
{
    m_dashes.clear();
    for (UINT i = 0; i < dashesCount; i++)
    {
        IFC_RETURN(m_dashes.push_back(dashes[i]));
    }

    return S_OK;
}
