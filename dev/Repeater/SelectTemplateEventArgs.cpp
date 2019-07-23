// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#include "ItemsRepeater.common.h"
#include "SelectTemplateEventArgs.h"
#include <common.h>
#include <pch.h>


#pragma region ISelectTemplateEventArgs

winrt::hstring SelectTemplateEventArgs::TemplateKey()
{
    return m_templateKey;
}

void SelectTemplateEventArgs::TemplateKey(winrt::hstring const& value)
{
    m_templateKey = value;
}

winrt::IInspectable SelectTemplateEventArgs::DataContext()
{
    return m_dataContext.get();
}

winrt::UIElement SelectTemplateEventArgs::Owner()
{
    return m_owner.get();
}

#pragma endregion

void SelectTemplateEventArgs::DataContext(const winrt::IInspectable& value)
{
    m_dataContext.set(value);
}

void SelectTemplateEventArgs::Owner(const winrt::UIElement& value)
{
    m_owner.set(value);
}