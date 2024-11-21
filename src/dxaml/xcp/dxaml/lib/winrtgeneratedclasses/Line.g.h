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

#define __Line_GUID "d0d8a3d1-7559-4055-b841-4dd9684e2438"

namespace DirectUI
{
    class Line;

    class __declspec(novtable) __declspec(uuid(__Line_GUID)) Line:
        public DirectUI::Shape
        , public ABI::Microsoft::UI::Xaml::Shapes::ILine
    {

        INSPECTABLE_CLASS(L"Microsoft.UI.Xaml.Shapes.Line");

        BEGIN_INTERFACE_MAP(Line, DirectUI::Shape)
            INTERFACE_ENTRY(Line, ABI::Microsoft::UI::Xaml::Shapes::ILine)
        END_INTERFACE_MAP(Line, DirectUI::Shape)

    public:
        Line();
        ~Line() override;

        // Event source typedefs.

        KnownTypeIndex GetTypeIndex() const override
        {
            return KnownTypeIndex::Line;
        }

        static XCP_FORCEINLINE KnownTypeIndex GetTypeIndexStatic()
        {
            return KnownTypeIndex::Line;
        }

        // Properties.
        IFACEMETHOD(get_X1)(_Out_ DOUBLE* pValue) override;
        IFACEMETHOD(put_X1)(DOUBLE value) override;
        IFACEMETHOD(get_X2)(_Out_ DOUBLE* pValue) override;
        IFACEMETHOD(put_X2)(DOUBLE value) override;
        IFACEMETHOD(get_Y1)(_Out_ DOUBLE* pValue) override;
        IFACEMETHOD(put_Y1)(DOUBLE value) override;
        IFACEMETHOD(get_Y2)(_Out_ DOUBLE* pValue) override;
        IFACEMETHOD(put_Y2)(DOUBLE value) override;

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
    class __declspec(novtable) LineFactory:
       public ctl::BetterCoreObjectActivationFactory
        , public ABI::Microsoft::UI::Xaml::Shapes::ILineStatics
    {
        BEGIN_INTERFACE_MAP(LineFactory, ctl::BetterCoreObjectActivationFactory)
            INTERFACE_ENTRY(LineFactory, ABI::Microsoft::UI::Xaml::Shapes::ILineStatics)
        END_INTERFACE_MAP(LineFactory, ctl::BetterCoreObjectActivationFactory)

    public:
        // Factory methods.

        // Static properties.

        // Dependency properties.
        IFACEMETHOD(get_X1Property)(_Out_ ABI::Microsoft::UI::Xaml::IDependencyProperty** ppValue) override;
        IFACEMETHOD(get_Y1Property)(_Out_ ABI::Microsoft::UI::Xaml::IDependencyProperty** ppValue) override;
        IFACEMETHOD(get_X2Property)(_Out_ ABI::Microsoft::UI::Xaml::IDependencyProperty** ppValue) override;
        IFACEMETHOD(get_Y2Property)(_Out_ ABI::Microsoft::UI::Xaml::IDependencyProperty** ppValue) override;

        // Attached properties.

        // Static methods.

        // Static events.

    protected:
        HRESULT QueryInterfaceImpl(_In_ REFIID iid, _Outptr_ void** ppObject) override;

        KnownTypeIndex GetTypeIndex() const override
        {
            return KnownTypeIndex::Line;
        }


    private:

        // Customized static properties.

        // Customized static  methods.
    };
}
