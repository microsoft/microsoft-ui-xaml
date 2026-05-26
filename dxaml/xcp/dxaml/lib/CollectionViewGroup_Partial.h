// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#pragma once

#include "CollectionViewGroup.g.h"

namespace DirectUI
{
    class GroupedDataCollectionView;

    PARTIAL_CLASS(CollectionViewGroup)
    {
    public:

        CollectionViewGroup();
        ~CollectionViewGroup() override;

    public:

        _Check_return_ HRESULT SetSource(
            _In_ IInspectable *pSource,
            _In_ IInspectable *pItems,      // Note that this can be same as above
            _In_ GroupedDataCollectionView *pOwner);

        void ClearOwner();

        _Check_return_ HRESULT get_GroupImpl(_Outptr_ IInspectable** pValue);
        _Check_return_ HRESULT get_GroupItemsImpl(_Outptr_ wfc::IObservableVector<IInspectable*>** pValue);

        // ICustomPropertyProvider
        _Check_return_ HRESULT get_TypeImpl(_Out_ wxaml_interop::TypeName* pValue);
        _Check_return_ HRESULT GetCustomPropertyImpl(_In_ HSTRING name, _Outptr_ xaml_data::ICustomProperty** returnValue);
        _Check_return_ HRESULT GetIndexedPropertyImpl(_In_ HSTRING name, _In_ wxaml_interop::TypeName type, _Outptr_ xaml_data::ICustomProperty** returnValue)
        {
            *returnValue = nullptr;
            RRETURN(S_OK);
        }
        _Check_return_ HRESULT GetStringRepresentationImpl(_Out_ HSTRING* returnValue);

    private:

        _Check_return_ HRESULT GetObservableVector(
            _In_ IInspectable *pSource,
            _Out_ wfc::IObservableVector<IInspectable *> **ppObservableVector,
            _Out_ bool *pfSourceIsObservable);

        _Check_return_ HRESULT OnItemsVectorChanged(_In_ wfc::IVectorChangedEventArgs *pArgs);

        static _Check_return_ HRESULT GetGroupPropertyValue(_In_ IInspectable* pTarget, _Outptr_ IInspectable** ppValue);
        static _Check_return_ HRESULT GetGroupItemsPropertyValue(_In_ IInspectable* pTarget, _Outptr_ IInspectable** ppValue);

        ctl::WeakRefPtr m_spOwnerRef;
        TrackerPtr<wfc::IObservableVector<IInspectable *>> m_tpObservableItems;
        TrackerPtr<IInspectable> m_tpGroup;
        TrackerPtr<IInspectable> m_tpGroupItems;

        ctl::EventPtr<VectorChangedEventCallback> m_epVectorChangedHandler;
    };
}
