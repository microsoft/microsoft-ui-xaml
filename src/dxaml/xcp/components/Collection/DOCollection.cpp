// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#include "precomp.h"
#include <CDependencyObject.h>
#include <collectionbase.h>
#include <DOCollection.h>
#include <DXamlServices.h>
#include <AutoReentrantReferenceLock.h>
#include "ICollectionChangeCallback.h"
#include <CValue.h>
#include <UIElement.h>
#include <double.h>
#include <Point.h>
#include <dopointercast.h>
#include <corep.h>
#include "theming\inc\Theme.h"
#include <FxCallbacks.h>

using namespace DirectUI;

CDOCollection::~CDOCollection()
{
    //we are not allowed to call directly abstract members
    VERIFYHR(Destroy());
}

//------------------------------------------------------------------------
//
//  Method:   Create
//
//  Synopsis:
//      Creates an instance of the DO collection class.
//
//------------------------------------------------------------------------

#pragma warning(push)
#pragma warning(disable: 6387)
_Check_return_
HRESULT
CDOCollection::Create(
    _Outptr_ CDependencyObject **ppObject,
    _In_ CREATEPARAMETERS *pCreate
)
{
    HRESULT hr = S_OK;

// There is only one type of parallel timeline to create so we can ignore the parameters.

   *ppObject = NULL;
    RRETURN(hr);
}
#pragma warning(pop)

//------------------------------------------------------------------------
//
//  Method:   CDOCollection::PropagateOwnerInfo
//
//  Synopsis:
//      Sets the parent of items in the collection to the owner of the Collection
//------------------------------------------------------------------------

_Check_return_ HRESULT
CDOCollection::PropagateOwnerInfo(_In_opt_ CDependencyObject* pOwner, _In_opt_ CDependencyObject *pOldOwner)
{
    for (auto& item : m_items)
    {
        IFCPTR_RETURN(item);

        // If the child supports parent(s) move it's parent pointer to pOwner.
        if(item->IsParentAware())
        {
            IFC_RETURN(ChangeParent(item, pOwner, pOldOwner, false /*bPublic*/));
        }

        // Otherwise, or if requested (ItemCollection), update peer references
        if(!item->IsParentAware() || ShouldKeepPeerReference() )
        {
            // Add to the new owner first so we guarantee that the managed
            // reference is still safe, then remove from the old owner
            if (pOwner)
            {
                IFC_RETURN(pOwner->AddPeerReferenceToItem(item));
            }

            if (pOldOwner)
            {
                IFC_RETURN(pOldOwner->RemovePeerReferenceToItem(item));
            }
        }
    }

    return S_OK;
}

//------------------------------------------------------------------------
//
//  Method: EnterImpl
//
//  Synopsis:
//      Causes the object and its "children" to enter scope. If bLive,
//      then the object can now respond to OM requests and perform actions
//      like downloads and animation.
//
//      Derived classes are expected to first call <base>::EnterImpl, and
//      then call Enter on any "children".
//
//------------------------------------------------------------------------
_Check_return_
HRESULT
CDOCollection::EnterImpl(_In_ CDependencyObject *pNamescopeOwner, EnterParams params)
{
    HRESULT hr;
    bool fUnlock = false;

    m_fIsLeaving = false;

    // Call the base class to enter all of this object's properties
    IFC(CDependencyObject::EnterImpl(pNamescopeOwner, params));

        // This kind of breaks the standard iterator pattern. How painful would it be
        // to lock an empty array and unlock it?
    if (GetCount() > 0)
    {
        // Prevent the collection from being changed while its items are
        // being enumerated.
        Lock();
        fUnlock = true;

        for (auto& item : m_items )
        {
            IFCPTR(item);
            IFC(ChildEnter(item, pNamescopeOwner, params, TRUE));
        }
    }

Cleanup:
    if (fUnlock)
    {
        Unlock();
    }

    RRETURN(hr);
}

//------------------------------------------------------------------------
//
//  Method: LeaveImpl
//
//  Synopsis:
//      Causes the object and its properties to leave scope. If bLive,
//      then the object is leaving the "Live" tree, and the object can no
//      longer respond to OM requests related to being Live.   Actions
//      like downloads and animation will be halted.
//
//      Derived classes are expected to first call <base>::LeaveImpl, and
//      then call Leave on any "children".
//
//------------------------------------------------------------------------

_Check_return_
HRESULT
CDOCollection::LeaveImpl(_In_ CDependencyObject *pNamescopeOwner, LeaveParams params)
{
    HRESULT hr;
    bool fUnlock = false;

    //mark as leaving
    m_fIsLeaving = true;

    // Call the base class to leave all of this object's properties
    IFC(CDependencyObject::LeaveImpl(pNamescopeOwner, params));

    if (!m_fIsProcessingNeat)
    {
        // Another lock, another GetCount.

        if (GetCount() > 0)
        {
            // Prevent the collection from being changed while its items are
            // being enumerated.
            Lock();
            fUnlock = TRUE;

            for (auto& item : m_items)
            {
                IFCPTR(item);

                IFC(ChildLeave(item, pNamescopeOwner, params, TRUE));
            }
        }

        // The only reason that we set up this temporary owner was to allow
        // resolution of names.  It that has cone away, then we can remove this
        // temporary arrangement.
        if (m_fOwnerIsFallback && !params.fSkipNameRegistration)
        {
            IFC(SetOwner(NULL, TRUE));
        }
    }

Cleanup:
    if (fUnlock)
    {
        Unlock();
    }
    RRETURN(hr);
}

