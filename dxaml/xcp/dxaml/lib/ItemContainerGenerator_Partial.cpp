// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#include "precomp.h"
#include "ItemContainerGenerator.g.h"
#include "GroupStyle.g.h"
#include "Panel.g.h"
#include "GroupItem.g.h"
#include "SelectorItem.g.h"
#include "ItemsChangedEventArgs.g.h"
#include "IGeneratorHost.g.h"

using namespace DirectUI;
using namespace DirectUISynonyms;


ItemContainerGenerator::ItemContainerGenerator():
    m_pGenerator(NULL),
    m_pItemMap(NULL),
    m_iStartIndexForUIFromItem(0),
    m_nLevel(0),
    m_bIsGrouping(FALSE)
{
    m_ObservableHostItemsChangedToken.value = 0;
    m_ObservableItemsSourceChangedToken.value = 0;
    m_GroupStyleChangedToken.value = 0;

    // Special value indicating "not in use".
    m_PositionFromNotify.Index = -1;
    m_PositionFromNotify.Offset = -1;
}

ItemContainerGenerator::~ItemContainerGenerator()
{
    IGNOREHR(ClearBlocks(/*bRecycle*/ FALSE));

    if (auto peg = m_tpObservableHostItems.TryMakeAutoPeg())
    {
        IGNOREHR(m_tpObservableHostItems->remove_VectorChanged(m_ObservableHostItemsChangedToken));
        m_tpObservableHostItems.Clear();
    }

    auto spObservableItemsSource = m_tpObservableItemsSource.GetSafeReference();
    if (spObservableItemsSource)
    {
        IGNOREHR(spObservableItemsSource->remove_VectorChanged(m_ObservableItemsSourceChangedToken));
    }

    if (auto peg = m_tpGroupStyle.TryMakeAutoPeg())
    {
        IGNOREHR(m_tpGroupStyle.Cast<GroupStyle>()->remove_PropertyChanged(m_GroupStyleChangedToken));
    }

    auto spRecyclableContainers = m_tpRecyclableContainers.GetSafeReference();
    if (spRecyclableContainers)
    {
        IGNOREHR(spRecyclableContainers->Clear());
    }

    auto spSharedRecyclableContainers = m_tpSharedRecyclableContainers.GetSafeReference();
    if (spSharedRecyclableContainers)
    {
        IGNOREHR(spSharedRecyclableContainers->Clear());
    }

    delete m_pGenerator;
    m_pGenerator = NULL;
}

_Check_return_
HRESULT
ItemContainerGenerator::CreateGenerator(
    _In_ IGeneratorHost* pHost,
    _Outptr_ ItemContainerGenerator** ppGenerator)
{
    HRESULT hr = S_OK;
    ctl::ComPtr<ItemContainerGenerator> spGenerator;
    ctl::ComPtr<wfc::VectorChangedEventHandler<IInspectable*>> spItemCollectionVectorChangedHandler;
    ctl::ComPtr<wfc::IVector<IInspectable*>> spView;
    ctl::ComPtr<wfc::IObservableVector<IInspectable*>> spObservableVector;
    ctl::ComPtr<TrackerCollection<xaml::DependencyObject*>> spRecyclableContainers;

    IFCPTR(pHost);
    IFCPTR(ppGenerator);
    *ppGenerator = NULL;

    IFC(ctl::make(&spRecyclableContainers));

    IFC(CreateGenerator(NULL, pHost, NULL, spRecyclableContainers.Get(), 0, &spGenerator));

    IFC(pHost->get_View(&spView));
    IFC(spView.As(&spObservableVector));
    if(spObservableVector)
    {
        spItemCollectionVectorChangedHandler.Attach(
            new ClassMemberEventHandler<
                ItemContainerGenerator,
                IItemContainerGenerator,
                wfc::VectorChangedEventHandler<IInspectable*>,
                wfc::IObservableVector<IInspectable*>,
                wfc::IVectorChangedEventArgs>(spGenerator.Get(), &ItemContainerGenerator::OnItemCollectionChanged));

        IFC(spObservableVector->add_VectorChanged(spItemCollectionVectorChangedHandler.Get(), &(spGenerator->m_ObservableHostItemsChangedToken)));

        spGenerator->SetPtrValue(spGenerator->m_tpObservableHostItems, spObservableVector);
    }

    IFC(spGenerator.CopyTo(ppGenerator));

Cleanup:
    RRETURN(hr);
}

_Check_return_
HRESULT
ItemContainerGenerator::CreateGenerator(
    _In_ ItemContainerGenerator* pParent,
    _In_ GroupItem* pGroupItem,
    _In_ wfc::IVector<xaml::DependencyObject*>* pRecyclableContainers,
    _Outptr_ ItemContainerGenerator** ppGenerator)
{
    HRESULT hr = S_OK;
    ctl::ComPtr<IGeneratorHost> spHost;

    *ppGenerator = NULL;

    IFC(pParent->m_wrHost.As(&spHost));
    IFC(CreateGenerator(pParent, spHost.Get(), pGroupItem, pRecyclableContainers, pParent->m_nLevel+1, ppGenerator));

Cleanup:
    RRETURN(hr);
}

_Check_return_
HRESULT
ItemContainerGenerator::CreateGenerator(
    _In_opt_ ItemContainerGenerator* pParent,
    _In_ IGeneratorHost* pHost,
    _In_opt_ IGroupItem* pPeer,
    _In_ wfc::IVector<xaml::DependencyObject*>* pRecyclableContainers,
    _In_ UINT nLevel,
    _Outptr_ ItemContainerGenerator** ppGenerator)
{
    HRESULT hr = S_OK;
    ctl::ComPtr<ItemContainerGenerator> spGenerator;

    IFCEXPECT(pHost);
    IFCEXPECT(ppGenerator);
    IFCEXPECT(pRecyclableContainers);

    IFC(ctl::make(&spGenerator));
    spGenerator->SetPtrValue(spGenerator->m_tpRecyclableContainers, pRecyclableContainers);

    IFC(ctl::AsWeak(ctl::as_iinspectable(pParent), &spGenerator->m_wrParent));
    IFC(ctl::AsWeak(pPeer, &(spGenerator->m_wrPeer)));

    spGenerator->m_nLevel = nLevel;

    IFC(ctl::AsWeak(pHost, &(spGenerator->m_wrHost)));

    IFC(spGenerator->OnRefresh());

    IFC(spGenerator.CopyTo(ppGenerator));

Cleanup:
    RRETURN(hr);
}

// Return the ItemContainerGenerator appropriate for use by the given panel
_Check_return_ HRESULT ItemContainerGenerator::GetItemContainerGeneratorForPanelImpl(
    _In_opt_ xaml_controls::IPanel* panel,
    _Outptr_ xaml_controls::IItemContainerGenerator** returnValue)
{
    ctl::ComPtr<DependencyObject> spTemplatedParent;
    IFCPTR_RETURN(returnValue);
    *returnValue = nullptr;

    if (!panel)
    {
        return S_OK;
    }

    BOOLEAN isItemsHost = FALSE;
    IFC_RETURN(panel->get_IsItemsHost(&isItemsHost));

    // TODO: Report Error Resx.ItemContainerGenerator_PanelIsNotItemsHost
    // Bug#95997
    IFCEXPECT_RETURN(isItemsHost);

    IFC_RETURN(static_cast<Panel*>(panel)->get_TemplatedParent(&spTemplatedParent));
    // if panel came from a style, use the main generator
    if (spTemplatedParent)
    {
        *returnValue = this;
        AddRefInterface(*returnValue);
    }
    // otherwise the panel doesn't have a generator

    return S_OK;
}

// Begin generating at the given position and direction
// This method must be called before calling GenerateNext.  It returns an
// IDisposable object that tracks the lifetime of the generation loop.
// This method sets the generator's status to GeneratingContent;  when
// the IDisposable is disposed, the status changes to ContentReady or
// Error, as appropriate.
_Check_return_ HRESULT ItemContainerGenerator::StartAtImpl(
    _In_ xaml_primitives::GeneratorPosition position,
    _In_ xaml_primitives::GeneratorDirection direction,
    _In_ BOOLEAN allowStartAtRealizedItem)
{
    HRESULT hr = S_OK;
    Generator* pGenerator = NULL;
    ctl::ComPtr<wfc::IVector<IInspectable*>> spView;
    UINT nCount = 0;

    // TODO: Report Error Resx.ItemContainerGenerator_GenerationInProgress
    // Bug#95997
    IFCEXPECT(!m_pGenerator);

    IFC(get_View(&spView));
    IFC(spView->get_Size(&nCount));

    pGenerator = new Generator(this, nCount, position, direction, allowStartAtRealizedItem);

    m_pGenerator = pGenerator;
    pGenerator = NULL;

Cleanup:
    delete pGenerator;
    RRETURN(hr);
}

// Return the container element used to display the next item.
// When the next item has not been realized, this method returns a container
// and sets isNewlyRealized to true.  When the next item has been realized,
// this method returns the existing container and sets isNewlyRealized to
// false.
// This method must be called after a previous call to StartAt.
_Check_return_ HRESULT ItemContainerGenerator::GenerateNextImpl(
    _Out_ BOOLEAN* isNewlyRealized,
    _Outptr_ xaml::IDependencyObject** returnValue)
{
    HRESULT hr = S_OK; // WARNING_IGNORES_FAILURES

    // TODO: Report Error Resx.ItemContainerGenerator_GenerationNotInProgress
    // Bug#95997
    IFCEXPECT(m_pGenerator);

    IFC(m_pGenerator->GenerateNext(isNewlyRealized, returnValue));

Cleanup:
    RRETURN(S_OK);
}

// Dispose the generator
_Check_return_ HRESULT ItemContainerGenerator::StopImpl()
{
    HRESULT hr = S_OK;
    IFCEXPECT(m_pGenerator);
    delete m_pGenerator;
    m_pGenerator = NULL;

Cleanup:
    RRETURN(hr);
}

// Prepare the given element to act as the container for the
// corresponding item.  This includes applying the container style,
// forwarding information from the host control (ItemTemplate, etc.),
// and other small adjustments.
// This method must be called after the element has been added to the
// visual tree, so that resource references and inherited properties
// work correctly.
_Check_return_ HRESULT ItemContainerGenerator::PrepareItemContainerImpl(
    _In_ xaml::IDependencyObject* container)
{
    HRESULT hr = S_OK;
    ctl::ComPtr<IGeneratorHost> spHost;

    ctl::ComPtr<IInspectable> spItem;

    IFC(static_cast<DependencyObject*>(container)->ReadLocalValue(
        MetadataAPI::GetDependencyPropertyByIndex(KnownPropertyIndex::ItemContainerGenerator_ItemForItemContainer),
        &spItem));
    IFC(m_wrHost.As(&spHost));

    if (spHost)
    {
        IFC(spHost->PrepareItemContainer(container, spItem.Get()));
    }

Cleanup:
    RRETURN(hr);
}

// Remove generated elements.
_Check_return_ HRESULT ItemContainerGenerator::RemoveImpl(
    _In_ xaml_primitives::GeneratorPosition position,
    _In_ INT count)
{
    RRETURN(Remove(position, count, /*isRecycling = */FALSE));
}

