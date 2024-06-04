// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

//  Abstract:
//      Implements an adapter that makes using an ICollectionView
//      or instance of ISupportIncrementalLoading seamless.

#pragma once

namespace DirectUI
{
    class IncrementalLoadingAdapter:
        public ctl::implements_inspectable<xaml_data::ISupportIncrementalLoading>
    {
    private:

        IncrementalLoadingAdapter(_In_ xaml_data::ICollectionView *pCV)
            : m_spCV(pCV)
        { }

    public:

        // ISupportIncrementalLoading interface
        IFACEMETHOD(get_HasMoreItems)(_Out_ BOOLEAN *value) override
        {
            RRETURN(m_spCV->get_HasMoreItems(value));
        }

        IFACEMETHOD(LoadMoreItemsAsync)(
            _In_ UINT32 count, 
            _Outptr_ wf::IAsyncOperation<xaml_data::LoadMoreItemsResult> **operation) override
        {
            RRETURN(m_spCV->LoadMoreItemsAsync(count, operation));
        }

    public:

        // Note: This method will create a new wrapper for ICollectionView everytime it is called therefore
        // you cannot use equality comparisons to find out if they represent the same object
        // TODO: If we need to compare wether two different ISupportIncrementalLoading interfaces are the same
        // that have obtained from this method then we need to do some sort of aggregation and return
        // a "cannonical" IUnknown/IInspectable for comparison.
        static xaml_data::ISupportIncrementalLoading *AsIncrementalLoading(_In_ IInspectable *pSource)
        {
            xaml_data::ISupportIncrementalLoading *pResult = NULL;
            xaml_data::ICollectionView *pCV = NULL;

            // If the source is already an incremental loading vector then we're done
            pResult = ctl::query_interface<xaml_data::ISupportIncrementalLoading>(pSource);
            if (pResult)
            {
                goto Cleanup;
            }

            // Try to see if this is a collection view and if so wrap it in the adapter
            // otherwise we're done
            pCV = ctl::query_interface<xaml_data::ICollectionView>(pSource);
            if (pCV == NULL)
            {
                goto Cleanup;
            }

            // TODO: This allocation might fail but at least we will be notified
            // of the failure, eventhough we will return NULL
            pResult = new IncrementalLoadingAdapter(pCV);

        Cleanup:

            ReleaseInterface(pCV);

            return pResult;
        }

        static bool SupportsIncrementalLoading(_In_ IInspectable *pSource)
        {
            return ctl::is<xaml_data::ISupportIncrementalLoading>(pSource) ||
                ctl::is<xaml_data::ICollectionView>(pSource);
        }

    private:

        ctl::ComPtr<xaml_data::ICollectionView> m_spCV;
    };
}