// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#include "pch.h"
#include "common.h"
#include "CommandBarFlyout.h"
#include "CommandBarFlyoutCommandBar.h"
#include "Vector.h"
#include "RuntimeProfiler.h"

#include "CommandBarFlyout.properties.cpp"

// Change to 'true' to turn on debugging outputs in Output window
bool CommandBarFlyoutTrace::s_IsDebugOutputEnabled{ false };
bool CommandBarFlyoutTrace::s_IsVerboseDebugOutputEnabled{ false };

CommandBarFlyout::CommandBarFlyout()
{
    __RP_Marker_ClassById(RuntimeProfiler::ProfId_CommandBarFlyout);

    if (winrt::IFlyoutBase6 thisAsFlyoutBase6 = *this)
    {
        thisAsFlyoutBase6.ShouldConstrainToRootBounds(false);
    }
    
    if (winrt::IFlyoutBase5 thisAsFlyoutBase5 = *this)
    {
        thisAsFlyoutBase5.AreOpenCloseAnimationsEnabled(false);
    }

    m_primaryCommands = winrt::make<Vector<winrt::ICommandBarElement>>().as<winrt::IObservableVector<winrt::ICommandBarElement>>();
    m_secondaryCommands = winrt::make<Vector<winrt::ICommandBarElement>>().as<winrt::IObservableVector<winrt::ICommandBarElement>>();

    m_primaryCommandsVectorChangedToken = m_primaryCommands.VectorChanged({
        [this](winrt::IObservableVector<winrt::ICommandBarElement> const& sender, winrt::IVectorChangedEventArgs const& args)
        {
            if (auto commandBar = m_commandBar.get())
            {
                SharedHelpers::ForwardVectorChange(sender, commandBar.PrimaryCommands(), args);
            }
        }
    });

    m_secondaryCommandsVectorChangedToken = m_secondaryCommands.VectorChanged({
        [this](winrt::IObservableVector<winrt::ICommandBarElement> const& sender, winrt::IVectorChangedEventArgs const& args)
        {
            if (auto commandBar = m_commandBar.get())
            {
                SharedHelpers::ForwardVectorChange(sender, commandBar.SecondaryCommands(), args);

                // We want to ensure that any interaction with secondary items causes the CommandBarFlyout
                // to close, so we'll attach a Click handler to any buttons and Checked/Unchecked handlers
                // to any toggle buttons that we get and close the flyout when they're invoked.
                // The only exception is buttons with flyouts - in that case, clicking on the button
                // will just open the flyout rather than executing an action, so we don't want that to
                // do anything.
                const int index = args.Index();
                const auto closeFlyoutFunc = [this](auto const& sender, auto const& args) { Hide(); };

                switch (args.CollectionChange())
                {
                case winrt::CollectionChange::ItemChanged:
                {
                    auto element = sender.GetAt(index);
                    auto button = element.try_as<winrt::AppBarButton>();
                    auto toggleButton = element.try_as<winrt::AppBarToggleButton>();

                    if (button && !button.Flyout())
                    {
                        m_secondaryButtonClickRevokerByIndexMap[index] = button.Click(winrt::auto_revoke, closeFlyoutFunc);
                        SharedHelpers::EraseIfExists(m_secondaryToggleButtonCheckedRevokerByIndexMap, index);
                        SharedHelpers::EraseIfExists(m_secondaryToggleButtonUncheckedRevokerByIndexMap, index);
                    }
                    else if (toggleButton)
                    {
                        SharedHelpers::EraseIfExists(m_secondaryButtonClickRevokerByIndexMap, index);
                        m_secondaryToggleButtonCheckedRevokerByIndexMap[index] = toggleButton.Checked(winrt::auto_revoke, closeFlyoutFunc);
                        m_secondaryToggleButtonUncheckedRevokerByIndexMap[index] = toggleButton.Unchecked(winrt::auto_revoke, closeFlyoutFunc);
                    }
                    else
                    {
                        SharedHelpers::EraseIfExists(m_secondaryButtonClickRevokerByIndexMap, index);
                        SharedHelpers::EraseIfExists(m_secondaryToggleButtonCheckedRevokerByIndexMap, index);
                        SharedHelpers::EraseIfExists(m_secondaryToggleButtonUncheckedRevokerByIndexMap, index);
                    }
                    break;
                }
                case winrt::CollectionChange::ItemInserted:
                {
                    auto element = sender.GetAt(index);
                    auto button = element.try_as<winrt::AppBarButton>();
                    auto toggleButton = element.try_as<winrt::AppBarToggleButton>();

                    if (button && !button.Flyout())
                    {
                        m_secondaryButtonClickRevokerByIndexMap[index] = button.Click(winrt::auto_revoke, closeFlyoutFunc);
                    }
                    else if (toggleButton)
                    {
                        m_secondaryToggleButtonCheckedRevokerByIndexMap[index] = toggleButton.Checked(winrt::auto_revoke, closeFlyoutFunc);
                        m_secondaryToggleButtonUncheckedRevokerByIndexMap[index] = toggleButton.Unchecked(winrt::auto_revoke, closeFlyoutFunc);
                    }
                    break;
                }
                case winrt::CollectionChange::ItemRemoved:
                    SharedHelpers::EraseIfExists(m_secondaryButtonClickRevokerByIndexMap, index);
                    SharedHelpers::EraseIfExists(m_secondaryToggleButtonCheckedRevokerByIndexMap, index);
                    SharedHelpers::EraseIfExists(m_secondaryToggleButtonUncheckedRevokerByIndexMap, index);
                    break;
                case winrt::CollectionChange::Reset:
                    SetSecondaryCommandsToCloseWhenExecuted();
                    break;
                default:
                    MUX_ASSERT(false);
                }
            }
        }
    });

    Opening({
        [this](auto const&, auto const&)
        {
            // The CommandBarFlyout is shown in standard mode in the case
            // where it's being opened as a context menu, rather than as a selection flyout.
            // In that circumstance, we want to have the flyout be open from the start.
            winrt::IFlyoutBase5 thisAsFlyoutBase5 = *this;

            if (auto commandBar = m_commandBar.get())
            {
                // If we don't have IFlyoutBase5 available, then we assume a standard show mode.
                if (!thisAsFlyoutBase5 || thisAsFlyoutBase5.ShowMode() == winrt::FlyoutShowMode::Standard)
                {
                    commandBar.IsOpen(true);
                }

                // When CommandBarFlyout is in AlwaysOpen state, don't show the overflow button
                if (AlwaysExpanded())
                {
                    commandBar.IsOpen(true);
                    commandBar.OverflowButtonVisibility(winrt::Windows::UI::Xaml::Controls::CommandBarOverflowButtonVisibility::Collapsed);
                }
                else
                {
                    commandBar.OverflowButtonVisibility(winrt::Windows::UI::Xaml::Controls::CommandBarOverflowButtonVisibility::Auto);
                }
            }

            if (m_primaryCommands.Size() > 0)
            {
                AddDropShadow();
            }
        }
    });

    Opened({
        [this](auto const&, auto const&)
        {
            if (auto commandBar = winrt::get_self<CommandBarFlyoutCommandBar>(m_commandBar.get()))
            {
                if (commandBar->HasOpenAnimation())
                {
                    commandBar->PlayOpenAnimation();
                }
            }
        }
    });

    Closing({
        [this](auto const&, winrt::FlyoutBaseClosingEventArgs const& args)
        {
            if (auto commandBar = winrt::get_self<CommandBarFlyoutCommandBar>(m_commandBar.get()))
            {
                RemoveDropShadow();

                if (!m_isClosingAfterCloseAnimation && commandBar->HasCloseAnimation())
                {
                    args.Cancel(true);

                    commandBar->PlayCloseAnimation(
                        [this]()
                        {
                            m_isClosingAfterCloseAnimation = true;
                            Hide();
                            m_isClosingAfterCloseAnimation = false;
                        });
                }
                // Close commandbar and thus other associated flyouts
                commandBar->IsOpen(false);

                //CommandBarFlyoutCommandBar.Closed will be called when
                //clicking the more (...) button, we clear the translations
                //here
                commandBar->ClearShadow();
            }
        }
    });

    // If we didn't close the CommandBar in the Closing event, we want to do it here,
    // in order to ensure that we're always starting from a known state when opening the flyout.
    Closed({
        [this](auto const&, auto const&)
        {
            if (auto commandBar = m_commandBar.get())
            {
                if (commandBar.IsOpen())
                {
                    commandBar.IsOpen(false);
                }
            }
        }
    });
}

