// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#pragma once

#include "AnimationManager.h"
#include "ViewManager.h"
#include "VirtualizationInfo.h"
#include "ItemsRepeaterElementPreparedEventArgs.h"
#include "ItemsRepeaterElementClearingEventArgs.h"
#include "ItemsRepeaterElementIndexChangedEventArgs.h"
#include "ItemsRepeater.g.h"
#include "ItemsRepeater.properties.h"
#include "ViewportManager.h"

class VirtualizationInfo;

class ItemsRepeater :
    public ReferenceTracker<ItemsRepeater, DeriveFromPanelHelper_base, winrt::ItemsRepeater, winrt::IItemsRepeater2>,
    public ItemsRepeaterProperties
{
public:
    ItemsRepeater();
    ~ItemsRepeater();

    // StackLayout measurements are shortcut when m_stackLayoutMeasureCounter reaches this value
    // to prevent a layout cycle exception.
    static constexpr uint8_t s_maxStackLayoutIterations = 60u;

    static winrt::Point ClearedElementsArrangePosition;
    // A convention we use in the ItemsRepeater codebase for an invalid Rect value.
    static winrt::Rect InvalidRect;

    using ItemsRepeaterProperties::Background;

#pragma region IUIElementOverrides

    winrt::AutomationPeer OnCreateAutomationPeer();

#pragma endregion

#pragma region IUIElementOverrides7

    winrt::IIterable<winrt::DependencyObject> GetChildrenInTabFocusOrder();

#pragma endregion

#pragma region IUIElementOverrides8

    void OnBringIntoViewRequested(winrt::BringIntoViewRequestedEventArgs const& e);

#pragma endregion

#pragma region IFrameworkElementOverrides

    winrt::Size MeasureOverride(winrt::Size const& availableSize);
    winrt::Size ArrangeOverride(winrt::Size const& finalSize);

#pragma endregion

#pragma region IRepeater interface.

    winrt::ItemsSourceView ItemsSourceView();

    // Mapping APIs
    int32_t GetElementIndex(winrt::UIElement const& element);
    winrt::UIElement TryGetElement(int index);
    winrt::UIElement GetOrCreateElement(int index);

#pragma endregion

    winrt::Microsoft::UI::Xaml::Controls::IElementFactoryShim ItemTemplateShim() { return m_itemTemplateWrapper; };
    ViewManager& ViewManager() { return m_viewManager; }
    AnimationManager& AnimationManager() { return m_animationManager; }

    winrt::UIElement GetElementImpl(int index, bool forceCreate, bool suppressAutoRecycle);
    void ClearElementImpl(const winrt::UIElement& element);

    // Mapping APIs (exception based)
    int GetElementIndexImpl(const winrt::UIElement& element);
    winrt::UIElement GetElementFromIndexImpl(int index);


    winrt::UIElement GetOrCreateElementImpl(int index);

    static winrt::com_ptr<VirtualizationInfo> TryGetVirtualizationInfo(const winrt::UIElement& element);
    static winrt::com_ptr<VirtualizationInfo> GetVirtualizationInfo(const winrt::UIElement& element);
    static winrt::com_ptr<VirtualizationInfo> CreateAndInitializeVirtualizationInfo(const winrt::UIElement& element);

    winrt::IInspectable LayoutState() const { return m_layoutState.get(); }
    void LayoutState(const winrt::IInspectable& value) { m_layoutState.set(value); }
    winrt::Rect VisibleWindow() const { return m_viewportManager->GetLayoutVisibleWindow(); }
    winrt::Rect RealizationWindow() const { return m_viewportManager->GetLayoutRealizationWindow(); }
    winrt::UIElement SuggestedAnchor() const { return m_viewportManager->SuggestedAnchor(); }
    winrt::UIElement MadeAnchor() const { return m_viewportManager->MadeAnchor(); }
    winrt::Point LayoutOrigin() const { return m_layoutOrigin; }
    void LayoutOrigin(winrt::Point value) { m_layoutOrigin = value; }

    // Pinning APIs
    void PinElement(winrt::UIElement const& element);
    void UnpinElement(winrt::UIElement const& element);

    void OnPropertyChanged(const winrt::DependencyPropertyChangedEventArgs& args);

    void OnElementPrepared(const winrt::UIElement& element, int index);
    void OnElementClearing(const winrt::UIElement& element);
    void OnElementIndexChanged(const winrt::UIElement& element, int oldIndex, int newIndex);

    static winrt::DependencyProperty GetVirtualizationInfoProperty()
    {
        static GlobalDependencyProperty s_VirtualizationInfoProperty =
            InitializeDependencyProperty(
                L"VirtualizationInfo",
                winrt::name_of<winrt::IInspectable>(),
                winrt::name_of<winrt::ItemsRepeater>(),
                true /* isAttached */,
                nullptr /* defaultValue */);

        return s_VirtualizationInfoProperty;
    }

    int Indent();

private:
    void OnLoaded(const winrt::IInspectable& /*sender*/, const winrt::RoutedEventArgs& /*args*/);
    void OnUnloaded(const winrt::IInspectable& /*sender*/, const winrt::RoutedEventArgs& /*args*/);
    void OnLayoutUpdated(const winrt::IInspectable& /*sender*/, const winrt::IInspectable& /*args*/);

    void OnDataSourcePropertyChanged(const winrt::ItemsSourceView& oldValue, const winrt::ItemsSourceView& newValue);
    void OnItemTemplateChanged(const winrt::IElementFactory& oldValue, const winrt::IElementFactory& newValue);
    void OnLayoutChanged(const winrt::Layout& oldValue, const winrt::Layout& newValue);
    void OnAnimatorChanged(const winrt::ElementAnimator& oldValue, const winrt::ElementAnimator& newValue);

    void OnItemsSourceViewChanged(const winrt::IInspectable& sender, const winrt::NotifyCollectionChangedEventArgs& args);
    void InvalidateMeasureForLayout(winrt::Layout const& sender, winrt::IInspectable const& args);
    void InvalidateArrangeForLayout(winrt::Layout const& sender, winrt::IInspectable const& args);

    winrt::VirtualizingLayoutContext GetLayoutContext();
    bool IsProcessingCollectionChange() const { return m_processingItemsSourceChange != nullptr; }

    winrt::IIterable<winrt::DependencyObject> CreateChildrenInTabFocusOrderIterable();

    ::AnimationManager m_animationManager{ this };
    ::ViewManager m_viewManager{ this };
    std::shared_ptr<::ViewportManager> m_viewportManager{ nullptr };

    tracker_ref<winrt::ItemsSourceView> m_itemsSourceView{ this };

    winrt::Microsoft::UI::Xaml::Controls::IElementFactoryShim m_itemTemplateWrapper{ nullptr };

    tracker_ref<winrt::VirtualizingLayoutContext> m_layoutContext{ this };
    tracker_ref<winrt::IInspectable> m_layoutState{ this };
    // Value is different from null only while we are on the OnItemsSourceChanged call stack.
    tracker_ref<winrt::NotifyCollectionChangedEventArgs> m_processingItemsSourceChange{ this };

    winrt::Size m_lastAvailableSize{};
    bool m_isLayoutInProgress{ false };
    // The value of _layoutOrigin is expected to be set by the layout
    // when it gets measured. It should not be used outside of measure.
    winrt::Point m_layoutOrigin{};

    // Event revokers
    winrt::ItemsSourceView::CollectionChanged_revoker m_itemsSourceViewChanged{};
    winrt::Layout::MeasureInvalidated_revoker m_measureInvalidated{};
    winrt::Layout::ArrangeInvalidated_revoker m_arrangeInvalidated{};

    // Cached Event args to avoid creation cost every time
    tracker_ref<winrt::ItemsRepeaterElementPreparedEventArgs> m_elementPreparedArgs{ this };
    tracker_ref<winrt::ItemsRepeaterElementClearingEventArgs> m_elementClearingArgs{ this };
    tracker_ref<winrt::ItemsRepeaterElementIndexChangedEventArgs> m_elementIndexChangedArgs{ this };

    // Loaded events fire on the first tick after an element is put into the tree 
    // while unloaded is posted on the UI tree and may be processed out of sync with subsequent loaded
    // events. We keep these counters to detect out-of-sync unloaded events and take action to rectify.
    int _loadedCounter{};
    int _unloadedCounter{};

    // Used to avoid layout cycles with StackLayout layouts where variable sized children prevent
    // the ItemsRepeater's layout to settle.
    uint8_t m_stackLayoutMeasureCounter{ 0u };

    // Bug in framework's reference tracking causes crash during
    // UIAffinityQueue cleanup. To avoid that bug, take a strong ref
    winrt::IElementFactory m_itemTemplate{ nullptr };
    winrt::Layout m_layout{ nullptr };
    winrt::ElementAnimator m_animator{ nullptr };

    // Bug where DataTemplate with no content causes a crash.
    // See: https://github.com/microsoft/microsoft-ui-xaml/issues/776
    // Solution: Have flag that is only true when DataTemplate exists but it is empty.
    bool m_isItemTemplateEmpty{ false };
};
