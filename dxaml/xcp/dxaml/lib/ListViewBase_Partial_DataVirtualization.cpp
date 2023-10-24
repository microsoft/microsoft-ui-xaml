// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#include "precomp.h"
#include "ListViewBase.g.h"
#include "ScrollViewer.g.h"
#include "ItemCollection.g.h"
#include "IPaginatedPanel.g.h"
#include "IScrollInfo.g.h"
#include "ModernCollectionBasePanel.g.h"
#include "ItemIndexRange.g.h"
#include "incrementalloading.h"
#include "IncrementalLoadingAdapter.h"

using namespace DirectUI;
using namespace DirectUISynonyms;

// Called when the ScrollHost's scrolling offsets change.
// Triggers a LoadMoreItems call if necessary.
_Check_return_ HRESULT ListViewBase::ProcessDataVirtualizationScrollOffsets()
{
    HRESULT hr = S_OK;
    ctl::ComPtr<IPanel> spItemsHost;
    ctl::ComPtr<IPaginatedPanel> spPaginatedPanel;

    IFC(get_ItemsHost(&spItemsHost));
    spPaginatedPanel = spItemsHost.AsOrNull<IPaginatedPanel>();

    if (spPaginatedPanel && m_tpScrollViewer && !m_isLoadAsyncInProgress)
    {
        ctl::ComPtr<IScrollInfo> spScrollInfo;
        INT lastItemInViewport = -1;

        IFC(m_tpScrollViewer.Cast<ScrollViewer>()->get_ScrollInfo(&spScrollInfo));

        // Get the index of the last item in the viewport.
        // If we're grouping this index will be the index to the group item therefore it must be compared against
        // group counts not item count
        IFC(spPaginatedPanel->GetLastItemIndexInViewport(spScrollInfo.Get(), &lastItemInViewport));

        if (m_scrollHostOffsetChangeAction == ScrollHostOffsetChangeAction_IncrementalEdgeTrigger)
        {
            ctl::ComPtr<wfc::IObservableVector<IInspectable*>> spItems;
            UINT itemsCount = 0;
            DOUBLE pageLoadThreshold = 0;
            UINT pagesFromEnd = 0;
            BOOLEAN fIsGrouping = FALSE;
            DOUBLE itemsPerPage = 0;

            // Get the number of items per page.
            IFC(spPaginatedPanel->GetItemsPerPage(spScrollInfo.Get(), &itemsPerPage));

            // Determine if we're grouping to see what count of items we need to get
            IFC(get_IsGrouping(&fIsGrouping));

            // Check if we are within the threshold for triggering an incremental load.
            if (fIsGrouping)
            {
                // Since we're grouping we need to get the number of groups
                ctl::ComPtr<xaml_data::ICollectionView> spCollectionView;
                ctl::ComPtr<wfc::IObservableVector<IInspectable *>> spGroupsObservable;
                ctl::ComPtr<wfc::IVector<IInspectable *>> spGroups;

                IFC(get_CollectionView(&spCollectionView));
                IFCCATASTROPHIC(spCollectionView);

                IFC(spCollectionView->get_CollectionGroups(&spGroupsObservable));
                IFC(spGroupsObservable.As<wfc::IVector<IInspectable *>>(&spGroups));

                IFC(spGroups->get_Size(&itemsCount));
            }
            else
            {
                // Get the total number of data items (different from realized UI containers).
                // Since we're not grouping then the count will be the number of items
                IFC(get_Items(&spItems));
                IFC(spItems.Cast<ItemCollection>()->get_Size(&itemsCount));
            }

            // Get the increment load threshold specified in page units.
            IFC(get_IncrementalLoadingThreshold(&pageLoadThreshold));

            if (itemsPerPage > 0)
            {
                // LastItemInViewport is obtained from the panel while itemsCount is from
                // the data source. It is possible that when items are deleted the data is
                // updated making the itemsCount smaller but the panel has not gotten the notifications
                // yet, so the lastItemInViewport is still old information. In that case you could
                // end up with lastItemInViewport being greater than the itemsCount. This can only
                // happen when you are towards the end. We want pagesFromEnd=0 for that case.
                if (static_cast<INT>(itemsCount) >= lastItemInViewport + 1)
                {
                    pagesFromEnd = static_cast<UINT>((itemsCount - (lastItemInViewport + 1)) / itemsPerPage);
                }
            }

            if (pagesFromEnd <= pageLoadThreshold)
            {
                Microsoft::WRL::ComPtr<LoadMoreItemsOperation> spLoadMoreItemsOperation = NULL;

                auto p = Microsoft::WRL::Callback<wf::IAsyncOperationCompletedHandler<xaml_data::LoadMoreItemsResult>>(this, &ListViewBase::OnLoadAsyncCompleted);

                IFC(Microsoft::WRL::MakeAndInitialize<LoadMoreItemsOperation>(&spLoadMoreItemsOperation));
                IFC(spLoadMoreItemsOperation->Init(this, /*isDataProcessing*/TRUE));
                IFC(spLoadMoreItemsOperation->put_Completed(p.Get()));
                IFC(spLoadMoreItemsOperation->Start());

                // Within the threshold, trigger an incremental load.
                if (spLoadMoreItemsOperation->HasStatus())
                {
                    wf::AsyncStatus status = wf::AsyncStatus::Completed;
                    IFC(spLoadMoreItemsOperation->GetStatus(&status));
                    if (status == wf::AsyncStatus::Started)
                    {
                        m_isLoadAsyncInProgress = TRUE;
                    }
                    else
                    {
                        // Since the operation is completed we need to be done with it
                        IFC(spLoadMoreItemsOperation->Cancel());
                    }
                }
                else
                {
                    // we're not using the operation. so we need to close it
                    IFC(spLoadMoreItemsOperation->Cancel());
                }
            }
        }
    }

Cleanup:
    RRETURN(hr);
}

