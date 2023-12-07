// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#include "precomp.h"
#include "WriteableBitmap.g.h"

using namespace DirectUI;
using namespace DirectUISynonyms;

//------------------------------------------------------------------------
//
//  Synopsis:
//      Constructs a WriteableBitmap.
//
//------------------------------------------------------------------------

WriteableBitmap::WriteableBitmap()
{
}

//------------------------------------------------------------------------
//
//  Synopsis:
//      Destructs the WriteableBitmap and releases its buffer.
//
//------------------------------------------------------------------------

WriteableBitmap::~WriteableBitmap()
{
}

//------------------------------------------------------------------------
//
//  Synopsis:
//      Returns a PixelBuffer object as an IBuffer interface. Developers
//      need to query for IBufferByteAccess in order to access a pointer
//      to the actual memory.
//
//------------------------------------------------------------------------

_Check_return_ HRESULT WriteableBitmap::get_PixelBufferImpl(_Outptr_ wsts::IBuffer** ppValue)
{
    HRESULT hr = S_OK;

    IFCPTR(ppValue);

    IFC(CheckThread());

    if (m_spPixelBuffer == NULL)
    {
        // If the buffer is null, then we're either trying to access protected content
        // or the memory hasn't been allocated yet.
        IFC(E_ACCESSDENIED);
    }

    IFC(m_spPixelBuffer.CopyTo(ppValue));

Cleanup:
    RRETURN(hr);
}

//------------------------------------------------------------------------
//
//  Synopsis:
//      Invalidates the pixels of the WriteableBitmap.
//
//------------------------------------------------------------------------

_Check_return_ HRESULT WriteableBitmap::InvalidateImpl()
{
    HRESULT hr = S_OK;

    IFC(CheckThread());

    IFC(CoreImports::WriteableBitmap_Invalidate(static_cast<CWriteableBitmap*>(GetHandle())));

Cleanup:
    RRETURN(hr);
}

//------------------------------------------------------------------------
//
//  Synopsis:
//      Handles the completed event of an asynchronous stream read.
//
//------------------------------------------------------------------------

_Check_return_ HRESULT
WriteableBitmap::OnStreamReadCompleted(_In_ wsts::IStreamReadOperation *pOperation, _In_ wf::AsyncStatus status)
{
    HRESULT hr = S_OK;
    IFC(CheckThread());

    // Call the base handler
    IFC(BitmapSource::OnStreamReadCompleted(pOperation, status));
    m_spPixelBuffer->MarkDirty();

Cleanup:
    RRETURN(hr);
}

//------------------------------------------------------------------------
//
//  Synopsis:
//      Handles the completed event of an asynchronous stream read.
//
//------------------------------------------------------------------------

_Check_return_ HRESULT
WriteableBitmap::RefreshPixelBuffer()
{
    HRESULT hr = S_OK;

    XINT32 pixelWidth = 0;
    XINT32 pixelHeight = 0;
    XBYTE* pPixels = NULL;
    ::Windows::Storage::Streams::IBufferByteAccess *pBufferInternal = NULL;

    IFC(this->get_PixelWidth(&pixelWidth));
    IFC(this->get_PixelHeight(&pixelHeight));

    // Allocate memory for the new source and get a pointer to the bytes.
    IFC(m_spPixelBuffer->InitializeBuffer(pixelHeight * pixelWidth * 4, this));
    IFC(m_spPixelBuffer.CopyTo(&pBufferInternal));
    IFC(pBufferInternal->Buffer(&pPixels));

    // Call into core to copy the source image over to the WriteableBitmap pixels.
    IFC(CoreImports::WriteableBitmap_CopyPixels(static_cast<CWriteableBitmap*>(GetHandle()), static_cast<XHANDLE>(pPixels)));

Cleanup:
    ReleaseInterface(pBufferInternal);
    RRETURN(hr);
}


