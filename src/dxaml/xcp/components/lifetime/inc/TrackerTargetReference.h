// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#pragma once

#include "ReferenceTrackerInterfaces.h"
#include "AutoPeg.h"

namespace ctl
{
    class WeakReferenceSourceNoThreadId;
}

namespace DirectUI
{
    //+------------------------------------------------------------------------------
    //
    //  TrackerTargetReference
    //
    //  This wraps a reference to what might be an IReferenceTrackerTarget.  If it is, special
    //  ref-counting is used.
    //
    //+------------------------------------------------------------------------------

    class TrackerTargetReference
    {
    private:
        IUnknown* m_value;

        // This structure is never instantiated in native apps and, in managed apps, it's typically
        // instantiated for about 4-5% of the references.
        // If the CLR allowed us to query for IReferenceTrackerTarget during GC, we may not need it.
        struct ReferenceTrackerTargetData
        {
            xaml_hosting::IReferenceTrackerInternal* ReferenceTracker;
            IReferenceTrackerTarget* TrackerTarget;
        };

        // Truth table for m_trackerData usage:
        //  m_isTrackerInternal &&  m_isManagedReference: m_trackerData's type is ReferenceTrackerTargetData*
        //  m_isTrackerInternal && !m_isManagedReference: m_trackerData's type is IReferenceTrackerInternal*
        // !m_isTrackerInternal &&  m_isManagedReference: m_trackerData's type is IReferenceTrackerTarget*
        // !m_isTrackerInternal && !m_isManagedReference: m_trackerData is not used and should be null
        union
        {
            ReferenceTrackerTargetData* m_trackerData;
            IReferenceTrackerTarget* m_trackerTarget;
            xaml_hosting::IReferenceTrackerInternal* m_referenceTracker;
        };

#if DBG
    public:
        bool m_fHasBeenWalked : 1;
        bool m_fStatic : 1;
        IDXamlCore *m_pCore;
        ctl::WeakReferenceSourceNoThreadId *m_pOwner;
#endif

        // See if a value is set (this doesn't assert IsValueSafeToUse)
        bool IsSet() const
        {
            return m_value != nullptr;
        }

        TrackerTargetReference& operator= (const TrackerTargetReference& other) = default;

    protected:
        TrackerTargetReference();
        TrackerTargetReference(const TrackerTargetReference& other) = delete;
        TrackerTargetReference(TrackerTargetReference&& other);

        // Note: This method is safe to call under the lock 
        // if this object is empty
        TrackerTargetReference& Assign(
            _In_ TrackerTargetReference&& other);

        bool operator==(const TrackerTargetReference& other) const;
        bool operator!=(const TrackerTargetReference& other) const;

        void Set(
            _In_ IUnknown* const pValue
#if DBG
            , bool fStatic = false
#endif
            );

        IUnknown* Get() const;

        //
        // Call this method to peg the target of this TrackerPtr, if the target is still reachable.
        // If it's not reachable, the returned AutoPeg behaves as a nullptr.  When the returned
        // AutoPeg falls out of scope it will release the peg.
        //
        // This is primarily useful during destruction of an object, where it wants to clean its state
        // out of referenced objects, but needn't/shouldn't if those objects are not reachable
        // (i.e. they have been GC'd).
        //
        ctl::AutoPeg<xaml_hosting::IReferenceTrackerInternal, true> TryMakeAutoPeg();

        // Two versions of TryGetSafeReference.  The original returns false if the value is null, the second returns true.
        bool TryGetSafeReference(_In_ REFIID riid, _Outptr_result_maybenull_ void** ppSafeReference) const;
        bool TryGetSafeReferenceNullValueReturnsTrue(_In_ REFIID riid, _Outptr_result_maybenull_ void** ppSafeReference) const;

        xaml::IDependencyObject* GetAsDO() const;
        xaml_hosting::IReferenceTrackerInternal* GetAsReferenceTrackerUnsafe() const;

        CDependencyObject* GetAsCoreDO() const;

        ~TrackerTargetReference();

        void Clear();

        // This version of Clear does  not make any calls out.  This enables the fields to be cleared while under the lock, without
        // risking deadlock; the calls out could block on GC, which on another thread is in ReferenceTrackerManager trying to take the lock.
        void ClearWithNoCallouts(
            _Out_ IReferenceTrackerTarget **ppPreviousTrackerTarget,
            _Out_ xaml_hosting::IReferenceTrackerInternal **ppPreviousTrackerInternal,
            _Out_ DependencyObject **ppPreviousDO,
            _Out_ IUnknown **ppPreviousValue);


    protected:
        bool m_trackerPeg : 1;
        bool m_isDO : 1;
        bool m_isRegistered : 1;
        bool m_extraExpectedRef : 1; // An extra ctl::release_expected is necessary in Clear()
        bool m_isTrackerInternal : 1;
        bool m_isManagedReference : 1;

#if DBG
        INSTRUCTION_ADDRESS m_frameAddresses[40];
#endif

        // PReferenceTrackerInternal
        void ReferenceTrackerWalk(EReferenceTrackerWalkType walkType);
        void ClearTrackerPeg();
#if DBG
        bool IsValueSafeToUse() const;
#endif
        bool IsValueSafeToUse(_In_opt_ ctl::WeakReferenceSourceNoThreadId* pOwner) const;

    private:
        void Initialize();

        IUnknown* GetNoMptStress() const;

        bool IsRegistered() const;

#if DBG
        // In Debug builds these functions do more work that have dependencies on other headers which need to be separated
        // into their own .cpp file for dependency reasons.
        void Register();
        void Unregister();
#else
        void Register() { m_isRegistered = true; }
        void Unregister() { m_isRegistered = false; }
#endif

        IUnknown* GetValue() const;
        IReferenceTrackerTarget* GetTrackerTarget() const;
        xaml_hosting::IReferenceTrackerInternal* GetReferenceTracker() const;

        void SetRawValue(_In_ IUnknown* value, _In_ IReferenceTrackerTarget* trackerTarget, _In_ xaml_hosting::IReferenceTrackerInternal* referenceTracker);
        void ClearRawValue();

#if DBG
        void Register(ctl::WeakReferenceSourceNoThreadId* pOwner);
        ctl::WeakReferenceSourceNoThreadId* GetOwner() const;
#endif

        // TrackerTargetReference was intended to go away, and be folded into TrackerPtr.
        // So no one should be using TrackerTargetReference directly.  But there's some old code
        // in a couple of classes that hasn't been updated.
        friend class ctl::WeakReferenceSourceNoThreadId;
        friend class DirectUI::ReferenceTrackerManager;

        template <typename T>
        friend class TrackerPtr;

    };
}
