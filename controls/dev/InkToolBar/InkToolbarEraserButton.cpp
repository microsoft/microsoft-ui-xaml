// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

// Faithful C++/WinRT port of onecoreuap\...\inkcontrols\lib\InkToolbarEraserButton_Partial.cpp.
// Structure/logic preserved 1:1. Lift adaptations (documented): (1) the UWP InkToolbarEraserButtonInternal
// indirection is dropped - SelectedEraser/IsStrokeEraserVisible/ArePrecisionErasersVisible/IsClearAllVisible
// are DPs on the button itself in the lift; (2) FindStringResource localized names are resource gaps;
// (3) the eraser flyout content template must be ported into the lift theme resources for the flyout to
// populate (FindChild returns null until then); (4) RS2 quirk is always false (modern behavior).

#include "pch.h"
#include "common.h"
#include "InkToolbarEraserButton.h"
#include "InkToolbarFlyoutItem.h"
#include "ButtonManager.h"
#include "InkToolbar.h"

namespace
{
    constexpr wchar_t STROKEERASER_ITEM_NAME[] = L"StrokeEraser";
    constexpr wchar_t SMALLERASER_ITEM_NAME[] = L"SmallEraser";
    constexpr wchar_t LARGEERASER_ITEM_NAME[] = L"LargeEraser";
    constexpr wchar_t CLEARALL_ITEM_NAME[] = L"ClearAll";
    constexpr wchar_t ERASER_RADIOGROUPNAME[] = L"Erasers";

    // UWP ::FindChild: depth-first search of the visual subtree for a FrameworkElement with the given Name.
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

    winrt::DependencyObject GetChild(winrt::DependencyObject const& root, int index)
    {
        if (root && winrt::VisualTreeHelper::GetChildrenCount(root) > index)
        {
            return winrt::VisualTreeHelper::GetChild(root, index);
        }
        return nullptr;
    }

    // Build one L3 flyout item (glyph icon + name column) in code. The item resolves its control
    // template from the InkToolbarFlyoutItem default style (generic scope) via SetDefaultStyleKey.
    // Lift adaptation: UWP inflated these items from the InkToolbarEraserButtonFlyoutContentTemplate
    // DataTemplate scoped in generic.xaml; the lift builds the identical two-column tree in code
    // because XamlControlsResources merges the app-referenced themeresources (not the flyout data
    // templates), so a keyed DataTemplate lookup from Application.Current.Resources is unreliable.
    winrt::InkToolbarFlyoutItem MakeFlyoutItem(
        std::wstring_view name,
        std::wstring_view automationId,
        winrt::InkToolbarFlyoutItemKind kind,
        wchar_t const* glyph)
    {
        winrt::InkToolbarFlyoutItem item{};
        item.Name(winrt::hstring{ name });
        item.Kind(kind);
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

        // The localized item name is a resource gap in the lift (empty); the TextBlock is still
        // created so the layout matches the UWP two-column template.
        winrt::TextBlock text{};
        text.VerticalAlignment(winrt::VerticalAlignment::Center);
        text.Margin(winrt::ThicknessHelper::FromLengths(0, 0, 12, 0));
        winrt::Grid::SetColumn(text, 1);
        grid.Children().Append(text);

        item.Content(grid);
        return item;
    }
}

void InkToolbarEraserButton::OnPropertyChanged(winrt::DependencyPropertyChangedEventArgs const& args)
{
    auto property = args.Property();

    if (property == winrt::InkToolbarEraserButton::SelectedEraserProperty())
    {
        auto oldValue = winrt::unbox_value_or<winrt::InkToolbarEraserKind>(args.OldValue(), winrt::InkToolbarEraserKind::Stroke);
        auto newValue = winrt::unbox_value_or<winrt::InkToolbarEraserKind>(args.NewValue(), winrt::InkToolbarEraserKind::Stroke);
        OnSelectedEraserChanged(oldValue, newValue);
    }
    else if (property == winrt::InkToolbarEraserButton::IsClearAllVisibleProperty()
        || property == winrt::InkToolbarEraserButton::IsStrokeEraserVisibleProperty()
        || property == winrt::InkToolbarEraserButton::ArePrecisionErasersVisibleProperty())
    {
        OnL3ItemsVisibilitiesChanged();
    }
    else
    {
        InkToolbarToolButton::OnPropertyChanged(args);
    }
}

