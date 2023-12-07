// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#include "pch.h"
#include "common.h"
#include <DoubleUtil.h>
#include "TypeLogging.h"
#include "ItemsView.h"
#include "RuntimeProfiler.h"
#include "ItemsViewTestHooks.h"
#include "ItemsViewItemInvokedEventArgs.h"
#include "ItemsViewSelectionChangedEventArgs.h"
#include "ItemsViewAutomationPeer.h"
#include "NullSelector.h"
#include "SingleSelector.h"
#include "MultipleSelector.h"
#include "ExtendedSelector.h"
#include "ItemContainer.h"
#include "ItemContainerRevokers.h"
#include "ScrollView.h"

// Change to 'true' to turn on debugging outputs in Output window
bool ItemsViewTrace::s_IsDebugOutputEnabled{ false };
bool ItemsViewTrace::s_IsVerboseDebugOutputEnabled{ false };

// Number of CompositionTarget.Rendering event occurrences after bring-into-view completion before resetting m_bringIntoViewElement as the scroll anchoring element.
const uint8_t c_renderingEventsPostBringIntoView = 4;

ItemsView::ItemsView()
{
    ITEMSVIEW_TRACE_INFO(nullptr, TRACE_MSG_METH, METH_NAME, this);

    __RP_Marker_ClassById(RuntimeProfiler::ProfId_ItemsView);

    static const auto s_ItemsViewItemContainerRevokersPropertyInit = []()
    {
        s_ItemsViewItemContainerRevokersProperty =
            InitializeDependencyProperty(
                L"ItemsViewItemContainerRevokers",
                winrt::name_of<winrt::IInspectable>(),
                winrt::name_of<winrt::UIElement>(),
                true /* isAttached */,
                nullptr /* defaultValue */);
        return false;
    }();

    EnsureProperties();
    SetDefaultStyleKey(this);

    m_loadedRevoker = Loaded(winrt::auto_revoke, { this, &ItemsView::OnLoaded });
    m_unloadedRevoker = Unloaded(winrt::auto_revoke, { this, &ItemsView::OnUnloaded });
    m_selectionModelSelectionChangedRevoker = m_selectionModel.SelectionChanged(winrt::auto_revoke, { this, &ItemsView::OnSelectionModelSelectionChanged });
    m_currentElementSelectionModelSelectionChangedRevoker = m_currentElementSelectionModel.SelectionChanged(winrt::auto_revoke, { this, &ItemsView::OnCurrentElementSelectionModelSelectionChanged });

    // m_currentElementSelectionModel tracks the single current element.
    m_currentElementSelectionModel.SingleSelect(true);

    UpdateSelector();
}

ItemsView::~ItemsView()
{
    ITEMSVIEW_TRACE_INFO(nullptr, TRACE_MSG_METH, METH_NAME, this);

    m_loadedRevoker.revoke();
    m_unloadedRevoker.revoke();
    m_selectionModelSelectionChangedRevoker.revoke();
    m_currentElementSelectionModelSelectionChangedRevoker.revoke();

    UnhookCompositionTargetRendering();
    UnhookItemsRepeaterEvents(true /*isForDestructor*/);
    UnhookScrollViewEvents(true /*isForDestructor*/);
}

#pragma region IItemsView

winrt::ScrollView ItemsView::ScrollView() const
{
    return m_scrollView.get().as<winrt::ScrollView>();
}

// Returns the index of the closest realized item to a point specified by the two viewport ratio numbers.
// See GetItemInternal for details.
bool ItemsView::TryGetItemIndex(
    double horizontalViewportRatio, 
    double verticalViewportRatio, 
    int& index)
{
    ITEMSVIEW_TRACE_INFO(*this, TRACE_MSG_METH_DBL_DBL, METH_NAME, this, horizontalViewportRatio, verticalViewportRatio);

    const winrt::IndexBasedLayoutOrientation indexBasedLayoutOrientation = GetLayoutIndexBasedLayoutOrientation();
    const bool isHorizontalDistanceFavored = indexBasedLayoutOrientation == winrt::IndexBasedLayoutOrientation::TopToBottom;
    const bool isVerticalDistanceFavored = indexBasedLayoutOrientation == winrt::IndexBasedLayoutOrientation::LeftToRight;

    index = GetItemInternal(
                horizontalViewportRatio,
                verticalViewportRatio,
                isHorizontalDistanceFavored,
                isVerticalDistanceFavored,
                false /*useKeyboardNavigationReferenceHorizontalOffset*/,
                false /*useKeyboardNavigationReferenceVerticalOffset*/,
                false /*capItemEdgesToViewportRatioEdges*/,
                false /*forFocusableItemsOnly*/);
    return index != -1;    
}

// Invokes UIElement::StartBringIntoView with the provided BringIntoViewOptions instance, if any,
// for the element associated with the given data index.
// If that element is currently virtualized, it is realized and synchronously layed out, before that StartBringIntoView call.
// Note that because of lines 111-112 of ViewportManagerWithPlatformFeatures::GetLayoutVisibleWindow() and line 295 of
// ViewportManagerWithPlatformFeatures::OnBringIntoViewRequested(...), animated bring-into-view operations are not supported.
// ViewportManagerWithPlatformFeatures::GetLayoutVisibleWindow snaps the RealizationWindow to 0,0 while
// ViewportManagerWithPlatformFeatures::OnBringIntoViewRequested turns off BringIntoViewRequestedEventArgs::AnimationDesired.
void ItemsView::StartBringItemIntoView(
    int32_t index,
    const winrt::BringIntoViewOptions& options)
{
    ITEMSVIEW_TRACE_INFO(*this, TRACE_MSG_METH, METH_NAME, this);

    bool success = StartBringItemIntoViewInternal(true /*throwOutOfBounds*/, true /* throwOnAnyFailure */, index, options);
    MUX_ASSERT(success);
}

winrt::IVectorView<winrt::IInspectable> ItemsView::SelectedItems()
{
    return m_selectionModel.SelectedItems();
}

void ItemsView::Select(int itemIndex)
{
    m_selectionModel.Select(itemIndex);
}

void ItemsView::Deselect(int itemIndex)
{
    m_selectionModel.Deselect(itemIndex);
}

bool ItemsView::IsSelected(int itemIndex)
{
    const winrt::IReference<bool> isItemIndexSelected = m_selectionModel.IsSelected(itemIndex);
    return isItemIndexSelected != nullptr && isItemIndexSelected.Value();
}

void ItemsView::SelectAll()
{
    // TODO: Update once ItemsView has grouping.
    // This function assumes a flat list data source.
    m_selectionModel.SelectAllFlat();
}

void ItemsView::DeselectAll()
{
    m_selectionModel.ClearSelection();
}

void ItemsView::InvertSelection()
{
    if (auto const& itemsRepeater = m_itemsRepeater.get())
    {
        if (auto const& itemsSourceView = itemsRepeater.ItemsSourceView())
        {
            const auto selectedIndices = m_selectionModel.SelectedIndices();
            int indexEnd = itemsSourceView.Count() - 1;

            // We loop backwards through the selected indices so we can deselect as we go
            for (int i = selectedIndices.Size() - 1; i >= 0; i--)
            {
                auto indexPath = selectedIndices.GetAt(i);
                // TODO: Update once ItemsView has grouping.
                int index = indexPath.GetAt(0);

                // Select all the unselected items
                if (index < indexEnd)
                {
                    auto startIndex = winrt::IndexPath::CreateFrom(index + 1);
                    auto endIndex = winrt::IndexPath::CreateFrom(indexEnd);
                    m_selectionModel.SelectRange(startIndex, endIndex);
                }

                m_selectionModel.DeselectAt(indexPath);
                indexEnd = index - 1;
            }

            // Select the remaining unselected items at the beginning of the collection
            if (indexEnd >= 0)
            {
                auto startIndex = winrt::IndexPath::CreateFrom(0);
                auto endIndex = winrt::IndexPath::CreateFrom(indexEnd);
                m_selectionModel.SelectRange(startIndex, endIndex);
            }
        }
    }
}

#pragma endregion

#pragma region IControlOverrides

void ItemsView::OnGotFocus(
    winrt::RoutedEventArgs const& args)
{
    ITEMSVIEW_TRACE_VERBOSE(*this, TRACE_MSG_METH, METH_NAME, this);

    __super::OnGotFocus(args);
}

#pragma endregion

#pragma region IFrameworkElementOverrides

void ItemsView::OnApplyTemplate()
{
    ITEMSVIEW_TRACE_INFO(*this, TRACE_MSG_METH, METH_NAME, this);

    RestoreOriginalVerticalScrollControllerAndVisibility();

    __super::OnApplyTemplate();

    winrt::IControlProtected thisAsControlProtected = *this;

    winrt::ScrollView scrollView = GetTemplateChildT<winrt::ScrollView>(s_scrollViewPartName, thisAsControlProtected);

    UpdateScrollView(scrollView);

    winrt::ItemsRepeater itemsRepeater = GetTemplateChildT<winrt::ItemsRepeater>(s_itemsRepeaterPartName, thisAsControlProtected);

    UpdateItemsRepeater(itemsRepeater);

    m_setVerticalScrollControllerOnLoaded = true;
}

#pragma endregion

#pragma region IUIElementOverrides

winrt::AutomationPeer ItemsView::OnCreateAutomationPeer()
{
    return winrt::make<ItemsViewAutomationPeer>(*this);
}

#pragma endregion

// Invoked when a dependency property of this ItemsView has changed.
void ItemsView::OnPropertyChanged(const winrt::DependencyPropertyChangedEventArgs& args)
{
    auto dependencyProperty = args.Property();

    ITEMSVIEW_TRACE_VERBOSE_DBG(nullptr, L"%s(property: %s)\n", METH_NAME, DependencyPropertyToStringDbg(dependencyProperty).c_str());

    if (dependencyProperty == s_IsItemInvokedEnabledProperty)
    {
        OnIsItemInvokedEnabledChanged();
    }
    else if (dependencyProperty == s_ItemTemplateProperty)
    {
        OnItemTemplateChanged();
    }
    else if (dependencyProperty == s_SelectionModeProperty)
    {
        OnSelectionModeChanged();
    }
    else if (dependencyProperty == s_ItemsSourceProperty)
    {
        OnItemsSourceChanged();
    }
    else if (dependencyProperty == s_VerticalScrollControllerProperty)
    {
        OnVerticalScrollControllerChanged();
    }
#ifdef DBG
    else if (dependencyProperty == s_LayoutProperty)
    {
        OnLayoutChangedDbg();
    }
#endif
}

// Make sure the default ItemTemplate is used when the ItemsSource is non-null and the ItemTemplate is null.
// Prevents ViewManager::GetElementFromElementFactory from setting the ItemsRepeater.ItemTemplate property 
// which would clear the template-binding between the ItemsView & ItemsRepeater ItemTemplate properties.
void ItemsView::EnsureItemTemplate()
{
    ITEMSVIEW_TRACE_VERBOSE(*this, TRACE_MSG_METH, METH_NAME, this);

    if (ItemsSource() != nullptr && ItemTemplate() == nullptr)
    {
        // The default ItemTemplate uses an ItemContainer for its root element since this is an ItemsView requirement for now.
        auto const defaultItemTemplate = winrt::XamlReader::Load(L"<DataTemplate xmlns='http://schemas.microsoft.com/winfx/2006/xaml/presentation'><ItemContainer><TextBlock Text='{Binding}'/></ItemContainer></DataTemplate>").as<winrt::DataTemplate>();

        ItemTemplate(defaultItemTemplate);
    }
}

