// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

// Specialized smart pointers for strings, with an API structured to
// promote safe and performant allocation strategies and reference
// counting of strings.

#pragma once

#include <functional>
#include <minerror.h>
#include <minxcptypes.h>
#include <macros.h>
#include <cstdint>
#include <hstring.h>

// Note: the extra L"" in XSTRING_PTR_STORAGE is to ensure only string literals
// are used with the macro. Use XSTRING_PTR_STORAGE2 for module buffers passed
// in as references to module storage.
#define XSTRING_PTR_STORAGE(buffer) { STR_LEN_PAIR(buffer L""), FALSE, FALSE }
#define XSTRING_PTR_STORAGE2(buffer, count) { buffer, count, FALSE, FALSE }

#define DECLARE_CONST_XSTRING_PTR_STORAGE(name, buffer) static constexpr xstring_ptr_storage name = XSTRING_PTR_STORAGE(buffer L"")

#define XSTRING_PTR_FROM_STORAGE(storage) (xstring_ptr(storage))

// This macro should only be used at a function-level scope unless you're really sure you know what you're doing.  Using it in a global scope
// adds a dynamic initializer to the DLL because xstring_ptr is a class that isn't a POD type.  You probably don't want to do that when you can
// just DECLARE_CONST_XSTRING_PTR_STORAGE at the global scope and then construct the actual xstring_ptr in the scope you need it.
#define DECLARE_CONST_STRING_IN_FUNCTION_SCOPE(name, buffer) \
    static constexpr xstring_ptr_storage s##name##Storage = XSTRING_PTR_STORAGE(buffer L""); \
    const xstring_ptr name {(s##name##Storage)}

#define DECLARE_CONST_STRING_IN_TEST_CODE(name, buffer) \
    DECLARE_CONST_STRING_IN_FUNCTION_SCOPE(name, buffer)

// This macro should only be used at a function-level scope unless you're really sure you know what you're doing.  Using it in a global scope
// adds a dynamic initializer to the DLL because xstring_ptr is a class that isn't a POD type.  You probably don't want to do that when you can
// just DECLARE_CONST_XSTRING_PTR_STORAGE at the global scope and then construct the actual xstring_ptr in the scope you need it.
#define DECLARE_STATIC_CONST_STRING_IN_FUNCTION_SCOPE(name, buffer) \
    static constexpr xstring_ptr_storage s##name##Storage = XSTRING_PTR_STORAGE(buffer L""); \
    const xstring_ptr name {(s##name##Storage)}

#define DECLARE_STATIC_CONST_STRING_IN_TEST_CODE(name, buffer) \
    DECLARE_STATIC_CONST_STRING_IN_FUNCTION_SCOPE(name, buffer)

// Note: the extra L"" in XSTRING_PTR_EPHEMERAL is to ensure only string literals
// are used with the macro. Use XSTRING_PTR_EPHEMERAL2 for module buffers passed
// in as references to module storage.
#define XSTRING_PTR_EPHEMERAL(buffer) (xephemeral_string_ptr(STR_LEN_PAIR(buffer L"")))
#define XSTRING_PTR_EPHEMERAL2(buffer, count) (xephemeral_string_ptr(buffer, count))
#define XSTRING_PTR_EPHEMERAL_FROM_HSTRING(handle) (xephemeral_string_ptr(handle))
#define XSTRING_PTR_EPHEMERAL_FROM_BUILDER(builder) (xephemeral_string_ptr((builder).GetBuffer(), (builder).GetCount()))


//------------------------------------------------------------------------------
//
//  Certain string operations, like Equals, Find and [Starts|Ends]With can
//  use the xstrncmpi function to perform case-insensitive comparisons.
//
//  See xstrncmpi for a brief description of its limitations.
//
//------------------------------------------------------------------------------
enum xstrCompareBehavior
{
    xstrCompareCaseSensitive = 0,
    xstrCompareCaseInsensitive
};


#pragma pack(push, 4)
struct xencoded_string_ptr;

//------------------------------------------------------------------------------
//
//  xstring_ptr_storage structure is responsible for keeping track of a buffer
//  of characters, or to an HSTRING. The structure is intended to be stored in
//  the DLL's .rdata section (describing constant strings), or inlined as part
//  of xephemeral_string_ptr.
//
//------------------------------------------------------------------------------
struct xstring_ptr_storage
{
    friend class xstring_ptr_view;
    friend class xstring_ptr;
    friend class xephemeral_string_ptr;
    friend class xruntime_string_ptr;

public:
    static constexpr const xstring_ptr_storage& NullString();
    static constexpr const xstring_ptr_storage& EmptyString();

    static constexpr UINT32 c_MaximumStringStorageSize = (1 << 30) - 1;

    union
    {
        const WCHAR* Buffer;
        HSTRING Handle;
    };

    UINT32 Count : 30;
    UINT32 IsEphemeral : 1;
    UINT32 IsRuntimeStringHandle : 1;

private:
    xencoded_string_ptr AsEncodedStorage() const;

    const WCHAR* GetBuffer() const;
};

constexpr uintptr_t c_StorageRefersToRuntimeStringHandle = 0x00000001;

//------------------------------------------------------------------------------
//
//  xencoded_string_ptr is a pointer-sized structure that holds a pointer
//  to an xstring_ptr_storage, or a reference to an HSTRING.
//
//  The two cases are distinguished by the value of the least significant
//  bit of the union: if it's set, the anonymous union below represents
//  an HSTRING.
//
//------------------------------------------------------------------------------
struct xencoded_string_ptr
{
    friend class xstring_ptr_view;
    friend class xstring_ptr;
    friend class xephemeral_string_ptr;
    friend class xruntime_string_ptr;

public:
    union
    {
        const xstring_ptr_storage* Storage;
        HSTRING Handle;
        uintptr_t RuntimeStringHandleMarker;
    };

    xencoded_string_ptr() = default;

    explicit constexpr xencoded_string_ptr(const xstring_ptr_storage* storage)
        : Storage(storage)
    {}

    constexpr xencoded_string_ptr(const xencoded_string_ptr&) = default;
    xencoded_string_ptr& operator=(const xencoded_string_ptr&) = default;

    const WCHAR* GetBufferAndCount(
        _Out_ XUINT32* pCount
        ) const;

    static constexpr const xencoded_string_ptr& NullString();
    static constexpr const xencoded_string_ptr& EmptyString();

    bool IsNull() const;

    bool IsNullOrEmpty() const;

    xencoded_string_ptr Clone() const;

    void Reset();

private:
    constexpr bool IsRuntimeStringHandle() const
    {
        return (0 != (RuntimeStringHandleMarker & c_StorageRefersToRuntimeStringHandle));
    }

    HSTRING GetRuntimeStringHandle() const
    {
        return reinterpret_cast<HSTRING>(
              reinterpret_cast<uintptr_t>(Handle)
            & ~c_StorageRefersToRuntimeStringHandle);
    }

    xstring_ptr_storage AsStorage() const;
};
#pragma pack(pop)

// Declaring these constants as class members is currently the only way to get a global
// constexpr object with a unique address and avoid ODR violations. declspec(selectany)
// only works on objects with external linkage, but constexpr definitions must always
// appear inline. However, class members must be unique across translation units

struct xstring_ptr_constants
{
    static constexpr xstring_ptr_storage c_xstring_ptr_storage_null = { nullptr, 0, FALSE, FALSE };
    static constexpr xstring_ptr_storage c_xstring_ptr_storage_empty{ L"", 0, FALSE, FALSE };
    static constexpr xencoded_string_ptr c_xencoded_string_ptr_null{ &c_xstring_ptr_storage_null };
    static constexpr xencoded_string_ptr c_xencoded_string_ptr_empty{ &c_xstring_ptr_storage_empty };
};

constexpr const xstring_ptr_storage& xstring_ptr_storage::NullString() { return xstring_ptr_constants::c_xstring_ptr_storage_null; }
constexpr const xstring_ptr_storage& xstring_ptr_storage::EmptyString() { return xstring_ptr_constants::c_xstring_ptr_storage_empty; }

constexpr const xencoded_string_ptr& xencoded_string_ptr::NullString() { return xstring_ptr_constants::c_xencoded_string_ptr_null; }
constexpr const xencoded_string_ptr& xencoded_string_ptr::EmptyString() { return xstring_ptr_constants::c_xencoded_string_ptr_empty; }

//------------------------------------------------------------------------------
//
//  Copy constructor and assignment operator can only be enabled for strings
//  that have been constructed in a way that guarantees no-fail reference
//  counting.
//
//  Specifically, this means xstring_ptr and xruntime_string_ptr.
//
//------------------------------------------------------------------------------
#define XSTRING_PTR_FORBID_COPY_AND_ASSIGNMENT(Ty) \
    Ty(const Ty&) = delete; \
    Ty& operator=(const Ty&) = delete;

#define XSTRING_PTR_ENABLE_COPY_AND_ASSIGNMENT(Ty) \
    public: \
        Ty(const Ty& other) \
            : xstring_ptr_view(other.CloneNonEphemeralEncodedStorage()) { } \
        Ty& operator=(const Ty& other) { \
            if (this != &other) { \
                SetEncodedStorage(other.CloneNonEphemeralEncodedStorage()); \
            }\
            return (* this); \
        }

#define XSTRING_PTR_FORBID_MOVE_SEMANTICS(Ty) \
    Ty(Ty&&) = delete; \
    Ty& operator=(Ty&&) = delete;

#define XSTRING_PTR_ENABLE_MOVE_SEMANTICS(Ty) \
    public: \
        Ty(Ty&& other) noexcept \
            : xstring_ptr_view(other.ReleaseEncodedStorage()) { } \
        Ty& operator=(Ty&& other) { \
            if (this != &other) { SetEncodedStorage(other.ReleaseEncodedStorage()); } \
            return (* this); \
        }


class xstring_ptr;
class xephemeral_string_ptr;
class xruntime_string_ptr;

//------------------------------------------------------------------------------
//
//  xstring_ptr_view is the base for the xstring_ptr family of classes. It
//  provides uniform means of accessing the contents of a string, and exposes
//  a few helper methods (like Find, or Equals) that do not depend on the
//  kind of allocation backing the string.
//
//------------------------------------------------------------------------------
class xstring_ptr_view
{
    friend class xephemeral_string_ptr;

    xstring_ptr_view(const xstring_ptr_view&) = delete;
    xstring_ptr_view& operator=(const xstring_ptr_view&) = delete;

protected:
    xstring_ptr_view()
        : m_encodedStorage(xencoded_string_ptr::NullString())
    {
    }

    explicit xstring_ptr_view(
        const xencoded_string_ptr& encodedStorage
        )
        : m_encodedStorage(encodedStorage)
    {
    }

    ~xstring_ptr_view();

    void SetEncodedStorage(
        const xencoded_string_ptr& storage
        );

    xencoded_string_ptr ReleaseEncodedStorage();

    _Check_return_ HRESULT CloneEncodedStorage(
        bool forceToRuntimeStringHandle,
        _Out_ xencoded_string_ptr* pClonedEncodedStorage
        ) const;

public:
    static const UINT32 npos = static_cast<UINT32>(-1);

    bool IsNull() const;
    bool IsNullOrEmpty() const;

    inline void Reset()
    {
        m_encodedStorage.Reset();
    }

    inline const WCHAR* GetBufferAndCount(
        _Out_ XUINT32* pCount
        ) const
    {
        return m_encodedStorage.GetBufferAndCount(pCount);
    }

    _Ret_z_ const WCHAR* GetBuffer() const;

    WCHAR GetChar(XUINT32 index) const
    {
        return GetBuffer()[index];
    }

    XUINT32 GetCount() const;

    _Check_return_ HRESULT Promote(
        _Out_ xstring_ptr_storage* pstrPromoted
        ) const;

    _Check_return_ HRESULT Promote(
        _Out_ xstring_ptr* pstrPromoted
        ) const;

    _Check_return_ HRESULT Promote(
        _Out_ xruntime_string_ptr* pstrPromoted
        ) const;

    // Note: this is a dangerous method -- caller needs to ensure that the
    // underlying storage for the ephemeral string needs to be kept alive
    // as long as the demoted string is in use.
    void Demote(
        _Out_ xephemeral_string_ptr* pstrDemoted
        ) const;

    // Functionality defined in xstring_ptr_view.inl follows

    _Check_return_ HRESULT Find(
        _In_ const xstring_ptr_view& strSource,
        XUINT32 ichStartingIndex,
        xstrCompareBehavior eCompareBehavior,
        _Out_ XUINT32 *pichFound
        ) const;

    _Check_return_ HRESULT Find(
        _In_ const xstring_ptr_view& strSource,
        xstrCompareBehavior eCompareBehavior,
        _Out_ XUINT32 *pichFound
        ) const
    {
        return Find(strSource, 0, eCompareBehavior, pichFound);
    }

    // Looks for the first occurrence of the specified character in this
    // string view.
    // Follows the same pattern as std::basic_string::rfind()
    // If found, returns an offset from the start of the string.
    // If startingIndex >= size(), returns npos.
    UINT32 FindChar(WCHAR ch, UINT32 startingIndex = 0) const;

    // Looks for the last occurrence of the specified character in this
    // string view.
    // Follows the same pattern as std::basic_string::rfind()
    // If npos or any value >= size() is passed as startingIndex, the whole string will be searched
    // If found, returns an offset from the start of the string. Otherwise, returns npos.
    UINT32 FindLastChar(WCHAR ch, UINT32 startingIndex = xstring_ptr_view::npos) const;

    bool StartsWith(
        _In_ const xstring_ptr_view& strStringToFind,
        xstrCompareBehavior eCompareBehavior = xstrCompareCaseSensitive
        ) const;

    bool EndsWith(
        _In_ const xstring_ptr_view& strStringToFind,
        xstrCompareBehavior eCompareBehavior = xstrCompareCaseSensitive
        ) const;

    bool IsAllWhitespace() const;

    bool Equals(
        _In_ const xstring_ptr_view& rstrSource,
        xstrCompareBehavior eCompareBehavior = xstrCompareCaseSensitive
        ) const;

    bool Equals(
        _In_reads_(length) const WCHAR* buffer,
        XUINT32 length,
        xstrCompareBehavior eCompareBehavior = xstrCompareCaseSensitive
        ) const;

    bool Equals(
        _In_z_ const WCHAR* buffer,
        xstrCompareBehavior eCompareBehavior = xstrCompareCaseSensitive
        ) const;

    XINT32 Compare(
        _In_ const xstring_ptr_view& strSource,
        xstrCompareBehavior eCompareBehavior = xstrCompareCaseSensitive
        ) const;

    XINT32 Compare(
        _In_ const xstring_ptr_view& strSource,
        XUINT32 ichStartingIndex,
        XUINT32 cchCompare,
        xstrCompareBehavior eCompareBehavior = xstrCompareCaseSensitive
        ) const;

    void SubString(
        XUINT32 startIndex,
        XUINT32 endIndex,
        _Out_ xephemeral_string_ptr* pstrSubstring
        ) const;

    _Check_return_ HRESULT SubString(
        XUINT32 startIndex,
        XUINT32 endIndex,
        _Out_ xstring_ptr* pstrSubstring
        ) const;

    _Check_return_ HRESULT SubString(
        XUINT32 startIndex,
        XUINT32 endIndex,
        _Out_ xruntime_string_ptr* pstrSubstring
        ) const;

    char* WideCharToChar(
        _Inout_opt_ XUINT32* pcstr = nullptr,
        _In_reads_opt_(*pcstr) char* pstrDest = nullptr
        ) const;

    // Allows xstring_ptr_view to be templated by STD types (e.g., std::map).
    bool operator<(
        _In_ const xstring_ptr_view& strSource) const;

    // Allows xstring_ptr_view to be templated by STD types (e.g., std::unordered_map).
    bool operator==(
        _In_ const xstring_ptr_view& strSource) const;

    WCHAR* MakeBufferCopy() const;

    // Hash algorithm for xstring_ptr_view. Uses the Jenkins one-at-a-time hash algorithm.
    std::size_t GetHash() const
    {
        std::size_t hash = 0;
        unsigned int length;
        const WCHAR* buffer = GetBufferAndCount(&length);

        for (unsigned int i = 0; i < length; ++i)
        {
            hash += buffer[i];
            hash += (hash << 10);
            hash ^= (hash >> 6);
        }
        hash += (hash << 3);
        hash ^= (hash >> 11);
        hash += (hash << 15);

        return hash;
    }

protected:
    xencoded_string_ptr m_encodedStorage;
};


//------------------------------------------------------------------------------
//
//  xstring_ptr represents a string that is backed by either constant .rdata
//  storage, or by an HSTRING. In the second case, construction of the string
//  object ensures that the HSTRING will be ref-countable, thus enabling full
//  copy and move semantics for the class.
//
//------------------------------------------------------------------------------
class xstring_ptr : public xstring_ptr_view
{
    friend class xstring_ptr_view;
    friend class xephemeral_string_ptr;

    XSTRING_PTR_ENABLE_COPY_AND_ASSIGNMENT(xstring_ptr);
    XSTRING_PTR_ENABLE_MOVE_SEMANTICS(xstring_ptr);

private:
    explicit xstring_ptr(
        const xencoded_string_ptr& encodedStorage
        )
        : xstring_ptr_view(encodedStorage)
    {
    }

    xencoded_string_ptr CloneNonEphemeralEncodedStorage() const;

public:
    xstring_ptr()
    {
    }

    explicit xstring_ptr(
        const xstring_ptr_storage& storage
        )
        : xstring_ptr_view(xencoded_string_ptr(&storage))
    {
        ASSERT(!storage.IsEphemeral);
    }

    ~xstring_ptr()
    {
        Reset();
    }

    static xstring_ptr NullString();

    static xstring_ptr EmptyString();

    static xstring_ptr Decode(const xencoded_string_ptr& toDecode);

    static xencoded_string_ptr Encode(const xstring_ptr& toEncode);

    static xencoded_string_ptr MoveEncode(xstring_ptr&& toEncode);

    static xstring_ptr WrapModuleBuffer(
        _In_reads_(count) const WCHAR* buffer,
        XUINT32 count
        );

    static _Check_return_ HRESULT CloneBuffer(
        _In_reads_(count) const WCHAR* buffer,
        XUINT32 count,
        _Out_ xstring_ptr* pstrCloned
        );

    static _Check_return_ HRESULT CloneBuffer(
        _In_z_ const WCHAR* buffer,
        _Out_ xstring_ptr* pstrCloned
        );

    static _Check_return_ HRESULT CloneBufferTrimWhitespace(
        _In_reads_(count) const WCHAR* buffer,
        XUINT32 count,
        _Out_ xstring_ptr* pstrCloned
        );

    static _Check_return_ HRESULT CloneBufferCharToWideChar(
        _In_reads_(cstr) const char* pstr,
        XUINT32 cstr,
        _Out_ xstring_ptr* pstrCloned
        );

    // This method will add a reference to the handle
    static _Check_return_ HRESULT CloneRuntimeStringHandle(
        _In_ HSTRING handle,
        _Out_ xstring_ptr* pstrCloned
        );

#if 0
    // Note: under the xstring_ptr scheme, we cannot blindly assume
    // ownership of HSTRINGs -- they could be backed by non-reference
    // counted fast-pass strings which we cannot safely clone.
    static void AdoptRuntimeStringHandle(
        _Inout_ HSTRING* pHandle,
        _Out_ xstring_ptr* pstrAdopted
        );
#endif

    static _Check_return_ HRESULT Concatenate(
        _In_ const xstring_ptr_view& strFront,
        XUINT32 ichFrontStart,
        _In_ const xstring_ptr_view& strBack,
        XUINT32 ichBackStart,
        _Out_ xstring_ptr* pstrConcatenated
        );

    static _Check_return_ HRESULT CreateFromUInt32(
        XUINT32 value,
        _Out_ xstring_ptr* pstrValue
        );

    xstring_ptr Clone() const;

    void Swap(xstring_ptr& other)
    {
        ::std::swap(m_encodedStorage, other.m_encodedStorage);
    }
};

void swap(xstring_ptr& lhs, xstring_ptr& rhs);


//------------------------------------------------------------------------------
//
//  xephemeral_string_ptr represents a string that does not own its
//  allocation, instead pointing to an WCHAR buffer or an HSTRING that
//  has been allocated outside of its scope.
//
//  This allows for detaching strings from a stack-based XStringBuilder
//  or returning a substring of another xstring_ptr without incurring
//  the cost of a heap allocation.
//
//------------------------------------------------------------------------------
class xephemeral_string_ptr : public xstring_ptr_view
{
    friend class xstring_ptr_view;
    friend class xstring_ptr;

    XSTRING_PTR_FORBID_COPY_AND_ASSIGNMENT(xephemeral_string_ptr);
    XSTRING_PTR_FORBID_MOVE_SEMANTICS(xephemeral_string_ptr);

private:
    xephemeral_string_ptr(
        const xstring_ptr_storage& storage
        )
        : xstring_ptr_view()
        , m_ephemeralStorage(storage)
    {
        SetEncodedStorage(m_ephemeralStorage.AsEncodedStorage());
    }

public:
    xephemeral_string_ptr()
        : xstring_ptr_view()
    {
    }

    explicit xephemeral_string_ptr(
        const xstring_ptr_view& other
        );

    explicit xephemeral_string_ptr(
        _In_reads_(count) const WCHAR* buffer,
        XUINT32 count
        );

    explicit xephemeral_string_ptr(
        _In_ HSTRING handle
        );

    ~xephemeral_string_ptr();

    static void Decode(
        const xencoded_string_ptr& encoded,
        _Out_ xephemeral_string_ptr* pstrDecoded
        );

private:
    xstring_ptr_storage m_ephemeralStorage;
};


//------------------------------------------------------------------------------
//
//  xruntime_string_ptr is a string that is guaranteed to be backed
//  by an HSTRING.
//
//------------------------------------------------------------------------------
class xruntime_string_ptr : public xstring_ptr_view
{
    friend class xstring_ptr_view;

    XSTRING_PTR_FORBID_COPY_AND_ASSIGNMENT(xruntime_string_ptr);
    XSTRING_PTR_ENABLE_MOVE_SEMANTICS(xruntime_string_ptr);

private:
    xruntime_string_ptr(
        const xencoded_string_ptr& encodedStorage
        )
        : xstring_ptr_view(encodedStorage)
    {
        ASSERT(encodedStorage.IsRuntimeStringHandle());
    }

public:
    xruntime_string_ptr()
    {
    }

    ~xruntime_string_ptr()
    {
        Reset();
    }

    static _Check_return_ HRESULT DecodeAndPromote(
        _In_ const xencoded_string_ptr& toDecode,
        _Out_ xruntime_string_ptr* pstrDecoded
        );

    HSTRING GetHSTRING() const
    {
        return m_encodedStorage.GetRuntimeStringHandle();
    }

    HSTRING DetachHSTRING()
    {
        ASSERT(IsNullOrEmpty() || m_encodedStorage.IsRuntimeStringHandle());

        HSTRING result = NULL;

        if (!IsNullOrEmpty())
        {
            result = ReleaseEncodedStorage().GetRuntimeStringHandle();
        }

        return result;
    }

    void Swap(xruntime_string_ptr& other)
    {
        ::std::swap(m_encodedStorage, other.m_encodedStorage);
    }
};

void swap(xruntime_string_ptr& lhs, xruntime_string_ptr& rhs);

namespace std {
    // A hash specialization for xstring_ptr_view.
    template <> struct hash<xstring_ptr_view>
    {
        std::size_t operator()(const xstring_ptr_view& inputString) const
        {
            return inputString.GetHash();
        }
    };

    // A hash specialization for xstring_ptr.
    template <> struct hash<xstring_ptr>
    {
        std::size_t operator()(const xstring_ptr& inputString) const
        {
            return inputString.GetHash();
        }
    };
}

struct xstrCaseInsensitiveHasher
{
    std::size_t operator()(const xstring_ptr_view& input) const
    {
        std::uint32_t hash = 0;
        unsigned int length;
        const WCHAR* buffer = input.GetBufferAndCount(&length);

        // Hash algorithm used in xstring_ptr, modified to create a case insensitive hash key. Uses the Jenkins one-at-a-time hash algorithm.
        for (unsigned int i = 0; i < length; ++i)
        {
            hash += tolower(buffer[i]);
            hash += (hash << 10);
            hash ^= (hash >> 6);
        }
        hash += (hash << 3);
        hash ^= (hash >> 11);
        hash += (hash << 15);

        return static_cast<std::size_t>(hash);
    }
};

struct xstrCaseInsensitiveEqual
{
    bool operator()(const xstring_ptr_view& left, const xstring_ptr_view& right) const
    {
        return left.Equals(right, xstrCompareCaseInsensitive);
    }
};

struct xstrCaseSensitiveHasher
{
    std::size_t operator()(const xstring_ptr_view& input) const
    {
        return input.GetHash();
    }
};

struct xstrCaseSensitiveEqual
{
    bool operator()(const xstring_ptr_view& left, const xstring_ptr_view& right) const
    {
        return left.Equals(right, xstrCompareCaseSensitive);
    }
};
