// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#include <pch.h>
#include <common.h>
#include "AutoSuggestBoxHelper.h"
#include "DispatcherHelper.h"
#include "CornerRadiusFilterConverter.h"
#include "ResourceAccessor.h"

static constexpr auto c_popupName = L"SuggestionsPopup"sv;
static constexpr auto c_popupBorderName = L"SuggestionsContainer"sv;
static constexpr auto c_textBoxName = L"TextBox"sv;
static constexpr auto c_textBoxBorderName = L"BorderElement"sv;
static constexpr auto c_controlCornerRadiusKey = L"ControlCornerRadius"sv;
static constexpr auto c_overlayCornerRadiusKey = L"OverlayCornerRadius"sv;
GlobalDependencyProperty AutoSuggestBoxHelper::s_AutoSuggestEventRevokersProperty{ nullptr };

AutoSuggestBoxHelper::AutoSuggestBoxHelper()
{
    EnsureProperties();
}

void AutoSuggestBoxHelper::EnsureProperties()
{
    if (!s_AutoSuggestEventRevokersProperty)
    {
        s_AutoSuggestEventRevokersProperty =
            InitializeDependencyProperty(
                L"AutoSuggestEventRevokers",
                winrt::name_of<winrt::IInspectable>(),
                winrt::name_of<winrt::AutoSuggestBoxHelper>(),
                true /* isAttached */,
                ValueHelper<winrt::IInspectable>::BoxedDefaultValue(),
                nullptr);
    }

    AutoSuggestBoxHelperProperties::EnsureProperties();
}

void AutoSuggestBoxHelper::ClearProperties()
{
    s_AutoSuggestEventRevokersProperty = nullptr;
    AutoSuggestBoxHelperProperties::ClearProperties();
}


// The corner radius needs to be updated dynamically depending on whether the suggestion list is opening up or down.
// Xaml is not lifted yet when we implementing this feature so we don't have access to AutoSuggestBox code.
// Creating this attached property to help us plug in some extra logic without touching the actual code.
void AutoSuggestBoxHelper::OnKeepInteriorCornersSquarePropertyChanged(
    const winrt::DependencyObject& sender,
    const winrt::DependencyPropertyChangedEventArgs& args)
{
    if (auto autoSuggestBox = sender.try_as<winrt::AutoSuggestBox>())
    {
        bool shouldMonitorAutoSuggestEvents = unbox_value<bool>(args.NewValue());
        if (shouldMonitorAutoSuggestEvents)
        {
            auto revokersInspectable = winrt::make<AutoSuggestEventRevokers>();
            auto revokers = revokersInspectable.as<AutoSuggestEventRevokers>();

            revokers->m_autoSuggestBoxLoadedRevoker = autoSuggestBox.Loaded(winrt::auto_revoke, AutoSuggestBoxHelper::OnAutoSuggestBoxLoaded);
            autoSuggestBox.SetValue(AutoSuggestEventRevokersProperty(), revokersInspectable);
        }
        else
        {
            autoSuggestBox.SetValue(AutoSuggestEventRevokersProperty(), nullptr);
        }
    }
}

void AutoSuggestBoxHelper::OnAutoSuggestBoxLoaded(const winrt::IInspectable& sender, const winrt::IInspectable& args)
{
    auto autoSuggestBox = sender.as<winrt::AutoSuggestBox>();
    auto revokers = autoSuggestBox.GetValue(AutoSuggestEventRevokersProperty()).as<AutoSuggestEventRevokers>();

    if (!revokers->m_popupOpenedRevoker || !revokers->m_popupClosedRevoker)
    {
        if (auto popup = GetTemplateChildT<winrt::Popup>(c_popupName, autoSuggestBox))
        {
            auto autoSuggestBoxWeakRef = winrt::make_weak(autoSuggestBox);

            revokers->m_popupOpenedRevoker = popup.Opened(winrt::auto_revoke,
                [autoSuggestBoxWeakRef](const winrt::IInspectable& sender, const winrt::IInspectable& args)
                {
                    if (auto autoSuggestBox = autoSuggestBoxWeakRef.get())
                    {
                        UpdateCornerRadius(autoSuggestBox, /*IsDropDownOpen=*/true);
                    }
                });

            revokers->m_popupClosedRevoker = popup.Closed(winrt::auto_revoke,
                [autoSuggestBoxWeakRef](const winrt::IInspectable& sender, const winrt::IInspectable& args)
                {
                    if (auto autoSuggestBox = autoSuggestBoxWeakRef.get())
                    {
                        UpdateCornerRadius(autoSuggestBox, /*IsDropDownOpen=*/false);
                    }
                });
        }
    }
}

void AutoSuggestBoxHelper::UpdateCornerRadius(const winrt::AutoSuggestBox& autoSuggestBox, bool isPopupOpen)
{
    auto textBoxRadius = unbox_value<winrt::CornerRadius>(ResourceAccessor::ResourceLookup(autoSuggestBox, box_value(c_controlCornerRadiusKey)));
    auto popupRadius = unbox_value<winrt::CornerRadius>(ResourceAccessor::ResourceLookup(autoSuggestBox, box_value(c_overlayCornerRadiusKey)));

    if (winrt::IControl7 autoSuggextBoxControl7 = autoSuggestBox)
    {
        textBoxRadius = autoSuggextBoxControl7.CornerRadius();
    }

    if (isPopupOpen)
    {
        auto const isOpenDown = IsPopupOpenDown(autoSuggestBox);
        auto cornerRadiusConverter = winrt::make_self<CornerRadiusFilterConverter>();

        auto popupRadiusFilter = isOpenDown ? winrt::CornerRadiusFilterKind::Bottom : winrt::CornerRadiusFilterKind::Top;
        popupRadius = cornerRadiusConverter->Convert(popupRadius, popupRadiusFilter);

        auto textBoxRadiusFilter = isOpenDown ? winrt::CornerRadiusFilterKind::Top : winrt::CornerRadiusFilterKind::Bottom;
        textBoxRadius = cornerRadiusConverter->Convert(textBoxRadius, textBoxRadiusFilter);
    }

    if (auto popupBorder = GetTemplateChildT<winrt::Border>(c_popupBorderName, autoSuggestBox))
    {
        popupBorder.CornerRadius(popupRadius);
    }

    if (auto textBox = GetTemplateChildT<winrt::TextBox>(c_textBoxName, autoSuggestBox))
    {
        if (winrt::IControl7 textBoxControl7 = textBox)
        {
            textBoxControl7.CornerRadius(textBoxRadius);
        }
        else
        {
            if (auto textBoxBorder = GetTemplateChildT<winrt::Border>(c_textBoxBorderName, textBox))
            {
                textBoxBorder.CornerRadius(textBoxRadius);
            }
        }
    }
}

bool AutoSuggestBoxHelper::IsPopupOpenDown(const winrt::AutoSuggestBox& autoSuggestBox)
{
    double verticalOffset = 0;
    if (auto popupBorder = GetTemplateChildT<winrt::Border>(c_popupBorderName, autoSuggestBox))
    {
        if (auto textBox = GetTemplateChildT<winrt::TextBox>(c_textBoxName, autoSuggestBox))
        {
            auto transform = popupBorder.TransformToVisual(textBox);
            auto popupTop = transform.TransformPoint(winrt::Point(0, 0));
            verticalOffset = popupTop.Y;
        }
    }
    return verticalOffset >= 0;
}
