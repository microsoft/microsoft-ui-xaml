// **************************************************************
// Copyright (c) Microsoft Corporation. All rights reserved.
//
// windowscollections.h
//
// Reusable COM/WinRT collections implementations
//
// Namespace: ::Windows::Foundation::Collections::Internal
//
// Traits
// ======
//
// Lifetime, equality, and hash traits have default values
// for integer, floating point, string, delegate, and interface
// types.  They must be explicitly provided for structs.
//
// The defaults are provided via the DefaultLifetimeTraits<T>,
// DefaultEqualityPredicate<T>, and DefaultHash<T> traits classes.
//
// DefaultHash<T> uses Fnv1a for hashing various data types.
// DefaultHash<T> fulfills the normal equality invariants with
// respect to DefaultEquality<T>.
//
// Equality is defined using operator== for numbers, ordinal string
// comparison for strings, and using COM IUnknown identity for
// other objects.
//
//
// Collections
// ===========
//
// ### class Vector
//   A Dynamically resizable array.
//
//   Template arguments:
//     T                 - element type
//     EqualityPredicate - (opt*) used for IndexOf equality.
//     LifetimeTraits    - (opt*) used for managing resource lifetime of elements.
//     Options           - (opt)  other collections options, see VectorOptions
//   Methods:
//     static HRESULT Make(Vector**);
//     static HRESULT Make(EqPred, Vector**);
//       Creates an instance of Vector.  Eg.
//       |   ComPtr<Vector<int>> spV;
//       |   hr = Vector<int>::Make(&spV);
//     <implements IVector<T> and IIterable<T>>
//
// ### class HashMap
//   A hash table.
//
//   Template arguments:
//     K                 - key element type
//     V                 - value element type
//     EqualityPredicate - (opt*) used for IndexOf equality.
//     KeyLifetime       - (opt*) used for managing resource lifetime of elements.
//     ValueLifetime     - (opt*) used for managing resource lifetime of elements.
//     Options           - (opt)  other collections options, see HashMapOptions
//   Methods:
//     static HRESULT Make(HashMap**);
//     static HRESULT Make(HashFn, EqPred, HashMap**);
//       Creates an instance of HashMap.  Eg.
//       |   ComPtr<HashMap<int,int>> spM;
//       |   hr = HashMap<int,int>::Make(&spM);
//     <implements IMap<K,V> and IIterable<IKeyValuePair<K,V>>>
//
//
// ### VectorOptions and HashMapOptions
//   These classes are used to configure support for notifications in the collection. The
//   VectorOptions class takes the element type and a boolean value that indicates support
//   of the IObservableVector interface. When the Vector class is instantiated with this
//   support, the resulting class implements the IObservableVector interface.
//
//   VectorOptions class
//   Template arguments:
//     T                     - element type
//     supportsObservable    - boolean to indicate support for notifications to clients
//
//   HashMapOptions takes the Key and Value element types, the KeyLifetimeTraits type and
//   a boolean indicating support for the IHashMap interface.
//
//   HashMapOptions class
//   Template arguments:
//     K                 - key element type
//     V                 - value element type
//     KeyLifetime       - used for managing resource lifetime of elements.
//     supportsObservable    - boolean to indicate support for notifications to clients
//
// Reusable helper objects
// =======================
//
// ### class SimpleVectorView
//   A read only wrapper around vector implementations.  May be used to
//   implement IVector<T>::GetView and IIterable<T> on an IVector
//   implementation that is otherwise complete.  May also be used
//   on types which do not implement IVector, but which have the
//   appropriate member functions.
//
//   OPTIONAL, version tagging.
//   The version tag may optionally be specified.  It must be default
//   constructable, constructable from TVector*, and support op==.
//   'a == b' should be true if the collection was not modified between
//   the time 'a' was constructed and the time 'b' was constructed.  This
//   is used to invalidate the view should the collection change.  Default
//   implementation never invalidates.
//
//   Requirements:
//       Requires that TVector implements the methods get_Size and GetAt.
//
//   Template arguments:
//       TElem      - element type
//       TVector    - vector interface or class
//       VersionTag - (opt) version tag
//
//   Methods:
//       static HRESULT Make(TVector*, SimpleVectorView**);
//         Create an instance.  Eg.
//
// ### class SimpleVectorIterator
//   A read only iterator around vector implementations.  May be used to
//   implement IIterable<T>::First on an IVector implementation that is
//   otherwise complete.  May also be used on types which do not implement
//   IVector, but which have the appropriate member functions.
//
//   OPTIONAL, version tagging.
//   See SimpleVectorView for more information.
//
//   Requirements:
//       Requires that TVector implements the methods get_Size and GetAt.
//
//   Template arguments:
//      TElem      - element type
//      TVector    - vector interface or class
//      VersionTag - (opt) version tag
//
//   Methods:
//      static HRESULT Make(TVector*, SimpleVectorIterator**);
//        Create an instance.
//
// ### VectorIndexOf
//   A function that implements IndexOf search.  May be used to
//   Implement IVector::IndexOf or IVectorView::IndexOf on an
//   implementation that is otherwise complete.
//
//   Requirements:
//       Requires that TVector implements the methods get_Size and GetAt.
//
//   Arguments:
//       TVector* pVector        - vector implementation
//       const TElem& element    - element being searched for
//       const EqPred& pred      - (opt) an equality predicate over TElem
//       LifetimeTraits          - (opt) TElem lifetime traits
//       _Out_ unsigned* puIndex - index where found, if found.
//       _Out_ boolean*  pfFound - indicates if element was found
//
// ### class NaiveSplitMapView
//   Provides an implementation of IMapView::Split for otherwise complete
//   IMapView implementations to reuse.
//
//   Requirements:
//     User implementation must implement IIterable::First and
//     IMapView::Size methods.
//
//   Arguments (roughly same as HashMap)
//     K                 - key element type
//     V                 - value element type
//     EqualityPredicate - (opt*) used for IndexOf equality.
//     KeyLifetime       - (opt*) used for managing resource lifetime of elements.
//     ValueLifetime     - (opt*) used for managing resource lifetime of elements.
//     Options           - (opt)  other collections options, see CollectionOptions
//
//   Methods:
//     static HRESULT Split(IMapView* pOriginal, IMapView** ppFirstPartition, IMapView** ppSecondPartition);
//     static HRESULT Split(IMapView* pOriginal, IMapView** ppFirstPartition, IMapView** ppSecondPartition, const EqualityPredicate&);
//        Potentially creates an instance and perform split, if needed.
//        Note: equality predicate must be provided if default does not exist
//        for keys -- eg, if keys are structs.
//
//
//   Example Usage:
//   |  HRESULT MyMapView::Split(IMapView<K,V>** ppFirstPartition,
//   |                           IMapView<K,V>** ppSecondPartition)
//   |  {
//   |     return NaiveSplitView<K,V>::Split(this, ppFirstPartition, ppSecondPartition);
//   |  }
//
//
// **************************************************************

#pragma once

#define WINRT_WINDOWSCOLLECTIONSP_H

// Collection types sometimes appear in internally published libs.
// To prevent developers from accidentally linking binary-incompatible
// versions of the headers leading to difficult-to=debug crashes, bump
// this value when changing the binary layout of objects. This value
// should not change frequently.
//
// If you are seeing a build error on this line, you need to 'git pull' to
// get the latest built publics or need to snap your enlistment to the
// build timestamp.
#pragma detect_mismatch("windowscollectionsp_odr_check", "Version-2")

#include <intrin.h>

// minATL
#include <wrl\event.h>
#include <wrl\ftm.h>

// internal definitions
#include "xhashmap.h"
#include <roerrorapi.h>

#include <string.h>
#include <guiddef.h>
#include <windows.h>

#include "AssertMacros.h"

#ifndef WINDOWS_FOUNDATION_COLLECTIONS_H
#error \
You MUST include your WINRT/IDL generated header *before* \
including winrt\windowscollectionsp.h, so that the necessary \
collections interface templates will be defined for you.
#endif

namespace Windows { namespace Foundation { namespace Collections { namespace Internal
{
    template <class T>
    struct DefaultEqualityPredicate;

    template <class T>
    struct DefaultHash;

    template <class T>
    struct DefaultLifetimeTraits;

}}}}

namespace XWinRT
{
    // PURPOSE
    //  STL functions that are needed for proper rvalue ref handling, and
    //  for alignment calculation
    //
    // NOTE
    //  This code is essentially copied unchanged from the STL 10 code.
    //
    //  we need to put the STL stuff in its own namespace so that
    //  WRL won't improperly pick up our swap/move/forward definitions
    //  when comptr's to XWinRT objects are in play.
    namespace FakeStl
    {
        /*
         * Currently, all MS C compilers for Win32 platforms default to 8 byte
         * alignment.
         */
        #define XWINRT_CRT_PACKING 8
        #pragma pack(push,XWINRT_CRT_PACKING)

        // TEMPLATE CLASS integral_constant
        template<class _ty, _ty _val>
        struct integral_constant
        {    // convenient template for integral constant types
            static const _ty value = _val;

            typedef _ty value_type;
            typedef integral_constant<_ty, _val> type;
        };

            // TEMPLATE _Cat_base
        template<bool>
        struct _cat_base;
        template<>
        struct _cat_base<false>
        : integral_constant<bool, false>
        {   // base class for type predicates
        };

        template<>
        struct _cat_base<true>
        : integral_constant<bool, true>
        {   // base class for type predicates
        };

        // TEMPLATE CLASS is_base_of
        template<class _base, class _der>
        struct is_base_of : _cat_base<__is_base_of(_base, _der)>
        {   // determine whether _Base is a base of or the same as _Der
        };

        template<class _ty>
        struct is_enum : _cat_base<__is_enum(_ty)>
        {   // determine whether _Ty is an enumerated type
        };

        // TEMPLATE CLASS identity
        template<class T>
        struct identity
        {    // map _Ty to type unchanged
            typedef T type;

            const T& operator()(const T& left) const
            {    // apply identity operator to operand
                return (left);
            }
        };

        // TEMPLATE FUNCTION forward
        template<class T> inline
        T&& forward(typename XWinRT::FakeStl::identity<T>::type& arg)
        {    // forward arg, given explicitly specified type parameter
            return ((T&&)arg);
        }

        template<class T>
        struct remove_reference
        {    // remove reference
            typedef T type;
        };

        template<class T>
        struct remove_reference<T&>
        {    // remove reference
            typedef T type;
        };

        template<class T>
        struct remove_reference<T&&>
        {    // remove rvalue reference
            typedef T type;
        };

        // TEMPLATE FUNCTION move
        template<class T> inline
        typename XWinRT::FakeStl::remove_reference<T>::type&&
        move(T&& arg)
        {    // forward _Arg as movable
            return ((typename XWinRT::FakeStl::remove_reference<T>::type&&)arg);
        }

        // TEMPLATE FUNCTION swap (from <algorithm>)
        template<class T> inline
        void swap(T& lhs, T& rhs)
        {    // exchange values stored at lhs and rhs
            using XWinRT::FakeStl::move;
            T tmp = move(lhs);
            lhs = move(rhs);
            rhs = move(tmp);
        }

        // TEMPLATE CLASS alignment_of
        template<class _ty>
        struct _get_align
        {    // struct used to determine alignemt of _ty
            _ty _elt0;
            char _elt1;
            _ty _elt2;
        };
        #define XWINRT_ALIGN_OF(ty) (sizeof(_get_align<ty>) - 2 * sizeof(ty))

        template<class _ty>
        struct alignment_of
        : integral_constant<size_t, XWINRT_ALIGN_OF(_ty)>
        {    // determine alignment of _ty
        };

        template<class _ty>
        struct alignment_of<_ty&>
        : integral_constant<size_t, XWINRT_ALIGN_OF(_ty *)>
        {    // assume references are aligned like pointers
        };

        #define XWINRT_FITS(ty) _align == XWINRT_ALIGN_OF(ty)
        #define XWINRT_NEXT_ALIGN(ty) \
            typedef typename _aligned<_len, _align, ty, XWINRT_FITS(ty)>::_type _type

        template<class _ty, size_t _len> union _align_type
        {    // union with size _len bytes and alignment of _ty
            _ty _val;
            char _pad[_len];
        };

        template<size_t _len, size_t _align, class _ty, bool _ok>
        struct _aligned;

        template<size_t _len, size_t _align, class _ty>
        struct _aligned<_len, _align, _ty, true>
        {    // define type with size _len and alignment _ty
            typedef _align_type<_ty, _len> _type;
        };

        template<size_t _len, size_t _align>
        struct _aligned<_len, _align, long, false>
        {    // define type with size _len and alignment _ty
            typedef _align_type<double, _len> _type;
        };

        template<size_t _len, size_t _align>
        struct _aligned<_len, _align, int, false>
        {    // define type with size _len and alignment _ty
            XWINRT_NEXT_ALIGN(long);
        };

        template<size_t _len, size_t _align>
        struct _aligned<_len, _align, short, false>
        {    // define type with size _len and alignment _ty
            XWINRT_NEXT_ALIGN(int);
        };

        template<size_t _len, size_t _align>
        struct _aligned<_len, _align, char, false>
        {    // define type with size _len and alignment _ty
            XWINRT_NEXT_ALIGN(short);
        };

        template<size_t _len, size_t _align>
        struct aligned_storage
        {    // define type with size _len and alignment _ty
            typedef typename _aligned<_len, _align, char, XWINRT_FITS(char)>::_type type;
        };

        #undef XWINRT_ALIGN_OF
        #undef XWINRT_FITS
        #undef XWINRT_NEXT_ALIGN

        #undef XWINRT_CRT_PACKING
        #pragma pack(pop)
    }

    namespace detail
    {

        // BEHAVIOR
        //   calls RoOriginateError with 'hr', and then returns 'hr'
        // PURPOSE
        //   Helper function to report errors in a fluent style. This
        //   reduces code duplication.  Eg, calling RoOriginateError
        //   directly:
        //       RoOriginateError(E_FOO, nullptr);
        //       hr = E_FOO;
        //   the duplication permits the possibility of the two lines
        //   to drift out of sync.  Instead:
        //       hr = FailsWith(E_FOO);
        //   avoids this duplication.
        //
        //   Also, without RoOriginateError, it makes a handy debug break-point
        //     in no_opt builds (but inlines away in opt builds)
        inline HRESULT FailsWith(HRESULT hr, HSTRING message = nullptr)
        {
            ::RoOriginateError(hr, message);
            return hr;
        }

        struct AcquireRead
        {
            template <class TLock>
            static bool acquire(TLock& lock)
            {
                lock.EnterRead();
                return true;
            }
            template <class TLock>
            static void release(TLock& lock)
            {
                lock.LeaveRead();
            }
        };

        struct AcquireWrite
        {
            template <class TLock>
            static bool acquire(TLock& lock)
            {
                lock.EnterWrite();
                return true;
            }
            template <class TLock>
            static void release(TLock& lock)
            {
                lock.LeaveWrite();
            }
        };

        struct TryAcquireRead
        {
            template <class TLock>
            static bool acquire(TLock& lock)
            {
                return lock.TryEnterRead();
            }
            template <class TLock>
            static void release(TLock& lock)
            {
                lock.LeaveRead();
            }
        };

        struct TryAcquireWrite
        {
            template <class TLock>
            static bool acquire(TLock& lock)
            {
                return lock.TryEnterWrite();
            }
            template <class TLock>
            static void release(TLock& lock)
            {
                lock.LeaveWrite();
            }
        };

        // generic lock RAII object
        template <class TLock, class LockVerbs>
        class LockHolder
        {
        public:
            LockHolder()
            : _pLock(nullptr)
            {
            }
            LockHolder(LockHolder&& other)
            : _pLock(nullptr)
            {
                using XWinRT::FakeStl::swap;
                swap(_pLock, other._pLock);
            }
            LockHolder(TLock* pLock)
            : _pLock(pLock)
            {
                if (!LockVerbs::acquire(*_pLock))
                {
                    _pLock = nullptr;
                }
            }
            typedef TLock* (LockHolder::*bool_type);
            operator bool_type()
            {
                return _pLock ? &LockHolder::_pLock : nullptr;
            }

            // exits the lock early
            void Reset()
            {
                if (_pLock)
                {
                    LockVerbs::release(*_pLock);
                    _pLock = nullptr;
                }
            }

            ~LockHolder()
            {
                Reset();
            }

        private:
            LockHolder(const LockHolder&); // = delete
            void operator=(const LockHolder&); // = delete

            TLock* _pLock;
        };

        // No reentrancy if vector is not observable, implement as no-op
        template <bool supportsObservable>
        struct ReentrancyGuard {
            const HRESULT hr() { return S_OK; }
            bool IsFlagged() const { return true; }
            ReentrancyGuard(...) {}
        };

        // To use this guard declare a ReentrancyGuard variable with default parameters, then
        //   inside the relevant lock assign to it a new ReentrancyGuard created with the single
        //   parameter constructor, passing the address of the long variable over which the guard
        //   is implemented. When the ReentrancyGuards go out of scope, the lock will be released.
        template<>
        struct ReentrancyGuard<true> {
        private:
            HRESULT _hr;
            volatile long* _pGuard;
            ReentrancyGuard(const ReentrancyGuard&); // = delete
        public:
            const HRESULT hr()
            {
                // This function is called to check and see if the reentrancy guard
                // was acquired on construction.  The actual failure happens in the
                // constructor, but the reporting of the error is delayed until hr()
                // is called.  Usually, this will succeed, and we only want to report a
                // failure if we actually fail.
                if (FAILED(_hr))
                {
                    (void) XWinRT::detail::FailsWith(_hr);
                }
                return _hr;
            }
            bool IsFlagged() const
            {
                return (_pGuard != nullptr);
            }
            // This constructor is used to acquire a guard (operation must be done under write lock)
            ReentrancyGuard() : _pGuard(nullptr), _hr(RO_E_CHANGE_NOTIFICATION_IN_PROGRESS) {}
            ReentrancyGuard(long listeners, volatile long* pGuard) : _pGuard(nullptr), _hr(RO_E_CHANGE_NOTIFICATION_IN_PROGRESS)
            {
                if ( *pGuard == 0 )
                {
                    _hr = S_OK;

                    // No memory barrier necessary.  Used to determine whether to fire the event or not.  Such is protected
                    // by its own locking.
                    if (listeners > 0)
                    {
                        _pGuard = pGuard;
                        *_pGuard = 1;
                    }
                }
            }
            // Move constructor is used to transfer ownership of the reentrancy guard
            ReentrancyGuard(ReentrancyGuard&& rg)
            {
                _pGuard = rg._pGuard;
                _hr = rg._hr;

                //reinitialize the source to default behavior
                rg._pGuard = nullptr;   // To not release the guard prematurely
                rg._hr = RO_E_CHANGE_NOTIFICATION_IN_PROGRESS;
            }
            // Copy/move combined assignment operator
            ReentrancyGuard& operator=(ReentrancyGuard rg)
            {
                XWinRT::FakeStl::swap(_pGuard, rg._pGuard);
                XWinRT::FakeStl::swap(_hr, rg._hr);
                return *this;
            }
            // Destructor releases the guard
            ~ReentrancyGuard()
            {
                if ( nullptr != _pGuard )
                {
                    *_pGuard = 0;
                }
            }
        };

        // Couples invalidation-checking with performing an action.
        // The action is executed without any lock being held.
        // Because version-change can be detected after the action is performed,
        // an undoFunctor is needed.
        template<class TVector, class VersionTag>
        class InvalidationChecker
        {
        public:
            InvalidationChecker(TVector *pVector) : _hrInvalid(S_OK), _version(pVector) {}

            template<class T, class U>
            HRESULT Do(TVector* pVector, const T &doFunctor, const U &undoFunctor)
            {
                HRESULT hr = _hrInvalid;

                if (FAILED(hr))
                {
                    ::RoOriginateError(hr, nullptr);
                }
                else
                {
                    hr = doFunctor();

                    VersionTag vt = VersionTag(pVector);
                    if (_version == vt)
                    {
                        // No version change, so our work is done.  The HRESULT
                        // returned by the doFunctor will propagate.
                    }
                    else
                    {
                        // The underlying vector has been changed, possibly before
                        // we actually called doFunctor (but after any previous
                        // SimpleVectorView call), or possibly by a parallel
                        // thread.

                        // We'll remember that the change has occurred
                        // so that additional operations will fail quickly.
                        _hrInvalid = E_CHANGED_STATE;

                        // But we still must report the error correctly to
                        // any present debugger, and if doFunctor succeeded,
                        // we have to undo that operation.
                        if (SUCCEEDED(hr))
                        {
                            ::RoOriginateError(E_CHANGED_STATE, nullptr);
                            undoFunctor();
                        }
                        else
                        {
                            ::RoTransformError(hr, E_CHANGED_STATE, nullptr);
                        }

                        // Regardless of whether doFunctor succeeded or failed,
                        // we will return E_CHANGED_STATE.
                        hr =  E_CHANGED_STATE;
                    }
                }

                return hr;
            }

        private:

            // NOTE: This need not be volatile.  If we misobserve as a result, the *worst* thing that will happen
            // is that an operation which might not have been performed will be and will detect the version conflict
            // in a more expensive way.
            HRESULT _hrInvalid;
            VersionTag _version;
        };

        // Helper class to perform a lock free atomic update of a variable based on an operation.  Optimizes out the
        // CAS loop for STA only objects.
        struct AtomicUpdates
        {
        public:

            template<class OP, class UNDO>
            static HRESULT AtomicUpdate(unsigned *pValue, const OP& op, const UNDO& undo)
            {
                HRESULT hr = S_OK;

                unsigned originalValue = *pValue;
                for(;;)
                {
                    unsigned newValue;
                    hr = op(originalValue, &newValue);
                    if (SUCCEEDED(hr))
                    {
                        unsigned xchgValue = InterlockedCompareExchange(reinterpret_cast<volatile LONG *>(pValue), newValue, originalValue);
                        if (xchgValue == originalValue)
                        {
                            break;
                        }
                        else
                        {
                            undo();
                            originalValue = xchgValue;
                        }
                    }
                    else
                    {
                        break;
                    }
                }

                return hr;
            }

            template<class OP>
            static HRESULT STAUpdate(unsigned *pValue, const OP& op)
            {
                return op(*pValue, pValue);
            }
        };

        // AtomicUpdater is a class which encapsulates the concept of a lock-free way to update some variable based on the output of a functor.
        // For an MTA or agile collection, this encapsulation is essentially a CAS loop.  For an STA collection, this is purely optimized out.
        //
        // The function which performs the update is:
        //
        // template<class OP, class UNDO>
        // DoUpdate(unsigned *pValue, const OP& op, const UNDO& undo)
        //
        // where:
        //
        //     pValue -- A pointer to the value to update
        //     op     -- A functor of the signature [...](unsigned originalValue, unsigned* newValue) -> HRESULT {...}
        //               This must perform the operation with the intent of adjusting pValue from originalValue to *newValue.  If the operation
        //               fails due to contention, the undo functor is called.
        //     undo   -- A functor of the signature [...]() -> void {...}
        //               If the operation fails due to contention, the undo functor is called and the operation is retried.
        class AtomicUpdater
        {
        public:

