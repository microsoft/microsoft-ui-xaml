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


#define __StateTriggerBase_GUID "c331ef4e-e041-471f-8377-212f97989e8a"

namespace DirectUI
{
    class StateTriggerBase;

    class __declspec(novtable) StateTriggerBaseGenerated:
        public DirectUI::DependencyObject
        , public ABI::Microsoft::UI::Xaml::IStateTriggerBase
        , public ABI::Microsoft::UI::Xaml::IStateTriggerBaseProtected
    {
        friend class DirectUI::StateTriggerBase;

        INSPECTABLE_CLASS(L"Microsoft.UI.Xaml.StateTriggerBase");

        BEGIN_INTERFACE_MAP(StateTriggerBaseGenerated, DirectUI::DependencyObject)
            INTERFACE_ENTRY(StateTriggerBaseGenerated, ABI::Microsoft::UI::Xaml::IStateTriggerBase)
            INTERFACE_ENTRY(StateTriggerBaseGenerated, ABI::Microsoft::UI::Xaml::IStateTriggerBaseProtected)
        END_INTERFACE_MAP(StateTriggerBaseGenerated, DirectUI::DependencyObject)

    public:
        StateTriggerBaseGenerated();
        ~StateTriggerBaseGenerated() override;

        // Event source typedefs.

        KnownTypeIndex GetTypeIndex() const override
        {
            return KnownTypeIndex::StateTriggerBase;
        }

        static XCP_FORCEINLINE KnownTypeIndex GetTypeIndexStatic()
        {
            return KnownTypeIndex::StateTriggerBase;
        }

        // Properties.
        _Check_return_ HRESULT get_TriggerState(_Out_ BOOLEAN* pValue);
        _Check_return_ HRESULT put_TriggerState(BOOLEAN value);

        // Events.

        // Methods.
        IFACEMETHOD(SetActive)(BOOLEAN IsActive) override;


    protected:
        HRESULT QueryInterfaceImpl(_In_ REFIID iid, _Outptr_ void** ppObject) override;

    private:

        // Fields.
    };
}

#include "StateTriggerBase_Partial.h"

namespace DirectUI
{
    // Note that the ordering of the base types here is important - the base factory comes first, followed by all the
    // interfaces specific to this type.  By doing this, we allow every Factory's CreateInstance method to be more
    // COMDAT-folding-friendly.  Because this ensures that the first vfptr contains GetTypeIndex, it means that all
    // CreateInstance functions with the same base factory generate the same assembly instructions and thus will
    // fold together.  This is significant for binary size in Microsoft.UI.Xaml.dll so change this only with great
    // care.
    class __declspec(novtable) StateTriggerBaseFactory:
       public ctl::BetterAggregableAbstractCoreObjectActivationFactory
        , public ABI::Microsoft::UI::Xaml::IStateTriggerBaseFactory
    {
        BEGIN_INTERFACE_MAP(StateTriggerBaseFactory, ctl::BetterAggregableAbstractCoreObjectActivationFactory)
            INTERFACE_ENTRY(StateTriggerBaseFactory, ABI::Microsoft::UI::Xaml::IStateTriggerBaseFactory)
        END_INTERFACE_MAP(StateTriggerBaseFactory, ctl::BetterAggregableAbstractCoreObjectActivationFactory)

    public:
        // Factory methods.
        IFACEMETHOD(CreateInstance)(_In_opt_ IInspectable* pOuter, _Outptr_ IInspectable** ppInner, _Outptr_ ABI::Microsoft::UI::Xaml::IStateTriggerBase** ppInstance);

        // Static properties.

        // Dependency properties.
        

        // Attached properties.

        // Static methods.

        // Static events.

    protected:
        HRESULT QueryInterfaceImpl(_In_ REFIID iid, _Outptr_ void** ppObject) override;

        KnownTypeIndex GetTypeIndex() const override
        {
            return KnownTypeIndex::StateTriggerBase;
        }


    private:

        // Customized static properties.

        // Customized static  methods.
    };
}
