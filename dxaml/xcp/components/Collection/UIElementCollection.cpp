// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#include "precomp.h"
#include <CDependencyObject.h>
#include <DependencyObjectTraits.h>
#include <DependencyObjectTraits.g.h>
#include <CPropertyPath.h>
#include <collectionbase.h>
#include <DOCollection.h>
#include <DXamlServices.h>
#include <AutoReentrantReferenceLock.h>
#include <CValue.h>
#include <UIElement.h>
#include <double.h>
#include <Point.h>
#include <dopointercast.h>
#include <corep.h>
#include "ICollectionChangeCallback.h"
#include <MultiParentShareableDependencyObject.h>
#include <Panel.h>
#include <Layouttransition.h>
#include <TransitionCollection.h>
#include <TrimWhitespace.h>
#include <StringConversions.h>
#include <UIElementCollection.h>
#include <UIElement.h>
#include <Framework.h>
#include <CControl.h>
#include <ContentControl.h>
#include <UIElementStructs.h>
#include <EventArgs.h>
#include <Canvas.h>
#include <TransitionRoot.h>
#include <EventMgr.h>
#include <Popup.h>
#include <LayoutTransitionElement.h>
#include <LayoutManager.h>
#include <AutomationPeer.h>
#include <ConnectedAnimationService.h>
#include <ImplicitAnimations.h>
#include <DCompTreeHost.h>
#include <FxCallbacks.h>

#pragma warning(disable:4267) //'var' : conversion from 'size_t' to 'type', possible loss of data

bool
CUIElementCollection::IsUnloadingElement(_In_ CUIElement* pTarget)
{
    return HasUnloadingStorage() && m_pUnloadingStorage->ContainsKey(pTarget);
}

/*static*/
_Check_return_ HRESULT CUIElementCollection::EnsureUnloadingStorage(_In_ CUIElementCollection* pCollection)
{
    HRESULT hr = S_OK;

    if (!pCollection->m_pUnloadingStorage)
    {
        UIElementCollectionUnloadingStorage* pStorage = new UIElementCollectionUnloadingStorage();

        pCollection->m_pUnloadingStorage = pStorage;
    }

    RRETURN(hr);//RRETURN_REMOVAL
}

_Check_return_ HRESULT
 CUIElementCollection::OnAddToCollection(_In_ CDependencyObject *pDO)
{
    CUIElement * const pUIElement = static_cast<CUIElement*>(pDO);
    CCoreServices * const pContext = GetContext();

    IFCPTR_RETURN(pUIElement);
    IFCPTR_RETURN(pContext);

    // Release our hold on the core; since we are being added into the tree we no longer need to
    // keep our own addref on the core
    pUIElement->ContextRelease();

    if (pUIElement->IsActive())
    {
        // Raise any loaded events that were just added to the 'live' tree.
        CEventManager *pEventManager;

        if (pContext->CheckWatch(WATCH_LOADED_EVENTS))
        {
            pEventManager = pContext->GetEventManager();

            if (pEventManager)
            {
                IFC_RETURN(pEventManager->RequestRaiseLoadedEventOnNextTick());
            }
        }
    }

    // Create storage for transitions pro-actively.
    if (CTransition::HasPossibleTransition(pUIElement))
    {
        IFC_RETURN(CUIElement::EnsureLayoutTransitionStorage(pUIElement, NULL, TRUE));
    }

    return S_OK;
}

//------------------------------------------------------------------------
//
//  Method:   OnRemoveFromCollection
//
//  Synopsis: Calls full OnRemoveFromCollection. Is here to codify the
//  need to call without an index.
//
//------------------------------------------------------------------------
_Check_return_ HRESULT
CUIElementCollection::OnRemoveFromCollection(_Inout_opt_ CDependencyObject *pDO)
{
    RRETURN(OnRemoveFromCollection(pDO, -1));
}

//------------------------------------------------------------------------
//
//  Synopsis: Marks this action as deferred if element is unloading.
//
//------------------------------------------------------------------------
void CUIElementCollection::OnRemovingDependencyObject(_In_ CDependencyObject* pDO)
{
    CUIElement *pUIElement = static_cast<CUIElement*>(pDO);
    if (HasUnloadingStorage() && m_pUnloadingStorage->ContainsKey(pUIElement))
    {
        UnloadCleanup* pRemoveLogicToExecute = m_pUnloadingStorage->Get(pUIElement);
        *pRemoveLogicToExecute = (UnloadCleanup) (*pRemoveLogicToExecute | UC_OnRemoveDOFromCollection);
    }
    else
    {
        CDOCollection::OnRemovingDependencyObject(pDO);
    }
}

_Check_return_ HRESULT
CUIElementCollection::OnRemoveFromCollection(
    _Inout_opt_ CDependencyObject *pDO,
    _In_ XINT32 iPreviousIndex)
{
    // NOTE: Do not rely on the previous index parameter since its value is not meaningful during
    // unloading scenarios.

    CUIElement * const pUIElement = static_cast<CUIElement*>(pDO);

    IFCPTR_RETURN(pUIElement);

    // If we are actively unloading, we do not want to execute the code below, but we will schedule
    // that to happen later.
    if (HasUnloadingStorage() && m_pUnloadingStorage->ContainsKey(pUIElement))
    {
        UnloadCleanup* pRemoveLogicToExecute = m_pUnloadingStorage->Get(pUIElement);
        *pRemoveLogicToExecute = (UnloadCleanup) (*pRemoveLogicToExecute | UC_OnRemoveFromCollection);

        return S_OK;
    }

    pUIElement->Shutdown();

    // Since we are being pulled out of the tree grab a reference to the core, so that the core
    // will not go away on us. This will be released when we are added to the tree or we are
    // deleted.
    pUIElement->ContextAddRef();

    return S_OK;
}

//------------------------------------------------------------------------
//
//  Synopsis:
//      dtor
//
//------------------------------------------------------------------------
CUIElementCollection::~CUIElementCollection()
{
    ReleaseInterface(m_pLocalTransitionRoot);
    IGNOREHR(RemoveAllElements(false)); // do not attempt to unload elements that we are removing when destructing.
    delete m_pUnloadingStorage;
}

