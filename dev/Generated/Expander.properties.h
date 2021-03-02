// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

// DO NOT EDIT! This file was generated by CustomTasks.DependencyPropertyCodeGen
#pragma once

class ExpanderProperties
{
public:
    ExpanderProperties();

    void ExpandDirection(winrt::ExpandDirection const& value);
    winrt::ExpandDirection ExpandDirection();

    void Header(winrt::IInspectable const& value);
    winrt::IInspectable Header();

    void HeaderTemplate(winrt::DataTemplate const& value);
    winrt::DataTemplate HeaderTemplate();

    void HeaderTemplateSelector(winrt::DataTemplateSelector const& value);
    winrt::DataTemplateSelector HeaderTemplateSelector();

    void IsExpanded(bool value);
    bool IsExpanded();

    void TemplateSettings(winrt::ExpanderTemplateSettings const& value);
    winrt::ExpanderTemplateSettings TemplateSettings();

    static winrt::DependencyProperty ExpandDirectionProperty() { return s_ExpandDirectionProperty; }
    static winrt::DependencyProperty HeaderProperty() { return s_HeaderProperty; }
    static winrt::DependencyProperty HeaderTemplateProperty() { return s_HeaderTemplateProperty; }
    static winrt::DependencyProperty HeaderTemplateSelectorProperty() { return s_HeaderTemplateSelectorProperty; }
    static winrt::DependencyProperty IsExpandedProperty() { return s_IsExpandedProperty; }
    static winrt::DependencyProperty TemplateSettingsProperty() { return s_TemplateSettingsProperty; }

    static GlobalDependencyProperty s_ExpandDirectionProperty;
    static GlobalDependencyProperty s_HeaderProperty;
    static GlobalDependencyProperty s_HeaderTemplateProperty;
    static GlobalDependencyProperty s_HeaderTemplateSelectorProperty;
    static GlobalDependencyProperty s_IsExpandedProperty;
    static GlobalDependencyProperty s_TemplateSettingsProperty;

    winrt::event_token Collapsed(winrt::TypedEventHandler<winrt::Expander, winrt::ExpanderCollapsedEventArgs> const& value);
    void Collapsed(winrt::event_token const& token);
    winrt::event_token Expanded(winrt::TypedEventHandler<winrt::Expander, winrt::ExpanderExpandedEventArgs> const& value);
    void Expanded(winrt::event_token const& token);

    event_source<winrt::TypedEventHandler<winrt::Expander, winrt::ExpanderCollapsedEventArgs>> m_collapsedEventSource;
    event_source<winrt::TypedEventHandler<winrt::Expander, winrt::ExpanderExpandedEventArgs>> m_expandedEventSource;

    static void EnsureProperties();
    static void ClearProperties();

    static void OnExpandDirectionPropertyChanged(
        winrt::DependencyObject const& sender,
        winrt::DependencyPropertyChangedEventArgs const& args);

    static void OnIsExpandedPropertyChanged(
        winrt::DependencyObject const& sender,
        winrt::DependencyPropertyChangedEventArgs const& args);
};
