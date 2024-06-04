// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#include "precomp.h"

#include "XStringUtils.h"
#include "xstrutil.h"
#include <algorithm>

//-------------------------------------------------------------------------------
//
//  xstring_ptr_view provides:
//
//  1) an interface between storage and the three string types, and
//
//  2) typical string operations, and
//
//  3) ability for all three string types to be promoted to either an
//     xstring_ptr, or a strongly referenced Windows Runtime string.
//
//-------------------------------------------------------------------------------

xstring_ptr_view::~xstring_ptr_view()
{
    ASSERT(m_encodedStorage.Storage == &xstring_ptr_storage::NullString());
#if DBG
    // In CHK bits, null out the internal storage to force an access violation when accessing after destruction
    m_encodedStorage.Storage = nullptr;
#endif
}

void
xstring_ptr_view::SetEncodedStorage(
    const xencoded_string_ptr& encodedStorage)
{
    m_encodedStorage.Reset();

    m_encodedStorage = encodedStorage;
}

xencoded_string_ptr
xstring_ptr_view::ReleaseEncodedStorage()
{
    xencoded_string_ptr releasedEncodedStorage = m_encodedStorage;

    m_encodedStorage = xencoded_string_ptr::NullString();

    return releasedEncodedStorage;
}

_Check_return_ HRESULT
xstring_ptr_view::CloneEncodedStorage(
    bool forceToRuntimeStringHandle,
    _Out_ xencoded_string_ptr* pClonedEncodedStorage) const
{
    bool clonedAsRuntimeStringHandle = false;

    if (m_encodedStorage.IsRuntimeStringHandle())
    {
        IFC_RETURN(WindowsDuplicateString(
            m_encodedStorage.GetRuntimeStringHandle(),
            &(pClonedEncodedStorage->Handle)));

        clonedAsRuntimeStringHandle = TRUE;
    }
    else if (nullptr != m_encodedStorage.Storage
        && (m_encodedStorage.Storage->IsEphemeral
        || forceToRuntimeStringHandle))
    {
        XUINT32 nCount;
        const WCHAR* buffer = m_encodedStorage.GetBufferAndCount(&nCount);

        IFC_RETURN(WindowsCreateString(
            buffer,
            nCount,
            &(pClonedEncodedStorage->Handle)));

        clonedAsRuntimeStringHandle = TRUE;
    }

    if (clonedAsRuntimeStringHandle)
    {
        pClonedEncodedStorage->RuntimeStringHandleMarker |= c_StorageRefersToRuntimeStringHandle;
    }
    else
    {
        *pClonedEncodedStorage = m_encodedStorage;
    }

    return S_OK;
}

_Ret_z_
const WCHAR*
xstring_ptr_view::GetBuffer() const
{
    const WCHAR* buffer;

    if (m_encodedStorage.IsRuntimeStringHandle())
    {
        buffer = WindowsGetStringRawBuffer(
            m_encodedStorage.GetRuntimeStringHandle(),
            nullptr /* length */);
    }
    else
    {
        if (m_encodedStorage.Storage->IsRuntimeStringHandle)
        {
            buffer = WindowsGetStringRawBuffer(
                m_encodedStorage.Storage->Handle,
                nullptr /* length */);
        }
        else
        {
            buffer = m_encodedStorage.Storage->Buffer;
        }
    }

    return buffer;
}

XUINT32
xstring_ptr_view::GetCount() const
{
    XUINT32 count;

    if (m_encodedStorage.IsRuntimeStringHandle())
    {
        count = WindowsGetStringLen(m_encodedStorage.GetRuntimeStringHandle());
    }
    else
    {
        if (m_encodedStorage.Storage->IsRuntimeStringHandle)
        {
            count = WindowsGetStringLen(m_encodedStorage.Storage->Handle);
        }
        else
        {
            count = m_encodedStorage.Storage->Count;
        }
    }

    return count;
}