//------------------------------------------------------------------------
//
//  Method:   CUIElementCollection::SetOwner
//
//  Synopsis:
//
//------------------------------------------------------------------------
_Check_return_ HRESULT
CUIElementCollection::SetOwner(_In_opt_ CDependencyObject *pOwner, _In_opt_ RENDERCHANGEDPFN pfnOwnerDirtyFlag)
{
    if (pOwner &&
        (!pOwner->OfTypeByIndex<KnownTypeIndex::UIElement>() || pfnOwnerDirtyFlag != RENDERCHANGEDPFN(CUIElement::NWSetSubgraphDirty)))
    {
        RRETURN(E_INVALIDARG);
    }

    // Update the parent on the transition root.
    if (m_pLocalTransitionRoot)
    {
        IFC_RETURN(m_pLocalTransitionRoot->RemoveParent(GetOwner()));
        IFC_RETURN(m_pLocalTransitionRoot->AddParent(pOwner));
    }

    IFC_RETURN(CCollection::SetOwner(pOwner, pfnOwnerDirtyFlag));

    return S_OK;
}

//------------------------------------------------------------------------
//
//  Synopsis:
//      Clears the sorted element collection.
//
//------------------------------------------------------------------------
void
CUIElementCollection::DestroySortedCollection()
{
    SAFE_DELETE_ARRAY(m_ppSortedUIElements);
}

//------------------------------------------------------------------------
//
//  Synopsis:
//      Returns the children in the order that they should be rendered. If
//      a child has a ZIndex or a projection applied then the list will be
//      sorted in z order. Otherwise the list will be in the order of the
//      child collection.
//      Special case:  Implicit Hide animations/Connected animations put
//      elements into unloading storage which are also incorporated, see
//      implementation notes below.
//
//------------------------------------------------------------------------
void
CUIElementCollection::GetChildrenInRenderOrderInternal(
    _Outptr_opt_result_buffer_(*puiChildCount) CUIElement ***pppUIElements,
    _Out_ XUINT32 *puiChildCount
    )
{
    *pppUIElements = NULL;
    *puiChildCount = 0;

    UINT uiChildCount = GetCount();

    // A special case is made for unloading storage:
    // Implicit Hide animations and Connected Animations put elements into unloading storage and also require
    // the RenderWalk to render these elements.  To accommodate this case, we include these elements in the collection
    // with the following Z-ordering behaviors:
    // 1) If this is Panel's collection, we put these elements at the end of the collection (highest in z order).
    // 2) If this is not a Panel, we put these elements at the beginning of the collection (lowest in z order).
    //    This is done so that for single child scenarios (eg Page navigation), the outgoing page is lower in z order.
    // 3) We sort the overall collection so that Canvas.ZIndex can be used to override the default ordering.
    std::vector<CUIElement*> additionalElements;
    if (HasUnloadingStorage())
    {
        for (UIElementCollectionUnloadingStorage::UnloadingMap::const_iterator it = m_pUnloadingStorage->m_unloadingElements.begin(); it != m_pUnloadingStorage->m_unloadingElements.end(); ++it)
        {
            if ((*(it->second) & UC_REFERENCE_ImplicitAnimation) != 0 ||
                (*(it->second) & UC_REFERENCE_ConnectedAnimation) != 0)
            {
                additionalElements.push_back(it->first);
            }
        }
    }

    if (additionalElements.size() > 0)
    {
        bool isParentPanel = GetOwner()->OfTypeByIndex<KnownTypeIndex::Panel>();
        *puiChildCount = SortChildren(additionalElements, isParentPanel);
        *pppUIElements = m_ppSortedUIElements;
    }
    else
    {
        if (uiChildCount == 1)
        {
            //
            // There's only one child so order doesn't matter. Return the unsorted collection.
            // The collection may be dirty for sorting when it contains only one element.
            // Keep it dirty so that when a second element is added we check whether sorting
            // is necessary.
            //
            *pppUIElements = doarray_to_elementarray_cast(GetCollection().data());
            *puiChildCount = 1;
        }
        else if (uiChildCount > 1)
        {
            CUIElement **ppUnsortedUIElements = doarray_to_elementarray_cast(GetCollection().data());

            if (!m_fSortedElementsDirty)
            {
                //
                // If we're not dirty for sorting then return the existing sorted list, or,
                // if we don't have one, the elements in natural order.
                //
                *pppUIElements = m_ppSortedUIElements ? m_ppSortedUIElements : ppUnsortedUIElements;
            }
            else
            {
                //
                // We're dirty for sorting but we don't know whether we need a sorted
                // collection. By default we don't need a sorted collection and we render in
                // the order of the child elements. But if one of the child elements has a
                // nonzero ZIndex or a projection, then we need to sort the children and
                // render in sorted order.
                //
                if (NeedsToSort(GetCollection()))
                {
                    SortChildren(additionalElements, true);
                    *pppUIElements = m_ppSortedUIElements;
                }
                else
                {
                    DestroySortedCollection();
                    SetSortedCollectionDirty(false);
                    *pppUIElements = ppUnsortedUIElements;
                }
            }
            *puiChildCount = uiChildCount;
        }
    }
}

//------------------------------------------------------------------------
//
//  Synopsis:
//      Determines whether a list of CUIElements needs to be sorted
//      before being rendered.
//
//------------------------------------------------------------------------
bool
CUIElementCollection::NeedsToSort(
    _In_ const std::vector<CDependencyObject*>& unsortedUIElements) const
{
    for (XUINT32 i = 0; i < unsortedUIElements.size(); i++)
    {
        CUIElement *pChild = reinterpret_cast<CUIElement*>(unsortedUIElements[i]);
        if (pChild->GetZIndex() != 0)
        {
            return true;
        }
    }

    return false;
}

