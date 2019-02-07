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
#include "ViewportManager.h"

class VirtualizationInfo;

class ItemsRepeater :
    public ReferenceTracker<ItemsRepeater, DeriveFromPanelHelper_base, winrt::ItemsRepeater, winrt::IItemsRepeater2>
{
public:
    ItemsRepeater();
    ~ItemsRepeater();

    static winrt::Point ClearedElementsArrangePosition;
    // A convention we use in the ItemsRepeater codebase for an invalid Rect value.
    static winrt::Rect InvalidRect;

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

    winrt::IInspectable ItemsSource();
    void ItemsSource(winrt::IInspectable const& value);

    winrt::ItemsSourceView ItemsSourceView();
    
    winrt::IElementFactory ItemTemplate();
    void ItemTemplate(winrt::IElementFactory const& value);

    winrt::Layout Layout();
    void Layout(winrt::Layout const& value);

    winrt::ElementAnimator Animator();
    void Animator(winrt::ElementAnimator const& value);

    double HorizontalCacheLength();
    void HorizontalCacheLength(double value);

    double VerticalCacheLength();
    void VerticalCacheLength(double value);

    winrt::Brush Background();
    void Background(winrt::Brush const& value);

    // Mapping APIs
    int32_t GetElementIndex(winrt::UIElement const& element);
    winrt::UIElement TryGetElement(int index);
    winrt::UIElement GetOrCreateElement(int index);

    // Element events
    winrt::event_token ElementPrepared(winrt::TypedEventHandler<winrt::ItemsRepeater, winrt::ItemsRepeaterElementPreparedEventArgs> const& value);
    void ElementPrepared(winrt::event_token const& token);

    winrt::event_token ElementClearing(winrt::TypedEventHandler<winrt::ItemsRepeater, winrt::ItemsRepeaterElementClearingEventArgs> const& value);
    void ElementClearing(winrt::event_token const& token);

    winrt::event_token ElementIndexChanged(winrt::TypedEventHandler<winrt::ItemsRepeater, winrt::ItemsRepeaterElementIndexChangedEventArgs> const& value);
    void ElementIndexChanged(winrt::event_token const& token);
#pragma endregion

#ifndef BUILD_WINDOWS
    winrt::Microsoft::UI::Xaml::Controls::IElementFactoryShim ItemTemplateShim() { return m_itemTemplateWrapper; };
#else
    winrt::IElementFactory ItemTemplateShim() { return m_itemTemplate; };
#endif

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
        return s_VirtualizationInfoProperty;
    }

    static winrt::DependencyProperty ItemsSourceProperty() { return s_itemsSourceProperty; }
    static winrt::DependencyProperty ItemTemplateProperty() { return s_itemTemplateProperty; }
    static winrt::DependencyProperty LayoutProperty() { return s_layoutProperty; }
    static winrt::DependencyProperty AnimatorProperty() { return s_animatorProperty; }

    static winrt::DependencyProperty HorizontalCacheLengthProperty() { return s_horizontalCacheLengthProperty; }
    static winrt::DependencyProperty VerticalCacheLengthProperty() { return s_verticalCacheLengthProperty; }

    static winrt::DependencyProperty BackgroundProperty() { return winrt::Panel::BackgroundProperty(); }

    static GlobalDependencyProperty s_itemsSourceProperty;
    static GlobalDependencyProperty s_itemTemplateProperty;
    static GlobalDependencyProperty s_layoutProperty;
    static GlobalDependencyProperty s_animatorProperty;
    static GlobalDependencyProperty s_horizontalCacheLengthProperty;
    static GlobalDependencyProperty s_verticalCacheLengthProperty;

    static void EnsureProperties();
    static void ClearProperties();

