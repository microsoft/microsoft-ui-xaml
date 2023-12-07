// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#include "precomp.h"
#include "DXamlFontCollectionLoader.h"
#include "DXamlFontFileEnumerator.h"

const GUID DXamlFontCollectionLoader::IID_IDWriteFontCollectionLoader = {
    0xcca920e4,
    0x52f0,
    0x492b,
    { 0xbf, 0xa8, 0x29, 0xc7, 0x2e, 0xe0, 0xa4, 0x68 } };

const GUID DXamlFontCollectionLoader::IID_IUnknown = {
    0x00000000,
    0x0000,
    0x0000,
    { 0xc0, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x46 } };

//---------------------------------------------------------------------------
//
//  Member:
//      DXamlFontCollectionLoader::DXamlFontCollectionLoader
//
//  Synopsis:
//      Initializes a new instance of the DXamlFontCollectionLoader class.
//
//---------------------------------------------------------------------------
DXamlFontCollectionLoader::DXamlFontCollectionLoader() :
    m_referenceCount(1)
{
}

//---------------------------------------------------------------------------
//
//  Member:
//      DXamlFontCollectionLoader::~DXamlFontCollectionLoader
//
//  Synopsis:
//      Release resources associated with the DXamlFontCollectionLoader.
//
//---------------------------------------------------------------------------
DXamlFontCollectionLoader::~DXamlFontCollectionLoader()
{
}

//---------------------------------------------------------------------------
//
//  Member:
//      DXamlFontFileEnumerator::CreateEnumeratorFromKey
//
//  Synopsis:
//      Creates a font file enumerator object that encapsulates a collection 
//      of font files. The font system calls back to this interface to create 
//      a font collection.
//
//---------------------------------------------------------------------------
HRESULT DXamlFontCollectionLoader::CreateEnumeratorFromKey(
        _In_ IDWriteFactory *pFactory,
        _In_reads_bytes_(collectionKeySize) void const *pCollectionKey,
        _In_ XUINT32 collectionKeySize,
        _Out_ IDWriteFontFileEnumerator ** ppFontFileEnumerator
        )
{
    HRESULT hr = S_OK;
    DXamlFontFileEnumerator* pDXamlFontFileEnumerator = NULL;
    pDXamlFontFileEnumerator = new DXamlFontFileEnumerator(pFactory, reinterpret_cast<WCHAR const*>(pCollectionKey));
    *ppFontFileEnumerator = pDXamlFontFileEnumerator;

    RRETURN(hr);//RRETURN_REMOVAL
}

//---------------------------------------------------------------------------
//
//  Member:
//      DXamlFontCollectionLoader::AddRef
//
//  Synopsis:
//      Increments the count of references to this object.
//
//---------------------------------------------------------------------------
ULONG DXamlFontCollectionLoader::AddRef()
{
    return ++m_referenceCount;
}

//---------------------------------------------------------------------------
//
//  Member:
//      DXamlFontCollectionLoader::Release
//
//  Synopsis:
//      Decrements the count of references to this object.
//
//---------------------------------------------------------------------------
ULONG DXamlFontCollectionLoader::Release()
{
    ASSERT(m_referenceCount > 0);

    ULONG referenceCount = --m_referenceCount;

    if (0 == m_referenceCount)
    {
        delete this;
    }

    return referenceCount;
}

//---------------------------------------------------------------------------
//
//  Member:
//      DXamlFontCollectionLoader::QueryInterface
//
//  Synopsis:
//      Retrieves pointers to the supported interfaces on an object.
//
//---------------------------------------------------------------------------
HRESULT DXamlFontCollectionLoader::QueryInterface(
    _In_ REFIID riid,
    _Outptr_ void** ppObject
    )
{
    if (riid == DXamlFontCollectionLoader::IID_IDWriteFontCollectionLoader)
    {
        *ppObject = static_cast<IDWriteFontCollectionLoader*>(this);
        AddRef();
        return S_OK;
    }
    else if (riid == DXamlFontCollectionLoader::IID_IUnknown)
    {
        *ppObject = static_cast<IUnknown*>(static_cast<IDWriteFontCollectionLoader*>(this));
        AddRef();
        return S_OK;
    }
    return E_NOINTERFACE;
}