//------------------------------------------------------------------------
//
//  Method: GetNamescopeParent
//
//  Synopsis:
//      Returns the parent for Namescope resolution purposes. Normally,
//      this would be m_pOwner.
//
//      Given that CCollection may have a reference (in EnterImpl) to an
//      object that may no longer exist, we need to be considerably more
//      circumspect.  One thing that we can be sure of:  if the Namescope
//      owner has been deleted, then it will have removed its Namescope
//      registration.
//
//      At first blush it may seem that this is an expensive thing to do
//      on *every* GetNamescopeParent, but if we find ourselves in this
//      situation (m_fOwnerIsFallback) then the owner that we return will
//      have skipped large swaths of parents.
//
//------------------------------------------------------------------------
CDependencyObject*
CDOCollection::GetStandardNameScopeParent()
{
    // See comment in EnterImpl; just use the standard namescope code
    if( ShouldEnsureNameResolution() )
    {
        return CCollection::GetStandardNameScopeParent();
    }


    CDependencyObject * const pOwner = GetOwner();
    if (m_fOwnerIsFallback && pOwner)
    {
        if (!GetContext()->HasRegisteredNames(pOwner))
        {
            return NULL;
        }
    }

    return pOwner;
}



//------------------------------------------------------------------------
//
//  Method:   CanProcessEnterLeave
//
//  Synopsis:
//      Should the collection process Enter/Leave given the totality of
//      circumstances, specifically:
//
//      If the derived collection would like to process Enter/Leave and
//      If at this point this is a Namescope Owner, or has the means to find one and
//      If we are in some sort of tree (Either the live tree or a Namescope)
//
//------------------------------------------------------------------------
bool
CDOCollection::CanProcessEnterLeave()
{
    return ((GetStandardNameScopeParent() != nullptr || IsStandardNameScopeOwner())
            && (IsActive() || IsStandardNameScopeMember()));
}

#if DBG
//------------------------------------------------------------------------
//
//  Method:   VerifyChildParent
//
//  Synopsis:
//      Verifies that parents are set for namescope resolution as expected.
//      DBG-only since this just enforces an invariant.
//
//------------------------------------------------------------------------
void CDOCollection::VerifyParentIsThisCollection(_In_ CDependencyObject *pChild)
{
    // If the owner is fallback for name resolution, then the collection must be the parent of each item to ensure a path
    // to the namescope owner.  There may not only be other objects or another owner acting as parent to the items.
    bool fCollectionIsParent = false;
    bool fChildIsParentAware = pChild->IsParentAware();
    if (fChildIsParentAware)
    {
        fCollectionIsParent = pChild->HasParent(this);
    }

    ASSERT(   !m_fOwnerIsFallback      // If the owner isn't for name resolution, there are no restrictions.
                  || !fChildIsParentAware     // If the child doesn't store parent references it's okay.
                  || !m_fShouldBeParentToItems // If the collection shouldn't set the parent on its items it's okay.
                  || fCollectionIsParent      // Otherwise, the collection MUST be a parent.
                  );
}
#endif

//------------------------------------------------------------------------
//
//  Method:   ChildEnter
//
//  Synopsis:
//      Process Enter on the Child if appropriate
//
//------------------------------------------------------------------------
_Check_return_ HRESULT
CDOCollection::ChildEnter(
    _In_ CDependencyObject *pChild,
    _In_ CDependencyObject *pNamescopeOwner,
    EnterParams params,
    bool fCanProcessEnterLeave
    )
{
    EnterParams enterParams(params);

    #if DBG
    // Skip the check for ResourceDictionary, because we allow someone to declare things like
    // a Storyboard inside a ResourceDictionary while its being referenced inside the visual tree.
    if (!OfTypeByIndex<KnownTypeIndex::ResourceDictionary>())
    {
        VerifyParentIsThisCollection(pChild);
    }
    #endif

    if (fCanProcessEnterLeave)
    {
        // First pass is name registration
        if (!params.fSkipNameRegistration)
        {
            enterParams.fIsLive = false;
            IFC_RETURN(pChild->Enter(pNamescopeOwner, enterParams));
        }

        if (params.fIsLive)
        {
            enterParams.fIsLive = true;
            enterParams.fSkipNameRegistration = true;
            // Second pass is the actual invocation - only for live trees
            IFC_RETURN(pChild->Enter(pNamescopeOwner, enterParams));
        }
        else if (params.fIsForKeyboardAccelerator)
        {
            IFC_RETURN(pChild->Enter(pNamescopeOwner, enterParams));
        }
    }

    return S_OK;
}


//------------------------------------------------------------------------
//
//  Method:   ChildLeave
//
//  Synopsis:
//      Process Leave on the Child if appropriate
//
//------------------------------------------------------------------------
//
_Check_return_ HRESULT
CDOCollection::ChildLeave(
    _In_ CDependencyObject *pChild,
    _In_ CDependencyObject *pNamescopeOwner,
    LeaveParams params,
    bool fCanProcessEnterLeave
    )
{
    if (fCanProcessEnterLeave)
    {
        IFC_RETURN(pChild->Leave(pNamescopeOwner, params));
    }

    #if DBG
    VerifyParentIsThisCollection(pChild);
    #endif

    return S_OK;
}

//----------------------------------------------------------------------------
//
//  Synopsis:
//      The virtual method which does the tree walk to clean up all
//      the device related resources like brushes, textures,
//      primitive composition data etc. in this subgraph.
//
//-----------------------------------------------------------------------------
void CDOCollection::CleanupDeviceRelatedResourcesRecursive(_In_ bool cleanupDComp)
{
    bool fUnlock = false;

    CCollection::CleanupDeviceRelatedResourcesRecursive(cleanupDComp);

    // YAWL (Yet Another Weird Lock)

    if (GetCount() > 0)
    {
        // Prevent the collection from being changed while its items are
        // being enumerated.
        Lock();
        fUnlock = TRUE;

        for (auto& item : m_items)
        {
            // We don't fail if there is somehow a null child but we don't want
            // to crash if one exists.

            if (item != nullptr)
            {
                item->CleanupDeviceRelatedResourcesRecursive(cleanupDComp);
            }
        }
    }

    if (fUnlock)
    {
        Unlock();
    }
}

