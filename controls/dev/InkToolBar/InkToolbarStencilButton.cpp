// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

// Faithful C++/WinRT port of onecoreuap\...\inkcontrols\lib\InkToolbarStencilButton_Partial.cpp.
// Structure/logic preserved 1:1. Lift adaptations (documented): Ruler/Protractor/SelectedStencil/
// IsRulerItemVisible/IsProtractorItemVisible are DPs on the button (inherited); there is no
// InkPresenterStencil base in the lift so Ruler/Protractor IsVisible is handled per-kind; the stencil
// flyout content template must be ported for the flyout to populate; localized names are resource gaps.

#include "pch.h"
#include "common.h"
#include "InkToolbarStencilButton.h"
#include "InkToolbarFlyoutItem.h"
#include "ButtonManager.h"
#include "InkToolbar.h"
#include "ResourceAccessor.h"

namespace
{
    constexpr wchar_t STENCIL_RADIOGROUPNAME[] = L"Stencils";
    constexpr wchar_t STENCIL_RULERITEMNAME[] = L"StencilRuler";
    constexpr wchar_t STENCIL_PROTRACTORITEMNAME[] = L"StencilProtractor";
    constexpr wchar_t RULER_ICON[] = L"\uECC6";
    constexpr wchar_t PROTRACTOR_ICON[] = L"\uF0B4";

    winrt::Microsoft::UI::Xaml::Controls::Control FindChild(winrt::DependencyObject const& root, std::wstring_view name)
    {
        if (!root)
        {
            return nullptr;
        }
        int count = winrt::VisualTreeHelper::GetChildrenCount(root);
        for (int i = 0; i < count; ++i)
        {
            auto child = winrt::VisualTreeHelper::GetChild(root, i);
            if (auto fe = child.try_as<winrt::FrameworkElement>())
            {
                if (fe.Name() == name)
                {
                    if (auto control = fe.try_as<winrt::Microsoft::UI::Xaml::Controls::Control>())
                    {
                        return control;
                    }
                }
            }
            if (auto found = FindChild(child, name))
            {
                return found;
            }
        }
        return nullptr;
    }
}

void InkToolbarStencilButton::OnApplyTemplate()
{
    InkToolbarMenuButton::OnApplyTemplate();

    // Is<Stencil>ItemVisible takes precedence over SelectedStencil at template time (see UWP comment):
    // if the developer hides the ruler, keep only the protractor rather than re-adding the ruler.
    bool isRulerVisible = IsRulerItemVisible();
    bool isProtractorVisible = IsProtractorItemVisible();

    if (!(isRulerVisible || isProtractorVisible))
    {
        throw winrt::hresult_error(E_UNEXPECTED, L"Neither stencil is visible; the stencil button shouldn't be populated");
    }

    winrt::hstring icon;
    switch (SelectedStencil())
    {
    case winrt::InkToolbarStencilKind::Ruler:
        if (isRulerVisible)
        {
            icon = RULER_ICON;
        }
        else if (isProtractorVisible)
        {
            SelectedStencil(winrt::InkToolbarStencilKind::Protractor);
            icon = PROTRACTOR_ICON;
        }
        break;

    case winrt::InkToolbarStencilKind::Protractor:
        if (isProtractorVisible)
        {
            icon = PROTRACTOR_ICON;
        }
        else if (isRulerVisible)
        {
            SelectedStencil(winrt::InkToolbarStencilKind::Ruler);
            icon = RULER_ICON;
        }
        break;

    default:
        throw winrt::hresult_invalid_argument(L"Invalid InkToolbarStencilKind");
    }

    if (!icon.empty())
    {
        if (auto content = GetTemplateChild(L"Content").try_as<winrt::TextBlock>())
        {
            content.Text(icon);
        }
        if (auto checkedContent = GetTemplateChild(L"CheckedContent").try_as<winrt::TextBlock>())
        {
            checkedContent.Text(icon);
        }
    }

    // Load the stencil flyout content template into the attached flyout.
    if (auto resources = Resources())
    {
        auto key = winrt::box_value(L"InkToolbarStencilButtonFlyoutContentTemplate");
        if (resources.HasKey(key))
        {
            if (auto stencilFlyoutTemplate = resources.Lookup(key).try_as<winrt::DataTemplate>())
            {
                if (auto flyoutBase = winrt::FlyoutBase::GetAttachedFlyout(*this))
                {
                    if (auto flyout = flyoutBase.try_as<winrt::Flyout>())
                    {
                        if (auto flyoutContent = stencilFlyoutTemplate.LoadContent().try_as<winrt::UIElement>())
                        {
                            flyout.Content(flyoutContent);
                        }
                    }
                }
            }
        }
    }
}

