// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#include "pch.h"
#include "common.h"
#include "TabView.h"
#include "TabViewAutomationPeer.h"
#include "DoubleUtil.h"
#include "RuntimeProfiler.h"
#include "ResourceAccessor.h"
#include "SharedHelpers.h"
#include <Vector.h>
#include "InspectingDataSource.h"
#include "TabViewItem.h"

static constexpr double c_tabMinimumWidth = 48.0;
static constexpr double c_tabMaximumWidth = 200.0;

static constexpr wstring_view c_tabViewItemMinWidthName{ L"TabViewItemMinWidth"sv };
static constexpr wstring_view c_tabViewItemMaxWidthName{ L"TabViewItemMaxWidth"sv };

// TODO: what is the right number and should this be customizable?
static constexpr double c_scrollAmount = 50.0;

TabView::TabView()
{
    __RP_Marker_ClassById(RuntimeProfiler::ProfId_TabView);

    auto items = winrt::make<Vector<winrt::IInspectable>>();
    SetValue(s_ItemsProperty, items);

    SetDefaultStyleKey(this);

    Loaded({ this, &TabView::OnLoaded });
    SizeChanged({ this, &TabView::OnSizeChanged });

    // KeyboardAccelerator is only available on RS3+
    if (SharedHelpers::IsRS3OrHigher())
    {
        winrt::KeyboardAccelerator ctrlf4Accel;
        ctrlf4Accel.Key(winrt::VirtualKey::F4);
        ctrlf4Accel.Modifiers(winrt::VirtualKeyModifiers::Control);
        ctrlf4Accel.Invoked({ this, &TabView::OnCtrlF4Invoked });
        ctrlf4Accel.ScopeOwner(*this);
        KeyboardAccelerators().Append(ctrlf4Accel);
    }
    m_selectionModel.SingleSelect(true);

    // Ctrl+Tab as a KeyboardAccelerator only works on 19H1+
    if (SharedHelpers::Is19H1OrHigher())
    {
        winrt::KeyboardAccelerator ctrlTabAccel;
        ctrlTabAccel.Key(winrt::VirtualKey::Tab);
        ctrlTabAccel.Modifiers(winrt::VirtualKeyModifiers::Control);
        ctrlTabAccel.Invoked({ this, &TabView::OnCtrlTabInvoked });
        ctrlTabAccel.ScopeOwner(*this);
        KeyboardAccelerators().Append(ctrlTabAccel);

        winrt::KeyboardAccelerator ctrlShiftTabAccel;
        ctrlShiftTabAccel.Key(winrt::VirtualKey::Tab);
        ctrlShiftTabAccel.Modifiers(winrt::VirtualKeyModifiers::Control | winrt::VirtualKeyModifiers::Shift);
        ctrlShiftTabAccel.Invoked({ this, &TabView::OnCtrlShiftTabInvoked });
        ctrlShiftTabAccel.ScopeOwner(*this);
        KeyboardAccelerators().Append(ctrlShiftTabAccel);
    }
}

