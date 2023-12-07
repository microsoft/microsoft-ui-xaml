// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#include "precomp.h"

#include <TrimWhitespace.h>
#include <xstrutil.h>
#include <XStringBuilder.h>
#include <XStringUtils.h>

static_assert(sizeof(xstring_ptr_storage) == sizeof(WCHAR*) + sizeof(XUINT32), "xstring_ptr_storage is expected to be tightly packed");
static_assert(sizeof(xencoded_string_ptr) == sizeof(void*), "xencoded_string_ptr is expected to be pointer-sized");
static_assert(sizeof(xstring_ptr_view) == sizeof(xencoded_string_ptr), "xstring_ptr_view is expected to contain just the storage");
static_assert(sizeof(xstring_ptr_view) == sizeof(xstring_ptr), "xstring_ptr is expected to be identical in layout to xstring_ptr_view");
static_assert(sizeof(xstring_ptr_view) + sizeof(xstring_ptr_storage) == sizeof(xephemeral_string_ptr), "xephemeral_string_ptr is expected to be identical in layout to xstring_ptr_view and carry a local copy of the storage");
static_assert(sizeof(xstring_ptr_view) == sizeof(xruntime_string_ptr), "xruntime_string_ptr is expected to be identical in layout to xstring_ptr_view");

xencoded_string_ptr
xstring_ptr_storage::AsEncodedStorage() const
{
    xencoded_string_ptr encodedStorage;

    if (IsRuntimeStringHandle)
    {
        encodedStorage.Handle = Handle;

        // We expect the handle to be a pointer aligned to at least a 2 byte boundary.
        ASSERT(!(reinterpret_cast<uintptr_t>(Handle) & c_StorageRefersToRuntimeStringHandle));

        encodedStorage.RuntimeStringHandleMarker |= c_StorageRefersToRuntimeStringHandle;
    }
    else
    {
        // We expect the storage allocation to be aligned to at least a 2 byte boundary.
        ASSERT(!(reinterpret_cast<uintptr_t>(this) & c_StorageRefersToRuntimeStringHandle));

        encodedStorage.Storage = this;
    }

    return encodedStorage;
}

_Ret_z_ const WCHAR*
xstring_ptr_storage::GetBuffer() const
{
    const WCHAR* buffer;

    if (IsRuntimeStringHandle)
    {
        buffer = WindowsGetStringRawBuffer(
            Handle,
            nullptr /* length */);
    }
    else
    {
        buffer = Buffer;
    }

    return buffer;
}

const WCHAR*
xencoded_string_ptr::GetBufferAndCount(
    _Out_ XUINT32* pCount
    ) const
{
    const WCHAR* buffer;

    if (IsRuntimeStringHandle())
    {
        buffer = WindowsGetStringRawBuffer(
            GetRuntimeStringHandle(),
            pCount);
    }
    else
    {
        if (Storage->IsRuntimeStringHandle)
        {
            buffer = WindowsGetStringRawBuffer(
                Storage->Handle,
                pCount);
        }
        else
        {
            buffer = Storage->Buffer;
            *pCount = Storage->Count;
        }
    }

    return buffer;
}

//------------------------------------------------------------------------------
//
//  In a few select places, we will want to distinguish between an unset
//  string and an empty string.
//
//------------------------------------------------------------------------------
bool
xencoded_string_ptr::IsNull() const
{
    return IsRuntimeStringHandle()
        ? NULL == GetRuntimeStringHandle()
        : !!Storage->IsRuntimeStringHandle
          ? !!WindowsIsStringEmpty(Storage->Handle) // WinRT doesn't have a concept of null strings.
          : NULL == Storage->Buffer && 0 == Storage->Count;
}

bool
xencoded_string_ptr::IsNullOrEmpty() const
{
    XUINT32 count;
    const WCHAR* buffer = GetBufferAndCount(&count);

    return nullptr == buffer
        || 0 == count
        || L'\0' == buffer[0];
}

xencoded_string_ptr
xencoded_string_ptr::Clone() const
{
    xencoded_string_ptr cloned;

    if (IsRuntimeStringHandle())
    {
        if (FAILED(WindowsDuplicateString(
            GetRuntimeStringHandle(),
            &cloned.Handle)))
        {
            //
            // Note: the encoded string is not ephemeral. See the comment in
            // xstring_ptr::CloneNonEphemeralEncodedStorage for rationale
            // of fail-fasting here.
            //
            XAML_FAIL_FAST();
        }

        cloned.RuntimeStringHandleMarker |= c_StorageRefersToRuntimeStringHandle;
    }
    else
    {
        cloned = *this;
    }

    return cloned;
}