void ItemsView::UpdateItemsRepeater(
    const winrt::ItemsRepeater& itemsRepeater)
{
    ITEMSVIEW_TRACE_VERBOSE(*this, TRACE_MSG_METH, METH_NAME, this);

    m_keyboardNavigationReferenceResetPending = false;

    UnhookItemsRepeaterEvents(false /*isForDestructor*/);

    // Reset inner ItemsRepeater dependency properties
    if (auto oldItemsRepeater = m_itemsRepeater.get())
    {
        oldItemsRepeater.ClearValue(winrt::ItemsRepeater::ItemsSourceProperty());
        oldItemsRepeater.ClearValue(winrt::ItemsRepeater::ItemTemplateProperty());
        oldItemsRepeater.ClearValue(winrt::ItemsRepeater::LayoutProperty());
    }

    m_itemsRepeater.set(itemsRepeater);

    HookItemsRepeaterEvents();
}

void ItemsView::UpdateScrollView(
    const winrt::ScrollView& scrollView)
{
    ITEMSVIEW_TRACE_VERBOSE(*this, TRACE_MSG_METH, METH_NAME, this);

    UnhookScrollViewEvents(false /*isForDestructor*/);

    m_scrollView.set(scrollView);

    SetValue(s_ScrollViewProperty, scrollView);

    HookScrollViewEvents();
}

void ItemsView::UpdateSelector()
{
    ITEMSVIEW_TRACE_VERBOSE(*this, TRACE_MSG_METH_INT, METH_NAME, this, SelectionMode());

    m_selectionModel.SingleSelect(false);

    switch (SelectionMode())
    {
        case winrt::ItemsViewSelectionMode::None:
        {
            m_selectionModel.ClearSelection();
            m_selector = std::make_shared<NullSelector>();
            break;
        }

        case winrt::ItemsViewSelectionMode::Single:
        {
            m_selectionModel.SingleSelect(true);

            m_selector = std::make_shared<SingleSelector>();
            m_selector->SetSelectionModel(m_selectionModel);
            break;
        }

        case winrt::ItemsViewSelectionMode::Multiple:
        {
            m_selector = std::make_shared<MultipleSelector>();
            m_selector->SetSelectionModel(m_selectionModel);
            break;
        }

        case winrt::ItemsViewSelectionMode::Extended:
        {
            m_selector = std::make_shared<ExtendedSelector>();
            m_selector->SetSelectionModel(m_selectionModel);
            break;
        }
    }
}

bool ItemsView::CanRaiseItemInvoked(
    const winrt::ItemContainerInteractionTrigger& interactionTrigger,
    const winrt::ItemContainer& itemContainer)
{
    MUX_ASSERT(itemContainer != nullptr);

#ifdef MUX_PRERELEASE
    const winrt::ItemContainerUserInvokeMode canUserInvoke = itemContainer.CanUserInvoke();
#else
    const winrt::ItemContainerUserInvokeMode canUserInvoke = winrt::get_self<ItemContainer>(itemContainer)->CanUserInvokeInternal();
#endif

    if (static_cast<int>(canUserInvoke & (winrt::ItemContainerUserInvokeMode::UserCannotInvoke | winrt::ItemContainerUserInvokeMode::UserCanInvoke)) != static_cast<int>(winrt::ItemContainerUserInvokeMode::UserCanInvoke))
    {
        ITEMSVIEW_TRACE_INFO_DBG(*this, TRACE_MSG_METH_STR, METH_NAME, this, L"Returns false based on ItemContainer.CanUserInvoke.");

        return false;
    }

    const bool cannotRaiseItemInvoked = 
        (!IsItemInvokedEnabled()  || 
        (SelectionMode() == winrt::ItemsViewSelectionMode::None && interactionTrigger == winrt::ItemContainerInteractionTrigger::DoubleTap) ||
        (SelectionMode() != winrt::ItemsViewSelectionMode::None && (interactionTrigger == winrt::ItemContainerInteractionTrigger::Tap || interactionTrigger == winrt::ItemContainerInteractionTrigger::SpaceKey)));

    const bool canRaiseItemInvoked = !cannotRaiseItemInvoked;

    ITEMSVIEW_TRACE_INFO_DBG(*this, TRACE_MSG_METH_STR_INT, METH_NAME, this, L"Returns based on ItemsView.IsItemInvokedEnabled.", canRaiseItemInvoked);

    return canRaiseItemInvoked;
}

void ItemsView::RaiseItemInvoked(
    const winrt::UIElement& element)
{
    if (m_itemInvokedEventSource)
    {
        bool itemInvokedFound = false;

        auto const& itemInvoked = GetElementItem(element, &itemInvokedFound);

        ITEMSVIEW_TRACE_INFO(*this, TRACE_MSG_METH_PTR_INT, METH_NAME, this, itemInvoked, itemInvokedFound);

        if (itemInvokedFound)
        {
            auto itemsViewItemInvokedEventArgs = winrt::make_self<ItemsViewItemInvokedEventArgs>(itemInvoked);

            m_itemInvokedEventSource(*this, *itemsViewItemInvokedEventArgs);
        }
    }
}

void ItemsView::RaiseSelectionChanged()
{
    if (m_selectionChangedEventSource)
    {
        ITEMSVIEW_TRACE_INFO(*this, TRACE_MSG_METH, METH_NAME, this);

        auto itemsViewSelectionChangedEventArgs = winrt::make_self<ItemsViewSelectionChangedEventArgs>();

        m_selectionChangedEventSource(*this, *itemsViewSelectionChangedEventArgs);
    }
}

void ItemsView::HookItemsRepeaterEvents()
{
    ITEMSVIEW_TRACE_VERBOSE(*this, TRACE_MSG_METH, METH_NAME, this);

    if (auto itemsRepeater = m_itemsRepeater.get())
    {
        m_itemsRepeaterElementPreparedRevoker = itemsRepeater.ElementPrepared(winrt::auto_revoke, { this, &ItemsView::OnItemsRepeaterElementPrepared });
        m_itemsRepeaterElementClearingRevoker = itemsRepeater.ElementClearing(winrt::auto_revoke, { this, &ItemsView::OnItemsRepeaterElementClearing });
        m_itemsRepeaterElementIndexChangedRevoker = itemsRepeater.ElementIndexChanged(winrt::auto_revoke, { this, &ItemsView::OnItemsRepeaterElementIndexChanged });
        m_itemsRepeaterLayoutUpdatedRevoker = itemsRepeater.LayoutUpdated(winrt::auto_revoke, { this, &ItemsView::OnItemsRepeaterLayoutUpdated });
        m_itemsRepeaterSizeChangedRevoker = itemsRepeater.SizeChanged(winrt::auto_revoke, { this, &ItemsView::OnItemsRepeaterSizeChanged });
        m_itemsRepeaterItemsSourcePropertyChangedRevoker = RegisterPropertyChanged(itemsRepeater, winrt::ItemsRepeater::ItemsSourceProperty(), { this, &ItemsView::OnItemsRepeaterItemsSourceChanged });
    }
}

void ItemsView::UnhookItemsRepeaterEvents(
    bool isForDestructor)
{
    if (isForDestructor)
    {
        ITEMSVIEW_TRACE_VERBOSE(nullptr, TRACE_MSG_METH, METH_NAME, this);
    }
    else
    {
        ITEMSVIEW_TRACE_VERBOSE(*this, TRACE_MSG_METH, METH_NAME, this);
    }

    if (auto itemRepeater = isForDestructor ? m_itemsRepeater.safe_get() : m_itemsRepeater.get())
    {
        m_itemsRepeaterElementPreparedRevoker.revoke();
        m_itemsRepeaterElementClearingRevoker.revoke();
        m_itemsRepeaterElementIndexChangedRevoker.revoke();
        m_itemsRepeaterItemsSourcePropertyChangedRevoker.revoke();
        m_itemsRepeaterLayoutUpdatedRevoker.revoke();
        m_itemsRepeaterSizeChangedRevoker.revoke();

        if (isForDestructor)
        {
            ClearAllItemsViewItemContainerRevokers();
        }
    }
}

void ItemsView::HookScrollViewEvents()
{
    ITEMSVIEW_TRACE_VERBOSE(*this, TRACE_MSG_METH, METH_NAME, this);

    if (auto const& scrollView = m_scrollView.get())
    {
        m_scrollViewAnchorRequestedRevoker = scrollView.AnchorRequested(winrt::auto_revoke, { this, &ItemsView::OnScrollViewAnchorRequested });
        m_scrollViewBringingIntoViewRevoker = scrollView.BringingIntoView(winrt::auto_revoke, { this, &ItemsView::OnScrollViewBringingIntoView });
#ifdef DBG
        m_scrollViewExtentChangedRevokerDbg = scrollView.ExtentChanged(winrt::auto_revoke, { this, &ItemsView::OnScrollViewExtentChangedDbg });
#endif
        m_scrollViewScrollCompletedRevoker = scrollView.ScrollCompleted(winrt::auto_revoke, { this, &ItemsView::OnScrollViewScrollCompleted });
    }
}

void ItemsView::UnhookScrollViewEvents(
    bool isForDestructor)
{
    if (isForDestructor)
    {
        ITEMSVIEW_TRACE_VERBOSE(nullptr, TRACE_MSG_METH, METH_NAME, this);
    }
    else
    {
        ITEMSVIEW_TRACE_VERBOSE(*this, TRACE_MSG_METH, METH_NAME, this);
    }

    if (auto scrollView = isForDestructor ? m_scrollView.safe_get() : m_scrollView.get())
    {
        m_scrollViewAnchorRequestedRevoker.revoke();
        m_scrollViewBringingIntoViewRevoker.revoke();
#ifdef DBG
        m_scrollViewExtentChangedRevokerDbg.revoke();
#endif
        m_scrollViewScrollCompletedRevoker.revoke();
    }
}

void ItemsView::HookCompositionTargetRendering()
{
    if (!m_renderingRevoker)
    {
        winrt::Microsoft::UI::Xaml::Media::CompositionTarget compositionTarget{ nullptr };
        m_renderingRevoker = compositionTarget.Rendering(winrt::auto_revoke, { this, &ItemsView::OnCompositionTargetRendering });
    }
}

void ItemsView::UnhookCompositionTargetRendering()
{
    m_renderingRevoker.revoke();
}

void ItemsView::CacheOriginalVerticalScrollControllerAndVisibility()
{
    if (auto scrollView = m_scrollView.get())
    {
        if (const auto scrollPresenter = scrollView.ScrollPresenter())
        {
            m_originalVerticalScrollController.set(scrollPresenter.VerticalScrollController());
        }

        m_originalVerticalScrollBarVisibility = scrollView.VerticalScrollBarVisibility();
    }

    ITEMSVIEW_TRACE_VERBOSE_DBG(*this, TRACE_MSG_METH_PTR_STR, METH_NAME, this, m_originalVerticalScrollController, L"m_originalVerticalScrollController");
    ITEMSVIEW_TRACE_VERBOSE_DBG(*this, TRACE_MSG_METH_STR_INT, METH_NAME, this, L"m_originalVerticalScrollBarVisibility", m_originalVerticalScrollBarVisibility);
}

