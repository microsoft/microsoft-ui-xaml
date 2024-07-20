// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#pragma once

#include "DOCollection.h"
#include "DependencyObjectTraits.h"
#include "uielement.h"

enum UnloadCleanup;
class CTransitionRoot;
class UIElementCollectionUnloadingStorage;
struct ICollectionChangeCallback;

class CUIElementCollection final : public CDOCollection
{
public:
    CUIElementCollection(_In_ CCoreServices *pCore)
        : CDOCollection(pCore)
    {}

    ~CUIElementCollection() override;

    DECLARE_CREATE(CUIElementCollection);

    // CDependencyObject overrides
    KnownTypeIndex GetTypeIndex() const override
    {
        return DependencyObjectTraits<CUIElementCollection>::Index;
    }

    XUINT32 ParticipatesInManagedTreeInternal() override
    {
        // UIElementCollection's state is always stored in native, so it
        // doesn't need to particpate in the managed tree.
        return DOESNT_PARTICIPATE_IN_MANAGED_TREE;
    }

    // CCollection overrides...
    _Check_return_ HRESULT SetOwner(_In_opt_ CDependencyObject *pOwner, _In_opt_ RENDERCHANGEDPFN pfnOwnerDirtyFlag = NULL) override;
    _Check_return_ HRESULT OnAddToCollection(_In_ CDependencyObject *pDO) override;
    _Check_return_ HRESULT OnRemoveFromCollection(_Inout_opt_ CDependencyObject *pDO, _In_ XINT32 iPreviousIndex) override;

    _Check_return_ HRESULT Append(_In_ CDependencyObject *pObject, _Out_opt_ XUINT32 *pnIndex = NULL) override;
    _Check_return_ HRESULT Insert(_In_ XUINT32 nIndex, _In_ CDependencyObject *pObject) override;
    _Check_return_ CDependencyObject *Remove(_In_ CDependencyObject *pObject) override;
    _Check_return_ void *RemoveAt(_In_ XUINT32 nIndex) override;
    _Check_return_ HRESULT Clear() override;
    _Check_return_ HRESULT MoveInternal(_In_ XINT32 nIndex, _In_ XINT32 nPosition) override;

    // CDOCollection overrides...
    _Check_return_ CDependencyObject* RemoveAtImpl(_In_ XUINT32 nIndex) override;
    _Check_return_ HRESULT ValidateItem(_In_ CDependencyObject *pObject) override;

    void SetChangeCallback(
        _In_ const std::weak_ptr<ICollectionChangeCallback>& callback) override
    {
        m_wrChangeCallback = callback;
    }

    // CUIElementCollection methods...
    void GetChildrenInRenderOrderInternal(
        _Outptr_opt_result_buffer_(*puiChildCount) CUIElement ***pppUIElements,
        _Out_ XUINT32 *puiChildCount
        );

    _Check_return_ HRESULT Move(_In_ XINT32 nOldIndex, _In_ XINT32 nNewIndex);
    _Check_return_ HRESULT RemoveAllElements(_In_ bool bTryUnloadingElements);

    // TODO: MERGE: Clean up all the GetChildren methods and fix SAL annotations

    void SetSortedCollectionDirty(_In_ bool fDirty) { m_fSortedElementsDirty = fDirty; }

    // the bool indicates whether we executed extra removal code which indicates this was an element that
    // was unloading.
    _Check_return_ HRESULT RemoveUnloadedElement(_In_ CUIElement* pTarget, UINT unloadContext, _Out_ bool* pfExecutedUnload);

    UINT GetUnloadingContext(_In_ CUIElement* element);

    CTransitionRoot* GetLocalTransitionRoot(bool ensureTransitionRoot);

    void CleanupDeviceRelatedResourcesRecursive(_In_ bool cleanupDComp) override;

    XCP_FORCEINLINE bool HasUnloadingStorage() const
    {
        return m_pUnloadingStorage != nullptr;
    }

