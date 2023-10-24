// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#include "pch.h"
#include "common.h"
#include "TabView.h"
#include "TabViewItem.h"
#include "RuntimeProfiler.h"
#include "ResourceAccessor.h"
#include "XamlControlsResources.h"
#include "SharedHelpers.h"
#include <FrameworkUdk/Containment.h>

// Bug 46770740: I can still see a thin line under the active tab in file explorer
#define WINAPPSDK_CHANGEID_46770740 46770740

TabViewItem::TabViewItem()
{
    __RP_Marker_ClassById(RuntimeProfiler::ProfId_TabViewItem);

    SetDefaultStyleKey(this);

    SetValue(s_TabViewTemplateSettingsProperty, winrt::make<TabViewItemTemplateSettings>());

    Loaded({ this, &TabViewItem::OnLoaded });
    SizeChanged({ this, &TabViewItem::OnSizeChanged });

    RegisterPropertyChangedCallback(winrt::SelectorItem::IsSelectedProperty(), { this, &TabViewItem::OnIsSelectedPropertyChanged });
    RegisterPropertyChangedCallback(winrt::Control::ForegroundProperty(), { this, &TabViewItem::OnForegroundPropertyChanged });
}

void TabViewItem::OnApplyTemplate()
{
    __super::OnApplyTemplate();

    winrt::IControlProtected controlProtected{ *this };

    m_selectedBackgroundPathSizeChangedRevoker.revoke();
    m_closeButtonClickRevoker.revoke();
    m_tabDragStartingRevoker.revoke();
    m_tabDragCompletedRevoker.revoke();

    m_selectedBackgroundPath.set(GetTemplateChildT<winrt::Path>(s_selectedBackgroundPathName, controlProtected));

    if (const auto selectedBackgroundPath = m_selectedBackgroundPath.get())
    {
        m_selectedBackgroundPathSizeChangedRevoker = selectedBackgroundPath.SizeChanged(winrt::auto_revoke,
        {
            [this](auto const&, auto const&)
            {
                UpdateSelectedBackgroundPathTranslateTransform();
            }
        });
    }

    m_headerContentPresenter.set(GetTemplateChildT<winrt::ContentPresenter>(s_contentPresenterName, controlProtected));

    auto tabView = SharedHelpers::GetAncestorOfType<winrt::TabView>(winrt::VisualTreeHelper::GetParent(*this));
    auto internalTabView = tabView
        ? winrt::get_self<TabView>(tabView)
        : nullptr;

    m_closeButton.set([this, controlProtected, internalTabView]() {
        auto closeButton = GetTemplateChildT<winrt::Button>(s_closeButtonName, controlProtected);
        if (closeButton)
        {
            // Do localization for the close button automation name
            if (winrt::AutomationProperties::GetName(closeButton).empty())
            {
                auto const closeButtonName = ResourceAccessor::GetLocalizedStringResource(SR_TabViewCloseButtonName);
                winrt::AutomationProperties::SetName(closeButton, closeButtonName);
            }

            if (internalTabView)
            {
                // Setup the tooltip for the close button
                auto tooltip = winrt::ToolTip();
                tooltip.Content(box_value(internalTabView->GetTabCloseButtonTooltipText()));
                winrt::ToolTipService::SetToolTip(closeButton, tooltip);
            }

            m_closeButtonClickRevoker = closeButton.Click(winrt::auto_revoke, { this, &TabViewItem::OnCloseButtonClick });
        }
        return closeButton;
        }());

    OnHeaderChanged();
    OnIconSourceChanged();

    if (tabView)
    {
        if (!WinAppSdk::Containment::IsChangeEnabled<WINAPPSDK_CHANGEID_46770740>())
        {
            if (internalTabView)
            {          
                m_shadow = winrt::ThemeShadow{};

                double shadowDepth = unbox_value<double>(SharedHelpers::FindInApplicationResources(c_tabViewShadowDepthName, box_value(c_tabShadowDepth)));

                const auto currentTranslation = Translation();
                const auto translation = winrt::float3{ currentTranslation.x, currentTranslation.y, static_cast<float>(shadowDepth) };
                Translation(translation);

                // Set separate shadow for TabDragVisualContainer.
                m_tabDragVisualContainer.set(GetTemplateChildT<winrt::UIElement>(s_tabDragVisualContainer, controlProtected));

                if (const auto tabDragVisualContainer = m_tabDragVisualContainer.get())
                {
                    tabDragVisualContainer.Translation(translation);
                }         

                UpdateShadow();
            }
        }
        m_tabDragStartingRevoker = tabView.TabDragStarting(winrt::auto_revoke, { this, &TabViewItem::OnTabDragStarting });
        m_tabDragCompletedRevoker = tabView.TabDragCompleted(winrt::auto_revoke, { this, &TabViewItem::OnTabDragCompleted });
    }

    UpdateCloseButton();
    UpdateForeground();
    UpdateWidthModeVisualState();
    UpdateTabGeometry();
}