// Remove generated elements.
_Check_return_
HRESULT
ItemContainerGenerator::Remove(
    _In_ xaml_primitives::GeneratorPosition position,
    _In_ INT count,
    _In_ BOOLEAN isRecycling)
{
    RealizedItemBlock* pBlockLeft = nullptr;
    RealizedItemBlock* pBlockRight = nullptr;
    RealizedItemBlock* pRealizedBlock = nullptr;
    UnrealizedItemBlock* pBlockTarget = nullptr;
    ItemBlock* pPredecessorBlock = nullptr;
    RealizedItemBlock* pExtraBlock = nullptr;
    ctl::ComPtr<IInspectable> spIsRecycling;

    // TODO: Report Error Resx.ItemContainerGenerator_RemoveRequiresOffsetZero
    // Bug#95997
    IFCEXPECT_RETURN(position.Offset == 0);

    // TODO: Report Error Resx.ItemContainerGenerator_RemoveRequiresPositiveCount
    // Bug#95997
    IFCEXPECT_RETURN(count > 0);

    if (isRecycling)
    {
        IFC_RETURN(PropertyValue::CreateFromBoolean(TRUE, &spIsRecycling));
    }

    INT index = position.Index;
    ItemBlock* pBlock = nullptr;

    // find the leftmost item to remove
    INT offsetL = index;
    for (pBlock = m_pItemMap->m_pNextBlock; pBlock != m_pItemMap; pBlock = pBlock->m_pNextBlock)
    {
        if (offsetL < pBlock->get_ContainerCount())
            break;

        offsetL -= pBlock->get_ContainerCount();
    }

    pBlockLeft = pBlock->AsRealizedBlock();

    // find the rightmost item to remove
    INT offsetR = offsetL + count - 1;
    for (; pBlock != m_pItemMap; pBlock = pBlock->m_pNextBlock)
    {
        // TODO: Report Error Resx.ItemContainerGenerator_CannotRemoveUnrealizedItems
        // Bug#95997
        IFCEXPECT_RETURN(pBlock->AsRealizedBlock());

        if (offsetR < pBlock->get_ContainerCount())
            break;

        offsetR -= pBlock->get_ContainerCount();
    }

    pBlockRight = pBlock->AsRealizedBlock();

    // de-initialize the containers that are being removed
    pRealizedBlock = pBlockLeft;
    INT offset = offsetL;

    const CDependencyProperty* pIsRecycledContainerProperty = MetadataAPI::GetDependencyPropertyByIndex(KnownPropertyIndex::ItemContainerGenerator_IsRecycledContainer);

    while (pRealizedBlock != pBlockRight || offset <= offsetR)
    {
        ctl::ComPtr<xaml::IDependencyObject> spContainer;

        IFC_RETURN(pRealizedBlock->get_ContainerAt(offset, &spContainer));

        if (isRecycling)
        {
            // need to let the container know it is being recycled
            IFC_RETURN(spContainer.Cast<DependencyObject>()->SetValueInternal(pIsRecycledContainerProperty, spIsRecycling.Get(), /* fAllowReadOnly */ TRUE));
        }

        IFC_RETURN(UnlinkContainerFromItem(spContainer.Get(), isRecycling));

        if (isRecycling)
        {
#if DBG
            UINT nIndex = 0;
            BOOLEAN bFound = FALSE;
            IFC_RETURN(m_tpRecyclableContainers->IndexOf(spContainer.Get(), &nIndex, &bFound));
            ASSERT(!bFound, L"Trying to add a container to the collection twice");
#endif
            //    if (_containerType == null)
            //    {
            //        _containerType = container.GetType();
            //    }
            //    else if (_containerType != container.GetType())
            //    {
            //        throw new InvalidOperationException(Resx.GetString(
            //            Resx.ItemContainerGenerator_CannotRecyleHeterogeneousTypes));
            //    }

            IFC_RETURN(m_tpRecyclableContainers->Append(spContainer.Get()));

            // We do not want to tab into the recycled elements.
            if (spContainer.Cast<UIElement>()->GetHandle())
            {
                static_cast<CUIElement*>(spContainer.Cast<UIElement>()->GetHandle())->SetSkipFocusSubtree(true);
            }

            if (ctl::is<IGroupItem>(spContainer.Get()))
            {
                IFC_RETURN(spContainer.Cast<GroupItem>()->Recycle());
            }
        }

        if (++offset >= pRealizedBlock->get_ContainerCount() && pRealizedBlock != pBlockRight)
        {
            pRealizedBlock = pRealizedBlock->m_pNextBlock->AsRealizedBlock();
            offset = 0;
        }
    }

    // see whether the range hits the edge of a block on either side,
    // and whether the a`butting block is an unrealized gap
    BOOLEAN edgeL = (offsetL == 0);
    BOOLEAN edgeR = (offsetR == pBlockRight->m_nItemCount - 1);
    BOOLEAN abutL = edgeL && (pBlockLeft->m_pPrevBlock->AsUnrealizedBlock());
    BOOLEAN abutR = edgeR && (pBlockRight->m_pNextBlock->AsUnrealizedBlock());

    // determine the target (unrealized) block,
    // the offset within the target at which to insert items,
    // and the initial change in cumulative item count
    INT offsetT = 0;
    INT deltaCount = 0;

    if (abutL)
    {
        pBlockTarget = pBlockLeft->m_pPrevBlock->AsUnrealizedBlock();
        offsetT = pBlockTarget->m_nItemCount;
        deltaCount = -pBlockTarget->m_nItemCount;
    }
    else if (abutR)
    {
        pBlockTarget = pBlockRight->m_pNextBlock->AsUnrealizedBlock();
        offsetT = 0;
        deltaCount = offsetL;
    }
    else
    {
        pBlockTarget = new UnrealizedItemBlock();

        offsetT = 0;
        deltaCount = offsetL;

        // remember where the new block goes, so we can insert it later
        pPredecessorBlock = (edgeL) ? pBlockLeft->m_pPrevBlock : pBlockLeft;
    }

    // move items within the range to the target block
    for (pBlock = pBlockLeft; pBlock != pBlockRight; pBlock = pBlock->m_pNextBlock)
    {
        INT itemCount = pBlock->m_nItemCount;
        MoveItems(pBlock, offsetL, itemCount - offsetL,
                    pBlockTarget, offsetT, deltaCount);
        offsetT += itemCount - offsetL;
        offsetL = 0;
        deltaCount -= itemCount;
        if (pBlock->m_nItemCount == 0)
        {
            ItemBlock* pTemp = pBlock->m_pPrevBlock;
            pBlock->Remove();
            pBlock = pTemp;
        }
    }

    // the last block in the range is a little special...
    // Move the last unrealized piece.
    int remaining = pBlock->m_nItemCount - 1 - offsetR;
    MoveItems(pBlock, offsetL, offsetR - offsetL + 1,
                pBlockTarget, offsetT, deltaCount);

    // Move the remaining realized items
    pExtraBlock = pBlockRight;
    if (!edgeR)
    {
        if (pBlockLeft == pBlockRight && !edgeL)
        {
            pExtraBlock = new RealizedItemBlock();
        }

        MoveItems(pBlock, offsetR + 1, remaining,
                    pExtraBlock, 0, offsetR + 1);
    }

    // if we created any new blocks, insert them in the list
    if (pPredecessorBlock)
        pBlockTarget->InsertAfter(pPredecessorBlock);

    if (pExtraBlock != pBlockRight)
        pExtraBlock->InsertAfter(pBlockTarget);

    RemoveAndCoalesceBlocksIfNeeded(pBlock);

    return S_OK;
}

_Check_return_ HRESULT ItemContainerGenerator::ClearBlocks(
    _In_ BOOLEAN bRecycle)
{
    HRESULT hr = S_OK;
    ItemBlock* pItemMap = m_pItemMap;
    m_pItemMap = NULL;

    // de-initialize the containers that are being removed
    if (pItemMap != NULL)
    {
        do
        {
            ItemBlock* pBlock = pItemMap->m_pNextBlock;
            RealizedItemBlock* pRealizedBlock = pBlock->AsRealizedBlock();

            if (pRealizedBlock != NULL)
            {
                for (int offset = 0; offset < pRealizedBlock->get_ContainerCount(); ++offset)
                {
                    xaml::IDependencyObject* pContainerNoRef = NULL;

                    // Get a direct reference to the IDependencyObject.  If we call get_ContainerAt, it will attempt an
                    // AddRef, which could fail during destruction due to the controlling unknown having already been
                    // fully released.

                    IFC(pRealizedBlock->get_ContainerAtNoRef(offset, &pContainerNoRef));

                    // If we can't get a peg on the container, it means the container isn't reachable, and we needn't clean
                    // it up.
                    auto peggedContainer = ctl::try_make_autopeg(static_cast<DependencyObject*>(pContainerNoRef));
                    if( peggedContainer )
                    {
                        IFC(UnlinkContainerFromItem(pContainerNoRef, /*isRecycling*/FALSE));
                        if (bRecycle)
                        {
                            IFC(m_tpRecyclableContainers->Append(pContainerNoRef));
                        }
                    }
                }
            }
            if (pBlock != pItemMap)
            {
                pBlock->Remove();
            }
        }
        while(pItemMap != pItemMap->m_pNextBlock);
        delete pItemMap;
    }

Cleanup:
    RRETURN(hr);
}

// Remove all generated elements.
_Check_return_ HRESULT ItemContainerGenerator::RemoveAllImpl()
{
    ctl::ComPtr<wfc::IVector<IInspectable*>> spView;
    UINT nCount = 0;
    BOOLEAN isRecycling = FALSE;
    BOOLEAN isUnsetValue = FALSE;

    // Clear recycling queue only for Level0 generator
    if (0 == m_nLevel)
    {
        IFC_RETURN(m_tpRecyclableContainers->Clear());
    }
    else
    {
        ctl::ComPtr<IGroupItem> spGroupItem;

        IFC_RETURN(m_wrPeer.As(&spGroupItem));
        if (spGroupItem)
        {
            ctl::ComPtr<IInspectable> spIsRecycling;

            IFC_RETURN(spGroupItem.Cast<GroupItem>()->ReadLocalValue(
                MetadataAPI::GetDependencyPropertyByIndex(KnownPropertyIndex::ItemContainerGenerator_IsRecycledContainer),
                &spIsRecycling));
            IFC_RETURN(DependencyPropertyFactory::IsUnsetValue(spIsRecycling.Get(), isUnsetValue));

            if (!isUnsetValue)
            {
                IFC_RETURN(ctl::do_get_value(isRecycling, spIsRecycling.Get()));
            }
        }
    }

    IFC_RETURN(ClearBlocks(isRecycling));

    IFC_RETURN(PrepareGrouping());

    // re-initialize the data structure
    m_pItemMap = new ItemBlock();

    m_pItemMap->m_pPrevBlock = m_pItemMap->m_pNextBlock = m_pItemMap;

    UnrealizedItemBlock* uib = new UnrealizedItemBlock();

    uib->InsertAfter(m_pItemMap);

    IFC_RETURN(get_View(&spView));
    IFC_RETURN(spView->get_Size(&nCount));

    uib->m_nItemCount = nCount;

    // tell generators what happened
    if (m_pGenerator != nullptr)
    {
        m_pGenerator->OnMapChanged(nullptr, -1, 0, uib, 0, 0);
    }

    return S_OK;
}

// Primitives::IRecyclingItemContainerGenerator
_Check_return_ HRESULT ItemContainerGenerator::RecycleImpl(
    _In_ xaml_primitives::GeneratorPosition position,
    _In_ INT count)
{
    RRETURN(Remove(position, count, /*isRecyling = */TRUE));
}

// Map an index into the items collection to a GeneratorPosition.
_Check_return_ HRESULT ItemContainerGenerator::GeneratorPositionFromIndexImpl(
    _In_ INT itemIndex,
    _Out_ xaml_primitives::GeneratorPosition* returnValue)
{
    HRESULT hr = S_OK;
    xaml_primitives::GeneratorPosition position = {-1, 0};
    ItemBlock* pItemBlock = NULL;
    INT offsetFromBlockStart = 0;

    *returnValue = position;

    if (itemIndex < 0)
    {
        if (itemIndex == -1)
        {
            goto Cleanup;
        }
        else
        {
            IFC(E_INVALIDARG);
        }
    }

    IFC(GetBlockAndPosition(itemIndex, position, pItemBlock, offsetFromBlockStart));
    *returnValue = position;

Cleanup:
    RRETURN(hr);
}

// Map a GeneratorPosition to an index into the items collection.
_Check_return_ HRESULT ItemContainerGenerator::IndexFromGeneratorPositionImpl(
    _In_ xaml_primitives::GeneratorPosition position,
    _Out_ INT* returnValue)
{

    IFCPTR_RETURN(returnValue);
    *returnValue = -1;

    INT index = position.Index;

    if (index == -1)
    {
        // offset is relative to the fictitious boundary item
        if (position.Offset >= 0)
        {
            *returnValue = position.Offset - 1;
        }
        else
        {
            ctl::ComPtr<wfc::IVector<IInspectable*>> spView;

            UINT nCount = 0;
            IFC_RETURN(get_View(&spView));
            IFC_RETURN(spView->get_Size(&nCount));

            *returnValue = nCount + position.Offset;
        }
        return S_OK;
    }

    if (m_pItemMap != nullptr)
    {
        INT itemIndex = 0;      // number of items we've skipped over

        // locate container at the given index
        for (ItemBlock* pBlock = m_pItemMap->m_pNextBlock; pBlock != m_pItemMap; pBlock = pBlock->m_pNextBlock)
        {
            if (index < pBlock->get_ContainerCount())
            {
                // container is within this block.  return the answer
                *returnValue = itemIndex + index + position.Offset;
                return S_OK;
            }

            // skip over this block
            itemIndex += pBlock->m_nItemCount;
            index -= pBlock->get_ContainerCount();
        }
    }

    return S_OK;
}

