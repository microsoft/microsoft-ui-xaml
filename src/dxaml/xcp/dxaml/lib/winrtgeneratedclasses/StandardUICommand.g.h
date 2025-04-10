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

#include "XamlUICommand.g.h"

#define __StandardUICommand_GUID "0eeac8b5-7bf9-463f-9d3e-be8475935cc6"

namespace DirectUI
{
    class StandardUICommand;

    class __declspec(novtable) StandardUICommandGenerated:
        public DirectUI::XamlUICommand
        , public ABI::Microsoft::UI::Xaml::Input::IStandardUICommand
    {
        friend class DirectUI::StandardUICommand;

        INSPECTABLE_CLASS(L"Microsoft.UI.Xaml.Input.StandardUICommand");

        BEGIN_INTERFACE_MAP(StandardUICommandGenerated, DirectUI::XamlUICommand)
            INTERFACE_ENTRY(StandardUICommandGenerated, ABI::Microsoft::UI::Xaml::Input::IStandardUICommand)
        END_INTERFACE_MAP(StandardUICommandGenerated, DirectUI::XamlUICommand)

    public:
        StandardUICommandGenerated();
        ~StandardUICommandGenerated() override;

        // Event source typedefs.

        KnownTypeIndex GetTypeIndex() const override
        {
            return KnownTypeIndex::StandardUICommand;
        }

        static XCP_FORCEINLINE KnownTypeIndex GetTypeIndexStatic()
        {
            return KnownTypeIndex::StandardUICommand;
        }

        // Properties.
        IFACEMETHOD(get_Kind)(_Out_ ABI::Microsoft::UI::Xaml::Input::StandardUICommandKind* pValue) override;
        IFACEMETHOD(put_Kind)(ABI::Microsoft::UI::Xaml::Input::StandardUICommandKind value) override;

        // Events.

        // Methods.


    protected:
        HRESULT QueryInterfaceImpl(_In_ REFIID iid, _Outptr_ void** ppObject) override;

    private:

        // Fields.
    };
}

#include "StandardUICommand_Partial.h"

namespace DirectUI
{
    // Note that the ordering of the base types here is important - the base factory comes first, followed by all the
    // interfaces specific to this type.  By doing this, we allow every Factory's CreateInstance method to be more
    // COMDAT-folding-friendly.  Because this ensures that the first vfptr contains GetTypeIndex, it means that all
    // CreateInstance functions with the same base factory generate the same assembly instructions and thus will
    // fold together.  This is significant for binary size in Microsoft.UI.Xaml.dll so change this only with great
    // care.
    class __declspec(novtable) StandardUICommandFactory:
       public ctl::BetterAggregableCoreObjectActivationFactory
        , public ABI::Microsoft::UI::Xaml::Input::IStandardUICommandFactory
        , public ABI::Microsoft::UI::Xaml::Input::IStandardUICommandStatics
    {
        BEGIN_INTERFACE_MAP(StandardUICommandFactory, ctl::BetterAggregableCoreObjectActivationFactory)
            INTERFACE_ENTRY(StandardUICommandFactory, ABI::Microsoft::UI::Xaml::Input::IStandardUICommandFactory)
            INTERFACE_ENTRY(StandardUICommandFactory, ABI::Microsoft::UI::Xaml::Input::IStandardUICommandStatics)
        END_INTERFACE_MAP(StandardUICommandFactory, ctl::BetterAggregableCoreObjectActivationFactory)

    public:
        // Factory methods.
        IFACEMETHOD(CreateInstance)(_In_opt_ IInspectable* pOuter, _Outptr_ IInspectable** ppInner, _Outptr_ ABI::Microsoft::UI::Xaml::Input::IStandardUICommand** ppInstance);
        IFACEMETHOD(CreateInstanceWithKind)(ABI::Microsoft::UI::Xaml::Input::StandardUICommandKind kind, _In_opt_ IInspectable* pOuter, _Outptr_ IInspectable** ppInner, _Outptr_ ABI::Microsoft::UI::Xaml::Input::IStandardUICommand** ppInstance);

        // Static properties.
        IFACEMETHOD(get_KindProperty)(_Outptr_result_maybenull_ ABI::Microsoft::UI::Xaml::IDependencyProperty** ppValue) override;

        // Dependency properties.
        

        // Attached properties.

        // Static methods.

        // Static events.

    protected:
        HRESULT QueryInterfaceImpl(_In_ REFIID iid, _Outptr_ void** ppObject) override;

        KnownTypeIndex GetTypeIndex() const override
        {
            return KnownTypeIndex::StandardUICommand;
        }


    private:
        _Check_return_ HRESULT CreateInstanceWithKindImpl(ABI::Microsoft::UI::Xaml::Input::StandardUICommandKind kind, _In_opt_ IInspectable* pOuter, _Outptr_ IInspectable** ppInner, _Outptr_ ABI::Microsoft::UI::Xaml::Input::IStandardUICommand** ppInstance);

        // Customized static properties.
         _Check_return_ HRESULT get_KindPropertyImpl(_Outptr_result_maybenull_ ABI::Microsoft::UI::Xaml::IDependencyProperty** ppValue); 

        // Customized static  methods.
    };
}
