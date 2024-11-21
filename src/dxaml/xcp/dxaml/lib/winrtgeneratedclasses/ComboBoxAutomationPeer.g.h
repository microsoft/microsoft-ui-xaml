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

#define __ComboBoxAutomationPeer_GUID "a01d529c-37f4-4d9d-b37a-928e00e8ec8c"

namespace DirectUI
{
    class ComboBoxAutomationPeer;

    class __declspec(novtable) ComboBoxAutomationPeerGenerated:
        public DirectUI::SelectorAutomationPeer
        , public ABI::Microsoft::UI::Xaml::Automation::Peers::IComboBoxAutomationPeer
        , public ABI::Microsoft::UI::Xaml::Automation::Provider::IExpandCollapseProvider
        , public ABI::Microsoft::UI::Xaml::Automation::Provider::IValueProvider
        , public ABI::Microsoft::UI::Xaml::Automation::Provider::IWindowProvider
    {
        friend class DirectUI::ComboBoxAutomationPeer;

        INSPECTABLE_CLASS(L"Microsoft.UI.Xaml.Automation.Peers.ComboBoxAutomationPeer");

        BEGIN_INTERFACE_MAP(ComboBoxAutomationPeerGenerated, DirectUI::SelectorAutomationPeer)
            INTERFACE_ENTRY(ComboBoxAutomationPeerGenerated, ABI::Microsoft::UI::Xaml::Automation::Peers::IComboBoxAutomationPeer)
            INTERFACE_ENTRY(ComboBoxAutomationPeerGenerated, ABI::Microsoft::UI::Xaml::Automation::Provider::IExpandCollapseProvider)
            INTERFACE_ENTRY(ComboBoxAutomationPeerGenerated, ABI::Microsoft::UI::Xaml::Automation::Provider::IValueProvider)
            INTERFACE_ENTRY(ComboBoxAutomationPeerGenerated, ABI::Microsoft::UI::Xaml::Automation::Provider::IWindowProvider)
        END_INTERFACE_MAP(ComboBoxAutomationPeerGenerated, DirectUI::SelectorAutomationPeer)

    public:
        ComboBoxAutomationPeerGenerated();
        ~ComboBoxAutomationPeerGenerated() override;

        // Event source typedefs.

        KnownTypeIndex GetTypeIndex() const override
        {
            return KnownTypeIndex::ComboBoxAutomationPeer;
        }

        static XCP_FORCEINLINE KnownTypeIndex GetTypeIndexStatic()
        {
            return KnownTypeIndex::ComboBoxAutomationPeer;
        }

        // Properties.
        IFACEMETHOD(get_ExpandCollapseState)(_Out_ ABI::Microsoft::UI::Xaml::Automation::ExpandCollapseState* pValue) override;
        IFACEMETHOD(get_InteractionState)(_Out_ ABI::Microsoft::UI::Xaml::Automation::WindowInteractionState* pValue) override;
        IFACEMETHOD(get_IsModal)(_Out_ BOOLEAN* pValue) override;
        IFACEMETHOD(get_IsReadOnly)(_Out_ BOOLEAN* pValue) override;
        IFACEMETHOD(get_IsTopmost)(_Out_ BOOLEAN* pValue) override;
        IFACEMETHOD(get_Maximizable)(_Out_ BOOLEAN* pValue) override;
        IFACEMETHOD(get_Minimizable)(_Out_ BOOLEAN* pValue) override;
        IFACEMETHOD(get_Value)(_Out_ HSTRING* pValue) override;
        IFACEMETHOD(get_VisualState)(_Out_ ABI::Microsoft::UI::Xaml::Automation::WindowVisualState* pValue) override;

        // Events.

        // Methods.
        IFACEMETHOD(Close)() override;
        IFACEMETHOD(Collapse)() override;
        IFACEMETHOD(Expand)() override;
        IFACEMETHOD(SetValue)(_In_ HSTRING value) override;
        IFACEMETHOD(SetVisualState)(ABI::Microsoft::UI::Xaml::Automation::WindowVisualState state) override;
        IFACEMETHOD(WaitForInputIdle)(INT milliseconds, _Out_ BOOLEAN* pReturnValue) override;


    protected:
        HRESULT QueryInterfaceImpl(_In_ REFIID iid, _Outptr_ void** ppObject) override;

    private:

        // Fields.
    };
}

#include "ComboBoxAutomationPeer_Partial.h"

namespace DirectUI
{
    // Note that the ordering of the base types here is important - the base factory comes first, followed by all the
    // interfaces specific to this type.  By doing this, we allow every Factory's CreateInstance method to be more
    // COMDAT-folding-friendly.  Because this ensures that the first vfptr contains GetTypeIndex, it means that all
    // CreateInstance functions with the same base factory generate the same assembly instructions and thus will
    // fold together.  This is significant for binary size in Microsoft.UI.Xaml.dll so change this only with great
    // care.
    class __declspec(novtable) ComboBoxAutomationPeerFactory:
       public ctl::BetterAggregableCoreObjectActivationFactory
        , public ABI::Microsoft::UI::Xaml::Automation::Peers::IComboBoxAutomationPeerFactory
    {
        BEGIN_INTERFACE_MAP(ComboBoxAutomationPeerFactory, ctl::BetterAggregableCoreObjectActivationFactory)
            INTERFACE_ENTRY(ComboBoxAutomationPeerFactory, ABI::Microsoft::UI::Xaml::Automation::Peers::IComboBoxAutomationPeerFactory)
        END_INTERFACE_MAP(ComboBoxAutomationPeerFactory, ctl::BetterAggregableCoreObjectActivationFactory)

    public:
        // Factory methods.
        IFACEMETHOD(CreateInstanceWithOwner)(_In_ ABI::Microsoft::UI::Xaml::Controls::IComboBox* pOwner, _In_opt_ IInspectable* pOuter, _Outptr_ IInspectable** ppInner, _Outptr_ ABI::Microsoft::UI::Xaml::Automation::Peers::IComboBoxAutomationPeer** ppInstance);

        // Static properties.

        // Dependency properties.

        // Attached properties.

        // Static methods.

        // Static events.

    protected:
        HRESULT QueryInterfaceImpl(_In_ REFIID iid, _Outptr_ void** ppObject) override;

        KnownTypeIndex GetTypeIndex() const override
        {
            return KnownTypeIndex::ComboBoxAutomationPeer;
        }


    private:
        _Check_return_ HRESULT CreateInstanceWithOwnerImpl(_In_ ABI::Microsoft::UI::Xaml::Controls::IComboBox* pOwner, _In_opt_ IInspectable* pOuter, _Outptr_ IInspectable** ppInner, _Outptr_ ABI::Microsoft::UI::Xaml::Automation::Peers::IComboBoxAutomationPeer** ppInstance);

        // Customized static properties.

        // Customized static  methods.
    };
}