    bool IsUnloadingElement(_In_ CUIElement* pTarget);

    _Check_return_ HRESULT RemoveAllUnloadingChildren(
        const bool removeFromKeepAliveList,
        _In_opt_ DCompTreeHost* dcompTreeHost);

    CUIElement* operator[](_In_ size_t index) const { return static_cast<CUIElement*>(CDOCollection::operator[](index)); }

    // Special callback used by XamlDiagnosics for repositioning the in-app toolbar when it's added
    void SetToolbarAddedCallback(_In_ std::function<HRESULT(CUIElement*)> toolbarHandleCallback)
    {
        m_toolbarAddedCallback = toolbarHandleCallback;
    }

protected:
    _Check_return_ HRESULT LeaveImpl(_In_ CDependencyObject *pNamescopeOwner, _In_ LeaveParams params) override;
    void OnRemovingDependencyObject(_In_ CDependencyObject* pDO) override;

private:
    void OnChildrenChanged(_In_opt_ CDependencyObject *pChildSender);

    bool HasSortedChildren() const { return m_ppSortedUIElements != NULL; }

    bool NeedsToSort(_In_ const std::vector<CDependencyObject*>& unsortedUIElements) const;

    void DestroySortedCollection();
    UINT SortChildren(const std::vector<CUIElement*>& additionalElements, bool additionalElementsAboveChildren);
    _Check_return_ bool UnloadElement(_In_ XUINT32 nIndex, _In_ CUIElement *pRemove, _In_ UnloadCleanup initialClearLogic);
    _Check_return_ CDependencyObject* Remove(_In_ XUINT32 nIndex, _In_ CDependencyObject *pObject);
    _Check_return_ HRESULT OnRemoveFromCollection(_Inout_opt_ CDependencyObject *pDO);
    XUINT32 IndexIndicatingUnloadingElement() { return GetCount() + 1; };
    _Check_return_ static HRESULT EnsureUnloadingStorage(_In_ CUIElementCollection* pCollection);

public:
    // storage to keep a list of unloading elements etc.
    UIElementCollectionUnloadingStorage*    m_pUnloadingStorage     = nullptr;

private:
    CTransitionRoot*                        m_pLocalTransitionRoot  = nullptr;

    // Array of no-ref pointers to elements in the collection sorted by z-index.
    // Because this list doesn't hold references, it's not safe to access once the collection is modified.
    CUIElement**                            m_ppSortedUIElements    = nullptr;

    bool m_fSortedElementsDirty = false;

    std::weak_ptr<ICollectionChangeCallback> m_wrChangeCallback;
    std::function<HRESULT(CUIElement*)> m_toolbarAddedCallback;
};

// Wrapper around CUIElementCollection to make it easier for some users to inspect the collection,
// including handling in GetCount() the possibility of the collection being null.
//
// Note: The lifetime of this object is only as good as the underylying CUIElementCollection, which
// the caller must ensure remains alive (such as ensuring the element stays alive).
class CUIElementCollectionWrapper final
{
public:
    CUIElementCollectionWrapper(_In_opt_ CUIElementCollection* pCollection) :
        m_pCollectionNoRef(pCollection)
    {}

    UINT32 GetCount() const { return (m_pCollectionNoRef != nullptr) ? m_pCollectionNoRef->GetCount() : 0; }

    CUIElement* operator[](_In_ size_t index) const { return (*m_pCollectionNoRef)[index]; }

private:
    CUIElementCollection* m_pCollectionNoRef;
};

static_assert(sizeof(CUIElementCollectionWrapper) == sizeof(CUIElementCollection*),
    "CUIElementCollectionWrapper should be the same size as the pointer, so it is effectively the same as passing around the pointer.");

// Helper to cast from the array returned from CUIElementCollection->GetCollection().data()
// to a CUIElement**.  Please try to not add new uses of this caster.
CUIElement** doarray_to_elementarray_cast(CDependencyObject * const * doArray);
