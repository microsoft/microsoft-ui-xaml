// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#pragma once
#include "AutoSuggestBoxHelper.g.h"
#include "AutoSuggestBoxHelper.properties.h"

class AutoSuggestBoxHelper
    : public winrt::implementation::AutoSuggestBoxHelperT<AutoSuggestBoxHelper>
    , public AutoSuggestBoxHelperProperties
{
public:
    AutoSuggestBoxHelper();

    static void EnsureProperties();
    static void ClearProperties();

    static winrt::DependencyProperty AutoSuggestEventRevokersProperty() { return s_AutoSuggestEventRevokersProperty; }
    static void OnKeepInteriorCornersSquarePropertyChanged(const winrt::DependencyObject& sender, const winrt::DependencyPropertyChangedEventArgs& args);

    static GlobalDependencyProperty s_AutoSuggestEventRevokersProperty;

private:
    static void OnAutoSuggestBoxLoaded(const winrt::IInspectable& sender, const winrt::IInspectable& args);

    static void UpdateCornerRadius(const winrt::AutoSuggestBox& autoSuggestBox, bool isPopupOpen);
    static bool IsPopupOpenDown(const winrt::AutoSuggestBox& autoSuggestBox);
};

class AutoSuggestEventRevokers
    : public winrt::implements<AutoSuggestEventRevokers, winrt::Windows::Foundation::IInspectable>
{
public:
    winrt::AutoSuggestBox::Loaded_revoker m_autoSuggestBoxLoadedRevoker;
    winrt::Popup::Opened_revoker m_popupOpenedRevoker;
    winrt::Popup::Closed_revoker m_popupClosedRevoker;
};
