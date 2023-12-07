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
    _In_ IDWriteFontCollection *pDWriteFontCollection
    ) : m_pDWriteFontCollection(pDWriteFontCollection),
        m_segoeUIFamilyIndex(0),
        m_containsSegoeUIFamily(FALSE)
{
    AddRefInterface(pDWriteFontCollection);
}

//---------------------------------------------------------------------------
//
//  Initializes a new instance of the DWriteFontCollection class.
//
//---------------------------------------------------------------------------
_Check_return_ HRESULT DWriteFontCollection::Create(
    _In_  IDWriteFontCollection *pDWriteFontCollection,
    _Outptr_ PALText::IFontCollection **ppFontCollection)
{
    HRESULT hr = S_OK;
    static const WCHAR* pSegoeUI = L"Segoe UI";
    DWriteFontCollection *pFontCollection = NULL;
    BOOL exists = FALSE;
    XUINT32 segoeUIFamilyIndex = 0;
    
    IFC(pDWriteFontCollection->FindFamilyName(pSegoeUI, &segoeUIFamilyIndex, &exists));
    
    pFontCollection = new DWriteFontCollection(pDWriteFontCollection);
    pFontCollection->m_containsSegoeUIFamily = !!exists;
    pFontCollection->m_segoeUIFamilyIndex = segoeUIFamilyIndex;
    
    *ppFontCollection = pFontCollection;
    pFontCollection = NULL;

Cleanup:
    ReleaseInterface(pFontCollection);
    RRETURN(hr);
}

//---------------------------------------------------------------------------
//
//  Release resources associated with the DWriteFontCollection.
//
//---------------------------------------------------------------------------
DWriteFontCollection::~DWriteFontCollection()
{
    ReleaseInterface(m_pDWriteFontCollection);
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

    IFC(m_pDWriteFontCollection->FindFamilyName(pFamilyName, &index, &exists));

    if (exists)
    {      
        IFC(m_pDWriteFontCollection->GetFontFamily(index, &pFontFamily));
        pDWriteFontFamily = new DWriteFontFamily(
            pFontFamily,
            (m_containsSegoeUIFamily && (index == m_segoeUIFamilyIndex)));
    }

    *ppPhysicalFontFamily = pDWriteFontFamily;

Cleanup:
    ReleaseInterface(pFontFamily);
    RRETURN(hr);
}