void InkToolbarEraserButton::OnApplyTemplateCore()
{
    // Build the eraser L3 flyout content in code and set it on the attached flyout (created empty in
    // the InkToolbarToolButton ctor). The named items (StrokeEraser/SmallEraser/LargeEraser/ClearAll)
    // are what SetupL3 / FindChild wire up to the eraser Checked handling and radio group.
    if (auto flyoutBase = winrt::FlyoutBase::GetAttachedFlyout(*this))
    {
        if (auto flyout = flyoutBase.try_as<winrt::Flyout>())
        {
            winrt::StackPanel panel{};
            panel.Name(L"InkToolbarEraserButtonFlyoutContent");
            panel.Margin(winrt::ThicknessHelper::FromLengths(0, 1, 0, 1));
            panel.Children().Append(MakeFlyoutItem(STROKEERASER_ITEM_NAME, L"InkToolbarStrokeEraser", winrt::InkToolbarFlyoutItemKind::RadioCheck, L"\uF128"));
            panel.Children().Append(MakeFlyoutItem(SMALLERASER_ITEM_NAME, L"InkToolbarSmallEraser", winrt::InkToolbarFlyoutItemKind::RadioCheck, L"\uF129"));
            panel.Children().Append(MakeFlyoutItem(LARGEERASER_ITEM_NAME, L"InkToolbarLargeEraser", winrt::InkToolbarFlyoutItemKind::RadioCheck, L"\uF12A"));
            panel.Children().Append(MakeFlyoutItem(CLEARALL_ITEM_NAME, L"InkToolbarClearAll", winrt::InkToolbarFlyoutItemKind::Simple, L"\uE74D"));
            flyout.Content(panel);
        }
    }

    // Open L3 on access-key invocation when the eraser is checked and has an L3 to show.
    m_accessKeyInvokedToken = AccessKeyInvoked(
        winrt::TypedEventHandler<winrt::UIElement, winrt::AccessKeyInvokedEventArgs>(
            [this](winrt::UIElement const& s, winrt::AccessKeyInvokedEventArgs const& e) { OnAccessKeyInvoked(s, e); }));
}

// Set up flyout items in the eraser flyout and hook up to the Checked events.
void InkToolbarEraserButton::SetupL3(wchar_t const* itemName)
{
    auto thisAsButtonBase = try_as<winrt::Microsoft::UI::Xaml::Controls::Primitives::ButtonBase>();

    winrt::UIElement flyoutContent{ nullptr };
    if (ButtonManager::GetL3Content(thisAsButtonBase, flyoutContent))
    {
        auto itemAsControl = FindChild(flyoutContent, itemName);
        if (!itemAsControl)
        {
            throw winrt::hresult_error(E_UNEXPECTED, L"SetupL3: eraser L3 item is missing.");
        }

        auto item = itemAsControl.try_as<winrt::InkToolbarFlyoutItem>();
        if (!item)
        {
            throw winrt::hresult_error(E_UNEXPECTED, L"SetupL3: eraser L3 item is not an InkToolbarFlyoutItem.");
        }

        auto itemImpl = winrt::get_self<InkToolbarFlyoutItem>(item);
        // Set the eraser button as parent of the flyout items and the radio group name.
        itemImpl->ConfigureItemEvents(thisAsButtonBase);
        itemImpl->SetRadioGroupName(ERASER_RADIOGROUPNAME);

        // Listen to the Checked event to update the eraser type.
        m_eraserItemCheckedRegistrationToken = item.Checked(
            winrt::TypedEventHandler<winrt::InkToolbarFlyoutItem, winrt::IInspectable>(
                [this](winrt::InkToolbarFlyoutItem const& s, winrt::IInspectable const& e) { OnEraserItemChecked(s, e); }));
    }
}