void InkToolbarStencilButton::OnPropertyChanged(winrt::DependencyPropertyChangedEventArgs const& args)
{
    auto property = args.Property();

    if (property == winrt::InkToolbarStencilButton::RulerProperty()
        || property == winrt::InkToolbarStencilButton::ProtractorProperty())
    {
        // No-op.
    }
    else if (property == winrt::InkToolbarStencilButton::IsRulerItemVisibleProperty()
        || property == winrt::InkToolbarStencilButton::IsProtractorItemVisibleProperty())
    {
        OnL3ItemsVisibilitiesChanged();
    }
    else if (property == winrt::InkToolbarStencilButton::SelectedStencilProperty())
    {
        OnSelectedStencilChanged(args);
    }
    else
    {
        InkToolbarMenuButton::OnPropertyChanged(args);
    }
}

void InkToolbarStencilButton::OnL3ItemsVisibilitiesChanged()
{
    auto thisAsMenuButton = try_as<winrt::InkToolbarMenuButton>();
    auto isChecked = InkToolbar::IsButtonChecked(thisAsMenuButton);

    bool shouldShowExtensionGlyph =
        (NumberOfStencils() > 1) && (isChecked == InkToolbarMenuButtonCheckedState::Checked);

    IsExtensionGlyphShown(shouldShowExtensionGlyph);

    InkToolbarMenuButton::UpdateMenuButtonToolTip();
}

void InkToolbarStencilButton::OnSelectedStencilChanged(winrt::DependencyPropertyChangedEventArgs const& args)
{
    auto oldKind = winrt::unbox_value_or<winrt::InkToolbarStencilKind>(args.OldValue(), winrt::InkToolbarStencilKind::Ruler);
    auto newKind = winrt::unbox_value_or<winrt::InkToolbarStencilKind>(args.NewValue(), winrt::InkToolbarStencilKind::Ruler);

    if (oldKind == newKind)
    {
        return;
    }

    // Hide the old stencil (if visible), uncheck it, and uncheck the button if nothing remains selected.
    bool oldVisible = false;
    switch (oldKind)
    {
    case winrt::InkToolbarStencilKind::Ruler:
        if (auto ruler = Ruler())
        {
            oldVisible = ruler.IsVisible();
            if (oldVisible) { ruler.IsVisible(false); }
        }
        break;
    case winrt::InkToolbarStencilKind::Protractor:
        if (auto protractor = Protractor())
        {
            oldVisible = protractor.IsVisible();
            if (oldVisible) { protractor.IsVisible(false); }
        }
        break;
    default:
        return;
    }

    if (oldVisible)
    {
        SetL3StencilItemCheck(oldKind, false);
        if (!IsAnyStencilSelected())
        {
            auto thisAsMenuButton = try_as<winrt::InkToolbarMenuButton>();
            InkToolbar::SetButtonCheck(thisAsMenuButton, InkToolbarMenuButtonCheckedState::Unchecked);
        }
    }

    // Make sure the corresponding L3 item of the new stencil is shown.
    switch (newKind)
    {
    case winrt::InkToolbarStencilKind::Ruler:
        IsRulerItemVisible(true);
        break;
    case winrt::InkToolbarStencilKind::Protractor:
        IsProtractorItemVisible(true);
        break;
    default:
        return;
    }

    UpdateIcon(newKind);
    InkToolbarMenuButton::UpdateMenuButtonToolTip();
}

