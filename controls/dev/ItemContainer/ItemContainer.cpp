// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#include "pch.h"
#include "common.h"
#include "TypeLogging.h"
#include "ItemContainer.h"
#include "RuntimeProfiler.h"
#include "ResourceAccessor.h"
#include "PointerInfoRevokers.h"
#include "ItemContainerInvokedEventArgs.h"
#include "ItemContainerAutomationPeer.h"

// Change to 'true' to turn on debugging outputs in Output window
bool ItemContainerTrace::s_IsDebugOutputEnabled{ false };
bool ItemContainerTrace::s_IsVerboseDebugOutputEnabled{ false };

// Keeps track of the one ItemContainer with the mouse pointer over, if any.
// The OnPointerExited method does not get invoked when the ItemContainer is scrolled away from the mouse pointer.
// This static instance allows to clear the mouse over state when any other ItemContainer gets the mouse over state.
static thread_local winrt::weak_ref<ItemContainer> s_mousePointerOverInstance = nullptr;

ItemContainer::ItemContainer()
{
    ITEMCONTAINER_TRACE_INFO(nullptr, TRACE_MSG_METH, METH_NAME, this);

    __RP_Marker_ClassById(RuntimeProfiler::ProfId_ItemContainer);

    EnsureProperties();
    SetDefaultStyleKey(this);
}

ItemContainer::~ItemContainer()
{
    ITEMCONTAINER_TRACE_INFO(nullptr, TRACE_MSG_METH, METH_NAME, this);

    if (auto mousePointerOverInstance = s_mousePointerOverInstance.get())
    {
        if (mousePointerOverInstance.get() == this)
        {
            s_mousePointerOverInstance = nullptr;
        }
    }
}

void ItemContainer::OnApplyTemplate()
{
    ITEMCONTAINER_TRACE_VERBOSE(*this, TRACE_MSG_METH, METH_NAME, this);

    // Unload events from previous template when ItemContainer gets a new template.
    if (auto&& selectionCheckbox = m_selectionCheckbox.get())
    {
        m_checked_revoker.revoke();
        m_unchecked_revoker.revoke();
    }

    winrt::IControlProtected controlProtected{ *this };
    m_rootPanel.set(GetTemplateChildT<winrt::Panel>(s_containerRootName, controlProtected));

    // If the Child property is already set, add it to the tree. After this point, the
    // property changed event for Child property will add it to the tree.
    if (const auto& child = Child())
    {
        if (auto&& rootPanel = m_rootPanel.get())
        {
            rootPanel.Children().InsertAt(0, child);
        }
    }

    m_pointerInfo = std::make_shared<PointerInfo<ItemContainer>>();
    m_isEnabledChangedRevoker.revoke();
    m_isEnabledChangedRevoker = IsEnabledChanged(winrt::auto_revoke, { this,  &ItemContainer::OnIsEnabledChanged });

    __super::OnApplyTemplate();

    UpdateVisualState(true);
    UpdateMultiSelectState(true);
}

winrt::AutomationPeer ItemContainer::OnCreateAutomationPeer()
{
    return winrt::make<ItemContainerAutomationPeer>(*this);
}

void ItemContainer::OnIsEnabledChanged(winrt::IInspectable const& sender, winrt::DependencyPropertyChangedEventArgs const& args)
{
    ITEMCONTAINER_TRACE_INFO(*this, TRACE_MSG_METH_INT, METH_NAME, this, IsEnabled());

    if (!IsEnabled())
    {
        ProcessPointerCanceled(nullptr);
    }
    else
    {
        UpdateVisualState(true);
    }
}

