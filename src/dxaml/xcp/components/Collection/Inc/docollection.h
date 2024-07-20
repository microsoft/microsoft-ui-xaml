// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#pragma once

#include <xref_ptr.h>
#include "collectionbase.h"

struct ICollectionChangeCallback;

class CDOCollection
    : public CCollection
{
public:
    typedef std::vector<CDependencyObject*> storage_type;

#if defined(__XAML_UNITTESTS__)
    CDOCollection()  // !!! FOR UNIT TESTING ONLY !!!
        : CDOCollection(nullptr)
    {}
#endif

    ~CDOCollection() override;

    static _Check_return_ HRESULT Create(
        _Outptr_ CDependencyObject **ppObject,
        _In_ CREATEPARAMETERS *pCreate);

    // Do NOT make any of these methods virtual ever.
#pragma region Modern DOCollection interface
    storage_type::const_iterator begin() const;
    storage_type::const_iterator end() const;
    size_t size() const;
    bool empty() const;
    CDependencyObject* const& operator[](_In_ size_t index) const;

    // This method will ASSERT if a failure condition is detected. It
    // is NOT compatible with derived classes of CDOCollection that can fail
    // in their ValidateItem implementation. Long-term CDOCollection will become
    // a small collection-based class that provides Enter/Leave and parenting of
    // DOs in a comprehensible manor. Classes that need to add additional
    // behavior will need to derive from CDOCollection. Since most of these checks
    // are for validation of actions that are the result of public API calls
    // we likely can create a wrapper that simply performs that validation to be
    // used when operating with values that are not guaranteed to be safe.
    //
    // Today this takes a const reference to a xref_ptr, in the future once the internal
    // storage of this class moves to a std::vector<xref_ptr<CDOCollection>> we can
    // make this a value and perform a std::move to avoid the extra AddRef/Releases.
    void push_back(_In_ const xref_ptr<CDependencyObject>& dependencyObject);

    void insert(_In_ storage_type::const_iterator position, const xref_ptr<CDependencyObject>& val);

    // Ideally this would return a reference, but because the collection isn't stored
    // in terms of xref_ptrs just yet we can't do that. Soon.
    xref_ptr<CDependencyObject> back() const;
    void pop_back();

    template<class UnaryPredicate>
    void remove_if(_In_ UnaryPredicate pred)
    {
        m_suspendVectorModifications = true;
        auto newItemEnd = std::partition(m_items.begin(), m_items.end(), pred);
        for (auto eraseIter = m_items.begin(); eraseIter != newItemEnd; eraseIter++)
        {
            static_cast<CDependencyObject*>(RemoveAt(static_cast<UINT32>(std::distance(m_items.begin(), eraseIter))))->Release();
        }
        m_items.erase(m_items.begin(), newItemEnd);
        m_suspendVectorModifications = false;
    }

    void remove(_In_ CDependencyObject* obj);
    void clear();
#pragma endregion

    UINT32 GetCount() const override
    {
        if (m_fIsProcessingNeat)
        {
            return 0;
        }
        return static_cast<UINT32>(m_items.size());
    }
    bool IsDOCollection() const final { return true; }

    const storage_type& GetCollection() { return m_items; }
    _Check_return_ HRESULT Reserve(_In_ XUINT32 size) override { m_items.reserve(size); return S_OK; }

    _Ret_notnull_ CDependencyObject* GetParentForItems()
    {
        // Child items are parented to the owner if it exists and it isn't fallback for name resolution.
        // Otherwise, they are parented to the collection itself.
        CDependencyObject *pOwner = GetOwner();
        return (pOwner && !m_fOwnerIsFallback) ? pOwner : this;
    }

    bool IsLeaving() { return m_fIsLeaving; }

    _Check_return_ HRESULT PropagateOwnerInfo(_In_opt_ CDependencyObject *pOwner, _In_opt_ CDependencyObject *pOldOwner) final;

    //////
    // Virtual function that allows the owner of the collection to do
    //  processing on the addition of items in its collection
    //////
    _Check_return_ HRESULT PreAddToCollection(_In_ CValue& newItem) final;
    virtual _Check_return_ HRESULT OnAddToCollection(_In_ CDependencyObject *pDO);
    _Check_return_ HRESULT OnAddToCollection(_In_ const CValue& value) final
    {
        ASSERT(value.GetType() == valueObject);
        return OnAddToCollection(value.AsObject());
    }


    //////
    // Virtual function that allows the owner of the collection to do
    //  processing on the removal of items in its collection
    //////
    virtual _Check_return_ HRESULT OnRemoveFromCollection(_In_ CDependencyObject *pDO, _In_ XINT32 iPreviousIndex);
    _Check_return_ HRESULT OnRemoveFromCollection(_In_ const CValue& value, _In_ XINT32 iPreviousIndex) final
    {
        ASSERT(value.GetType() == valueObject);
        return OnRemoveFromCollection(value.AsObject(), iPreviousIndex);
    }

    //////
    // Virtual function that allows the owner of the collection to do
    //  processing on the removal of all items in its collection
    //////
    _Check_return_ HRESULT OnClear() override;
    _Check_return_ HRESULT Append(CValue& value, _Out_opt_ XUINT32 *pnIndex = NULL) override;
    virtual _Check_return_ HRESULT Append(_In_ CDependencyObject *pObject, _Out_opt_ XUINT32 *pnIndex = NULL);
    virtual _Check_return_ HRESULT AppendImpl(_In_ CDependencyObject *pObject, _Out_opt_ XUINT32 *pnIndex = NULL);
    _Check_return_ HRESULT Insert(_In_ XUINT32 nIndex, CValue& value) override;
    virtual _Check_return_ HRESULT Insert(_In_ XUINT32 nIndex, _In_ CDependencyObject *pObject);
    virtual _Check_return_ HRESULT InsertImpl(_In_ XUINT32 nIndex, _In_ CDependencyObject *pObject);
    virtual _Check_return_ void *Remove(_In_ void *pObject) { return Remove(static_cast <CDependencyObject *>(pObject)); };
    virtual _Check_return_ CDependencyObject *Remove(_In_ CDependencyObject *pObject);
    // Note: Does not release the object. That needs to be done manually.
    _Check_return_ void *RemoveAt(_In_ XUINT32 nIndex) override;
    virtual _Check_return_ CDependencyObject *RemoveAtImpl(_In_ XUINT32 nIndex);
    _Check_return_ void *GetItemWithAddRef(_In_ XUINT32 nIndex) override;
    _Check_return_ CDependencyObject *GetItemImpl(_In_ XUINT32 nIndex);
    // GetItemAtFastImpl is intended to provide a mechanism for loops to quickly get to an item, without additional
    // checks/initialization that may have been already performed at the start of a loop.
    _Check_return_ CDependencyObject *GetItemAtFastImpl(_In_ XUINT32 index, __in_xcount(check performed outside at the start of the loop) CDependencyObject **ppDO) { return ppDO[index]; }
    _Check_return_ CDependencyObject *GetItemDOWithAddRef(_In_ XUINT32 nIndex) { return static_cast<CDependencyObject*>(GetItemWithAddRef(nIndex)); }
    _Check_return_ HRESULT IndexOf(CValue& value, _Out_ XINT32 *pIndex) final;
    virtual _Check_return_ HRESULT IndexOf(_In_ CDependencyObject *pObject, _Out_ XINT32 *pIndex) final;
    _Check_return_ HRESULT IndexOfImpl(_In_ CDependencyObject *pObject, _Out_ XINT32 *pIndex);
    _Check_return_ HRESULT MoveInternal(_In_ XINT32 nIndex, _In_ XINT32 nPosition) override;
    bool IsObjectAssociated(_In_ CDependencyObject *pObject) const
    {
        return pObject->DoesAllowMultipleAssociation() ? false : pObject->IsAssociated();
    }
    virtual _Check_return_ HRESULT CycleCheck(_In_ CDependencyObject *pObject);
    virtual _Check_return_ bool ShouldAssociateChildren(_In_ CDependencyObject*) { return true; }

    // CDependencyObject methods
    void CleanupDeviceRelatedResourcesRecursive(_In_ bool cleanupDComp) override;

    bool CollectionShouldOwnItems() const { return m_fShouldBeParentToItems;  }
protected:
    CDOCollection(_In_ CCoreServices* pCore, bool shouldBeParentToItems = true)
        : CCollection(pCore)
        , m_fIsLeaving(false)
        , m_fIsProcessingNeat(false)
        , m_fShouldBeParentToItems(shouldBeParentToItems)
        , m_suspendVectorModifications(false)
    {}

    // Adding and removing elements to this collection is a pretty expensive
    // thing to do. Often derived classes will want to swap elements around for
    // sorting operations. We'll provide this interface until we can better
    // reconcile this design.
    //
    // The primary purpose behind swap is to allow derived types to order the collection.
    // There's probably a better algorithm here using something like std::sort.
    void swap(_In_ size_t n1, _In_ size_t n2);

    void OnCollectionChangeHelper();

    _Check_return_ HRESULT ChangeParent(
        _In_ CDependencyObject *pChild,
        _In_opt_ CDependencyObject *pNewParent,
        _In_opt_ CDependencyObject *pOldParent,
        bool fPublic
        );

    _Check_return_ HRESULT NotifyThemeChangedCore(_In_ Theming::Theme theme, _In_ bool fForceRefresh = false) override;

    _Check_return_ HRESULT SetChildParent(
        _In_ CDependencyObject *pChild,
        _In_opt_ CDependencyObject *pParent,
        _In_opt_ CDependencyObject *pOldParent,
        bool fPublic
        );

    // It's not clear why a null parent shouldn't be public.
    virtual bool ShouldParentBePublic() const
    {
        return GetOwner() != nullptr;
    }

    // This flag used to, during the Enter walk, create this fallback parent concept. The items aren't
    // parented to the owner like usual, they are instead parented to the collection itself. The
    // Owner is instead used as the NamescopeParent value. Much of the layout dirty code is broken
    // in this case and wouldn't work. Don't use this flag unless you have a really good reason to.
    virtual bool ShouldEnsureNameResolution() { return false; }

    // The core lifetime logic mostly relies on the visual tree and DPs, but that gets
    // complicated with ItemsControl's host panel logic.
    //
    // During startup, if you have elements in ItemsControl.Items, the elements get
    // parented to the ItemsCollection's owner (i.e. the ItemsControl), and so the
    // ItemsControl is protecting the element peers.
    //
    // On first layout, the elements get re-parented to the items host, and
    // consequently it's the items host (panel) that's protecting the lifetime
    // of the element's peer.
    //
    // On a ItemsControl.Clear(), however, the elements are removed from
    // the items host and the items host is thrown away.  At that point
    // the CDOCollection is keeping a ref on the element's core peer, but
    // nothing is protecting the element's framework (managed) peer. It
    // stays that way until the next layout, at which point the
    // items host is re-created. So, since the ItemsCollection is maintaining
    // the lifetime of the core peer, it should be similarly maintaining the
    // lifetime of the framework peer, thus the call to
    // AddPeerReferenceToItem, which is triggered by this flag.
    virtual bool ShouldKeepPeerReference() { return false; }

    _Check_return_ HRESULT EnterImpl(_In_ CDependencyObject *pNamescopeOwner, _In_ EnterParams params) override;
    _Check_return_ HRESULT LeaveImpl(_In_ CDependencyObject *pNamescopeOwner, _In_ LeaveParams params) override;
    CDependencyObject* GetStandardNameScopeParent() override;
    bool CanProcessEnterLeave();

    virtual _Check_return_ HRESULT ChildEnter(
        _In_ CDependencyObject *pChild,
        _In_ CDependencyObject *pNamescopeOwner,
        EnterParams params,
        bool fCanProcessEnterLeave
        );

    virtual _Check_return_ HRESULT ChildLeave(
        _In_ CDependencyObject *pChild,
        _In_ CDependencyObject *pNamescopeOwner,
        LeaveParams params,
        bool fCanProcessEnterLeave
        );

    _Check_return_ HRESULT Neat(_In_ bool bBreak = false) override;
    _Check_return_ HRESULT NeatImpl();
    virtual _Check_return_ HRESULT ValidateItem(_In_ CDependencyObject *pObject);

    bool ReferenceTrackerWalkCore(
        _In_ DirectUI::EReferenceTrackerWalkType walkType,
        _In_ bool isRoot,
        _In_ bool shouldWalkPeer) override;

private:
    storage_type m_items;

    bool m_fIsLeaving : 1;
    bool m_fIsProcessingNeat : 1;
    bool m_fShouldBeParentToItems : 1;
    // CDOCollection is a pretty hairy design. There's not an easy way today to
    // separate out all the fragile auxiliary logic from the logic that actually
    // touches the underlying vector. To allow for efficient operations we create
    // this flag that lets us suspend operations on the vector for the duration of
    // a method.
    bool m_suspendVectorModifications : 1;

#if DBG
    void VerifyParentIsThisCollection(_In_ CDependencyObject *pChild);
#endif
};

