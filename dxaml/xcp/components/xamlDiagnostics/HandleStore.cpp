// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#include "precomp.h"
#include "HandleStore.h"

namespace XamlDiagnosticsShared {

_Use_decl_annotations_
InstanceHandle HandleStore::GetHandle(
    IInspectable* pObject)
{
    return HandleMap::GetHandle(pObject);
}

_Use_decl_annotations_
HRESULT HandleStore::GetHandle(
    CDependencyObject* obj,
    InstanceHandle* handle)
{
    IFC_RETURN(HandleMap::GetHandle(obj, handle));
    return S_OK;
}

// Converts an IInspectable to its equivalent InstanceHandle.
_Use_decl_annotations_
HRESULT HandleStore::CreateHandle(
    IInspectable* inspectable,
    InstanceHandle* result)
{
    IFC_RETURN(HandleMap::CreateHandle(inspectable, result));
    return S_OK;
}

_Use_decl_annotations_
void HandleStore::AddElement(InstanceHandle elementHandle)
{
    auto lock = m_handleMapLock.Lock();
    m_handleMap.AddElementToMap(elementHandle);
}

_Use_decl_annotations_
void HandleStore::AddElement(IInspectable* element)
{
    InstanceHandle handle;
    IFCFAILFAST(CreateHandle(element, &handle));
    AddElement(handle);
}

_Use_decl_annotations_
void HandleStore::AddElementToMap(InstanceHandle pObject)
{
    AddElement(pObject);
}

_Use_decl_annotations_
void HandleStore::AddProperty(
    InstanceHandle elementHandle,
    InstanceHandle propertyHandle,
    int propertyIndex)
{
    auto lock = m_handleMapLock.Lock();
    m_handleMap.AddPropertyToMap(
        elementHandle,
        propertyHandle,
        propertyIndex,
        BaseValueSourceLocal,
        false /*removePropertyFromElementMap*/);
}

_Use_decl_annotations_
void HandleStore::AddPropertyToMap(
    InstanceHandle elementHandle,
    InstanceHandle propertyHandle,
    int propertyIndex,
    BaseValueSource valueSource,
    int sourcePosition)
{
    auto lock = m_handleMapLock.Lock();
    m_handleMap.AddPropertyToMap(
        elementHandle,
        propertyHandle,
        propertyIndex,
        valueSource,
        sourcePosition);
}

_Use_decl_annotations_
void HandleStore::AddToCreatedMap(
    InstanceHandle object)
{
    auto lock = m_handleMapLock.Lock();
    m_handleMap.AddToCreatedMap(object);
}

_Use_decl_annotations_
HRESULT HandleStore::FindElementOrProperty(
    InstanceHandle elementOrPropertyHandle,
    IInspectable** elementOrProperty)
{
    *elementOrProperty = nullptr;

    auto lock = m_handleMapLock.Lock();

    wrl::ComPtr<IInspectable> inspectableFromHandle;
    if (SUCCEEDED(m_handleMap.GetElementFromHandle(elementOrPropertyHandle, &inspectableFromHandle)))
    {
        *elementOrProperty = inspectableFromHandle.Detach();
        return S_OK;
    }

    if (SUCCEEDED(m_handleMap.GetPropertyFromHandle(elementOrPropertyHandle, &inspectableFromHandle)))
    {
        *elementOrProperty = inspectableFromHandle.Detach();
        return S_OK;
    }

    return E_NOTFOUND;
}

_Use_decl_annotations_
bool HandleStore::HasElement(IInspectable* element)
{
    InstanceHandle handle{};
    if (FAILED(CreateHandle(element, &handle)))
    {
        return false;
    }
    auto lock = m_handleMapLock.Lock();
    return m_handleMap.HasElement(handle);
}

_Use_decl_annotations_
bool HandleStore::HasElement(InstanceHandle handle)
{
    auto lock = m_handleMapLock.Lock();
    return m_handleMap.HasElement(handle);
}

_Use_decl_annotations_
bool HandleStore::HasProperty(
    InstanceHandle handle)
{
    auto lock = m_handleMapLock.Lock();
    return m_handleMap.HasProperty(handle);
}

_Use_decl_annotations_
HRESULT HandleStore::ReleaseElement(InstanceHandle elementHandle)
{
    auto lock = m_handleMapLock.Lock();
    m_handleMap.ReleaseElement(elementHandle);
    return S_OK;
}

_Use_decl_annotations_
HRESULT HandleStore::ReleaseElement(IInspectable* element)
{
    InstanceHandle handle;
    IFC_RETURN(CreateHandle(element, &handle));

    auto lock = m_handleMapLock.Lock();
    m_handleMap.ReleaseElement(handle);
    return S_OK;
}

_Use_decl_annotations_
HRESULT HandleStore::ReleaseProperties(InstanceHandle ownerHandle)
{
    auto lock = m_handleMapLock.Lock();
    m_handleMap.ReleaseProperties(ownerHandle);
    return S_OK;
}


_Use_decl_annotations_
void HandleStore::RemoveProperty(
    InstanceHandle elementHandle,
    InstanceHandle propertyHandle)
{
    auto lock = m_handleMapLock.Lock();
    m_handleMap.RemoveProperty(elementHandle, propertyHandle);
}

_Use_decl_annotations_
void HandleStore::RemoveProperty(
    InstanceHandle elementHandle,
    int propertyIndex,
    BaseValueSource valueSource)
{
    auto lock = m_handleMapLock.Lock();
    m_handleMap.RemoveProperty(elementHandle, propertyIndex, valueSource);
}

_Use_decl_annotations_
void HandleStore::ReplaceProperty(
    InstanceHandle oldPropertyHandle,
    InstanceHandle newPropertyHandle)
{
    auto lock = m_handleMapLock.Lock();
    m_handleMap.ReplaceProperty(oldPropertyHandle, newPropertyHandle);
}

_Use_decl_annotations_
void HandleStore::AddEvalutedValueToMap(
    InstanceHandle bindingHandle,
    InstanceHandle valueHandle,
    const Diagnostics::PropertyChainData& info)
{
    auto lock = m_handleMapLock.Lock();
    m_handleMap.AddEvalutedValueToMap(bindingHandle, valueHandle, info);
}

_Use_decl_annotations_
HRESULT HandleStore::GetEvaluatedValue(
    InstanceHandle bindingHandle,
    Diagnostics::PropertyChainData* info,
    InstanceHandle* valueHandle)
{
    auto lock = m_handleMapLock.Lock();
    IFC_RETURN(m_handleMap.GetEvaluatedValue(bindingHandle, info, valueHandle));
    return S_OK;
}

_Use_decl_annotations_
HRESULT HandleStore::FindHandle(
    IInspectable* inspectable,
    InstanceHandle* handle)
{
    auto lock = m_handleMapLock.Lock();
    IFC_RETURN(m_handleMap.FindHandle(inspectable, handle));
    return S_OK;
}

_Use_decl_annotations_
wrl::ComPtr<xaml::IResourceDictionary> HandleStore::TryGetDictionaryForResource(
    InstanceHandle resourceHandle)
{
    auto lock = m_handleMapLock.Lock();
    return m_handleMap.TryGetDictionaryForResource(resourceHandle);
}

_Use_decl_annotations_
wrl::ComPtr<xaml::IUIElement> HandleStore::TryGetUIElementOwnerOfProperty(
    InstanceHandle propHandle)
{
    auto lock = m_handleMapLock.Lock();
    return m_handleMap.TryGetUIElementOwnerOfProperty(propHandle);
}

HandleMap::ElementHandleMap& HandleStore::GetElements()
{
    auto lock = m_handleMapLock.Lock();
    return m_handleMap.GetElements();
}

HandleMap::PropertyHandleMap& HandleStore::GetProperties()
{
    auto lock = m_handleMapLock.Lock();
    return m_handleMap.GetProperties();
}

void HandleStore::Clear()
{
    auto lock = m_handleMapLock.Lock();
    m_handleMap.Clear();
}

std::vector<InstanceHandle> HandleStore::GetOwnersOfProperty(_In_ InstanceHandle propertyHandle)
{
    auto lock = m_handleMapLock.Lock();
    return m_handleMap.GetOwnersOfProperty(propertyHandle);
}
}