void ItemContainer::OnPropertyChanged(winrt::DependencyPropertyChangedEventArgs const& args)
{
    auto dependencyProperty = args.Property();

#ifdef DBG
    if (dependencyProperty == s_IsSelectedProperty)
    {
        const bool oldValue = unbox_value<bool>(args.OldValue());
        const bool newValue = unbox_value<bool>(args.NewValue());

        ITEMCONTAINER_TRACE_VERBOSE(nullptr, TRACE_MSG_METH_STR_INT_INT, METH_NAME, this, L"IsSelected", oldValue, newValue);
    }
    else if (dependencyProperty == s_MultiSelectModeProperty)
    {
        const winrt::ItemContainerMultiSelectMode oldValue = unbox_value<winrt::ItemContainerMultiSelectMode>(args.OldValue());
        const winrt::ItemContainerMultiSelectMode newValue = unbox_value<winrt::ItemContainerMultiSelectMode>(args.NewValue());

        ITEMCONTAINER_TRACE_VERBOSE(nullptr, TRACE_MSG_METH_STR_STR, METH_NAME, this, L"MultiSelectMode", TypeLogging::ItemContainerMultiSelectModeToString(newValue).c_str());
    }
    else
    {
        ITEMCONTAINER_TRACE_VERBOSE(nullptr, L"%s[%p](property: %s)\n", METH_NAME, this, DependencyPropertyToString(dependencyProperty).c_str());
    }
#endif

    if (dependencyProperty == s_IsSelectedProperty)
    {
#ifdef MUX_PRERELEASE
        const bool isMultipleOrExtended = (static_cast<int>(MultiSelectMode() & (winrt::ItemContainerMultiSelectMode::Multiple | winrt::ItemContainerMultiSelectMode::Extended)));
#else
        const bool isMultipleOrExtended = (static_cast<int>(MultiSelectModeInternal() & (winrt::ItemContainerMultiSelectMode::Multiple | winrt::ItemContainerMultiSelectMode::Extended)));
#endif

        const winrt::AutomationEvents eventToRaise =
            IsSelected() ?
            (isMultipleOrExtended ? winrt::AutomationEvents::SelectionItemPatternOnElementAddedToSelection : winrt::AutomationEvents::SelectionItemPatternOnElementSelected) :
            (isMultipleOrExtended ? winrt::AutomationEvents::SelectionItemPatternOnElementRemovedFromSelection :winrt::AutomationEvents::PropertyChanged);

        if (eventToRaise != winrt::AutomationEvents::PropertyChanged && winrt::AutomationPeer::ListenerExists(eventToRaise))
        {
            if (const auto peer = winrt::FrameworkElementAutomationPeer::CreatePeerForElement(*this))
            {
                peer.RaiseAutomationEvent(eventToRaise);
            }
        }
    }
    else if (dependencyProperty == s_ChildProperty)
    {
        if (auto&& rootPanel = m_rootPanel.get())
        {
            unsigned int oldChildIndex{};

            if (args.OldValue() != nullptr && rootPanel.Children().IndexOf(args.OldValue().as<winrt::UIElement>(), oldChildIndex))
            {
                rootPanel.Children().RemoveAt(oldChildIndex);
            }

            rootPanel.Children().InsertAt(0, args.NewValue().as<winrt::UIElement>());
        }
    }

    if (m_pointerInfo != nullptr)
    {
        if (dependencyProperty == s_IsSelectedProperty ||
            dependencyProperty == s_MultiSelectModeProperty)
        {
            UpdateVisualState(true);
            UpdateMultiSelectState(true);
        }
    }
}

void ItemContainer::GoToState(std::wstring_view const& stateName, bool useTransitions)
{
    ITEMCONTAINER_TRACE_VERBOSE(*this, TRACE_MSG_METH_STR_INT, METH_NAME, this, stateName.data(), useTransitions);

    winrt::VisualStateManager::GoToState(*this, stateName, useTransitions);
}