void TabView::OnApplyTemplate()
{
    winrt::IControlProtected controlProtected{ *this };

    m_tabContentPresenter.set(GetTemplateChildT<winrt::ContentPresenter>(L"TabContentPresenter", controlProtected));
    m_rightContentPresenter.set(GetTemplateChildT<winrt::ContentPresenter>(L"RightContentPresenter", controlProtected));

    m_leftContentColumn.set(GetTemplateChildT<winrt::ColumnDefinition>(L"LeftContentColumn", controlProtected));
    m_tabColumn.set(GetTemplateChildT<winrt::ColumnDefinition>(L"TabColumn", controlProtected));
    m_addButtonColumn.set(GetTemplateChildT<winrt::ColumnDefinition>(L"AddButtonColumn", controlProtected));
    m_rightContentColumn.set(GetTemplateChildT<winrt::ColumnDefinition>(L"RightContentColumn", controlProtected));

    m_tabContainerGrid.set(GetTemplateChildT<winrt::Grid>(L"TabContainerGrid", controlProtected));
    m_scrollViewer.set(GetTemplateChildT<winrt::FxScrollViewer>(L"ScrollViewer", controlProtected));

    m_rootGrid.set(GetTemplateChildT<winrt::Grid>(L"RepeaterGrid", controlProtected));
    m_gridDragOverRevoker = m_rootGrid.get().DragOver(winrt::auto_revoke, { this, &TabView::OnGridDragOver });
    m_gridDropRevoker = m_rootGrid.get().Drop(winrt::auto_revoke, { this, &TabView::OnGridDrop });

    m_scrollViewerLoadedRevoker = m_scrollViewer.get().Loaded(winrt::auto_revoke, { this, &TabView::OnScrollViewerLoaded });

    m_itemsRepeater.set([this, controlProtected]() {
        auto repeater = GetTemplateChildT<winrt::ItemsRepeater>(L"TabItemsRepeater", controlProtected);
        if (repeater)
        {
            m_repeaterLoadedRevoker = repeater.Loaded(winrt::auto_revoke, { this, &TabView::OnRepeaterLoaded });
            m_repeaterElementPreparedRevoker = repeater.ElementPrepared(winrt::auto_revoke, { this, &TabView::OnRepeaterElementPrepared });
            m_repeaterElementIndexChangedRevoker = repeater.ElementIndexChanged(winrt::auto_revoke, { this, &TabView::OnRepeaterElementIndexChanged });
			m_listViewGettingFocusRevoker = repeater.GettingFocus(winrt::auto_revoke, { this, &TabView::OnListViewGettingFocus });
        }
        return repeater;
        }());

    m_selectionChangedRevoker = m_selectionModel.SelectionChanged(winrt::auto_revoke, { this, &TabView::OnSelectionChanged });

    m_addButton.set([this, controlProtected]() {
        auto addButton = GetTemplateChildT<winrt::Button>(L"AddButton", controlProtected);
        if (addButton)
        {
            // Do localization for the add button
            if (winrt::AutomationProperties::GetName(addButton).empty())
            {
                auto addButtonName = ResourceAccessor::GetLocalizedStringResource(SR_TabViewAddButtonName);
                winrt::AutomationProperties::SetName(addButton, addButtonName);
            }

            auto toolTip = winrt::ToolTipService::GetToolTip(addButton);
            if (!toolTip)
            {
                winrt::ToolTip tooltip = winrt::ToolTip();
                tooltip.Content(box_value(ResourceAccessor::GetLocalizedStringResource(SR_TabViewAddButtonTooltip)));
                winrt::ToolTipService::SetToolTip(addButton, tooltip);
            }

            m_addButtonClickRevoker = addButton.Click(winrt::auto_revoke, { this, &TabView::OnAddButtonClick });
        }
        return addButton;
        }());

    UpdateItemsSource();
}

void TabView::OnListViewGettingFocus(const winrt::IInspectable& sender, const winrt::GettingFocusEventArgs& args)
{
    // TabViewItems overlap each other by one pixel in order to get the desired visuals for the separator.
    // This causes problems with 2d focus navigation. Because the items overlap, pressing Down or Up from a
    // TabViewItem navigates to the overlapping item which is not desired.
    //
    // To resolve this issue, we detect the case where Up or Down focus navigation moves from one TabViewItem
    // to another.
    // How we handle it, depends on the input device.
    // For GamePad, we want to move focus to something in the direction of movement (other than the overlapping item)
    // For Keyboard, we cancel the focus movement.

    auto direction = args.Direction();
    if (direction == winrt::FocusNavigationDirection::Up || direction == winrt::FocusNavigationDirection::Down)
    {
        auto oldItem = args.OldFocusedElement().try_as<winrt::TabViewItem>();
        auto newItem = args.NewFocusedElement().try_as<winrt::TabViewItem>();
        if (oldItem && newItem)
        {
            if (auto repeater = m_itemsRepeater.get())
            {
                bool oldItemIsFromThisTabView = repeater.GetElementIndex(oldItem) != -1;
                bool newItemIsFromThisTabView = repeater.GetElementIndex(newItem) != -1;
                if (oldItemIsFromThisTabView && newItemIsFromThisTabView)
                {
                    auto inputDevice = args.InputDevice();
                    if (inputDevice == winrt::FocusInputDeviceKind::GameController)
                    {
                        auto listViewBoundsLocal = winrt::Rect{ 0, 0, static_cast<float>(repeater.ActualWidth()), static_cast<float>(repeater.ActualHeight()) };
                        auto listViewBounds = repeater.TransformToVisual(nullptr).TransformBounds(listViewBoundsLocal);
                        winrt::FindNextElementOptions options;
                        options.ExclusionRect(listViewBounds);
                        auto next = winrt::FocusManager::FindNextElement(direction, options);
                        if(auto args2 = args.try_as<winrt::IGettingFocusEventArgs2>())
                        {
                            args2.TrySetNewFocusedElement(next);
                        }
                        else
                        {
                            // Without TrySetNewFocusedElement, we cannot set focus while it is changing.
                            m_dispatcherHelper.RunAsync([next]()
                            {
                                SetFocus(next, winrt::FocusState::Programmatic);
                            });
                        }
                        args.Handled(true);
                    }
                    else
                    {
                        args.Cancel(true);
                        args.Handled(true);
                    }
                }
            }
        }
    }
}

