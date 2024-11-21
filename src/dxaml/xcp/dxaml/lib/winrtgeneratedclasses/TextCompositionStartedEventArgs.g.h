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


#define __TextCompositionStartedEventArgs_GUID "c3c3155c-942a-4518-94c0-268a87057987"

namespace DirectUI
{
    class TextCompositionStartedEventArgs;

    class __declspec(novtable) __declspec(uuid(__TextCompositionStartedEventArgs_GUID)) TextCompositionStartedEventArgs :
        public ABI::Microsoft::UI::Xaml::Controls::ITextCompositionStartedEventArgs,
        public DirectUI::EventArgs
    {

        INSPECTABLE_CLASS(L"Microsoft.UI.Xaml.Controls.TextCompositionStartedEventArgs");

        BEGIN_INTERFACE_MAP(TextCompositionStartedEventArgs, DirectUI::EventArgs)
            INTERFACE_ENTRY(TextCompositionStartedEventArgs, ABI::Microsoft::UI::Xaml::Controls::ITextCompositionStartedEventArgs)
        END_INTERFACE_MAP(TextCompositionStartedEventArgs, DirectUI::EventArgs)

    public:
        TextCompositionStartedEventArgs();
        ~TextCompositionStartedEventArgs() override;

        // Properties.
        IFACEMETHOD(get_StartIndex)(_Out_ INT* pValue) override;
        _Check_return_ HRESULT put_StartIndex(INT value);
        IFACEMETHOD(get_Length)(_Out_ INT* pValue) override;
        _Check_return_ HRESULT put_Length(INT value);

        // Methods.

    protected:
        HRESULT QueryInterfaceImpl(_In_ REFIID iid, _Outptr_ void** ppObject) override;

    private:
        // Customized properties.

        // Customized methods.

        // Fields.
    };
}


