// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#pragma once

namespace Private
{
    // FYI: Ideally this class shouldn't exist and "\windows\dxaml\xcp\dxaml\lib\BindableVectorWrapper.h/cpp" should be used instead.
    class BindableToObservableVectorWrapper
        : public Private::ReferenceTrackerRuntimeClass<
            wfc::IVector<IInspectable*>
            , wfc::IObservableVector<IInspectable*>
            >
    {
        WuxpInspectableClass(L"Private::BindableToObservableVectorWrapper", TrustLevel::BaseTrust);

    public:
        BindableToObservableVectorWrapper();
        virtual ~BindableToObservableVectorWrapper();

        _Check_return_ HRESULT RuntimeClassInitialize(_In_ xaml_interop::IBindableVector* pList);

        // wfc::IVector<IInspectable*>
        _Check_return_ HRESULT STDMETHODCALLTYPE GetAt(_In_opt_ unsigned index, _Outptr_ IInspectable* *item) override;
        _Check_return_ HRESULT STDMETHODCALLTYPE get_Size(_Out_ unsigned *size) override;
        _Check_return_ HRESULT STDMETHODCALLTYPE GetView(_Outptr_result_maybenull_ wfc::IVectorView<IInspectable*> **view) override;
        _Check_return_ HRESULT STDMETHODCALLTYPE IndexOf(_In_opt_ IInspectable* value, _Out_ unsigned *index, _Out_ boolean *found) override;
        _Check_return_ HRESULT STDMETHODCALLTYPE SetAt(_In_ unsigned index, _In_opt_ IInspectable* item) override;
        _Check_return_ HRESULT STDMETHODCALLTYPE InsertAt(_In_ unsigned index, _In_opt_ IInspectable* item) override;
        _Check_return_ HRESULT STDMETHODCALLTYPE RemoveAt(_In_ unsigned index) override;
        _Check_return_ HRESULT STDMETHODCALLTYPE Append(_In_opt_ IInspectable* item) override;
        _Check_return_ HRESULT STDMETHODCALLTYPE RemoveAtEnd() override;
        _Check_return_ HRESULT STDMETHODCALLTYPE Clear() override;

        // wfc::IObservableVector<IInspectable*>
        _Check_return_ HRESULT STDMETHODCALLTYPE add_VectorChanged(_In_opt_ wfc::VectorChangedEventHandler<IInspectable*>* handler, _Out_ EventRegistrationToken*  token) override;
        _Check_return_ HRESULT STDMETHODCALLTYPE remove_VectorChanged(_In_ EventRegistrationToken  token) override;

    private:
        _Check_return_ HRESULT OnCollectionChanged(_In_ IInspectable* pSender, _In_ xaml_interop::INotifyCollectionChangedEventArgs* pArgs);
        _Check_return_ HRESULT NotifyVectorChanged(wfc::CollectionChange change, unsigned index);

        _Check_return_ HRESULT ProcessVectorChange(_In_ xaml_interop::IBindableObservableVector* pSender, _In_ IInspectable* pArgs);
        _Check_return_ HRESULT NotifyVectorChanged(_In_ wfc::IVectorChangedEventArgs* pArgs);

    private:
        class VectorChangedEventArgs;

        Private::TrackerPtr<xaml_interop::IBindableVector> _tpItemSourceAsBindable;
        Private::TrackerPtr<xaml_interop::INotifyCollectionChanged> _tpItemsSourceAsNotify;
        EventRegistrationToken _tokenCollectionChanged;

        Private::TrackerEventSource<wfc::VectorChangedEventHandler<IInspectable*>> _tpEvent;
        wrl::ComPtr<VectorChangedEventArgs> _spVectorChangedArgs;

        Private::TrackerPtr<xaml_interop::IBindableObservableVector> _tpObservableVector;
        EventRegistrationToken _tokenVectorChanged;
    };

}

