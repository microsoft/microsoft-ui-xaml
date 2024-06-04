// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#include "precomp.h"

using namespace DirectUI;
using namespace RichTextServices;

//---------------------------------------------------------------------------
//
//  Member:
//      TextParagraphProperties::TextParagraphProperties()
//
//  Synopsis:
//      Constructor.
//
//---------------------------------------------------------------------------
TextParagraphProperties::TextParagraphProperties(
    _In_ RichTextServices::FlowDirection::Enum flowDirection,
        // Paragraph's flow direction.
    _In_ TextRunProperties *pDefaultTextRunProperties,
        // Paragraph's default run properties.
    _In_ XFLOAT firstLineIndent,
    _In_ TextWrapping textWrapping,
    _In_ TextLineBounds textLineBounds,
    DirectUI::TextAlignment textAlignment
    )
    : m_flowDirection(flowDirection)
    , m_pDefaultTextRunProperties(pDefaultTextRunProperties)
    , m_firstLineIndent(firstLineIndent)
    , m_textWrapping(textWrapping)
    , m_textLineBounds(textLineBounds)
    , m_textAlignment(textAlignment)
    , m_flags(0)
{
    ASSERT(pDefaultTextRunProperties);
    pDefaultTextRunProperties->AddRef();
}

//---------------------------------------------------------------------------
//
//  Member:
//      TextParagraphProperties::~TextParagraphProperties()
//
//  Synopsis:
//      Destructor.
//
//---------------------------------------------------------------------------
TextParagraphProperties::~TextParagraphProperties()
{
    ReleaseInterface(m_pDefaultTextRunProperties);
}

