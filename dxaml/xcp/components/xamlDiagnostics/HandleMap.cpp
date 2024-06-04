// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#include "precomp.h"
#include "handlemap.h"
#include <stack>
#include "diagnosticsInterop\inc\PropertyChainIterator.h"
#include "DXamlServices.h"
#include "DependencyObject.h"
#include "DiagnosticsInterop.h"

InstanceHandle
HandleMap::GetHandle(
    _In_opt_ IInspectable* pObject)
{
    InstanceHandle handle = 0;
    if (pObject)
    {
        wrl::ComPtr<IInspectable> identity;
        IGNOREHR(pObject->QueryInterface<IInspectable>(&identity));
        handle = reinterpret_cast<InstanceHandle>(identity.Get());
    }
    return handle;
}

HRESULT
HandleMap::GetHandle(
    _In_ CDependencyObject* obj,
    _Out_ InstanceHandle* handle)
{
    ctl::ComPtr<DirectUI::DependencyObject> peer;

    // If obj is a CApplication, it won't have a peer and we need to handle it specially
    HRESULT hr = DirectUI::DXamlServices::GetPeer(obj, &peer);
    if (FAILED(hr) || peer.Get() == nullptr)
    {
        if (obj->OfTypeByIndex<KnownTypeIndex::Application>())
        {
            wrl::ComPtr<xaml::IApplication> spApplication;
            IFC_RETURN(Diagnostics::DiagnosticsInterop::GetApplicationStatic(&spApplication));
            *handle = HandleMap::GetHandle(spApplication.Get());
            return S_OK;
        }
        else
        {
            // We couldn't get the object's peer and it also wasn't an app object - should never hit here.
            // But if we do and GetPeer has an hresult indicating why it failed, we should return that,
            // otherwise a generic failure.  Note that GetPeer() returns S_OK even if there wasn't a peer.
            hr = (FAILED(hr)) ? hr : E_FAIL;
        }
    }

    // Back to normal case where we successfully got a peer.
    *handle = HandleMap::GetHandle(peer.Cast<xaml::IDependencyObject>());
    return S_OK;
}

HRESULT
HandleMap::CreateHandle(
    _In_opt_ IInspectable* pObject,
    _Out_ InstanceHandle* pHandle)
{
    IFCPTR_RETURN(pHandle);
    *pHandle = GetHandle(pObject);
    return S_OK;
}

void HandleMap::AddElementToMap(
    _In_ InstanceHandle elementHandle)
{
    IInspectable* element = reinterpret_cast<IInspectable*>(elementHandle);
    if (element)
    {
        m_elementMap.emplace(elementHandle, wrl::ComPtr<IInspectable>(element));
    }
}

void
HandleMap::AddToCreatedMap(
    _In_ InstanceHandle elementHandle)
{
    IInspectable* element = reinterpret_cast<IInspectable*>(elementHandle);
    if (element)
    {
        m_createdMap.emplace(elementHandle, wrl::ComPtr<IInspectable>(element));
    }
}

// InternalCreateOwnership creates a strong reference to an owned object that is managed by the lifetime
// of its parent. If clearInternalRef is true, this will remove the instance from our internal handle map,
// thus giving complete lifetime control of the child to its parent. This should be used when an object
// is created using CreateInstance and either added to a collection via AddChild, or set to a property via
// SetPropertyValue.
void
HandleMap::AddPropertyToMap(
    _In_ InstanceHandle elementHandle,
    _In_ InstanceHandle propertyHandle,
    _In_ int propertyIndex,
    _In_ BaseValueSource valueSource,
    _In_ int sourcePosition)
{
    IInspectable* property = reinterpret_cast<IInspectable*>(propertyHandle);
    if (elementHandle != 0 && property)
    {
        //When overwriting a property, we may have to delete the old one from our property map
        //if it would be orphaned
        InstanceHandle potentialOrphanedHandle = 0;

        std::pair<PropertyHandleMap::iterator, bool> propertyPlacedIter = m_propertyMap.emplace(std::make_tuple(elementHandle, propertyIndex, valueSource, sourcePosition), wrl::ComPtr<IInspectable>(property));
        // if the insertion didn't take place because it already exists, then just replace the property
        if (!propertyPlacedIter.second)
        {
            ASSERT(potentialOrphanedHandle == 0);
            wrl::ComPtr<IInspectable> spOldValue(propertyPlacedIter.first->second);
            potentialOrphanedHandle = GetHandle(spOldValue.Get());
            propertyPlacedIter.first->second = property;
        }

        //Check for an orphaned handle here - if we didn't find an old local property
        //or the property we overwrote is the same as the orphaned handle
        //(so we definitely do want to keep it around), we can skip this
        if (potentialOrphanedHandle != 0 && potentialOrphanedHandle != propertyHandle)
        {
            //We should only remove an orphaned handle if the property we just overwrote is the only one that owned it.
            //Our property map has a quirk where a property also has a reference to itself if we've expanded it with GetPropertyValuesChain,
            //so we need to check if there either are no owners or the only owner is the orphaned handle itself.
            std::vector<InstanceHandle> owners = GetOwnersOfProperty(potentialOrphanedHandle);
            size_t ownersSize = owners.size();
            if (ownersSize == 0 || (ownersSize == 1 && owners[0] == potentialOrphanedHandle))
            {
                ReleaseProperties(potentialOrphanedHandle);
            }
        }
    }

    // If this property was in the created map, then remove it from that map now
    // that it will be managed as a property of this object.
    auto iter = m_createdMap.find(propertyHandle);
    if (iter != m_createdMap.end())
    {
        m_createdMap.erase(iter);
    }
}