// Called when the SelectedEraser property changes.
void InkToolbarEraserButton::OnSelectedEraserChanged(winrt::InkToolbarEraserKind oldValue, winrt::InkToolbarEraserKind newValue)
{
    if (newValue == oldValue)
    {
        return;
    }

    // Make sure the item(s) are visible for the new EraserKind.
    switch (newValue)
    {
    case winrt::InkToolbarEraserKind::Stroke:
        // Only show the stroke eraser if there are other items in the flyout.
        if (ArePrecisionErasersVisible() || IsClearAllVisible())
        {
            IsStrokeEraserVisible(true);
        }
        else
        {
            IsStrokeEraserVisible(false);
        }
        break;

    case winrt::InkToolbarEraserKind::PrecisionSmall:
    case winrt::InkToolbarEraserKind::PrecisionLarge:
        // The two precision erasers are always shown/hidden together.
        ArePrecisionErasersVisible(true);
        break;

    default:
        throw winrt::hresult_invalid_argument(L"Unexpected eraser kind");
    }

    SetL3EraserItemCheck(newValue, true);
}

// Called when any L3 item visibility changes: update the extension glyph.
void InkToolbarEraserButton::OnL3ItemsVisibilitiesChanged()
{
    auto self = try_as<winrt::InkToolbarToolButton>();
    bool shouldShowExtensionGlyph = ShouldShowL3() && InkToolbar::IsButtonChecked(self);
    IsExtensionGlyphShown(shouldShowExtensionGlyph);
}

// Update EraserKind when an eraser item is checked.
void InkToolbarEraserButton::OnEraserItemChecked(winrt::InkToolbarFlyoutItem const& sender, winrt::IInspectable const& arg)
{
    UNREFERENCED_PARAMETER(arg);

    auto name = sender.as<winrt::FrameworkElement>().Name();
    if (name == STROKEERASER_ITEM_NAME)
    {
        SelectedEraser(winrt::InkToolbarEraserKind::Stroke);
    }
    else if (name == SMALLERASER_ITEM_NAME)
    {
        SelectedEraser(winrt::InkToolbarEraserKind::PrecisionSmall);
    }
    else if (name == LARGEERASER_ITEM_NAME)
    {
        SelectedEraser(winrt::InkToolbarEraserKind::PrecisionLarge);
    }
}

wchar_t const* InkToolbarEraserButton::EraserKindToEraserItemName(winrt::InkToolbarEraserKind kind)
{
    switch (kind)
    {
    case winrt::InkToolbarEraserKind::Stroke: return STROKEERASER_ITEM_NAME;
    case winrt::InkToolbarEraserKind::PrecisionSmall: return SMALLERASER_ITEM_NAME;
    case winrt::InkToolbarEraserKind::PrecisionLarge: return LARGEERASER_ITEM_NAME;
    default:
        throw winrt::hresult_invalid_argument(L"Unexpected eraser kind");
    }
}

winrt::InkToolbarEraserKind InkToolbarEraserButton::EraserItemNameToEraserKind(std::wstring_view name)
{
    if (name == STROKEERASER_ITEM_NAME) { return winrt::InkToolbarEraserKind::Stroke; }
    if (name == SMALLERASER_ITEM_NAME) { return winrt::InkToolbarEraserKind::PrecisionSmall; }
    if (name == LARGEERASER_ITEM_NAME) { return winrt::InkToolbarEraserKind::PrecisionLarge; }
    throw winrt::hresult_error(E_UNEXPECTED, L"Item name doesn't match an eraser kind");
}

