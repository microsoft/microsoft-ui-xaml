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




namespace DirectUI
{
    // Note that the ordering of the base types here is important - the base factory comes first, followed by all the
    // interfaces specific to this type.  By doing this, we allow every Factory's CreateInstance method to be more
    // COMDAT-folding-friendly.  Because this ensures that the first vfptr contains GetTypeIndex, it means that all
    // CreateInstance functions with the same base factory generate the same assembly instructions and thus will
    // fold together.  This is significant for binary size in Microsoft.UI.Xaml.dll so change this only with great
    // care.
    class __declspec(novtable) MediaTransportControlsHelperFactory:
       public ctl::AbstractActivationFactory
        , public ABI::Microsoft::UI::Xaml::Controls::IMediaTransportControlsHelperStatics
    {
        BEGIN_INTERFACE_MAP(MediaTransportControlsHelperFactory, ctl::AbstractActivationFactory)
            INTERFACE_ENTRY(MediaTransportControlsHelperFactory, ABI::Microsoft::UI::Xaml::Controls::IMediaTransportControlsHelperStatics)
        END_INTERFACE_MAP(MediaTransportControlsHelperFactory, ctl::AbstractActivationFactory)

    public:
        // Factory methods.

        // Static properties.

        // Dependency properties.

        // Attached properties.
        static _Check_return_ HRESULT GetDropoutOrderStatic(_In_ ABI::Microsoft::UI::Xaml::IUIElement* pElement, _Out_ ABI::Windows::Foundation::IReference<INT>** ppValue);
        static _Check_return_ HRESULT SetDropoutOrderStatic(_In_ ABI::Microsoft::UI::Xaml::IUIElement* pElement, ABI::Windows::Foundation::IReference<INT>* pValue);
        IFACEMETHOD(get_DropoutOrderProperty)(_Out_ ABI::Microsoft::UI::Xaml::IDependencyProperty** ppValue) override;
        IFACEMETHOD(GetDropoutOrder)(_In_ ABI::Microsoft::UI::Xaml::IUIElement* pElement, _Out_ ABI::Windows::Foundation::IReference<INT>** ppValue);
        IFACEMETHOD(SetDropoutOrder)(_In_ ABI::Microsoft::UI::Xaml::IUIElement* pElement, ABI::Windows::Foundation::IReference<INT>* pValue);

        // Static methods.

        // Static events.

    protected:
        HRESULT QueryInterfaceImpl(_In_ REFIID iid, _Outptr_ void** ppObject) override;

        KnownTypeIndex GetTypeIndex() const override
        {
            return KnownTypeIndex::MediaTransportControlsHelper;
        }


    private:

        // Customized static properties.

        // Customized static  methods.
    };
}
