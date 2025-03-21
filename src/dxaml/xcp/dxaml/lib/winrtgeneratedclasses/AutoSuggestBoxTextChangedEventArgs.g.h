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


#define __AutoSuggestBoxTextChangedEventArgs_GUID "360f0fab-8c97-430b-824e-db279240f68c"

namespace DirectUI
{
    class AutoSuggestBoxTextChangedEventArgs;

    class __declspec(novtable) AutoSuggestBoxTextChangedEventArgsGenerated:
        public DirectUI::DependencyObject
        , public ABI::Microsoft::UI::Xaml::Controls::IAutoSuggestBoxTextChangedEventArgs
    {
        friend class DirectUI::AutoSuggestBoxTextChangedEventArgs;

        INSPECTABLE_CLASS(L"Microsoft.UI.Xaml.Controls.AutoSuggestBoxTextChangedEventArgs");

        BEGIN_INTERFACE_MAP(AutoSuggestBoxTextChangedEventArgsGenerated, DirectUI::DependencyObject)
            INTERFACE_ENTRY(AutoSuggestBoxTextChangedEventArgsGenerated, ABI::Microsoft::UI::Xaml::Controls::IAutoSuggestBoxTextChangedEventArgs)
        END_INTERFACE_MAP(AutoSuggestBoxTextChangedEventArgsGenerated, DirectUI::DependencyObject)

    public:
        AutoSuggestBoxTextChangedEventArgsGenerated();
        ~AutoSuggestBoxTextChangedEventArgsGenerated() override;

        // Event source typedefs.

        KnownTypeIndex GetTypeIndex() const override
        {
            return KnownTypeIndex::AutoSuggestBoxTextChangedEventArgs;
        }

        static XCP_FORCEINLINE KnownTypeIndex GetTypeIndexStatic()
        {
            return KnownTypeIndex::AutoSuggestBoxTextChangedEventArgs;
        }

        // Properties.
        IFACEMETHOD(get_Reason)(_Out_ ABI::Microsoft::UI::Xaml::Controls::AutoSuggestionBoxTextChangeReason* pValue) override;
        IFACEMETHOD(put_Reason)(ABI::Microsoft::UI::Xaml::Controls::AutoSuggestionBoxTextChangeReason value) override;

        // Events.

        // Methods.
        IFACEMETHOD(CheckCurrent)(_Out_ BOOLEAN* pReturnValue) override;


    protected:
        HRESULT QueryInterfaceImpl(_In_ REFIID iid, _Outptr_ void** ppObject) override;

    private:

        // Fields.
    };
}

#include "AutoSuggestBoxTextChangedEventArgs_Partial.h"

namespace DirectUI
{
    // Note that the ordering of the base types here is important - the base factory comes first, followed by all the
    // interfaces specific to this type.  By doing this, we allow every Factory's CreateInstance method to be more
    // COMDAT-folding-friendly.  Because this ensures that the first vfptr contains GetTypeIndex, it means that all
    // CreateInstance functions with the same base factory generate the same assembly instructions and thus will
    // fold together.  This is significant for binary size in Microsoft.UI.Xaml.dll so change this only with great
    // care.
    class __declspec(novtable) AutoSuggestBoxTextChangedEventArgsFactory:
       public ctl::BetterCoreObjectActivationFactory
        , public ABI::Microsoft::UI::Xaml::Controls::IAutoSuggestBoxTextChangedEventArgsStatics
    {
        BEGIN_INTERFACE_MAP(AutoSuggestBoxTextChangedEventArgsFactory, ctl::BetterCoreObjectActivationFactory)
            INTERFACE_ENTRY(AutoSuggestBoxTextChangedEventArgsFactory, ABI::Microsoft::UI::Xaml::Controls::IAutoSuggestBoxTextChangedEventArgsStatics)
        END_INTERFACE_MAP(AutoSuggestBoxTextChangedEventArgsFactory, ctl::BetterCoreObjectActivationFactory)

    public:
        // Factory methods.

        // Static properties.

        // Dependency properties.
        IFACEMETHOD(get_ReasonProperty)(_Out_ ABI::Microsoft::UI::Xaml::IDependencyProperty** ppValue) override;

        // Attached properties.

        // Static methods.

        // Static events.

    protected:
        HRESULT QueryInterfaceImpl(_In_ REFIID iid, _Outptr_ void** ppObject) override;

        KnownTypeIndex GetTypeIndex() const override
        {
            return KnownTypeIndex::AutoSuggestBoxTextChangedEventArgs;
        }


    private:

        // Customized static properties.

        // Customized static  methods.
    };
}