winrt::hstring InkToolbarStencilButton::GetLocalizedToolName()
{
    // UWP StencilButton::GetLocalizedToolName: name reflects the currently selected stencil.
    switch (SelectedStencil())
    {
    case winrt::InkToolbarStencilKind::Ruler:
        return ResourceAccessor::GetLocalizedStringResource(SR_InkToolbarStencilRulerName);
    case winrt::InkToolbarStencilKind::Protractor:
        return ResourceAccessor::GetLocalizedStringResource(SR_InkToolbarStencilProtractorName);
    default:
        return {};
    }
}

winrt::hstring InkToolbarStencilButton::GetFlyoutName()
{
    // Localized flyout name is a lift resource gap (documented).
    return {};
}

unsigned InkToolbarStencilButton::NumberOfStencils()
{
    return (IsRulerItemVisible() ? 1u : 0u) + (IsProtractorItemVisible() ? 1u : 0u);
}

bool InkToolbarStencilButton::IsAnyStencilSelected()
{
    auto stencilAsButtonBase = try_as<winrt::Microsoft::UI::Xaml::Controls::Primitives::ButtonBase>();

    winrt::UIElement flyoutContent{ nullptr };
    if (ButtonManager::GetL3Content(stencilAsButtonBase, flyoutContent))
    {
        auto stencilItemAsControl = FindChild(flyoutContent, STENCIL_RULERITEMNAME);
        if (!stencilItemAsControl)
        {
            return false;
        }
        if (auto item = stencilItemAsControl.try_as<winrt::InkToolbarFlyoutItem>())
        {
            return winrt::get_self<InkToolbarFlyoutItem>(item)->IsAnySelectedInRadioGroup();
        }
    }
    return false;
}

void InkToolbarStencilButton::SetupL3(wchar_t const* itemName)
{
    auto thisAsButtonBase = try_as<winrt::Microsoft::UI::Xaml::Controls::Primitives::ButtonBase>();

    winrt::UIElement flyoutContent{ nullptr };
    if (ButtonManager::GetL3Content(thisAsButtonBase, flyoutContent))
    {
        auto itemAsControl = FindChild(flyoutContent, itemName);
        if (!itemAsControl)
        {
            throw winrt::hresult_error(E_UNEXPECTED, L"SetupL3: stencil L3 item missing.");
        }

        if (auto item = itemAsControl.try_as<winrt::InkToolbarFlyoutItem>())
        {
            auto itemImpl = winrt::get_self<InkToolbarFlyoutItem>(item);
            itemImpl->ConfigureItemEvents(thisAsButtonBase);
            itemImpl->SetRadioGroupName(STENCIL_RADIOGROUPNAME);
        }
    }
}

void InkToolbarStencilButton::SetL3StencilItemCheck(winrt::InkToolbarStencilKind stencilKind, bool check)
{
    auto thisAsButtonBase = try_as<winrt::Microsoft::UI::Xaml::Controls::Primitives::ButtonBase>();

    winrt::UIElement flyoutContent{ nullptr };
    if (ButtonManager::GetL3Content(thisAsButtonBase, flyoutContent))
    {
        wchar_t const* itemName = nullptr;
        switch (stencilKind)
        {
        case winrt::InkToolbarStencilKind::Ruler: itemName = STENCIL_RULERITEMNAME; break;
        case winrt::InkToolbarStencilKind::Protractor: itemName = STENCIL_PROTRACTORITEMNAME; break;
        default: return;
        }

        auto itemAsControl = FindChild(flyoutContent, itemName);
        if (!itemAsControl)
        {
            return;
        }
        if (auto item = itemAsControl.try_as<winrt::InkToolbarFlyoutItem>())
        {
            item.IsChecked(check);
        }
    }
}

