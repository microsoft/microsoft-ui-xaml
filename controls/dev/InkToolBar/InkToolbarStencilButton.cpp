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
#include "InkToolbarTrace.h"

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

    // Build one L3 flyout item (glyph icon + name column) in code. The item resolves its control
    // template from the InkToolbarFlyoutItem default style (generic scope) via SetDefaultStyleKey.
    // Lift adaptation: UWP inflated these items from the InkToolbarStencilButtonFlyoutContentTemplate
    // DataTemplate scoped in generic.xaml; the lift builds the identical two-column tree in code
    // because a keyed DataTemplate lookup from XamlControlsResources / Application.Current.Resources
    // is unreliable (see InkToolbarEraserButton.cpp).
    winrt::InkToolbarFlyoutItem MakeFlyoutItem(
        std::wstring_view name,
        std::wstring_view automationId,
        wchar_t const* glyph,
        wchar_t const* labelResourceKey)
    {
        winrt::InkToolbarFlyoutItem item{};
        item.Name(winrt::hstring{ name });
        item.Kind(winrt::InkToolbarFlyoutItemKind::RadioCheck);
        winrt::AutomationProperties::SetAutomationId(item, winrt::hstring{ automationId });

        winrt::Grid grid{};
        winrt::ColumnDefinition iconColumn{};
        iconColumn.Width(winrt::GridLengthHelper::FromValueAndType(0, winrt::GridUnitType::Auto));
        winrt::ColumnDefinition textColumn{};
        textColumn.Width(winrt::GridLengthHelper::FromValueAndType(0, winrt::GridUnitType::Auto));
        grid.ColumnDefinitions().Append(iconColumn);
        grid.ColumnDefinitions().Append(textColumn);

        winrt::TextBlock icon{};
        icon.FontFamily(winrt::FontFamily{ L"Segoe Fluent Icons, Segoe MDL2 Assets" });
        icon.FontSize(16.0);
        icon.Text(winrt::hstring{ glyph });
        icon.HorizontalAlignment(winrt::HorizontalAlignment::Center);
        icon.VerticalAlignment(winrt::VerticalAlignment::Center);
        icon.Margin(winrt::ThicknessHelper::FromLengths(12, 0, 12, 0));
        winrt::Grid::SetColumn(icon, 0);
        grid.Children().Append(icon);

        // Localized display name for the flyout item (UWP stencil L3 Ruler/Protractor labels).
        // Guard the lookup so a missing resource degrades to an empty label instead of propagating out
        // of OnApplyTemplateCore (parity with InkToolbarToolButton::OnApplyTemplate).
        winrt::TextBlock text{};
        try
        {
            text.Text(ResourceAccessor::GetLocalizedStringResource(labelResourceKey));
        }
        catch (winrt::hresult_error const& e)
        {
            InkToolbarLogHResult(e.code(), L"stencil flyout item label lookup");
        }
        text.VerticalAlignment(winrt::VerticalAlignment::Center);
        text.Margin(winrt::ThicknessHelper::FromLengths(0, 0, 12, 0));
        winrt::Grid::SetColumn(text, 1);
        grid.Children().Append(text);

        item.Content(grid);
        return item;
    }
}

void InkToolbarStencilButton::OnApplyTemplateCore()
{
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

    // Build the stencil L3 flyout content in code and set it on the attached flyout (created empty in
    // the InkToolbarMenuButton ctor). The named items (StencilRuler/StencilProtractor) are what
    // SetupL3 / FindChild wire up to the stencil Checked handling and radio group.
    if (auto flyoutBase = winrt::FlyoutBase::GetAttachedFlyout(*this))
    {
        if (auto flyout = flyoutBase.try_as<winrt::Flyout>())
        {
            winrt::StackPanel panel{};
            panel.Name(L"InkToolbarStencilButtonFlyoutContent");
            panel.Margin(winrt::ThicknessHelper::FromLengths(0, 1, 0, 1));
            panel.Children().Append(MakeFlyoutItem(STENCIL_RULERITEMNAME, L"InkToolbarStencilRuler", RULER_ICON, SR_InkToolbarStencilRulerName));
            panel.Children().Append(MakeFlyoutItem(STENCIL_PROTRACTORITEMNAME, L"InkToolbarStencilProtractor", PROTRACTOR_ICON, SR_InkToolbarStencilProtractorName));
            flyout.Content(panel);
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
    // The extension glyph is only meaningful once this button has been realized as a ToggleButton and
    // is checked. A visibility property can change before the button is fully realized (immediately
    // after construction, or in a bare test host with no visual tree), so guard the cast: an
    // unrealized button is treated as unchecked rather than dereferencing a null interface.
    bool shouldShowExtensionGlyph = false;
    if (auto self = try_as<winrt::InkToolbarMenuButton>())
    {
        shouldShowExtensionGlyph =
            (NumberOfStencils() > 1) &&
            (InkToolbar::IsButtonChecked(self) == InkToolbarMenuButtonCheckedState::Checked);
    }

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
            // Guard the cast: this can run before the button is realized as a ToggleButton, in which
            // case there is no checked state to clear (matches UWP's guaranteed-non-null QueryInterface).
            if (auto self = try_as<winrt::InkToolbarMenuButton>())
            {
                InkToolbar::SetButtonCheck(self, InkToolbarMenuButtonCheckedState::Unchecked);
            }
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
            InkToolbarLogHResult(E_UNEXPECTED, L"SetupL3: stencil L3 item missing");
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
