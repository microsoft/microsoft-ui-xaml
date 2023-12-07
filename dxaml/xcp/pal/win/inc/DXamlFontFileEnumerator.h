// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

//  Abstract:
//      Implementation of IDWriteFontFileEnumerator.

#pragma once

//---------------------------------------------------------------------------
//
//  DXamlFontFileEnumerator
//
//  DXAML implementation of the DWrite interface IDWriteFontFileEnumerator
//
//---------------------------------------------------------------------------
class DXamlFontFileEnumerator final : public IDWriteFontFileEnumerator
{
public:

    // Initializes a new instance of the DXamlFontFileEnumerator class.
    DXamlFontFileEnumerator(
        _In_   IDWriteFactory *pDWriteFactory,
        _In_z_ WCHAR const    *pUri
        );

    // Increments the count of references to this object.
    ULONG __stdcall AddRef() override;

    // Decrements the count of references to this object.
    ULONG __stdcall Release() override;

    // Retrieves pointers to the supported interfaces on an object.
    HRESULT __stdcall QueryInterface(
        _In_ REFIID riid,
        _Outptr_ void** ppvObject
        ) override;

    // Advances to the next font file in the collection. When it is first created, the enumerator is positioned
    // before the first element of the collection and the first call to MoveNext advances to the first file.
     HRESULT __stdcall MoveNext(_Out_ BOOL* pHasCurrentFile) override;

    // Gets a reference to the current font file.
     HRESULT __stdcall GetCurrentFontFile(_Out_ IDWriteFontFile** ppFontFile) override;

private:

    ULONG m_referenceCount;

    XUINT32 m_index;

    XUINT32 m_count;

    // A Null terminated URI.
    WCHAR const *m_pUri;

    IDWriteFactory *m_pDWriteFactory;


    // Release resources associated with the DXamlFontFileEnumerator.
    ~DXamlFontFileEnumerator();

    static const GUID IID_IDWriteFontFileEnumerator;

    static const GUID IID_IUnknown;
};