// Called when LoadAsync fires completed event
_Check_return_ HRESULT ListViewBase::OnLoadAsyncCompleted(wf::IAsyncOperation<xaml_data::LoadMoreItemsResult>* getOperation, wf::AsyncStatus status)
{
    HRESULT hr = S_OK;
    IFC(CheckThread());
    m_isLoadAsyncInProgress = FALSE;

    if (IsInLiveTree())
    {
        // Re-try to see if we need to load more items if the listview,
        // We do that only as long as the ListView is in the live tree.
        // If the ListView is not live it will
        // not get measured and the last visible indexes will not be updated, which
        // will end up causing a loop where we keep calling load more items repeatedly.
        IFC(ProcessDataVirtualizationScrollOffsets());
    }

Cleanup:
    RRETURN(hr);
}

// Create an IAsyncOperation to load an additional DataFetchSize number of
// items.
_Check_return_ HRESULT ListViewBase::LoadMoreItemsAsyncImpl(
    _Outptr_ wf::IAsyncOperation<xaml_data::LoadMoreItemsResult>** returnValue)
{
    HRESULT hr = S_OK;
    Microsoft::WRL::ComPtr<LoadMoreItemsOperation> spLoadMoreItemsOperation;

    IFC(Microsoft::WRL::MakeAndInitialize<LoadMoreItemsOperation>(&spLoadMoreItemsOperation));
    IFC(spLoadMoreItemsOperation->Init(this, /*isDataProcessing*/FALSE));
    IFC(spLoadMoreItemsOperation->Start());

    IFC(spLoadMoreItemsOperation.CopyTo(returnValue));

Cleanup:
    // OK to release spLoadMoreItemsOperation, object itself tracks its lifetime.
    RRETURN(hr);
}

