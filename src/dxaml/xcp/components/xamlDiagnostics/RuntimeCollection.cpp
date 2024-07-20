// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#include "precomp.h"
#include "RuntimeCollection.h"
#include "DiagnosticsInterop.h"
#include "ResourceOperations.h"
#include "ResourceGraph.h"

namespace Diagnostics
{
    void RuntimeCollection::EnsureItems()
    {
        if (GetBackingObject() != nullptr && HasParent())
        {
            FAIL_FAST_ASSERT(DiagnosticsInterop::IsCollection(GetBackingObject().Get()));
            unsigned size = 0;

            IFCFAILFAST(DiagnosticsInterop::GetSize(GetBackingObject().Get(), &size));
            auto currentSize = static_cast<unsigned>(m_items.size());
            if (size > currentSize)
            {
                m_items.reserve(size);
                for (unsigned index = currentSize; index < size; ++index)
                {
                    wrl::ComPtr<IInspectable> item;
                    IFCFAILFAST(DiagnosticsInterop::GetAt(GetBackingObject().Get(), index, &item));
                    m_items.emplace_back(GetRuntimeObject(item.Get(), GetOwnerOfItems()));
                }
            }
        }
    }

    bool RuntimeCollection::empty() const
    {
        return m_items.empty();
    }

    size_t RuntimeCollection::size() const
    {
        return m_items.size();
    }

    std::vector<std::shared_ptr<RuntimeObject>>::const_iterator RuntimeCollection::begin() const
    {
        return m_items.begin();
    }

    std::vector<std::shared_ptr<RuntimeObject>>::const_iterator RuntimeCollection::end() const
    {
        return m_items.end();
    }

    bool RuntimeCollection::TryInsertAt(size_t index, std::shared_ptr<RuntimeObject> item)
    {
        // Add to the internal collection first, this way we'll be properly parented in SignalMutation
        ModifyInternalStateToMatchBackingCollectionChange(index, item, wfc::CollectionChange_ItemInserted);
        bool succeeded = SUCCEEDED(DiagnosticsInterop::InsertAt(GetBackingObject().Get(), item->GetBackingObject().Get(), static_cast<unsigned int>(index)));
        
        if (!succeeded)
        {
            ModifyInternalStateToMatchBackingCollectionChange(index, item, wfc::CollectionChange_ItemRemoved);
        }
        return succeeded;
    }

    bool RuntimeCollection::TryClear()
    {
        std::vector<ResourceGraphKeyWithParent> removedKeys;
        wrl::ComPtr<wfc::IVector<xaml::ResourceDictionary*>> parentDictionaryCollection;
        // If the collection being cleared is a MergedDictionary collection, then get all keys from the dictionary
        // so that we can update the references.
        if (SUCCEEDED(GetBackingObject().As(&parentDictionaryCollection)) && parentDictionaryCollection)
        {
            removedKeys = DiagnosticsInterop::GetAllKeys(parentDictionaryCollection.Get());
        }

        bool succeeded = SUCCEEDED(DiagnosticsInterop::Clear(GetBackingObject().Get()));

        // We clear the local items afterwards so that we will be properly parented during leave
        if (succeeded)
        {
            m_items.clear();

            for (const auto& removedKey : removedKeys)
            {
                succeeded = SUCCEEDED(ResourceOperations::UpdateDependentItemsOnRemove(removedKey));
                if (!succeeded) break;
            }
        }
        return succeeded;
    }

    std::shared_ptr<RuntimeObject> const& RuntimeCollection::operator[](_In_ size_t index) const
    {
        return m_items[index];
    }

    bool RuntimeCollection::TryRemoveAt(size_t index)
    {
        auto item = m_items[index];
        bool succeeded = SUCCEEDED(DiagnosticsInterop::RemoveAt(GetBackingObject().Get(), static_cast<unsigned int>(index)));
        // Remove afterwards so that we will be properly parented during leave
        if (succeeded)
        {
            ModifyInternalStateToMatchBackingCollectionChange(index, item, wfc::CollectionChange_ItemRemoved);
        }
        return succeeded;
    }

    bool RuntimeCollection::TryGetAsCollection(std::shared_ptr<RuntimeCollection>& collection)
    {
        collection = derived_shared_from_this<RuntimeCollection>();
        return true;
    }

    std::shared_ptr<RuntimeObject> RuntimeCollection::GetOwnerOfItems()
    {
        // Some collections (like UIElementCollection) aren't the actual parent to the items, but instead,
        // the owner of the collection is considered the parent.
        if (DiagnosticsInterop::CollectionIsParentToItems(GetBackingObject().Get()))
        {
            return shared_from_this();
        }

        return GetParent();
    }

    bool RuntimeCollection::TryGetIndexOf(const std::shared_ptr<RuntimeObject>& item, size_t& index) const
    {
        // First look in our cache of objects, if we don't have it, then default to looking through
        // the underlying collection.
        auto iter = std::find(m_items.begin(), m_items.end(), item);
        if (iter != m_items.end())
        {
            index = iter - m_items.begin();
            return true;
        }

        return DiagnosticsInterop::TryGetIndexOf(GetBackingObject().Get(), item->GetBackingObject().Get(), index);
    }

    void RuntimeCollection::ModifyInternalStateToMatchBackingCollectionChange(size_t index, std::shared_ptr<RuntimeObject> item, wfc::CollectionChange change)
    {
        if (change == wfc::CollectionChange_ItemRemoved)
        {
            // When elements leave the tree, they could have already been removed. So only try removing
            // if the object is in the collection
            size_t foundIndex = 0;
            if (item && TryGetIndexOf(item, foundIndex) && foundIndex == index)
            {
                item->SetParent(nullptr);
                m_items.erase(m_items.begin() + index);
            }
            return;
        }

        // Set the items parent so that the lifetime of the item is properly managed
        item->SetParent(GetOwnerOfItems());
        if (change == wfc::CollectionChange_ItemChanged)
        {
            m_items[index] = item;
        }
        else
        {
            if (index >= m_items.size())
            {
                m_items.push_back(item);
            }
            else
            {
                m_items.insert(m_items.begin() + index, item);
            }
        }
    }
}
