// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

//  Abstract:
//      DependencyObjectCollection is a container for DependencyObjects with
//      support for inheriting/propagating the data-context.

#pragma once

#include "DependencyObjectCollection.g.h"

namespace DirectUI
{
    PARTIAL_CLASS(DependencyObjectCollection)
    {
    public:
        // Initializes a new instance of the UIElement class.
        DependencyObjectCollection();

        IFACEMETHODIMP SetAt(
            _In_ UINT index,
            _In_opt_ xaml::IDependencyObject* item) override;

        IFACEMETHODIMP InsertAt(
            _In_ UINT index,
            _In_opt_ xaml::IDependencyObject* item) override;

        IFACEMETHODIMP RemoveAt(_In_ UINT index) override;

        IFACEMETHODIMP Append(_In_opt_ xaml::IDependencyObject* item) override;

        IFACEMETHODIMP Clear() override;

        IFACEMETHODIMP add_VectorChanged(
            _In_ wfc::VectorChangedEventHandler<xaml::DependencyObject*> *pHandler,
            _Out_ EventRegistrationToken *token) override;

        IFACEMETHODIMP remove_VectorChanged(_In_ EventRegistrationToken token) override;

        _Check_return_ HRESULT OnChildUpdated(_In_ DependencyObject *pChild) override;

        _Check_return_ HRESULT OnCollectionChanged(_In_ XUINT32 nCollectionChangeType, _In_ XUINT32 nIndex) override;

    protected:
        // Needed to walk the event source
        void OnReferenceTrackerWalk(INT walkType) final;

    private:
        _Check_return_ HRESULT RaiseVectorChanged(
            _In_ wfc::CollectionChange action,
            UINT index);

    private:
        TrackerEventSource<
            wfc::VectorChangedEventHandler<xaml::DependencyObject *>,
            wfc::IObservableVector<xaml::DependencyObject *>,
            wfc::IVectorChangedEventArgs> m_vectorChangedHandlers;
    };
}
