// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#include "precomp.h"
#include <PixelFormat.h>

// Helper class for abstracting a buffer on the heap or stack, with RAII
template <int StackBufferSize>
class StackOrHeapStringBuffer
{
public:
    StackOrHeapStringBuffer()
        : m_heapBufferSize(0)
        , m_heapBuffer(nullptr)
    {
        m_stackBuffer[0] = '\0';
    }

    ~StackOrHeapStringBuffer()
    {
        delete[] m_heapBuffer;
    }

    WCHAR* GetBuffer()
    {
        return m_heapBuffer ? m_heapBuffer : m_stackBuffer;
    }

    HRESULT ReserveSpace(int charsWithNull)
    {
        HRESULT hr = S_OK;

        if (charsWithNull > GetSize())
        {
            ASSERT(m_heapBuffer == nullptr);
            m_heapBuffer = new WCHAR[charsWithNull];
            m_heapBufferSize = charsWithNull;
        }

        RRETURN(hr);//RRETURN_REMOVAL
    }

    int GetSize() const
    {
        return m_heapBuffer ? m_heapBufferSize : StackBufferSize;
    }

private:
    int m_heapBufferSize;
    WCHAR* m_heapBuffer;
    WCHAR m_stackBuffer[StackBufferSize];
};


_Check_return_ HRESULT
GenerateCacheIdentifier(
    _In_ const xstring_ptr_view& canonicalUri,
    XUINT32 decodeWidth,
    XUINT32 decodeHeight,
    PixelFormat pixelFormat,
    bool includeInvalidationId,
    XUINT32 invalidationId,
    _Inout_ xstring_ptr* cacheIdentifier)
{
    int charCount = -1;
    StackOrHeapStringBuffer<1024> buffer;

    // Include storage for the additional decode size suffix and null terminator
    IFC_RETURN(buffer.ReserveSpace(canonicalUri.GetCount() + 100));

    //NOTE: Cache identifier should use values that are not a valid in a uri to ensure a valid uri
    //      can not overlap with a cache identifier.
    if (includeInvalidationId)
    {
        charCount = swprintf_s(
            buffer.GetBuffer(), buffer.GetSize(),
            L"%s / width:[ %u ] height:[ %u ] format:[ %u ] / invalidationid:[ %u ]",
            canonicalUri.GetBuffer(),
            decodeWidth,
            decodeHeight,
            pixelFormat,
            invalidationId);
    }
    else
    {
        charCount = swprintf_s(
            buffer.GetBuffer(), buffer.GetSize(),
            L"%s / width:[ %u ] height:[ %u ] format:[ %u ]",
            canonicalUri.GetBuffer(),
            decodeWidth,
            decodeHeight,
            pixelFormat);
    }
    IFCCATASTROPHIC_RETURN(charCount >= 0);

    IFC_RETURN(xstring_ptr::CloneBuffer(buffer.GetBuffer(), charCount, cacheIdentifier));

    return S_OK;
}
