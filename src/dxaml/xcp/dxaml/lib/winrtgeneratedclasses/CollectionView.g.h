// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.
//------------------------------------------------------------------------
//
//  Abstract:
//
//      XAML types.
//      NOTE: This file was generated by a tool.
//
//------------------------------------------------------------------------

#pragma once


#define __CollectionView_GUID "890bfa69-fe1b-4e08-a931-dcaa672da7b2"

namespace DirectUI
{
    class CollectionView;

    class __declspec(novtable) CollectionViewGenerated:
        public DirectUI::DependencyObject
        , public ABI::Microsoft::UI::Xaml::Data::ICollectionView
        , public ABI::Microsoft::UI::Xaml::Data::ICustomPropertyProvider
        , public ABI::Microsoft::UI::Xaml::Data::INotifyPropertyChanged
        , public ABI::Windows::Foundation::Collections::IIterable<IInspectable*>
        , public ABI::Windows::Foundation::Collections::IObservableVector<IInspectable*>
        , public ABI::Windows::Foundation::Collections::IVector<IInspectable*>
    {
        friend class DirectUI::CollectionView;


        BEGIN_INTERFACE_MAP(CollectionViewGenerated, DirectUI::DependencyObject)
            INTERFACE_ENTRY(CollectionViewGenerated, ABI::Microsoft::UI::Xaml::Data::ICollectionView)
            INTERFACE_ENTRY(CollectionViewGenerated, ABI::Microsoft::UI::Xaml::Data::ICustomPropertyProvider)
            INTERFACE_ENTRY(CollectionViewGenerated, ABI::Microsoft::UI::Xaml::Data::INotifyPropertyChanged)
            INTERFACE_ENTRY(CollectionViewGenerated, ABI::Windows::Foundation::Collections::IIterable<IInspectable*>)
            INTERFACE_ENTRY(CollectionViewGenerated, ABI::Windows::Foundation::Collections::IObservableVector<IInspectable*>)
            INTERFACE_ENTRY(CollectionViewGenerated, ABI::Windows::Foundation::Collections::IVector<IInspectable*>)
        END_INTERFACE_MAP(CollectionViewGenerated, DirectUI::DependencyObject)

    public:
        CollectionViewGenerated();
        ~CollectionViewGenerated() override;

        // Event source typedefs.
        typedef CEventSource<ABI::Microsoft::UI::Xaml::Data::IPropertyChangedEventHandler, IInspectable, ABI::Microsoft::UI::Xaml::Data::IPropertyChangedEventArgs> PropertyChangedEventSourceType;

        KnownTypeIndex GetTypeIndex() const override
        {
            return KnownTypeIndex::CollectionView;
        }

        static XCP_FORCEINLINE KnownTypeIndex GetTypeIndexStatic()
        {
            return KnownTypeIndex::CollectionView;
        }

        // Properties.
        IFACEMETHOD(get_CollectionGroups)(_Outptr_result_maybenull_ ABI::Windows::Foundation::Collections::IObservableVector<IInspectable*>** ppValue) override;
        IFACEMETHOD(get_CurrentItem)(_Outptr_result_maybenull_ IInspectable** ppValue) override;
        IFACEMETHOD(get_CurrentPosition)(_Out_ INT* pValue) override;
        IFACEMETHOD(get_HasMoreItems)(_Out_ BOOLEAN* pValue) override;
        IFACEMETHOD(get_IsCurrentAfterLast)(_Out_ BOOLEAN* pValue) override;
        IFACEMETHOD(get_IsCurrentBeforeFirst)(_Out_ BOOLEAN* pValue) override;
        IFACEMETHOD(get_Type)(_Out_ ABI::Windows::UI::Xaml::Interop::TypeName* pValue) override;

        // Events.
        IFACEMETHOD(add_CurrentChanged)(_In_ ABI::Windows::Foundation::IEventHandler<IInspectable*>* pValue, _Out_ EventRegistrationToken* pToken) = 0;
        IFACEMETHOD(remove_CurrentChanged)(EventRegistrationToken token) = 0;
        IFACEMETHOD(add_CurrentChanging)(_In_ ABI::Microsoft::UI::Xaml::Data::ICurrentChangingEventHandler* pValue, _Out_ EventRegistrationToken* pToken) = 0;
        IFACEMETHOD(remove_CurrentChanging)(EventRegistrationToken token) = 0;
        _Check_return_ HRESULT GetPropertyChangedEventSourceNoRef(_Outptr_ PropertyChangedEventSourceType** ppEventSource);
        IFACEMETHOD(add_PropertyChanged)(_In_ ABI::Microsoft::UI::Xaml::Data::IPropertyChangedEventHandler* pValue, _Out_ EventRegistrationToken* pToken) override;
        IFACEMETHOD(remove_PropertyChanged)(EventRegistrationToken token) override;

        // Methods.
        IFACEMETHOD(GetCustomProperty)(_In_ HSTRING name, _Outptr_ ABI::Microsoft::UI::Xaml::Data::ICustomProperty** ppReturnValue) override;
        IFACEMETHOD(GetIndexedProperty)(_In_ HSTRING name, ABI::Windows::UI::Xaml::Interop::TypeName type, _Outptr_ ABI::Microsoft::UI::Xaml::Data::ICustomProperty** ppReturnValue) override;
        IFACEMETHOD(GetStringRepresentation)(_Out_ HSTRING* pReturnValue) override;
        IFACEMETHOD(LoadMoreItemsAsync)(UINT count, _Outptr_ ABI::Windows::Foundation::IAsyncOperation<ABI::Microsoft::UI::Xaml::Data::LoadMoreItemsResult>** ppReturnValue) override;
        IFACEMETHOD(MoveCurrentTo)(_In_opt_ IInspectable* pItem, _Out_ BOOLEAN* pReturnValue) override;
        IFACEMETHOD(MoveCurrentToFirst)(_Out_ BOOLEAN* pReturnValue) override;
        IFACEMETHOD(MoveCurrentToLast)(_Out_ BOOLEAN* pReturnValue) override;
        IFACEMETHOD(MoveCurrentToNext)(_Out_ BOOLEAN* pReturnValue) override;
        IFACEMETHOD(MoveCurrentToPosition)(INT index, _Out_ BOOLEAN* pReturnValue) override;
        IFACEMETHOD(MoveCurrentToPrevious)(_Out_ BOOLEAN* pReturnValue) override;


    protected:
        HRESULT QueryInterfaceImpl(_In_ REFIID iid, _Outptr_ void** ppObject) override;

    private:

        // Fields.
    };
}

#include "CollectionView_Partial.h"

