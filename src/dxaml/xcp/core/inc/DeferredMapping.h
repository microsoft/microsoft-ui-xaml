// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#pragma once

class CDeferredElementStorage;

// This class provides storage of proxies for parents in a declaring scope.  (Parent, property id) key maps to an
// instance of CDeferredElementStorage which stores the actual proxies.  Currently, they are stored in a sorted vector.
class CDeferredStorage
{
public:
    // Called from parser to add proxy for parent/property id pair and registers self as parent's storage in map help in Core.
    _Check_return_ HRESULT AddProxy(
        _In_ CDependencyObject* declaringScope,
        _In_ CDependencyObject* parent,
        _In_opt_ CDependencyObject* parentCollection,
        _In_ KnownPropertyIndex index,
        _In_ CDeferredElement* proxy);

    // Removes storage for a given parent (if propertyIndex = UnknownType_UnknownProperty) or parent/property id.
    _Check_return_ HRESULT RemoveStorage(
        _In_ CDependencyObject* parent,
        _In_ KnownPropertyIndex propertyIndex);

    // Called when DO's property value is set.
    _Check_return_ HRESULT NotifyParentValueSet(
        _In_ CDependencyObject* parent,
        _In_ KnownPropertyIndex index);

    _Check_return_ HRESULT RegisterUnrealizedProxyNames(
        _In_ CDependencyObject* parent);

    _Check_return_ HRESULT UnregisterUnrealizedProxyNames(
        _In_ CDependencyObject* parent);

private:
    typedef std::vector<std::shared_ptr<CDeferredElementStorage>> StorageType;

    // Helper for finding storage for a given parent/property id.
    // If found, return iterator point to is and found = true, otherwise return iterator points to the point where such parent/id pair should be inserted.
    StorageType::iterator FindStorage(
        _In_ CDependencyObject* parent,
        _In_ KnownPropertyIndex index,
        _Out_ bool* found);

    StorageType m_deferredElementStorage;
};

// Maintains external storage of deferral related data for CDOs.  The two kinds which have data stored here
// are declaring scopes and parents of deferred elements (an element can be both).  For declaring scope
// data stored is a pointer to CDeferredStorage which stores proxies.  In case of parents, it keeps a back
// pointer to declaring scope and entered scope.  These are necessary to relay enter/leave and destruction
// notifications to CDeferredStorage.
// DeferredInfo is created by either mapping element to scope or by creating declaring scope deferred storage.
// It is removed from map during object's destruction.
class CDeferredMapping
{
public:
    ~CDeferredMapping();

    bool Empty() const;

    // Methods for parent containing deferred properties

    // Associates parent element with declaring scope.
    static void MapElementToDeclaringScope(
        _In_ CCoreServices* core,
        _In_ CDependencyObject* element,
        _In_ CDependencyObject* declaringScope);

    // Relays the call to appropriate parent/index storage.
    static _Check_return_ HRESULT NotifyParentValueSet(
        _In_ CDependencyObject* parent,
        _In_ KnownPropertyIndex index);

    // Called when parent DO is processing Enter.
    static _Check_return_ HRESULT NotifyEnter(
        _In_ CDependencyObject* newNamescope,
        _In_ CDependencyObject* element,
        _In_ bool skipNameRegistration);

    // Called when parent DO is processing Leave.
    static _Check_return_ HRESULT NotifyLeave(
        _In_ CDependencyObject* oldNamescope,
        _In_ CDependencyObject* element,
        _In_ bool skipNameRegistration);

    // Methods for scope storage

    static CDeferredStorage& EnsureScopeDeferredStorage(
        _In_ CDependencyObject* declaringScope);

    // Methods for both

    // Called when parent DO is being destroyed.
    static _Check_return_ HRESULT NotifyDestroyed(
        _In_ CDependencyObject* element);

private:
    struct DeferredInfo
    {
        DeferredInfo()
            : m_enteredScope(nullptr)
        {}

        DeferredInfo(DeferredInfo&& other)
            : m_declaringScope(std::move(other.m_declaringScope))
            , m_enteredScope(other.m_enteredScope)
            , m_declaringScopeDeferredStorage(std::move(other.m_declaringScopeDeferredStorage))
        {
            other.m_enteredScope = nullptr;
        }

        // If m_declaringScope is set - this is parent of deferred element declared in declaringScope.
        // If m_declaringScopeDeferredStorage is set - this is declaring scope.
        xref::weakref_ptr<CDependencyObject> m_declaringScope;
        CDependencyObject* m_enteredScope;
        std::unique_ptr<CDeferredStorage> m_declaringScopeDeferredStorage;
    };

    typedef std::unordered_map<CDependencyObject*, DeferredInfo> ElementToInfoMap;

    static CDeferredStorage* GetScopeDeferredStorage(
        _In_ CCoreServices* core,
        _In_ CDependencyObject* declaringScope);

    ElementToInfoMap m_elementToInfo;
};