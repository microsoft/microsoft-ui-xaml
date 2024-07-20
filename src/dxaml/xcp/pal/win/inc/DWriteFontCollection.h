// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

//  Abstract:
//      DWrite based implementation of IFontCollection.

#pragma once

//---------------------------------------------------------------------------
//
//  DWriteFontCollection
//
//  DWrite based implementation of IFontCollection.
//
//---------------------------------------------------------------------------
class DWriteFontCollection : public CXcpObjectBase<PALText::IFontCollection, CXcpObjectAddRefPolicy>
{
public:

     static _Check_return_ HRESULT Create(
         _In_  IDWriteFontCollection *pDWriteFontCollection,
         _Outptr_ PALText::IFontCollection **ppFontCollection);

    // Looks for a physical font in the font collection.
    HRESULT LookupPhysicalFontFamily(
        _In_z_ WCHAR const *pFamilyName,
        _Outptr_result_maybenull_ PALText::IFontFamily **ppPhysicalFontFamily
        ) override;

private:
    // TODO: Consider exposing m_pDWriteFontCollection via a public getter.
    friend class FontFallbackBuilderWrapper;
    friend class FontFallbackWrapper;
    friend class DWriteFontAndScriptServices;
    friend class CTextBlock;

    // DWrite's peer associated with this object.
    IDWriteFontCollection *m_pDWriteFontCollection;

    // Index of the Segoe UI font family in the current font collection.
    XUINT32 m_segoeUIFamilyIndex;

    // This checks whether Segoe UI is available in this font collection.
    // It might not be found if this is a custom font collection
    // (or somebody removed it from the system which is unlikely)
    bool m_containsSegoeUIFamily;

    // Initializes a new instance of the DWriteFontCollection class.
    DWriteFontCollection(
        _In_ IDWriteFontCollection *pDWriteFontCollection
        );

    // Release resources associated with the DWriteFontCollection.
    ~DWriteFontCollection() override;
};