// Return the item corresponding to the given UI element.
// If the element was not generated as a container for this generator's
// host, the method returns DependencyProperty.UnsetValue.
_Check_return_ HRESULT ItemContainerGenerator::ItemFromContainerImpl(
    _In_ xaml::IDependencyObject* container,
    _Outptr_ IInspectable** returnValue)
{
    HRESULT hr = S_OK;
    BOOLEAN isUnsetValue = FALSE;
    ctl::ComPtr<IInspectable> spItem;

    IFCPTR(container);
    IFCPTR(returnValue);
    *returnValue = NULL;

    IFC(static_cast<DependencyObject*>(container)->ReadLocalValue(
        MetadataAPI::GetDependencyPropertyByIndex(KnownPropertyIndex::ItemContainerGenerator_ItemForItemContainer),
        &spItem));
    IFC(DependencyPropertyFactory::IsUnsetValue(spItem.Get(), isUnsetValue));

    if (!isUnsetValue)
    {
        BOOLEAN isHost = FALSE;
        ctl::ComPtr<IGeneratorHost> spHost;

        IFC(m_wrHost.As(&spHost));
        if (spHost)
        {
            IFC(spHost->IsHostForItemContainer(container, &isHost));
        }
        if (!isHost)
        {
            IFC(DependencyPropertyFactory::GetUnsetValue(&spItem));
        }
    }

    IFC(spItem.CopyTo(returnValue));

Cleanup:
    RRETURN(hr);
}

// Return the UI element corresponding to the given item.
// Returns null if the item does not belong to the item collection,
// or if no UI has been generated for it.
_Check_return_ HRESULT ItemContainerGenerator::ContainerFromItemImpl(
    _In_opt_ IInspectable* item,
    _Outptr_ xaml::IDependencyObject** returnValue)
{
    HRESULT hr = S_OK;

    IFCPTR(returnValue);
    *returnValue = NULL;

    IFC(DoLinearSearch(returnValue, item, nullptr));

Cleanup:
    RRETURN(hr);
}

// Given a generated UI element, return the index of the corresponding item
// within the ItemCollection.
_Check_return_ HRESULT ItemContainerGenerator::IndexFromContainerImpl(
    _In_ xaml::IDependencyObject* container,
    _Out_ INT* returnValue)
{
    HRESULT hr = S_OK;
    IFCPTR(container);

    IFC(DoLinearSearch(&container, nullptr, returnValue));

Cleanup:
    RRETURN(hr);
}

//     Performs a linear search.

//     There's no avoiding a linear search, which leads to O(n^2) performance
//     if someone calls ContainerFromItem or IndexFromContainer for every item.
//     To mitigate this, we start each search at _startIndexForUIFromItem, and
//     heuristically set this in various places to where we expect the next
//     call to occur.

//     For example, after a successul search, we set it to the resulting
//     index, hoping that the next call will query either the same item or
//     the one after it.  And after inserting a new item, we expect a query
//     about the new item.  Etc.

//     Saving this as an index instead of a (block, offset) pair, makes it
//     more robust during insertions/deletions.  If the index ends up being
//     wrong, the worst that happens is a full search (as opposed to following
//     a reference to a block that's no longer in use).

//     To re-use the search code for two methods, please read the description
//     of the parameters.

// <param name="container">
//     If non-null, then searches for the container.
//     Otherwise, updated with container for the item.
// </param>
// <param name="item">
//     If non-null, then searches for the item.
//     Otherwise, updated with the item that the container represents.
// </param>
// <param name="itemIndex">
//     If a container or item is found, then updated with its index.
//     Otherwise, set to -1.
// </param>
_Check_return_
HRESULT
ItemContainerGenerator::DoLinearSearch(
    _Inout_ xaml::IDependencyObject** ppContainer,
    _In_opt_ IInspectable* pItem,
    _Out_opt_ INT* pItemIndex,
    _Out_opt_ bool* found)
{
    if (pItemIndex)
    {
        *pItemIndex = 0;
    }

    if (!m_pItemMap)
    {
        // _itemMap can be null if we re-enter the generator.  Scenario:  user calls RemoveAll(), we Unlink every container, fire
        // ClearContainerForItem for each, and someone overriding ClearContainerForItem decides to look up the container.
        return S_OK;
    }

    if (found)
    {
        *found = false;
    }

    // Move to the starting point of the search
    ItemBlock* pStartBlock = m_pItemMap->m_pNextBlock;
    INT index = 0;      // index of first item in current block
    RealizedItemBlock* pRealizedBlock = nullptr;
    INT startOffset = 0;

    while (index <= m_iStartIndexForUIFromItem && pStartBlock != m_pItemMap)
    {
        index += pStartBlock->m_nItemCount;
        pStartBlock = pStartBlock->m_pNextBlock;
    }
    pStartBlock = pStartBlock->m_pPrevBlock;
    index -= pStartBlock->m_nItemCount;
    pRealizedBlock = pStartBlock->AsRealizedBlock();

    if (pRealizedBlock)
    {
        startOffset = m_iStartIndexForUIFromItem - index;
        if (startOffset >= pRealizedBlock->m_nItemCount)
        {
            // we can get here if items get removed since the last
            // time we saved m_iStartIndexForUIFromItem - so the
            // saved offset is no longer meaningful.  To make the
            // search work, we need to make sure the first loop
            // does at least one iteration.  Setting startOffset to 0
            // does exactly that.
            startOffset = 0;
        }
    }
    else
    {
        startOffset = 0;
    }

    // search for the desired item, wrapping around the end
    ItemBlock* pBlock = pStartBlock;
    INT offset = startOffset;
    INT endOffset = pStartBlock->m_nItemCount;
    while (true)
    {
        // search the current block (only need to search realized blocks)
        if (pRealizedBlock)
        {
            for (; offset < endOffset; ++offset)
            {
                ctl::ComPtr<xaml::IDependencyObject> spContainer;
                ctl::ComPtr<IInspectable> spLocalItem;
                bool tempFound = false;

                if (*ppContainer)
                {
                    IFC_RETURN(pRealizedBlock->get_ContainerAt(offset, &spContainer));
                    if (spContainer.Get() == *ppContainer)
                    {
                        tempFound = true;
                    }
                }
                else
                {
                    IFC_RETURN(pRealizedBlock->get_ItemAt(offset, &spLocalItem));
                    IFC_RETURN(PropertyValue::AreEqual(spLocalItem.Get(), pItem, &tempFound));
                    if (tempFound)
                    {
                        IFC_RETURN(pRealizedBlock->get_ContainerAt(offset, &spContainer));
                        IFC_RETURN(spContainer.CopyTo(ppContainer));
                    }
                }

                if (m_bIsGrouping && !tempFound)
                {
                    IFC_RETURN(pRealizedBlock->get_ItemAt(offset, &spLocalItem));
                    if (ctl::is<ICollectionViewGroup>(spLocalItem.Get()))
                    {
                        ctl::ComPtr<IGroupItem> spGroupItem;

                        IFC_RETURN(pRealizedBlock->get_ContainerAt(offset, &spContainer));
                        IFC_RETURN(spContainer.As(&spGroupItem));
                        IFC_RETURN(spGroupItem.Cast<GroupItem>()->m_tpGenerator.Cast<ItemContainerGenerator>()->DoLinearSearch(ppContainer, pItem, pItemIndex, &tempFound));
                    }
                }

                if (tempFound)
                {
                    // found the item;  update state and return
                    m_iStartIndexForUIFromItem = index + offset;
                    if (pItemIndex)
                    {
                        INT realizedCount = 0;
                        INT blockCount = 0;
                        IFC_RETURN(GetRealizedItemBlockCount(pRealizedBlock, offset, realizedCount));
                        IFC_RETURN(GetCount(pBlock, blockCount));
                        *pItemIndex += realizedCount + blockCount;
                    }
                    if (found)
                    {
                        *found = true;
                    }
                    return S_OK;
                }
            }

            // check for termination
            if (pBlock == pStartBlock && offset == startOffset)
            {
                if (pItemIndex)
                {
                    *pItemIndex = -1;
                }
                return S_OK;
            }
        }

        // advance to next block
        index += pBlock->m_nItemCount;
        offset = 0;
        pBlock = pBlock->m_pNextBlock;

        // if we've reached the end, wrap around
        if (pBlock == m_pItemMap)
        {
            pBlock = pBlock->m_pNextBlock;
            index = 0;
        }

        // prepare to search the block
        endOffset = pBlock->m_nItemCount;
        pRealizedBlock = pBlock->AsRealizedBlock();

        // check for termination
        if (pBlock == pStartBlock)
        {
            if (pRealizedBlock)
            {
                endOffset = startOffset;    // search first part of block
            }
            else
            {
                if (pItemIndex)
                {
                    *pItemIndex = -1;
                }
                return S_OK;
            }
        }
    }
    return S_OK;
}

_Check_return_
HRESULT
ItemContainerGenerator::GetCount(
    _Out_ INT& count)
{
    RRETURN(GetCount(m_pItemMap, count));
}


_Check_return_
HRESULT
ItemContainerGenerator::GetCount(
    _In_ ItemBlock* pStopBlock,
    _Out_ INT& count)
{
    HRESULT hr = S_OK;
    ctl::ComPtr<wfc::IVector<IInspectable*>> spView;
    UINT nCurrentGroupItemCount = 0;
    UINT currentIndex = 0;

    count = 0;
    ItemBlock* pStartBlock = m_pItemMap;
    ItemBlock* pBlock = pStartBlock->m_pNextBlock;

    IFC(get_View(&spView));
    while (pBlock != pStopBlock)
    {
        RealizedItemBlock* pRealizedBlock = pBlock->AsRealizedBlock();
        if (pRealizedBlock)
        {
            INT realizedCount = 0;
            IFC(GetRealizedItemBlockCount(pRealizedBlock, pBlock->m_nItemCount, realizedCount));
            count += realizedCount;
            currentIndex += pBlock->m_nItemCount;
        }
        else if (!m_bIsGrouping)
        {
            count += pBlock->m_nItemCount;
            currentIndex += pBlock->m_nItemCount;
        }
        else
        {
            for (INT i = 0; i < pBlock->m_nItemCount; i++)
            {
                ctl::ComPtr<IInspectable> spCurrentItem;
                IFC(spView->GetAt(currentIndex, &spCurrentItem));
                IFC(GetItemsCount(spCurrentItem.Get(), nCurrentGroupItemCount));
                count += nCurrentGroupItemCount;
                currentIndex++;
            }
        }

        pBlock = pBlock->m_pNextBlock;
    }

Cleanup:
    RRETURN(hr);
}

_Check_return_ HRESULT ItemContainerGenerator::GetItemsCount(
    _In_ IInspectable* pItem,
    _Out_ UINT& nItemCount)
{
    HRESULT hr = S_OK;
    ctl::ComPtr<ICollectionViewGroup> spCurrentCollectionViewGroup;
    UINT nGroupSize = 0;
    UINT nCurrentGroupItemCount = 0;

    nItemCount = 1;

    spCurrentCollectionViewGroup = ctl::query_interface_cast<ICollectionViewGroup>(pItem);

    if (spCurrentCollectionViewGroup)
    {
        ctl::ComPtr<wfc::IVector<IInspectable*>> spGroupView;
        ctl::ComPtr<wfc::IObservableVector<IInspectable*>> spGroupItems;

        nItemCount = 0;
        IFC(spCurrentCollectionViewGroup->get_GroupItems(&spGroupItems));
        IFC(spGroupItems.As(&spGroupView));
        IFC(spGroupView->get_Size(&nGroupSize));
        for(UINT i = 0; i < nGroupSize; i++)
        {
            ctl::ComPtr<IInspectable> spCurrentItem;
            IFC(spGroupView->GetAt(i, &spCurrentItem));
            IFC(GetItemsCount(spCurrentItem.Get(), nCurrentGroupItemCount));
            nItemCount += nCurrentGroupItemCount;
        }
    }

Cleanup:
    RRETURN(hr);
}

