// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#include "precomp.h"
#include <CDependencyObject.h>
#include "DependencyObjectAbstractionHelpers.h"


class DCompTreeHost;

namespace DirectUI
{

DependencyObject* DependencyObjectAbstractionHelpers::IDOtoDO(_In_ xaml::IDependencyObject* ido)
{
    return nullptr;
}

xaml::IDependencyObject* DependencyObjectAbstractionHelpers::DOtoIDO(_In_ DirectUI::DependencyObject*)
{
    return nullptr;
}

xaml_hosting::IReferenceTrackerInternal* DependencyObjectAbstractionHelpers::DOtoIRTI(_In_ DependencyObject* obj)
{
    return nullptr;
}

DependencyObject* DependencyObjectAbstractionHelpers::IRTItoDO(_In_ xaml_hosting::IReferenceTrackerInternal* irti)
{
    return nullptr;
}

xaml::IDependencyObject* DependencyObjectAbstractionHelpers::IRTItoIDO(_In_ xaml_hosting::IReferenceTrackerInternal* irti)
{
    return nullptr;
}

::ctl::WeakReferenceSourceNoThreadId* DependencyObjectAbstractionHelpers::DOtoWRSNTI(_In_ DependencyObject* obj)
{
    return nullptr;
}

::ctl::WeakReferenceSourceNoThreadId* DependencyObjectAbstractionHelpers::IDOtoWRSNTI(_In_ xaml::IDependencyObject* ido)
{
    return nullptr;
}

CDependencyObject* DependencyObjectAbstractionHelpers::GetHandle(_In_ DependencyObject* obj)
{
    return nullptr;
}

}
