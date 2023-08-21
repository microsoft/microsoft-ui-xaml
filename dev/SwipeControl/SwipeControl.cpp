﻿// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#include "pch.h"
#include "common.h"
#include "SwipeControl.h"
#include "SwipeControlInteractionTrackerOwner.h"
#include "SwipeItems.h"
#include "Vector.h"
#include "SwipeItem.h"
#include "RuntimeProfiler.h"
#include "SwipeTestHooks.h"

// Change to 'true' to turn on debugging outputs in Output window
bool SwipeControlTrace::s_IsDebugOutputEnabled{ false };
bool SwipeControlTrace::s_IsVerboseDebugOutputEnabled{ false };

static const double c_epsilon = 0.0001;
static const float c_ThresholdValue = 100.0;
static const float c_MinimumCloseVelocity = 31.0;

static thread_local winrt::weak_ref<SwipeControl> s_lastInteractedWithSwipeControl = nullptr;

SwipeControl::SwipeControl()
{
    __RP_Marker_ClassById(RuntimeProfiler::ProfId_SwipeControl);
    SetDefaultStyleKey(this);
}

SwipeControl::~SwipeControl()
{
    DetachEventHandlers(true /*useSafeGet*/);

    if (auto lastInteractedWithSwipeControl = s_lastInteractedWithSwipeControl.get())
    {
        if (lastInteractedWithSwipeControl.get() == this)
        {
            s_lastInteractedWithSwipeControl = nullptr;
            if (auto globalTestHooks = SwipeTestHooks::GetGlobalTestHooks())
            {
                globalTestHooks->NotifyLastInteractedWithSwipeControlChanged();
            }
        }
    }
}

#pragma region ISwipeControl

void SwipeControl::Close()
{
    CheckThread();

    if (m_isOpen && !m_lastActionWasClosing && !m_isInteracting)
    {
        m_lastActionWasClosing = true;
        if (!m_isIdle)
        {
            winrt::float3 initialPosition{ 0.0f };
            switch (m_createdContent)
            {
            case CreatedContent::Left:
                initialPosition.x = static_cast<float>(-m_swipeContentStackPanel.get().ActualWidth());
                break;
            case CreatedContent::Top:
                initialPosition.y = static_cast<float>(-m_swipeContentStackPanel.get().ActualHeight());
                break;
            case CreatedContent::Right:
                initialPosition.x = static_cast<float>(m_swipeContentStackPanel.get().ActualWidth());
                break;
            case CreatedContent::Bottom:
                initialPosition.y = static_cast<float>(m_swipeContentStackPanel.get().ActualHeight());
                break;
            case CreatedContent::None:
                break;
            default:
                assert(false);
            }
            m_interactionTracker.get().TryUpdatePosition(initialPosition);
        }

        winrt::float3 addedVelocity{ 0.0f };
        switch (m_createdContent)
        {
        case CreatedContent::Left:
            addedVelocity.x = c_MinimumCloseVelocity;
            break;
        case CreatedContent::Top:
            addedVelocity.y = c_MinimumCloseVelocity;
            break;
        case CreatedContent::Right:
            addedVelocity.x = -c_MinimumCloseVelocity;
            break;
        case CreatedContent::Bottom:
            addedVelocity.y = -c_MinimumCloseVelocity;
            break;
        case CreatedContent::None:
            break;
        default:
            assert(false);
        }
        m_interactionTracker.get().TryUpdatePositionWithAdditionalVelocity(addedVelocity);
    }
}
#pragma endregion

#pragma region IFrameworkElementOverrides
void SwipeControl::OnApplyTemplate()
{
    ThrowIfHasVerticalAndHorizontalContent(/*setIsHorizontal*/ true);

    DetachEventHandlers(false /*useSafeGet*/);
    GetTemplateParts();
    EnsureClip();
    AttachEventHandlers();
}

void SwipeControl::OnPropertyChanged(const winrt::DependencyPropertyChangedEventArgs& args)
{
    winrt::IDependencyProperty property = args.Property();
    if (property == s_LeftItemsProperty)
    {
        OnLeftItemsCollectionChanged(args);
    }
    if (property == s_RightItemsProperty)
    {
        OnRightItemsCollectionChanged(args);
    }
    if (property == s_TopItemsProperty)
    {
        OnTopItemsCollectionChanged(args);
    }
    if (property == s_BottomItemsProperty)
    {
        OnBottomItemsCollectionChanged(args);
    }
}

//Swipe control is usually placed in a list view item. When this is the case the swipe item needs to be the same size as the list view item.
//This is to ensure that swiping from anywhere on the list view item causes pointer pressed events in the SwipeControl.  Without this measure
//override it is usually not the case that swipe control will fill the available space.  This is because list view item is a content control
//and those by convension only provide it's children space for at most their desired size. However list view item itself will take up a different
//ammount of space. In the past we solved this issue by requiring the list view item to have the HorizontalContentAlignment and VerticalContentAlignment
//set to stretch. This property changes the measure cycle to give as much space as possible to the list view items children.  Instead we can 
//just do this ourselves in this measure override and prevent the confusing need for properties set on the parent of swipe control to use it at all.
winrt::Size SwipeControl::MeasureOverride(winrt::Size const& availableSize)
{
    m_rootGrid.get().Measure(availableSize);
    winrt::Size contentDesiredSize = m_rootGrid.get().DesiredSize();
    if (availableSize.Width != std::numeric_limits<float>::infinity())
    {
        contentDesiredSize.Width = availableSize.Width;
    }
    if (availableSize.Height != std::numeric_limits<float>::infinity())
    {
        contentDesiredSize.Height = availableSize.Height;
    }
    return contentDesiredSize;
}
#pragma endregion

void SwipeControl::CustomAnimationStateEntered(
    winrt::InteractionTrackerCustomAnimationStateEnteredArgs const& /*args*/)
{
    SWIPECONTROL_TRACE_INFO(*this, TRACE_MSG_METH, METH_NAME, this);

    m_isInteracting = true;

    if (m_isIdle)
    {
        m_isIdle = false;
        if (auto globalTestHooks = SwipeTestHooks::GetGlobalTestHooks())
        {
            globalTestHooks->NotifyIdleStatusChanged(*this);
        }
    }
}

void SwipeControl::RequestIgnored(
    winrt::InteractionTrackerRequestIgnoredArgs const& /*args*/)
{
    SWIPECONTROL_TRACE_INFO(*this, TRACE_MSG_METH, METH_NAME, this);
}

void SwipeControl::IdleStateEntered(
    winrt::InteractionTrackerIdleStateEnteredArgs const& /*args*/)
{
    SWIPECONTROL_TRACE_INFO(*this, TRACE_MSG_METH, METH_NAME, this);

    m_isInteracting = false;
    UpdateIsOpen(m_interactionTracker.get().Position() != winrt::float3::zero());

    if (m_isOpen)
    {
        if (m_currentItems && m_currentItems.get().Mode() == winrt::SwipeMode::Execute && m_currentItems.get().Size() > 0)
        {
            auto swipeItem = static_cast<winrt::SwipeItem>(m_currentItems.get().GetAt(0));
            winrt::get_self<SwipeItem>(swipeItem)->InvokeSwipe(*this);
        }
    }
    else
    {
        if (auto swipeContentStackPanel = m_swipeContentStackPanel.get())
        {
            swipeContentStackPanel.Background(nullptr);
            if (auto swipeContentStackPanelChildren = swipeContentStackPanel.Children())
            {
                swipeContentStackPanelChildren.Clear();
            }
        }
        if (auto swipeContentRoot = m_swipeContentRoot.get())
        {
            swipeContentRoot.Background(nullptr);
        }

        m_currentItems.set(nullptr);
        m_createdContent = CreatedContent::None;
    }

    if (!m_isIdle)
    {
        m_isIdle = true;
        if (auto globalTestHooks = SwipeTestHooks::GetGlobalTestHooks())
        {
            globalTestHooks->NotifyIdleStatusChanged(*this);
        }
    }
}

void SwipeControl::InteractingStateEntered(
    winrt::InteractionTrackerInteractingStateEnteredArgs const& /*args*/)
{
    SWIPECONTROL_TRACE_INFO(*this, TRACE_MSG_METH, METH_NAME, this);

    if (m_isIdle)
    {
        m_isIdle = false;
        if (auto globalTestHooks = SwipeTestHooks::GetGlobalTestHooks())
        {
            globalTestHooks->NotifyIdleStatusChanged(*this);
        }
    }

    m_lastActionWasClosing = false;
    m_lastActionWasOpening = false;
    m_isInteracting = true;

    //Once the user has started interacting with a SwipeControl in the closed state we are free to unblock contents.
    //Contents of items opposite the currently opened ones will not be created.
    if (!m_isOpen)
    {
        m_blockNearContent = false;
        m_blockFarContent = false;
        m_interactionTracker.get().Properties().InsertBoolean(s_blockNearContentPropertyName, false);
        m_interactionTracker.get().Properties().InsertBoolean(s_blockFarContentPropertyName, false);
    }
}