void TabView::UpdateItemsSource()
{
    if (auto repeater = m_itemsRepeater.get())
    {
        if (ItemsSource())
        {
            repeater.ItemsSource(ItemsSource());
           
        }
        else
        {
            repeater.ItemsSource(Items());
        }

        m_selectionModel.Source(repeater.ItemsSourceView());
        m_collectionChangedRevoker = repeater.ItemsSourceView().CollectionChanged(winrt::auto_revoke, { this, &TabView::OnItemsChanged });               
    }
}

void TabView::OnItemsPropertyChanged(const winrt::DependencyPropertyChangedEventArgs& args)
{
    UpdateItemsSource();
}

void TabView::OnItemsSourcePropertyChanged(const winrt::DependencyPropertyChangedEventArgs& args)
{
    UpdateItemsSource();
}

void TabView::OnSelectedIndexPropertyChanged(const winrt::DependencyPropertyChangedEventArgs& args)
{
    UpdateSelectedIndex();
}

void TabView::OnSelectedItemPropertyChanged(const winrt::DependencyPropertyChangedEventArgs& args)
{
    UpdateSelectedItem();
}

void TabView::OnTabWidthModePropertyChanged(const winrt::DependencyPropertyChangedEventArgs&)
{
    UpdateTabWidths();
}

void TabView::OnAddButtonClick(const winrt::IInspectable&, const winrt::RoutedEventArgs& args)
{
    m_addButtonClickEventSource(*this, args);
}

winrt::AutomationPeer TabView::OnCreateAutomationPeer()
{
    return winrt::make<TabViewAutomationPeer>(*this);
}

void TabView::OnLoaded(const winrt::IInspectable&, const winrt::RoutedEventArgs&)
{
    UpdateTabContent();
}

void TabView::OnRepeaterLoaded(const winrt::IInspectable&, const winrt::RoutedEventArgs& args)
{
    if (ReadLocalValue(s_SelectedIndexProperty) != winrt::DependencyProperty::UnsetValue())
    {
        UpdateSelectedIndex();
    }
    if (ReadLocalValue(s_SelectedItemProperty) != winrt::DependencyProperty::UnsetValue())
    {
        UpdateSelectedItem();
    }

    if (auto repeater = m_itemsRepeater.get())
    {
        if (auto selectedIndex = m_selectionModel.SelectedIndex())
        {
            SelectedIndex(selectedIndex.GetAt(0));
            SelectedItem(m_selectionModel.SelectedItem());
        }
    }
}

void TabView::OnScrollViewerLoaded(const winrt::IInspectable&, const winrt::RoutedEventArgs& args)
{
    if (auto && scrollViewer = m_scrollViewer.get())
    {
        m_scrollDecreaseButton.set([this, scrollViewer]() {
            auto decreaseButton = SharedHelpers::FindInVisualTreeByName(scrollViewer, L"ScrollDecreaseButton").as<winrt::RepeatButton>();
            m_scrollDecreaseClickRevoker = decreaseButton.Click(winrt::auto_revoke, { this, &TabView::OnScrollDecreaseClick });
            return decreaseButton;
            }());

        m_scrollIncreaseButton.set([this, scrollViewer]() {
            auto increaseButton = SharedHelpers::FindInVisualTreeByName(scrollViewer, L"ScrollIncreaseButton").as<winrt::RepeatButton>();
            m_scrollIncreaseClickRevoker = increaseButton.Click(winrt::auto_revoke, { this, &TabView::OnScrollIncreaseClick });
            return increaseButton;
            }());
    }

    UpdateTabWidths();
}

