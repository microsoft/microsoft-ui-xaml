// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

// DO NOT EDIT! This file was generated by CustomTasks.DependencyPropertyCodeGen
#pragma once

class RadioButtonsTestHooksProperties
{
public:
    RadioButtonsTestHooksProperties();



    winrt::event_token LayoutChanged(winrt::TypedEventHandler<winrt::RadioButtons, winrt::IInspectable> const& value);
    void LayoutChanged(winrt::event_token const& token);

    event_source<winrt::TypedEventHandler<winrt::RadioButtons, winrt::IInspectable>> m_layoutChangedEventSource;

    static void EnsureProperties();
    static void ClearProperties();
};
