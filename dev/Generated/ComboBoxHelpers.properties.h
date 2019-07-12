// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

// DO NOT EDIT! This file was generated by CustomTasks.DependencyPropertyCodeGen
#pragma once

class ComboBoxHelpersProperties
{
public:
    ComboBoxHelpersProperties();

    static void SetApplyDynamicCornerRadius(winrt::UIElement const& target, bool value);
    static bool GetApplyDynamicCornerRadius(winrt::UIElement const& target);

    static winrt::DependencyProperty ApplyDynamicCornerRadiusProperty() { return s_ApplyDynamicCornerRadiusProperty; }

    static GlobalDependencyProperty s_ApplyDynamicCornerRadiusProperty;

    static void EnsureProperties();
    static void ClearProperties();
};