void TabView::OnSizeChanged(const winrt::IInspectable&, const winrt::SizeChangedEventArgs&)
{
    UpdateTabWidths();
}

void TabView::OnItemsChanged(const winrt::IInspectable& dataSource, const winrt::NotifyCollectionChangedEventArgs& args)
{
    int numItems = static_cast<int>(Items().Size());
    if (args.Action() == winrt::NotifyCollectionChangedAction::Remove && numItems > 0)
    {
        if (SelectedIndex() == static_cast<int32_t>(args.OldStartingIndex()))
        {
            // Find the closest tab to select instead.
            int startIndex = static_cast<int>(args.OldStartingIndex());
            if (startIndex >= numItems)
            {
                startIndex = numItems - 1;
            }
            int index = startIndex;

            do
            {
                auto nextItem = ContainerFromIndex(index).as<winrt::TabViewItem>();

                if (nextItem && nextItem.IsEnabled() && nextItem.Visibility() == winrt::Visibility::Visible)
                {
                    //SelectedItem(Items().GetAt(index));
                    m_selectionModel.Select(index);
                    break;
                }

                // try the next item
                index++;
                if (index >= numItems)
                {
                    index = 0;
                }
            } while (index != startIndex);
        }
    }

    UpdateTabWidths();
}

void TabView::OnSelectionChanged(const winrt::SelectionModel& sender, const winrt::SelectionModelSelectionChangedEventArgs& args)
{
    SelectedItem(sender.SelectedItem());
    if (sender.SelectedIndex())
    {
        SelectedIndex(sender.SelectedIndex().GetAt(0));
    }
    UpdateTabContent();
    // TODO: Make a selection changed event args instance.
    m_selectionChangedEventSource(*this, nullptr); 
}

void TabView::StartDragAnimations(int dragItemIndex, double dragElementWidth)
{
    auto compositor = winrt::Window::Current().Compositor();
    auto props = winrt::ElementCompositionPreview::GetPointerPositionPropertySet(m_itemsRepeater.get());
    // yfactor * smoothStepFunction
    constexpr auto expr = L"(Clamp((200 + (pointer.Position.Y - top)) / 200, 0, 1)) *  (halfDraggingElementWidth * (2 / Pi) * ATan((Pi / 2) * -(pointer.Position.X - center)) * (pointer.Position.X < center ? canInsertBefore : 1) * (pointer.Position.X > center ? canInsertAfter : 1))";

    if (!m_dragAnimation)
    {
        m_dragAnimation = compositor.CreateExpressionAnimation();

        m_dragAnimation.SetReferenceParameter(L"pointer", props);
        m_dragAnimation.Target(L"Translation.X");
    }

    int startIndex = 0;
    int count = m_itemsRepeater.get().ItemsSourceView().Count();
    for (int i = 0; i < count; i++)
    {
        if (auto element = m_itemsRepeater.get().TryGetElement(i))
        {
            m_dragAnimation.Expression(expr);

            winrt::ElementCompositionPreview::SetIsTranslationEnabled(element, true);
            auto bounds = winrt::LayoutInformation::GetLayoutSlot(element.as<winrt::FrameworkElement>());
            bool canInsertBefore = i >= startIndex;
            bool canInsertAfter = i <= startIndex + count;

            m_dragAnimation.SetScalarParameter(L"canInsertBefore", canInsertBefore ? 1.0f : -1.0f);
            m_dragAnimation.SetScalarParameter(L"canInsertAfter", canInsertAfter ? 1.0f : -1.0f);
            m_dragAnimation.SetScalarParameter(L"top", (float)bounds.Y);
            m_dragAnimation.SetScalarParameter(L"center", (float)(bounds.X + 0.5 * bounds.Width));
            m_dragAnimation.SetScalarParameter(L"halfDraggingElementWidth", (float)(dragElementWidth * 0.4));
            element.StartAnimation(m_dragAnimation);
            element.as<winrt::FrameworkElement>().Tag(m_dragAnimation);
        }
    }
}