            AtomicUpdater()
            {
                HRESULT hr;
                APTTYPE aptType;
                APTTYPEQUALIFIER aptQualifier;

                // this should only fail if the caller fails to CoInit.  But
                //  we will default to MTA if it fails
                hr = ::CoGetApartmentType(&aptType, &aptQualifier);
                ASSERTSUCCEEDED(hr);

                if (FAILED(hr))
                {
                    aptType = APTTYPE_MTA;
                }

                _fSta = (aptType == APTTYPE_MAINSTA || aptType == APTTYPE_STA);
            }

            template<class OP, class UNDO>
            HRESULT DoUpdate(unsigned *pValue, const OP& op, const UNDO& undo)
            {
                if (_fSta)
                {
                    return AtomicUpdates::STAUpdate(pValue, op);
                }
                else
                {
                    return AtomicUpdates::AtomicUpdate(pValue, op, undo);
                }
            }

        private:

            bool _fSta;

        };

        // min and max are defined in windows.h, but can be optionally removed with a macro. Use local definitions here to
        // ensure that windowscollectionsp works in that mode.
        template <typename t>
        const t& wcp_min(const t& l, const t& r)
        {
            return (l < r) ? l : r;
        }

        template <typename t>
        const t& wcp_max(const t& l, const t& r)
        {
            return (l > r) ? l : r;
        }
    }

    // PURPOSE
    //  slim reader/writer lock, RAII class
    // USAGE:
    //  note:
    //      if acquisition fails with a runtime error, then the lock
    //      was not acquired/entered
    //
    //  example usage:
    //      SRWLock srw;  // create a slim reader/writer lock
    //
    //      auto acquired = srw.TryRead(); // try acquire read lock
    //      if (acquired)
    //      {
    //          // do something only if lock was successfully acquired
    //      }
    //      // acquired's destructor releases the lock, if needed
    //
    class SRWLock
    {
    public:
        SRWLock()
        {
           InitializeSRWLock(&_srw);
        }

        // PURPOSE
        //  attempt to acquire write lock.  returns true if acquired
        bool TryEnterWrite()
        {
            return ::TryAcquireSRWLockExclusive(&_srw) != FALSE;
        }

        // PURPOSE
        //  attempt to acquire read lock.  returns true if acquired
        bool TryEnterRead()
        {
            return ::TryAcquireSRWLockShared(&_srw) != FALSE;
        }

        // PURPOSE
        //  block, acquiring write lock.
        void EnterWrite()
        {
            ::AcquireSRWLockExclusive(&_srw);
        }

        // PURPOSE
        //  block, acquiring read lock
        void EnterRead()
        {
            ::AcquireSRWLockShared(&_srw);
        }

        // PURPOSE
        //  release write lock.
        void LeaveWrite()
        {
            ::ReleaseSRWLockExclusive(&_srw);
        }

        // PURPOSE
        //  release read lock
        void LeaveRead()
        {
            ::ReleaseSRWLockShared(&_srw);
        }

        // RAII wrapper functions
        detail::LockHolder<SRWLock, detail::TryAcquireWrite> TryWrite() { return this; }
        detail::LockHolder<SRWLock, detail::TryAcquireRead>  TryRead()  { return this; }
        detail::LockHolder<SRWLock, detail::AcquireWrite>    Write()    { return this; }
        detail::LockHolder<SRWLock, detail::AcquireRead>     Read()     { return this; }

    private:
        ::SRWLOCK _srw;
    };

    // PURPOSE
    //  STA apartment reentrancy check, modeling the locking interface
    //  of SRWLock (only supports Try* operations)
    //
    // iReadCount == 0 // no locks held
    // iReadCount >  0 // read locks held
    // iReadCount <  0 // write lock is held
    //
    // Reentrant reads are permitted, but reentrant writes are not
    class StaReentrancyLock
    {
    public:

        bool TryEnterWrite()
        {
            if (_iReadCount == 0)
            {
                _iReadCount += sc_iWriteOffset;
                return true;
            }
            return false;
        }

        bool TryEnterRead()
        {
            if (_iReadCount >= 0)
            {
                ++_iReadCount;
                return true;
            }
            return false;
        }

        void EnterWrite()
        {
            VERIFY(TryEnterWrite()); // assert not reentrant
        }

        void EnterRead()
        {
            VERIFY(TryEnterRead()); // assert not reentrant
        }

        void LeaveWrite()
        {
            _iReadCount -= sc_iWriteOffset;
        }

        void LeaveRead()
        {
            --_iReadCount;
        }

        detail::LockHolder<StaReentrancyLock, detail::TryAcquireWrite> TryWrite() { return this; }
        detail::LockHolder<StaReentrancyLock, detail::TryAcquireRead>  TryRead()  { return this; }
        detail::LockHolder<StaReentrancyLock, detail::AcquireWrite>    Write()    { return this; }
        detail::LockHolder<StaReentrancyLock, detail::AcquireRead>     Read()     { return this; }

    private:
        int _iReadCount {};

        // used as a flag, essentially.  We leave some room
        // to make debugging easier in case we accidentally get
        // into reentrant write by mistake, it should be easy to spot
        static const int sc_iWriteOffset = -0x10000000;
    };

    // holds either a SRWLock or StaReentrancyLock depending on
    //  apartment kind.
    //
    // note, neither lock is reentrant, so be careful not to
    //  attempt to take reentrant lock.
    class ComLock
    {
    private:
        enum Kind
        {
            Kind_Srw = 0,
            Kind_StaReentrancy = 1
        };
    public:
        ComLock()
        {
            APTTYPE aptType;
            APTTYPEQUALIFIER aptQualifier;
            HRESULT hr;

            hr = ::CoGetApartmentType(&aptType, &aptQualifier);
            ASSERTSUCCEEDED(hr);

            if (FAILED(hr))
            {
                aptType = APTTYPE_MTA;
            }

            if (aptType == APTTYPE_MAINSTA|| aptType == APTTYPE_STA)
            {
                kind = Kind_StaReentrancy;
                new (AsSrl()) StaReentrancyLock;
            }
            else
            {
                kind = Kind_Srw;
                new (AsSrw()) SRWLock;
            }
        }

        ~ComLock()
        {
            if (kind == Kind_StaReentrancy)
            {
                AsSrl()->~StaReentrancyLock();
            }
            else
            {
                AsSrw()->~SRWLock();
            }
        }

        bool TryEnterWrite()
        {
            return (kind == Kind_StaReentrancy) ? AsSrl()->TryEnterWrite() : AsSrw()->TryEnterWrite();
        }

        bool TryEnterRead()
        {
            return (kind == Kind_StaReentrancy) ? AsSrl()->TryEnterRead() : AsSrw()->TryEnterRead();
        }

        void EnterWrite()
        {
            if (kind == Kind_StaReentrancy)
            {
                AsSrl()->EnterWrite();
            }
            else
            {
                AsSrw()->EnterWrite();
            }
        }

        void EnterRead()
        {
            if (kind == Kind_StaReentrancy)
            {
                AsSrl()->EnterRead();
            }
            else
            {
                AsSrw()->EnterRead();
            }
        }

        void LeaveWrite()
        {
            if (kind == Kind_StaReentrancy)
            {
                AsSrl()->LeaveWrite();
            }
            else
            {
                AsSrw()->LeaveWrite();
            }
        }

        void LeaveRead()
        {
            if (kind == Kind_StaReentrancy)
            {
                AsSrl()->LeaveRead();
            }
            else
            {
                AsSrw()->LeaveRead();
            }
        }

        detail::LockHolder<ComLock, detail::TryAcquireWrite> TryWrite() { return this; }
        detail::LockHolder<ComLock, detail::TryAcquireRead>  TryRead()  { return this; }
        detail::LockHolder<ComLock, detail::AcquireWrite>    Write()    { return this; }
        detail::LockHolder<ComLock, detail::AcquireRead>     Read()     { return this; }

    private:
        Kind kind;
        union {
            XWinRT::FakeStl::aligned_storage<sizeof(SRWLock),           XWinRT::FakeStl::alignment_of<SRWLock>::value>::type asSrw;
            XWinRT::FakeStl::aligned_storage<sizeof(StaReentrancyLock), XWinRT::FakeStl::alignment_of<StaReentrancyLock>::value>::type asSrl;
        };

        SRWLock* AsSrw()
        {
            return reinterpret_cast<SRWLock*>(&asSrw);
        }

        StaReentrancyLock* AsSrl()
        {
            return reinterpret_cast<StaReentrancyLock*>(&asSrl);
        }
    };

    struct SerializingLockPolicy
    {
        static detail::LockHolder<ComLock, detail::AcquireRead> Read(ComLock& lock, HRESULT *phr)
        {
            auto acquired = lock.Read();
            *phr = S_OK;
            return acquired;
        }

        static detail::LockHolder<ComLock, detail::AcquireWrite> Write(ComLock& lock, HRESULT *phr)
        {
            auto acquired = lock.Write();
            *phr = S_OK;
            return acquired;
        }
    };

    struct IntVersionTag
    {
        int _iVersion;
        template <class TContainer>
        IntVersionTag(TContainer* object) : _iVersion(object->GetVersion())
        {
        }
        bool operator==(const IntVersionTag& other)
        {
            return _iVersion == other._iVersion;
        }
    };

    // PURPOSE
    //  version tag that uses a heap pointer to ensure value
    //  is unique, and will not overflow.
    // NOTE: HashMap needs a version tag that can read tags
    //  safely only under a reader-lock
    struct SecureVersionTag
    {
        class Tag
        {
        public:
            Tag() : _iRefCount(1)
            {
            }

            long AddRef()
            {
                return _InterlockedIncrement(&_iRefCount);
            }
            long Release()
            {
                long iResult = _InterlockedDecrement(&_iRefCount);
                if (iResult == 0)
                {
                    delete this;
                }
                return iResult;
            }
            // must be called under write (exclusive) lock
            //  needs to be volatile, as view/iterator destruction
            //  may asynchronously bring ref count back to 1.
            bool IsShared() const
            {
                return static_cast<const volatile long&>(_iRefCount) > 1;
            }

        private:
            Tag(const Tag&); // = delete
            void operator=(const Tag&); // = delete

            long _iRefCount;
        };

        // PURPOSE
        //  used by collection owner, automates changing
        //  tag states.
        // DESIGN
        //  when modified, releases the current tag.
        //  before sharing a view, call EnsureTag to
        //  ensure you have a version available.
        class TagManager
        {
        public:
            TagManager() : _pTag(nullptr)
            {
            }

            HRESULT Initialize()
            {
                _pTag = new (std::nothrow) Tag;
                return _pTag ? S_OK : E_OUTOFMEMORY;
            }

            ~TagManager()
            {
                if (_pTag)
                {
                    _pTag->Release();
                }
            }

            Tag* GetVersion() const
            {
                return _pTag;
            }

            HRESULT ChangeVersion()
            {
                // skip change of version if no views/iterators
                //  are sharing it.
                if (!_pTag->IsShared())
                {
                    return S_OK;
                }

                Tag* pNewTag = new (std::nothrow) Tag;
                if (!pNewTag)
                {
                    return E_OUTOFMEMORY;
                }
                _pTag->Release();
                _pTag = pNewTag;
                return S_OK;
            }

        private:
            Tag* _pTag;
        };

        // NOTE
        //  does _not_ addref.  This must be performed manually by caller
        //  if the caller wishes to persist their reference longer
        //  than the current call
        template <class TContainer>
        SecureVersionTag(TContainer* object) : _pVersion(object->GetVersion())
        {
        }

        bool operator==(const SecureVersionTag& other)
        {
            return _pVersion == other._pVersion;
        }

        long AddRef()
        {
            return _pVersion->AddRef();
        }
        long Release()
        {
            return _pVersion->Release();
        }

    private:
        Tag* _pVersion;
    };

    // PURPOSE
    //  raii object used to manage the lifetime of WinRT value (object or
    //  builtin) in local variables.
    template <
        class T,
        class LifetimeTraits = wfci_::DefaultLifetimeTraits<T>
        >
    class AutoValue
    {
    public:
        AutoValue()
        {
            LifetimeTraits::Construct(&_value);
        }

        template <class U>
        AutoValue(const U& value, _Out_ HRESULT *pHr)
        {
            HRESULT hr = *pHr = LifetimeTraits::Construct(&_value, value);
            if (FAILED(hr))
            {
                // construct empty object
                LifetimeTraits::Construct(&_value);
            }
        }

        // PURPOSE
        //  for passing in an outparam position
        T* OutParam()
        {
            return &_value;
        }

        const T& get() const
        {
            return _value;
        }

        // take ownership of an existing value
        void Attach(T value)
        {
            Clear();
            _value = value;
        }

        // Relinquish lifetime management to caller, by clearing
        //  current value without destroying it.
        void Abandon()
        {
            LifetimeTraits::Construct(&_value);
        }
        void Clear()
        {
            LifetimeTraits::Destroy(&_value);
            LifetimeTraits::Construct(&_value);
        }
        void Swap(AutoValue& other)
        {
            using XWinRT::FakeStl::swap;
            swap(_value, other._value);
        }
        ~AutoValue()
        {
            LifetimeTraits::Destroy(&_value);
        }

    private:
        T       _value;
    };

    namespace detail
    {
        // Holders that introduce conversion temporaries where necessary when referencing a value on the
        // boundary between a storage type and an ABI type.
        template <
            class T_srctype,
            class T_reftype,
            class LifetimeTraits
            >
        class PresentationReference : private AutoValue<T_reftype, LifetimeTraits>
        {
        public:

            const T_reftype& get() const
            {
                return AutoValue::get();
            }

            PresentationReference()
            {
            }

            PresentationReference(const T_srctype& value, _Out_ HRESULT *pHr)
            {
                *pHr = Attach(value);
            }

            HRESULT Attach(const T_srctype& value)
            {
                T_reftype tmpValue;
                HRESULT hr = LifetimeTraits::Construct(&tmpValue, value);
                if (SUCCEEDED(hr))
                {
                    AutoValue::Attach(tmpValue);
                }
                return hr;
            }

        };

        template <
            class T_srctype,
            class LifetimeTraits
            >
        class PresentationReference<T_srctype, T_srctype, LifetimeTraits>
        {
        public:

            PresentationReference()
            {
            }

            PresentationReference(const T_srctype& value, _Out_ HRESULT *pHr)
            {
                *pHr = Attach(value);
            }

            const T_srctype& get() const
            {
                return *_pStorageAddress;
            }

            HRESULT Attach(const T_srctype& value)
            {
                _pStorageAddress = &value;
                return S_OK;
            }

        private:

            const T_srctype *_pStorageAddress;
        };

        template<
            class T_abi,
            class T_storage,
            class LifetimeTraits = wfci_::DefaultLifetimeTraits<T_abi>
            >
        class StorageReference : public PresentationReference<T_abi, T_storage, LifetimeTraits>
        {
            using base = PresentationReference<T_abi, T_storage, LifetimeTraits>;

        public:
            StorageReference() :
                base()
            {
            }

            StorageReference(const T_abi& value, HRESULT *pHr) :
                base(value, pHr)
            {
            }
        };

        template<
            class T_abi,
            class T_storage,
            class LifetimeTraits = wfci_::DefaultLifetimeTraits<T_abi>
            >
        class AbiReference : public PresentationReference<T_storage, T_abi, LifetimeTraits>
        {
        public:

            AbiReference() :
                PresentationReference<T_storage, T_abi, LifetimeTraits>()
            {
            }

            AbiReference(const T_storage& value, HRESULT *pHr) :
                PresentationReference<T_storage, T_abi, LifetimeTraits>(value, pHr)
            {
            }
        };
    }
}

namespace XWinRT
{
    // use built in language operator==
    struct BuiltinTypeEqualPredicate
    {
        template <class T>
        HRESULT operator()(const T& lhs, const T& rhs, _Out_ bool* fEquals) const
        {
            *fEquals = !!(lhs == rhs);
            return S_OK;
        }
    };

    struct StringEquals
    {
        HRESULT operator()(HSTRING strLhs, HSTRING strRhs, _Out_ bool* fEquals) const
        {
            HRESULT hr = S_OK;
            // not using WindowsCompareStringOrdinal to avoid
            //  dealing with a failed HRESULT, and to ensure
            //  hashing behavior matches equality
            UINT32 uLenLhs;
            PCWSTR pcwszLhs = WindowsGetStringRawBuffer(strLhs, &uLenLhs);
            UINT32 uLenRhs;
            PCWSTR pcwszRhs = WindowsGetStringRawBuffer(strRhs, &uLenRhs);

            if (uLenLhs != uLenRhs)
            {
                *fEquals = false;
            }
            else
            {
                *fEquals = memcmp(pcwszLhs, pcwszRhs, uLenRhs*sizeof(WCHAR)) == 0;
            }
            return hr;
        }
    };

    struct InterfaceEquals
    {
        // Uses IUnknown identity as an equivalence relation
        template <class TInterface>
        HRESULT operator()(TInterface* a, TInterface* b, _Out_ bool* pfEquals) const
        {
            static_assert(XWinRT::FakeStl::is_base_of<IUnknown, TInterface>::value,
                      "DefaultEqualityPredicate only supports interface derived from IUnknown");

            *pfEquals = false;
            HRESULT hr = S_OK;

            if (a == b)
            {
                *pfEquals = true;
            }
            else if (a == nullptr || b == nullptr)
            {
                *pfEquals = (a==b);
            }
            else
            {
                Microsoft::WRL::ComPtr<IUnknown> spAIdent, spBIdent;

                hr = a->template QueryInterface<IUnknown>(&spAIdent);

                if (SUCCEEDED(hr))
                {
                    hr = b->template QueryInterface<IUnknown>(&spBIdent);

                    if (SUCCEEDED(hr))
                    {
                        *pfEquals = (spAIdent == spBIdent);
                    }
                }
            }
            return hr;
        }
    };

    // PURPOSE
    //  computes the Fnv-1a hash of the byte sequence.
    inline unsigned Fnv1a(_In_reads_bytes_(cData) const BYTE* pbData, _In_ size_t cData)
    {
        // forked from multimedia\dmd\Crescent\wmp\dev\mls\hme

        unsigned dwHash = 2166136261u; // FNV Offset Basis
        for(size_t i = 0; i < cData; ++i)
        {
            dwHash ^= pbData[i];
            dwHash = dwHash * 16777619u; // FNV Prime
        }

        return dwHash;
    }

    // hash the binary value for pod types
    // Note that PodTypeHash is meant for use by DefaultHash, defined below. In that context, it is used for hashing enums.
    // PodTypeHash should *not* be used for user defined types such as structs or classes, as undefined values in padding bytes will make the hash inconsistent.
    struct PodTypeHash
    {
        template <class T>
        HRESULT operator()(T value, _Out_ UINT32* uHash) const
        {
            *uHash = Fnv1a(reinterpret_cast<BYTE*>(&value), sizeof(value));
            return S_OK;
        }
    };

    struct StringHash
    {
        HRESULT operator()(HSTRING strValue, _Out_ UINT32* uHash) const
        {
            UINT32 uLen;
            PCWSTR pcwsz = WindowsGetStringRawBuffer(strValue, &uLen);

            *uHash = Fnv1a(reinterpret_cast<const BYTE*>(pcwsz), uLen * sizeof(WCHAR));
            return S_OK;
        }
    };

    struct InterfaceHash
    {
        // NOTES: if value is null,static  hash value is zero.
        template <class TInterface>
        HRESULT operator()(TInterface* value, _Out_ UINT32* uHash) const
        {
            static_assert(XWinRT::FakeStl::is_base_of<IUnknown, TInterface>::value,
                          "DefaultHash only supports interface derived from IUnknown");
            *uHash = 0;
            Microsoft::WRL::ComPtr<IUnknown> spIdent;

            HRESULT hr = S_OK;
            if (value != nullptr)
            {
                hr = value->QueryInterface<IUnknown>(&spIdent);

                if (SUCCEEDED(hr))
                {
                    // hash the pointer, as it will be used for equality later.
                    IUnknown* punk = spIdent.Get();
                    *uHash = Fnv1a(reinterpret_cast<BYTE*>(&punk), sizeof(punk));
                }
            }

            return hr;
        }
    };

    struct PodLifetimeTraits
    {
        //  Initialize an uninitialized buffer to an 'empty' value.
        //  Used to no-fail return or set an 'empty' value.
        template <class T>
        static void Construct(_Out_ T* uninitializedElement)
        {
            memset(uninitializedElement, 0, sizeof(*uninitializedElement));
        };

        //  Initialized an element from an already initialized one,
        //  'copying' it.
        template <class T>
        static HRESULT Construct(_Out_ T* uninitializedElement, T source)
        {
            *uninitializedElement = source;
            return S_OK;
        }

        //  Release all resources owned by the specified
        //  element, leaving it in an uninitialized state
        template <class T>
        static void Destroy(_Inout_ T* /* initializedElement */)
        {
        };
    };
    struct StringLifetimeTraits
    {
        //  Initialize an uninitialized buffer to an 'empty' value.
        //  Used to no-fail return or set an 'empty' value.
        static void Construct(_Out_ HSTRING* uninitializedElement)
        {
            *uninitializedElement = nullptr;
        };

        //  Initialized an element from an already initialized one,
        //  'copying' it.
        static HRESULT Construct(_Out_ HSTRING* uninitializedElement, HSTRING source)
        {
            return WindowsDuplicateString(source, uninitializedElement);
        }

        //  Release all resources owned by the specified
        //  element, leaving it in an uninitialized state
        static void Destroy(_Inout_ HSTRING* initializedElement)
        {
            HRESULT hr = WindowsDeleteString(*initializedElement);
            XWINRT_ASSERT(SUCCEEDED(hr));

            *initializedElement = nullptr;
        };
    };
    struct InterfaceLifetimeTraits
    {
        //  Initialize an uninitialized buffer to an 'empty' value.
        //  Used to no-fail return or set an 'empty' value.
        template <class TInterface>
        static void Construct(_Outptr_result_maybenull_ TInterface **uninitializedElement)
        {
            static_assert(XWinRT::FakeStl::is_base_of<IUnknown, TInterface>::value,
                          "DefaultLifetimeTraits only supports interface derived from IUnknown");
            *uninitializedElement = nullptr;
        };

        //  Initialized an element from an already initialized one,
        //  'copying' it.
        template <class TInterface>
        static HRESULT Construct(_Outptr_result_maybenull_ TInterface **uninitializedElement, _In_opt_ TInterface* source)
        {
            static_assert(XWinRT::FakeStl::is_base_of<IUnknown, TInterface>::value,
                          "DefaultLifetimeTraits only supports interface derived from IUnknown");
            *uninitializedElement = source;
            if (source)
            {
                source->AddRef();
            }
            return S_OK;
        }