void SwipeControl::InertiaStateEntered(
    winrt::InteractionTrackerInertiaStateEnteredArgs const& args)
{
    SWIPECONTROL_TRACE_INFO(*this, TRACE_MSG_METH, METH_NAME, this);

    m_isInteracting = false;

    if (m_isIdle)
    {
        m_isIdle = false;
        if (auto globalTestHooks = SwipeTestHooks::GetGlobalTestHooks())
        {
            globalTestHooks->NotifyIdleStatusChanged(*this);
        }
    }

    //It is possible that the user has flicked from a negative position to a position that would result in the interaction
    //tracker coming to rest at the positive open position (or vise versa). The != zero check does not account for this.
    //Instead we check to ensure that the current position and the ModifiedRestingPosition have the same sign (multiply to a positive number)
    //If they do not then we are in this situation and want the end result of the interaction to be the closed state, so close without any animation and return
    //to prevent further processing of this inertia state.
    const auto flickToOppositeSideCheck = m_interactionTracker.get().Position() * args.ModifiedRestingPosition().Value();
    if (m_isHorizontal ? flickToOppositeSideCheck.x < 0 : flickToOppositeSideCheck.y < 0)
    {
        CloseWithoutAnimation();
        return;
    }

    UpdateIsOpen(args.ModifiedRestingPosition().Value() != winrt::float3::zero());
    // If the user has panned the interaction tracker past 0 in the opposite direction of the previously
    // opened swipe items then when we set m_isOpen to true the animations will snap to that value.
    // To avoid this we block that side of the animation until the interacting state is entered.
    if (m_isOpen)
    {
        switch (m_createdContent)
        {
        case CreatedContent::Bottom:
        case CreatedContent::Right:
            m_blockNearContent = true;
            m_blockFarContent = false;
            m_interactionTracker.get().Properties().InsertBoolean(s_blockNearContentPropertyName, true);
            m_interactionTracker.get().Properties().InsertBoolean(s_blockFarContentPropertyName, false);
            break;
        case CreatedContent::Top:
        case CreatedContent::Left:
            m_blockNearContent = false;
            m_blockFarContent = true;
            m_interactionTracker.get().Properties().InsertBoolean(s_blockNearContentPropertyName, false);
            m_interactionTracker.get().Properties().InsertBoolean(s_blockFarContentPropertyName, true);
            break;
        case CreatedContent::None:
            m_blockNearContent = false;
            m_blockFarContent = false;
            m_interactionTracker.get().Properties().InsertBoolean(s_blockNearContentPropertyName, false);
            m_interactionTracker.get().Properties().InsertBoolean(s_blockFarContentPropertyName, false);
            break;
        default:
            assert(false);
        }
    }
}

void SwipeControl::ValuesChanged(
    winrt::InteractionTrackerValuesChangedArgs const& args)
{
    SWIPECONTROL_TRACE_VERBOSE(*this, TRACE_MSG_METH, METH_NAME, this);

    auto lastInteractedWithSwipeControl = s_lastInteractedWithSwipeControl.get();
    if (m_isInteracting && (!lastInteractedWithSwipeControl || lastInteractedWithSwipeControl.get() != this))
    {
        if (lastInteractedWithSwipeControl)
        {
            lastInteractedWithSwipeControl->CloseIfNotRemainOpenExecuteItem();
        }
        s_lastInteractedWithSwipeControl = get_weak();

        if (auto globalTestHooks = SwipeTestHooks::GetGlobalTestHooks())
        {
            globalTestHooks->NotifyLastInteractedWithSwipeControlChanged();
        }
    }

    float value = 0.0f;

    if (m_isHorizontal)
    {
        value = args.Position().x;
        if (!m_blockNearContent && m_createdContent != CreatedContent::Left && value < -c_epsilon)
        {
            CreateLeftContent();
        }
        else if (!m_blockFarContent && m_createdContent != CreatedContent::Right && value > c_epsilon)
        {
            CreateRightContent();
        }
    }
    else
    {
        value = args.Position().y;
        if (!m_blockNearContent && m_createdContent != CreatedContent::Top && value < -c_epsilon)
        {
            CreateTopContent();
        }
        else if (!m_blockFarContent && m_createdContent != CreatedContent::Bottom && value > c_epsilon)
        {
            CreateBottomContent();
        }
    }
    UpdateThresholdReached(value);
}

#pragma region TestHookHelpers
winrt::SwipeControl SwipeControl::GetLastInteractedWithSwipeControl()
{
    if (auto lastInteractedWithSwipeControl = s_lastInteractedWithSwipeControl.get())
    {
        return *lastInteractedWithSwipeControl;
    }
    return nullptr;
}

bool SwipeControl::GetIsOpen()
{
    return m_isOpen;
}

bool SwipeControl::GetIsIdle()
{
    return m_isIdle;
}
#pragma endregion

void SwipeControl::OnLeftItemsCollectionChanged(const winrt::DependencyPropertyChangedEventArgs& args)
{
    SWIPECONTROL_TRACE_INFO(*this, TRACE_MSG_METH, METH_NAME, this);

    if (m_leftItemsChangedToken.value != 0)
    {
        auto observableVector = args.OldValue().try_as<winrt::IObservableVector<winrt::SwipeItem>>();
        observableVector.VectorChanged(m_leftItemsChangedToken);
        m_leftItemsChangedToken.value = 0;
    }

    if (args.NewValue())
    {
        ThrowIfHasVerticalAndHorizontalContent();
        auto observableVector = args.NewValue().try_as<winrt::IObservableVector<winrt::SwipeItem>>();
        m_leftItemsChangedToken = observableVector.VectorChanged({ this, &SwipeControl::OnLeftItemsChanged });
    }

    if (m_interactionTracker)
    {
        m_interactionTracker.get().Properties().InsertBoolean(s_hasLeftContentPropertyName, args.NewValue() && args.NewValue().try_as<winrt::IVector<winrt::SwipeItem>>().Size() > 0);
    }

    if (m_createdContent == CreatedContent::Left)
    {
        CreateLeftContent();
    }
}

void SwipeControl::OnRightItemsCollectionChanged(const winrt::DependencyPropertyChangedEventArgs& args)
{
    SWIPECONTROL_TRACE_INFO(*this, TRACE_MSG_METH, METH_NAME, this);

    if (m_rightItemsChangedToken.value != 0)
    {
        auto observableVector = args.OldValue().try_as<winrt::IObservableVector<winrt::SwipeItem>>();
        observableVector.VectorChanged(m_rightItemsChangedToken);
        m_rightItemsChangedToken.value = 0;
    }

    if (args.NewValue())
    {
        ThrowIfHasVerticalAndHorizontalContent();
        auto observableVector = args.NewValue().try_as<winrt::IObservableVector<winrt::SwipeItem>>();
        m_rightItemsChangedToken = observableVector.VectorChanged({ this, &SwipeControl::OnRightItemsChanged });
    }

    if (m_interactionTracker)
    {
        m_interactionTracker.get().Properties().InsertBoolean(s_hasRightContentPropertyName, args.NewValue() && args.NewValue().try_as<winrt::IVector<winrt::SwipeItem>>().Size() > 0);
    }

    if (m_createdContent == CreatedContent::Right)
    {
        CreateRightContent();
    }
}

void SwipeControl::OnTopItemsCollectionChanged(const winrt::DependencyPropertyChangedEventArgs& args)
{
    SWIPECONTROL_TRACE_INFO(*this, TRACE_MSG_METH, METH_NAME, this);

    if (m_topItemsChangedToken.value != 0)
    {
        auto observableVector = args.OldValue().try_as<winrt::IObservableVector<winrt::SwipeItem>>();
        observableVector.VectorChanged(m_topItemsChangedToken);
        m_topItemsChangedToken.value = 0;
    }

    if (args.NewValue())
    {
        ThrowIfHasVerticalAndHorizontalContent();
        auto observableVector = args.NewValue().try_as<winrt::IObservableVector<winrt::SwipeItem>>();
        m_topItemsChangedToken = observableVector.VectorChanged({ this, &SwipeControl::OnTopItemsChanged });
    }

    if (m_interactionTracker)
    {
        m_interactionTracker.get().Properties().InsertBoolean(s_hasTopContentPropertyName, args.NewValue() && args.NewValue().try_as<winrt::IVector<winrt::SwipeItem>>().Size() > 0);
    }

    if (m_createdContent == CreatedContent::Top)
    {
        CreateTopContent();
    }
}

void SwipeControl::OnBottomItemsCollectionChanged(const winrt::DependencyPropertyChangedEventArgs& args)
{
    SWIPECONTROL_TRACE_INFO(*this, TRACE_MSG_METH, METH_NAME, this);

    if (m_bottomItemsChangedToken.value != 0)
    {
        auto observableVector = args.OldValue().try_as<winrt::IObservableVector<winrt::SwipeItem>>();
        observableVector.VectorChanged(m_bottomItemsChangedToken);
        m_bottomItemsChangedToken.value = 0;
    }

    if (args.NewValue())
    {
        ThrowIfHasVerticalAndHorizontalContent();
        auto observableVector = args.NewValue().try_as<winrt::IObservableVector<winrt::SwipeItem>>();
        m_bottomItemsChangedToken = observableVector.VectorChanged({ this, &SwipeControl::OnBottomItemsChanged });
    }

    if (m_interactionTracker)
    {
        m_interactionTracker.get().Properties().InsertBoolean(s_hasBottomContentPropertyName, args.NewValue() && args.NewValue().try_as<winrt::IVector<winrt::SwipeItem>>().Size() > 0);
    }

    if (m_createdContent == CreatedContent::Bottom)
    {
        CreateBottomContent();
    }
}

void SwipeControl::OnLoaded(const winrt::IInspectable& /*sender*/, const winrt::RoutedEventArgs& /*args*/)
{
    SWIPECONTROL_TRACE_INFO(*this, TRACE_MSG_METH, METH_NAME, this);

    if (!m_hasInitialLoadedEventFired)
    {
        m_hasInitialLoadedEventFired = true;
        InitializeInteractionTracker();
        TryGetSwipeVisuals();
    }
    //If the swipe control has been added to the tree for a subsequent time, for instance when a list view item has been recycled,
    //Ensure that we are in the closed interaction tracker state.
    CloseWithoutAnimation();
}

