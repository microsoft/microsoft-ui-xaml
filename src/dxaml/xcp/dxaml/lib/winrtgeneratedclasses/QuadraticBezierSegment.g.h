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

#include "PathSegment.g.h"

#define __QuadraticBezierSegment_GUID "a0a8c6e2-9ad7-403a-8294-391e5176649a"

namespace DirectUI
{
    class QuadraticBezierSegment;

    class __declspec(novtable) __declspec(uuid(__QuadraticBezierSegment_GUID)) QuadraticBezierSegment:
        public DirectUI::PathSegment
        , public ABI::Microsoft::UI::Xaml::Media::IQuadraticBezierSegment
    {

        INSPECTABLE_CLASS(L"Microsoft.UI.Xaml.Media.QuadraticBezierSegment");

        BEGIN_INTERFACE_MAP(QuadraticBezierSegment, DirectUI::PathSegment)
            INTERFACE_ENTRY(QuadraticBezierSegment, ABI::Microsoft::UI::Xaml::Media::IQuadraticBezierSegment)
        END_INTERFACE_MAP(QuadraticBezierSegment, DirectUI::PathSegment)

    public:
        QuadraticBezierSegment();
        ~QuadraticBezierSegment() override;

        // Event source typedefs.

        KnownTypeIndex GetTypeIndex() const override
        {
            return KnownTypeIndex::QuadraticBezierSegment;
        }

        static XCP_FORCEINLINE KnownTypeIndex GetTypeIndexStatic()
        {
            return KnownTypeIndex::QuadraticBezierSegment;
        }

        // Properties.
        IFACEMETHOD(get_Point1)(_Out_ ABI::Windows::Foundation::Point* pValue) override;
        IFACEMETHOD(put_Point1)(ABI::Windows::Foundation::Point value) override;
        IFACEMETHOD(get_Point2)(_Out_ ABI::Windows::Foundation::Point* pValue) override;
        IFACEMETHOD(put_Point2)(ABI::Windows::Foundation::Point value) override;

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
    class __declspec(novtable) QuadraticBezierSegmentFactory:
       public ctl::BetterCoreObjectActivationFactory
        , public ABI::Microsoft::UI::Xaml::Media::IQuadraticBezierSegmentStatics
    {
        BEGIN_INTERFACE_MAP(QuadraticBezierSegmentFactory, ctl::BetterCoreObjectActivationFactory)
            INTERFACE_ENTRY(QuadraticBezierSegmentFactory, ABI::Microsoft::UI::Xaml::Media::IQuadraticBezierSegmentStatics)
        END_INTERFACE_MAP(QuadraticBezierSegmentFactory, ctl::BetterCoreObjectActivationFactory)

    public:
        // Factory methods.

        // Static properties.

        // Dependency properties.
        IFACEMETHOD(get_Point1Property)(_Out_ ABI::Microsoft::UI::Xaml::IDependencyProperty** ppValue) override;
        IFACEMETHOD(get_Point2Property)(_Out_ ABI::Microsoft::UI::Xaml::IDependencyProperty** ppValue) override;

        // Attached properties.

        // Static methods.

        // Static events.

    protected:
        HRESULT QueryInterfaceImpl(_In_ REFIID iid, _Outptr_ void** ppObject) override;

        KnownTypeIndex GetTypeIndex() const override
        {
            return KnownTypeIndex::QuadraticBezierSegment;
        }


    private:

        // Customized static properties.

        // Customized static  methods.
    };
}