void TabView::StopDragAnimations()
{
    int count = m_itemsRepeater.get().ItemsSourceView().Count();
    for (int i = 0; i < count; i++)
    {
        if (auto element = m_itemsRepeater.get().TryGetElement(i))
        {
            if (auto animation = element.as<winrt::FrameworkElement>().Tag().try_as<winrt::ExpressionAnimation>())
            {
                animation.Expression(L"0");
                element.StartAnimation(animation);
                // Stop animation does not seem to really stop the animation :/
                //element.StopAnimation(animation);
            }
        }
    }
}

void TabView::OnItemDragStarting(const winrt::TabViewItem& item, const winrt::DragStartingEventArgs& args)
{
    // TODO: build args 
    m_tabStripDragItemsStartingEventSource(*this, nullptr);
    CloneDragVisual(item, args);
}

winrt::IAsyncAction TabView::CloneDragVisual(const winrt::TabViewItem& item, const winrt::DragStartingEventArgs& args)
{
    m_draggedItemIndex = m_itemsRepeater.get().GetElementIndex(item);

    auto deferral = args.GetDeferral();
    m_draggedItem = Items().GetAt(m_draggedItemIndex);

    StartDragAnimations(m_draggedItemIndex, item.ActualWidth());

    args.Data().RequestedOperation(winrt::DataPackageOperation::Move);
    args.Data().SetText(L"ItemMoving..");
    args.Data().Properties().Title(L"Hello world");

    m_dataPackageOperationCompletedRevoker = args.Data().OperationCompleted(winrt::auto_revoke, { this, &TabView::OnDataPackageOperationCompleted });

    // take a snapshot of the item because we are removing it from the source.
    auto dragUI = args.DragUI();
    auto renderTargetBitmap = winrt::RenderTargetBitmap();
    co_await renderTargetBitmap.RenderAsync(item);
    auto buffer = co_await renderTargetBitmap.GetPixelsAsync();
    auto bitmap = winrt::SoftwareBitmap::CreateCopyFromBuffer(buffer,
        winrt::BitmapPixelFormat::Bgra8,
        renderTargetBitmap.PixelWidth(),
        renderTargetBitmap.PixelHeight(),
        winrt::BitmapAlphaMode::Premultiplied);
    dragUI.SetContentFromSoftwareBitmap(bitmap);

    Items().RemoveAt(m_draggedItemIndex);

    deferral.Complete();
}

void TabView::OnGridDragOver(const winrt::IInspectable& sender, const winrt::DragEventArgs& args)
{
    m_tabStripDragOverEventSource(*this, args);

    args.AcceptedOperation(winrt::DataPackageOperation::Move);
    args.DragUIOverride().IsGlyphVisible(true);
    args.DragUIOverride().IsContentVisible(true);
}

void TabView::OnGridDrop(const winrt::IInspectable& sender, const winrt::DragEventArgs& args)
{
    m_tabStripDropEventSource(*this, args);

    StopDragAnimations();
    int index = GetInsertionIndex(args.GetPosition(m_itemsRepeater.get()), 100 /* dropped item width */);

    if (index > 0)
    {
        Items().InsertAt(index, m_draggedItem);
        m_draggedItemIndex = -1;
        m_draggedItem = nullptr;
    }
}

int TabView::GetInsertionIndex(const winrt::Point& position, int droppedElementWidth)
{
    int count = m_itemsRepeater.get().ItemsSourceView().Count();
    auto dropBetweenScale = 0.5;
    for (int i = 0; i < count; i++)
    {
        if (auto element = m_itemsRepeater.get().TryGetElement(i))
        {
            auto bounds = winrt::LayoutInformation::GetLayoutSlot(element.as<winrt::FrameworkElement>());
            if (position.X < (bounds.X + dropBetweenScale * bounds.Width))
            {
                return i;
            }
        }
    }

    return -1;
}

