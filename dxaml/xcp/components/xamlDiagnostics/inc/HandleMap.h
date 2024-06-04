// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#pragma once

#include "includes.h"
#include <map>
#include "diagnosticsInterop\inc\PropertyChainIterator.h"
#include "XamlOM.WinUI.Private.h"

namespace Windows { namespace UI { namespace Xaml { namespace Tests { namespace XamlDiagnostics {
    class HandleMapUnitTests;
} } } } }

class CDependencyObject;

class HandleMap final
{
    friend class ::Windows::UI::Xaml::Tests::XamlDiagnostics::HandleMapUnitTests;

public:
    // A map of Instance Handle of an element to a pointer to the element
    using ElementHandleMap = std::map<InstanceHandle, wrl::ComPtr<IInspectable>>;

    // A map of Element Handle, property index, and source position index (for Styles) to the property
    using PropertyKey = std::tuple<InstanceHandle, int, BaseValueSource, int>;
    using PropertyHandleMap = std::map<PropertyKey, wrl::ComPtr<IInspectable>>;

    // A map of Binding handles to their evaluated value info.
    using BindingEvaluatedValueMap = std::map<InstanceHandle, std::pair<InstanceHandle, Diagnostics::PropertyChainData>>;

    // HandleMap::GetHandle should be the only way we go from IInspectable*->InstanceHandle.
    // no user should be reinterpret_casting an IInpectable*->InstanceHandle before adding an element
    // the map.

    static InstanceHandle GetHandle(
        _In_ IInspectable* pObject);

    static HRESULT GetHandle(
        _In_ CDependencyObject* obj,
        _Out_ InstanceHandle* handle);

    // Deprected - use GetHandle
    static HRESULT CreateHandle(
        _In_ IInspectable* pObject,
        _Out_ InstanceHandle* pHandle);

    void AddElementToMap(
        _In_ InstanceHandle pObject);

    void AddPropertyToMap(
        _In_ InstanceHandle elementHandle,
        _In_ InstanceHandle propertyHandle,
        _In_ int propertyIndex,
        _In_ BaseValueSource valueSource,
        _In_ int sourcePosition = -1);

    void AddToCreatedMap(
        InstanceHandle object);

    template <typename Interface>
    HRESULT GetFromHandle(
        _In_ InstanceHandle instanceHandle,
        _Inout_ wrl::Details::ComPtrRef<wrl::ComPtr<Interface>> prop)
    {
        wrl::ComPtr<IInspectable> object;
        if (SUCCEEDED(GetElementFromHandle(instanceHandle, &object)))
        {
            return object.CopyTo(prop);
        }

        if (SUCCEEDED(GetPropertyFromHandle(instanceHandle, &object)))
        {
            return object.CopyTo(prop);
        }

        auto iter = m_createdMap.find(instanceHandle);
        if (iter != m_createdMap.end())
        {
            return iter->second.CopyTo(prop);
        }

        return E_NOTFOUND;
    }


    template <typename T>
    HRESULT GetElementFromHandle(
        _In_ InstanceHandle handle,
        _Inout_ wrl::Details::ComPtrRef<wrl::ComPtr<T>> element)
    {
        ElementHandleMap::iterator foundPosition = m_elementMap.find(handle);
        if (foundPosition == m_elementMap.end() || !foundPosition->second)
        {
            return E_NOTFOUND;
        }

        IFC_RETURN(foundPosition->second.As(element));

        return S_OK;
    }

    std::vector<InstanceHandle> GetOwnersOfProperty(
        _In_ InstanceHandle propertyHandle
    );

    template <typename T>
    HRESULT GetPropertyFromHandle(
        _In_ InstanceHandle propertyHandle,
        _Inout_ wrl::Details::ComPtrRef<wrl::ComPtr<T>> prop)
    {
        // Loop through the keys and the map and see if we can find the owner of the handle.
        PropertyHandleMap::iterator iter = std::find_if(m_propertyMap.begin(), m_propertyMap.end(), [propertyHandle]
        (const std::pair<PropertyKey, wrl::ComPtr<IInspectable>> iter) -> bool {
            return propertyHandle == reinterpret_cast<InstanceHandle>(iter.second.Get());
        });

        if (iter == m_propertyMap.end())
        {
            return E_NOTFOUND;
        }

        IFC_RETURN(iter->second.As(prop));

        return S_OK;
    }

    // Get's the dictionary for this resoure, if it is contained in one. If not,
    // it returns nullptr.
    wrl::ComPtr<xaml::IResourceDictionary> TryGetDictionaryForResource(
        InstanceHandle resourceHandle);

    wrl::ComPtr<xaml::IUIElement> TryGetUIElementOwnerOfProperty(
        _In_ InstanceHandle propHandle);

    void AddEvalutedValueToMap(
        _In_ InstanceHandle bindingHandle,
        _In_ InstanceHandle valueHandle,
        _In_ const Diagnostics::PropertyChainData& info);

    HRESULT GetEvaluatedValue(
        _In_ InstanceHandle bindingHandle,
        _Out_ Diagnostics::PropertyChainData* info,
        _Out_ InstanceHandle* valueHandle);

    void ReleaseElement(
        _In_ InstanceHandle handle);

    void ReleaseProperties(
        _In_ InstanceHandle ownerHandle
        );

    void RemoveProperty(
        _In_ InstanceHandle elementHandle,
        _In_ InstanceHandle propertyHandle
        );

    void RemoveProperty(
        _In_ InstanceHandle elementHandle,
        _In_ int propertyIndex,
        _In_ BaseValueSource valueSource);

    // updates all instances in our map of elements
    // using this property to point to the new one
    void ReplaceProperty(
        _In_ InstanceHandle oldPropertyHandle,
        _In_ InstanceHandle newPropertyHandle
    );

    bool HasElement(_In_ InstanceHandle handle) const;

    bool HasProperty(
        _In_ InstanceHandle handle);

    HRESULT FindHandle(
        _In_ IInspectable* inspectable,
        _Out_ InstanceHandle* handle);

    void Clear();

    ElementHandleMap& GetElements();
    PropertyHandleMap& GetProperties();

private:
    ElementHandleMap m_elementMap;
    ElementHandleMap m_createdMap;
    PropertyHandleMap m_propertyMap;
    BindingEvaluatedValueMap m_bindingEvaluatedValueMap;
};