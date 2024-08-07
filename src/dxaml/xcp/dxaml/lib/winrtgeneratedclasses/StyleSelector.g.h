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


#define __StyleSelector_GUID "26c5d894-49a7-4d33-aad5-cb6437e4937c"

namespace DirectUI
{
    class StyleSelector;
    class Style;

    class __declspec(novtable) StyleSelectorGenerated:
        public ctl::WeakReferenceSource
        , public ABI::Microsoft::UI::Xaml::Controls::IStyleSelector
        , public ABI::Microsoft::UI::Xaml::Controls::IStyleSelectorOverrides
    {
        friend class DirectUI::StyleSelector;

        INSPECTABLE_CLASS(L"Microsoft.UI.Xaml.Controls.StyleSelector");

        BEGIN_INTERFACE_MAP(StyleSelectorGenerated, ctl::WeakReferenceSource)
            INTERFACE_ENTRY(StyleSelectorGenerated, ABI::Microsoft::UI::Xaml::Controls::IStyleSelector)
            INTERFACE_ENTRY(StyleSelectorGenerated, ABI::Microsoft::UI::Xaml::Controls::IStyleSelectorOverrides)
        END_INTERFACE_MAP(StyleSelectorGenerated, ctl::WeakReferenceSource)

    public:
        StyleSelectorGenerated();
        ~StyleSelectorGenerated() override;

        // Event source typedefs.

        KnownTypeIndex GetTypeIndex() const override
        {
            return KnownTypeIndex::StyleSelector;
        }

        static XCP_FORCEINLINE KnownTypeIndex GetTypeIndexStatic()
        {
            return KnownTypeIndex::StyleSelector;
        }

        // Properties.

        // Events.

        // Methods.
        IFACEMETHOD(SelectStyle)(_In_opt_ IInspectable* pItem, _In_ ABI::Microsoft::UI::Xaml::IDependencyObject* pContainer, _Outptr_ ABI::Microsoft::UI::Xaml::IStyle** ppReturnValue) override;
        IFACEMETHOD(SelectStyleCore)(_In_opt_ IInspectable* pItem, _In_ ABI::Microsoft::UI::Xaml::IDependencyObject* pContainer, _Outptr_ ABI::Microsoft::UI::Xaml::IStyle** ppReturnValue) override;


    protected:
        HRESULT QueryInterfaceImpl(_In_ REFIID iid, _Outptr_ void** ppObject) override;

    private:

        // Fields.
    };
}

#include "StyleSelector_Partial.h"

namespace DirectUI
{
    // Note that the ordering of the base types here is important - the base factory comes first, followed by all the
    // interfaces specific to this type.  By doing this, we allow every Factory's CreateInstance method to be more
    // COMDAT-folding-friendly.  Because this ensures that the first vfptr contains GetTypeIndex, it means that all
    // CreateInstance functions with the same base factory generate the same assembly instructions and thus will
    // fold together.  This is significant for binary size in Microsoft.UI.Xaml.dll so change this only with great
    // care.
    class __declspec(novtable) StyleSelectorFactory:
       public ctl::AggregableActivationFactory<DirectUI::StyleSelector>
        , public ABI::Microsoft::UI::Xaml::Controls::IStyleSelectorFactory
    {
        BEGIN_INTERFACE_MAP(StyleSelectorFactory, ctl::AggregableActivationFactory<DirectUI::StyleSelector>)
            INTERFACE_ENTRY(StyleSelectorFactory, ABI::Microsoft::UI::Xaml::Controls::IStyleSelectorFactory)
        END_INTERFACE_MAP(StyleSelectorFactory, ctl::AggregableActivationFactory<DirectUI::StyleSelector>)

    public:
        // Factory methods.
        IFACEMETHOD(CreateInstance)(_In_opt_ IInspectable* pOuter, _Outptr_ IInspectable** ppInner, _Outptr_ ABI::Microsoft::UI::Xaml::Controls::IStyleSelector** ppInstance);

        // Static properties.

        // Dependency properties.

        // Attached properties.

        // Static methods.

        // Static events.

    protected:
        HRESULT QueryInterfaceImpl(_In_ REFIID iid, _Outptr_ void** ppObject) override;

        KnownTypeIndex GetTypeIndex() const override
        {
            return KnownTypeIndex::StyleSelector;
        }


    private:

        // Customized static properties.

        // Customized static  methods.
    };
}