_Check_return_
HRESULT
ItemContainerGenerator::GetRealizedItemBlockCount(
    _In_ RealizedItemBlock* pRealizedBlock,
    _In_ INT end,
    _Out_ INT& count)
{
    HRESULT hr = S_OK;
    ctl::ComPtr<IInspectable> spItem;
    ctl::ComPtr<xaml::IDependencyObject> spContainer;
    ctl::ComPtr<IGroupItem> spGroupItem;

    if (!m_bIsGrouping)
    {
        // when the UI is not grouping, each item counts as 1,
        // even groups
        count = end;
        goto Cleanup;
    }

    count = 0;
    for (INT offset = 0; offset < end; ++offset)
    {
        IFC(pRealizedBlock->get_ItemAt(offset, &spItem));
        if (ctl::is<ICollectionViewGroup>(spItem.Get()))
        {
            // found a group, count the group
            INT realizedCount = 0;
            IFC(pRealizedBlock->get_ContainerAt(offset, &spContainer));
            IFC(spContainer.As(&spGroupItem));
            IFC(spGroupItem.Cast<GroupItem>()->m_tpGenerator.Cast<ItemContainerGenerator>()->GetCount(realizedCount));
            count += realizedCount;
        }
        else
        {
            count++;
        }
    }

Cleanup:
    RRETURN(hr);
}

// Return the UI element corresponding to the item at the given index
// within the current level.
_Check_return_ HRESULT
ItemContainerGenerator::ContainerFromGroupIndex(
    _In_ INT index,
    _Outptr_ xaml::IDependencyObject** returnValue)
{
    HRESULT hr = S_OK;
    ctl::ComPtr<xaml::IDependencyObject> spContainer;

    IFCPTR(returnValue);
    *returnValue = NULL;

    // search the table for the item
    for (ItemBlock* pBlock = m_pItemMap->m_pNextBlock; pBlock != m_pItemMap; pBlock = pBlock->m_pNextBlock)
    {
        if (index < pBlock->m_nItemCount)
        {
            IFC(pBlock->get_ContainerAt(index, &spContainer));
            break;
        }

        index -= pBlock->m_nItemCount;
    }

    IFC(spContainer.CopyTo(returnValue));

Cleanup:
    RRETURN(hr);
}

// Return the UI element corresponding to the item at the given index
// within the ItemCollection.
_Check_return_ HRESULT ItemContainerGenerator::ContainerFromIndexImpl(
    _In_ INT index,
    _Outptr_ xaml::IDependencyObject** ppReturnValue)
{
    ctl::ComPtr<xaml::IDependencyObject> spContainer;
    ctl::ComPtr<wfc::IVector<IInspectable*>> spView;

    IFCPTR_RETURN(ppReturnValue);
    *ppReturnValue = nullptr;

    INT entryIndex = 0;
    UINT groupIndex = 0;
    UINT groupsCount = 0;

    if (m_bIsGrouping)
    {
        IFC_RETURN(get_View(&spView));
        IFC_RETURN(spView->get_Size(&groupsCount));
        if (groupsCount == 0)
        {
            // If there aren't any groups then nothing to do
            return S_OK;
        }
    }

    // search the table for the item
    for (ItemBlock* pBlock = m_pItemMap->m_pNextBlock; pBlock != m_pItemMap;)
    {
        if (m_bIsGrouping)
        {
            ctl::ComPtr<ICollectionViewGroup> spGroup;

            if (entryIndex < pBlock->m_nItemCount)
            {
                IFC_RETURN(pBlock->get_ContainerAt(entryIndex, &spContainer));
                if (spContainer)
                {
                    ctl::ComPtr<IGroupItem> spGroupItem = spContainer.AsOrNull<IGroupItem>();

                    if (spGroupItem)
                    {
                        groupIndex++;

                        INT count = 0;
                        IFC_RETURN(spGroupItem.Cast<GroupItem>()->m_tpGenerator.Cast<ItemContainerGenerator>()->GetCount(count));
                        if (index < count)
                        {
                            IFC_RETURN(spGroupItem.Cast<GroupItem>()->m_tpGenerator->ContainerFromIndex(index, &spContainer));
                            break;
                        }
                        else
                        {
                            index -= count;
                        }
                    }
                    else
                    {
                        index--;
                    }
                }
                else
                {
                    ctl::ComPtr<IInspectable> spItemGroup;
                    IFCEXPECT_RETURN(groupsCount > groupIndex);
                    IFC_RETURN(spView->GetAt(groupIndex, &spItemGroup));
                    spGroup = spItemGroup.AsOrNull<ICollectionViewGroup>();

                    if (spGroup)
                    {
                        UINT nGroupCount = 0;
                        ctl::ComPtr<wfc::IVector<IInspectable*>> spGroupView;
                        ctl::ComPtr<wfc::IObservableVector<IInspectable*>> spGroupItems;

                        groupIndex++;

                        IFC_RETURN(spGroup->get_GroupItems(&spGroupItems));
                        IFC_RETURN(spGroupItems.As(&spGroupView));
                        IFC_RETURN(spGroupView->get_Size(&nGroupCount));
                        index -= nGroupCount;
                    }
                    else
                    {
                        index--;
                    }
                }

                if (index < 0)
                {
                    break;
                }
                entryIndex++;
            }
            else
            {
                entryIndex = 0;
                pBlock = pBlock->m_pNextBlock;
            }
        }
        else
        {
            if (index < pBlock->m_nItemCount)
            {
                IFC_RETURN(pBlock->get_ContainerAt(index, &spContainer));
                break;
            }

            index -= pBlock->m_nItemCount;
            pBlock = pBlock->m_pNextBlock;
        }
    }

    IFC_RETURN(spContainer.CopyTo(ppReturnValue));

    return S_OK;
}

// The ItemsChanged event is raised by a ItemContainerGenerator to inform
// layouts that the items collection has changed.
_Check_return_
HRESULT
ItemContainerGenerator::GetItemsChangedEventSourceNoRef(
    _Outptr_ ItemsChangedEventSourceType** ppEventSource)
{
    HRESULT hr = S_OK;

    IFCPTR(ppEventSource);
    *ppEventSource = NULL;

    if (!m_spEventSource)
    {
        IFC(ctl::make(&m_spEventSource));
        m_spEventSource->Initialize(KnownEventIndex::ItemContainerGenerator_ItemsChanged, this, FALSE);
    }

    *ppEventSource = m_spEventSource.Get();

Cleanup:
    RRETURN(hr);
}

// The read only view of items
_Check_return_
HRESULT
ItemContainerGenerator::get_View(
    _Outptr_ wfc::IVector<IInspectable*>** pValue)
{
    HRESULT hr = S_OK;

    IFCPTR(pValue);
    IFCEXPECT(m_tpView);

    *pValue = m_tpView.Get();
    AddRefInterface(*pValue);

Cleanup:
    RRETURN(hr);
}

//     Generator is the object that generates UI on behalf of an ItemsControl,
//     working under the supervision of an ItemContainerGenerator.
ItemContainerGenerator::Generator::Generator(
    _In_ ItemContainerGenerator* pFactory,
    _In_ UINT nItemsCount,
    _In_ xaml_primitives::GeneratorPosition position,
    _In_ xaml_primitives::GeneratorDirection direction,
    _In_ BOOLEAN allowStartAtRealizedItem)
{
    HRESULT hr = S_OK;

    m_pFactory = pFactory;
    m_direction = direction;

    IFC(m_pFactory->MoveToPosition(position, direction, allowStartAtRealizedItem, nItemsCount, m_cachedState));
    m_bDone = (nItemsCount == 0);

Cleanup:
    VERIFYHR(hr);
}

// Generate UI for the next item or group
_Check_return_
HRESULT
ItemContainerGenerator::Generator::GenerateNext(
    _Out_ BOOLEAN* isNewlyRealized,
    _Outptr_ xaml::IDependencyObject** returnValue)
{
    TraceGenerateContainerBegin();
    HRESULT hr = S_OK;
    ctl::ComPtr<xaml::IDependencyObject> spContainer;
    ctl::ComPtr<IGeneratorHost> spHost;
    const CDependencyProperty* pIsRecycledContainerProperty = MetadataAPI::GetDependencyPropertyByIndex(KnownPropertyIndex::ItemContainerGenerator_IsRecycledContainer);

    IFCPTR(isNewlyRealized);
    *isNewlyRealized = FALSE;

    IFCPTR(returnValue);

    IFC(m_pFactory->m_wrHost.As(&spHost));
    IFCEXPECT(spHost);

    while (!spContainer)
    {
        ctl::ComPtr<wfc::IVector<IInspectable*>> spView;

        UnrealizedItemBlock* uBlock = m_cachedState.Block->AsUnrealizedBlock();
        INT32 itemIndex = m_cachedState.ItemIndex;
        UINT nCount = 0;

        *returnValue = NULL;

        IFC(m_pFactory->get_View(&spView));


        IFC(spView->get_Size(&nCount));

        if (m_cachedState.Block == m_pFactory->m_pItemMap)
            m_bDone = true;            // we've reached the end of the list

        if (itemIndex < 0 || itemIndex >= static_cast<INT>(nCount))
            m_bDone = true;

        if (m_bDone)
        {
            goto Cleanup;
        }

        if (uBlock != NULL)
        {
            UINT nRecyclingCount = 0;
            ctl::ComPtr<IInspectable> spItem;
            ctl::ComPtr<ICollectionViewGroup> spGroup;
            ctl::ComPtr<xaml::IDependencyObject> spRecycledContainer;

            *isNewlyRealized = TRUE;

            // We don't have a realized container for this item.  Try to use a recycled container
            // if possible, otherwise generate a new container.
            IFC(m_pFactory->m_tpRecyclableContainers->get_Size(&nRecyclingCount));

            if (nRecyclingCount > 0)
            {
                BOOLEAN isOwnContainer = FALSE;
                IFC(spHost->IsItemItsOwnContainer(spItem.Get(), &isOwnContainer));
                if (!isOwnContainer)
                {
                    IFC(m_pFactory->m_tpRecyclableContainers->GetAt(0, &spRecycledContainer));
                    #if DBG
                    bool isTransitioning = false;
                    IFC(CoreImports::UIElement_GetHasTransition(static_cast<CUIElement*>(spRecycledContainer.Cast<UIElement>()->GetHandle()), &isTransitioning, NULL));
                    ASSERT(!isTransitioning, L"we should not have put a transitioning item in the recycle queue");
                    #endif

                    IFC(m_pFactory->m_tpRecyclableContainers->RemoveAt(0));
                    IFC(spRecycledContainer.Cast<DependencyObject>()->ClearValue(pIsRecycledContainerProperty));  // no longer in recycle queue

                    if (spRecycledContainer.Cast<UIElement>()->GetHandle())
                    {
                        // It is okay to tab into this element.
                        static_cast<CUIElement*>(spRecycledContainer.Cast<UIElement>()->GetHandle())->SetSkipFocusSubtree(false);
                    }

                    // When the container is coming from recycling queue we should mark it as not newly realized.
                    // It tells to the panel that the container is in the visual tree. But in level1 and higher generators
                    // we remove containers from the visual tree and just share them between generators. Thus we should treat them as
                    // newly created containers.
                    if (0 == m_pFactory->m_nLevel)
                    {
                        *isNewlyRealized = FALSE;
                    }
                }
            }

            IFC(spView->GetAt(itemIndex, &spItem));
            spGroup = spItem.AsOrNull<ICollectionViewGroup>();

            if (!spGroup || !m_pFactory->m_bIsGrouping)
            {
                // generate container for an item
                IFC(spHost->GetContainerForItem(spItem.Get(), spRecycledContainer.Get(), &spContainer));
                IFC(m_pFactory->LinkContainerToItem(spContainer.Get(), spItem.Get()));
            }
            else
            {
                // generate container for a group
                IFC(m_pFactory->ContainerForGroup(spGroup.Get(), spRecycledContainer.Get(), &spContainer));
            }

            // add the (item, container) to the current block
            if (spContainer)
            {
                uBlock = m_cachedState.Block->AsUnrealizedBlock();
                IFCEXPECT(uBlock);
                IFC(m_pFactory->Realize(uBlock, m_cachedState.Offset, spItem.Get(), spContainer.Get()));
            }

        }
        else
        {
            // return existing realized container
            *isNewlyRealized = FALSE;
            RealizedItemBlock* rib = m_cachedState.Block->AsRealizedBlock();
            IFC(rib->get_ContainerAt(m_cachedState.Offset, &spContainer));
        }

        // advance to the next item
        m_cachedState.ItemIndex = itemIndex;
        if (m_direction == xaml_primitives::GeneratorDirection::GeneratorDirection_Forward)
        {
            m_cachedState.Block->StepForward(m_cachedState, TRUE);
        }
        else
        {
            m_cachedState.Block->StepBackward(m_cachedState, TRUE);
        }
    }

    IFC(spContainer.CopyTo(returnValue));

Cleanup:
    TraceGenerateContainerEnd();
    RRETURN(hr);
}

