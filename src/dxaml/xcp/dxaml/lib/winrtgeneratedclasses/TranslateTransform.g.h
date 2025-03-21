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

#define __TranslateTransform_GUID "42e7c42a-302e-4d8f-a155-f50c9f588e46"

namespace DirectUI
{
    class TranslateTransform;

    class __declspec(novtable) __declspec(uuid(__TranslateTransform_GUID)) TranslateTransform:
        public DirectUI::Transform
        , public ABI::Microsoft::UI::Xaml::Media::ITranslateTransform
    {

        INSPECTABLE_CLASS(L"Microsoft.UI.Xaml.Media.TranslateTransform");

        BEGIN_INTERFACE_MAP(TranslateTransform, DirectUI::Transform)
            INTERFACE_ENTRY(TranslateTransform, ABI::Microsoft::UI::Xaml::Media::ITranslateTransform)
        END_INTERFACE_MAP(TranslateTransform, DirectUI::Transform)

    public:
        TranslateTransform();
        ~TranslateTransform() override;

        // Event source typedefs.

        KnownTypeIndex GetTypeIndex() const override
        {
            return KnownTypeIndex::TranslateTransform;
        }

        static XCP_FORCEINLINE KnownTypeIndex GetTypeIndexStatic()
        {
            return KnownTypeIndex::TranslateTransform;
        }

        // Properties.
        IFACEMETHOD(get_X)(_Out_ DOUBLE* pValue) override;
        IFACEMETHOD(put_X)(DOUBLE value) override;
        _Check_return_ HRESULT get_XAnimation(_Outptr_result_maybenull_ IInspectable** ppValue);
        _Check_return_ HRESULT put_XAnimation(_In_opt_ IInspectable* pValue);
        IFACEMETHOD(get_Y)(_Out_ DOUBLE* pValue) override;
        IFACEMETHOD(put_Y)(DOUBLE value) override;
        _Check_return_ HRESULT get_YAnimation(_Outptr_result_maybenull_ IInspectable** ppValue);
        _Check_return_ HRESULT put_YAnimation(_In_opt_ IInspectable* pValue);

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
    class __declspec(novtable) TranslateTransformFactory:
       public ctl::BetterCoreObjectActivationFactory
        , public ABI::Microsoft::UI::Xaml::Media::ITranslateTransformStatics
    {
        BEGIN_INTERFACE_MAP(TranslateTransformFactory, ctl::BetterCoreObjectActivationFactory)
            INTERFACE_ENTRY(TranslateTransformFactory, ABI::Microsoft::UI::Xaml::Media::ITranslateTransformStatics)
        END_INTERFACE_MAP(TranslateTransformFactory, ctl::BetterCoreObjectActivationFactory)

    public:
        // Factory methods.

        // Static properties.

        // Dependency properties.
        IFACEMETHOD(get_XProperty)(_Out_ ABI::Microsoft::UI::Xaml::IDependencyProperty** ppValue) override;
        
        IFACEMETHOD(get_YProperty)(_Out_ ABI::Microsoft::UI::Xaml::IDependencyProperty** ppValue) override;
        

        // Attached properties.

        // Static methods.

        // Static events.

    protected:
        HRESULT QueryInterfaceImpl(_In_ REFIID iid, _Outptr_ void** ppObject) override;

        KnownTypeIndex GetTypeIndex() const override
        {
            return KnownTypeIndex::TranslateTransform;
        }


    private:

        // Customized static properties.

        // Customized static  methods.
    };
}