_Check_return_ HRESULT CDOCollection::PreAddToCollection(_In_ CValue& newItem)
{
    IFC_RETURN(__super::PreAddToCollection(newItem));

    if (newItem.GetType() == valueObject)
    {
        CDependencyObject* newDO = newItem.AsObject();

        // Make sure that the incoming object is not currently associated.
        if (IsObjectAssociated(newDO))
        {
            CUIElement* pTarget = do_pointer_cast<CUIElement>(newDO);

            if (pTarget)
            {
                IFC_RETURN(pTarget->OnAssociationFailure());
            }

            if (IsObjectAssociated(newDO))
            {
                // We cannot set a new value that is already associated to another element
                IFC_RETURN(SetAndOriginateError(E_NER_INVALID_OPERATION, ManagedError, AG_E_MANAGED_ELEMENT_ASSOCIATED));
            }
        }

        IFC_RETURN(CycleCheck(newDO));
    }

    return S_OK;
}

//------------------------------------------------------------------------
//
//  Method:   Append (overrides CCollection)
//
//  Synopsis:
//      Our base, CCollection, could pass in doubles but we can only
//      accept CDependencyObjects.
//
//------------------------------------------------------------------------

_Check_return_
HRESULT
CDOCollection::Append(CValue& value, _Out_opt_ XUINT32 *pnIndex)
{
    return value.GetType() == valueObject ? Append(value.AsObject(), pnIndex) : E_INVALIDARG;
}

//------------------------------------------------------------------------
//
//  Method:   Append
//
//  Synopsis:
//      Append a reference to an object to the collection.
//
//------------------------------------------------------------------------

_Check_return_
HRESULT
CDOCollection::Append(_In_ CDependencyObject *pObject, _Out_opt_ XUINT32 *pnIndex)
{
    HRESULT hr = S_OK;
    bool fReleaseOnFail = false;
    bool fParented = false;
    CDependencyObject * const pOwner = GetOwner();

    EnterParams enterParams(
        /*isLive*/                IsActive(),
        /*skipNameRegistration*/  pOwner ? pOwner->SkipNameRegistrationForChildren() : false,
        /*coercedIsEnabled*/      pOwner ? pOwner->GetCoercedIsEnabled() : true, // Get this collection's owner's IsEnabled value
        /*useLayoutRounding*/     pOwner ? pOwner->GetUseLayoutRounding() : EnterParams::UseLayoutRoundingDefault,
        /*visualTree*/            pOwner ? VisualTree::GetForElementNoRef(pOwner, LookupOptions::NoFallback) : nullptr
    );

    enterParams.fCheckForResourceOverrides = ShouldCheckForResourceOverrides();

    IFC(ValidateItem(pObject));

    IFC(AppendImpl(pObject, pnIndex));
    fReleaseOnFail = true;

    IFC(SetChildParent(
        pObject, // pChild
        GetParentForItems(), // pParent
        NULL,  // pOldParent
        ShouldParentBePublic() // fPublic
        ));

    fParented = true;

    IFC(ChildEnter(pObject, GetStandardNameScopeOwner(), enterParams, CanProcessEnterLeave()));

Cleanup:
    if (FAILED(hr))
    {
        HRESULT recordHr = S_OK;

        if (fReleaseOnFail)
        {
            IGNORERESULT(Remove(pObject));
        }
        if (fParented)
        {
            RECORDFAILURE(SetChildParent(
                pObject, // pChild
                NULL, // pParent
                GetParentForItems(), // pOldParent
                false // fPublic
                ));
        }
        // Since Remove only removes but doesn't release the interface we need to do so.
        if (fReleaseOnFail)
        {
            ReleaseInterface(pObject);
        }
    }

    RRETURN(hr);
}


//------------------------------------------------------------------------
//
//  Method:   Insert (overrides CCollection)
//
//  Synopsis:
//      Our base, CCollection, could pass in doubles but we can only
//      accept CDependencyObjects.
//
//------------------------------------------------------------------------

_Check_return_
HRESULT
CDOCollection::Insert(_In_ XUINT32 nIndex, CValue& value)
{
    return value.GetType() == valueObject ? Insert(nIndex, value.AsObject()) : E_INVALIDARG;
}


//------------------------------------------------------------------------
//
//  Method:   Insert
//
//  Synopsis:
//      Insert a reference to an object into the collection.
//
//------------------------------------------------------------------------

_Check_return_
HRESULT
CDOCollection::Insert(_In_ XUINT32 nIndex, _In_ CDependencyObject *pObject)
{
    HRESULT hr = S_OK;
    bool fReleaseOnFail = false;
    bool fParented = false;
    CDependencyObject * const pOwner = GetOwner();

    // Redirect calls that would generate an append operation
    if (nIndex >= GetCount())
    {
        RRETURN(CDOCollection::Append(pObject));
    }

    EnterParams enterParams(
        /*isLive*/                IsActive(),
        /*skipNameRegistration*/  pOwner ? pOwner->SkipNameRegistrationForChildren() : false,
        /*coercedIsEnabled*/      pOwner ? pOwner->GetCoercedIsEnabled() : true, // Get this collection's owner's IsEnabled value
        /*useLayoutRounding*/     pOwner ? pOwner->GetUseLayoutRounding() : EnterParams::UseLayoutRoundingDefault,
        /*visualTree*/            pOwner ? VisualTree::GetForElementNoRef(pOwner, LookupOptions::NoFallback) : nullptr
    );

    enterParams.fCheckForResourceOverrides = ShouldCheckForResourceOverrides();

    IFC(ValidateItem(pObject));

    // Unlike appends which we optimize for parsing inserts are optimized for the
    // flattened case. So if the collection hasn't been flattened do so now. Note
    // the new flag which causes flattened to reserve some space.
    IFC(InsertImpl(nIndex, pObject));
    fReleaseOnFail = true;

    IFC(SetChildParent(
            pObject, // pChild
            GetParentForItems(), // pParent
            NULL, // pOldParent
            ShouldParentBePublic() // fPublic
            ));

    fParented = true;

    IFC(ChildEnter(pObject, GetStandardNameScopeOwner(), enterParams, CanProcessEnterLeave()));

Cleanup:
    if (FAILED(hr))
    {
        if (fParented)
        {
            VERIFYHR(SetChildParent(
                pObject, // pChild
                NULL, // pParent
                GetParentForItems(), // pOldParent
                false // fPublic
                ));
        }

        // Since Remove only removes but doesn't release the interface we need to do so.
        if (fReleaseOnFail)
        {
            IGNORERESULT(Remove(pObject));
            ReleaseInterface(pObject);
        }
    }

    RRETURN(hr);
}


