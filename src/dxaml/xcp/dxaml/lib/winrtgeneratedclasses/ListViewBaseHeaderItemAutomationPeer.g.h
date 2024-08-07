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

#include "FrameworkElementAutomationPeer.g.h"

#define __ListViewBaseHeaderItemAutomationPeer_GUID "1e405b56-86bb-4432-90bc-41975a3234fa"

namespace DirectUI
{
    class ListViewBaseHeaderItemAutomationPeer;

    class __declspec(novtable) ListViewBaseHeaderItemAutomationPeerGenerated:
        public DirectUI::FrameworkElementAutomationPeer
        , public ABI::Microsoft::UI::Xaml::Automation::Peers::IListViewBaseHeaderItemAutomationPeer
    {
        friend class DirectUI::ListViewBaseHeaderItemAutomationPeer;

        INSPECTABLE_CLASS(L"Microsoft.UI.Xaml.Automation.Peers.ListViewBaseHeaderItemAutomationPeer");

        BEGIN_INTERFACE_MAP(ListViewBaseHeaderItemAutomationPeerGenerated, DirectUI::FrameworkElementAutomationPeer)
            INTERFACE_ENTRY(ListViewBaseHeaderItemAutomationPeerGenerated, ABI::Microsoft::UI::Xaml::Automation::Peers::IListViewBaseHeaderItemAutomationPeer)
        END_INTERFACE_MAP(ListViewBaseHeaderItemAutomationPeerGenerated, DirectUI::FrameworkElementAutomationPeer)

    public:
        ListViewBaseHeaderItemAutomationPeerGenerated();
        ~ListViewBaseHeaderItemAutomationPeerGenerated() override;

        // Event source typedefs.

        KnownTypeIndex GetTypeIndex() const override
        {
            return KnownTypeIndex::ListViewBaseHeaderItemAutomationPeer;
        }

        static XCP_FORCEINLINE KnownTypeIndex GetTypeIndexStatic()
        {
            return KnownTypeIndex::ListViewBaseHeaderItemAutomationPeer;
        }

        // Properties.

        // Events.

        // Methods.


    protected:
        HRESULT QueryInterfaceImpl(_In_ REFIID iid, _Outptr_ void** ppObject) override;

    private:

        // Fields.
    };
}

#include "ListViewBaseHeaderItemAutomationPeer_Partial.h"

namespace DirectUI
{
    // Note that the ordering of the base types here is important - the base factory comes first, followed by all the
    // interfaces specific to this type.  By doing this, we allow every Factory's CreateInstance method to be more
    // COMDAT-folding-friendly.  Because this ensures that the first vfptr contains GetTypeIndex, it means that all
    // CreateInstance functions with the same base factory generate the same assembly instructions and thus will
    // fold together.  This is significant for binary size in Microsoft.UI.Xaml.dll so change this only with great
    // care.
    class __declspec(novtable) ListViewBaseHeaderItemAutomationPeerFactory:
       public ctl::BetterAggregableAbstractCoreObjectActivationFactory
        , public ABI::Microsoft::UI::Xaml::Automation::Peers::IListViewBaseHeaderItemAutomationPeerFactory
    {
        BEGIN_INTERFACE_MAP(ListViewBaseHeaderItemAutomationPeerFactory, ctl::BetterAggregableAbstractCoreObjectActivationFactory)
            INTERFACE_ENTRY(ListViewBaseHeaderItemAutomationPeerFactory, ABI::Microsoft::UI::Xaml::Automation::Peers::IListViewBaseHeaderItemAutomationPeerFactory)
        END_INTERFACE_MAP(ListViewBaseHeaderItemAutomationPeerFactory, ctl::BetterAggregableAbstractCoreObjectActivationFactory)

    public:
        // Factory methods.
        IFACEMETHOD(CreateInstanceWithOwner)(_In_ ABI::Microsoft::UI::Xaml::Controls::IListViewBaseHeaderItem* pOwner, _In_opt_ IInspectable* pOuter, _Outptr_ IInspectable** ppInner, _Outptr_ ABI::Microsoft::UI::Xaml::Automation::Peers::IListViewBaseHeaderItemAutomationPeer** ppInstance);

        // Static properties.

        // Dependency properties.

        // Attached properties.

        // Static methods.

        // Static events.

    protected:
        HRESULT QueryInterfaceImpl(_In_ REFIID iid, _Outptr_ void** ppObject) override;

        KnownTypeIndex GetTypeIndex() const override
        {
            return KnownTypeIndex::ListViewBaseHeaderItemAutomationPeer;
        }


    private:
        _Check_return_ HRESULT CreateInstanceWithOwnerImpl(_In_ ABI::Microsoft::UI::Xaml::Controls::IListViewBaseHeaderItem* pOwner, _In_opt_ IInspectable* pOuter, _Outptr_ IInspectable** ppInner, _Outptr_ ABI::Microsoft::UI::Xaml::Automation::Peers::IListViewBaseHeaderItemAutomationPeer** ppInstance);

        // Customized static properties.

        // Customized static  methods.
    };
}