        //  Release all resources owned by the specified
        //  element, leaving it in an uninitialized state
        template <class TInterface>
        static void Destroy(_Inout_ TInterface ** initializedElement)
        {
            static_assert(XWinRT::FakeStl::is_base_of<IUnknown, TInterface>::value,
                          "DefaultLifetimeTraits only supports interface derived from IUnknown");
            if (*initializedElement)
            {
                (*initializedElement)->Release();
                *initializedElement = nullptr;
            }
        };

    };

    //
    // GIT localization for agile collections presents an interesting problem.  The act of localizing may call into the apartment
    // to perform a lazy marshaling/unmarshaling.  This means that we cannot safely localize while holding the blocking lock necessary
    // for serializing semantics.
    //
    // In order to avoid this, we must pull GIT localization outside of the scope of the collections locks.  Doing this means introducing a
    // temporary holder (a reference on the GIT cookie) which lives until outside the scope of the collections lock.  We do not want to do this
    // for collections which are non-agile or do not hold interfaces as the introduction of such a temp may involve unnecessary reference counting,
    // structure copying, string duplication, etc...
    //
    // In order to prevent this, this class performs a series of operations based on whether a temp is injected or not.  Two methods exist:
    //
    //     CopyOnDemand --  performs the copy from storage type to either temp or ABI type depending on whether the temp is required (a string
    //                      duplication does not require the temp, a GIT localization does).
    //
    //     ResolveDemand -- does nothing if a temp is not required.  Resolves the temp to an ABI type if it was.
    //
    template<class T_storage, class T_abi, class T_tmp, class LifetimeTraits>
    struct StorageTempTraits
    {
        typedef T_tmp type;

        static HRESULT CopyOnDemand(type* pTemp, T_abi*, T_storage src)
        {
            return LifetimeTraits::Construct(pTemp, src);
        }

        static HRESULT ResolveDemand(type *pTemp, T_abi* pDest)
        {
            return LifetimeTraits::Move(pDest, pTemp);
        }

        static bool RequiresTempCopy(T_storage src)
        {
            return LifetimeTraits::RequiresTempCopy(src);
        }

        static void DestroyTemp(type* pTemp)
        {
            LifetimeTraits::Destroy(pTemp);
        }
    };

    template<class T_storage, class T_abi, class LifetimeTraits>
    struct StorageTempTraits<T_storage, T_abi, T_abi, LifetimeTraits>
    {
        // If there is no intermediary type, an empty struct will serve as the type so
        // that all the templates work properly.  This should ideally get optimized out.
        struct type
        {
        };

        static HRESULT CopyOnDemand(type*, T_abi *pDest, T_storage src)
        {
            return LifetimeTraits::Construct(pDest, src);
        }

        static HRESULT ResolveDemand(type*, T_abi*)
        {
            return S_OK;
        }

        static bool RequiresTempCopy(T_storage)
        {
            return false;
        }

        static void DestroyTemp(type*)
        {
        }
    };
}

namespace Windows { namespace Foundation { namespace Collections { namespace Internal
{
    namespace detail
    {
        template<bool C, class T, class F>
        struct _if
        {
            typedef T type;
        };

        template<class T, class F>
        struct _if<false, T, F>
        {
            typedef F type;
        };

        // A set of templates that determine the appropriate storage type for a given type T.
        template<class T>
        struct StorageType
        {
            typedef T type;
            typedef T temp_type;
        };

        // A set of templates that determine the validity of the hash for a given object T.
        template<class T>
        struct BaseHashValidator
        {
            static HRESULT IsHashable(const T& /*obj*/)
            {
                return S_OK;
            }
        };

        template<class T, class Hasher>
        struct HashValidator : public BaseHashValidator<T>
        {
        };

        // Custom equality predicate and hash function must not be defined over an interface type.   These templates
        // validate such.
        struct AllFunctionsValid
        {
            static const bool value = true;
        };

        template<class Function, class DefaultFunction>
        struct DefaultFunctionsValid
        {
            static const bool value = false;
        };

        template<class Function>
        struct DefaultFunctionsValid<Function, Function>
        {
            static const bool value = true;
        };

        template<class T, class Function, class DefaultFunction>
        struct CollectionsFunctionValid : public AllFunctionsValid
        {
        };

        // Anything on an HSTRING is okay.
        template<class Function, class DefaultFunction>
        struct CollectionsFunctionValid<HSTRING, Function, DefaultFunction> : public AllFunctionsValid
        {
        };

        // Default functions on an interface (pointer non HSTRING) are okay.
        template<class T, class Function, class DefaultFunction>
        struct CollectionsFunctionValid<T*, Function, DefaultFunction> : public DefaultFunctionsValid<Function, DefaultFunction>
        {
        };
    }

    // PS_WIN8(143913) - Required for InspectableClass macro
    using namespace Microsoft::WRL;

    #define XWINRT_DEFINE_BUILTIN_FUNCTIONS( logicalBuiltinType ) \
        template <> struct DefaultEqualityPredicate< logicalBuiltinType > \
        : XWinRT::BuiltinTypeEqualPredicate {}; \
        template <> struct DefaultHash< logicalBuiltinType > \
        : XWinRT::PodTypeHash {}; \
        template <> struct DefaultLifetimeTraits< logicalBuiltinType > \
        : XWinRT::PodLifetimeTraits {};

    // builtin list gathered from REXPP sources.  See:
    //   s_rgBasicTypes
    //   GetIdlName
    // in rexpp code base.
    XWINRT_DEFINE_BUILTIN_FUNCTIONS(INT32);
    XWINRT_DEFINE_BUILTIN_FUNCTIONS(__int64);
    XWINRT_DEFINE_BUILTIN_FUNCTIONS(float);
    XWINRT_DEFINE_BUILTIN_FUNCTIONS(double);
    XWINRT_DEFINE_BUILTIN_FUNCTIONS(byte);

    #ifdef _NATIVE_WCHAR_T_DEFINED
    XWINRT_DEFINE_BUILTIN_FUNCTIONS(wchar_t);
    #endif
    XWINRT_DEFINE_BUILTIN_FUNCTIONS(unsigned short);
    XWINRT_DEFINE_BUILTIN_FUNCTIONS(signed short);


    XWINRT_DEFINE_BUILTIN_FUNCTIONS(bool);
    XWINRT_DEFINE_BUILTIN_FUNCTIONS(UINT32);
    XWINRT_DEFINE_BUILTIN_FUNCTIONS(UINT64);
    XWINRT_DEFINE_BUILTIN_FUNCTIONS(GUID);

    #undef XWINRT_DEFINE_BUILTIN_FUNCTIONS

    template <>
    struct DefaultEqualityPredicate<HSTRING>
    : XWinRT::StringEquals
    {
    };

    template <>
    struct DefaultHash<HSTRING>
    : XWinRT::StringHash
    {
    };

    template <>
    struct DefaultLifetimeTraits<HSTRING>
    : XWinRT::StringLifetimeTraits
    {
    };

    template <class TInterface>
    struct DefaultEqualityPredicate<TInterface*>
    : XWinRT::InterfaceEquals
    {
    };

    template <class TInterface>
    struct DefaultHash<TInterface*>
    : XWinRT::InterfaceHash
    {
    };

    // NOTES:
    //   for runtime classes and interface groups, the value for "TInterface*"
    //   is not the same type for which lifetime will be performed. Eg, "RCFoo".
    //   Instead, InterfaceLifetimeTraits will automatically deduce its arguments,
    //   so as to pick up the default interface when runctime classes or interface
    //   groups are used.  We static_assert that it is derived from IUnknown
    //   in the member functions.
    template <class TInterface>
    struct DefaultLifetimeTraits<TInterface*>
    : XWinRT::InterfaceLifetimeTraits
    {
    };

    namespace detail
    {
        template <class T>
        struct UnsupportedTypeForDefault
        {
            // note, dummy test, ensures reasonable error message
            static_assert(XWinRT::FakeStl::is_base_of<T, IUnknown>::value,
                          "The type 'T' is not supported by the built in equality, hash, and lifetime traits. Please provide your own implementation");
        };
    }

    // NOTES:
    //  equality, hash, and lifetime traits for enums, and
    //  potentially any other type that needs to be examined by traits.
    template <class T>
    struct DefaultEqualityPredicate
    : detail::_if<XWinRT::FakeStl::is_enum<T>::value,
                  XWinRT::BuiltinTypeEqualPredicate,
                  detail::UnsupportedTypeForDefault<T>>::type
    {
    };

    template <class T>
    struct DefaultHash
    : detail::_if<XWinRT::FakeStl::is_enum<T>::value,
                  XWinRT::PodTypeHash,
                  detail::UnsupportedTypeForDefault<T>>::type
    {
    };

    template <class T>
    struct DefaultLifetimeTraits
    : detail::_if<XWinRT::FakeStl::is_enum<T>::value,
                  XWinRT::PodLifetimeTraits,
                  detail::UnsupportedTypeForDefault<T>>::type
    {
    };

    namespace detail {

#if defined(MIDL_NS_PREFIX) || defined(XAML_USE_PUBLIC_SDK)
        namespace OuterWindows = ABI::Windows;
#else
        namespace OuterWindows = Windows;
#endif
        using OuterWindows::Foundation::Internal::GetAbiType;
    }

    namespace wfc = detail::OuterWindows::Foundation::Collections;


    class VectorChangedEventArgs WrlSealed : public Microsoft::WRL::RuntimeClass<wfc::IVectorChangedEventArgs>
    {
    private:
        InspectableClass(L"Windows.Foundation.Collections.IVectorChangedEventArgs", (::BaseTrust))

        wfc::CollectionChange _change;
        UINT32 _index;
    public:
        VectorChangedEventArgs(wfc::CollectionChange change, UINT32 index ) :
            _change(change), _index(index) { }
        STDMETHODIMP get_CollectionChange ( _Out_ wfc::CollectionChange* value ) override
        {
            *value = _change;
            return S_OK;
        }
        STDMETHODIMP get_Index (_Out_ UINT32* value) override
        {
            *value = _index;
            return S_OK;
        }
    };

    template <class T, bool supportsObservable, bool propagateError = false>
    struct VectorOptions
    {
        static const bool hasObservable = false;

        static const wchar_t* z_get_rc_name_impl() { return wfc::IVector<T>::z_get_rc_name_impl(); }
        typedef struct {} EventType;
        typedef struct {} EventTrackingType;
        static HRESULT RaiseEvent(...) { return S_OK; };
        static HRESULT AddEventHandler(...) { return S_OK; };
        static HRESULT RemoveEventHandler(...) { return S_OK; };
        static void InitializeListenerTracking(...) { }
        static void TrackListener(...) { }
        static void UntrackListener(...) { }
        static void HasTrackedListener(...) { }

        typedef XWinRT::detail::ReentrancyGuard<false> ReentrancyGuard;
        typedef XWinRT::SerializingLockPolicy LockPolicy;
    };

    template <class T, bool propagateError>
    struct VectorOptions<T, true, propagateError>
    {
        static const bool hasObservable = true;

        static const wchar_t* z_get_rc_name_impl() { return wfc::IObservableVector<T>::z_get_rc_name_impl(); }

        typedef Microsoft::WRL::InvokeModeOptions<Microsoft::WRL::FireAll> PropagateType;
        typedef Microsoft::WRL::EventSource<wfc::VectorChangedEventHandler<T>, PropagateType> EventType;
        typedef long EventTrackingType;

        typedef XWinRT::detail::ReentrancyGuard<true> ReentrancyGuard;
        typedef XWinRT::SerializingLockPolicy LockPolicy;

        static HRESULT RaiseEvent(ReentrancyGuard& guard, EventType& e, wfc::IObservableVector<T>* sender, wfc::CollectionChange change, UINT32 index)
        {
            HRESULT hr = S_OK;
            // At the moment, observers interfere with serializibility.  When we see no observers, we make a guarantee
            // about serialization under that lock policy.  Once we've noticed that there are no observers, we commit
            // to that decision in raising the event.  One cannot "sneak through".  Even if one comes, the event will
            // not get fired.  The only way this happens is adding an observer during a write.  In this case, we
            // serialize in the "write then add" order regardless of when the observer is placed in the actual list.
            if (guard.IsFlagged())
            {
                Microsoft::WRL::ComPtr<VectorChangedEventArgs> spArgs =
                    Microsoft::WRL::Make<VectorChangedEventArgs>(change, index);
                // Check if we failed to create the arguments object (OOM), and if so don't call the notification
                if ( nullptr != spArgs.Get() )
                {
                    // Notification result ignored
                    hr = e.InvokeAll(sender, spArgs.Get());
                }
            }

            return hr;
        }
        static HRESULT AddEventHandler(EventType& e, _In_opt_ wfc::VectorChangedEventHandler<T>* handler, _Out_ EventRegistrationToken* token)
        {
            return e.Add(handler, token);
        };
        static HRESULT RemoveEventHandler(EventType& e, EventRegistrationToken token)
        {
            return e.Remove(token);
        };
        static void InitializeListenerTracking(EventTrackingType& t)
        {
            t = 0;
        }
        static void TrackListener(EventTrackingType& t)
        {
            InterlockedIncrement(static_cast<volatile long *>(&t));
        }
        static void UntrackListener(EventTrackingType& t)
        {
            InterlockedDecrement(static_cast<volatile long *>(&t));
        }
        static void HasTrackedListener(EventTrackingType& t)
        {
            // No memory barrier necessary.  Used to determine whether to fire the event or not.  Such is protected
            // by its own locking.
            return (t > 0);
        }

    };

    template <class T>
    struct DefaultVectorOptions : VectorOptions<T, false, false>
    {
    };

    template <
        class T,
        class EqualityPredicate = DefaultEqualityPredicate<T>,
        class LifetimeTraits = DefaultLifetimeTraits<T>,
        class Options = DefaultVectorOptions<T>
    >
    class Vector : public
        detail::_if <Options::hasObservable,
            Microsoft::WRL::RuntimeClass<wfc::IVector<T>, wfc::IIterable<T>, wfc::IObservableVector<T>>,
            Microsoft::WRL::RuntimeClass<wfc::IVector<T>, wfc::IIterable<T>>
            >::type
    {
        using base = typename detail::_if <Options::hasObservable,
                Microsoft::WRL::RuntimeClass<wfc::IVector<T>, wfc::IIterable<T>, wfc::IObservableVector<T>>,
                Microsoft::WRL::RuntimeClass<wfc::IVector<T>, wfc::IIterable<T>>
            >::type;

        using RuntimeClassT = typename base::RuntimeClassT;
        typedef typename detail::GetAbiType<typename wfc::IVector<T>::T_complex>::type     T_abi;
        typedef typename detail::StorageType<T_abi>::type                                   T_storage;
        typedef typename detail::StorageType<T_abi>::temp_type                              T_tmp;
        typedef typename XWinRT::StorageTempTraits<T_storage, T_abi, T_tmp, LifetimeTraits>        T_TempTraits;

        static const wchar_t* z_get_rc_name()
        {
            // use the MIDLRT generated name string, which substitutes
            //   the proper metadata-name-representation of T
            return Options::z_get_rc_name_impl();
        }
        InspectableClass(z_get_rc_name(), (::BaseTrust))

    private:
        // PURPOSE
        //  constructor must be public for WRL to be able to call it,
        //  but should not generally be callable by clients.
        struct permission{};

    public:
        Vector(const EqualityPredicate& p, permission);

    public:
        static HRESULT Make(const EqualityPredicate& fnEquals, _COM_Outptr_ Vector** ppVector)
        {
            *ppVector = nullptr;
            HRESULT hr = S_OK;

            auto sp = Microsoft::WRL::Make<Vector>(fnEquals, permission());
            if (!sp)
            {
                hr = E_OUTOFMEMORY;
            }
            if (SUCCEEDED(hr))
            {
                *ppVector = sp.Detach();
            }
            return hr;
        }
        static HRESULT Make(_COM_Outptr_ Vector** ppVector)
        {
            return Make(EqualityPredicate(), ppVector);
        }

        int GetVersion();

        // IIterable
        IFACEMETHODIMP First(_Outptr_result_maybenull_ wfc::IIterator<T> **first) override;

        // read methods
        IFACEMETHODIMP GetAt(_In_ unsigned index, _Out_ T_abi *returnValue) override;
        /* propget */ IFACEMETHODIMP get_Size(_Out_ unsigned *returnValue) override;
        IFACEMETHODIMP GetView(_Outptr_result_maybenull_ wfc::IVectorView<T> **returnValue) override;
        IFACEMETHODIMP IndexOf(_In_opt_ T_abi value, _Out_ unsigned *index, _Out_ boolean *returnValue) override;

        // write methods
        IFACEMETHODIMP SetAt(_In_ unsigned index, _In_opt_ T_abi value) override;
        IFACEMETHODIMP InsertAt(_In_ unsigned index, _In_opt_ T_abi value) override;
        IFACEMETHODIMP RemoveAt(_In_ unsigned index) override;
        IFACEMETHODIMP Append(_In_opt_ T_abi value) override;
        IFACEMETHODIMP RemoveAtEnd() override;
        IFACEMETHODIMP Clear() override;

        // bulk transfer methods
        IFACEMETHODIMP GetMany(_In_  unsigned startIndex, _In_ unsigned capacity, _Out_writes_to_(capacity,*actual) T_abi *value, _Out_ unsigned *actual) override;
        IFACEMETHODIMP ReplaceAll(_In_ unsigned count, _In_reads_(count) T_abi *value) override;


        // IObservableVector
        IFACEMETHODIMP add_VectorChanged(_In_opt_ wfc::VectorChangedEventHandler<T> *, _Out_ EventRegistrationToken *);
        IFACEMETHODIMP remove_VectorChanged(_In_ EventRegistrationToken);


    protected:
        ~Vector() override;

    private:
        Vector(const Vector&);          // = delete
        void operator=(const Vector&);  // = delete

        // Internal implementation of InsertAt/Append and RemoveAt/RemoveAtEnd.  Both of these methods
        // exist such that all described calls fall under the serialization guarantees we are providing.  If
        // fLast is true, they are being called for Append or RemoveAtEnd and the index parameter is ignored.
        HRESULT InsertAtInternal(_In_ unsigned index, _In_opt_ T_abi value, _In_ bool fLast = false);
        HRESULT RemoveAtInternal(_In_ unsigned index, _In_ bool fLast = false);
        HRESULT IndexOfInternal(_In_ T_storage *pStorage, _In_ unsigned uSize, _In_opt_ T_abi value, _Inout_ unsigned *index, _Inout_ boolean *found);

        HRESULT ResizeStorage(_In_ unsigned newSize);
        void _EraseAll(T_storage** ppaT, unsigned *puSize);
        static void _Free(T_storage* paT, unsigned uSize);

        unsigned _uSize;
        unsigned _uCapacity;
        unsigned _uDemandCopy;
        T_storage* _paT;
        EqualityPredicate _fnEquals;
        XWinRT::ComLock _comLock;
        int _iVersion;
        static const unsigned int s_cuMaxSize = 0x80000000-1; // 2**31-1

        typename Options::EventType _evtVectorChanged;
        typename Options::EventTrackingType _listeners;
        long                        _reentrancyGuard;
    };

    template <
        class T,
        class EqualityPredicate = DefaultEqualityPredicate<T>,
        class LifetimeTraits = DefaultLifetimeTraits<T>
    >
    class ObservableVector WrlSealed : public Vector < T, EqualityPredicate, LifetimeTraits, VectorOptions <T, true> >
    {
        using base = Vector<T, EqualityPredicate, LifetimeTraits, VectorOptions<T, true>>;

    private:
        ObservableVector();
    public:
        static HRESULT Make(const EqualityPredicate& fnEquals, _COM_Outptr_ ObservableVector** ppVector)
        {
            // The cast in the following line works because ObservableVector derives *only* from Vector and
            //   does not contain any data members. Any change to these conditions is bound to break this call
            //   and will result in failures at runtime.
            return base::Make( fnEquals, reinterpret_cast<base**>(ppVector) );
        }
        static HRESULT Make(_COM_Outptr_ ObservableVector** ppVector)
        {
            return Make(EqualityPredicate(), ppVector);
        }
    };

    // PURPOSE
    //  reusable IVector::IndexOf implementation
    template <
        class TVector,
        class T_abi,
        class EqualityPredicate,
        class LifetimeTraits
    >
    HRESULT VectorIndexOf(
        TVector*                    pVector,
        const T_abi&                element,
        const EqualityPredicate&    fnEquals,
        LifetimeTraits,
        _Out_ unsigned*             puIndex,
        _Out_ boolean*              pfFound
        )
    {
        using XWinRT::AutoValue;

        HRESULT hr = S_OK;
        *puIndex = 0;
        *pfFound = false;
        unsigned uIndex = 0;
        unsigned uSize;

        hr = pVector->get_Size(&uSize);

        while (SUCCEEDED(hr) && uIndex < uSize)
        {
            AutoValue<T_abi, LifetimeTraits> curElem;
            bool fEquals;

            hr = pVector->GetAt(uIndex, curElem.OutParam());
            if (SUCCEEDED(hr))
            {
                hr = fnEquals(element, curElem.get(), &fEquals);
                if (SUCCEEDED(hr))
                {
                    if (fEquals)
                    {
                        *pfFound = true;
                        *puIndex = uIndex;
                        break;
                    }
                    uIndex++;
                }
            }
        }
        return hr;
    }

    // PURPOSE
    //  reusable IVector::IndexOf implementation
    template <
        class TVector,
        class T_abi,
        class EqualityPredicate
    >
    HRESULT VectorIndexOf(
        TVector*                    pVector,
        const T_abi&                element,
        const EqualityPredicate&    fnEquals,
        _Out_ unsigned*             puIndex,
        _Out_ boolean*              pfFound
        )
    {
        return VectorIndexOf(pVector, element, fnEquals, DefaultLifetimeTraits<T_abi>(), puIndex, pfFound);
    }

    // PURPOSE
    //  reusable IVector::IndexOf implementation
    template <
        class TVector,
        class T_abi
    >
    HRESULT VectorIndexOf(
        TVector*                    pVector,
        const T_abi&                element,
        _Out_ unsigned*             puIndex,
        _Out_ boolean*              pfFound
        )
    {
        return VectorIndexOf(pVector, element, DefaultEqualityPredicate<T_abi>(), DefaultLifetimeTraits<T_abi>(), puIndex, pfFound);
    }

    template <
        class T,
        class EqualityPredicate,
        class LifetimeTraits,
        class Options
    >
    Vector<T,EqualityPredicate,LifetimeTraits,Options>
        ::Vector(const EqualityPredicate& p, permission) :
        _uSize(0),
        _uCapacity(0),
        _uDemandCopy(0),
        _paT(nullptr),
        _fnEquals(p),
        _iVersion(0),
        _reentrancyGuard(0)
    {
        // Providing custom equality predicates on interface types risks deadlock if calling into an (A)STA object.  Prevent this for now.
        static_assert(detail::CollectionsFunctionValid<T, EqualityPredicate, DefaultEqualityPredicate<T>>::value,
            "Custom equality predicates can not be provided on interface types");

        Options::InitializeListenerTracking(_listeners);
    }

