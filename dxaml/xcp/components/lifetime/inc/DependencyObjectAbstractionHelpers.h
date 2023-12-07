// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#pragma once

#include "ReferenceTrackerExtension.h"

class CDependencyObject;

namespace ctl
{
    class WeakReferenceSourceNoThreadId;
}

namespace DirectUI
{
    class DependencyObject;

    // DependencyObject is not an isolated component yet.
    // Until it becomes one, we need this helper class to help us cast between
    // xaml::IDependencyObject*, DependencyObject*, WeakReferenceSourceNoThreadId*
    // and IReferenceTrackerInternal* as well as retrieve the DO handle.
    // Those methods are implemented in the dxaml layer because only there we know about
    // DependencyObject's memory layout.
    
    // PERF NOTE:
    // The methods below gets inlined by LTCG. If you add new ones, make sure that's the case
    // for the added helpers.

    /* static */
    class DependencyObjectAbstractionHelpers
    {
    public:
        static DependencyObject* IDOtoDO(_In_ xaml::IDependencyObject* ido);
        static xaml::IDependencyObject* DOtoIDO(_In_ DependencyObject* obj);
        static DependencyObject* IRTItoDO(_In_ xaml_hosting::IReferenceTrackerInternal* irti);
        static xaml::IDependencyObject* IRTItoIDO(_In_ xaml_hosting::IReferenceTrackerInternal* ido);
        static xaml_hosting::IReferenceTrackerInternal* DOtoIRTI(_In_ DependencyObject* obj);
        static ::ctl::WeakReferenceSourceNoThreadId* DOtoWRSNTI(_In_ DependencyObject* obj);
        static ::ctl::WeakReferenceSourceNoThreadId* IDOtoWRSNTI(_In_ xaml::IDependencyObject* obj);
        
        static CDependencyObject* GetHandle(_In_ DependencyObject* obj);
    };
}
