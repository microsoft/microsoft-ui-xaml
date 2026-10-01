// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#include "precomp.h"
#include "DependentResourceUnitTests.h"
#include <DependentResource.h>
#include <functional>
#include <type_traits>

namespace
{
    struct Lifetime
    {
        unsigned references = 1;
        unsigned cleanups = 0;
        unsigned releases = 0;
        std::function<void()> onCleanup;
    };

    struct Owner
    {
        Lifetime& lifetime;
        void AddRef() { ++lifetime.references; }
        void Release()
        {
            if (--lifetime.references == 0)
            {
                ++lifetime.releases;
                delete this;
            }
        }
    };

    struct Resource
    {
        Owner* origin;
    };

    struct Traits
    {
        using Handle = DependentResource<Owner, Resource*, Traits>;

        static Handle Create(Owner& owner, Resource*& slot)
        {
            return slot ? Handle::Adopt(owner, slot) : Handle{};
        }

        static Handle CreateNullView(Owner& owner)
        {
            Resource* view = nullptr;
            return Handle::Adopt(owner, view);
        }
        static void Destroy(Owner& owner, Resource* resource) noexcept
        {
            VERIFY_IS_TRUE(&owner == resource->origin);
            VERIFY_IS_TRUE(owner.lifetime.references > 0);
            ++owner.lifetime.cleanups;
            if (owner.lifetime.onCleanup) { owner.lifetime.onCleanup(); }
            VERIFY_ARE_EQUAL(0u, owner.lifetime.releases);
        }
    };

    using Handle = Traits::Handle;
    static_assert(!std::is_copy_constructible<Handle>::value, "A resource has one cleanup owner.");
    static_assert(std::is_nothrow_move_constructible<Handle>::value, "Handoff cannot fail.");
    static_assert(std::is_nothrow_move_assignable<Handle>::value, "Handoff cannot fail.");
}

void DependentResourceUnitTests::SharedOwnerSurvivesUntilLastCleanup()
{
    Lifetime lifetime;
    auto owner = new Owner{lifetime};
    Resource first{owner}, second{owner};
    auto firstSlot = &first;
    auto secondSlot = &second;
    auto a = Traits::Create(*owner, firstSlot);
    auto b = Traits::Create(*owner, secondSlot);
    VERIFY_IS_NULL(firstSlot);
    VERIFY_IS_NULL(secondSlot);
    owner->Release();
    a.Reset();
    VERIFY_ARE_EQUAL(0u, lifetime.releases);
    b.Reset();
    VERIFY_ARE_EQUAL(2u, lifetime.cleanups);
    VERIFY_ARE_EQUAL(1u, lifetime.releases);
}

void DependentResourceUnitTests::MoveTransfersTheWholePair()
{
    Lifetime firstLifetime, secondLifetime;
    auto firstOwner = new Owner{firstLifetime};
    auto secondOwner = new Owner{secondLifetime};
    Resource first{firstOwner}, second{secondOwner};
    auto firstSlot = &first;
    auto secondSlot = &second;
    auto source = Traits::Create(*firstOwner, firstSlot);
    auto target = Traits::Create(*secondOwner, secondSlot);
    firstOwner->Release();
    secondOwner->Release();
    Handle moved(std::move(source));
    VERIFY_IS_NULL(source.Get());
    target = std::move(moved);
    VERIFY_IS_NULL(moved.Get());
    VERIFY_ARE_EQUAL(1u, secondLifetime.cleanups);
    VERIFY_ARE_EQUAL(1u, secondLifetime.releases);
    auto& self = target;
    target = std::move(self);
    VERIFY_IS_TRUE(target.Get() == &first);
    target.Reset();
    target.Reset();
    VERIFY_ARE_EQUAL(1u, firstLifetime.cleanups);
    VERIFY_ARE_EQUAL(1u, firstLifetime.releases);
}

void DependentResourceUnitTests::ReentrantResetSeesEmptyState()
{
    Lifetime lifetime;
    auto owner = new Owner{lifetime};
    Resource resource{owner};
    auto slot = &resource;
    auto handle = Traits::Create(*owner, slot);
    owner->Release();
    lifetime.onCleanup = [&handle]
    {
        VERIFY_IS_NULL(handle.Get());
        handle.Reset();
    };
    handle.Reset();
    VERIFY_ARE_EQUAL(1u, lifetime.cleanups);
    VERIFY_ARE_EQUAL(1u, lifetime.releases);
}

void DependentResourceUnitTests::ReentrantMovePreservesReplacement()
{
    Lifetime oldLifetime, incomingLifetime, replacementLifetime;
    auto oldOwner = new Owner{oldLifetime};
    auto incomingOwner = new Owner{incomingLifetime};
    auto replacementOwner = new Owner{replacementLifetime};
    Resource oldResource{oldOwner}, incomingResource{incomingOwner}, replacementResource{replacementOwner};
    auto oldSlot = &oldResource;
    auto incomingSlot = &incomingResource;
    auto replacementSlot = &replacementResource;
    auto target = Traits::Create(*oldOwner, oldSlot);
    auto incoming = Traits::Create(*incomingOwner, incomingSlot);
    auto replacement = Traits::Create(*replacementOwner, replacementSlot);
    oldOwner->Release();
    incomingOwner->Release();
    replacementOwner->Release();
    oldLifetime.onCleanup = [&] { target = std::move(replacement); };
    target = std::move(incoming);
    VERIFY_IS_TRUE(target.Get() == &replacementResource);
    VERIFY_ARE_EQUAL(1u, oldLifetime.releases);
    VERIFY_ARE_EQUAL(1u, incomingLifetime.releases);
    target.Reset();
    VERIFY_ARE_EQUAL(1u, replacementLifetime.releases);
}

void DependentResourceUnitTests::GuardUnwindsBeforeOwner()
{
    Lifetime lifetime;
    auto owner = new Owner{lifetime};
    Resource resource{owner};
    auto slot = &resource;
    {
        auto guard = Traits::Create(*owner, slot);
        owner->Release();
        // Models leaving the factory before the wrapper can accept the guard.
    }
    VERIFY_IS_NULL(slot);
    VERIFY_ARE_EQUAL(1u, lifetime.cleanups);
    VERIFY_ARE_EQUAL(1u, lifetime.releases);
}

void DependentResourceUnitTests::EmptyAdoptionDoesNotRetainOwner()
{
    Lifetime lifetime;
    auto owner = new Owner{lifetime};
    Resource* slot = nullptr;
    auto handle = Traits::Create(*owner, slot);
    VERIFY_ARE_EQUAL(1u, lifetime.references);
    owner->Release();
    handle.Reset();
    VERIFY_ARE_EQUAL(0u, lifetime.cleanups);
    VERIFY_ARE_EQUAL(1u, lifetime.releases);
}

void DependentResourceUnitTests::NullViewCanRetainItsLock()
{
    Lifetime lifetime;
    auto owner = new Owner{lifetime};
    auto handle = Traits::CreateNullView(*owner);
    owner->Release();
    VERIFY_ARE_EQUAL(0u, lifetime.releases);
    Handle moved(std::move(handle));
    handle.Reset();
    VERIFY_ARE_EQUAL(0u, lifetime.releases);
    moved.Reset();
    VERIFY_ARE_EQUAL(0u, lifetime.cleanups);
    VERIFY_ARE_EQUAL(1u, lifetime.releases);
}
