// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#include "precomp.h"
#include "DependencyPropertyProxy.g.h"
#include "TargetPropertyPath.g.h"

using namespace DirectUI;
using namespace xaml;

_Check_return_ HRESULT 
TargetPropertyPathFactory::CreateInstanceImpl(
    _In_ xaml::IDependencyProperty* pTargetProperty, 
    _Outptr_ xaml::ITargetPropertyPath** ppInstance)
{
    ctl::ComPtr<TargetPropertyPath> spTargetPropertyPath;
    ctl::ComPtr<DependencyPropertyProxy> spProxy;

    IFCPTR_RETURN(pTargetProperty);
    IFCPTR_RETURN(ppInstance);

    IFC_RETURN(ctl::make(&spTargetPropertyPath));
    
    IFC_RETURN(DependencyPropertyProxy::CreateObject(pTargetProperty, &spProxy));
    IFC_RETURN(spTargetPropertyPath->SetValueCore(MetadataAPI::GetDependencyPropertyByIndex(KnownPropertyIndex::TargetPropertyPath_CachedStyleSetterProperty), spProxy.Cast<xaml::IDependencyObject>()));

    *ppInstance = spTargetPropertyPath.Detach();

    return S_OK;
}