// Internal method for starting an incremental load, used for both manual and
// automatic incremental loads.
_Check_return_ HRESULT ListViewBase::LoadMoreItemsAsyncInternal(
    _In_opt_ LoadMoreItemsOperation* pChainedAsyncOperation,
    _Outptr_ IAsyncInfo** ppAsyncInfo)
{
    ctl::ComPtr<wfc::IIterable<IInspectable*>> spItemsSource;
    ctl::ComPtr<IAsyncInfo> spAsyncInfo;

    IFC_RETURN(get_ItemsSource(&spItemsSource));
    if (spItemsSource)
    {
        ctl::ComPtr<ISupportIncrementalLoading> spIncrementalLoadingVector;

        // Check if the data source supports incremental loading.
        spIncrementalLoadingVector.Attach(IncrementalLoadingAdapter::AsIncrementalLoading(spItemsSource.Get()));
        if (spIncrementalLoadingVector)
        {
            BOOLEAN hasMoreItems = false;

            // Check if the data source has more items to load.
            IFC_RETURN(spIncrementalLoadingVector->get_HasMoreItems(&hasMoreItems));
            if (hasMoreItems)
            {
                ctl::ComPtr<IPanel> spItemsHost;
                ctl::ComPtr<IPaginatedPanel> spPaginatedPanel;

                IFC_RETURN(get_ItemsHost(&spItemsHost));
                spPaginatedPanel = spItemsHost.AsOrNull<IPaginatedPanel>();
                if (spPaginatedPanel)
                {
                    ctl::ComPtr<IScrollInfo> spScrollInfo;
                    INT itemsToLoad = 0;
                    DOUBLE pagesToLoad = 0;
                    DOUBLE itemsPerPage = 0;
                    ctl::ComPtr<wf::IAsyncOperation<xaml_data::LoadMoreItemsResult>> spAsyncOperation;
                    ctl::ComPtr<IncrementalLoadAsyncCompleteHandler> spLoadCompletedHandler;
                    ctl::ComPtr<wf::IAsyncOperationCompletedHandler<xaml_data::LoadMoreItemsResult>> spOperationCompletedHandler;

                    // Calculate the items to load via pages-to-load and items-per
                    // page values.
                    IFC_RETURN(m_tpScrollViewer.Cast<ScrollViewer>()->get_ScrollInfo(&spScrollInfo));
                    IFC_RETURN(spPaginatedPanel->GetItemsPerPage(spScrollInfo.Get(), &itemsPerPage));
                    IFC_RETURN(get_DataFetchSize(&pagesToLoad));
                    itemsToLoad = static_cast<INT>(pagesToLoad * itemsPerPage);
                    if (itemsToLoad <= 0)
                    {
                        // We know that there's data to be loaded, at least load one item
                        itemsToLoad = 1;
                    }

                    // Call out to the data source to load more items asynchronously.
                    IFC_RETURN(spIncrementalLoadingVector->LoadMoreItemsAsync(itemsToLoad, &spAsyncOperation));
                    IFCPTR_RETURN(spAsyncOperation.Get());

                    // Set up a complete handler for the data source to call on when
                    // it has completed the incremental load.
                    IFC_RETURN(ctl::make<IncrementalLoadAsyncCompleteHandler>(&spLoadCompletedHandler));
                    IFC_RETURN(spLoadCompletedHandler.As<wf::IAsyncOperationCompletedHandler<xaml_data::LoadMoreItemsResult>>(&spOperationCompletedHandler));
                    spLoadCompletedHandler->SetChainedAsyncOperation(pChainedAsyncOperation);
                    IFC_RETURN(spAsyncOperation->put_Completed(spOperationCompletedHandler.Get()));

                    // Do not start the operation yet. In case of manual incremental
                    // load, this async operation is chained with another async
                    // operation controlled by the public LoadMoreItemsAsync API. We
                    // should wait for Start to be called on that AsyncOperation.
                    IFC_RETURN(spAsyncOperation.As<IAsyncInfo>(&spAsyncInfo));
                }
            }
        }
    }

    IFC_RETURN(spAsyncInfo.CopyTo(ppAsyncInfo));

    return S_OK;
}

// overridden from ItemsControl (IGeneratorHost)
_Check_return_ IFACEMETHODIMP ListViewBase::VirtualizationFinished()
{
    return InvokeDataSourceRangesChanged();
}

// Stores a handle to the items source as an IItemsRangeInfo if possible
_Check_return_ HRESULT ListViewBase::InitializeDataSourceItemsRangeInfo()
{
    ctl::ComPtr<IInspectable> spItemsSourceAsIInspectable;

    IFC_RETURN(get_ItemsSource(&spItemsSourceAsIInspectable));
    if (spItemsSourceAsIInspectable)
    {
        auto spItemsSourceAsIRI = spItemsSourceAsIInspectable.AsOrNull<xaml_data::IItemsRangeInfo>();
        if (spItemsSourceAsIRI)
        {
            ctl::ComPtr<ItemIndexRange> spLastPassedVisibleRange;

            SetPtrValue(m_tpDataSourceAsItemsRangeInfo, spItemsSourceAsIRI);

            IFC_RETURN(ctl::make(&spLastPassedVisibleRange));
            SetPtrValue(m_tpLastPassedVisibleRange, spLastPassedVisibleRange);

            // if it was already created, just clear the collection
            if (m_tpLastPassedTrackedRanges)
            {
                IFC_RETURN(m_tpLastPassedTrackedRanges->Clear());
            }
            else
            {
                ctl::ComPtr<TrackerCollection<xaml_data::ItemIndexRange*>> spLastPassedTrackedRanges;

                IFC_RETURN(ctl::make(&spLastPassedTrackedRanges));
                SetPtrValue(m_tpLastPassedTrackedRanges, spLastPassedTrackedRanges);
            }
        }
    }

    return S_OK;
}