void TabViewItem::UpdateTabGeometry()
{
    auto const templateSettings = winrt::get_self<TabViewItemTemplateSettings>(TabViewTemplateSettings());
    auto const height = ActualHeight();
    auto const popupRadius = unbox_value<winrt::CornerRadius>(ResourceAccessor::ResourceLookup(*this, box_value(c_overlayCornerRadiusKey)));
    auto const leftCorner = popupRadius.TopLeft;
    auto const rightCorner = popupRadius.TopRight;

    // Assumes 4px curving-out corners, which are hardcoded in the markup
    auto data = L"<Geometry xmlns='http://schemas.microsoft.com/winfx/2006/xaml/presentation'>F1 M0,%f  a 4,4 0 0 0 4,-4  L 4,%f  a %f,%f 0 0 1 %f,-%f  l %f,0  a %f,%f 0 0 1 %f,%f  l 0,%f  a 4,4 0 0 0 4,4 Z</Geometry>";

    WCHAR strOut[1024];
    StringCchPrintf(strOut, ARRAYSIZE(strOut), data,
        height,
        leftCorner, leftCorner, leftCorner, leftCorner, leftCorner,
        ActualWidth() - (leftCorner + rightCorner),
        rightCorner, rightCorner, rightCorner, rightCorner,
        height - (4.0f + rightCorner));

    const auto geometry = winrt::XamlReader::Load(strOut).try_as<winrt::Geometry>();

    templateSettings->TabGeometry(geometry);
}

void TabViewItem::OnLoaded(const winrt::IInspectable& sender, const winrt::RoutedEventArgs& args)
{
    if (const auto tabView = GetParentTabView())
    {
        const auto internalTabView = winrt::get_self<TabView>(tabView);
        const auto index = internalTabView->IndexFromContainer(*this);
        internalTabView->SetTabSeparatorOpacity(index);
    }
}

void TabViewItem::OnSizeChanged(const winrt::IInspectable&, const winrt::SizeChangedEventArgs& args)
{
    m_dispatcherHelper.RunAsync([strongThis = get_strong()]()
    {
        strongThis->UpdateTabGeometry();
    });
}

void TabViewItem::OnIsSelectedPropertyChanged(const winrt::DependencyObject& sender, const winrt::DependencyProperty& args)
{
    
    if (const auto peer = winrt::FrameworkElementAutomationPeer::CreatePeerForElement(*this))
    {
        peer.RaiseAutomationEvent(winrt::AutomationEvents::SelectionItemPatternOnElementSelected);
    }

    if (IsSelected())
    {
        SetValue(winrt::Canvas::ZIndexProperty(), box_value(20));
        StartBringTabIntoView();
    }
    else
    {
        SetValue(winrt::Canvas::ZIndexProperty(), box_value(0));
    }

    if (!WinAppSdk::Containment::IsChangeEnabled<WINAPPSDK_CHANGEID_46770740>())
    {
        UpdateShadow();
    }
    UpdateWidthModeVisualState();

    UpdateCloseButton();
    UpdateForeground();
}

void TabViewItem::OnForegroundPropertyChanged(const winrt::DependencyObject&, const winrt::DependencyProperty&)
{
    UpdateForeground();
}

