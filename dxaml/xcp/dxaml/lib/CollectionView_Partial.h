// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

//  Abstract:
//      The base class CollectionView, for all implemented collection views

#pragma once

#include "CollectionView.g.h"

namespace DirectUI
{
    PARTIAL_CLASS(CollectionView)
    {
    public:
        typedef CFrameworkEventSource<ICurrentChangedEventSource, wf::IEventHandler<IInspectable*>, IInspectable, IInspectable> CurrentChangedEventSourceType;
        typedef CFrameworkEventSource<ICurrentChangingEventSource, xaml_data::ICurrentChangingEventHandler, IInspectable, xaml_data::ICurrentChangingEventArgs> CurrentChangingEventSourceType;

        IFACEMETHOD(add_CurrentChanged)(_In_ wf::IEventHandler<IInspectable*>* pValue, _Out_ EventRegistrationToken* ptToken) override;
        IFACEMETHOD(remove_CurrentChanged)(_In_ EventRegistrationToken tToken) override;
        IFACEMETHOD(add_CurrentChanging)(_In_ xaml_data::ICurrentChangingEventHandler* pValue, _Out_ EventRegistrationToken* ptToken) override;
        IFACEMETHOD(remove_CurrentChanging)(_In_ EventRegistrationToken tToken) override;

    protected:

        CollectionView();
        ~CollectionView() override;

        // Override the name of the class to return to the outside world here
        INSPECTABLE_CLASS(L"Microsoft.UI.Xaml.Data.ICollectionView");

    public:

        // ICollectionView impls
        _Check_return_ HRESULT get_CurrentItemImpl(_Outptr_ IInspectable **value);

        _Check_return_ HRESULT get_CurrentPositionImpl(_Out_ INT *value);

        _Check_return_ HRESULT get_IsCurrentAfterLastImpl(_Out_ BOOLEAN *value);

        _Check_return_ HRESULT get_IsCurrentBeforeFirstImpl(_Out_ BOOLEAN *value);

        _Check_return_ HRESULT MoveCurrentToImpl(_In_opt_ IInspectable *pItem, _Out_ BOOLEAN *returnValue);

        _Check_return_ HRESULT MoveCurrentToPositionImpl(_In_ INT index, _Out_ BOOLEAN *returnValue);

        _Check_return_ HRESULT MoveCurrentToFirstImpl(_Out_ BOOLEAN *returnValue);

        _Check_return_ HRESULT MoveCurrentToLastImpl(_Out_ BOOLEAN *returnValue);

        _Check_return_ HRESULT MoveCurrentToNextImpl(_Out_ BOOLEAN *returnValue);

        _Check_return_ HRESULT MoveCurrentToPreviousImpl(_Out_ BOOLEAN *returnValue);

        virtual _Check_return_ HRESULT get_CollectionGroupsImpl(_Out_ wfc::IObservableVector<IInspectable *> **value);

        virtual _Check_return_ HRESULT get_HasMoreItemsImpl(_Out_ BOOLEAN *value);

        virtual _Check_return_ HRESULT LoadMoreItemsAsyncImpl(
            _In_ UINT32 count,
            _Outptr_ wf::IAsyncOperation<xaml_data::LoadMoreItemsResult> **operation);

        // ICustomPropertyProvider
        _Check_return_ HRESULT get_TypeImpl(_Out_ wxaml_interop::TypeName* pValue);
        _Check_return_ HRESULT GetCustomPropertyImpl(_In_ HSTRING name, _Outptr_ xaml_data::ICustomProperty** returnValue);
        _Check_return_ HRESULT GetIndexedPropertyImpl(_In_ HSTRING name, _In_ wxaml_interop::TypeName type, _Outptr_ xaml_data::ICustomProperty** returnValue)
        {
            *returnValue = nullptr;
            RRETURN(S_OK);
        }
        _Check_return_ HRESULT GetStringRepresentationImpl(_Out_ HSTRING* returnValue);

    protected:

        // IObservableVector<IInspectable *>
        IFACEMETHOD(add_VectorChanged)(
            _In_ wfc::VectorChangedEventHandler<IInspectable *> *pHandler,
            _Out_ EventRegistrationToken *token) override;

        IFACEMETHOD(remove_VectorChanged)(
            _In_ EventRegistrationToken token) override;

        _Check_return_ HRESULT ProcessCollectionChange(_In_ wfc::IVectorChangedEventArgs *pArgs);
        _Check_return_ HRESULT RaisePropertyChanged(_In_reads_(nLength) const WCHAR* name, _In_ const XUINT32 nLength);

        void OnReferenceTrackerWalk(INT walkType) final;

    private:
        _Check_return_ HRESULT GetCurrentChangedEventSource(_Outptr_ CurrentChangedEventSourceType** ppEventSource);
        _Check_return_ HRESULT GetCurrentChangingEventSource(_Outptr_ CurrentChangingEventSourceType** ppEventSource);

        _Check_return_ HRESULT SetCurrentIndexAt(_In_ INT newIndex, _Out_opt_ BOOLEAN *valid);
        _Check_return_ HRESULT RaiseCurrentChanged();
        _Check_return_ HRESULT RaiseCurrentChanging(_In_ xaml_data::ICurrentChangingEventArgs *pArgs);
        _Check_return_ HRESULT RaiseCurrentChanging();
        _Check_return_ HRESULT RaiseVectorChanged(_In_ wfc::IVectorChangedEventArgs *pArgs);

        _Check_return_ HRESULT OkToChangeCurrent(_Out_ BOOLEAN *pfOkToChange);

        static _Check_return_ HRESULT GetCurrentItemPropertyValue(_In_ IInspectable* pTarget, _Outptr_ IInspectable** ppValue);
        static _Check_return_ HRESULT GetCollectionGroupsPropertyValue(_In_ IInspectable* pTarget, _Outptr_ IInspectable** ppValue);
        static _Check_return_ HRESULT GetCurrentPositionPropertyValue(_In_ IInspectable* pTarget, _Outptr_ IInspectable** ppValue);
        static _Check_return_ HRESULT GetIsCurrentAfterLastPropertyValue(_In_ IInspectable* pTarget, _Outptr_ IInspectable** ppValue);
        static _Check_return_ HRESULT GetIsCurrentBeforeFirstPropertyValue(_In_ IInspectable* pTarget, _Outptr_ IInspectable** ppValue);
        static _Check_return_ HRESULT GetHasMoreItemsPropertyValue(_In_ IInspectable* pTarget, _Outptr_ IInspectable** ppValue);

    private:
        TrackerEventSource<
            wfc::VectorChangedEventHandler<IInspectable *>,
            wfc::IObservableVector<IInspectable *>,
            wfc::IVectorChangedEventArgs> m_vectorChangedHandlers;

        TrackerPtr<IInspectable> m_tpCurrentItem;
        INT m_currentPosition;
        BOOLEAN m_isCurrentAfterLast;
        BOOLEAN m_isCurrentBeforeFirst;

        // TODO: This ComPtr will not cause implicit pegging that could lead to a a reference tracker
        // leak since EventSource derives from ComBase only (not WeakReferenceSourceNoThreadID) and has no reference tracking functionality.
        // This will get cleaned up when we move event sources in general to be managed using std::shared_ptr<> - see TFS 993969.
        ctl::ComPtr<CurrentChangingEventSourceType> m_spCurrentChangingEventSource;
        ctl::ComPtr<CurrentChangedEventSourceType> m_spCurrentChangedEventSource;
    };
}
