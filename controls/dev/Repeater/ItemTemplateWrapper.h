// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#pragma once

#include "common.h"

class ItemTemplateWrapper :
    public winrt::implements<ItemTemplateWrapper, winrt::IElementFactory>
{
public:
    ItemTemplateWrapper(winrt::DataTemplate const& dataTemplate);
    ItemTemplateWrapper(winrt::DataTemplateSelector const& dataTemplateSelector);

    winrt::DataTemplate Template();
    void Template(winrt::DataTemplate const& value);

    winrt::DataTemplateSelector TemplateSelector();
    void TemplateSelector(winrt::DataTemplateSelector const& value);

    void EnableTracking(const ITrackerHandleManager* owner);
    void ResetTracking() noexcept;

#pragma region IElementFactory
    winrt::UIElement GetElement(winrt::ElementFactoryGetArgs const& args);
    void RecycleElement(winrt::ElementFactoryRecycleArgs const& args);
#pragma endregion

private:
    winrt::DataTemplate m_dataTemplate{ nullptr };
    std::optional<tracker_ref<winrt::DataTemplate>> m_trackedDataTemplate;
    std::optional<tracker_ref<winrt::DataTemplateSelector>> m_trackedDataTemplateSelector;
    bool m_isTrackingReset{ false };

    winrt::DataTemplate GetDataTemplate() const;
    winrt::DataTemplateSelector GetDataTemplateSelector() const;

    winrt::DataTemplateSelector m_dataTemplateSelector{ nullptr };
};