void TabViewItem::UpdateForeground()
{
    // We only need to set the foreground state when the TabViewItem is in rest state and not selected.
    if (!IsSelected() && !m_isPointerOver)
    {
        // If Foreground is set, then change icon and header foreground to match.
        winrt::VisualStateManager::GoToState(
            *this,
            ReadLocalValue(winrt::Control::ForegroundProperty()) == winrt::DependencyProperty::UnsetValue() ? s_foregroundSetStateName : s_foregroundNotSetStateName,
            false /*useTransitions*/);
    }
}

void TabViewItem::UpdateShadow()
{
    if (!WinAppSdk::Containment::IsChangeEnabled<WINAPPSDK_CHANGEID_46770740>())
    {
        if (IsSelected())
        {
            if (m_isCheckingforDrag && m_isBeingDragged)
            {
                if (const auto tabDragVisualContainer = m_tabDragVisualContainer.get())
                {
                    tabDragVisualContainer.Shadow(m_shadow.as<winrt::ThemeShadow>());

                    // Disable shadow on Tab Strip.
                    Shadow(nullptr);
                }
            }
            else if (!m_isBeingDragged)
            {
                Shadow(m_shadow.as<winrt::ThemeShadow>());
            }
        }
        else
        {
            Shadow(nullptr);
        }
    }
}


void TabViewItem::UpdateSelectedBackgroundPathTranslateTransform()
{
    if (const auto selectedBackgroundPath = m_selectedBackgroundPath.get())
    {
        const auto selectedBackgroundPathActualOffset = selectedBackgroundPath.ActualOffset();
        const auto roundedSelectedBackgroundPathActualOffsetY = std::round(selectedBackgroundPathActualOffset.y);

        if (roundedSelectedBackgroundPathActualOffsetY > selectedBackgroundPathActualOffset.y)
        {
            // Move the SelectedBackgroundPath element down by a fraction of a pixel to avoid a faint gap line
            // between the selected TabViewItem and its content.
            winrt::TranslateTransform translateTransform;

            translateTransform.Y(roundedSelectedBackgroundPathActualOffsetY - selectedBackgroundPathActualOffset.y);

            selectedBackgroundPath.RenderTransform(translateTransform);
        }
        else if (selectedBackgroundPath.RenderTransform())
        {
            // Reset any TranslateTransform that may have been set above.
            selectedBackgroundPath.RenderTransform(nullptr);
        }
    }
}


void TabViewItem::OnTabDragStarting(const winrt::IInspectable& sender, const winrt::TabViewTabDragStartingEventArgs& args)
{
    m_isBeingDragged = true;
}

void TabViewItem::OnTabDragCompleted(const winrt::IInspectable& sender, const winrt::TabViewTabDragCompletedEventArgs& args)
{
    m_isBeingDragged = false;

    StopCheckingForDrag(m_dragPointerId);
    UpdateDragDropVisualState(false);
    UpdateForeground();
}

winrt::AutomationPeer TabViewItem::OnCreateAutomationPeer()
{
    return winrt::make<TabViewItemAutomationPeer>(*this);
}

void TabViewItem::OnCloseButtonOverlayModeChanged(winrt::TabViewCloseButtonOverlayMode const& mode)
{
    m_closeButtonOverlayMode = mode;
    UpdateCloseButton();
}

winrt::TabView TabViewItem::GetParentTabView()
{
    return m_parentTabView.get();
}

void TabViewItem::SetParentTabView(winrt::TabView const& tabView)
{
    m_parentTabView = winrt::make_weak(tabView);
}

void TabViewItem::OnTabViewWidthModeChanged(winrt::TabViewWidthMode const& mode)
{
    m_tabViewWidthMode = mode;
    UpdateWidthModeVisualState();
}

void TabViewItem::UpdateCloseButton()
{
    if (!IsClosable())
    {
        winrt::VisualStateManager::GoToState(*this, s_closeButtonCollapsedStateName, false);
    }
    else
    {
        switch (m_closeButtonOverlayMode)
        {
        case winrt::TabViewCloseButtonOverlayMode::OnPointerOver:
        {
            // If we only want to show the button on hover, we also show it when we are selected, otherwise hide it
            if (IsSelected() || m_isPointerOver)
            {
                winrt::VisualStateManager::GoToState(*this, s_closeButtonVisibleStateName, false);
            }
            else
            {
                winrt::VisualStateManager::GoToState(*this, s_closeButtonCollapsedStateName, false);
            }
            break;
        }
        default:
        {
            // Default, use "Auto"
            winrt::VisualStateManager::GoToState(*this, s_closeButtonVisibleStateName, false);
            break;
        }
        }
    }
}