void
xencoded_string_ptr::Reset()
{
    if (IsRuntimeStringHandle())
    {
        // From MSDN: WindowsDeleteString always returns S_OK
        const auto hr = WindowsDeleteString(GetRuntimeStringHandle());
        ASSERTSUCCEEDED(hr);
    }

    *this = xencoded_string_ptr::NullString();
}

xstring_ptr_storage
xencoded_string_ptr::AsStorage() const
{
    xstring_ptr_storage storage;

    if (IsRuntimeStringHandle())
    {
        ASSERT(WindowsGetStringLen(GetRuntimeStringHandle()) <= xstring_ptr_storage::c_MaximumStringStorageSize);

        storage.Handle = GetRuntimeStringHandle();
        storage.Count = WindowsGetStringLen(storage.Handle);
        storage.IsEphemeral = FALSE;
        storage.IsRuntimeStringHandle = TRUE;
    }
    else
    {
        storage = *Storage;
    }

    return storage;
}


//-------------------------------------------------------------------------------
//
//  xstring_ptr can be safely cloned.
//
//-------------------------------------------------------------------------------

/* static */ xstring_ptr
xstring_ptr::NullString()
{
    return xstring_ptr(xencoded_string_ptr::NullString());
}

/* static */ xstring_ptr
xstring_ptr::EmptyString()
{
    return xstring_ptr(xencoded_string_ptr::EmptyString());
}

/* static */ xstring_ptr
xstring_ptr::Decode(
    const xencoded_string_ptr& toDecode
    )
{
    return xstring_ptr(toDecode.Clone());
}

/* static */ _Check_return_ HRESULT
xruntime_string_ptr::DecodeAndPromote(
    _In_ const xencoded_string_ptr& toDecode,
    _Out_ xruntime_string_ptr* decodedAndPromoted
    )
{
    if (toDecode.IsRuntimeStringHandle())
    {
        decodedAndPromoted->SetEncodedStorage(toDecode.Clone());
    }
    else
    {
        IFC_RETURN(xstring_ptr::Decode(toDecode).Promote(decodedAndPromoted));
    }

    return S_OK;
}

/* static */ xencoded_string_ptr
xstring_ptr::Encode(
    const xstring_ptr& toEncode
    )
{
    return toEncode.CloneNonEphemeralEncodedStorage();
}

/* static */ xencoded_string_ptr
xstring_ptr::MoveEncode(
    xstring_ptr&& toEncode
    )
{
    ASSERT(toEncode.m_encodedStorage.IsRuntimeStringHandle()
        || !toEncode.m_encodedStorage.Storage->IsEphemeral);

    xencoded_string_ptr encoded = toEncode.m_encodedStorage;

    toEncode.m_encodedStorage = xencoded_string_ptr::NullString();

    return encoded;
}

/* static */ _Check_return_ HRESULT
xstring_ptr::CloneBuffer(
    _In_reads_(count) const WCHAR* buffer,
    XUINT32 count,
    _Out_ xstring_ptr* pstrCloned
    )
{
    xstring_ptr_storage storage;

    IFCCHECK_RETURN(count <= xstring_ptr_storage::c_MaximumStringStorageSize);

    IFC_RETURN(WindowsCreateString(
        buffer,
        count,
        &storage.Handle));

    storage.Count = count;
    storage.IsEphemeral = FALSE;
    storage.IsRuntimeStringHandle = TRUE;

    pstrCloned->SetEncodedStorage(storage.AsEncodedStorage());

    return S_OK;
}

/* static */ _Check_return_ HRESULT
xstring_ptr::CloneBuffer(
    _In_z_ const WCHAR* buffer,
    _Out_ xstring_ptr* pstrCloned
    )
{
    RRETURN(CloneBuffer(buffer, xstrlen(buffer), pstrCloned));
}

/* static */ _Check_return_ HRESULT
xstring_ptr::CloneBufferTrimWhitespace(
    _In_reads_(count) const WCHAR* buffer,
    XUINT32 count,
    _Out_ xstring_ptr* pstrCloned
    )
{
    TrimWhitespace(count, buffer, &count, &buffer);
    TrimTrailingWhitespace(count, buffer, &count, &buffer);

    IFC_RETURN(CloneBuffer(buffer, count, pstrCloned));

    return S_OK;
}

