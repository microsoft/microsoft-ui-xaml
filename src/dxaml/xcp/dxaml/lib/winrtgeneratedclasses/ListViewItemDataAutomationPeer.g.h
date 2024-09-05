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

#include "ListViewBaseItemDataAutomationPeer.g.h"

#define __ListViewItemDataAutomationPeer_GUID "4c51cee4-4727-496f-a874-b602f06f038d"

namespace DirectUI
{
    class ListViewItemDataAutomationPeer;

    class __declspec(novtable) ListViewItemDataAutomationPeerGenerated:
        public DirectUI::ListViewBaseItemDataAutomationPeer
        , public ABI::Microsoft::UI::Xaml::Automation::Peers::IListViewItemDataAutomationPeer
        , public ABI::Microsoft::UI::Xaml::Automation::Provider::IScrollItemProvider
    {
        friend class DirectUI::ListViewItemDataAutomationPeer;

        INSPECTABLE_CLASS(L"Microsoft.UI.Xaml.Automation.Peers.ListViewItemDataAutomationPeer");

        BEGIN_INTERFACE_MAP(ListViewItemDataAutomationPeerGenerated, DirectUI::ListViewBaseItemDataAutomationPeer)
            INTERFACE_ENTRY(ListViewItemDataAutomationPeerGenerated, ABI::Microsoft::UI::Xaml::Automation::Peers::IListViewItemDataAutomationPeer)
            INTERFACE_ENTRY(ListViewItemDataAutomationPeerGenerated, ABI::Microsoft::UI::Xaml::Automation::Provider::IScrollItemProvider)
        END_INTERFACE_MAP(ListViewItemDataAutomationPeerGenerated, DirectUI::ListViewBaseItemDataAutomationPeer)

    public:
        ListViewItemDataAutomationPeerGenerated();
        ~ListViewItemDataAutomationPeerGenerated() override;

        // Event source typedefs.

        KnownTypeIndex GetTypeIndex() const override
        {
            return KnownTypeIndex::ListViewItemDataAutomationPeer;
        }

        static XCP_FORCEINLINE KnownTypeIndex GetTypeIndexStatic()
        {
            return KnownTypeIndex::ListViewItemDataAutomationPeer;
        }

        // Properties.

        // Events.

        // Methods.
        IFACEMETHOD(ScrollIntoView)() override;


    protected:
        HRESULT QueryInterfaceImpl(_In_ REFIID iid, _Outptr_ void** ppObject) override;

    private:

        // Fields.
    };
}

#include "ListViewItemDataAutomationPeer_Partial.h"

namespace DirectUI
{
    // Note that the ordering of the base types here is important - the base factory comes first, followed by all the
    // interfaces specific to this type.  By doing this, we allow every Factory's CreateInstance method to be more
    // COMDAT-folding-friendly.  Because this ensures that the first vfptr contains GetTypeIndex, it means that all
    // CreateInstance functions with the same base factory generate the same assembly instructions and thus will
    // fold together.  This is significant for binary size in Microsoft.UI.Xaml.dll so change this only with great
    // care.
    class __declspec(novtable) ListViewItemDataAutomationPeerFactory:
       public ctl::BetterAggregableCoreObjectActivationFactory
        , public ABI::Microsoft::UI::Xaml::Automation::Peers::IListViewItemDataAutomationPeerFactory
    {
        BEGIN_INTERFACE_MAP(ListViewItemDataAutomationPeerFactory, ctl::BetterAggregableCoreObjectActivationFactory)
            INTERFACE_ENTRY(ListViewItemDataAutomationPeerFactory, ABI::Microsoft::UI::Xaml::Automation::Peers::IListViewItemDataAutomationPeerFactory)
        END_INTERFACE_MAP(ListViewItemDataAutomationPeerFactory, ctl::BetterAggregableCoreObjectActivationFactory)

    public:
        // Factory methods.
        IFACEMETHOD(CreateInstanceWithParentAndItem)(_In_ IInspectable* pItem, _In_ ABI::Microsoft::UI::Xaml::Automation::Peers::IListViewBaseAutomationPeer* pParent, _In_opt_ IInspectable* pOuter, _Outptr_ IInspectable** ppInner, _Outptr_ ABI::Microsoft::UI::Xaml::Automation::Peers::IListViewItemDataAutomationPeer** ppInstance);

        // Static properties.

        // Dependency properties.

        // Attached properties.

        // Static methods.

        // Static events.

    protected:
        HRESULT QueryInterfaceImpl(_In_ REFIID iid, _Outptr_ void** ppObject) override;

        KnownTypeIndex GetTypeIndex() const override
        {
            return KnownTypeIndex::ListViewItemDataAutomationPeer;
        }


    private:
        _Check_return_ HRESULT CreateInstanceWithParentAndItemImpl(_In_ IInspectable* pItem, _In_ ABI::Microsoft::UI::Xaml::Automation::Peers::IListViewBaseAutomationPeer* pParent, _In_opt_ IInspectable* pOuter, _Outptr_ IInspectable** ppInner, _Outptr_ ABI::Microsoft::UI::Xaml::Automation::Peers::IListViewItemDataAutomationPeer** ppInstance);

        // Customized static properties.

        // Customized static  methods.
    };
}