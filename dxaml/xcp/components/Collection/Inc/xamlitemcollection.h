// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#pragma once

//  Abstract:  CItemCollection is the collection of ItemsControl.Items.
//             This keeps a reference back to the ItemsControl so that
//             it can notify it of changes.

class CItemsControl;
struct ICollectionChangeCallback;

class CItemCollection final : public CDOCollection
{
private:
    CItemCollection(_In_ CCoreServices *pCore)
        : CDOCollection(pCore)
    {}

// private destructor prevents inheritance from CItemCollection
    ~CItemCollection() override;

// Creation method
public:
    DECLARE_CREATE(CItemCollection);

// CDependencyObject overrides
public:
    KnownTypeIndex GetTypeIndex() const override
    {
        return DependencyObjectTraits<CItemCollection>::Index;
    }

    bool IsActive() const override;

// CCollection overrides...
public:
    _Check_return_ HRESULT Clear() override;
    _Check_return_ HRESULT SetOwner(_In_opt_ CDependencyObject *pOwner, _In_opt_ RENDERCHANGEDPFN pfnOwnerDirtyFlag = NULL) override;
    _Check_return_ HRESULT MoveInternal(_In_ XINT32 nIndex, _In_ XINT32 nPosition) override;

    void SetChangeCallback(
        _In_ const std::weak_ptr<ICollectionChangeCallback>& callback) override
    {
        m_wrChangeCallback = callback;
    }

// CDOCollectionOverrides ...
public:
    _Check_return_ HRESULT Append(_In_ CDependencyObject *pObject, _Out_opt_ XUINT32 *pnIndex = NULL) override;
    _Check_return_ HRESULT Insert(_In_ XUINT32 nIndex, _In_ CDependencyObject *pObject) override;
    _Check_return_ void * RemoveAt(_In_ XUINT32 nIndex) override;

protected:

    // For consistency sake we'll allow associating of multiple associated objects.
    _Check_return_ bool ShouldAssociateChildren(_In_ CDependencyObject* pChild) override { return pChild->DoesAllowMultipleAssociation(); }  // an item collection does not associate elements since it does not bring them into the visualtree
    void OnRemovingDependencyObject(_In_ CDependencyObject* pDO) override { }

protected:
// Items has ItemsControl as private parent
    bool ShouldParentBePublic() const final
    {
        return false;
    }

    // Use AddPeerReferenceToItem, whether or not acting as a parent to items.
    bool ShouldKeepPeerReference() override { return true; }


private:
    void ClearLogicalParentForItems();

    CItemsControl *m_pItemsControl = nullptr;
    std::weak_ptr<ICollectionChangeCallback> m_wrChangeCallback;
};
