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

#define __Canvas_GUID "9478cc51-d00b-47fe-8cca-f845cc070b7a"

namespace DirectUI
{
    class Canvas;

    class __declspec(novtable) __declspec(uuid(__Canvas_GUID)) Canvas:
        public DirectUI::Panel
        , public ABI::Microsoft::UI::Xaml::Controls::ICanvas
    {

        INSPECTABLE_CLASS(L"Microsoft.UI.Xaml.Controls.Canvas");

        BEGIN_INTERFACE_MAP(Canvas, DirectUI::Panel)
            INTERFACE_ENTRY(Canvas, ABI::Microsoft::UI::Xaml::Controls::ICanvas)
        END_INTERFACE_MAP(Canvas, DirectUI::Panel)

    public:
        Canvas();
        ~Canvas() override;

        // Event source typedefs.

        KnownTypeIndex GetTypeIndex() const override
        {
            return KnownTypeIndex::Canvas;
        }

        static XCP_FORCEINLINE KnownTypeIndex GetTypeIndexStatic()
        {
            return KnownTypeIndex::Canvas;
        }

        // Properties.

        // Events.

        // Methods.


    protected:
        HRESULT QueryInterfaceImpl(_In_ REFIID iid, _Outptr_ void** ppObject) override;

    private:
        // Customized properties.

        // Customized methods.

        // Fields.
    };
}


namespace DirectUI
{
    // Note that the ordering of the base types here is important - the base factory comes first, followed by all the
    // interfaces specific to this type.  By doing this, we allow every Factory's CreateInstance method to be more
    // COMDAT-folding-friendly.  Because this ensures that the first vfptr contains GetTypeIndex, it means that all
    // CreateInstance functions with the same base factory generate the same assembly instructions and thus will
    // fold together.  This is significant for binary size in Microsoft.UI.Xaml.dll so change this only with great
    // care.
    class __declspec(novtable) CanvasFactory:
       public ctl::BetterAggregableCoreObjectActivationFactory
        , public ABI::Microsoft::UI::Xaml::Controls::ICanvasFactory
        , public ABI::Microsoft::UI::Xaml::Controls::ICanvasStatics
    {
        BEGIN_INTERFACE_MAP(CanvasFactory, ctl::BetterAggregableCoreObjectActivationFactory)
            INTERFACE_ENTRY(CanvasFactory, ABI::Microsoft::UI::Xaml::Controls::ICanvasFactory)
            INTERFACE_ENTRY(CanvasFactory, ABI::Microsoft::UI::Xaml::Controls::ICanvasStatics)
        END_INTERFACE_MAP(CanvasFactory, ctl::BetterAggregableCoreObjectActivationFactory)

    public:
        // Factory methods.
        IFACEMETHOD(CreateInstance)(_In_opt_ IInspectable* pOuter, _Outptr_ IInspectable** ppInner, _Outptr_ ABI::Microsoft::UI::Xaml::Controls::ICanvas** ppInstance);

        // Static properties.

        // Dependency properties.

        // Attached properties.
        static _Check_return_ HRESULT GetLeftStatic(_In_ ABI::Microsoft::UI::Xaml::IUIElement* pElement, _Out_ DOUBLE* pLength);
        static _Check_return_ HRESULT SetLeftStatic(_In_ ABI::Microsoft::UI::Xaml::IUIElement* pElement, DOUBLE length);
        IFACEMETHOD(get_LeftProperty)(_Out_ ABI::Microsoft::UI::Xaml::IDependencyProperty** ppValue) override;
        IFACEMETHOD(GetLeft)(_In_ ABI::Microsoft::UI::Xaml::IUIElement* pElement, _Out_ DOUBLE* pLength);
        IFACEMETHOD(SetLeft)(_In_ ABI::Microsoft::UI::Xaml::IUIElement* pElement, DOUBLE length);
        static _Check_return_ HRESULT GetTopStatic(_In_ ABI::Microsoft::UI::Xaml::IUIElement* pElement, _Out_ DOUBLE* pLength);
        static _Check_return_ HRESULT SetTopStatic(_In_ ABI::Microsoft::UI::Xaml::IUIElement* pElement, DOUBLE length);
        IFACEMETHOD(get_TopProperty)(_Out_ ABI::Microsoft::UI::Xaml::IDependencyProperty** ppValue) override;
        IFACEMETHOD(GetTop)(_In_ ABI::Microsoft::UI::Xaml::IUIElement* pElement, _Out_ DOUBLE* pLength);
        IFACEMETHOD(SetTop)(_In_ ABI::Microsoft::UI::Xaml::IUIElement* pElement, DOUBLE length);
        static _Check_return_ HRESULT GetZIndexStatic(_In_ ABI::Microsoft::UI::Xaml::IUIElement* pElement, _Out_ INT* pValue);
        static _Check_return_ HRESULT SetZIndexStatic(_In_ ABI::Microsoft::UI::Xaml::IUIElement* pElement, INT value);
        IFACEMETHOD(get_ZIndexProperty)(_Out_ ABI::Microsoft::UI::Xaml::IDependencyProperty** ppValue) override;
        IFACEMETHOD(GetZIndex)(_In_ ABI::Microsoft::UI::Xaml::IUIElement* pElement, _Out_ INT* pValue);
        IFACEMETHOD(SetZIndex)(_In_ ABI::Microsoft::UI::Xaml::IUIElement* pElement, INT value);

        // Static methods.

        // Static events.

    protected:
        HRESULT QueryInterfaceImpl(_In_ REFIID iid, _Outptr_ void** ppObject) override;

        KnownTypeIndex GetTypeIndex() const override
        {
            return KnownTypeIndex::Canvas;
        }


    private:

        // Customized static properties.

        // Customized static  methods.
    };
}
