// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#include "precomp.h"
#include "HWSolidColorBrush.h"
#include "TextHelpers.h"

//---------------------------------------------------------------------------
//
//  Initializes a new instance of the HWSolidColorBrush class.
//
//---------------------------------------------------------------------------
HWSolidColorBrush::HWSolidColorBrush(
    _In_ CONST D2D1_COLOR_F *color
    )
    : m_referenceCount(1)
    , m_color(*color)
    , m_transform(TRUE)
    , m_opacity(1)
{
}

//---------------------------------------------------------------------------
//
//  Release resources associated with the HWSolidColorBrush.
//
//---------------------------------------------------------------------------
HWSolidColorBrush::~HWSolidColorBrush()
{
}

HRESULT HWSolidColorBrush::QueryInterface(
    REFIID riid,
    _Outptr_ void **ppvObject
    )
{
    ASSERT(FALSE);
    *ppvObject = NULL;
    return E_NOTIMPL;
}

ULONG HWSolidColorBrush::AddRef()
{
    return ++m_referenceCount;
}

ULONG HWSolidColorBrush::Release()
{
    ASSERT(m_referenceCount > 0);

    XUINT32 referenceCount = --m_referenceCount;
    if (0 == m_referenceCount)
    {
        delete this;
    }
    return referenceCount;
}

void HWSolidColorBrush::GetFactory(
    _Outptr_ ID2D1Factory **factory 
    ) const
{
    ASSERT(FALSE);
    *factory = NULL;
}

void HWSolidColorBrush::SetOpacity(
    FLOAT opacity 
    )
{
    m_opacity = opacity;
}

void HWSolidColorBrush::SetTransform(
    _In_ CONST D2D1_MATRIX_3X2_F *transform 
    )
{
    m_transform = GetMILMatrix(*D2D1::Matrix3x2F::ReinterpretBaseType(transform));
}

FLOAT HWSolidColorBrush::GetOpacity(
    ) const
{
    return m_opacity;
}

void HWSolidColorBrush::GetTransform(
    _Out_ D2D1_MATRIX_3X2_F *transform 
    ) const
{
    *transform = GetD2DMatrix(m_transform);
}

void HWSolidColorBrush::SetColor(
    _In_ CONST D2D1_COLOR_F *color 
    )
{
    m_color = *color;
}

D2D1_COLOR_F HWSolidColorBrush::GetColor(
    ) const
{
    return m_color;
}
