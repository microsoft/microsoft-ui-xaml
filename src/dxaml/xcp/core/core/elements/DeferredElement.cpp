// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#include "precomp.h"

#include <DeferredElement.h>
#include "DeferredMapping.h"
#include "DeferredElementCustomRuntimeData.h"
#include <BinaryFormatObjectWriter.h>
#include <CustomWriterRuntimeContext.h>
#include <CustomWriterRuntimeObjectCreator.h>
#include <StreamOffsetToken.h>
#include <MetadataAPI.h>
#include <CValue.h>
#include <DeferredElementStateChange.h>
#include <LayoutManager.h>
#include <ThemeResource.h>

// #define ENABLE_LOGGING

#ifdef ENABLE_LOGGING
  #define DBG_LOG(...) LOG(__VA_ARGS__)
#else
  #define DBG_LOG(...)
#endif

CDeferredElement::~CDeferredElement()
{
    // If realized element outlives proxy, let's clear its proxy pointer.
    auto realizedElement = m_wrRealizedElement.lock();

    if (realizedElement)
    {
        VERIFYHR(realizedElement->ClearRealizingProxy());
    }
}

CDependencyObject* CDeferredElement::GetParent() const
{
    if (m_owningStorage)
    {
        return m_owningStorage->GetParent();
    }
    else
    {
        return nullptr;
    }
}

CDependencyObject* CDeferredElement::GetDeclaringOwner()
{
    return m_spRuntimeContext->GetNameScope()->GetNamescopeOwner();
}

_Check_return_ HRESULT CDeferredElement::Realize(
    _Outptr_ CDependencyObject** ppResult)
{
    ASSERT(m_owningStorage);

    *ppResult = nullptr;

    // Unregister proxy name to prevent conflicts.
    IFC_RETURN(UnregisterProxyName(false));

    // If something went wrong, re-register the proxy name so it is not lost.
    auto registerGuard = wil::scope_exit([&]()
    {
        IGNOREHR(RegisterProxyName(false));
    });

    // Load element which was deferred.
    std::shared_ptr<CDependencyObject> result;
    IFC_RETURN(LoadContent(&result));

    m_wrRealizedElement = xref::get_weakref(result.get());

    IFC_RETURN(result->SetRealizingProxy(this));

    // Guard against trying to insert when containing collection is locked.  In
    // such case enqueue request and return realized element.
    bool canInsertRealizedElement = false;

    IFC_RETURN(CanInsertRealizedElement(canInsertRealizedElement));

    if (canInsertRealizedElement)
    {
        // This call will addref loaded element when it's inserted into namescope and tree.
        IFC_RETURN(FinalizeElementInsertion(result.get()));
    }
    else
    {
        CLayoutManager* layoutManager = VisualTree::GetLayoutManagerForElement(result.get());
        layoutManager->EnqueueElementInsertion(result.get());

        DBG_LOG(L"CDeferredElement [%p] '%s' enqueued [%p] %s in parent/index [%p/%u] '%s' @ position %u",
            this,
            m_strName.GetBuffer(),
            result.get(),
            result->GetClassName().GetBuffer(),
            m_owningStorage->GetParent(),
            m_owningStorage->GetPropertyIndex(),
            m_owningStorage->GetParent()->m_strName.GetBuffer(),
            m_index);
    }

    registerGuard.release();

    *ppResult = xref_ptr<CDependencyObject>(result.get()).detach();

    return S_OK;
}

_Check_return_ HRESULT CDeferredElement::FinalizeElementInsertion(
    _In_ CDependencyObject* realizedElement)
{
    ASSERT(m_wrRealizedElement.lock() == realizedElement);

    IFC_RETURN(InsertRealizedElement(realizedElement));

    auto templatedParent = GetTemplatedParent();

    if (templatedParent && templatedParent->OfTypeByIndex<KnownTypeIndex::Control>())
    {
        checked_cast<CControl>(templatedParent)->RequestTemplateBindingRefresh();
    }

    DBG_LOG(L"CDeferredElement [%p] '%s' realized [%p] %s in parent/index [%p/%u] '%s' @ position %u",
        this,
        m_strName.GetBuffer(),
        realizedElement,
        realizedElement->GetClassName().GetBuffer(),
        m_owningStorage->GetParent(),
        m_owningStorage->GetPropertyIndex(),
        m_owningStorage->GetParent()->m_strName.GetBuffer(),
        m_index);

    return S_OK;
}

