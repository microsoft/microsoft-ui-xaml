// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#include "precomp.h"
#include "DeferredMapping.h"

// #define ENABLE_LOGGING

#ifdef ENABLE_LOGGING
  #define DBG_LOG(...) LOG(__VA_ARGS__)
#else
  #define DBG_LOG(...)
#endif

// CDeferredStorage

CDeferredStorage::StorageType::iterator CDeferredStorage::FindStorage(
    _In_ CDependencyObject* parent,
    _In_ KnownPropertyIndex index,
    _Out_ bool* found)
{
    *found = false;

    StorageType::iterator iter = std::lower_bound(
        m_deferredElementStorage.begin(),
        m_deferredElementStorage.end(),
        std::make_pair(parent, index),
        [](const std::shared_ptr<CDeferredElementStorage>& lhs, std::pair<CDependencyObject*, KnownPropertyIndex> rhs) -> bool
    {
        if (lhs->GetParent() < rhs.first)
        {
            return true;
        }
        else if (lhs->GetParent() == rhs.first)
        {
            return lhs->GetPropertyIndex() < rhs.second;
        }
        else
        {
            return false;
        }
    });

    *found = iter != m_deferredElementStorage.end() &&
        (*iter)->GetParent() == parent &&
        (*iter)->GetPropertyIndex() == index;

    return iter;
}

_Check_return_ HRESULT CDeferredStorage::AddProxy(
    _In_ CDependencyObject* declaringScope,
    _In_ CDependencyObject* parent,
    _In_opt_ CDependencyObject* parentCollection,
    _In_ KnownPropertyIndex index,
    _In_ CDeferredElement* proxy)
{
    bool found = false;
    auto iter = FindStorage(parent, index, &found);

    if (!found)
    {
        auto storage =
            std::make_shared<CDeferredElementStorage>(
                this,
                parent,
                index);

        iter = m_deferredElementStorage.emplace(
            iter,
            std::move(storage));
    }

    ASSERT(iter != m_deferredElementStorage.end());

    CCollection* collection = do_pointer_cast<CCollection>(parentCollection);
    UINT32 childrenCount = 0;

    if (collection)
    {
        childrenCount = collection->GetCount();
        collection->SetChangeCallback(*iter);
    }

    IFC_RETURN((*iter)->AppendDeferredElement(proxy, childrenCount));

    CDeferredMapping::MapElementToDeclaringScope(
        parent->GetContext(),
        parent,
        declaringScope);

    IFC_RETURN(proxy->PostAppendInit());

    return S_OK;
}

_Check_return_ HRESULT CDeferredStorage::RemoveStorage(
    _In_ CDependencyObject* parent,
    _In_ KnownPropertyIndex index)
{
    StorageType::iterator firstParentProperty = m_deferredElementStorage.end();
    StorageType::iterator lastParentProperty = firstParentProperty;

    DBG_LOG(
        L"CDeferredStorage::RemoveStorage [%p], parent/index [%p/%u]",
        this,
        parent,
        index);

    if (index == KnownPropertyIndex::UnknownType_UnknownProperty)
    {
        bool found = false;

        // since unknown property has the lowest index, it will not be found, but will give iterator to where the first parent would be
        firstParentProperty = FindStorage(parent, KnownPropertyIndex::UnknownType_UnknownProperty, &found);
        lastParentProperty = firstParentProperty;

        // advance to the last entry for the same parent
        // they are sorted according to parent, index - guaranteed to be consecutive
        while (lastParentProperty != m_deferredElementStorage.end() &&
               (*lastParentProperty)->GetParent() == parent)
        {
            ++lastParentProperty;
        }
    }
    else
    {
        bool found = false;
        firstParentProperty = FindStorage(parent, index, &found);

        if (found)
        {
            // This should be the case if it has been found.
            ASSERT(firstParentProperty != m_deferredElementStorage.end());
            lastParentProperty = std::next(firstParentProperty);
        }
    }

    if (firstParentProperty != m_deferredElementStorage.end())
    {
        m_deferredElementStorage.erase(firstParentProperty, lastParentProperty);
    }

    return S_OK;
}

_Check_return_ HRESULT CDeferredStorage::NotifyParentValueSet(
    _In_ CDependencyObject* parent,
    _In_ KnownPropertyIndex index)
{
    bool found = false;
    auto iter = FindStorage(parent, index, &found);

    if (found)
    {
        // This should be the case if it has been found.
        ASSERT(iter != m_deferredElementStorage.end());
        IFC_RETURN((*iter)->ValueSet());
    }

    return S_OK;
}