void SwipeControl::AttachEventHandlers()
{
    SWIPECONTROL_TRACE_INFO(*this, TRACE_MSG_METH, METH_NAME, this);

    MUX_ASSERT(m_loadedToken.value == 0);
    m_loadedToken = Loaded({ this, &SwipeControl::OnLoaded });
    m_hasInitialLoadedEventFired = false;

    MUX_ASSERT(m_onSizeChangedToken.value == 0);
    m_onSizeChangedToken = SizeChanged({ this, &SwipeControl::OnSizeChanged });

    MUX_ASSERT(m_onSwipeContentStackPanelSizeChangedToken.value == 0);
    m_onSwipeContentStackPanelSizeChangedToken = m_swipeContentStackPanel.get().SizeChanged({ this, &SwipeControl::OnSwipeContentStackPanelSizeChanged });

    // also get any action from any inside button, or a clickable/tappable control
    if (!m_onPointerPressedEventHandler)
    {
        m_onPointerPressedEventHandler.set(winrt::box_value<winrt::PointerEventHandler>({ this, &SwipeControl::OnPointerPressedEvent }));
        AddHandler(winrt::UIElement::PointerPressedEvent(), m_onPointerPressedEventHandler.get(), true);
    }

    MUX_ASSERT(m_inputEaterTappedToken.value == 0);
    m_inputEaterTappedToken = m_inputEater.get().Tapped({ this, &SwipeControl::InputEaterGridTapped });
}

void SwipeControl::DetachEventHandlers(bool useSafeGet)
{
    SWIPECONTROL_TRACE_INFO(nullptr, TRACE_MSG_METH, METH_NAME, this);

    if (m_loadedToken.value != 0)
    {
        Loaded(m_loadedToken);
        m_loadedToken.value = 0;
    }

    if (m_onSizeChangedToken.value != 0)
    {
        SizeChanged(m_onSizeChangedToken);
        m_onSizeChangedToken.value = 0;
    }

    if (m_onSwipeContentStackPanelSizeChangedToken.value != 0)
    {
        if (auto swipeContentStackPanel = useSafeGet ? m_swipeContentStackPanel.safe_get() : m_swipeContentStackPanel.get())
        {
            swipeContentStackPanel.SizeChanged(m_onSwipeContentStackPanelSizeChangedToken);
        }
        m_onSwipeContentStackPanelSizeChangedToken.value = 0;
    }

    if (m_onPointerPressedEventHandler)
    {
        RemoveHandler(winrt::UIElement::PointerPressedEvent(), m_onPointerPressedEventHandler.get());
        m_onPointerPressedEventHandler.set(nullptr);
    }

    if (m_inputEaterTappedToken.value != 0)
    {
        if (auto inputEater = useSafeGet ? m_inputEater.safe_get() : m_inputEater.get())
        {
            inputEater.Tapped(m_inputEaterTappedToken);
        }
        m_inputEaterTappedToken.value = 0;
    }

    DetachDismissingHandlers();
}

void SwipeControl::OnSizeChanged(const winrt::IInspectable& /*sender*/, const winrt::SizeChangedEventArgs& /*args*/)
{
    EnsureClip();
    for (winrt::UIElement uiElement : m_swipeContentStackPanel.get().Children())
    {
        winrt::AppBarButton appBarButton = uiElement.try_as<winrt::AppBarButton>();
        if (appBarButton)
        {
            if (m_isHorizontal)
            {
                appBarButton.Height(ActualHeight());
                if (m_currentItems && m_currentItems.get().Mode() == winrt::SwipeMode::Execute)
                {
                    appBarButton.Width(ActualWidth());
                }
            }
            else
            {
                appBarButton.Width(ActualWidth());
                if (m_currentItems && m_currentItems.get().Mode() == winrt::SwipeMode::Execute)
                {
                    appBarButton.Height(ActualHeight());
                }
            }
        }
    }
}

void SwipeControl::OnSwipeContentStackPanelSizeChanged(const winrt::IInspectable& sender, const winrt::SizeChangedEventArgs& args)
{
    if (m_interactionTracker)
    {
        m_interactionTracker.get().MinPosition({ -args.NewSize().Width, -args.NewSize().Height, 0.0f });
        m_interactionTracker.get().MaxPosition({ args.NewSize().Width, args.NewSize().Height, 0.0f });
        ConfigurePositionInertiaRestingValues();
    }
}

void SwipeControl::OnPointerPressedEvent(
    const winrt::IInspectable& sender,
    const winrt::PointerRoutedEventArgs& args)
{
    SWIPECONTROL_TRACE_INFO(*this, TRACE_MSG_METH, METH_NAME, this);

    if (args.Pointer().PointerDeviceType() == winrt::Devices::Input::PointerDeviceType::Touch && m_visualInteractionSource)
    {
        if (m_currentItems &&
            m_currentItems.get().Mode() == winrt::SwipeMode::Execute &&
            m_currentItems.get().Size() > 0 &&
            m_currentItems.get().GetAt(0).BehaviorOnInvoked() == winrt::SwipeBehaviorOnInvoked::RemainOpen &&
            m_isOpen)
        {
            //If the swipe control is currently open on an Execute item's who's behaviorOnInvoked property is set to RemainOpen
            //we don't want to allow the user interaction to effect the swipe control anymore, so don't redirect the manipulation
            //to the interaction tracker.
            return;
        }
        try
        {
            m_visualInteractionSource.get().TryRedirectForManipulation(args.GetCurrentPoint(*this));
        }
        catch (const winrt::hresult_error& e)
        {
            // Swallowing Access Denied error because of InteractionTracker bug 17434718 which has been
            // causing crashes at least in RS3, RS4 and RS5.
            if (e.to_abi() != E_ACCESSDENIED)
            {
                throw;
            }
        }
    }
}

void SwipeControl::InputEaterGridTapped(const winrt::IInspectable& /*sender*/, const winrt::TappedRoutedEventArgs& args)
{
    SWIPECONTROL_TRACE_INFO(*this, TRACE_MSG_METH, METH_NAME, this);

    if (m_isOpen)
    {
        CloseIfNotRemainOpenExecuteItem();
        args.Handled(true);
    }
}

void SwipeControl::AttachDismissingHandlers()
{
    SWIPECONTROL_TRACE_INFO(*this, TRACE_MSG_METH, METH_NAME, this);

    DetachDismissingHandlers();

    if (winrt::IUIElement10 uiElement10 = *this)
    {
        if (const auto xamlRoot = uiElement10.XamlRoot())
        {
            if (const auto xamlRootContent = xamlRoot.Content())
            {
                m_xamlRootPointerPressedEventRevoker = AddRoutedEventHandler<RoutedEventType::PointerPressed>(
                    xamlRootContent,
                    [this](auto const&, auto const& args)
                    {
                        DismissSwipeOnAnExternalTap(args.GetCurrentPoint(nullptr).Position());
                    },
                    true /*handledEventsToo*/);

                m_xamlRootKeyDownEventRevoker = AddRoutedEventHandler<RoutedEventType::KeyDown>(
                    xamlRootContent,
                    [this](auto const&, auto const& args)
                    {
                        CloseIfNotRemainOpenExecuteItem();
                    },
                    true /*handledEventsToo*/);
            }

            m_xamlRootChangedRevoker = RegisterXamlRootChanged(xamlRoot, { this, &SwipeControl::CurrentXamlRootChanged });
        }
    }
    else
    {
        if (auto currentWindow = winrt::Window::Current())
        {
            if (auto coreWindow = currentWindow.CoreWindow())
            {
                m_coreWindowPointerPressedRevoker = coreWindow.PointerPressed(winrt::auto_revoke, { this, &SwipeControl::DismissSwipeOnAnExternalCoreWindowTap });
                m_coreWindowKeyDownRevoker = coreWindow.KeyDown(winrt::auto_revoke, { this, &SwipeControl::DismissSwipeOnCoreWindowKeyDown });
                m_windowMinimizeRevoker = coreWindow.VisibilityChanged(winrt::auto_revoke, { this, &SwipeControl::CurrentWindowVisibilityChanged });
                m_windowSizeChangedRevoker = currentWindow.SizeChanged(winrt::auto_revoke, { this, &SwipeControl::CurrentWindowSizeChanged });
            }
        }
    }

    if (auto coreWindow = winrt::CoreWindow::GetForCurrentThread())
    {
        if (auto dispatcher = coreWindow.Dispatcher())
        {
            m_acceleratorKeyActivatedRevoker = dispatcher.AcceleratorKeyActivated(winrt::auto_revoke, { this, &SwipeControl::DismissSwipeOnAcceleratorKeyActivator });
        }
    }
}

void SwipeControl::DetachDismissingHandlers()
{
    SWIPECONTROL_TRACE_INFO(nullptr, TRACE_MSG_METH, METH_NAME, this);

    m_xamlRootPointerPressedEventRevoker.revoke();
    m_xamlRootKeyDownEventRevoker.revoke();
    m_xamlRootChangedRevoker.revoke();

    m_acceleratorKeyActivatedRevoker.revoke();
    m_coreWindowPointerPressedRevoker.revoke();
    m_coreWindowKeyDownRevoker.revoke();
    m_windowMinimizeRevoker.revoke();
    m_windowSizeChangedRevoker.revoke();
}