//-------------------------------------------------------------------------------
//
//  xstring_ptr_view provides typical string operations and
//  allows all three string types to be promoted to a strongly referenced
//  Windows Runtime string.
//
//-------------------------------------------------------------------------------

bool
xstring_ptr_view::IsNull() const
{
    return m_encodedStorage.IsNull();
}

bool
xstring_ptr_view::IsNullOrEmpty() const
{
    return m_encodedStorage.IsNullOrEmpty();
}

_Check_return_ HRESULT
xstring_ptr_view::Promote(
    _Out_ xstring_ptr_storage* pstrPromoted) const
{
    xencoded_string_ptr cloned;

    IFC_RETURN(CloneEncodedStorage(
        FALSE /* forceToRuntimeStringHandle */,
        &cloned));

    *pstrPromoted = cloned.AsStorage();

    return S_OK;
}

_Check_return_ HRESULT
xstring_ptr_view::Promote(
    _Out_ xstring_ptr* pstrPromoted) const
{
    xencoded_string_ptr cloned;

    IFC_RETURN(CloneEncodedStorage(
        FALSE /* forceToRuntimeStringHandle */,
        &cloned));

    pstrPromoted->SetEncodedStorage(cloned);

    return S_OK;
}

_Check_return_ HRESULT
xstring_ptr_view::Promote(
    _Out_ xruntime_string_ptr* pstrPromoted) const
{
    xencoded_string_ptr cloned;

    IFC_RETURN(CloneEncodedStorage(
        TRUE /* forceToRuntimeStringHandle */,
        &cloned));

    pstrPromoted->SetEncodedStorage(cloned);

    return S_OK;
}

void
xstring_ptr_view::Demote(
    _Out_ xephemeral_string_ptr* pstrDemoted) const
{
    xephemeral_string_ptr::Decode(m_encodedStorage, pstrDemoted);
}

//------------------------------------------------------------------------------
//
//  Finds the index of the first occurrence of pstrSource within this
//  xstring_ptr_view.
//
//  pstrSource          -- the string that we are trying to find.
//  ichStartingIndex    -- the index within the current string to start
//                         looking for pstrSource.
//  eCompareBehavior    -- whether the comparison should be case-sensitive.
//  pichFound           -- the index within the current string that pstrSource
//                         was found.
//
//------------------------------------------------------------------------------
_Check_return_ HRESULT
xstring_ptr_view::Find(
    _In_ const xstring_ptr_view& other,
    XUINT32 ichStartingIndex,
    xstrCompareBehavior eCompareBehavior,
    _Out_ XUINT32 *pichFound) const
{
    HRESULT hr = E_FAIL;

    XUINT32 thisCount;
    const WCHAR* thisBuffer = GetBufferAndCount(&thisCount);
    XUINT32 otherCount;
    const WCHAR* otherBuffer = other.GetBufferAndCount(&otherCount);

    // avoid integer overflow
    if (ichStartingIndex >= (XUINT32_MAX - otherCount))
    {
        return hr;
    }

    //  only do the search if we have enough space within the current string
    //  that it would be possible to find pstrSource.  Even if every character that could
    //  match did match we still wouldn't be able to find the string, so why bother.
    if (ichStartingIndex + otherCount <= thisCount)
    {
        const WCHAR* pchBuffer = thisBuffer + ichStartingIndex;
        WCHAR* pchFound;
        if (xstrCompareCaseSensitive == eCompareBehavior)
        {
            pchFound = xstrnstr(pchBuffer, otherBuffer, otherCount);
        }
        else
        {
            pchFound = xstrnistr(pchBuffer, otherBuffer, otherCount);
        }

        if (pchFound)
        {
            *pichFound = static_cast<XUINT32>(pchFound - thisBuffer);
            hr = S_OK;
        }
    }

    return hr;
}


