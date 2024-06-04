// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#include "precomp.h"
#include "RenderTargetBitmap.g.h"
#include "UIElement.g.h"

using namespace DirectUI;
using namespace DirectUISynonyms;

ULONG RenderTargetBitmap::z_ulUniqueAsyncActionId = 1;

RenderTargetBitmap::RenderTargetBitmap()
{
}

RenderTargetBitmap::~RenderTargetBitmap()
{
}

//------------------------------------------------------------------------
//
//  Synopsis:
//      Renders the given UIElement onto the RenderTargetBitmap.
//      Supports  the WinRT async pattern
//
//------------------------------------------------------------------------
_Check_return_ HRESULT
RenderTargetBitmap::RenderAsyncImpl(
    _In_opt_ xaml::IUIElement* pElement,
    _Outptr_ wf::IAsyncAction** ppReturnValue)
{
    RRETURN(RenderToSizeAsyncImpl(pElement, 0 /* scaledWidth */, 0 /* scaledHeight */, ppReturnValue));
}

//------------------------------------------------------------------------
//
//  Synopsis:
//      Renders the given UIElement onto the RenderTargetBitmap with
//      the given scale size.
//      Supports  the WinRT async pattern
//
//------------------------------------------------------------------------
_Check_return_ HRESULT
RenderTargetBitmap::RenderToSizeAsyncImpl(
    _In_opt_ xaml::IUIElement* pElement,
    _In_ INT scaledWidth,
    _In_ INT scaledHeight,
    _Outptr_ wf::IAsyncAction** ppReturnValue)
{
    HRESULT hr = S_OK;
    ctl::ComPtr<DirectUI::IDispatcher> spDispatcher;
    Microsoft::WRL::ComPtr<RenderTargetBitmapRenderAsyncAction> spRenderAsyncAction;
    ULONG actionId = 0;
    IFC(CheckThread());
    IFC(GetXamlDispatcher(&spDispatcher));
    actionId = ::InterlockedIncrement(&RenderTargetBitmap::z_ulUniqueAsyncActionId);
    IFC(Microsoft::WRL::MakeAndInitialize<RenderTargetBitmapRenderAsyncAction>(
        &spRenderAsyncAction,
        actionId,
        spDispatcher.Get()));

    IFC(spRenderAsyncAction->StartOperation());
    IFC(static_cast<CRenderTargetBitmap*>(GetHandle())->RequestRender(
        static_cast<CUIElement*>(pElement ? static_cast<UIElement*>(pElement)->GetHandle() : NULL),
        MAX(0, scaledWidth),
        MAX(0, scaledHeight),
        spRenderAsyncAction.Get()));

    IFC(spRenderAsyncAction.CopyTo(ppReturnValue));
    spRenderAsyncAction.Detach();
Cleanup:
    if (spRenderAsyncAction != nullptr)
    {
        spRenderAsyncAction->Cancel();
        spRenderAsyncAction->CoreFireCompletion();
    }
    RRETURN(hr);
}

//------------------------------------------------------------------------
//
//  Synopsis:
//      Gets the pixels out of the RenderTargetBitmap for the previous render call.
//      Supports  the WinRT async pattern
//
//------------------------------------------------------------------------
_Check_return_ HRESULT
RenderTargetBitmap::GetPixelsAsyncImpl(
    _Outptr_ wf::IAsyncOperation<wsts::IBuffer*>** ppReturnValue)
{
    HRESULT hr = S_OK;
    ctl::ComPtr<DirectUI::IDispatcher> spDispatcher;
    Microsoft::WRL::ComPtr<RenderTargetBitmapGetPixelsAsyncOperation> spGetPixelsAsyncAction;
    ULONG actionId = 0;

    IFC(CheckThread());
    IFC(GetXamlDispatcher(&spDispatcher));
    actionId = ::InterlockedIncrement(&RenderTargetBitmap::z_ulUniqueAsyncActionId);
    IFC(Microsoft::WRL::MakeAndInitialize<RenderTargetBitmapGetPixelsAsyncOperation>(
        &spGetPixelsAsyncAction,
        actionId,
        spDispatcher.Get()));

    IFC(spGetPixelsAsyncAction->StartOperation());
    IFC(static_cast<CRenderTargetBitmap*>(GetHandle())->RequestPixels(spGetPixelsAsyncAction.Get()));

    IFC(spGetPixelsAsyncAction.CopyTo(ppReturnValue));
    spGetPixelsAsyncAction.Detach();
Cleanup:
    if (spGetPixelsAsyncAction != nullptr)
    {
        spGetPixelsAsyncAction->Cancel();
        spGetPixelsAsyncAction->CoreFireCompletion();
    }
    RRETURN(hr);
}

//------------------------------------------------------------------------
//
//  Synopsis:
//      Constructs a RenderTargetBitmapPixelBuffer, which is used by RenderTargetBitmap
//      for exposing a pointer to its pixels.
//
//------------------------------------------------------------------------
RenderTargetBitmapPixelBuffer::RenderTargetBitmapPixelBuffer() :
    m_length(0),
    m_pBuffer(NULL)
{
}

