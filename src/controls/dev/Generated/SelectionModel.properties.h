// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

// DO NOT EDIT! This file was generated by CustomTasks.DependencyPropertyCodeGen
#pragma once

class SelectionModelProperties
{
public:
    SelectionModelProperties();



    winrt::event_token ChildrenRequested(winrt::TypedEventHandler<winrt::SelectionModel, winrt::SelectionModelChildrenRequestedEventArgs> const& value);
    void ChildrenRequested(winrt::event_token const& token);
    winrt::event_token PropertyChanged(winrt::PropertyChangedEventHandler const& value);
    void PropertyChanged(winrt::event_token const& token);
    winrt::event_token SelectionChanged(winrt::TypedEventHandler<winrt::SelectionModel, winrt::SelectionModelSelectionChangedEventArgs> const& value);
    void SelectionChanged(winrt::event_token const& token);

    event_source<winrt::TypedEventHandler<winrt::SelectionModel, winrt::SelectionModelChildrenRequestedEventArgs>> m_childrenRequestedEventSource;
    event_source<winrt::PropertyChangedEventHandler> m_propertyChangedEventSource;
    event_source<winrt::TypedEventHandler<winrt::SelectionModel, winrt::SelectionModelSelectionChangedEventArgs>> m_selectionChangedEventSource;

    static void EnsureProperties();
    static void ClearProperties();
};
