// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#include "pch.h"
#include "common.h"
#include "CommandBarFlyoutCommandBar.h"
#include "CommandBarFlyoutCommandBarTemplateSettings.h"

CommandBarFlyoutCommandBar::CommandBarFlyoutCommandBar()
{
    SetDefaultStyleKey(this);

    SetValue(s_FlyoutTemplateSettingsProperty, winrt::make<CommandBarFlyoutCommandBarTemplateSettings>());

    Loaded({
        [this](auto const&, auto const&)
        {
            UpdateUI();

            // If we have no primary commands, then there won't be any elements in the content root to take focus,
            // so we'll give focus to the first secondary item instead, if there is one.
            if (PrimaryCommands().Size() == 0 && SecondaryCommands().Size() > 0)
            {
                m_firstSecondaryItemLoadedRevoker = SecondaryCommands().GetAt(0).as<winrt::FrameworkElement>().Loaded(winrt::auto_revoke, {
                    [this](winrt::IInspectable const& sender, auto const&)
                    {
                        sender.as<winrt::Control>().Focus(winrt::FocusState::Programmatic);
                        m_firstSecondaryItemLoadedRevoker.revoke();
                    }
                });
            }
        }
    });
    
    SizeChanged({ [this](auto const&, auto const&) { UpdateUI(); } });
    Closed({ [this](auto const&, auto const&) { m_secondaryItemsRootSized = false; } });

    RegisterPropertyChangedCallback(
        winrt::AppBar::IsOpenProperty(),
        [this](auto const&, auto const&)
        {
            UpdateFlowsFromAndFlowsTo();
            UpdateUI();
        });

    // Since we own these vectors, we don't need to cache the event tokens -
    // in fact, if we tried to remove these handlers in our destructor,
    // these properties will have already been cleared, and we'll nullref.
    PrimaryCommands().VectorChanged({
        [this](auto const&, auto const&)
        {
            UpdateFlowsFromAndFlowsTo();
            UpdateUI();
        }
    });

    SecondaryCommands().VectorChanged({
        [this](auto const&, auto const&)
        {
            m_secondaryItemsRootSized = false;
            UpdateFlowsFromAndFlowsTo();
            UpdateUI();
        }
    });
}

CommandBarFlyoutCommandBar::~CommandBarFlyoutCommandBar()
{
    DetachEventHandlers(true /* useSafeGet */);
}

void CommandBarFlyoutCommandBar::OnApplyTemplate()
{
    __super::OnApplyTemplate();
    DetachEventHandlers();
    
    winrt::IControlProtected thisAsControlProtected = *this;

    m_primaryItemsRoot.set(GetTemplateChildT<winrt::FrameworkElement>(L"PrimaryItemsRoot", thisAsControlProtected));
    m_secondaryItemsRoot.set(GetTemplateChildT<winrt::FrameworkElement>(L"OverflowContentRoot", thisAsControlProtected));
    m_moreButton.set(GetTemplateChildT<winrt::ButtonBase>(L"MoreButton", thisAsControlProtected));
    m_openingStoryboard.set(GetTemplateChildT<winrt::Storyboard>(L"OpeningStoryboard", thisAsControlProtected));
    m_closingStoryboard.set(GetTemplateChildT<winrt::Storyboard>(L"ClosingStoryboard", thisAsControlProtected));

    AttachEventHandlers();
    UpdateFlowsFromAndFlowsTo();
    UpdateUI(false /* useTransitions */);
}

void CommandBarFlyoutCommandBar::SetOwningFlyout(winrt::CommandBarFlyout const& owningFlyout)
{
    m_owningFlyout = owningFlyout;
}

void CommandBarFlyoutCommandBar::AttachEventHandlers()
{
    // In addition to closing the CommandBar if someone hits the escape key,
    // we also want to close the whole flyout, as well.
    m_keyDownHandler = winrt::box_value<winrt::KeyEventHandler>({
        [this](auto const&, winrt::KeyRoutedEventArgs const& args)
        {
            if (auto owningFlyout = m_owningFlyout.get())
            {
                if (args.Key() == winrt::VirtualKey::Escape)
                {
                    owningFlyout.Hide();
                }
            }
        }
    });

    AddHandler(winrt::UIElement::KeyDownEvent(), m_keyDownHandler, true);

    if (auto secondaryItemsRoot = m_secondaryItemsRoot.get())
    {
        m_secondaryItemsRootSizeChangedRevoker = secondaryItemsRoot.SizeChanged(winrt::auto_revoke, {
            [this](auto const&, auto const&)
            {
                m_secondaryItemsRootSized = true;
                UpdateUI();
            }
        });

        secondaryItemsRoot.AddHandler(winrt::UIElement::KeyDownEvent(), m_keyDownHandler, true);
    }

    if (m_openingStoryboard)
    {
        m_openingStoryboardCompletedRevoker =
            m_openingStoryboard.get().Completed(winrt::auto_revoke, {
                [this](auto const&, auto const&) { m_openingStoryboard.get().Stop(); }
            });
    }

    if (m_closingStoryboard)
    {
        m_closingStoryboardCompletedRevoker =
            m_closingStoryboard.get().Completed(winrt::auto_revoke, {
                [this](auto const&, auto const&) { m_closingStoryboard.get().Stop(); }
            });
    }
}

