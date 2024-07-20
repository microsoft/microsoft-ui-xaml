// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#include "precomp.h"
#include "ValidationErrorsObservableVectorWrapper.h"
#include "VectorChangedEventArgs.g.h"
#include <windowscollections.h>

using namespace DirectUI;
using namespace xaml_data;
using namespace xaml_interop;

// Helper method for when we need to turn the collection into a read-only collection through IVector.GetView or IIterable.First
// We're doing an extra copy which isnt' ideal, but this isn't a wildly expected scenario, and only internal callers ever interact
// with this class. External callers iterating over the Validation.Errors collection won't run into this path as they'll be referencing
// the collection directly, not this special wrapper.
wrl::ComPtr<wfci_::Vector<IInspectable*>> MakeCopy(_In_ ValidationErrorsCollection* vector);

ValidationErrorsObservableVectorWrapper::ValidationErrorsObservableVectorWrapper()
{
}

ValidationErrorsObservableVectorWrapper::~ValidationErrorsObservableVectorWrapper()
{
    if (m_vectorChangedHandler)
    {
        if (m_vector)
        {
            VERIFYHR(m_vectorChangedHandler.DetachEventHandler(ctl::iinspectable_cast(m_vector.Get())));
        }
    }
}

ctl::ComPtr<wfc::IIterable<IInspectable *>> ValidationErrorsObservableVectorWrapper::CreateInstance(_In_ ValidationErrorsCollection* vector)
{
    ctl::ComPtr<ValidationErrorsObservableVectorWrapper> instance;
    IFCFAILFAST(ctl::make(&instance));
    instance->SetVector(vector);

    ctl::ComPtr<wfc::IIterable<IInspectable *>> result;
    IFCFAILFAST(instance.As(&result));
    return result;
}

ctl::ComPtr<wfc::IVector<IInspectable *>> ValidationErrorsObservableVectorWrapper::CreateInstanceAsVector(_In_ ValidationErrorsCollection* vector)
{
    ctl::ComPtr<wfc::IVector<IInspectable *>> result;
    IFCFAILFAST(CreateInstance(vector).As(&result));
    return result;
}

HRESULT ValidationErrorsObservableVectorWrapper::QueryInterfaceImpl(_In_ const IID& iid, _COM_Outptr_ void** ppObject)
{
    if (InlineIsEqualGUID(iid, __uuidof(wfc::IObservableVector<IInspectable *>)))
    {
        *ppObject = static_cast<wfc::IObservableVector<IInspectable *> *>(this);
    }
    else if (InlineIsEqualGUID(iid, __uuidof(wfc::IVector<IInspectable *>)))
    {
        *ppObject = static_cast<wfc::IVector<IInspectable *> *>(this);
    }
    else if (InlineIsEqualGUID(iid, __uuidof(wfc::IIterable<IInspectable *>)))
    {
        *ppObject = static_cast<wfc::IIterable<IInspectable *> *>(this);
    }
    else
    {
        return ctl::WeakReferenceSource::QueryInterfaceImpl(iid, ppObject);
    }

    AddRefOuter();
    return S_OK;
}

// IObservableVector<IInspectable *>
IFACEMETHODIMP ValidationErrorsObservableVectorWrapper::add_VectorChanged(
    _In_ wfc::VectorChangedEventHandler<IInspectable *> *handler,
    _Out_ EventRegistrationToken *token)
{
    m_vectorChangedHandlers.AddHandler(handler, token);
    return S_OK;
}

IFACEMETHODIMP ValidationErrorsObservableVectorWrapper::remove_VectorChanged(EventRegistrationToken token)
{
   return m_vectorChangedHandlers.RemoveHandler(token);
}

void ValidationErrorsObservableVectorWrapper::SetVector(_In_ ValidationErrorsCollection* vector)
{
    m_vector = vector;
    IFCFAILFAST(m_vectorChangedHandler.AttachEventHandler(m_vector.Get(),
        [this](wfc::IObservableVector<xaml_controls::InputValidationError*> *sender, IInspectable *args)
        {
            return ProcessVectorChange(args);
        }));
}

void ValidationErrorsObservableVectorWrapper::OnReferenceTrackerWalk(INT walkType)
{
    m_vectorChangedHandlers.ReferenceTrackerWalk(static_cast<EReferenceTrackerWalkType>(walkType));
    ctl::WeakReferenceSource::OnReferenceTrackerWalk(walkType);
}