template<typename T>
constexpr T* do_pointer_cast(CDependencyObject*);

// Since CDOCollection isn't public, do_pointer_cast doesn't work, so we specialize it below
template<>
constexpr CDOCollection* do_pointer_cast<CDOCollection>(_In_opt_ CDependencyObject* pDO)
{
    // These are in alphabetical order, and may not be the complete list of collections
    // that derive from CDOCollection. If you find one missing, add it to the right spot.
    if (pDO &&
       (pDO->OfTypeByIndex<KnownTypeIndex::BrushCollection>() ||
        pDO->OfTypeByIndex<KnownTypeIndex::ColumnDefinitionCollection>() ||
        pDO->OfTypeByIndex<KnownTypeIndex::DependencyObjectCollection>() ||
        pDO->OfTypeByIndex<KnownTypeIndex::GeometryCollection>() ||
        pDO->OfTypeByIndex<KnownTypeIndex::GradientStopCollection>() ||
        pDO->OfTypeByIndex<KnownTypeIndex::ItemCollection>() ||
        pDO->OfTypeByIndex<KnownTypeIndex::PathSegmentCollection>() ||
        pDO->OfTypeByIndex<KnownTypeIndex::PointCollection>() ||
        pDO->OfTypeByIndex<KnownTypeIndex::ResourceDictionaryCollection>() ||
        pDO->OfTypeByIndex<KnownTypeIndex::SetterBaseCollection>() ||
        pDO->OfTypeByIndex<KnownTypeIndex::StateTriggerCollection>() ||
        pDO->OfTypeByIndex<KnownTypeIndex::StoryboardCollection>() ||
        pDO->OfTypeByIndex<KnownTypeIndex::TextElementCollection>() ||
        pDO->OfTypeByIndex<KnownTypeIndex::TransformCollection>() ||
        pDO->OfTypeByIndex<KnownTypeIndex::TransitionCollection>() ||
        pDO->OfTypeByIndex<KnownTypeIndex::UIElementCollection>() ||
        pDO->OfTypeByIndex<KnownTypeIndex::VisualTransitionCollection>() ||
        pDO->OfTypeByIndex<KnownTypeIndex::VisualStateCollection>() ||
        pDO->OfTypeByIndex<KnownTypeIndex::VisualStateGroupCollection>() ||
        pDO->OfTypeByIndex<KnownTypeIndex::XamlLightCollection>()))
    {
        return static_cast<CDOCollection*>(pDO);
    }

    return nullptr;
}

