// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#pragma once

#include "ItemCollection.g.h"

XAML_ABI_NAMESPACE_BEGIN namespace Microsoft
{
    namespace UI
    {
        namespace Xaml
        {
            namespace Controls
            {
                // Internal IItemCollection interface so we can QI for it.
                MIDL_INTERFACE("76a9221d-4ca2-46e0-8bb5-3e45f8b4cfd7")
                IItemCollection : public IInspectable
                {
                    // empty interface. Only for safe casting.
                };
            }
        }
    }
} XAML_ABI_NAMESPACE_END

namespace DirectUI
{

    // Represents a ItemCollection.
    //
    PARTIAL_CLASS(ItemCollection)
        , public xaml_controls::IItemCollection
    {
        friend class ItemsControl;

    public:
        // IIterable
        IFACEMETHOD(First)(
            _Outptr_result_maybenull_ wfc::IIterator<IInspectable*>** first) override;

        // read methods
        IFACEMETHOD(GetAt)(
            _In_opt_ UINT index,
            _Out_ IInspectable** item) override;

        IFACEMETHOD(get_Size)(_Out_ UINT* size) override;

        IFACEMETHOD(GetView)(
            _Outptr_result_maybenull_ wfc::IVectorView<IInspectable*>** view) override;

        IFACEMETHOD(IndexOf)(
            _In_opt_ IInspectable* value,
            _Out_ UINT* index,
            _Out_ BOOLEAN* found) override;

        // write methods
        IFACEMETHOD(SetAt)(
            _In_ UINT index,
            _In_opt_ IInspectable* item) override;

        IFACEMETHOD(InsertAt)(
            _In_ UINT index,
            _In_opt_ IInspectable* item) override;

        IFACEMETHOD(RemoveAt)(_In_ UINT index) override;

        IFACEMETHOD(Append)(_In_opt_ IInspectable* item) override;

        IFACEMETHOD(Clear)() override;

    protected:
        ItemCollection();
        ~ItemCollection() override;

        // Supports the IItemCollection interface.
        _Check_return_ HRESULT QueryInterfaceImpl(
            _In_ REFIID iid,
            _Outptr_ void** ppObject) override;

    private:
        // ItemsControl API
        _Check_return_ HRESULT Init(_In_ ItemsControl* pOwner);
        _Check_return_ HRESULT NotifyCollectionReady();

        _Check_return_ HRESULT UpdateItemsSourceList(
            _In_ wfc::IIterable<IInspectable*>* pSource);

    public:

        BOOLEAN ItemsSourceActive();

        _Check_return_ HRESULT DisconnectVisualChildrenRecursive();

    private:
        // Raise RaiseVectorChanged event
        _Check_return_ HRESULT RaiseVectorChanged(
            _In_ wfc::CollectionChange action,
            _In_ UINT index);

        _Check_return_ HRESULT CheckReentrancy();

        _Check_return_ HRESULT OnCollectionChanged(
            _In_ wfc::IObservableVector<IInspectable*>* pSender,
            _In_ wfc::IVectorChangedEventArgs* e);

    private:
        // weak reference to the ItemsControl
        ctl::WeakRefPtr m_wrOwner;
        BOOLEAN m_bInItemChanging;
        BOOLEAN m_bBusy;
        TrackerPtr<wfc::IIterable<IInspectable*>> m_tpItemsSource;
        TrackerPtr<wfc::IVector<IInspectable*>> m_tpItemsView;
        TrackerPtr<wfc::IObservableVector<IInspectable*>> m_tpItemsObservableVector;

        ctl::EventPtr<VectorChangedEventCallback> m_epVectorChangedHandler;
    };
}