_Check_return_ HRESULT CDeferredStorage::RegisterUnrealizedProxyNames(
    _In_ CDependencyObject* parent)
{
    bool found = false;
    auto iter = FindStorage(parent, KnownPropertyIndex::UnknownType_UnknownProperty, &found);

    if (iter != m_deferredElementStorage.end() &&
        (*iter)->GetParent() == parent)
    {
        for (auto sameParent = iter; sameParent != m_deferredElementStorage.end() && (*sameParent)->GetParent() == parent; ++sameParent)
        {
            IFC_RETURN((*sameParent)->RegisterUnrealizedProxies());
        }
    }

    return S_OK;
}

_Check_return_ HRESULT CDeferredStorage::UnregisterUnrealizedProxyNames(
    _In_ CDependencyObject* parent)
{
    bool found = false;
    auto iter = FindStorage(parent, KnownPropertyIndex::UnknownType_UnknownProperty, &found);

    if (iter != m_deferredElementStorage.end() &&
        (*iter)->GetParent() == parent)
    {
        for (auto sameParent = iter; sameParent != m_deferredElementStorage.end() && (*sameParent)->GetParent() == parent; ++sameParent)
        {
            IFC_RETURN((*sameParent)->UnregisterUnrealizedProxies());
        }
    }

    return S_OK;
}

// CDeferredMapping
CDeferredMapping::~CDeferredMapping()
{
    // Bug 18096379: XamlObjects events can be fired after its parent Island has been Disposed
    LOG_ASSERT(m_elementToInfo.empty(), "If there are no leaks, everything should be removed by now.");
}

bool CDeferredMapping::Empty() const
{
    return m_elementToInfo.empty();
}

_Check_return_ HRESULT CDeferredMapping::NotifyEnter(
    _In_ CDependencyObject* newNamescope,
    _In_ CDependencyObject* element,
    _In_ bool skipNameRegistration)
{
    DBG_LOG(
        L"CDeferredMapping::NotifyEnter, element [%p] '%s', namescope [%p], skipNameRegistration [%d]",
        element,
        element->m_strName.GetBuffer(),
        newNamescope,
        skipNameRegistration);

    CCoreServices* core = element->GetContext();
    CDeferredMapping::ElementToInfoMap& mapping = core->GetDeferredMapping()->m_elementToInfo;
    auto iter = mapping.find(element);

    ASSERT(iter != mapping.end());
    ASSERT(element->HasDeferred());

    xref_ptr<CDependencyObject> declaringScope = iter->second.m_declaringScope.lock();

    // Since call to this method will be made if HasDeferred() is true AND the flag is set for either declaring scope or parent, check if this is for declaring scope.
    if (declaringScope)
    {
        if (iter->second.m_enteredScope == nullptr)
        {
            iter->second.m_enteredScope = newNamescope;
        }
        else if (!skipNameRegistration)
        {
            CDependencyObject* enteredNamescope = iter->second.m_enteredScope;

            if (declaringScope == newNamescope ||
                (enteredNamescope && enteredNamescope == newNamescope))
            {
                IFC_RETURN(GetScopeDeferredStorage(core, declaringScope)->RegisterUnrealizedProxyNames(element));
            }
        }
    }

    return S_OK;
}

_Check_return_ HRESULT CDeferredMapping::NotifyLeave(
    _In_ CDependencyObject* oldNamescope,
    _In_ CDependencyObject* element,
    _In_ bool skipNameRegistration)
{
    DBG_LOG(L"CDeferredMapping::NotifyLeave, element [%p] '%s', namescope [%p], skipNameRegistration [%d]",
        element,
        element->m_strName.GetBuffer(),
        oldNamescope,
        skipNameRegistration);

    if (!skipNameRegistration)
    {
        CCoreServices* core = element->GetContext();
        CDeferredMapping::ElementToInfoMap& mapping = core->GetDeferredMapping()->m_elementToInfo;
        auto iter = mapping.find(element);

        ASSERT(iter != mapping.end());
        ASSERT(element->HasDeferred());

        xref_ptr<CDependencyObject> declaringScope = iter->second.m_declaringScope.lock();

        // Since call to this method will be made if HasDeferred() is true AND the flag is set for either declaring scope or parent, check if this is for declaring scope.
        if (declaringScope)
        {
            CDependencyObject* enteredNamescope = iter->second.m_enteredScope;

            if (declaringScope == oldNamescope ||
                (enteredNamescope && enteredNamescope == oldNamescope))
            {
                IFC_RETURN(GetScopeDeferredStorage(core, declaringScope)->UnregisterUnrealizedProxyNames(element));
            }
        }
    }

    return S_OK;
}

