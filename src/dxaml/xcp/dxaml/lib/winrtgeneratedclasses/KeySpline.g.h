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


#define __KeySpline_GUID "1403230b-bacb-4fc1-a85a-f624db3a9317"

namespace DirectUI
{
    class KeySpline;

    class __declspec(novtable) __declspec(uuid(__KeySpline_GUID)) KeySpline:
        public DirectUI::DependencyObject
        , public ABI::Microsoft::UI::Xaml::Media::Animation::IKeySpline
    {

        INSPECTABLE_CLASS(L"Microsoft.UI.Xaml.Media.Animation.KeySpline");

        BEGIN_INTERFACE_MAP(KeySpline, DirectUI::DependencyObject)
            INTERFACE_ENTRY(KeySpline, ABI::Microsoft::UI::Xaml::Media::Animation::IKeySpline)
        END_INTERFACE_MAP(KeySpline, DirectUI::DependencyObject)

    public:
        KeySpline();
        ~KeySpline() override;

        // Event source typedefs.

        KnownTypeIndex GetTypeIndex() const override
        {
            return KnownTypeIndex::KeySpline;
        }

        static XCP_FORCEINLINE KnownTypeIndex GetTypeIndexStatic()
        {
            return KnownTypeIndex::KeySpline;
        }

        // Properties.
        IFACEMETHOD(get_ControlPoint1)(_Out_ ABI::Windows::Foundation::Point* pValue) override;
        IFACEMETHOD(put_ControlPoint1)(ABI::Windows::Foundation::Point value) override;
        IFACEMETHOD(get_ControlPoint2)(_Out_ ABI::Windows::Foundation::Point* pValue) override;
        IFACEMETHOD(put_ControlPoint2)(ABI::Windows::Foundation::Point value) override;

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
    class __declspec(novtable) KeySplineFactory:
       public ctl::BetterCoreObjectActivationFactory
    {

    public:
        // Factory methods.

        // Static properties.

        // Dependency properties.
        
        

        // Attached properties.

        // Static methods.

        // Static events.

    protected:

        KnownTypeIndex GetTypeIndex() const override
        {
            return KnownTypeIndex::KeySpline;
        }


    private:

        // Customized static properties.

        // Customized static  methods.
    };
}