//------------------------------------------------------------------------
//
//  Method:   IndexOf (overrides CCollection)
//
//  Synopsis:
//      Our base, CCollection, could pass in doubles but we can only
//      accept CDependencyObjects.
//
//------------------------------------------------------------------------
_Check_return_
HRESULT
CDOCollection::IndexOf(CValue& value, _Out_ XINT32 *pIndex)
{
    *pIndex = -1;

    if (value.GetType() == valueObject)
    {
        IFC_RETURN(IndexOf(value.AsObject(), pIndex));
    }
    else
    {
        // WPF compat: Null value allowed
        IFCEXPECT_RETURN(value.GetType() == valueNull);
    }

    return S_OK;
}


//------------------------------------------------------------------------
//
//  Method:   IndexOf
//
//  Synopsis:
//      Retrieve the index of an object in the collection.
//
//------------------------------------------------------------------------
_Check_return_
HRESULT
CDOCollection::IndexOf(_In_ CDependencyObject* pDO, _Out_ XINT32 *pIndex)
{
    RRETURN(IndexOfImpl(pDO,pIndex));
}



//------------------------------------------------------------------------
//
//  Method:   Remove
//
//  Synopsis:
//      Remove an object from the collection.
//
//------------------------------------------------------------------------

_Check_return_ CDependencyObject *
CDOCollection::Remove(_In_ CDependencyObject *pObject)
{
    XINT32 iObject;

    if(SUCCEEDED(IndexOf(pObject, &iObject)) && iObject != -1)
    {
        // We now have an index.
        return static_cast<CDependencyObject*>(RemoveAt(iObject));
    }

    return NULL;
}

_Check_return_ void *
CDOCollection::RemoveAt(_In_ XUINT32 nIndex)
{
    CDependencyObject *pRemove = nullptr;
    CDependencyObject * const pOwner = GetOwner();
    bool peggedPeer = false;
    bool peerIsPendingDelete = false;

    LeaveParams leaveParams(
        /*fIsLive*/               IsActive(),
        /*fSkipNameRegistration*/ pOwner ? pOwner->SkipNameRegistrationForChildren() : false,
        /*fCoercedIsEnabled*/     true,
        /*fVisualTreeBeingReset*/ false
    );

    // pRemove is no longer reachable for GC after it is removed from the collection by RemoveAtImpl,
    // so peg it until ChildLeave & SetChildParent has completed, to prevent GC from collecting
    // it during those operations.
    pRemove = GetItemImpl(nIndex);
    if (pRemove)
    {
        pRemove->TryPegPeer(&peggedPeer, &peerIsPendingDelete);
    }

    pRemove = RemoveAtImpl(nIndex);
    if (pRemove)
    {
        VERIFYHR(ChildLeave(pRemove, GetStandardNameScopeOwner(), leaveParams, CanProcessEnterLeave()));

        VERIFYHR(SetChildParent(
            pRemove, // pChild
            NULL, // pParent
            GetParentForItems(), // pOldParent
            ShouldParentBePublic() // fPublic
            ));

        VERIFYHR(OnRemoveFromCollection(pRemove, nIndex));
    }

    if (peggedPeer)
    {
        pRemove->UnpegManagedPeer();
    }

    return pRemove;
}

//------------------------------------------------------------------------
//
//  Method:   GetItem
//
//  Synopsis:
//      Retrieves an object from the collection.
//
//------------------------------------------------------------------------

_Check_return_ void *
CDOCollection::GetItemWithAddRef(_In_ XUINT32 nIndex)
{
    CDependencyObject *pItem;

    pItem = GetItemImpl(nIndex);
    AddRefInterface(pItem);

    return pItem;
}

//------------------------------------------------------------------------
//
//  Method:   Neat
//
//  Synopsis:
//      Release the simple collection
//
//------------------------------------------------------------------------

_Check_return_
HRESULT
CDOCollection::Neat(_In_ bool bBreak)
{
    m_fIsProcessingNeat = true;
    HRESULT hr = S_OK;
    bool fCanProcessEnterLeave = false;
    CDependencyObject *pDOTemp = NULL;

    CDependencyObject *pStoredNamescopeOwner = NULL;

    LeaveParams leaveParams(
        /*fIsLive*/               IsActive(),
        /*fSkipNameRegistration*/ false,
        /*fCoercedIsEnabled*/     true,
        /*fVisualTreeBeingReset*/ false
    );

    if (bBreak)
    {
        fCanProcessEnterLeave = CanProcessEnterLeave();
        pStoredNamescopeOwner = GetStandardNameScopeOwner();
    }

    for (unsigned int index = 0; index < m_items.size(); index++)
    {
        pDOTemp = m_items[index];
        IFCPTR(pDOTemp);

        if (bBreak)
        {
            VERIFYHR(ChildLeave(pDOTemp, pStoredNamescopeOwner, leaveParams, fCanProcessEnterLeave));

            VERIFYHR(SetChildParent(
                pDOTemp, // pChild
                NULL, // pParent
                GetParentForItems(), // pOldParent
                ShouldParentBePublic() // fPublic
                ));

            if (ShouldAssociateChildren(pDOTemp))
            {
                pDOTemp->SetAssociated(false, nullptr);
            }

            // CDOCollection items are walked for GC, so take GC lock,
            // set the item in the vector to null and proceed to release it.
            {
                AutoReentrantReferenceLock lock(DXamlServices::GetPeerTableHost());

                m_items[index] = nullptr;
            }

            //if break is false we convert Flatten to Broaden - do not need ReleaseInterface
            ReleaseInterface(pDOTemp);
        }
    }

    IFC(NeatImpl());

Cleanup:
    m_fIsProcessingNeat = false;
    RRETURN(hr);
}