    // Frees all elements and storage array.  Does not take lock.
    template <
        class T,
        class EqualityPredicate,
        class LifetimeTraits,
        class Options
    >
    void Vector<T,EqualityPredicate,LifetimeTraits,Options>
        ::_EraseAll(T_storage **ppaT, unsigned *puSize)
    {
        // Must provide both or neither.
        ASSERT((ppaT && puSize) || (!ppaT && !puSize));

        T_storage *paT = _paT;
        unsigned uSize = _uSize;

        _uSize = 0;
        _uCapacity = 0;
        _uDemandCopy = 0;
        _iVersion++;
        _paT = nullptr;

        if (ppaT)
        {
            *ppaT = paT;
            *puSize = uSize;
        }
        else
        {
            _Free(paT, uSize);
        }
    }

    // Frees all elements and storage array.  Does not take lock.
    template <
        class T,
        class EqualityPredicate,
        class LifetimeTraits,
        class Options
    >
    void Vector<T,EqualityPredicate,LifetimeTraits,Options>
        ::_Free(T_storage *paT, unsigned uSize)
    {
        if(paT)
        {
            for(unsigned i=0; i < uSize; i++)
            {
                LifetimeTraits::Destroy(&paT[i]);
            }
            ::free(paT);
        }
    }

    template <
        class T,
        class EqualityPredicate,
        class LifetimeTraits,
        class Options
    >
    Vector<T,EqualityPredicate,LifetimeTraits,Options>
        ::~Vector()
    {
        _EraseAll(nullptr, nullptr);
    }

    // GetVersion
    template <
        class T,
        class EqualityPredicate,
        class LifetimeTraits,
        class Options
    >
    int Vector<T,EqualityPredicate,LifetimeTraits,Options>
        ::GetVersion()
    {
        return _iVersion;
    }

    //IIterable::First
    template <
        class T,
        class EqualityPredicate,
        class LifetimeTraits,
        class Options
    >
    IFACEMETHODIMP Vector<T,EqualityPredicate,LifetimeTraits,Options>
        ::First(_Outptr_result_maybenull_ wfc::IIterator<T> **first)
    {
        HRESULT hr = S_OK;
        *first = nullptr;

        Microsoft::WRL::ComPtr<SimpleVectorIterator<T, Vector, LifetimeTraits, XWinRT::IntVersionTag>> sp;

        hr = SimpleVectorIterator<T, Vector, LifetimeTraits, XWinRT::IntVersionTag>::Make(this, &sp);

        if (SUCCEEDED(hr))
        {
            *first = static_cast<Microsoft::WRL::ComPtr<wfc::IIterator<T>>>(sp).Detach();
        }

        return hr;
    }

    // IVector::GetAt
    template <
        class T,
        class EqualityPredicate,
        class LifetimeTraits,
        class Options
    >
    IFACEMETHODIMP Vector<T,EqualityPredicate,LifetimeTraits,Options>
        ::GetAt(_In_ unsigned index, _Out_ T_abi *returnValue)
    {
        LifetimeTraits::Construct(returnValue);

        HRESULT hr;

        typename T_TempTraits::type temp;

        { // lock duration
            auto acquired = Options::LockPolicy::Read(_comLock, &hr);

            if (SUCCEEDED(hr))
            {
                if (index >= _uSize)
                {
                    hr = XWinRT::detail::FailsWith(E_BOUNDS);
                }

                if (SUCCEEDED(hr))
                {
                    LifetimeTraits::Destroy(returnValue);

                    // Depending on the temp traits, this may make a temporary copy into &temp in order to perform
                    // certain operations outside of lock scope during a ResolveDemand call.
                    hr = T_TempTraits::CopyOnDemand(&temp, returnValue, _paT[index]);
                }
            }
        } // end of lock scope

        if (SUCCEEDED(hr))
        {
            // Ensure that unsafe operations (e.g.: GIT localization) happen outside of lock scope for types which require it.
            // This call may be a NOP for certain types.
            hr = T_TempTraits::ResolveDemand(&temp, returnValue);
        }

        return hr;
    }

    // IVector::get_Size
    template <
        class T,
        class EqualityPredicate,
        class LifetimeTraits,
        class Options
    >
    IFACEMETHODIMP Vector<T,EqualityPredicate,LifetimeTraits,Options>
        ::get_Size(_Out_ unsigned *returnValue)
    {
        HRESULT hr = S_OK;
        *returnValue = 0;

        auto acquired = Options::LockPolicy::Read(_comLock, &hr);

        if (SUCCEEDED(hr))
        {
            *returnValue = _uSize;
        }

        return hr;
    }

    // IVector::GetView
    template <
        class T,
        class EqualityPredicate,
        class LifetimeTraits,
        class Options
    >
    IFACEMETHODIMP Vector<T,EqualityPredicate,LifetimeTraits,Options>
        ::GetView(_Outptr_result_maybenull_ wfc::IVectorView<T> **returnValue)
    {
        HRESULT hr = S_OK;
        *returnValue = nullptr;

        Microsoft::WRL::ComPtr<SimpleVectorView<T, Vector, LifetimeTraits, XWinRT::IntVersionTag>> sp;
        hr = SimpleVectorView<T, Vector, LifetimeTraits, XWinRT::IntVersionTag>::Make(this, &sp);

        if (SUCCEEDED(hr))
        {
            *returnValue = static_cast<Microsoft::WRL::ComPtr<wfc::IVectorView<T>>>(sp).Detach();
        }

        return hr;

    }

    // Helper for ::IndexOf which searches an array of storage values.
    template<
        class T,
        class EqualityPredicate,
        class LifetimeTraits,
        class Options
    >
    HRESULT Vector<T,EqualityPredicate,LifetimeTraits,Options>
        ::IndexOfInternal(_In_ T_storage *pStorage, _In_ unsigned uSize, _In_opt_ T_abi value, _Inout_ unsigned *index, _Inout_ boolean *found)
    {
        using XWinRT::detail::AbiReference;

        bool fEquals = false;

        HRESULT hr = S_OK;
        for (unsigned uIndex = 0; uIndex < uSize && SUCCEEDED(hr); ++uIndex)
        {
            AbiReference<T_abi, T_storage, LifetimeTraits> abiElement(pStorage[uIndex], &hr);
            if (SUCCEEDED(hr))
            {
                hr = _fnEquals(value, abiElement.get(), &fEquals);
            }

            if (SUCCEEDED(hr) && fEquals)
            {
                *found = true;
                *index = uIndex;
                break;
            }
        }

        return hr;
    }

    // IVector::IndexOf
    template <
        class T,
        class EqualityPredicate,
        class LifetimeTraits,
        class Options
    >
    IFACEMETHODIMP Vector<T,EqualityPredicate,LifetimeTraits,Options>
        ::IndexOf(_In_opt_ T_abi value, _Out_ unsigned *index, _Out_ boolean *found)
    {
        *index = 0;
        *found = false;

        T_storage *pSnapShot = nullptr;
        unsigned snapshotted = 0;

        HRESULT hr;
        { // lock duration
            auto acquired = Options::LockPolicy::Read(_comLock, &hr);
            if (SUCCEEDED(hr))
            {
                // If there are values backed by GIT cookies, we cannot localize the cookies under the lock in order to perform the comparisons
                // here as doing so can lead to deadlock.  Instead, we must either snapshot the list and do the comparison outside the scope of the
                // lock or play drop/try/acquire/check version games.
                //
                // This takes the tack of snapshotting the entire vector.  Note that we cannot snapshot only GIT backed elements as doing so would change
                // the semantic of returning the first match.
                if (_uDemandCopy == 0)
                {
                    hr = IndexOfInternal(_paT, _uSize, value, index, found);
                }
                else
                {
                    pSnapShot = new(std::nothrow) T_storage[_uSize];
                    if (pSnapShot == nullptr)
                    {
                        hr = E_OUTOFMEMORY;
                    }
                    else
                    {
                        for (unsigned uIndex = 0; uIndex < _uSize && SUCCEEDED(hr); ++uIndex)
                        {
                            hr = LifetimeTraits::Construct(&pSnapShot[uIndex], _paT[uIndex]);
                            if (SUCCEEDED(hr))
                            {
                                snapshotted++;
                            }
                            else
                            {
                                break;
                            }
                        }
                    }
                }
            }
        } // end of lock scope

        if (SUCCEEDED(hr) && snapshotted > 0)
        {
            hr = IndexOfInternal(pSnapShot, snapshotted, value, index, found);
        }

        if (snapshotted > 0)
        {
            for (unsigned uIndex = 0; uIndex < snapshotted; ++uIndex)
            {
                LifetimeTraits::Destroy(&pSnapShot[uIndex]);
            }
        }

        delete[] pSnapShot;

        return hr;
    }

    // write methods

    // IVector::SetAt
    template <
        class T,
        class EqualityPredicate,
        class LifetimeTraits,
        class Options
    >
    IFACEMETHODIMP Vector<T,EqualityPredicate,LifetimeTraits,Options>
        ::SetAt(_In_ unsigned index, _In_opt_ T_abi value)
    {
        using XWinRT::AutoValue;

        HRESULT hr = S_OK;
        typename Options::ReentrancyGuard guard;

        {
            AutoValue<T_storage, LifetimeTraits> sOldValue;
            AutoValue<T_storage, LifetimeTraits> sNewValue(value, &hr);

            if (SUCCEEDED(hr))
            { // lock duration
                auto acquired = Options::LockPolicy::Write(_comLock, &hr);
                if (SUCCEEDED(hr))
                {
                    if (index >= _uSize)
                    {
                        hr = XWinRT::detail::FailsWith(E_BOUNDS);
                    }

                    if (SUCCEEDED(hr))
                    {
                        guard = typename Options::ReentrancyGuard(_listeners, &_reentrancyGuard);
                        hr = guard.hr();
                        if (SUCCEEDED(hr))
                        {
                            T_storage tTmp;

                            errno_t err = 0;
                            err = memmove_s(
                                    &tTmp,          // void* dest
                                    sizeof(tTmp),   // size_t numberOfElements (dest buffer size)
                                    &_paT[index],   // void* src
                                    sizeof(T_storage));     // count (number of characters to copy)
                            if(err)
                            {
                                // Failure in memmove is unexpected and represents a
                                // programming error in this component
                                ASSERT(err == 0);
                                hr = XWinRT::detail::FailsWith(E_UNEXPECTED);
                            }

                            if (SUCCEEDED(hr))
                            {
                                _paT[index] = sNewValue.get();
                                sNewValue.Abandon();
                                _iVersion++;
                                sOldValue.Attach(tTmp);

                                if (T_TempTraits::RequiresTempCopy(_paT[index]))
                                {
                                    _uDemandCopy++;
                                }
                                if (T_TempTraits::RequiresTempCopy(tTmp))
                                {
                                    _uDemandCopy--;
                                }
                            }
                        }
                    }
                }
            } // end of lock scope...  old values destruct in next scope
        }

        if ( SUCCEEDED(hr) )
        {
            hr = Options::RaiseEvent(guard, _evtVectorChanged, this, wfc::CollectionChange_ItemChanged, index);
        }

        return hr;
    }

    // InsertAtInternal
    template <
        class T,
        class EqualityPredicate,
        class LifetimeTraits,
        class Options
    >
    HRESULT Vector<T,EqualityPredicate,LifetimeTraits,Options>
        ::InsertAtInternal(_In_ unsigned index, _In_opt_ T_abi value, _In_ bool fLast)
    {
        using XWinRT::AutoValue;

        HRESULT hr = S_OK;
        typename Options::ReentrancyGuard guard;

        AutoValue<T_storage, LifetimeTraits> sNewValue(value, &hr);

        if (SUCCEEDED(hr))
        { // lock duration
            auto acquired = Options::LockPolicy::Write(_comLock, &hr);
            if (SUCCEEDED(hr))
            {
                if (fLast)
                {
                    index = _uSize;
                }

                if (index > _uSize)
                {
                    hr = XWinRT::detail::FailsWith(E_BOUNDS);
                }
                else if (_uSize >= s_cuMaxSize)
                {
                    hr = E_OUTOFMEMORY;
                }

                if (SUCCEEDED(hr))
                {
                    guard = typename Options::ReentrancyGuard(_listeners, &_reentrancyGuard);
                    hr = guard.hr();

                    // Allocate if necessary
                    if (SUCCEEDED(hr))
                    {
                        if(_uSize + 1 > _uCapacity)
                        {
                            unsigned newSize = XWinRT::detail::wcp_max(_uCapacity + 1, _uCapacity + _uCapacity / 2);
                            hr = ResizeStorage(newSize);
                        }
                    }

                    // Move things out of the way (if necessary)
                    if (SUCCEEDED(hr) && (index < _uSize))
                    {
                        errno_t err = 0;
                        err = memmove_s(
                                &_paT[index + 1],                        // void* dest
                                (_uCapacity - (index + 1)) * sizeof(T_storage),// size_t numberOfElements (dest buffer size)
                                &_paT[index],                           // void* src
                                (_uSize - index) * sizeof(T_storage));   // count (number of characters to copy)
                        if(err)
                        {
                            // Failure in memmove is unexpected and represents a
                            // programming error in this component
                            ASSERT(err == 0);
                            hr = XWinRT::detail::FailsWith(E_UNEXPECTED);
                        }
                    }

                    // Copy in the new element
                    if (SUCCEEDED(hr))
                    {
                        _paT[index] = sNewValue.get();
                        sNewValue.Abandon();
                        _uSize++;
                        _iVersion++;

                        if (T_TempTraits::RequiresTempCopy(_paT[index]))
                        {
                            _uDemandCopy++;
                        }
                    }
                }
            }
        }

        if ( SUCCEEDED(hr) )
        {
            hr = Options::RaiseEvent(guard, _evtVectorChanged, this, wfc::CollectionChange_ItemInserted, index);
        }
        return hr;
    }

    // IVector::InsertAt
    template <
        class T,
        class EqualityPredicate,
        class LifetimeTraits,
        class Options
    >
    IFACEMETHODIMP Vector<T,EqualityPredicate,LifetimeTraits,Options>
        ::InsertAt(_In_ unsigned index, _In_opt_ T_abi value)
    {
        return InsertAtInternal(index, value, false);
    }

    // RemoveAtInternal
    template <
        class T,
        class EqualityPredicate,
        class LifetimeTraits,
        class Options
    >
    HRESULT Vector<T,EqualityPredicate,LifetimeTraits,Options>
        ::RemoveAtInternal(_In_ unsigned index, _In_ bool fLast)
    {
        using XWinRT::AutoValue;

        HRESULT hr = S_OK;
        typename Options::ReentrancyGuard guard;

        {
            AutoValue<T_storage, LifetimeTraits> sOldValue;

            { // lock duration
                auto acquired = Options::LockPolicy::Write(_comLock, &hr);
                if (SUCCEEDED(hr))
                {
                    if (fLast)
                    {
                        // No need to check for bounds. If _uSize == 0 then the argument will
                        // wrap around and then index below will be bigger than _uSize
                        // and we will return E_BOUNDS as requred.
                        index = _uSize-1;
                    }

                    if (index >= _uSize)
                    {
                        hr = XWinRT::detail::FailsWith(E_BOUNDS);
                    }

                    if (SUCCEEDED(hr))
                    {
                        guard = typename Options::ReentrancyGuard(_listeners, &_reentrancyGuard);
                        hr = guard.hr();
                        if (SUCCEEDED(hr))
                        {
                            // destroy the element and move everything down
                            if (T_TempTraits::RequiresTempCopy(_paT[index]))
                            {
                                _uDemandCopy--;
                            }
                            sOldValue.Attach(_paT[index]);

                            if (index < _uSize - 1)
                            {
                                errno_t err = 0;
                                err = memmove_s(
                                    &_paT[index],                              // void* dest
                                    (_uSize - index - 1) * sizeof(T_storage),  // size_t numberOfElements (dest buffer size)
                                    &_paT[index + 1],                         // void* src
                                    (_uSize - index - 1) * sizeof(T_storage)); // count (number of characters to copy)

                                if(err)
                                {
                                    // Failure in memmove is unexpected and represents a
                                    // programming error in this component
                                    ASSERT(err == 0);
                                    hr = XWinRT::detail::FailsWith(E_UNEXPECTED);
                                }
                            }
                         }

                         // shrink the storage if necessary
                         if (SUCCEEDED(hr))
                         {
                            _iVersion++;
                            _uSize--;
                            if (_uSize < _uCapacity / 3)
                            {
                                // When we resize the storage, it should get resized to 1.5
                                // times the current capacity. We resize down we resize by the
                                // inverse of 1.5 or 2/3.
                                unsigned newSize = XWinRT::detail::wcp_min(_uCapacity - 1, _uCapacity - _uCapacity / 3);
                                hr = ResizeStorage(newSize);
                            }
                         }
                    }
                }
            } // end of lock scope...  old values destruct in next scope.
        }

        if ( SUCCEEDED(hr) )
        {
            hr = Options::RaiseEvent(guard, _evtVectorChanged, this, wfc::CollectionChange_ItemRemoved, index);
        }
        return hr;
    }

    // IVector::RemoveAt
    template <
        class T,
        class EqualityPredicate,
        class LifetimeTraits,
        class Options
    >
    IFACEMETHODIMP Vector<T,EqualityPredicate,LifetimeTraits,Options>
        ::RemoveAt(_In_ unsigned index)
    {
        return RemoveAtInternal(index, false);
    }

    // IVector::Append
    template <
        class T,
        class EqualityPredicate,
        class LifetimeTraits,
        class Options
    >
    IFACEMETHODIMP Vector<T,EqualityPredicate,LifetimeTraits,Options>
        ::Append(_In_opt_ T_abi value)
    {
        return InsertAtInternal(0, value, true);
    }

    // IVector::RemoveAtEnd
    template <
        class T,
        class EqualityPredicate,
        class LifetimeTraits,
        class Options
    >
    IFACEMETHODIMP Vector<T,EqualityPredicate,LifetimeTraits,Options>
        ::RemoveAtEnd()
    {
        return RemoveAtInternal(0, true);
    }

    // IVector::Clear
    template <
        class T,
        class EqualityPredicate,
        class LifetimeTraits,
        class Options
    >
    IFACEMETHODIMP Vector<T,EqualityPredicate,LifetimeTraits,Options>
        ::Clear()
    {
        HRESULT hr;
        bool needNotification = false;
        typename Options::ReentrancyGuard guard;

        T_storage *paT = nullptr;
        unsigned uSize = 0;

        { // lock duration
            auto acquired = Options::LockPolicy::Write(_comLock, &hr);
            if (SUCCEEDED(hr))
            {
                guard = typename Options::ReentrancyGuard(_listeners, &_reentrancyGuard);
                hr = guard.hr();

                if (SUCCEEDED(hr))
                {
                    if ( _uSize > 0 )
                    {
                        needNotification = true;
                    }

                    _EraseAll(&paT, &uSize);
                }
            }
        }

        // Ensure that the actual release of objects in the vector happens outside the scope of the lock.
        _Free(paT, uSize);

        if ( SUCCEEDED(hr) && needNotification )
        {
            hr = Options::RaiseEvent(guard, _evtVectorChanged, this, wfc::CollectionChange_Reset, 0);
        }
        return hr;
    }

    //IVector::GetMany
    template <
        class T,
        class EqualityPredicate,
        class LifetimeTraits,
        class Options
    >
    IFACEMETHODIMP Vector<T,EqualityPredicate,LifetimeTraits,Options>
        ::GetMany(_In_  unsigned startIndex, _In_ unsigned capacity, _Out_writes_to_(capacity,*actual) T_abi *value, _Out_ unsigned *actual)
    {
        unsigned index = 0;
        unsigned copied = 0;
        unsigned temped = 0;
        unsigned sizeSnap = 0;
        HRESULT hr = S_OK;

        // Initialize the out parameters
        for(index =0; index < capacity; index++)
        {
            LifetimeTraits::Construct(&value[index]);
        }
        *actual = 0;

        typename T_TempTraits::type *pTemps = nullptr;

        // Acquire the read lock
        { // lock duration
            auto acquired = Options::LockPolicy::Read(_comLock, &hr);

            if (_uDemandCopy != 0)
            {
                pTemps = new(std::nothrow) typename T_TempTraits::type[capacity];
                if (pTemps == nullptr)
                {
                    hr = XWinRT::detail::FailsWith(E_OUTOFMEMORY);
                }
            }

            if (SUCCEEDED(hr))
            {
                sizeSnap = _uSize;

                // do the boundschecing
                if (startIndex > sizeSnap)
                {
                    // If we are more than one past the end, then we return E_BOUNDS;
                    hr = XWinRT::detail::FailsWith(E_BOUNDS);
                }
            }

            if (SUCCEEDED(hr))
            {
                // we are gaurenteed to be one past the end or less.  If we are one past the end
                // we won't enter the for loop, and we'll get S_OK but nothing returned.
                // If we are at the end or earlier, we'll actually get something in the output

                // If we know a-priori that there are no objects which require a temp (e.g.: GIT cookies) stored within
                // the collection, avoid the cost of the memory allocation.
                if (_uDemandCopy != 0)
                {
                    for (index = 0; (index < capacity) && (index + startIndex < sizeSnap); index++)
                    {
                        // Depending on the temp traits, this may make a temporary copy into pTempts[index] in order to perform
                        // certain operations outside of lock scope during a ResolveDemand call.
                        hr = T_TempTraits::CopyOnDemand(&pTemps[index], &value[index], _paT[index + startIndex]);
                        if (SUCCEEDED(hr))
                        {
                            temped++;
                        }
                        else
                        {
                            break;
                        }
                    }
                }
                else
                {
                    for (index = 0; (index < capacity) && (index + startIndex < sizeSnap) && SUCCEEDED(hr); index++)
                    {
                        hr = LifetimeTraits::Construct(&value[index], _paT[index + startIndex]);
                        if (SUCCEEDED(hr))
                        {
                            copied++;
                        }
                        else
                        {
                            break;
                        }
                    }
                }
            }
        }

        if (SUCCEEDED(hr) && temped > 0)
        {
            for (index = 0; index < capacity && (index + startIndex < sizeSnap); index++)
            {
                // Ensure that unsafe operations (e.g.: GIT localization) happen outside of lock scope for types which require it.
                // This call may be a NOP for certain types.
                hr = T_TempTraits::ResolveDemand(&pTemps[index], &value[index]);
                if (SUCCEEDED(hr))
                {
                    copied++;
                }
                else
                {
                    break;
                }
            }
        }

        // copy the number of elements into the output count
        if (SUCCEEDED(hr))
        {
            *actual = copied;
        }

        // on falure, cleanup everything that we might have copied into the output array
        if (FAILED(hr))
        {
            for(index = 0; index < copied; index++)
            {
                LifetimeTraits::Destroy(&value[index]);
            }

            // Anything which we successfully copied was already dealt with in the ::ResolveDemand call above.  Hence, only the elements
            // from [copied, temped) need to be destroyed within the temp array.
            for(index = copied; index < temped; index++)
            {
                T_TempTraits::DestroyTemp(&pTemps[index]);
            }
        }

        delete[] pTemps;

        return hr;
    }

