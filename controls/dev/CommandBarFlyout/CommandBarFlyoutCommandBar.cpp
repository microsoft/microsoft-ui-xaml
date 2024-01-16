// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#include "pch.h"
#include "common.h"
#include "CommandBarFlyout.h"
#include "CommandBarFlyoutCommandBar.h"
#include "CommandBarFlyoutCommandBarTemplateSettings.h"
#include "ResourceAccessor.h"
#include "TypeLogging.h"
#include "Vector.h"
#include "velocity.h"
#include <FrameworkUdk/Containment.h>

// Bug 47050253: [1.4 servicing] Insiders report that even with the latest bug fixes in the dev channel they can still see the context menu with a transparent background sometimes
#define WINAPPSDK_CHANGEID_47050253 47050253

// Bug 48006563: [1.4 servicing] File Explorer crash due to bad reentrancy from CommandBarFlyoutCommandBar calling FocusCommand (STOWED_EXCEPTION_8000ffff_Microsoft.UI.Xaml.dll!CXcpDispatcher::OnReentrancyProtectedWindowMessage)
#define WINAPPSDK_CHANGEID_48006563 48006563

CommandBarFlyoutCommandBar::CommandBarFlyoutCommandBar()
{
    SetDefaultStyleKey(this);

    SetValue(s_FlyoutTemplateSettingsProperty, winrt::make<CommandBarFlyoutCommandBarTemplateSettings>());

    Loaded({
        [this](auto const&, auto const&)
        {
#ifdef DBG
            COMMANDBARFLYOUT_TRACE_INFO(*this, TRACE_MSG_METH_STR, METH_NAME, this, L"Loaded");
#endif

            UpdateUI(!m_commandBarFlyoutIsOpening);

            if (auto owningFlyout = m_owningFlyout.get())
            {
                // We only want to focus an initial element if we're opening in standard mode -
                // in transient mode, we don't want to be taking focus.
                if (owningFlyout.ShowMode() == winrt::FlyoutShowMode::Standard)
                {
                    // Programmatically focus the first primary command if any, else programmatically focus the first secondary command if any.
                    auto commands = PrimaryCommands().Size() > 0 ? PrimaryCommands() : (SecondaryCommands().Size() > 0 ? SecondaryCommands() : nullptr);

                    if (commands)
                    {
                        const bool usingPrimaryCommands = commands == PrimaryCommands();
                        const bool ensureTabStopUniqueness = usingPrimaryCommands;
                        const auto firstCommandAsFrameworkElement = commands.GetAt(0).try_as<winrt::FrameworkElement>();

                        if (firstCommandAsFrameworkElement)
                        {
                            if (firstCommandAsFrameworkElement.IsLoaded())
                            {
                                if (WinAppSdk::Containment::IsChangeEnabled<WINAPPSDK_CHANGEID_48006563>())
                                {
                                    FocusCommandAsync(
                                        commands,
                                        usingPrimaryCommands ? m_moreButton.get() : nullptr /*moreButton*/,
                                        winrt::FocusState::Programmatic /*focusState*/,
                                        true /*firstCommand*/,
                                        ensureTabStopUniqueness);
                                }
                                else
                                {
                                    FocusCommandSync(
                                        commands,
                                        usingPrimaryCommands ? m_moreButton.get() : nullptr /*moreButton*/,
                                        winrt::FocusState::Programmatic /*focusState*/,
                                        true /*firstCommand*/,
                                        ensureTabStopUniqueness);
                                }
                            }
                            else
                            {
                                m_firstItemLoadedRevoker = firstCommandAsFrameworkElement.Loaded(winrt::auto_revoke,
                                {
                                    [this, commands, usingPrimaryCommands, ensureTabStopUniqueness](winrt::IInspectable const& sender, auto const&)
                                    {
                                        if (WinAppSdk::Containment::IsChangeEnabled<WINAPPSDK_CHANGEID_48006563>())
                                        {
                                            FocusCommandAsync(
                                                commands,
                                                usingPrimaryCommands ? m_moreButton.get() : nullptr /*moreButton*/,
                                                winrt::FocusState::Programmatic /*focusState*/,
                                                true /*firstCommand*/,
                                                ensureTabStopUniqueness);
                                        }
                                        else
                                        {
                                            FocusCommandSync(
                                                commands,
                                                usingPrimaryCommands ? m_moreButton.get() : nullptr /*moreButton*/,
                                                winrt::FocusState::Programmatic /*focusState*/,
                                                true /*firstCommand*/,
                                                ensureTabStopUniqueness);
                                        }

                                        m_firstItemLoadedRevoker.revoke();
                                    }
                                });
                            }
                        }
                    }
                }
            }
        }
    });

    SizeChanged({
        [this](auto const&, auto const&)
        {
#ifdef DBG
            COMMANDBARFLYOUT_TRACE_VERBOSE(*this, TRACE_MSG_METH_STR, METH_NAME, this, L"SizeChanged");
#endif

            UpdateUI(!m_commandBarFlyoutIsOpening, true /*isForSizeChange*/);
        }
    });

    Closing({
        [this](auto const&, auto const&)
        {
#ifdef DBG
            COMMANDBARFLYOUT_TRACE_VERBOSE(*this, TRACE_MSG_METH_STR, METH_NAME, this, L"Closing");
#endif

            if (auto owningFlyout = m_owningFlyout.get())
            {
                if (owningFlyout.AlwaysExpanded())
                {
                    // Don't close the secondary commands list when the flyout is AlwaysExpanded.
                    IsOpen(true);
                }
            }
        }
    });

    Closed({
        [this](auto const&, auto const&)
        {
#ifdef DBG
            COMMANDBARFLYOUT_TRACE_VERBOSE(*this, TRACE_MSG_METH_STR, METH_NAME, this, L"Closed");
#endif

            m_secondaryItemsRootSized = false;
        }
    });

    RegisterPropertyChangedCallback(
        winrt::AppBar::IsOpenProperty(),
        [this](auto const&, auto const&)
        {
#ifdef DBG
            COMMANDBARFLYOUT_TRACE_VERBOSE(*this, TRACE_MSG_METH_STR, METH_NAME, this, L"IsOpenProperty changed");
#endif

            UpdateFlowsFromAndFlowsTo();
            UpdateUI(!m_commandBarFlyoutIsOpening);
        });

    // Since we own these vectors, we don't need to cache the event tokens -
    // in fact, if we tried to remove these handlers in our destructor,
    // these properties will have already been cleared, and we'll nullref.
    PrimaryCommands().VectorChanged({
        [this](auto const&, auto const&)
        {
#ifdef DBG
            COMMANDBARFLYOUT_TRACE_VERBOSE(*this, TRACE_MSG_METH_STR, METH_NAME, this, L"PrimaryCommands VectorChanged");
#endif

            EnsureLocalizedControlTypes();
            PopulateAccessibleControls();
            UpdateFlowsFromAndFlowsTo();
            UpdateUI(!m_commandBarFlyoutIsOpening);
        }
    });

    SecondaryCommands().VectorChanged({
        [this](auto const&, auto const&)
        {
#ifdef DBG
            COMMANDBARFLYOUT_TRACE_VERBOSE(*this, TRACE_MSG_METH_STR, METH_NAME, this, L"SecondaryCommands VectorChanged");
#endif

            m_secondaryItemsRootSized = false;
            EnsureLocalizedControlTypes();
            PopulateAccessibleControls();
            UpdateFlowsFromAndFlowsTo();
            UpdateUI(!m_commandBarFlyoutIsOpening);
        }
    });
}

