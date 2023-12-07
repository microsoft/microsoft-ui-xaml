// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#include "precomp.h"
#include "DXamlFontFileEnumerator.h"

const GUID DXamlFontFileEnumerator::IID_IDWriteFontFileEnumerator = {
    0x72755049,
    0x5ff7,
    0x435d,
    { 0x83, 0x48, 0x4b, 0xe9, 0x7c, 0xfa, 0x6c, 0x7c } };

const GUID DXamlFontFileEnumerator::IID_IUnknown = {
    0x00000000,
    0x0000,
    0x0000,
    { 0xc0, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x46 } };

//---------------------------------------------------------------------------
//
//  Member:
//      DXamlFontFileEnumerator::DXamlFontFileEnumerator
//
//  Synopsis:
//      Initializes a new instance of the DXamlFontFileEnumerator class.
//
//---------------------------------------------------------------------------
DXamlFontFileEnumerator::DXamlFontFileEnumerator(
     _In_   IDWriteFactory *pDWriteFactory,
     _In_z_ WCHAR const    *pUri
    ) :
    m_referenceCount(1),
    m_index(0),
    m_count(1),
    m_pUri(pUri),
    m_pDWriteFactory(pDWriteFactory)
{
    AddRefInterface(m_pDWriteFactory);
}

//---------------------------------------------------------------------------
//
//  Member:
//      DXamlFontFileEnumerator::~DXamlFontFileEnumerator
//
//  Synopsis:
//      Release resources associated with the DXamlFontFileEnumerator.
//
//---------------------------------------------------------------------------
DXamlFontFileEnumerator::~DXamlFontFileEnumerator()
{
    ReleaseInterface(m_pDWriteFactory);
}

//---------------------------------------------------------------------------
//
//  Member:
//      DXamlFontFileEnumerator::MoveNext
//
//  Synopsis:
//      Advances to the next font file in the collection. When it is first created, the enumerator is positioned
//      before the first element of the collection and the first call to MoveNext advances to the first file.
//
//---------------------------------------------------------------------------
HRESULT DXamlFontFileEnumerator::MoveNext(_Out_ BOOL* pHasCurrentFile)
{
    if (m_index < m_count)
    {
        m_index++;
        *pHasCurrentFile = TRUE;
    }
    else
    {
        *pHasCurrentFile = FALSE;
    }

    return S_OK;
}

//---------------------------------------------------------------------------
//
//  Member:
//      DXamlFontFileEnumerator::GetCurrentFontFile
//
//  Synopsis:
//      Gets a reference to the current font file.
//
//---------------------------------------------------------------------------
HRESULT DXamlFontFileEnumerator::GetCurrentFontFile(_Out_ IDWriteFontFile** ppFontFile)
{
    HRESULT hr = S_OK;   
    IFC(m_pDWriteFactory->CreateFontFileReference(m_pUri,
                                                  NULL,          //_In_opt_ FILETIME const* lastWriteTime
                                                  ppFontFile));

Cleanup:
    RRETURN(hr);
}

//---------------------------------------------------------------------------
//
//  Member:
//      DXamlFontFileEnumerator::AddRef
//
//  Synopsis:
//      Increments the count of references to this object.
//
//---------------------------------------------------------------------------
ULONG DXamlFontFileEnumerator::AddRef()
{
    return ++m_referenceCount;
}

//---------------------------------------------------------------------------
//
//  Member:
//      DXamlFontFileEnumerator::Release
//
//  Synopsis:
//      Decrements the count of references to this object.
//
//---------------------------------------------------------------------------
ULONG DXamlFontFileEnumerator::Release()
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
//      DXamlFontFileEnumerator::QueryInterface
//
//  Synopsis:
//      Retrieves pointers to the supported interfaces on an object.
//
//---------------------------------------------------------------------------
HRESULT DXamlFontFileEnumerator::QueryInterface(
    _In_ REFIID riid,
    _Outptr_ void** ppObject
    )
{
    if (riid == DXamlFontFileEnumerator::IID_IDWriteFontFileEnumerator)
    {
        *ppObject = static_cast<IDWriteFontFileEnumerator*>(this);
        AddRef();
        return S_OK;
    }
    else if (riid == DXamlFontFileEnumerator::IID_IUnknown)
    {
        *ppObject = static_cast<IUnknown*>(static_cast<IDWriteFontFileEnumerator*>(this));
        AddRef();
        return S_OK;
    }
    return E_NOINTERFACE;
}