//------------------------------------------------------------------------
//
//  Synopsis:
//      Destructs the RenderTargetBitmapPixelBuffer and releases its memory.
//
//------------------------------------------------------------------------
RenderTargetBitmapPixelBuffer::~RenderTargetBitmapPixelBuffer()
{
    ReleaseBuffer();
}

//------------------------------------------------------------------------
//
//  Synopsis:
//      Returns pointers to the interfaces supported by RenderTargetBitmapPixelBuffer.
//
//------------------------------------------------------------------------
HRESULT
RenderTargetBitmapPixelBuffer::QueryInterfaceImpl(_In_ REFIID riid, _Out_ void** ppObject)
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
//      Returns the capacity (in bytes) of the RenderTargetBitmapPixelBuffer, which is
//      always equal to its length.
//
//------------------------------------------------------------------------
IFACEMETHODIMP
RenderTargetBitmapPixelBuffer::get_Capacity(_Out_ UINT32 *pValue)
{
    HRESULT hr = S_OK; // WARNING_IGNORES_FAILURES

    IFCPTR(pValue);
    *pValue = m_length;

Cleanup:
    RRETURN(S_OK);
}

//------------------------------------------------------------------------
//
//  Synopsis:
//      Returns the current length of the RenderTargetBitmapPixelBuffer, which is always
//      equal to its capacity.
//
//------------------------------------------------------------------------
IFACEMETHODIMP
RenderTargetBitmapPixelBuffer::get_Length(_Out_ UINT32 *pValue)
{
    HRESULT hr = S_OK; // WARNING_IGNORES_FAILURES

    IFCPTR(pValue);
    *pValue = m_length;

Cleanup:
    RRETURN(S_OK);
}

//------------------------------------------------------------------------
//
//  Synopsis:
//      Sets the length of the RenderTargetBitmapPixelBuffer. This functionality is not
//      supported, so only a value equal to the existing length will
//      succeed.
//
//------------------------------------------------------------------------
IFACEMETHODIMP
RenderTargetBitmapPixelBuffer::put_Length(_In_ UINT32 value)
{
    HRESULT hr = S_OK;

    if (value != m_length)
    {
        // Length cannot be changed after initialization.
        IFC(E_FAIL);
    }

Cleanup:
    RRETURN(hr);
}

//------------------------------------------------------------------------
//
//  Synopsis:
//      Returns a pointer to the allocated bytes.
//
//------------------------------------------------------------------------
IFACEMETHODIMP
RenderTargetBitmapPixelBuffer::Buffer(_Outptr_ BYTE **ppBuffer)
{
    HRESULT hr = S_OK; // WARNING_IGNORES_FAILURES

    IFCPTR(ppBuffer);
    *ppBuffer = m_pBuffer;

Cleanup:
    RRETURN(S_OK);
}

//------------------------------------------------------------------------
//
//  Synopsis:
//      Sets the buffer to passed in bytes.
//
//------------------------------------------------------------------------
_Check_return_ HRESULT
RenderTargetBitmapPixelBuffer::InitializeBuffer(
    _In_ UINT32 length,
    _In_reads_(length) XBYTE *pBytes)
{
    HRESULT hr = S_OK;

    IFCEXPECT(pBytes);

    if (m_pBuffer != pBytes)
    {
        ReleaseBuffer();
        m_length = length;
        m_pBuffer = pBytes;
    }

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
RenderTargetBitmapPixelBuffer::ReleaseBuffer()
{
    if (m_pBuffer != NULL)
    {
        delete [] m_pBuffer;
        m_pBuffer = NULL;
    }
    m_length = 0;
}

//------------------------------------------------------------------------
//
//  Synopsis:
//      Returns the pixel buffer as the result.
//
//------------------------------------------------------------------------
STDMETHODIMP RenderTargetBitmapGetPixelsAsyncOperation::GetResults(
    _Outptr_result_maybenull_ wsts::IBuffer **results)
{
    HRESULT hr = S_OK;
    *results = nullptr;
    IFC(__super::CheckValidStateForResultsCall());
    if (m_spPixelBuffer != NULL)
    {
        IFC(m_spPixelBuffer.CopyTo(results));
    }
Cleanup:
    RRETURN(hr);
}

//------------------------------------------------------------------------
//
//  Synopsis:
//      Creates RenderTargetBitmapPixelBuffer out of the passed in byte array and
//      saves it as the result.
//
//------------------------------------------------------------------------
_Check_return_ HRESULT
RenderTargetBitmapGetPixelsAsyncOperation::CoreSetResults(RenderTargetBitmapPixelData results)
{
    HRESULT hr = S_OK;
    RenderTargetBitmapPixelBuffer* pPixelBuffer = NULL;

    if (m_spPixelBuffer == NULL)
    {
        // Create the PixelBuffer.
        IFC(ctl::ComObject<RenderTargetBitmapPixelBuffer>::CreateInstance(&pPixelBuffer));

        m_spPixelBuffer.Attach(pPixelBuffer);
        pPixelBuffer = NULL;
    }

    // Initialize buffer with the bytes.
    IFC(m_spPixelBuffer->InitializeBuffer(results.m_length, results.m_pBytes));

Cleanup:
    ctl::release_interface(pPixelBuffer);
    RRETURN(hr);
}