CommandBarFlyout::~CommandBarFlyout()
{
    m_primaryCommands.VectorChanged(m_primaryCommandsVectorChangedToken);
    m_secondaryCommands.VectorChanged(m_secondaryCommandsVectorChangedToken);
}

winrt::IObservableVector<winrt::ICommandBarElement> CommandBarFlyout::PrimaryCommands()
{
    return m_primaryCommands;
}

winrt::IObservableVector<winrt::ICommandBarElement> CommandBarFlyout::SecondaryCommands()
{
    return m_secondaryCommands;
}

winrt::Control CommandBarFlyout::CreatePresenter()
{
    auto commandBar = winrt::make_self<CommandBarFlyoutCommandBar>();

    m_commandBarOpenedRevoker = commandBar->Opened(winrt::auto_revoke, {
        [this](auto const&, auto const&)
        {
            if (winrt::IFlyoutBase5 thisAsFlyoutBase5 = *this)
            {
                // If we open the CommandBar, then we should no longer be in a transient show mode -
                // we now know that the user wants to interact with us.
                thisAsFlyoutBase5.ShowMode(winrt::FlyoutShowMode::Standard);
            }
        }
    });

    SharedHelpers::CopyVector(m_primaryCommands, commandBar->PrimaryCommands());
    SharedHelpers::CopyVector(m_secondaryCommands, commandBar->SecondaryCommands());

    SetSecondaryCommandsToCloseWhenExecuted();

    winrt::FlyoutPresenter presenter;
    presenter.Background(nullptr);
    presenter.Foreground(nullptr);
    presenter.BorderBrush(nullptr);
    presenter.MinWidth(0);
    presenter.MaxWidth(std::numeric_limits<double>::infinity());
    presenter.MinHeight(0);
    presenter.MaxHeight(std::numeric_limits<double>::infinity());
    presenter.BorderThickness(winrt::ThicknessHelper::FromUniformLength(0));
    presenter.Padding(winrt::ThicknessHelper::FromUniformLength(0));
    presenter.Content(*commandBar);

    // Disable the default shadow, as we'll be providing our own shadow.
    if (winrt::IFlyoutPresenter2 presenter2 = presenter)
    {
        presenter2.IsDefaultShadowEnabled(false);
    }

    if (winrt::IControl7 presenterControl7 = presenter)
    {
        // When >21H1, we'll need to manage the presenter's CornerRadius when the overflow is opened/
        // closed. In order for drop shadows to render correctly, the element with the shadow needs to have
        // the correct corner radius as well. Otherwise the shadow will render with sharp corners.
        if (SharedHelpers::Is21H1OrHigher())
        {
            presenterControl7.CornerRadius({ 4 });
        }
        else
        {
            presenterControl7.CornerRadius({ 0 });
        }
    }

    // Since DropShadows don't play well with the entrance animation for the presenter,
    // we'll need to fade it in. This name helps us locate the element to set the flag.
    // We didn't want to apply this name to all FlyoutPresenters, which is why it's done
    // in code behind here.
    if (SharedHelpers::Is21H1OrHigher())
    {
        presenter.Name(L"DropShadowFadeInTarget");
    }

    m_presenter.set(presenter);
    m_commandBarOpeningRevoker = commandBar->Opening(winrt::auto_revoke, {
    [this](auto const&, auto const&)
    {
        // When >21H1, if the CommandBar is opening, set the presenter's corner
        // radius to 0 to let the CommandBar's corner radius show through.
        if (SharedHelpers::Is21H1OrHigher())
        {
            if (m_presenter)
            {
                if (m_secondaryCommands.Size() > 0)
                {
                    if (winrt::IControl7 presenterControl7 = m_presenter.get())
                    {
                        presenterControl7.CornerRadius({ 0 });
                    }
                }
            }

            // During the SecondaryCommands's animations, drop shadows need to disappear.
            // However, performing a Remove during Closing/Opening and a Add during Closed/Opened
            // doesn't line up perfectly with the end of the animations. So, if there are animations
            // we'll remove the shadow in the Closing and Opening events, but we'll put back the shadow
            // inside of CommandBarFlyoutCommandBar's Storyboard Completed event handlers.
            if (auto commandBar = winrt::get_self<CommandBarFlyoutCommandBar>(m_commandBar.get()))
            {
                if (commandBar->HasSecondaryOpenCloseAnimations())
                {
                    if (!AlwaysExpanded() && m_secondaryCommands.Size() > 0)
                    {
                        RemoveDropShadow();
                    }
                }
            }
        }
    }
    });

    m_commandBarClosingRevoker = commandBar->Closing(winrt::auto_revoke, {
    [this](auto const&, auto const&)
    {
        if (SharedHelpers::Is21H1OrHigher())
        {
            if (auto commandBar = winrt::get_self<CommandBarFlyoutCommandBar>(m_commandBar.get()))
            {
                if (commandBar->HasSecondaryOpenCloseAnimations())
                {
                    RemoveDropShadow();
                }
            }
        }
    }
    });

    m_commandBarClosedRevoker = commandBar->Closed(winrt::auto_revoke, {
    [this](auto const&, auto const&)
    {
        // When >21H1, if the CommandBar is closed, set the presenter's corner radius to 4.
        if (SharedHelpers::Is21H1OrHigher())
        {
            if (m_presenter)
            {
                if (winrt::IControl7 presenterControl7 = m_presenter.get())
                {
                    presenterControl7.CornerRadius({ 4 });
                }
            }
        }
    }
    });

    commandBar->SetOwningFlyout(*this);

    m_commandBar.set(*commandBar);
    return presenter;
}