    //IVector::ReplaceAll
    template <
        class T,
        class EqualityPredicate,
        class LifetimeTraits,
        class Options
    >
    IFACEMETHODIMP Vector<T,EqualityPredicate,LifetimeTraits,Options>
        :: ReplaceAll(_In_ unsigned count, _In_reads_(count) T_abi *value)
    {
        HRESULT hr = S_OK;
        typename Options::ReentrancyGuard guard;

        T_storage* paT = nullptr;
        unsigned size = 0;
        unsigned capacity = 0;
        unsigned demandCopy = 0;

        // Allocate new array, exactly the required size
        // note we do not allocate if the requested size is zero.
        if(SUCCEEDED(hr) && (count > 0))
        {
            paT = static_cast<T_storage*>( ::malloc(count * sizeof(T_storage)) );
            if (nullptr == paT)
            {
                hr = E_OUTOFMEMORY;
            }
            else
            {
                size = 0;
                capacity = count;
            }
        }

        // fill new array
        if (SUCCEEDED(hr))
        {
            for (size = 0; size < capacity; size++)
            {
                hr = LifetimeTraits::Construct(&paT[size], value[size]);
                if (FAILED(hr))
                {
                    break;
                }
                else
                {
                    if (T_TempTraits::RequiresTempCopy(paT[size]))
                    {
                        demandCopy++;
                    }
                }
            }
        }

        if (SUCCEEDED(hr))
        { // lock duration
            auto acquired = Options::LockPolicy::Write(_comLock, &hr);
            if (SUCCEEDED(hr))
            {
                guard = typename Options::ReentrancyGuard(_listeners, &_reentrancyGuard);
                hr = guard.hr();
                }

            // swap out old array
            if (SUCCEEDED(hr))
            {
                XWinRT::FakeStl::swap(_uSize, size);
                XWinRT::FakeStl::swap(_uCapacity, capacity);
                XWinRT::FakeStl::swap(_uDemandCopy, demandCopy);
                XWinRT::FakeStl::swap(_paT, paT);
                _iVersion++;
            }
        }

        // cleanup
        if (paT)
        {
            for (unsigned index = 0; index < size; index++)
            {
                LifetimeTraits::Destroy(&paT[index]);
            }
            ::free(paT);
            paT = nullptr;
        }

        if ( SUCCEEDED(hr) )
        {
            hr = Options::RaiseEvent(guard, _evtVectorChanged, this, wfc::CollectionChange_Reset, 0);
        }

        return hr;
    }

    // IObservableVector::add_VectorChanged
    template <
        class T,
        class EqualityPredicate,
        class LifetimeTraits,
        class Options
    >
    IFACEMETHODIMP Vector<T,EqualityPredicate,LifetimeTraits,Options>
        ::add_VectorChanged(_In_opt_ wfc::VectorChangedEventHandler<T>* handler, _Out_ EventRegistrationToken* token)
    {
        HRESULT hr = Options::AddEventHandler ( _evtVectorChanged, handler, token );
        if (SUCCEEDED(hr))
        {
            Options::TrackListener(_listeners);
        }
        return hr;
    }

    // IObservableVector::remove_VectorChanged
    template <
        class T,
        class EqualityPredicate,
        class LifetimeTraits,
        class Options
    >
    IFACEMETHODIMP Vector<T,EqualityPredicate,LifetimeTraits,Options>
        ::remove_VectorChanged(_In_ EventRegistrationToken token)
    {
        HRESULT hr = Options::RemoveEventHandler ( _evtVectorChanged, token );
        if (SUCCEEDED(hr))
        {
            Options::UntrackListener(_listeners);
        }
        return hr;
    }

    // ResizeStorage
    template <
        class T,
        class EqualityPredicate,
        class LifetimeTraits,
        class Options
    >
    HRESULT Vector<T,EqualityPredicate,LifetimeTraits,Options>
        ::ResizeStorage(_In_ unsigned newSize)
    {
        HRESULT hr = S_OK;
        T_storage *pAllocatedMemory;
        if (nullptr == _paT)
        {
            pAllocatedMemory = static_cast<T_storage*>( ::malloc(newSize * sizeof(T_storage)) );
        }
        else
        {
            pAllocatedMemory = static_cast<T_storage*>( ::realloc(_paT, newSize * sizeof (T_storage)) );
        }

        if (!pAllocatedMemory)
        {
            hr = E_OUTOFMEMORY;
        }

        if (SUCCEEDED(hr))
        {
            _paT = pAllocatedMemory;
            _uCapacity = newSize;
        }

        return hr;
    }

    struct NoVersionTag
    {
        template <class Container>
        NoVersionTag(Container*)
        {
        }
        bool operator==(const NoVersionTag&) const
        {
            return true;
        }
    };

    // Free-threadedness can only be set if the given vector is free-threaded
    template <
        class T,
        class TVector = wfc::IVector<T>,
        class LifetimeTraits = DefaultLifetimeTraits<T>,
        class VersionTag = NoVersionTag
        >
    class SimpleVectorView WrlSealed : public Microsoft::WRL::RuntimeClass<wfc::IVectorView<T>, wfc::IIterable<T>>
    {
        typedef typename detail::GetAbiType<typename wfc::IVectorView<T>::T_complex>::type       T_abi;

        using RuntimeClassT = typename Microsoft::WRL::RuntimeClass<wfc::IVectorView<T>, wfc::IIterable<T>>::RuntimeClassT;

        static const wchar_t* z_get_rc_name()
        {
            // use the MIDLRT generated name string, which substitutes
            //   the proper metadata-name-representation of T
            return wfc::IVectorView<T>::z_get_rc_name_impl();
        }
        InspectableClass(z_get_rc_name(), (::BaseTrust))

    private:
        // PURPOSE
        //  constructor must be public for WRL to be able to call it,
        //  but should not generally be callable by clients.
        struct permission{};

    public:
        SimpleVectorView(_In_ TVector* pVector, permission);

    public:
        static HRESULT Make(_In_ TVector* pVector, _COM_Outptr_ SimpleVectorView** ppView)
        {
            *ppView = nullptr;
            HRESULT hr = S_OK;

            Microsoft::WRL::ComPtr<SimpleVectorView> sp = Microsoft::WRL::Make<SimpleVectorView>(pVector, permission());

            if (!sp)
            {
                hr = E_OUTOFMEMORY;
            }
            if (SUCCEEDED(hr))
            {
                *ppView = sp.Detach();
            }
            return hr;
        }

        // IIterable
        IFACEMETHODIMP First(_Outptr_result_maybenull_ wfc::IIterator<T> **first) override;

        // IVector View
        IFACEMETHODIMP GetAt(_In_ unsigned index, _Out_ T_abi *item) override;
        IFACEMETHODIMP get_Size(_Out_ unsigned *size) override;
        IFACEMETHODIMP IndexOf(_In_opt_ T_abi value, _Out_ unsigned *index, _Out_ boolean *found) override;
        IFACEMETHODIMP GetMany(_In_  unsigned startIndex, _In_ unsigned capacity, _Out_writes_to_(capacity,*actual) T_abi *value, _Out_ unsigned *actual) override;

    private:
        template<class T, class U>
        HRESULT _DoWithInvalidationCheck(const T &doFunctor, const U &undoFunctor)
        {
            return _checker.Do(_spVector.Get(), doFunctor, undoFunctor);
        }

        SimpleVectorView(const SimpleVectorView&); // = delete
        void operator=(const SimpleVectorView&); // = delete
        Microsoft::WRL::ComPtr<TVector> _spVector;
        // used to protect invalidation
        XWinRT::detail::InvalidationChecker<TVector, VersionTag> _checker;
    };

    template <
        class T,
        class TVector,
        class LifetimeTraits,
        class VersionTag
    >
    SimpleVectorView <T, TVector, LifetimeTraits, VersionTag>
        ::SimpleVectorView(_In_ TVector* pVector, permission) :
            _spVector(pVector),
            _checker(pVector)
    {

    }

    // IIterable::First
    template <
        class T,
        class TVector,
        class LifetimeTraits,
        class VersionTag
    >

    IFACEMETHODIMP SimpleVectorView <T, TVector, LifetimeTraits, VersionTag>
        ::First(_Outptr_result_maybenull_ wfc::IIterator<T> **first)
    {
        *first = nullptr;

        return _DoWithInvalidationCheck(
            [&]() -> HRESULT {
                HRESULT hr = S_OK;
                {
                    Microsoft::WRL::ComPtr<wfc::IIterable<T>> spIterable;

                    hr = _spVector.As(&spIterable);
                    ASSERTSUCCEEDED(hr);

                    if (SUCCEEDED(hr))
                    {
                        hr = spIterable->First(first);
                    }
                }
                return hr;
            },
            [&]() {
                    wfc::IIterator<T> *firstTemp = *first;
                    *first = nullptr;
                    if (firstTemp)
                    {
                        firstTemp->Release();
                    }
            });
    }

    // IVectorView::GetAt
    template <
        class T,
        class TVector,
        class LifetimeTraits,
        class VersionTag
    >
    IFACEMETHODIMP SimpleVectorView <T, TVector, LifetimeTraits, VersionTag>
        ::GetAt(_In_ unsigned index, _Out_ T_abi *item)
    {
        LifetimeTraits::Construct(item);
        return _DoWithInvalidationCheck(
            [&]() -> HRESULT {
                return _spVector->GetAt(index,item);
            },
            [&]() {
                LifetimeTraits::Destroy(item);
                LifetimeTraits::Construct(item);
            });
    }

    // IVectorView::get_Size
    template <
        class T,
        class TVector,
        class LifetimeTraits,
        class VersionTag
    >
    IFACEMETHODIMP SimpleVectorView <T, TVector, LifetimeTraits, VersionTag>
        ::get_Size(_Out_ unsigned *size)
    {
        *size = 0;
        return _DoWithInvalidationCheck(
            [&]() -> HRESULT {
                return _spVector->get_Size(size);
            },
            [&]() {
                *size = 0;
            });
    }

    // IVectorView::IndexOf
    template <
        class T,
        class TVector,
        class LifetimeTraits,
        class VersionTag
    >
    IFACEMETHODIMP SimpleVectorView <T, TVector, LifetimeTraits, VersionTag>
        ::IndexOf(_In_opt_ T_abi value, _Out_ unsigned *index, _Out_ boolean *found)
    {
        *index = 0;
        *found = false;
        return _DoWithInvalidationCheck(
            [&]() -> HRESULT {
                return _spVector->IndexOf(value, index, found);
            },
            [&]() {
                *index = 0;
                *found = false;
            });
    }

    // IVectorView::GetMany
    template <
        class T,
        class TVector,
        class LifetimeTraits,
        class VersionTag
    >
    IFACEMETHODIMP SimpleVectorView <T, TVector, LifetimeTraits, VersionTag>
        ::GetMany(_In_  unsigned startIndex, _In_ unsigned capacity, _Out_writes_to_(capacity,*actual) T_abi *value, _Out_ unsigned *actual)
    {
        // Initialize the out parameters
        for(unsigned index = 0; index < capacity; index++)
        {
            LifetimeTraits::Construct(&value[index]);
        }
        *actual = 0;

        return _DoWithInvalidationCheck(
            [&]() -> HRESULT {
                return _spVector->GetMany(startIndex, capacity, value, actual);
            },
            [&]() {
                for (unsigned index = 0; index < *actual; index++)
                {
                    LifetimeTraits::Destroy(&value[index]);
                    LifetimeTraits::Construct(&value[index]);
                }
                *actual = 0;
            });
    }

    // Free-threadedness can only be set if the given vector is free threaded.
    template <
        class T,
        class TVector = wfc::IVector<T>,
        class LifetimeTraits = DefaultLifetimeTraits<T>,
        class VersionTag = NoVersionTag
        >
    class SimpleVectorIterator WrlSealed : public
        Microsoft::WRL::RuntimeClass< wfc::IIterator<T>>
    {
        using RuntimeClassT = typename Microsoft::WRL::RuntimeClass< wfc::IIterator<T>>::RuntimeClassT;
        typedef typename detail::GetAbiType<typename wfc::IIterator<T>::T_complex>::type       T_abi;

        static const wchar_t* z_get_rc_name()
        {
            // use the MIDLRT generated name string, which substitutes
            //   the proper metadata-name-representation of T
            return wfc::IIterator<T>::z_get_rc_name_impl();
        }
        InspectableClass(z_get_rc_name(), (::BaseTrust))

    private:
        // PURPOSE
        //  constructor must be public for WRL to be able to call it,
        //  but should not generally be callable by clients.
        struct permission{};

    public:
        SimpleVectorIterator(_In_ TVector* pVector, permission);

    private:
        HRESULT Initialize();

    public:
        static HRESULT Make(_In_ TVector* pOriginal, _COM_Outptr_ SimpleVectorIterator** ppIter)
        {
            *ppIter = nullptr;
            HRESULT hr = S_OK;

            Microsoft::WRL::ComPtr<SimpleVectorIterator> sp = Microsoft::WRL::Make<SimpleVectorIterator>(pOriginal, permission());

            if (!sp)
            {
                hr = E_OUTOFMEMORY;
            }
            if (SUCCEEDED(hr))
            {
                hr = sp->Initialize();
            }
            if (SUCCEEDED(hr))
            {
                *ppIter = sp.Detach();
            }
            return hr;
        }

        // IIterator
        IFACEMETHODIMP get_Current(_Out_ T_abi *current) override;
        IFACEMETHODIMP get_HasCurrent(_Out_ boolean *hasCurrent) override;
        IFACEMETHODIMP MoveNext(_Out_ boolean *hasCurrent) override;
        IFACEMETHODIMP GetMany(_In_ unsigned capacity, _Out_writes_to_(capacity,*actual) T_abi *value, _Out_ unsigned *actual) override;

    private:

        template<class T, class U>
        HRESULT _DoWithInvalidationCheck(const T &doFunctor, const U &undoFunctor)
        {
            return _checker.Do(_spVector.Get(), doFunctor, undoFunctor);
        }

        SimpleVectorIterator(const SimpleVectorIterator&); // = delete
        void operator=(const SimpleVectorIterator&); // = delete
        Microsoft::WRL::ComPtr<TVector> _spVector;
        unsigned _uIndex;
        unsigned _uSize;
        XWinRT::detail::InvalidationChecker<TVector, VersionTag> _checker;
        XWinRT::detail::AtomicUpdater _atomicUpdater;
    };

    template <
        class T,
        class TVector,
        class LifetimeTraits,
        class VersionTag
        >
    SimpleVectorIterator<T, TVector, LifetimeTraits, VersionTag>
        ::SimpleVectorIterator(_In_ TVector* pVector, permission) :
        _spVector(pVector),
        _checker(pVector),
        _uIndex(0),
        _uSize(0)
    {

    }

    template <
        class T,
        class TVector,
        class LifetimeTraits,
        class VersionTag
        >
    HRESULT SimpleVectorIterator<T, TVector, LifetimeTraits, VersionTag>
        :: Initialize()
    {
        return _spVector->get_Size(&_uSize);
    }

    // IIterator::get_Current
    template <
        class T,
        class TVector,
        class LifetimeTraits,
        class VersionTag
        >
    IFACEMETHODIMP SimpleVectorIterator<T, TVector, LifetimeTraits, VersionTag>
        :: get_Current(_Out_ T_abi *current)
    {
        LifetimeTraits::Construct(current);

        return _DoWithInvalidationCheck(
            [&]() -> HRESULT {
                return _spVector->GetAt(_uIndex, current);
            },
            [&]() {
                LifetimeTraits::Destroy(current);
                LifetimeTraits::Construct(current);
            });
    }

    // IIterator::get_HasCurrent
    template <
        class T,
        class TVector,
        class LifetimeTraits,
        class VersionTag
        >
    IFACEMETHODIMP SimpleVectorIterator<T, TVector, LifetimeTraits, VersionTag>
        :: get_HasCurrent(_Out_ boolean *hasCurrent)
    {
        *hasCurrent = false;

        return _DoWithInvalidationCheck(
            [&]() -> HRESULT
            {
                *hasCurrent = (_uIndex < _uSize);
                return S_OK;
            },
            [&](){
                *hasCurrent = false;
            });
    }

    // IIterator::MoveNext
    template <
        class T,
        class TVector,
        class LifetimeTraits,
        class VersionTag
        >
    IFACEMETHODIMP SimpleVectorIterator<T, TVector, LifetimeTraits, VersionTag>
        ::MoveNext(_Out_ boolean *hasCurrent)
    {
        *hasCurrent = false;

        auto undoFn = [&]()
        {
            *hasCurrent = false;
        };

        return _DoWithInvalidationCheck(
            [&]() -> HRESULT
            {
                auto pThis = this; // compiler quirk
                return _atomicUpdater.DoUpdate(
                    &_uIndex,
                    [&](unsigned uIndex, unsigned *puIndexNew) -> HRESULT
                    {
                        *hasCurrent = false;
                        if (uIndex < pThis->_uSize)
                        {
                            *puIndexNew = uIndex + 1;
                            *hasCurrent = (*puIndexNew < pThis->_uSize);
                            return S_OK;
                        }

                        RoOriginateError(E_BOUNDS, nullptr);
                        return E_BOUNDS;
                    },
                    undoFn);
            },
            undoFn);
    }

    // IIterator::GetMany
    template <
        class T,
        class TVector,
        class LifetimeTraits,
        class VersionTag
        >
    IFACEMETHODIMP SimpleVectorIterator<T, TVector, LifetimeTraits, VersionTag>
        ::GetMany(_In_ unsigned capacity, _Out_writes_to_(capacity,*actual) T_abi *value, _Out_ unsigned *actual)
    {
        *actual = 0;
        for (unsigned index = 0; index < capacity; index++)
        {
            LifetimeTraits::Construct(&value[index]);
        }

        auto undoFn = [&]()
        {
            // Since we have fetched elements, we must clear them before proceeding.
            for (unsigned i = 0; i < *actual; ++i)
            {
                LifetimeTraits::Destroy(&value[i]);
                LifetimeTraits::Construct(&value[i]);
            }
            *actual = 0;
        };

        return _DoWithInvalidationCheck(
            [&]() -> HRESULT {

                auto pThis = this;

                // In order to maintain semantics of not understanding the internal implementation details of the vector while
                // being lock free, this is going to take a tack of fetch before increment.  This means that if a concurrent
                // MoveNext/GetMany happens, we need to roll back.
                //
                // Given that multithreaded usage of a single mutable iterator is a bad pattern, this should not be a problem.

                return _atomicUpdater.DoUpdate(
                    &_uIndex,
                    [&](unsigned uIndex, unsigned *puIndexNew) -> HRESULT
                    {
                        HRESULT hr = pThis->_spVector->GetMany(uIndex, capacity, value, actual);
                        if (SUCCEEDED(hr))
                        {
                            *puIndexNew = uIndex + *actual;
                        }
                        return hr;
                    },
                    undoFn);
            },
            undoFn);
    }


}}}} // ::Windows::Foundation::Collections::Internal

namespace Windows { namespace Foundation { namespace Collections { namespace Internal {

    template <class K, class KeyLifetimeTraits>
    class MapChangedEventArgs WrlSealed : public Microsoft::WRL::RuntimeClass<wfc::IMapChangedEventArgs<K>>
    {
    private:
        typedef typename detail::GetAbiType<typename wfc::IMapChangedEventArgs<K>::K_complex>::type K_abi;

        static const wchar_t* z_get_rc_name()
        {
            // use the MIDLRT generated name string, which substitutes
            //   the proper metadata-name-representation of T
            return wfc::IMapChangedEventArgs<K>::z_get_rc_name_impl();
        }
        InspectableClass(z_get_rc_name(), (::BaseTrust))

        wfc::CollectionChange _change;
        K_abi _key;
    public:
        MapChangedEventArgs(wfc::CollectionChange change) :
            _change(change)
        {

        }
        ~MapChangedEventArgs()
        {
            KeyLifetimeTraits::Destroy(&_key);
        }
        HRESULT Initialize(K_abi& key)
        {
            return KeyLifetimeTraits::Construct(&_key, key);
        }
        STDMETHODIMP get_CollectionChange ( _Out_ wfc::CollectionChange* value )
        {
            *value = _change;
            return S_OK;
        }
        STDMETHODIMP get_Key (_Out_ K_abi* value)
        {
            return KeyLifetimeTraits::Construct ( value, _key );
        }
    };

    template <class K, class V, class KeyLifetimeTraits, bool supportsObservable, bool propagateError = false>
    struct HashMapOptions
    {
        static const wchar_t* z_get_rc_name_impl() { return wfc::IMap<K, V>::z_get_rc_name_impl(); }
        typedef struct {} EventType;
        typedef struct {} EventTrackingType;
        static const bool hasObservable = false;
        static HRESULT RaiseEvent(...) { return S_OK; };
        static HRESULT AddEventHandler(...) { return S_OK; };
        static HRESULT RemoveEventHandler(...) { return S_OK; };
        static void InitializeListenerTracking(...) { }
        static void TrackListener(...) { }
        static void UntrackListener(...) { }
        static void HasTrackedListener(...) { }

        typedef XWinRT::detail::ReentrancyGuard<false> ReentrancyGuard;
        typedef XWinRT::SerializingLockPolicy LockPolicy;
    };

    template <class K, class V, class KeyLifetimeTraits, bool propagateError>
    struct HashMapOptions<K, V, KeyLifetimeTraits, true, propagateError>
    {
        static const bool hasObservable = true;

        static const wchar_t* z_get_rc_name_impl() { return wfc::IObservableMap<K, V>::z_get_rc_name_impl(); }
        typedef typename detail::GetAbiType<typename wfc::IObservableMap<K,V>::K_complex>::type K_abi;

        typedef Microsoft::WRL::InvokeModeOptions<Microsoft::WRL::FireAll> PropagateType;
        typedef Microsoft::WRL::EventSource < wfc::MapChangedEventHandler<K,V>, PropagateType > EventType;
        typedef long EventTrackingType;

        typedef XWinRT::detail::ReentrancyGuard<true> ReentrancyGuard;
        typedef XWinRT::SerializingLockPolicy LockPolicy;

