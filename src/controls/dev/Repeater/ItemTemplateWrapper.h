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

    // Enables reference tracking for m_dataTemplate using the provided owner's
    // ITrackerHandleManager. This makes the DataTemplate visible to the XAML
    // reference tracker, allowing cycle detection and breaking for:
    //   ItemsRepeater → ItemTemplateWrapper → DataTemplate → RecyclePool → m_owner → ItemsRepeater
    // Same pattern as RecyclePool::ElementInfo and ViewManager::PinnedElementInfo.
    // Must be called after construction (cannot be done in constructor because
    // winrt::make doesn't support ITrackerHandleManager* as first argument).
    void EnableTracking(const ITrackerHandleManager* owner);

#pragma region IElementFactory
    winrt::UIElement GetElement(winrt::ElementFactoryGetArgs const& args);
    void RecycleElement(winrt::ElementFactoryRecycleArgs const& args);
#pragma endregion

private:
    winrt::DataTemplate m_dataTemplate{ nullptr };
    // When EnableTracking is called, this tracker_ref takes over from m_dataTemplate.
    // The tracker_ref uses the caller's ITrackerHandleManager so the XAML reference
    // tracker can walk the reference and break cycles.
    std::optional<tracker_ref<winrt::DataTemplate>> m_trackedDataTemplate;
    bool m_isTracking{ false };

    winrt::DataTemplate GetDataTemplate() const;

    winrt::DataTemplateSelector m_dataTemplateSelector{ nullptr };
};