_Check_return_ HRESULT CDeferredElement::Defer()
{
    ASSERT(IsRealized());

    auto realizedElement = m_wrRealizedElement.lock();

    if (realizedElement)
    {
        IFC_RETURN(RemoveRealizedElement(realizedElement.get()));
        IFC_RETURN(realizedElement->ClearRealizingProxy());
    }

    m_wrRealizedElement.reset();

    IFC_RETURN(RegisterProxyName(true));

    DBG_LOG(L"CDeferredElement [%p] '%s' re-deferred [%p] %s in parent/index [%p/%u] '%s' @ position %u",
        this,
        m_strName.GetBuffer(),
        realizedElement.get(),
        realizedElement->GetClassName().GetBuffer(),
        m_owningStorage->GetParent(),
        m_owningStorage->GetPropertyIndex(),
        m_owningStorage->GetParent()->m_strName.GetBuffer(),
        m_index);

    return S_OK;
}

_Check_return_ HRESULT CDeferredElement::PostAppendInit()
{
    if (m_realizeOnInit)
    {
        xref_ptr<CDependencyObject> spResult;
        IFC_RETURN(Realize(spResult.ReleaseAndGetAddressOf()));
        // Realized object is owned by the tree, so it's ok to drop ref here.
    }

    return S_OK;
}

_Check_return_ HRESULT CDeferredElement::LoadContent(_Out_ std::shared_ptr<CDependencyObject>* pResult)
{
    uint64_t instanceToken = 0;

    TraceRealizeDeferredElementBegin((uint64_t)this);

    auto traceEndGuard = wil::scope_exit([&]() {
        TraceRealizeDeferredElementEnd(reinterpret_cast<uint64_t>(this), instanceToken);
    });

    xref_ptr<CThemeResource> unused;
    IFC_RETURN(CustomWriterRuntimeObjectCreator(
        NameScopeRegistrationMode::RegisterEntries,
        m_spRuntimeContext.get()).CreateInstance(pResult, &unused));

    instanceToken = reinterpret_cast<uint64_t>(pResult->get());

    return S_OK;
}

static bool NeedsToModifyViaFramework(
    KnownPropertyIndex propertyIndex)
{
    return propertyIndex == KnownPropertyIndex::CommandBar_PrimaryCommands ||
           propertyIndex == KnownPropertyIndex::CommandBar_SecondaryCommands ||
           propertyIndex == KnownPropertyIndex::ItemsControl_Items;
}

static bool NeedsToModifyViaChildrenCollection(
    _In_ CDependencyObject* element,
    KnownPropertyIndex propertyIndex)
{
    return element->OfTypeByIndex<KnownTypeIndex::UIElement>() &&
           element->GetContentProperty()->GetIndex() == propertyIndex &&
           DirectUI::MetadataAPI::GetDependencyPropertyByIndex(propertyIndex)->GetPropertyType()->GetIndex() == KnownTypeIndex::UIElementCollection;
}

static bool NeedsToModifyViaCollection(
    KnownPropertyIndex propertyIndex)
{
    return DirectUI::MetadataAPI::GetDependencyPropertyByIndex(propertyIndex)->GetPropertyType()->IsCollection();
}

_Check_return_ HRESULT CDeferredElement::CanInsertRealizedElement(_Out_ bool& result)
{
    result = false;

    CDependencyObject* parent = m_owningStorage->GetParent();
    KnownPropertyIndex propertyIndex = m_owningStorage->GetPropertyIndex();

    if (NeedsToModifyViaChildrenCollection(parent, propertyIndex))
    {
        result = checked_cast<CUIElement>(parent)->CanModifyChildrenCollection();
    }
    else if (NeedsToModifyViaCollection(propertyIndex))
    {
        CValue itemsValue;

        IFC_RETURN(parent->GetValueByIndex(propertyIndex, &itemsValue));

        ASSERT(itemsValue.GetType() == valueObject);
        ASSERT(itemsValue.AsObject() != nullptr);

        result = !checked_cast<CCollection>(itemsValue.AsObject())->IsLocked();
    }
    else
    {
        result = true;
    }

    return S_OK;
}

