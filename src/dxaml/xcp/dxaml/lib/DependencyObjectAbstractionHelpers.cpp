// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#include "precomp.h"
#include "DependencyObjectAbstractionHelpers.h"
#include "DependencyObject.h"
#include "WeakReferenceSourceNoThreadId.h"

using namespace DirectUI;

/*static*/
DependencyObject* DependencyObjectAbstractionHelpers::IDOtoDO(
    _In_ xaml::IDependencyObject* ido)
{
    return static_cast<DependencyObject*>(ido);
}

/*static*/
xaml::IDependencyObject* DependencyObjectAbstractionHelpers::DOtoIDO(_In_ DependencyObject* obj)
{
    return obj;
}

/*static*/
DependencyObject* DependencyObjectAbstractionHelpers::IRTItoDO(
    _In_ xaml_hosting::IReferenceTrackerInternal* irti)
{
    auto ptr = ctl::impl_cast<ctl::WeakReferenceSourceNoThreadId>(irti);
    return static_cast<DependencyObject*>(ptr);
}

/*static*/
xaml::IDependencyObject* DependencyObjectAbstractionHelpers::IRTItoIDO(
    _In_ xaml_hosting::IReferenceTrackerInternal* irti)
{
    return IRTItoDO(irti);
}

/*static*/
xaml_hosting::IReferenceTrackerInternal* DependencyObjectAbstractionHelpers::DOtoIRTI(
    _In_ DependencyObject* obj)
{
    return ctl::interface_cast<xaml_hosting::IReferenceTrackerInternal>(obj);
}

/*static*/
::ctl::WeakReferenceSourceNoThreadId* DependencyObjectAbstractionHelpers::DOtoWRSNTI(
    _In_ DependencyObject* obj)
{
    return static_cast<::ctl::WeakReferenceSourceNoThreadId*>(obj);
}

/*static*/
::ctl::WeakReferenceSourceNoThreadId* DependencyObjectAbstractionHelpers::IDOtoWRSNTI(
    _In_ xaml::IDependencyObject* ido)
{
    return static_cast<DependencyObject*>(ido);
}

/*static*/
CDependencyObject* DependencyObjectAbstractionHelpers::GetHandle(_In_ DependencyObject* obj)
{
    return obj->GetHandle();
}
