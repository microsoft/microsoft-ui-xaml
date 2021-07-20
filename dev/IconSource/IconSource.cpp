// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#include "pch.h"
#include "common.h"
#include "SharedHelpers.h"

#include "IconSource.h"
#include <Vector.h>

winrt::IconElement IconSource::CreateIconElement()
{
    auto const element = CreateIconElementCore();
    m_createdIconElements.push_back(winrt::make_weak<winrt::IconElement>(element));
    return element;
}

void IconSource::OnPropertyChanged(const winrt::DependencyPropertyChangedEventArgs& args)
{
    if (auto const iconProp = GetIconElementPropertyCore(args.Property()))
    {
        m_createdIconElements.erase(std::remove_if(m_createdIconElements.begin(), m_createdIconElements.end(),
            [iconProp, newValue = args.NewValue()](winrt::weak_ref<winrt::IconElement> weakElement)
        {
            auto const element = weakElement.get();
            if (element)
            {
                element.SetValue(iconProp, newValue);
            }
            return !element;
        }), m_createdIconElements.end());
    }
}

winrt::DependencyProperty IconSource::GetIconElementPropertyCore(winrt::DependencyProperty sourceProperty)
{
    if (sourceProperty == s_ForegroundProperty)
    {
        return winrt::IconElement::ForegroundProperty();
    }

    return nullptr;
}
