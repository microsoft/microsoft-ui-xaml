// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#pragma once

class CDependencyObject;

namespace Jupiter {
    namespace NameScoping {
        class INameScopeDeferredElementCreator;
        
        // The size of the largest element held by a NameScopeTableEntry instance.
        const size_t c_storageSize = sizeof(void*) * 2;

        // Represents an entry in a NameScope table. To avoid extra heap allocations the
        // heterogeneous set of possible elements is stored in an unrestricted union with a custom
        // dtor that destructs according to the set tag type.
        class NameScopeTableEntry
        {
        public:
            NameScopeTableEntry();
            explicit NameScopeTableEntry(xref_ptr<CDependencyObject> element);
            explicit NameScopeTableEntry(xref::weakref_ptr<CDependencyObject> weakElement);
            explicit NameScopeTableEntry(std::weak_ptr<INameScopeDeferredElementCreator> deferredCreator);

            // Will get/create an instance of a given object. In the case of the element
            // failing to create (the deferred creation case) this method will throw the
            // inner exception. In the case of a weak ref no longer being available this method
            // will simply return an empty pointer.
            xref_ptr<CDependencyObject> TryGetElement(_Out_ bool* shouldRetry) const;

            ~NameScopeTableEntry();

            NameScopeTableEntry(const NameScopeTableEntry& other);
            NameScopeTableEntry& operator=(const NameScopeTableEntry& other);

            NameScopeTableEntry(NameScopeTableEntry&& other) noexcept;
            NameScopeTableEntry& operator=(NameScopeTableEntry&& other) noexcept;

        private:

            void ClearResources();
            void MoveResourcesFrom(NameScopeTableEntry&& other);
            void CopyResourcesFrom(const NameScopeTableEntry& other);

            enum class Tag
            {
                Empty,
                StrongRef,
                WeakRef,
                DeferredElementCreator
            };

            void SetTagAndConstructResources(Tag tag);

            const xref_ptr<CDependencyObject>& AsXRef() const;
            const xref::weakref_ptr<CDependencyObject>& AsXWeakRef() const;
            const std::weak_ptr<INameScopeDeferredElementCreator>& AsNameScopeElementCreatorWeakRef() const;

            xref_ptr<CDependencyObject>& AsXRef();
            xref::weakref_ptr<CDependencyObject>& AsXWeakRef();
            std::weak_ptr<INameScopeDeferredElementCreator>& AsNameScopeElementCreatorWeakRef();

            Tag m_tag = Tag::Empty;
            uint8_t m_storage[c_storageSize];
        };

        static_assert(c_storageSize >= sizeof(xref_ptr<CDependencyObject>), "Storage in NameScopeTableEntry must be large enough for its unioned types.");
        static_assert(c_storageSize >= sizeof(xref::weakref_ptr<CDependencyObject>), "Storage in NameScopeTableEntry must be large enough for its unioned types.");
        static_assert(c_storageSize >= sizeof(std::weak_ptr<INameScopeDeferredElementCreator>), "Storage in NameScopeTableEntry must be large enough for its unioned types.");
    }
}