wrl::ComPtr<xaml::IResourceDictionary>
HandleMap::TryGetDictionaryForResource(
    InstanceHandle resourceHandle)
{
    // Loop through the keys and the map and see if we can find the owner of the handle.
    auto iter = std::find_if(m_propertyMap.begin(), m_propertyMap.end(), [resourceHandle](const auto key) {
        return resourceHandle == reinterpret_cast<InstanceHandle>(key.second.Get());
    });

    wrl::ComPtr<xaml::IResourceDictionary> dictionary;

    if (iter != m_propertyMap.end())
    {
        // A resource dictionary would be in the property map if found. Ignore the HRESULT
        // if the owner of this isn't a ResourceDictionary
        IGNOREHR(GetPropertyFromHandle(std::get<0>(iter->first), &dictionary));
    }

    return dictionary;
}

wrl::ComPtr<xaml::IUIElement>
HandleMap::TryGetUIElementOwnerOfProperty(
    _In_ InstanceHandle propHandle)
{
    auto owners = GetOwnersOfProperty(propHandle);
    wrl::ComPtr<xaml::IUIElement> ownerElement;
    for (const auto& owner : owners)
    {
        if (SUCCEEDED(GetElementFromHandle(owner, &ownerElement)))
        {
            break;
        }
    }
    return ownerElement;
}

std::vector<InstanceHandle>
HandleMap::GetOwnersOfProperty(
    _In_ InstanceHandle propertyHandle)
{
    std::vector<InstanceHandle> owners;
    // Loop through the keys and the map and see if we can find the owner of the handle.
    for (auto& pair : m_propertyMap)
    {
        if (propertyHandle == reinterpret_cast<InstanceHandle>(pair.second.Get()) && std::find(owners.begin(), owners.end(), std::get<0>(pair.first)) == owners.end())
        {
            owners.push_back(std::get<0>(pair.first));
        }
    }

    return owners;

}

void
HandleMap::ReleaseElement(
    _In_ InstanceHandle handle)
{
    m_elementMap.erase(handle);
    ReleaseProperties(handle);
}

void
HandleMap::ReleaseProperties(
    _In_ InstanceHandle ownerHandle)
{
    std::stack<InstanceHandle> evalStack;
    evalStack.push(ownerHandle);

    while (!evalStack.empty())
    {
        InstanceHandle curHandle = evalStack.top();
        evalStack.pop();

        // If the current element isn't in the live tree, then it is safe to release it's properties
        if (!HasElement(curHandle))
        {
            // Find a property in the map that is owned by the current element.
            PropertyHandleMap::iterator iter = std::find_if(m_propertyMap.begin(), m_propertyMap.end(), [curHandle]
            (const std::pair<PropertyKey, wrl::ComPtr<IInspectable>> propertyPair) -> bool {
                return (curHandle == std::get<0>(propertyPair.first));
            });

            // While there are still properties for the current element
            while (iter != m_propertyMap.end())
            {
                auto handle = reinterpret_cast<InstanceHandle>(iter->second.Get());
                // push the property handle onto the stack to be checked if it references any properties that need to be removed.
                evalStack.push(handle);

                // We can remove the current element->property mapping because we know this element is not
                // in the live visual tree
                m_propertyMap.erase(iter);

                // Remove evaluated values if any exist
                m_bindingEvaluatedValueMap.erase(handle);

                // Get the next property
                iter = std::find_if(m_propertyMap.begin(), m_propertyMap.end(), [curHandle]
                (const std::pair<PropertyKey, wrl::ComPtr<IInspectable>> propertyPair) -> bool {
                    return (curHandle == std::get<0>(propertyPair.first));
                });
            }
        }
    }
}

void
HandleMap::RemoveProperty(
    _In_ InstanceHandle elementHandle,
    _In_ int propertyIndex,
    _In_ BaseValueSource valueSource)
{
    m_propertyMap.erase(std::make_tuple(elementHandle, propertyIndex, valueSource, -1));
}