// Looks for the first occurrence of the specified character in this
// string view.
// Follows the same pattern as std::basic_string::rfind()
// If npos or any value >= size() is passed as startingIndex, the whole string will be searched
// If found, returns an offset from the start of the string. Otherwise, returns npos.
UINT32
xstring_ptr_view::FindChar(
    WCHAR ch,
    UINT32 startingIndex /* = 0 */) const
{
    UINT32 thisCount;
    const auto thisBuffer = GetBufferAndCount(&thisCount);

    if (startingIndex < thisCount)
    {
        const auto end = thisBuffer + thisCount;
        auto iter = std::find(thisBuffer + startingIndex, end, ch);
        if (iter != end)
        {
            return static_cast<UINT32>(iter - thisBuffer);
        }
    }

    // No match found
    return xstring_ptr_view::npos;
}


// Looks for the last occurrence of the specified character in this
// string view.
// Follows the same pattern as std::basic_string::rfind()
// If npos or any value >= size() is passed as startingIndex, the whole string will be searched
// If found, returns an offset from the start of the string. Otherwise, returns npos.
UINT32
xstring_ptr_view::FindLastChar(
    WCHAR ch,
    UINT32 startingIndex /* = npos */) const
{
    UINT32 thisCount;
    const auto thisBuffer = GetBufferAndCount(&thisCount);

    if (thisCount > 0)
    {
        // Start at the last searchable character
        auto ptr = thisBuffer + std::min(thisCount - 1, startingIndex);
        for (;; --ptr)
        {
            if (*ptr == ch) // found a match
                return static_cast<UINT32>(ptr - thisBuffer);
            else if (ptr == thisBuffer) // at beginning
                break;
        }
    }

    // No match
    return xstring_ptr_view::npos;
}


//------------------------------------------------------------------------------
//
//  Determines whether this instance starts with pstrStringToFind.
//
//  pstrStringToFind    -- the string that we are trying to find.
//  eCompareBehavior    -- whether the search is case sensitive.
//
//------------------------------------------------------------------------------
bool
xstring_ptr_view::StartsWith(
    _In_ const xstring_ptr_view& strStringToFind,
    xstrCompareBehavior eCompareBehavior) const
{
    const XUINT32 thisCount = GetCount();
    const XUINT32 stringToFindCount = strStringToFind.GetCount();

    if (thisCount >= stringToFindCount)
    {
        return (this->Compare(strStringToFind, 0, stringToFindCount, eCompareBehavior) == 0);
    }

    return false;
}


//------------------------------------------------------------------------------
//
//  Determines whether this instance ends with pstrStringToFind.
//
//  strStringToFind     -- the string that we are trying to find.
//  eCompareBehavior    -- whether the search is case sensitive.
//
//------------------------------------------------------------------------------
bool
xstring_ptr_view::EndsWith(
    _In_ const xstring_ptr_view& strStringToFind,
    xstrCompareBehavior eCompareBehavior) const
{
    const XUINT32 thisCount = GetCount();
    const XUINT32 stringToFindCount = strStringToFind.GetCount();

    if (thisCount >= stringToFindCount)
    {
        return (this->Compare(
            strStringToFind,
            thisCount - stringToFindCount,
            stringToFindCount,
            eCompareBehavior) == 0);
    }

    return false;
}


//------------------------------------------------------------------------------
//
//  Returns true if the string view is empty, or if consists of only
//  characters that isspace considers to be whitespace.
//
//------------------------------------------------------------------------------
bool
xstring_ptr_view::IsAllWhitespace() const
{
    XUINT32 thisCount;
    const WCHAR* thisBuffer = GetBufferAndCount(&thisCount);

    if (nullptr != thisBuffer)
    {
        for (XUINT32 i = 0; i < thisCount; i++)
        {
            if (!isspace(thisBuffer[i]))
            {
                return false;
            }
        }
    }

    return true;
}


//------------------------------------------------------------------------------
//
//  Boolean string-to-string comparison.
//
//------------------------------------------------------------------------------
bool
xstring_ptr_view::Equals(
    _In_ const xstring_ptr_view& other,
    xstrCompareBehavior eCompareBehavior) const
{
    if (GetCount() != other.GetCount())
    {
        return false;
    }
    else
    {
        return Compare(other, eCompareBehavior) == 0;
    }
}


