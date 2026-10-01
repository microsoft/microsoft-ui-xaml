// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#pragma once

#include <type_traits>
#include <utility>
#include <xref_ptr.h>

// A uniquely owned native resource and the owner required by its
// consuming cleanup. Traits is the only adoption authority; it must verify
// provenance and keep the original context valid. No thread dispatch is implied.
template<class Owner, class Resource, class Traits>
class DependentResource
{
    static_assert(std::is_pointer<Resource>::value, "This holder uses nullptr as its empty resource.");
    friend Traits;

public:
    DependentResource() noexcept = default;
    DependentResource(const DependentResource&) = delete;
    DependentResource& operator=(const DependentResource&) = delete;

    DependentResource(DependentResource&& other) noexcept
        : m_owner(std::move(other.m_owner)),
          m_resource(std::exchange(other.m_resource, nullptr))
    {
    }

    DependentResource& operator=(DependentResource&& other) noexcept
    {
        if (this != &other)
        {
            // Publish the complete incoming pair before old cleanup can reenter.
            DependentResource old(std::move(*this));
            m_owner = std::move(other.m_owner);
            m_resource = std::exchange(other.m_resource, nullptr);
        }
        return *this;
    }

    ~DependentResource() { Reset(); }

    Resource Get() const noexcept { return m_resource; }

    void Reset() noexcept
    {
        auto owner = std::move(m_owner);
        auto resource = std::exchange(m_resource, nullptr);
        if (resource)
        {
            static_assert(noexcept(Traits::Destroy(std::declval<Owner&>(), std::declval<Resource>())),
                "Cleanup must not throw.");
            Traits::Destroy(*owner, resource);
        }
        // The local owner releases only after consuming cleanup has returned.
    }

private:
    static DependentResource Adopt(Owner& origin, Resource& producerSlot) noexcept
    {
        DependentResource result;
        // A zero-length borrowed view can be null while still requiring its lock.
        // Adapters with no resource/owner obligation return a default holder instead.
        result.m_owner = xref_ptr<Owner>(&origin);
        result.m_resource = std::exchange(producerSlot, nullptr);
        return result;
    }

    xref_ptr<Owner> m_owner;
    Resource m_resource = nullptr;
};
