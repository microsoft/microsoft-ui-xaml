// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

// DO NOT EDIT! This file was generated by CustomTasks.DependencyPropertyCodeGen
#pragma once

class TeachingTipTemplateSettingsProperties
{
public:
    TeachingTipTemplateSettingsProperties();

    void IconElement(winrt::IconElement const& value);
    winrt::IconElement IconElement();

    void TopLeftHighlightMargin(winrt::Thickness const& value);
    winrt::Thickness TopLeftHighlightMargin();

    void TopRightHighlightMargin(winrt::Thickness const& value);
    winrt::Thickness TopRightHighlightMargin();

    static winrt::DependencyProperty IconElementProperty() { return s_IconElementProperty; }
    static winrt::DependencyProperty TopLeftHighlightMarginProperty() { return s_TopLeftHighlightMarginProperty; }
    static winrt::DependencyProperty TopRightHighlightMarginProperty() { return s_TopRightHighlightMarginProperty; }

    static GlobalDependencyProperty s_IconElementProperty;
    static GlobalDependencyProperty s_TopLeftHighlightMarginProperty;
    static GlobalDependencyProperty s_TopRightHighlightMarginProperty;

    static void EnsureProperties();
    static void ClearProperties();
};