//------------------------------------------------------------------------------
//
//  Boolean string-to-string comparison, with the "other" string specified
//  by a character buffer and its size.
//
//------------------------------------------------------------------------------
bool
xstring_ptr_view::Equals(
    _In_reads_(length) const WCHAR* buffer,
    XUINT32 length,
    xstrCompareBehavior eCompareBehavior) const
{
    if(GetCount() != length)
    {
        return false;
    }
    else
    {
        xephemeral_string_ptr other = XSTRING_PTR_EPHEMERAL2(buffer, length);
        return Compare(other, eCompareBehavior) == 0;
    }
}


//------------------------------------------------------------------------------
//
//  Boolean string-to-string comparison, with the "other" string specified
//  by a null-terminated character buffer.
//
//------------------------------------------------------------------------------
bool
xstring_ptr_view::Equals(
    _In_z_ const WCHAR* buffer,
    xstrCompareBehavior eCompareBehavior) const
{
    xephemeral_string_ptr wrappedBuffer = XSTRING_PTR_EPHEMERAL2(buffer, xstrlen(buffer));

    return Equals(wrappedBuffer, eCompareBehavior);
}



//------------------------------------------------------------------------------
//
//  strcmp-like string-to-string comparison
//
//------------------------------------------------------------------------------
XINT32
xstring_ptr_view::Compare(
    _In_ const xstring_ptr_view& other,
    xstrCompareBehavior eCompareBehavior) const
{
    XINT32 result = 0;

    //
    // If this and the other strings point to the same storage (or HSTRING),
    // there is no need to run any actual comparisons, as we know the strings
    // will be identical.
    //
    if (m_encodedStorage.Storage != other.m_encodedStorage.Storage)
    {
        XUINT32 thisCount;
        const WCHAR* thisBuffer = GetBufferAndCount(&thisCount);
        XUINT32 otherCount;
        const WCHAR* otherBuffer = other.GetBufferAndCount(&otherCount);

        if (!thisBuffer || !otherBuffer)
        {
            return (thisCount - otherCount);
        }

        const XUINT32 minCount = std::min(thisCount, otherCount);

        if (xstrCompareCaseSensitive == eCompareBehavior)
        {
            result = std::wcsncmp(otherBuffer, thisBuffer, minCount);
        }
        else
        {
            result = _wcsnicmp(otherBuffer, thisBuffer, minCount);
        }

        if (result == 0)
        {
            return (thisCount - otherCount);
        }
    }

    return result;
}


//------------------------------------------------------------------------------
//
//  strcmp-like substring comparison.
//
//------------------------------------------------------------------------------
XINT32
xstring_ptr_view::Compare(
    _In_ const xstring_ptr_view& other,
    XUINT32 ichStartingIndex,
    XUINT32 cchCompare,
    xstrCompareBehavior eCompareBehavior) const
{
    XINT32 result = 0;

    //
    // If this and the other strings point to the same storage (or HSTRING),
    // there is no need to run any actual comparisons, as we know the strings
    // will be identical.
    //
    if (m_encodedStorage.Storage != other.m_encodedStorage.Storage && cchCompare > 0)
    {
        XUINT32 thisCount;
        const WCHAR* thisBuffer = GetBufferAndCount(&thisCount);
        XUINT32 otherCount;
        const WCHAR* otherBuffer = other.GetBufferAndCount(&otherCount);

        const XUINT32 minCount = std::min(std::min(thisCount - ichStartingIndex, otherCount), cchCompare);

        if (ichStartingIndex < thisCount && thisBuffer && otherBuffer)
        {
            if (xstrCompareCaseSensitive == eCompareBehavior)
            {
                result = std::wcsncmp(otherBuffer, thisBuffer + ichStartingIndex, minCount);
            }
            else
            {
                result = _wcsnicmp(otherBuffer, thisBuffer + ichStartingIndex, minCount);
            }
        }

        if (result == 0)
        {
            return (std::min(ichStartingIndex > thisCount ? 0 : thisCount - ichStartingIndex, cchCompare) 
                    - std::min(otherCount, cchCompare));
        }
    }

    return result;
}