//------------------------------------------------------------------------
//
//  Synopsis:
//      Builds the sorted collection, sorting by draw order.
//
//------------------------------------------------------------------------
UINT CUIElementCollection::SortChildren(
    const std::vector<CUIElement*>& additionalElements, // Elements to include in addition to the live elements in the collection
    bool additionalElementsAboveChildren)               // true => position additionalElement above live children, else below
{
    CUIElement **sortedElements = NULL;

    UINT liveChildCount = GetCount();
    UINT additionalChildCount = static_cast<UINT>(additionalElements.size());
    UINT totalChildCount = liveChildCount + additionalChildCount;

    ASSERT(liveChildCount > 1 || additionalElements.size() > 0);

    DestroySortedCollection();

    sortedElements = new CUIElement *[totalChildCount];

    // Use stable insertion sort.
    for (UINT i = 0, j; i < totalChildCount; i++)
    {
        //
        // Everything from 0 to i-1 has been sorted. Get the element at i and swap it
        // left into its sorted position.
        //
        CUIElement *next;
        if (additionalElementsAboveChildren)
        {
            // Position additionalElements above the live children
            if (i < liveChildCount)
            {
                next = static_cast<CUIElement*>((*this)[i]);
            }
            else
            {
                next = additionalElements[i - liveChildCount];
            }
        }
        else
        {
            // Position additionalElements below the live children
            if (i < additionalChildCount)
            {
                next = additionalElements[i];
            }
            else
            {
                next = static_cast<CUIElement*>((*this)[i - additionalChildCount]);
            }
        }
        for (j = i; j > 0 && next->IsDrawOrderSmallerThan(sortedElements[j - 1]); j--)
        {
            sortedElements[j] = sortedElements[j - 1];
        }
        sortedElements[j] = next;
    }

    m_ppSortedUIElements = sortedElements;
    m_fSortedElementsDirty = false;

    return totalChildCount;
}

//------------------------------------------------------------------------
//
//  Synopsis:
//      Adds an element at the end of this collection.
//
//------------------------------------------------------------------------
_Check_return_ HRESULT
CUIElementCollection::Append(_In_ CDependencyObject *pObject, _Out_opt_ XUINT32 *pnIndex)
{
    CUIElement* pUIElement = static_cast<CUIElement*>(pObject);
    CUIElement* const pOwner = static_cast<CUIElement* const>(GetOwner());

    IFCPTR_RETURN(pUIElement);
    IFCPTR_RETURN(pOwner);

    if (pOwner == pUIElement)
    {
        IFC_RETURN(E_UNEXPECTED);
    }

    GetContext()->BeginWatch();

    IFC_RETURN(CDOCollection::Append(pObject, pnIndex));

    if (m_toolbarAddedCallback != nullptr)
    {
        IFC_RETURN(m_toolbarAddedCallback(pUIElement));
    }

    // create storage for transition pro-actively
    if (CTransition::HasPossibleTransition(pUIElement))
    {
        IFC_RETURN(CUIElement::EnsureLayoutTransitionStorage(pUIElement, NULL, TRUE));
    }

    OnChildrenChanged(pObject);
    pOwner->InvalidateMeasure();

    return S_OK;
}

//------------------------------------------------------------------------
//
//  Synopsis:
//      Adds an element at a specific location in this collection.
//
//------------------------------------------------------------------------
_Check_return_ HRESULT CUIElementCollection::Insert(_In_ XUINT32 nIndex, _In_ CDependencyObject *pObject)
{
    CUIElement *pUIElement = static_cast<CUIElement*>(pObject);
    CUIElement * const pOwner = static_cast<CUIElement * const>(GetOwner());

    IFCPTR_RETURN(pUIElement);
    IFCPTR_RETURN(pOwner);
    if (pOwner == pUIElement)
    {
        IFC_RETURN(E_UNEXPECTED);
    }

    GetContext()->BeginWatch();

    IFC_RETURN(CDOCollection::Insert(nIndex, pObject));

    // create storage for transition pro-actively
    if (CTransition::HasPossibleTransition(pUIElement))
    {
        IFC_RETURN(CUIElement::EnsureLayoutTransitionStorage(pUIElement, NULL, TRUE));
    }

    OnChildrenChanged(pObject);
    pOwner->InvalidateMeasure();

    {
        auto callback = m_wrChangeCallback.lock();

        if (callback)
        {
            IFC_RETURN(callback->ElementInserted(nIndex));
        }

        if (m_toolbarAddedCallback != nullptr)
        {
            IFC_RETURN(m_toolbarAddedCallback(pUIElement));
        }
    }

    return S_OK;
}

//------------------------------------------------------------------------
//
//  Synopsis:
//      Called on Append and Insert before the new child is actually added to the collection.
//
//------------------------------------------------------------------------
_Check_return_ HRESULT
CUIElementCollection::ValidateItem(_In_ CDependencyObject *pObject)
{
    ASSERT(GetOwner() && pObject);
    IFC_RETURN(CDOCollection::ValidateItem(pObject));
    return S_OK;
}

//------------------------------------------------------------------------
//
//  Synopsis:
//      Moves the item at nIndex to index nPosition.
//
//------------------------------------------------------------------------
_Check_return_ HRESULT
CUIElementCollection::MoveInternal(_In_ XINT32 nIndex, _In_ XINT32 nPosition)
{
    HRESULT hr = S_OK;

    CUIElement *pMover = static_cast<CUIElement*>(GetItemWithAddRef(nIndex));
    ASSERT(pMover != NULL && pMover->OfTypeByIndex<KnownTypeIndex::UIElement>());

    IFC(pMover->OnZOrderChanged());

    IFC(CDOCollection::MoveInternal(nIndex, nPosition));

    {
        auto callback = m_wrChangeCallback.lock();

        if (callback)
        {
            IFC(callback->ElementMoved(
                static_cast<UINT32>(nIndex),
                static_cast<UINT32>(nPosition)));
        }
    }

Cleanup:
    ReleaseInterface(pMover);
    RRETURN(hr);
}

//------------------------------------------------------------------------
//
//  Synopsis:
//      Invokes the internal Move method and then invalidates layout of
//      owning UIElement.
//
//------------------------------------------------------------------------
_Check_return_ HRESULT
CUIElementCollection::Move(_In_ XINT32 nOldIndex, _In_ XINT32 nNewIndex)
{
    XUINT32 cElements = GetCount();
    CUIElement* const pOwner = static_cast<CUIElement* const>(GetOwner());

    ASSERT(nOldIndex >= 0);
    ASSERT(nNewIndex >= 0);

    if (static_cast<XUINT32>(nOldIndex) >= cElements || static_cast<XUINT32>(nNewIndex) >= cElements)
    {
        // Out-of-bounds situation.
        IFC_RETURN(E_INVALIDARG);
    }

    if (nOldIndex == nNewIndex)
    {
        // Nothing to do.
        return S_OK;
    }

    // This call does the actual move in the children collection. This internal Move method
    // takes a pre-move position for the target location, rather than a post-move index, thus
    // the potential +1 adjustment.
    IFC_RETURN(MoveInternal(nOldIndex /*nIndex*/, (nOldIndex < nNewIndex) ? nNewIndex + 1 : nNewIndex /*nPosition*/));

    // TODO: Automation clients potentially need to be notified of the children reordering. See bug 332439.

    if (pOwner)
    {
        // The children reordering might have an influence on the MeasureOverride evaluation,
        // so InvalidateMeasure is called instead of InvalidateArrange.
        pOwner->InvalidateMeasure();
    }

    return S_OK;
}