CommandBarFlyoutCommandBar::~CommandBarFlyoutCommandBar()
{
    if (WinAppSdk::Containment::IsChangeEnabled<WINAPPSDK_CHANGEID_47050253>())
    {
        // The SystemBackdrop DP has already been cleared out. Use our cached field.
        if (auto systemBackdrop = m_systemBackdrop.get())
        {
            systemBackdrop.OnTargetDisconnected(m_backdropLink);
            systemBackdrop.OnTargetDisconnected(m_overflowPopupBackdropLink);
        }
    }
}

void CommandBarFlyoutCommandBar::OnApplyTemplate()
{
    COMMANDBARFLYOUT_TRACE_INFO(*this, TRACE_MSG_METH, METH_NAME, this);

    __super::OnApplyTemplate();
    DetachEventHandlers();

    if (auto& overflowPopup = m_overflowPopup.get())
    {
        overflowPopup.PlacementTarget(nullptr);
        overflowPopup.DesiredPlacement(winrt::PopupPlacementMode::Auto);
    }

    winrt::AutomationProperties::SetLocalizedControlType(*this, ResourceAccessor::GetLocalizedStringResource(SR_CommandBarFlyoutCommandBarLocalizedControlType));
    EnsureLocalizedControlTypes();

    winrt::IControlProtected thisAsControlProtected = *this;

    m_primaryItemsRoot.set(GetTemplateChildT<winrt::FrameworkElement>(L"PrimaryItemsRoot", thisAsControlProtected));
    m_overflowPopup.set(GetTemplateChildT<winrt::Popup>(L"OverflowPopup", thisAsControlProtected));
    m_secondaryItemsRoot.set(GetTemplateChildT<winrt::FrameworkElement>(L"OverflowContentRoot", thisAsControlProtected));
    m_moreButton.set(GetTemplateChildT<winrt::ButtonBase>(L"MoreButton", thisAsControlProtected));
    m_openingStoryboard.set([this, thisAsControlProtected]()
        {
            if (auto const opacityStoryBoard = GetTemplateChildT<winrt::Storyboard>(L"OpeningOpacityStoryboard", thisAsControlProtected))
            {
                m_openAnimationKind = CommandBarFlyoutOpenCloseAnimationKind::Opacity;
                return opacityStoryBoard;
            }
            m_openAnimationKind = CommandBarFlyoutOpenCloseAnimationKind::Clip;
            return GetTemplateChildT<winrt::Storyboard>(L"OpeningStoryboard", thisAsControlProtected);
        }());
    m_closingStoryboard.set([this, thisAsControlProtected]()
        {
            if (auto const opacityStoryBoard = GetTemplateChildT<winrt::Storyboard>(L"ClosingOpacityStoryboard", thisAsControlProtected))
            {
                return opacityStoryBoard;
            }
            return GetTemplateChildT<winrt::Storyboard>(L"ClosingStoryboard", thisAsControlProtected);
        }());

    m_primaryItemsSystemBackdropRoot.set(GetTemplateChildT<winrt::FrameworkElement>(L"PrimaryItemsSystemBackdropRoot", thisAsControlProtected));
    m_overflowPopupSystemBackdropRoot.set(GetTemplateChildT<winrt::FrameworkElement>(L"OverflowPopupSystemBackdropRoot", thisAsControlProtected));
    m_outerOverflowContentRootV2.set(GetTemplateChildT<winrt::FrameworkElement>(L"OuterOverflowContentRootV2", thisAsControlProtected));

    auto& primaryItemsRoot = m_primaryItemsRoot.get();
    auto& primaryItemsSystemBackdropRoot = m_primaryItemsSystemBackdropRoot.get();
    if (primaryItemsRoot && primaryItemsSystemBackdropRoot && m_backdropLink)
    {
        winrt::Visual placementVisual = m_backdropLink.PlacementVisual();

        // Hard-code a large size for the placement visual. The size and position of this lifted visual controls the
        // size and position of the system visual with the backdrop. This visual is parented in a windowed popup, so it
        // should use the popup's coordinate space, but we're seeing it use the main island's coordinate space instead.
        // We don't easily have the popup hwnd's offset from outside MUX.dll, so just size the placement visual to a
        // large number to cover everything. We'll apply a clip to this placement visual later to size it to the
        // CommandBarFlyoutCommandBar's contents.
        placementVisual.Size({10000, 10000});
        // Replace with this when the coordinate space issue is fixed - http://task.ms/44279950
        //placementVisual.Size({static_cast<float>(primaryItemsRoot.ActualWidth()), static_cast<float>(primaryItemsRoot.ActualHeight())});

        winrt::ElementCompositionPreview::SetElementChildVisual(primaryItemsSystemBackdropRoot, placementVisual);
    }

    const auto& overflowContentRootV2 = m_outerOverflowContentRootV2.get();
    const auto& overflowPopupSystemBackdropRoot = m_overflowPopupSystemBackdropRoot.get();
    if (overflowContentRootV2 && overflowPopupSystemBackdropRoot && m_overflowPopupBackdropLink)
    {
        winrt::Visual popupPlacementVisual = m_overflowPopupBackdropLink.PlacementVisual();

        // Use a hardcoded size. See above.
        popupPlacementVisual.Size({10000, 10000});
        // Replace with this when the coordinate space issue is fixed - http://task.ms/44279950
        //popupPlacementVisual.Size({static_cast<float>(overflowContentRootV2.ActualWidth()), static_cast<float>(overflowContentRootV2.ActualHeight())});

        // Don't put the overflow popup's backdrop visual in the tree yet. The popup may not be open yet and we don't
        // want anything to flicker. The backdrop will be put in the tree in UpdateVisualState once we've opened and
        // measured the overflow popup.
    }

    if (auto& overflowPopup = m_overflowPopup.get())
    {
        overflowPopup.PlacementTarget(m_primaryItemsRoot.get());
        overflowPopup.DesiredPlacement(winrt::PopupPlacementMode::BottomEdgeAlignedLeft);
    }

    if (auto& moreButton = m_moreButton.get())
    {
        // Initially only the first focusable primary and secondary commands
        // keep their IsTabStop set to True.
        if (moreButton.IsTabStop())
        {
            moreButton.IsTabStop(false);
        }
    }

    if (m_owningFlyout)
    {
        AttachEventsToSecondaryStoryboards();
    }

    // Keep the owning FlyoutPresenter's corner radius in sync with the
    // primary commands's corner radius.
    BindOwningFlyoutPresenterToCornerRadius();

    AttachEventHandlers();
    PopulateAccessibleControls();
    UpdateFlowsFromAndFlowsTo();
    UpdateUI(false /* useTransitions */);
    SetPresenterName(m_flyoutPresenter.get());
}

void CommandBarFlyoutCommandBar::SetOwningFlyout(
    winrt::CommandBarFlyout const& owningFlyout)
{
    m_owningFlyout = owningFlyout;
}

