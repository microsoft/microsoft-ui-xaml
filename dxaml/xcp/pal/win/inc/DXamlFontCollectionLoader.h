// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

//  Abstract:
//      Implementation of DXamlFontCollectionLoader.

#pragma once

//---------------------------------------------------------------------------
//
//  DXamlFontCollectionLoader
//
//  DXAML implementation of the DWrite interface DXamlFontCollectionLoader
//
//---------------------------------------------------------------------------
class DXamlFontCollectionLoader final : public IDWriteFontCollectionLoader
{
public:

    // Initializes a new instance of the DXamlFontCollectionLoader class.
    DXamlFontCollectionLoader();

    // Increments the count of references to this object.
    ULONG __stdcall AddRef() override;

    // Decrements the count of references to this object.
    ULONG __stdcall Release() override;

    // Retrieves pointers to the supported interfaces on an object.
    HRESULT __stdcall QueryInterface(
        _In_ REFIID riid,
        _Outptr_ void** ppvObject
        ) override;

    // Creates a font file enumerator object that encapsulates a collection of font files.
    // The font system calls back to this interface to create a font collection.
     HRESULT __stdcall CreateEnumeratorFromKey(
        _In_ IDWriteFactory *pFactory,
        _In_reads_bytes_(collectionKeySize) void const *pCollectionKey,
        _In_ XUINT32 collectionKeySize,
        _Out_ IDWriteFontFileEnumerator ** ppFontFileEnumerator
        ) override;

private:

    ULONG m_referenceCount;

    // Release resources associated with the DXamlFontFileEnumerator.
    ~DXamlFontCollectionLoader();

    static const GUID IID_IDWriteFontCollectionLoader;

    static const GUID IID_IUnknown;
};
