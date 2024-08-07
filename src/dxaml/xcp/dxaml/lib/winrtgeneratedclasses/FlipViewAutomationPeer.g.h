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

#include "SelectorAutomationPeer.g.h"

#define __FlipViewAutomationPeer_GUID "7a88fc54-9e47-4916-a01d-dce76d8b4327"

namespace DirectUI
{
    class FlipViewAutomationPeer;

    class __declspec(novtable) FlipViewAutomationPeerGenerated:
        public DirectUI::SelectorAutomationPeer
        , public ABI::Microsoft::UI::Xaml::Automation::Peers::IFlipViewAutomationPeer
    {
        friend class DirectUI::FlipViewAutomationPeer;

        INSPECTABLE_CLASS(L"Microsoft.UI.Xaml.Automation.Peers.FlipViewAutomationPeer");

        BEGIN_INTERFACE_MAP(FlipViewAutomationPeerGenerated, DirectUI::SelectorAutomationPeer)
            INTERFACE_ENTRY(FlipViewAutomationPeerGenerated, ABI::Microsoft::UI::Xaml::Automation::Peers::IFlipViewAutomationPeer)
        END_INTERFACE_MAP(FlipViewAutomationPeerGenerated, DirectUI::SelectorAutomationPeer)

    public:
        FlipViewAutomationPeerGenerated();
        ~FlipViewAutomationPeerGenerated() override;

        // Event source typedefs.

        KnownTypeIndex GetTypeIndex() const override
        {
            return KnownTypeIndex::FlipViewAutomationPeer;
        }

        static XCP_FORCEINLINE KnownTypeIndex GetTypeIndexStatic()
        {
            return KnownTypeIndex::FlipViewAutomationPeer;
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

#include "FlipViewAutomationPeer_Partial.h"

namespace DirectUI
{
    // Note that the ordering of the base types here is important - the base factory comes first, followed by all the
    // interfaces specific to this type.  By doing this, we allow every Factory's CreateInstance method to be more
    // COMDAT-folding-friendly.  Because this ensures that the first vfptr contains GetTypeIndex, it means that all
    // CreateInstance functions with the same base factory generate the same assembly instructions and thus will
    // fold together.  This is significant for binary size in Microsoft.UI.Xaml.dll so change this only with great
    // care.
    class __declspec(novtable) FlipViewAutomationPeerFactory:
       public ctl::BetterAggregableCoreObjectActivationFactory
        , public ABI::Microsoft::UI::Xaml::Automation::Peers::IFlipViewAutomationPeerFactory
    {
        BEGIN_INTERFACE_MAP(FlipViewAutomationPeerFactory, ctl::BetterAggregableCoreObjectActivationFactory)
            INTERFACE_ENTRY(FlipViewAutomationPeerFactory, ABI::Microsoft::UI::Xaml::Automation::Peers::IFlipViewAutomationPeerFactory)
        END_INTERFACE_MAP(FlipViewAutomationPeerFactory, ctl::BetterAggregableCoreObjectActivationFactory)

    public:
        // Factory methods.
        IFACEMETHOD(CreateInstanceWithOwner)(_In_ ABI::Microsoft::UI::Xaml::Controls::IFlipView* pOwner, _In_opt_ IInspectable* pOuter, _Outptr_ IInspectable** ppInner, _Outptr_ ABI::Microsoft::UI::Xaml::Automation::Peers::IFlipViewAutomationPeer** ppInstance);

        // Static properties.

        // Dependency properties.

        // Attached properties.

        // Static methods.

        // Static events.

    protected:
        HRESULT QueryInterfaceImpl(_In_ REFIID iid, _Outptr_ void** ppObject) override;

        KnownTypeIndex GetTypeIndex() const override
        {
            return KnownTypeIndex::FlipViewAutomationPeer;
        }


    private:
        _Check_return_ HRESULT CreateInstanceWithOwnerImpl(_In_ ABI::Microsoft::UI::Xaml::Controls::IFlipView* pOwner, _In_opt_ IInspectable* pOuter, _Outptr_ IInspectable** ppInner, _Outptr_ ABI::Microsoft::UI::Xaml::Automation::Peers::IFlipViewAutomationPeer** ppInstance);

        // Customized static properties.

        // Customized static  methods.
    };
}