void CommandBarFlyoutCommandBar::DetachEventHandlers(bool useSafeGet)
{
    if (m_keyDownHandler)
    {
        RemoveHandler(winrt::UIElement::KeyDownEvent(), m_keyDownHandler);

        if (auto secondaryItemsRoot = useSafeGet ? m_secondaryItemsRoot.safe_get() : m_secondaryItemsRoot.get())
        {
            secondaryItemsRoot.RemoveHandler(winrt::UIElement::KeyDownEvent(), m_keyDownHandler);
        }
    }
}

bool CommandBarFlyoutCommandBar::HasOpenAnimation()
{
    return static_cast<bool>(m_openingStoryboard);
}

void CommandBarFlyoutCommandBar::PlayOpenAnimation()
{
    if (auto openingStoryboard = m_openingStoryboard.get())
    {
        if (openingStoryboard.GetCurrentState() != winrt::ClockState::Active)
        {
            openingStoryboard.Begin();
        }
    }
}

bool CommandBarFlyoutCommandBar::HasCloseAnimation()
{
    return static_cast<bool>(m_closingStoryboard);
}

void CommandBarFlyoutCommandBar::PlayCloseAnimation(std::function<void()> onCompleteFunc)
{
    if (auto closingStoryboard = m_closingStoryboard.get())
    {
        if (closingStoryboard.GetCurrentState() != winrt::ClockState::Active)
        {
            m_closingStoryboardCompletedCallbackRevoker = closingStoryboard.Completed(winrt::auto_revoke, {
                [this, onCompleteFunc](auto const&, auto const&)
                {
                    onCompleteFunc();
                }
            });

            UpdateTemplateSettings();
            closingStoryboard.Begin();
        }
    }
    else
    {
        onCompleteFunc();
    }
}

void CommandBarFlyoutCommandBar::UpdateFlowsFromAndFlowsTo()
{
    if (m_currentPrimaryItemsEndElement)
    {
        winrt::AutomationProperties::GetFlowsTo(m_currentPrimaryItemsEndElement.get()).Clear();
        m_currentPrimaryItemsEndElement.set(nullptr);
    }

    if (m_currentSecondaryItemsStartElement)
    {
        winrt::AutomationProperties::GetFlowsFrom(m_currentSecondaryItemsStartElement.get()).Clear();
        m_currentSecondaryItemsStartElement.set(nullptr);
    }

    // If we're not open, then we don't want to do anything special - the only time we do need to do something special
    // is when the secondary commands are showing, in which case we want to connect the primary and secondary command lists.
    if (IsOpen())
    {
        auto isElementFocusable = [](winrt::ICommandBarElement const& element)
        {
            winrt::Control primaryCommandAsControl = element.try_as<winrt::Control>();
            return
                primaryCommandAsControl &&
                primaryCommandAsControl.Visibility() == winrt::Visibility::Visible &&
                (primaryCommandAsControl.IsEnabled() || primaryCommandAsControl.AllowFocusWhenDisabled()) &&
                primaryCommandAsControl.IsTabStop();
        };

        // If we have a more button, then that's the last element in our primary items list.
        // Otherwise, we'll find the last focusable element in the primary commands.
        if (m_moreButton)
        {
            m_currentPrimaryItemsEndElement.set(m_moreButton.get());
        }
        else
        {
            auto primaryCommands = PrimaryCommands();
            for (int i = static_cast<int>(primaryCommands.Size() - 1); i >= 0; i--)
            {
                auto primaryCommand = primaryCommands.GetAt(i);
                if (isElementFocusable(primaryCommand))
                {
                    m_currentPrimaryItemsEndElement.set(primaryCommand.try_as<winrt::FrameworkElement>());
                    break;
                }
            }
        }

        for (const auto& secondaryCommand : SecondaryCommands())
        {
            if (isElementFocusable(secondaryCommand))
            {
                m_currentSecondaryItemsStartElement.set(secondaryCommand.try_as<winrt::FrameworkElement>());
                break;
            }
        }

        if (m_currentPrimaryItemsEndElement && m_currentSecondaryItemsStartElement)
        {
            winrt::AutomationProperties::GetFlowsTo(m_currentPrimaryItemsEndElement.get()).Append(m_currentSecondaryItemsStartElement.get());
            winrt::AutomationProperties::GetFlowsFrom(m_currentSecondaryItemsStartElement.get()).Append(m_currentPrimaryItemsEndElement.get());
        }
    }
}

