// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#pragma once

#include "ComposingTrackerTargetWrapper.h"
#include "ComposingTrackerExtensionWrapper.h"

namespace DirectUI
{
    // Holds fields that are used in WinRT aggregation.
    // Those fields are managed by the implementation in ComposingTrackerTargetWrapper and
    // ComposingTrackerExtensionWrapper.
    struct ComposingTrackerWrapper
    {
        ~ComposingTrackerWrapper()
        {
            if (m_composingTrackerTarget)
            {
#if DBG
                // The ref count here tracks only our refs to the target. The count changes via 
                // calls to ComposingTrackerTargetWrapper's AddRefTarget/ReleaseTarget throughout the 
                // life of the associated DependencyObject. The release here should be the last one, 
                // otherwise the target could either leak or prematurely destruct.
                if (m_composingTrackerTargetRefCount != 1)
                {
                    FAIL_FAST();
                }
                m_composingTrackerTargetRefCount--;
#endif
                // Since the object is being destructed, we can release the
                // jupiter ref on the Outer CCW which we kept to ensure that 
                // we could unpeg it when this DO was unreachable but still in 
                // the release queue.
                // Reference Win8 Bug: 643960
                m_composingTrackerTarget->ReleaseFromReferenceTracker();
            }
        }

        // If the WeakReferenceSourceNoThreadId instance is managed, this points to the CCW.
        IReferenceTrackerTarget* m_composingTrackerTarget = nullptr;

#if DBG
        int m_composingTrackerTargetRefCount = 0;
#endif

        // If the WeakReferenceSourceNoThreadId is a MUXP instance, these pointers
        // reference the outer most MUXP object.
        // Note: We are only allowed to call OnReferenceTrackerWalk on m_composingTrackerOverridesNoRef.
        //       See comments in ReferenceTrackerRuntimeClass for more details.
        ::IReferenceTrackerExtension* m_composingTrackerExtensionNoRef = nullptr;
        xaml_hosting::IReferenceTrackerInternal* m_composingTrackerOverridesNoRef = nullptr;
    };
}