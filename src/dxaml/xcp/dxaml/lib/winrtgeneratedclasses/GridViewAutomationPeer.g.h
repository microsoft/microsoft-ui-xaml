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

#include "ListViewBaseAutomationPeer.g.h"

#define __GridViewAutomationPeer_GUID "067eab2b-4443-45a8-8b21-5d9f1b889000"

namespace DirectUI
{
    class GridViewAutomationPeer;

    class __declspec(novtable) GridViewAutomationPeerGenerated:
        public DirectUI::ListViewBaseAutomationPeer
        , public ABI::Microsoft::UI::Xaml::Automation::Peers::IGridViewAutomationPeer
    {
        friend class DirectUI::GridViewAutomationPeer;

        INSPECTABLE_CLASS(L"Microsoft.UI.Xaml.Automation.Peers.GridViewAutomationPeer");

        BEGIN_INTERFACE_MAP(GridViewAutomationPeerGenerated, DirectUI::ListViewBaseAutomationPeer)
            INTERFACE_ENTRY(GridViewAutomationPeerGenerated, ABI::Microsoft::UI::Xaml::Automation::Peers::IGridViewAutomationPeer)
        END_INTERFACE_MAP(GridViewAutomationPeerGenerated, DirectUI::ListViewBaseAutomationPeer)

    public:
        GridViewAutomationPeerGenerated();
        ~GridViewAutomationPeerGenerated() override;

        // Event source typedefs.

        KnownTypeIndex GetTypeIndex() const override
        {
            return KnownTypeIndex::GridViewAutomationPeer;
        }

        static XCP_FORCEINLINE KnownTypeIndex GetTypeIndexStatic()
        {
            return KnownTypeIndex::GridViewAutomationPeer;
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

#include "GridViewAutomationPeer_Partial.h"

namespace DirectUI
{
    // Note that the ordering of the base types here is important - the base factory comes first, followed by all the
    // interfaces specific to this type.  By doing this, we allow every Factory's CreateInstance method to be more
    // COMDAT-folding-friendly.  Because this ensures that the first vfptr contains GetTypeIndex, it means that all
    // CreateInstance functions with the same base factory generate the same assembly instructions and thus will
    // fold together.  This is significant for binary size in Microsoft.UI.Xaml.dll so change this only with great
    // care.
    class __declspec(novtable) GridViewAutomationPeerFactory:
       public ctl::BetterAggregableCoreObjectActivationFactory
        , public ABI::Microsoft::UI::Xaml::Automation::Peers::IGridViewAutomationPeerFactory
    {
        BEGIN_INTERFACE_MAP(GridViewAutomationPeerFactory, ctl::BetterAggregableCoreObjectActivationFactory)
            INTERFACE_ENTRY(GridViewAutomationPeerFactory, ABI::Microsoft::UI::Xaml::Automation::Peers::IGridViewAutomationPeerFactory)
        END_INTERFACE_MAP(GridViewAutomationPeerFactory, ctl::BetterAggregableCoreObjectActivationFactory)

    public:
        // Factory methods.
        IFACEMETHOD(CreateInstanceWithOwner)(_In_ ABI::Microsoft::UI::Xaml::Controls::IGridView* pOwner, _In_opt_ IInspectable* pOuter, _Outptr_ IInspectable** ppInner, _Outptr_ ABI::Microsoft::UI::Xaml::Automation::Peers::IGridViewAutomationPeer** ppInstance);

        // Static properties.

        // Dependency properties.

        // Attached properties.

        // Static methods.

        // Static events.

    protected:
        HRESULT QueryInterfaceImpl(_In_ REFIID iid, _Outptr_ void** ppObject) override;

        KnownTypeIndex GetTypeIndex() const override
        {
            return KnownTypeIndex::GridViewAutomationPeer;
        }


    private:
        _Check_return_ HRESULT CreateInstanceWithOwnerImpl(_In_ ABI::Microsoft::UI::Xaml::Controls::IGridView* pOwner, _In_opt_ IInspectable* pOuter, _Outptr_ IInspectable** ppInner, _Outptr_ ABI::Microsoft::UI::Xaml::Automation::Peers::IGridViewAutomationPeer** ppInstance);

        // Customized static properties.

        // Customized static  methods.
    };
}
