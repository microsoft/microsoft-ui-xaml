// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#pragma once
#include "RuntimeObject.h"
#include "wil\resource.h"
#include <set>
#include "RuntimeDictionaryKey.h"
#include "RuntimeDictionaryValue.h"

class xstring_ptr;
namespace Diagnostics
{
    struct ResourceGraphKey;
    struct ResourceGraphKeyWithParent;

    // RuntimeDictionary doesnt derive from RuntimeCollection because ResourceDictionaries
    // are not designed to be treated as generic collections and their indices are not guaranteed.
    // Furthermore, trying to grab certain items can fail if there is an invalid item
    // (such as brush with resource reference to color that doesn't exist) then we will never get past that.
    // We support a few generic collection APIs to maintain compatibility with VisualStudio.
    class RuntimeDictionary final : public RuntimeObject
    {
        friend std::shared_ptr<RuntimeObject> GetRuntimeObject(_In_ IInspectable* backingObject, std::shared_ptr<RuntimeObject> parent);
    public:

        // APIs for modifying and retrieving dictionary items
        bool TryGetItem(_In_z_ const wchar_t* key, const bool lookForImplicitStyle, std::shared_ptr<RuntimeObject>& item);
        _Check_return_ HRESULT TryReplaceItem(std::shared_ptr<RuntimeObject> key, std::shared_ptr<RuntimeObject> item);
        _Check_return_ HRESULT InsertItem(std::shared_ptr<RuntimeObject> key, std::shared_ptr<RuntimeObject> item, _Out_opt_ ResourceGraphKey* graphKey = nullptr);
        ResourceGraphKeyWithParent RemoveItem(std::shared_ptr<RuntimeObject> key);
        bool TryClear();

        // Overrides for setting/clearing properties
        bool TrySetValue(const RuntimeProperty& prop, std::shared_ptr<RuntimeObject> value) final;
        bool TryClearValue(const RuntimeProperty& prop) final;
        
        bool TryGetAsDictionary(std::shared_ptr<RuntimeDictionary>& dictionary) final;
        Microsoft::WRL::ComPtr<xaml::IResourceDictionary> GetBackingDictionary() const;

        bool ShouldKeepAlive(const std::shared_ptr<RuntimeObject>& value) const;

        // This method is needed to maintain compat with VS who requires GetCollectionElements to work with ResourceDictionaries
        std::vector<std::shared_ptr<RuntimeObject>> GetItemsByIndex();

    private:
        void AddToCache(const RuntimeDictionaryKey& key, const std::shared_ptr<RuntimeObject>& value);
        void AddToCache(const std::shared_ptr<RuntimeObject>& key, const std::shared_ptr<RuntimeObject>& value);
        void AddToCache(const RuntimeDictionaryKeyData& key, const std::shared_ptr<RuntimeObject>& value);
        void RemoveFromCache(const std::shared_ptr<RuntimeObject>& key);
        std::shared_ptr<RuntimeObject> KeyToObject(_In_z_ const wchar_t* key, const bool lookForImplicitStyle);
    private:
        _Check_return_ HRESULT TryUpdateAllDependentItems() const;
        bool IsThemeDictionaries() const;
        std::map<RuntimeDictionaryKey, RuntimeDictionaryValue> m_items;
        std::weak_ptr<RuntimeObject> m_lastRetrievedValue;
        RuntimeDictionary() = default;
    };


}
