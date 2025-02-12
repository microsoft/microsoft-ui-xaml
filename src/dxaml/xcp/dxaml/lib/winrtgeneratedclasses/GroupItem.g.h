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

#include "ContentControl.g.h"
#include "ITransitionContextProvider.g.h"

#define __GroupItem_GUID "161af7e4-07ac-4b1a-bcef-8c7a05a557ef"

namespace DirectUI
{
    class GroupItem;

    class __declspec(novtable) GroupItemGenerated:
        public DirectUI::ContentControl
        , public ABI::Microsoft::UI::Xaml::Controls::IGroupItem
        , public DirectUI::ITransitionContextProvider
    {
        friend class DirectUI::GroupItem;

        INSPECTABLE_CLASS(L"Microsoft.UI.Xaml.Controls.GroupItem");

        BEGIN_INTERFACE_MAP(GroupItemGenerated, DirectUI::ContentControl)
            INTERFACE_ENTRY(GroupItemGenerated, ABI::Microsoft::UI::Xaml::Controls::IGroupItem)
            INTERFACE_ENTRY(GroupItemGenerated, DirectUI::ITransitionContextProvider)
        END_INTERFACE_MAP(GroupItemGenerated, DirectUI::ContentControl)

    public:
        GroupItemGenerated();
        ~GroupItemGenerated() override;

        // Event source typedefs.

        KnownTypeIndex GetTypeIndex() const override
        {
            return KnownTypeIndex::GroupItem;
        }

        static XCP_FORCEINLINE KnownTypeIndex GetTypeIndexStatic()
        {
            return KnownTypeIndex::GroupItem;
        }

        // Properties.

        // Events.

        // Methods.
        IFACEMETHOD(GetCurrentTransitionContext)(INT LayoutTickId, _Out_ DirectUI::ThemeTransitionContext* pReturnValue) = 0;
        IFACEMETHOD(GetDropOffsetToRoot)(_Out_ ABI::Windows::Foundation::Point* pReturnValue) = 0;
        IFACEMETHOD(IsCollectionMutatingFast)(_Out_ BOOLEAN* pReturnValue) = 0;


    protected:
        HRESULT QueryInterfaceImpl(_In_ REFIID iid, _Outptr_ void** ppObject) override;

    private:

        // Fields.
    };
}

#include "GroupItem_Partial.h"

namespace DirectUI
{
    // Note that the ordering of the base types here is important - the base factory comes first, followed by all the
    // interfaces specific to this type.  By doing this, we allow every Factory's CreateInstance method to be more
    // COMDAT-folding-friendly.  Because this ensures that the first vfptr contains GetTypeIndex, it means that all
    // CreateInstance functions with the same base factory generate the same assembly instructions and thus will
    // fold together.  This is significant for binary size in Microsoft.UI.Xaml.dll so change this only with great
    // care.
    class __declspec(novtable) GroupItemFactory:
       public ctl::BetterAggregableCoreObjectActivationFactory
        , public ABI::Microsoft::UI::Xaml::Controls::IGroupItemFactory
    {
        BEGIN_INTERFACE_MAP(GroupItemFactory, ctl::BetterAggregableCoreObjectActivationFactory)
            INTERFACE_ENTRY(GroupItemFactory, ABI::Microsoft::UI::Xaml::Controls::IGroupItemFactory)
        END_INTERFACE_MAP(GroupItemFactory, ctl::BetterAggregableCoreObjectActivationFactory)

    public:
        // Factory methods.
        IFACEMETHOD(CreateInstance)(_In_opt_ IInspectable* pOuter, _Outptr_ IInspectable** ppInner, _Outptr_ ABI::Microsoft::UI::Xaml::Controls::IGroupItem** ppInstance);

        // Static properties.

        // Dependency properties.

        // Attached properties.

        // Static methods.

        // Static events.

    protected:
        HRESULT QueryInterfaceImpl(_In_ REFIID iid, _Outptr_ void** ppObject) override;

        KnownTypeIndex GetTypeIndex() const override
        {
            return KnownTypeIndex::GroupItem;
        }


    private:

        // Customized static properties.

        // Customized static  methods.
    };
}