void TabViewItem::UpdateWidthModeVisualState()
{
    // Handling compact/non compact width mode
    if (!IsSelected() && m_tabViewWidthMode == winrt::TabViewWidthMode::Compact)
    {
        winrt::VisualStateManager::GoToState(*this, s_compactStateName, false);
    }
    else
    {
        winrt::VisualStateManager::GoToState(*this, s_standardWidthStateName, false);
    }
}

void TabViewItem::UpdateDragDropVisualState(bool isVisible)
{
    if (isVisible)
    {
        winrt::VisualStateManager::GoToState(*this, s_dragDropVisualVisibleStateName, false);     
    }
    else
    {
        winrt::VisualStateManager::GoToState(*this, s_dragDropVisualNotVisibleStateName, false);
    }

    UpdateShadow();
}

void TabViewItem::RequestClose()
{
    if (auto tabView = SharedHelpers::GetAncestorOfType<winrt::TabView>(winrt::VisualTreeHelper::GetParent(*this)))
    {
        if (auto internalTabView = winrt::get_self<TabView>(tabView))
        {
            internalTabView->RequestCloseTab(*this, false);
        }
    }
}

void TabViewItem::RaiseRequestClose(TabViewTabCloseRequestedEventArgs const& args)
{
    // This should only be called from TabView, to ensure that both this event and the TabView TabRequestedClose event are raised
    m_closeRequestedEventSource(*this, args);
}

void TabViewItem::OnCloseButtonClick(const winrt::IInspectable&, const winrt::RoutedEventArgs&)
{
    RequestClose();
}

void TabViewItem::OnIsClosablePropertyChanged(const winrt::DependencyPropertyChangedEventArgs&)
{
    UpdateCloseButton();
}

void TabViewItem::OnHeaderPropertyChanged(const winrt::DependencyPropertyChangedEventArgs& args)
{
    OnHeaderChanged();
}

void TabViewItem::OnHeaderChanged()
{
    if (const auto headerContentPresenter = m_headerContentPresenter.get())
    {
        headerContentPresenter.Content(Header());
    }

    if (m_firstTimeSettingToolTip)
    {
        m_firstTimeSettingToolTip = false;

        if (!winrt::ToolTipService::GetToolTip(*this))
        {
            // App author has not specified a tooltip; use our own
            m_toolTip.set([this]() {
                auto toolTip = winrt::ToolTip();
                toolTip.Placement(winrt::Controls::Primitives::PlacementMode::Mouse);
                winrt::ToolTipService::SetToolTip(*this, toolTip);
                return toolTip;
                }());
        }
    }

    if (auto&& toolTip = m_toolTip.get())
    {
        // Update tooltip text to new header text
        auto headerContent = Header();
        auto potentialString = headerContent.try_as<winrt::IPropertyValue>();

        // Only show tooltip if header is a non-empty string.
        if (potentialString && potentialString.Type() == winrt::PropertyType::String && !potentialString.GetString().empty())
        {
            toolTip.Content(headerContent);
            toolTip.IsEnabled(true);
        }
        else
        {
            toolTip.Content(nullptr);
            toolTip.IsEnabled(false);
        }
    }
}

