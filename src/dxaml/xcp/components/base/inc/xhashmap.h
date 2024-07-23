// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

// The following code is derived from the ATL
//  library, for use by Winrt types only.
//
// Please use ::Windows::Foundation::Collections::Internal::HashMap instead.

#pragma once

#define WINRT_XHASHMAP_H

#include <intsafe.h>
#include <stdlib.h>
#include <string.h>
#include <new.h>
#include <unknwn.h>
#include "AssertMacros.h"

#ifndef XWINRT_ASSERT
    // NOTES:
    //  ASSERT does not evaluate its argument in fre builds,
    //  potentially leading to unevaludated variable warnings/errors.
    //  temporary is used to avoid this warning, and ensure condition
    //  is always evaluated.
    #define XWINRT_ASSERT(condition)  \
    { \
        auto c=condition; \
        ASSERT(c); \
        __pragma(warning(suppress : 4555)) \
        c; \
    }
    #define XWINRT_ASSUME(expr)  do { XWINRT_ASSERT(expr); __analysis_assume(!!(expr)); } while(0)
    #define XWINRT_ENSURE(expr)  \
        XWINRT_ENSURE_THROW(expr, E_UNEXPECTED)

    #define XWINRT_ENSURE_THROW(expr, hr)           \
        do {                                        \
            int condValzzz=!!(expr);                \
            XWINRT_ASSERT(condValzzz);              \
            if(!(condValzzz)) XRAISE(hr);           \
        } while (0)
#endif


#pragma push_macro("XRAISE")
#undef XRAISE
#pragma push_macro("XPROPOGATE")
#undef XPROPOGATE
#pragma push_macro("XRETURN")
#undef XRETURN
#pragma push_macro("XRETURN0")
#undef XRETURN0
#pragma push_macro("XCALL")
#undef XCALL

// PURPOSE
//  raise a new failure, with description message.
// TODO
//  will use Win8/WinRT error APIs
#define XRAISE( failure )\
    { XWINRT_UNWIND_WITH(failure); }

// PURPOSE
//  propogate an existing failure
#define XPROPOGATE( failure )\
    XWINRT_UNWIND_WITH(failure)

// INTERNAL
//  internal implementation detail, for XRAISE and XPROPOGATE
#define XWINRT_UNWIND_WITH( failure )\
    { HRESULT hrxxx = (failure); \
      return hrxxx; }

// PURPOSE
//  return a logical return value
// NOTES:
//  signal_return invokes
#define XRETURN( result )\
    { *retval = (result); \
      return S_OK; }

// PURPOSE
//  return from a procedure that has no out-retval result.
#define XRETURN0() \
    { return S_OK; }

// PURPOSE
//  calling another HRESULT based function
#define XCALL(expr)\
    { HRESULT hrzzz = (expr); if (FAILED(hrzzz)) { XPROPOGATE(hrzzz); } }

// PURPOSE
//   BEGIN_CALL expression END_CALL is an alternate form of
//   XCALL(expression), but which preserves line numbers.  For use
//   in multi-line call expressions
#define BEGIN_XCALL\
    { HRESULT hrzzz; hrzzz =

#define END_XCALL\
    ; if (FAILED(hrzzz)) { XPROPOGATE(hrzzz); }}

// PURPOSE
//  internal namespace, for Windows Runtime implementation details.
//  do not use directly in non-winrt code.
namespace XWinRT {

namespace Checked
{
    using ::memmove_s;
}

struct TXPOSITION
{
};
typedef TXPOSITION* XPOSITION;


struct XPlex     // warning variable length structure
{
    XPlex* pNext;
#if NEVER
    // we never support overriding default packing.  keep to align with
    //  diffs of Atl
    DWORD dwReserved[1];
#endif
    // BYTE data[maxNum*elementSize];

    void* data()
    {
        return this+1;
    }

    static XPlex* Create(
        _Inout_ XPlex*& head,
        _In_ size_t nMax,
        _In_ size_t cbElement);
            // like 'calloc' but no zero fill
            // may throw memory exceptions

    void FreeDataChain();       // free this one and links
};

inline XPlex* XPlex::Create(
    _Inout_ XPlex*& pHead,
    _In_ size_t nMax,
    _In_ size_t nElementSize)
{
    XPlex* pPlex;

    XWINRT_ASSERT( nMax > 0 );
    XWINRT_ASSERT( nElementSize > 0 );

    size_t nBytes=0;
    if(     FAILED(SizeTMult(nMax, nElementSize, &nBytes)) ||
        FAILED(SizeTAdd(nBytes, sizeof(XPlex), &nBytes)) )
    {
        return NULL;
    }
    pPlex = static_cast< XPlex* >( ::malloc( nBytes ) );
    if( pPlex == NULL )
    {
        return( NULL );
    }

    pPlex->pNext = pHead;
    pHead = pPlex;

    return( pPlex );
}

inline void XPlex::FreeDataChain()
{
    XPlex* pPlex;

    pPlex = this;
    while( pPlex != NULL )
    {
        XPlex* pNextPlex = pPlex->pNext;
        ::free( pPlex );
        pPlex = pNextPlex;
    }
}

template< typename T >
class CElementTraitsBase
{
public:
    typedef const T& INARGTYPE;
    typedef T& OUTARGTYPE;

//
//  Not used. keep for consistency with ATL:
//
//  static void CopyElements(
//      _Out_bytecap_x_(nElements * sizeof(T)) T* pDest,
//      _In_bytecount_x_(nElements * sizeof(T)) const T* pSrc,
//      _In_ size_t nElements)
//  {
//      for( size_t iElement = 0; iElement < nElements; iElement++ )
//      {
//          pDest[iElement] = pSrc[iElement];
//      }
//  }