_Check_return_ HRESULT CDeferredElement::InsertRealizedElement(
    _In_ CDependencyObject* realizedElement)
{
    ASSERT(m_owningStorage);

    CDependencyObject* parent = m_owningStorage->GetParent();
    KnownPropertyIndex propertyIndex = m_owningStorage->GetPropertyIndex();

    m_owningStorage->Suspend();

    auto resumeGuard = wil::scope_exit([&]
    {
        m_owningStorage->Resume();
    });

    if (NeedsToModifyViaFramework(propertyIndex))
    {
        // Special-case properties which need to be added via framework (e.g. into collection raising events)

        IFC_RETURN(FxCallbacks::DependencyObject_NotifyDeferredElementStateChanged(
            parent,
            propertyIndex,
            DeferredElementStateChange::Realized,
            m_owningStorage->CalculateRealizedElementInsertionIndex(this),
            realizedElement));
    }
    else if (NeedsToModifyViaChildrenCollection(parent, propertyIndex))
    {
        // Children collection - InsertChild does some more work in addition to collection->Insert() (e.g. raise loaded event)
        // therefore it needs to be called if it is applicable.

        IFC_RETURN(checked_cast<CUIElement>(parent)->InsertChild(
            m_owningStorage->CalculateRealizedElementInsertionIndex(this),
            checked_cast<CUIElement>(realizedElement)));
    }
    else if (NeedsToModifyViaCollection(propertyIndex))
    {
        // any other collection

        CValue itemsValue;

        IFC_RETURN(parent->GetValueByIndex(propertyIndex, &itemsValue));

        ASSERT(itemsValue.GetType() == valueObject);
        ASSERT(itemsValue.AsObject() != nullptr);

        CValue value;
        value.SetAddRef<valueObject>(realizedElement);

        IFC_RETURN(checked_cast<CCollection>(itemsValue.AsObject())->Insert(
            m_owningStorage->CalculateRealizedElementInsertionIndex(this),
            value));
    }
    else
    {
        // Single element property - set value

        CValue value;
        value.SetAddRef<valueObject>(realizedElement);
        IFC_RETURN(parent->SetValueByIndex(propertyIndex, value));
    }

    return S_OK;
}

_Check_return_ HRESULT CDeferredElement::RemoveRealizedElement(
    _In_ CDependencyObject* realizedElement)
{
    ASSERT(m_owningStorage);

    CDependencyObject* parent = m_owningStorage->GetParent();
    KnownPropertyIndex propertyIndex = m_owningStorage->GetPropertyIndex();

    m_owningStorage->Suspend();

    auto resumeGuard = wil::scope_exit([&]
    {
        m_owningStorage->Resume();
    });

    if (NeedsToModifyViaFramework(propertyIndex))
    {
        // Special-case properties which need to be removed via framework (e.g. into collection raising events)

        IFC_RETURN(FxCallbacks::DependencyObject_NotifyDeferredElementStateChanged(
            parent,
            propertyIndex,
            DeferredElementStateChange::Deferred,
            0,  // Don't care about collection index when removing.
            realizedElement));
    }
    else if (NeedsToModifyViaChildrenCollection(parent, propertyIndex))
    {
        // Remove from collection

        CUIElement* parentAsUIE = checked_cast<CUIElement>(parent);
        CUIElementCollection* children = parentAsUIE->GetChildren();

        if (children)
        {
            CUIElement* realizedElementAsUIE = checked_cast<CUIElement>(realizedElement);
            XINT32 index = -1;

            IFC_RETURN(children->IndexOf(
                realizedElementAsUIE,
                &index));

            if (index >= 0)
            {
                IFC_RETURN(parentAsUIE->RemoveChild(realizedElementAsUIE));
            }
        }
    }
    else if (NeedsToModifyViaCollection(propertyIndex))
    {
        // any other collection

        CValue itemsValue;

        IFC_RETURN(parent->GetValueByIndex(propertyIndex, &itemsValue));

        ASSERT(itemsValue.GetType() == valueObject);
        ASSERT(itemsValue.AsObject() != nullptr);

        CCollection* collection = checked_cast<CCollection>(itemsValue.AsObject());

        CValue value;
        value.Wrap<valueObject>(realizedElement);

        XINT32 index = -1;

        IFC_RETURN(collection->IndexOf(
            value,
            &index));

        if (index >= 0)
        {
            if (collection->ContainsNoRefItems())
            {
                (void)(collection->RemoveAt(index));
            }
            else
            {
                // RemoveAt returns the object with a ref count still on it. We have to release that reference.
                xref_ptr<CDependencyObject> removedCDO;
                removedCDO.attach(static_cast<CDependencyObject*>(collection->RemoveAt(index)));
            }
        }
    }
    else
    {
        // Single element property

        CValue value;
        value.SetNull();
        IFC_RETURN(parent->SetValueByIndex(propertyIndex, value));
    }

    return S_OK;
}