void CommandBarFlyoutCommandBar::AttachEventHandlers()
{
    COMMANDBARFLYOUT_TRACE_INFO(*this, TRACE_MSG_METH, METH_NAME, this);

    if (auto overflowPopup = m_overflowPopup.get())
    {
        m_overflowPopupActualPlacementChangedRevoker = overflowPopup.ActualPlacementChanged(winrt::auto_revoke,
        {
            [this](auto const&, auto const&)
            {
#ifdef DBG
                COMMANDBARFLYOUT_TRACE_VERBOSE(*this, TRACE_MSG_METH_STR, METH_NAME, this, L"OverflowPopup ActualPlacementChanged");
#endif

                UpdateUI();
            }
        });
    }

    if (auto secondaryItemsRoot = m_secondaryItemsRoot.get())
    {
        m_secondaryItemsRootSizeChangedRevoker = secondaryItemsRoot.SizeChanged(winrt::auto_revoke,
        {
            [this](auto const&, auto const&)
            {
#ifdef DBG
                COMMANDBARFLYOUT_TRACE_VERBOSE(*this, TRACE_MSG_METH_STR, METH_NAME, this, L"secondaryItemsRoot SizeChanged");
#endif

                m_secondaryItemsRootSized = true;
                UpdateUI(!m_commandBarFlyoutIsOpening, true /*isForSizeChange*/);
            }
        });

        m_secondaryItemsRootPreviewKeyDownRevoker = secondaryItemsRoot.PreviewKeyDown(winrt::auto_revoke,
        {
            [this](auto const&, winrt::KeyRoutedEventArgs const& args)
            {
                COMMANDBARFLYOUT_TRACE_INFO(*this, TRACE_MSG_METH_STR, METH_NAME, this,
                    TypeLogging::KeyRoutedEventArgsToString(args).c_str());

                if (args.Handled())
                {
                    return;
                }

                switch (args.Key())
                {
                case winrt::VirtualKey::Escape:
                {
                    // In addition to closing the CommandBar if someone hits the escape key,
                    // we also want to close the whole flyout.
                    if (auto owningFlyout = m_owningFlyout.get())
                    {
                        owningFlyout.Hide();
                    }
                    break;
                }

                case winrt::VirtualKey::Down:
                case winrt::VirtualKey::Up:
                {
                    OnKeyDown(args);
                    break;
                }
                }
            }
        });
    }

    if (m_openingStoryboard)
    {
        m_openingStoryboardCompletedRevoker =
            m_openingStoryboard.get().Completed(winrt::auto_revoke,
            {
                [this](auto const&, auto const&) { m_openingStoryboard.get().Stop(); }
            });
    }
}

void CommandBarFlyoutCommandBar::DetachEventHandlers()
{
    COMMANDBARFLYOUT_TRACE_INFO(*this, TRACE_MSG_METH, METH_NAME, this);

    m_keyDownRevoker.revoke();
    m_secondaryItemsRootPreviewKeyDownRevoker.revoke();
    m_secondaryItemsRootSizeChangedRevoker.revoke();
    m_firstItemLoadedRevoker.revoke();
    m_openingStoryboardCompletedRevoker.revoke();
    m_closingStoryboardCompletedCallbackRevoker.revoke();
    m_expandedUpToCollapsedStoryboardRevoker.revoke();
    m_expandedDownToCollapsedStoryboardRevoker.revoke();
    m_collapsedToExpandedUpStoryboardRevoker.revoke();
    m_collapsedToExpandedDownStoryboardRevoker.revoke();
}

bool CommandBarFlyoutCommandBar::HasOpenAnimation()
{
    return static_cast<bool>(m_openingStoryboard) && SharedHelpers::IsAnimationsEnabled();
}

void CommandBarFlyoutCommandBar::PlayOpenAnimation()
{
    if (auto const& closingStoryboard = m_closingStoryboard.get())
    {
        closingStoryboard.Stop();
    }

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
    return static_cast<bool>(m_closingStoryboard) && SharedHelpers::IsAnimationsEnabled();
}