void CommandBarFlyoutCommandBar::UpdateUI(bool useTransitions)
{
    UpdateTemplateSettings();
    UpdateVisualState(useTransitions);
}

void CommandBarFlyoutCommandBar::UpdateVisualState(bool useTransitions)
{
    // If we're currently open, have overflow items, and haven't yet sized our overflow item root,
    // then we want to wait until then to update visual state - otherwise, we'll be animating
    // to incorrect values.  Animations only retrieve values from bindings when they begin,
    // so if we begin an animation and then update a bound template setting, that won't take effect.
    if (IsOpen() && !m_secondaryItemsRootSized)
    {
        return;
    }

    if (IsOpen())
    {
        bool shouldExpandUp = false;

        // If there isn't enough space to display the overflow below the command bar,
        // and if there is enough space above, then we'll display it above instead.
        if (auto window = winrt::Window::Current() && m_secondaryItemsRoot)
        {
            double availableHeight = -1;
            bool isConstrainedToRootBounds = true;
            auto controlBounds = TransformToVisual(nullptr).TransformBounds({ 0, 0, static_cast<float>(ActualWidth()), static_cast<float>(ActualHeight()) });
            
#ifdef USE_INTERNAL_SDK
            if (winrt::IFlyoutBase6 owningFlyoutAsFlyoutBase6 = m_owningFlyout.get())
            {
                isConstrainedToRootBounds = owningFlyoutAsFlyoutBase6.IsConstrainedToRootBounds();
            }
#endif

            try
            {
                // Note: this doesn't work right for islands scenarios
                // Bug 19617460: CommandBarFlyoutCommandBar isn't able to decide whether to open up or down because it doesn't know where it is relative to the monitor
                auto view = winrt::ApplicationView::GetForCurrentView();
                availableHeight = view.VisibleBounds().Height;
            }
            catch (winrt::hresult_error)
            {
                // Calling GetForCurrentView on threads without a CoreWindow throws an error. This comes up in places like LogonUI.
                // In this circumstance, we'll just always expand down, since we can't get bounds information.
            }

            if (availableHeight >= 0)
            {
                m_secondaryItemsRoot.get().Measure({ std::numeric_limits<float>::infinity(), std::numeric_limits<float>::infinity() });
                auto overflowPopupSize = m_secondaryItemsRoot.get().DesiredSize();

                shouldExpandUp =
                    controlBounds.Y + controlBounds.Height + overflowPopupSize.Height > availableHeight &&
                    controlBounds.Y - overflowPopupSize.Height >= 0;
            }
        }

        if (shouldExpandUp)
        {
            winrt::VisualStateManager::GoToState(*this, L"ExpandedUp", useTransitions);
        }
        else
        {
            winrt::VisualStateManager::GoToState(*this, L"ExpandedDown", useTransitions);
        }

        // Union of AvailableCommandsStates and ExpansionStates
        bool hasPrimaryCommands = (PrimaryCommands().Size() != 0);
        if (hasPrimaryCommands)
        {
            if (shouldExpandUp)
            {
                winrt::VisualStateManager::GoToState(*this, L"ExpandedUpWithPrimaryCommands", useTransitions);
            }
            else
            {
                winrt::VisualStateManager::GoToState(*this, L"ExpandedDownWithPrimaryCommands", useTransitions);
            }
        }
        else
        {
            if (shouldExpandUp)
            {
                winrt::VisualStateManager::GoToState(*this, L"ExpandedUpWithoutPrimaryCommands", useTransitions);
            }
            else
            {
                winrt::VisualStateManager::GoToState(*this, L"ExpandedDownWithoutPrimaryCommands", useTransitions);
            }
        }
    }
    else
    {
        winrt::VisualStateManager::GoToState(*this, L"Collapsed", useTransitions);
    }
}

