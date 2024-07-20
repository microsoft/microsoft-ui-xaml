// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#pragma once
#include "RuntimeObject.h"
#include <vector>
#include <memory>

namespace Diagnostics
{
    class RuntimeElement;
    class RuntimeCollection : public RuntimeObject
    {
        friend std::shared_ptr<RuntimeObject> GetRuntimeObject(_In_ IInspectable* backingObject, std::shared_ptr<RuntimeObject> parent);

    public:
        size_t size() const;
        bool empty() const;
        std::vector<std::shared_ptr<RuntimeObject>>::const_iterator begin() const;
        std::vector<std::shared_ptr<RuntimeObject>>::const_iterator end() const;

        virtual bool TryInsertAt(size_t index, std::shared_ptr<RuntimeObject> item);

        virtual bool TryClear();
        virtual bool TryRemoveAt(size_t index);
        std::shared_ptr<RuntimeObject> const& operator[](_In_ size_t index) const;

        bool TryGetAsCollection(std::shared_ptr<RuntimeCollection>& collection) override;

        virtual bool TryGetIndexOf(const std::shared_ptr<RuntimeObject>& item, size_t& index) const;

        void ModifyInternalStateToMatchBackingCollectionChange(size_t index, std::shared_ptr<RuntimeObject> item, wfc::CollectionChange change);
        virtual void EnsureItems();

    protected:
        RuntimeCollection() = default;
        virtual std::shared_ptr<RuntimeObject> GetOwnerOfItems();

    protected:
        std::vector<std::shared_ptr<RuntimeObject>> m_items;
    };
}