//------------------------------------------------------------------------
//
//  Method:   ValidateItem
//
//  Synopsis:
//
// Before we can insert the object to the collection we have to be sure
// it is of the correct type.  This means we need to match the incoming
// type to the type of the content property for the collection.  At the
// moment this is magically at slot '1' in the property array
//------------------------------------------------------------------------
_Check_return_
HRESULT
CDOCollection::ValidateItem(_In_ CDependencyObject *pObject)
{
    const CClassInfo* pClass = NULL;

    if (!pObject)
        IFC_RETURN(E_INVALIDARG);

    pClass = GetClassInformation();
    IFCEXPECT_RETURN(pClass);

    // all collections should have a content property.
    ASSERT(pClass->GetContentProperty()->GetIndex() != KnownPropertyIndex::UnknownType_UnknownProperty);

    if (!pObject->OfTypeByIndex(MetadataAPI::GetDependencyPropertyByIndex(pClass->GetContentProperty()->GetIndex())->GetPropertyType()->GetIndex()))
    {
        IFC_RETURN(E_INVALIDARG);
    }

    return S_OK;
}

//------------------------------------------------------------------------
//
//  Method:   CycleCheck
//
//  Synopsis:
//      Check to see if adding this item will result in a cycle....
//
//------------------------------------------------------------------------
_Check_return_
HRESULT
CDOCollection::CycleCheck(_In_ CDependencyObject *pObject)
{
    CDependencyObject * const pOwner = GetOwner();

    IFCPTR_RETURN(pObject);

    // If this collection has an owner we also need to make sure
    // that we do not cause a cycle.
    if (pOwner)
    {
        // We only need to do this when the tree being added to (the collection) is not live,
        // since we cannot add a live tree to a live tree.
        if (!pOwner->IsActive())
        {
            // in case of ItemsControl items is going to move from one collection (CItemCollection)
            // to another (CUIElementCollection or StackPanel.Children) which both has the same root
            CDependencyObject * pAncestor = pOwner;
            while (pAncestor)
            {
                if (pAncestor == pObject)
                {
                    IFC_RETURN(E_INVALIDARG);
                }
                pAncestor = pAncestor->GetParentInternal(false);
            }
        }
    }
    else
    {
        // If this collection has no owner then it
        // cannot be an element of itself
        if (this == pObject)
        {
            IFC_RETURN(E_INVALIDARG);
        }
    }
    return S_OK;
}

//------------------------------------------------------------------------
//
//  Method:   CDOCollection::OnCollectionChangeHelper
//
//  Synopsis:
//      Starts the tree walk to propagate layout dirty flags on a collection change
//------------------------------------------------------------------------

void CDOCollection::OnCollectionChangeHelper()
{
    CDependencyObject * const pOwner = GetOwner();
    if(pOwner && (m_fAffectsOwnerMeasure || m_fAffectsOwnerArrange))
    {
        pOwner->PropagateLayoutDirty(m_fAffectsOwnerMeasure, m_fAffectsOwnerArrange);
    }
}

//------------------------------------------------------------------------
//
//  Method:   CDOCollection::OnAddToCollection
//
//  Synopsis:
//      Override to act on collection change
//------------------------------------------------------------------------

 _Check_return_ HRESULT CDOCollection::OnAddToCollection(_In_ CDependencyObject *pDO)
{
    OnCollectionChangeHelper();
    RRETURN(S_OK);
}

//------------------------------------------------------------------------
//
//  Method:   CDOCollection::OnRemoveFromCollection
//
//  Synopsis:
//      Override to act on collection change
//------------------------------------------------------------------------
 _Check_return_ HRESULT CDOCollection::OnRemoveFromCollection(_In_ CDependencyObject *pDO, _In_ XINT32 iPreviousIndex)
{
    OnCollectionChangeHelper();
    RRETURN(S_OK);
}

//------------------------------------------------------------------------
//
//  Method:   CDOCollection::OnClear
//
//  Synopsis:
//      Override to act on collection change
//------------------------------------------------------------------------

 _Check_return_ HRESULT CDOCollection::OnClear()
{
    OnCollectionChangeHelper();
    RRETURN(S_OK);
}


//------------------------------------------------------------------------
//
//  Method:   CDOCollection::ChangeParent
//
//  Synopsis:
//      Change the parent of a DO to a new value.
//
//------------------------------------------------------------------------

_Check_return_ HRESULT CDOCollection::ChangeParent(
    _In_ CDependencyObject *pChild,
    _In_opt_ CDependencyObject *pNewParent,
    _In_opt_ CDependencyObject *pOldParent,
    bool fPublic
    )
{
    HRESULT hr = S_OK;
    bool fProtectChildPeer = false;

    // If the child participates in the managed version of the DO tree, we need to
    // protected it from GC during this operation (unless the child is losing its
    // parent anyway).
    if (   pNewParent != NULL
        && pChild->HasManagedPeer()
        && pChild->ParticipatesInManagedTree())
    {
        // Add the child to the (poorly-named) list of managed objects that can't be GC-ed.
        pChild->PegManagedPeerNoRef();
        fProtectChildPeer = TRUE;
    }

    // Remove the old parent.
    if (pOldParent)
    {
        IFC(pChild->RemoveParent(pOldParent));
    }

    // Add the new parent and set its render state association.
    if (pNewParent)
    {
        RENDERCHANGEDPFN pfnRenderChangedHandler = NULL;
            // TODO: This is a bit nasty - all the owner code should move into this class since it's only useful here.
            // TODO: This isn't well-factored if there is no owner.  If children are parented to the collection they should propagate
            // TODO:      a flag determined by the collection, they shouldn't share the one from the collection's parent,  so the
            // TODO:      handler should be looked up in the property table or set by default.  For now, just hard-coding it.
            // If the parent used for the collection elements is the owner, then the change handler from collection to owner is stored in
            // m_NWOwnerAssociation.  If the parent used for elements is the collection itself, then the children need to propagate an association
            // to the collection that is determined by the collection itself.
            ASSERT(pNewParent == GetOwner() || pNewParent == this);
            pfnRenderChangedHandler = (pNewParent == GetOwner()) ? m_pfnOwnerRenderChangedHandler : RENDERCHANGEDPFN(CDependencyObject::NWSetRenderDirty);

        IFC(pChild->AddParent(pNewParent, fPublic, pfnRenderChangedHandler));
    }

Cleanup:
    if (fProtectChildPeer)
    {
        pChild->UnpegManagedPeerNoRef();
    }

    RRETURN(hr);
}