    static void RelocateElements(
        _Out_bytecap_x_(nElements * sizeof(T)) T* pDest,
        _In_bytecount_x_(nElements * sizeof(T)) T* pSrc,
        _In_ size_t nElements)
    {
        // A simple memmove works for nearly all types.
        // You'll have to override this for types that have pointers to their
        // own members.
        Checked::memmove_s( pDest, nElements*sizeof( T ), pSrc, nElements*sizeof( T ));
    }
};

template< typename T >
class CDefaultHashTraits
{
public:
    static HRESULT Hash(_In_ void * /*pOwner*/, _In_ const T& element, UINT32 *puHash) noexcept
    {
        *puHash = ( ULONG( ULONG_PTR( element ) ) );
        return S_OK;
    }
};

template< typename T >
class CDefaultCompareTraits
{
public:
    static HRESULT CompareElements(
        _In_ void * /*pOwner*/,
        _In_ const T& element1,
        _In_ const T& element2,
        _Out_ bool * pfEqual)
    {
        pfEqual = ( (element1 == element2) != 0 );  // != 0 to handle overloads of operator== that return BOOL instead of bool
        return S_OK;
    }

#if NEVER
    static int CompareElementsOrdered(
        _In_ void * /*pOwner*/,
        _In_ const T& element1,
        _In_ const T& element2)
    {
        if( element1 < element2 )
        {
            return( -1 );
        }
        else if( element1 == element2 )
        {
            return( 0 );
        }
        else
        {
            XWINRT_ASSERT( element1 > element2 );
            return( 1 );
        }
    }
#endif
};

template< typename T >
class CDefaultElementTraits :
    public CElementTraitsBase< T >,
    public CDefaultHashTraits< T >,
    public CDefaultCompareTraits< T >
{
};

template< typename T >
class CElementTraits :
    public CDefaultElementTraits< T >
{
};

template< typename T >
class CPrimitiveElementTraits :
    public CDefaultElementTraits< T >
{
public:
    typedef T INARGTYPE;
    typedef T& OUTARGTYPE;
};

#define DECLARE_PRIMITIVE_TRAITS( T ) \
    template<> \
    class CElementTraits< T > : \
        public CPrimitiveElementTraits< T > \
    { \
    };

DECLARE_PRIMITIVE_TRAITS( unsigned char )
DECLARE_PRIMITIVE_TRAITS( unsigned short )
DECLARE_PRIMITIVE_TRAITS( unsigned int )
DECLARE_PRIMITIVE_TRAITS( unsigned long )
DECLARE_PRIMITIVE_TRAITS( unsigned __int64 )
DECLARE_PRIMITIVE_TRAITS( signed char )
DECLARE_PRIMITIVE_TRAITS( char )
DECLARE_PRIMITIVE_TRAITS( short )
DECLARE_PRIMITIVE_TRAITS( int )
DECLARE_PRIMITIVE_TRAITS( long )
DECLARE_PRIMITIVE_TRAITS( __int64 )
DECLARE_PRIMITIVE_TRAITS( float )
DECLARE_PRIMITIVE_TRAITS( double )
DECLARE_PRIMITIVE_TRAITS( bool )
#ifdef _NATIVE_WCHAR_T_DEFINED
DECLARE_PRIMITIVE_TRAITS( wchar_t )
#endif
//DECLARE_PRIMITIVE_TRAITS( void* )

#undef DECLARE_PRIMITIVE_TRAITS


template<
    typename K,
    typename V,
    class KTraits = CElementTraits< K >,
    class VTraits = CElementTraits< V > >
class XHashMap
{
public:
    typedef typename KTraits::INARGTYPE KINARGTYPE;
    typedef typename KTraits::OUTARGTYPE KOUTARGTYPE;
    typedef typename VTraits::INARGTYPE VINARGTYPE;
    typedef typename VTraits::OUTARGTYPE VOUTARGTYPE;

    class CPair :
        public TXPOSITION
    {
    protected:
        CPair(/* _In_ */ KINARGTYPE key) :
            m_key( key )
        {
        }

    public:
        CPair(const CPair&); // = delete
        void operator=(const CPair&); // = delete

        const K m_key;
        V m_value;
    };

private:
    class CNode :
        public CPair
    {
    public:
        CNode(
                /* _In_ */ KINARGTYPE key,
                _In_ UINT nHash) :
            CPair( key ),
            m_nHash( nHash )
        {
        }

    public:
        UINT GetHash() const noexcept
        {
            return( m_nHash );
        }

    public:
        CNode(const CNode&); // = delete
        void operator=(const CNode&); // = delete

        CNode* m_pNext;
        UINT m_nHash;
    };

public:
    XHashMap(
        _In_ void* pOwner,
        _In_ UINT nBins = 17,
        _In_ float fOptimalLoad = 0.75f,
        _In_ float fLoThreshold = 0.25f,
        _In_ float fHiThreshold = 2.25f,
        _In_ UINT nBlockSize = 10) noexcept;

    XHashMap(XHashMap&& src) noexcept;
    XHashMap& operator=(XHashMap&& src) noexcept;

    size_t GetCount() const noexcept;
    bool IsEmpty() const noexcept;

