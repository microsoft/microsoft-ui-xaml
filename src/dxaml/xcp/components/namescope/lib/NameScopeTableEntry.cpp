// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#include "precomp.h"

#include <NameScopeTableEntry.h>

#include <CDependencyObject.h>
#include <INameScopeDeferredElementCreator.h>

namespace Jupiter {
    namespace NameScoping {
        NameScopeTableEntry::NameScopeTableEntry()
            : m_storage()
        {
        }

        NameScopeTableEntry::NameScopeTableEntry(const NameScopeTableEntry& other)
            : NameScopeTableEntry()
        {
            CopyResourcesFrom(other);
        }

        NameScopeTableEntry& NameScopeTableEntry::operator=(const NameScopeTableEntry& other)
        {
            if (this != &other)
            {
                ClearResources();
                CopyResourcesFrom(other);
            }

            return *this;
        }

        NameScopeTableEntry::NameScopeTableEntry(xref_ptr<CDependencyObject> element)
            : NameScopeTableEntry()
        {
            ASSERT(element);
            auto refPtr = new (m_storage)xref_ptr<CDependencyObject>();
            *refPtr = std::move(element);
            m_tag = Tag::StrongRef;
        }

        NameScopeTableEntry::NameScopeTableEntry(xref::weakref_ptr<CDependencyObject> weakElement)
            : NameScopeTableEntry()
        {
            auto weakRefPtr = new (m_storage)xref::weakref_ptr<CDependencyObject>();
            *weakRefPtr = std::move(weakElement);
            m_tag = Tag::WeakRef;
        }

        NameScopeTableEntry::NameScopeTableEntry(std::weak_ptr<INameScopeDeferredElementCreator> deferredCreator)
            : NameScopeTableEntry()
        {
            auto deferredPtr = new (m_storage)std::weak_ptr<INameScopeDeferredElementCreator>();
            *deferredPtr = std::move(deferredCreator);
            m_tag = Tag::DeferredElementCreator;
        }

        NameScopeTableEntry::NameScopeTableEntry(NameScopeTableEntry&& other) noexcept
            : NameScopeTableEntry()
        {
            MoveResourcesFrom(std::move(other));
        }
        
        xref_ptr<CDependencyObject> NameScopeTableEntry::TryGetElement(_Out_ bool* shouldRetry) const
        {
            *shouldRetry = false;

            switch (m_tag)
            {
                case Tag::StrongRef:
                    return AsXRef();
                case Tag::WeakRef:
                {
                    auto& weakRef = AsXWeakRef();
                    // It's important to check if we're expired- lock will ASSERT if we're
                    // being called from the dtor, which is in fact the site at which namescope
                    // entries unregister themselves.
                    return !weakRef.expired() ? weakRef.lock() : nullptr;
                }
                case Tag::DeferredElementCreator:
                {
                    auto locked = AsNameScopeElementCreatorWeakRef().lock();
                    return locked ? locked->TryGetOrCreateElement(shouldRetry) : nullptr;                
                }
                default:
                    ASSERT(m_tag == Tag::Empty);
                    return nullptr;
            }
        }

        NameScopeTableEntry::~NameScopeTableEntry()
        {
            ClearResources();
        }

        void NameScopeTableEntry::ClearResources()
        {
            switch(m_tag)
            {
                case Tag::StrongRef:
                {
                    AsXRef().~xref_ptr<CDependencyObject>();
                    break;  
                }
                case Tag::WeakRef:
                {
                    AsXWeakRef().~weakref_ptr<CDependencyObject>();
                    break;
                }
                case Tag::DeferredElementCreator:
                {
                    AsNameScopeElementCreatorWeakRef().~weak_ptr<INameScopeDeferredElementCreator>();
                    break;
                }
            }
            m_tag = Tag::Empty;
        }

        void NameScopeTableEntry::SetTagAndConstructResources(Tag tag)
        {
            ASSERT(m_tag == Tag::Empty);
            switch (tag)
            {
                case Tag::StrongRef:
                {
                    new (m_storage)xref_ptr<CDependencyObject>();
                    break;
                }
                case Tag::WeakRef:
                {
                    new (m_storage)xref::weakref_ptr<CDependencyObject>();
                    break;
                }
                case Tag::DeferredElementCreator:
                {
                    new (m_storage)std::weak_ptr<INameScopeDeferredElementCreator>();
                    break;
                }
            }
            m_tag = tag;
        }

        void NameScopeTableEntry::MoveResourcesFrom(NameScopeTableEntry&& other)
        {
            SetTagAndConstructResources(other.m_tag);
            switch (other.m_tag)
            {
                case Tag::StrongRef:
                {
                    AsXRef() = std::move(other.AsXRef());
                    break;
                }
                case Tag::WeakRef:
                {
                    AsXWeakRef() = std::move(other.AsXWeakRef());
                    break;
                }
                case Tag::DeferredElementCreator:
                {
                    AsNameScopeElementCreatorWeakRef() = std::move(other.AsNameScopeElementCreatorWeakRef());
                    break;
                }
            }
        }

        void NameScopeTableEntry::CopyResourcesFrom(const NameScopeTableEntry& other)
        {
            SetTagAndConstructResources(other.m_tag);
            switch (other.m_tag)
            {
                case Tag::StrongRef:
                {
                    AsXRef() = other.AsXRef();
                    break;
                }
                case Tag::WeakRef:
                {
                    AsXWeakRef() = other.AsXWeakRef();
                    break;
                }
                case Tag::DeferredElementCreator:
                {
                    AsNameScopeElementCreatorWeakRef() = other.AsNameScopeElementCreatorWeakRef();
                    break;
                }
            }
        }

        NameScopeTableEntry& NameScopeTableEntry::operator=(NameScopeTableEntry&& other)
        {
            if (this != &other)
            {
                ClearResources();
                MoveResourcesFrom(std::move(other));
            }

            return *this;
        }

        const xref_ptr<CDependencyObject>& NameScopeTableEntry::AsXRef() const
        {
            ASSERT(m_tag == Tag::StrongRef);
            return *(reinterpret_cast<const xref_ptr<CDependencyObject>*>(m_storage));
        }

        const xref::weakref_ptr<CDependencyObject>& NameScopeTableEntry::AsXWeakRef() const
        {
            ASSERT(m_tag == Tag::WeakRef);
            return *(reinterpret_cast<const xref::weakref_ptr<CDependencyObject>*>(m_storage));
        }

        const std::weak_ptr<INameScopeDeferredElementCreator>& NameScopeTableEntry::AsNameScopeElementCreatorWeakRef() const
        {
            ASSERT(m_tag == Tag::DeferredElementCreator);
            return *(reinterpret_cast<const std::weak_ptr<INameScopeDeferredElementCreator>*>(m_storage));
        }

        xref_ptr<CDependencyObject>& NameScopeTableEntry::AsXRef()
        {
            ASSERT(m_tag == Tag::StrongRef);
            return *(reinterpret_cast<xref_ptr<CDependencyObject>*>(m_storage));
        }

        xref::weakref_ptr<CDependencyObject>& NameScopeTableEntry::AsXWeakRef()
        {
            ASSERT(m_tag == Tag::WeakRef);
            return *(reinterpret_cast<xref::weakref_ptr<CDependencyObject>*>(m_storage));
        }

        std::weak_ptr<INameScopeDeferredElementCreator>& NameScopeTableEntry::AsNameScopeElementCreatorWeakRef()
        {
            ASSERT(m_tag == Tag::DeferredElementCreator);
            return *(reinterpret_cast<std::weak_ptr<INameScopeDeferredElementCreator>*>(m_storage));
        }
    }
}