void InkToolbarEraserButton::SetL3EraserItemCheck(winrt::InkToolbarEraserKind kind, bool check)
{
    auto thisAsButtonBase = try_as<winrt::Microsoft::UI::Xaml::Controls::Primitives::ButtonBase>();

    winrt::UIElement flyoutContent{ nullptr };
    if (ButtonManager::GetL3Content(thisAsButtonBase, flyoutContent))
    {
        auto itemAsControl = FindChild(flyoutContent, EraserKindToEraserItemName(kind));
        if (!itemAsControl)
        {
            return;   // Some eraser L3 items are missing (template not present yet).
        }

        auto item = itemAsControl.as<winrt::InkToolbarFlyoutItem>();
        if (itemAsControl.Visibility() == winrt::Visibility::Collapsed)
        {
            // Try to set the first visible item in the group.
            if (auto firstItem = winrt::get_self<InkToolbarFlyoutItem>(item)->GetFirstVisibleItemInGroup())
            {
                firstItem.IsChecked(check);
            }
        }
        else
        {
            item.IsChecked(check);
        }
    }
}

bool InkToolbarEraserButton::ShouldShowL3()
{
    return IsStrokeEraserVisible() || ArePrecisionErasersVisible() || IsClearAllVisible();
}

bool InkToolbarEraserButton::GetIsItemVisible(EraserFlyoutItemKind kind)
{
    switch (kind)
    {
    case EraserFlyoutItemKind::StrokeEraser: return IsStrokeEraserVisible();
    case EraserFlyoutItemKind::PrecisionSmallEraser:
    case EraserFlyoutItemKind::PrecisionLargeEraser: return ArePrecisionErasersVisible();
    case EraserFlyoutItemKind::ClearAll: return IsClearAllVisible();
    default:
        throw winrt::hresult_invalid_argument(L"Unexpected EraserFlyoutItemKind");
    }
}

// Hook up the Click event to the item if not hooked up already, and show/hide based on visibility.
void InkToolbarEraserButton::ConfigureEraserFlyoutItems(
    winrt::RoutedEventHandler const& handler,
    bool shouldHookupEvent,
    winrt::event_token& token,
    winrt::DependencyObject const& flyoutContent,
    EraserFlyoutItemKind kind)
{
    wchar_t const* itemName = nullptr;
    switch (kind)
    {
    case EraserFlyoutItemKind::StrokeEraser: itemName = STROKEERASER_ITEM_NAME; break;
    case EraserFlyoutItemKind::PrecisionSmallEraser: itemName = SMALLERASER_ITEM_NAME; break;
    case EraserFlyoutItemKind::PrecisionLargeEraser: itemName = LARGEERASER_ITEM_NAME; break;
    case EraserFlyoutItemKind::ClearAll: itemName = CLEARALL_ITEM_NAME; break;
    default:
        throw winrt::hresult_invalid_argument(L"Unexpected EraserFlyoutItemKind");
    }

    auto itemAsControl = FindChild(flyoutContent, itemName);
    if (!itemAsControl)
    {
        return;   // Item missing (template not present yet).
    }

    auto itemAsButtonBase = itemAsControl.as<winrt::Microsoft::UI::Xaml::Controls::Primitives::ButtonBase>();

    if (shouldHookupEvent)
    {
        // Order matters: set up the L3 item (hooks the Checked event) before hooking Click.
        SetupL3(itemName);
        token = itemAsButtonBase.Click(handler);
        // ButtonManager::PutLocalizedContent(itemAsButtonBase, stringId) - localized content is a lift gap (no-op).
    }

    bool showItem = GetIsItemVisible(kind);
    winrt::Visibility visibility = winrt::Visibility::Collapsed;
    if (showItem)
    {
        if (kind == EraserFlyoutItemKind::StrokeEraser)
        {
            // Don't show stroke eraser if the precision erasers are hidden.
            if (ArePrecisionErasersVisible())
            {
                visibility = winrt::Visibility::Visible;
            }
        }
        else
        {
            visibility = winrt::Visibility::Visible;
        }
    }
    itemAsControl.Visibility(visibility);
}