_Check_return_ HRESULT CDeferredMapping::NotifyParentValueSet(
    _In_ CDependencyObject* parent,
    _In_ KnownPropertyIndex index)
{
    CCoreServices* core = parent->GetContext();
    CDeferredMapping::ElementToInfoMap& mapping = core->GetDeferredMapping()->m_elementToInfo;
    auto iter = mapping.find(parent);

    ASSERT(iter != mapping.end());
    ASSERT(parent->HasDeferred());

    xref_ptr<CDependencyObject> declaringScope = iter->second.m_declaringScope.lock();

    // Since call to this method will be made if HasDeferred() is true AND the flag is set for either declaring scope or parent, check if this is for declaring scope.
    if (declaringScope)
    {
        IFC_RETURN(GetScopeDeferredStorage(core, declaringScope)->NotifyParentValueSet(parent, index));
    }

    return S_OK;
}

_Check_return_ HRESULT CDeferredMapping::NotifyDestroyed(
    _In_ CDependencyObject* element)
{
    // The following references to element being destroyed may exist, and need to be removed:
    // 1. CDeferredMapping entry for this element (always)
    // 2. Declaring scope's storage has pointers stored to this object, if it is a deferred element parent

    CCoreServices* core = element->GetContext();
    CDeferredMapping::ElementToInfoMap& mapping = core->GetDeferredMapping()->m_elementToInfo;
    auto iter = mapping.find(element);

    ASSERT(iter != mapping.end());
    ASSERT(element->HasDeferred());

    xref_ptr<CDependencyObject> declaringScope = iter->second.m_declaringScope.lock();

    auto declaringScopeDeferredStorage = std::move(iter->second.m_declaringScopeDeferredStorage);

    DBG_LOG(
        L"CDeferredMapping::NotifyDestroyed [%p] '%s', declaringScope [%p], deferredStorage [%p]",
        element,
        element->m_strName.GetBuffer(),
        declaringScope,
        declaringScopeDeferredStorage.get());

    // 1. Remove map entry for this element and unmark it.

    element->SetHasDeferred(false);
    mapping.erase(iter); // iter is not valid after this!

    if (declaringScope)
    {
        // 2. For parent, remove references from declaring scope's storage

        if (declaringScope == element)
        {
            // Parent is declaring scope.  It was already removed from the map earlier in this method, so GetScopeDeferredStorage would not work, but we saved it...
            ASSERT(declaringScopeDeferredStorage);
            IFC_RETURN(declaringScopeDeferredStorage->RemoveStorage(element, KnownPropertyIndex::UnknownType_UnknownProperty));
        }
        else
        {
            IFC_RETURN(GetScopeDeferredStorage(core, declaringScope)->RemoveStorage(element, KnownPropertyIndex::UnknownType_UnknownProperty));
        }
    }

    return S_OK;
}

void CDeferredMapping::MapElementToDeclaringScope(
    _In_ CCoreServices* core,
    _In_ CDependencyObject* element,
    _In_ CDependencyObject* declaringScope)
{
    CDeferredMapping::ElementToInfoMap& mapping = core->GetDeferredMapping()->m_elementToInfo;
    auto& entry = mapping[element];

    if (!entry.m_declaringScope)
    {
        DBG_LOG(
            L"CDeferredMapping::MapElementToDeclaringScope, declaringScope [%p], element [%p] '%s'",
            declaringScope,
            element,
            element->m_strName.GetBuffer());

        entry.m_declaringScope = xref::get_weakref(declaringScope);
        entry.m_enteredScope = nullptr;

        element->SetHasDeferred(true);
    }
}

CDeferredStorage& CDeferredMapping::EnsureScopeDeferredStorage(
    _In_ CDependencyObject* declaringScope)
{
    CCoreServices* core = declaringScope->GetContext();
    CDeferredMapping::ElementToInfoMap& mapping = core->GetDeferredMapping()->m_elementToInfo;
    auto& entry = mapping[declaringScope];

    if (!entry.m_declaringScopeDeferredStorage)
    {
        entry.m_declaringScopeDeferredStorage.reset(new CDeferredStorage());
        declaringScope->SetHasDeferred(true);

        DBG_LOG(
            L"CDeferredMapping::EnsureScopeDeferredStorage, declaringScope [%p], CDeferredStorage [%p]",
            declaringScope,
            entry.m_declaringScopeDeferredStorage.get());
    }

    return *entry.m_declaringScopeDeferredStorage;
}

CDeferredStorage* CDeferredMapping::GetScopeDeferredStorage(
    _In_ CCoreServices* core,
    _In_ CDependencyObject* declaringScope)
{
    ElementToInfoMap& mapping = core->GetDeferredMapping()->m_elementToInfo;
    auto iter = mapping.find(declaringScope);
    ASSERT(iter != mapping.end());
    return iter->second.m_declaringScopeDeferredStorage.get();
}
