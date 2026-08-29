// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#include "precomp.h"
#include "RuntimeDictionary.h"
#include "DiagnosticsInterop.h"
#include "ResourceGraph.h"
#include "xstring_ptr.h"
#include "ComUtils.h"
#include "DOPointerCast.h"
#include "ResourceOperations.h"
#include "ResourceResolver.h"
#include "resources.h"
#include "valueboxer\inc\Value.h"
#include "MetadataAPI.h"

namespace Diagnostics
{
    bool RuntimeDictionary::TryGetItem(_In_z_ const wchar_t* key, const bool lookForImplicitStyle, std::shared_ptr<RuntimeObject>& item)
    {
        wrl::ComPtr<IInspectable> foundItem;
        
        RuntimeDictionaryKeyData keyData {lookForImplicitStyle};
        VERIFYHR(xstring_ptr::CloneBuffer(key, &keyData.Key));

        bool found = DiagnosticsInterop::TryGetDictionaryItem(
            GetBackingDictionary().Get(),
            keyData.Key,
            lookForImplicitStyle, &foundItem);
        if (found)
        {
            item = GetRuntimeObject(foundItem.Get(), shared_from_this());
            AddToCache(keyData, item);
            // Keep a ref to this object in-case Visual Studio tries to re-use this item after they've removed it.
            m_lastRetrievedValue = item;
        }
        return found;
    }

    wrl::ComPtr<xaml::IResourceDictionary> RuntimeDictionary::GetBackingDictionary() const
    {
        wrl::ComPtr<xaml::IResourceDictionary> backingDictionary;
        IFCFAILFAST(GetBackingObject().As(&backingDictionary));
        return backingDictionary;
    }

    _Check_return_ HRESULT RuntimeDictionary::TryReplaceItem(std::shared_ptr<RuntimeObject> key, std::shared_ptr<RuntimeObject> item)
    {
        // VisualStudio only reused instance handles when adding a dictionary item. The pattern they used was GetDictionaryItem,
        // RemoveDictionaryItem, then a call too AddDictionaryItem. If they're using this API, then we don't need to keep this item alive. 
        m_lastRetrievedValue.reset();
        RemoveItem(key);
        IFC_RETURN(InsertItem(key, item));
        const auto resourceGraph = GetResourceGraph();
        auto coreObj = checked_cast<CResourceDictionary>(DiagnosticsInterop::ConvertToCore(GetBackingDictionary().Get()));
        IFC_RETURN(resourceGraph->ReplaceResource(coreObj, key->GetBackingObject().Get(), item->GetBackingObject().Get()));
        return S_OK;
    }

    _Check_return_ HRESULT RuntimeDictionary::InsertItem(std::shared_ptr<RuntimeObject> key, std::shared_ptr<RuntimeObject> item, _Out_opt_ ResourceGraphKey* graphKey)
    {
        ResourceGraphKey tmpGraphKey;
        IFC_RETURN(DiagnosticsInterop::AddDictionaryItem(
            GetBackingDictionary().Get(),
            key->GetBackingObject().Get(),
            item->GetBackingObject().Get(),
            tmpGraphKey));

        AddToCache(key, item);
        m_lastRetrievedValue.reset();
        std::shared_ptr<RuntimeDictionary> itemDictionary;
        if (item->TryGetAsDictionary(itemDictionary) && IsThemeDictionaries())
        {
            IFC_RETURN(itemDictionary->TryUpdateAllDependentItems());
        }
        if (graphKey) 
        {
            *graphKey = tmpGraphKey;
        }
        return S_OK;
    }