void InkToolbarStencilButton::SetAllStencilItemsCheck(bool check)
{
    SetL3StencilItemCheck(winrt::InkToolbarStencilKind::Ruler, check);
    SetL3StencilItemCheck(winrt::InkToolbarStencilKind::Protractor, check);
}

bool InkToolbarStencilButton::GetIsItemVisible(winrt::InkToolbarStencilKind kind)
{
    switch (kind)
    {
    case winrt::InkToolbarStencilKind::Ruler: return IsRulerItemVisible();
    case winrt::InkToolbarStencilKind::Protractor: return IsProtractorItemVisible();
    default:
        throw winrt::hresult_invalid_argument(L"Unexpected stencil kind");
    }
}

void InkToolbarStencilButton::ConfigureStencilFlyoutItems(
    winrt::RoutedEventHandler const& handler,
    bool shouldHookupEvent,
    winrt::DependencyObject const& flyoutContent,
    winrt::InkToolbarStencilKind kind,
    winrt::event_token& token)
{
    wchar_t const* itemName = nullptr;
    switch (kind)
    {
    case winrt::InkToolbarStencilKind::Ruler: itemName = STENCIL_RULERITEMNAME; break;
    case winrt::InkToolbarStencilKind::Protractor: itemName = STENCIL_PROTRACTORITEMNAME; break;
    default:
        throw winrt::hresult_invalid_argument(L"Unexpected stencil kind");
    }

    auto itemAsControl = FindChild(flyoutContent, itemName);
    if (!itemAsControl)
    {
        return;
    }

    auto itemAsButtonBase = itemAsControl.as<winrt::Microsoft::UI::Xaml::Controls::Primitives::ButtonBase>();

    if (shouldHookupEvent)
    {
        SetupL3(itemName);
        token = itemAsButtonBase.Click(handler);
        // ButtonManager::PutLocalizedContent(itemAsButtonBase, stringId) - localized content is a lift gap.
    }

    itemAsControl.Visibility(GetIsItemVisible(kind) ? winrt::Visibility::Visible : winrt::Visibility::Collapsed);
}

void InkToolbarStencilButton::HookUpToStencilEvents(winrt::RoutedEventHandler const& handler, bool& eventHookedUp, winrt::event_token& token)
{
    auto thisAsButtonBase = try_as<winrt::Microsoft::UI::Xaml::Controls::Primitives::ButtonBase>();

    winrt::UIElement flyoutContent{ nullptr };
    if (ButtonManager::GetL3Content(thisAsButtonBase, flyoutContent))
    {
        bool shouldHookup = (eventHookedUp == false);
        ConfigureStencilFlyoutItems(handler, shouldHookup, flyoutContent, winrt::InkToolbarStencilKind::Ruler, token);
        ConfigureStencilFlyoutItems(handler, shouldHookup, flyoutContent, winrt::InkToolbarStencilKind::Protractor, token);
        eventHookedUp = true;
    }
}

void InkToolbarStencilButton::UpdateIcon(winrt::InkToolbarStencilKind kind)
{
    wchar_t const* icon = nullptr;
    switch (kind)
    {
    case winrt::InkToolbarStencilKind::Ruler: icon = RULER_ICON; break;
    case winrt::InkToolbarStencilKind::Protractor: icon = PROTRACTOR_ICON; break;
    default: return;
    }

    if (auto content = GetTemplateChild(L"Content").try_as<winrt::TextBlock>())
    {
        content.Text(icon);
    }
    if (auto checkedContent = GetTemplateChild(L"CheckedContent").try_as<winrt::TextBlock>())
    {
        checkedContent.Text(icon);
    }
}