_Check_return_ HRESULT CDeferredElement::SetCustomWriterRuntimeData(
    std::shared_ptr<CustomWriterRuntimeData> data,
    std::unique_ptr<CustomWriterRuntimeContext> context)
{
    // First, move the context, since there could be calls relying on it being set
    m_spRuntimeContext = std::move(context);

    DeferredElementCustomRuntimeData* customRuntimeData = static_cast<DeferredElementCustomRuntimeData*>(data.get());

    IFC_RETURN(SetName(customRuntimeData->GetName()));

    // This call relies on m_strName set, so don't call before SetName.
    IFC_RETURN(RegisterProxyName(true));

    // Don't hold a reference to entire custom data just for a bool - copy it.
    m_realizeOnInit = customRuntimeData->Realize();

    // If there are properties being passed from the custom runtime data, we
    // will hold a reference to it so that we can get them later.
    if (!customRuntimeData->GetNonDeferredProperties().empty())
    {
        m_runtimeData = std::move(std::static_pointer_cast<DeferredElementCustomRuntimeData>(data));
    }

    return S_OK;
}

_Check_return_ HRESULT CDeferredElement::RegisterProxyName(bool force)
{
    if (force || !IsTemplateNamescopeMember())
    {
        DBG_LOG(L"CDeferredElement [%p] '%s' proxy registered in [%p]",
            this,
            m_strName.GetBuffer(),
            m_spRuntimeContext->GetNameScope()->GetNamescopeOwner());

        IFC_RETURN(GetContext()->SetNamedObject(
            m_strName,
            m_spRuntimeContext->GetNameScope()->GetNamescopeOwner(),
            m_spRuntimeContext->GetNameScope()->GetNameScopeType(),
            this));
    }

    return S_OK;
}

_Check_return_ HRESULT CDeferredElement::UnregisterProxyName(bool force)
{
    if (force || !IsTemplateNamescopeMember())
    {
        DBG_LOG(L"CDeferredElement [%p] '%s' proxy unregistered from [%p]",
            this,
            m_strName.GetBuffer(),
            m_spRuntimeContext->GetNameScope()->GetNamescopeOwner());

        IFC_RETURN(GetContext()->ClearNamedObject(
            m_strName,
            m_spRuntimeContext->GetNameScope()->GetNamescopeOwner(),
            this));
    }
    return S_OK;
}

_Check_return_ HRESULT CDeferredElement::TryDefer(
    _In_ CDependencyObject* element)
{
    CDeferredElement* realizingProxy = nullptr;

    IFC_RETURN(element->GetRealizingProxy(&realizingProxy));

    if (realizingProxy)
    {
        ASSERT(element == realizingProxy->m_wrRealizedElement.lock_noref());
        IFC_RETURN(realizingProxy->Defer());
    }
    else
    {
        return E_INVALIDARG;
    }

    return S_OK;
}

// CDeferredElementStorage

CDeferredElementStorage::CDeferredElementStorage(
    _In_ CDeferredStorage* deferredStorage,
    _In_ CDependencyObject* parent,
    _In_ KnownPropertyIndex index)
    : m_deferredStorage(deferredStorage)
    , m_parent(parent)
    , m_index(index)
    , m_suspended(false)
{
    DBG_LOG(L"CDeferredElementStorage::ctor [%p], CDeferredStorage [%p], parent/index [%p/%u] '%s'",
        this,
        deferredStorage,
        parent,
        index,
        parent->m_strName.GetBuffer());
}

CDeferredElementStorage::~CDeferredElementStorage()
{
    DBG_LOG(L"CDeferredElementStorage::dtor [%p]", this);
    IGNOREHR(UnregisterUnrealizedProxies());
}

_Check_return_ HRESULT CDeferredElementStorage::ElementInserted(
    _In_ UINT32 indexInChildrenCollection)
{
    if (!m_suspended)
    {
        for (auto current = m_proxyStorage.rbegin(); current != m_proxyStorage.rend() && indexInChildrenCollection <= (*current)->GetIndex(); ++current)
        {
            ++(*current)->GetIndexRef();
        }
    }

    return S_OK;
}