//------------------------------------------------------------------------
//
//  Synopsis: Walks all unloading children and forces the animation to end.
//
//------------------------------------------------------------------------
_Check_return_ HRESULT
CUIElementCollection::RemoveAllUnloadingChildren(
    const bool removeFromKeepAliveList,
    _In_opt_ DCompTreeHost* dcompTreeHost)
{
    CLayoutManager* pLayoutManager = NULL;

    if (GetContext())
    {
        pLayoutManager = VisualTree::GetLayoutManagerForElement(this);
    }

    if (HasUnloadingStorage() && m_pUnloadingStorage->Count() > 0)
    {
        // Removing an element will remove it from the m_unloadingElements list.
        // because xchainedmap does not have a reverse iterator,will copy the list to an
        // xvector. In the 99% case this is a no-op.
        // It is not common to find elements in this list, since it would mean
        // this element is destroyed before finishing its unload storyboards.
        xvector<xref_ptr<CUIElement>> cleanupList;

        for (UIElementCollectionUnloadingStorage::UnloadingMap::const_iterator it = m_pUnloadingStorage->m_unloadingElements.begin(); it != m_pUnloadingStorage->m_unloadingElements.end(); ++it)
        {
            IFC_RETURN(cleanupList.push_back(xref_ptr<CUIElement>((*it).first)));
        }

        xvector<xref_ptr<CUIElement>>::const_reverse_iterator rend = cleanupList.rend();
        if (pLayoutManager)
        {
            for (xvector<xref_ptr<CUIElement>>::reverse_iterator it = cleanupList.rbegin(); it != rend; ++it)
            {
                if (*it)
                {
                    LayoutTransitionStorage* pStorage = (*it)->m_pLayoutTransitionStorage;
                    if (pStorage)
                    {
                        pStorage->UnregisterElementForTransitions(*it);
                    }
                    IFC_RETURN(CTransition::CancelTransitions(*it));

                    // Cancel any running implicit Hide animation, and finish unloading it if it's been kept visible for Hide animations.
                    (*it)->CancelImplicitAnimation(ImplicitAnimationType::Hide);
                    (*it)->FlushPendingKeepVisibleOperations();

                    // If the element is held for a connected animation, discard it.
                    bool bWasUnloading = false;
                    IFCFAILFAST(RemoveUnloadedElement((*it), UC_REFERENCE_ConnectedAnimation, &bWasUnloading));
                }
            }
        }

        if (removeFromKeepAliveList)
        {
            for (auto& element : cleanupList)
            {
                if (element->IsKeepingAliveUntilHiddenEventIsRaised())
                {
                    dcompTreeHost->RemoveElementWithKeepAliveCount(element.get());
                }
            }
        }

        m_pUnloadingStorage->Clear();
        SetSortedCollectionDirty(TRUE);
    }
    return S_OK;
}