//------------------------------------------------------------------------
//
//  Synopsis:
//      Initializes a new WriteableBitmap from the specified dimensions.
//
//------------------------------------------------------------------------

_Check_return_ HRESULT WriteableBitmapFactory::CreateInstanceWithDimensionsImpl(
    _In_ INT pixelWidth,
    _In_ INT pixelHeight,
    _Outptr_ xaml_imaging::IWriteableBitmap** ppInstance)
{
    HRESULT hr = S_OK;
    ctl::ComPtr<WriteableBitmap> spWriteableBitmap;
    PixelBuffer* pPixelBuffer = NULL;
    ::Windows::Storage::Streams::IBufferByteAccess* pBufferInternal = NULL;
    XBYTE* pPixels = NULL;

    IFCPTR(ppInstance);

    IFC(CheckActivationAllowed());

    ARG_EXPECT(pixelWidth > 0, "pixelWidth");
    ARG_EXPECT(pixelHeight > 0, "pixelHeight");

    // Create the WriteableBitmap.
    IFC(ctl::make(&spWriteableBitmap));

    // Create the PixelBuffer.
    IFC(ctl::ComObject<PixelBuffer>::CreateInstance(&pPixelBuffer));

    // Allocate the memory and get a pointer to the bytes.
    IFC(pPixelBuffer->InitializeBuffer(pixelHeight * pixelWidth * 4, spWriteableBitmap.Get()));
    IFC(ctl::do_query_interface(pBufferInternal, pPixelBuffer));
    IFC(pBufferInternal->Buffer(&pPixels));

    // Call into core for initialization.
    IFC(CoreImports::WriteableBitmap_Create(
        static_cast<CWriteableBitmap*>(spWriteableBitmap->GetHandle()),
        static_cast<XHANDLE>(pPixels),
        pixelWidth,
        pixelHeight));

    spWriteableBitmap->m_spPixelBuffer.Attach(pPixelBuffer);
    pPixelBuffer = NULL;

    *ppInstance = spWriteableBitmap.Detach();

Cleanup:
    ReleaseInterface(pBufferInternal);
    ctl::release_interface(pPixelBuffer);
    RRETURN(hr);
}

//------------------------------------------------------------------------
//
//  Synopsis:
//      Constructs a PixelBuffer, which is used by WriteableBitmap
//      for exposing a pointer to its pixels.
//
//------------------------------------------------------------------------

PixelBuffer::PixelBuffer() :
    m_length(0),
    m_pBuffer(NULL),
    m_pWriteableBitmap(NULL),
    m_fDirty(FALSE)
{
}

//------------------------------------------------------------------------
//
//  Synopsis:
//      Destructs the PixelBuffer and releases its memory.
//
//------------------------------------------------------------------------

PixelBuffer::~PixelBuffer()
{
    ReleaseBuffer();
}

//------------------------------------------------------------------------
//
//  Synopsis:
//      Returns pointers to the interfaces supported by PixelBuffer.
//
//------------------------------------------------------------------------

HRESULT
PixelBuffer::QueryInterfaceImpl(_In_ REFIID riid, _Out_ void** ppObject)
{
    if (InlineIsEqualGUID(riid, __uuidof(wsts::IBuffer)))
    {
        *ppObject = static_cast<wsts::IBuffer*>(this);
    }
    else if (InlineIsEqualGUID(riid, __uuidof(::Windows::Storage::Streams::IBufferByteAccess)))
    {
        *ppObject = static_cast<::Windows::Storage::Streams::IBufferByteAccess*>(this);
    }
    else
    {
        RRETURN(ctl::ComBase::QueryInterfaceImpl(riid, ppObject));
    }

    AddRefOuter();
    RRETURN(S_OK);
}

//------------------------------------------------------------------------
//
//  Synopsis:
//      Returns the capacity (in bytes) of the PixelBuffer, which is
//      always equal to its length.
//
//------------------------------------------------------------------------

