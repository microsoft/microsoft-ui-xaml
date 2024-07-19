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


#define __DependencyPropertyChangedEventArgs_GUID "90e72981-7783-4e2a-861b-55660ef58d33"

namespace DirectUI
{
    class DependencyPropertyChangedEventArgs;

    class __declspec(novtable) DependencyPropertyChangedEventArgsGenerated:
        public ctl::WeakReferenceSource
        , public ABI::Microsoft::UI::Xaml::IDependencyPropertyChangedEventArgs
    {
        friend class DirectUI::DependencyPropertyChangedEventArgs;

        INSPECTABLE_CLASS(L"Microsoft.UI.Xaml.DependencyPropertyChangedEventArgs");

        BEGIN_INTERFACE_MAP(DependencyPropertyChangedEventArgsGenerated, ctl::WeakReferenceSource)
            INTERFACE_ENTRY(DependencyPropertyChangedEventArgsGenerated, ABI::Microsoft::UI::Xaml::IDependencyPropertyChangedEventArgs)
        END_INTERFACE_MAP(DependencyPropertyChangedEventArgsGenerated, ctl::WeakReferenceSource)

    public:
        DependencyPropertyChangedEventArgsGenerated();
        ~DependencyPropertyChangedEventArgsGenerated() override;

        // Event source typedefs.


        // Properties.
        IFACEMETHOD(get_NewValue)(_Outptr_result_maybenull_ IInspectable** ppValue) override;
        _Check_return_ HRESULT put_NewValue(_In_opt_ IInspectable* pValue);
        IFACEMETHOD(get_OldValue)(_Outptr_result_maybenull_ IInspectable** ppValue) override;
        _Check_return_ HRESULT put_OldValue(_In_opt_ IInspectable* pValue);
        IFACEMETHOD(get_Property)(_Outptr_result_maybenull_ ABI::Microsoft::UI::Xaml::IDependencyProperty** ppValue) override;

        // Events.

        // Methods.


    protected:
        HRESULT QueryInterfaceImpl(_In_ REFIID iid, _Outptr_ void** ppObject) override;

    private:

        // Fields.
        TrackerPtr<IInspectable> m_pNewValue;
        TrackerPtr<IInspectable> m_pOldValue;
    };
}

#include "DependencyPropertyChangedEventArgs_Partial.h"

