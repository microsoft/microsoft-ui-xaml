// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#pragma once

namespace TextBoxPlaceholderTextHelper
{
    _Check_return_ HRESULT SetupPlaceholderTextBlockDescribedBy(
        _In_ ctl::ComPtr<xaml::IUIElement> spOwner);

    _Check_return_ HRESULT ClearPlaceholderTextBlockDescribedBy(
        _In_ xaml::IUIElement* textBox);

    _Check_return_ HRESULT UpdatePlaceholderTextPresenterVisibility(
        _In_ xaml::IUIElement* textBox,
        _In_ xaml::IUIElement* placeholderTextAsUIElement,
        _In_ bool isEnabled);

    bool ShouldMakePlaceholderTextVisible(
        _In_opt_ xaml::IUIElement* placeholderTextAsUIElement,
        _In_opt_ xaml::IUIElement* textControlAsUIElement
    );

    bool ShouldCollapsePlaceholderText(
        _In_opt_ xaml::IUIElement* placeholderTextAsUIElement,
        _In_opt_ xaml::IUIElement* textControlAsUIElement
    );

}