void ItemContainer::UpdateVisualState(bool useTransitions)
{
    ITEMCONTAINER_TRACE_VERBOSE(*this, TRACE_MSG_METH_INT, METH_NAME, this, useTransitions);

    // DisabledStates
    if (!IsEnabled())
    {
        GoToState(s_disabledStateName, useTransitions);
        GoToState(IsSelected() ? s_selectedNormalStateName : s_unselectedNormalStateName, useTransitions);
    }
    else
    {
        GoToState(s_enabledStateName, useTransitions);

        // CombinedStates
        if (m_pointerInfo == nullptr)
        {
            return;
        }

        if (m_pointerInfo->IsPressed() && IsSelected())
        {
            GoToState(s_selectedPressedStateName, useTransitions);
        }
        else if (m_pointerInfo->IsPressed())
        {
            GoToState(s_unselectedPressedStateName, useTransitions);
        }
        else if (m_pointerInfo->IsPointerOver() && IsSelected())
        {
            GoToState(s_selectedPointerOverStateName, useTransitions);
        }
        else if (m_pointerInfo->IsPointerOver())
        {
            GoToState(s_unselectedPointerOverStateName, useTransitions);
        }
        else if (IsSelected())
        {
            GoToState(s_selectedNormalStateName, useTransitions);
        }
        else
        {
            GoToState(s_unselectedNormalStateName, useTransitions);
        }
    }
}

void ItemContainer::UpdateMultiSelectState(bool useTransitions)
{
    ITEMCONTAINER_TRACE_VERBOSE(*this, TRACE_MSG_METH_INT, METH_NAME, this, useTransitions);

#ifdef MUX_PRERELEASE
    const winrt::ItemContainerMultiSelectMode multiSelectMode = MultiSelectMode();
#else
    const winrt::ItemContainerMultiSelectMode multiSelectMode = MultiSelectModeInternal();
#endif

    // MultiSelectStates
    if (static_cast<int>(multiSelectMode & winrt::ItemContainerMultiSelectMode::Multiple))
    {
        GoToState(s_multipleStateName, useTransitions);

        if (!m_selectionCheckbox.get())
        {
            LoadSelectionCheckbox();
        }

        UpdateCheckboxState();
    }
    else
    {
        GoToState(s_singleStateName, useTransitions);
    }
}

void ItemContainer::ProcessPointerOver(winrt::PointerRoutedEventArgs const& args, bool isPointerOver)
{
    if (m_pointerInfo)
    {
        const bool oldIsPointerOver = m_pointerInfo->IsPointerOver();
        auto const& pointerDeviceType = args.Pointer().PointerDeviceType();

        if (pointerDeviceType == winrt::PointerDeviceType::Touch)
        {
            if (isPointerOver)
            {
                m_pointerInfo->SetIsTouchPointerOver();
            }
            else
            {
                // Once a touch pointer leaves the ItemContainer it is no longer tracked because this
                // ItemContainer may never receive a PointerReleased, PointerCanceled or PointerCaptureLost
                // for that PointerId.
                m_pointerInfo->ResetAll();
            }
        }
        else if (pointerDeviceType == winrt::PointerDeviceType::Pen)
        {
            if (isPointerOver)
            {
                m_pointerInfo->SetIsPenPointerOver();
            }
            else
            {
                // Once a pen pointer leaves the ItemContainer it is no longer tracked because this
                // ItemContainer may never receive a PointerReleased, PointerCanceled or PointerCaptureLost
                // for that PointerId.
                m_pointerInfo->ResetAll();
            }
        }
        else
        {
            if (isPointerOver)
            {
                m_pointerInfo->SetIsMousePointerOver();
            }
            else
            {
                m_pointerInfo->ResetIsMousePointerOver();
            }
            UpdateMousePointerOverInstance(isPointerOver);
        }

        if (m_pointerInfo->IsPointerOver() != oldIsPointerOver)
        {
            UpdateVisualState(true);
        }
    }
}