    ResourceGraphKeyWithParent RuntimeDictionary::RemoveItem(std::shared_ptr<RuntimeObject> key)
    {
        // Make sure we properly remove this item from the dictionary, so get the item associated with this
        // key and remove it
        ResourceGraphKeyWithParent graphKey;
        bool removeSucceeded = DiagnosticsInterop::TryRemoveDictionaryItem(
            GetBackingDictionary().Get(),
            key->GetBackingObject().Get(),
            graphKey);
        MICROSOFT_TELEMETRY_ASSERT_DISABLED(removeSucceeded);
        if (removeSucceeded)
        {
            RemoveFromCache(key);
        }
        return graphKey;
    }

    bool RuntimeDictionary::TryGetAsDictionary(std::shared_ptr<RuntimeDictionary>& dictionary)
    {
        dictionary = derived_shared_from_this<RuntimeDictionary>();
        return true;
    }

    void RuntimeDictionary::AddToCache(const RuntimeDictionaryKey& key, const std::shared_ptr<RuntimeObject>& value)
    {
        auto iter = m_items.find(key);
        // Skip unnecessary removal and insertion if the value is the same
        const bool skipAdd = (iter != m_items.end()) && iter->second.GetValue() == value;
        if (!skipAdd)
        {
            if (iter != m_items.end())
            {
                iter = m_items.erase(iter);
            }

            m_items.emplace_hint(iter, key, RuntimeDictionaryValue(value, derived_shared_from_this<RuntimeDictionary>()));
        }
    }

    void RuntimeDictionary::RemoveFromCache(const std::shared_ptr<RuntimeObject>& key)
    {
        // We always need to keep the key because Visual Studio will reuse it. So on remove, just store an empty value
        AddToCache(key, nullptr);
    }

    void RuntimeDictionary::AddToCache(const std::shared_ptr<RuntimeObject>& key, const std::shared_ptr<RuntimeObject>& value)
    {
        AddToCache(RuntimeDictionaryKey(key, derived_shared_from_this<RuntimeDictionary>()), value);
    }

    void RuntimeDictionary::AddToCache(const RuntimeDictionaryKeyData& key, const std::shared_ptr<RuntimeObject>& item)
    {
        // We don't want to stomp on an existing key that VS (shouldn't) is trying to reuse. So, if we already have a matching Key, then we'll use that.
        // Otherwise, we will just create one as a placeholder.
        RuntimeDictionaryKey dictionarykey(key);
        auto iter = m_items.find(dictionarykey);
        if (iter != m_items.end())
        {
            // Use the existing key, as it could be a key that needs to be retrieved later
            dictionarykey = iter->first;
        }

        AddToCache(dictionarykey, item);
    }

    // Workaround for VisualStudio misuse of InstanceHandles. They remove a dictionary item before
    // adding it. The removal means there are no more references to the handle, and so we should
    // no longer find the handle when they try to add it. However, we used to leak objects, so they
    // were able to do this before.
    bool RuntimeDictionary::ShouldKeepAlive(const std::shared_ptr<RuntimeObject>& value) const
    {
        return value == m_lastRetrievedValue.lock();
    }

    bool RuntimeDictionary::TryClear()
    {
        // Get the keys associated with this RuntimeDictionary so that it can be cleared. We don't rely
        // on our internal cache state, as it isn't guaranteed that they have all been added at this point.
        // We only add them when requested.
        auto removedKeys = DiagnosticsInterop::GetAllKeys(GetBackingDictionary().Get());
        bool succeeded = SUCCEEDED(DiagnosticsInterop::Clear(GetBackingObject().Get()));

        if (succeeded)
        {
            // Clearing the dictionary worked, clear local state and update all ResourceDependencies.
            m_items.clear();
            for (const auto& key : removedKeys)
            {
                succeeded = SUCCEEDED(ResourceOperations::UpdateDependentItemsOnRemove(key));
                if (!succeeded) break;
            }
        }
        return succeeded;
    }

