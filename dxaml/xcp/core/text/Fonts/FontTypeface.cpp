// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#include "precomp.h"

//------------------------------------------------------------------------
//
//  Method: CFontTypeface::MapCharacters
//
//  Map characters from the specified range in the ITextAnalysisSource to
//  a single font.
//
//------------------------------------------------------------------------

_Check_return_ HRESULT CFontTypeface::MapCharacters(
    _In_z_ const WCHAR *pText,
    XUINT32 textLength,
    _In_z_ const WCHAR *pLocaleName,
    _In_z_ const WCHAR *pLocaleNameList,
    _In_opt_ const NumberSubstitutionData* pNumberSubstitutionData,
    _Outptr_ IFssFontFace **ppMappedFontFace,
    _Out_ XUINT32 *pMappedLength,
    _Out_ XFLOAT *pMappedScale
    )
{
    return m_pCompositeFontFamily->MapCharacters(
        pText,
        textLength,
        pLocaleName,
        pLocaleNameList,
        pNumberSubstitutionData,
        m_weightStyleStretch,
        ppMappedFontFace,
        pMappedLength,
        pMappedScale
    );
}



//------------------------------------------------------------------------
//
//  Method: CFontTypeface destructor
//
//------------------------------------------------------------------------

CFontTypeface::~CFontTypeface()
{
}
