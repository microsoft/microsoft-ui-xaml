// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#pragma once

namespace Private::Tests::Framework::XDefer {

    ref class CustomControl;

    extern std::function<void(CustomControl^)>                      s_measureCallback;
    extern std::function<void(CustomControl^)>                      s_onApplyTemplateCallback;

    [::Windows::Foundation::Metadata::WebHostHidden]
    public ref class CustomControl sealed : public Microsoft::UI::Xaml::Controls::ContentControl
    {
    public:
        CustomControl();
        virtual ~CustomControl();
        Microsoft::UI::Xaml::DependencyObject^ PublicGetTemplateChild(Platform::String^ name);
        Microsoft::UI::Xaml::DependencyObject^ GetRealizedInApplyTemplate();

    protected:
        void OnApplyTemplate() override;
        ::Windows::Foundation::Size MeasureOverride(::Windows::Foundation::Size availableSize) override;

    private:
        Microsoft::UI::Xaml::DependencyObject^ m_realizedInApplyTemplate;
    };
}