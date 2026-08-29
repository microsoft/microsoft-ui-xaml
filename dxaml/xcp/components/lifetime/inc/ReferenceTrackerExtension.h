// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#pragma once

#include <Microsoft.UI.Xaml.hosting.referencetracker.h>
#include <abi/xaml_abi.h>

//
// This file contains the interfaces used to allow MUXP controls
// to hook into MUX's internal reference tracking mechanism.
//

namespace DirectUI
{
    interface IDXamlCore;
    enum EReferenceTrackerWalkType;
}

XAML_ABI_NAMESPACE_BEGIN namespace Microsoft { namespace UI { namespace Xaml { namespace Hosting {

    struct __declspec(novtable) ITrackerPtrWrapper
    {
        virtual _Check_return_ HRESULT Set(_In_ IUnknown* pValue) = 0;
        virtual IUnknown* Get() = 0;
        virtual bool TryGetSafeReference(_Outptr_ IUnknown** ppValue) = 0;
        virtual void ReferenceTrackerWalk(_In_ INT walkType) = 0;

        virtual ~ITrackerPtrWrapper() = 0 { };
    };

    struct __declspec(novtable) IReferenceTrackerGCLock
    {
        virtual INT64 AcquireLock() = 0;
        virtual void ReleaseLock(INT64 token) = 0;
    };

    enum class RefCountType : bool
    {
        Actual,
        Expected
    };

    enum class RefCountUpdateType : bool
    {
        Add,
        Remove
    };

    MIDL_INTERFACE("02DD3AD0-B9DE-4B55-A0C3-507235EAE8EA")
    IReferenceTrackerInternal :
        public IReferenceTracker
    {
        virtual void PrepareForReferenceWalking() = 0;
        virtual void OnReferenceTrackingProcessed(_In_ DirectUI::IDXamlCore* pCore) = 0;
        virtual void ClearReferenceTrackerPeg() = 0;

        virtual bool IsReferencedByTrackerSource() = 0;
        virtual bool IsReferenceTrackerPegSet() = 0;
        virtual bool IsReachable() = 0;
        virtual bool IsPegged(bool isRefCountPegged) = 0;
        virtual bool IsNativeAndComposed() = 0;

        virtual bool HasBeenWalked(_In_ DirectUI::EReferenceTrackerWalkType walkType) = 0;

        virtual void UpdatePeg(bool peg) = 0;

        virtual ULONG GetRefCount(RefCountType refCountType) = 0;
        virtual void UpdateExpectedRefCount(RefCountUpdateType updateType) = 0;
        virtual bool IsAlive() = 0;
        virtual bool ImplicitPegAllowed() = 0;
        virtual void ResetLastFindWalkId() = 0;

        // PReferenceTrackerInternal portion
        // We want to avoid double inheritance to save a vtable slot.
        virtual bool ReferenceTrackerWalk(_In_ DirectUI::EReferenceTrackerWalkType walkType, _In_ bool fIsRoot = false) = 0;

        // Portion of IReferenceTrackerInternal that MUXP calls.
        virtual _Check_return_ HRESULT NewTrackerPtrWrapper(_Outptr_ ITrackerPtrWrapper** ppValue) = 0;
        virtual _Check_return_ HRESULT DeleteTrackerPtrWrapper(_In_ ITrackerPtrWrapper* pValue) = 0;
        virtual IReferenceTrackerGCLock* GetGCLock() = 0;
        virtual _Check_return_ HRESULT SetExtensionInterfaces(_In_ IReferenceTrackerExtension* pExtension, _In_ IReferenceTrackerInternal* pOverrides) = 0;

        // Portion of IReferenceTrackerInternal that MUXP overrides.
        virtual void OnReferenceTrackerWalk(_In_ INT walkType) = 0;

        virtual _Check_return_ HRESULT EndShutdown() = 0;

        // In order to support fre/chk mismatch between MUX and MUXP,
        // we put these DBG-only methods at the end of the interface.
#if DBG
        virtual bool ShouldSkipTrackerLeakCheck() = 0;
        virtual void ClearSkipTrackerLeakCheck() = 0;
        virtual void TouchDebugMemory() = 0;
#endif
    };

} } } } XAML_ABI_NAMESPACE_END