void CommandBarFlyout::SetSecondaryCommandsToCloseWhenExecuted()
{
    m_secondaryButtonClickRevokerByIndexMap.clear();
    m_secondaryToggleButtonCheckedRevokerByIndexMap.clear();
    m_secondaryToggleButtonUncheckedRevokerByIndexMap.clear();

    const auto closeFlyoutFunc = [this](auto const& sender, auto const& args) { Hide(); };

    for (uint32_t i = 0; i < SecondaryCommands().Size(); i++)
    {
        auto element = SecondaryCommands().GetAt(i);
        auto button = element.try_as<winrt::AppBarButton>();
        auto toggleButton = element.try_as<winrt::AppBarToggleButton>();

        if (button && !button.Flyout())
        {
            m_secondaryButtonClickRevokerByIndexMap[i] = button.Click(winrt::auto_revoke, closeFlyoutFunc);
        }
        else if (toggleButton)
        {
            m_secondaryToggleButtonCheckedRevokerByIndexMap[i] = toggleButton.Checked(winrt::auto_revoke, closeFlyoutFunc);
            m_secondaryToggleButtonUncheckedRevokerByIndexMap[i] = toggleButton.Unchecked(winrt::auto_revoke, closeFlyoutFunc);
        }
    }
}

void CommandBarFlyout::AddDropShadow()
{
    if (SharedHelpers::Is21H1OrHigher())
    {
        if (m_presenter)
        {
            winrt::Windows::UI::Xaml::Media::ThemeShadow shadow;
            m_presenter.get().Shadow(shadow);
        }
    }
}

void CommandBarFlyout::RemoveDropShadow()
{
    if (SharedHelpers::Is21H1OrHigher())
    {
        if (m_presenter)
        {
            m_presenter.get().Shadow(nullptr);
        }
    }
}
