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

#define __AutoSuggestBoxAutomationPeer_GUID "04149440-3043-4199-ad5b-febc61750aa2"

namespace DirectUI
{
    class AutoSuggestBoxAutomationPeer;

    class __declspec(novtable) AutoSuggestBoxAutomationPeerGenerated:
        public DirectUI::FrameworkElementAutomationPeer
        , public ABI::Microsoft::UI::Xaml::Automation::Peers::IAutoSuggestBoxAutomationPeer
        , public ABI::Microsoft::UI::Xaml::Automation::Provider::IInvokeProvider
    {
        friend class DirectUI::AutoSuggestBoxAutomationPeer;

        INSPECTABLE_CLASS(L"Microsoft.UI.Xaml.Automation.Peers.AutoSuggestBoxAutomationPeer");

        BEGIN_INTERFACE_MAP(AutoSuggestBoxAutomationPeerGenerated, DirectUI::FrameworkElementAutomationPeer)
            INTERFACE_ENTRY(AutoSuggestBoxAutomationPeerGenerated, ABI::Microsoft::UI::Xaml::Automation::Peers::IAutoSuggestBoxAutomationPeer)
            INTERFACE_ENTRY(AutoSuggestBoxAutomationPeerGenerated, ABI::Microsoft::UI::Xaml::Automation::Provider::IInvokeProvider)
        END_INTERFACE_MAP(AutoSuggestBoxAutomationPeerGenerated, DirectUI::FrameworkElementAutomationPeer)

    public:
        AutoSuggestBoxAutomationPeerGenerated();
        ~AutoSuggestBoxAutomationPeerGenerated() override;

        // Event source typedefs.

        KnownTypeIndex GetTypeIndex() const override
        {
            return KnownTypeIndex::AutoSuggestBoxAutomationPeer;
        }

        static XCP_FORCEINLINE KnownTypeIndex GetTypeIndexStatic()
        {
            return KnownTypeIndex::AutoSuggestBoxAutomationPeer;
        }

        // Properties.

        // Events.

        // Methods.
        IFACEMETHOD(Invoke)() override;


    protected:
        HRESULT QueryInterfaceImpl(_In_ REFIID iid, _Outptr_ void** ppObject) override;

    private:

        // Fields.
    };
}

#include "AutoSuggestBoxAutomationPeer_Partial.h"

namespace DirectUI
{
    // Note that the ordering of the base types here is important - the base factory comes first, followed by all the
    // interfaces specific to this type.  By doing this, we allow every Factory's CreateInstance method to be more
    // COMDAT-folding-friendly.  Because this ensures that the first vfptr contains GetTypeIndex, it means that all
    // CreateInstance functions with the same base factory generate the same assembly instructions and thus will
    // fold together.  This is significant for binary size in Microsoft.UI.Xaml.dll so change this only with great
    // care.
    class __declspec(novtable) AutoSuggestBoxAutomationPeerFactory:
       public ctl::AbstractActivationFactory
        , public ABI::Microsoft::UI::Xaml::Automation::Peers::IAutoSuggestBoxAutomationPeerFactory
    {
        BEGIN_INTERFACE_MAP(AutoSuggestBoxAutomationPeerFactory, ctl::AbstractActivationFactory)
            INTERFACE_ENTRY(AutoSuggestBoxAutomationPeerFactory, ABI::Microsoft::UI::Xaml::Automation::Peers::IAutoSuggestBoxAutomationPeerFactory)
        END_INTERFACE_MAP(AutoSuggestBoxAutomationPeerFactory, ctl::AbstractActivationFactory)

    public:
        // Factory methods.
        IFACEMETHOD(CreateInstanceWithOwner)(_In_ ABI::Microsoft::UI::Xaml::Controls::IAutoSuggestBox* pOwner, _Outptr_ ABI::Microsoft::UI::Xaml::Automation::Peers::IAutoSuggestBoxAutomationPeer** ppInstance);

        // Static properties.

        // Dependency properties.

        // Attached properties.

        // Static methods.

        // Static events.

    protected:
        HRESULT QueryInterfaceImpl(_In_ REFIID iid, _Outptr_ void** ppObject) override;

        KnownTypeIndex GetTypeIndex() const override
        {
            return KnownTypeIndex::AutoSuggestBoxAutomationPeer;
        }


    private:
        _Check_return_ HRESULT CreateInstanceWithOwnerImpl(_In_ ABI::Microsoft::UI::Xaml::Controls::IAutoSuggestBox* pOwner, _Outptr_ ABI::Microsoft::UI::Xaml::Automation::Peers::IAutoSuggestBoxAutomationPeer** ppInstance);

        // Customized static properties.

        // Customized static  methods.
    };
}