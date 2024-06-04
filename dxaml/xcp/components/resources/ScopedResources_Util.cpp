// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#include "precomp.h"

#include <DependencyObject.h>
#include <CDependencyObject.h>
#include <xstring_ptr.h>

namespace Resources { namespace ScopedResources
{
    _Check_return_ HRESULT GetRuntimeClassNameFromCoreObject(
        _In_ const CDependencyObject* const coreObject,
        _Out_ xstring_ptr* runtimeClassName)
    {
        *runtimeClassName = xstring_ptr::NullString();

        DirectUI::DependencyObject* dxPeer = coreObject->GetDXamlPeer();
        wrl_wrappers::HString str;
        IFC_RETURN(ctl::iinspectable_cast(dxPeer)->GetRuntimeClassName(str.ReleaseAndGetAddressOf()));
        IFC_RETURN(xstring_ptr::CloneRuntimeStringHandle(str.Get(), runtimeClassName));

        return S_OK;
    }
} }