void SwipeControl::DismissSwipeOnAcceleratorKeyActivator(const winrt::Windows::UI::Core::CoreDispatcher& sender, const winrt::AcceleratorKeyEventArgs& args)
{
    CloseIfNotRemainOpenExecuteItem();
}

void SwipeControl::CurrentXamlRootChanged(const winrt::XamlRoot & /*sender*/, const winrt::XamlRootChangedEventArgs &/*args*/)
{
    CloseIfNotRemainOpenExecuteItem();
}

void SwipeControl::DismissSwipeOnCoreWindowKeyDown(const winrt::CoreWindow& sender, const winrt::KeyEventArgs& args)
{
    CloseIfNotRemainOpenExecuteItem();
}

void SwipeControl::CurrentWindowSizeChanged(const winrt::IInspectable & /*sender*/, const winrt::WindowSizeChangedEventArgs &/*args*/)
{
    CloseIfNotRemainOpenExecuteItem();
}

void SwipeControl::CurrentWindowVisibilityChanged(const winrt::CoreWindow & /*sender*/, const winrt::VisibilityChangedEventArgs /*args*/)
{
    CloseIfNotRemainOpenExecuteItem();
}

void SwipeControl::DismissSwipeOnAnExternalCoreWindowTap(const winrt::CoreWindow& sender, const winrt::PointerEventArgs& args)
{
    DismissSwipeOnAnExternalTap(args.CurrentPoint().RawPosition());
}

void SwipeControl::DismissSwipeOnAnExternalTap(winrt::Point const& tapPoint)
{
    SWIPECONTROL_TRACE_INFO(*this, TRACE_MSG_METH, METH_NAME, this);

    const winrt::GeneralTransform transform = TransformToVisual(nullptr);
    const winrt::Point p(0, 0);

    // start of the swipe control
    const auto transformedElementOrigin = transform.TransformPoint(p);

    // If point is not within the item's bounds, close it.
    if (*this && tapPoint.X < transformedElementOrigin.X || tapPoint.Y < transformedElementOrigin.Y ||
        (tapPoint.X - transformedElementOrigin.X) > ActualWidth() ||
        (tapPoint.Y - transformedElementOrigin.Y) > ActualHeight())
    {
        CloseIfNotRemainOpenExecuteItem();
    }
}

void SwipeControl::GetTemplateParts()
{
    winrt::IControlProtected thisAsControlProtected = *this;
    m_rootGrid.set(GetTemplateChildT<winrt::Grid>(s_rootGridName, thisAsControlProtected));
    m_inputEater.set(GetTemplateChildT<winrt::Grid>(s_inputEaterName, thisAsControlProtected));
    m_content.set(GetTemplateChildT<winrt::Grid>(s_ContentRootName, thisAsControlProtected));
    m_swipeContentRoot.set(GetTemplateChildT<winrt::Grid>(s_swipeContentRootName, thisAsControlProtected));
    m_swipeContentStackPanel.set(GetTemplateChildT<winrt::StackPanel>(s_swipeContentStackPanelName, thisAsControlProtected));

    //Before RS5 these elements were not in the template but were instead created in code behind when the swipe content was created.
    //Post RS5 the code behind expects these elements to always be in the tree.
    if (!m_swipeContentRoot)
    {
        winrt::Grid swipeContentRoot;
        swipeContentRoot.Name(L"SwipeContentRoot");
        m_swipeContentRoot.set(swipeContentRoot);
        m_rootGrid.get().Children().InsertAt(0, swipeContentRoot);
    }
    if (!m_swipeContentStackPanel)
    {
        winrt::StackPanel swipeContentStackPanel;
        swipeContentStackPanel.Name(L"SwipeContentStackPanel");
        m_swipeContentStackPanel.set(swipeContentStackPanel);
        m_swipeContentRoot.get().Children().Append(swipeContentStackPanel);
    }
    m_swipeContentStackPanel.get().Orientation(m_isHorizontal ? winrt::Orientation::Horizontal : winrt::Orientation::Vertical);

    if (auto lookedUpStyle = SharedHelpers::FindInApplicationResources(s_swipeItemStyleName))
    {
        m_swipeItemStyle.set(lookedUpStyle.try_as<winrt::UI::Xaml::Style>());
    }
}

void SwipeControl::InitializeInteractionTracker()
{
    SWIPECONTROL_TRACE_INFO(*this, TRACE_MSG_METH, METH_NAME, this);

    if (!m_interactionTrackerOwner)
    {
        m_interactionTrackerOwner = winrt::make_self<SwipeControlInteractionTrackerOwner>(*this).try_as<winrt::IInteractionTrackerOwner>();
    }

    if (!m_compositor)
    {
        m_compositor.set(winrt::ElementCompositionPreview::GetElementVisual(m_rootGrid.get()).Compositor());
    }

    m_visualInteractionSource.set(winrt::VisualInteractionSource::Create(FindVisualInteractionSourceVisual()));
    m_visualInteractionSource.get().IsPositionXRailsEnabled(m_isHorizontal);
    m_visualInteractionSource.get().IsPositionYRailsEnabled(!m_isHorizontal);
    m_visualInteractionSource.get().ManipulationRedirectionMode(winrt::VisualInteractionSourceRedirectionMode::CapableTouchpadOnly);
    m_visualInteractionSource.get().PositionXSourceMode(m_isHorizontal ? winrt::InteractionSourceMode::EnabledWithInertia : winrt::InteractionSourceMode::Disabled);
    m_visualInteractionSource.get().PositionYSourceMode(!m_isHorizontal ? winrt::InteractionSourceMode::EnabledWithInertia : winrt::InteractionSourceMode::Disabled);
    if (m_isHorizontal)
    {
        m_visualInteractionSource.get().PositionXChainingMode(winrt::InteractionChainingMode::Never);
    }
    else
    {
        m_visualInteractionSource.get().PositionYChainingMode(winrt::InteractionChainingMode::Never);
    }

    m_interactionTracker.set(winrt::InteractionTracker::CreateWithOwner(m_compositor.get(), m_interactionTrackerOwner));
    m_interactionTracker.get().InteractionSources().Add(m_visualInteractionSource.get());
    m_interactionTracker.get().Properties().InsertBoolean(s_isFarOpenPropertyName, false);
    m_interactionTracker.get().Properties().InsertBoolean(s_isNearOpenPropertyName, false);
    m_interactionTracker.get().Properties().InsertBoolean(s_blockNearContentPropertyName, false);
    m_interactionTracker.get().Properties().InsertBoolean(s_blockFarContentPropertyName, false);
    m_interactionTracker.get().Properties().InsertBoolean(s_hasLeftContentPropertyName, LeftItems() && LeftItems().Size() > 0);
    m_interactionTracker.get().Properties().InsertBoolean(s_hasRightContentPropertyName, RightItems() && RightItems().Size() > 0);
    m_interactionTracker.get().Properties().InsertBoolean(s_hasTopContentPropertyName, TopItems() && TopItems().Size() > 0);
    m_interactionTracker.get().Properties().InsertBoolean(s_hasBottomContentPropertyName, BottomItems() && BottomItems().Size() > 0);
    m_interactionTracker.get().MaxPosition({ std::numeric_limits<float>::infinity(), std::numeric_limits<float>::infinity(), 0.0f });
    m_interactionTracker.get().MinPosition({ -1.0f * std::numeric_limits<float>::infinity(), -1.0f * std::numeric_limits<float>::infinity(), 0.0f });

    // Create and initialize the Swipe animations:
    // If the swipe control is already opened it should not be possible to open the opposite side's items, without first closing the swipe control.
    // This prevents the user from flicking the swipe control closed and accidently opening the other due to inertia.
    // To acheive this we insert the isFarOpen and isNearOpen boolean properties on the interaction tracker and alter the expression output based on these.
    // The opened state is maintained in the interaction trackers IdleStateEntered handler, this means we need to ensure this state is entered each time the swipe control
    // is opened or closed.

    // A more readable version of the expression:

    /*m_swipeAnimation.set(m_compositor.get().CreateExpressionAnimation(L"isHorizontal ?"
        "Vector3(tracker.isFarOpen || tracker.blockNearContent ? Clamp(-tracker.Position.X, -this.Target.Size.X, 0) :"
                "tracker.isNearOpen  || tracker.blockFarContent ? Clamp(-tracker.Position.X,  0, this.Target.Size.X) :"
                "Clamp(-tracker.Position.X, (tracker.hasRightContent ? -10000 : 0), (tracker.hasLeftContent ? 10000 : 0)), 0, 0) :"
        "Vector3(0, tracker.isFarOpen  || tracker.blockNearContent ? Clamp(-tracker.Position.Y, -this.Target.Size.Y, 0) :"
                   "tracker.isNearOpen || tracker.blockFarContent ? Clamp(-tracker.Position.Y, 0,  this.Target.Size.Y) :"
                   "Clamp(-tracker.Position.Y, (tracker.hasBottomContent ? -10000 : 0), (tracker.hasTopContent ? 10000 : 0)), 0)"));*/

    m_swipeAnimation.set(m_compositor.get().CreateExpressionAnimation(isHorizontalPropertyName() + L" ?"
        "Vector3(" + trackerPropertyName() + L"." + isFarOpenPropertyName() + L" || " + trackerPropertyName() + L"." + blockNearContentPropertyName() + L" ? Clamp(-" + trackerPropertyName() + L".Position.X, -this.Target.Size.X, 0) :"
        + trackerPropertyName() + L"." + isNearOpenPropertyName() + L" || " + trackerPropertyName() + L"." + blockFarContentPropertyName() + L" ? Clamp(-" + trackerPropertyName() + L".Position.X,  0, this.Target.Size.X) :"
        "Clamp(-" + trackerPropertyName() + L".Position.X, (" + trackerPropertyName() + L"." + hasRightContentPropertyName() + L" ? -10000 : 0), (" + trackerPropertyName() + L"." + hasLeftContentPropertyName() + L" ? 10000 : 0)), 0, 0) :"
        "Vector3(0, " + trackerPropertyName() + L"." + isFarOpenPropertyName() + L" || " + trackerPropertyName() + L"." + blockNearContentPropertyName() + L"  ? Clamp(-" + trackerPropertyName() + L".Position.Y, -this.Target.Size.Y, 0) :"
        + trackerPropertyName() + L"." + isNearOpenPropertyName() + L" || " + trackerPropertyName() + L"." + blockFarContentPropertyName() + L" ? Clamp(-" + trackerPropertyName() + L".Position.Y, 0,  this.Target.Size.Y) :"
        "Clamp(-" + trackerPropertyName() + L".Position.Y, (" + trackerPropertyName() + L"." + hasBottomContentPropertyName() + L" ? -10000 : 0), (" + trackerPropertyName() + L"." + hasTopContentPropertyName() + L" ? 10000 : 0)), 0)"));

    m_swipeAnimation.get().SetReferenceParameter(s_trackerPropertyName, m_interactionTracker.get());
    m_swipeAnimation.get().SetBooleanParameter(s_isHorizontalPropertyName, m_isHorizontal);
    if (IsTranslationFacadeAvailableForSwipeControl(m_content.get()))
    {
        m_swipeAnimation.get().Target(s_translationPropertyName);
    }

    //A more readable version of the expression:

    /*m_executeExpressionAnimation.set(m_compositor.get().CreateExpressionAnimation(L"(foregroundVisual." + GetAnimationTarget() + L" * 0.5) + (isHorizontal ?"
        "Vector3((isNearContent ? -0.5, 0.5) * this.Target.Size.X, 0, 0) : "
        "Vector3(0, (isNearContent ? -0.5, 0.5) * this.Target.Size.Y, 0))"));*/

    m_executeExpressionAnimation.set(m_compositor.get().CreateExpressionAnimation(L"(" + foregroundVisualPropertyName() + L"." + GetAnimationTarget(m_swipeContentStackPanel.get()) + L" * 0.5) + (" + isHorizontalPropertyName() + L" ? "
        "Vector3((" + isNearContentPropertyName() + L" ? -0.5 : 0.5) * this.Target.Size.X, 0, 0) : "
        "Vector3(0, (" + isNearContentPropertyName() + L" ? -0.5 : 0.5) * this.Target.Size.Y, 0))"));

    m_executeExpressionAnimation.get().SetBooleanParameter(s_isHorizontalPropertyName, m_isHorizontal);
    if (IsTranslationFacadeAvailableForSwipeControl(m_swipeContentStackPanel.get()))
    {
        m_executeExpressionAnimation.get().Target(s_translationPropertyName);
    }

    //A more readable version of the expression:

    /*m_clipExpressionAnimation.set(m_compositor.get().CreateExpressionAnimation(L"isHorizontal ?
        Max(swipeRootVisual.Size.X + (isNearContent ? tracker.Position.X : -tracker.Position.X) , 0) :
        Max(swipeRootVisual.Size.Y + (isNearContent ? tracker.Position.Y : -tracker.Position.Y) , 0)"));*/

    m_clipExpressionAnimation.set(m_compositor.get().CreateExpressionAnimation(isHorizontalPropertyName() + L" ? "
        "Max(" + swipeRootVisualPropertyName() + L".Size.X + (" + isNearContentPropertyName() + L" ? " + trackerPropertyName() + L".Position.X : -" + trackerPropertyName() + L".Position.X) , 0) : "
        "Max(" + swipeRootVisualPropertyName() + L".Size.Y + (" + isNearContentPropertyName() + L" ? " + trackerPropertyName() + L".Position.Y : -" + trackerPropertyName() + L".Position.Y) , 0)"));

    m_clipExpressionAnimation.get().SetReferenceParameter(s_trackerPropertyName, m_interactionTracker.get());
    m_clipExpressionAnimation.get().SetBooleanParameter(s_isHorizontalPropertyName, m_isHorizontal);
}