void ItemContainer::ProcessPointerCanceled(winrt::PointerRoutedEventArgs const& args)
{
    ITEMCONTAINER_TRACE_INFO(*this, TRACE_MSG_METH_INT, METH_NAME, this, args == nullptr ? -1 : args.Pointer().PointerId());

    if (m_pointerInfo)
    {
        if (args == nullptr)
        {
            m_pointerInfo->ResetAll();
        }
        else
        {
            auto const& pointer = args.Pointer();

            if (m_pointerInfo->IsPointerIdTracked(pointer.PointerId()))
            {
                m_pointerInfo->ResetTrackedPointerId();
            }

            auto const& pointerDeviceType = pointer.PointerDeviceType();

            switch (pointerDeviceType)
            {
            case winrt::PointerDeviceType::Touch:
                m_pointerInfo->ResetPointerPressed();
                m_pointerInfo->ResetIsTouchPointerOver();
                break;
            case winrt::PointerDeviceType::Pen:
                m_pointerInfo->ResetPointerPressed();
                m_pointerInfo->ResetIsPenPointerOver();
                break;
            case winrt::PointerDeviceType::Mouse:
                m_pointerInfo->ResetIsMouseButtonPressed(true /*isForLeftMouseButton*/);
                m_pointerInfo->ResetIsMouseButtonPressed(false /*isForLeftMouseButton*/);
                m_pointerInfo->ResetIsMousePointerOver();
                UpdateMousePointerOverInstance(false /*isPointerOver*/);
                break;
            }
        }
    }

    UpdateVisualState(true);
}

void ItemContainer::OnPointerEntered(winrt::PointerRoutedEventArgs const& args)
{
    ITEMCONTAINER_TRACE_VERBOSE(*this, TRACE_MSG_METH_INT, METH_NAME, this, args.Pointer().PointerId());

    __super::OnPointerEntered(args);

    ProcessPointerOver(args, true /*isPointerOver*/);
}

void ItemContainer::OnPointerMoved(winrt::PointerRoutedEventArgs const& args)
{
    __super::OnPointerMoved(args);

    ProcessPointerOver(args, true /*isPointerOver*/);
}

void ItemContainer::OnPointerExited(winrt::PointerRoutedEventArgs const& args)
{
    ITEMCONTAINER_TRACE_VERBOSE(*this, TRACE_MSG_METH_INT, METH_NAME, this, args.Pointer().PointerId());

    __super::OnPointerExited(args);

    ProcessPointerOver(args, false /*isPointerOver*/);
}

void ItemContainer::OnPointerPressed(winrt::PointerRoutedEventArgs const& args)
{
    ITEMCONTAINER_TRACE_VERBOSE(*this, TRACE_MSG_METH_INT, METH_NAME, this, args.Pointer().PointerId());

    __super::OnPointerPressed(args);

    if (m_pointerInfo == nullptr || args.Handled())
    {
        return;
    }

    auto const& pointer = args.Pointer();
    auto const& pointerDeviceType = pointer.PointerDeviceType();
    winrt::PointerUpdateKind pointerUpdateKind{ winrt::PointerUpdateKind::Other };

    if (pointerDeviceType == winrt::PointerDeviceType::Mouse)
    {
        const auto pointerProperties = args.GetCurrentPoint(*this).Properties();

        pointerUpdateKind = pointerProperties.PointerUpdateKind();

        if (pointerUpdateKind != winrt::PointerUpdateKind::LeftButtonPressed &&
            pointerUpdateKind != winrt::PointerUpdateKind::RightButtonPressed)
        {
            return;
        }
    }

    ITEMCONTAINER_TRACE_VERBOSE(*this, TRACE_MSG_METH_STR_STR, METH_NAME, this, TypeLogging::PointerDeviceTypeToString(pointerDeviceType).c_str(), TypeLogging::PointerUpdateKindToString(pointerUpdateKind).c_str());

    const auto pointerId = pointer.PointerId();

    if (!m_pointerInfo->IsTrackingPointer())
    {
        m_pointerInfo->TrackPointerId(pointerId);
    }
    else if (!m_pointerInfo->IsPointerIdTracked(pointerId))
    {
        return;
    }

    bool canRaiseItemInvoked = CanRaiseItemInvoked();

    switch (pointerDeviceType)
    {
    case winrt::PointerDeviceType::Touch:
        m_pointerInfo->SetPointerPressed();
        m_pointerInfo->SetIsTouchPointerOver();
        break;
    case winrt::PointerDeviceType::Pen:
        m_pointerInfo->SetPointerPressed();
        m_pointerInfo->SetIsPenPointerOver();
        break;
    case winrt::PointerDeviceType::Mouse:
        m_pointerInfo->SetIsMouseButtonPressed(pointerUpdateKind == winrt::PointerUpdateKind::LeftButtonPressed /*isForLeftMouseButton*/);
        canRaiseItemInvoked &= pointerUpdateKind == winrt::PointerUpdateKind::LeftButtonPressed;
        m_pointerInfo->SetIsMousePointerOver();
        UpdateMousePointerOverInstance(true /*isPointerOver*/);
        break;
    }

    if (canRaiseItemInvoked)
    {
        args.Handled(RaiseItemInvoked(winrt::ItemContainerInteractionTrigger::PointerPressed, args.OriginalSource()));
    }

    UpdateVisualState(true);
}