//-------------------------------------------------------------------------
//
//  SetChildParent
//
//  The the parent of a child, if appropriate.
//
//-------------------------------------------------------------------------

_Check_return_ HRESULT CDOCollection::SetChildParent(
    _In_ CDependencyObject *pChild,
    _In_opt_ CDependencyObject *pParent,
    _In_opt_ CDependencyObject *pOldParent,
    bool fPublic
    )
{
    bool fSkipTreeUpdates = false;

    // Some collections don't set themselves as a parent to their items
    // (CItemCollection), and for those there's nothing we need to do
    // and for those there's nothing we need to do.

    if (!m_fShouldBeParentToItems)
    {
        return S_OK;
    }

    // This logic should not run when the old parent is not actually a parent of this item.
    // The reason for this check is ItemsControl which uses both an ItemsCollection and an
    // Panel. Once an item moves to a panel, it is not removed from the ItemsCollection.
    // When we remove the element, it is first removed from the ItemsCollection and then
    // from the visual parent: the panel.
    // Unloading such an element using an unload transition would not work since after the
    // ItemsCollection has already set he parent to null, the element now cannot walk up
    // to its visual parent any more. This is a bug that we will probably never fix, so that
    // is the reason for this fix.
    //
    // For multi-shareable elements I do not want to pay the cost of walking the parent vector so
    // only apply this check if the child is not a multi-shareable element
    if (!pChild->DoesAllowMultipleParents() && !pChild->DoesAllowMultipleAssociation()
        && pOldParent != NULL && pOldParent != this && pOldParent != pChild->GetParentInternal(false))
    {
        fSkipTreeUpdates = true;
    }


    // Don't set the parent on shareables that do not support parents list
    // But do set peer references, or do both if requested (ItemCollection).
    if (!pChild->IsParentAware() || ShouldKeepPeerReference() )
    {
        if (pParent)
        {
            // We're adding the element to the collection this means that we need
            // to keep a reference to the object
            IFC_RETURN(this->AddPeerReferenceToItem(pChild));

            // InheritanceContext special case for EasingFunctions which do not
            // otherwise get a parent.  We do not want to do this with anything else
            // because of the perf impact
            if (!fSkipTreeUpdates && pChild->OfTypeByIndex<KnownTypeIndex::EasingFunctionBase>())
            {
                pChild->SetParentForInheritanceContextOnly(pParent);
            }
        }
        else
        {
            // We're removing the item from the collection, this means that we need
            // to remove the reference to the object
            IFC_RETURN(this->RemovePeerReferenceToItem(pChild));

            // Clear any potential InheritanceContext we might have set above
            if (!fSkipTreeUpdates && pChild->OfTypeByIndex<KnownTypeIndex::EasingFunctionBase>())
            {
                pChild->SetParentForInheritanceContextOnly(NULL);
            }
        }

    }

    // Set or clear the associated bit
    if (ShouldAssociateChildren(pChild))
    {
        if (pParent == NULL)
        {
            pChild->SetAssociated(false, nullptr);
        }
        else
        {
            pChild->SetAssociated(true, pParent);
        }
    }

    if( !pChild->IsParentAware() || fSkipTreeUpdates )
    {
        return S_OK;
    }

    // Set the parent
    IFC_RETURN(ChangeParent(pChild, pParent, pOldParent, fPublic));

    return S_OK;
}

//  The DO collection methods below are a specific implementation for the
//  DOCollection which uses an array based storage.
//
//  Please keep storage implementation specific details only in these
//  methods as there can exist different storage implementations like
//  TreeCollection used by RichTextBox.

_Check_return_
HRESULT
CDOCollection::AppendImpl(_In_ CDependencyObject *pObject, _Out_opt_ XUINT32 *pnIndex)
{
    pObject->AddRef();

    if (m_suspendVectorModifications)
    {
        // When we're suspending vector operations we don't want to try and keep track
        // of return values like this.
        ASSERT(!pnIndex);
    }
    else
    {
        // CDOCollection items are walked for GC, so take GC lock
        AutoReentrantReferenceLock lock(DXamlServices::GetPeerTableHost());

        m_items.push_back(pObject);
        if(pnIndex)
        {
            *pnIndex = static_cast<XUINT32>(m_items.size()-1);
        }
    }
    return S_OK;
}

// Core insert method that inserts the DependencyObject into the block storage
_Check_return_ HRESULT
CDOCollection::InsertImpl(_In_ XUINT32 nIndex, _In_ CDependencyObject *pObject)
{
    // CDOCollection items are walked for GC, so take GC lock
    AutoReentrantReferenceLock lock(DXamlServices::GetPeerTableHost());

    // Insertion isn't compatible with the types of bulk updates you'd want to
    // suspend vector modifications for.
    ASSERT(!m_suspendVectorModifications);

    m_items.insert(m_items.begin() + nIndex, pObject);
    pObject->AddRef();
    return S_OK;
}

_Check_return_ HRESULT
CDOCollection::IndexOfImpl(_In_ CDependencyObject* pDO, _Out_ XINT32 *pIndex)
{
    auto iter = std::find(m_items.begin(), m_items.end(), pDO);
    *pIndex = iter != m_items.end() ? static_cast<int>(std::distance(m_items.begin(), iter)) : -1;
    return S_OK;
}

_Check_return_ CDependencyObject*
CDOCollection::GetItemImpl(_In_ XUINT32 nIndex)
{
    if (nIndex >= m_items.size() || m_fIsProcessingNeat)
    {
        return nullptr;
    }
    return m_items[nIndex];
}

_Check_return_ CDependencyObject*
CDOCollection::RemoveAtImpl(_In_ XUINT32 nIndex)
{
    // CDOCollection items are walked for GC, so take GC lock
    AutoReentrantReferenceLock lock(DXamlServices::GetPeerTableHost());

    CDependencyObject *pRemove = m_items[nIndex];
    if (!m_suspendVectorModifications)
    {
        m_items.erase(m_items.begin() + nIndex);
    }
    return pRemove;
}

