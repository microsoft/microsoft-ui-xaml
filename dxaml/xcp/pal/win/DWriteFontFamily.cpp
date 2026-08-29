// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#include "precomp.h"
#include "DWriteFontFamily.h"
#include "DWriteFontFace.h"
#include "DWriteFontAndScriptServices.h"

//---------------------------------------------------------------------------
//
//  Initializes a new instance of the DWriteFontFamily class.
//
//---------------------------------------------------------------------------
DWriteFontFamily::DWriteFontFamily(
    _In_ IDWriteFontFamily *pDWriteFontFamily,
    _In_ bool useTypographicModel
    ) : m_pDWriteFontFamily(pDWriteFontFamily),
        m_useTypographicModel(useTypographicModel)
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
    Microsoft::WRL::ComPtr<IDWriteFontFace> fontFace;
    Microsoft::WRL::ComPtr<IDWriteFont> font;

    if (m_useTypographicModel)
    {
        Microsoft::WRL::ComPtr<IDWriteFontFamily2> fontFamily2;
        IFC_RETURN(m_pDWriteFontFamily->QueryInterface(IID_PPV_ARGS(&fontFamily2)));
        DWRITE_FONT_AXIS_VALUE axisValues[] = {
            { DWRITE_FONT_AXIS_TAG_WEIGHT, DWriteFontAndScriptServices::DWriteWeightToFontAxisWeight(static_cast<DWRITE_FONT_WEIGHT>(weight)) },
            { DWRITE_FONT_AXIS_TAG_ITALIC, DWriteFontAndScriptServices::DWriteStyleToFontAxisItalic(static_cast<DWRITE_FONT_STYLE>(style)) },
            { DWRITE_FONT_AXIS_TAG_SLANT, DWriteFontAndScriptServices::DWriteStyleToFontAxisSlant(static_cast<DWRITE_FONT_STYLE>(style)) },
            { DWRITE_FONT_AXIS_TAG_WIDTH, DWriteFontAndScriptServices::DWriteStretchToFontAxisWidth(static_cast<DWRITE_FONT_STRETCH>(stretch)) }
        };
        Microsoft::WRL::ComPtr<IDWriteFontList2> matchingFonts;
        hr = fontFamily2->GetMatchingFonts(axisValues, ARRAYSIZE(axisValues), matchingFonts.ReleaseAndGetAddressOf());

        // We don't error on DWRITE_E_REMOTEFONT since it can happen for downloadable fonts and
        // we do not want to trigger the unexpected error handling in IFC.
        if (hr == DWRITE_E_REMOTEFONT) return hr;
        IFC_RETURN(hr);
        if (matchingFonts->GetFontCount() == 0) return S_OK;

        Microsoft::WRL::ComPtr<IDWriteFont> font3;
        hr = matchingFonts->GetFont(0, font3.ReleaseAndGetAddressOf());
        if (hr == DWRITE_E_REMOTEFONT) return hr;
        IFC_RETURN(hr);
        IFC_RETURN(font3.As(&font));
    }
    else
    {
        hr = m_pDWriteFontFamily->GetFirstMatchingFont(static_cast<DWRITE_FONT_WEIGHT>(weight),
                static_cast<DWRITE_FONT_STRETCH>(stretch),
                static_cast<DWRITE_FONT_STYLE>(style),
                font.ReleaseAndGetAddressOf());

        // Explicitly handle this particular error because it can happen for downloadable fonts,
        // and we do not want to trigger the unexpected error handling callback of IFC.
        if (hr == DWRITE_E_REMOTEFONT) return hr;
        IFC_RETURN(hr);
    }

    hr = font->CreateFontFace(fontFace.ReleaseAndGetAddressOf());
    if (hr == DWRITE_E_REMOTEFONT) return hr;
    IFC_RETURN(hr);

    *ppFontFace = new DWriteFontFace(fontFace.Get(), this);

    return S_OK;
}
