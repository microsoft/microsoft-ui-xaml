// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#include "precomp.h"
#include "DWriteFontCollection.h"
#include "DWriteFontFamily.h"

//---------------------------------------------------------------------------
//
//  Initializes a new instance of the DWriteFontCollection class.
//
//---------------------------------------------------------------------------
DWriteFontCollection::DWriteFontCollection(
    _In_ IDWriteFontCollection2 *pDWriteFontCollection
    ) : m_dwriteFontCollection(pDWriteFontCollection)
{
}

//---------------------------------------------------------------------------
//
//  Initializes a new instance of the DWriteFontCollection class.
//
//---------------------------------------------------------------------------
_Check_return_ HRESULT DWriteFontCollection::Create(
    _In_  IDWriteFontCollection2 *pDWriteFontCollection,
    _Outptr_ PALText::IFontCollection **ppFontCollection)
{
    Microsoft::WRL::ComPtr<DWriteFontCollection> fontCollection;
    fontCollection.Attach(new DWriteFontCollection(pDWriteFontCollection));

    *ppFontCollection = fontCollection.Detach();
    return S_OK;
}

//---------------------------------------------------------------------------
//
//  Looks for a physical font in the font collection.
//
//---------------------------------------------------------------------------  
HRESULT DWriteFontCollection::LookupPhysicalFontFamily(
    _In_z_ WCHAR const *pFamilyName,
    _Outptr_result_maybenull_ PALText::IFontFamily **ppPhysicalFontFamily
    ) 
{
    HRESULT hr = S_OK;
    XUINT32 index;
    BOOL exists;
    IDWriteFontFamily *pFontFamily = NULL;
    DWriteFontFamily *pDWriteFontFamily = NULL;

    IFC(m_dwriteFontCollection->FindFamilyName(pFamilyName, &index, &exists));

    if (exists)
    {      
        IFC(m_dwriteFontCollection->GetFontFamily(index, &pFontFamily));
        pDWriteFontFamily = new DWriteFontFamily(
            pFontFamily,
            m_dwriteFontCollection->GetFontFamilyModel() == DWRITE_FONT_FAMILY_MODEL_TYPOGRAPHIC);
    }

    *ppPhysicalFontFamily = pDWriteFontFamily;

Cleanup:
    ReleaseInterface(pFontFamily);
    RRETURN(hr);
}