_Check_return_ HRESULT
CDOCollection::NeatImpl()
{
    // CDOCollection items are walked for GC, so take GC lock
    AutoReentrantReferenceLock lock(DXamlServices::GetPeerTableHost());

    ASSERT(!m_suspendVectorModifications);
    m_items.clear();
    return S_OK;
}

// nPosition is supposed to be one past the destination if moving towards the end,
// but supposed to be exactly the destination if moving towards the beginning!
_Check_return_ HRESULT
CDOCollection::MoveInternal(_In_ XINT32 nIndex, _In_ XINT32 nPosition)
{
    // CDOCollection items are walked for GC, so take GC lock
    AutoReentrantReferenceLock lock(DXamlServices::GetPeerTableHost());

    ASSERT(!m_suspendVectorModifications);

    XINT32 nCount = GetCount();

    IFCEXPECT_RETURN(nIndex >= 0 && nIndex < nCount && nPosition >= 0 && nPosition <= nCount);
    if ((nIndex == nPosition) || (nPosition - nIndex == 1))
    {
        // element already at the position
        return S_OK;
    }

    IFC_RETURN(FailIfLocked());

    if (nPosition < nIndex)
    {
        auto oldFirst = m_items.begin() + nPosition;
        auto newFirst = m_items.begin() + nIndex;
        auto oldLast = newFirst + 1;
        std::rotate(oldFirst, newFirst, oldLast);
    }
    else
    {
        ASSERT(nPosition > nIndex);
        auto oldFirst = m_items.begin() + nIndex;
        auto newFirst = oldFirst + 1;
        auto oldLast = m_items.begin() + nPosition;
        std::rotate(oldFirst, newFirst, oldLast);
    }

    return S_OK;
}

_Check_return_ HRESULT CDOCollection::NotifyThemeChangedCore(
    _In_ Theming::Theme theme,
    _In_ bool fForceRefresh)
{
    // Notify collection's properties that theme has changed
    IFC_RETURN(CDependencyObject::NotifyThemeChangedCore(theme, fForceRefresh));

    // Notify collection items that theme has changed.
    // There's no guarantee that the collection won't change as a side
    // effect of calling to an item, e.g. a new theme resolution causing
    // un-deferral of additional resources, so we use a temp stack vector
    // of the items to prevent using an invalid iterator.
    Jupiter::stack_vector<xref::weakref_ptr<CDependencyObject>, 24> tmp;
    tmp.m_vector.reserve(this->size()); // avoid reallocations
    for (const auto& item : *this)
    {
        tmp.m_vector.push_back(xref::get_weakref(item));
    }

    for (auto& item : tmp.m_vector)
    {
        xref_ptr<CDependencyObject> itemDO = item.lock();

        if (itemDO)
        {
            IFC_RETURN(itemDO->NotifyThemeChanged(theme, fForceRefresh));
        }
    }

    return S_OK;
}

//------------------------------------------------------------------------
//
// GC's ReferenceTrackerWalk. Walk collection items
// Called on GC thread.
//
//------------------------------------------------------------------------

bool CDOCollection::ReferenceTrackerWalkCore(
    _In_ DirectUI::EReferenceTrackerWalkType walkType,
    _In_ bool isRoot,
    _In_ bool shouldWalkPeer)
{
    bool walked = CDependencyObject::ReferenceTrackerWalkCore(walkType, isRoot, shouldWalkPeer);

    if (walked)
    {
        // Iterate using the actual CDOCollection::m_items instead of CDOCollection::GetCount/GetItem
        // because CDOCollection::GetCount will return 0 if it is processing Neat() and other
        // GetCount overrides may adjust the count.
        for (unsigned int i = 0; i < m_items.size(); i++)
        {
            CDependencyObject *pItemNoRef = m_items[i];

            if (pItemNoRef)
            {
                pItemNoRef->ReferenceTrackerWalk(
                   walkType,
                   false,  //isRoot
                   true);  //shouldWalkPeer
            }
        }
    }

    return walked;
}

xref_ptr<CDependencyObjectCollection> CDependencyObjectCollection::MakeDiagnosticsRootCollection(_In_ CCoreServices* coreServices)
{
    xref_ptr<CDependencyObjectCollection> collection;

    // The roots shouldn't have a parent. Since this CDependencyObjectCollection won't ever have a parent
    // and it's not supposed to be the parent to it's items. We get what we want.
    const bool shouldBeParentToItems = false;
    collection.attach(new CDependencyObjectCollection(coreServices, shouldBeParentToItems));
    return collection;
}

KnownTypeIndex CDependencyObjectCollection::GetTypeIndex() const
{
    return DependencyObjectTraits<CDependencyObjectCollection>::Index;
}

_Check_return_ HRESULT CDependencyObjectCollection::OnInheritanceContextChanged()
{
    // Notify the children that the inheritance context changed.
    for (auto& item : *this)
    {
        // When processing neat we will have set some of the items to
        // a nullptr. If one of those items holds a circular reference to
        // a parent in the tree it's possible for that parent to call back
        // into CDOCollection with a InheritanceContextChanged notification.
        // We bail out early in that case to avoid dereferencing a nullptr.
        if (!item) continue;

        IFC_RETURN(item->NotifyInheritanceContextChanged());
    }

    return S_OK;
}

_Check_return_ HRESULT
CDependencyObjectCollection::SetAt(
    _In_ XUINT32 nIndex,
    _In_ CValue& value)
{
    if (nIndex >= GetCount() || value.GetType() != valueObject)
    {
        IFC_RETURN(E_INVALIDARG);
    }

    // Make sure that the incoming object is not currently associated.
    CDependencyObject* valueAsObject = value.AsObject();

    if (IsObjectAssociated(valueAsObject))
    {
        CUIElement* pTarget = do_pointer_cast<CUIElement>(valueAsObject);

        if (pTarget)
        {
            IFC_RETURN(pTarget->OnAssociationFailure());
        }

        if (IsObjectAssociated(valueAsObject))
        {
            // We cannot set a new value that is already associated to another element
            IFC_RETURN(E_INVALIDARG);
        }
    }

    // Check for a cycle and insert the item on the specified index.
    IFC_RETURN(CycleCheck(valueAsObject));

    IFC_RETURN(InsertCore(nIndex, value, CollectionChangeBehavior::Default));
    IFC_RETURN(OnAddToCollection(valueAsObject));

    // Remove the previous item at that index.
    xref_ptr<CDependencyObject> removedDO;
    removedDO.attach(static_cast<CDependencyObject *>(CDOCollection::RemoveAt(nIndex + 1)));

    // Notify the collection changed for item changed.
    IFC_RETURN(FxCallbacks::DependencyObject_OnCollectionChanged(GetParentForItems(), static_cast<XUINT32>(DirectUI::CollectionChange::ItemChanged), nIndex));

    return S_OK;
}

