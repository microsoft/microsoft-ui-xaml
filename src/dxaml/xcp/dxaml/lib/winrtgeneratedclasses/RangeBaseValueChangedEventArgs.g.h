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

#include "RoutedEventArgs.g.h"

#define __RangeBaseValueChangedEventArgs_GUID "dc32416f-263a-48d3-ba91-b3a60127041e"

namespace DirectUI
{
    class RangeBaseValueChangedEventArgs;

    class __declspec(novtable) __declspec(uuid(__RangeBaseValueChangedEventArgs_GUID)) RangeBaseValueChangedEventArgs :
        public ABI::Microsoft::UI::Xaml::Controls::Primitives::IRangeBaseValueChangedEventArgs,
        public DirectUI::RoutedEventArgs
    {

        INSPECTABLE_CLASS(L"Microsoft.UI.Xaml.Controls.Primitives.RangeBaseValueChangedEventArgs");

        BEGIN_INTERFACE_MAP(RangeBaseValueChangedEventArgs, DirectUI::RoutedEventArgs)
            INTERFACE_ENTRY(RangeBaseValueChangedEventArgs, ABI::Microsoft::UI::Xaml::Controls::Primitives::IRangeBaseValueChangedEventArgs)
        END_INTERFACE_MAP(RangeBaseValueChangedEventArgs, DirectUI::RoutedEventArgs)

    public:
        RangeBaseValueChangedEventArgs();
        ~RangeBaseValueChangedEventArgs() override;

        // Properties.
        IFACEMETHOD(get_OldValue)(_Out_ DOUBLE* pValue) override;
        _Check_return_ HRESULT put_OldValue(DOUBLE value);
        IFACEMETHOD(get_NewValue)(_Out_ DOUBLE* pValue) override;
        _Check_return_ HRESULT put_NewValue(DOUBLE value);

        // Methods.

    protected:
        HRESULT QueryInterfaceImpl(_In_ REFIID iid, _Outptr_ void** ppObject) override;

    private:
        // Customized properties.

        // Customized methods.

        // Fields.
        DOUBLE m_oldValue;
        DOUBLE m_newValue;
    };
}