void ItemContainer::OnPointerReleased(winrt::PointerRoutedEventArgs const& args)
{
    ITEMCONTAINER_TRACE_VERBOSE(*this, TRACE_MSG_METH_INT, METH_NAME, this, args.Pointer().PointerId());

    __super::OnPointerReleased(args);

    if (m_pointerInfo == nullptr ||
        !m_pointerInfo->IsPointerIdTracked(args.Pointer().PointerId()) ||
        !m_pointerInfo->IsPressed())
    {
        return;
    }

    m_pointerInfo->ResetTrackedPointerId();

    bool canRaiseItemInvoked = CanRaiseItemInvoked();
    auto const& pointerDeviceType = args.Pointer().PointerDeviceType();

    if (pointerDeviceType == winrt::PointerDeviceType::Mouse)
    {
        const auto pointerProperties = args.GetCurrentPoint(*this).Properties();
        const auto pointerUpdateKind = pointerProperties.PointerUpdateKind();

        ITEMCONTAINER_TRACE_VERBOSE(*this, TRACE_MSG_METH_STR_STR, METH_NAME, this, TypeLogging::PointerDeviceTypeToString(pointerDeviceType).c_str(), TypeLogging::PointerUpdateKindToString(pointerUpdateKind).c_str());

        if (pointerUpdateKind == winrt::PointerUpdateKind::LeftButtonReleased ||
            pointerUpdateKind == winrt::PointerUpdateKind::RightButtonReleased)
        {
            m_pointerInfo->ResetIsMouseButtonPressed(pointerUpdateKind == winrt::PointerUpdateKind::LeftButtonReleased /*isForLeftMouseButton*/);
        }
        canRaiseItemInvoked &= pointerUpdateKind == winrt::PointerUpdateKind::LeftButtonReleased;
    }
    else
    {
        ITEMCONTAINER_TRACE_VERBOSE(*this, TRACE_MSG_METH_STR, METH_NAME, this, TypeLogging::PointerDeviceTypeToString(pointerDeviceType).c_str());

        m_pointerInfo->ResetPointerPressed();
    }

    if (canRaiseItemInvoked &&
        !args.Handled() &&
        !m_pointerInfo->IsPressed())
    {
        args.Handled(RaiseItemInvoked(winrt::ItemContainerInteractionTrigger::PointerReleased, args.OriginalSource()));
    }

    UpdateVisualState(true);
}