_Check_return_ HRESULT CDependencyObjectCollection::Insert(
    _In_ UINT32 nIndex,
    _In_ CValue& value)
{
    IFC_RETURN(InsertCore(nIndex, value, CollectionChangeBehavior::RaiseCollectionChangedEvent));

    {
        auto callback = m_wrChangeCallback.lock();

        if (callback)
        {
            IFC_RETURN(callback->ElementInserted(nIndex));
        }
    }

    return S_OK;
}

_Check_return_ HRESULT
CDependencyObjectCollection::InsertCore(
    _In_ UINT32 nIndex,
    _In_ const CValue& value,
    _In_ CollectionChangeBehavior behavior)
{
    if (value.GetType() != valueObject)
    {
        IFC_RETURN(E_INVALIDARG);
    }

    // Redirect calls that would generate an append operation
    if (nIndex >= GetCount())
    {
        return Append(const_cast<CValue&>(value));
    }

    IFC_RETURN(CDOCollection::Insert(nIndex, value.AsObject()));

    // Notify the collection changed for item inserted.
    if (behavior == CollectionChangeBehavior::RaiseCollectionChangedEvent)
    {
        IFC_RETURN(FxCallbacks::DependencyObject_OnCollectionChanged(GetParentForItems(), static_cast<XUINT32>(DirectUI::CollectionChange::ItemInserted), nIndex));
    }

    return S_OK;
}

_Check_return_ void*
CDependencyObjectCollection::RemoveAt(
    _In_ XUINT32 nIndex)
{
    HRESULT hr = S_OK;  // WARNING_IGNORES_FAILURES
    void* pRemovedItem = CDOCollection::RemoveAt(nIndex);

    // Notify the collection changed for item removed.
    IFC(FxCallbacks::DependencyObject_OnCollectionChanged(GetParentForItems(), static_cast<XUINT32>(DirectUI::CollectionChange::ItemRemoved), nIndex));

    {
        auto callback = m_wrChangeCallback.lock();

        if (callback)
        {
            IFC(callback->ElementRemoved(nIndex));
        }
    }

Cleanup:
    return pRemovedItem;
}

 _Check_return_ HRESULT
CDependencyObjectCollection::OnClear()
{
    IFC_RETURN(CDOCollection::OnClear());

    // Notify the collection changed for item reset.
    IFC_RETURN(FxCallbacks::DependencyObject_OnCollectionChanged(GetParentForItems(), static_cast<XUINT32>(DirectUI::CollectionChange::Reset), 0));

    {
        auto callback = m_wrChangeCallback.lock();

        if (callback)
        {
            IFC_RETURN(callback->CollectionCleared());
        }
    }

    return S_OK;
}

_Check_return_ HRESULT
CDependencyObjectCollection::Append(
    _In_ CValue& value,
    _Out_opt_ XUINT32 *pnIndex)
{
    IFC_RETURN(CDOCollection::Append(value, pnIndex));

    // Notify the collection changed for item inserted.
    ASSERT(GetCount() > 0);
    IFC_RETURN(FxCallbacks::DependencyObject_OnCollectionChanged(GetParentForItems(), static_cast<XUINT32>(DirectUI::CollectionChange::ItemInserted), pnIndex ? *pnIndex : GetCount() - 1));

    return S_OK;
}

_Check_return_ HRESULT CDependencyObjectCollection::MoveInternal(
    _In_ XINT32 nIndex,
    _In_ XINT32 nPosition)
{
    IFC_RETURN(CDOCollection::MoveInternal(nIndex, nPosition));

    {
        auto callback = m_wrChangeCallback.lock();

        if (callback)
        {
            IFC_RETURN(callback->ElementMoved(
                static_cast<UINT32>(nIndex),
                static_cast<UINT32>(nPosition)));
        }
    }

    return S_OK;
}

_Check_return_ HRESULT CDependencyObjectCollection::ChildEnter(
    _In_ CDependencyObject* child,
    _In_ CDependencyObject* namescopeOwner,
    _In_ EnterParams params,
    _In_ bool canProcessEnterLeave)
{
    HRESULT hr = CDOCollection::ChildEnter(child, namescopeOwner, params, canProcessEnterLeave);
    if (SUCCEEDED(hr))
    {
        // For compat reasons with Windows 8.1, we force a top-level InheritanceContextChanged event on the child. This
        // only applies to DOC. Eventually we should revisit our InheritanceContextChanged semantics, and come up with a
        // more consistent story that works across all of our types.
        IFC_RETURN(child->NotifyInheritanceContextChanged(InheritanceContextChangeKind::ForceTopLevelEvent));
    }
    return hr;
}

_Check_return_ HRESULT CDependencyObjectCollection::ChildLeave(
    _In_ CDependencyObject* child,
    _In_ CDependencyObject* namescopeOwner,
    _In_ LeaveParams params,
    _In_ bool canProcessEnterLeave)
{
    if (child->IsActive())
    {
        // For compat reasons with Windows 8.1, we force a top-level InheritanceContextChanged event on the child. This
        // only applies to DOC. Eventually we should revisit our InheritanceContextChanged semantics, and come up with a
        // more consistent story that works across all of our types.
        IFC_RETURN(child->NotifyInheritanceContextChanged(InheritanceContextChangeKind::ForceTopLevelEvent));
    }
    return CDOCollection::ChildLeave(child, namescopeOwner, params, canProcessEnterLeave);
}
