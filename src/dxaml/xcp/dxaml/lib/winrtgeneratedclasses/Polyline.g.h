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

#include "Shape.g.h"

#define __Polyline_GUID "0d7b50f1-4277-44c1-995c-a4ba0d863140"

namespace DirectUI
{
    class Polyline;
    class PointCollection;

    class __declspec(novtable) __declspec(uuid(__Polyline_GUID)) Polyline:
        public DirectUI::Shape
        , public ABI::Microsoft::UI::Xaml::Shapes::IPolyline
    {

        INSPECTABLE_CLASS(L"Microsoft.UI.Xaml.Shapes.Polyline");

        BEGIN_INTERFACE_MAP(Polyline, DirectUI::Shape)
            INTERFACE_ENTRY(Polyline, ABI::Microsoft::UI::Xaml::Shapes::IPolyline)
        END_INTERFACE_MAP(Polyline, DirectUI::Shape)

    public:
        Polyline();
        ~Polyline() override;

        // Event source typedefs.

        KnownTypeIndex GetTypeIndex() const override
        {
            return KnownTypeIndex::Polyline;
        }

        static XCP_FORCEINLINE KnownTypeIndex GetTypeIndexStatic()
        {
            return KnownTypeIndex::Polyline;
        }

        // Properties.
        IFACEMETHOD(get_FillRule)(_Out_ ABI::Microsoft::UI::Xaml::Media::FillRule* pValue) override;
        IFACEMETHOD(put_FillRule)(ABI::Microsoft::UI::Xaml::Media::FillRule value) override;
        IFACEMETHOD(get_Points)(_Outptr_result_maybenull_ ABI::Windows::Foundation::Collections::IVector<ABI::Windows::Foundation::Point>** ppValue) override;
        IFACEMETHOD(put_Points)(_In_opt_ ABI::Windows::Foundation::Collections::IVector<ABI::Windows::Foundation::Point>* pValue) override;

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
    class __declspec(novtable) PolylineFactory:
       public ctl::BetterCoreObjectActivationFactory
        , public ABI::Microsoft::UI::Xaml::Shapes::IPolylineStatics
    {
        BEGIN_INTERFACE_MAP(PolylineFactory, ctl::BetterCoreObjectActivationFactory)
            INTERFACE_ENTRY(PolylineFactory, ABI::Microsoft::UI::Xaml::Shapes::IPolylineStatics)
        END_INTERFACE_MAP(PolylineFactory, ctl::BetterCoreObjectActivationFactory)

    public:
        // Factory methods.

        // Static properties.

        // Dependency properties.
        IFACEMETHOD(get_FillRuleProperty)(_Out_ ABI::Microsoft::UI::Xaml::IDependencyProperty** ppValue) override;
        IFACEMETHOD(get_PointsProperty)(_Out_ ABI::Microsoft::UI::Xaml::IDependencyProperty** ppValue) override;

        // Attached properties.

        // Static methods.

        // Static events.

    protected:
        HRESULT QueryInterfaceImpl(_In_ REFIID iid, _Outptr_ void** ppObject) override;

        KnownTypeIndex GetTypeIndex() const override
        {
            return KnownTypeIndex::Polyline;
        }


    private:

        // Customized static properties.

        // Customized static  methods.
    };
}