// Used to inform the data source of the items it is tracking
// in this function, we check if the new data is the same as the last passed ones
// we invoke if the data is different
_Check_return_ HRESULT ListViewBase::InvokeDataSourceRangesChangedHelper(
    _In_ xaml_data::IItemIndexRange* visibleRange,
    _In_ TrackerCollection<xaml_data::ItemIndexRange*>* trackedItems)
{
    bool bAreRangesEqual = false;

    // we invoke only in the case where the ranges are not identical to the ones we already passed
    // first we test the visible range
    IFC_RETURN(ItemIndexRange::AreItemIndexRangesEqual(m_tpLastPassedVisibleRange.Get(), visibleRange, &bAreRangesEqual));

    // if the above returned true, then we test the tracked items collection
    // otherwise, no need since we know that we have to invoke the RangesChanged regardless
    if (bAreRangesEqual)
    {
        IFC_RETURN(ItemIndexRange::AreItemIndexRangeCollectionsEqual(m_tpLastPassedTrackedRanges.Get(), trackedItems, &bAreRangesEqual));
    }

    if (!bAreRangesEqual)
    {
        ctl::ComPtr<wfc::IVectorView<xaml_data::ItemIndexRange*>> spTrackedItemsAsIVV;

        // getting the VectorView from the TrackerCollection
        IFC_RETURN(trackedItems->GetView(spTrackedItemsAsIVV.GetAddressOf()));

        // invoking the RangesChanged method
        IFC_RETURN(m_tpDataSourceAsItemsRangeInfo->RangesChanged(visibleRange, spTrackedItemsAsIVV.Get()));

        // saving the new ranges
        SetPtrValue(m_tpLastPassedVisibleRange, visibleRange);
        SetPtrValue(m_tpLastPassedTrackedRanges, trackedItems);
    }

    return S_OK;
}

// Used to inform the data source of the items it is tracking
// in this function, we collect the data
_Check_return_ HRESULT ListViewBase::InvokeDataSourceRangesChanged()
{
    if (m_tpDataSourceAsItemsRangeInfo)
    {
        ctl::ComPtr<IPanel> spItemsPanelRoot;

        IFC_RETURN(get_ItemsPanelRoot(&spItemsPanelRoot));
        if (spItemsPanelRoot)
        {
            ctl::ComPtr<IModernCollectionBasePanel> spIModernCollectionBasePanel;

            spIModernCollectionBasePanel = spItemsPanelRoot.AsOrNull<IModernCollectionBasePanel>();
            if (spIModernCollectionBasePanel)
            {
                INT firstVisibleIndex = -1;

                // get the data from the panel
                IFC_RETURN(spIModernCollectionBasePanel.Cast<ModernCollectionBasePanel>()->get_FirstVisibleIndexBase(&firstVisibleIndex));

                if (firstVisibleIndex != -1)
                {
                    INT lastVisibleIndex = -1;
                    INT firstCachedIndex = -1;
                    INT lastCachedIndex = -1;
                    std::vector<unsigned int> pinnedElementsIndices;
                    ctl::ComPtr<ItemIndexRange> spVisibleRange;
                    ctl::ComPtr<TrackerCollection<xaml_data::ItemIndexRange*>> spTrackedItemsAsTC;

                    // get the data from the panel
                    IFC_RETURN(spIModernCollectionBasePanel.Cast<ModernCollectionBasePanel>()->get_LastVisibleIndexBase(&lastVisibleIndex));
                    IFC_RETURN(spIModernCollectionBasePanel.Cast<ModernCollectionBasePanel>()->get_FirstCacheIndexBase(&firstCachedIndex));
                    IFC_RETURN(spIModernCollectionBasePanel.Cast<ModernCollectionBasePanel>()->get_LastCacheIndexBase(&lastCachedIndex));
                    IFC_RETURN(spIModernCollectionBasePanel.Cast<ModernCollectionBasePanel>()->GetPinnedElementsIndexVector(xaml_controls::ElementType_ItemContainer, &pinnedElementsIndices));

                    // visible range
                    IFC_RETURN(ctl::make(&spVisibleRange));
                    IFC_RETURN(spVisibleRange->put_FirstIndex(firstVisibleIndex));
                    IFC_RETURN(spVisibleRange->put_Length(lastVisibleIndex - firstVisibleIndex + 1));

                    // create the tracked items collection
                    IFC_RETURN(ctl::make(&spTrackedItemsAsTC));

                    // cached range (includes visible range)
                    IFC_RETURN(ItemIndexRange::AppendItemIndexRangeToItemIndexRangeCollection(firstCachedIndex, lastCachedIndex - firstCachedIndex + 1, spTrackedItemsAsTC.Get()));

                    // pinned elements
                    if (pinnedElementsIndices.size() > 0)
                    {
                        // sorting the elements in ascending order
                        std::sort(pinnedElementsIndices.begin(), pinnedElementsIndices.end());

                        // creating ranges from the indices and appending them to the tracked items
                        IFC_RETURN(ItemIndexRange::AppendItemIndexRangesFromSortedVectorToItemIndexRangeCollection(pinnedElementsIndices, spTrackedItemsAsTC.Get()));
                    }

                    // focused item range
                    IFC_RETURN(ItemIndexRange::AppendItemIndexRangeToItemIndexRangeCollection(m_lastFocusedIndex, 1, spTrackedItemsAsTC.Get()));

                    // compare the new ranges with the passed ones and invoke
                    IFC_RETURN(InvokeDataSourceRangesChangedHelper(spVisibleRange.Get(), spTrackedItemsAsTC.Get()));
                }
            }
        }
    }

    return S_OK;
}
