// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#pragma once

#include "ItemsViewTrace.h"
#include "ItemsView.g.h"
#include "ItemsView.properties.h"
#include "SelectorBase.h"
#include "PointerInfo.h"

class ItemsView :
    public ReferenceTracker<ItemsView, winrt::implementation::ItemsViewT>,
    public ItemsViewProperties
{
public:
    ItemsView();
    ~ItemsView();

    // Properties' default values.
    static constexpr winrt::ItemsViewSelectionMode s_defaultSelectionMode{ winrt::ItemsViewSelectionMode::Single };

#pragma region IItemsView

    winrt::ScrollView ScrollView() const;
    winrt::IVectorView<winrt::IInspectable> SelectedItems();

    bool TryGetItemIndex(double horizontalViewportRatio, double verticalViewportRatio, int& index);
    void StartBringItemIntoView(int32_t index, const winrt::BringIntoViewOptions& options);

    void Select(int itemIndex);
    void Deselect(int itemIndex);
    bool IsSelected(int itemIndex);
    void SelectAll();
    void DeselectAll();
    void InvertSelection();

#pragma endregion

    // Invoked by ItemsViewTestHooks
    int GetCurrentElementIndex() const;
    winrt::Point GetKeyboardNavigationReferenceOffset() const;
    winrt::ItemsRepeater GetItemsRepeaterPart() const;
    winrt::ScrollView GetScrollViewPart() const;
    winrt::SelectionModel GetSelectionModel() const;

    void OnPropertyChanged(const winrt::DependencyPropertyChangedEventArgs& args);

#ifdef DBG
    static winrt::hstring DependencyPropertyToStringDbg(const winrt::IDependencyProperty& dependencyProperty);
#endif

#pragma region IControlOverrides
#ifdef DBG
    void OnGotFocus(
        winrt::RoutedEventArgs const& args);
#endif
    void OnKeyDown(
        winrt::KeyRoutedEventArgs const& args);
#pragma endregion

#pragma region IFrameworkElementOverrides
    // unoverridden methods provided by FrameworkElementOverridesHelper
    void OnApplyTemplate(); // not actually final for 'derived' classes
#pragma endregion

#pragma region IUIElementOverrides
    winrt::AutomationPeer OnCreateAutomationPeer();
#pragma endregion

private:
    void EnsureItemTemplate();
    void UpdateItemsRepeater(
        const winrt::ItemsRepeater& itemsRepeater);
    void UpdateScrollView(
        const winrt::ScrollView& scrollView);
    void UpdateSelector();
    void UpdateKeyboardNavigationReference();

    void RaiseItemInvoked(
        const winrt::UIElement& element);

    void RaiseSelectionChanged();

    void HookItemsSourceViewEvents();
    void HookItemsRepeaterEvents();
    void UnhookItemsRepeaterEvents(
        bool isForDestructor);
    void HookScrollViewEvents();
    void UnhookScrollViewEvents(
        bool isForDestructor);

    void HookCompositionTargetRendering();
    void UnhookCompositionTargetRendering();

    bool ProcessInteraction(
        const winrt::UIElement& element,
        const winrt::FocusState& focusState);

    bool ProcessNavigationKeys();

    void QueueNavigationKey(
        const winrt::VirtualKey& key);

    void CompleteStartBringItemIntoView();

    void OnItemsRepeaterElementPrepared(
        const winrt::ItemsRepeater& itemsRepeater,
        const winrt::ItemsRepeaterElementPreparedEventArgs& args);

    void OnItemsRepeaterElementClearing(
        const winrt::ItemsRepeater& itemsRepeater,
        const winrt::ItemsRepeaterElementClearingEventArgs& args);

    void OnItemsRepeaterElementIndexChanged(
        const winrt::ItemsRepeater& itemsRepeater,
        const winrt::ItemsRepeaterElementIndexChangedEventArgs& args);

    void OnItemsRepeaterItemsSourceChanged(
        const winrt::DependencyObject& sender,
        const winrt::DependencyProperty& args);

    void OnItemsRepeaterLayoutUpdated(
        const winrt::IInspectable& sender,
        const winrt::IInspectable& args);

    void OnItemsRepeaterSizeChanged(
        const winrt::IInspectable& sender,
        const winrt::SizeChangedEventArgs& args);

    void OnScrollViewAnchorRequested(
        const winrt::ScrollView& scrollView,
        const winrt::ScrollingAnchorRequestedEventArgs& args);

    void OnScrollViewBringingIntoView(
        const winrt::ScrollView& scrollView,
        const winrt::ScrollingBringingIntoViewEventArgs& args);

#ifdef DBG
    void OnScrollViewExtentChangedDbg(
        const winrt::ScrollView& scrollView,
        const winrt::IInspectable& args);
#endif

    void OnScrollViewScrollCompleted(
        const winrt::ScrollView& scrollView,
        const winrt::ScrollingScrollCompletedEventArgs& args);

    void OnItemsViewElementGettingFocus(
        const winrt::UIElement& element,
        const winrt::GettingFocusEventArgs& args);

    void OnItemsViewElementKeyDown(
        const winrt::IInspectable& sender,
        const winrt::KeyRoutedEventArgs& args);

#ifdef DBG
    void OnItemsViewElementLosingFocusDbg(
        const winrt::UIElement& element,
        const winrt::LosingFocusEventArgs& args);
#endif

    void OnItemsViewItemContainerIsSelectedChanged(
        const winrt::DependencyObject& sender,
        const winrt::DependencyProperty& args);

    void OnItemsViewItemContainerItemInvoked(
        const winrt::ItemContainer& itemContainer,
        const winrt::ItemContainerInvokedEventArgs& args);

#ifdef DBG_VERBOSE
    void OnItemsViewItemContainerSizeChangedDbg(
        const winrt::IInspectable& sender,
        const winrt::SizeChangedEventArgs& args);
#endif

#ifdef DBG
    void OnLayoutMeasureInvalidatedDbg(
        const winrt::Layout& sender,
        const winrt::IInspectable&);

    void OnLayoutArrangeInvalidatedDbg(
        const winrt::Layout&,
        const winrt::IInspectable&);
#endif

    void OnCompositionTargetRendering(
        const winrt::IInspectable& sender,
        const winrt::IInspectable& args);

    void OnItemsSourceChanged();
#ifdef DBG
    void OnLayoutChangedDbg();
#endif
    void OnIsItemInvokedEnabledChanged();
    void OnItemTemplateChanged();
    void OnSelectionModeChanged();
    void OnVerticalScrollControllerChanged();

    void OnCurrentElementSelectionModelSelectionChanged(
        const winrt::SelectionModel& selectionModel,
        const winrt::SelectionModelSelectionChangedEventArgs& args);
    void OnSelectionModelSelectionChanged(
        const winrt::SelectionModel& selectionModel,
        const winrt::SelectionModelSelectionChangedEventArgs& args);
    void OnSourceListChanged(
        const winrt::IInspectable& dataSource,
        const winrt::NotifyCollectionChangedEventArgs& args);
    void OnLoaded(
        const winrt::IInspectable& sender,
        const winrt::RoutedEventArgs& args);
    void OnUnloaded(
        const winrt::IInspectable& sender,
        const winrt::RoutedEventArgs& args);

    void ApplySelectionModelSelectionChange();
    int GetAdjacentFocusableElementByDirection(
        const winrt::FocusNavigationDirection& focusNavigationDirection,
        bool hasIndexBasedLayoutOrientation);
    int GetAdjacentFocusableElementByIndex(
        bool getPreviousFocusableElement);
    int GetCornerFocusableItem(
        bool isForTopLeftItem);
    void GetDistanceToKeyboardNavigationReferenceOffset(
        const winrt::FocusNavigationDirection& focusNavigationDirection,
        const winrt::Rect& currentElementRect,
        const winrt::UIElement& element,
        const winrt::ItemsRepeater& itemsRepeater,
        float keyboardNavigationReferenceOffset,
        double roundingScaleFactor,
        float* navigationDirectionDistance,
        float* noneNavigationDirectionDistance) const;
    int GetElementIndex(
        const winrt::UIElement& element) const;
    winrt::IndexPath GetElementIndexPath(
        const winrt::UIElement& element,
        bool* isValid) const;
    winrt::IInspectable GetElementItem(
        const winrt::UIElement& element,
        bool* valueReturned) const;
    winrt::Rect GetElementRect(
        const winrt::UIElement& element,
        const winrt::ItemsRepeater& itemsRepeater) const;
    int GetItemInternal(
        double horizontalViewportRatio,
        double verticalViewportRatio,
        bool isHorizontalDistanceFavored,
        bool isVerticalDistanceFavored,
        bool useKeyboardNavigationReferenceHorizontalOffset,
        bool useKeyboardNavigationReferenceVerticalOffset,
        bool capItemEdgesToViewportRatioEdges,
        bool forFocusableItemsOnly);
    winrt::IndexBasedLayoutOrientation GetLayoutIndexBasedLayoutOrientation();
    double GetRoundingScaleFactor(
        const winrt::UIElement& xamlRootReference) const;
    int GetTrailingNavigationKeyCount() const;
    winrt::Point GetUpdatedKeyboardNavigationReferenceOffset();
    bool SetCurrentElementIndex(
        int index,
        winrt::FocusState focusState,
        bool forceKeyboardNavigationReferenceReset,
        bool startBringIntoView = false,
        bool expectBringIntoView = false);
    bool SetFocusElementIndex(
        int index,
        winrt::FocusState focusState,
        bool startBringIntoView = false,
        bool expectBringIntoView = false);
    bool StartBringItemIntoViewInternal(
        bool throwOutOfBounds,
        bool throwOnAnyFailure,
        int32_t index,
        const winrt::BringIntoViewOptions& options);
    winrt::UIElement TryGetElement(
        int index) const;

    void SetItemsViewItemContainerRevokers(
        const winrt::ItemContainer& itemContainer);
    void ClearItemsViewItemContainerRevokers(
        const winrt::ItemContainer& itemContainer);
    void ClearAllItemsViewItemContainerRevokers() noexcept;
    void RevokeItemsViewItemContainerRevokers(
        const winrt::ItemContainer& itemContainer);

    void CacheOriginalVerticalScrollControllerAndVisibility();
    void RestoreOriginalVerticalScrollControllerAndVisibility();
    void SetVerticalScrollControllerOnLoaded();
    void ApplyVerticalScrollController();

    bool AreNavigationKeysOpposite(
        const winrt::VirtualKey& key1,
        const winrt::VirtualKey& key2) const;

    bool CanRaiseItemInvoked(
        const winrt::ItemContainerInteractionTrigger& interactionTrigger,
        const winrt::ItemContainer& itemContainer);
    bool CanScrollVertically();

    bool IsCancelingNavigationKey(
        const winrt::VirtualKey& key,
        bool isRepeatKey);

    bool IsLayoutOrientationIndexBased(
        bool horizontal);

    bool IsNavigationKey(
        const winrt::VirtualKey& key) const;

    bool ValidateItemIndex(
        bool throwIfInvalid,
        int index) const;

private:
    static inline GlobalDependencyProperty s_ItemsViewItemContainerRevokersProperty{ nullptr };

    static constexpr std::wstring_view s_itemsRepeaterPartName{ L"PART_ItemsRepeater"sv };
    static constexpr std::wstring_view s_scrollViewPartName{ L"PART_ScrollView"sv };
    static constexpr std::wstring_view s_indexOutOfBounds{ L"Index is out of bounds."sv };
    static constexpr std::wstring_view s_invalidItemTemplateRoot{ L"ItemTemplate's root element must be an ItemContainer."sv };
    static constexpr std::wstring_view s_itemsSourceNull{ L"ItemsSource does not have a value."sv };
    static constexpr std::wstring_view s_itemsViewNotParented{ L"ItemsView is not parented."sv };
    static constexpr std::wstring_view s_missingItemsRepeaterPart{ L"ItemsRepeater part is not available."sv };

    // CorrelationId of the bring-into-view scroll resulting from a navigation key processing.
    int32_t m_navigationKeyBringIntoViewCorrelationId{ -1 };
    // Ticks count left before the next queued navigation key is processed,
    // after a navigation-key-induced bring-into-view scroll completed a large offset change.
    uint8_t m_navigationKeyProcessingCountdown{ 0 };
    // Incremented in SetFocusElementIndex when a navigation key processing causes a new item to get focus.
    // This will trigger an OnScrollViewBringingIntoView call where it is decremented.
    // Used to delay a navigation key processing until the content has settled on a new viewport.
    uint8_t m_navigationKeyBringIntoViewPendingCount{ 0 };
    // Caches the most recent navigation key processed.
    winrt::VirtualKey m_lastNavigationKeyProcessed{};

    // CorrelationId of the bring-into-view scroll resulting from a StartBringItemIntoView call.
    int32_t m_bringIntoViewCorrelationId{ -1 };
    // Ticks count left after bring-into-view scroll completed before the m_bringIntoViewElement
    // field is reset and no longer returned in OnScrollViewAnchorRequested.
    uint8_t m_bringIntoViewElementRetentionCountdown{ 0 };
    // ScrollView anchor ratios to restore after a StartBringItemIntoView operation completes.
    double m_scrollViewHorizontalAnchorRatio{ -1.0 };
    double m_scrollViewVerticalAnchorRatio{ -1.0 };

    // Set to True when a ResetKeyboardNavigationReference() call is delayed until the next ItemsView::OnItemsRepeaterLayoutUpdated occurrence.
    bool m_keyboardNavigationReferenceResetPending{ false };
    // Caches the element index used to define m_keyboardNavigationReferenceRect.
    int m_keyboardNavigationReferenceIndex{ -1 };
    // Bounds of the reference element for directional keyboard navigations.
    winrt::Rect m_keyboardNavigationReferenceRect{ -1.0f, -1.0f, -1.0f, -1.0f };

    // Set to True during a user action processing which updates selection.
    bool m_isProcessingInteraction{ false };

    bool m_setVerticalScrollControllerOnLoaded{ false };

    // Set to True in ItemsView::OnSelectionModelSelectionChanged to delay the application
    // of the selection changes until the imminent ItemsView::OnSourceListChanged call.
    bool m_applySelectionChangeOnSourceListChanged{ false };

    std::shared_ptr<SelectorBase> m_selector;

    winrt::Microsoft::UI::Xaml::Media::CompositionTarget::Rendering_revoker m_renderingRevoker{};
    winrt::SelectionModel::SelectionChanged_revoker m_selectionModelSelectionChangedRevoker{};
    winrt::SelectionModel::SelectionChanged_revoker m_currentElementSelectionModelSelectionChangedRevoker{};
    winrt::ItemsRepeater::ElementPrepared_revoker m_itemsRepeaterElementPreparedRevoker{};
    winrt::ItemsRepeater::ElementClearing_revoker m_itemsRepeaterElementClearingRevoker{};
    winrt::ItemsRepeater::ElementIndexChanged_revoker m_itemsRepeaterElementIndexChangedRevoker{};
    winrt::FrameworkElement::LayoutUpdated_revoker m_itemsRepeaterLayoutUpdatedRevoker{};
    winrt::FrameworkElement::SizeChanged_revoker m_itemsRepeaterSizeChangedRevoker{};
    winrt::FrameworkElement::Unloaded_revoker m_unloadedRevoker{};
    winrt::FrameworkElement::Loaded_revoker m_loadedRevoker{};
    winrt::ItemsSourceView::CollectionChanged_revoker m_itemsSourceViewChangedRevoker{};
#ifdef DBG
    winrt::Layout::MeasureInvalidated_revoker m_layoutMeasureInvalidatedDbg{};
    winrt::Layout::ArrangeInvalidated_revoker m_layoutArrangeInvalidatedDbg{};
#endif
    winrt::ScrollView::AnchorRequested_revoker m_scrollViewAnchorRequestedRevoker{};
    winrt::ScrollView::BringingIntoView_revoker m_scrollViewBringingIntoViewRevoker{};
    winrt::ScrollView::ScrollCompleted_revoker m_scrollViewScrollCompletedRevoker{};
#ifdef DBG
    winrt::ScrollView::ExtentChanged_revoker m_scrollViewExtentChangedRevokerDbg{};
#endif
    PropertyChanged_revoker m_itemsRepeaterItemsSourcePropertyChangedRevoker{};

    // Tracks selected elements.
    winrt::SelectionModel m_selectionModel{};
    // Tracks current element.
    winrt::SelectionModel m_currentElementSelectionModel{};

    // ScrollView's vertical scrollbar visibility to restore in the event VerticalScrollController gets assigned back to
    // its original value (m_originalVerticalScrollController read in OnApplyTemplate) after being set to a custom value.
    winrt::ScrollingScrollBarVisibility m_originalVerticalScrollBarVisibility{ winrt::ScrollingScrollBarVisibility::Auto };
    // Original VerticalScrollController read in OnApplyTemplate, which by default is the ScrollView's ScrollBarController.
    tracker_ref<winrt::IScrollController> m_originalVerticalScrollController{ this };

    tracker_ref<winrt::ItemsRepeater> m_itemsRepeater{ this };
    tracker_ref<winrt::ScrollView> m_scrollView{ this };
    tracker_ref<winrt::UIElement> m_bringIntoViewElement{ this };

    std::list<winrt::VirtualKey> m_navigationKeysToProcess;
    std::set<winrt::ItemContainer> m_itemContainersWithRevokers;
    std::map<winrt::ItemContainer, std::shared_ptr<PointerInfo<ItemsView>>> m_itemContainersPointerInfos;
};