void
HandleMap::RemoveProperty(
    _In_ InstanceHandle elementHandle,
    _In_ InstanceHandle propertyHandle)
{
    // Loop through the keys and the map and see if we can find the owner of the handle.
    auto iter = std::find_if(m_propertyMap.begin(), m_propertyMap.end(), [elementHandle, propertyHandle]
        (const std::pair<PropertyKey, wrl::ComPtr<IInspectable>> iter) -> bool {
        return (elementHandle == std::get<0>(iter.first)) && (propertyHandle == reinterpret_cast<InstanceHandle>(iter.second.Get()));
    });

    if (iter != m_propertyMap.end())
    {
        ReleaseElement(propertyHandle);
    }
}

void
HandleMap::ReplaceProperty(
    _In_ InstanceHandle oldPropertyHandle,
    _In_ InstanceHandle newPropertyHandle)
{
    // Remove references to the previous element and update any mappings to the new
    // property
    m_elementMap.erase(oldPropertyHandle);
    for (auto& iter : m_propertyMap)
    {
        // if any element in the tree currently points to the old property, update it
        // so it's points to the new property
        if (oldPropertyHandle == reinterpret_cast<InstanceHandle>(iter.second.Get()))
        {
            iter.second.Swap(reinterpret_cast<IInspectable*>(newPropertyHandle));
        }

        // if the old property was being used as a key in the property map, update
        // it so it contains the new value
        if (oldPropertyHandle == std::get<0>(iter.first))
        {
            InstanceHandle* addr = const_cast<InstanceHandle*>(&std::get<0>(iter.first));
            *addr = newPropertyHandle;
        }
    };

    // Add this assertion in here, it doesn't make sense to try and replace a binding object.
    ASSERT(m_bindingEvaluatedValueMap.find(oldPropertyHandle) == m_bindingEvaluatedValueMap.end());
}

bool HandleMap::HasElement(_In_ InstanceHandle handle) const
{
    const auto& it = m_elementMap.find(handle);
    return it != m_elementMap.end();
}

bool
HandleMap::HasProperty(
    _In_ InstanceHandle handle)
{
    // Loop through the keys and the map and see if we can find the owner of the handle.
    PropertyHandleMap::iterator iter = std::find_if(m_propertyMap.begin(), m_propertyMap.end(), [handle]
    (const std::pair<PropertyKey, wrl::ComPtr<IInspectable>> iter) -> bool {
        return (handle == reinterpret_cast<InstanceHandle>(iter.second.Get()));
    });

    return iter != m_propertyMap.end();
}

void
HandleMap::AddEvalutedValueToMap(
    _In_ InstanceHandle bindingHandle,
    _In_ InstanceHandle valueHandle,
    _In_ const Diagnostics::PropertyChainData& info)
{
    // If there already exists an evaluated value for this binding, we can
    // go ahead and replace it. When we store this, we are expecting to
    // get a GetPropertyValuesChain call on the Binding to happen immediately
    // after (current VS behavior). If this changes and that call doesn't happen,
    // then we will just remove what we have.
    auto emplace = m_bindingEvaluatedValueMap.emplace(bindingHandle, std::make_pair(valueHandle, info));
    if (!emplace.second)
    {
        emplace.first->second.second = info;
        emplace.first->second.first = valueHandle;
    }
}

HRESULT
HandleMap::GetEvaluatedValue(
    _In_ InstanceHandle bindingHandle,
    _Out_ Diagnostics::PropertyChainData* info,
    _Out_ InstanceHandle* valueHandle)
{
    auto iter = m_bindingEvaluatedValueMap.find(bindingHandle);
    if (iter == m_bindingEvaluatedValueMap.end())
    {
        return E_NOTFOUND;
    }

    *info = iter->second.second;
    *valueHandle = iter->second.first;
    return S_OK;
}

void
HandleMap::Clear()
{
    m_bindingEvaluatedValueMap.clear();
    m_elementMap.clear();
    m_propertyMap.clear();
}

HRESULT
HandleMap::FindHandle(
    _In_ IInspectable * inspectable,
    _Out_ InstanceHandle * handle)
{
    *handle = 0;
    InstanceHandle inspHandle = 0;
    IFC_RETURN(HandleMap::CreateHandle(inspectable, &inspHandle));

    if (m_elementMap.find(inspHandle) != m_elementMap.end())
    {
        *handle = inspHandle;
        return S_OK;
    }

    PropertyHandleMap::iterator iter = std::find_if(m_propertyMap.begin(), m_propertyMap.end(), [inspHandle]
    (const std::pair<PropertyKey, wrl::ComPtr<IInspectable>> iter) -> bool {
        return (inspHandle == reinterpret_cast<InstanceHandle>(iter.second.Get()));
    });

    if (iter != m_propertyMap.end())
    {
        *handle = inspHandle;
        return S_OK;
    }

    return E_NOTFOUND;
}

HandleMap::ElementHandleMap&
HandleMap::GetElements()
{
    return m_elementMap;
}

HandleMap::PropertyHandleMap&
HandleMap::GetProperties()
{
    return m_propertyMap;
}