void TabView::OnDataPackageOperationCompleted(const winrt::DataPackage& sender, const winrt::OperationCompletedEventArgs& args)
{
    DispatcherHelper dispatcherHelper(*this);
    dispatcherHelper.RunAsync([this]()
        {
            if (m_draggedItemIndex >= 0)
            {
                // insert it back 
                Items().InsertAt(m_draggedItemIndex, m_draggedItem);
                UpdateLayout();

                const auto item = m_draggedItem;
                auto tab = ContainerFromItem(item).try_as<winrt::TabViewItem>();

                if (!tab)
                {
                    if (auto fe = item.try_as<winrt::FrameworkElement>())
                    {
                        tab = winrt::VisualTreeHelper::GetParent(fe).try_as<winrt::TabViewItem>();
                    }
                }

                if (!tab)
                {
                    // This is a fallback scenario for tabs without a data context
                    auto numItems = static_cast<int>(Items().Size());
                    for (int i = 0; i < numItems; i++)
                    {
                        auto tabItem = ContainerFromIndex(i).try_as<winrt::TabViewItem>();
                        if (tabItem.Content() == item)
                        {
                            tab = tabItem;
                            break;
                        }
                    }
                }

                auto myArgs = winrt::make_self<TabViewTabDraggedOutsideEventArgs>(item, tab);
                m_tabDraggedOutsideEventSource(*this, *myArgs);

              
                m_draggedItemIndex = -1;
                m_draggedItem = nullptr;
            }

            StopDragAnimations();

           
        });
}

void TabView::UpdateTabContent()
{
    if (auto tabContentPresenter = m_tabContentPresenter.get())
    {
        if (!SelectedItem())
        { 
            tabContentPresenter.Content(nullptr);
            tabContentPresenter.ContentTemplate(nullptr);
            tabContentPresenter.ContentTemplateSelector(nullptr);
        }
        else
        {
            if (auto container = ContainerFromItem(SelectedItem()).as<winrt::TabViewItem>())
            {
                // If the focus was in the old tab content, we will lose focus when it is removed from the visual tree.
                // We should move the focus to the new tab content.
                // The new tab content is not available at the time of the LosingFocus event, so we need to
                // move focus later.
                bool shouldMoveFocusToNewTab = false;
                auto revoker = tabContentPresenter.LosingFocus(winrt::auto_revoke, [&shouldMoveFocusToNewTab](const winrt::IInspectable&, const winrt::LosingFocusEventArgs& args)
                {
                    shouldMoveFocusToNewTab = true;
                });

                tabContentPresenter.Content(container.Content());
                tabContentPresenter.ContentTemplate(container.ContentTemplate());
                tabContentPresenter.ContentTemplateSelector(container.ContentTemplateSelector());

                // It is not ideal to call UpdateLayout here, but it is necessary to ensure that the ContentPresenter has expanded its content
                // into the live visual tree.
                tabContentPresenter.UpdateLayout();

                if (shouldMoveFocusToNewTab)
                {
                    auto focusable = winrt::FocusManager::FindFirstFocusableElement(tabContentPresenter);
                    if (!focusable)
                    {
                        // If there is nothing focusable in the new tab, just move focus to the TabView itself.
                        focusable = winrt::FocusManager::FindFirstFocusableElement(*this);
                    }

                    if (focusable)
                    {
                        SetFocus(focusable, winrt::FocusState::Programmatic);
                    }
                }
            }
        }
    }
}

void TabView::CloseTab(winrt::TabViewItem const& container)
{
    if (auto repeater = m_itemsRepeater.get())
    {
        int index = repeater.GetElementIndex(container);
        auto args = winrt::make_self<TabViewTabClosingEventArgs>(repeater.ItemsSourceView().GetAt(index));
        m_tabClosingEventSource(*this, *args);

        if (!args->Cancel())
        {
            Items().RemoveAt(index);
        }
    }
}

void TabView::OnScrollDecreaseClick(const winrt::IInspectable&, const winrt::RoutedEventArgs&)
{
    if (auto scrollViewer = m_scrollViewer.get())
    {
        scrollViewer.ChangeView(std::max(0.0, scrollViewer.HorizontalOffset() - c_scrollAmount), nullptr, nullptr);
    }
}

void TabView::OnScrollIncreaseClick(const winrt::IInspectable&, const winrt::RoutedEventArgs&)
{
    if (auto scrollViewer = m_scrollViewer.get())
    {
        scrollViewer.ChangeView(std::min(scrollViewer.ScrollableWidth(), scrollViewer.HorizontalOffset() + c_scrollAmount), nullptr, nullptr);
    }
}