// Restore the original ScrollView and ScrollController properties when
// the ItemsView gets re-templated.
void ItemsView::RestoreOriginalVerticalScrollControllerAndVisibility()
{
    ITEMSVIEW_TRACE_VERBOSE_DBG(*this, TRACE_MSG_METH_PTR_STR, METH_NAME, this, m_originalVerticalScrollController, L"m_originalVerticalScrollController");
    ITEMSVIEW_TRACE_VERBOSE_DBG(*this, TRACE_MSG_METH_STR_INT, METH_NAME, this, L"m_originalVerticalScrollBarVisibility", m_originalVerticalScrollBarVisibility);

    if (auto scrollView = m_scrollView.get())
    {
        if (auto scrollPresenter = scrollView.ScrollPresenter())
        {
            scrollPresenter.VerticalScrollController(m_originalVerticalScrollController.get());
        }

        scrollView.VerticalScrollBarVisibility(m_originalVerticalScrollBarVisibility);
    }

    m_originalVerticalScrollController.set(nullptr);
    m_originalVerticalScrollBarVisibility = winrt::ScrollingScrollBarVisibility::Auto;
}

void ItemsView::SetVerticalScrollControllerOnLoaded()
{
    if (VerticalScrollController())
    {
        // Apply the VerticalScrollController property value that was set
        // before the Loaded event.
        ApplyVerticalScrollController();
    }
    else
    {
        // The VerticalScrollController property was left null prior to the
        // Loaded event. Use the value from the inner ScrollPresenter.
        // This may invoke ItemsView::OnVerticalScrollControllerChanged and
        // ItemsView::ApplyVerticalScrollController but will be a no-op.
        VerticalScrollController(m_originalVerticalScrollController.get());
    }
}

// Propagates the ItemsView.VerticalScrollController property value to the
// inner ScrollPresenter.VerticalScrollController property.
// If the value is other than original value read in the Loaded event,
// the inner ScrollView's VerticalScrollBarVisibility is set to Hidden.
// Else, the original visibility is restored.
void ItemsView::ApplyVerticalScrollController()
{
    ITEMSVIEW_TRACE_INFO_DBG(*this, TRACE_MSG_METH_PTR_STR, METH_NAME, this, VerticalScrollController(), L"VerticalScrollController");

    if (auto scrollView = m_scrollView.get())
    {
        const auto verticalScrollController = VerticalScrollController();

        if (verticalScrollController == m_originalVerticalScrollController.get())
        {
            scrollView.VerticalScrollBarVisibility(m_originalVerticalScrollBarVisibility);
        }
        else
        {
            scrollView.VerticalScrollBarVisibility(winrt::ScrollingScrollBarVisibility::Hidden);
        }

        auto scrollViewImpl = winrt::get_self<::ScrollView>(scrollView);

        if (auto scrollPresenter = scrollViewImpl->ScrollPresenter())
        {
            scrollPresenter.VerticalScrollController(verticalScrollController);
        }
    }
}

// Invoked at the beginning of a StartBringItemIntoView call to abort any potential bring-into-view operation, and a few ticks after a bring-into-view
// operation completed, giving time for the new layout to settle and scroll anchoring to do its job of freezing its target element.
void ItemsView::CompleteStartBringItemIntoView()
{
    ITEMSVIEW_TRACE_INFO_DBG(*this, TRACE_MSG_METH_PTR_STR, METH_NAME, this, m_bringIntoViewElement.get(), L"m_bringIntoViewElement reset.");
    ITEMSVIEW_TRACE_INFO_DBG(*this, TRACE_MSG_METH_STR_INT, METH_NAME, this, L"m_bringIntoViewCorrelationId reset", m_bringIntoViewCorrelationId);
    ITEMSVIEW_TRACE_INFO_DBG(*this, TRACE_MSG_METH_STR_INT, METH_NAME, this, L"m_bringIntoViewElementRetentionCountdown reset", m_bringIntoViewElementRetentionCountdown);

    m_bringIntoViewElement.set(nullptr);
    m_bringIntoViewElementRetentionCountdown = 0;
    m_bringIntoViewCorrelationId = -1;

    if (m_scrollViewHorizontalAnchorRatio != -1)
    {
        MUX_ASSERT(m_scrollViewVerticalAnchorRatio != -1);

        // Restore the pre-operation anchor settings.
        if (const auto scrollView = m_scrollView.get())
        {
            scrollView.HorizontalAnchorRatio(m_scrollViewHorizontalAnchorRatio);
            scrollView.VerticalAnchorRatio(m_scrollViewVerticalAnchorRatio);
        }

        m_scrollViewHorizontalAnchorRatio = -1;
        m_scrollViewVerticalAnchorRatio = -1;
    }

    if (m_navigationKeyProcessingCountdown == 0)
    {
        UnhookCompositionTargetRendering();
    }
}

void ItemsView::OnItemsRepeaterElementPrepared(
    const winrt::ItemsRepeater& itemsRepeater,
    const winrt::ItemsRepeaterElementPreparedEventArgs& args)
{
    if (const auto element = args.Element())
    {
        const auto index = args.Index();

#ifdef DBG_VERBOSE
        ITEMSVIEW_TRACE_VERBOSE(*this, TRACE_MSG_METH_INT, METH_NAME, this, index);
#endif

        MUX_ASSERT(index == GetElementIndex(element));

        const auto itemContainer = element.try_as<winrt::ItemContainer>();

        if (itemContainer != nullptr)
        {
#ifdef MUX_PRERELEASE
            if (static_cast<int>(itemContainer.CanUserInvoke() & winrt::ItemContainerUserInvokeMode::Auto))
            {
                winrt::ItemContainerUserInvokeMode canUserInvoke = winrt::ItemContainerUserInvokeMode::Auto;

                canUserInvoke |= IsItemInvokedEnabled() ? winrt::ItemContainerUserInvokeMode::UserCanInvoke : winrt::ItemContainerUserInvokeMode::UserCannotInvoke;

                itemContainer.CanUserInvoke(canUserInvoke);
            }

            if (static_cast<int>(itemContainer.MultiSelectMode() & winrt::ItemContainerMultiSelectMode::Auto))
            {
                winrt::ItemContainerMultiSelectMode multiSelectMode = winrt::ItemContainerMultiSelectMode::Auto;

                switch (SelectionMode())
                {
                    case winrt::ItemsViewSelectionMode::None:
                    case winrt::ItemsViewSelectionMode::Single:
                        multiSelectMode |= winrt::ItemContainerMultiSelectMode::Single;
                        break;
                    case winrt::ItemsViewSelectionMode::Extended:
                        multiSelectMode |= winrt::ItemContainerMultiSelectMode::Extended;
                        break;
                    case winrt::ItemsViewSelectionMode::Multiple:
                        multiSelectMode |= winrt::ItemContainerMultiSelectMode::Multiple;
                        break;
                }

                itemContainer.MultiSelectMode(multiSelectMode);
            }

            if (static_cast<int>(itemContainer.CanUserSelect() & winrt::ItemContainerUserSelectMode::Auto))
            {
                winrt::ItemContainerUserSelectMode canUserSelect = winrt::ItemContainerUserSelectMode::Auto;

                canUserSelect |= SelectionMode() == winrt::ItemsViewSelectionMode::None ? winrt::ItemContainerUserSelectMode::UserCannotSelect : winrt::ItemContainerUserSelectMode::UserCanSelect;

                itemContainer.CanUserSelect(canUserSelect);
            }
#else
            if (const auto& itemContainerImpl = winrt::get_self<ItemContainer>(itemContainer))
            {
                if (static_cast<int>(itemContainerImpl->CanUserInvokeInternal() & winrt::ItemContainerUserInvokeMode::Auto))
                {
                    winrt::ItemContainerUserInvokeMode canUserInvoke = winrt::ItemContainerUserInvokeMode::Auto;

                    canUserInvoke |= IsItemInvokedEnabled() ? winrt::ItemContainerUserInvokeMode::UserCanInvoke : winrt::ItemContainerUserInvokeMode::UserCannotInvoke;

                    itemContainerImpl->CanUserInvokeInternal(canUserInvoke);
                }

                if (static_cast<int>(itemContainerImpl->MultiSelectModeInternal() & winrt::ItemContainerMultiSelectMode::Auto))
                {
                    winrt::ItemContainerMultiSelectMode multiSelectMode = winrt::ItemContainerMultiSelectMode::Auto;

                    switch (SelectionMode())
                    {
                        case winrt::ItemsViewSelectionMode::None:
                        case winrt::ItemsViewSelectionMode::Single:
                            multiSelectMode |= winrt::ItemContainerMultiSelectMode::Single;
                            break;
                        case winrt::ItemsViewSelectionMode::Extended:
                            multiSelectMode |= winrt::ItemContainerMultiSelectMode::Extended;
                            break;
                        case winrt::ItemsViewSelectionMode::Multiple:
                            multiSelectMode |= winrt::ItemContainerMultiSelectMode::Multiple;
                            break;
                    }

                    itemContainerImpl->MultiSelectModeInternal(multiSelectMode);
                }

                if (static_cast<int>(itemContainerImpl->CanUserSelectInternal() & winrt::ItemContainerUserSelectMode::Auto))
                {
                    winrt::ItemContainerUserSelectMode canUserSelect = winrt::ItemContainerUserSelectMode::Auto;

                    canUserSelect |= SelectionMode() == winrt::ItemsViewSelectionMode::None ? winrt::ItemContainerUserSelectMode::UserCannotSelect : winrt::ItemContainerUserSelectMode::UserCanSelect;

                    itemContainerImpl->CanUserSelectInternal(canUserSelect);
                }
            }
#endif

            winrt::IReference<bool> isItemContainerSelected = m_selectionModel.IsSelected(index);

            itemContainer.IsSelected(isItemContainerSelected != nullptr && isItemContainerSelected.Value());

            SetItemsViewItemContainerRevokers(itemContainer);
        }
        else
        {
            throw winrt::hresult_error(E_INVALIDARG, s_invalidItemTemplateRoot);
        }

        if (const auto itemsSourceView = itemsRepeater.ItemsSourceView())
        {
            element.SetValue(winrt::AutomationProperties::PositionInSetProperty(), box_value(index + 1));
            element.SetValue(winrt::AutomationProperties::SizeOfSetProperty(), box_value(itemsSourceView.Count()));
        }
    }
}

