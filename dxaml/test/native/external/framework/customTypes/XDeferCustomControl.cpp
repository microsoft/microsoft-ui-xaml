// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#include "pch.h"
#include "XDeferCustomControl.h"

namespace Tests::Native::External::Framework::XDefer {

    std::function<void(CustomControl^)>  s_measureCallback;
    std::function<void(CustomControl^)>  s_onApplyTemplateCallback;

    CustomControl::CustomControl()
        : m_realizedInApplyTemplate(nullptr)
    {
    }

    CustomControl::~CustomControl()
    {
    }

    Microsoft::UI::Xaml::DependencyObject^ CustomControl::PublicGetTemplateChild(Platform::String^ name)
    {
        return GetTemplateChild(name);
    }

    Microsoft::UI::Xaml::DependencyObject^ CustomControl::GetRealizedInApplyTemplate()
    {
        return m_realizedInApplyTemplate;
    }

    void CustomControl::OnApplyTemplate()
    {
        Control::OnApplyTemplate();
        m_realizedInApplyTemplate = GetTemplateChild(L"realize_in_applytemplate");

        if (s_onApplyTemplateCallback)
        {
            s_onApplyTemplateCallback(this);
        }
    }

    ::Windows::Foundation::Size CustomControl::MeasureOverride(::Windows::Foundation::Size availableSize)
    {
        ::Windows::Foundation::Size result = Control::MeasureOverride(availableSize);

        if (s_measureCallback)
        {
            s_measureCallback(this);
        }

        return result;
    }
}