void CommandBarFlyoutCommandBar::PlayCloseAnimation(
    const winrt::weak_ref<winrt::CommandBarFlyout>& weakCommandBarFlyout,
    std::function<void()> onCompleteFunc)
{
    COMMANDBARFLYOUT_TRACE_INFO(*this, TRACE_MSG_METH, METH_NAME, this);

    if (auto closingStoryboard = m_closingStoryboard.get())
    {
        if (closingStoryboard.GetCurrentState() != winrt::ClockState::Active)
        {
            m_closingStoryboardCompletedCallbackRevoker = closingStoryboard.Completed(winrt::auto_revoke,
            {
                [this, weakCommandBarFlyout, onCompleteFunc](auto const&, auto const&)
                {
                    if (weakCommandBarFlyout.get())
                    {
                        onCompleteFunc();
                    }
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
    auto moreButton = m_moreButton.get();

    // Ensure there is only one focusable command with IsTabStop set to True
    // to enable tabbing from primary to secondary commands and vice-versa
    // with a single Tab keystroke.
    EnsureTabStopUniqueness(PrimaryCommands(), moreButton);
    EnsureTabStopUniqueness(SecondaryCommands(), nullptr);

    // Ensure the SizeOfSet and PositionInSet automation properties
    // for the primary commands and the MoreButton account for the
    // potential MoreButton.
    EnsureAutomationSetCountAndPosition();

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
            return IsControlFocusable(primaryCommandAsControl, false);
        };

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

        // If we have a more button and at least one focusable primary item, then
        // we'll use the more button as the last element in our primary items list.
        if (moreButton && moreButton.Visibility() == winrt::Visibility::Visible && m_currentPrimaryItemsEndElement)
        {
            m_currentPrimaryItemsEndElement.set(moreButton);
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

void CommandBarFlyoutCommandBar::UpdateUI(
    bool useTransitions, bool isForSizeChange)
{
    COMMANDBARFLYOUT_TRACE_VERBOSE(*this, TRACE_MSG_METH_INT_INT, METH_NAME, this, useTransitions, isForSizeChange);

    UpdateTemplateSettings();
    UpdateVisualState(useTransitions, isForSizeChange);
}

void CommandBarFlyoutCommandBar::UpdateVisualState(
    bool useTransitions,
    bool isForSizeChange)
{
    if (IsOpen())
    {
        //
        // If we're currently open, have overflow items, and haven't yet sized our overflow item root, then we want to
        // wait until then to update visual state - otherwise, we'll be animating to incorrect values.  Animations only
        // retrieve values from bindings when they begin, so if we begin an animation and then update a bound template
        // setting, that won't take effect.
        //
        // This also skips copying the size into the placement visual's clip. If the secondary items haven't been sized
        // yet, then we can't copy that size into the clip. Note that we don't want to remove the backdrop placement
        // visual from the tree. It's possible for the items to change while their layout size stay the same, in which
        // case the m_secondaryItemsRootSized flag will never be reset. In that case the old placement visual still has
        // the correct size, so leave it in the tree.
        //
        if (!m_secondaryItemsRootSized)
        {
            return;
        }

        bool shouldExpandUp = false;
        bool hadActualPlacement = false;

        if (auto overflowPopup = m_overflowPopup.get())
        {
            // If we have a value set for ActualPlacement, then we'll directly use that -
            // that tells us where our popup's been placed, so we don't need to try to
            // infer where it should go.
            if (overflowPopup.ActualPlacement() != winrt::PopupPlacementMode::Auto)
            {
                hadActualPlacement = true;
                shouldExpandUp =
                    overflowPopup.ActualPlacement() == winrt::PopupPlacementMode::TopEdgeAlignedLeft ||
                    overflowPopup.ActualPlacement() == winrt::PopupPlacementMode::TopEdgeAlignedRight;
            }
        }

        // If there isn't enough space to display the overflow below the command bar,
        // and if there is enough space above, then we'll display it above instead.
        if (m_secondaryItemsRoot)
        {
            double availableHeight = -1;
            const auto controlBounds = TransformToVisual(nullptr).TransformBounds({ 0, 0, static_cast<float>(ActualWidth()), static_cast<float>(ActualHeight()) });

            if (winrt::CoreWindow::GetForCurrentThread())
            {
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
            }

            if (availableHeight >= 0)
            {
                m_secondaryItemsRoot.get().Measure({ std::numeric_limits<float>::infinity(), std::numeric_limits<float>::infinity() });
                const auto overflowPopupSize = m_secondaryItemsRoot.get().DesiredSize();

                shouldExpandUp =
                    controlBounds.Y + controlBounds.Height + overflowPopupSize.Height > availableHeight &&
                    controlBounds.Y - overflowPopupSize.Height >= 0;
            }
        }

        if (isForSizeChange)
        {
            // UpdateVisualState is called as a result of a size change (for instance caused by a secondary command bar element dependency property change). This CommandBarFlyoutCommandBar is already open
            // and expanded. Jump to the Collapsed and back to ExpandedUp/ExpandedDown state to apply all refreshed CommandBarFlyoutCommandBarTemplateSettings values.
            winrt::VisualStateManager::GoToState(*this, L"Collapsed", false);
        }

        winrt::VisualStateManager::GoToState(*this, shouldExpandUp ? L"ExpandedUp" : L"ExpandedDown", useTransitions && !isForSizeChange);

        // Union of AvailableCommandsStates and ExpansionStates
        bool hasPrimaryCommands = (PrimaryCommands().Size() != 0);
        if (hasPrimaryCommands)
        {
            if (shouldExpandUp)
            {
                winrt::VisualStateManager::GoToState(*this, L"NoOuterOverflowContentRootShadow", useTransitions);
                winrt::VisualStateManager::GoToState(*this, L"ExpandedUpWithPrimaryCommands", useTransitions);
            }
            else
            {
                winrt::VisualStateManager::GoToState(*this, L"OuterOverflowContentRootShadow", useTransitions);
                winrt::VisualStateManager::GoToState(*this, L"ExpandedDownWithPrimaryCommands", useTransitions);
            }
        }
        else
        {
            winrt::VisualStateManager::GoToState(*this, L"OuterOverflowContentRootShadow", useTransitions);
            if (shouldExpandUp)
            {
                winrt::VisualStateManager::GoToState(*this, L"ExpandedUpWithoutPrimaryCommands", useTransitions);
            }
            else
            {
                winrt::VisualStateManager::GoToState(*this, L"ExpandedDownWithoutPrimaryCommands", useTransitions);
            }
        }

        // Update the corner radius clip on the backdrop. We copy the corner radius from the elements in the template,
        // which depends on the visual state of the CommandBarFlyoutCommandBar.
        if (m_overflowPopupBackdropLink && m_outerOverflowContentRootV2)
        {
            winrt::Visual popupPlacementVisual = m_overflowPopupBackdropLink.PlacementVisual();
            winrt::Grid overflowContentRootV2 = m_outerOverflowContentRootV2.get().try_as<winrt::Grid>();
            auto cornerRadius = overflowContentRootV2.CornerRadius();

            auto compositor = popupPlacementVisual.Compositor();
            winrt::RectangleClip rectangleClip = compositor.CreateRectangleClip();
            rectangleClip.Right(static_cast<float>(overflowContentRootV2.ActualWidth()));
            rectangleClip.Bottom(static_cast<float>(overflowContentRootV2.ActualHeight()));
            rectangleClip.TopLeftRadius({static_cast<float>(cornerRadius.TopLeft), static_cast<float>(cornerRadius.TopLeft)});
            rectangleClip.TopRightRadius({static_cast<float>(cornerRadius.TopRight), static_cast<float>(cornerRadius.TopRight)});
            rectangleClip.BottomLeftRadius({static_cast<float>(cornerRadius.BottomLeft), static_cast<float>(cornerRadius.BottomLeft)});
            rectangleClip.BottomRightRadius({static_cast<float>(cornerRadius.BottomRight), static_cast<float>(cornerRadius.BottomRight)});
            popupPlacementVisual.Clip(rectangleClip);
        }

        if (m_overflowPopupBackdropLink && m_overflowPopupSystemBackdropRoot)
        {
            winrt::Visual popupPlacementVisual = m_overflowPopupBackdropLink.PlacementVisual();
            const auto& overflowPopupSystemBackdropRoot = m_overflowPopupSystemBackdropRoot.get();
            winrt::ElementCompositionPreview::SetElementChildVisual(overflowPopupSystemBackdropRoot, popupPlacementVisual);
        }
    }
    else
    {
        winrt::VisualStateManager::GoToState(*this, L"NoOuterOverflowContentRootShadow", useTransitions);
        winrt::VisualStateManager::GoToState(*this, L"Default", useTransitions);
        winrt::VisualStateManager::GoToState(*this, L"Collapsed", useTransitions);

        // Take the backdrop behind the overflow popup out of the tree. If the entire CommandBarFlyoutCommandBar is
        // closed and reopens, the overflow popup could be closed and we don't want the backdrop behind the overflow
        // popup to flicker.
        if (m_overflowPopupBackdropLink && m_overflowPopupSystemBackdropRoot)
        {
            const auto& overflowPopupSystemBackdropRoot = m_overflowPopupSystemBackdropRoot.get();
            winrt::ElementCompositionPreview::SetElementChildVisual(overflowPopupSystemBackdropRoot, nullptr);
        }
    }

    // Update the corner radius clip on the backdrop. We copy the corner radius from the elements in the template, which
    // depends on the visual state of the CommandBarFlyoutCommandBar.
    if (m_backdropLink && m_primaryItemsRoot)
    {
        winrt::Visual placementVisual = m_backdropLink.PlacementVisual();

        winrt::Grid primaryItemsRoot = m_primaryItemsRoot.get().try_as<winrt::Grid>();
        auto cornerRadius = primaryItemsRoot.CornerRadius();

        // Copy the rounded corner clip from the templated element with the rounded corner clip set on it
        auto compositor = placementVisual.Compositor();
        winrt::RectangleClip rectangleClip = compositor.CreateRectangleClip();
        rectangleClip.Right(static_cast<float>(primaryItemsRoot.ActualWidth()));
        rectangleClip.Bottom(static_cast<float>(primaryItemsRoot.ActualHeight()));
        rectangleClip.TopLeftRadius({static_cast<float>(cornerRadius.TopLeft), static_cast<float>(cornerRadius.TopLeft)});
        rectangleClip.TopRightRadius({static_cast<float>(cornerRadius.TopRight), static_cast<float>(cornerRadius.TopRight)});
        rectangleClip.BottomLeftRadius({static_cast<float>(cornerRadius.BottomLeft), static_cast<float>(cornerRadius.BottomLeft)});
        rectangleClip.BottomRightRadius({static_cast<float>(cornerRadius.BottomRight), static_cast<float>(cornerRadius.BottomRight)});
        placementVisual.Clip(rectangleClip);
    }
}

void CommandBarFlyoutCommandBar::UpdateTemplateSettings()
{
    COMMANDBARFLYOUT_TRACE_INFO(*this, TRACE_MSG_METH_INT, METH_NAME, this, IsOpen());

    if (m_primaryItemsRoot && m_secondaryItemsRoot)
    {
        const auto flyoutTemplateSettings = winrt::get_self<CommandBarFlyoutCommandBarTemplateSettings>(FlyoutTemplateSettings());
        const auto maxWidth = static_cast<float>(MaxWidth());

#ifdef DBG
        COMMANDBARFLYOUT_TRACE_VERBOSE(*this, TRACE_MSG_METH_STR_DBL, METH_NAME, this, L"old ExpandedWidth:", flyoutTemplateSettings->ExpandedWidth());
        COMMANDBARFLYOUT_TRACE_VERBOSE(*this, TRACE_MSG_METH_STR_DBL, METH_NAME, this, L"old CurrentWidth:", flyoutTemplateSettings->CurrentWidth());
        COMMANDBARFLYOUT_TRACE_VERBOSE(*this, TRACE_MSG_METH_STR_DBL, METH_NAME, this, L"old WidthExpansionDelta:", flyoutTemplateSettings->WidthExpansionDelta());
        COMMANDBARFLYOUT_TRACE_VERBOSE(*this, TRACE_MSG_METH_STR_DBL, METH_NAME, this, L"old WidthExpansionAnimationStartPosition:", flyoutTemplateSettings->WidthExpansionAnimationStartPosition());
        COMMANDBARFLYOUT_TRACE_VERBOSE(*this, TRACE_MSG_METH_STR_DBL, METH_NAME, this, L"old WidthExpansionAnimationEndPosition:", flyoutTemplateSettings->WidthExpansionAnimationEndPosition());
        COMMANDBARFLYOUT_TRACE_VERBOSE(*this, TRACE_MSG_METH_STR_DBL, METH_NAME, this, L"old OpenAnimationStartPosition:", flyoutTemplateSettings->OpenAnimationStartPosition());
        COMMANDBARFLYOUT_TRACE_VERBOSE(*this, TRACE_MSG_METH_STR_DBL, METH_NAME, this, L"old OpenAnimationEndPosition:", flyoutTemplateSettings->OpenAnimationEndPosition());
        COMMANDBARFLYOUT_TRACE_VERBOSE(*this, TRACE_MSG_METH_STR_DBL, METH_NAME, this, L"old CloseAnimationEndPosition:", flyoutTemplateSettings->CloseAnimationEndPosition());
        COMMANDBARFLYOUT_TRACE_VERBOSE(*this, TRACE_MSG_METH_STR_FLT, METH_NAME, this, L"MaxWidth:", maxWidth);
#endif

        const winrt::Size infiniteSize = { std::numeric_limits<float>::infinity(), std::numeric_limits<float>::infinity() };
        m_primaryItemsRoot.get().Measure(infiniteSize);
        const winrt::Size primaryItemsRootDesiredSize = m_primaryItemsRoot.get().DesiredSize();
        float collapsedWidth = std::min(maxWidth, primaryItemsRootDesiredSize.Width);

        if (m_secondaryItemsRoot)
        {
            m_secondaryItemsRoot.get().Measure(infiniteSize);
            const auto overflowPopupSize = m_secondaryItemsRoot.get().DesiredSize();

            flyoutTemplateSettings->ExpandedWidth(std::min(maxWidth, std::max(collapsedWidth, overflowPopupSize.Width)));
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

        const double expandedWidth = flyoutTemplateSettings->ExpandedWidth();

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

#ifdef DBG
        COMMANDBARFLYOUT_TRACE_VERBOSE(*this, TRACE_MSG_METH_STR_FLT, METH_NAME, this, L"collapsedWidth:", collapsedWidth);
        COMMANDBARFLYOUT_TRACE_VERBOSE(*this, TRACE_MSG_METH_STR_DBL, METH_NAME, this, L"new ExpandedWidth:", expandedWidth);
        COMMANDBARFLYOUT_TRACE_VERBOSE(*this, TRACE_MSG_METH_STR_DBL, METH_NAME, this, L"new CurrentWidth:", flyoutTemplateSettings->CurrentWidth());
        COMMANDBARFLYOUT_TRACE_VERBOSE(*this, TRACE_MSG_METH_STR_DBL, METH_NAME, this, L"new WidthExpansionDelta:", flyoutTemplateSettings->WidthExpansionDelta());
        COMMANDBARFLYOUT_TRACE_VERBOSE(*this, TRACE_MSG_METH_STR_DBL, METH_NAME, this, L"new WidthExpansionAnimationStartPosition:", flyoutTemplateSettings->WidthExpansionAnimationStartPosition());
        COMMANDBARFLYOUT_TRACE_VERBOSE(*this, TRACE_MSG_METH_STR_DBL, METH_NAME, this, L"new WidthExpansionAnimationEndPosition:", flyoutTemplateSettings->WidthExpansionAnimationEndPosition());
#endif

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

#ifdef DBG
        COMMANDBARFLYOUT_TRACE_VERBOSE(*this, TRACE_MSG_METH_STR_DBL, METH_NAME, this, L"new OpenAnimationStartPosition:", flyoutTemplateSettings->OpenAnimationStartPosition());
        COMMANDBARFLYOUT_TRACE_VERBOSE(*this, TRACE_MSG_METH_STR_DBL, METH_NAME, this, L"new OpenAnimationEndPosition:", flyoutTemplateSettings->OpenAnimationEndPosition());
        COMMANDBARFLYOUT_TRACE_VERBOSE(*this, TRACE_MSG_METH_STR_DBL, METH_NAME, this, L"new CloseAnimationEndPosition:", flyoutTemplateSettings->CloseAnimationEndPosition());
#endif

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

void CommandBarFlyoutCommandBar::EnsureAutomationSetCountAndPosition()
{
    COMMANDBARFLYOUT_TRACE_VERBOSE(*this, TRACE_MSG_METH, METH_NAME, this);

    auto moreButton = m_moreButton.get();
    int sizeOfSet = 0;

    for (const auto& command : PrimaryCommands())
    {
        if (auto commandAsUIElement = command.try_as<winrt::UIElement>())
        {
            // Don't count AppBarSeparator if IsTabStop is false
            if (auto separator = commandAsUIElement.try_as<winrt::AppBarSeparator>())
            {
                if (!separator.IsTabStop())
                {
                    continue;
                }
            }
            else if (commandAsUIElement.Visibility() == winrt::Visibility::Visible)
            {
                sizeOfSet++;
            }
        }
    }

    if (moreButton && moreButton.Visibility() == winrt::Visibility::Visible)
    {
        // Accounting for the MoreButton
        sizeOfSet++;
    }

    for (const auto& command : PrimaryCommands())
    {
        if (auto commandAsUIElement = command.try_as<winrt::UIElement>())
        {
            // Don't count AppBarSeparator if IsTabStop is false
            if (auto separator = commandAsUIElement.try_as<winrt::AppBarSeparator>())
            {
                if (!separator.IsTabStop())
                {
                    continue;
                }
            }
            else if (commandAsUIElement.Visibility() == winrt::Visibility::Visible)
            {
                winrt::AutomationProperties::SetSizeOfSet(commandAsUIElement, sizeOfSet);
            }
        }
    }

    if (moreButton && moreButton.Visibility() == winrt::Visibility::Visible)
    {
        winrt::AutomationProperties::SetSizeOfSet(moreButton, sizeOfSet);
        winrt::AutomationProperties::SetPositionInSet(moreButton, sizeOfSet);
    }
}

void CommandBarFlyoutCommandBar::EnsureLocalizedControlTypes()
{
    COMMANDBARFLYOUT_TRACE_VERBOSE(*this, TRACE_MSG_METH, METH_NAME, this);

    for (auto const& command : PrimaryCommands())
    {
        SetKnownCommandLocalizedControlTypes(command);
    }

    for (auto const& command : SecondaryCommands())
    {
        SetKnownCommandLocalizedControlTypes(command);
    }
}

void CommandBarFlyoutCommandBar::CacheLocalizedStringResources()
{
    m_localizedCommandBarFlyoutAppBarButtonControlType = ResourceAccessor::GetLocalizedStringResource(SR_CommandBarFlyoutAppBarButtonLocalizedControlType);
    m_localizedCommandBarFlyoutAppBarToggleButtonControlType = ResourceAccessor::GetLocalizedStringResource(SR_CommandBarFlyoutAppBarToggleButtonLocalizedControlType);
    m_areLocalizedStringResourcesCached = true;
}

void CommandBarFlyoutCommandBar::ClearLocalizedStringResourceCache()
{
    m_areLocalizedStringResourcesCached = false;
    m_localizedCommandBarFlyoutAppBarButtonControlType.clear();
    m_localizedCommandBarFlyoutAppBarToggleButtonControlType.clear();
}

void CommandBarFlyoutCommandBar::SetKnownCommandLocalizedControlTypes(winrt::ICommandBarElement const& command)
{
    COMMANDBARFLYOUT_TRACE_VERBOSE(*this, TRACE_MSG_METH, METH_NAME, this);

    if (auto const& appBarButton = command.try_as<winrt::AppBarButton>())
    {
        if (m_areLocalizedStringResourcesCached)
        {
            MUX_ASSERT(!m_localizedCommandBarFlyoutAppBarButtonControlType.empty());
            winrt::AutomationProperties::SetLocalizedControlType(appBarButton, m_localizedCommandBarFlyoutAppBarButtonControlType);
        }
        else
        {
            winrt::AutomationProperties::SetLocalizedControlType(appBarButton, ResourceAccessor::GetLocalizedStringResource(SR_CommandBarFlyoutAppBarButtonLocalizedControlType));
        }
    }
    else if (auto const& appBarToggleButton = command.try_as<winrt::AppBarToggleButton>())
    {
        if (m_areLocalizedStringResourcesCached)
        {
            MUX_ASSERT(!m_localizedCommandBarFlyoutAppBarToggleButtonControlType.empty());
            winrt::AutomationProperties::SetLocalizedControlType(appBarToggleButton, m_localizedCommandBarFlyoutAppBarToggleButtonControlType);
        }
        else
        {
            winrt::AutomationProperties::SetLocalizedControlType(appBarToggleButton, ResourceAccessor::GetLocalizedStringResource(SR_CommandBarFlyoutAppBarToggleButtonLocalizedControlType));
        }
    }
}

void CommandBarFlyoutCommandBar::PopulateAccessibleControls()
{
    COMMANDBARFLYOUT_TRACE_VERBOSE(*this, TRACE_MSG_METH, METH_NAME, this);

    // The primary commands and the more button are the only controls accessible
    // using left and right arrow keys. All of the commands are accessible using
    // the up and down arrow keys.
    if (!m_horizontallyAccessibleControls)
    {
        MUX_ASSERT(!m_verticallyAccessibleControls);

        m_horizontallyAccessibleControls = winrt::make<Vector<winrt::Control>>();
        m_verticallyAccessibleControls = winrt::make<Vector<winrt::Control>>();
    }
    else
    {
        MUX_ASSERT(m_verticallyAccessibleControls);

        m_horizontallyAccessibleControls.Clear();
        m_verticallyAccessibleControls.Clear();
    }

    const auto primaryCommands = PrimaryCommands();

    for (winrt::ICommandBarElement const& command : primaryCommands)
    {
        if (auto const& commandAsControl = command.try_as<winrt::Control>())
        {
            m_horizontallyAccessibleControls.Append(commandAsControl);
            m_verticallyAccessibleControls.Append(commandAsControl);
        }
    }

    if (auto const& moreButton = m_moreButton.get())
    {
        if (primaryCommands.Size() > 0)
        {
            m_horizontallyAccessibleControls.Append(moreButton);
            m_verticallyAccessibleControls.Append(moreButton);
        }
    }

    for (winrt::ICommandBarElement const& command : SecondaryCommands())
    {
        if (auto const& commandAsControl = command.try_as<winrt::Control>())
        {
            m_verticallyAccessibleControls.Append(commandAsControl);
        }
    }
}

void CommandBarFlyoutCommandBar::OnKeyDown(
    winrt::KeyRoutedEventArgs const& args)
{
    COMMANDBARFLYOUT_TRACE_INFO(*this, TRACE_MSG_METH_STR, METH_NAME, this,
        TypeLogging::KeyRoutedEventArgsToString(args).c_str());

    if (args.Handled())
    {
        return;
    }

    switch (args.Key())
    {
    case winrt::VirtualKey::Tab:
    {
        if (SecondaryCommands().Size() > 0 && !IsOpen())
        {
            // Ensure the secondary commands flyout is open ...
            IsOpen(true);

            // ... and focus the first focusable command
            if (WinAppSdk::Containment::IsChangeEnabled<WINAPPSDK_CHANGEID_48006563>())
            {
                FocusCommandAsync(
                    SecondaryCommands() /*commands*/,
                    nullptr /*moreButton*/,
                    winrt::FocusState::Keyboard /*focusState*/,
                    true /*firstCommand*/,
                    true /*ensureTabStopUniqueness*/);
            }
            else
            {
                FocusCommandSync(
                    SecondaryCommands() /*commands*/,
                    nullptr /*moreButton*/,
                    winrt::FocusState::Keyboard /*focusState*/,
                    true /*firstCommand*/,
                    true /*ensureTabStopUniqueness*/);
            }
        }
        break;
    }

    case winrt::VirtualKey::Escape:
    {
        if (auto owningFlyout = m_owningFlyout.get())
        {
            owningFlyout.Hide();
            args.Handled(true);
        }
        break;
    }

    case winrt::VirtualKey::Right:
    case winrt::VirtualKey::Left:
    case winrt::VirtualKey::Down:
    case winrt::VirtualKey::Up:
    {
        const bool isRightToLeft = m_primaryItemsRoot && m_primaryItemsRoot.get().FlowDirection() == winrt::FlowDirection::RightToLeft;
        const bool isLeft = (args.Key() == winrt::VirtualKey::Left && !isRightToLeft) || (args.Key() == winrt::VirtualKey::Right && isRightToLeft);
        const bool isRight = (args.Key() == winrt::VirtualKey::Right && !isRightToLeft) || (args.Key() == winrt::VirtualKey::Left && isRightToLeft);
        const bool isDown = args.Key() == winrt::VirtualKey::Down;
        const bool isUp = args.Key() == winrt::VirtualKey::Up;

        // To avoid code duplication, we'll use the key directionality to determine
        // both which control list to use and in which direction to iterate through
        // it to find the next control to focus.  Then we'll do that iteration
        // to focus the next control.
        auto const& accessibleControls{ isUp || isDown ? m_verticallyAccessibleControls : m_horizontallyAccessibleControls };
        int const startIndex = isLeft || isUp ? accessibleControls.Size() - 1 : 0;
        int const endIndex = isLeft || isUp ? -1 : accessibleControls.Size();
        int const deltaIndex = isLeft || isUp ? -1 : 1;
        bool const shouldLoop = isUp || isDown;
        winrt::Control focusedControl{ nullptr };
        int focusedControlIndex = -1;

        for (int i = startIndex;
            // We'll stop looping at the end index unless we're looping,
            // in which case we want to wrap back around to the start index.
            (i != endIndex || shouldLoop) ||
            // If we found a focused control but have looped all the way back around,
            // then there wasn't another control to focus, so we should quit.
            (focusedControlIndex > 0 && i == focusedControlIndex);
            i += deltaIndex)
        {
            // If we've reached the end index, that means we want to loop.
            // We'll wrap around to the start index.
            if (i == endIndex)
            {
                MUX_ASSERT(shouldLoop);

                if (focusedControl)
                {
                    i = startIndex;
                }
                else
                {
                    // If no focused control was found after going through the entire list of controls,
                    // then we have nowhere for focus to go.  Let's early-out in that case.
                    break;
                }
            }

            auto const& control = accessibleControls.GetAt(i);

            // If we've yet to find the focused control, we'll keep looking for it.
            // Otherwise, we'll try to focus the next control after it.
            if (!focusedControl)
            {
                if (control.FocusState() != winrt::FocusState::Unfocused)
                {
                    focusedControl = control;
                    focusedControlIndex = i;
                }
            }
            else if (IsControlFocusable(control, false /*checkTabStop*/))
            {
                // If the control we're trying to focus is in the secondary command list,
                // then we'll make sure that that list is open before trying to focus the control.
                if (auto const& controlAsCommandBarElement = control.try_as<winrt::ICommandBarElement>())
                {
                    uint32_t index = 0;
                    if (SecondaryCommands().IndexOf(controlAsCommandBarElement, index) && !IsOpen())
                    {
                        IsOpen(true);
                    }
                }

                if (WinAppSdk::Containment::IsChangeEnabled<WINAPPSDK_CHANGEID_48006563>())
                {
                    FocusControlAsync(
                        accessibleControls.GetAt(i) /*newFocus*/,
                        focusedControl /*oldFocus*/,
                        winrt::FocusState::Keyboard /*focusState*/,
                        true /*updateTabStop*/);

                    args.Handled(true);
                    break;
                }
                else
                {
                    if (FocusControlSync(
                        accessibleControls.GetAt(i) /*newFocus*/,
                        focusedControl /*oldFocus*/,
                        winrt::FocusState::Keyboard /*focusState*/,
                        true /*updateTabStop*/))
                    {
                        args.Handled(true);
                        break;
                    }
                }
            }
        }

        if (!args.Handled())
        {
            // Occurs for example with Right key while MoreButton has focus. Stay on that MoreButton.
            args.Handled(true);
        }
        break;
    }
    }

    __super::OnKeyDown(args);
}

bool CommandBarFlyoutCommandBar::IsControlFocusable(
    winrt::Control const& control,
    bool checkTabStop)
{
    return control &&
        control.Visibility() == winrt::Visibility::Visible &&
        (control.IsEnabled() || control.AllowFocusWhenDisabled()) &&
        (control.IsTabStop() || (!checkTabStop && !control.try_as<winrt::AppBarSeparator>())); // AppBarSeparator is not focusable if IsTabStop is false
}

winrt::Control CommandBarFlyoutCommandBar::GetFirstTabStopControl(
    winrt::IObservableVector<winrt::ICommandBarElement> const& commands)
{
    for (const auto& command : commands)
    {
        if (auto commandAsControl = command.try_as<winrt::Control>())
        {
            if (commandAsControl.IsTabStop())
            {
                return commandAsControl;
            }
        }
    }
    return nullptr;
}

winrt::IAsyncOperation<bool> CommandBarFlyoutCommandBar::FocusControlAsync(
    winrt::Control newFocus,
    winrt::Control oldFocus,
    winrt::FocusState focusState,
    bool updateTabStop)
{
    MUX_ASSERT(newFocus);

    if (updateTabStop)
    {
        newFocus.IsTabStop(true);
    }

    // Setting focus can cause us to enter the window message handler loop, which is bad if
    // CXcpDispatcher::OnReentrancyProtectedWindowMessage is on the callstack, since that can lead to reentry.
    // Switching to a background thread and then back to the UI thread ensures that this call to Control.Focus()
    // occurs outside that callstack.
    winrt::apartment_context uiThread;
    co_await winrt::resume_background();
    co_await uiThread;

    if (newFocus.Focus(focusState))
    {
        if (oldFocus && updateTabStop)
        {
            oldFocus.IsTabStop(false);
        }
        co_return true;
    }
    co_return false;
}

winrt::IAsyncOperation<bool> CommandBarFlyoutCommandBar::FocusCommandAsync(
    winrt::IObservableVector<winrt::ICommandBarElement> commands,
    winrt::Control moreButton,
    winrt::FocusState focusState,
    bool firstCommand,
    bool ensureTabStopUniqueness)
{
    COMMANDBARFLYOUT_TRACE_VERBOSE(nullptr, TRACE_MSG_METH, METH_NAME, nullptr);

    MUX_ASSERT(commands);

    // Give focus to the first or last focusable command
    winrt::Control focusedControl = nullptr;
    int startIndex = 0;
    int endIndex = static_cast<int>(commands.Size());
    int deltaIndex = 1;

    if (!firstCommand)
    {
        deltaIndex = -1;
        startIndex = endIndex - 1;
        endIndex = -1;
    }

    for (int index = startIndex; index != endIndex; index += deltaIndex)
    {
        auto command = commands.GetAt(index);

        if (auto commandAsControl = command.try_as<winrt::Control>())
        {
            if (IsControlFocusable(commandAsControl, !ensureTabStopUniqueness /*checkTabStop*/))
            {
                if (!focusedControl)
                {
                    if (co_await FocusControlAsync(
                        commandAsControl /*newFocus*/,
                        nullptr /*oldFocus*/,
                        focusState /*focusState*/,
                        ensureTabStopUniqueness /*updateTabStop*/))
                    {
                        if (ensureTabStopUniqueness && moreButton && moreButton.IsTabStop())
                        {
                            moreButton.IsTabStop(false);
                        }

                        focusedControl = commandAsControl;

                        if (!ensureTabStopUniqueness)
                        {
                            break;
                        }
                    }
                }
                else if (focusedControl && commandAsControl.IsTabStop())
                {
                    commandAsControl.IsTabStop(false);
                }
            }
        }
    }

    co_return focusedControl != nullptr;
}

bool CommandBarFlyoutCommandBar::FocusControlSync(
    winrt::Control const& newFocus,
    winrt::Control const& oldFocus,
    winrt::FocusState const& focusState,
    bool updateTabStop)
{
    MUX_ASSERT(newFocus);

    if (updateTabStop)
    {
        newFocus.IsTabStop(true);
    }

    if (newFocus.Focus(focusState))
    {
        if (oldFocus && updateTabStop)
        {
            oldFocus.IsTabStop(false);
        }
        return true;
    }
    return false;
}

bool CommandBarFlyoutCommandBar::FocusCommandSync(
    winrt::IObservableVector<winrt::ICommandBarElement> const& commands,
    winrt::Control const& moreButton,
    winrt::FocusState const& focusState,
    bool firstCommand,
    bool ensureTabStopUniqueness)
{
    COMMANDBARFLYOUT_TRACE_VERBOSE(nullptr, TRACE_MSG_METH, METH_NAME, nullptr);

    MUX_ASSERT(commands);

    // Give focus to the first or last focusable command
    winrt::Control focusedControl = nullptr;
    int startIndex = 0;
    int endIndex = static_cast<int>(commands.Size());
    int deltaIndex = 1;

    if (!firstCommand)
    {
        deltaIndex = -1;
        startIndex = endIndex - 1;
        endIndex = -1;
    }

    for (int index = startIndex; index != endIndex; index += deltaIndex)
    {
        auto command = commands.GetAt(index);

        if (auto commandAsControl = command.try_as<winrt::Control>())
        {
            if (IsControlFocusable(commandAsControl, !ensureTabStopUniqueness /*checkTabStop*/))
            {
                if (!focusedControl)
                {
                    if (FocusControlSync(
                            commandAsControl /*newFocus*/,
                            nullptr /*oldFocus*/,
                            focusState /*focusState*/,
                            ensureTabStopUniqueness /*updateTabStop*/))
                    {
                        if (ensureTabStopUniqueness && moreButton && moreButton.IsTabStop())
                        {
                            moreButton.IsTabStop(false);
                        }

                        focusedControl = commandAsControl;

                        if (!ensureTabStopUniqueness)
                        {
                            break;
                        }
                    }
                }
                else if (focusedControl && commandAsControl.IsTabStop())
                {
                    commandAsControl.IsTabStop(false);
                }
            }
        }
    }

    return focusedControl != nullptr;
}

void CommandBarFlyoutCommandBar::EnsureTabStopUniqueness(
    winrt::IObservableVector<winrt::ICommandBarElement> const& commands,
    winrt::Control const& moreButton)
{
    COMMANDBARFLYOUT_TRACE_VERBOSE(nullptr, TRACE_MSG_METH, METH_NAME, nullptr);

    MUX_ASSERT(commands);

    bool tabStopSeen = moreButton && moreButton.IsTabStop();

    if (tabStopSeen || GetFirstTabStopControl(commands))
    {
        // Make sure only one command or the MoreButton has IsTabStop set
        for (const auto& command : commands)
        {
            if (auto commandAsControl = command.try_as<winrt::Control>())
            {
                if (IsControlFocusable(commandAsControl, false /*checkTabStop*/) && commandAsControl.IsTabStop())
                {
                    if (!tabStopSeen)
                    {
                        tabStopSeen = true;
                    }
                    else
                    {
                        commandAsControl.IsTabStop(false);
                    }
                }
            }
        }
    }
    else
    {
        // Set IsTabStop to first focusable command
        for (const auto& command : commands)
        {
            if (auto commandAsControl = command.try_as<winrt::Control>())
            {
                if (IsControlFocusable(commandAsControl, false /*checkTabStop*/))
                {
                    commandAsControl.IsTabStop(true);
                    break;
                }
            }
        }
    }
}

void CommandBarFlyoutCommandBar::ClearShadow()
{
    winrt::VisualStateManager::GoToState(*this, L"NoOuterOverflowContentRootShadow", true/*useTransitions*/);
}

void CommandBarFlyoutCommandBar::SetPresenter(winrt::FlyoutPresenter const& presenter)
{
    m_flyoutPresenter = winrt::make_weak(presenter);
}

void CommandBarFlyoutCommandBar::SetPresenterName(winrt::FlyoutPresenter const& presenter)
{
    // Since DropShadows don't play well with clip entrance animations for the presenter,
    // we'll need to fade it in. This name helps us locate the element to set the fade in
    // flag in the OS code.
    if (presenter)
    {
        if (OpenAnimationKind() == CommandBarFlyoutOpenCloseAnimationKind::Clip)
        {
            presenter.Name(L"DropShadowFadeInTarget");
        }
        else
        {
            presenter.Name(L"");
        }
    }
}

bool CommandBarFlyoutCommandBar::HasSecondaryOpenCloseAnimations()
{
    return SharedHelpers::IsAnimationsEnabled() &&
           static_cast<bool>(m_expandedDownToCollapsedStoryboardRevoker ||
                             m_expandedUpToCollapsedStoryboardRevoker ||
                             m_collapsedToExpandedUpStoryboardRevoker ||
                             m_collapsedToExpandedDownStoryboardRevoker);
}

void CommandBarFlyoutCommandBar::AttachEventsToSecondaryStoryboards()
{
    winrt::IControlProtected thisAsControlProtected = *this;

    const auto addDropShadowFunc = [this](auto const&, auto const&)
    {
        if (SharedHelpers::IsAnimationsEnabled())
        {
            if (const auto owningFlyout = m_owningFlyout.get())
            {
                if (const auto actualFlyout = winrt::get_self<CommandBarFlyout>(owningFlyout))
                {
                    actualFlyout->AddDropShadow();
                }
            }
        }
    };

    if (const auto expandedDownToCollapsed = GetTemplateChildT<winrt::Storyboard>(L"ExpandedDownToCollapsed", thisAsControlProtected))
    {
        m_expandedDownToCollapsedStoryboardRevoker = expandedDownToCollapsed.Completed(winrt::auto_revoke, addDropShadowFunc);
    }

    if (const auto expandedUpToCollapsed = GetTemplateChildT<winrt::Storyboard>(L"ExpandedUpToCollapsed", thisAsControlProtected))
    {
        m_expandedUpToCollapsedStoryboardRevoker = expandedUpToCollapsed.Completed(winrt::auto_revoke, addDropShadowFunc);
    }

    if (const auto collapsedToExpandedUp = GetTemplateChildT<winrt::Storyboard>(L"CollapsedToExpandedUp", thisAsControlProtected))
    {
        m_collapsedToExpandedUpStoryboardRevoker = collapsedToExpandedUp.Completed(winrt::auto_revoke, addDropShadowFunc);
    }

    if (const auto collapsedToExpandedDown = GetTemplateChildT<winrt::Storyboard>(L"CollapsedToExpandedDown", thisAsControlProtected))
    {
        m_collapsedToExpandedDownStoryboardRevoker = collapsedToExpandedDown.Completed(winrt::auto_revoke, addDropShadowFunc);
    }
}

void CommandBarFlyoutCommandBar::BindOwningFlyoutPresenterToCornerRadius()
{
    if (const auto owningFlyout = m_owningFlyout.get())
    {
        if (const auto actualFlyout = winrt::get_self<CommandBarFlyout>(owningFlyout))
        {
            winrt::IControlProtected thisAsControlProtected = *this;
            if (const auto root = GetTemplateChildT<winrt::Grid>(L"LayoutRoot", thisAsControlProtected))
            {
                winrt::Binding binding;
                binding.Source(root);
                binding.Path(winrt::PropertyPath(L"CornerRadius"));
                binding.Mode(winrt::BindingMode::OneWay);
                if (auto&& presenter = actualFlyout->GetPresenter().get())
                {
                    presenter.SetBinding(winrt::Control::CornerRadiusProperty(), binding);
                }
            }
        }
    }
}

// Invoked by CommandBarFlyout when a secondary AppBarButton or AppBarToggleButton dependency property changed.
void CommandBarFlyoutCommandBar::OnCommandBarElementDependencyPropertyChanged()
{
    COMMANDBARFLYOUT_TRACE_VERBOSE(*this, TRACE_MSG_METH, METH_NAME, this);

    // Only refresh the UI when the CommandBarFlyoutCommandBar is already open since it will be refreshed anyways in the event it gets opened.
    if (IsOpen())
    {
        UpdateUI(!m_commandBarFlyoutIsOpening, true /*isForSizeChange*/);
    }
}

void CommandBarFlyoutCommandBar::OnPropertyChanged(const winrt::DependencyPropertyChangedEventArgs& args)
{
    const winrt::IDependencyProperty& property = args.Property();

    if (property == s_SystemBackdropProperty)
    {
        if (args.NewValue() != args.OldValue())
        {
            const auto& oldSystemBackdrop = args.OldValue().try_as<winrt::SystemBackdrop>();
            const auto& newSystemBackdrop = args.NewValue().try_as<winrt::SystemBackdrop>();

            if (oldSystemBackdrop)
            {
                oldSystemBackdrop.OnTargetDisconnected(m_backdropLink);
                oldSystemBackdrop.OnTargetDisconnected(m_overflowPopupBackdropLink);
            }

            if (WinAppSdk::Containment::IsChangeEnabled<WINAPPSDK_CHANGEID_47050253>())
            {
                m_systemBackdrop = newSystemBackdrop;
            }

            if (newSystemBackdrop)
            {
                if (!m_backdropLink)
                {
                    auto visual = winrt::ElementCompositionPreview::GetElementVisual(*this);
                    auto compositor = visual.Compositor();
                    m_backdropLink = winrt::ContentExternalBackdropLink::Create(compositor);
                    m_overflowPopupBackdropLink = winrt::ContentExternalBackdropLink::Create(compositor);
                }

                newSystemBackdrop.OnTargetConnected(m_backdropLink, XamlRoot());
                newSystemBackdrop.OnTargetConnected(m_overflowPopupBackdropLink, XamlRoot());
            }
            else
            {
                m_backdropLink = nullptr;
                m_overflowPopupBackdropLink = nullptr;
            }
        }
    }
}
