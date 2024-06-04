// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#pragma once

#include "includes.h"
#include "HandleMap.h"

namespace XamlDiagnosticsShared
{

//
// A thread-safe wrapper around HandleMap, for use by XamlDiagnostics and CompositionDiagnostics.
//
class HandleStore final
{

public:
    static InstanceHandle GetHandle(
        _In_ IInspectable* pObject);

    static HRESULT GetHandle(
        _In_ CDependencyObject* obj,
        _Out_ InstanceHandle* handle);

    static HRESULT CreateHandle(
        _In_ IInspectable* inspectable,
        _Out_ InstanceHandle* result);

    void AddElement(_In_ InstanceHandle elementHandle);

    void AddElement(_In_ IInspectable* element);

    void AddElementToMap(
        _In_ InstanceHandle pObject);

    void AddProperty(
        _In_ InstanceHandle elementHandle,
        _In_ InstanceHandle propertyHandle,
        _In_ int propertyIndex);

    void AddPropertyToMap(
        _In_ InstanceHandle elementHandle,
        _In_ InstanceHandle propertyHandle,
        _In_ int propertyIndex,
        _In_ BaseValueSource valueSource,
        _In_ int sourcePosition = -1);

    void AddToCreatedMap(
        InstanceHandle object);

    HRESULT FindElementOrProperty(
        _In_ InstanceHandle elementOrPropertyHandle,
        _COM_Outptr_ IInspectable** elementOrProperty);

    bool HasElement(_In_ IInspectable* element);

    bool HasElement(_In_ InstanceHandle handle);

    bool HasProperty(_In_ InstanceHandle handle);

    HRESULT ReleaseElement(_In_ InstanceHandle elementHandle);

    HRESULT ReleaseElement(_In_ IInspectable* element);

    HRESULT ReleaseProperties(_In_ InstanceHandle ownerHandle);

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
        _In_ InstanceHandle newPropertyHandle);

    void AddEvalutedValueToMap(
        _In_ InstanceHandle bindingHandle,
        _In_ InstanceHandle valueHandle,
        _In_ const Diagnostics::PropertyChainData& info);

    HRESULT GetEvaluatedValue(
        _In_ InstanceHandle bindingHandle,
        _Out_ Diagnostics::PropertyChainData* info,
        _Out_ InstanceHandle* valueHandle);

    HRESULT FindHandle(
        _In_ IInspectable* inspectable,
        _Out_ InstanceHandle* handle);

    template <typename Interface>
    HRESULT GetFromHandle(
        _In_ InstanceHandle instanceHandle,
        _Inout_ wrl::Details::ComPtrRef<wrl::ComPtr<Interface>> prop)
    {
        auto lock = m_handleMapLock.Lock();
        IFC_RETURN(m_handleMap.GetFromHandle(instanceHandle, prop));
        return S_OK;
    }

    template <typename T>
    HRESULT GetElementFromHandle(
        _In_ InstanceHandle handle,
        _Inout_ wrl::Details::ComPtrRef<wrl::ComPtr<T>> element)
    {
        auto lock = m_handleMapLock.Lock();
        IFC_RETURN(m_handleMap.GetElementFromHandle(handle, element));
        return S_OK;
    }

    template <typename T>
    HRESULT GetPropertyFromHandle(
        _In_ InstanceHandle propertyHandle,
        _Inout_ wrl::Details::ComPtrRef<wrl::ComPtr<T>> prop)
    {
        auto lock = m_handleMapLock.Lock();
        IFC_RETURN(m_handleMap.GetPropertyFromHandle(propertyHandle, prop));
        return S_OK;
    }

    // Get's the dictionary for this resoure, if it is contained in one. If not,
    // it returns nullptr.
    wrl::ComPtr<xaml::IResourceDictionary> TryGetDictionaryForResource(
        _In_ InstanceHandle resourceHandle);

    wrl::ComPtr<xaml::IUIElement> TryGetUIElementOwnerOfProperty(
        _In_ InstanceHandle propHandle);

    void Clear();

    HandleMap::ElementHandleMap& GetElements();
    HandleMap::PropertyHandleMap& GetProperties();

    std::vector<InstanceHandle> GetOwnersOfProperty(_In_ InstanceHandle propertyHandle);

private:
    // Keeps track of all IInspectables that are handed out to Visual Studio as InstanceHandles.
    HandleMap m_handleMap;

    // Prevents concurrent access to m_handleMap.
    wrl_wrappers::CriticalSection m_handleMapLock;
};
}