// The map data structure has changed, so the state must change accordingly.
// This is called in various different ways.
//  A. Items were moved within the data structure, typically because
//  items were realized or un-realized.  In this case, the args are:
//      block - the block from where the items were moved
//      offset - the offset within the block of the first item moved
//      count - how many items moved
//      newBlock - the block to which the items were moved
//      newOffset - the offset within the new block of the first item moved
//      deltaCount - the difference between the cumululative item counts
//                  of newBlock and block
//  B. An item was added or removed from the data structure.  In this
//  case the args are:
//      block - null  (to distinguish case B from case A)
//      offset - the index of the changed item, w.r.t. the entire item list
//      count - +1 for insertion, -1 for deletion
//      others - unused
//  C. Refresh: all items are returned to a single unrealized block.
//  In this case, the args are:
//      block - null
//      offset - -1 (to distinguish case C from case B)
//      newBlock = the single unrealized block
//      others - unused
void
ItemContainerGenerator::Generator::OnMapChanged(
    _In_opt_ ItemBlock* pBlock,
    _In_ INT offset,
    _In_ INT count,
    _In_opt_ ItemBlock* pNewBlock,
    _In_ INT newOffset,
    _In_ INT deltaCount)
{
    // Case A.  Items were moved within the map data structure
    if (pBlock != NULL)
    {
        // if the move affects this generator, update the cached state
        if (pBlock == m_cachedState.Block && offset <= m_cachedState.Offset &&
            m_cachedState.Offset < offset + count)
        {
            m_cachedState.Block = pNewBlock;
            m_cachedState.Offset += newOffset - offset;
            m_cachedState.Count += deltaCount;
        }
    }
    // Case B.  An item was inserted or deleted
    else if (offset >= 0)
    {
        // if the item occurs before my block, update my item count
        if (offset < m_cachedState.Count)
        {
            m_cachedState.Count += count;
            m_cachedState.ItemIndex += count;
        }
        // if the item occurs within my block before my item, update my offset
        else if (offset < m_cachedState.Count + m_cachedState.Offset)
        {
            m_cachedState.Offset += count;
            m_cachedState.ItemIndex += count;
        }
        // if an insert occurs at my position, update my offset
        else if (offset == m_cachedState.Count + m_cachedState.Offset &&
            count > 0)
        {
            m_cachedState.Offset += count;
            m_cachedState.ItemIndex += count;
        }
    }
    // Case C.  Refresh
    else
    {
        m_cachedState.Block = pNewBlock;
        m_cachedState.Offset += m_cachedState.Count;
        m_cachedState.Count = 0;
    }
}

_Check_return_
HRESULT
ItemContainerGenerator::MoveToPosition(
    _In_ xaml_primitives::GeneratorPosition position,
    _In_ xaml_primitives::GeneratorDirection direction,
    _In_ BOOLEAN allowStartAtRealizedItem,
    _In_ UINT nItemsCount,
    _Out_ GeneratorState& state)
{
    HRESULT hr = S_OK;
    ItemBlock* pBlock = m_pItemMap;
    int itemIndex = 0;

    // first move to the indexed (realized) item
    if (position.Index != -1)
    {
        // find the right block
        int itemCount = 0;
        int index = position.Index;
        pBlock = pBlock->m_pNextBlock;
        while (index >= pBlock->get_ContainerCount())
        {
            itemCount += pBlock->m_nItemCount;
            index -= pBlock->get_ContainerCount();
            itemIndex += pBlock->m_nItemCount;
            pBlock = pBlock->m_pNextBlock;
        }

        // set the position
        state.Block = pBlock;
        state.Offset = index;
        state.Count = itemCount;
        state.ItemIndex = itemIndex + index;
    }
    else
    {
        state.Block = m_pItemMap;
        state.Offset = 0;
        state.Count = 0;
        state.ItemIndex = -1;
    }

    // adjust the offset - we always set the state so it points to the next
    // item to be generated.
    int offset = position.Offset;
    if (offset == 0 && (!allowStartAtRealizedItem || state.Block == m_pItemMap))
    {
        offset = (direction == xaml_primitives::GeneratorDirection::GeneratorDirection_Forward) ? 1 : -1;
    }

    // advance the state according to the offset
    if (offset > 0)
    {
        do
        {
            state.Block->MoveToForward(state, offset, allowStartAtRealizedItem);
        }
        while (offset > 0 && state.Block != m_pItemMap);
    }
    else if (offset < 0)
    {
        if (state.Block == m_pItemMap)
        {
            state.ItemIndex = state.Count = nItemsCount;
        }
        do
        {
            state.Block->MoveToBackward(state, offset, allowStartAtRealizedItem);
        }
        while (offset > 0 && state.Block != m_pItemMap);
    }

    RRETURN(hr);
}

// "Realize" the item in a block at the given offset, to be
// the given item with corresponding container.  This means updating
// the item map data structure so that the item belongs to a Realized block.
// It also requires updating the state of every generator to track the
// changes we make here.
_Check_return_
HRESULT
ItemContainerGenerator::Realize(
    _In_ UnrealizedItemBlock* pBlock,
    _In_ INT offset,
    _In_ IInspectable* pItem,
    _In_ xaml::IDependencyObject* pContainer)
{
    HRESULT hr = S_OK;
    RealizedItemBlock* prevR = NULL;
    RealizedItemBlock* nextR = NULL;

    RealizedItemBlock* newBlock = NULL; // new location of the target item
    int newOffset = 0;                  // its offset within the new block

    // if we're realizing the leftmost item and there's room in the
    // previous block, move it there
    if (offset == 0 &&
        (prevR = pBlock->m_pPrevBlock->AsRealizedBlock()) != NULL &&
        prevR->m_nItemCount < ItemBlock::BlockSize)
    {
        newBlock = prevR;
        newOffset = prevR->m_nItemCount;
        MoveItems(pBlock, offset, 1, newBlock, newOffset, -prevR->m_nItemCount);
        MoveItems(pBlock, 1, pBlock->m_nItemCount, pBlock, 0, +1);
    }

    // if we're realizing the rightmost item and there's room in the
    // next block, move it there
    else if (offset == pBlock->m_nItemCount - 1 &&
        (nextR = pBlock->m_pNextBlock->AsRealizedBlock()) != NULL &&
        nextR->m_nItemCount < ItemBlock::BlockSize)
    {
        newBlock = nextR;
        newOffset = 0;
        MoveItems(newBlock, 0, newBlock->m_nItemCount, newBlock, 1, -1);
        MoveItems(pBlock, offset, 1, newBlock, newOffset, offset);
    }

    // otherwise we need a new block for the target item
    else
    {
        newBlock = new RealizedItemBlock();

        newOffset = 0;

        // if target is leftmost item, insert it before remaining items
        if (offset == 0)
        {
            newBlock->InsertBefore(pBlock);
            MoveItems(pBlock, offset, 1, newBlock, newOffset, 0);
            MoveItems(pBlock, 1, pBlock->m_nItemCount, pBlock, 0, +1);
        }

        // if target is rightmost item, insert it after remaining items
        else if (offset == pBlock->m_nItemCount - 1)
        {
            newBlock->InsertAfter(pBlock);
            MoveItems(pBlock, offset, 1, newBlock, newOffset, offset);
        }

        // otherwise split the block into two, with the target in the middle
        else
        {
            UnrealizedItemBlock* newUBlock = new UnrealizedItemBlock();

            newUBlock->InsertAfter(pBlock);
            newBlock->InsertAfter(pBlock);
            MoveItems(pBlock, offset + 1, pBlock->m_nItemCount - offset - 1, newUBlock, 0, offset + 1);
            MoveItems(pBlock, offset, 1, newBlock, 0, offset);
        }
    }

    RemoveAndCoalesceBlocksIfNeeded(pBlock);

    // add the new target to the map
    newBlock->RealizeItem(newOffset, pItem, pContainer);

    RRETURN(hr);//RRETURN_REMOVAL
}

void
ItemContainerGenerator::RemoveAndCoalesceBlocksIfNeeded(
    _In_ ItemBlock* pBlock)
{
    if (pBlock != NULL && pBlock != m_pItemMap && pBlock->m_nItemCount == 0)
    {
        // coalesce adjacent unrealized blocks
        if (pBlock->m_pPrevBlock->AsUnrealizedBlock() != NULL && pBlock->m_pNextBlock->AsUnrealizedBlock() != NULL)
        {
            MoveItems(pBlock->m_pNextBlock, 0, pBlock->m_pNextBlock->m_nItemCount, pBlock->m_pPrevBlock, pBlock->m_pPrevBlock->m_nItemCount, -pBlock->m_pPrevBlock->m_nItemCount - 1);
            pBlock->m_pNextBlock->Remove();
        }

        pBlock->Remove();
    }
}

// Move 'count' items starting at position 'offset' in block 'block'
// to position 'newOffset' in block 'newBlock'.  The difference between
// the cumulative item counts of newBlock and block is given by 'deltaCount'.
void
ItemContainerGenerator::MoveItems(
    _In_ ItemBlock* pBlock,
    _In_ INT offset,
    _In_ INT count,
    _In_ ItemBlock* pNewBlock,
    _In_ INT newOffset,
    _In_ INT deltaCount)
{
    RealizedItemBlock* ribSrc = pBlock->AsRealizedBlock();
    RealizedItemBlock* ribDst = pNewBlock->AsRealizedBlock();

    // when both blocks are Realized, entries must be physically copied
    if (ribSrc != NULL && ribDst != NULL)
    {
        ribDst->CopyEntries(ribSrc, offset, count, newOffset);
    }
    // when the source block is Realized, clear the vacated entries -
    // to avoid leaks.  (No need if it's now empty - the block will get GC'd).
    else if (ribSrc != NULL && ribSrc->m_nItemCount > count)
    {
        ribSrc->ClearEntries(offset, count);
    }

    // update block information
    pBlock->m_nItemCount -= count;
    pNewBlock->m_nItemCount += count;

    // tell generators what happened
    if (m_pGenerator != NULL)
    {
        m_pGenerator->OnMapChanged(pBlock, offset, count, pNewBlock, newOffset, deltaCount);
    }
}

// create a group item for the given group
_Check_return_
HRESULT
ItemContainerGenerator::ContainerForGroup(
    _In_ ICollectionViewGroup* pGroup,
    _Outptr_ xaml::IDependencyObject** ppContainer)
{
    HRESULT hr = S_OK;
    BOOLEAN shouldHide = FALSE;
    ctl::ComPtr<GroupItem> spGroupItem;
    ctl::ComPtr<ItemContainerGenerator> spGenerator;
    ctl::ComPtr<TrackerCollection<xaml::DependencyObject*>> spRecyclableContainers;

    IFCPTR(ppContainer);
    *ppContainer = NULL;

    IFC(ShouldHide(pGroup, shouldHide));

    // normal group - link a new GroupItem]
    IFC(ctl::make(&spGroupItem));

    IFC(LinkContainerToItem(spGroupItem.Get(), pGroup));

    // create the generator
    IFC(ctl::make(&spRecyclableContainers));
    IFC(ItemContainerGenerator::CreateGenerator(this, spGroupItem.Get(), spRecyclableContainers.Get(), &spGenerator));
    spGroupItem->SetGenerator(spGenerator.Get());

    if (shouldHide)
    {
        IFC(spGroupItem->Hide());
    }

    IFC(spGroupItem.CopyTo(ppContainer));

Cleanup:
    RRETURN(hr);
}