void ItemContainer::OnPointerCanceled(winrt::PointerRoutedEventArgs const& args)
{
    ITEMCONTAINER_TRACE_VERBOSE(*this, TRACE_MSG_METH_INT, METH_NAME, this, args.Pointer().PointerId());

    __super::OnPointerCanceled(args);

    ProcessPointerCanceled(args);
}

void ItemContainer::OnPointerCaptureLost(winrt::PointerRoutedEventArgs const& args)
{
    ITEMCONTAINER_TRACE_VERBOSE(*this, TRACE_MSG_METH_INT, METH_NAME, this, args.Pointer().PointerId());

    __super::OnPointerCaptureLost(args);

    ProcessPointerCanceled(args);
}

void ItemContainer::OnTapped(winrt::TappedRoutedEventArgs const& args)
{
    ITEMCONTAINER_TRACE_VERBOSE(*this, TRACE_MSG_METH, METH_NAME, this);

    __super::OnTapped(args);

    if (CanRaiseItemInvoked() && !args.Handled())
    {
        args.Handled(RaiseItemInvoked(winrt::ItemContainerInteractionTrigger::Tap, args.OriginalSource()));
    }
}

void ItemContainer::OnDoubleTapped(winrt::DoubleTappedRoutedEventArgs const& args)
{
    ITEMCONTAINER_TRACE_VERBOSE(*this, TRACE_MSG_METH, METH_NAME, this);

    __super::OnDoubleTapped(args);

    if (CanRaiseItemInvoked() && !args.Handled())
    {
        args.Handled(RaiseItemInvoked(winrt::ItemContainerInteractionTrigger::DoubleTap, args.OriginalSource()));
    }
}

void ItemContainer::OnKeyDown(winrt::KeyRoutedEventArgs const& args)
{
    ITEMCONTAINER_TRACE_VERBOSE(*this, TRACE_MSG_METH_STR, METH_NAME, this, TypeLogging::KeyRoutedEventArgsToString(args).c_str());

    __super::OnKeyDown(args);

    if (!args.Handled() && CanRaiseItemInvoked())
    {
        if (args.Key() == winrt::VirtualKey::Enter)
        {
            args.Handled(RaiseItemInvoked(winrt::ItemContainerInteractionTrigger::EnterKey, args.OriginalSource()));
        }
        else if (args.Key() == winrt::VirtualKey::Space)
        {
            args.Handled(RaiseItemInvoked(winrt::ItemContainerInteractionTrigger::SpaceKey, args.OriginalSource()));
        }
    }
}

bool ItemContainer::CanRaiseItemInvoked()
{
#ifdef MUX_PRERELEASE
    return static_cast<int>(CanUserInvoke() & winrt::ItemContainerUserInvokeMode::UserCanInvoke) ||
        static_cast<int>(CanUserSelect() & (winrt::ItemContainerUserSelectMode::Auto | winrt::ItemContainerUserSelectMode::UserCanSelect));
#else
    return static_cast<int>(CanUserInvokeInternal() & winrt::ItemContainerUserInvokeMode::UserCanInvoke) ||
        static_cast<int>(CanUserSelectInternal() & (winrt::ItemContainerUserSelectMode::Auto | winrt::ItemContainerUserSelectMode::UserCanSelect));
#endif
}

bool ItemContainer::RaiseItemInvoked(winrt::ItemContainerInteractionTrigger const& interactionTrigger, winrt::IInspectable const& originalSource)
{
    ITEMCONTAINER_TRACE_VERBOSE(*this, TRACE_MSG_METH_STR, METH_NAME, this, TypeLogging::ItemContainerInteractionTriggerToString(interactionTrigger).c_str());

    if (m_itemInvokedEventSource)
    {
        const auto itemInvokedEventArgs = winrt::make_self<ItemContainerInvokedEventArgs>(interactionTrigger, originalSource);
        m_itemInvokedEventSource(*this, *itemInvokedEventArgs);

        return itemInvokedEventArgs->Handled();
    }

    return false;
}