//------------------------------------------------------------------------
//
//  Synopsis: Finishes the removal of an element that was unloading.
//
//  See UnloadElement for some synopsis comments.
//  The logic here should mirror the actions that would have been applied
//  to the element had it not been unloaded.
//  Note that this method calls Remove (not RemoveImpl) to execute the leave
//  and child setting logic.
//  In the case of a clear that means executing code that would have been
//  called from CDOCollection::Neat, a method that we can not call from
//  here.
//
//------------------------------------------------------------------------
_Check_return_ HRESULT
CUIElementCollection::RemoveUnloadedElement(_In_ CUIElement* pTarget, UINT unloadContext, _Out_ bool* pfExecutedUnload)
{
    HRESULT hr = S_OK;
    UnloadCleanup* removeLogic = NULL;
    ASSERT(HasUnloadingStorage());
    ASSERT(unloadContext == UC_REFERENCE_ThemeTransition ||
           unloadContext == UC_REFERENCE_ConnectedAnimation ||
           unloadContext == UC_REFERENCE_ImplicitAnimation);
    bool executedUnload = false;
    bool fPeggedTarget = true;
    XUINT16 leftTreeCounter = pTarget->m_leftTreeCounter;   // cache the counter, since it is overwritten when Leave occurs. This method will restore it.

    bool canUnload = false;
    if (HasUnloadingStorage() && m_pUnloadingStorage->ContainsKey(pTarget))
    {
        removeLogic = m_pUnloadingStorage->Get(pTarget);

        // Unset unloadContext flag as this is the reason the caller is requesting we finish unloading the element.
        // If there are no remaining reasons for keeping this in unloading storage, then finish unloading,
        // otherwise there are remaining reasons to keep this element in unloading storage.
        *removeLogic = static_cast<UnloadCleanup>(*removeLogic & ~unloadContext);
        if (((*removeLogic & UC_REFERENCE_ThemeTransition) == 0) &&
            ((*removeLogic & UC_REFERENCE_ConnectedAnimation) == 0) &&
            ((*removeLogic & UC_REFERENCE_ImplicitAnimation) == 0))
        {
            canUnload = true;
        }
        else
        {
            removeLogic = nullptr;
        }
    }

    if (canUnload)
    {
        executedUnload = TRUE;
        removeLogic = m_pUnloadingStorage->Remove(pTarget);
        SetSortedCollectionDirty(TRUE);

        // an index has no meaning for unloading items.
        // however, the implementation we have to call expects an index.
        // For performance reasons, we will not calculate the index in the unloadingElements storage
        // since it is not optimized for that. Instead, we will pass a non-existing index and store
        // the unloading element trading a bit of memory for speed. Our RemoveAtImpl override has the
        // logic to return that stored object.
        ASSERT(m_pUnloadingStorage->m_pUnloadingElementBeingRemoved == NULL);
        m_pUnloadingStorage->m_pUnloadingElementBeingRemoved = pTarget;

        if (*removeLogic & UC_ContentControlLeave)
        {
            CContentControl* pCCParent = do_pointer_cast<CContentControl>(pTarget->GetParentInternal());
            ASSERT(pCCParent);
            if (pCCParent)
            {
                HRESULT recordHr = S_OK;
                RECORDFAILURE(pCCParent->DeferredContentRemovalLogic(pTarget));
                ReleaseInterface(pCCParent);    // we took a ref when we registered for this action
                IFC(recordHr);
            }
        }

        if (*removeLogic & UC_UnlinkContainer)
        {
            // Need to keep the target peer alive while we callback the fx to ExecuteDeferredUnlinkAction
            IFC(pTarget->PegManagedPeer());
            fPeggedTarget = true;
        }

        IGNORERESULT(CDOCollection::RemoveAt(IndexIndicatingUnloadingElement()));

        if (*removeLogic & UC_UnlinkContainer)
        {
            IFC(FxCallbacks::Virtualization_ExecuteDeferredUnlinkAction(pTarget));

            ASSERT(fPeggedTarget);
            pTarget->UnpegManagedPeer();
            fPeggedTarget = false;
        }

        if (*removeLogic  & UC_OnRemoveDOFromCollection)
        {
            pTarget->SetAssociated(false, nullptr);
        }
        if (*removeLogic & UC_OnRemoveFromCollection)
        {
            IFC(OnRemoveFromCollection(pTarget));
        }
        if (*removeLogic & UC_ClearLogic)
        {
            // simulating this came from clear
            pTarget->Shutdown();
            ReleaseInterfaceNoNULL(pTarget);
            pTarget->SetAssociated(false, nullptr);
            ReleaseInterfaceNoNULL(pTarget);
            OnCollectionChangeHelper();
        }
        if (*removeLogic & UC_RemoveLogicalParent)
        {
            CFrameworkElement* pLogicalParent = do_pointer_cast<CFrameworkElement>(pTarget->GetLogicalParentNoRef());
            if (pLogicalParent)
            {
                pLogicalParent->RemoveLogicalChild(pTarget);

                if (*removeLogic & UC_ReleaseParentPopup)
                {
                    // The parent Popup was kept alive to allow unload transitions to run.  Release that extra ref now.
                    CPopup* pParentPopup = do_pointer_cast<CPopup>(pLogicalParent);
                    ASSERT(pParentPopup);
                    if (pParentPopup)
                    {
                        pParentPopup->UnpegManagedPeer();
                        pParentPopup->Release();
                    }
                }
            }
            ReleaseInterfaceNoNULL(pTarget); // matching the addref we do before we subscribe for UC_RemoveLogicalParent
        }
        else if (*removeLogic & UC_ReleaseParentPopup)
        {
            // The parent Popup was kept alive to allow unload transitions to run.  Release that extra ref now.
            CPopup* pParentPopup = do_pointer_cast<CPopup>(pTarget->GetLogicalParentNoRef());
            ASSERT(pParentPopup);
            if (pParentPopup)
            {
                pParentPopup->UnpegManagedPeer();
                pParentPopup->Release();
            }
        }

        // set lifecycle to unloaded. The calls above will have forced a leave.
        // but that leave was deferred.
        pTarget->m_leftTreeCounter = leftTreeCounter;

        // Release addref/peg that was set by UnloadElement
        pTarget->UnpegManagedPeer();
        ReleaseInterface(pTarget);  // on failure will be matched
    }

    *pfExecutedUnload = executedUnload;

Cleanup:
    delete removeLogic;
    if (HasUnloadingStorage())
    {
        m_pUnloadingStorage->m_pUnloadingElementBeingRemoved = NULL;
    }
    if (FAILED(hr) && executedUnload && pTarget)
    {
        if (fPeggedTarget)
        {
            pTarget->UnpegManagedPeer();
        }

        // Release addref/peg that was set by UnloadElement
        pTarget->UnpegManagedPeer();
        pTarget->Release();
    }
    RRETURN(hr);
}

// Helper function, returns all the reasons why this element is being kept in unloading storage, as bit-flags
UINT CUIElementCollection::GetUnloadingContext(_In_ CUIElement* element)
{
    UINT result = 0;

    UnloadCleanup* removeLogic = NULL;
    if (HasUnloadingStorage() && m_pUnloadingStorage->ContainsKey(element))
    {
        removeLogic = m_pUnloadingStorage->Get(element);
        result = *removeLogic & (UC_REFERENCE_ThemeTransition | UC_REFERENCE_ConnectedAnimation | UC_REFERENCE_ImplicitAnimation);
    }

    return result;
}

//------------------------------------------------------------------------
//  Synopsis:
//      Returns the local transition root which can be used to hook LTE's
//      If one doesn't exist, it will create one.
//
// Note: TransitionRoot actually lives on the uielementcollection, but to
// mirror AddChild, this method is also defined on UIElement.
//
// Note: we never pro-actively remove the transitionroot (not even when
// all children are removed) because once a uielement has had transitions
// it is likely that it will have them in the future, making it more
// expensive to cleanup and create transitionroots all the time.
//
//------------------------------------------------------------------------
CTransitionRoot* CUIElementCollection::GetLocalTransitionRoot(bool ensureTransitionRoot)
{
    if (!m_pLocalTransitionRoot && ensureTransitionRoot)
    {
        CREATEPARAMETERS cp(GetContext());

        CTransitionRoot* pTransitionRoot = NULL;
        IFCFAILFAST(CTransitionRoot::Create((CDependencyObject**) &pTransitionRoot, &cp));
        m_pLocalTransitionRoot = pTransitionRoot;

        // Setting the parent so dirty propagation works correctly.
        CUIElement* const pOwner = static_cast<CUIElement* const>(GetOwner());
        ASSERT(pOwner);
        IFCFAILFAST(pTransitionRoot->AddParent(pOwner, false, CUIElement::NWSetSubgraphDirty));
    }

    return m_pLocalTransitionRoot;
}

//----------------------------------------------------------------------------
//
//  Synopsis:
//      The virtual method which does the tree walk to clean up all
//      the device related resources like brushes, textures,
//      primitive composition data etc. in this subgraph.
//
//-----------------------------------------------------------------------------
void CUIElementCollection::CleanupDeviceRelatedResourcesRecursive(_In_ bool cleanupDComp)
{
    CDOCollection::CleanupDeviceRelatedResourcesRecursive(cleanupDComp);

    if (m_pLocalTransitionRoot != nullptr)
    {
        m_pLocalTransitionRoot->CleanupDeviceRelatedResourcesRecursive(cleanupDComp);
    }

    if (m_pUnloadingStorage != nullptr)
    {
        if (m_pUnloadingStorage->m_pUnloadingElementBeingRemoved != nullptr)
        {
            m_pUnloadingStorage->m_pUnloadingElementBeingRemoved->CleanupDeviceRelatedResourcesRecursive(cleanupDComp);
        }

        if (m_pUnloadingStorage->m_pUnloadingRoot != nullptr)
        {
            // TODO: m_pUnloadingRoot seems to be an unused field. Remove it.
            m_pUnloadingStorage->m_pUnloadingRoot->CleanupDeviceRelatedResourcesRecursive(cleanupDComp);
        }

        for (UIElementCollectionUnloadingStorage::UnloadingMap::const_iterator it = m_pUnloadingStorage->m_unloadingElements.begin();
            it != m_pUnloadingStorage->m_unloadingElements.end();
            ++it)
        {
            (*it).first->CleanupDeviceRelatedResourcesRecursive(cleanupDComp);
        }
    }
}