        static HRESULT RaiseEvent( ReentrancyGuard& guard, EventType& e, wfc::IObservableMap<K,V>* sender, wfc::CollectionChange change, K_abi key )
        {
            HRESULT hr = S_OK;
            // At the moment, observers interfere with serializibility.  When we see no observers, we make a guarantee
            // about serialization under that lock policy.  Once we've noticed that there are no observers, we commit
            // to that decision in raising the event.  One cannot "sneak through".  Even if one comes, the event will
            // not get fired.  The only way this happens is adding an observer during a write.  In this case, we
            // serialize in the "write then add" order regardless of when the observer is placed in the actual list.
            if (guard.IsFlagged())
            {
                Microsoft::WRL::ComPtr<MapChangedEventArgs<K, KeyLifetimeTraits>> spArgs =
                    Microsoft::WRL::Make<MapChangedEventArgs<K, KeyLifetimeTraits>>(change);
                // Check if we failed to create the arguments object (OOM), and if so don't call the notification
                if ( nullptr != spArgs.Get() )
                {
                    HRESULT hr = spArgs->Initialize(key);
                    if ( SUCCEEDED(hr) )
                    {
                        // Notification result ignored
                        hr = e.InvokeAll(sender, spArgs.Get());
                    }
                }
            }

            return hr;
        };
        static HRESULT AddEventHandler( EventType& e, _In_ wfc::MapChangedEventHandler<K,V>* handler, _Out_ EventRegistrationToken* token )
        {
            return e.Add(handler, token);
        };
        static HRESULT RemoveEventHandler( EventType& e, EventRegistrationToken token )
        {
            return e.Remove(token);
        };
        static void InitializeListenerTracking(EventTrackingType& t)
        {
            t = 0;
        }
        static void TrackListener(EventTrackingType& t)
        {
            InterlockedIncrement(static_cast<volatile long *>(&t));
        }
        static void UntrackListener(EventTrackingType& t)
        {
            InterlockedDecrement(static_cast<volatile long *>(&t));
        }
        static void HasTrackedListener(EventTrackingType& t)
        {
            // No memory barrier necessary.  Used to determine whether to fire the event or not.  Such is protected
            // by its own locking.
            return (t > 0);
        }
    };



    template <class K, class V, class KeyLifetimeTraits>
    struct DefaultHashMapOptions : HashMapOptions<K, V, KeyLifetimeTraits, false, false>
    {
    };

    template <
        class K,
        class V,
        class Hasher            = DefaultHash<K>,
        class EqualityPredicate = DefaultEqualityPredicate<K>,
        class KeyLifetime       = DefaultLifetimeTraits<K>,
        class ValueLifetime     = DefaultLifetimeTraits<V>,
        class HashMapOptions    = DefaultHashMapOptions<K, V, KeyLifetime>
    >
    class HashMap : public
        detail::_if <HashMapOptions::hasObservable,
            Microsoft::WRL::RuntimeClass<wfc::IMap<K, V>, wfc::IIterable<wfc::IKeyValuePair<K, V>*>, wfc::IObservableMap<K,V>>,
            Microsoft::WRL::RuntimeClass<wfc::IMap<K, V>, wfc::IIterable<wfc::IKeyValuePair<K, V>*>>
        >::type
    {
        typedef typename detail::GetAbiType<typename wfc::IMap<K,V>::K_complex>::type                         K_abi;
        typedef typename detail::GetAbiType<typename wfc::IMap<K,V>::V_complex>::type                         V_abi;
        typedef typename detail::StorageType<K_abi>::type                                   K_storage;
        typedef typename detail::StorageType<V_abi>::type                                   V_storage;
        typedef typename detail::StorageType<V_abi>::temp_type                              V_tmp;
        typedef typename XWinRT::StorageTempTraits<V_storage, V_abi, V_tmp, ValueLifetime>  V_TempTraits;

        static const wchar_t* z_get_rc_name()
        {
            // use the MIDLRT generated name string, which substitutes
            //   the proper metadata-name-representation of K and V
            return HashMapOptions::z_get_rc_name_impl();
        }
        InspectableClass(z_get_rc_name(), (::BaseTrust))

    private:
        // PURPOSE
        //  constructor must be public for WRL to be able to call it,
        //  but should not generally be callable by clients.
        struct permission{};

    public:
        HashMap(const Hasher& fnHasher, const EqualityPredicate& fnEquals, permission);

    private:
        STDMETHODIMP Initialize();

    protected:
        ~HashMap();

    public:
        static HRESULT Make(const Hasher& fnHasher, const EqualityPredicate& fnEquals, _COM_Outptr_ HashMap** ppHashMap)
        {
            *ppHashMap = nullptr;
            HRESULT hr = S_OK;

            auto sp = Microsoft::WRL::Make<HashMap>(fnHasher, fnEquals, permission());
            if (!sp)
            {
                hr = E_OUTOFMEMORY;
            }
            if (SUCCEEDED(hr))
            {
                hr = sp->Initialize();
            }
            if (SUCCEEDED(hr))
            {
                *ppHashMap = sp.Detach();
            }
            return hr;
        }
        static HRESULT Make(_COM_Outptr_ HashMap** ppHashMap)
        {
            return Make(Hasher(), EqualityPredicate(), ppHashMap);
        }

        // placeholder impls:

        IFACEMETHOD(First)(_Outptr_result_maybenull_ wfc::IIterator<wfc::IKeyValuePair<K, V>*> **first);

        IFACEMETHOD(Lookup)(_In_opt_ K_abi key, _Out_ V_abi *value);
        /*propget*/  IFACEMETHOD(get_Size)(_Out_ unsigned int *size);
        IFACEMETHOD(HasKey)(_In_opt_ K_abi key, _Out_ boolean *found);
        IFACEMETHOD(GetView)(_Outptr_result_maybenull_ wfc::IMapView<K,V> **view);

        // write methods
        IFACEMETHOD(Insert)(_In_opt_ K_abi key, _In_opt_ V_abi value, _Out_ boolean *replaced);
        IFACEMETHOD(Remove)(_In_opt_ K_abi key);
        IFACEMETHOD(Clear)();

        // IObservableMap
        IFACEMETHODIMP add_MapChanged ( _In_ wfc::MapChangedEventHandler<K,V>* handler, _Out_ EventRegistrationToken* token );
        IFACEMETHODIMP remove_MapChanged( EventRegistrationToken token );

        Hasher& get_Hash() { return _fnHash; }
        EqualityPredicate& get_Equals(){ return _fnEquals; }

    private:

        friend XWinRT::SecureVersionTag;
        XWinRT::SecureVersionTag::Tag* GetVersion() const //-> decltype(_versionManager.GetVersion()) const
        {
            return _versionManager.GetVersion();
        }

        class View;
        class Iterator;

        friend Iterator;

        static const unsigned int s_cuMaxSize = 0x80000000-1; // 2**31-1

        HashMap(const HashMap&); // = delete
        void operator=(const HashMap&); // = delete

        // PURPOSE
        //  Maps winrt type traits to ATL-ish key/value traits.
        //  Shadows only the Hash and Compare functions
        struct KeyTraits : XWinRT::CElementTraits<K_storage>
        {
            static HRESULT Hash(
                void* pOwner,
                _In_ const K_storage& element,
                _Out_ UINT32* puHash)
            {
                using XWinRT::detail::AbiReference;

                // NOTE: The usage of this AbiReference under lock is presently safe because hash maps cannot have non-agile keys.
                HRESULT hr;
                AbiReference<K_abi, K_storage, KeyLifetime> abiElement(element, &hr);

                if (SUCCEEDED(hr))
                {
                    // We must validate the ability of the hash function to hash this particular type of key.
                    // The default hash provider cannot be used to hash non-agile interfaces if the hash map
                    // itself is agile.
                    hr = detail::HashValidator<K_abi, Hasher>::IsHashable(abiElement.get());
                    if (SUCCEEDED(hr))
                    {
                        auto& fnHash = static_cast<HashMap*>(pOwner)->get_Hash();
                        hr = fnHash(abiElement.get(), puHash);
                    }
                }

                return hr;
            }
            static UINT32 CompareElements(
                _In_ void* pOwner,
                _In_ const K_storage& element1,
                _In_ const K_storage& element2,
                _Out_ bool * pfEquals)
            {
                using XWinRT::detail::AbiReference;

                // NOTE: The usage of this AbiReference under lock is presently safe because hash maps cannot have non-agile keys.
                HRESULT hr;
                AbiReference<K_abi, K_storage, KeyLifetime> abiElement1(element1, &hr);
                if (SUCCEEDED(hr))
                {
                    AbiReference<K_abi, K_storage, KeyLifetime> abiElement2(element2, &hr);
                    if (SUCCEEDED(hr))
                    {
                        EqualityPredicate& Equals = static_cast<HashMap*>(pOwner)->get_Equals();
                        hr = Equals(abiElement1.get(), abiElement2.get(), pfEquals);
                    }
                }

                return hr;
            }
        };

        typedef typename XWinRT::XHashMap<K_storage, V_storage, KeyTraits> HashStore;
        typedef typename HashStore::CPair InnerCPair;

        void _EraseAll(HashStore *pStore);
        static void _Free(HashStore *pStore);

        // PURPOSE
        //  update version change on modification
        void ChangeVersion()
        {
            _versionManager.ChangeVersion();
        }
        HRESULT EnsureInitialized() const
        {
            XWINRT_ASSERT(_initialized);
            return _initialized ? S_OK : XWinRT::detail::FailsWith(E_UNEXPECTED);
        }

        HashStore                                   _hashMap;
        Hasher                                      _fnHash;
        EqualityPredicate                           _fnEquals;
        XWinRT::ComLock                             _comLock;
        XWinRT::SecureVersionTag::TagManager        _versionManager;
        bool                                        _initialized;

        typename HashMapOptions::EventType          _evtMapChanged;
        typename HashMapOptions::EventTrackingType  _listeners;
        long                                        _reentrancyGuard;
    };

    template <
        class K,
        class V,
        class Hasher            = DefaultHash<K>,
        class EqualityPredicate = DefaultEqualityPredicate<K>,
        class KeyLifetime       = DefaultLifetimeTraits<K>,
        class ValueLifetime     = DefaultLifetimeTraits<V> >
    class ObservableHashMap WrlSealed : public HashMap <
            K,
            V,
            Hasher,
            EqualityPredicate,
            KeyLifetime,
            ValueLifetime,
            HashMapOptions < K, V, KeyLifetime, true > >
    {
        using base = HashMap<
            K,
            V,
            Hasher,
            EqualityPredicate,
            KeyLifetime,
            ValueLifetime,
            HashMapOptions< K, V, KeyLifetime, true>>;

    private:
        ObservableHashMap() {}
    public:
        static HRESULT Make(const Hasher& fnHasher, const EqualityPredicate& fnEquals, _COM_Outptr_ ObservableHashMap** ppHashMap)
        {
            // The cast in the following line works because ObservableHashMap derives *only* from HashMap and
            //   does not contain any data members. Any change to these conditions is bound to break this call
            //   and will result in failures at runtime.
            return base::Make(fnHasher, fnEquals, reinterpret_cast<base**>(ppHashMap));
        }
        static HRESULT Make(_COM_Outptr_ ObservableHashMap** ppHashMap)
        {
            return Make(Hasher(), EqualityPredicate(), ppHashMap);
        }
    };

    #pragma warning( push )
    #pragma warning( disable: 4355 ) // 'this' used in base member initializer
    template <
        class K,
        class V,
        class Hasher,
        class EqualityPredicate,
        class KeyLifetime,
        class ValueLifetime,
        class HashMapOptions
    >
    HashMap<K,V,Hasher,EqualityPredicate, KeyLifetime, ValueLifetime, HashMapOptions>
        ::HashMap(const Hasher& fnHash, const EqualityPredicate& fnEquals, permission)
    : _hashMap(this)
    , _fnHash(fnHash)
    , _fnEquals(fnEquals)
    , _reentrancyGuard(0)
    , _initialized(false)
    {
        // Providing custom equality predicates on interface types risks deadlock if calling into an (A)STA object.  Prevent this for now.
        static_assert(detail::CollectionsFunctionValid<K, EqualityPredicate, DefaultEqualityPredicate<K>>::value,
            "Custom equality predicates can not be provided on interface types");

        // Providing custom hashers on interface types risks deadlock if calling into an (A)STA object.  Prevent this for now.
        static_assert(detail::CollectionsFunctionValid<K, Hasher, DefaultHash<K>>::value,
            "Custom hashers can not be provided on interface types");
    }
    #pragma warning( pop )

    template <
        class K,
        class V,
        class Hasher,
        class EqualityPredicate,
        class KeyLifetime,
        class ValueLifetime,
        class HashMapOptions
    >
    void HashMap<K,V,Hasher,EqualityPredicate, KeyLifetime, ValueLifetime, HashMapOptions>
        ::_EraseAll(HashStore *pStore)
    {
        if (_initialized)
        {
            _versionManager.ChangeVersion();

            if (pStore != nullptr)
            {
                using XWinRT::FakeStl::swap;
                swap(*pStore, _hashMap);
            }
            else
            {
                _Free(&_hashMap);
            }
        }
    }

    template <
        class K,
        class V,
        class Hasher,
        class EqualityPredicate,
        class KeyLifetime,
        class ValueLifetime,
        class HashMapOptions
    >
    void HashMap<K,V,Hasher,EqualityPredicate, KeyLifetime, ValueLifetime, HashMapOptions>
        ::_Free(HashStore *pStore)
    {
        if (pStore != nullptr)
        {
            // release all the elements held in the hashmap
            for(auto iter = pStore->GetStartPosition();
                    iter != NULL;)
            {
                // move iterator forward, and retrieve a pair
                auto pPair = pStore->GetNext(iter);
                KeyLifetime::Destroy(const_cast<K_storage*>(&pPair->m_key));
                ValueLifetime::Destroy(&pPair->m_value);
            }

            // If failed, all elements have been removed but new buckets may
            //   not have been set up -- that's OK though, ignore it.
            (void)pStore->RemoveAll();
        }
    }

    template <
        class K,
        class V,
        class Hasher,
        class EqualityPredicate,
        class KeyLifetime,
        class ValueLifetime,
        class HashMapOptions
    >
    HashMap<K,V,Hasher,EqualityPredicate, KeyLifetime, ValueLifetime, HashMapOptions>
        ::~HashMap()
    {
        _EraseAll(nullptr);
    }

    template <
        class K,
        class V,
        class Hasher,
        class EqualityPredicate,
        class KeyLifetime,
        class ValueLifetime,
        class HashMapOptions
    >
    STDMETHODIMP HashMap<K,V,Hasher,EqualityPredicate, KeyLifetime, ValueLifetime, HashMapOptions>
        ::Initialize()
    {
        HRESULT hr = _versionManager.Initialize();

        if (SUCCEEDED(hr))
        {
            HashMapOptions::InitializeListenerTracking(_listeners);
            _initialized = true;
        }

        return hr;
    }

    template <
        class K,
        class V,
        class Hasher,
        class EqualityPredicate,
        class KeyLifetime,
        class ValueLifetime,
        class HashMapOptions
    >
    IFACEMETHODIMP HashMap<K,V,Hasher,EqualityPredicate, KeyLifetime, ValueLifetime, HashMapOptions>
        ::First(_Outptr_result_maybenull_ wfc::IIterator<wfc::IKeyValuePair<K, V>*> **first)
    {
        *first = nullptr;
        HRESULT hr = EnsureInitialized();

        if (SUCCEEDED(hr))
        {
            auto acquired = HashMapOptions::LockPolicy::Read(_comLock, &hr);
            if (SUCCEEDED(hr))
            {
                Microsoft::WRL::ComPtr<wfc::IIterator<wfc::IKeyValuePair<K, V>*>> spIter
                    = Microsoft::WRL::Make<Iterator>(this);

                if (spIter)
                {
                    *first = spIter.Detach();
                }
                else
                {
                    hr = E_OUTOFMEMORY;
                }
            }
        }
        return hr;
    }

    template <
        class K,
        class V,
        class Hasher,
        class EqualityPredicate,
        class KeyLifetime,
        class ValueLifetime,
        class HashMapOptions
    >
    IFACEMETHODIMP HashMap<K,V,Hasher,EqualityPredicate, KeyLifetime, ValueLifetime, HashMapOptions>
        ::Lookup(_In_opt_ K_abi key, _Out_ V_abi *value)
    {
        using XWinRT::detail::StorageReference;

        HRESULT hr = EnsureInitialized();

        V_TempTraits::type tempValue;

        if (SUCCEEDED(hr))
        {
            auto acquired = HashMapOptions::LockPolicy::Read(_comLock, &hr);
            if (SUCCEEDED(hr))
            {
                InnerCPair* pPair;
                StorageReference<K_abi, K_storage, KeyLifetime> storageKey(key, &hr);
                if (SUCCEEDED(hr))
                {
                    hr = _hashMap.Lookup(storageKey.get(), &pPair);
                    if (SUCCEEDED(hr))
                    {
                        if (pPair)
                        {
                            // Depending on the temp traits, this may make a temporary copy into &tempValue in order to perform
                            // certain operations outside of lock scope during a ResolveDemand call.
                            hr = V_TempTraits::CopyOnDemand(&tempValue, value, pPair->m_value);
                        }
                        else
                        {
                            // element not found
                            // Do not OriginateError here because it is too noisy.  It is known
                            // that Lookup will often be called for keys that are not present in the map.
                            hr = E_BOUNDS;
                        }
                    }
                }
            }
        }

        if (SUCCEEDED(hr))
        {
            // Ensure that unsafe operations (e.g.: GIT localization) happen outside of lock scope for types which require it.
            // This call may be a NOP for certain types.
            hr = V_TempTraits::ResolveDemand(&tempValue, value);
        }

        if (FAILED(hr))
        {
            // construct empty value on failure
            ValueLifetime::Construct(value);
        }
        return hr;
    }

    template <
        class K,
        class V,
        class Hasher,
        class EqualityPredicate,
        class KeyLifetime,
        class ValueLifetime,
        class HashMapOptions
    >
    /*propget*/ IFACEMETHODIMP HashMap<K,V,Hasher,EqualityPredicate, KeyLifetime, ValueLifetime, HashMapOptions>
        ::get_Size(_Out_ unsigned int *size)
    {
        *size = 0;
        HRESULT hr = EnsureInitialized();

        if (SUCCEEDED(hr))
        {
            auto acquired = HashMapOptions::LockPolicy::Read(_comLock, &hr);
            if (SUCCEEDED(hr))
            {
                *size = static_cast<unsigned int>(_hashMap.GetCount());
            }
        }
        return hr;
    }

    template <
        class K,
        class V,
        class Hasher,
        class EqualityPredicate,
        class KeyLifetime,
        class ValueLifetime,
        class HashMapOptions
    >
    IFACEMETHODIMP HashMap<K,V,Hasher,EqualityPredicate, KeyLifetime, ValueLifetime, HashMapOptions>
        ::HasKey(_In_opt_ K_abi key, _Out_ boolean *found)
    {
        using XWinRT::detail::StorageReference;

        *found = false;
        HRESULT hr = EnsureInitialized();

        if (SUCCEEDED(hr))
        {
            auto acquired = HashMapOptions::LockPolicy::Read(_comLock, &hr);
            if (SUCCEEDED(hr))
            {
                InnerCPair* pPair;
                StorageReference<K_abi, K_storage, KeyLifetime> storageKey(key, &hr);
                if (SUCCEEDED(hr))
                {
                    hr = _hashMap.Lookup(storageKey.get(), &pPair);
                    if (SUCCEEDED(hr))
                    {
                        if (pPair)
                        {
                            *found = true;
                        }
                        else
                        {
                            // element not found
                            *found = false;
                        }
                    }
                }
            }
        }
        return hr;
    }

    template <
        class K,
        class V,
        class Hasher,
        class EqualityPredicate,
        class KeyLifetime,
        class ValueLifetime,
        class HashMapOptions
    >
    IFACEMETHODIMP HashMap<K,V,Hasher,EqualityPredicate, KeyLifetime, ValueLifetime, HashMapOptions>
        ::GetView(_Outptr_result_maybenull_ wfc::IMapView<K,V> **view)
    {
        *view = nullptr;
        HRESULT hr = EnsureInitialized();

        if (SUCCEEDED(hr))
        {
            auto acquired = HashMapOptions::LockPolicy::Read(_comLock, &hr);
            if (SUCCEEDED(hr))
            {
                Microsoft::WRL::ComPtr<wfc::IMapView<K,V>> spView = Microsoft::WRL::Make<View>(this, _fnEquals);
                if (spView)
                {
                    *view = spView.Detach();
                }
                else
                {
                    hr = E_OUTOFMEMORY;
                }
            }
        }
        return hr;
    }

    template <
        class K,
        class V,
        class Hasher,
        class EqualityPredicate,
        class KeyLifetime,
        class ValueLifetime,
        class HashMapOptions
    >
    IFACEMETHODIMP HashMap<K,V,Hasher,EqualityPredicate, KeyLifetime, ValueLifetime, HashMapOptions>
        ::Insert(_In_opt_ K_abi key, _In_opt_ V_abi value, _Out_ boolean *replaced)
    {
        using XWinRT::AutoValue;

        *replaced = false;
        HRESULT hr = EnsureInitialized();

        HashMapOptions::ReentrancyGuard guard;

        AutoValue<K_storage, KeyLifetime>   sNewKey;
        hr = KeyLifetime::Construct(sNewKey.OutParam(), key);
        if (SUCCEEDED(hr))
        {
            AutoValue<V_storage, ValueLifetime> sNewValue(value, &hr);
            AutoValue<V_storage, ValueLifetime> sOldValue;

            if (SUCCEEDED(hr))
            { // scope for lock
                auto acquired = HashMapOptions::LockPolicy::Write(_comLock, &hr);
                if (SUCCEEDED(hr))
                {
                    guard = HashMapOptions::ReentrancyGuard(_listeners, &_reentrancyGuard);
                    hr = guard.hr();

                    if (SUCCEEDED(hr))
                    {
                        _versionManager.ChangeVersion();

                        InnerCPair* pPair;
                        hr = _hashMap.Lookup(sNewKey.get(), &pPair);
                        if (SUCCEEDED(hr))
                        {
                            if (pPair) // replace element
                            {
                                // remember old value, free it outside the lock
                                sOldValue.Attach(pPair->m_value);

                                pPair->m_value = sNewValue.get();
                                sNewValue.Abandon();

                                *replaced = true;
                            }
                            else // insert new element
                            {
                                // max upper bound, to interoperate with programming
                                //  languages where counts are signed 32bit.
                                if (_hashMap.GetCount() == s_cuMaxSize)
                                {
                                    hr = E_OUTOFMEMORY;
                                }

                                if (SUCCEEDED(hr))
                                {
                                    // TODO: WIN8TFS 81438 - this else case relies on the user's
                                    //   hash function to be repeatable for correct lifetime management.
                                    //   How much of a problem is that?
                                    XWinRT::XPOSITION position;
                                    hr = _hashMap.SetAt(sNewKey.get(), sNewValue.get(), &position);
                                    if (SUCCEEDED(hr))
                                    {
                                        sNewKey.Abandon();
                                        sNewValue.Abandon();
                                    }
                                }
                            }
                        }
                    }
                }
            } // lock released, then old values destructed
        }
        if (SUCCEEDED(hr))
        {
            wfc::CollectionChange change = *replaced ? wfc::CollectionChange_ItemChanged : wfc::CollectionChange_ItemInserted;
            hr = HashMapOptions::RaiseEvent(guard, _evtMapChanged, this, change, key);
        }
        return hr;
    }