_Check_return_ HRESULT ValidationErrorsObservableVectorWrapper::ProcessVectorChange(_In_ IInspectable* args)
{
    ctl::ComPtr<wfc::IVectorChangedEventArgs> vectorChangedEventArgs ;
    IFC_RETURN(args->QueryInterface(vectorChangedEventArgs.ReleaseAndGetAddressOf()));

    IFC_RETURN(RaiseVectorChanged(vectorChangedEventArgs.Get()));
    return S_OK;
}

_Check_return_ HRESULT ValidationErrorsObservableVectorWrapper::RaiseVectorChanged(_In_ wfc::IVectorChangedEventArgs* args)
{
    return m_vectorChangedHandlers.Raise(this, args);
}

// IVector
IFACEMETHODIMP ValidationErrorsObservableVectorWrapper::GetAt(unsigned index, _COM_Outptr_ IInspectable **item)
{
    wrl::ComPtr<xaml_controls::IInputValidationError> error;
    IFC_RETURN(m_vector->GetAt(index, &error));
    *item = error.Detach();
    return S_OK;
}

IFACEMETHODIMP ValidationErrorsObservableVectorWrapper::get_Size(_Out_ unsigned *size)
{
    return m_vector->get_Size(size);
}

IFACEMETHODIMP ValidationErrorsObservableVectorWrapper::GetView(_COM_Outptr_ wfc::IVectorView<IInspectable *>** view)
{
    return MakeCopy(m_vector.Get())->GetView(view);
}

IFACEMETHODIMP ValidationErrorsObservableVectorWrapper::IndexOf(_In_ IInspectable * value, _Out_ unsigned *index, _Out_ BOOLEAN *found)
{
    wrl::ComPtr<xaml_controls::IInputValidationError> error;
    IFC_RETURN(value->QueryInterface(error.ReleaseAndGetAddressOf()));
    return m_vector->IndexOf(error.Get(), index, found);
}

IFACEMETHODIMP ValidationErrorsObservableVectorWrapper::SetAt(unsigned index, _In_ IInspectable *item)
{
    wrl::ComPtr<xaml_controls::IInputValidationError> error;
    IFC_RETURN(item->QueryInterface(error.ReleaseAndGetAddressOf()));
    return m_vector->SetAt(index, error.Get());
}

IFACEMETHODIMP ValidationErrorsObservableVectorWrapper::InsertAt(unsigned index, _In_ IInspectable *item)
{
    wrl::ComPtr<xaml_controls::IInputValidationError> error;
    IFC_RETURN(item->QueryInterface(error.ReleaseAndGetAddressOf()));
    return m_vector->InsertAt(index, error.Get());
}

IFACEMETHODIMP ValidationErrorsObservableVectorWrapper::RemoveAt(unsigned index)
{
    return m_vector->RemoveAt(index);
}

IFACEMETHODIMP ValidationErrorsObservableVectorWrapper::Append(_In_ IInspectable * item)
{
    wrl::ComPtr<xaml_controls::IInputValidationError> error;
    IFC_RETURN(item->QueryInterface(error.ReleaseAndGetAddressOf()));
    return m_vector->Append(error.Get());
}

IFACEMETHODIMP ValidationErrorsObservableVectorWrapper::RemoveAtEnd()
{
    return m_vector->RemoveAtEnd();
}

IFACEMETHODIMP ValidationErrorsObservableVectorWrapper::Clear()
{
    return m_vector->Clear();
}

// IIterable
IFACEMETHODIMP ValidationErrorsObservableVectorWrapper::First(_COM_Outptr_ wfc::IIterator<IInspectable *> **value)
{
    return MakeCopy(m_vector.Get())->First(value);
}

wrl::ComPtr<wfci_::Vector<IInspectable*>> MakeCopy(_In_ ValidationErrorsCollection* vector)
{
    wrl::ComPtr<wfci_::Vector<IInspectable*>> copy;
    IFCFAILFAST(wfci_::Vector<IInspectable*>::Make(&copy));

    wrl::ComPtr<wfc::IIterator<xaml_controls::InputValidationError*>> iterator;
    boolean hasCurrent = false;
    IFCFAILFAST(vector->First(&iterator));
    IFCFAILFAST(iterator->get_HasCurrent(&hasCurrent));
    while (!!hasCurrent)
    {
        wrl::ComPtr<xaml_controls::IInputValidationError> current;
        IFCFAILFAST(iterator->get_Current(&current));
        IFCFAILFAST(copy->Append(current.Get()));
        IFCFAILFAST(iterator->MoveNext(&hasCurrent));
    }
    return copy;
}