void TabView::UpdateTabWidths()
{
    double tabWidth = std::numeric_limits<double>::quiet_NaN();

    if (auto tabGrid = m_tabContainerGrid.get())
    {
        // Add up width taken by custom content and + button
        double widthTaken = 0.0;
        if (auto leftContentColumn = m_leftContentColumn.get())
        {
            widthTaken += leftContentColumn.ActualWidth();
        }
        if (auto addButtonColumn = m_addButtonColumn.get())
        {
            widthTaken += addButtonColumn.ActualWidth();
        }
        if (auto && rightContentColumn = m_rightContentColumn.get())
        {
            if (auto rightContentPresenter = m_rightContentPresenter.get())
            {
                winrt::Size rightContentSize = rightContentPresenter.DesiredSize();
                rightContentColumn.MinWidth(rightContentSize.Width);
                widthTaken += rightContentSize.Width;
            }
        }

        if (auto tabColumn = m_tabColumn.get())
        {
            auto availableWidth = ActualWidth() - widthTaken;

            if (TabWidthMode() == winrt::TabViewWidthMode::SizeToContent)
            {
                tabColumn.MaxWidth(availableWidth);
                tabColumn.Width(winrt::GridLengthHelper::FromValueAndType(1.0, winrt::GridUnitType::Auto));
            }
            else if (TabWidthMode() == winrt::TabViewWidthMode::Equal)
            {
                // Tabs should all be the same size, proportional to the amount of space.
                double minTabWidth = unbox_value<double>(SharedHelpers::FindResource(c_tabViewItemMinWidthName, winrt::Application::Current().Resources(), box_value(c_tabMinimumWidth)));
                double maxTabWidth = unbox_value<double>(SharedHelpers::FindResource(c_tabViewItemMaxWidthName, winrt::Application::Current().Resources(), box_value(c_tabMaximumWidth)));

                // Calculate the proportional width of each tab given the width of the ScrollViewer.
                auto padding = Padding();
                double tabWidthForScroller = (availableWidth - (padding.Left + padding.Right)) / (double)(Items().Size());

                tabWidth = std::clamp(tabWidthForScroller, minTabWidth, maxTabWidth);

                // If the min tab width causes the ScrollViewer to scroll, show the increase/decrease buttons.
                auto decreaseButton = m_scrollDecreaseButton.get();
                auto increaseButton = m_scrollIncreaseButton.get();
                if (decreaseButton && increaseButton)
                {
                    if (tabWidthForScroller < tabWidth)
                    {
                        decreaseButton.Visibility(winrt::Visibility::Visible);
                        increaseButton.Visibility(winrt::Visibility::Visible);
                    }
                    else
                    {
                        decreaseButton.Visibility(winrt::Visibility::Collapsed);
                        increaseButton.Visibility(winrt::Visibility::Collapsed);
                    }
                }

                // Size tab column to needed size
                tabColumn.MaxWidth(availableWidth);
                auto requiredWidth = tabWidth * Items().Size();
                if (requiredWidth >= availableWidth)
                {
                    tabColumn.Width(winrt::GridLengthHelper::FromPixels(availableWidth));
                }
                else
                {
                    tabColumn.Width(winrt::GridLengthHelper::FromValueAndType(1.0, winrt::GridUnitType::Auto));
                }
            }
        }
    }

    for (auto item : Items())
    {
        // Set the calculated width on each tab.
        if (auto container = ContainerFromItem(item).as<winrt::TabViewItem>())
        {
            container.Width(tabWidth);
        }
    }
}


void TabView::UpdateSelectedItem()
{
    uint32_t index;
    if (Items().IndexOf(SelectedItem(), index))
    {
        m_selectionModel.Select(index);
    }
}

void TabView::UpdateSelectedIndex()
{
    if (!m_selectionModel.SelectedIndex() ||
        m_selectionModel.SelectedIndex().GetAt(0) != SelectedIndex())
    {
        m_selectionModel.Select(SelectedIndex());
    }
}

winrt::DependencyObject TabView::ContainerFromItem(winrt::IInspectable const& item)
{
    if (auto repeater = m_itemsRepeater.get())
    {
        uint32_t index;
        if (Items().IndexOf(item, index))
        {
            return repeater.TryGetElement(index);
        }
    }
    return nullptr;
}

