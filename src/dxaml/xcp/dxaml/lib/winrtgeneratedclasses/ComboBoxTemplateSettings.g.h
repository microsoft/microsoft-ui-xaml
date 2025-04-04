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


#define __ComboBoxTemplateSettings_GUID "39993b65-1c2a-45e3-b7ab-a4e7ddadd6aa"

namespace DirectUI
{
    class ComboBoxTemplateSettings;

    class __declspec(novtable) __declspec(uuid(__ComboBoxTemplateSettings_GUID)) ComboBoxTemplateSettings:
        public DirectUI::DependencyObject
        , public ABI::Microsoft::UI::Xaml::Controls::Primitives::IComboBoxTemplateSettings
    {

        INSPECTABLE_CLASS(L"Microsoft.UI.Xaml.Controls.Primitives.ComboBoxTemplateSettings");

        BEGIN_INTERFACE_MAP(ComboBoxTemplateSettings, DirectUI::DependencyObject)
            INTERFACE_ENTRY(ComboBoxTemplateSettings, ABI::Microsoft::UI::Xaml::Controls::Primitives::IComboBoxTemplateSettings)
        END_INTERFACE_MAP(ComboBoxTemplateSettings, DirectUI::DependencyObject)

    public:
        ComboBoxTemplateSettings();
        ~ComboBoxTemplateSettings() override;

        // Event source typedefs.

        KnownTypeIndex GetTypeIndex() const override
        {
            return KnownTypeIndex::ComboBoxTemplateSettings;
        }

        static XCP_FORCEINLINE KnownTypeIndex GetTypeIndexStatic()
        {
            return KnownTypeIndex::ComboBoxTemplateSettings;
        }

        // Properties.
        IFACEMETHOD(get_DropDownClosedHeight)(_Out_ DOUBLE* pValue) override;
        _Check_return_ HRESULT put_DropDownClosedHeight(DOUBLE value);
        IFACEMETHOD(get_DropDownContentMinWidth)(_Out_ DOUBLE* pValue) override;
        _Check_return_ HRESULT put_DropDownContentMinWidth(DOUBLE value);
        IFACEMETHOD(get_DropDownOffset)(_Out_ DOUBLE* pValue) override;
        _Check_return_ HRESULT put_DropDownOffset(DOUBLE value);
        IFACEMETHOD(get_DropDownOpenedHeight)(_Out_ DOUBLE* pValue) override;
        _Check_return_ HRESULT put_DropDownOpenedHeight(DOUBLE value);
        IFACEMETHOD(get_SelectedItemDirection)(_Out_ ABI::Microsoft::UI::Xaml::Controls::Primitives::AnimationDirection* pValue) override;
        _Check_return_ HRESULT put_SelectedItemDirection(ABI::Microsoft::UI::Xaml::Controls::Primitives::AnimationDirection value);

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
    class __declspec(novtable) ComboBoxTemplateSettingsFactory:
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
            return KnownTypeIndex::ComboBoxTemplateSettings;
        }


    private:

        // Customized static properties.

        // Customized static  methods.
    };
}