/* static */ _Check_return_ HRESULT
xstring_ptr::CloneBufferCharToWideChar(
    _In_reads_(cstr) const char* pstr,
    XUINT32 cstr,
    _Out_ xstring_ptr* pstrCloned
    )
{
    HRESULT hr = S_OK;

    WCHAR* pBuffer = NULL;

    if (cstr > 0)
    {
        XUINT32 cstrTmp = cstr;

        pBuffer = CharToWideChar(&cstrTmp, pstr);

        IFC(CloneBuffer(
            pBuffer,
            cstrTmp,
            pstrCloned));
    }
    else
    {
        pstrCloned->Reset();
    }

Cleanup:
    SAFE_DELETE_ARRAY(pBuffer);

    RRETURN(hr);
}

/* static */ _Check_return_ HRESULT
xstring_ptr::CloneRuntimeStringHandle(
    _In_ HSTRING handle,
    _Out_ xstring_ptr* pstrCloned
    )
{
    xstring_ptr_storage storage;

    IFC_RETURN(WindowsDuplicateString(
        handle,
        &storage.Handle));

    {
        XUINT32 count = WindowsGetStringLen(storage.Handle);
        IFCCHECK_RETURN(count <= xstring_ptr_storage::c_MaximumStringStorageSize);
        storage.Count = count;
    }

    storage.IsEphemeral = FALSE;
    storage.IsRuntimeStringHandle = TRUE;

    pstrCloned->SetEncodedStorage(storage.AsEncodedStorage());

    return S_OK;
}

/* static */ _Check_return_ HRESULT
xstring_ptr::Concatenate(
    _In_ const xstring_ptr_view& strFront,
    XUINT32 ichFrontStart,
    _In_ const xstring_ptr_view& strBack,
    XUINT32 ichBackStart,
    _Out_ xstring_ptr* pstrConcatenated
    )
{
    XStringBuilder concatenationBuilder;

    XUINT32 frontCount;
    const WCHAR* frontBuffer = strFront.GetBufferAndCount(&frontCount);
    XUINT32 backCount;
    const WCHAR* backBuffer = strBack.GetBufferAndCount(&backCount);

    // avoid integer underflows
    if (   (ichFrontStart > frontCount)
        || (ichBackStart > backCount))
    {
        XAML_FAIL_FAST();
    }

    frontCount -= ichFrontStart;
    backCount -= ichBackStart;

    // avoid integer overflow
    if (frontCount > (XUINT32_MAX - backCount))
    {
        XAML_FAIL_FAST();
    }

    IFC_RETURN(concatenationBuilder.Initialize(
        frontCount + backCount));

    IFC_RETURN(concatenationBuilder.Append(
        frontBuffer + ichFrontStart,
        frontCount));

    IFC_RETURN(concatenationBuilder.Append(
        backBuffer + ichBackStart,
        backCount));

    IFC_RETURN(concatenationBuilder.DetachString(
        pstrConcatenated));

    return S_OK;
}

/* static */ _Check_return_ HRESULT
xstring_ptr::CreateFromUInt32(
    XUINT32 value,
    _Out_ xstring_ptr* pstrValueOut
    )
{
    HRESULT hr = S_OK;

    WCHAR* pBuffer = NULL;
    XUINT32 cBuffer = 0;

    //TODO: do better than this!
    pBuffer = xstritoa(value, &cBuffer);

    IFC(xstring_ptr::CloneBuffer(pBuffer, cBuffer, pstrValueOut));

Cleanup:
    delete [] pBuffer;

    RRETURN(hr);
}


xencoded_string_ptr
xstring_ptr::CloneNonEphemeralEncodedStorage() const
{
    xencoded_string_ptr cloned;

    ASSERT(m_encodedStorage.IsRuntimeStringHandle()
        || !m_encodedStorage.Storage->IsEphemeral);

    if (FAILED(CloneEncodedStorage(
        FALSE /* forceToRuntimeStringHandle */,
        &cloned)))
    {
        //
        // The clone_storage call should fail only if we run out of memory
        // while cloning a string backed by:
        //
        //      (1) a fast-pass Windows Runtime string handle, or
        //
        //      (2) an ephemeral buffer allocated on the stack or on the heap.
        //
        // We guarantee that an xstring_ptr cannot be backed by either type of
        // an allocation by carefully structuring the descendants of the
        // xstring_ptr_view class, with xruntime_string_ptr handling (1) and
        // xephemeral_string_ptr taking care of (2), and thus do not expect
        // this call to ever fail.
        //
        XAML_FAIL_FAST();
    }

    return cloned;
}