winrt::DependencyObject TabView::ContainerFromIndex(int index)
{
    if (auto repeater = m_itemsRepeater.get())
    {
        return repeater.TryGetElement(index);
    }
    return nullptr;
}

int TabView::GetItemCount()
{
    if (auto itemssource = ItemsSource())
    {
        return winrt::make<InspectingDataSource>(ItemsSource()).Count();
    }
    else
    {
        return static_cast<int>(Items().Size());
    }
}

bool TabView::SelectNextTab(int increment)
{
    bool handled = false;
    const int itemsSize = GetItemCount();
    if (itemsSize > 1)
    {
        auto index = SelectedIndex();
        index = (index + increment + itemsSize) % itemsSize;
        SelectedIndex(index);
        handled = true;
    }
    return handled;
}

bool TabView::CloseCurrentTab()
{
    bool handled = false;
    if (auto selectedTab = SelectedItem().try_as<winrt::TabViewItem>())
    {
        if (selectedTab.IsCloseable())
        {
            // Close the tab on ctrl + F4
            CloseTab(selectedTab);
            handled = true;
        }
    }

    return handled;
}

void TabView::OnKeyDown(winrt::KeyRoutedEventArgs const& args)
{
    if (auto coreWindow = winrt::CoreWindow::GetForCurrentThread())
    {
        if (args.Key() == winrt::VirtualKey::F4)
        {
            // Handle Ctrl+F4 on RS2 and lower
            // On RS3+, it is handled by a KeyboardAccelerator
            if (!SharedHelpers::IsRS3OrHigher())
            {
                auto isCtrlDown = (coreWindow.GetKeyState(winrt::VirtualKey::Control) & winrt::CoreVirtualKeyStates::Down) == winrt::CoreVirtualKeyStates::Down;
                if (isCtrlDown)
                {
                    args.Handled(CloseCurrentTab());
                }
            }
        }
        else if (args.Key() == winrt::VirtualKey::Tab)
        {
            // Handle Ctrl+Tab/Ctrl+Shift+Tab on RS5 and lower
            // On 19H1+, it is handled by a KeyboardAccelerator
            if (!SharedHelpers::Is19H1OrHigher())
            {
                auto isCtrlDown = (coreWindow.GetKeyState(winrt::VirtualKey::Control) & winrt::CoreVirtualKeyStates::Down) == winrt::CoreVirtualKeyStates::Down;
                auto isShiftDown = (coreWindow.GetKeyState(winrt::VirtualKey::Shift) & winrt::CoreVirtualKeyStates::Down) == winrt::CoreVirtualKeyStates::Down;

                if (isCtrlDown && !isShiftDown)
                {
                    args.Handled(SelectNextTab(1));
                }
                else if (isCtrlDown && isShiftDown)
                {
                    args.Handled(SelectNextTab(-1));
                }
            }
        }
    }
}

void TabView::OnCtrlF4Invoked(const winrt::KeyboardAccelerator& sender, const winrt::KeyboardAcceleratorInvokedEventArgs& args)
{
    args.Handled(CloseCurrentTab());
}

void TabView::OnCtrlTabInvoked(const winrt::KeyboardAccelerator& sender, const winrt::KeyboardAcceleratorInvokedEventArgs& args)
{
    args.Handled(SelectNextTab(1));
}

void TabView::OnCtrlShiftTabInvoked(const winrt::KeyboardAccelerator& sender, const winrt::KeyboardAcceleratorInvokedEventArgs& args)
{
    args.Handled(SelectNextTab(-1));     
}

void TabView::OnRepeaterElementPrepared(const winrt::ItemsRepeater& sender, const winrt::ItemsRepeaterElementPreparedEventArgs& args)
{
    if (auto item = args.Element().try_as<winrt::TabViewItem>())
    {
        auto tabItem = winrt::get_self<TabViewItem>(item);
        tabItem->RepeatedIndex(args.Index());
        tabItem->SelectionModel(m_selectionModel);
    }
}

void TabView::OnRepeaterElementIndexChanged(const winrt::ItemsRepeater& sender, const winrt::ItemsRepeaterElementIndexChangedEventArgs& args)
{
    if (auto item = args.Element().try_as<winrt::TabViewItem>())
    {
        auto tabItem = winrt::get_self<TabViewItem>(item);
        tabItem->RepeatedIndex(args.NewIndex());
    }
}