private:
    static void ItemsRepeater::OnPropertyChanged(
        const winrt::DependencyObject& sender,
        const winrt::DependencyPropertyChangedEventArgs& args);

    static GlobalDependencyProperty s_VirtualizationInfoProperty;

    void OnLoaded(const winrt::IInspectable& /*sender*/, const winrt::RoutedEventArgs& /*args*/);
    void OnUnloaded(const winrt::IInspectable& /*sender*/, const winrt::RoutedEventArgs& /*args*/);

    void OnDataSourcePropertyChanged(const winrt::ItemsSourceView& oldValue, const winrt::ItemsSourceView& newValue);
    void OnItemTemplateChanged(const winrt::IElementFactory& oldValue, const winrt::IElementFactory& newValue);
    void OnLayoutChanged(const winrt::Layout& oldValue, const winrt::Layout& newValue);
    void OnAnimatorChanged(const winrt::ElementAnimator& oldValue, const winrt::ElementAnimator& newValue);

    void OnDataSourceChanged(const winrt::IInspectable& sender, const winrt::NotifyCollectionChangedEventArgs& args);
    void InvalidateMeasureForLayout(winrt::Layout const& sender, winrt::IInspectable const& args);
    void InvalidateArrangeForLayout(winrt::Layout const& sender, winrt::IInspectable const& args);

    winrt::VirtualizingLayoutContext GetLayoutContext();
    bool IsProcessingCollectionChange() const { return m_processingDataSourceChange != nullptr; }

    winrt::IIterable<winrt::DependencyObject> CreateChildrenInTabFocusOrderIterable();

    ::AnimationManager m_animationManager{ this };
    ::ViewManager m_viewManager{ this };
    std::shared_ptr<::ViewportManager> m_viewportManager{ nullptr };

    tracker_ref<winrt::ItemsSourceView> m_dataSource{ this };
    winrt::IElementFactory m_itemTemplate{ nullptr };

#ifndef BUILD_WINDOWS
    winrt::Microsoft::UI::Xaml::Controls::IElementFactoryShim m_itemTemplateWrapper{ nullptr };
#endif

    winrt::Layout m_layout{ nullptr };
    winrt::ElementAnimator m_animator{ nullptr };

    tracker_ref<winrt::VirtualizingLayoutContext> m_layoutContext{ this };
    tracker_ref<winrt::IInspectable> m_layoutState{ this };
    // Value is different from null only while we are on the OnDataSourceChanged call stack.
    tracker_ref<winrt::NotifyCollectionChangedEventArgs> m_processingDataSourceChange{ this };

    winrt::Size m_lastAvailableSize{};
    bool m_isLayoutInProgress{ false };
    // The value of _layoutOrigin is expected to be set by the layout
    // when it gets measured. It should not be used outside of measure.
    winrt::Point m_layoutOrigin{};

    // Event tokens
    winrt::event_token m_dataSourceChanged{};
    winrt::event_token m_measureInvalidated{};
    winrt::event_token m_arrangeInvalidated{};

    event_source<winrt::TypedEventHandler<winrt::ItemsRepeater, winrt::ItemsRepeaterElementPreparedEventArgs>> m_elementPreparedEventSource{ this };
    event_source<winrt::TypedEventHandler<winrt::ItemsRepeater, winrt::ItemsRepeaterElementClearingEventArgs>> m_elementClearingEventSource{ this };
    event_source<winrt::TypedEventHandler<winrt::ItemsRepeater, winrt::ItemsRepeaterElementIndexChangedEventArgs>> m_elementIndexChangedEventSource{ this };

    // Cached Event args to avoid creation cost every time
    tracker_ref<winrt::ItemsRepeaterElementPreparedEventArgs> m_elementPreparedArgs{ this };
    tracker_ref<winrt::ItemsRepeaterElementClearingEventArgs> m_elementClearingArgs{ this };
    tracker_ref<winrt::ItemsRepeaterElementIndexChangedEventArgs> m_elementIndexChangedArgs{ this };

    // Loaded events fire on the first tick after an element is put into the tree 
    // while unloaded is posted on the UI tree and may be processed out of sync with subsequent loaded
    // events. We keep these counters to detect out-of-sync unloaded events and take action to rectify.
    int _loadedCounter{};
    int _unloadedCounter{};
};
