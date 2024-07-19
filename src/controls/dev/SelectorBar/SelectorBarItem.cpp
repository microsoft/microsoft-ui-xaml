// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#include "pch.h"
#include "common.h"
#include "SelectorBarItem.h"
#include "SelectorBarItemAutomationPeer.h"
#include "SelectorBarTrace.h"

SelectorBarItem::SelectorBarItem()
{
    SELECTORBAR_TRACE_INFO(nullptr, TRACE_MSG_METH, METH_NAME, this);

    EnsureProperties();
    SetDefaultStyleKey(this);
}

SelectorBarItem::~SelectorBarItem()
{
    SELECTORBAR_TRACE_INFO(nullptr, TRACE_MSG_METH, METH_NAME, this);
}

winrt::AutomationPeer SelectorBarItem::OnCreateAutomationPeer()
{
    return winrt::make<SelectorBarItemAutomationPeer>(*this);
}

void SelectorBarItem::OnApplyTemplate()
{
    SELECTORBAR_TRACE_VERBOSE(*this, TRACE_MSG_METH, METH_NAME, this);

    ItemContainer::OnApplyTemplate();

    winrt::IControlProtected controlProtected{ *this };

    m_iconVisual.set(GetTemplateChildT<winrt::ContentPresenter>(s_iconVisualPartName, controlProtected));
    m_textVisual.set(GetTemplateChildT<winrt::TextBlock>(s_textVisualPartName, controlProtected));

    UpdatePartsVisibility(true /*isForIcon*/, true /*isForText*/);
}

void SelectorBarItem::OnPropertyChanged(winrt::DependencyPropertyChangedEventArgs const& args)
{
    const auto dependencyProperty = args.Property();
    const bool isForIcon = dependencyProperty == s_IconProperty;
    const bool isForText = dependencyProperty == s_TextProperty;

#ifdef DBG
    SELECTORBAR_TRACE_VERBOSE(nullptr, L"%s[%p](property: %s)\n", METH_NAME, this, DependencyPropertyToString(dependencyProperty).c_str());
#endif

    if (isForIcon || isForText)
    {
        UpdatePartsVisibility(isForIcon, isForText);
    }
}

void SelectorBarItem::UpdatePartsVisibility(bool isForIcon, bool isForText)
{
    MUX_ASSERT(isForIcon || isForText);

    SELECTORBAR_TRACE_VERBOSE(*this, TRACE_MSG_METH_INT_INT, METH_NAME, this, isForIcon, isForText);

    winrt::UIElement iconParent{ nullptr }, textParent{ nullptr };
    bool hasIcon{}, hasText{};

    if (auto&& iconVisual = m_iconVisual.get())
    {
        iconParent = winrt::VisualTreeHelper::GetParent(iconVisual).as<winrt::UIElement>();
        hasIcon = Icon() != nullptr;

        if (isForIcon)
        {
            iconVisual.Visibility(hasIcon ? winrt::Visibility::Visible : winrt::Visibility::Collapsed);
        }
    }

    if (auto&& textVisual = m_textVisual.get())
    {
        textParent = winrt::VisualTreeHelper::GetParent(textVisual).as<winrt::UIElement>();
        hasText = !(Text().empty());

        if (isForText)
        {
            textVisual.Visibility(hasText ? winrt::Visibility::Visible : winrt::Visibility::Collapsed);
        }
    }

    if (iconParent && iconParent == textParent)
    {
        // When there is no icon and no text, and the PART_IconVisual / PART_TextVisual have a common parent (an unnamed
        // StackPanel by default), that common parent is collapsed so it does not take any real estate because of its Margin.
        textParent.Visibility((hasIcon || hasText) ? winrt::Visibility::Visible : winrt::Visibility::Collapsed);
    }
}

#ifdef DBG

winrt::hstring SelectorBarItem::DependencyPropertyToString(
    const winrt::IDependencyProperty& dependencyProperty)
{
    if (dependencyProperty == s_IconProperty)
    {
        return L"Icon";
    }
    else if (dependencyProperty == s_TextProperty)
    {
        return L"Text";
    }
    else
    {
        return L"UNKNOWN";
    }
}

#endif
