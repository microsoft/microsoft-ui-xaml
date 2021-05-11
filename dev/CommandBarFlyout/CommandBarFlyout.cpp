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
                SharedHelpers::QueueCallbackForCompositionRendering(
                    [strongThis = get_strong(), thisAsFlyoutBase5, commandBar]
                    {
                        if (auto const commandBarFlyoutCommandBar = winrt::get_self<CommandBarFlyoutCommandBar>(commandBar))
                        {
                            auto const scopeGuard = gsl::finally([commandBarFlyoutCommandBar]()
                                {
                                    commandBarFlyoutCommandBar->m_commandBarFlyoutIsOpening = false;
                                });
                            commandBarFlyoutCommandBar->m_commandBarFlyoutIsOpening = true;

                            // If we don't have IFlyoutBase5 available, then we assume a standard show mode.
                            if (!thisAsFlyoutBase5 || thisAsFlyoutBase5.ShowMode() == winrt::FlyoutShowMode::Standard)
                            {
                                commandBar.IsOpen(true);
                            }

                            // When CommandBarFlyout is in AlwaysOpen state, don't show the overflow button
                            if (strongThis->AlwaysExpanded())
                            {
                                commandBar.IsOpen(true);
                                commandBar.OverflowButtonVisibility(winrt::Windows::UI::Xaml::Controls::CommandBarOverflowButtonVisibility::Collapsed);
                            }
                            else
                            {
                                commandBar.OverflowButtonVisibility(winrt::Windows::UI::Xaml::Controls::CommandBarOverflowButtonVisibility::Auto);
                            }
                        }
                    }
                );
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
                else
                {
                    // If we don't have an animation, close the command bar and thus it's subflyouts.
                    commandBar->IsOpen(false);
                }

                //CommandBarFlyoutCommandBar.Closed will be called when
                //clicking the more (...) button, we clear the translations
                //here
                commandBar->ClearShadow();
            }
        }
    });

    // Close the CommandBar in order to ensure that we're always starting from a known state when opening the flyout.
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

    m_presenter.set(presenter);

    m_commandBarOpenedRevoker = commandBar->Opened(winrt::auto_revoke, {
        [this](auto const&, auto const&)
        {
            if (const winrt::IFlyoutBase5 thisAsFlyoutBase5 = *this)
            {
                // If we open the CommandBar, then we should no longer be in a transient show mode -
                // we now know that the user wants to interact with us.
                thisAsFlyoutBase5.ShowMode(winrt::FlyoutShowMode::Standard);
            }
        }
    });

    if (SharedHelpers::Is21H1OrHigher())
    {
        // Since DropShadows don't play well with the entrance animation for the presenter,
        // we'll need to fade it in. This name helps us locate the element to set the fade in
        // flag in the OS code.
        presenter.Name(L"DropShadowFadeInTarget");

        // We'll need to remove the presenter's drop shadow on the commandBar's Opening/Closing
        // because we need it to disappear during its expand/shrink animation when the Overflow is opened.
        // It will be re-added once the storyboard for the overflow animations are completed.
        // That code can be found inside CommandBarFlyoutCommandBar.
        m_commandBarOpeningRevoker = commandBar->Opening(winrt::auto_revoke, {
            [this, presenter](auto const&, auto const&)
            {
                if (const auto commandBar = winrt::get_self<CommandBarFlyoutCommandBar>(m_commandBar.get()))
                {
                    if (commandBar->HasSecondaryOpenCloseAnimations())
                    {
                        // We'll only need to do the mid-animation remove/add when the "..." button is
                        // pressed to open/close the overflow. This means we shouldn't do it for AlwaysExpanded
                        // and if there's nothing in the overflow.
                        if (!AlwaysExpanded() && m_secondaryCommands.Size() > 0)
                        {
                            RemoveDropShadow();
                        }
                    }
                }
            }
            });

        m_commandBarClosingRevoker = commandBar->Closing(winrt::auto_revoke, {
            [this](auto const&, auto const&)
            {
                if (const auto commandBar = winrt::get_self<CommandBarFlyoutCommandBar>(m_commandBar.get()))
                {
                    if (commandBar->HasSecondaryOpenCloseAnimations())
                    {
                        RemoveDropShadow();
                    }
                }
            }
            });
    }

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
        if (auto&& presenter = m_presenter.get())
        {
            winrt::Windows::UI::Xaml::Media::ThemeShadow shadow;
            presenter.Shadow(shadow);
        }
    }
}

void CommandBarFlyout::RemoveDropShadow()
{
    if (SharedHelpers::Is21H1OrHigher())
    {
        if (auto&& presenter = m_presenter.get())
        {
            presenter.Shadow(nullptr);
        }
    }
}

tracker_ref<winrt::FlyoutPresenter> CommandBarFlyout::GetPresenter()
{
    return m_presenter;
}