void TabViewItem::OnPointerPressed(winrt::PointerRoutedEventArgs const& args)
{ 
    this->IsSelected(true);

    const auto pointer = args.Pointer();
    const auto pointerDeviceType = pointer.PointerDeviceType();
    const auto pointerPoint = args.GetCurrentPoint(*this);

    if (pointerDeviceType == winrt::PointerDeviceType::Mouse || pointerDeviceType == winrt::PointerDeviceType::Pen)
    {
        if (pointerPoint.Properties().IsLeftButtonPressed())
        {
            m_lastPointerPressedPosition = pointerPoint.Position();

            BeginCheckingForDrag(pointer.PointerId());

            bool ctrlDown = (args.KeyModifiers() & winrt::VirtualKeyModifiers::Control) == winrt::VirtualKeyModifiers::Control;

            if (ctrlDown)
            {
                // Return here so the base class will not pick it up, but let it remain unhandled so someone else could handle it.
                return;
            }
        }
    }
    else if (pointerDeviceType == winrt::PointerDeviceType::Touch)
    {
        m_lastPointerPressedPosition = pointerPoint.Position();

        BeginCheckingForDrag(pointer.PointerId());
    }

    __super::OnPointerPressed(args);

    if (args.GetCurrentPoint(nullptr).Properties().PointerUpdateKind() == winrt::PointerUpdateKind::MiddleButtonPressed)
    {
        if (CapturePointer(pointer))
        {
            m_hasPointerCapture = true;
            m_isMiddlePointerButtonPressed = true;
        }
    }
}

void TabViewItem::OnPointerMoved(winrt::PointerRoutedEventArgs const& args)
{
    __super::OnPointerMoved(args);

    if (ShouldStartDrag(args))
    {
        UpdateDragDropVisualState(true);
    }
}

void TabViewItem::OnPointerReleased(winrt::PointerRoutedEventArgs const& args)
{
    __super::OnPointerReleased(args);

    const auto pointer = args.Pointer();

    StopCheckingForDrag(pointer.PointerId());
    UpdateDragDropVisualState(false);

    if (m_hasPointerCapture)
    {
        if (args.GetCurrentPoint(nullptr).Properties().PointerUpdateKind() == winrt::PointerUpdateKind::MiddleButtonReleased)
        {
            const bool wasPressed = m_isMiddlePointerButtonPressed;
            m_isMiddlePointerButtonPressed = false;
            ReleasePointerCapture(pointer);

            if (wasPressed)
            {
                if (IsClosable())
                {
                    RequestClose();
                }
            }
        }
    }
}

bool TabViewItem::IsOutsideDragRectangle(winrt::Point const& testPoint, winrt::Point const& dragRectangleCenter)
{
    const double dx = abs(testPoint.X - dragRectangleCenter.X);
    const double dy = abs(testPoint.Y - dragRectangleCenter.Y);

    double maxDx = GetSystemMetrics(SM_CXDRAG);
    double maxDy = GetSystemMetrics(SM_CYDRAG);

    maxDx *= c_tabViewItemMouseDragThresholdMultiplier;
    maxDy *= c_tabViewItemMouseDragThresholdMultiplier;

    return (dx > maxDx || dy > maxDy);
}

bool TabViewItem::ShouldStartDrag(winrt::PointerRoutedEventArgs const& args)
{
    return
        m_isCheckingforDrag &&
        IsOutsideDragRectangle(args.GetCurrentPoint(*this).Position(), m_lastPointerPressedPosition) &&
        m_dragPointerId == args.Pointer().PointerId();
}

void TabViewItem::BeginCheckingForDrag(uint32_t const& pointerId)
{
    m_dragPointerId = pointerId;
    m_isCheckingforDrag = true;
}

void TabViewItem::StopCheckingForDrag(uint32_t const& pointerId)
{
    if (m_isCheckingforDrag && m_dragPointerId == pointerId)
    {
        m_dragPointerId = 0;
        m_isCheckingforDrag = false;
    }
}

void TabViewItem::HideLeftAdjacentTabSeparator()
{
    if (const auto tabView = GetParentTabView())
    {
        const auto internalTabView = winrt::get_self<TabView>(tabView);
        const auto index = internalTabView->IndexFromContainer(*this);
        internalTabView->SetTabSeparatorOpacity(index - 1, 0);
    }
}

void TabViewItem::RestoreLeftAdjacentTabSeparatorVisibility()
{
    if (const auto tabView = GetParentTabView())
    {
        const auto internalTabView = winrt::get_self<TabView>(tabView);
        const auto index = internalTabView->IndexFromContainer(*this);
        internalTabView->SetTabSeparatorOpacity(index - 1);
    }
}