void CommandBarFlyoutCommandBar::UpdateTemplateSettings()
{
    if (m_primaryItemsRoot && m_secondaryItemsRoot)
    {
        auto flyoutTemplateSettings = winrt::get_self<CommandBarFlyoutCommandBarTemplateSettings>(FlyoutTemplateSettings());

        winrt::Size infiniteSize = { std::numeric_limits<float>::infinity(), std::numeric_limits<float>::infinity() };
        m_primaryItemsRoot.get().Measure(infiniteSize);
        winrt::Size primaryItemsRootDesiredSize = m_primaryItemsRoot.get().DesiredSize();
        float collapsedWidth = primaryItemsRootDesiredSize.Width;

        if (m_secondaryItemsRoot)
        {
            m_secondaryItemsRoot.get().Measure(infiniteSize);
            auto overflowPopupSize = m_secondaryItemsRoot.get().DesiredSize();

            flyoutTemplateSettings->ExpandedWidth(std::max(collapsedWidth, overflowPopupSize.Width));
            flyoutTemplateSettings->ExpandUpOverflowVerticalPosition(-overflowPopupSize.Height);
            flyoutTemplateSettings->ExpandUpAnimationStartPosition(overflowPopupSize.Height / 2);
            flyoutTemplateSettings->ExpandUpAnimationEndPosition(0);
            flyoutTemplateSettings->ExpandUpAnimationHoldPosition(overflowPopupSize.Height);
            flyoutTemplateSettings->ExpandDownAnimationStartPosition(-overflowPopupSize.Height / 2);
            flyoutTemplateSettings->ExpandDownAnimationEndPosition(0);
            flyoutTemplateSettings->ExpandDownAnimationHoldPosition(-overflowPopupSize.Height);
            // This clip needs to cover the border at the bottom of the overflow otherwise it'll 
            // clip the border. The measure size seems slightly off from what we eventually require
            // so we're going to compensate just a bit to make sure there's room for any borders.
            flyoutTemplateSettings->OverflowContentClipRect({ 0, 0, static_cast<float>(flyoutTemplateSettings->ExpandedWidth()), overflowPopupSize.Height + 2 });
        }
        else
        {
            flyoutTemplateSettings->ExpandedWidth(collapsedWidth);
            flyoutTemplateSettings->ExpandUpOverflowVerticalPosition(0);
            flyoutTemplateSettings->ExpandUpAnimationStartPosition(0);
            flyoutTemplateSettings->ExpandUpAnimationEndPosition(0);
            flyoutTemplateSettings->ExpandUpAnimationHoldPosition(0);
            flyoutTemplateSettings->ExpandDownAnimationStartPosition(0);
            flyoutTemplateSettings->ExpandDownAnimationEndPosition(0);
            flyoutTemplateSettings->ExpandDownAnimationHoldPosition(0);
            flyoutTemplateSettings->OverflowContentClipRect({ 0, 0, 0, 0 });
        }

        double expandedWidth = flyoutTemplateSettings->ExpandedWidth();

        // If collapsedWidth is 0, then we'll never be showing in collapsed mode,
        // so we'll set it equal to expandedWidth to ensure that our open/close animations are correct.
        if (collapsedWidth == 0)
        {
            collapsedWidth = static_cast<float>(expandedWidth);
        }

        flyoutTemplateSettings->WidthExpansionDelta(collapsedWidth - expandedWidth);
        flyoutTemplateSettings->WidthExpansionAnimationStartPosition(-flyoutTemplateSettings->WidthExpansionDelta() / 2.0);
        flyoutTemplateSettings->WidthExpansionAnimationEndPosition(-flyoutTemplateSettings->WidthExpansionDelta());
        flyoutTemplateSettings->ContentClipRect({ 0, 0, static_cast<float>(expandedWidth), primaryItemsRootDesiredSize.Height });

        if (IsOpen())
        {
            flyoutTemplateSettings->CurrentWidth(expandedWidth);
        }
        else
        {
            flyoutTemplateSettings->CurrentWidth(collapsedWidth);
        }

        // If we're currently playing the close animation, don't update these properties -
        // the animation is expecting them not to change out from under it.
        // After the close animation has completed, the flyout will close and no further
        // visual updates will occur, so there's no need to update these values in that case.
        bool isPlayingCloseAnimation = false;

        if (auto closingStoryboard = m_closingStoryboard.get())
        {
            isPlayingCloseAnimation = closingStoryboard.GetCurrentState() == winrt::ClockState::Active;
        }

        if (!isPlayingCloseAnimation)
        {
            if (IsOpen())
            {
                flyoutTemplateSettings->OpenAnimationStartPosition(-expandedWidth / 2);
                flyoutTemplateSettings->OpenAnimationEndPosition(0);
            }
            else
            {
                flyoutTemplateSettings->OpenAnimationStartPosition(flyoutTemplateSettings->WidthExpansionDelta() - collapsedWidth / 2);
                flyoutTemplateSettings->OpenAnimationEndPosition(flyoutTemplateSettings->WidthExpansionDelta());
            }

            flyoutTemplateSettings->CloseAnimationEndPosition(-expandedWidth);
        }

        flyoutTemplateSettings->WidthExpansionMoreButtonAnimationStartPosition(flyoutTemplateSettings->WidthExpansionDelta() / 2);
        flyoutTemplateSettings->WidthExpansionMoreButtonAnimationEndPosition(flyoutTemplateSettings->WidthExpansionDelta());

        if (PrimaryCommands().Size() > 0)
        {
            flyoutTemplateSettings->ExpandDownOverflowVerticalPosition(Height());
        }
        else
        {
            flyoutTemplateSettings->ExpandDownOverflowVerticalPosition(0);
        }
    }
}
