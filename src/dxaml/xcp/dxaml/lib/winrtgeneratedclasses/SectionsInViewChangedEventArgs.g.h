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


#define __SectionsInViewChangedEventArgs_GUID "01c46c61-6ba8-4a3f-bda3-d081f6b37bb3"

namespace DirectUI
{
    class SectionsInViewChangedEventArgs;

    class __declspec(novtable) SectionsInViewChangedEventArgsGenerated :
        public ABI::Microsoft::UI::Xaml::Controls::ISectionsInViewChangedEventArgs,
        public DirectUI::EventArgs
    {
        friend class DirectUI::SectionsInViewChangedEventArgs;

        INSPECTABLE_CLASS(L"Microsoft.UI.Xaml.Controls.SectionsInViewChangedEventArgs");

        BEGIN_INTERFACE_MAP(SectionsInViewChangedEventArgsGenerated, DirectUI::EventArgs)
            INTERFACE_ENTRY(SectionsInViewChangedEventArgsGenerated, ABI::Microsoft::UI::Xaml::Controls::ISectionsInViewChangedEventArgs)
        END_INTERFACE_MAP(SectionsInViewChangedEventArgsGenerated, DirectUI::EventArgs)

    public:
        SectionsInViewChangedEventArgsGenerated();
        ~SectionsInViewChangedEventArgsGenerated() override;

        // Properties.
        IFACEMETHOD(get_AddedSections)(_Outptr_result_maybenull_ ABI::Windows::Foundation::Collections::IVector<ABI::Microsoft::UI::Xaml::Controls::HubSection*>** ppValue) override;
        _Check_return_ HRESULT put_AddedSections(_In_opt_ ABI::Windows::Foundation::Collections::IVector<ABI::Microsoft::UI::Xaml::Controls::HubSection*>* pValue);
        IFACEMETHOD(get_RemovedSections)(_Outptr_result_maybenull_ ABI::Windows::Foundation::Collections::IVector<ABI::Microsoft::UI::Xaml::Controls::HubSection*>** ppValue) override;
        _Check_return_ HRESULT put_RemovedSections(_In_opt_ ABI::Windows::Foundation::Collections::IVector<ABI::Microsoft::UI::Xaml::Controls::HubSection*>* pValue);

        // Methods.

    protected:
        HRESULT QueryInterfaceImpl(_In_ REFIID iid, _Outptr_ void** ppObject) override;

    private:

        // Fields.
    };
}

#include "SectionsInViewChangedEventArgs_Partial.h"

namespace DirectUI
{
    // Note that the ordering of the base types here is important - the base factory comes first, followed by all the
    // interfaces specific to this type.  By doing this, we allow every Factory's CreateInstance method to be more
    // COMDAT-folding-friendly.  Because this ensures that the first vfptr contains GetTypeIndex, it means that all
    // CreateInstance functions with the same base factory generate the same assembly instructions and thus will
    // fold together.  This is significant for binary size in Microsoft.UI.Xaml.dll so change this only with great
    // care.
    class __declspec(novtable) SectionsInViewChangedEventArgsFactory:
       public ctl::AggregableActivationFactory<DirectUI::SectionsInViewChangedEventArgs>
        , public ABI::Microsoft::UI::Xaml::Controls::ISectionsInViewChangedEventArgsFactory
    {
        BEGIN_INTERFACE_MAP(SectionsInViewChangedEventArgsFactory, ctl::AggregableActivationFactory<DirectUI::SectionsInViewChangedEventArgs>)
            INTERFACE_ENTRY(SectionsInViewChangedEventArgsFactory, ABI::Microsoft::UI::Xaml::Controls::ISectionsInViewChangedEventArgsFactory)
        END_INTERFACE_MAP(SectionsInViewChangedEventArgsFactory, ctl::AggregableActivationFactory<DirectUI::SectionsInViewChangedEventArgs>)

    public:
        // Factory methods.

        // Static properties.

        // Dependency properties.

        // Attached properties.

        // Static methods.

        // Static events.

    protected:
        HRESULT QueryInterfaceImpl(_In_ REFIID iid, _Outptr_ void** ppObject) override;



    private:

        // Customized static properties.

        // Customized static  methods.
    };
}