void TabViewItem::OnPointerEntered(winrt::PointerRoutedEventArgs const& args)
{
    __super::OnPointerEntered(args);

    m_isPointerOver = true;

    if (m_hasPointerCapture)
    {
        m_isMiddlePointerButtonPressed = true;
    }

    UpdateCloseButton();
    HideLeftAdjacentTabSeparator();
}

void TabViewItem::OnPointerExited(winrt::PointerRoutedEventArgs const& args)
{
    __super::OnPointerExited(args);

    m_isPointerOver = false;
    m_isMiddlePointerButtonPressed = false;

    UpdateCloseButton();
    UpdateForeground();
    RestoreLeftAdjacentTabSeparatorVisibility();
}

void TabViewItem::OnPointerCanceled(winrt::PointerRoutedEventArgs const& args)
{
    __super::OnPointerCanceled(args);

    const auto pointer = args.Pointer();

    StopCheckingForDrag(pointer.PointerId());

    if (m_hasPointerCapture)
    {
        ReleasePointerCapture(pointer);
        m_isMiddlePointerButtonPressed = false;
    }

    RestoreLeftAdjacentTabSeparatorVisibility();
}

void TabViewItem::OnPointerCaptureLost(winrt::PointerRoutedEventArgs const& args)
{
    __super::OnPointerCaptureLost(args);

    m_hasPointerCapture = false;
    m_isMiddlePointerButtonPressed = false;
    RestoreLeftAdjacentTabSeparatorVisibility();
}

// Note that the ItemsView will handle the left and right arrow keys if we don't do so before it does,
// so this needs to be handled below the items view. That's why we can't put this in TabView's OnKeyDown.
void TabViewItem::OnKeyDown(winrt::KeyRoutedEventArgs const& args)
{
    if (!args.Handled() && (args.Key() == winrt::VirtualKey::Left || args.Key() == winrt::VirtualKey::Right))
    {
        // Alt+Shift+Arrow reorders tabs, so we don't want to handle that combination.
        // ListView also handles Alt+Arrow  (no Shift) by just doing regular XY focus,
        // same as how it handles Arrow without any modifier keys, so in that case
        // we do want to handle things so we get the improved keyboarding experience.
        auto isAltDown = (winrt::InputKeyboardSource::GetKeyStateForCurrentThread(winrt::VirtualKey::Menu) & winrt::CoreVirtualKeyStates::Down) == winrt::CoreVirtualKeyStates::Down;
        auto isShiftDown = (winrt::InputKeyboardSource::GetKeyStateForCurrentThread(winrt::VirtualKey::Shift) & winrt::CoreVirtualKeyStates::Down) == winrt::CoreVirtualKeyStates::Down;

        if (!isAltDown || !isShiftDown)
        {
            const bool moveForward =
                (FlowDirection() == winrt::FlowDirection::LeftToRight && args.Key() == winrt::VirtualKey::Right) ||
                (FlowDirection() == winrt::FlowDirection::RightToLeft && args.Key() == winrt::VirtualKey::Left);

            args.Handled(winrt::get_self<TabView>(GetParentTabView())->MoveFocus(moveForward));
        }
    }

    if (!args.Handled())
    {
        __super::OnKeyDown(args);
    }
}

void TabViewItem::OnIconSourcePropertyChanged(const winrt::DependencyPropertyChangedEventArgs& args)
{
    OnIconSourceChanged();
}

void TabViewItem::OnIconSourceChanged()
{
    auto const templateSettings = winrt::get_self<TabViewItemTemplateSettings>(TabViewTemplateSettings());
    if (auto const source = IconSource())
    {
        templateSettings->IconElement(SharedHelpers::MakeIconElementFrom(source));
        winrt::VisualStateManager::GoToState(*this, s_iconStateName, false);
    }
    else
    {
        templateSettings->IconElement(nullptr);
        winrt::VisualStateManager::GoToState(*this, s_noIconStateName, false);
    }
}

void TabViewItem::StartBringTabIntoView()
{
    // we need to set the TargetRect to be slightly wider than the TabViewItem size in order to avoid cutting off the end of the Tab
    winrt::BringIntoViewOptions options;
    options.TargetRect(winrt::Rect{ 0, 0, DesiredSize().Width + c_targetRectWidthIncrement, DesiredSize().Height});
    StartBringIntoView(options);
}
