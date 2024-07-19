// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#include "precomp.h"

#include <SatMacros.h>
#include <NamespaceAliases.h>
#include <ReferenceTrackerExtension.h>
#include <TrackerPtrFamily.h>
#include <ReferenceTrackerRuntimeClass.h>

#include <Windows.Foundation.h>
#include <windowscollections.h>
#include <Microsoft.UI.Xaml.h>

#include <BindableToObservableVectorWrapper.h>

namespace Private
{

class BindableToObservableVectorWrapper::VectorChangedEventArgs
    : public wrl::RuntimeClass<
        wrl::RuntimeClassFlags<wrl::RuntimeClassType::WinRtClassicComMix>
        , wfc::IVectorChangedEventArgs
        >
{
    InspectableClass(L"Private::BindableToObservableVectorWrapper::VectorChangedEventArgs", TrustLevel::BaseTrust);

public:
    VectorChangedEventArgs()
        : _change(wfc::CollectionChange_Reset)
        , _index(0)
    {
    }

    virtual ~VectorChangedEventArgs()
    {
    }

    _Check_return_ HRESULT
    RuntimeClassInitialize()
    {
        return S_OK;
    }

    _Check_return_ HRESULT STDMETHODCALLTYPE
    get_CollectionChange(_Out_ wfc::CollectionChange *change)
    {
        *change = _change;
        return S_OK;
    }

    _Check_return_ HRESULT STDMETHODCALLTYPE get_Index(_Out_ unsigned int *index)
    {
        *index = _index;
        return S_OK;
    }

public:
    wfc::CollectionChange _change;
    unsigned _index;
};

BindableToObservableVectorWrapper::BindableToObservableVectorWrapper()
{
}

BindableToObservableVectorWrapper::~BindableToObservableVectorWrapper()
{
    wrl::ComPtr<xaml_interop::INotifyCollectionChanged> spItemsSourceAsNotify;
    if (_tpItemsSourceAsNotify.TryGetSafeReference(&spItemsSourceAsNotify) && spItemsSourceAsNotify)
    {
        IGNOREHR(spItemsSourceAsNotify->remove_CollectionChanged(_tokenCollectionChanged));
    }

    wrl::ComPtr<xaml_interop::IBindableObservableVector> spObservableVector;
    if (_tpObservableVector.TryGetSafeReference(&spObservableVector) && spObservableVector)
    {
        IGNOREHR(spObservableVector->remove_VectorChanged(_tokenVectorChanged));
    }
}

_Check_return_ HRESULT
BindableToObservableVectorWrapper::RuntimeClassInitialize(_In_ xaml_interop::IBindableVector* pList)
{
    wrl::ComPtr<xaml_interop::INotifyCollectionChanged> spItemsSourceAsNotify;
    wrl::ComPtr<xaml_interop::IBindableObservableVector> spItemsSourceAsObservableVector;
    // Create event handler args object
    IFC_RETURN(wrl::MakeAndInitialize<VectorChangedEventArgs>(&_spVectorChangedArgs));

    // Register event handler
    IFC_RETURN(RegisterEventSource(_tpEvent));

    IFC_RETURN(SetPtrValue(_tpItemSourceAsBindable, pList));

    // Check if list can notify
    if (SUCCEEDED(_tpItemSourceAsBindable.As(&spItemsSourceAsNotify)))
    {
        IFC_RETURN(SetPtrValue(_tpItemsSourceAsNotify, spItemsSourceAsNotify.Get()));

        IFC_RETURN(spItemsSourceAsNotify->add_CollectionChanged(
            wrl::Callback<xaml_interop::INotifyCollectionChangedEventHandler>(
                this,
                &BindableToObservableVectorWrapper::OnCollectionChanged).Get(),
            &_tokenCollectionChanged));
    }
    else if (SUCCEEDED(_tpItemSourceAsBindable.As(&spItemsSourceAsObservableVector)))
    {
        IFC_RETURN(SetPtrValue(_tpObservableVector, spItemsSourceAsObservableVector.Get()));

        IFC_RETURN(_tpObservableVector->add_VectorChanged(
            wrl::Callback<xaml_interop::IBindableVectorChangedEventHandler>(
            this,
            &BindableToObservableVectorWrapper::ProcessVectorChange).Get(),
            &_tokenVectorChanged));
    }


    return S_OK;
}

_Check_return_
HRESULT
BindableToObservableVectorWrapper::ProcessVectorChange(
    _In_ xaml_interop::IBindableObservableVector* pSender,
    _In_ IInspectable* pArgs)
{
    wrl::ComPtr<wfc::IVectorChangedEventArgs> spVectorChangedEventArgs;
    wrl::ComPtr<IInspectable> spVectorChangedEventArgsAsII(pArgs);

    UNREFERENCED_PARAMETER(pSender);
    IFC_RETURN(spVectorChangedEventArgsAsII.As(&spVectorChangedEventArgs));

    IFC_RETURN(NotifyVectorChanged(spVectorChangedEventArgs.Get()));

    return S_OK;
}

_Check_return_
HRESULT
BindableToObservableVectorWrapper::NotifyVectorChanged(_In_ wfc::IVectorChangedEventArgs* pArgs)
{
    RRETURN(_tpEvent.InvokeAll(this, pArgs));
}

_Check_return_ HRESULT STDMETHODCALLTYPE
BindableToObservableVectorWrapper::GetAt(_In_opt_ unsigned index, _Outptr_ IInspectable** item)
{
    HRESULT hr = S_OK;

    IFC(_tpItemSourceAsBindable->GetAt(index, item))

Cleanup:
    return hr;
}

_Check_return_ HRESULT STDMETHODCALLTYPE
BindableToObservableVectorWrapper::get_Size(_Out_ unsigned *size)
{
    HRESULT hr = S_OK;

    IFC(_tpItemSourceAsBindable->get_Size(size))

Cleanup:
    return hr;
}

_Check_return_ HRESULT
BindableToObservableVectorWrapper::OnCollectionChanged(
    _In_ IInspectable* pSender,
    _In_ xaml_interop::INotifyCollectionChangedEventArgs* pArgs)
{
    xaml_interop::NotifyCollectionChangedAction action;

    UNREFERENCED_PARAMETER(pSender);

    IFC_RETURN(pArgs->get_Action(&action));

    if (static_cast<int>(action) == static_cast<int>(wxaml_interop::NotifyCollectionChangedAction_Add) ||
        static_cast<int>(action) == static_cast<int>(wxaml_interop::NotifyCollectionChangedAction_Replace))
    {
        INT index = 0;
        UINT count = 0;
        wrl::ComPtr<xaml_interop::IBindableVector> newItems;

        wfc::CollectionChange change = (static_cast<int>(action) == static_cast<int>(wxaml_interop::NotifyCollectionChangedAction_Add)) ?
            wfc::CollectionChange_ItemInserted :
            wfc::CollectionChange_ItemChanged;

        IFC_RETURN(pArgs->get_NewStartingIndex(&index));

        IFC_RETURN(pArgs->get_NewItems(&newItems));
        IFC_RETURN(newItems->get_Size(&count));

        for (INT i = index; i < index + (INT)count; ++i)
        {
            IFC_RETURN(NotifyVectorChanged(change, i));
        }
    }
    else if (static_cast<int>(action) == static_cast<int>(wxaml_interop::NotifyCollectionChangedAction_Remove))
    {
        INT index = 0;
        UINT count = 0;
        wrl::ComPtr<xaml_interop::IBindableVector> oldItems;

        IFC_RETURN(pArgs->get_OldStartingIndex(&index));

        IFC_RETURN(pArgs->get_OldItems(&oldItems));
        IFC_RETURN(oldItems->get_Size(&count));

        for (INT i = index + (INT)count - 1; i >= index; --i)
        {
            IFC_RETURN(NotifyVectorChanged(wfc::CollectionChange_ItemRemoved, i));
        }
    }
    else
    {
        IFC_RETURN(NotifyVectorChanged(wfc::CollectionChange_Reset, 0));
    }

    return S_OK;
}

_Check_return_ HRESULT
BindableToObservableVectorWrapper::NotifyVectorChanged(wfc::CollectionChange change, unsigned index)
{
    _spVectorChangedArgs->_change = change;
    _spVectorChangedArgs->_index = index;

    IFC_RETURN(_tpEvent.InvokeAll(this, _spVectorChangedArgs.Get()));

    return S_OK;
}

_Check_return_ HRESULT STDMETHODCALLTYPE
BindableToObservableVectorWrapper::GetView(_Outptr_result_maybenull_ wfc::IVectorView<IInspectable*> **view)
{
    UNREFERENCED_PARAMETER(view);

    return E_NOTIMPL;
}

_Check_return_ HRESULT STDMETHODCALLTYPE
BindableToObservableVectorWrapper::IndexOf(_In_opt_ IInspectable* item, _Out_ unsigned *index, _Out_ boolean *found)
{
    UNREFERENCED_PARAMETER(item);
    UNREFERENCED_PARAMETER(index);
    UNREFERENCED_PARAMETER(found);

    return E_NOTIMPL;
}

_Check_return_ HRESULT STDMETHODCALLTYPE
BindableToObservableVectorWrapper::SetAt(_In_ unsigned index, _In_opt_ IInspectable* item)
{
    UNREFERENCED_PARAMETER(index);
    UNREFERENCED_PARAMETER(item);

    return E_NOTIMPL;
}

_Check_return_ HRESULT STDMETHODCALLTYPE
BindableToObservableVectorWrapper::InsertAt(_In_ unsigned index, _In_opt_ IInspectable* item)
{
    UNREFERENCED_PARAMETER(index);
    UNREFERENCED_PARAMETER(item);

    return E_NOTIMPL;
}

_Check_return_ HRESULT STDMETHODCALLTYPE
BindableToObservableVectorWrapper::RemoveAt(_In_ unsigned index)
{
    UNREFERENCED_PARAMETER(index);

    return E_NOTIMPL;
}

_Check_return_ HRESULT STDMETHODCALLTYPE
BindableToObservableVectorWrapper::Append(_In_opt_ IInspectable* item)
{
    UNREFERENCED_PARAMETER(item);

    return E_NOTIMPL;
}

_Check_return_ HRESULT STDMETHODCALLTYPE
BindableToObservableVectorWrapper::RemoveAtEnd()
{
    return E_NOTIMPL;
}

_Check_return_ HRESULT STDMETHODCALLTYPE
BindableToObservableVectorWrapper::Clear()
{
    return E_NOTIMPL;
}

_Check_return_ HRESULT STDMETHODCALLTYPE
BindableToObservableVectorWrapper::add_VectorChanged(_In_opt_ wfc::VectorChangedEventHandler<IInspectable*>* handler, _Out_ EventRegistrationToken* token)
{
    ARG_VALIDRETURNPOINTER(token);
    ARG_NOTNULL_RETURN(handler, "handler");

    IFC_RETURN(_tpEvent.Add(handler, token));

    return S_OK;
}

_Check_return_ HRESULT STDMETHODCALLTYPE
BindableToObservableVectorWrapper::remove_VectorChanged(_In_ EventRegistrationToken token)
{
    IFC_RETURN(_tpEvent.Remove(token));

    return S_OK;
}

}
