// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#pragma once

#include "SelectTemplateEventArgs.g.h"

class SelectTemplateEventArgs :
    public ReferenceTracker<SelectTemplateEventArgs, winrt::implementation::SelectTemplateEventArgsT, winrt::composable, winrt::composing>
{
public:

#pragma region ISelectTemplateEventArgs
    winrt::hstring TemplateKey();
    void TemplateKey(winrt::hstring const& value);

    winrt::IInspectable DataContext();
    winrt::UIElement Owner();
#pragma endregion

    void DataContext(winrt::IInspectable const& value);
    void Owner(const winrt::UIElement& value);

private:
    winrt::hstring m_templateKey{};
    tracker_ref<winrt::IInspectable> m_dataContext{ this };
    tracker_ref<winrt::UIElement> m_owner{ this };
};