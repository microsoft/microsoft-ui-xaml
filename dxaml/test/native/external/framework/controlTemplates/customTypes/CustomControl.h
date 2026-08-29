// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#pragma once

namespace Tests { namespace Native { namespace External { namespace Framework { namespace ControlTemplates {

    [::Windows::Foundation::Metadata::WebHostHiddenAttribute]
    [Microsoft::UI::Xaml::Data::BindableAttribute]
    public ref class AbstractCustomControl : public Microsoft::UI::Xaml::Controls::Control
    {
    internal:
        AbstractCustomControl()
        {
            OnApplyTemplateInvoked = false;
        }

    public:
        property bool OnApplyTemplateInvoked;

        Microsoft::UI::Xaml::DependencyObject^ PublicGetTemplateChild(Platform::String^ name)
        {
            return GetTemplateChild(name);
        }

    protected:
        virtual void OnApplyTemplate() override
        {
            Control::OnApplyTemplate();
            OnApplyTemplateInvoked = true;
        }
    };

    [::Windows::Foundation::Metadata::WebHostHiddenAttribute]
    [Microsoft::UI::Xaml::Data::BindableAttribute]
    public ref class CustomControl sealed : public AbstractCustomControl
    {
    public:
        CustomControl()
            : AbstractCustomControl()
        {

        }
    };

} } } } }