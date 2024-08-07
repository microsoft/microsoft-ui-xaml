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

#define __FlipViewItemAutomationPeer_GUID "c44a8983-2e9d-429e-936a-60dbc0e30886"

namespace DirectUI
{
    class FlipViewItemAutomationPeer;

    class __declspec(novtable) FlipViewItemAutomationPeerGenerated:
        public DirectUI::FrameworkElementAutomationPeer
        , public ABI::Microsoft::UI::Xaml::Automation::Peers::IFlipViewItemAutomationPeer
    {
        friend class DirectUI::FlipViewItemAutomationPeer;

        INSPECTABLE_CLASS(L"Microsoft.UI.Xaml.Automation.Peers.FlipViewItemAutomationPeer");

        BEGIN_INTERFACE_MAP(FlipViewItemAutomationPeerGenerated, DirectUI::FrameworkElementAutomationPeer)
            INTERFACE_ENTRY(FlipViewItemAutomationPeerGenerated, ABI::Microsoft::UI::Xaml::Automation::Peers::IFlipViewItemAutomationPeer)
        END_INTERFACE_MAP(FlipViewItemAutomationPeerGenerated, DirectUI::FrameworkElementAutomationPeer)

    public:
        FlipViewItemAutomationPeerGenerated();
        ~FlipViewItemAutomationPeerGenerated() override;

        // Event source typedefs.

        KnownTypeIndex GetTypeIndex() const override
        {
            return KnownTypeIndex::FlipViewItemAutomationPeer;
        }

        static XCP_FORCEINLINE KnownTypeIndex GetTypeIndexStatic()
        {
            return KnownTypeIndex::FlipViewItemAutomationPeer;
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

#include "FlipViewItemAutomationPeer_Partial.h"

namespace DirectUI
{
    // Note that the ordering of the base types here is important - the base factory comes first, followed by all the
    // interfaces specific to this type.  By doing this, we allow every Factory's CreateInstance method to be more
    // COMDAT-folding-friendly.  Because this ensures that the first vfptr contains GetTypeIndex, it means that all
    // CreateInstance functions with the same base factory generate the same assembly instructions and thus will
    // fold together.  This is significant for binary size in Microsoft.UI.Xaml.dll so change this only with great
    // care.
    class __declspec(novtable) FlipViewItemAutomationPeerFactory:
       public ctl::BetterAggregableCoreObjectActivationFactory
        , public ABI::Microsoft::UI::Xaml::Automation::Peers::IFlipViewItemAutomationPeerFactory
    {
        BEGIN_INTERFACE_MAP(FlipViewItemAutomationPeerFactory, ctl::BetterAggregableCoreObjectActivationFactory)
            INTERFACE_ENTRY(FlipViewItemAutomationPeerFactory, ABI::Microsoft::UI::Xaml::Automation::Peers::IFlipViewItemAutomationPeerFactory)
        END_INTERFACE_MAP(FlipViewItemAutomationPeerFactory, ctl::BetterAggregableCoreObjectActivationFactory)

    public:
        // Factory methods.
        IFACEMETHOD(CreateInstanceWithOwner)(_In_ ABI::Microsoft::UI::Xaml::Controls::IFlipViewItem* pOwner, _In_opt_ IInspectable* pOuter, _Outptr_ IInspectable** ppInner, _Outptr_ ABI::Microsoft::UI::Xaml::Automation::Peers::IFlipViewItemAutomationPeer** ppInstance);

        // Static properties.

        // Dependency properties.

        // Attached properties.

        // Static methods.

        // Static events.

    protected:
        HRESULT QueryInterfaceImpl(_In_ REFIID iid, _Outptr_ void** ppObject) override;

        KnownTypeIndex GetTypeIndex() const override
        {
            return KnownTypeIndex::FlipViewItemAutomationPeer;
        }


    private:
        _Check_return_ HRESULT CreateInstanceWithOwnerImpl(_In_ ABI::Microsoft::UI::Xaml::Controls::IFlipViewItem* pOwner, _In_opt_ IInspectable* pOuter, _Outptr_ IInspectable** ppInner, _Outptr_ ABI::Microsoft::UI::Xaml::Automation::Peers::IFlipViewItemAutomationPeer** ppInstance);

        // Customized static properties.

        // Customized static  methods.
    };
}