IFACEMETHODIMP
PixelBuffer::get_Capacity(_Out_ UINT32 *pValue)
{
    HRESULT hr = S_OK; // WARNING_IGNORES_FAILURES

    IFCEXPECT(m_pWriteableBitmap);
    IFC(m_pWriteableBitmap->CheckThread());

    IFCPTR(pValue);
    *pValue = m_length;

Cleanup:
    RRETURN(S_OK);
}

//------------------------------------------------------------------------
//
//  Synopsis:
//      Returns the current length of the PixelBuffer, which is always
//      equal to its capacity.
//
//------------------------------------------------------------------------

IFACEMETHODIMP
PixelBuffer::get_Length(_Out_ UINT32 *pValue)
{
    HRESULT hr = S_OK; // WARNING_IGNORES_FAILURES

    IFCEXPECT(m_pWriteableBitmap);
    IFC(m_pWriteableBitmap->CheckThread());

    IFCPTR(pValue);
    *pValue = m_length;

Cleanup:
    RRETURN(S_OK);
}

//------------------------------------------------------------------------
//
//  Synopsis:
//      Sets the length of the PixelBuffer. This functionality is not
//      supported, so only a value equal to the existing length will
//      succeed.
//
//------------------------------------------------------------------------

IFACEMETHODIMP
PixelBuffer::put_Length(_In_ UINT32 value)
{
    HRESULT hr = S_OK; // WARNING_IGNORES_FAILURES

    IFCEXPECT(m_pWriteableBitmap);
    IFC(m_pWriteableBitmap->CheckThread());

    if (value != m_length || m_pWriteableBitmap == NULL)
    {
        // Length cannot be changed after initialization.
        RRETURN(E_FAIL);
    }

    IFC(m_pWriteableBitmap->Invalidate());

Cleanup:
    RRETURN(S_OK);
}

//------------------------------------------------------------------------
//
//  Synopsis:
//      Returns a pointer to the allocated bytes.
//
//------------------------------------------------------------------------

IFACEMETHODIMP
PixelBuffer::Buffer(_Outptr_ BYTE **ppBuffer)
{
    HRESULT hr = S_OK; // WARNING_IGNORES_FAILURES

    IFCPTR(ppBuffer);
    IFCEXPECT(m_pWriteableBitmap);
    IFC(m_pWriteableBitmap->CheckThread());
    if (m_fDirty)
    {
        m_fDirty = FALSE;
        IFC(m_pWriteableBitmap->RefreshPixelBuffer());
    }

    *ppBuffer = m_pBuffer;

Cleanup:
    RRETURN(S_OK);
}

//------------------------------------------------------------------------
//
//  Synopsis:
//      Allocates and zero-fills the specified number of bytes for a
//      particular WriteableBitmap.
//
//------------------------------------------------------------------------

_Check_return_ HRESULT
PixelBuffer::InitializeBuffer(
    _In_ UINT32 length,
    _In_ WriteableBitmap *pWriteableBitmap)
{
    HRESULT hr = S_OK;

    IFCEXPECT(pWriteableBitmap);
    IFC(pWriteableBitmap->CheckThread());

    ReleaseBuffer();
    m_pBuffer = new(NO_FAIL_FAST) XBYTE[length];
    IFCOOM(m_pBuffer);
    ZeroMemory(m_pBuffer, sizeof(XBYTE) * length);

    m_length = length;
    m_pWriteableBitmap = pWriteableBitmap;

Cleanup:
    RRETURN(hr);
}

//------------------------------------------------------------------------
//
//  Synopsis:
//      Releases the allocated bytes.
//
//------------------------------------------------------------------------

void
PixelBuffer::ReleaseBuffer()
{
    if (m_pBuffer != NULL)
    {
        delete [] m_pBuffer;
        m_pBuffer = NULL;
    }
    m_length = 0;
    m_pWriteableBitmap = NULL;
}

