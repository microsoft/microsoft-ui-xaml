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


#define __ToolTipTemplateSettings_GUID "18dd595b-4dc8-46ca-9b78-ca401c0c3d94"

namespace DirectUI
{
    class ToolTipTemplateSettings;

    class __declspec(novtable) __declspec(uuid(__ToolTipTemplateSettings_GUID)) ToolTipTemplateSettings:
        public DirectUI::DependencyObject
        , public ABI::Microsoft::UI::Xaml::Controls::Primitives::IToolTipTemplateSettings
    {

        INSPECTABLE_CLASS(L"Microsoft.UI.Xaml.Controls.Primitives.ToolTipTemplateSettings");

        BEGIN_INTERFACE_MAP(ToolTipTemplateSettings, DirectUI::DependencyObject)
            INTERFACE_ENTRY(ToolTipTemplateSettings, ABI::Microsoft::UI::Xaml::Controls::Primitives::IToolTipTemplateSettings)
        END_INTERFACE_MAP(ToolTipTemplateSettings, DirectUI::DependencyObject)

    public:
        ToolTipTemplateSettings();
        ~ToolTipTemplateSettings() override;

        // Event source typedefs.

        KnownTypeIndex GetTypeIndex() const override
        {
            return KnownTypeIndex::ToolTipTemplateSettings;
        }

        static XCP_FORCEINLINE KnownTypeIndex GetTypeIndexStatic()
        {
            return KnownTypeIndex::ToolTipTemplateSettings;
        }

        // Properties.
        IFACEMETHOD(get_FromHorizontalOffset)(_Out_ DOUBLE* pValue) override;
        _Check_return_ HRESULT put_FromHorizontalOffset(_In_ DOUBLE value);
        IFACEMETHOD(get_FromVerticalOffset)(_Out_ DOUBLE* pValue) override;
        _Check_return_ HRESULT put_FromVerticalOffset(_In_ DOUBLE value);

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
    class __declspec(novtable) ToolTipTemplateSettingsFactory:
       public ctl::AbstractActivationFactory
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
            return KnownTypeIndex::ToolTipTemplateSettings;
        }


    private:

        // Customized static properties.

        // Customized static  methods.
    };
}
