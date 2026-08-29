// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#include "precomp.h"
#include <SatelliteBase\inc\XamlTypeInfo.h>
#include <ExternalDependency.h>

namespace Private {
    PROVIDE_DEPENDENCY(XamlRuntimeType);

     _Check_return_ HRESULT
    XamlRuntimeType::ActivateInstance(
        _In_ UINT16,
        _Outptr_ IInspectable**) const
    {
        return E_NOT_FOUND;
    }

    _Check_return_ HRESULT
    XamlRuntimeType::SetValue(
        _In_ UINT16, 
        _In_ IInspectable*,
        _In_ IInspectable*) const
    {
        return E_NOT_FOUND;
    }
    
    _Check_return_ HRESULT
    XamlRuntimeType::GetValue(
        _In_ UINT16 typeLabel,
        _In_ IInspectable* instance,
        _Outptr_result_maybenull_ IInspectable** value) const
    {
        return E_NOT_FOUND;
    }

    _Check_return_ HRESULT
    XamlRuntimeType::BoxEnum(
        _In_ UINT16 ,
        _In_ UINT32 ,
        _Outptr_ IInspectable **) const
    {
        return E_NOT_FOUND;
    }

    _Check_return_ HRESULT
        XamlRuntimeType::AddToVector(
            _In_ UINT16 /*addToVectorId*/,
            _In_ IInspectable* /*instance*/,
            _In_ IInspectable* /*value*/) const
    {
        return E_NOT_FOUND;
    }

    _Check_return_ HRESULT
        XamlRuntimeType::AddToMap(
            _In_ UINT16 /*addToMapId*/,
            _In_ IInspectable* /*instance*/,
            _In_ IInspectable* /*key*/,
            _In_ IInspectable* /*value*/) const
    {
        return E_NOT_FOUND;
    }
    
    _Check_return_ HRESULT
    XamlRuntimeType::EnsureDependencyProperties(_In_ UINT16) const
    {
        return S_OK;
    }

    void
    XamlRuntimeType::ResetDependencyProperties() const
    {
    }
}

