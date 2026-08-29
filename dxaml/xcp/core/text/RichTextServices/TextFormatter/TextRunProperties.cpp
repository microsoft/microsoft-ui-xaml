// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#include "precomp.h"

using namespace RichTextServices;

//---------------------------------------------------------------------------
//
//  Member:
//      TextRunProperties::TextRunProperties
//
//  Synopsis:
//      Initializes a new instance of the TextRunProperties class.
//
//---------------------------------------------------------------------------
TextRunProperties::TextRunProperties(
    _In_ FontTypeface *pFontTypeface,
        // Font typeface representing font family, weight, style, stretch and language.
    _In_ XFLOAT fontSize,
        // Font rendering em size for run's character data.
    _In_ bool hasUnderline,
        // Value indicating whether underline is present.
    _In_ bool hasStrikethrough,
    _In_ XINT32 characterSpacing,
    _In_ xref::weakref_ptr<CDependencyObject> pForegroundBrushSource,
    _In_ CultureInfo strCultureInfo,
    _In_ CultureInfo strCultureListInfo,

        // Pointer to the source of properties.
    _In_opt_ const InheritedProperties *pInheritedProperties
        // Optional collection of over 40 OpenType feature selections
    ) :
    m_pFontTypeface(pFontTypeface),
    m_fontSize(fontSize),
    m_pForegroundBrushSource(std::move(pForegroundBrushSource)),
    m_hasStrikethrough(hasStrikethrough),
    m_hasUnderline(hasUnderline),
    m_characterSpacing(characterSpacing),
    m_pInheritedProperties(pInheritedProperties),
    m_strCultureInfo(strCultureInfo),
    m_strCultureListInfo(strCultureListInfo)
{
    AddRefInterface(m_pFontTypeface);
    AddRefInterface(m_pInheritedProperties);
}

//---------------------------------------------------------------------------
//
//  TextRunProperties destructor
//
//---------------------------------------------------------------------------
TextRunProperties::~TextRunProperties()
{
    ReleaseInterface(m_pFontTypeface);
    InheritedProperties::ReleaseFromNonDependencyObject(&m_pInheritedProperties);
}

//---------------------------------------------------------------------------
//
//  Member:
//      TextRunProperties::EqualsForShaping
//
//  Synopsis:
//      Checks for equality with another TextRunProperties object, for purposes
//      of shaping.
//
//  Remarks:
//      Full equality of text runs is not required to shape across multiple runs. 
//      This method previously checked for full equality, but this meant that 
//      it was not possible to have both correct shaping and variable sub-word 
//      coloring (Windows Blue Bug# 263695). Now the checks include all 
//      TextRunProperties except for the foreground brush and underlining.        
//
//---------------------------------------------------------------------------
bool TextRunProperties::EqualsForShaping(
    _In_ const TextRunProperties *pProperties
        // Pointer to TextRunProperties object to be compared.
    ) const
{
    bool equals = false;

    if (pProperties != NULL)
    {
        if (m_pFontTypeface == pProperties->m_pFontTypeface
            &&  m_fontSize == pProperties->m_fontSize
            &&  m_characterSpacing == pProperties->m_characterSpacing
            && (m_pInheritedProperties == pProperties->m_pInheritedProperties
            || (m_pInheritedProperties != NULL
            &&  pProperties->m_pInheritedProperties != NULL
            &&  m_pInheritedProperties->m_typography.IsTypographySame(&pProperties->m_pInheritedProperties->m_typography))))
        {
            equals = m_strCultureInfo.Equals(pProperties->m_strCultureInfo);
        }
    }

    return equals;
}

