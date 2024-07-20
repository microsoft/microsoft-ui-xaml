// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#pragma once

#include <unordered_set>

class CDependencyObject;

// Helper object that stores a list of CDependencyObjects that created DComp animation objects. These have to be
// cleaned up on device loss. Meant to be inlined.
//
// The pointers to the objects do not take a reference. An object should call Register when it makes its first
// DComp resource and call Unregister when it is destroyed. It's okay if an object stays registered even after
// it has released its DComp resources - the cleanup will just no-op for that object.
//
// Note: RS device lost doesn't require us to release the WUC compositor, so this registry is no longer needed in
// the normal case. If Xaml ever forces a device loss that releases the compositor, then this registry is still
// needed. Currently MockDComp injection forces the compositor to be released, so this is still needed.
class DependencyObjectDCompRegistry
{
public:
    // Registers a DO that holds DComp resources, if not already registered. The registered DO will be cleaned up
    // on device lost.
    void EnsureObjectWithDCompResourceRegistered(_In_ CDependencyObject* pObject);

    // Unregisters a DO that no longer holds DComp resources. This should be called on object destruction so that
    // the list does not grow indefinitely. It is not necessary to call this whenever an object discards its DComp
    // resources.
    void UnregisterObject(_In_ CDependencyObject* pObject);

    // Releases DComp resources on all registered DOs and clears the list. If a DO creates more DComp resources,
    // it will register itself again.
    void ReleaseDCompResources();

    // Clears the entries without calling them to release their DComp resources. Used on app shutdown to prevent
    // calling back into released objects from the property system.
    void AbandonRegistry();

private:
    // Objects are removed from this map when they're deleted, and weakref_ptr can return null if it detects that
    // the target object is being deleted, so we can't use the weak ref as the key. Instead, use the raw pointer
    // as the key and store the weak ref as the value.
    std::unordered_map<CDependencyObject*, xref::weakref_ptr<CDependencyObject>> m_objectsWithDCompResourcesNoRef;
};
