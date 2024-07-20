// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#pragma once
#include "ComboBoxHelper.g.h"
#include "ComboBoxHelper.properties.h"

class ComboBoxHelper
    : public winrt::implementation::ComboBoxHelperT<ComboBoxHelper>
    , public ComboBoxHelperProperties
{
public:
    ComboBoxHelper();

    static void EnsureProperties();
    static void ClearProperties();

    static winrt::DependencyProperty DropDownEventRevokersProperty() { return s_DropDownEventRevokersProperty; }
    static void OnKeepInteriorCornersSquarePropertyChanged(const winrt::DependencyObject& sender, const winrt::DependencyPropertyChangedEventArgs& args);

    static GlobalDependencyProperty s_DropDownEventRevokersProperty;
private:
    static void OnDropDownOpened(const winrt::IInspectable& sender, const winrt::IInspectable& args);
    static void OnDropDownClosed(const winrt::IInspectable& sender, const winrt::IInspectable& args);

    static void UpdateCornerRadius(const winrt::ComboBox& comboBox, bool isDropDownOpen);
    static bool IsPopupOpenDown(const winrt::ComboBox& comboBox);
};

class ComboBoxDropDownEventRevokers
    : public winrt::implements<ComboBoxDropDownEventRevokers, winrt::Windows::Foundation::IInspectable>
{
public:
    void RevokeAll()
    {
        m_dropDownOpenedRevoker.revoke();
        m_dropDownClosedRevoker.revoke();
    }

    winrt::ComboBox::DropDownOpened_revoker m_dropDownOpenedRevoker;
    winrt::ComboBox::DropDownClosed_revoker m_dropDownClosedRevoker;
};