void ItemsView::OnItemsRepeaterElementClearing(
    const winrt::ItemsRepeater& itemsRepeater,
    const winrt::ItemsRepeaterElementClearingEventArgs& args)
{
    if (const auto element = args.Element())
    {
#ifdef DBG_VERBOSE
        ITEMSVIEW_TRACE_VERBOSE(*this, TRACE_MSG_METH_INT, METH_NAME, this, GetElementIndex(element));
#endif

        const auto itemContainer = element.try_as<winrt::ItemContainer>();

        if (itemContainer != nullptr)
        {
            // Clear all the revokers first before touching ItemContainer properties to avoid side effects.
            // For example, if you clear IsSelected before clearing revokers, we will listen to that change and
            // update SelectionModel which is incorrect.
            ClearItemsViewItemContainerRevokers(itemContainer);

#ifdef MUX_PRERELEASE
            if (static_cast<int>(itemContainer.CanUserInvoke() & winrt::ItemContainerUserInvokeMode::Auto))
            {
                itemContainer.CanUserInvoke(winrt::ItemContainerUserInvokeMode::Auto);
            }

            if (static_cast<int>(itemContainer.MultiSelectMode() & winrt::ItemContainerMultiSelectMode::Auto))
            {
                itemContainer.MultiSelectMode(winrt::ItemContainerMultiSelectMode::Auto);
            }

            if (static_cast<int>(itemContainer.CanUserSelect() & winrt::ItemContainerUserSelectMode::Auto))
            {
                itemContainer.CanUserSelect(winrt::ItemContainerUserSelectMode::Auto);
            }
#else
            if (const auto& itemContainerImpl = winrt::get_self<ItemContainer>(itemContainer))
            {
                if (static_cast<int>(itemContainerImpl->CanUserInvokeInternal() & winrt::ItemContainerUserInvokeMode::Auto))
                {
                    itemContainerImpl->CanUserInvokeInternal(winrt::ItemContainerUserInvokeMode::Auto);
                }

                if (static_cast<int>(itemContainerImpl->MultiSelectModeInternal() & winrt::ItemContainerMultiSelectMode::Auto))
                {
                    itemContainerImpl->MultiSelectModeInternal(winrt::ItemContainerMultiSelectMode::Auto);
                }

                if (static_cast<int>(itemContainerImpl->CanUserSelectInternal() & winrt::ItemContainerUserSelectMode::Auto))
                {
                    itemContainerImpl->CanUserSelectInternal(winrt::ItemContainerUserSelectMode::Auto);
                }
            }
#endif

            itemContainer.IsSelected(false);
        }

        element.ClearValue(winrt::AutomationProperties::PositionInSetProperty());
        element.ClearValue(winrt::AutomationProperties::SizeOfSetProperty());
    }
}

void ItemsView::OnItemsRepeaterElementIndexChanged(
    const winrt::ItemsRepeater& itemsRepeater,
    const winrt::ItemsRepeaterElementIndexChangedEventArgs& args)
{
    if (const auto element = args.Element())
    {
        const auto newIndex = args.NewIndex();

        ITEMSVIEW_TRACE_VERBOSE(*this, TRACE_MSG_METH_INT, METH_NAME, this, newIndex);

        element.SetValue(winrt::AutomationProperties::PositionInSetProperty(), box_value(newIndex + 1));
    }
}

void ItemsView::OnItemsRepeaterItemsSourceChanged(
    const winrt::DependencyObject& sender,
    const winrt::DependencyProperty& args)
{
    ITEMSVIEW_TRACE_VERBOSE(*this, TRACE_MSG_METH, METH_NAME, this);

    m_itemsSourceViewChangedRevoker.revoke();

    if (auto const& itemsRepeater = m_itemsRepeater.get())
    {
        if (auto const& itemsSourceView = itemsRepeater.ItemsSourceView())
        {
            m_itemsSourceViewChangedRevoker = itemsSourceView.CollectionChanged(winrt::auto_revoke, { this, &ItemsView::OnSourceListChanged });
        }
    }

    auto const& itemsSource = ItemsSource();

    // Updating the selection model's ItemsSource here rather than earlier in ItemsView::OnPropertyChanged/ItemsView::OnItemsSourceChanged so that
    // Layout::OnItemsChangedCore is executed before ItemsView::OnSelectionModelSelectionChanged. Otherwise OnSelectionModelSelectionChanged
    // would operate on out-of-date ItemsRepeater children.
    m_selectionModel.Source(itemsSource);

    m_currentElementSelectionModel.Source(itemsSource);
}

void ItemsView::OnItemsRepeaterLayoutUpdated(
    const winrt::IInspectable& sender,
    const winrt::IInspectable& args)
{
#ifdef DBG_VERBOSE
    ITEMSVIEW_TRACE_VERBOSE(*this, TRACE_MSG_METH, METH_NAME, this);
#endif

    if (m_keyboardNavigationReferenceResetPending)
    {
        m_keyboardNavigationReferenceResetPending = false;
        UpdateKeyboardNavigationReference();
    }
}

void ItemsView::OnItemsRepeaterSizeChanged(
    const winrt::IInspectable& sender,
    const winrt::SizeChangedEventArgs& args)
{
    ITEMSVIEW_TRACE_VERBOSE(*this, TRACE_MSG_METH_STR_STR, METH_NAME, this, TypeLogging::SizeToString(args.PreviousSize()).c_str(), TypeLogging::SizeToString(args.NewSize()).c_str());

    UpdateKeyboardNavigationReference();
}

void ItemsView::OnScrollViewAnchorRequested(
    const winrt::ScrollView& scrollView,
    const winrt::ScrollingAnchorRequestedEventArgs& args)
{
    ITEMSVIEW_TRACE_INFO_DBG(*this, TRACE_MSG_METH_STR_INT, METH_NAME, this, L"ScrollingAnchorRequestedEventArgs.AnchorCandidates.Size", args.AnchorCandidates().Size());

    if (m_bringIntoViewElement != nullptr)
    {
        // During a StartBringItemIntoView operation, its target element is used as the scroll anchor so that any potential shuffling of the Layout does not disturb the final visual.
        // This anchor is used until the new layout has a chance to settle.
        ITEMSVIEW_TRACE_INFO_DBG(*this, TRACE_MSG_METH_PTR_STR, METH_NAME, this, m_bringIntoViewElement.get(), L"ScrollingAnchorRequestedEventArgs.AnchorElement set to m_bringIntoViewElement.");
        ITEMSVIEW_TRACE_INFO_DBG(*this, TRACE_MSG_METH_STR_INT, METH_NAME, this, L"at index", GetElementIndex(m_bringIntoViewElement.get()));

        args.AnchorElement(m_bringIntoViewElement.get());
    }
#ifdef DBG
    else
    {
        ITEMSVIEW_TRACE_INFO(*this, TRACE_MSG_METH_STR, METH_NAME, this, L"ScrollingAnchorRequestedEventArgs.AnchorElement unset. m_bringIntoViewElement null.");
    }
#endif
}

void ItemsView::OnScrollViewBringingIntoView(
    const winrt::ScrollView& scrollView,
    const winrt::ScrollingBringingIntoViewEventArgs& args)
{
#ifdef DBG
    ITEMSVIEW_TRACE_INFO(*this, TRACE_MSG_METH_STR_INT, METH_NAME, this,
        L"ScrollingBringingIntoViewEventArgs.CorrelationId", args.CorrelationId());
    ITEMSVIEW_TRACE_INFO(*this, TRACE_MSG_METH_STR_DBL, METH_NAME, this,
        L"ScrollingBringingIntoViewEventArgs.TargetHorizontalOffset", args.TargetHorizontalOffset());
    ITEMSVIEW_TRACE_INFO(*this, TRACE_MSG_METH_STR_DBL, METH_NAME, this,
        L"ScrollingBringingIntoViewEventArgs.TargetVerticalOffset", args.TargetVerticalOffset());

    if (auto const& requestEventArgs = args.RequestEventArgs())
    {
        ITEMSVIEW_TRACE_INFO(*this, L"%s[0x%p](ScrollingBringingIntoViewEventArgs.RequestEventArgs: AnimationDesired:%d, H/V AlignmentRatio:%lf,%lf, H/V Offset:%f,%f, TargetRect:%s, TargetElement:0x%p)\n",
            METH_NAME, this,
            requestEventArgs.AnimationDesired(),
            requestEventArgs.HorizontalAlignmentRatio(), requestEventArgs.VerticalAlignmentRatio(),
            requestEventArgs.HorizontalOffset(), requestEventArgs.VerticalOffset(),
            TypeLogging::RectToString(requestEventArgs.TargetRect()).c_str(),
            requestEventArgs.TargetElement());

        if (requestEventArgs.AnimationDesired())
        {
            ITEMSVIEW_TRACE_INFO(*this, TRACE_MSG_METH_STR, METH_NAME, this,
                L"ScrollingBringingIntoViewEventArgs.RequestEventArgs.AnimationDesired unexpectedly True");
        }
    }

    ITEMSVIEW_TRACE_INFO(*this, TRACE_MSG_METH_PTR_STR, METH_NAME, this,
        m_bringIntoViewElement.get(), L"m_bringIntoViewElement");
#endif

    if (m_bringIntoViewCorrelationId == -1 &&
        m_bringIntoViewElement != nullptr &&
        args.RequestEventArgs() != nullptr &&
        m_bringIntoViewElement.get() == args.RequestEventArgs().TargetElement())
    {
        // Record the CorrelationId for the bring-into-view operation so ItemsView::OnScrollViewScrollCompleted can trigger the countdown to a stable layout.
        m_bringIntoViewCorrelationId = args.CorrelationId();

        ITEMSVIEW_TRACE_INFO_DBG(*this, TRACE_MSG_METH_STR_INT, METH_NAME, this, L"for m_bringIntoViewElement index", GetElementIndex(m_bringIntoViewElement.get()));
        ITEMSVIEW_TRACE_INFO_DBG(*this, TRACE_MSG_METH_STR_INT, METH_NAME, this, L"m_bringIntoViewCorrelationId set", m_bringIntoViewCorrelationId);
    }

    if (m_navigationKeyBringIntoViewPendingCount > 0)
    {
        m_navigationKeyBringIntoViewPendingCount--;
        // Record the CorrelationId for the navigation-key-triggered bring-into-view operation so ItemsView::OnScrollViewScrollCompleted can trigger the countdown
        // to a stable layout for large offset changes or immediately process queued navigation keys.
        m_navigationKeyBringIntoViewCorrelationId = args.CorrelationId();

        ITEMSVIEW_TRACE_INFO_DBG(*this, TRACE_MSG_METH_STR_INT, METH_NAME, this, L"m_navigationKeyBringIntoViewPendingCount decremented", m_navigationKeyBringIntoViewPendingCount);
        ITEMSVIEW_TRACE_INFO_DBG(*this, TRACE_MSG_METH_STR_INT, METH_NAME, this, L"m_navigationKeyBringIntoViewCorrelationId set", m_navigationKeyBringIntoViewCorrelationId);
    }
}

#ifdef DBG
void ItemsView::OnScrollViewExtentChangedDbg(
    const winrt::ScrollView& scrollView,
    const winrt::IInspectable& args)
{
    if (auto const& scrollView = m_scrollView.get())
    {
        ITEMSVIEW_TRACE_INFO(*this, TRACE_MSG_METH_STR_DBL, METH_NAME, this, L"ScrollView.ExtentWidth", scrollView.ExtentWidth());
        ITEMSVIEW_TRACE_INFO(*this, TRACE_MSG_METH_STR_DBL, METH_NAME, this, L"ScrollView.ExtentHeight", scrollView.ExtentHeight());
    }
}
#endif