// create a group item for the given group using recycled group item if possible
_Check_return_
HRESULT
ItemContainerGenerator::ContainerForGroup(
    _In_ ICollectionViewGroup* pGroup,
    _In_opt_ xaml::IDependencyObject* pRecycledContainer,
    _Outptr_ xaml::IDependencyObject** ppContainer)
{
    HRESULT hr = S_OK;
    BOOLEAN shouldHide = FALSE;
    ctl::ComPtr<GroupItem> spGroupItem;

    *ppContainer = NULL;

    IFC(ShouldHide(pGroup, shouldHide));

    // normal group - link a new GroupItem
    if (!pRecycledContainer)
    {
        IFC(ctl::make<GroupItem>(&spGroupItem));
    }
    else
    {
        ctl::ComPtr<IGroupItem> spAbstractItem;
        IFC(ctl::do_query_interface<IGroupItem>(spAbstractItem, pRecycledContainer));
        spGroupItem = spAbstractItem.Cast<GroupItem>();
    }

    IFC(LinkContainerToItem(spGroupItem.Get(), pGroup));

    // create the generator
    if (!pRecycledContainer)
    {
        ctl::ComPtr<ItemContainerGenerator> spGenerator;
        if (!m_tpSharedRecyclableContainers)
        {
            ctl::ComPtr<TrackerCollection<xaml::DependencyObject*>> spSharedRecyclableContainers;
            IFC(ctl::make(&spSharedRecyclableContainers));
            SetPtrValue(m_tpSharedRecyclableContainers, spSharedRecyclableContainers);
        }
        IFC(ItemContainerGenerator::CreateGenerator(this, spGroupItem.Get(), m_tpSharedRecyclableContainers.Get(), &spGenerator));
        spGroupItem->SetGenerator(spGenerator.Get());
    }
    else
    {
        IFCCHECK(spGroupItem->m_tpGenerator);
        IFC(spGroupItem->m_tpGenerator.Cast<ItemContainerGenerator>()->Refresh());
    }

    if (shouldHide)
    {
        IFC(spGroupItem->Hide());
    }

    IFC(spGroupItem.CopyTo(ppContainer));

Cleanup:
    RRETURN(hr);
}

_Check_return_
HRESULT
ItemContainerGenerator::GetBlockAndPosition(
    _In_ UINT itemIndex,
    _Out_ xaml_primitives::GeneratorPosition& position,
    _Out_ ItemBlock*& pItemBlock,
    _Out_ INT& offsetFromBlockStart)
{
    INT containerIndex = 0;

    pItemBlock = NULL;
    offsetFromBlockStart = itemIndex;
    position.Index = -1;
    position.Offset = 0;

    for (pItemBlock = m_pItemMap->m_pNextBlock; pItemBlock != m_pItemMap; pItemBlock = pItemBlock->m_pNextBlock)
    {
        if (offsetFromBlockStart >= pItemBlock->m_nItemCount)
        {
            // item belongs to a later block, increment the containerIndex
            containerIndex += pItemBlock->get_ContainerCount();
            offsetFromBlockStart -=  pItemBlock->m_nItemCount;
        }
        else
        {
            // item belongs to this block.  Determine the container index and offset
            if (pItemBlock->get_ContainerCount() > 0)
            {
                // block has realized items
                position.Index = containerIndex + offsetFromBlockStart;
                position.Offset = 0;
            }
            else
            {
                // block has unrealized items
                position.Index = containerIndex - 1;
                position.Offset = offsetFromBlockStart + 1;
            }

            break;
        }
    }

    if (pItemBlock == m_pItemMap)
    {
        // itemIndex out of the range of items.
        IFC_RETURN(E_INVALIDARG);
    }

    return S_OK;
}

// prepare the grouping information.  Called from RemoveAll.
_Check_return_
HRESULT
ItemContainerGenerator::PrepareGrouping()
{
    HRESULT hr = S_OK;
    ctl::ComPtr<IGeneratorHost> spHost;
    ctl::ComPtr<IGroupStyle> spGroupStyle;
    ctl::ComPtr<wfc::IVector<IInspectable*>> spView;
    ctl::ComPtr<wfc::IObservableVector<IInspectable*>> spObservableItems;
    ctl::ComPtr<wfc::IVector<IInspectable*>> spHostView;

    IFC(m_wrHost.As(&spHost));
    IFCEXPECT(spHost);

    IFC(spHost->get_View(&spView));
    spHostView = spView;

    if (m_nLevel == 0)
    {
        BOOLEAN isGrouping = FALSE;

        IFC(spHost->GetGroupStyle(NULL, m_nLevel, &spGroupStyle));

        if (spGroupStyle)
        {
            ctl::ComPtr<ICollectionView> spCollectionView;

            IFC(spHost->get_CollectionView(&spCollectionView));
            if (spCollectionView)
            {
                IFC(spCollectionView->get_CollectionGroups(&spObservableItems));
                if (spObservableItems) // if collection groups are null we default to no grouping even if the GroupStyle is set.
                {
                    IFC(spObservableItems.As(&spView));
                    isGrouping = TRUE;
                }
            }
        }

        IFC(spHost->SetIsGrouping(isGrouping));
    }
    else
    {
        ctl::ComPtr<IInspectable> spItem;
        ctl::ComPtr<IGroupItem> spGroupItem;
        ctl::ComPtr<ICollectionViewGroup> spGroup;

        IFC(m_wrPeer.As(&spGroupItem));

        IFC(spGroupItem.Cast<GroupItem>()->ReadLocalValue(
            MetadataAPI::GetDependencyPropertyByIndex(KnownPropertyIndex::ItemContainerGenerator_ItemForItemContainer),
            &spItem));
        spGroup = spItem.AsOrNull<ICollectionViewGroup>();

        if (spGroup)
        {
            IFC(spHost->GetGroupStyle(spGroup.Get(), m_nLevel, &spGroupStyle));
            IFC(spGroup->get_GroupItems(&spObservableItems));
            IFC(spObservableItems.As(&spView));
            spHostView = spView;
        }
    }

    if (m_tpObservableItemsSource.Get() != spObservableItems.Get())
    {
        if (m_tpObservableItemsSource)
        {
            IFC(m_tpObservableItemsSource->remove_VectorChanged(m_ObservableItemsSourceChangedToken));
        }

        m_tpObservableItemsSource.Set(spObservableItems);

        if (m_tpObservableItemsSource)
        {
            ctl::ComPtr<wfc::VectorChangedEventHandler<IInspectable*>> spItemCollectionVectorChangedHandler;

            spItemCollectionVectorChangedHandler.Attach(
                new ClassMemberEventHandler<
                    ItemContainerGenerator,
                    IItemContainerGenerator,
                    wfc::VectorChangedEventHandler<IInspectable*>,
                    wfc::IObservableVector<IInspectable*>,
                    wfc::IVectorChangedEventArgs>(this, &ItemContainerGenerator::OnItemCollectionChanged));

            IFC(m_tpObservableItemsSource->add_VectorChanged(spItemCollectionVectorChangedHandler.Get(), &m_ObservableItemsSourceChangedToken));
        }
    }

    if (m_tpGroupStyle.Get() != spGroupStyle.Get())
    {
        if (m_tpGroupStyle)
        {
            IFC(m_tpGroupStyle.Cast<GroupStyle>()->remove_PropertyChanged(m_GroupStyleChangedToken));
        }

        SetPtrValue(m_tpGroupStyle, spGroupStyle);

        if (m_tpGroupStyle)
        {
            ctl::ComPtr<IPropertyChangedEventHandler> spGroupStyleChangedHandler;

            spGroupStyleChangedHandler.Attach(
                new ClassMemberEventHandler<
                    ItemContainerGenerator,
                    IItemContainerGenerator,
                    IPropertyChangedEventHandler,
                    IInspectable,
                    IPropertyChangedEventArgs>(this, &ItemContainerGenerator::OnGroupStylePropertyChanged));

            IFC(m_tpGroupStyle.Cast<GroupStyle>()->add_PropertyChanged(spGroupStyleChangedHandler.Get(), &m_GroupStyleChangedToken));
        }
    }

    m_bIsGrouping = (spView != spHostView);

    SetPtrValue(m_tpView, spView);

Cleanup:
    RRETURN(hr);
}

// should the given group be hidden?
_Check_return_
HRESULT
ItemContainerGenerator::ShouldHide(
    _In_ xaml_data::ICollectionViewGroup* pGroup,
    _Out_ BOOLEAN& returnValue)
{
    HRESULT hr = S_OK;
    UINT nItemsCount = 0;

    IFCEXPECT(m_tpGroupStyle);
    IFC(m_tpGroupStyle.Cast<GroupStyle>()->get_HidesIfEmpty(&returnValue)); // user asked to hide
    if (returnValue)
    {
        ctl::ComPtr<wfc::IObservableVector<IInspectable*>> spGroupItems;
        ctl::ComPtr<wfc::IVector<IInspectable*>> spGroupItemsVector;

        IFC(pGroup->get_GroupItems(&spGroupItems));
        IFC(spGroupItems.As(&spGroupItemsVector));
        IFC(spGroupItemsVector->get_Size(&nItemsCount));
        returnValue = nItemsCount == 0; // group is empty
    }

Cleanup:
    RRETURN(hr);
}

// Called when any property of GroupStyle changes.
_Check_return_
HRESULT
ItemContainerGenerator::OnGroupStylePropertyChanged(
    _In_ IInspectable* pSender,
    _In_ IPropertyChangedEventArgs* e)
{
    RRETURN(OnRefresh());
}

// establish the link from the container to the corresponding item
_Check_return_
HRESULT
ItemContainerGenerator::LinkContainerToItem(
    _In_ xaml::IDependencyObject* pContainer,
    _In_ IInspectable* pItem)
{
    HRESULT hr = S_OK;
    ctl::ComPtr<xaml::IDependencyObject> spDOInterface;
    ctl::ComPtr<xaml::IFrameworkElement> spContainerAsFE;

    IFCPTR(pContainer);

    // always set the ItemForItemContainer property
    IFC(static_cast<DependencyObject*>(pContainer)->SetValueInternal(
        MetadataAPI::GetDependencyPropertyByIndex(KnownPropertyIndex::ItemContainerGenerator_ItemForItemContainer), pItem, /* fAllowReadOnly */ TRUE));


    IGNOREHR(ctl::do_query_interface(spDOInterface, pItem));
    if (spDOInterface.Get() == pContainer)
    {
        goto Cleanup;
    }

    // for non-direct items, set the DataContext property
    IFC(ctl::do_query_interface(spContainerAsFE, pContainer));
    IFC(spContainerAsFE->put_DataContext(pItem));

Cleanup:
    RRETURN(hr);
}