    template <
        class K,
        class V,
        class Hasher,
        class EqualityPredicate,
        class KeyLifetime,
        class ValueLifetime,
        class HashMapOptions
    >
    IFACEMETHODIMP HashMap<K,V,Hasher,EqualityPredicate, KeyLifetime, ValueLifetime, HashMapOptions>
        ::Remove(_In_opt_ K_abi key)
    {
        using XWinRT::AutoValue;
        using XWinRT::detail::StorageReference;

        HRESULT hr = EnsureInitialized();

        HashMapOptions::ReentrancyGuard guard;

        if (SUCCEEDED(hr))
        {
            AutoValue<K_storage, KeyLifetime> sOldKey;
            AutoValue<V_storage, ValueLifetime> sOldValue;
            StorageReference<K_abi, K_storage, KeyLifetime> storageKey(key, &hr);

            if (SUCCEEDED(hr))
            { // scope for lock
                auto acquired = HashMapOptions::LockPolicy::Write(_comLock, &hr);
                if (SUCCEEDED(hr))
                {
                    guard = HashMapOptions::ReentrancyGuard(_listeners, &_reentrancyGuard);
                    hr = guard.hr();
                    InnerCPair* pPair = nullptr;
                    if (SUCCEEDED(hr))
                    {
                        hr = _hashMap.Lookup(storageKey.get(), &pPair);
                    }
                    if (SUCCEEDED(hr))
                    {
                        if (!pPair)
                        {
                            hr = XWinRT::detail::FailsWith(E_BOUNDS);
                        }
                    }

                    if (SUCCEEDED(hr))
                    {
                        // update version tag.  This must be performed
                        //  before the collection is actually modified,
                        //  in case it fails.
                        hr = _versionManager.ChangeVersion();
                    }
                    if (SUCCEEDED(hr))
                    {
                        sOldKey.Attach(pPair->m_key);
                        sOldValue.Attach(pPair->m_value);

                        hr = _hashMap.RemoveAtPos(pPair);

                        // only destroy elements if successfully removed
                        if (FAILED(hr))
                        {
                            sOldKey.Abandon();
                            sOldValue.Abandon();
                        }
                    }
                }
            }// leave lock
        }
        if ( SUCCEEDED (hr) )
        {
            hr = HashMapOptions::RaiseEvent(guard, _evtMapChanged, this, wfc::CollectionChange_ItemRemoved, key);
        }

        // values will be released on exit
        return hr;
    }

    template <
        class K,
        class V,
        class Hasher,
        class EqualityPredicate,
        class KeyLifetime,
        class ValueLifetime,
        class HashMapOptions
    >
    IFACEMETHODIMP HashMap<K,V,Hasher,EqualityPredicate, KeyLifetime, ValueLifetime, HashMapOptions>
        ::Clear()
    {
        K_abi emptyKey;
        KeyLifetime::Construct(&emptyKey);
        bool notificationNeeded = false;
        HashMapOptions::ReentrancyGuard guard;
        HRESULT hr = EnsureInitialized();

        if (SUCCEEDED(hr))
        {
            HashStore temporaryStore(this);
            {
                auto acquired = HashMapOptions::LockPolicy::Write(_comLock, &hr);
                if (SUCCEEDED(hr))
                {
                    guard = HashMapOptions::ReentrancyGuard(_listeners, &_reentrancyGuard);
                    hr = guard.hr();
                    if (SUCCEEDED(hr))
                    {
                        if (!_hashMap.IsEmpty())
                        {
                            // There was at least one element in the map, we should call the event listener
                            notificationNeeded = true;
                        }

                        _EraseAll(&temporaryStore);
                    }
                }
            } // end of lock scope
            _Free(&temporaryStore);
        }
        if (SUCCEEDED(hr) && notificationNeeded)
        {
            hr = HashMapOptions::RaiseEvent(guard, _evtMapChanged, this, wfc::CollectionChange_Reset, emptyKey);
        }
        return hr;
    }

    template <
        class K,
        class V,
        class Hasher,
        class EqualityPredicate,
        class KeyLifetime,
        class ValueLifetime,
        class HashMapOptions
    >
    IFACEMETHODIMP HashMap<K,V,Hasher,EqualityPredicate, KeyLifetime, ValueLifetime, HashMapOptions>
        ::add_MapChanged ( _In_ wfc::MapChangedEventHandler<K,V>* handler, _Out_ EventRegistrationToken* token )
    {
        HRESULT hr = HashMapOptions::AddEventHandler( _evtMapChanged, handler, token );
        if (SUCCEEDED(hr))
        {
            HashMapOptions::TrackListener(_listeners);
        }
        return hr;
    }

    template <
        class K,
        class V,
        class Hasher,
        class EqualityPredicate,
        class KeyLifetime,
        class ValueLifetime,
        class HashMapOptions
    >
    IFACEMETHODIMP HashMap<K,V,Hasher,EqualityPredicate, KeyLifetime, ValueLifetime, HashMapOptions>
        ::remove_MapChanged ( EventRegistrationToken token )
    {
        HRESULT hr = HashMapOptions::RemoveEventHandler( _evtMapChanged, token );
        if (SUCCEEDED(hr))
        {
            HashMapOptions::UntrackListener(_listeners);
        }
        return hr;
    }

    // PURPOSE
    //  used by HashMap::GetView to provide a read-only, version invalidated
    //  view of the hash table.
    template <
        class K,
        class V,
        class Hasher,
        class EqualityPredicate,
        class KeyLifetime,
        class ValueLifetime,
        class HashMapOptions
    >
    class HashMap<K,V,Hasher,EqualityPredicate, KeyLifetime, ValueLifetime, HashMapOptions>::View WrlSealed : public
        Microsoft::WRL::RuntimeClass< wfc::IMapView<K, V>, wfc::IIterable<wfc::IKeyValuePair<K, V>*> >
    {
    private:
        typedef typename detail::GetAbiType<typename wfc::IMapView<K,V>::K_complex>::type       K_abi;
        typedef typename detail::GetAbiType<typename wfc::IMapView<K,V>::V_complex>::type       V_abi;

        static const wchar_t* z_get_rc_name()
        {
            // use the MIDLRT generated name string, which substitutes
            //   the proper metadata-name-representation of K and V
            return wfc::IMapView<K,V>::z_get_rc_name_impl();
        }
        InspectableClass(z_get_rc_name(), (::BaseTrust))

    public:
        View(_In_ HashMap* pHashMap, EqualityPredicate fnEquals)
        : _spHashMap(pHashMap)
        , _expectedVersion(pHashMap)
        , _fnEquals(fnEquals)
        {
            _expectedVersion.AddRef();
        }

        IFACEMETHOD(First)(_Outptr_result_maybenull_ wfc::IIterator<wfc::IKeyValuePair<K, V>*> **first)
        {
            HRESULT hr = EnsureVersionMatches();
            if (SUCCEEDED(hr))
            {
                return _spHashMap->First(first);
            }
            return hr;
        }

        IFACEMETHOD(Lookup)(_In_opt_ K_abi key, _Out_ V_abi *value)
        {
            HRESULT hr = EnsureVersionMatches();
            if (SUCCEEDED(hr))
            {
                return _spHashMap->Lookup(key, value);
            }
            return hr;
        }
        /*propget*/ IFACEMETHOD(get_Size)(_Out_ unsigned int *size)
        {
            HRESULT hr = EnsureVersionMatches();
            if (SUCCEEDED(hr))
            {
                return _spHashMap->get_Size(size);
            }
            return hr;
        }
        IFACEMETHOD(HasKey)(_In_opt_ K_abi key, _Out_ boolean *found)
        {
            HRESULT hr = EnsureVersionMatches();
            if (SUCCEEDED(hr))
            {
                return _spHashMap->HasKey(key, found);
            }
            return hr;
        }
        IFACEMETHOD(Split)(_Outptr_result_maybenull_ wfc::IMapView<K,V> **firstPartition, _Outptr_result_maybenull_ wfc::IMapView<K,V> **secondPartition);

    protected:
        ~View()
        {
            _expectedVersion.Release();
        }

    private:
        View(const View&); // = delete
        void operator=(const View&); // = delete

        // PURPOSE
        //  verify versions matched, or fail with E_CHANGED_STATE.
        HRESULT EnsureVersionMatches()
        {
            HRESULT hr = S_OK;
            XWinRT::SecureVersionTag currentVersion(_spHashMap.Get());
            if (!(currentVersion == _expectedVersion))
            {
                // NOTE: cannot safely release _spHashMap
                //  without needing to use locks throughout the
                //  implementation.
                hr = XWinRT::detail::FailsWith(E_CHANGED_STATE);
            }

            return hr;
        }

        Microsoft::WRL::ComPtr<HashMap>     _spHashMap;
        XWinRT::SecureVersionTag    _expectedVersion;
        EqualityPredicate           _fnEquals;
    };

    // PURPOSE
    //  general purpose key-value pair class
    // USAGE
    //  construct with Microsoft::WRL::Make, then Initialize.  If it
    //  succeeds, use as normal. Otherwise release immediately.
    template <
        class K,
        class V,
        class KeyLifetime = DefaultLifetimeTraits<K>,
        class ValueLifetime = DefaultLifetimeTraits<V>
    >
    class SimpleKeyValuePair WrlSealed : public Microsoft::WRL::RuntimeClass<wfc::IKeyValuePair<K,V>>
    {
        typedef typename detail::GetAbiType<typename wfc::IMap<K,V>::K_complex>::type       K_abi;
        typedef typename detail::GetAbiType<typename wfc::IMap<K,V>::V_complex>::type       V_abi;
        typedef typename detail::StorageType<K_abi>::type                               K_storage;
        typedef typename detail::StorageType<V_abi>::type                               V_storage;

        static const wchar_t* z_get_rc_name()
        {
            // use the MIDLRT generated name string, which substitutes
            //   the proper metadata-name-representation of K and V
            return wfc::IKeyValuePair<K,V>::z_get_rc_name_impl();
        }
        InspectableClass(z_get_rc_name(), (::BaseTrust))

    private:
        // PURPOSE
        //  constructor must be public for WRL to be able to call it,
        //  but should not generally be callable by clients.
        struct permission {};

    public:
        SimpleKeyValuePair(permission)
        {
        }

    private:
        HRESULT Initialize(const K_storage& key, const V_storage& value)
        {
            using XWinRT::AutoValue;

            HRESULT hr = S_OK;

            AutoValue<K_storage, KeyLifetime> sNewKey(key, &hr);
            if (SUCCEEDED(hr))
            {
                AutoValue<V_storage, ValueLifetime> sNewValue(value, &hr);
                if (SUCCEEDED(hr))
                {
                    _sKey.Swap(sNewKey);
                    _sValue.Swap(sNewValue);
                }
            }

            return hr;
        }

    public:
        static HRESULT Make(_In_ const K_storage& key, _In_ const V_storage& value, _COM_Outptr_ SimpleKeyValuePair** ppPair)
        {
            *ppPair = nullptr;
            HRESULT hr = S_OK;

            Microsoft::WRL::ComPtr<SimpleKeyValuePair> sp = Microsoft::WRL::Make<SimpleKeyValuePair>(permission());
            if (!sp)
            {
                hr = E_OUTOFMEMORY;
            }
            if (SUCCEEDED(hr))
            {
                hr = sp->Initialize(key, value);
            }
            if (SUCCEEDED(hr))
            {
                *ppPair = sp.Detach();
            }
            return hr;
        }

        IFACEMETHOD(get_Key)(_Out_ K_abi *key)
        {
            return KeyLifetime::Construct(key, _sKey.get());
        }
        IFACEMETHOD(get_Value)(_Out_ V_abi *value)
        {
            return ValueLifetime::Construct(value, _sValue.get());
        }

    private:
        XWinRT::AutoValue<K_storage, KeyLifetime>   _sKey;
        XWinRT::AutoValue<V_storage, ValueLifetime> _sValue;
    };

    // PURPOSE
    //  iterator type for HashMap
    //  used by HashMap::First and HashMap::View::First
    // NOTES
    //  since Iterator stores internal pointers to HashMap's data structures,
    //  it needs to accurately detect version changes in the HashMap.
    //
    //  See further documentation on the _position data member.
    //
    template <
        class K,
        class V,
        class Hasher,
        class EqualityPredicate,
        class KeyLifetime,
        class ValueLifetime,
        class HashMapOptions
    >
    class HashMap<K,V,Hasher,EqualityPredicate, KeyLifetime, ValueLifetime, HashMapOptions>::Iterator WrlSealed : public
            Microsoft::WRL::RuntimeClass< wfc::IIterator<wfc::IKeyValuePair<K, V>*> >
    {
        static const wchar_t* z_get_rc_name()
        {
            // use the MIDLRT generated name string, which substitutes
            //   the proper metadata-name-representation of K and V
            return wfc::IIterator<wfc::IKeyValuePair<K, V>*>::z_get_rc_name_impl();
        }
        InspectableClass(z_get_rc_name(), (::BaseTrust))

    public:
        Iterator(_In_ HashMap* pHashMap)
        : _spHashMap(pHashMap)
        , _position(pHashMap->_hashMap.GetStartPosition())
        , _expectedVersion(pHashMap)
        {
            _expectedVersion.AddRef();
        }

        /* propget */ IFACEMETHOD(get_Current)(_Outptr_result_maybenull_ wfc::IKeyValuePair<K, V>  **current)
        {
            *current = nullptr;
            HRESULT hr = S_OK;

            auto acquired = HashMapOptions::LockPolicy::Read(_spHashMap->_comLock, &hr);
            if (SUCCEEDED(hr))
            {
                // version check needs to be performed under lock
                hr = EnsureVersionMatches();
                auto pCurPair = _spHashMap->_hashMap.GetAt(_position);
                if (SUCCEEDED(hr) && pCurPair == nullptr)
                {
                    // at-end
                    hr = XWinRT::detail::FailsWith(E_BOUNDS);
                }

                if (SUCCEEDED(hr))
                {
                    // construct a new keyValuePair
                    Microsoft::WRL::ComPtr<SimpleKeyValuePair<K, V, KeyLifetime, ValueLifetime>> spPair;
                    hr = SimpleKeyValuePair<K, V, KeyLifetime, ValueLifetime>::Make(
                        pCurPair->m_key,
                        pCurPair->m_value,
                        &spPair);
                    if (SUCCEEDED(hr))
                    {
                        *current = static_cast<Microsoft::WRL::ComPtr<wfc::IKeyValuePair<K,V>>>(spPair).Detach();
                    }
                }
            }
            return hr;
        }
        /* propget */ IFACEMETHOD(get_HasCurrent)(_Out_ boolean *hasCurrent)
        {
            // race-prevention lock is not needed to read POD type

            *hasCurrent = !!_position;
            return S_OK;
        }
        IFACEMETHOD(MoveNext)(_Out_ boolean *hasCurrent)
        {
            *hasCurrent = false;
            HRESULT hr = S_OK;

            auto acquired = HashMapOptions::LockPolicy::Read(_spHashMap->_comLock, &hr);
            if (SUCCEEDED(hr))
            {
                // version check needs to be performed under lock
                hr = EnsureVersionMatches();
                if (SUCCEEDED(hr))
                {
                    XWinRT::XPOSITION position = _position;
                    if (position == nullptr)
                    {
                        hr = XWinRT::detail::FailsWith(E_BOUNDS);
                    }
                    else
                    {
                        _spHashMap->_hashMap.GetNext(position);
                        _position = position;
                        *hasCurrent = !!position;
                    }
                }
            }
            return hr;
        }


        //IIterator::GetMany
        IFACEMETHOD(GetMany)(_In_ unsigned capacity, _Out_writes_to_(capacity,*actual) _Outptr_result_maybenull_ wfc::IKeyValuePair<K, V> *rgI[], _Out_ unsigned *actual)
        {
            HRESULT hr = S_OK;
            unsigned count = 0;

            // null the out parameters
            *actual = 0;
            for(unsigned index = 0; index < capacity; index++)
            {
                rgI[index] = nullptr;
            }

            auto acquired = HashMapOptions::LockPolicy::Read(_spHashMap->_comLock, &hr);
            if (SUCCEEDED(hr))
            {
                // version check needs to be performed under lock
                hr = EnsureVersionMatches();
            }

            XWinRT::XPOSITION position = _position;
            for(unsigned index = 0; (SUCCEEDED(hr)) && (index < capacity); index++)
            {
                auto pCurPair = _spHashMap->_hashMap.GetAt(position);
                if (SUCCEEDED(hr) && pCurPair == nullptr)
                {
                    // at-end
                    break;
                }

                if (SUCCEEDED(hr))
                {
                    // construct a new keyValuePair
                    Microsoft::WRL::ComPtr<SimpleKeyValuePair<K, V, KeyLifetime, ValueLifetime>> spPair;
                    hr = SimpleKeyValuePair<K, V, KeyLifetime, ValueLifetime>::Make(
                        pCurPair->m_key,
                        pCurPair->m_value,
                        &spPair);

                    if (SUCCEEDED(hr))
                    {
                        rgI[index] = static_cast<Microsoft::WRL::ComPtr<wfc::IKeyValuePair<K,V>>>(spPair).Detach();
                        _spHashMap->_hashMap.GetNext(position);
                        count++;
                        if (!position)
                        {
                            break;
                        }
                    }
                }
            }

            if (SUCCEEDED(hr))
            {
                _position = position;
                *actual = count;
            }

            if(FAILED(hr) && (count > 0))
            {
                // Loop through and release any allocated before the failure
                for (unsigned i = 0; i < count; ++i)
                {
                    rgI[i]->Release();
                    rgI[i] = nullptr;
                }
            }

            return hr;
        }

    protected:
        ~Iterator()
        {
            _expectedVersion.Release();
        }

    private:
        // PURPOSE
        //  verify versions matched, or fail with E_CHANGED_STATE.
        HRESULT EnsureVersionMatches()
        {
            XWinRT::SecureVersionTag currentVersion(_spHashMap.Get());
            if (!(currentVersion == _expectedVersion))
            {
                // NOTE: cannot safely release _spHashMap
                //  without needing to use locks throughout the
                //  implementation.
                return XWinRT::detail::FailsWith(E_CHANGED_STATE);
            }

            return S_OK;
        }

        Microsoft::WRL::ComPtr<HashMap>         _spHashMap;

        // Threading: the _position variable may be accessed from multiple threads
        //    simultaneously, and this component needs to be robust (not crash) in the
        //    face of such data races.  We to not need to ensure sequential consistency,
        //    but only robustness.
        // To prevent runtime data corruption or AV, always perform atomic reads and writes
        //    with barrier (VC compiler provides this semantics whenever volatile is used on
        //    a pointer or smaller sized builtin), and validate the pointer before use
        //    (eg, using the secure version tag).
        volatile XWinRT::XPOSITION              _position;
        XWinRT::SecureVersionTag                _expectedVersion;
    };