void SwipeControl::ConfigurePositionInertiaRestingValues()
{
    SWIPECONTROL_TRACE_INFO(*this, TRACE_MSG_METH, METH_NAME, this);

    if (m_isHorizontal)
    {
        winrt::IVector<winrt::InteractionTrackerInertiaModifier> xModifiers = winrt::make<Vector<winrt::InteractionTrackerInertiaModifier>>();

        winrt::ExpressionAnimation leftCondition = m_compositor.get().CreateExpressionAnimation(L"this.Target." + hasLeftContentPropertyName() + L" && !this.Target." + isFarOpenPropertyName() + L" && this.Target.NaturalRestingPosition.x <= -1 * (this.Target." + isNearOpenPropertyName() + L" ? " + swipeContentSizeParameterName() + L" : min(" + swipeContentSizeParameterName() + L", " + maxThresholdPropertyName() + L"))");
        leftCondition.SetScalarParameter(s_swipeContentSizeParameterName, static_cast<float>(m_swipeContentStackPanel.get().ActualWidth()));
        leftCondition.SetScalarParameter(s_maxThresholdPropertyName, c_ThresholdValue);
        winrt::ExpressionAnimation leftRestingPoint = m_compositor.get().CreateExpressionAnimation(L"-" + swipeContentSizeParameterName());
        leftRestingPoint.SetScalarParameter(s_swipeContentSizeParameterName, static_cast<float>(m_swipeContentStackPanel.get().ActualWidth()));
        winrt::InteractionTrackerInertiaRestingValue leftOpen = winrt::InteractionTrackerInertiaRestingValue::Create(m_compositor.get());
        leftOpen.Condition(leftCondition);
        leftOpen.RestingValue(leftRestingPoint);
        xModifiers.Append(leftOpen);

        winrt::ExpressionAnimation rightCondition = m_compositor.get().CreateExpressionAnimation(L"this.Target." + hasRightContentPropertyName() + L" && !this.Target." + isNearOpenPropertyName() + L" && this.Target.NaturalRestingPosition.x >= (this.Target." + isFarOpenPropertyName() + L" ? " + swipeContentSizeParameterName() + L" : min(" + swipeContentSizeParameterName() + L", " + maxThresholdPropertyName() + L"))");
        rightCondition.SetScalarParameter(s_swipeContentSizeParameterName, static_cast<float>(m_swipeContentStackPanel.get().ActualWidth()));
        rightCondition.SetScalarParameter(s_maxThresholdPropertyName, c_ThresholdValue);
        winrt::ExpressionAnimation rightRestingValue = m_compositor.get().CreateExpressionAnimation(s_swipeContentSizeParameterName);
        rightRestingValue.SetScalarParameter(s_swipeContentSizeParameterName, static_cast<float>(m_swipeContentStackPanel.get().ActualWidth()));
        winrt::InteractionTrackerInertiaRestingValue rightOpen = winrt::InteractionTrackerInertiaRestingValue::Create(m_compositor.get());
        rightOpen.Condition(rightCondition);
        rightOpen.RestingValue(rightRestingValue);
        xModifiers.Append(rightOpen);

        winrt::ExpressionAnimation condition = m_compositor.get().CreateExpressionAnimation(L"true");
        winrt::ExpressionAnimation restingValue = m_compositor.get().CreateExpressionAnimation(L"0");
        winrt::InteractionTrackerInertiaRestingValue neutralX = winrt::InteractionTrackerInertiaRestingValue::Create(m_compositor.get());
        neutralX.Condition(condition);
        neutralX.RestingValue(restingValue);
        xModifiers.Append(neutralX);

        m_interactionTracker.get().ConfigurePositionXInertiaModifiers(xModifiers);
    }
    else
    {
        winrt::IVector<winrt::InteractionTrackerInertiaModifier> yModifiers = winrt::make<Vector<winrt::InteractionTrackerInertiaModifier>>();

        winrt::ExpressionAnimation topCondition = m_compositor.get().CreateExpressionAnimation(L"this.Target." + hasTopContentPropertyName() + L" && !this.Target." + isFarOpenPropertyName() + L" && this.Target.NaturalRestingPosition.y <= -1 * (this.Target." + isNearOpenPropertyName() + L" ? " + swipeContentSizeParameterName() + L" : min(" + swipeContentSizeParameterName() + L", " + maxThresholdPropertyName() + L"))");
        topCondition.SetScalarParameter(s_swipeContentSizeParameterName, static_cast<float>(m_swipeContentStackPanel.get().ActualHeight()));
        topCondition.SetScalarParameter(s_maxThresholdPropertyName, c_ThresholdValue);
        winrt::ExpressionAnimation topRestingValue = m_compositor.get().CreateExpressionAnimation(L"-" + swipeContentSizeParameterName());
        topRestingValue.SetScalarParameter(s_swipeContentSizeParameterName, static_cast<float>(m_swipeContentStackPanel.get().ActualHeight()));
        winrt::InteractionTrackerInertiaRestingValue topOpen = winrt::InteractionTrackerInertiaRestingValue::Create(m_compositor.get());
        topOpen.Condition(topCondition);
        topOpen.RestingValue(topRestingValue);
        yModifiers.Append(topOpen);

        winrt::ExpressionAnimation bottomCondition = m_compositor.get().CreateExpressionAnimation(L"this.Target." + hasBottomContentPropertyName() + L" && !this.Target." + isNearOpenPropertyName() + L" && this.Target.NaturalRestingPosition.y >= (this.Target." + isFarOpenPropertyName() + L" ? " + swipeContentSizeParameterName() + L" : min(" + swipeContentSizeParameterName() + L", " + maxThresholdPropertyName() + L"))");
        bottomCondition.SetScalarParameter(s_swipeContentSizeParameterName, static_cast<float>(m_swipeContentStackPanel.get().ActualHeight()));
        bottomCondition.SetScalarParameter(s_maxThresholdPropertyName, c_ThresholdValue);
        winrt::ExpressionAnimation bottomRestingValue = m_compositor.get().CreateExpressionAnimation(s_swipeContentSizeParameterName);
        bottomRestingValue.SetScalarParameter(s_swipeContentSizeParameterName, static_cast<float>(m_swipeContentStackPanel.get().ActualHeight()));
        winrt::InteractionTrackerInertiaRestingValue bottomOpen = winrt::InteractionTrackerInertiaRestingValue::Create(m_compositor.get());
        bottomOpen.Condition(bottomCondition);
        bottomOpen.RestingValue(bottomRestingValue);
        yModifiers.Append(bottomOpen);

        winrt::ExpressionAnimation condition = m_compositor.get().CreateExpressionAnimation(L"true");
        winrt::ExpressionAnimation restingValue = m_compositor.get().CreateExpressionAnimation(L"0");
        winrt::InteractionTrackerInertiaRestingValue neutralY = winrt::InteractionTrackerInertiaRestingValue::Create(m_compositor.get());
        neutralY.Condition(condition);
        neutralY.RestingValue(restingValue);
        yModifiers.Append(neutralY);

        m_interactionTracker.get().ConfigurePositionYInertiaModifiers(yModifiers);
    }
}