void ItemsView::OnScrollViewScrollCompleted(
    const winrt::ScrollView& scrollView,
    const winrt::ScrollingScrollCompletedEventArgs& args)
{
    ITEMSVIEW_TRACE_INFO_DBG(*this, TRACE_MSG_METH_STR_INT, METH_NAME, this, L"ScrollingScrollCompletedEventArgs.CorrelationId", args.CorrelationId());

    if (args.CorrelationId() == m_bringIntoViewCorrelationId)
    {
        MUX_ASSERT(m_bringIntoViewElement != nullptr);

        m_bringIntoViewCorrelationId = -1;

        ITEMSVIEW_TRACE_INFO_DBG(*this, TRACE_MSG_METH_STR, METH_NAME, this, L"m_bringIntoViewCorrelationId reset & m_bringIntoViewElementRetentionCountdown initialized to 4.");

        m_bringIntoViewElementRetentionCountdown = c_renderingEventsPostBringIntoView;

        // ItemsView::OnCompositionTargetRendering will decrement m_bringIntoViewElementRetentionCountdown until it reaches 0,
        // at which point m_bringIntoViewElement is reset to null and the ScrollView anchor alignments are restored.
        HookCompositionTargetRendering();
    }

    if (args.CorrelationId() == m_navigationKeyBringIntoViewCorrelationId)
    {
        if (m_lastNavigationKeyProcessed == winrt::VirtualKey::Left ||
            m_lastNavigationKeyProcessed == winrt::VirtualKey::Right ||
            m_lastNavigationKeyProcessed == winrt::VirtualKey::Up ||
            m_lastNavigationKeyProcessed == winrt::VirtualKey::Down)
        {
            ITEMSVIEW_TRACE_INFO_DBG(*this, TRACE_MSG_METH_STR, METH_NAME, this, L"m_navigationKeyBringIntoViewCorrelationId reset.");

            m_navigationKeyBringIntoViewCorrelationId = -1;

            if (m_navigationKeyBringIntoViewPendingCount == 0)
            {
                // After a small offset change, for better perf, immediately process the remaining queued navigation keys as no item re-layout is likely.
                ProcessNavigationKeys();
            }
        }
        else
        {
            ITEMSVIEW_TRACE_INFO_DBG(*this, TRACE_MSG_METH_STR, METH_NAME, this, L"m_navigationKeyProcessingCountdown initialized to 4.");

            // After a large offset change for PageDown/PageUp/Home/End, wait a few ticks for the UI to settle before processing the remaining queued
            // navigation keys, so the content is properly layed out.
            m_navigationKeyProcessingCountdown = c_renderingEventsPostBringIntoView;

            HookCompositionTargetRendering();
        }
    }
}

void ItemsView::OnItemsViewItemContainerIsSelectedChanged(
    const winrt::DependencyObject& sender,
    const winrt::DependencyProperty& args)
{
    auto const& element = sender.try_as<winrt::UIElement>();

    MUX_ASSERT(element != nullptr);

    if (element != nullptr)
    {
        const int itemIndex = GetElementIndex(element);

        if (itemIndex != -1)
        {
            auto const& itemContainer = element.try_as<winrt::ItemContainer>();

            MUX_ASSERT(itemContainer != nullptr);

            if (itemContainer != nullptr)
            {
                const winrt::IReference<bool> isSelectionModelSelectedAsNullable = m_selectionModel.IsSelected(itemIndex);
                const bool isSelectionModelSelected = isSelectionModelSelectedAsNullable != nullptr && isSelectionModelSelectedAsNullable.Value();

                if (itemContainer.IsSelected())
                {
                    if (!isSelectionModelSelected)
                    {
                        if (SelectionMode() == winrt::ItemsViewSelectionMode::None)
                        {
                            // Permission denied.
                            itemContainer.IsSelected(false);
                        }
                        else
                        {
                            // For all other selection modes, simply select the item.
                            // No need to go through the SingleSelector, MultipleSelector or ExtendedSelector policy.
                            m_selectionModel.Select(itemIndex);
                        }
                    }
                }
                else
                {
                    if (isSelectionModelSelected)
                    {
                        // For all selection modes, simply deselect the item & preserve the anchor if any.
                        m_selector->DeselectWithAnchorPreservation(itemIndex);
                    }
                }
            }
        }
    }
}

#ifdef DBG
void ItemsView::OnLayoutMeasureInvalidatedDbg(
    const winrt::Layout& sender,
    const winrt::IInspectable& args)
{
    ITEMSVIEW_TRACE_VERBOSE(*this, TRACE_MSG_METH, METH_NAME, this);
}

void ItemsView::OnLayoutArrangeInvalidatedDbg(
    const winrt::Layout& sender,
    const winrt::IInspectable& args)
{
    ITEMSVIEW_TRACE_VERBOSE(*this, TRACE_MSG_METH, METH_NAME, this);
}
#endif

void ItemsView::OnCompositionTargetRendering(
    const winrt::IInspectable& sender,
    const winrt::IInspectable& args)
{
    ITEMSVIEW_TRACE_INFO_DBG(*this, TRACE_MSG_METH_STR_INT, METH_NAME, this, L"m_bringIntoViewElementRetentionCountdown", m_bringIntoViewElementRetentionCountdown);
    ITEMSVIEW_TRACE_INFO_DBG(*this, TRACE_MSG_METH_STR_INT, METH_NAME, this, L"m_navigationKeyProcessingCountdown", m_navigationKeyProcessingCountdown);

    MUX_ASSERT(m_bringIntoViewElementRetentionCountdown > 0 || m_navigationKeyProcessingCountdown > 0);

    if (m_bringIntoViewElementRetentionCountdown > 0)
    {
        // Waiting for the new layout to settle before discarding the bring-into-view target element and no longer using it as a scroll anchor.
        m_bringIntoViewElementRetentionCountdown--;

        if (m_bringIntoViewElementRetentionCountdown == 0)
        {
            CompleteStartBringItemIntoView();
        }
    }

    if (m_navigationKeyProcessingCountdown > 0)
    {
        // Waiting for the new layout to settle before processing remaining queued navigation keys.
        m_navigationKeyProcessingCountdown--;

        if (m_navigationKeyProcessingCountdown == 0)
        {
            if (m_bringIntoViewElementRetentionCountdown == 0)
            {
                UnhookCompositionTargetRendering();
            }

            ITEMSVIEW_TRACE_INFO_DBG(*this, TRACE_MSG_METH_STR, METH_NAME, this, L"m_navigationKeyBringIntoViewCorrelationId reset.");

            m_navigationKeyBringIntoViewCorrelationId = -1;

            if (m_navigationKeyBringIntoViewPendingCount == 0)
            {
                // With no pending ItemsView::OnScrollViewBringingIntoView calls, it is time to process the remaining queue navigation keys.
                ProcessNavigationKeys();
            }
        }
    }
}

void ItemsView::OnItemsSourceChanged()
{
    ITEMSVIEW_TRACE_INFO(*this, TRACE_MSG_METH, METH_NAME, this);

    // When the inner ItemsRepeater has not been loaded yet, set the selection models' Source
    // right away as ItemsView::OnItemsRepeaterItemsSourceChanged will not be invoked.
    // There is no reason to delay the updates to ItemsView::OnItemsRepeaterItemsSourceChanged
    // in this case since ItemsRepeater and its children do not exist yet.
    auto const& itemsRepeater = m_itemsRepeater.get();

    if (itemsRepeater == nullptr)
    {
        auto const& itemsSource = ItemsSource();

        m_selectionModel.Source(itemsSource);
        m_currentElementSelectionModel.Source(itemsSource);
    }

    // Make sure the default ItemTemplate is used when the ItemsSource is non-null and the ItemTemplate is null.
    EnsureItemTemplate();
}

void ItemsView::OnVerticalScrollControllerChanged()
{
    ITEMSVIEW_TRACE_INFO_DBG(*this, TRACE_MSG_METH_PTR_STR, METH_NAME, this, VerticalScrollController(), L"VerticalScrollController");

    ApplyVerticalScrollController();
}

#ifdef DBG
void ItemsView::OnLayoutChangedDbg()
{
    ITEMSVIEW_TRACE_VERBOSE(*this, TRACE_MSG_METH, METH_NAME, this);

    m_layoutMeasureInvalidatedDbg.revoke();
    m_layoutArrangeInvalidatedDbg.revoke();

    if (auto const& layout = Layout())
    {
        m_layoutMeasureInvalidatedDbg = layout.MeasureInvalidated(winrt::auto_revoke, { this, &ItemsView::OnLayoutMeasureInvalidatedDbg });
        m_layoutArrangeInvalidatedDbg = layout.ArrangeInvalidated(winrt::auto_revoke, { this, &ItemsView::OnLayoutArrangeInvalidatedDbg });
    }
}
#endif

void ItemsView::OnIsItemInvokedEnabledChanged()
{
    ITEMSVIEW_TRACE_VERBOSE(*this, TRACE_MSG_METH, METH_NAME, this);

    // For all ItemContainer children, update the CanUserInvoke property according to the new ItemsView.IsItemInvokedEnabled property.
    if (auto const& itemsRepeater = m_itemsRepeater.get())
    {
        const auto count = winrt::VisualTreeHelper::GetChildrenCount(itemsRepeater);
        const winrt::ItemContainerUserInvokeMode canUserInvoke = IsItemInvokedEnabled() ?
            winrt::ItemContainerUserInvokeMode::Auto | winrt::ItemContainerUserInvokeMode::UserCanInvoke :
            winrt::ItemContainerUserInvokeMode::Auto | winrt::ItemContainerUserInvokeMode::UserCannotInvoke;

        for (int32_t childIndex = 0; childIndex < count; childIndex++)
        {
            auto elementAsDO = winrt::VisualTreeHelper::GetChild(itemsRepeater, childIndex);
            auto itemContainer = elementAsDO.try_as<winrt::ItemContainer>();

            if (itemContainer != nullptr)
            {
#ifdef MUX_PRERELEASE
                if (static_cast<int>(itemContainer.CanUserInvoke() & winrt::ItemContainerUserInvokeMode::Auto))
                {
                    itemContainer.CanUserInvoke(canUserInvoke);
                }
#else
                if (const auto& itemContainerImpl = winrt::get_self<ItemContainer>(itemContainer))
                {
                    if (static_cast<int>(itemContainerImpl->CanUserInvokeInternal() & winrt::ItemContainerUserInvokeMode::Auto))
                    {
                        itemContainerImpl->CanUserInvokeInternal(canUserInvoke);
                    }
                }
#endif
            }
        }
    }
}

void ItemsView::OnItemTemplateChanged()
{
    ITEMSVIEW_TRACE_INFO(*this, TRACE_MSG_METH, METH_NAME, this);

    // Make sure the default ItemTemplate is used when the ItemsSource is non-null and the ItemTemplate is null.
    EnsureItemTemplate();
}

