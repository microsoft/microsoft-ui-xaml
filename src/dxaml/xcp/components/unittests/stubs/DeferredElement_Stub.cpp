// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#include "precomp.h"

// Windows headers define "GetClassName" to "GetClassNameW", so CDependencyObject::GetClassName in the stub
// becomes CDependencyObject::GetClassNameW, which then fails to link against the real CDO::GetClassName
#ifdef GetClassName
#undef GetClassName
#endif

#include <ICollectionChangeCallback.h>
#include <DeferredElement.h>
#include <DeferredMapping.h>
#include <CustomWriterRuntimeContext.h>


CDeferredElement::~CDeferredElement()
{
}

CDependencyObject* CDeferredElement::GetParent() const
{
    return nullptr;
}

CDependencyObject* CDeferredElement::GetDeclaringOwner()
{
    return nullptr;
}

_Check_return_ HRESULT CDeferredElement::Realize(
    _Outptr_ CDependencyObject** ppResult)
{
    return E_NOTIMPL;
}

_Check_return_ HRESULT CDeferredElement::Defer()
{
    return E_NOTIMPL;
}

_Check_return_ HRESULT CDeferredElement::RegisterProxyName(bool force)
{
    return E_NOTIMPL;
}

_Check_return_ HRESULT CDeferredElement::UnregisterProxyName(bool force)
{
    return E_NOTIMPL;
}

_Check_return_ HRESULT CDeferredElement::SetCustomWriterRuntimeData(
    _In_ std::shared_ptr<CustomWriterRuntimeData> data,
    _In_ std::unique_ptr<CustomWriterRuntimeContext> context)
{
    return E_NOTIMPL;
}

_Check_return_ HRESULT CDeferredElement::LoadContent(_Out_ std::shared_ptr<CDependencyObject>* pOut)
{
    *pOut = std::shared_ptr<CDependencyObject>();
    return E_NOTIMPL;
}

_Check_return_ HRESULT CDeferredElement::InsertRealizedElement(
    _In_ CDependencyObject* realizedElement)
{
    return E_NOTIMPL;
}

_Check_return_ HRESULT CDeferredElement::RemoveRealizedElement(
    _In_ CDependencyObject* realizedElement)
{
    return E_NOTIMPL;
}

_Check_return_ HRESULT CDeferredElement::TryDefer(
    _In_ CDependencyObject* element)
{
    return E_NOTIMPL;
}

CDeferredElementStorage::CDeferredElementStorage(
    _In_ CDeferredStorage* storage,
    _In_ CDependencyObject* parent,
    _In_ KnownPropertyIndex propertyIndex)
    : m_deferredStorage(storage),
    m_parent(parent),
    m_index(propertyIndex),
    m_suspended(false)
{}

CDeferredElementStorage::~CDeferredElementStorage()
{}
    
_Check_return_ HRESULT CDeferredElementStorage::ElementInserted(
    _In_ UINT32 indexInChildrenCollection)
{
    return E_NOTIMPL;
}

_Check_return_ HRESULT CDeferredElementStorage::ElementRemoved(
    _In_ UINT32 indexInChildrenCollection)
{
    return E_NOTIMPL;
}

_Check_return_ HRESULT CDeferredElementStorage::ElementMoved(
    _In_ UINT32 indexInChildrenCollection,
    _In_ UINT32 newIndexInChildrenCollection)
{
    return E_NOTIMPL;
}

_Check_return_ HRESULT CDeferredElementStorage::CollectionCleared()
{
    return E_NOTIMPL;
}

_Check_return_ HRESULT CDeferredElementStorage::ValueSet()
{
    return E_NOTIMPL;
}

_Check_return_ HRESULT CDeferredElementStorage::AppendDeferredElement(
    _In_ CDeferredElement* pElement,
    _In_ UINT32 childrenCount)
{
    return E_NOTIMPL;
}

UINT32 CDeferredElementStorage::CalculateRealizedElementInsertionIndex(
    _In_ CDeferredElement* pElement)
{
    return 0;
}

_Check_return_ HRESULT CDeferredElementStorage::RegisterUnrealizedProxies()
{
    return E_NOTIMPL;
}

_Check_return_ HRESULT CDeferredElementStorage::UnregisterUnrealizedProxies()
{
    return E_NOTIMPL;
}

_Check_return_ HRESULT CDeferredStorage::AddProxy(
    _In_ CDependencyObject* scope,
    _In_ CDependencyObject* parent,
    _In_opt_ CDependencyObject* parentCollection,
    _In_ KnownPropertyIndex index,
    _In_ CDeferredElement* proxy)
{
    return E_NOTIMPL;
}

_Check_return_ HRESULT CDeferredStorage::RemoveStorage(
    _In_ CDependencyObject* parent,
    _In_ KnownPropertyIndex propertyIndex)
{
    return E_NOTIMPL;
}

_Check_return_ HRESULT CDeferredStorage::NotifyParentValueSet(
    _In_ CDependencyObject* parent,
    _In_ KnownPropertyIndex index)
{
    return E_NOTIMPL;
}

_Check_return_ HRESULT CDeferredStorage::RegisterUnrealizedProxyNames(
    _In_ CDependencyObject* parent)
{
    return E_NOTIMPL;
}

_Check_return_ HRESULT CDeferredStorage::UnregisterUnrealizedProxyNames(
    _In_ CDependencyObject* parent)
{
    return E_NOTIMPL;
}

void CDeferredMapping::MapElementToDeclaringScope(
    _In_ CCoreServices* core,
    _In_ CDependencyObject* element,
    _In_ CDependencyObject* declaringScope)
{}

_Check_return_ HRESULT CDeferredMapping::NotifyParentValueSet(
    _In_ CDependencyObject* parent,
    _In_ KnownPropertyIndex index)
{
    return E_NOTIMPL;
}

_Check_return_ HRESULT CDeferredMapping::NotifyDestroyed(
    _In_ CDependencyObject* element)
{
    return E_NOTIMPL;
}

_Check_return_ HRESULT CDeferredMapping::NotifyEnter(
    _In_ CDependencyObject* newNamescope,
    _In_ CDependencyObject* element,
    _In_ bool skipNameRegistration)
{
    return E_NOTIMPL;
}

_Check_return_ HRESULT CDeferredMapping::NotifyLeave(
    _In_ CDependencyObject* oldNamescope,
    _In_ CDependencyObject* element,
    _In_ bool skipNameRegistration)
{
    return E_NOTIMPL;
}

CDeferredStorage& CDeferredMapping::EnsureScopeDeferredStorage(
    _In_ CDependencyObject* scope)
{
    static CDeferredStorage s_instance;
    return s_instance;
}

CDeferredStorage* CDeferredMapping::GetScopeDeferredStorage(
    _In_ CCoreServices* core,
    _In_ CDependencyObject* declaringScope)
{
    return nullptr;
}
