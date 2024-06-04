// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#pragma once

#include "CDependencyObject.h"
#include <ICustomWriterRuntimeDataReceiver.h>
#include "ICollectionChangeCallback.h"
#include <CustomWriterRuntimeContext.h>

class DeferredElementCustomRuntimeData;
class CDeferredElementStorage;
class CDeferredStorage;

// Element proxy object
class CDeferredElement final
    : public CDependencyObject
    , public ICustomWriterRuntimeDataReceiver
{
    CDeferredElement(
        _In_ CCoreServices* core)
        : CDependencyObject(core)
    {}

public:
    ~CDeferredElement() override;

    DECLARE_CREATE(CDeferredElement);

    KnownTypeIndex GetTypeIndex() const override
    {
        return KnownTypeIndex::DeferredElement;
    }

    bool DoesAllowMultipleAssociation() const override
    {
        return true;
    }

    XUINT32 ParticipatesInManagedTreeInternal() override
    {
        return DOESNT_PARTICIPATE_IN_MANAGED_TREE;
    }

    void SetTemplatedParentImpl(_In_ CDependencyObject* parent) override
    {
        m_templatedParent = xref::get_weakref(parent);
    }

    CDependencyObject* GetTemplatedParent() override
    {
        return m_templatedParent.lock_noref();
    }

    DeferredElementCustomRuntimeData* GetCustomRuntimeData() const
    {
        return m_runtimeData.get();
    }

    CDependencyObject* GetParent() const override;

    CDependencyObject* GetDeclaringOwner();

    // Realizes element and inserts into parent's collection or sets parents value to realized element.
    _Check_return_ HRESULT Realize(
        _Outptr_ CDependencyObject** ppResult);

    _Check_return_ HRESULT Defer();

    _Check_return_ HRESULT PostAppendInit();

    bool IsRealized() const
    {
        return !!m_wrRealizedElement;
    }

    _Check_return_ HRESULT RegisterProxyName(bool force);
    _Check_return_ HRESULT UnregisterProxyName(bool force);

    _Check_return_ HRESULT FinalizeElementInsertion(
        _In_ CDependencyObject* realizedElement);

    bool IsAssociatedWithStorage() const
    {
        return m_owningStorage != nullptr;
    }

    void AssociateWithStorage(
        UINT32 index,
        _In_ CDeferredElementStorage* storage)
    {
        m_index = index;
        m_owningStorage = storage;
    }

    UINT32 GetIndex() const
    {
        return m_index;
    }

    UINT32& GetIndexRef()
    {
        return m_index;
    }

    bool RealizeOnInit() const
    {
        return m_realizeOnInit;
    }

    // ICustomWriterRuntimeDataReceiver
    _Check_return_ HRESULT SetCustomWriterRuntimeData(
        std::shared_ptr<CustomWriterRuntimeData> data,
        std::unique_ptr<CustomWriterRuntimeContext> context) override;

    static _Check_return_ HRESULT TryDefer(
        _In_ CDependencyObject* element);

private:
    _Check_return_ HRESULT LoadContent(_Out_ std::shared_ptr<CDependencyObject>* pResult);

    _Check_return_ HRESULT CanInsertRealizedElement(_Out_ bool& result);

    // Inserts realized element into property or collection on the parent.
    _Check_return_ HRESULT InsertRealizedElement(
        _In_ CDependencyObject* realizedElement);

    _Check_return_ HRESULT RemoveRealizedElement(
        _In_ CDependencyObject* realizedElement);

    std::shared_ptr<DeferredElementCustomRuntimeData>   m_runtimeData;
    std::unique_ptr<CustomWriterRuntimeContext>         m_spRuntimeContext;
    xref::weakref_ptr<CDependencyObject>                m_templatedParent;
    xref::weakref_ptr<CDependencyObject>                m_wrRealizedElement;
    CDeferredElementStorage*                            m_owningStorage         = nullptr;
    // Variable keeping track of where this element should live in children collection.
    UINT32                                              m_index                 = 0;
    bool                                                m_realizeOnInit         = false;
};

// Collection of proxies for parent instance and a property id.
class CDeferredElementStorage
    : public ICollectionChangeCallback
{
public:
    CDeferredElementStorage(
        _In_ CDeferredStorage* storage,
        _In_ CDependencyObject* parent,
        _In_ KnownPropertyIndex index);

    // Unregisters names of proxies and releases storage.
    ~CDeferredElementStorage() override;

    // ICollectionChangeCallback - receives notifications about collection updates, or values being set.

    _Check_return_ HRESULT ElementInserted(
        _In_ UINT32 indexInChildrenCollection) override;

    _Check_return_ HRESULT ElementRemoved(
        _In_ UINT32 indexInChildrenCollection) override;

    _Check_return_ HRESULT ElementMoved(
        _In_ UINT32 oldIndexInChildrenCollection,
        _In_ UINT32 newIndexInChildrenCollection) override;

    _Check_return_ HRESULT CollectionCleared() override;

    _Check_return_ HRESULT ValueSet();

    // Called from parser to add proxy.
    _Check_return_ HRESULT AppendDeferredElement(
        _In_ CDeferredElement* pElement,
        _In_ UINT32 childrenCount);

    CDependencyObject* GetParent() const
    {
        return m_parent;
    }

    KnownPropertyIndex GetPropertyIndex() const
    {
        return m_index;
    }

    // Helper for calculating insertion index for a proxy.  Takes into account other proxies in the collection.
    UINT32 CalculateRealizedElementInsertionIndex(
        _In_ CDeferredElement* pElement);

    // Sets flag which suspends updates to indices when realized element is being inserted.
    void Suspend()
    {
        ASSERT(!m_suspended);
        m_suspended = true;
    }

    // Clears flag which suspends updates to indices when realized element is being inserted.
    void Resume()
    {
        ASSERT(m_suspended);
        m_suspended = false;
    }

    _Check_return_ HRESULT RegisterUnrealizedProxies();

    _Check_return_ HRESULT UnregisterUnrealizedProxies();

private:
    std::vector<xref_ptr<CDeferredElement>> m_proxyStorage;
    CDeferredStorage*                       m_deferredStorage;
    CDependencyObject*                      m_parent;
    KnownPropertyIndex                      m_index;
    bool                                    m_suspended;
};
