// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#pragma once

enum class NavigationViewVisualStateDisplayMode
{
    Compact,
    Expanded,
    Minimal,
    MinimalWithBackButton
};

enum class NavigationViewRepeaterPosition
{
    LeftNav,
    TopPrimary,
    TopOverflow,
    LeftFooter,
    TopFooter
};

enum class NavigationViewPropagateTarget
{
    LeftListView,
    TopListView,
    OverflowListView,
    All
};

// Since RS5, a lot of functions in NavigationViewItem is moved to NavigationViewItemPresenter. So they both share some common codes.
// This class helps to initialize and maintain the status of SelectionIndicator and ToolTip
template<typename T>
class NavigationViewItemHelper
{
public:
    NavigationViewItemHelper(const ITrackerHandleManager* owner): m_owner(owner)
    {
    }

    winrt::UIElement GetSelectionIndicator() const { return m_selectionIndicator.get(); }

    void Init(const winrt::IControlProtected & controlProtected)
    {
        m_selectionIndicator.set(GetTemplateChildT<winrt::UIElement>(c_selectionIndicatorName, controlProtected));
    }

private:
    const ITrackerHandleManager* m_owner;
    tracker_ref<winrt::UIElement> m_selectionIndicator{ m_owner };

    static constexpr wstring_view c_selectionIndicatorName = L"SelectionIndicator"sv;
};

// Applies a NavigationView flyout background key (NavigationViewItemChildrenMenuFlyoutBackground or
// TopNavigationViewOverflowMenuBackground) to the flyout's FlyoutPresenter.Background.
// The template carries the key as {ThemeResource} on the flyout content's Tag, so the framework resolves it from the
// owner's resource scope and re-resolves it on theme/high contrast changes. The default value is a fully transparent
// SolidColorBrush, which means "not customized": the presenter then keeps FlyoutPresenterBackground from its default
// style, so existing app overrides of that key keep working.
class NavigationViewFlyoutBackgroundHelper
{
public:
    NavigationViewFlyoutBackgroundHelper(const ITrackerHandleManager* owner) : m_owner(owner)
    {
    }

    void Attach(const winrt::FlyoutBase& flyoutBase)
    {
        m_flyoutOpeningRevoker.revoke();
        m_flyoutOpenedRevoker.revoke();
        m_contentTagChangedRevoker.revoke();
        m_appliedBackground.set(nullptr);
        m_flyout.set(flyoutBase.try_as<winrt::Flyout>());

        if (auto const flyout = m_flyout.get())
        {
            m_flyoutOpeningRevoker = flyout.Opening(winrt::auto_revoke, [this](auto const&, auto const&) { Apply(); });
            m_flyoutOpenedRevoker = flyout.Opened(winrt::auto_revoke, [this](auto const&, auto const&) { Apply(); });
            if (auto const content = flyout.Content().try_as<winrt::FrameworkElement>())
            {
                m_contentTagChangedRevoker = RegisterPropertyChanged(content, winrt::FrameworkElement::TagProperty(),
                    [this](auto const&, auto const&) { Apply(); });
            }
        }
    }

    void Apply()
    {
        auto const flyout = m_flyout.get();
        auto const content = flyout ? flyout.Content().try_as<winrt::FrameworkElement>() : nullptr;
        auto const presenter = FindFlyoutPresenter(content);
        if (!presenter)
        {
            return;
        }

        auto const background = content.Tag().try_as<winrt::Brush>();
        if (IsCustomBackground(background))
        {
            if (presenter.Background() != background)
            {
                presenter.Background(background);
            }
            m_appliedBackground.set(background);
        }
        else if (auto const applied = m_appliedBackground.get())
        {
            // Only remove the value this helper set, so FlyoutPresenterBackground from the default style applies again.
            if (presenter.ReadLocalValue(winrt::Control::BackgroundProperty()) == applied)
            {
                presenter.ClearValue(winrt::Control::BackgroundProperty());
            }
            m_appliedBackground.set(nullptr);
        }
    }

private:
    static bool IsCustomBackground(const winrt::Brush& brush)
    {
        if (!brush)
        {
            return false;
        }
        if (auto const solidBrush = brush.try_as<winrt::SolidColorBrush>())
        {
            return solidBrush.Color().A != 0;
        }
        return true;
    }

    static winrt::FlyoutPresenter FindFlyoutPresenter(const winrt::FrameworkElement& content)
    {
        if (!content)
        {
            return nullptr;
        }
        if (auto const presenter = content.Parent().try_as<winrt::FlyoutPresenter>())
        {
            return presenter;
        }
        for (auto current = winrt::VisualTreeHelper::GetParent(content); current; current = winrt::VisualTreeHelper::GetParent(current))
        {
            if (auto const presenter = current.try_as<winrt::FlyoutPresenter>())
            {
                return presenter;
            }
        }
        return nullptr;
    }

    const ITrackerHandleManager* m_owner;
    tracker_ref<winrt::Flyout> m_flyout{ m_owner };
    tracker_ref<winrt::Brush> m_appliedBackground{ m_owner };
    winrt::FlyoutBase::Opening_revoker m_flyoutOpeningRevoker{};
    winrt::FlyoutBase::Opened_revoker m_flyoutOpenedRevoker{};
    PropertyChanged_revoker m_contentTagChangedRevoker{};
};

inline constexpr wstring_view c_OnLeftNavigationReveal = L"OnLeftNavigationReveal"sv;
inline constexpr wstring_view c_OnLeftNavigation = L"OnLeftNavigation"sv;
inline constexpr wstring_view c_OnTopNavigationPrimary = L"OnTopNavigationPrimary"sv;
inline constexpr wstring_view c_OnTopNavigationPrimaryReveal = L"OnTopNavigationPrimaryReveal"sv;
inline constexpr wstring_view c_OnTopNavigationOverflow = L"OnTopNavigationOverflow"sv;
