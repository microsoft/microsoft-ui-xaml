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

#include "Panel.g.h"

#define __TickBar_GUID "8e9eec1f-706a-4f47-be93-e78f9ae876d1"

namespace DirectUI
{
    class TickBar;
    class Brush;

    class __declspec(novtable) TickBarGenerated:
        public DirectUI::Panel
        , public ABI::Microsoft::UI::Xaml::Controls::Primitives::ITickBar
    {
        friend class DirectUI::TickBar;

        INSPECTABLE_CLASS(L"Microsoft.UI.Xaml.Controls.Primitives.TickBar");

        BEGIN_INTERFACE_MAP(TickBarGenerated, DirectUI::Panel)
            INTERFACE_ENTRY(TickBarGenerated, ABI::Microsoft::UI::Xaml::Controls::Primitives::ITickBar)
        END_INTERFACE_MAP(TickBarGenerated, DirectUI::Panel)

    public:
        TickBarGenerated();
        ~TickBarGenerated() override;

        // Event source typedefs.

        KnownTypeIndex GetTypeIndex() const override
        {
            return KnownTypeIndex::TickBar;
        }

        static XCP_FORCEINLINE KnownTypeIndex GetTypeIndexStatic()
        {
            return KnownTypeIndex::TickBar;
        }

        // Properties.
        IFACEMETHOD(get_Fill)(_Outptr_result_maybenull_ ABI::Microsoft::UI::Xaml::Media::IBrush** ppValue) override;
        IFACEMETHOD(put_Fill)(_In_opt_ ABI::Microsoft::UI::Xaml::Media::IBrush* pValue) override;

        // Events.

        // Methods.


    protected:
        HRESULT QueryInterfaceImpl(_In_ REFIID iid, _Outptr_ void** ppObject) override;

    private:

        // Fields.
    };
}

#include "TickBar_Partial.h"

namespace DirectUI
{
    // Note that the ordering of the base types here is important - the base factory comes first, followed by all the
    // interfaces specific to this type.  By doing this, we allow every Factory's CreateInstance method to be more
    // COMDAT-folding-friendly.  Because this ensures that the first vfptr contains GetTypeIndex, it means that all
    // CreateInstance functions with the same base factory generate the same assembly instructions and thus will
    // fold together.  This is significant for binary size in Microsoft.UI.Xaml.dll so change this only with great
    // care.
    class __declspec(novtable) TickBarFactory:
       public ctl::BetterCoreObjectActivationFactory
        , public ABI::Microsoft::UI::Xaml::Controls::Primitives::ITickBarStatics
    {
        BEGIN_INTERFACE_MAP(TickBarFactory, ctl::BetterCoreObjectActivationFactory)
            INTERFACE_ENTRY(TickBarFactory, ABI::Microsoft::UI::Xaml::Controls::Primitives::ITickBarStatics)
        END_INTERFACE_MAP(TickBarFactory, ctl::BetterCoreObjectActivationFactory)

    public:
        // Factory methods.

        // Static properties.

        // Dependency properties.
        IFACEMETHOD(get_FillProperty)(_Out_ ABI::Microsoft::UI::Xaml::IDependencyProperty** ppValue) override;

        // Attached properties.

        // Static methods.

        // Static events.

    protected:
        HRESULT QueryInterfaceImpl(_In_ REFIID iid, _Outptr_ void** ppObject) override;

        KnownTypeIndex GetTypeIndex() const override
        {
            return KnownTypeIndex::TickBar;
        }


    private:

        // Customized static properties.

        // Customized static  methods.
    };
}