winrt::Visual SwipeControl::FindVisualInteractionSourceVisual()
{
    winrt::Visual visualInteractionSource = nullptr;

    // Don't walk up the tree too far largely as an optimization for when SwipeControl isn't used
    // with a list.  The general-case when using swipe with a ListView will probably have the
    // LVIP as the visual parent of the SwipeControl but enabling checking for a few more
    // levels above that could enable more complex list item templates where SwipeControl
    // isn't the root element.
    const int maxSteps = 5;
    int steps = 0;
    auto current = winrt::VisualTreeHelper::GetParent(*this);
    while (current && steps < maxSteps)
    {
        if (auto lvip = current.try_as<winrt::ListViewItemPresenter>())
        {
            visualInteractionSource = winrt::ElementCompositionPreview::GetElementVisual(lvip);
            break;
        }

        current = winrt::VisualTreeHelper::GetParent(current);
        ++steps;
    }

    if (!visualInteractionSource)
    {
        visualInteractionSource = winrt::ElementCompositionPreview::GetElementVisual(*this);
    }

    return visualInteractionSource;
}

void SwipeControl::EnsureClip()
{
    const float width = static_cast<float>(ActualWidth());
    const float height = static_cast<float>(ActualHeight());
    const winrt::Rect rect = { 0.0f, 0.0f, width, height };
    winrt::Windows::UI::Xaml::Media::RectangleGeometry rectangleGeometry;
    rectangleGeometry.Rect(rect);
    Clip(rectangleGeometry);
}

void SwipeControl::CloseWithoutAnimation()
{
    SWIPECONTROL_TRACE_INFO(*this, TRACE_MSG_METH, METH_NAME, this);

    const bool wasIdle = m_isIdle;
    m_interactionTracker.get().TryUpdatePosition({ 0.0f, 0.0f, 0.0f });
    if (wasIdle)
    {
        IdleStateEntered(nullptr);
    }
}

void SwipeControl::CloseIfNotRemainOpenExecuteItem()
{
    SWIPECONTROL_TRACE_INFO(*this, TRACE_MSG_METH, METH_NAME, this);

    if (m_currentItems &&
        m_currentItems.get().Mode() == winrt::SwipeMode::Execute &&
        m_currentItems.get().Size() > 0 &&
        m_currentItems.get().GetAt(0).BehaviorOnInvoked() == winrt::SwipeBehaviorOnInvoked::RemainOpen &&
        m_isOpen)
    {
        //If we have a Mode set to Execute, and an item with BehaviorOnInvoked set to RemainOpen, we do not want to close, so no-op
        return;
    }
    Close();
}

void SwipeControl::CreateLeftContent()
{
    if (auto items = LeftItems())
    {
        m_createdContent = CreatedContent::Left;
        CreateContent(items);
    }
}

void SwipeControl::CreateRightContent()
{
    if (auto items = RightItems())
    {
        m_createdContent = CreatedContent::Right;
        CreateContent(items);
    }
}

void SwipeControl::CreateBottomContent()
{
    if (auto items = BottomItems())
    {
        m_createdContent = CreatedContent::Bottom;
        CreateContent(items);
    }
}

void SwipeControl::CreateTopContent()
{
    if (auto items = TopItems())
    {
        m_createdContent = CreatedContent::Top;
        CreateContent(items);
    }
}

void SwipeControl::CreateContent(const winrt::SwipeItems& items)
{
    if (m_swipeContentStackPanel && m_swipeContentStackPanel.get().Children())
    {
        m_swipeContentStackPanel.get().Children().Clear();
    }

    m_currentItems.set(items);
    if (m_currentItems)
    {
        AlignStackPanel();
        PopulateContentItems();
        SetupExecuteExpressionAnimation();
        SetupClipAnimation();
        UpdateColors();
    }
}

void SwipeControl::AlignStackPanel()
{
    SWIPECONTROL_TRACE_INFO(*this, TRACE_MSG_METH, METH_NAME, this);

    if (m_currentItems.get().Size() > 0)
    {
        switch (m_currentItems.get().Mode())
        {
        case winrt::SwipeMode::Execute:
        {
            if (m_isHorizontal)
            {
                m_swipeContentStackPanel.get().HorizontalAlignment(winrt::HorizontalAlignment::Stretch);
                m_swipeContentStackPanel.get().VerticalAlignment(winrt::VerticalAlignment::Center);
            }
            else
            {
                m_swipeContentStackPanel.get().HorizontalAlignment(winrt::HorizontalAlignment::Center);
                m_swipeContentStackPanel.get().VerticalAlignment(winrt::VerticalAlignment::Stretch);
            }
            break;
        }
        case winrt::SwipeMode::Reveal:
        {
            if (m_isHorizontal)
            {
                const auto swipeContentStackPanelHorizontalAlignment = m_createdContent == CreatedContent::Left ? winrt::HorizontalAlignment::Left :
                    m_createdContent == CreatedContent::Right ? winrt::HorizontalAlignment::Right :
                    winrt::HorizontalAlignment::Stretch;

                m_swipeContentStackPanel.get().HorizontalAlignment(swipeContentStackPanelHorizontalAlignment);
                m_swipeContentStackPanel.get().VerticalAlignment(winrt::VerticalAlignment::Center);
            }
            else
            {
                const auto swipeContentStackPanelVerticalAlignment = m_createdContent == CreatedContent::Top ? winrt::VerticalAlignment::Top :
                    m_createdContent == CreatedContent::Bottom ? winrt::VerticalAlignment::Bottom :
                    winrt::VerticalAlignment::Stretch;

                m_swipeContentStackPanel.get().HorizontalAlignment(winrt::HorizontalAlignment::Center);
                m_swipeContentStackPanel.get().VerticalAlignment(swipeContentStackPanelVerticalAlignment);
            }
            break;
        }
        default:
            assert(false);
            break;
        }
    }
}

void SwipeControl::PopulateContentItems()
{
    SWIPECONTROL_TRACE_INFO(*this, TRACE_MSG_METH, METH_NAME, this);

    for (winrt::SwipeItem swipeItem : m_currentItems.get())
    {
        m_swipeContentStackPanel.get().Children().Append(GetSwipeItemButton(swipeItem));
    }

    TryGetSwipeVisuals();
}

void SwipeControl::SetupExecuteExpressionAnimation()
{
    SWIPECONTROL_TRACE_INFO(*this, TRACE_MSG_METH, METH_NAME, this);

    if (IsTranslationFacadeAvailableForSwipeControl(m_swipeContentStackPanel.get()))
    {
        m_swipeContentStackPanel.get().StopAnimation(m_executeExpressionAnimation.get());
        m_swipeContentStackPanel.get().Translation({ 0.0f, 0.0f, 0.0f });
    }
    else if (m_swipeContentVisual)
    {
        m_swipeContentVisual.get().StopAnimation(GetAnimationTarget(m_swipeContentStackPanel.get()));
        m_swipeContentVisual.get().Properties().InsertVector3(GetAnimationTarget(m_swipeContentStackPanel.get()), { 0.0f, 0.0f, 0.0f });
    }

    if (m_currentItems.get().Mode() == winrt::SwipeMode::Execute)
    {
        assert(m_createdContent != CreatedContent::None);
        m_executeExpressionAnimation.get().SetBooleanParameter(s_isNearContentPropertyName, m_createdContent == CreatedContent::Left || m_createdContent == CreatedContent::Top);
        if (IsTranslationFacadeAvailableForSwipeControl(m_swipeContentStackPanel.get()))
        {
            m_swipeContentStackPanel.get().StartAnimation(m_executeExpressionAnimation.get());
        }
        if (m_swipeContentVisual)
        {
            m_swipeContentVisual.get().StartAnimation(GetAnimationTarget(m_swipeContentStackPanel.get()), m_executeExpressionAnimation.get());
        }
    }
}