// This class is the native peer of the managed internal class DOCollection
class CDependencyObjectCollection
    : public CDOCollection
{
protected:
    CDependencyObjectCollection(_In_ CCoreServices* pCore, bool shouldBeParentToItems = true)
        : CDOCollection(pCore, shouldBeParentToItems)
    {
        // We always want to propagate the inheritance context changed event to our children.
        SetWantsInheritanceContextChanged(true);
    }

public:
#if defined(__XAML_UNITTESTS__)
        CDependencyObjectCollection()  // !!! FOR UNIT TESTING ONLY !!!
            : CDOCollection()
        {}
#endif

    DECLARE_CREATE(CDependencyObjectCollection);

    static xref_ptr<CDependencyObjectCollection> MakeDiagnosticsRootCollection(_In_ CCoreServices* coreServices);

// CDependencyObject overrides
public:
    KnownTypeIndex GetTypeIndex() const override;

    virtual _Check_return_ HRESULT SetAt(_In_ XUINT32 nIndex, _In_ CValue& value);
    _Check_return_ HRESULT Append(_In_ CValue& value, _Out_opt_ XUINT32 *pnIndex = nullptr) override;

    _Check_return_ HRESULT OnInheritanceContextChanged() override;

    void SetChangeCallback(
        _In_ const std::weak_ptr<ICollectionChangeCallback>& callback) override
    {
        m_wrChangeCallback = callback;
    }

protected:
    _Check_return_ bool ShouldAssociateChildren(_In_ CDependencyObject*) override { return false; }

    _Check_return_ HRESULT Insert(_In_ XUINT32 nIndex, CValue& value) override;
    _Check_return_ void *RemoveAt(_In_ XUINT32 nIndex) override;

    _Check_return_ HRESULT OnClear() override;

    _Check_return_ HRESULT MoveInternal(
        _In_ XINT32 nIndex,
        _In_ XINT32 nPosition) override;

    _Check_return_ HRESULT ChildEnter(
        _In_ CDependencyObject* child,
        _In_ CDependencyObject* namescopeOwner,
        _In_ EnterParams params,
        _In_ bool canProcessEnterLeave) override;

    _Check_return_ HRESULT ChildLeave(
        _In_ CDependencyObject* child,
        _In_ CDependencyObject* namescopeOwner,
        _In_ LeaveParams params,
        _In_ bool canProcessEnterLeave) override;

private:
    enum class CollectionChangeBehavior : bool
    {
        Default = false,
        RaiseCollectionChangedEvent = true
    };

    _Check_return_ HRESULT InsertCore(_In_ UINT32 nIndex, _In_ const CValue& value, _In_ CollectionChangeBehavior behavior);

    std::weak_ptr<ICollectionChangeCallback> m_wrChangeCallback;
};
