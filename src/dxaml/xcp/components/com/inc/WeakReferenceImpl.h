// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#pragma once

#include <wil\common.h>
#include "ComTemplates.h"

namespace ctl
{
    class ComBase;

    namespace Details
    {
        class WeakReferenceImpl final
            : public ctl::implements <IWeakReference>
        {
            friend class ctl::ComBase;

        public:

            explicit WeakReferenceImpl(_In_ ComBase *pSourceRef)
                // Set ref count to 2 to avoid unnecessary interlocked increment operation while returning
                // WeakReferenceImpl from GetWeakReference method. One reference is hold by the object the second is hold
                // by the caller of GetWeakReference method.
                : implements(2 /* initialRefCount */)
                , m_pSourceRef(pSourceRef)
                , m_cSourceRef(1)
            {
                XCP_WEAK(&m_pSourceRef);
            }

        public:
            // IWeakReference methods
            IFACEMETHODIMP Resolve(_In_ REFIID iid, _Outptr_opt_ IInspectable **objectReference) override;

        public:

            ULONG AddRefSource()
            {
                return InterlockedIncrement(&m_cSourceRef);
            }

            ULONG ReleaseSource()
            {
                LONG cSourceRef = InterlockedDecrement(&m_cSourceRef);
                if (cSourceRef == 0)
                {
                    Clear();
                }
                return cSourceRef;
            }

            ULONG GetSourceRefCount() const
            {
                return m_cSourceRef;
            }

            void Clear()
            {
                m_pSourceRef = nullptr;
            }

            void Resurrect(_In_ ComBase* pSourceRef)
            {
                m_pSourceRef = pSourceRef;
            }

        private:
            // A bunch of helper methods to keep the WRL code we use for synchronisation as it is.
            unsigned long IncrementStrongReference() WI_NOEXCEPT { return AddRefSource(); }
            unsigned long DecrementStrongReference() WI_NOEXCEPT { return ReleaseSource(); }
            unsigned long GetStrongReferenceCount() const WI_NOEXCEPT { return GetSourceRefCount(); }
            void SetStrongReference(_In_ unsigned long value) WI_NOEXCEPT { m_cSourceRef = value; }

        private:
            ULONG m_cSourceRef;
            ComBase *m_pSourceRef;    // This is the weak reference
        };

        // To support storing, encoding and decoding reference-count/pointers regardless of the target platform.
        // In a RuntimeClass, the refCount_ member can mean either
        //    1. actual reference count
        //    2. pointer to the weak reference object which holds the strong count
        // The member
        //    1. If it is a count, the most significant bit will be OFF
        //    2. If it is an encoded pointer to the weak reference, the most significant bit will be turned ON
        // To test which mode it is
        //    1. Test for negative
        //    2. If it is, it is an encoded pointer to the weak reference
        //    3. If it is not, it is the actual reference count
        // To yield the encoded pointer
        //    1. Test the value for negative
        //    2. If it is, shift the value to the left and cast it to a WeakReferenceImpl*
        //
        const UINT_PTR EncodeWeakReferencePointerFlag = static_cast<UINT_PTR>(1) << ((sizeof(UINT_PTR) * 8) - 1);

        union ReferenceCountOrWeakReferencePointer
        {
            // Represents the count when it is a count (in practice only the least sigficant 4 bytes)
            UINT_PTR refCount;
            // Pointer size, *signed* to help with ease of casting and such
            INT_PTR rawValue;
            // The hint that this could also be a pointer
            void* ifHighBitIsSetThenShiftLeftToYieldPointerToWeakReference;
        };

        // Helper methods to test, decode and decode the different representations of ReferenceCountOrWeakReferencePointer
        inline bool IsValueAPointerToWeakReference(INT_PTR value)
        {
            return value < 0;
        }

        inline INT_PTR EncodeWeakReferencePointer(WeakReferenceImpl* value)
        {
            ASSERT((reinterpret_cast<INT_PTR>(value) & EncodeWeakReferencePointerFlag) == 0);
            return ((reinterpret_cast<INT_PTR>(value) >> 1) | EncodeWeakReferencePointerFlag);
        }

        inline WeakReferenceImpl* DecodeWeakReferencePointer(INT_PTR value)
        {
            ASSERT(IsValueAPointerToWeakReference(value));
            return reinterpret_cast<WeakReferenceImpl*>(value << 1);
        }

        inline WeakReferenceImpl* CreateWeakReference(_In_ ComBase* sourceRef)
        {
            return new WeakReferenceImpl(sourceRef);
        }
    }
}