_Check_return_ HRESULT CDeferredElementStorage::ElementRemoved(
    _In_ UINT32 indexInChildrenCollection)
{
    if (!m_suspended)
    {
        for (auto current = m_proxyStorage.rbegin(); current != m_proxyStorage.rend() && indexInChildrenCollection < (*current)->GetIndex(); ++current)
        {
            ASSERT((*current)->GetIndex() > 0);
            --(*current)->GetIndexRef();
        }
    }

    return S_OK;
}

_Check_return_ HRESULT CDeferredElementStorage::ElementMoved(
    _In_ UINT32 oldIndexInChildrenCollection,
    _In_ UINT32 newIndexInChildrenCollection)
{
    if (!m_suspended)
    {
        if (m_proxyStorage.empty() ||
            oldIndexInChildrenCollection == newIndexInChildrenCollection)
        {
            // Noop if: there's nothing deferred (to speed up Modern Panel case)
            //          or element already is at the correct position
            return S_OK;
        }

        IFC_RETURN(ElementRemoved(
            oldIndexInChildrenCollection));

        IFC_RETURN(ElementInserted(
            (newIndexInChildrenCollection > oldIndexInChildrenCollection) ? newIndexInChildrenCollection - 1 : newIndexInChildrenCollection));
    }

    return S_OK;
}

_Check_return_ HRESULT CDeferredElementStorage::CollectionCleared()
{
    if (!m_suspended)
    {
        DBG_LOG(L"CDeferredElementStorage::CollectionCleared, parent/index [%p/%u]",
            m_parent,
            m_index);

        IFC_RETURN(m_deferredStorage->RemoveStorage(m_parent, m_index));
    }
    return S_OK;
}

_Check_return_ HRESULT CDeferredElementStorage::ValueSet()
{
    if (!m_suspended)
    {
        DBG_LOG(L"CDeferredElementStorage::ValueSet, parent/index [%p/%u]",
            m_parent,
            m_index);

        IFC_RETURN(m_deferredStorage->RemoveStorage(m_parent, m_index));
    }
    return S_OK;
}

_Check_return_ HRESULT CDeferredElementStorage::AppendDeferredElement(
    _In_ CDeferredElement* pElement,
    _In_ UINT32 childrenCount)
{
    // Since this is only called from parser, only children with x:Reralize='True' should be realized.
    ASSERT(std::count_if(
        m_proxyStorage.begin(),
        m_proxyStorage.end(),
        [](const xref_ptr<CDeferredElement>& element)
        {
            return !element->RealizeOnInit() && element->IsRealized();
        }) == 0);

    ASSERT(std::find(
        m_proxyStorage.begin(),
        m_proxyStorage.end(),
        pElement) == m_proxyStorage.end());

    if (!pElement->IsAssociatedWithStorage())
    {
        pElement->AssociateWithStorage(
            static_cast<UINT32>(m_proxyStorage.size() + childrenCount),  // Account for deferred children as well.
            this);

        m_proxyStorage.emplace_back(pElement);
    }
    else
    {
        // If proxy has already been added then fail.
        // This could be the case if xDefer is set on Flyout which is placed in ResourceDictionary and multiple elements are referencing it.
        // Currently, this is not a supported scenario.
        // Rules on where xDefer can be set are enforced by markup compiler -- this is just a sanity safeguard.
        return E_FAIL;
    }

    return S_OK;
}

UINT32 CDeferredElementStorage::CalculateRealizedElementInsertionIndex(
    _In_ CDeferredElement* pElement)
{
    UINT32 insertIndex = pElement->GetIndex();

    ASSERT(pElement->IsRealized());

    ASSERT(std::find(
        m_proxyStorage.begin(),
        m_proxyStorage.end(),
        pElement) != m_proxyStorage.end());

    // Adjust index by however many elements in front of it are not realized
    for (auto current = m_proxyStorage.begin(); current != m_proxyStorage.end() && *current != pElement; ++current)
    {
        if (!(*current)->IsRealized())
        {
            --insertIndex;
        }
    }

    return insertIndex;
}

_Check_return_ HRESULT CDeferredElementStorage::RegisterUnrealizedProxies()
{
    for (auto& proxy : m_proxyStorage)
    {
        if (!proxy->IsRealized())
        {
            IFC_RETURN(proxy->RegisterProxyName(false));
        }
    }

    return S_OK;
}

_Check_return_ HRESULT CDeferredElementStorage::UnregisterUnrealizedProxies()
{
    for (auto& proxy : m_proxyStorage)
    {
        if (!proxy->IsRealized())
        {
            IFC_RETURN(proxy->UnregisterProxyName(false));
        }
    }

    return S_OK;
}