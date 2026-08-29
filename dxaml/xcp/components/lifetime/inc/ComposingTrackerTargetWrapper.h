// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#pragma once

#include "ReferenceTrackerInterfaces.h"
#include "ReferenceTrackerManager.h"

namespace ctl
{
    class WeakReferenceSourceNoThreadId;
}

namespace DirectUI
{
    //+--------------------------------------------------------------------------------
    //
    //  ComposingTrackerTargetWrapper
    //
    //  This class is used to safely wrap the controlling unknown of a DependencyObject, for the case
    //  where that an outer object is an IReferenceTrackerTarget.  Special care is necessary, because
    //  that object can get semi-deleted, to the point where it cannot respond to QueryInterface.
    //
    //+--------------------------------------------------------------------------------

    /* static */ class ComposingTrackerTargetWrapper final
    {
    public:

        static _Check_return_ HRESULT Ensure(_In_ ctl::WeakReferenceSourceNoThreadId* tracker);

        // Special IReferenceTrackerTarget form of AddRef
        // This special count is necessary for the target to understand what to expect during tracking.
        static ULONG AddRefTarget(_In_ const ctl::WeakReferenceSourceNoThreadId* tracker);

        // Special IReferenceTrackerTarget form of Release
        // This special count is necessary for the target to understand what to expect during tracking.
        static ULONG ReleaseTarget(_In_ const ctl::WeakReferenceSourceNoThreadId* tracker);

        static void ReferenceTrackerWalk(
            _In_ ctl::WeakReferenceSourceNoThreadId* tracker,
            _In_ EReferenceTrackerWalkType walkType)
        {
            // Usually, this method is called only during GC.
            // The exception being when we call ReferenceTrackerManager::RunValidation in CHK builds.
            // For that reason, we are calling GetTrackerTargetNoAssertIfEntered instead of GetTrackerTarget.
            // We should be careful and avoid potentially blocking call outs on the CCW.

            IReferenceTrackerTarget* trackerTarget;

#ifdef DBG
            if (walkType == RTW_TrackerPtrTest)
            {
                trackerTarget = GetTrackerTargetNoAssertIfEntered(tracker);
            }
            else
            {
#endif
                trackerTarget = GetTrackerTarget(tracker);
#ifdef DBG
            }
#endif

            if (trackerTarget)
            {
                ReferenceTrackerManager::ReferenceTrackerWalk(walkType, trackerTarget);
            }
        }

        static bool IsTrackerTarget(_In_ ctl::WeakReferenceSourceNoThreadId* tracker);

        static IReferenceTrackerTarget* GetTrackerTarget(_In_ const ctl::WeakReferenceSourceNoThreadId* tracker);

    private:
        static IReferenceTrackerTarget* GetTrackerTargetNoAssertIfEntered(_In_ const ctl::WeakReferenceSourceNoThreadId* tracker);
    };
}