//------------------------------------------------------------------------------
//
//  Return a new xephemeral_string_ptr created from the given string's
//  characters from startIndex to endIndex.
//
//------------------------------------------------------------------------------
void
xstring_ptr_view::SubString(
    XUINT32 startIndex,
    XUINT32 endIndex,
    _Out_ xephemeral_string_ptr* pstrSubstring) const
{
    XUINT32 thisCount;
    const WCHAR* thisBuffer = GetBufferAndCount(&thisCount);

    // validity checking
    if ((startIndex > thisCount)
        || (endIndex > thisCount)
        || (startIndex > endIndex))
    {
        pstrSubstring->Reset();
    }
    else
    {
        XSTRING_PTR_EPHEMERAL2(
            thisBuffer + startIndex,
            endIndex - startIndex).Demote(pstrSubstring);
    }
}


//------------------------------------------------------------------------------
//
//  Return a new xstring_ptr created from the given string's characters from
//  startIndex to endIndex. Note that this override can result in a memory
//  allocation and as such can fail.
//
//------------------------------------------------------------------------------
_Check_return_ HRESULT
    xstring_ptr_view::SubString(
    XUINT32 startIndex,
    XUINT32 endIndex,
    _Out_ xstring_ptr* pstrSubstring) const
{
    xephemeral_string_ptr localSubstring;

    SubString(startIndex, endIndex, &localSubstring);

    RRETURN(localSubstring.Promote(pstrSubstring));
}

//------------------------------------------------------------------------------
//
//  Return a new xruntime_string_ptr created from the given string's characters from
//  startIndex to endIndex. Note that this override can result in a memory
//  allocation and as such can fail.
//
//------------------------------------------------------------------------------
_Check_return_ HRESULT
    xstring_ptr_view::SubString(
    XUINT32 startIndex,
    XUINT32 endIndex,
    _Out_ xruntime_string_ptr* pstrSubstring) const
{
    xephemeral_string_ptr localSubstring;

    SubString(startIndex, endIndex, &localSubstring);

    RRETURN(localSubstring.Promote(pstrSubstring));
}


//------------------------------------------------------------------------------
//
//  Create and return an 8-bit character buffer with the string's contents.
//
//------------------------------------------------------------------------------
char*
xstring_ptr_view::WideCharToChar(
    _Inout_opt_ XUINT32* pcstr,
    _In_reads_opt_(*pcstr) char* pstrDest) const
{
    XUINT32 count;
    const WCHAR* buffer = GetBufferAndCount(&count);

    return ::WideCharToChar(count, buffer, pcstr, pstrDest);
}

// Overload "<" operator to allow use by STD types.
bool
xstring_ptr_view::operator<(
    _In_ const xstring_ptr_view& strSource) const
{
    return Compare(strSource) < 0;
}

// Overload "==" operator to allow use by STD types.
bool
xstring_ptr_view::operator==(
    _In_ const xstring_ptr_view& strSource) const
{
    return (Equals(strSource) == TRUE) ? true : false;
}


//------------------------------------------------------------------------------
//
//  Create a fresh copy of this string's contents as a null-terminated
//  character buffer.
//
//------------------------------------------------------------------------------
WCHAR*
xstring_ptr_view::MakeBufferCopy() const
{
    XUINT32 count;
    const WCHAR* buffer = GetBufferAndCount(&count);

    WCHAR* bufferCopy = nullptr;

    if (0 < count)
    {
        bufferCopy = new WCHAR[count + 1 /* for the null terminator */];
    }

    if (nullptr != bufferCopy)
    {
        memcpy(bufferCopy, buffer, count * sizeof(WCHAR));
        bufferCopy[count] = L'\0';
    }

    return bufferCopy;
}
