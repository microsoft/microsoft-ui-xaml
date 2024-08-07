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

#include "IconElement.g.h"

#define __IconSourceElement_GUID "a4a27b61-eb74-4d59-88dd-4898820884a0"

namespace DirectUI
{
    class IconSourceElement;
    class IconSource;

    class __declspec(novtable) __declspec(uuid(__IconSourceElement_GUID)) IconSourceElement:
        public DirectUI::IconElement
        , public ABI::Microsoft::UI::Xaml::Controls::IIconSourceElement
    {

        INSPECTABLE_CLASS(L"Microsoft.UI.Xaml.Controls.IconSourceElement");

        BEGIN_INTERFACE_MAP(IconSourceElement, DirectUI::IconElement)
            INTERFACE_ENTRY(IconSourceElement, ABI::Microsoft::UI::Xaml::Controls::IIconSourceElement)
        END_INTERFACE_MAP(IconSourceElement, DirectUI::IconElement)

    public:
        IconSourceElement();
        ~IconSourceElement() override;

        // Event source typedefs.

        KnownTypeIndex GetTypeIndex() const override
        {
            return KnownTypeIndex::IconSourceElement;
        }

        static XCP_FORCEINLINE KnownTypeIndex GetTypeIndexStatic()
        {
            return KnownTypeIndex::IconSourceElement;
        }

        // Properties.
        IFACEMETHOD(get_IconSource)(_Outptr_result_maybenull_ ABI::Microsoft::UI::Xaml::Controls::IIconSource** ppValue) override;
        IFACEMETHOD(put_IconSource)(_In_opt_ ABI::Microsoft::UI::Xaml::Controls::IIconSource* pValue) override;

        // Events.

        // Methods.


    protected:
        HRESULT QueryInterfaceImpl(_In_ REFIID iid, _Outptr_ void** ppObject) override;

    private:
        // Customized properties.

        // Customized methods.

        // Fields.
    };
}


namespace DirectUI
{
    // Note that the ordering of the base types here is important - the base factory comes first, followed by all the
    // interfaces specific to this type.  By doing this, we allow every Factory's CreateInstance method to be more
    // COMDAT-folding-friendly.  Because this ensures that the first vfptr contains GetTypeIndex, it means that all
    // CreateInstance functions with the same base factory generate the same assembly instructions and thus will
    // fold together.  This is significant for binary size in Microsoft.UI.Xaml.dll so change this only with great
    // care.
    class __declspec(novtable) IconSourceElementFactory:
       public ctl::BetterAggregableCoreObjectActivationFactory
        , public ABI::Microsoft::UI::Xaml::Controls::IIconSourceElementFactory
        , public ABI::Microsoft::UI::Xaml::Controls::IIconSourceElementStatics
    {
        BEGIN_INTERFACE_MAP(IconSourceElementFactory, ctl::BetterAggregableCoreObjectActivationFactory)
            INTERFACE_ENTRY(IconSourceElementFactory, ABI::Microsoft::UI::Xaml::Controls::IIconSourceElementFactory)
            INTERFACE_ENTRY(IconSourceElementFactory, ABI::Microsoft::UI::Xaml::Controls::IIconSourceElementStatics)
        END_INTERFACE_MAP(IconSourceElementFactory, ctl::BetterAggregableCoreObjectActivationFactory)

    public:
        // Factory methods.
        IFACEMETHOD(CreateInstance)(_In_opt_ IInspectable* pOuter, _Outptr_ IInspectable** ppInner, _Outptr_ ABI::Microsoft::UI::Xaml::Controls::IIconSourceElement** ppInstance);

        // Static properties.

        // Dependency properties.
        IFACEMETHOD(get_IconSourceProperty)(_Out_ ABI::Microsoft::UI::Xaml::IDependencyProperty** ppValue) override;

        // Attached properties.

        // Static methods.

        // Static events.

    protected:
        HRESULT QueryInterfaceImpl(_In_ REFIID iid, _Outptr_ void** ppObject) override;

        KnownTypeIndex GetTypeIndex() const override
        {
            return KnownTypeIndex::IconSourceElement;
        }


    private:

        // Customized static properties.

        // Customized static  methods.
    };
}