// unlink the container from the corresponding item
_Check_return_
HRESULT
ItemContainerGenerator::UnlinkContainerFromItem(
    _In_ xaml::IDependencyObject* pContainer,
    _In_ BOOLEAN isRecycling,
    _In_ BOOLEAN isFinishingDeferredAction)
{
    HRESULT hr = S_OK;

    IFCPTR(pContainer);

    // If we're part of a graph that got GC'ed, then don't worry about doing the following
    // cleanup. All of these objects will be removed from memory soon anyway.
    {
        auto peggedContainer = ctl::try_make_autopeg(static_cast<DependencyObject*>(pContainer));

        if (peggedContainer)
        {
            bool isTransitioning = false;
            ctl::ComPtr<IInspectable> spItem;
            ctl::ComPtr<UIElement> spContainerAsUI;
            ctl::ComPtr<xaml::IUIElement> spContainerAsIUI;
            const CDependencyProperty* pItemForItemContainerProperty = MetadataAPI::GetDependencyPropertyByIndex(KnownPropertyIndex::ItemContainerGenerator_ItemForItemContainer);
            TransitionTrigger trigger = TransitionTrigger::Layout;
            ctl::ComPtr<DeferredGeneratorUnlinkActionPayload> spPayload;

            IGNOREHR(ctl::do_query_interface(spContainerAsIUI, pContainer));
            spContainerAsUI = spContainerAsIUI.Cast<UIElement>();

            IFC(static_cast<DependencyObject*>(pContainer)->GetValue(pItemForItemContainerProperty, &spItem));

            if (spContainerAsUI->GetHandle() != NULL) // Note: GetHandle can return NULL during shutdown.
            {
                IFC(CoreImports::UIElement_GetHasTransition(static_cast<CUIElement*>(spContainerAsUI->GetHandle()), &isTransitioning, &trigger));
            }

            // only defer if we are unloading (recycling will have cancelled all other types)
            if (!isFinishingDeferredAction && isTransitioning && trigger == TransitionTrigger::Unload)
            {
                IFC(ctl::make(&spPayload));
                spPayload->InitializePayload(this, isRecycling);

                IFC(static_cast<DependencyObject*>(pContainer)->SetValueInternal(
                    MetadataAPI::GetDependencyPropertyByIndex(KnownPropertyIndex::ItemContainerGenerator_DeferredUnlinkingPayload), spPayload.Get(), /* fAllowReadOnly */ TRUE));

                IFC(CoreImports::UIElement_SetDeferUnlinkContainer(static_cast<CUIElement*>(static_cast<UIElement*>(pContainer)->GetHandle())));
            }
            else
            {
                ctl::ComPtr<IGeneratorHost> spHost;

                IFC(static_cast<DependencyObject*>(pContainer)->ClearValue(pItemForItemContainerProperty));

                if (SUCCEEDED(m_wrHost.As(&spHost)) && (spHost))
                {
                    IFC(spHost->ClearContainerForItem(pContainer, spItem.Get()));
                }
            }

            if (spContainerAsUI)
            {
                spContainerAsUI->SetIsLocationValid(false);
            }
        }
    }

Cleanup:
    RRETURN(hr);
}

// Called  when items collection changes, before Selector is called.
// This method won't be called for pure ItemsControl which is template part of GroupItem
_Check_return_
HRESULT
ItemContainerGenerator::NotifyOfSourceChanged(
    _In_ wfc::IObservableVector<IInspectable*>* pSender,
    _In_ wfc::IVectorChangedEventArgs* e)
{
    HRESULT hr = S_OK;
    wfc::CollectionChange action;
    UINT nIndex;

    IFCPTR(pSender);
    IFCPTR(e);
    IFC(e->get_CollectionChange(&action));

    // if we get reset event from parent collection we should handle it to refresh items in the group.
    if(wfc::CollectionChange_Reset != action && m_bIsGrouping && m_tpObservableItemsSource.Get() != pSender)
    {
        goto Cleanup;
    }

    switch (action)
    {
        case wfc::CollectionChange_ItemInserted:

            // Do stage 0 here so that it happens before the rest of Selector::NotifyOfSourceChanged().
            // This is only necessary when not grouping.
            // When we recycle GroupItem its generator will still get NotifyOfSourceChanged call from
            // collection which we used to listen. In ItemsControl we are no disconnecting from that collection due to
            // performance reasons but we are disconnecting from it in ItemContainerGenerator, thus m_tpObservableItemsSource
            // after disconnection is NULL.
            //
            // We know that in grouping scenario we have just level 0 generator so for non-grouping scenario we still call OnItemAdd.
            // For grouping scenarios m_tpObservableItemsSource will be not null for main collection - collection of groups,
            // m_tpObservableItemsSource will be not null for inner collections - items of group, but it will be null as we recycle it.
            // Thus checking m_tpObservableItemsSource is not always sufficient.
            if (!m_tpObservableItemsSource && m_nLevel == 0)
            {
                IFC(e->get_Index(&nIndex));
                // Do "stage 0" of item insertion now, to insure that the generator has a slot for the new
                // item before the rest of Selector::NotifyOfSourceChanged() runs.
                IFC(OnItemAdded(nIndex));
            }

            break;

        case wfc::CollectionChange_ItemRemoved:
            // nothing in stage 0; OnItemRemoved() happens in stage 1.
            break;

        case wfc::CollectionChange_ItemChanged:
            // nothing in stage 0; OnItemReplaced() happens in stage 1.
            break;

        case wfc::CollectionChange_Reset:
            // nothing in stage 0; OnRefresh() happens in stage 1.
            break;

        default:
            // Impossible to get here:
            __assume(0);
    }

Cleanup:
    RRETURN(hr);
}

// Called  when items collection changes, after Selector is called.
_Check_return_
HRESULT
ItemContainerGenerator::OnItemCollectionChanged(
    _In_ wfc::IObservableVector<IInspectable*>* pSender,
    _In_ wfc::IVectorChangedEventArgs* e)
{
    HRESULT hr = S_OK;
    wfc::CollectionChange action;
    UINT nIndex = 0;

    IFCPTR(pSender);
    IFCPTR(e);
    IFC(e->get_CollectionChange(&action));

    // if we get reset event from parent collection we should handle it to refresh items in the group.
    if(wfc::CollectionChange_Reset != action && m_bIsGrouping && m_tpObservableItemsSource.Get() != pSender)
    {
        goto Cleanup;
    }

    switch (action)
    {
        case wfc::CollectionChange_ItemInserted:

            // In grouping mode, stage 0 won't have happened yet, so do stage 0 here.
            if (m_PositionFromNotify.Index == -1 && m_PositionFromNotify.Offset == -1)
            {
                // Do "stage 0" here (instead of in NotifyOfSourceChanged(), which never got called).

                IFC(e->get_Index(&nIndex));
                IFC(OnItemAdded(nIndex));
            }

            // Most of CollectionChange_ItemInserted happened in "stage 0".

            // By this point, "stage 0" must have filled out m_PositionFromNotify, which means it
            // isn't equal to the "not in use" special value.
            ASSERT(!(m_PositionFromNotify.Index == -1 && m_PositionFromNotify.Offset == -1));

            // Tell layout what happened.  This is "stage 1".
            IFC(RaiseItemsChanged(action, m_PositionFromNotify, 1, 0));

            // Now m_PositionFromNotify is no longer in use, so set it back to the
            // "not in use" special value.
            m_PositionFromNotify.Index = -1;
            m_PositionFromNotify.Offset = -1;

            break;

        case wfc::CollectionChange_ItemRemoved:

            // Most of CollectionChange_ItemRemoved happens here (not in stage 0).

            IFC(e->get_Index(&nIndex));
            IFC(OnItemRemoved(nIndex));

            break;

        case wfc::CollectionChange_ItemChanged:

            // Most of CollectionChange_ItemChanged happens here (not in stage 0).

            IFC(e->get_Index(&nIndex));
            IFC(OnItemReplaced(nIndex));

            break;

        case wfc::CollectionChange_Reset:

            // Most of CollectionChange_Reset happens here (not in stage 0).

            IFC(OnRefresh());
            IFC(HideGroupItemIfEmpty());

            break;

        default:
            // impossible to get here:
            __assume(0);
    }

Cleanup:
    RRETURN(hr);
}

_Check_return_
HRESULT
ItemContainerGenerator::OnItemAdded(_In_ UINT nIndex)
{
    HRESULT hr = S_OK;
    xaml_primitives::GeneratorPosition position = {-1, 0};

    ItemBlock* pBlock = m_pItemMap->m_pNextBlock;
    int offset = nIndex;
    while (pBlock != m_pItemMap && offset >= pBlock->m_nItemCount)
    {
        offset -= pBlock->m_nItemCount;
        position.Index += pBlock->get_ContainerCount();
        pBlock = pBlock->m_pNextBlock;
    }

    position.Offset = offset + 1;

    // if it's an unrealized block, add the item by bumping the count
    UnrealizedItemBlock* uib = pBlock->AsUnrealizedBlock();
    if (uib != NULL)
    {
        MoveItems(uib, offset, 1, uib, offset + 1, 0);
        ++uib->m_nItemCount;
    }

    // if the item can be added to a previous unrealized block, do so
    else if ((offset == 0 || pBlock == m_pItemMap) &&
        ((uib = pBlock->m_pPrevBlock->AsUnrealizedBlock()) != NULL))
    {
        position.Offset = ++uib->m_nItemCount;
    }

    // otherwise, create a new unrealized block
    else
    {
        uib = new UnrealizedItemBlock();
        uib->m_nItemCount = 1;

        // split the current realized block, if necessary
        RealizedItemBlock* rib = NULL;
        if (offset > 0 && (rib = pBlock->AsRealizedBlock()) != NULL)
        {
            RealizedItemBlock* newBlock = new RealizedItemBlock();

            MoveItems(rib, offset, rib->m_nItemCount - offset, newBlock, 0, offset);
            newBlock->InsertAfter(rib);
            position.Index += pBlock->get_ContainerCount();
            position.Offset = 1;
            pBlock = newBlock;
        }

        uib->InsertBefore(pBlock);
    }

    // tell generators what happened
    if (m_pGenerator != NULL)
    {
        m_pGenerator->OnMapChanged(NULL, nIndex, +1, NULL, 0, 0);
    }

    // Remember the position from "stage 0" for use by "stage 1", and assert that
    // "stage 0" and "stage 1" of item insertion happen in pairs like they should.
    ASSERT(m_PositionFromNotify.Index == -1 && m_PositionFromNotify.Offset == -1);
    m_PositionFromNotify = position;
    ASSERT(!(m_PositionFromNotify.Index == -1 && m_PositionFromNotify.Offset == -1));

    RRETURN(hr);//RRETURN_REMOVAL
}

_Check_return_
HRESULT
ItemContainerGenerator::OnItemRemoved(_In_ UINT nIndex)
{
    xaml_primitives::GeneratorPosition position = {nIndex, 0};
    ctl::ComPtr<xaml::IDependencyObject> spContainer;    // the corresponding container
    int containerCount = 0;

    // search for the deleted item
    ItemBlock* pBlock = nullptr;
    INT offsetFromBlockStart = 0;
    IFC_RETURN(GetBlockAndPosition(nIndex, position, pBlock, offsetFromBlockStart));

    RealizedItemBlock* rib = pBlock->AsRealizedBlock();
    if (rib)
    {
        containerCount = 1;
        IFC_RETURN(rib->get_ContainerAt(offsetFromBlockStart, &spContainer));
    }

    // remove the item, and remove the block if it's now empty
    if( rib && rib->m_nItemCount-1 == offsetFromBlockStart )
    {
        // There's no entries in the block behind this one with which to overwrite it.
        // Instead, we just need to clear it.
        rib->ClearEntries(offsetFromBlockStart, 1);
    }
    else
    {
        MoveItems(pBlock, offsetFromBlockStart + 1, pBlock->m_nItemCount - offsetFromBlockStart - 1, pBlock, offsetFromBlockStart, 0);
    }
    --pBlock->m_nItemCount;

    RemoveAndCoalesceBlocksIfNeeded(pBlock);

    // tell generators what happened
    if (m_pGenerator != nullptr)
    {
        m_pGenerator->OnMapChanged(nullptr, nIndex, -1, nullptr, 0, 0);
    }

    // tell layout what happened
    IFC_RETURN(RaiseItemsChanged(wfc::CollectionChange_ItemRemoved, position, 1, containerCount));

    // unhook the container.  Do this after layout has (presumably) removed it from
    // the UI, so that it doesn't inherit DataContext falsely.
    if (spContainer)
    {
        IFC_RETURN(UnlinkContainerFromItem(spContainer.Get(), FALSE));
    }

    // Check if a group has become empty and notify the parent if so, so it can be hidden.
    IFC_RETURN(HideGroupItemIfEmpty());

    return S_OK;
}