void SwipeControl::SetupClipAnimation()
{
    SWIPECONTROL_TRACE_INFO(*this, TRACE_MSG_METH, METH_NAME, this);

    if (!m_insetClip)
    {
        m_insetClip.set(m_compositor.get().CreateInsetClip());
        m_swipeContentRootVisual.get().Clip(m_insetClip.get());
    }
    else
    {
        m_insetClip.get().StopAnimation(s_leftInsetTargetName);
        m_insetClip.get().StopAnimation(s_rightInsetTargetName);
        m_insetClip.get().StopAnimation(s_topInsetTargetName);
        m_insetClip.get().StopAnimation(s_bottomInsetTargetName);
        m_insetClip.get().LeftInset(0.0f);
        m_insetClip.get().RightInset(0.0f);
        m_insetClip.get().TopInset(0.0f);
        m_insetClip.get().BottomInset(0.0f);
    }

    m_clipExpressionAnimation.get().SetBooleanParameter(s_isNearContentPropertyName, m_createdContent == CreatedContent::Left || m_createdContent == CreatedContent::Top);

    if (m_createdContent == CreatedContent::None)
    {
        //If we have no created content then we don't need to start the clip animation yet.
        return;
    }

    m_insetClip.get().StartAnimation(DirectionToInset(m_createdContent), m_clipExpressionAnimation.get());
}

void SwipeControl::UpdateColors()
{
    SWIPECONTROL_TRACE_INFO(*this, TRACE_MSG_METH, METH_NAME, this);

    if (m_currentItems.get().Mode() == winrt::SwipeMode::Execute)
    {
        UpdateColorsIfExecuteItem();
    }
    else
    {
        UpdateColorsIfRevealItems();
    }
}

winrt::AppBarButton SwipeControl::GetSwipeItemButton(const winrt::SwipeItem& swipeItem)
{
    winrt::AppBarButton itemAsButton;
    winrt::get_self<SwipeItem>(swipeItem)->GenerateControl(itemAsButton, m_swipeItemStyle.get());

    if (!swipeItem.Background())
    {
        if (auto lookedUpBrush = SharedHelpers::FindInApplicationResources(m_currentItems.get().Mode() == winrt::SwipeMode::Reveal ? s_swipeItemBackgroundResourceName : m_thresholdReached ? s_executeSwipeItemPostThresholdBackgroundResourceName : s_executeSwipeItemPreThresholdBackgroundResourceName))
        {
            itemAsButton.Background(lookedUpBrush.try_as<winrt::Brush>());
        }
    }

    if (!swipeItem.Foreground())
    {
        if (auto lookedUpBrush = SharedHelpers::FindInApplicationResources(m_currentItems.get().Mode() == winrt::SwipeMode::Reveal ? s_swipeItemForegroundResourceName : m_thresholdReached ? s_executeSwipeItemPostThresholdForegroundResourceName : s_executeSwipeItemPreThresholdForegroundResourceName))
        {
            itemAsButton.Foreground(lookedUpBrush.try_as<winrt::Brush>());
        }
    }

    if (m_isHorizontal)
    {
        itemAsButton.Height(ActualHeight());
        if (m_currentItems.get().Mode() == winrt::SwipeMode::Execute)
        {
            itemAsButton.Width(ActualWidth());
        }
    }
    else
    {
        itemAsButton.Width(ActualWidth());
        if (m_currentItems.get().Mode() == winrt::SwipeMode::Execute)
        {
            itemAsButton.Height(ActualHeight());
        }
    }
    return itemAsButton;
}

void SwipeControl::UpdateColorsIfExecuteItem()
{
    SWIPECONTROL_TRACE_INFO(*this, TRACE_MSG_METH, METH_NAME, this);

    if (!m_currentItems || m_currentItems.get().Mode() != winrt::SwipeMode::Execute)
    {
        return;
    }

    winrt::SwipeItem swipeItem = nullptr;
    if (m_currentItems.get().Size() > 0)
    {
        swipeItem = m_currentItems.get().GetAt(0);
    }
    UpdateExecuteBackgroundColor(swipeItem);
    UpdateExecuteForegroundColor(swipeItem);
}

void SwipeControl::UpdateExecuteBackgroundColor(const winrt::SwipeItem& swipeItem)
{
    SWIPECONTROL_TRACE_INFO(*this, TRACE_MSG_METH, METH_NAME, this);

    winrt::Brush background = nullptr;

    if (!m_thresholdReached)
    {
        if (auto lookedUpBackgroundBrush = SharedHelpers::FindInApplicationResources(s_executeSwipeItemPreThresholdBackgroundResourceName))
        {
            background = lookedUpBackgroundBrush.try_as<winrt::Brush>();
        }
    }
    else
    {
        if (auto lookedUpBackgroundBrush = SharedHelpers::FindInApplicationResources(s_executeSwipeItemPostThresholdBackgroundResourceName))
        {
            background = lookedUpBackgroundBrush.try_as<winrt::Brush>();
        }
    }

    if (swipeItem && swipeItem.Background())
    {
        background = swipeItem.Background();
    }

    m_swipeContentStackPanel.get().Background(background);
    m_swipeContentRoot.get().Background(nullptr);
}

void SwipeControl::UpdateExecuteForegroundColor(const winrt::SwipeItem& swipeItem)
{
    SWIPECONTROL_TRACE_INFO(*this, TRACE_MSG_METH, METH_NAME, this);

    if (m_swipeContentStackPanel.get().Children().Size() > 0)
    {
        if (auto appBarButton = m_swipeContentStackPanel.get().Children().GetAt(0).as<winrt::AppBarButton>())
        {
            winrt::Brush foreground = nullptr;

            if (!m_thresholdReached)
            {
                if (auto lookedUpForegroundBrush = SharedHelpers::FindInApplicationResources(s_executeSwipeItemPreThresholdForegroundResourceName))
                {
                    foreground = lookedUpForegroundBrush.try_as<winrt::Brush>();
                }
            }
            else
            {
                if (auto lookedUpForegroundBrush = SharedHelpers::FindInApplicationResources(s_executeSwipeItemPostThresholdForegroundResourceName))
                {
                    foreground = lookedUpForegroundBrush.try_as<winrt::Brush>();
                }
            }

            if (swipeItem && swipeItem.Foreground())
            {
                foreground = swipeItem.Foreground();
            }

            appBarButton.Foreground(foreground);
            appBarButton.Background(winrt::SolidColorBrush(winrt::Colors::Transparent()));
        }
    }
}

void SwipeControl::UpdateColorsIfRevealItems()
{
    SWIPECONTROL_TRACE_INFO(*this, TRACE_MSG_METH, METH_NAME, this);

    if (m_currentItems.get().Mode() != winrt::SwipeMode::Reveal)
    {
        return;
    }

    winrt::Brush rootGridBackground = nullptr;

    if (auto lookedUpBrush = SharedHelpers::FindInApplicationResources(s_swipeItemBackgroundResourceName))
    {
        rootGridBackground = lookedUpBrush.try_as<winrt::Brush>();
    }
    if (m_currentItems.get().Size() > 0)
    {
        switch (m_createdContent)
        {
        case CreatedContent::Left:
        case CreatedContent::Top:
        {
            auto itemBackground = m_currentItems.get().GetAt(m_swipeContentStackPanel.get().Children().Size() - 1).Background();
            if (itemBackground != nullptr)
            {
                rootGridBackground = itemBackground;
            }
            break;
        }
        case CreatedContent::Right:
        case CreatedContent::Bottom:
        {
            auto itemBackground = m_currentItems.get().GetAt(0).Background();
            if (itemBackground != nullptr)
            {
                rootGridBackground = itemBackground;
            }
            break;
        }
        case CreatedContent::None:
        {
            break;
        }
        default:
            assert(false);
            break;
        }
    }

    m_swipeContentRoot.get().Background(rootGridBackground);
    m_swipeContentStackPanel.get().Background(nullptr);
}

void SwipeControl::OnLeftItemsChanged(const winrt::IObservableVector<winrt::SwipeItem>& sender, const winrt::IVectorChangedEventArgs args)
{
    SWIPECONTROL_TRACE_INFO(*this, TRACE_MSG_METH, METH_NAME, this);

    ThrowIfHasVerticalAndHorizontalContent();
    if (m_interactionTracker)
    {
        m_interactionTracker.get().Properties().InsertBoolean(s_hasLeftContentPropertyName, sender.Size() > 0);
    }

    if (m_createdContent == CreatedContent::Left)
    {
        CreateLeftContent();
    }
}

void SwipeControl::OnRightItemsChanged(const winrt::IObservableVector<winrt::SwipeItem>& sender, const winrt::IVectorChangedEventArgs args)
{
    SWIPECONTROL_TRACE_INFO(*this, TRACE_MSG_METH, METH_NAME, this);

    ThrowIfHasVerticalAndHorizontalContent();

    if (m_interactionTracker)
    {
        m_interactionTracker.get().Properties().InsertBoolean(s_hasRightContentPropertyName, sender.Size() > 0);
    }

    if (m_createdContent == CreatedContent::Right)
    {
        CreateRightContent();
    }
}

