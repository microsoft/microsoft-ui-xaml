// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#pragma once

// The purpose of ReversedVector is to present a collection that implements IIterable<IInspectable*>, IVector<IInspectable*>, IObservableVector<IInspectable*>
// in backward order, without making a copy of the data, and propagating the collection change notifications correctly.
// The motivation is to allow AutoSuggestBox to present its suggestions in reverse order in its inner ListView.  We
// don't intend for apps to deal with this type directly, it's an adapter for use internally within the framework.
class ReversedVector :
        public wrl::RuntimeClass<
            wrl::RuntimeClassFlags<wrl::RuntimeClassType::WinRt>,
            wfc::IIterable<IInspectable*>,
            wfc::IVector<IInspectable*>,
            wfc::IObservableVector<IInspectable*>>
{
    InspectableClass(nullptr /*internal*/, BaseTrust);

public:
    ReversedVector();
    ~ReversedVector() override;

    _Check_return_ HRESULT SetSource(_In_ wfc::IObservableVector<IInspectable*>* source);

    bool IsBoundTo(_In_  wfc::IObservableVector<IInspectable*>* other) const;

    // IIterable is unimplemented, but we support the interface because IVector logically inherits from it

    IFACEMETHOD(First)(_Outptr_result_maybenull_ wfc::IIterator<IInspectable*> **first) final
    {
        return E_NOTIMPL;
    }

    // Implement IVector

    IFACEMETHOD(GetAt)(_In_ unsigned int index, _Out_ IInspectable** item) final;
    IFACEMETHOD(get_Size)(_Out_ unsigned int *size) final;
    IFACEMETHOD(IndexOf)(_In_ IInspectable* value, _Out_ unsigned int *index, _Out_ boolean *found) final;

    // The rest of IVector is unimplemented.  We don't expect apps to deal with this type directly, it's intended to
    // be used only by ItemCollection.  If needed, many of these could be implemented trivially, but the IVectorView
    // impl would need to build a new vector on demand which could be an unexpected memory hit for a developer who thought
    // he/she was just dealing with a normal vector.  So it's best to be clear right away this type isn't meant for that.
    IFACEMETHOD(GetView)(_Deref_out_opt_ wfc::IVectorView<IInspectable*> **returnValue) final
    {
        return E_NOTIMPL;
    }
    IFACEMETHOD(SetAt)(_In_ unsigned int index, _In_opt_ IInspectable* value) final
    {
        return E_NOTIMPL;
    }
    IFACEMETHOD(InsertAt)(_In_ unsigned int index, _In_opt_ IInspectable* value) final
    {
        return E_NOTIMPL;
    }
    IFACEMETHOD(RemoveAt)(_In_ unsigned int index) final
    {
        return E_NOTIMPL;
    }
    IFACEMETHOD(Append)(_In_ IInspectable* value) final
    {
        return E_NOTIMPL;
    }
    IFACEMETHOD(RemoveAtEnd)() final
    {
        return E_NOTIMPL;
    }
    IFACEMETHOD(Clear)() final
    {
        return E_NOTIMPL;
    }

    // IObservableVector

    IFACEMETHOD(add_VectorChanged)(_In_ wfc::VectorChangedEventHandler<IInspectable*>* handler, _Out_ EventRegistrationToken* token) final;
    IFACEMETHOD(remove_VectorChanged)(_In_ EventRegistrationToken token) final;

private:

    _Check_return_ HRESULT OnInnerVectorChanged(
        _In_ wfc::IObservableVector<IInspectable*>* pSender,
        _In_ wfc::IVectorChangedEventArgs* pArgs);

    unsigned int TransformIndex(unsigned int index);

private:
    wrl::ComPtr<wfc::IVector<IInspectable*>> m_source;
    wrl::ComPtr<wfc::IObservableVector<IInspectable*>> m_observableVector;

    wrl::EventSource<wfc::VectorChangedEventHandler<IInspectable*>> m_eventSource;
    EventRegistrationToken m_vectorChangedToken;
};
