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

#include "Geometry.g.h"

#define __GeometryGroup_GUID "437cd0b2-bd87-48b7-b0c8-89c55eafd57e"

namespace DirectUI
{
    class GeometryGroup;
    class GeometryCollection;

    class __declspec(novtable) __declspec(uuid(__GeometryGroup_GUID)) GeometryGroup:
        public DirectUI::Geometry
        , public ABI::Microsoft::UI::Xaml::Media::IGeometryGroup
    {

        INSPECTABLE_CLASS(L"Microsoft.UI.Xaml.Media.GeometryGroup");

        BEGIN_INTERFACE_MAP(GeometryGroup, DirectUI::Geometry)
            INTERFACE_ENTRY(GeometryGroup, ABI::Microsoft::UI::Xaml::Media::IGeometryGroup)
        END_INTERFACE_MAP(GeometryGroup, DirectUI::Geometry)

    public:
        GeometryGroup();
        ~GeometryGroup() override;

        // Event source typedefs.

        KnownTypeIndex GetTypeIndex() const override
        {
            return KnownTypeIndex::GeometryGroup;
        }

        static XCP_FORCEINLINE KnownTypeIndex GetTypeIndexStatic()
        {
            return KnownTypeIndex::GeometryGroup;
        }

        // Properties.
        IFACEMETHOD(get_Children)(_Outptr_result_maybenull_ ABI::Windows::Foundation::Collections::IVector<ABI::Microsoft::UI::Xaml::Media::Geometry*>** ppValue) override;
        IFACEMETHOD(put_Children)(_In_opt_ ABI::Windows::Foundation::Collections::IVector<ABI::Microsoft::UI::Xaml::Media::Geometry*>* pValue) override;
        IFACEMETHOD(get_FillRule)(_Out_ ABI::Microsoft::UI::Xaml::Media::FillRule* pValue) override;
        IFACEMETHOD(put_FillRule)(ABI::Microsoft::UI::Xaml::Media::FillRule value) override;

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
    class __declspec(novtable) GeometryGroupFactory:
       public ctl::BetterCoreObjectActivationFactory
        , public ABI::Microsoft::UI::Xaml::Media::IGeometryGroupStatics
    {
        BEGIN_INTERFACE_MAP(GeometryGroupFactory, ctl::BetterCoreObjectActivationFactory)
            INTERFACE_ENTRY(GeometryGroupFactory, ABI::Microsoft::UI::Xaml::Media::IGeometryGroupStatics)
        END_INTERFACE_MAP(GeometryGroupFactory, ctl::BetterCoreObjectActivationFactory)

    public:
        // Factory methods.

        // Static properties.

        // Dependency properties.
        IFACEMETHOD(get_FillRuleProperty)(_Out_ ABI::Microsoft::UI::Xaml::IDependencyProperty** ppValue) override;
        IFACEMETHOD(get_ChildrenProperty)(_Out_ ABI::Microsoft::UI::Xaml::IDependencyProperty** ppValue) override;

        // Attached properties.

        // Static methods.

        // Static events.

    protected:
        HRESULT QueryInterfaceImpl(_In_ REFIID iid, _Outptr_ void** ppObject) override;

        KnownTypeIndex GetTypeIndex() const override
        {
            return KnownTypeIndex::GeometryGroup;
        }


    private:

        // Customized static properties.

        // Customized static  methods.
    };
}
