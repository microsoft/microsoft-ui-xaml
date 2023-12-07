// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#include "precomp.h"
#include "DWriteFontFamily.h"
#include "DWriteFontFace.h"

//---------------------------------------------------------------------------
//
//  Initializes a new instance of the DWriteFontFamily class.
//
//---------------------------------------------------------------------------
DWriteFontFamily::DWriteFontFamily(
    _In_ IDWriteFontFamily *pDWriteFontFamily,
    _In_ bool canOptimizeShaping
    ) : m_pDWriteFontFamily(pDWriteFontFamily),
        m_canOptimizeShaping(canOptimizeShaping)
{
    AddRefInterface(pDWriteFontFamily);
}

//---------------------------------------------------------------------------
//
//  Release resources associated with the DWriteFontFamily.
//
//---------------------------------------------------------------------------
DWriteFontFamily::~DWriteFontFamily()
{
    ReleaseInterface(m_pDWriteFontFamily);
}

//---------------------------------------------------------------------------
//
//  For a given physical font typeface determines which glyph typeface to
//  use for rendering the missing glyph.
//
//---------------------------------------------------------------------------
HRESULT DWriteFontFamily::LookupNominalFontFace(
    _In_ XUINT32 weight,
    _In_ XUINT32 style, 
    _In_ XUINT32 stretch,
    _Outptr_ PALText::IFontFace **ppFontFace
    )
{
    HRESULT hr = S_OK;
    IDWriteFont     *pFont     = NULL;
    IDWriteFontFace *pFontFace = NULL;

    hr = m_pDWriteFontFamily->GetFirstMatchingFont(static_cast<DWRITE_FONT_WEIGHT>(weight),
            static_cast<DWRITE_FONT_STRETCH>(stretch),
            static_cast<DWRITE_FONT_STYLE>(style),
            &pFont);

    // Explicitly handle this particular error because it can happen for downloadable fonts,
    // and we do not want to trigger the unexpected error handling callback of IFC.
    if (hr == DWRITE_E_REMOTEFONT)
        goto Cleanup;

    IFC(hr);

    hr = pFont->CreateFontFace(&pFontFace);
    if (hr == DWRITE_E_REMOTEFONT)
        goto Cleanup;

    IFC(hr);

    *ppFontFace = new DWriteFontFace(pFontFace, this);

Cleanup:
    ReleaseInterface(pFontFace);
    ReleaseInterface(pFont);
    RRETURN(hr);
}