    std::vector<std::shared_ptr<RuntimeObject>> RuntimeDictionary::GetItemsByIndex()
    {
        std::vector<std::shared_ptr<RuntimeObject>> items;
        for (const auto& iter : DiagnosticsInterop::GetKeysOrderedByIndex(GetBackingDictionary().Get()))
        {
            const xstring_ptr& key = iter.first;
            const bool isImplicitStyle = iter.second;
            std::shared_ptr<RuntimeObject> item;
            const bool gotItem = TryGetItem(key.GetBuffer(), isImplicitStyle, item);
            // Getting the item shouldn't fail, however ResourceDictionary entries are deferred, so it's possible
            // trying to retrieve one could fail. If this assert just produces noise for app-related issues, then we can remove it.
            MICROSOFT_TELEMETRY_ASSERT_DISABLED(gotItem);
            if (gotItem)
            {
                items.push_back(std::move(item));
            }
        }
        m_lastRetrievedValue.reset(); // reset m_lastRetrievedValue so we don't re-add items to the cache if they are removed.
        return items;
    }

    bool RuntimeDictionary::TryClearValue(const RuntimeProperty& prop)
    {
        std::vector<ResourceGraphKeyWithParent> removedKeys;

        // Grab the keys before clearing the Source property as that will remove the dictionary entries
        if (prop.GetIndex() == static_cast<uint32_t>(KnownPropertyIndex::ResourceDictionary_Source))
        {
            removedKeys = DiagnosticsInterop::GetAllKeys(GetBackingDictionary().Get());
        }

        bool succeeded = RuntimeObject::TryClearValue(prop);
        if (succeeded)
        {
            // If clearing ResourceDictionary.Source, then update all ResourceDependencies that were
            // dependent on keys contained in this dictionary.
            for(const auto& key : removedKeys)
            {
                succeeded = SUCCEEDED(ResourceOperations::UpdateDependentItemsOnRemove(key));
                if (!succeeded) break;
            }
        }

        return succeeded;
    }

    bool RuntimeDictionary::TrySetValue(const RuntimeProperty& prop, std::shared_ptr<RuntimeObject> value)
    {
        bool succeeded = RuntimeObject::TrySetValue(prop, value);
        if (succeeded && prop.GetIndex() == static_cast<uint32_t>(KnownPropertyIndex::ResourceDictionary_Source))
        {
            succeeded = SUCCEEDED(TryUpdateAllDependentItems());
        }
        return succeeded;
    }

    _Check_return_ HRESULT RuntimeDictionary::TryUpdateAllDependentItems() const
    {
        auto coreDictionary = checked_sp_cast<CResourceDictionary>(DiagnosticsInterop::ConvertToCore(GetBackingDictionary().Get()));

        // VS adds the dictionary before setting ResourceDictionary.Source, this is opposite of what the parser would do, and in doing so,
        // this causes the m_pResourceOwner field to not be set. So let's manually set it now.
        coreDictionary->SetResourceOwner(coreDictionary->GetResourceOwnerNoRef());

        // Get each item in the dictionary that was just added
        for (const auto& item : DiagnosticsInterop::GetAllKeys(GetBackingDictionary().Get()))
        {
            // We need to start at the owner of the dictionary that was just populated via ResourceDictionary.Source to see if
            // there are any previous resolutions that need to be re-resolved.
            // The resource reference can't be an implicit style
            const bool isImplicitStyle = false;
            xref_ptr<CResourceDictionary> currentDictionary;
            IFC_RETURN(DiagnosticsInterop::FindOtherDictionaryForResolution(coreDictionary, item.Key, isImplicitStyle, currentDictionary));
            IFC_RETURN(ResourceOperations::UpdateDependentItemsOnAdd({ currentDictionary, item.Key }, GetBackingDictionary().Get()));
        }
        return S_OK;
    }

    bool RuntimeDictionary::IsThemeDictionaries() const
    {
        auto coreDictionary = checked_sp_cast<CResourceDictionary>(DiagnosticsInterop::ConvertToCore(GetBackingDictionary().Get()));
        return coreDictionary->IsThemeDictionaries();
    }
}

