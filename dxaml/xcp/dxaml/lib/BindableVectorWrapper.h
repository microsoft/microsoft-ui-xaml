// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

//  Abstract:
//      A wrapper on IBindableVector that translates
//      into the standard IVector<IInspectable *>

#pragma once

namespace DirectUI
{

    class BindableVectorWrapper:
        public wfc::IVector<IInspectable *>,
        public wfc::IIterable<IInspectable *>,
        public ctl::WeakReferenceSource
    {
    protected:

        BindableVectorWrapper();
        ~BindableVectorWrapper() override;

    protected:

        BEGIN_INTERFACE_MAP(BindableVectorWrapper, ctl::WeakReferenceSource)
            INTERFACE_ENTRY(BindableVectorWrapper, wfc::IVector<IInspectable *>)
            INTERFACE_ENTRY(BindableVectorWrapper, wfc::IIterable<IInspectable *>)
        END_INTERFACE_MAP(BindableVectorWrapper, ctl::WeakReferenceSource)

        HRESULT QueryInterfaceImpl(_In_ REFIID riid, _Outptr_ void** ppObject) override;

    public:

         // IVector<IInspectable *>
        IFACEMETHOD(GetAt)(_In_opt_ unsigned index, _Out_  IInspectable **item) override;

        IFACEMETHOD(get_Size)(_Out_ unsigned *size) override;

        IFACEMETHOD(GetView)(_Outptr_result_maybenull_ wfc::IVectorView<IInspectable *>** view) override;

        IFACEMETHOD(IndexOf)(_In_opt_ IInspectable * value, _Out_ unsigned *index, _Out_ boolean *found) override;

        IFACEMETHOD(SetAt)(_In_ unsigned index, _In_opt_ IInspectable *item) override;

        IFACEMETHOD(InsertAt)(_In_ unsigned index, _In_ IInspectable *item) override;

        IFACEMETHOD(RemoveAt)(_In_ unsigned index) override;

        IFACEMETHOD(Append)(_In_opt_ IInspectable * item) override;

        IFACEMETHOD(RemoveAtEnd)() override;

        IFACEMETHOD(Clear)() override;

        // IIterable<IInspectable *>
        IFACEMETHOD(First)(_Outptr_ wfc::IIterator<IInspectable *> **value) override;

    public:

        static _Check_return_ HRESULT CreateInstance(
            _In_ xaml_interop::IBindableVector *pVector,
            _Outptr_ wfc::IVector<IInspectable *> **ppVector);

    protected:

        _Check_return_ HRESULT SetVector(_In_ xaml_interop::IBindableVector *pVector);

    private:

        TrackerPtr<xaml_interop::IBindableVector> m_tpSource;

    };
}