// Called when the eraser L3 opens: hook the Click event once, and show/hide items per visibility.
void InkToolbarEraserButton::HookUpToEraserEvents(winrt::RoutedEventHandler const& handler, bool& eventHookedUp, winrt::event_token& token)
{
    auto thisAsButtonBase = try_as<winrt::Microsoft::UI::Xaml::Controls::Primitives::ButtonBase>();

    winrt::UIElement flyoutContent{ nullptr };
    if (ButtonManager::GetL3Content(thisAsButtonBase, flyoutContent))
    {
        bool shouldHookup = (eventHookedUp == false);
        ConfigureEraserFlyoutItems(handler, shouldHookup, token, flyoutContent, EraserFlyoutItemKind::StrokeEraser);
        ConfigureEraserFlyoutItems(handler, shouldHookup, token, flyoutContent, EraserFlyoutItemKind::PrecisionSmallEraser);
        ConfigureEraserFlyoutItems(handler, shouldHookup, token, flyoutContent, EraserFlyoutItemKind::PrecisionLargeEraser);
        ConfigureEraserFlyoutItems(handler, shouldHookup, token, flyoutContent, EraserFlyoutItemKind::ClearAll);
        eventHookedUp = true;
    }
}

// Refresh the flyout item visuals.
void InkToolbarEraserButton::UpdateFlyoutItemVisuals()
{
    auto thisAsButtonBase = try_as<winrt::Microsoft::UI::Xaml::Controls::Primitives::ButtonBase>();

    winrt::UIElement flyoutContent{ nullptr };
    if (ButtonManager::GetL3Content(thisAsButtonBase, flyoutContent))
    {
        if (auto child = GetChild(flyoutContent, 0))
        {
            if (auto item = child.try_as<winrt::InkToolbarFlyoutItem>())
            {
                winrt::get_self<InkToolbarFlyoutItem>(item)->UpdateVisualStatesForAllItems();
            }
        }
    }
}

void InkToolbarEraserButton::SetFocusToSelectedEraser(winrt::FocusState focusState)
{
    auto kind = SelectedEraser();
    auto thisAsButtonBase = try_as<winrt::Microsoft::UI::Xaml::Controls::Primitives::ButtonBase>();

    winrt::UIElement flyoutContent{ nullptr };
    if (ButtonManager::GetL3Content(thisAsButtonBase, flyoutContent))
    {
        auto itemAsControl = FindChild(flyoutContent, EraserKindToEraserItemName(kind));
        if (!itemAsControl)
        {
            return;
        }
        if (itemAsControl.Visibility() == winrt::Visibility::Visible)
        {
            itemAsControl.Focus(focusState);
        }
    }
}

void InkToolbarEraserButton::AdvanceEraserSelection(bool forward)
{
    auto kind = SelectedEraser();
    auto thisAsButtonBase = try_as<winrt::Microsoft::UI::Xaml::Controls::Primitives::ButtonBase>();

    winrt::UIElement flyoutContent{ nullptr };
    if (ButtonManager::GetL3Content(thisAsButtonBase, flyoutContent))
    {
        auto itemAsControl = FindChild(flyoutContent, EraserKindToEraserItemName(kind));
        if (!itemAsControl)
        {
            return;
        }

        auto item = itemAsControl.as<winrt::InkToolbarFlyoutItem>();
        auto relative = forward
            ? InkToolbarFlyoutItem::RelativeItem::RadioGroupNext
            : InkToolbarFlyoutItem::RelativeItem::RadioGroupPrevious;

        auto targetItem = winrt::get_self<InkToolbarFlyoutItem>(item)->GetRelativeItem(relative, item, false);
        if (targetItem)
        {
            auto targetName = targetItem.as<winrt::FrameworkElement>().Name();
            SelectedEraser(EraserItemNameToEraserKind(targetName));
            targetItem.as<winrt::Control>().Focus(winrt::FocusState::Keyboard);
        }
    }
}

void InkToolbarEraserButton::OnAccessKeyInvoked(winrt::IInspectable const& sender, winrt::AccessKeyInvokedEventArgs const& args)
{
    UNREFERENCED_PARAMETER(sender);
    UNREFERENCED_PARAMETER(args);

    auto self = try_as<winrt::InkToolbarToolButton>();
    if (ShouldShowL3() && InkToolbar::IsButtonChecked(self))
    {
        InkToolbarToolButton::OpenL3();
    }
}