void SwipeControl::OnTopItemsChanged(const winrt::IObservableVector<winrt::SwipeItem>& sender, const winrt::IVectorChangedEventArgs args)
{
    SWIPECONTROL_TRACE_INFO(*this, TRACE_MSG_METH, METH_NAME, this);

    ThrowIfHasVerticalAndHorizontalContent();
    if (m_interactionTracker)
    {
        m_interactionTracker.get().Properties().InsertBoolean(s_hasTopContentPropertyName, sender.Size() > 0);
    }

    if (m_createdContent == CreatedContent::Top)
    {
        CreateTopContent();
    }
}

void SwipeControl::OnBottomItemsChanged(const winrt::IObservableVector<winrt::SwipeItem>& sender, const winrt::IVectorChangedEventArgs args)
{
    SWIPECONTROL_TRACE_INFO(*this, TRACE_MSG_METH, METH_NAME, this);

    ThrowIfHasVerticalAndHorizontalContent();
    if (m_interactionTracker)
    {
        m_interactionTracker.get().Properties().InsertBoolean(s_hasBottomContentPropertyName, sender.Size() > 0);
    }

    if (m_createdContent == CreatedContent::Bottom)
    {
        CreateBottomContent();
    }
}

void SwipeControl::TryGetSwipeVisuals()
{
    SWIPECONTROL_TRACE_INFO(*this, TRACE_MSG_METH, METH_NAME, this);

    if (IsTranslationFacadeAvailableForSwipeControl(m_content.get()))
    {
        m_swipeAnimation.get().Target(GetAnimationTarget(m_content.get()));
        m_content.get().StartAnimation(m_swipeAnimation.get());
    }
    else
    {
        auto mainContentVisual = winrt::ElementCompositionPreview::GetElementVisual(m_content.get());
        if (mainContentVisual && m_mainContentVisual.get() != mainContentVisual)
        {
            m_mainContentVisual.set(mainContentVisual);

            if (DownlevelHelper::SetIsTranslationEnabledExists())
            {
                winrt::ElementCompositionPreview::SetIsTranslationEnabled(m_content.get(), true);
                mainContentVisual.Properties().InsertVector3(s_translationPropertyName, { 0.0f, 0.0f, 0.0f });
            }
            mainContentVisual.StartAnimation(GetAnimationTarget(m_content.get()), m_swipeAnimation.get());

            m_executeExpressionAnimation.get().SetReferenceParameter(s_foregroundVisualPropertyName, mainContentVisual);
        }
    }

    if (IsTranslationFacadeAvailableForSwipeControl(m_swipeContentStackPanel.get()))
    {
        m_swipeAnimation.get().Target(GetAnimationTarget(m_swipeContentStackPanel.get()));
    }
    else
    {
        auto swipeContentVisual = winrt::ElementCompositionPreview::GetElementVisual(m_swipeContentStackPanel.get());
        if (swipeContentVisual && m_swipeContentVisual.get() != swipeContentVisual)
        {
            m_swipeContentVisual.set(swipeContentVisual);

            if (DownlevelHelper::SetIsTranslationEnabledExists())
            {
                winrt::ElementCompositionPreview::SetIsTranslationEnabled(m_swipeContentStackPanel.get(), true);
                swipeContentVisual.Properties().InsertVector3(s_translationPropertyName, { 0.0f, 0.0f, 0.0f });
            }

            ConfigurePositionInertiaRestingValues();
        }
    }

    auto swipeContentRootVisual = winrt::ElementCompositionPreview::GetElementVisual(m_swipeContentRoot.get());
    if (swipeContentRootVisual && m_swipeContentRootVisual.get() != swipeContentRootVisual)
    {
        m_swipeContentRootVisual.set(swipeContentRootVisual);
        m_clipExpressionAnimation.get().SetReferenceParameter(s_swipeRootVisualPropertyName, swipeContentRootVisual);
        if (m_insetClip)
        {
            swipeContentRootVisual.Clip(m_insetClip.get());
        }
    }
}

void SwipeControl::UpdateIsOpen(bool isOpen)
{
    SWIPECONTROL_TRACE_INFO(*this, TRACE_MSG_METH, METH_NAME, this);

    if (isOpen)
    {
        if (!m_isOpen)
        {
            m_isOpen = true;
            m_lastActionWasOpening = true;
            switch (m_createdContent)
            {
            case CreatedContent::Right:
            case CreatedContent::Bottom:
                m_interactionTracker.get().Properties().InsertBoolean(s_isFarOpenPropertyName, true);
                m_interactionTracker.get().Properties().InsertBoolean(s_isNearOpenPropertyName, false);
                break;
            case CreatedContent::Left:
            case CreatedContent::Top:
                m_interactionTracker.get().Properties().InsertBoolean(s_isFarOpenPropertyName, false);
                m_interactionTracker.get().Properties().InsertBoolean(s_isNearOpenPropertyName, true);
                break;
            case CreatedContent::None:
                m_interactionTracker.get().Properties().InsertBoolean(s_isFarOpenPropertyName, false);
                m_interactionTracker.get().Properties().InsertBoolean(s_isNearOpenPropertyName, false);
                break;
            default:
                assert(false);
            }

            if (m_currentItems.get().Mode() != winrt::SwipeMode::Execute)
            {
                AttachDismissingHandlers();
            }

            if (auto globalTestHooks = SwipeTestHooks::GetGlobalTestHooks())
            {
                globalTestHooks->NotifyOpenedStatusChanged(*this);
            }
        }
    }
    else
    {
        if (m_isOpen)
        {
            m_isOpen = false;
            m_lastActionWasClosing = true;
            DetachDismissingHandlers();
            m_interactionTracker.get().Properties().InsertBoolean(s_isFarOpenPropertyName, false);
            m_interactionTracker.get().Properties().InsertBoolean(s_isNearOpenPropertyName, false);

            if (auto globalTestHooks = SwipeTestHooks::GetGlobalTestHooks())
            {
                globalTestHooks->NotifyOpenedStatusChanged(*this);
            }
        }
    }
}

void SwipeControl::UpdateThresholdReached(float value)
{
    SWIPECONTROL_TRACE_VERBOSE(*this, TRACE_MSG_METH, METH_NAME, this);

    const bool oldValue = m_thresholdReached;
    const float effectiveStackPanelSize = static_cast<float>((m_isHorizontal ? m_swipeContentStackPanel.get().ActualWidth() : m_swipeContentStackPanel.get().ActualHeight()) - 1);
    if (!m_isOpen || m_lastActionWasOpening)
    {
        //If we are opening new swipe items then we need to scroll open c_ThresholdValue
        m_thresholdReached = abs(value) > std::min(effectiveStackPanelSize, c_ThresholdValue);
    }
    else
    {
        //If we already have an open swipe item then swiping it closed by any amount will close it.
        m_thresholdReached = abs(value) < effectiveStackPanelSize;
    }
    if (m_thresholdReached != oldValue)
    {
        UpdateColorsIfExecuteItem();
    }
}

void SwipeControl::ThrowIfHasVerticalAndHorizontalContent(bool setIsHorizontal)
{
    const bool hasLeftContent = LeftItems() && LeftItems().Size() > 0;
    const bool hasRightContent = RightItems() && RightItems().Size() > 0;
    const bool hasTopContent = TopItems() && TopItems().Size() > 0;
    const bool hasBottomContent = BottomItems() && BottomItems().Size() > 0;
    if (setIsHorizontal)
    {
        m_isHorizontal = hasLeftContent || hasRightContent || !(hasTopContent || hasBottomContent);
    }

    if (this->Template())
    {
        if (m_isHorizontal && (hasTopContent || hasBottomContent))
        {
            throw winrt::hresult_invalid_argument(L"This SwipeControl is horizontal and can not have vertical items.");
        }
        if (!m_isHorizontal && (hasLeftContent || hasRightContent))
        {
            throw winrt::hresult_invalid_argument(L"This SwipeControl is vertical and can not have horizontal items.");
        }
    }
    else
    {
        if ((hasLeftContent || hasRightContent) && (hasTopContent || hasBottomContent))
        {
            throw winrt::hresult_invalid_argument(L"SwipeControl can't have both horizontal items and vertical items set at the same time.");
        }
    }
}

std::wstring SwipeControl::GetAnimationTarget(winrt::UIElement child)
{
    if (DownlevelHelper::SetIsTranslationEnabledExists() || SharedHelpers::IsTranslationFacadeAvailable(child))
    {
        return s_translationPropertyName.data();
    }
    else
    {
        return s_offsetPropertyName.data();
    }
}

winrt::SwipeControl SwipeControl::GetThis()
{
    return *this;
}

bool SwipeControl::IsTranslationFacadeAvailableForSwipeControl(const winrt::UIElement& element)
{
    //For now Facade's are causing more issues than they are worth for swipe control. Revist this
    //when we have a little more time.

    //There are concerns about swipe consumers having taken a dependency on the ElementCompositionPreview
    //Api's that this is exclusive with and also the target property of the swipe expression animations
    //is not resolving with the use of Facade's
    return false;
    //return SharedHelpers::IsTranslationFacadeAvailable(element);
}

wstring_view SwipeControl::DirectionToInset(const CreatedContent& createdContent)
{
    switch (createdContent)
    {
    case CreatedContent::Right:
        return s_leftInsetTargetName;
    case CreatedContent::Left:
        return s_rightInsetTargetName;
    case CreatedContent::Bottom:
        return s_topInsetTargetName;
    case CreatedContent::Top:
        return s_bottomInsetTargetName;
    case CreatedContent::None:
        return L"";
    default:
        assert(false);
        return L"";
    }
}