xstring_ptr
xstring_ptr::Clone() const
{
    return xstring_ptr(CloneNonEphemeralEncodedStorage());
}

//------------------------------------------------------------------------------
//
//  Specialization of xstring_ptr_view that is backed by
//  a stack or a heap memory allocation, or by a freshly wrapped Windows
//  Runtime string (in which case we have no way of guaranteeing it is
//  not backed by a fast-pass string that is ephemeral in nature).
//
//  Callers need to exercise caution when managing this type of strings.
//
//------------------------------------------------------------------------------

xephemeral_string_ptr::xephemeral_string_ptr(
    const xstring_ptr_view& other
    )
    : xstring_ptr_view()
{
    other.Demote(this);
}

xephemeral_string_ptr::xephemeral_string_ptr(
    _In_reads_(count) const WCHAR* buffer,
    XUINT32 count
    )
    : xstring_ptr_view()
{
    FAIL_FAST_ASSERT(count <= xstring_ptr_storage::c_MaximumStringStorageSize);

    m_ephemeralStorage.Buffer = buffer;
    m_ephemeralStorage.Count = count;
    m_ephemeralStorage.IsEphemeral = TRUE;
    m_ephemeralStorage.IsRuntimeStringHandle = FALSE;

    SetEncodedStorage(m_ephemeralStorage.AsEncodedStorage());
}

xephemeral_string_ptr::xephemeral_string_ptr(
    _In_ HSTRING handle
    )
    : xstring_ptr_view()
{
    XUINT32 count;

    // Note: we crack the handle open to save on subsequent calls
    // to WindowsGetStringRawBuffer at the potential of increasing
    // the cost of a possible promotion to xstring_ptr. This seems
    // to be a good trade-off, as we expect most of the HSTRINGs
    // passed in to be fast-pass strings, which are expensive to
    // promote anyway.
    m_ephemeralStorage.Buffer = WindowsGetStringRawBuffer(handle, &count);

    FAIL_FAST_ASSERT(count <= xstring_ptr_storage::c_MaximumStringStorageSize);

    m_ephemeralStorage.Count = count;
    m_ephemeralStorage.IsEphemeral = TRUE;
    m_ephemeralStorage.IsRuntimeStringHandle = FALSE;

    SetEncodedStorage(m_ephemeralStorage.AsEncodedStorage());
}

xephemeral_string_ptr::~xephemeral_string_ptr()
{
    m_encodedStorage = xencoded_string_ptr::NullString();
    m_ephemeralStorage = xstring_ptr_storage::NullString();
}

/* static */ void
xephemeral_string_ptr::Decode(
    const xencoded_string_ptr& encoded,
    _Out_ xephemeral_string_ptr* pstrDecoded
    )
{
    xstring_ptr_storage* pEphemeralStorage = &(pstrDecoded->m_ephemeralStorage);

    if (encoded.IsRuntimeStringHandle())
    {
        XUINT32 count;

        pEphemeralStorage->Buffer = WindowsGetStringRawBuffer(
            encoded.GetRuntimeStringHandle(),
            &count);

        ASSERT(count <= xstring_ptr_storage::c_MaximumStringStorageSize);

        pEphemeralStorage->Count = count;
    }
    else
    {
        pEphemeralStorage->Buffer = encoded.Storage->Buffer;
        pEphemeralStorage->Count = encoded.Storage->Count;
    }

    pEphemeralStorage->IsEphemeral = TRUE;
    pEphemeralStorage->IsRuntimeStringHandle = FALSE;

    pstrDecoded->SetEncodedStorage(
        pEphemeralStorage->AsEncodedStorage());
}

void swap(xstring_ptr& lhs, xstring_ptr& rhs)
{
    lhs.Swap(rhs);
}

void swap(xruntime_string_ptr& lhs, xruntime_string_ptr& rhs)
{
    lhs.Swap(rhs);
}
