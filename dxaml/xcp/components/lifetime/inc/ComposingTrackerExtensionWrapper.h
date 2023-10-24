// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#pragma once

#include "ReferenceTrackerInterfaces.h"
#include "ReferenceTrackerExtension.h"

namespace ctl
{
    class WeakReferenceSourceNoThreadId;
}

namespace DirectUI
{
    //
    // If a tracker extension is available, this wrapper class will call into it for operations
    // where tracker extensions should be involved. Otherwise, it will just call into the WeakReferenceSourceNoThreadId
    // implementation.
    //
    /* static */ class ComposingTrackerExtensionWrapper final
    {
    public:
        static void OnReferenceTrackerWalk(_In_ ctl::WeakReferenceSourceNoThreadId* tracker, _In_ EReferenceTrackerWalkType walkType);

        static bool IsComposed(_In_ ctl::WeakReferenceSourceNoThreadId* tracker);

        static ULONG GetActualRefCount(_In_ ctl::WeakReferenceSourceNoThreadId* tracker);

        static _Check_return_ HRESULT SetExtensionInterfaces(
            _In_ ctl::WeakReferenceSourceNoThreadId* tracker,
            _In_ ::IReferenceTrackerExtension* extension,
            _In_opt_ xaml_hosting::IReferenceTrackerInternal* overrides);

    private:
        static ::IReferenceTrackerExtension* GetComposingTrackerExtension(_In_ ctl::WeakReferenceSourceNoThreadId* tracker);
        static xaml_hosting::IReferenceTrackerInternal* GetComposingTrackerOverrides(_In_ ctl::WeakReferenceSourceNoThreadId* tracker);
    };
}