//------------------------------------------------------------------------
//
//  Synopsis: Removes an element at a specific index. If the
//  passed in index indicates this element to be unloading, do not remove
//  (since the element is already out of storage) but mark it as such.
//
//------------------------------------------------------------------------
_Check_return_ CDependencyObject* CUIElementCollection::RemoveAtImpl(_In_ XUINT32 nIndex)
{
    if (nIndex == IndexIndicatingUnloadingElement())
    {
        ASSERT(HasUnloadingStorage());
        // no work to be done, except returning the correct object
        return m_pUnloadingStorage->m_pUnloadingElementBeingRemoved;
    }
    else
    {
        return CDOCollection::RemoveAtImpl(nIndex);
    }
}

//------------------------------------------------------------------------
//
//  Method:   UnloadElement
//
//  Synopsis: Helper method being called by both Remove and Clear implementations
//            to unload this element. Returns false if it is not unloading the
//            element so that the element can be removed in the regular way.
//
//  Unloading works as follows: an element is marked as unloading inside this method
//  and moved to secondary storage. It is actually removed from the primary collection
//  using RemoveImpl - so no side effects are executed. From that point forward, all
//  actions that 'would have' executed on it, have to be deferred and I do that by
//  marking actions using the UnloadCleanup flags.
//  When it is time to actually remove the element, it will check that enum and
//  execute all its deferred actions.
//
//------------------------------------------------------------------------
_Check_return_ bool CUIElementCollection::UnloadElement(_In_ XUINT32 nIndex, _In_ CUIElement *pRemove, _In_ UnloadCleanup initialClearLogic)
{
    bool bUnloaded = false;
    HRESULT hr = S_OK; // WARNING_IGNORES_FAILURES
    LayoutTransitionStorage* pStorage = NULL;
    UnloadCleanup* removeLogicToExecute = NULL;
    bool bRemovedButFailedToUnload = false;
    CLayoutManager* pLayoutManager = VisualTree::GetLayoutManagerForElement(pRemove);
    ASSERT(pLayoutManager);
    bool fSetOpacityAtLeave = true;
    XFLOAT opacityCache = 0.0f;
    xvector<CTransition*> applicableUnloadTransitions;
    CUIElement* pParent = do_pointer_cast<CUIElement>(pRemove->GetParentInternal());
    CPanel* pParentAsPanel = do_pointer_cast<CPanel>(pParent);
    CCoreServices* context = GetContext();
    bool isPopup = false;
    CConnectedAnimationService* connectedAnimationService = nullptr;
    bool hasConnectedAnimation = false;

    // APIs that trigger keep alive include ECP's implicit hide animations and UIE's RequestKeepAlive.
    bool areKeepAliveAPIsPresent = false;

    // opacity needs to be determined before we leave the tree.
    // only needed if we have a current layouttransition element representing this element on the screen.
    if (CTransition::HasTransitionAnimations(pRemove))
    {
        CLayoutTransitionElement* pPrimaryBrush = pRemove->GetLayoutTransitionStorage()->GetPrimaryBrush();
        opacityCache = pPrimaryBrush->GetOpacityCombined();
        fSetOpacityAtLeave = false;
    }

    // we allow panels to 'ignore transitions'.
    if (!pParentAsPanel || !pParentAsPanel->GetIsIgnoringTransitions())
    {
        IFC(CTransition::AppendAllTransitionsNoAddRefs(pRemove, DirectUI::TransitionTrigger::Unload, applicableUnloadTransitions));   // cache to pass into processUnload
    }

    // Check for implicit Hide animation and KeepAlive counts
    {
        DCompTreeHost* dcompTreeHost = pRemove->GetDCompTreeHost();
        auto iaMap = dcompTreeHost->GetImplicitAnimationsMap(ImplicitAnimationType::Hide);

        // If the removed element's subtree contains elements with Hidden event handlers, we have to call RequestKeepAlive on them
        // to keep them in the tree until we can raise the Hidden event later this frame. The app's event handlers could start animations
        // on them request they be kept alive, but we need to keep those elements in the tree until Hidden gets raised and those event
        // handlers get to run. Taking the element out and putting it back later could have many unwanted side effects, so we're assuming
        // that the element will still be needed and keeping it in the tree. After Hidden is raised we'll call ReleaseKeepAlive on those
        // same elements, and any elements that the event handlers didn't keep alive will be removed at that time.
        dcompTreeHost->RequestKeepAliveForHiddenEventHandlersInSubtree(pRemove);

        areKeepAliveAPIsPresent = !iaMap.empty() || dcompTreeHost->HasElementsWithKeepAliveCount();
    }

    // Determine whether we should retain the unloading element and add it to the unloading storage.
    connectedAnimationService = context->GetConnectedAnimationServiceNoRef();
    hasConnectedAnimation = connectedAnimationService && connectedAnimationService->HasAnimations();

    isPopup = pRemove->OfTypeByIndex<KnownTypeIndex::Popup>();

    if ((applicableUnloadTransitions.size() > 0 || hasConnectedAnimation || areKeepAliveAPIsPresent) && // has unload transition, connected animation, or implicit hide animation/RequestKeepAlive
        pRemove->IsActive() && // is not itself already unloaded
        (pRemove->HasLayoutStorage() || (isPopup && areKeepAliveAPIsPresent)) // has not been shutdown by parent tree, or is a Popup that may be kept alive
        )
    {
        ASSERT(pParent && pParent->GetChildren() == this);

        // take reference, will be released in RemoveUnloadedElement
        AddRefInterface(pRemove);
        // Peg this element to prevent GC walk (CUIElement::ReferenceTrackerWalkCore) from marking this element
        // as unreachable after it is removed from the collection. Will be unpegged in RemoveUnloadedElement.
        IFC(pRemove->PegManagedPeer());

        if (CDOCollection::RemoveAtImpl(nIndex) == pRemove)
        {
            IFC(CUIElement::EnsureLayoutTransitionStorage(pRemove, NULL, TRUE));
            pStorage = pRemove->GetLayoutTransitionStorage();
            IFCPTR(pStorage);
            IFC(CUIElementCollection::EnsureUnloadingStorage(this));  // this is storage for unloading elements

            pRemove->m_leftTreeCounter = pLayoutManager->GetLayoutCounter();
            // we have left the tree, even though we might not yet have run leaveimpl.
            IFC(CTransition::ProcessUnloadTrigger(pRemove, pStorage, applicableUnloadTransitions));

            // There are multiple reasons why we might move this element into unloading storage.  All can exist simultaneously.
            // For each reason, we set one of the UC_REFERENCE_XXX flags and store this away along with the UnloadCleanup flags.
            // We then expect RemoveUnloadingElement to be called for each reason, only when all reasons for requiring unloading
            // storage are removed will we actually finish unloading the element.
            // This allows, for example, a ConnectedAnimation to run simultaneously with an implicit Hide animation, without
            // one or the other prematurely unloading the element and causing the other to not show its animation.
            bool retainElement = false;
            if (CTransition::HasActiveTransition(pRemove, DirectUI::TransitionTrigger::Unload))
            {
                retainElement = true;
                initialClearLogic = static_cast<UnloadCleanup>(initialClearLogic | UC_REFERENCE_ThemeTransition);
            }
            if (connectedAnimationService)
            {
                bool removedHasConnectedAnimation = false;
                IFC(connectedAnimationService->OnUnloadingElement(pRemove, &removedHasConnectedAnimation));
                if (removedHasConnectedAnimation)
                {
                    retainElement = true;
                    pRemove->OnZOrderChanged(); // Tears down DComp visuals and recreates in new z order position on next RenderWalk
                    initialClearLogic = static_cast<UnloadCleanup>(initialClearLogic | UC_REFERENCE_ConnectedAnimation);
                }
            }
            if (areKeepAliveAPIsPresent)
            {
                // Move to unloading storage if this element or any of its children has a Hide animation or RequestKeepAlive
                if (pRemove->ComputeKeepVisible())
                {
                    pRemove->SetKeepVisible(true);
                    pRemove->OnZOrderChanged(); // Tears down DComp visuals and recreates in new z order position on next RenderWalk
                    initialClearLogic = static_cast<UnloadCleanup>(initialClearLogic | UC_REFERENCE_ImplicitAnimation);
                    retainElement = true;

                    if (isPopup)
                    {
                        // Since we just moved the Popup to unloading storage, it will not leave the live tree yet.
                        // The policy is for Popups to automatically close themselves when they leave the live tree,
                        // so we keep that policy in place by manually closing the Popup now.  This also triggers
                        // special code that accompanies starting a Hide animation in the Popup child subtree.
                        IFCFAILFAST(pRemove->SetValueByKnownIndex(KnownPropertyIndex::Popup_IsOpen, false));
                    }
                }
            }
            if (retainElement)
            {
                removeLogicToExecute = new UnloadCleanup(initialClearLogic);

                m_pUnloadingStorage->Add(pRemove, removeLogicToExecute);
                removeLogicToExecute = NULL;
                bUnloaded = TRUE;
            }
            else
            {
                bRemovedButFailedToUnload = TRUE;
            }
        }
        else
        {
            bRemovedButFailedToUnload = TRUE;
        }
    }

    if (!bUnloaded && pRemove->HasLayoutTransitionStorage())
    {
        pStorage = pRemove->GetLayoutTransitionStorage();
        if (pStorage)
        {
            // did not unload the element. So it will just be removed
            // should stop current transition on it
            // this will as a side effect also set the correct currentInfo on
            // the pStorage
            pStorage->UnregisterElementForTransitions(pRemove);
        }
    }

    if (pRemove->HasLayoutTransitionStorage())
    {
        pRemove->GetLayoutTransitionStorage()->m_opacityCache = fSetOpacityAtLeave ? LeaveShouldDetermineOpacity : opacityCache;
    }


Cleanup:
    delete removeLogicToExecute;
    if (bRemovedButFailedToUnload)
    {
        // recover by restoring state and not unloading
        // all fail jumps are after removal from docollection
        if (nIndex >= GetCount())
        {
            IGNOREHR(CDOCollection::AppendImpl(pRemove));
        }
        else
        {
            IGNOREHR(CDOCollection::InsertImpl(nIndex, pRemove));
        }
        // insertimpl adds a ref whereas RemoveImpl does not, so release
        ReleaseInterfaceNoNULL(pRemove);

        // the following is to match the addref/peg we did that was supposed to be released in RemoveUnloadedElement
        pRemove->UnpegManagedPeer();
        pRemove->Release();
    }

    // note: not checking for failed + bUnloaded == TRUE combination since
    // if we failed after setting bUnloaded to true, the managed call is the one that failed.
    // Therefore doing another call to set it to FALSE is not needed.
    // However, if we ever add more code that can fail, we should take care in setting the flag to false again.


    return bUnloaded;
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
CUIElementCollection::LeaveImpl(_In_ CDependencyObject *pNamescopeOwner,_In_ LeaveParams params)
{
    IFC_RETURN(RemoveAllUnloadingChildren(false /* removeFromKeepAliveList */, nullptr /* dcompTreeHost */));

    IFC_RETURN(CDOCollection::LeaveImpl(pNamescopeOwner, params));
    return S_OK;
}


//------------------------------------------------------------------------
//
//  Method:   Remove - helperfunction
//
//  Synopsis: Move element into temporary storage if it needs to run an
//            unload animation.
//
//------------------------------------------------------------------------
_Check_return_ CDependencyObject*
CUIElementCollection::Remove(_In_ XUINT32 nIndex, _In_ CDependencyObject *pObject)
{
    CUIElement* pRemove = do_pointer_cast<CUIElement>(pObject);
    CUIElement * const pOwner = static_cast<CUIElement * const>(GetOwner());

    if (!pRemove || !UnloadElement(nIndex, pRemove, UC_Null))
    {
        // let base implementation take care of everything
        pObject = static_cast<CDependencyObject*>(CDOCollection::RemoveAt(nIndex));
    }

    if (pObject && pOwner)
    {
        pOwner->InvalidateMeasure();
        OnChildrenChanged(NULL);
    }

    {
        auto callback = m_wrChangeCallback.lock();

        if (callback)
        {
            VERIFYHR(callback->ElementRemoved(nIndex));
        }
    }

    return pObject;
}

//------------------------------------------------------------------------
//
//  Method:   Remove
//
//  Synopsis:
//
//------------------------------------------------------------------------
_Check_return_ CDependencyObject *
CUIElementCollection::Remove( _In_ CDependencyObject *pObject)
{
    XINT32 iObject;

    if(SUCCEEDED(IndexOf(pObject, &iObject)) && iObject != -1)
    {
        // We now have an index.
        return Remove(iObject, pObject);
    }

    return NULL;
}

//------------------------------------------------------------------------
//
//  Method:   RemoveAt
//
//  Synopsis:
//
//------------------------------------------------------------------------
_Check_return_ void*
CUIElementCollection::RemoveAt(_In_ XUINT32 nIndex)
{
    CDependencyObject *pRemove = static_cast<CDependencyObject*>(GetItemWithAddRef(nIndex));
    CDependencyObject *pDO = Remove(nIndex, pRemove);
    ReleaseInterface(pRemove);
    return pDO;
}

//------------------------------------------------------------------------
//
//  Synopsis:
//      Clears the collection.
//
//------------------------------------------------------------------------
_Check_return_ HRESULT
CUIElementCollection::RemoveAllElements(_In_ bool bTryUnloadingElements)
{
    CUIElement* const pOwner = static_cast<CUIElement* const>(GetOwner());

    DestroySortedCollection();

    // do the pre work that is needed
    // shutdown regular elements, and start unloading eligible elements.
    XUINT32 cIndex = GetCount();
    if (cIndex > 0)
    {
        do
        {
            cIndex --;
            CUIElement *pItem = static_cast<CUIElement *>(GetItemWithAddRef(cIndex));
            if (pItem)
            {
                if (!bTryUnloadingElements || !UnloadElement(cIndex, pItem, UC_ClearLogic))
                {
                    // TODO: This does a full breadth walk of the tree, and CDOCollection::Clear() call
                    // TODO: below does the same because it calls Leave() on each child.
                    pItem->Shutdown();

                    ReleaseInterface(pItem);
                }
            }
        } while (cIndex > 0);
    }

    IFC_RETURN(CDOCollection::Clear());

    if (!bTryUnloadingElements)
    {
        // also remove all currently unloading elements
        IFC_RETURN(RemoveAllUnloadingChildren(false /* removeFromKeepAliveList */, nullptr /* dcompTreeHost */));
    }
    else
    {
        if (HasUnloadingStorage())
        {
            // Since we removed the elements above in reverse z order, this will produce a reverse order in unloading storage.
            // Reverse this result so that the rendered z order more closely reflects the original z order in GetChildrenInRenderOrderInternal().
            std::reverse(m_pUnloadingStorage->m_unloadingElements.begin(), m_pUnloadingStorage->m_unloadingElements.end());
        }
    }

    if (pOwner)
    {
        pOwner->InvalidateMeasure();
    }

    {
        auto callback = m_wrChangeCallback.lock();

        if (callback)
        {
            IFC_RETURN(callback->CollectionCleared());
        }
    }

    return S_OK;
}


//------------------------------------------------------------------------
//
//  Method:   Clear
//
//  Synopsis:
//
//------------------------------------------------------------------------
_Check_return_
HRESULT
CUIElementCollection::Clear()
{
    RRETURN(RemoveAllElements(TRUE));
}

UIElementCollectionUnloadingStorage::UIElementCollectionUnloadingStorage()
    : m_pUnloadingElementBeingRemoved(nullptr)
    , m_pUnloadingRoot(nullptr)
{
}

UINT UIElementCollectionUnloadingStorage::Count()
{
    return m_unloadingElements.size();
}

bool UIElementCollectionUnloadingStorage::ContainsKey(_In_ CUIElement* element)
{
    for (UIElementCollectionUnloadingStorage::UnloadingMap::iterator it = m_unloadingElements.begin(); it != m_unloadingElements.end(); it++)
    {
        if (it->first == element)
        {
            return true;
        }
    }

    return false;
}

UnloadCleanup* UIElementCollectionUnloadingStorage::Get(_In_ CUIElement* element)
{
    for (UIElementCollectionUnloadingStorage::UnloadingMap::iterator it = m_unloadingElements.begin(); it != m_unloadingElements.end(); it++)
    {
        if (it->first == element)
        {
            return it->second;
        }
    }

    ASSERT(FALSE);
    return nullptr;
}

void UIElementCollectionUnloadingStorage::Add(_In_ CUIElement* element, UnloadCleanup* unloadCleanup)
{
    ASSERT(!ContainsKey(element));
    m_unloadingElements.push_back(std::pair<CUIElement*, UnloadCleanup*>(element, unloadCleanup));
}

UnloadCleanup* UIElementCollectionUnloadingStorage::Remove(_In_ CUIElement* element)
{
    UnloadCleanup* result = nullptr;

    ASSERT(ContainsKey(element));
    for (UIElementCollectionUnloadingStorage::UnloadingMap::iterator it = m_unloadingElements.begin(); it != m_unloadingElements.end(); it++)
    {
        if (it->first == element)
        {
            result = it->second;
            m_unloadingElements.erase(it);
            break;
        }
    }

    return result;
}

void UIElementCollectionUnloadingStorage::Clear()
{
    m_unloadingElements.clear();
}

CUIElement** doarray_to_elementarray_cast(CDependencyObject * const * doArray)
{
    // The original doArray usually comes from .data() on an std::vector, which returns a const
    // array.  Cast away the constness of the array, since the few callers are passing/returning
    // non-const arrays.
    CDependencyObject** nonConstArray = const_cast<CDependencyObject**>(doArray);

    // Now cast to CUIElement**, with a static assert to ensure no offset between CDO* and CUIElement*.

#if !defined(EXP_CLANG)
    // offset involves reinterpret_cast which strictly is not allowed in static_assert.
    static_assert(offsetof(CUIElement, m_strName) == offsetof(CDependencyObject, m_strName),
      "static_cast between CUIElement and CDependencyObject must be a no-op for the following reinterpret_cast to be safe.");
#endif // EXP_CLANG

    return reinterpret_cast<CUIElement**>(nonConstArray);
}