void ItemsView::OnSelectionModeChanged()
{
    ITEMSVIEW_TRACE_VERBOSE(*this, TRACE_MSG_METH, METH_NAME, this);

    UpdateSelector();

    // For all ItemContainer children, update the MultiSelectMode and CanUserSelect properties according to the new ItemsView.SelectionMode property.
    if (auto const& itemsRepeater = m_itemsRepeater.get())
    {
        const auto count = winrt::VisualTreeHelper::GetChildrenCount(itemsRepeater);
        const auto selectionMode = SelectionMode();
        winrt::ItemContainerMultiSelectMode multiSelectMode = winrt::ItemContainerMultiSelectMode::Auto;

        switch (selectionMode)
        {
            case winrt::ItemsViewSelectionMode::None:
            case winrt::ItemsViewSelectionMode::Single:
                multiSelectMode |= winrt::ItemContainerMultiSelectMode::Single;
                break;
            case winrt::ItemsViewSelectionMode::Extended:
                multiSelectMode |= winrt::ItemContainerMultiSelectMode::Extended;
                break;
            case winrt::ItemsViewSelectionMode::Multiple:
                multiSelectMode |= winrt::ItemContainerMultiSelectMode::Multiple;
                break;
        }

        const winrt::ItemContainerUserSelectMode canUserSelect = selectionMode == winrt::ItemsViewSelectionMode::None ?
            winrt::ItemContainerUserSelectMode::Auto | winrt::ItemContainerUserSelectMode::UserCannotSelect :
            winrt::ItemContainerUserSelectMode::Auto | winrt::ItemContainerUserSelectMode::UserCanSelect;

        for (int32_t childIndex = 0; childIndex < count; childIndex++)
        {
            auto elementAsDO = winrt::VisualTreeHelper::GetChild(itemsRepeater, childIndex);
            auto itemContainer = elementAsDO.try_as<winrt::ItemContainer>();

            if (itemContainer != nullptr)
            {
#ifdef MUX_PRERELEASE
                if (static_cast<int>(itemContainer.MultiSelectMode() & winrt::ItemContainerMultiSelectMode::Auto))
                {
                    itemContainer.MultiSelectMode(multiSelectMode);
                }

                if (static_cast<int>(itemContainer.CanUserSelect() & winrt::ItemContainerUserSelectMode::Auto))
                {
                    itemContainer.CanUserSelect(canUserSelect);
                }
#else
                if (const auto& itemContainerImpl = winrt::get_self<ItemContainer>(itemContainer))
                {
                    if (static_cast<int>(itemContainerImpl->MultiSelectModeInternal() & winrt::ItemContainerMultiSelectMode::Auto))
                    {
                        itemContainerImpl->MultiSelectModeInternal(multiSelectMode);
                    }

                    if (static_cast<int>(itemContainerImpl->CanUserSelectInternal() & winrt::ItemContainerUserSelectMode::Auto))
                    {
                        itemContainerImpl->CanUserSelectInternal(canUserSelect);
                    }
                }
#endif
            }
        }
    }
}

void ItemsView::OnSelectionModelSelectionChanged(
    const winrt::SelectionModel& selectionModel,
    const winrt::SelectionModelSelectionChangedEventArgs& args)
{
    ITEMSVIEW_TRACE_VERBOSE(*this, TRACE_MSG_METH, METH_NAME, this);

    // Update ItemsView properties
    SelectedItem(m_selectionModel.SelectedItem());

    // For all ItemContainer children, update the IsSelected property according to the new ItemsView.SelectionModel property.
    if (auto const& itemsRepeater = m_itemsRepeater.get())
    {
        const auto count = winrt::VisualTreeHelper::GetChildrenCount(itemsRepeater);

        for (int32_t childIndex = 0; childIndex < count; childIndex++)
        {
            auto elementAsDO = winrt::VisualTreeHelper::GetChild(itemsRepeater, childIndex);
            auto itemContainer = elementAsDO.try_as<winrt::ItemContainer>();

            if (itemContainer != nullptr)
            {
                const int32_t itemIndex = itemsRepeater.GetElementIndex(itemContainer);

                if (itemIndex >= 0)
                {
                    const winrt::IReference<bool> isItemContainerSelected = m_selectionModel.IsSelected(itemIndex);
                    const bool isSelected = isItemContainerSelected != nullptr && isItemContainerSelected.Value();

                    if (isSelected)
                    {
#ifdef MUX_PRERELEASE
                        const winrt::ItemContainerUserSelectMode canUserSelect = itemContainer.CanUserSelect();
#else
                        const winrt::ItemContainerUserSelectMode canUserSelect = winrt::get_self<ItemContainer>(itemContainer)->CanUserSelectInternal();
#endif

                        if (!m_isProcessingInteraction || !static_cast<int>(canUserSelect & winrt::ItemContainerUserSelectMode::UserCannotSelect))
                        {
                            itemContainer.IsSelected(true);
                        }
                        else
                        {
                            // Processing a user interaction while ItemContainer.CanUserSelect is ItemContainerUserSelectMode.UserCannotSelect. Deselect that item
                            // in the selection model without touching the potential anchor.
                            m_selector->DeselectWithAnchorPreservation(itemIndex);
                        }
                    }
                    else
                    {
                        itemContainer.IsSelected(false);
                    }
                }
            }
        }
    }

    RaiseSelectionChanged();
}

// Raised when the current element index changed.
void ItemsView::OnCurrentElementSelectionModelSelectionChanged(
    const winrt::SelectionModel& selectionModel,
    const winrt::SelectionModelSelectionChangedEventArgs& args)
{
    const int currentElementIndex = GetCurrentElementIndex();

    ITEMSVIEW_TRACE_VERBOSE(*this, TRACE_MSG_METH_INT, METH_NAME, this, currentElementIndex);

    CurrentItemIndex(currentElementIndex);
}

// Raised when the ItemsSource collection changed.
void ItemsView::OnSourceListChanged(
    const winrt::IInspectable& dataSource,
    const winrt::NotifyCollectionChangedEventArgs& args)
{
    m_keyboardNavigationReferenceResetPending = true;

    if (const auto itemsRepeater = m_itemsRepeater.get())
    {
        if (const auto itemsSourceView = itemsRepeater.ItemsSourceView())
        {
            const auto count = itemsSourceView.Count();

            for (auto index = 0; index < count; index++)
            {
                if (const auto element = itemsRepeater.TryGetElement(index))
                {
                    element.SetValue(winrt::AutomationProperties::SizeOfSetProperty(), box_value(count));
                }
            }
        }
    }
}

void ItemsView::OnLoaded(
    const winrt::IInspectable& /*sender*/,
    const winrt::RoutedEventArgs& /*args*/)
{
    if (m_setVerticalScrollControllerOnLoaded)
    {
        // First occurrence of the Loaded event after template
        // application. Now that the inner ScrollView and ScrollPresenter
        // are loaded, cache their original vertical scroll
        // controller visibility and value for potential restoration.
        m_setVerticalScrollControllerOnLoaded = false;

        CacheOriginalVerticalScrollControllerAndVisibility();

        // Either push the VerticalScrollController value already set
        // to the inner control or set VerticalScrollController to the
        // inner control's value.
        SetVerticalScrollControllerOnLoaded();
    }
}

void ItemsView::OnUnloaded(
    const winrt::IInspectable& /*sender*/,
    const winrt::RoutedEventArgs& /*args*/)
{
    ITEMSVIEW_TRACE_VERBOSE(*this, TRACE_MSG_METH, METH_NAME, this);

    if (!IsLoaded())
    {
        UnhookCompositionTargetRendering();

        ITEMSVIEW_TRACE_INFO_DBG(*this, TRACE_MSG_METH_STR, METH_NAME, this, L"Keyboard navigation fields reset.");

        m_navigationKeyBringIntoViewPendingCount = 0;
        m_navigationKeyBringIntoViewCorrelationId = -1;
        m_navigationKeyProcessingCountdown = 0;
        m_navigationKeysToProcess.clear();

        ITEMSVIEW_TRACE_INFO_DBG(*this, TRACE_MSG_METH_STR, METH_NAME, this, L"Bring-into-view fields reset.");

        m_bringIntoViewElement.set(nullptr);
        m_bringIntoViewCorrelationId = -1;
        m_bringIntoViewElementRetentionCountdown = 0;
    }
}

// When isForTopLeftItem is True, returns the top/left focusable item in the viewport. Returns the bottom/right item instead.
// Fully displayed items are favored over partially displayed ones. 
int ItemsView::GetCornerFocusableItem(
    bool isForTopLeftItem)
{
    // When FlowDirection is FlowDirection::RightToLeft, the top/right item is returned instead of top/left (and bottom/left instead of bottom/right).
    // GetItemInternal's input is unchanged as it handles FlowDirection itself.

    const winrt::IndexBasedLayoutOrientation indexBasedLayoutOrientation = GetLayoutIndexBasedLayoutOrientation();
    const int itemIndex = GetItemInternal(
        isForTopLeftItem ? 0.0 : 1.0 /*horizontalViewportRatio*/,
        isForTopLeftItem ? 0.0 : 1.0 /*verticalViewportRatio*/,
        indexBasedLayoutOrientation == winrt::IndexBasedLayoutOrientation::TopToBottom /*isHorizontalDistanceFavored*/,
        indexBasedLayoutOrientation == winrt::IndexBasedLayoutOrientation::LeftToRight /*isVerticalDistanceFavored*/,
        false /*useKeyboardNavigationReferenceHorizontalOffset*/,
        false /*useKeyboardNavigationReferenceVerticalOffset*/,
        true /*capItemEdgesToViewportRatioEdges*/,
        true /*forFocusableItemsOnly*/);

    ITEMSVIEW_TRACE_INFO(*this, TRACE_MSG_METH_INT, METH_NAME, this, itemIndex);

    return itemIndex;
}

int ItemsView::GetElementIndex(
    const winrt::UIElement& element) const
{
    MUX_ASSERT(element != nullptr);

    if (auto const& itemsRepeater = m_itemsRepeater.get())
    {
        return itemsRepeater.GetElementIndex(element);
    }

    return -1;
}

winrt::IndexPath ItemsView::GetElementIndexPath(
    const winrt::UIElement& element,
    bool* isValid) const
{
    MUX_ASSERT(element != nullptr);

    const int index = GetElementIndex(element);

    if (isValid)
    {
        *isValid = index >= 0;
    }

    return winrt::IndexPath::CreateFrom(index);
}

winrt::IInspectable ItemsView::GetElementItem(
    const winrt::UIElement& element,
    bool* valueReturned) const
{
    if (valueReturned)
    {
        *valueReturned = false;
    }

    if (auto const& itemsRepeater = m_itemsRepeater.get())
    {
        if (auto const& itemsSourceView = itemsRepeater.ItemsSourceView())
        {
            const int itemIndex = GetElementIndex(element);

            if (itemIndex >= 0 && itemsSourceView.Count() > itemIndex)
            {
                if (valueReturned)
                {
                    *valueReturned = true;
                }
                return itemsSourceView.GetAt(itemIndex);
            }
        }
    }

    return nullptr;
}

