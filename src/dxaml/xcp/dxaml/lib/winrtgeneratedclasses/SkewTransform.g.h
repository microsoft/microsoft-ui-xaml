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

#include "Transform.g.h"

#define __SkewTransform_GUID "1e6ddde7-b10f-4066-855b-e5e0838ce0c6"

namespace DirectUI
{
    class SkewTransform;

    class __declspec(novtable) __declspec(uuid(__SkewTransform_GUID)) SkewTransform:
        public DirectUI::Transform
        , public ABI::Microsoft::UI::Xaml::Media::ISkewTransform
    {

        INSPECTABLE_CLASS(L"Microsoft.UI.Xaml.Media.SkewTransform");

        BEGIN_INTERFACE_MAP(SkewTransform, DirectUI::Transform)
            INTERFACE_ENTRY(SkewTransform, ABI::Microsoft::UI::Xaml::Media::ISkewTransform)
        END_INTERFACE_MAP(SkewTransform, DirectUI::Transform)

    public:
        SkewTransform();
        ~SkewTransform() override;

        // Event source typedefs.

        KnownTypeIndex GetTypeIndex() const override
        {
            return KnownTypeIndex::SkewTransform;
        }

        static XCP_FORCEINLINE KnownTypeIndex GetTypeIndexStatic()
        {
            return KnownTypeIndex::SkewTransform;
        }

        // Properties.
        IFACEMETHOD(get_AngleX)(_Out_ DOUBLE* pValue) override;
        IFACEMETHOD(put_AngleX)(DOUBLE value) override;
        _Check_return_ HRESULT get_AngleXAnimation(_Outptr_result_maybenull_ IInspectable** ppValue);
        _Check_return_ HRESULT put_AngleXAnimation(_In_opt_ IInspectable* pValue);
        IFACEMETHOD(get_AngleY)(_Out_ DOUBLE* pValue) override;
        IFACEMETHOD(put_AngleY)(DOUBLE value) override;
        _Check_return_ HRESULT get_AngleYAnimation(_Outptr_result_maybenull_ IInspectable** ppValue);
        _Check_return_ HRESULT put_AngleYAnimation(_In_opt_ IInspectable* pValue);
        IFACEMETHOD(get_CenterX)(_Out_ DOUBLE* pValue) override;
        IFACEMETHOD(put_CenterX)(DOUBLE value) override;
        _Check_return_ HRESULT get_CenterXAnimation(_Outptr_result_maybenull_ IInspectable** ppValue);
        _Check_return_ HRESULT put_CenterXAnimation(_In_opt_ IInspectable* pValue);
        IFACEMETHOD(get_CenterY)(_Out_ DOUBLE* pValue) override;
        IFACEMETHOD(put_CenterY)(DOUBLE value) override;
        _Check_return_ HRESULT get_CenterYAnimation(_Outptr_result_maybenull_ IInspectable** ppValue);
        _Check_return_ HRESULT put_CenterYAnimation(_In_opt_ IInspectable* pValue);

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
    class __declspec(novtable) SkewTransformFactory:
       public ctl::BetterCoreObjectActivationFactory
        , public ABI::Microsoft::UI::Xaml::Media::ISkewTransformStatics
    {
        BEGIN_INTERFACE_MAP(SkewTransformFactory, ctl::BetterCoreObjectActivationFactory)
            INTERFACE_ENTRY(SkewTransformFactory, ABI::Microsoft::UI::Xaml::Media::ISkewTransformStatics)
        END_INTERFACE_MAP(SkewTransformFactory, ctl::BetterCoreObjectActivationFactory)

    public:
        // Factory methods.

        // Static properties.

        // Dependency properties.
        IFACEMETHOD(get_CenterXProperty)(_Out_ ABI::Microsoft::UI::Xaml::IDependencyProperty** ppValue) override;
        
        IFACEMETHOD(get_CenterYProperty)(_Out_ ABI::Microsoft::UI::Xaml::IDependencyProperty** ppValue) override;
        
        IFACEMETHOD(get_AngleXProperty)(_Out_ ABI::Microsoft::UI::Xaml::IDependencyProperty** ppValue) override;
        
        IFACEMETHOD(get_AngleYProperty)(_Out_ ABI::Microsoft::UI::Xaml::IDependencyProperty** ppValue) override;
        

        // Attached properties.

        // Static methods.

        // Static events.

    protected:
        HRESULT QueryInterfaceImpl(_In_ REFIID iid, _Outptr_ void** ppObject) override;

        KnownTypeIndex GetTypeIndex() const override
        {
            return KnownTypeIndex::SkewTransform;
        }


    private:

        // Customized static properties.

        // Customized static  methods.
    };
}