void ItemContainer::LoadSelectionCheckbox()
{
    winrt::IControlProtected controlProtected{ *this };

    m_selectionCheckbox.set(GetTemplateChildT<winrt::CheckBox>(s_selectionCheckboxName, controlProtected));

    if (auto&& selectionCheckbox = m_selectionCheckbox.get())
    {
        m_checked_revoker = selectionCheckbox.Checked(winrt::auto_revoke, { this, &ItemContainer::OnCheckToggle });
        m_unchecked_revoker = selectionCheckbox.Unchecked(winrt::auto_revoke, { this, &ItemContainer::OnCheckToggle });
    }
}

void ItemContainer::OnCheckToggle(winrt::IInspectable const& sender, winrt::RoutedEventArgs const& /*args*/)
{
    UpdateCheckboxState();
}

void ItemContainer::UpdateCheckboxState()
{
    if (auto&& selectionCheckbox = m_selectionCheckbox.get())
    {
        const bool isChecked = SharedHelpers::IsTrue(selectionCheckbox.IsChecked());
        const bool isSelected = IsSelected();

        if (isChecked != isSelected)
        {
            selectionCheckbox.IsChecked(isSelected);
        }
    }
}

void ItemContainer::UpdateMousePointerOverInstance(bool isPointerOver)
{
    auto mousePointerOverInstance = s_mousePointerOverInstance.get();

    if (isPointerOver)
    {
        if (mousePointerOverInstance == nullptr || mousePointerOverInstance.get() != this)
        {
            if (mousePointerOverInstance != nullptr && mousePointerOverInstance->m_pointerInfo != nullptr)
            {
                mousePointerOverInstance->m_pointerInfo->ResetIsMousePointerOver();
            }

            s_mousePointerOverInstance = get_weak();
        }
    }
    else
    {
        if (mousePointerOverInstance != nullptr && mousePointerOverInstance.get() == this)
        {
            s_mousePointerOverInstance = nullptr;
        }
    }
}

#ifdef DBG

winrt::hstring ItemContainer::DependencyPropertyToString(
    const winrt::IDependencyProperty& dependencyProperty)
{
    if (dependencyProperty == s_IsSelectedProperty)
    {
        return L"IsSelected";
    }
    else if (dependencyProperty == s_CanUserSelectProperty)
    {
        return L"CanUserSelect";
    }
    else if (dependencyProperty == s_CanUserInvokeProperty)
    {
        return L"CanUserInvoke";
    }
    else if (dependencyProperty == s_MultiSelectModeProperty)
    {
        return L"MultiSelectMode";
    }
    else
    {
        return L"UNKNOWN";
    }
}

winrt::Size ItemContainer::MeasureOverride(winrt::Size const& availableSize)
{
    ITEMCONTAINER_TRACE_VERBOSE(*this, TRACE_MSG_METH_STR_FLT_FLT, METH_NAME, this, L"availableSize", availableSize.Width, availableSize.Height);

    winrt::Size returnedSize = __super::MeasureOverride(availableSize);

    ITEMCONTAINER_TRACE_VERBOSE(*this, TRACE_MSG_METH_STR_FLT_FLT, METH_NAME, this, L"returnedSize", returnedSize.Width, returnedSize.Height);

    return returnedSize;
}

winrt::Size ItemContainer::ArrangeOverride(winrt::Size const& finalSize)
{
    ITEMCONTAINER_TRACE_VERBOSE(*this, TRACE_MSG_METH_STR_FLT_FLT, METH_NAME, this, L"finalSize", finalSize.Width, finalSize.Height);

    winrt::Size returnedSize = __super::ArrangeOverride(finalSize);

    ITEMCONTAINER_TRACE_VERBOSE(*this, TRACE_MSG_METH_STR_FLT_FLT, METH_NAME, this, L"returnedSize", returnedSize.Width, returnedSize.Height);

    return returnedSize;
}

#endif
