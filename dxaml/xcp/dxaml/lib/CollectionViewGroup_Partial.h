// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#pragma once

#include "CollectionViewGroup.g.h"

namespace DirectUI
{
    class GroupedDataCollectionView;

    // WPF's CollectionViewGroup for a group of an IsSourceGrouped view: the app's group object is Group, its items are
    // GroupItems, and the group is always a bottom-level group.
    PARTIAL_CLASS(CollectionViewGroup),
        public xaml_data::ICollectionViewGroup2
    {
    public:

        CollectionViewGroup();
        ~CollectionViewGroup() override;

        // ICollectionViewGroup2
        IFACEMETHOD(get_ItemCount)(_Out_ INT32* pValue) override;
        IFACEMETHOD(get_IsBottomLevel)(_Out_ BOOLEAN* pValue) override;

    protected:
        HRESULT QueryInterfaceImpl(_In_ REFIID iid, _Outptr_ void** ppObject) override;

        // GetIids reports ICollectionViewGroup2 when QueryInterfaceImpl answers it; both read m_isGroup2Exposed, so GetIids
        // sizes and fills its array from one answer. Feature_ExperimentalApi::IsEnabled() is a compile-time true in C++
        // in every build (FeatureFlags.h), so this answers in stable too, as generated experimental interfaces do: only
        // the IDL feature hides the interface from the stable WinMD.
        INT GetImplementedIIDsCount() override;
        void CopyIIDsToArray(INT first, IID* pResult) override;

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

        // Feature_ExperimentalApi, read once at construction (QueryInterfaceImpl, GetIids).
        const bool m_isGroup2Exposed;
    };
}