// isHorizontalDistanceFavored==True:
//  - Means distance on the horizontal axis supercedes the one on the vertical axis. i.e. the vertical distance is only considered when the horizontal distance is identical.
// isVerticalDistanceFavored==True:
//  - Means distance on the vertical axis supercedes the one on the horizontal axis. i.e. the horizontal distance is only considered when the vertical distance is identical.
// useKeyboardNavigationReferenceHorizontalOffset:
//  - Instead of using horizontalViewportRatio, m_keyboardNavigationReferenceIndex/m_keyboardNavigationReferenceRect define a target vertical line. The distance from the middle
//    of the items to that line is considered.
// useKeyboardNavigationReferenceVerticalOffset:
//  - Instead of using verticalViewportRatio, m_keyboardNavigationReferenceIndex/m_keyboardNavigationReferenceRect define a target horizontal line. The distance from the middle
//    of the items to that line is considered.
// capItemEdgesToViewportRatioEdges==False:
//  - Means
//    - horizontalViewportRatio <= 0.5: find item with left edge closest to viewport ratio edge
//    - horizontalViewportRatio > 0.5: find item with right edge closest to viewport ratio edge
//    - verticalViewportRatio <= 0.5: find item with top edge closest to viewport ratio edge
//    - verticalViewportRatio > 0.5: find item with bottom edge closest to viewport ratio edge
// capItemEdgesToViewportRatioEdges==True:
//  - Means that the two item edges used for distance measurements must be closer to the center of the viewport than viewport ratio edges.
//  - Used for PageUp/PageDown operations which respectively look for items with their near/far edge on the viewport center side of the viewport ratio edges.
//  - Additional restrictions compared to capItemEdgesToViewportRatioEdges==False above:
//    - horizontalViewportRatio <= 0.5: item left edge larger than viewport ratio edge
//    - horizontalViewportRatio > 0.5: item right edge smaller than viewport ratio edge
//    - verticalViewportRatio <= 0.5: item top edge larger than viewport ratio edge
//    - verticalViewportRatio > 0.5: item bottom edge smaller than viewport ratio edge
// Returns -1 ItemsRepeater or ScrollView part are null, or the data source is empty, or when no item fulfills the restrictions imposed by capItemEdgesToViewportRatioEdges and/or forFocusableItemsOnly.
int ItemsView::GetItemInternal(
    double horizontalViewportRatio,
    double verticalViewportRatio,
    bool isHorizontalDistanceFavored,
    bool isVerticalDistanceFavored,
    bool useKeyboardNavigationReferenceHorizontalOffset,
    bool useKeyboardNavigationReferenceVerticalOffset,
    bool capItemEdgesToViewportRatioEdges,
    bool forFocusableItemsOnly)
{
    ITEMSVIEW_TRACE_INFO(*this, TRACE_MSG_METH_DBL_DBL, METH_NAME, this, horizontalViewportRatio, verticalViewportRatio);
    ITEMSVIEW_TRACE_INFO(*this, TRACE_MSG_METH_INT_INT, METH_NAME, this, isHorizontalDistanceFavored, isVerticalDistanceFavored);
    ITEMSVIEW_TRACE_INFO(*this, TRACE_MSG_METH_INT_INT, METH_NAME, this, capItemEdgesToViewportRatioEdges, forFocusableItemsOnly);

    MUX_ASSERT(!isHorizontalDistanceFavored || !isVerticalDistanceFavored);
    MUX_ASSERT(!useKeyboardNavigationReferenceHorizontalOffset || horizontalViewportRatio == 0.0);
    MUX_ASSERT(!useKeyboardNavigationReferenceVerticalOffset || verticalViewportRatio == 0.0);

    if (auto const& itemsRepeater = m_itemsRepeater.get())
    {
        if (auto const& scrollView = m_scrollView.get())
        {
            const float zoomFactor = scrollView.ZoomFactor();
            bool useHorizontalItemNearEdge{ false };
            bool useVerticalItemNearEdge{ false };
            double horizontalTarget{};
            double verticalTarget{};

            if (!useKeyboardNavigationReferenceHorizontalOffset)
            {
                const double horizontalScrollOffset = scrollView.HorizontalOffset();
                const double viewportWidth = scrollView.ViewportWidth();
                const double horizontalViewportOffset = horizontalViewportRatio * viewportWidth;
                const double horizontalExtent = scrollView.ExtentWidth() * static_cast<double>(zoomFactor);

                horizontalTarget = horizontalScrollOffset + horizontalViewportOffset;
                horizontalTarget = std::max(0.0, horizontalTarget);
                horizontalTarget = std::min(horizontalExtent, horizontalTarget);

                useHorizontalItemNearEdge = horizontalViewportRatio <= 0.5;
            }

            if (!useKeyboardNavigationReferenceVerticalOffset)
            {
                const double verticalScrollOffset = scrollView.VerticalOffset();
                const double viewportHeight = scrollView.ViewportHeight();
                const double verticalViewportOffset = verticalViewportRatio * viewportHeight;
                const double verticalExtent = scrollView.ExtentHeight() * static_cast<double>(zoomFactor);

                verticalTarget = verticalScrollOffset + verticalViewportOffset;
                verticalTarget = std::max(0.0, verticalTarget);
                verticalTarget = std::min(verticalExtent, verticalTarget);

                useVerticalItemNearEdge = verticalViewportRatio <= 0.5;
            }

            float keyboardNavigationReferenceOffset{ -1.0f };

            if (useKeyboardNavigationReferenceHorizontalOffset || useKeyboardNavigationReferenceVerticalOffset)
            {
                const winrt::Point keyboardNavigationReferenceOffsetPoint = GetUpdatedKeyboardNavigationReferenceOffset();
                keyboardNavigationReferenceOffset = useKeyboardNavigationReferenceHorizontalOffset ? keyboardNavigationReferenceOffsetPoint.X : keyboardNavigationReferenceOffsetPoint.Y;

                MUX_ASSERT(keyboardNavigationReferenceOffset != -1.0f);
            }

            const double roundingScaleFactor = GetRoundingScaleFactor(scrollView);
            const int childrenCount = winrt::VisualTreeHelper::GetChildrenCount(itemsRepeater);
            winrt::UIElement closestElement = nullptr;
            double smallestFavoredDistance = std::numeric_limits<double>::max();
            double smallestUnfavoredDistance = std::numeric_limits<double>::max();

            for (int childIndex = 0; childIndex < childrenCount; childIndex++)
            {
                auto const& elementAsDO = winrt::VisualTreeHelper::GetChild(itemsRepeater, childIndex);
                auto const& element = elementAsDO.try_as<winrt::UIElement>();

                if (element != nullptr)
                {
                    if (forFocusableItemsOnly && !IsFocusableElement(element))
                    {
                        continue;
                    }

                    const winrt::Rect elementRect = GetElementRect(element, itemsRepeater);
                    const winrt::Rect elementZoomedRect{ elementRect.X * zoomFactor, elementRect.Y * zoomFactor, elementRect.Width * zoomFactor, elementRect.Height * zoomFactor };
                    double signedHorizontalDistance{};
                    double signedVerticalDistance{};

                    if (useKeyboardNavigationReferenceHorizontalOffset)
                    {
                        signedHorizontalDistance = elementZoomedRect.X + elementZoomedRect.Width / 2.0f - keyboardNavigationReferenceOffset * zoomFactor;
                    }
                    else
                    {
                        signedHorizontalDistance = useHorizontalItemNearEdge ? elementZoomedRect.X - horizontalTarget : horizontalTarget - elementZoomedRect.X - elementZoomedRect.Width;

                        if (capItemEdgesToViewportRatioEdges && signedHorizontalDistance < -1.0 / roundingScaleFactor)
                        {
                            continue;
                        }
                    }

                    if (useKeyboardNavigationReferenceVerticalOffset)
                    {
                        signedVerticalDistance = elementZoomedRect.Y + elementZoomedRect.Height / 2.0f - keyboardNavigationReferenceOffset * zoomFactor;
                    }
                    else
                    {
                        signedVerticalDistance = useVerticalItemNearEdge ? elementZoomedRect.Y - verticalTarget : verticalTarget - elementZoomedRect.Y - elementZoomedRect.Height;

                        if (capItemEdgesToViewportRatioEdges && signedVerticalDistance < -1.0 / roundingScaleFactor)
                        {
                            continue;
                        }
                    }

                    double horizontalDistance = std::abs(signedHorizontalDistance);
                    double verticalDistance = std::abs(signedVerticalDistance);

                    if (!isHorizontalDistanceFavored && !isVerticalDistanceFavored)
                    {
                        horizontalDistance = std::pow(horizontalDistance, 2.0);
                        verticalDistance = std::pow(verticalDistance, 2.0);

                        smallestUnfavoredDistance = 0.0;

                        if (horizontalDistance + verticalDistance < smallestFavoredDistance)
                        {
                            smallestFavoredDistance = horizontalDistance + verticalDistance;
                            closestElement = element;
                        }
                    }
                    else if (isHorizontalDistanceFavored)
                    {
                        if (horizontalDistance < smallestFavoredDistance)
                        {
                            smallestFavoredDistance = horizontalDistance;
                            smallestUnfavoredDistance = verticalDistance;
                            closestElement = element;
                        }
                        else if (horizontalDistance == smallestFavoredDistance && verticalDistance < smallestUnfavoredDistance)
                        {
                            smallestUnfavoredDistance = verticalDistance;
                            closestElement = element;
                        }
                    }
                    else
                    {
                        MUX_ASSERT(isVerticalDistanceFavored);

                        if (verticalDistance < smallestFavoredDistance)
                        {
                            smallestFavoredDistance = verticalDistance;
                            smallestUnfavoredDistance = horizontalDistance;
                            closestElement = element;
                        }
                        else if (verticalDistance == smallestFavoredDistance && horizontalDistance < smallestUnfavoredDistance)
                        {
                            smallestUnfavoredDistance = horizontalDistance;
                            closestElement = element;
                        }
                    }

                    if (smallestFavoredDistance == 0.0 && smallestUnfavoredDistance == 0.0)
                    {
                        break;
                    }
                }
            }

            return closestElement == nullptr ? -1 : itemsRepeater.GetElementIndex(closestElement);
        }
    }

    return -1;
}

double ItemsView::GetRoundingScaleFactor(
    const winrt::UIElement& xamlRootReference) const
{
    if (auto const& xamlRoot = xamlRootReference.XamlRoot())
    {
        return static_cast<float>(xamlRoot.RasterizationScale());
    }
    return 1.0; // Assuming a 1.0 scale factor when no XamlRoot is available at the moment.
}

bool ItemsView::SetCurrentElementIndex(
    int index,
    winrt::FocusState focusState,
    bool forceKeyboardNavigationReferenceReset,
    bool startBringIntoView,
    bool expectBringIntoView)
{
    const int currentElementIndex = GetCurrentElementIndex();

    if (index != currentElementIndex)
    {
        ITEMSVIEW_TRACE_INFO_DBG(*this, TRACE_MSG_METH_STR_INT, METH_NAME, this, L"current element index", currentElementIndex);
        ITEMSVIEW_TRACE_INFO_DBG(*this, TRACE_MSG_METH_STR_INT, METH_NAME, this, L"index", index);

        if (index == -1)
        {
            m_currentElementSelectionModel.ClearSelection();
        }
        else
        {
            m_currentElementSelectionModel.Select(index);
        }

        if (index == -1 || m_keyboardNavigationReferenceRect.X == -1.0 || forceKeyboardNavigationReferenceReset)
        {
            UpdateKeyboardNavigationReference();
        }
    }

    return SetFocusElementIndex(index, focusState, startBringIntoView, expectBringIntoView);
}