    // NOTE
    //   do not use directly.  Use NaiveSplit directly
    // PURPOSE
    //   Implements IMapView::Split over an Iterable
    template <
        class K,
        class V,
        class EqualityPredicate = DefaultEqualityPredicate<K>,
        class KeyLifetime       = DefaultLifetimeTraits<K>,
        class ValueLifetime     = DefaultLifetimeTraits<V>,
        class HashMapOptions    = DefaultHashMapOptions<K, V, KeyLifetime>
    >
    class NaiveSplitView WrlSealed : public
        Microsoft::WRL::RuntimeClass< wfc::IMapView<K, V>, wfc::IIterable<wfc::IKeyValuePair<K, V>*> >
    {
        typedef typename detail::GetAbiType<typename wfc::IMapView<K,V>::K_complex>::type K_abi;
        typedef typename detail::GetAbiType<typename wfc::IMapView<K,V>::V_complex>::type V_abi;

        static const wchar_t* z_get_rc_name()
        {
            // use the MIDLRT generated name string, which substitutes
            //   the proper metadata-name-representation of K and V
            return wfc::IMapView<K,V>::z_get_rc_name_impl();
        }
        InspectableClass(z_get_rc_name(), (::BaseTrust))

    private:

        typedef wfc::IKeyValuePair<K, V> PairT;

        // PURPOSE
        //  fixed size chunk of pair pointers
        class Chunk
        {
        public:
            static HRESULT make(Chunk** ppChunk)
            {
                Chunk* pTemp = *ppChunk = new (std::nothrow) Chunk;
                if (pTemp)
                {
                    return S_OK;
                }
                else
                {
                    return E_OUTOFMEMORY;
                }
            }

            long AddRef() const
            {
                return _InterlockedIncrement(&_uRefs);
            }
            long Release() const
            {
                auto iRefs = _InterlockedDecrement(&_uRefs);
                if (iRefs == 0)
                {
                    delete this;
                }
                return iRefs;
            }

            HRESULT AddPair(PairT* pPair)
            {
                HRESULT hr = S_OK;
                if (_uSize == sc_uChunkMax)
                {
                    hr = E_OUTOFMEMORY;
                }
                else // if (SUCCEEDED(hr)) <-- prefast gets confused by FailsWith,
                     //                        doesn't realize it will return a failed
                     //                        hr code. Use explicit conditional instead.
                {
                    pPair->AddRef();
                    _apPairs[_uSize++] = pPair;
                }
                return hr;
            }

            PairT* operator[](unsigned uIndex) const
            {
                XWINRT_ASSERT(uIndex < _uSize);

                return _apPairs[uIndex];
            }
            PairT*& operator[](unsigned uIndex)
            {
                XWINRT_ASSERT(uIndex < _uSize);
                return _apPairs[uIndex];
            }
            unsigned int get_Size() const
            {
                return _uSize;
            }

            static const unsigned sc_uChunkMax = 16;

        private:

            // Bug workaround: Win8:71243, define a virtual method so that
            //  no base offset is needed
            virtual void dummy() { }

            Chunk()
            : _uRefs(1)
            , _uSize(0)
            {
            }
            ~Chunk()
            {
                for (unsigned i = 0; i < _uSize; ++i)
                {
                    _apPairs[i]->Release();
                }
            }

            mutable volatile LONG   _uRefs;
            unsigned                _uSize;
            PairT*                  _apPairs[sc_uChunkMax];
        };

        class ChunkElementIterator WrlSealed : public
            Microsoft::WRL::RuntimeClass< wfc::IIterator<wfc::IKeyValuePair<K, V>*> >
        {
            static const wchar_t* z_get_rc_name()
            {
                // use the MIDLRT generated name string, which substitutes
                //   the proper metadata-name-representation of K and V
                return wfc::IIterator<wfc::IKeyValuePair<K,V>*>::z_get_rc_name_impl();
            }
            InspectableClass(z_get_rc_name(), (::BaseTrust))

        public:
            // Bug workaround: Win8:71107, cannot use const with comptr
            ChunkElementIterator(/*const*/ Chunk* pChunk)
            : _spChunk(pChunk), _uIndex(0)
            {
            }

            /* propget */ IFACEMETHOD(get_Current)(_Outptr_result_maybenull_ wfc::IKeyValuePair<K, V>  **current)
            {
                *current = nullptr;
                HRESULT hr = S_OK;

                if (_uIndex < _spChunk->get_Size())
                {
                    (*current = (*_spChunk.Get())[_uIndex])->AddRef();
                }
                else
                {
                    hr = XWinRT::detail::FailsWith(E_BOUNDS);
                }
                return hr;
            }

            /* propget */ IFACEMETHOD(get_HasCurrent)(_Out_ boolean *hasCurrent)
            {
                *hasCurrent = (_uIndex < _spChunk->get_Size());
                return S_OK;
            }

            IFACEMETHOD(MoveNext)(_Out_ boolean *hasCurrent)
            {
                *hasCurrent = false;

                HRESULT hr = S_OK;
                unsigned int uSize = _spChunk->get_Size();
                if (_uIndex < uSize)
                {
                    ++_uIndex;
                    *hasCurrent = (_uIndex < _spChunk->get_Size());
                }
                else
                {
                    hr = XWinRT::detail::FailsWith(E_BOUNDS);
                }
                return hr;
            }

            // Bug workaround: Win8:71107, cannot use const with comptr
            Microsoft::WRL::ComPtr</*const*/ Chunk>     _spChunk;
            unsigned                            _uIndex;
        };

        // PURPOSE
        //  a not-splitable chunk
        class ChunkView WrlSealed : public
            Microsoft::WRL::RuntimeClass< wfc::IMapView<K, V>, wfc::IIterable<wfc::IKeyValuePair<K, V>*> >
        {
            typedef typename detail::GetAbiType<typename wfc::IMapView<K,V>::K_complex>::type       K_abi;
            typedef typename detail::GetAbiType<typename wfc::IMapView<K,V>::V_complex>::type       V_abi;

            static const wchar_t* z_get_rc_name()
            {
                // use the MIDLRT generated name string, which substitutes
                //   the proper metadata-name-representation of K and V
                return wfc::IIterable<wfc::IKeyValuePair<K,V>*>::z_get_rc_name_impl();
            }
            InspectableClass(z_get_rc_name(), (::BaseTrust))

        public:
            ChunkView(const EqualityPredicate& fnEquals)
            : _fnEquals(fnEquals)
            {
            }

            // NOTES
            //  allocates a struct up front, reads first few values,
            //  but lazily fills in "remaining" pointer.
            //  This pointer is protected elsewhere with a srw lock
            //
            // THREAD SAFEY
            //  Initialize, as part of construction, is *not* thread safe.
            HRESULT Initialize(wfc::IIterator<PairT*>* pSource)
            {
                Microsoft::WRL::ComPtr<Chunk> spChunk;
                HRESULT hr = Chunk::make(&spChunk);
                if (SUCCEEDED(hr))
                {
                    boolean fHasCurrent;
                    hr = pSource->get_HasCurrent(&fHasCurrent);

                    for (unsigned i = 0; SUCCEEDED(hr) && fHasCurrent && i < Chunk::sc_uChunkMax; ++i)
                    {
                        Microsoft::WRL::ComPtr<PairT> item;

                        hr = pSource->get_Current(&item);
                        if (SUCCEEDED(hr))
                        {
                            hr = spChunk->AddPair(item.Get());
                        }
                        if (SUCCEEDED(hr))
                        {
                            hr = pSource->MoveNext(&fHasCurrent);
                        }
                    }
                }
                if (SUCCEEDED(hr))
                {
                    // from this point on, the chunk is not modified.
                    _spChunk = spChunk;
                }
                return hr;
            }
            IFACEMETHOD(First)(_Outptr_result_maybenull_ wfc::IIterator<wfc::IKeyValuePair<K, V>*> **first)
            {
                *first = nullptr;

                HRESULT hr = S_OK;
                Microsoft::WRL::ComPtr<wfc::IIterator<wfc::IKeyValuePair<K, V>*>> spIter
                    = Microsoft::WRL::Make<ChunkElementIterator>(_spChunk.Get());

                if (!spIter)
                {
                    hr = E_OUTOFMEMORY;
                }
                if (SUCCEEDED(hr))
                {
                    *first = spIter.Detach();
                }
                return hr;
            }

            IFACEMETHOD(Lookup)(_In_opt_ K_abi key, _Out_ V_abi *value)
            {
                using XWinRT::AutoValue;

                HRESULT hr = S_OK;
                unsigned int uSize = _spChunk->get_Size();
                bool fIsEqual = false;

                for (unsigned int i = 0; SUCCEEDED(hr) && i < uSize; ++i)
                {
                    PairT* pPair = (*_spChunk.Get())[i];

                    AutoValue<K_abi, KeyLifetime> sKey;
                    hr = pPair->get_Key(sKey.OutParam());
                    if (SUCCEEDED(hr))
                    {
                        hr = _fnEquals(key, sKey.get(), &fIsEqual);
                    }

                    if (SUCCEEDED(hr) && fIsEqual)
                    {
                        hr = pPair->get_Value(value);
                        // We logically return here.
                        break;
                    }
                }
                if (SUCCEEDED(hr) && !fIsEqual)
                {
                    // item not found
                    // Do not OriginateError here because it is too noisy.  It is known
                    // that Lookup will often be called for keys that are not present in the map.
                    hr = E_BOUNDS;
                }
                if (FAILED(hr))
                {
                    // construct empty
                    ValueLifetime::Construct(value);
                }
                return hr;
            }
            /*propget*/ IFACEMETHOD(get_Size)(_Out_ unsigned int *size)
            {
                *size = _spChunk->get_Size();
                return S_OK;
            }
            IFACEMETHOD(HasKey)(_In_opt_ K_abi key, _Out_ boolean *found)
            {
                using XWinRT::AutoValue;

                *found = false;
                unsigned int uSize = _spChunk->get_Size();
                bool fEquals = false;

                HRESULT hr = S_OK;
                for (unsigned int i = 0; SUCCEEDED(hr) && i < uSize; ++i)
                {
                    PairT* pPair = (*_spChunk.Get())[i];

                    AutoValue<K_abi, KeyLifetime> sKey;
                    hr = pPair->get_Key(sKey.OutParam());
                    if (SUCCEEDED(hr))
                    {
                        hr = _fnEquals(key, sKey.get(), &fEquals);
                    }
                    if (SUCCEEDED(hr) && fEquals)
                    {
                        *found = true;
                        break;
                    }
                }
                return hr;
            }
            // NOTE:
            //  chunks are not splitable -- the parent (NaiveSplitView) is.
            IFACEMETHOD(Split)(_Outptr_result_maybenull_ wfc::IMapView<K,V> **firstPartition, _Outptr_result_maybenull_ wfc::IMapView<K,V> **secondPartition)
            {
                *firstPartition = nullptr;
                *secondPartition = nullptr;
                return S_OK;
            }
        private:
            // Bug workaround: Win8:71107, cannot use const with comptr
            Microsoft::WRL::ComPtr</*const*/ Chunk>     _spChunk;
            EqualityPredicate               _fnEquals;
        };

        class SplitIterator;
        friend SplitIterator;

        // INVARIANTS
        //  if _spCurrentIterator is false, then we need to move to the next
        //  item -- EnsureForced() does this.
        //
        //  post condition of EnsureForced: if _spCurrentIterator is
        //  false, we are at-end
        class SplitIterator WrlSealed : public
            Microsoft::WRL::RuntimeClass< wfc::IIterator<wfc::IKeyValuePair<K, V>*> >
        {
            static const wchar_t* z_get_rc_name()
            {
                // use the MIDLRT generated name string, which substitutes
                //   the proper metadata-name-representation of K and V
                return wfc::IIterator<wfc::IKeyValuePair<K,V>*>::z_get_rc_name_impl();
            }
            InspectableClass(z_get_rc_name(), (::BaseTrust))

        public:
            SplitIterator(NaiveSplitView* pView)
            : _spNextView(pView), _spCurrentIter(nullptr)
            {
            }

            /* propget */ IFACEMETHOD(get_Current)(_Outptr_result_maybenull_ wfc::IKeyValuePair<K, V>  **current)
            {
                auto lock = _comLock.Write();
                HRESULT hr = EnsureForced();
                if (SUCCEEDED(hr))
                {
                    if (!_spCurrentIter)
                    {
                        hr = XWinRT::detail::FailsWith(E_BOUNDS);
                    }
                }
                if (SUCCEEDED(hr))
                {
                    hr = _spCurrentIter->get_Current(current);
                }
                return hr;
            }

            /* propget */ IFACEMETHOD(get_HasCurrent)(_Out_ boolean *hasCurrent)
            {
                *hasCurrent = false;
                auto lock = _comLock.Write();
                HRESULT hr = EnsureForced();
                if (SUCCEEDED(hr))
                {
                    *hasCurrent = !!_spCurrentIter;
                }
                return hr;
            }

            IFACEMETHOD(MoveNext)(_Out_ boolean *hasCurrent)
            {
                *hasCurrent = false;
                auto lock = _comLock.Write();
                HRESULT hr = EnsureForced();
                if (SUCCEEDED(hr))
                {
                    if (!_spCurrentIter)
                    {
                        hr = XWinRT::detail::FailsWith(E_BOUNDS);
                    }
                }
                if (SUCCEEDED(hr))
                {
                    boolean fTempHasCurrent;
                    hr = _spCurrentIter->MoveNext(&fTempHasCurrent);
                    if (SUCCEEDED(hr))
                    {
                        if (!fTempHasCurrent)
                        {
                            _spCurrentIter = nullptr;
                            hr = EnsureForced();
                        }
                        if (SUCCEEDED(hr))
                        {
                            *hasCurrent = !!_spCurrentIter;
                        }
                    }
                }
                return hr;
            }
        private:
            // PURPOSE
            //  ensures an iterator is available, or at-end
            // THREAD SAFETY
            //  must be called under write lock
            HRESULT EnsureForced()
            {
                Microsoft::WRL::ComPtr<wfc::IIterator<PairT*>> spIter;

                HRESULT hr = S_OK;
                if (!_spCurrentIter)
                {
                    if (_spNextView)
                    {
                        hr = _spNextView->Force();
                        // get iterator from left half, and move to right half
                        //  as 'next'.
                        if (SUCCEEDED(hr))
                        {
                            hr = _spNextView->_spLeftHalf->First(&spIter);
                        }
                        boolean fHasCurrent;
                        if (SUCCEEDED(hr))
                        {
                            hr = spIter->get_HasCurrent(&fHasCurrent);
                            if (SUCCEEDED(hr))
                            {
                                if (fHasCurrent)
                                {
                                    _spCurrentIter.Swap(spIter);
                                    _spNextView = _spNextView->_spRightHalf;
                                }
                                else
                                {
                                    // no items remain
                                    _spCurrentIter = nullptr;
                                    _spNextView = nullptr;
                                }
                            }
                        }
                    }
                    else
                    {
                        // no more items remaining
                        hr = S_OK;
                    }
                }
                return hr;
            }
            XWinRT::ComLock                             _comLock;
            Microsoft::WRL::ComPtr<NaiveSplitView>      _spNextView;
            Microsoft::WRL::ComPtr<wfc::IIterator<PairT*>>   _spCurrentIter;
        };

    private:
        // PURPOSE
        //  constructor must be public for WRL to be able to call it,
        //  but should not generally be callable by clients.
        struct permission{};

    public:
        NaiveSplitView(const EqualityPredicate& fnEquals, permission)
        : _spIterator(nullptr), _fnEquals(fnEquals), _fInitialized(false)
        {
        }

    private:
        // PURPOSE
        //  private constructor, for right-half chain
        HRESULT Initialize(wfc::IIterator<PairT*>* pSource, unsigned int uSize)
        {
            _spIterator = pSource;
            _uSize = uSize;
            _fInitialized = true;

            return S_OK;
        }

        // PURPOSE
        //  public constructor
        HRESULT Initialize(_In_ wfc::IMapView<K,V>* pSource)
        {
            unsigned int uSize;
            Microsoft::WRL::ComPtr<wfc::IIterator<PairT*>> spIter;

            HRESULT hr = pSource->get_Size(&uSize);
            if (SUCCEEDED(hr))
            {
                Microsoft::WRL::ComPtr<wfc::IIterable<PairT*>> spIterable;
                hr = pSource->QueryInterface<wfc::IIterable<PairT*>>(&spIterable);
                if (hr == E_NOINTERFACE)
                {
                    // TYPE ERROR: IMapView must successfully cast to IIterable
                    XWINRT_ASSERT(false);
                    HRESULT hrNew = XWinRT::detail::FailsWith(E_UNEXPECTED);
                    ::RoTransformError(hr, hrNew, nullptr);
                    hr = hrNew;
                }
                if (SUCCEEDED(hr))
                {
                    hr = spIterable->First(&spIter);
                }
            }
            if (SUCCEEDED(hr))
            {
                hr = Initialize(spIter.Get(), uSize);
            }
            return hr;
        }

    public:
        // factory function
        static HRESULT Split(
            _In_            wfc::IMapView<K,V>*  pOriginal,
            _Outptr_result_maybenull_ wfc::IMapView<K,V>** ppFirstPartition,
            _Outptr_result_maybenull_ wfc::IMapView<K,V>** ppSecondPartition,
            const EqualityPredicate&        fnEquals
            )
        {
            *ppFirstPartition = nullptr;
            *ppSecondPartition = nullptr;
            HRESULT hr = S_OK;

            Microsoft::WRL::ComPtr<NaiveSplitView> sp = Microsoft::WRL::Make<NaiveSplitView>(fnEquals, permission());

            if (!sp)
            {
                hr = E_OUTOFMEMORY;
            }
            if (SUCCEEDED(hr))
            {
                hr = sp->Initialize(pOriginal);
            }
            if (SUCCEEDED(hr))
            {
                hr = sp->Split(ppFirstPartition, ppSecondPartition);
            }
            return hr;
        }
        // factory function
        static HRESULT Split(
            _In_            wfc::IMapView<K,V>*  pOriginal,
            _Outptr_result_maybenull_ wfc::IMapView<K,V>** ppFirstPartition,
            _Outptr_result_maybenull_ wfc::IMapView<K,V>** ppSecondPartition
            )
        {
            return Split(pOriginal, ppFirstPartition, ppSecondPartition, EqualityPredicate());
        }

        IFACEMETHOD(First)(_Outptr_result_maybenull_ wfc::IIterator<wfc::IKeyValuePair<K, V>*> **first)
        {
            *first = nullptr;

            HRESULT hr = EnsureInitialized();

            if (SUCCEEDED(hr))
            {
                auto spIter = Microsoft::WRL::Make<SplitIterator>(this);
                if (!spIter)
                {
                    hr = E_OUTOFMEMORY;
                }
                if (SUCCEEDED(hr))
                {
                    (*first = spIter.Get())->AddRef();
                }
            }
            return hr;
        }

        IFACEMETHOD(Lookup)(_In_opt_ K_abi key, _Out_ V_abi *value)
        {
            using XWinRT::AutoValue;

            AutoValue<V_abi, ValueLifetime> v;

            HRESULT hr = EnsureInitialized();

            if (SUCCEEDED(hr))
            {
                hr = IterateOnChunks(
                    // for each
                    [&](ChunkView* pCur,
                        bool* pfBreak) -> HRESULT
                    {
                        *pfBreak = false;
                        HRESULT hr = pCur->Lookup(key, v.OutParam());
                        if (SUCCEEDED(hr))
                        {
                            *pfBreak = true;
                        }
                        else if (hr == E_BOUNDS)
                        {
                            // handle by continuing search
                            hr = S_OK;
                        }
                        return hr;
                    },
                    // else
                    []() -> HRESULT {
                        // Do not OriginateError here because it is too noisy.  It is known
                        // that Lookup will often be called for keys that are not present in the map.
                         return E_BOUNDS;
                    });
            }
            if (SUCCEEDED(hr))
            {
                *value = v.get();
                v.Abandon();
            }
            else
            {
                ValueLifetime::Construct(value);
            }
            return hr;
        }
        /*propget*/ IFACEMETHOD(get_Size)(_Out_ unsigned int *size)
        {
            *size = 0;

            HRESULT hr = EnsureInitialized();

            if (SUCCEEDED(hr))
            {
                *size = _uSize;
            }
            return hr;
        }
        IFACEMETHOD(HasKey)(_In_opt_ K_abi key, _Out_ boolean *found)
        {
            *found = false;
            HRESULT hr = EnsureInitialized();
            if (SUCCEEDED(hr))
            {
                hr = IterateOnChunks(
                    // for each
                    [&](ChunkView* pCur,
                        bool* pfBreak) -> HRESULT
                    {
                        *pfBreak = false;
                        HRESULT hr = pCur->HasKey(key, found);
                        if (SUCCEEDED(hr) && *found)
                        {
                            *pfBreak = true;
                        }
                        return hr;
                    },
                    // else
                    []() -> HRESULT {
                         return S_OK;
                    });
            }
            return hr;
        }
        IFACEMETHOD(Split)(_Outptr_result_maybenull_ wfc::IMapView<K,V> **firstPartition, _Outptr_result_maybenull_ wfc::IMapView<K,V> **secondPartition)
        {
            unsigned int uSize;

            *firstPartition = nullptr;
            *secondPartition = nullptr;

            HRESULT hr = EnsureInitialized();
            if (SUCCEEDED(hr))
            {
                hr = Force();
            }
            if (SUCCEEDED(hr))
            {
                hr = _spLeftHalf->get_Size(&uSize);

                // nesting to silence an "uninitialized variable" warning on uSize
                if (SUCCEEDED(hr))
                {
                    if (uSize != 0)
                    {
                        if (_spRightHalf)
                        {
                            hr = _spRightHalf->get_Size(&uSize);
                        }
                        else
                        {
                            uSize = 0;
                        }
                    }
                }
                if (SUCCEEDED(hr))
                {
                    if (uSize != 0)
                    {
                        *firstPartition = static_cast<Microsoft::WRL::ComPtr<wfc::IMapView<K,V>>>(_spLeftHalf).Detach();
                        *secondPartition = static_cast<Microsoft::WRL::ComPtr<wfc::IMapView<K,V>>>(_spRightHalf).Detach();
                    }
                }
            }

            return hr;
        }

    private:
        HRESULT EnsureInitialized()
        {
            XWINRT_ASSERT(_fInitialized);
            return _fInitialized ? S_OK : XWinRT::detail::FailsWith(E_UNEXPECTED);
        }

        // PURPOSE
        //  helper function, automate iterating a function on the
        //  right tree.
        // Parameters:
        //  fnIter : (NativeSplitChunkView* pCur, bool* pfBreak) -> HRESULT
        //      called each loop iteration
        //          pCur : current chunk
        //          pfBreak : write 'true' as result to exit the loop
        //  fnElse : () -> HRESULT
        //      called in the event that the loop terminates without
        //      an *pfBreak = true condition.
        template <class FnIter, class FnElse>
        HRESULT IterateOnChunks(FnIter fnIter, FnElse fnElse)
        {
            // iterate on right half
            NaiveSplitView* pCur = this;

            bool fBreak = false;
            HRESULT hr = S_OK;

            while (SUCCEEDED(hr) && !fBreak)
            {
                if (!pCur || pCur->IsEmpty())
                {
                    // end of chain reached.
                    hr = fnElse();
                    break;
                }

                hr = pCur->Force();

                if (SUCCEEDED(hr))
                {
                    hr = fnIter(pCur->_spLeftHalf.Get(), &fBreak);
                }
                if (SUCCEEDED(hr))
                {
                    pCur = pCur->_spRightHalf.Get();
                }
            }

            return hr;
        }

        bool IsEmpty() const
        {
            return _uSize == 0;
        }

        // PURPOSE
        //  ensures left and right halves have been created
        // THREAD SAFETY
        //  locks internally
        HRESULT Force()
        {
            HRESULT hr = S_OK;
            auto acquire = _lock.Write();
            if (_spIterator)
            {
                if (auto spLeftHalf = Microsoft::WRL::Make<ChunkView>(_fnEquals))
                {
                    hr = spLeftHalf->Initialize(_spIterator.Get());
                    if (SUCCEEDED(hr))
                    {
                        if (auto spRightHalf = Microsoft::WRL::Make<NaiveSplitView>(_fnEquals, permission()))
                        {
                            unsigned int uLeftSize, uRightSize;
                            hr = spLeftHalf->get_Size(&uLeftSize);
                            if (SUCCEEDED(hr))
                            {
                                uRightSize = _uSize - uLeftSize;
                                // if either the left partition is empty, or
                                // the right partition is expected to be empty, terminate.
                                if (uLeftSize != 0 && uRightSize != 0)
                                {
                                    hr = spRightHalf->Initialize(_spIterator.Get(), uRightSize);
                                }
                                else
                                {
                                    spRightHalf = nullptr;
                                }
                            }
                            if (SUCCEEDED(hr))
                            {
                                _spLeftHalf = spLeftHalf;
                                _spRightHalf = spRightHalf;
                                _spIterator = nullptr;
                            }
                        }
                        else
                        {
                            hr = E_OUTOFMEMORY;
                        }
                    }
                }
                else
                {
                    hr = E_OUTOFMEMORY;
                }
            }
            else
            {
                // _spIterator == nullptr indicates we've been lazily
                //   initialized already
                hr = S_OK;
            }
            return hr;
        }

        XWinRT::SRWLock                     _lock;
        Microsoft::WRL::ComPtr<ChunkView>           _spLeftHalf;
        Microsoft::WRL::ComPtr<NaiveSplitView>      _spRightHalf;
        Microsoft::WRL::ComPtr<wfc::IIterator<PairT*>>   _spIterator;
        unsigned int                        _uSize;
        EqualityPredicate                   _fnEquals;
        bool                                _fInitialized;
    };

    template <
        class K,
        class V,
        class Hasher,
        class EqualityPredicate,
        class KeyLifetime,
        class ValueLifetime,
        class HashMapOptions
    >
    IFACEMETHODIMP HashMap<K, V, Hasher, EqualityPredicate, KeyLifetime, ValueLifetime, HashMapOptions>::View
        ::Split(_Outptr_result_maybenull_ wfc::IMapView<K,V> **firstPartition, _Outptr_result_maybenull_ wfc::IMapView<K,V> **secondPartition)
    {
        *firstPartition = nullptr;
        *secondPartition = nullptr;

        HRESULT hr = EnsureVersionMatches();
        if (SUCCEEDED(hr))
        {
            hr = NaiveSplitView<K, V, EqualityPredicate, KeyLifetime, ValueLifetime, HashMapOptions>::Split(
                this,
                firstPartition,
                secondPartition);
        }
        return hr;
    }

}}}} // ::Windows::Foundation::Collections::Internal