_Check_return_
HRESULT
ItemContainerGenerator::OnItemReplaced(_In_ UINT nIndex)
{
    xaml_primitives::GeneratorPosition position = {nIndex, 0};

    // search for the replaced item
    ItemBlock* pBlock = nullptr;
    INT offsetFromBlockStart = 0;
    IFC_RETURN(GetBlockAndPosition(nIndex, position, pBlock, offsetFromBlockStart));

    // If the item is in an UnrealizedItemBlock, then this change need not
    // be made to the _itemsMap as we are replacing an unrealized item with another unrealized
    // item in the same place.
    RealizedItemBlock* rib = pBlock->AsRealizedBlock();
    if (rib)
    {
        ctl::ComPtr<IGeneratorHost> spHost;
        ctl::ComPtr<wfc::IVector<IInspectable*>> spView;
        ctl::ComPtr<IInspectable> spNewItem;
        ctl::ComPtr<xaml::IDependencyObject> spOldContainer;
        ctl::ComPtr<xaml_primitives::ISelectorItem> spOldContainerAsSelectorItem;
        BOOLEAN oldContainerIsPlaceholder = FALSE;

        IFC_RETURN(m_wrHost.As(&spHost));
        IFCEXPECT_RETURN(spHost);

        IFC_RETURN(get_View(&spView));
        IFC_RETURN(spView->GetAt(nIndex, &spNewItem));

        IFC_RETURN(rib->get_ContainerAt(offsetFromBlockStart, &spOldContainer));

        // behavior of replacing is to remove the old container and
        // insert a new container.
        // However, if the current container is a placeholder, we should be able
        // to re-use it. This is kind of a problem, because now we forego the
        // possibility of choosing a different container based on the item that
        // is being set. However, performance wise this is the only option that we should
        // offer.

        // find if the current container is a placeholder
        // this is marked on the selectorItem class. We should consider moving this to a much much
        // lower type, something like FE or UIE even.
        // Not only would that break this weird dependency, but it would open up the
        // coupling of selectoritem to placeholder functionality.
        spOldContainerAsSelectorItem = spOldContainer.AsOrNull<ISelectorItem>();
        if (spOldContainerAsSelectorItem)
        {
            oldContainerIsPlaceholder = spOldContainerAsSelectorItem.Cast<SelectorItem>()->GetIsPlaceholder();
        }

        if (oldContainerIsPlaceholder)
        {
            // if we are a placeholder, we should just unlink the container from the current item
            // and link it to the new one

            IFC_RETURN(UnlinkContainerFromItem(spOldContainer.Get(), FALSE)); // todo: this actually sets the location is invalid.. which is not true
            IFC_RETURN(LinkContainerToItem(spOldContainer.Get(), spNewItem.Get()));

            IFC_RETURN(spHost->PrepareItemContainer(spOldContainer.Get(), spNewItem.Get()));
        }
        else
        {
            ctl::ComPtr<xaml::IDependencyObject> spNewContainer;
            ctl::ComPtr<ICollectionViewGroup> spGroup;

            spGroup = spNewItem.AsOrNull<ICollectionViewGroup>();
            if (!spGroup || !m_bIsGrouping)
            {
                IFC_RETURN(spHost->GetContainerForItem(spNewItem.Get(), nullptr, &spNewContainer));
                ASSERT(spNewContainer.Get());

                // replace the old item with the new one
                rib->RealizeItem(offsetFromBlockStart, spNewItem.Get(), spNewContainer.Get());

                // hook up the container to the new item
                IFC_RETURN(LinkContainerToItem(spNewContainer.Get(), spNewItem.Get()));
            }
            else
            {
                IFC_RETURN(ContainerForGroup(spGroup.Get(), &spNewContainer));
                ASSERT(spNewContainer.Get());

                // replace the old item with the new one
                rib->RealizeItem(offsetFromBlockStart, spNewItem.Get(), spNewContainer.Get());
            }

            // tell layout what happened
            IFC_RETURN(RaiseItemsChanged(wfc::CollectionChange_ItemChanged, position, 1, 1));

            IFC_RETURN(UnlinkContainerFromItem(spOldContainer.Get(), FALSE));

            IFC_RETURN(spHost->PrepareItemContainer(spNewContainer.Get(), spNewItem.Get()));
        }
    }

    return S_OK;
}

_Check_return_
HRESULT ItemContainerGenerator::HideGroupItemIfEmpty()
{
    HRESULT hr = S_OK;
    INT itemsCount = 0;

    if (m_nLevel > 0)
    {
        IFC(GetCount(itemsCount));
        if (itemsCount == 0)
        {
            ctl::ComPtr<IGroupItem> spGroupItem;

            // This may be an empty subgroup.
            IFC(m_wrPeer.As(&spGroupItem));

            if (spGroupItem)
            {
                BOOLEAN shouldHide = FALSE;
                ctl::ComPtr<IInspectable> spItem;
                ctl::ComPtr<ICollectionViewGroup> spGroup;
                ctl::ComPtr<xaml_controls::IItemContainerGenerator> spParent;

                IFC(spGroupItem.Cast<GroupItem>()->ReadLocalValue(
                    MetadataAPI::GetDependencyPropertyByIndex(KnownPropertyIndex::ItemContainerGenerator_ItemForItemContainer),
                    &spItem));
                spGroup = spItem.AsOrNull<ICollectionViewGroup>();

                IFC(m_wrParent.As(&spParent));

                // The group could be null if the parent generator has already unhooked its container.
                if (spGroup &&
                    spParent)
                {
                    IFC(spParent.Cast<ItemContainerGenerator>()->ShouldHide(spGroup.Get(), shouldHide));
                }

                if (shouldHide)
                {
                    IFC(spGroupItem.Cast<GroupItem>()->Hide());
                }
            }
        }
    }

Cleanup:
    RRETURN(hr);
}

_Check_return_ HRESULT ItemContainerGenerator::OnRefresh()
{
    HRESULT hr = S_OK;
    xaml_primitives::GeneratorPosition position = {-1, 0};

    IFC(RaiseItemsChanged(wfc::CollectionChange_Reset, position, 0, 0));
    IFC(RemoveAll());

Cleanup:
    RRETURN(hr);
}

_Check_return_
HRESULT
ItemContainerGenerator::RaiseItemsChanged(
_In_ wfc::CollectionChange action,
_In_ xaml_primitives::GeneratorPosition position,
_In_ UINT nItemsCount,
_In_ UINT nContainersCount)
{
    HRESULT hr = S_OK;
    xaml_primitives::GeneratorPosition oldPosition = { -1, 0 };
    ctl::ComPtr<ItemsChangedEventArgs> spArgs;

    if (m_spEventSource)
    {
        // Create the args
        IFC(ctl::make(&spArgs));

        IFC(spArgs->put_Action(static_cast<INT>(action)));
        IFC(spArgs->put_Position(position));
        IFC(spArgs->put_OldPosition(oldPosition));
        IFC(spArgs->put_ItemCount(nItemsCount));
        IFC(spArgs->put_ItemUICount(nContainersCount));

        // Raise the event
        IFC(m_spEventSource->Raise(ctl::as_iinspectable(this), spArgs.Get()));
    }

Cleanup:
    RRETURN(hr);
}

void ItemContainerGenerator::OnReferenceTrackerWalk(INT walkType)
{
    // Do the standard walk first.
    ItemContainerGeneratorGenerated::OnReferenceTrackerWalk(walkType);

    EReferenceTrackerWalkType walkTypeAsEnum = static_cast<EReferenceTrackerWalkType>(walkType);

    m_tpObservableItemsSource.ReferenceTrackerWalk(walkTypeAsEnum);

    // Forward walk to the event source
    if (m_spEventSource)
    {
        m_spEventSource->ReferenceTrackerWalk(walkTypeAsEnum);
    }

    // Now walk any objects not tracked by the live tree.
    ItemBlock* pItemMap = m_pItemMap;
    if (pItemMap != NULL)
    {
        do
        {
            RealizedItemBlock* pRealizedBlock = pItemMap->AsRealizedBlock();

            if (pRealizedBlock != NULL)
            {
                for (int offset = 0; offset < pRealizedBlock->get_ContainerCount(); ++offset)
                {
                    pRealizedBlock->_entry[offset]._container.ReferenceTrackerWalk(walkTypeAsEnum);
                    pRealizedBlock->_entry[offset]._item.ReferenceTrackerWalk(walkTypeAsEnum);
                }
            }

            pItemMap = pItemMap->m_pNextBlock;
        } while (pItemMap != m_pItemMap);
    }
}

void
ItemContainerGenerator::DeferredGeneratorUnlinkActionPayload::InitializePayload(_In_ ItemContainerGenerator* pGenerator, _In_ BOOLEAN isRecycling )
{
    SetPtrValue(m_tpGenerator, pGenerator);
    m_isRecycling = isRecycling;
}

_Check_return_ HRESULT
ItemContainerGenerator::DeferredGeneratorUnlinkActionPayload::Execute(_In_ UIElement* pContainer)
{
    RRETURN(m_tpGenerator->UnlinkContainerFromItem(pContainer, m_isRecycling, TRUE));
}

//static
_Check_return_ HRESULT
ItemContainerGenerator::ExecuteDeferredUnlinkAction(_In_ CUIElement* nativeTarget)
{
    HRESULT hr = S_OK;

    ctl::ComPtr<IInspectable> spPayload;
    const CDependencyProperty* pDeferredUnlinkingPayloadProperty = MetadataAPI::GetDependencyPropertyByIndex(KnownPropertyIndex::ItemContainerGenerator_DeferredUnlinkingPayload);

    ctl::ComPtr<DependencyObject> spContainer;
    IFC(DXamlCore::GetCurrent()->GetPeer(nativeTarget, &spContainer));

    IFC(spContainer->GetValue(pDeferredUnlinkingPayloadProperty, &spPayload));

    if (spPayload)
    {
        IFC(spPayload.Cast<DeferredUnlinkActionPayload>()->Execute(spContainer.Cast<UIElement>()));
    }

    IFC(spContainer->ClearValue(pDeferredUnlinkingPayloadProperty));

Cleanup:
    RRETURN(hr);
}

// Protected implementation of IItemContainerMapping
HRESULT
    ItemContainerGenerator::QueryInterfaceImpl(
    _In_ REFIID iid,
    _Outptr_ void** ppObject)
{
    if (InlineIsEqualGUID(iid, __uuidof(IItemContainerMapping)))
    {
        *ppObject = static_cast<IItemContainerMapping*>(this);
    }
    else
    {
        RRETURN(ItemContainerGeneratorGenerated::QueryInterfaceImpl(iid, ppObject));
    }

    AddRefOuter();
    RRETURN(S_OK);
}

IFACEMETHODIMP
    ItemContainerGenerator::ItemFromContainer(
    _In_ xaml::IDependencyObject* container,
    _Outptr_ IInspectable** returnValue)
{
    RRETURN(ItemContainerGeneratorGenerated::ItemFromContainer(container, returnValue));
}

IFACEMETHODIMP
    ItemContainerGenerator::ContainerFromItem(
    _In_opt_ IInspectable* item,
    _Outptr_ xaml::IDependencyObject** returnValue)
{
    RRETURN(ItemContainerGeneratorGenerated::ContainerFromItem(item, returnValue));
}

IFACEMETHODIMP
    ItemContainerGenerator::IndexFromContainer(
    _In_ xaml::IDependencyObject* container,
    _Out_ INT* returnValue)
{
    RRETURN(ItemContainerGeneratorGenerated::IndexFromContainer(container, returnValue));
}

IFACEMETHODIMP
    ItemContainerGenerator::ContainerFromIndex(
    _In_ INT index,
    _Outptr_ xaml::IDependencyObject** returnValue)
{
    RRETURN(ItemContainerGeneratorGenerated::ContainerFromIndex(index, returnValue));
}

bool
ItemContainerGenerator::IsIndexInRange(
    _In_ INT itemIndex)
{
    bool isInRange = false;

    // The requested index value must be in the range of the stored item map.
    if (itemIndex >= 0)
    {
        ItemBlock* pItemBlock = nullptr;
        INT offsetFromBlockStart = itemIndex;

        for (pItemBlock = m_pItemMap->m_pNextBlock; pItemBlock != m_pItemMap; pItemBlock = pItemBlock->m_pNextBlock)
        {
            if (offsetFromBlockStart >= pItemBlock->m_nItemCount)
            {
                // The itemIndex belongs to a later block, decrease the offsetFromBlockStart
                offsetFromBlockStart -= pItemBlock->m_nItemCount;
            }
            else
            {
                break;
            }
        }

        if (pItemBlock != m_pItemMap)
        {
            isInRange = true;
        }
    }

    return isInRange;
}