// Returns True when a bring-into-view operation was initiated, and False otherwise. 
bool ItemsView::StartBringItemIntoViewInternal(
    bool throwOutOfBounds,
    bool throwOnAnyFailure,
    int32_t index,
    const winrt::BringIntoViewOptions& options)
{
#ifdef DBG
    ITEMSVIEW_TRACE_INFO(*this, TRACE_MSG_METH_STR_INT, METH_NAME, this, L"index", index);

    if (options != nullptr)
    {
        ITEMSVIEW_TRACE_INFO(*this, L"%s[0x%p](AnimationDesired:%d, H/V AlignmentRatio:%lf,%lf, H/V Offset:%f,%f, TargetRect:%s)\n",
            METH_NAME, this,
            options.AnimationDesired(),
            options.HorizontalAlignmentRatio(), options.VerticalAlignmentRatio(),
            options.HorizontalOffset(), options.VerticalOffset(),
            options.TargetRect() == nullptr ? L"null" : TypeLogging::RectToString(options.TargetRect().Value()).c_str());
    }
#endif

    auto const& itemsRepeater = m_itemsRepeater.get();

    if (itemsRepeater == nullptr)
    {
        // StartBringIntoView operation cannot be initiated without ItemsRepeater part.
        if (throwOnAnyFailure)
        {
            throw winrt::hresult_error(E_INVALID_OPERATION, s_missingItemsRepeaterPart);
        }

        return false;
    }

    bool isItemIndexValid = ValidateItemIndex(throwOutOfBounds || throwOnAnyFailure, index);

    if (!isItemIndexValid)
    {
        MUX_ASSERT(!throwOutOfBounds && !throwOnAnyFailure);

        return false;
    }

    // Reset the fields changed by any potential bring-into-view operation still in progress.
    CompleteStartBringItemIntoView();

    MUX_ASSERT(m_bringIntoViewElement == nullptr);
    MUX_ASSERT(m_bringIntoViewElementRetentionCountdown == 0);
    MUX_ASSERT(m_bringIntoViewCorrelationId == -1);
    MUX_ASSERT(m_scrollViewHorizontalAnchorRatio == -1);
    MUX_ASSERT(m_scrollViewVerticalAnchorRatio == -1);

    // Access or create the target element so its position within the ItemsRepeater can be evaluated.
    winrt::UIElement element = itemsRepeater.GetOrCreateElement(index);

    MUX_ASSERT(element != nullptr);

    const auto scrollView = m_scrollView.get();

    // During the initial position evaluation, scroll anchoring is turned off to avoid shifting offsets
    // which may result in an incorrect final scroll offset.
    if (scrollView)
    {
        // Current anchoring settings are recorded for post-operation restoration.
        m_scrollViewHorizontalAnchorRatio = scrollView.HorizontalAnchorRatio();
        m_scrollViewVerticalAnchorRatio = scrollView.VerticalAnchorRatio();

        scrollView.HorizontalAnchorRatio(DoubleUtil::NaN);
        scrollView.VerticalAnchorRatio(DoubleUtil::NaN);
    }

    ITEMSVIEW_TRACE_VERBOSE_DBG(*this, TRACE_MSG_METH_METH, METH_NAME, this, L"UIElement::UpdateLayout");

    // Ensure the item is given a valid position within the ItemsRepeater. It will determine the target scroll offset.
    element.UpdateLayout();

    // Make sure that the target index is still valid after the UpdateLayout call.
    isItemIndexValid = ValidateItemIndex(false /*throwIfInvalid*/, index);

    if (!isItemIndexValid)
    {
        // Restore scrollView.HorizontalAnchorRatio/VerticalAnchorRatio properties with m_scrollViewHorizontalAnchorRatio/m_scrollViewVerticalAnchorRatio cached values.
        CompleteStartBringItemIntoView();

        // StartBringIntoView operation cannot be initiated without ItemsRepeater part or in-bounds item index.
        if (throwOnAnyFailure)
        {
            throw winrt::hresult_error(E_INVALID_OPERATION, s_indexOutOfBounds);
        }
        return false;
    }

    if (scrollView)
    {
        // Turn on scroll anchoring during and after the scroll operation so target repositionings have no effect on the final visual.
        // The value 0.5 is used rather than 0.0 or 1.0 to avoid near and far edge anchoring which only take effect when the content
        // hits the viewport boundary. With any value between 0 and 1, anchoring is also effective in those extreme cases. 
        constexpr double anchorRatio = 0.5;

        scrollView.HorizontalAnchorRatio(anchorRatio);
        scrollView.VerticalAnchorRatio(anchorRatio);
    }

    // Access the element's index to account for the rare event where it was recycled during the layout.
    if (index != GetElementIndex(element))
    {
        // Access the target element which was created during the layout pass. Its position is already set.
        element = itemsRepeater.GetOrCreateElement(index);

        if (GetElementIndex(element) == -1)
        {
            // This situation arises when the ItemsView is not parented.
            // Restore the ScrollView's HorizontalAnchorRatio/VerticalAnchorRatio properties.
            CompleteStartBringItemIntoView();

            if (throwOnAnyFailure)
            {
                throw winrt::hresult_error(E_INVALID_OPERATION, s_itemsViewNotParented);
            }

            return false;
        }

        MUX_ASSERT(index == GetElementIndex(element));

        // Make sure animation is turned off in this rare case.
        if (options != nullptr)
        {
            options.AnimationDesired(false);
        }
    }

    m_bringIntoViewElement.set(element);

    // Trigger the actual bring-into-view scroll - it'll cause ItemsView::OnScrollViewBringingIntoView and ItemsView::OnScrollViewScrollCompleted calls.
    if (options == nullptr)
    {
        ITEMSVIEW_TRACE_INFO_DBG(*this, TRACE_MSG_METH_PTR_STR, METH_NAME, this, m_bringIntoViewElement.get(), L"m_bringIntoViewElement set. Invoking UIElement.StartBringIntoView without options.");

        element.StartBringIntoView();
    }
    else
    {
        ITEMSVIEW_TRACE_INFO_DBG(*this, TRACE_MSG_METH_PTR_STR, METH_NAME, this, m_bringIntoViewElement.get(), L"m_bringIntoViewElement set. Invoking UIElement.StartBringIntoView with options.");

        element.StartBringIntoView(options);
    }

    return true;
}

winrt::UIElement ItemsView::TryGetElement(int index) const
{
    if (auto const& itemsRepeater = m_itemsRepeater.get())
    {
        return itemsRepeater.TryGetElement(index);
    }

    return nullptr;
}

void ItemsView::SetItemsViewItemContainerRevokers(
    const winrt::ItemContainer& itemContainer)
{
    auto itemContainerRevokers = winrt::make_self<ItemContainerRevokers>();

    itemContainerRevokers->m_keyDownRevoker = itemContainer.KeyDown(
        winrt::auto_revoke,
        { this, &ItemsView::OnItemsViewElementKeyDown });

    itemContainerRevokers->m_gettingFocusRevoker = itemContainer.GettingFocus(
        winrt::auto_revoke,
        { this, &ItemsView::OnItemsViewElementGettingFocus });

#ifdef DBG
    itemContainerRevokers->m_losingFocusRevoker = itemContainer.LosingFocus(
        winrt::auto_revoke,
        { this, &ItemsView::OnItemsViewElementLosingFocusDbg });
#endif

    itemContainerRevokers->m_itemInvokedRevoker = itemContainer.ItemInvoked(
        winrt::auto_revoke,
        { this, &ItemsView::OnItemsViewItemContainerItemInvoked });

    itemContainerRevokers->m_isSelectedPropertyChangedRevoker =
        RegisterPropertyChanged(itemContainer, winrt::ItemContainer::IsSelectedProperty(), { this, &ItemsView::OnItemsViewItemContainerIsSelectedChanged });

#ifdef DBG_VERBOSE
    itemContainerRevokers->m_sizeChangedRevokerDbg = itemContainer.SizeChanged(
        winrt::auto_revoke,
        { this, &ItemsView::OnItemsViewItemContainerSizeChangedDbg });
#endif

    itemContainer.SetValue(s_ItemsViewItemContainerRevokersProperty, itemContainerRevokers.as<winrt::IInspectable>());

    m_itemContainersWithRevokers.insert(itemContainer);
}

void ItemsView::ClearItemsViewItemContainerRevokers(
    const winrt::ItemContainer& itemContainer)
{
    RevokeItemsViewItemContainerRevokers(itemContainer);
    itemContainer.SetValue(s_ItemsViewItemContainerRevokersProperty, nullptr);
    m_itemContainersWithRevokers.erase(itemContainer);
}

void ItemsView::ClearAllItemsViewItemContainerRevokers() noexcept
{
    for (const auto& itemContainer : m_itemContainersWithRevokers)
    {
        // ClearAllItemsViewItemContainerRevokers is only called in the destructor, where exceptions cannot be thrown.
        // If the associated ItemsView items have not yet been cleaned up, we must detach these revokers or risk a call into freed
        // memory being made.  However if they have been cleaned up these calls will throw. In this case we can ignore
        // those exceptions.
        try
        {
            RevokeItemsViewItemContainerRevokers(itemContainer);
            itemContainer.SetValue(s_ItemsViewItemContainerRevokersProperty, nullptr);
        }
        catch (...)
        {
        }
    }
    m_itemContainersWithRevokers.clear();
}

void ItemsView::RevokeItemsViewItemContainerRevokers(
    const winrt::ItemContainer& itemContainer)
{
    if (auto const revokers = itemContainer.GetValue(s_ItemsViewItemContainerRevokersProperty))
    {
        if (auto const itemContainerRevokers = revokers.try_as<ItemContainerRevokers>())
        {
            itemContainerRevokers->RevokeAll(itemContainer);
        }
    }
}

bool ItemsView::ValidateItemIndex(
    bool throwIfInvalid,
    int index) const
{
    if (auto const& itemsRepeater = m_itemsRepeater.get())
    {
        if (itemsRepeater.ItemsSourceView() == nullptr)
        {
            if (throwIfInvalid)
            {
                throw winrt::hresult_error(E_INVALID_OPERATION, s_itemsSourceNull);
            }
            return false;
        }

        if (index < 0 || index >= itemsRepeater.ItemsSourceView().Count())
        {
            if (throwIfInvalid)
            {
                throw winrt::hresult_out_of_bounds(s_indexOutOfBounds);
            }
            return false;
        }

        return true;
    }

    return false;
}

// Invoked by ItemsViewTestHooks
winrt::Point ItemsView::GetKeyboardNavigationReferenceOffset() const
{
    if (m_keyboardNavigationReferenceRect.X == -1.0f)
    {
        return winrt::Point{ -1.0f, -1.0f };
    }

    return winrt::Point{
        m_keyboardNavigationReferenceRect.X + m_keyboardNavigationReferenceRect.Width / 2.0f,
        m_keyboardNavigationReferenceRect.Y + m_keyboardNavigationReferenceRect.Height / 2.0f };
}

int ItemsView::GetCurrentElementIndex() const
{
    winrt::IndexPath currentElementIndexPath = m_currentElementSelectionModel.SelectedIndex();

    MUX_ASSERT(currentElementIndexPath == nullptr || currentElementIndexPath.GetSize() == 1);

    const int currentElementIndex = currentElementIndexPath == nullptr ? -1 : currentElementIndexPath.GetAt(0);

    return currentElementIndex;
}

winrt::ScrollView ItemsView::GetScrollViewPart() const
{
    return m_scrollView.get();
}

winrt::ItemsRepeater ItemsView::GetItemsRepeaterPart() const
{
    return m_itemsRepeater.get();
}

winrt::SelectionModel ItemsView::GetSelectionModel() const
{
    return m_selectionModel;
}

#ifdef DBG

winrt::hstring ItemsView::DependencyPropertyToStringDbg(
    const winrt::IDependencyProperty& dependencyProperty)
{
    if (dependencyProperty == s_ItemsSourceProperty)
    {
        return L"ItemsSource";
    }
    else if (dependencyProperty == s_ItemTemplateProperty)
    {
        return L"ItemTemplate";
    }
    else if (dependencyProperty == s_LayoutProperty)
    {
        return L"Layout";
    }
    else if (dependencyProperty == s_SelectionModeProperty)
    {
        return L"SelectionMode";
    }
    else if (dependencyProperty == s_ScrollViewProperty)
    {
        return L"ScrollView";
    }
    else
    {
        return L"UNKNOWN";
    }
}

#endif