    HRESULT Lookup(
        /* _In_ */ KINARGTYPE key,
        _Out_ VOUTARGTYPE value,
        _Out_ bool* retval) const;
    HRESULT Lookup(/* _In_ */ KINARGTYPE key, _Out_ const CPair** retval) const noexcept;
    HRESULT Lookup(/* _In_ */ KINARGTYPE key, _Out_ CPair** retval) noexcept;
    // V& operator[](/* _In_ */ KINARGTYPE key) throw(...);

    HRESULT SetAt(
        /* _In_ */ KINARGTYPE key,
        /* _In_ */ VINARGTYPE value,
        /* _Out_ */ XPOSITION *retval);
    void SetValueAt(
        _In_ XPOSITION pos,
        /* _In_ */ VINARGTYPE value);

    HRESULT RemoveKey(/* _In_ */ KINARGTYPE key, _Out_  bool* retval) noexcept;
    HRESULT RemoveAll();
    HRESULT RemoveAtPos(_In_ XPOSITION pos) noexcept;

    XPOSITION GetStartPosition() const noexcept;
    void GetNextAssoc(
        _Inout_ XPOSITION& pos,
        _Out_ KOUTARGTYPE key,
        _Out_ VOUTARGTYPE value) const;
    const CPair* GetNext(_Inout_ XPOSITION& pos) const noexcept;
    CPair* GetNext(_Inout_ XPOSITION& pos) noexcept;
    const K& GetNextKey(_Inout_ XPOSITION& pos) const;
    const V& GetNextValue(_Inout_ XPOSITION& pos) const;
    V& GetNextValue(_Inout_ XPOSITION& pos);
    void GetAt(
        _In_ XPOSITION pos,
        _Out_ KOUTARGTYPE key,
        _Out_ VOUTARGTYPE value) const;
    CPair* GetAt(_In_ XPOSITION pos) noexcept;
    const CPair* GetAt(_In_ XPOSITION pos) const noexcept;
    const K& GetKeyAt(_In_ XPOSITION pos) const;
    const V& GetValueAt(_In_ XPOSITION pos) const;
    V& GetValueAt(_In_ XPOSITION pos);

    UINT GetHashTableSize() const noexcept;
    HRESULT InitHashTable(
        _In_ UINT nBins,
        _In_ bool bAllocNow, // default value: true
        _Out_ bool *retval);

