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


#define __GroupStyleSelector_GUID "1b467be6-6bef-4910-b6f8-a44db1e078c9"

namespace DirectUI
{
    class GroupStyleSelector;
    class GroupStyle;

    class __declspec(novtable) GroupStyleSelectorGenerated:
        public ctl::WeakReferenceSource
        , public ABI::Microsoft::UI::Xaml::Controls::IGroupStyleSelector
        , public ABI::Microsoft::UI::Xaml::Controls::IGroupStyleSelectorOverrides
    {
        friend class DirectUI::GroupStyleSelector;

        INSPECTABLE_CLASS(L"Microsoft.UI.Xaml.Controls.GroupStyleSelector");

        BEGIN_INTERFACE_MAP(GroupStyleSelectorGenerated, ctl::WeakReferenceSource)
            INTERFACE_ENTRY(GroupStyleSelectorGenerated, ABI::Microsoft::UI::Xaml::Controls::IGroupStyleSelector)
            INTERFACE_ENTRY(GroupStyleSelectorGenerated, ABI::Microsoft::UI::Xaml::Controls::IGroupStyleSelectorOverrides)
        END_INTERFACE_MAP(GroupStyleSelectorGenerated, ctl::WeakReferenceSource)

    public:
        GroupStyleSelectorGenerated();
        ~GroupStyleSelectorGenerated() override;

        // Event source typedefs.

        KnownTypeIndex GetTypeIndex() const override
        {
            return KnownTypeIndex::GroupStyleSelector;
        }

        static XCP_FORCEINLINE KnownTypeIndex GetTypeIndexStatic()
        {
            return KnownTypeIndex::GroupStyleSelector;
        }

        // Properties.

        // Events.

        // Methods.
        IFACEMETHOD(SelectGroupStyle)(_In_opt_ IInspectable* pGroup, UINT level, _Outptr_ ABI::Microsoft::UI::Xaml::Controls::IGroupStyle** ppReturnValue) override;
        IFACEMETHOD(SelectGroupStyleCore)(_In_opt_ IInspectable* pGroup, UINT level, _Outptr_ ABI::Microsoft::UI::Xaml::Controls::IGroupStyle** ppReturnValue) override;


    protected:
        HRESULT QueryInterfaceImpl(_In_ REFIID iid, _Outptr_ void** ppObject) override;

    private:

        // Fields.
    };
}

#include "GroupStyleSelector_Partial.h"

namespace DirectUI
{
    // Note that the ordering of the base types here is important - the base factory comes first, followed by all the
    // interfaces specific to this type.  By doing this, we allow every Factory's CreateInstance method to be more
    // COMDAT-folding-friendly.  Because this ensures that the first vfptr contains GetTypeIndex, it means that all
    // CreateInstance functions with the same base factory generate the same assembly instructions and thus will
    // fold together.  This is significant for binary size in Microsoft.UI.Xaml.dll so change this only with great
    // care.
    class __declspec(novtable) GroupStyleSelectorFactory:
       public ctl::AggregableActivationFactory<DirectUI::GroupStyleSelector>
        , public ABI::Microsoft::UI::Xaml::Controls::IGroupStyleSelectorFactory
    {
        BEGIN_INTERFACE_MAP(GroupStyleSelectorFactory, ctl::AggregableActivationFactory<DirectUI::GroupStyleSelector>)
            INTERFACE_ENTRY(GroupStyleSelectorFactory, ABI::Microsoft::UI::Xaml::Controls::IGroupStyleSelectorFactory)
        END_INTERFACE_MAP(GroupStyleSelectorFactory, ctl::AggregableActivationFactory<DirectUI::GroupStyleSelector>)

    public:
        // Factory methods.
        IFACEMETHOD(CreateInstance)(_In_opt_ IInspectable* pOuter, _Outptr_ IInspectable** ppInner, _Outptr_ ABI::Microsoft::UI::Xaml::Controls::IGroupStyleSelector** ppInstance);

        // Static properties.

        // Dependency properties.

        // Attached properties.

        // Static methods.

        // Static events.

    protected:
        HRESULT QueryInterfaceImpl(_In_ REFIID iid, _Outptr_ void** ppObject) override;

        KnownTypeIndex GetTypeIndex() const override
        {
            return KnownTypeIndex::GroupStyleSelector;
        }


    private:

        // Customized static properties.

        // Customized static  methods.
    };
}