    void EnableAutoRehash() noexcept;
    void DisableAutoRehash() noexcept;
    HRESULT Rehash(_In_ UINT nBins = 0);
    HRESULT SetOptimalLoad(
        _In_ float fOptimalLoad,
        _In_ float fLoThreshold,
        _In_ float fHiThreshold,
        _In_ bool bRehashNow = false);

// Implementation
private:
    void* m_pOwner;
    CNode** m_ppBins;
    size_t m_nElements;
    UINT m_nBins;
    float m_fOptimalLoad;
    float m_fLoThreshold;
    float m_fHiThreshold;
    size_t m_nHiRehashThreshold;
    size_t m_nLoRehashThreshold;
    ULONG m_nLockCount;
    UINT m_nBlockSize;
    XPlex* m_pBlocks;
    CNode* m_pFree;

private:
    bool IsLocked() const noexcept;
    UINT PickSize(_In_ size_t nElements) const noexcept;
    HRESULT NewNode(
        /* _In_ */ KINARGTYPE key,
        _In_ UINT iBin,
        _In_ UINT nHash,
        _Out_ CNode** retval);
    HRESULT FreeNode(_Inout_ CNode* pNode);
    void FreePlexes() noexcept;
    HRESULT GetNode(
        /* _In_ */ KINARGTYPE key,
        _Out_ UINT& iBin,
        _Out_ UINT& nHash,
        _Out_opt_ CNode*& pPrev,
        _Out_ CNode** ppNode) const noexcept;
    HRESULT CreateNode(
        /* _In_ */ KINARGTYPE key,
        _In_ UINT iBin,
        _In_ UINT nHash,
        _Out_ CNode** retval) noexcept /*was throw(...) in ATL*/;
    void RemoveNode(
        _In_ CNode* pNode,
        _In_opt_ CNode* pPrev) noexcept;
    CNode* FindNextNode(_In_ CNode* pNode) const noexcept;
    void UpdateRehashThresholds() noexcept;

public:
    ~XHashMap() noexcept;

private:
    // Private to prevent use
    XHashMap(_In_ const XHashMap&) noexcept;
    XHashMap& operator=(_In_ const XHashMap&) noexcept;
};

template< typename K, typename V, class KTraits, class VTraits >
inline size_t XHashMap< K, V, KTraits, VTraits >::GetCount() const noexcept
{
    return( m_nElements );
}

template< typename K, typename V, class KTraits, class VTraits >
inline bool XHashMap< K, V, KTraits, VTraits >::IsEmpty() const noexcept
{
    return( m_nElements == 0 );
}

//
// TODO: remove, not conducive to hresult handling.
//
//template< typename K, typename V, class KTraits, class VTraits >
//inline V& XHashMap< K, V, KTraits, VTraits >::operator[](/* _In_ */ KINARGTYPE key) throw(...)
//{
//    CNode* pNode;
//    UINT iBin;
//    UINT nHash;
//    CNode* pPrev;
//
//    pNode = GetNode( key, iBin, nHash, pPrev );
//    if( pNode == NULL )
//    {
//        pNode = CreateNode( key, iBin, nHash );
//    }
//
//    return( pNode->m_value );
//}

template< typename K, typename V, class KTraits, class VTraits >
inline UINT XHashMap< K, V, KTraits, VTraits >::GetHashTableSize() const noexcept
{
    return( m_nBins );
}

template< typename K, typename V, class KTraits, class VTraits >
inline void XHashMap< K, V, KTraits, VTraits >::GetAt(
    _In_ XPOSITION pos,
    _Out_ KOUTARGTYPE key,
    _Out_ VOUTARGTYPE value) const
{
    XWINRT_ENSURE( pos != NULL );

    CNode* pNode = static_cast< CNode* >( pos );

    key = pNode->m_key;
    value = pNode->m_value;
}

template< typename K, typename V, class KTraits, class VTraits >
inline typename XHashMap< K, V, KTraits, VTraits >::CPair* XHashMap< K, V, KTraits, VTraits >::GetAt(
    _In_ XPOSITION pos) noexcept
{
    return( static_cast< CPair* >( pos ) );
}

template< typename K, typename V, class KTraits, class VTraits >
inline const typename XHashMap< K, V, KTraits, VTraits >::CPair* XHashMap< K, V, KTraits, VTraits >::GetAt(
    _In_ XPOSITION pos) const noexcept
{
    return( static_cast< const CPair* >( pos ) );
}

template< typename K, typename V, class KTraits, class VTraits >
inline const K& XHashMap< K, V, KTraits, VTraits >::GetKeyAt(_In_ XPOSITION pos) const
{
    XWINRT_ENSURE( pos != NULL );

    CNode* pNode = (CNode*)pos;

    return( pNode->m_key );
}

template< typename K, typename V, class KTraits, class VTraits >
inline const V& XHashMap< K, V, KTraits, VTraits >::GetValueAt(_In_ XPOSITION pos) const
{
    XWINRT_ENSURE( pos != NULL );

    CNode* pNode = (CNode*)pos;

    return( pNode->m_value );
}

template< typename K, typename V, class KTraits, class VTraits >
inline V& XHashMap< K, V, KTraits, VTraits >::GetValueAt(_In_ XPOSITION pos)
{
    XWINRT_ENSURE( pos != NULL );

    CNode* pNode = (CNode*)pos;

    return( pNode->m_value );
}

template< typename K, typename V, class KTraits, class VTraits >
inline void XHashMap< K, V, KTraits, VTraits >::DisableAutoRehash() noexcept
{
    m_nLockCount++;
}

template< typename K, typename V, class KTraits, class VTraits >
inline void XHashMap< K, V, KTraits, VTraits >::EnableAutoRehash() noexcept
{
    XWINRT_ASSUME( m_nLockCount > 0 );
    m_nLockCount--;
}

template< typename K, typename V, class KTraits, class VTraits >
inline bool XHashMap< K, V, KTraits, VTraits >::IsLocked() const noexcept
{
    return( m_nLockCount != 0 );
}

template< typename K, typename V, class KTraits, class VTraits >
UINT XHashMap< K, V, KTraits, VTraits >::PickSize(_In_ size_t nElements) const noexcept
{
    // List of primes such that s_anPrimes[i] is the smallest prime greater than 2^(5+i/3)
    static const UINT s_anPrimes[] =
    {
        17, 23, 29, 37, 41, 53, 67, 83, 103, 131, 163, 211, 257, 331, 409, 521, 647, 821,
        1031, 1291, 1627, 2053, 2591, 3251, 4099, 5167, 6521, 8209, 10331,
        13007, 16411, 20663, 26017, 32771, 41299, 52021, 65537, 82571, 104033,
        131101, 165161, 208067, 262147, 330287, 416147, 524309, 660563,
        832291, 1048583, 1321139, 1664543, 2097169, 2642257, 3329023, 4194319,
        5284493, 6658049, 8388617, 10568993, 13316089, UINT_MAX
    };

    size_t nBins = (size_t)(nElements/m_fOptimalLoad);
    UINT nBinsEstimate = UINT(  UINT_MAX < nBins ? UINT_MAX : nBins );

    // Find the smallest prime greater than our estimate
    int iPrime = 0;
    while( nBinsEstimate > s_anPrimes[iPrime] )
    {
        iPrime++;
    }

    if( s_anPrimes[iPrime] == UINT_MAX )
    {
        return( nBinsEstimate );
    }
    else
    {
        return( s_anPrimes[iPrime] );
    }
}

template< typename K, typename V, class KTraits, class VTraits >
HRESULT XHashMap< K, V, KTraits, VTraits >::CreateNode(
    /* _In_ */ KINARGTYPE key,
    _In_ UINT iBin,
    _In_ UINT nHash,
    _Out_ CNode** retval) noexcept /*was throw(...) in ATL*/
{
    CNode* pNode;

    if( m_ppBins == NULL )
    {
        bool bSuccess;

        XCALL( InitHashTable( m_nBins, /*default->*/ true, &bSuccess ));
        if( !bSuccess )
        {
            XRAISE( E_OUTOFMEMORY );
        }
    }

    XCALL( NewNode( key, iBin, nHash, &pNode ) );

    XRETURN( pNode );
}

template< typename K, typename V, class KTraits, class VTraits >
XPOSITION XHashMap< K, V, KTraits, VTraits >::GetStartPosition() const noexcept
{
    if( IsEmpty() )
    {
        return( NULL );
    }

    for( UINT iBin = 0; iBin < m_nBins; iBin++ )
    {
        if( m_ppBins[iBin] != NULL )
        {
            return( XPOSITION( m_ppBins[iBin] ) );
        }
    }
    XWINRT_ASSERT( false );

    return( NULL );
}

template< typename K, typename V, class KTraits, class VTraits >
HRESULT XHashMap< K, V, KTraits, VTraits >::SetAt(
    /* _In_ */ KINARGTYPE key,
    /* _In_ */ VINARGTYPE value,
    /* _Out_ */ XPOSITION* retval)
{
    CNode* pNode;
    UINT iBin;
    UINT nHash;
    CNode* pPrev;

    XCALL( GetNode( key, iBin, nHash, pPrev, &pNode));
    if( pNode == NULL )
    {
        XCALL( CreateNode( key, iBin, nHash, &pNode) );

        // caller will handle lifetime management
        //_ATLTRY
        //{
            pNode->m_value = value;
        //}
        //_ATLCATCHALL()
        //{
        //    RemoveAtPos( XPOSITION( pNode ) );
        //    _ATLRETHROW;
        //}
    }
    else
    {
        pNode->m_value = value;
    }

    XRETURN( XPOSITION( pNode ) );
}

template< typename K, typename V, class KTraits, class VTraits >
void XHashMap< K, V, KTraits, VTraits >::SetValueAt(
    _In_ XPOSITION pos,
    /* _In_ */ VINARGTYPE value)
{
    XWINRT_ASSUME( pos != NULL );

    CNode* pNode = static_cast< CNode* >( pos );

    pNode->m_value = value;
}

template< typename K, typename V, class KTraits, class VTraits >
XHashMap< K, V, KTraits, VTraits >::XHashMap(
        _In_ void* pOwner,
        _In_ UINT nBins,
        _In_ float fOptimalLoad,
        _In_ float fLoThreshold,
        _In_ float fHiThreshold,
        _In_ UINT nBlockSize) noexcept :
    m_pOwner( pOwner ),
    m_ppBins( NULL ),
    m_nBins( nBins ),
    m_nElements( 0 ),
    m_nLockCount( 0 ),  // Start unlocked
    m_fOptimalLoad( fOptimalLoad ),
    m_fLoThreshold( fLoThreshold ),
    m_fHiThreshold( fHiThreshold ),
    m_nHiRehashThreshold( UINT_MAX ),
    m_nLoRehashThreshold( 0 ),
    m_pBlocks( NULL ),
    m_pFree( NULL ),
    m_nBlockSize( nBlockSize )
{
    XWINRT_ASSERT( nBins > 0 );
    XWINRT_ASSERT( nBlockSize > 0 );

    HRESULT hr = SetOptimalLoad( fOptimalLoad, fLoThreshold, fHiThreshold, false );

    // constructor should not trigger rehash to fail, as the collection
    //  should currently be empty.
    XWINRT_ASSERT(SUCCEEDED(hr));
}

template< typename K, typename V, class KTraits, class VTraits >
XHashMap< K, V, KTraits, VTraits>::XHashMap(
    _In_ XHashMap&& src) noexcept
{
    m_pOwner = src.m_pOwner;
    m_ppBins = src.m_ppBins;
    m_nElements = src.m_nElements;
    m_nBins = src.m_nBins;
    m_fOptimalLoad = src.m_fOptimalLoad;
    m_fLoThreshold = src.m_fLoThreshold;
    m_fHiThreshold = src.m_fHiThreshold;
    m_nHiRehashThreshold = src.m_nHiRehashThreshold;
    m_nLoRehashThreshold = src.m_nLoRehashThreshold;
    m_nLockCount = src.m_nLockCount;
    m_nBlockSize = src.m_nBlockSize;
    m_pBlocks = src.m_pBlocks;
    m_pFree = src.m_pFree;

    src.m_ppBins = NULL;
    src.m_nElements = 0;
    src.m_pFree = NULL;
    src.m_pBlocks = NULL;
}

template< typename K, typename V, class KTraits, class VTraits >
XHashMap< K, V, KTraits, VTraits> &XHashMap< K, V, KTraits, VTraits>::operator=(
    _In_ XHashMap&& src) noexcept
{
    RemoveAll();

    m_pOwner = src.m_pOwner;
    m_ppBins = src.m_ppBins;
    m_nElements = src.m_nElements;
    m_nBins = src.m_nBins;
    m_fOptimalLoad = src.m_fOptimalLoad;
    m_fLoThreshold = src.m_fLoThreshold;
    m_fHiThreshold = src.m_fHiThreshold;
    m_nHiRehashThreshold = src.m_nHiRehashThreshold;
    m_nLoRehashThreshold = src.m_nLoRehashThreshold;
    m_nLockCount = src.m_nLockCount;
    m_nBlockSize = src.m_nBlockSize;
    m_pBlocks = src.m_pBlocks;
    m_pFree = src.m_pFree;

    src.m_ppBins = NULL;
    src.m_nElements = 0;
    src.m_pFree = NULL;
    src.m_pBlocks = NULL;

    return *this;
}

template< typename K, typename V, class KTraits, class VTraits >
HRESULT XHashMap< K, V, KTraits, VTraits >::SetOptimalLoad(
    _In_ float fOptimalLoad,
    _In_ float fLoThreshold,
    _In_ float fHiThreshold,
    _In_ bool bRehashNow)
{
    XWINRT_ASSERT( fOptimalLoad > 0 );
    XWINRT_ASSERT( (fLoThreshold >= 0) && (fLoThreshold < fOptimalLoad) );
    XWINRT_ASSERT( fHiThreshold > fOptimalLoad );

    m_fOptimalLoad = fOptimalLoad;
    m_fLoThreshold = fLoThreshold;
    m_fHiThreshold = fHiThreshold;

    UpdateRehashThresholds();

    if( bRehashNow && ((m_nElements > m_nHiRehashThreshold) ||
        (m_nElements < m_nLoRehashThreshold)) )
    {
        XCALL( Rehash( PickSize( m_nElements ) ));
    }
    XRETURN0();
}

template< typename K, typename V, class KTraits, class VTraits >
void XHashMap< K, V, KTraits, VTraits >::UpdateRehashThresholds() noexcept
{
    m_nHiRehashThreshold = size_t( m_fHiThreshold*m_nBins );
    m_nLoRehashThreshold = size_t( m_fLoThreshold*m_nBins );
    if( m_nLoRehashThreshold < 17 )
    {
        m_nLoRehashThreshold = 0;
    }
}

template< typename K, typename V, class KTraits, class VTraits >
HRESULT XHashMap< K, V, KTraits, VTraits >::InitHashTable(_In_ UINT nBins, _In_ bool bAllocNow, _Out_ bool *retval)
{
    XWINRT_ASSUME( m_nElements == 0 );
    XWINRT_ASSERT( nBins > 0 );

    if( m_ppBins != NULL )
    {
        delete[] m_ppBins;
        m_ppBins = NULL;
    }

    if( bAllocNow )
    {
        m_ppBins = new (std::nothrow) CNode*[nBins];
        if( m_ppBins == NULL )
        {
            XRETURN(false);
        }

        XWINRT_ENSURE( UINT_MAX / sizeof( CNode* ) >= nBins );
        memset( m_ppBins, 0, sizeof( CNode* )*nBins );
    }
    m_nBins = nBins;

    UpdateRehashThresholds();

    XRETURN(true);
}

template< typename K, typename V, class KTraits, class VTraits >
HRESULT XHashMap< K, V, KTraits, VTraits >::RemoveAll()
{
    DisableAutoRehash();
    if( m_ppBins != NULL )
    {
        for( UINT iBin = 0; iBin < m_nBins; iBin++ )
        {
            CNode* pNext;

            pNext = m_ppBins[iBin];
            while( pNext != NULL )
            {
                CNode* pKill;

                pKill = pNext;
                pNext = pNext->m_pNext;
                XCALL(FreeNode( pKill ));
            }
        }
    }

    delete[] m_ppBins;
    m_ppBins = NULL;
    m_nElements = 0;

    if( !IsLocked() )
    {
        bool fIgnored;
        XCALL(InitHashTable( PickSize( m_nElements ), false, &fIgnored ));
    }

    FreePlexes();
    EnableAutoRehash();

    XRETURN0();
}

template< typename K, typename V, class KTraits, class VTraits >
XHashMap< K, V, KTraits, VTraits >::~XHashMap() noexcept
{
    //_ATLTRY
    //{
        HRESULT hr = RemoveAll();
        XWINRT_ASSERT(SUCCEEDED(hr));
    //}
    //_ATLCATCHALL()
    //{
    //    XWINRT_ASSERT(false);
    //}
}

template< typename K, typename V, class KTraits, class VTraits >
HRESULT XHashMap< K, V, KTraits, VTraits >::NewNode(
    /* _In_ */ KINARGTYPE key,
    _In_ UINT iBin,
    _In_ UINT nHash,
    _Out_ CNode** retval)
{
    CNode* pNewNode;

    if( m_pFree == NULL )
    {
        XPlex* pPlex;
        CNode* pNode;

        pPlex = XPlex::Create( m_pBlocks, m_nBlockSize, sizeof( CNode ) );
        if( pPlex == NULL )
        {
            XRAISE( E_OUTOFMEMORY );
        }
        pNode = (CNode*)pPlex->data();
        pNode += m_nBlockSize-1;
        for( int iBlock = m_nBlockSize-1; iBlock >= 0; iBlock-- )
        {
            pNode->m_pNext = m_pFree;
            m_pFree = pNode;
            pNode--;
        }
    }
    XWINRT_ENSURE(m_pFree != NULL );
    pNewNode = m_pFree;
    m_pFree = pNewNode->m_pNext;

    // winrt usage, key's constructor does not throw.
    //_ATLTRY
    //{
        ::new( pNewNode ) CNode( key, nHash );
    //}
    //_ATLCATCHALL()
    //{
    //    pNewNode->m_pNext = m_pFree;
    //    m_pFree = pNewNode;
    //
    //    _ATLRETHROW;
    //}
    m_nElements++;

    pNewNode->m_pNext = m_ppBins[iBin];
    m_ppBins[iBin] = pNewNode;

    if( (m_nElements > m_nHiRehashThreshold) && !IsLocked() )
    {
        XCALL( Rehash( PickSize( m_nElements ) ));
    }

    XRETURN( pNewNode );
}

template< typename K, typename V, class KTraits, class VTraits >
HRESULT XHashMap< K, V, KTraits, VTraits >::FreeNode(_Inout_ CNode* pNode)
{
    XWINRT_ENSURE( pNode != NULL );

    pNode->~CNode();
    pNode->m_pNext = m_pFree;
    m_pFree = pNode;

    XWINRT_ASSUME( m_nElements > 0 );
    m_nElements--;

    if( (m_nElements < m_nLoRehashThreshold) && !IsLocked() )
    {
        HRESULT hr = Rehash( PickSize( m_nElements ) );

        // freeing a node should be no-fail, right?
        XWINRT_ASSERT(SUCCEEDED(hr));
    }

    if( m_nElements == 0 )
    {
        FreePlexes();
    }
    XRETURN0();
}

template< typename K, typename V, class KTraits, class VTraits >
void XHashMap< K, V, KTraits, VTraits >::FreePlexes() noexcept
{
    m_pFree = NULL;
    if( m_pBlocks != NULL )
    {
        m_pBlocks->FreeDataChain();
        m_pBlocks = NULL;
    }
}

template< typename K, typename V, class KTraits, class VTraits >
HRESULT XHashMap< K, V, KTraits, VTraits >::GetNode(
    /* _In_ */ KINARGTYPE key,
    _Out_ UINT& iBin,
    _Out_ UINT& nHash,
    _Out_opt_ CNode*& pPrev,
    _Out_ CNode** retval) const noexcept
{
    CNode* pFollow;

    XCALL( KTraits::Hash( m_pOwner, key, &nHash ) );
    iBin = nHash%m_nBins;

    if( m_ppBins == NULL )
    {
        XRETURN( NULL );
    }

    pFollow = NULL;
    pPrev = NULL;
    for( CNode* pNode = m_ppBins[iBin]; pNode != NULL; pNode = pNode->m_pNext )
    {
        if( (pNode->GetHash() == nHash) )
        {
            bool fEquals;
            XCALL( KTraits::CompareElements( m_pOwner, pNode->m_key, key, &fEquals));
            if(fEquals)
            {
                pPrev = pFollow;
                XRETURN( pNode );
            }
        }
        pFollow = pNode;
    }

    XRETURN( NULL );
}

template< typename K, typename V, class KTraits, class VTraits >
HRESULT XHashMap< K, V, KTraits, VTraits >::Lookup(
    /* _In_ */ KINARGTYPE key,
    _Out_ VOUTARGTYPE value,
    _Out_ bool* retval) const
{
    UINT iBin;
    UINT nHash;
    CNode* pNode;
    CNode* pPrev;

    XCALL( GetNode( key, iBin, nHash, pPrev, &pNode));
    if( pNode == NULL )
    {
        return( false );
    }

    value = pNode->m_value;

    XRETURN( true );
}

template< typename K, typename V, class KTraits, class VTraits >
HRESULT XHashMap< K, V, KTraits, VTraits >::Lookup(
    /* _In_ */ KINARGTYPE key,
    _Out_ const CPair** retval) const noexcept
{
    UINT iBin;
    UINT nHash;
    CNode* pNode;
    CNode* pPrev;

    XCALL( GetNode( key, iBin, nHash, pPrev, &pNode));

    XRETURN( pNode );
}

template< typename K, typename V, class KTraits, class VTraits >
HRESULT XHashMap< K, V, KTraits, VTraits >::Lookup(
    /* _In_ */ KINARGTYPE key,
    _Out_ CPair** retval) noexcept
{
    UINT iBin;
    UINT nHash;
    CNode* pNode;
    CNode* pPrev;

    XCALL( GetNode( key, iBin, nHash, pPrev, &pNode));

    XRETURN( pNode );
}

template< typename K, typename V, class KTraits, class VTraits >
HRESULT XHashMap< K, V, KTraits, VTraits >::RemoveKey(/* _In_ */ KINARGTYPE key, _Out_ bool* retval) noexcept
{
    CNode* pNode;
    UINT iBin;
    UINT nHash;
    CNode* pPrev;

    pPrev = NULL;
    XCALL( GetNode( key, iBin, nHash, pPrev, &pNode ));
    if( pNode == NULL )
    {
        XRETURN( false );
    }

    RemoveNode( pNode, pPrev );

    XRETURN( true );
}

template< typename K, typename V, class KTraits, class VTraits >
void XHashMap< K, V, KTraits, VTraits >::RemoveNode(
    _In_ CNode* pNode,
    _In_opt_ CNode* pPrev) noexcept
{
    XWINRT_ASSERT( pNode != NULL );

    UINT iBin = pNode->GetHash() % m_nBins;

    if( pPrev == NULL )
    {
        XWINRT_ASSUME( m_ppBins[iBin] == pNode );
        m_ppBins[iBin] = pNode->m_pNext;
    }
    else
    {
        XWINRT_ASSERT( pPrev->m_pNext == pNode );
        pPrev->m_pNext = pNode->m_pNext;
    }
    FreeNode( pNode );
}

template< typename K, typename V, class KTraits, class VTraits >
HRESULT XHashMap< K, V, KTraits, VTraits >::RemoveAtPos(_In_ XPOSITION pos) noexcept
{
    XWINRT_ENSURE( pos != NULL );

    CNode* pNode = static_cast< CNode* >( pos );
    CNode* pPrev = NULL;
    UINT iBin = pNode->GetHash() % m_nBins;

    XWINRT_ASSUME( m_ppBins[iBin] != NULL );
    if( pNode == m_ppBins[iBin] )
    {
        pPrev = NULL;
    }
    else
    {
        pPrev = m_ppBins[iBin];
        while( pPrev->m_pNext != pNode )
        {
            pPrev = pPrev->m_pNext;
            XWINRT_ASSERT( pPrev != NULL );
        }
    }
    RemoveNode( pNode, pPrev );
    XRETURN0();
}

template< typename K, typename V, class KTraits, class VTraits >
HRESULT XHashMap< K, V, KTraits, VTraits >::Rehash(_In_ UINT nBins)
{
    CNode** ppBins = NULL;

    if( nBins == 0 )
    {
        nBins = PickSize( m_nElements );
    }

    if( nBins == m_nBins )
    {
        XRETURN0();
    }

    if( m_ppBins == NULL )
    {
        // Just set the new number of bins
        bool fIgnored;
        XCALL(InitHashTable( nBins, false, &fIgnored));
        XRETURN0();
    }

    ppBins = new (std::nothrow) CNode*[nBins];
    if (ppBins == NULL)
    {

        XRAISE( E_OUTOFMEMORY );
    }

    XWINRT_ENSURE( UINT_MAX / sizeof( CNode* ) >= nBins );
    memset( ppBins, 0, nBins*sizeof( CNode* ) );

    // Nothing gets copied.  We just rewire the old nodes
    // into the new bins.
    for( UINT iSrcBin = 0; iSrcBin < m_nBins; iSrcBin++ )
    {
        CNode* pNode;

        pNode = m_ppBins[iSrcBin];
        while( pNode != NULL )
        {
            CNode* pNext;
            UINT iDestBin;

            pNext = pNode->m_pNext;  // Save so we don't trash it
            iDestBin = pNode->GetHash()%nBins;
            pNode->m_pNext = ppBins[iDestBin];
            ppBins[iDestBin] = pNode;

            pNode = pNext;
        }
    }

    delete[] m_ppBins;
    m_ppBins = ppBins;
    m_nBins = nBins;

    UpdateRehashThresholds();

    XRETURN0();
}

template< typename K, typename V, class KTraits, class VTraits >
void XHashMap< K, V, KTraits, VTraits >::GetNextAssoc(
    _Inout_ XPOSITION& pos,
    _Out_ KOUTARGTYPE key,
    _Out_ VOUTARGTYPE value) const
{
    CNode* pNode;
    CNode* pNext;

    XWINRT_ASSUME( m_ppBins != NULL );
    XWINRT_ENSURE( pos != NULL );

    pNode = (CNode*)pos;
    pNext = FindNextNode( pNode );

    pos = XPOSITION( pNext );
    key = pNode->m_key;
    value = pNode->m_value;
}

template< typename K, typename V, class KTraits, class VTraits >
const typename XHashMap< K, V, KTraits, VTraits >::CPair* XHashMap< K, V, KTraits, VTraits >::GetNext(
    _Inout_ XPOSITION& pos) const noexcept
{
    CNode* pNode;
    CNode* pNext;

    XWINRT_ASSUME( m_ppBins != NULL );
    XWINRT_ASSERT( pos != NULL );

    pNode = (CNode*)pos;
    pNext = FindNextNode( pNode );

    pos = XPOSITION( pNext );

    return( pNode );
}

template< typename K, typename V, class KTraits, class VTraits >
typename XHashMap< K, V, KTraits, VTraits >::CPair* XHashMap< K, V, KTraits, VTraits >::GetNext(
    _Inout_ XPOSITION& pos) noexcept
{
    XWINRT_ASSUME( m_ppBins != NULL );
    XWINRT_ASSERT( pos != NULL );

    CNode* pNode = static_cast< CNode* >( pos );
    CNode* pNext = FindNextNode( pNode );

    pos = XPOSITION( pNext );

    return( pNode );
}

template< typename K, typename V, class KTraits, class VTraits >
const K& XHashMap< K, V, KTraits, VTraits >::GetNextKey(
    _Inout_ XPOSITION& pos) const
{
    CNode* pNode;
    CNode* pNext;

    XWINRT_ASSUME( m_ppBins != NULL );
    XWINRT_ENSURE( pos != NULL );

    pNode = (CNode*)pos;
    pNext = FindNextNode( pNode );

    pos = XPOSITION( pNext );

    return( pNode->m_key );
}

template< typename K, typename V, class KTraits, class VTraits >
const V& XHashMap< K, V, KTraits, VTraits >::GetNextValue(
    _Inout_ XPOSITION& pos) const
{
    CNode* pNode;
    CNode* pNext;

    XWINRT_ASSUME( m_ppBins != NULL );
    XWINRT_ENSURE( pos != NULL );

    pNode = (CNode*)pos;
    pNext = FindNextNode( pNode );

    pos = XPOSITION( pNext );

    return( pNode->m_value );
}

template< typename K, typename V, class KTraits, class VTraits >
V& XHashMap< K, V, KTraits, VTraits >::GetNextValue(
    _Inout_ XPOSITION& pos)
{
    CNode* pNode;
    CNode* pNext;

    XWINRT_ASSUME( m_ppBins != NULL );
    XWINRT_ENSURE( pos != NULL );

    pNode = (CNode*)pos;
    pNext = FindNextNode( pNode );

    pos = XPOSITION( pNext );

    return( pNode->m_value );
}

template< typename K, typename V, class KTraits, class VTraits >
typename XHashMap< K, V, KTraits, VTraits >::CNode* XHashMap< K, V, KTraits, VTraits >::FindNextNode(
    _In_ CNode* pNode) const noexcept
{
    CNode* pNext;

    if(pNode == NULL)
    {
        XWINRT_ASSERT(FALSE);
        return NULL;
    }

    if( pNode->m_pNext != NULL )
    {
        pNext = pNode->m_pNext;
    }
    else
    {
        UINT iBin;

        pNext = NULL;
        iBin = (pNode->GetHash()%m_nBins)+1;
        while( (pNext == NULL) && (iBin < m_nBins) )
        {
            if( m_ppBins[iBin] != NULL )
            {
                pNext = m_ppBins[iBin];
            }

            iBin++;
        }
    }

    return( pNext );
}

}; // namespace XWinRT

// clean up introduced macro names
#pragma pop_macro("XCALL")
#pragma pop_macro("XRETURN0")
#pragma pop_macro("XRETURN")
#pragma pop_macro("XPROPOGATE")
#pragma pop_macro("XRAISE")