// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#include <pch.h>
#include <common.h>
#include "ItemsRepeater.common.h"
#include "ItemsRepeater.h"
#include "RepeaterLayoutContext.h"
#include "ChildrenInTabFocusOrderIterable.h"
#include "SharedHelpers.h"
#include "RepeaterAutomationPeer.h"
#include "ViewportManagerWithPlatformFeatures.h"
#include "ViewportManagerDownlevel.h"
#include "RuntimeProfiler.h"
#include "ItemTemplateWrapper.h"

// Change to 'true' to turn on debugging outputs in Output window
bool RepeaterTrace::s_IsDebugOutputEnabled{ false };

winrt::Point ItemsRepeater::ClearedElementsArrangePosition = winrt::Point(-10000.0f, -10000.0f);
winrt::Rect ItemsRepeater::InvalidRect = { -1.f, -1.f, -1.f, -1.f };

ItemsRepeater::ItemsRepeater()
{
    __RP_Marker_ClassById(RuntimeProfiler::ProfId_ItemsRepeater);

    if (SharedHelpers::IsRS5OrHigher())
    {
        m_viewportManager = std::make_shared<ViewportManagerWithPlatformFeatures>(this);
    }
    else
    {
        m_viewportManager = std::make_shared<ViewportManagerDownLevel>(this);
    }

    winrt::AutomationProperties::SetAccessibilityView(*this, winrt::AccessibilityView::Raw);
    if (SharedHelpers::IsRS3OrHigher())
    {
        TabFocusNavigation(winrt::KeyboardNavigationMode::Once);
        XYFocusKeyboardNavigation(winrt::XYFocusKeyboardNavigationMode::Enabled);
    }

    Loaded({ this, &ItemsRepeater::OnLoaded });
    Unloaded({ this, &ItemsRepeater::OnUnloaded });
    LayoutUpdated({ this, &ItemsRepeater::OnLayoutUpdated });

    // Initialize the cached layout to the default value
    auto layout = Layout().as<winrt::VirtualizingLayout>();
    OnLayoutChanged(nullptr, layout);
}

ItemsRepeater::~ItemsRepeater()
{
    m_itemTemplate = nullptr;
    m_animator = nullptr;
    m_layout = nullptr;
}

#pragma region IUIElementOverrides

winrt::AutomationPeer ItemsRepeater::OnCreateAutomationPeer()
{
    return winrt::make<RepeaterAutomationPeer>(*this);
}

#pragma endregion

#pragma region IUIElementOverrides7

winrt::IIterable<winrt::DependencyObject> ItemsRepeater::GetChildrenInTabFocusOrder()
{
    return CreateChildrenInTabFocusOrderIterable();
}

#pragma endregion

#pragma region IUIElementOverrides8

void ItemsRepeater::OnBringIntoViewRequested(winrt::BringIntoViewRequestedEventArgs const& e)
{
    m_viewportManager->OnBringIntoViewRequested(e);
}

#pragma endregion

#pragma region IFrameworkElementOverrides

winrt::Size ItemsRepeater::MeasureOverride(winrt::Size const& availableSize)
{
    if (m_isLayoutInProgress)
    {
        throw winrt::hresult_error(E_FAIL, L"Reentrancy detected during layout.");
    }

    if (IsProcessingCollectionChange())
    {
        throw winrt::hresult_error(E_FAIL, L"Cannot run layout in the middle of a collection change.");
    }

    auto layout = Layout();

    if (layout)
    {
        auto const stackLayout = layout.try_as<winrt::StackLayout>();

        if (stackLayout && ++m_stackLayoutMeasureCounter >= s_maxStackLayoutIterations)
        {
            REPEATER_TRACE_INFO(L"MeasureOverride shortcut - %d\n", m_stackLayoutMeasureCounter);
            // Shortcut the apparent layout cycle by returning the previous desired size.
            // This can occur when children have variable sizes that prevent the ItemsPresenter's desired size from settling.
            const winrt::Rect layoutExtent = m_viewportManager->GetLayoutExtent();
            const winrt::Size desiredSize{ layoutExtent.Width - layoutExtent.X, layoutExtent.Height - layoutExtent.Y };
            return desiredSize;
        }
    }

    m_viewportManager->OnOwnerMeasuring();

    m_isLayoutInProgress = true;
    auto layoutInProgress = gsl::finally([this]()
    {
        m_isLayoutInProgress = false;
    });

    m_viewManager.PrunePinnedElements();
    winrt::Rect extent{};
    winrt::Size desiredSize{};

    if (layout)
    {
        auto layoutContext = GetLayoutContext();

        // Expensive operation, do it only in debug builds.
#ifdef _DEBUG
        auto virtualContext = winrt::get_self<VirtualizingLayoutContext>(layoutContext);
        virtualContext->Indent(Indent());
#endif

        // Checking if we have an data template and it is empty
        if (m_isItemTemplateEmpty) {
            // Has no content, so we will draw nothing and request zero space
            extent = winrt::Rect{ m_layoutOrigin.X, m_layoutOrigin.Y, 0, 0 };
        }
        else {
            desiredSize = layout.Measure(layoutContext, availableSize);
            extent = winrt::Rect{ m_layoutOrigin.X, m_layoutOrigin.Y, desiredSize.Width, desiredSize.Height };
        }

        // Clear auto recycle candidate elements that have not been kept alive by layout - i.e layout did not
        // call GetElementAt(index).
        auto children = Children();
        for (unsigned i = 0u; i < children.Size(); ++i)
        {
            auto element = children.GetAt(i);
            auto virtInfo = GetVirtualizationInfo(element);

            if (virtInfo->Owner() == ElementOwner::Layout &&
                virtInfo->AutoRecycleCandidate() &&
                !virtInfo->KeepAlive())
            {
                REPEATER_TRACE_INFO(L"AutoClear - %d \n", virtInfo->Index());
                ClearElementImpl(element);
            }
        }
    }

    m_viewportManager->SetLayoutExtent(extent);
    m_lastAvailableSize = availableSize;
    return desiredSize;
}

winrt::Size ItemsRepeater::ArrangeOverride(winrt::Size const& finalSize)
{
    if (m_isLayoutInProgress)
    {
        throw winrt::hresult_error(E_FAIL, L"Reentrancy detected during layout.");
    }

    if (IsProcessingCollectionChange())
    {
        throw winrt::hresult_error(E_FAIL, L"Cannot run layout in the middle of a collection change.");
    }

    m_isLayoutInProgress = true;
    auto layoutInProgress = gsl::finally([this]()
    {
        m_isLayoutInProgress = false;
    });

    winrt::Size arrangeSize{};

    if (auto layout = Layout())
    {
        arrangeSize = layout.Arrange(GetLayoutContext(), finalSize);
    }

    // The view manager might clear elements during this call.
    // That's why we call it before arranging cleared elements
    // off screen.
    m_viewManager.OnOwnerArranged();

    auto children = Children();
    for (unsigned i = 0u; i < children.Size(); ++i)
    {
        auto element = children.GetAt(i);
        auto virtInfo = GetVirtualizationInfo(element);
        virtInfo->KeepAlive(false);

        if (virtInfo->Owner() == ElementOwner::ElementFactory ||
            virtInfo->Owner() == ElementOwner::PinnedPool)
        {
            // Toss it away. And arrange it with size 0 so that XYFocus won't use it.
            element.Arrange(winrt::Rect{
                ClearedElementsArrangePosition.X - static_cast<float>(element.DesiredSize().Width),
                ClearedElementsArrangePosition.Y - static_cast<float>(element.DesiredSize().Height),
                0.0f,
                0.0f });
        }
        else
        {
            const auto newBounds = CachedVisualTreeHelpers::GetLayoutSlot(element.as<winrt::FrameworkElement>());

            if (virtInfo->ArrangeBounds() != ItemsRepeater::InvalidRect &&
                newBounds != virtInfo->ArrangeBounds())
            {
                m_animationManager.OnElementBoundsChanged(element, virtInfo->ArrangeBounds(), newBounds);
            }

            virtInfo->ArrangeBounds(newBounds);
        }
    }

    m_viewportManager->OnOwnerArranged();
    m_animationManager.OnOwnerArranged();

    return arrangeSize;
}

#pragma endregion

#pragma region IRepeater interface.

winrt::ItemsSourceView ItemsRepeater::ItemsSourceView()
{
    return m_itemsSourceView.get();
}

int32_t ItemsRepeater::GetElementIndex(winrt::UIElement const& element)
{
    return GetElementIndexImpl(element);
}

winrt::UIElement ItemsRepeater::TryGetElement(int index)
{
    return GetElementFromIndexImpl(index);
}

void ItemsRepeater::PinElement(winrt::UIElement const& element)
{
    m_viewManager.UpdatePin(element, true /* addPin */);
}

void ItemsRepeater::UnpinElement(winrt::UIElement const& element)
{
    m_viewManager.UpdatePin(element, false /* addPin */);
}

winrt::UIElement ItemsRepeater::GetOrCreateElement(int index)
{
    return GetOrCreateElementImpl(index);
}

#pragma endregion

winrt::UIElement ItemsRepeater::GetElementImpl(int index, bool forceCreate, bool suppressAutoRecycle)
{
    auto element = m_viewManager.GetElement(index, forceCreate, suppressAutoRecycle);
    return element;
}

void ItemsRepeater::ClearElementImpl(const winrt::UIElement& element)
{
    // Clearing an element due to a collection change
    // is more strict in that pinned elements will be forcibly
    // unpinned and sent back to the view generator.
    const bool isClearedDueToCollectionChange =
        IsProcessingCollectionChange() &&
        (m_processingItemsSourceChange.get().Action() == winrt::NotifyCollectionChangedAction::Remove ||
            m_processingItemsSourceChange.get().Action() == winrt::NotifyCollectionChangedAction::Replace ||
            m_processingItemsSourceChange.get().Action() == winrt::NotifyCollectionChangedAction::Reset);

    m_viewManager.ClearElement(element, isClearedDueToCollectionChange);
    m_viewportManager->OnElementCleared(element);
}

int ItemsRepeater::GetElementIndexImpl(const winrt::UIElement& element)
{
    // Verify that element is actually a child of this ItemsRepeater
    auto const parent = winrt::VisualTreeHelper::GetParent(element);
    if (parent == *this)
    {
        auto virtInfo = TryGetVirtualizationInfo(element);
        return m_viewManager.GetElementIndex(virtInfo);
    }
    return -1;
}

winrt::UIElement ItemsRepeater::GetElementFromIndexImpl(int index)
{
    winrt::UIElement result = nullptr;

    auto children = Children();
    for (unsigned i = 0u; i < children.Size() && !result; ++i)
    {
        auto element = children.GetAt(i);
        auto virtInfo = TryGetVirtualizationInfo(element);
        if (virtInfo && virtInfo->IsRealized() && virtInfo->Index() == index)
        {
            result = element;
        }
    }

    return result;
}

winrt::UIElement ItemsRepeater::GetOrCreateElementImpl(int index)
{
    if (ItemsSourceView() == nullptr)
    {
       throw winrt::hresult_error(E_FAIL, L"ItemSource doesn't have a value");
    }

    if (index >= 0 && index >= ItemsSourceView().Count())
    {
        throw winrt::hresult_invalid_argument(L"Argument index is invalid.");
    }

    if (m_isLayoutInProgress)
    {
        throw winrt::hresult_error(E_FAIL, L"GetOrCreateElement invocation is not allowed during layout.");
    }

    auto element = GetElementFromIndexImpl(index);
    const bool isAnchorOutsideRealizedRange = !element;

    if (isAnchorOutsideRealizedRange)
    {
        if (!Layout())
        {
            throw winrt::hresult_error(E_FAIL, L"Cannot make an Anchor when there is no attached layout.");
        }

        element = GetLayoutContext().GetOrCreateElementAt(index);
        element.Measure({ std::numeric_limits<float>::infinity(), std::numeric_limits<float>::infinity() });
    }

    m_viewportManager->OnMakeAnchor(element, isAnchorOutsideRealizedRange);
    InvalidateMeasure();

    return element;
}

/*static*/
winrt::com_ptr<VirtualizationInfo> ItemsRepeater::TryGetVirtualizationInfo(const winrt::UIElement& element)
{
    auto value = element.GetValue(GetVirtualizationInfoProperty());
    return winrt::get_self<VirtualizationInfo>(value)->get_strong();
}

/*static*/
winrt::com_ptr<VirtualizationInfo> ItemsRepeater::GetVirtualizationInfo(const winrt::UIElement& element)
{
    auto result = TryGetVirtualizationInfo(element);

    if (!result)
    {
        result = CreateAndInitializeVirtualizationInfo(element);
    }

    return result;
}

/* static */
winrt::com_ptr<VirtualizationInfo> ItemsRepeater::CreateAndInitializeVirtualizationInfo(const winrt::UIElement& element)
{
    MUX_ASSERT(!TryGetVirtualizationInfo(element));
    auto result = winrt::make_self<VirtualizationInfo>();
    element.SetValue(GetVirtualizationInfoProperty(), result.as<winrt::IInspectable>());
    return result;
}

void ItemsRepeater::OnPropertyChanged(const winrt::DependencyPropertyChangedEventArgs& args)
{
    winrt::IDependencyProperty property = args.Property();

    if (property == s_ItemsSourceProperty)
    {
        if (args.NewValue() != args.OldValue())
        {
            auto newValue = args.NewValue();
            auto newDataSource = newValue.try_as<winrt::ItemsSourceView>();
            if (newValue && !newDataSource)
            {
                newDataSource = winrt::ItemsSourceView(newValue);
            }

            OnDataSourcePropertyChanged(m_itemsSourceView.get(), newDataSource);
        }
    }
    else if (property == s_ItemTemplateProperty)
    {
        OnItemTemplateChanged(args.OldValue().as<winrt::IElementFactory>(), args.NewValue().as<winrt::IElementFactory>());
    }
    else if (property == s_LayoutProperty)
    {
        OnLayoutChanged(args.OldValue().as<winrt::Layout>(), args.NewValue().as<winrt::Layout>());
    }
    else if (property == s_AnimatorProperty)
    {
        OnAnimatorChanged(args.OldValue().as<winrt::ElementAnimator>(), args.NewValue().as<winrt::ElementAnimator>());
    }
    else if (property == s_HorizontalCacheLengthProperty)
    {
        m_viewportManager->HorizontalCacheLength(unbox_value<double>(args.NewValue()));
    }
    else if (property == s_VerticalCacheLengthProperty)
    {
        m_viewportManager->VerticalCacheLength(unbox_value<double>(args.NewValue()));
    }
}

void ItemsRepeater::OnElementPrepared(const winrt::UIElement& element, int index)
{
    m_viewportManager->OnElementPrepared(element);
    if (m_elementPreparedEventSource)
    {
        if (!m_elementPreparedArgs)
        {
            m_elementPreparedArgs = tracker_ref<winrt::ItemsRepeaterElementPreparedEventArgs>(this, winrt::make<ItemsRepeaterElementPreparedEventArgs>(element, index));
        }
        else
        {
            winrt::get_self<ItemsRepeaterElementPreparedEventArgs>(m_elementPreparedArgs.get())->Update(element, index);
        }

        m_elementPreparedEventSource(*this, m_elementPreparedArgs.get());
    }
}

void ItemsRepeater::OnElementClearing(const winrt::UIElement& element)
{
    if (m_elementClearingEventSource)
    {
        if (!m_elementClearingArgs)
        {
            m_elementClearingArgs = tracker_ref<winrt::ItemsRepeaterElementClearingEventArgs>(this, winrt::make<ItemsRepeaterElementClearingEventArgs>(element));
        }
        else
        {
            winrt::get_self<ItemsRepeaterElementClearingEventArgs>(m_elementClearingArgs.get())->Update(element);
        }

        m_elementClearingEventSource(*this, m_elementClearingArgs.get());
    }
}

void ItemsRepeater::OnElementIndexChanged(const winrt::UIElement& element, int oldIndex, int newIndex)
{
    if (m_elementIndexChangedEventSource)
    {
        if (!m_elementIndexChangedArgs)
        {
            m_elementIndexChangedArgs = tracker_ref<winrt::ItemsRepeaterElementIndexChangedEventArgs>(this, winrt::make<ItemsRepeaterElementIndexChangedEventArgs>(element, oldIndex, newIndex));
        }
        else
        {
            winrt::get_self<ItemsRepeaterElementIndexChangedEventArgs>(m_elementIndexChangedArgs.get())->Update(element, oldIndex, newIndex);
        }

        m_elementIndexChangedEventSource(*this, m_elementIndexChangedArgs.get());
    }
}

// Provides an indentation based on repeater elements in the UI Tree that
// can be used to make logging a little easier to read.
int ItemsRepeater::Indent()
{
#ifdef _DEBUG
    // Expensive, so we do it only in debug builds.
    auto parent = this->Parent().as<winrt::FrameworkElement>();
    while (parent && !parent.try_as<winrt::ItemsRepeater>())
    {
        parent = parent.Parent().as<winrt::FrameworkElement>();
    }

    if (parent)
    {
        auto parentRepeater = winrt::get_self<ItemsRepeater>(parent.as<winrt::ItemsRepeater>());
        return parentRepeater->Indent() * 4;
    }
#endif

    return 4;
}

void ItemsRepeater::OnLoaded(const winrt::IInspectable& /*sender*/, const winrt::RoutedEventArgs& /*args*/)
{
    // If we skipped an unload event, reset the scrollers now and invalidate measure so that we get a new
    // layout pass during which we will hookup new scrollers.
    if (_loadedCounter > _unloadedCounter)
    {
        InvalidateMeasure();
        m_viewportManager->ResetScrollers();
    }
    ++_loadedCounter;
}

void ItemsRepeater::OnUnloaded(const winrt::IInspectable& /*sender*/, const winrt::RoutedEventArgs& /*args*/)
{
    m_stackLayoutMeasureCounter = 0u;

    ++_unloadedCounter;
    // Only reset the scrollers if this unload event is in-sync.
    if (_unloadedCounter == _loadedCounter)
    {
        m_viewportManager->ResetScrollers();
    }
}

void ItemsRepeater::OnLayoutUpdated(const winrt::IInspectable& /*sender*/, const winrt::IInspectable& /*args*/)
{
    // Now that the layout has settled, reset the measure counter to detect the next potential StackLayout layout cycle.
    m_stackLayoutMeasureCounter = 0u;
}

void ItemsRepeater::OnDataSourcePropertyChanged(const winrt::ItemsSourceView& oldValue, const winrt::ItemsSourceView& newValue)
{
    if (m_isLayoutInProgress)
    {
        throw winrt::hresult_error(E_FAIL, L"Cannot set ItemsSourceView during layout.");
    }

    m_itemsSourceView.set(newValue);

    if (oldValue)
    {
        m_itemsSourceViewChanged.revoke();
    }

    if (newValue)
    {
        m_itemsSourceViewChanged = newValue.CollectionChanged(winrt::auto_revoke, { this, &ItemsRepeater::OnItemsSourceViewChanged });
    }

    if (auto const layout = Layout())
    {
        auto const args = winrt::NotifyCollectionChangedEventArgs(
            winrt::NotifyCollectionChangedAction::Reset,
            nullptr /* newItems */,
            nullptr /* oldItems */,
            -1 /* newIndex */,
            -1 /* oldIndex */);
        args.Action();
        auto const processingChange = gsl::finally([this]()
            {
                m_processingItemsSourceChange.set(nullptr);
            });
        m_processingItemsSourceChange.set(args);

        if (auto const virtualLayout = layout.try_as<winrt::VirtualizingLayout>())
        {
            virtualLayout.OnItemsChangedCore(GetLayoutContext(), newValue, args);
        }
        else if (auto const nonVirtualLayout = layout.try_as<winrt::NonVirtualizingLayout>())
        {
            // Walk through all the elements and make sure they are cleared for
            // non-virtualizing layouts.
            for (const auto& element: Children())
            {
                if (GetVirtualizationInfo(element)->IsRealized())
                {
                    ClearElementImpl(element);
                }
            }

            Children().Clear();
        }

        InvalidateMeasure();
    }
}

void ItemsRepeater::OnItemTemplateChanged(const winrt::IElementFactory& oldValue, const winrt::IElementFactory& newValue)
{
    if (m_isLayoutInProgress && oldValue)
    {
        throw winrt::hresult_error(E_FAIL, L"ItemTemplate cannot be changed during layout.");
    }

    // Since the ItemTemplate has changed, we need to re-evaluate all the items that
    // have already been created and are now in the tree. The easiest way to do that
    // would be to do a reset.. Note that this has to be done before we change the template
    // so that the cleared elements go back into the old template.
    if (auto const layout = Layout())
    {
        auto const args = winrt::NotifyCollectionChangedEventArgs(
            winrt::NotifyCollectionChangedAction::Reset,
            nullptr /* newItems */,
            nullptr /* oldItems */,
            -1 /* newIndex */,
            -1 /* oldIndex */);
        args.Action();
        auto const processingChange = gsl::finally([this]()
            {
                m_processingItemsSourceChange.set(nullptr);
            });
        m_processingItemsSourceChange.set(args);

        if (auto const virtualLayout = layout.try_as<winrt::VirtualizingLayout>())
        {
            virtualLayout.OnItemsChangedCore(GetLayoutContext(), newValue, args);
        }
        else if (auto const nonVirtualLayout = layout.try_as<winrt::NonVirtualizingLayout>())
        {
            // Walk through all the elements and make sure they are cleared for
            // non-virtualizing layouts.
            for (auto const& child : Children())
            {
                if (GetVirtualizationInfo(child)->IsRealized())
                {
                    ClearElementImpl(child);
                }
            }
        }
    }

    if (!SharedHelpers::IsRS5OrHigher())
    {
        // Bug in framework's reference tracking causes crash during
        // UIAffinityQueue cleanup. To avoid that bug, take a strong ref
        m_itemTemplate = newValue;
    }
    // Clear flag for bug #776
    m_isItemTemplateEmpty = false;
    m_itemTemplateWrapper = newValue.try_as<winrt::IElementFactoryShim>();
    if (!m_itemTemplateWrapper)
    {
        // ItemTemplate set does not implement IElementFactoryShim. We also 
        // want to support DataTemplate and DataTemplateSelectors automagically.
        if (auto dataTemplate = newValue.try_as<winrt::DataTemplate>())
        {
            m_itemTemplateWrapper = winrt::make<ItemTemplateWrapper>(dataTemplate);
            if (auto content = dataTemplate.LoadContent().as<winrt::FrameworkElement>())
            {
                // Due to bug https://github.com/microsoft/microsoft-ui-xaml/issues/3057, we need to get the framework
                // to take ownership of the extra implicit ref that was returned by LoadContent. The simplest way to do
                // this is to add it to a Children collection and immediately remove it.
                auto children = Children();
                children.Append(content);
                children.RemoveAtEnd();
            }
            else
            {
                // We have a DataTemplate which is empty, so we need to set it to true
                m_isItemTemplateEmpty = true;
            }
        }
        else if (auto selector = newValue.try_as<winrt::DataTemplateSelector>())
        {
            m_itemTemplateWrapper = winrt::make<ItemTemplateWrapper>(selector);
        }
        else
        {
            throw winrt::hresult_invalid_argument(L"ItemTemplate");
        }
    }

    InvalidateMeasure();
}

void ItemsRepeater::OnLayoutChanged(const winrt::Layout& oldValue, const winrt::Layout& newValue)
{
    if (m_isLayoutInProgress)
    {
        throw winrt::hresult_error(E_FAIL, L"Layout cannot be changed during layout.");
    }

    m_viewManager.OnLayoutChanging();
    m_animationManager.OnLayoutChanging();

    if (oldValue)
    {
        oldValue.UninitializeForContext(GetLayoutContext());
        m_measureInvalidated.revoke();
        m_arrangeInvalidated.revoke();
        m_stackLayoutMeasureCounter = 0u;
        
        // Walk through all the elements and make sure they are cleared
        auto children = Children();
        for (unsigned i = 0u; i < children.Size(); ++i)
        {
            auto element = children.GetAt(i);
            if (GetVirtualizationInfo(element)->IsRealized())
            {
                ClearElementImpl(element);
            }
        }

        m_layoutState.set(nullptr);
    }

    if (!SharedHelpers::IsRS5OrHigher())
    {
        // Bug in framework's reference tracking causes crash during
        // UIAffinityQueue cleanup. To avoid that bug, take a strong ref
        m_layout = newValue;
    }

    if (newValue)
    {
        newValue.InitializeForContext(GetLayoutContext());
        m_measureInvalidated = newValue.MeasureInvalidated(winrt::auto_revoke, { this, &ItemsRepeater::InvalidateMeasureForLayout });
        m_arrangeInvalidated = newValue.ArrangeInvalidated(winrt::auto_revoke, { this, &ItemsRepeater::InvalidateArrangeForLayout });
    }

    bool isVirtualizingLayout = newValue != nullptr && newValue.try_as<winrt::VirtualizingLayout>() != nullptr;
    m_viewportManager->OnLayoutChanged(isVirtualizingLayout);
    InvalidateMeasure();
}

void ItemsRepeater::OnAnimatorChanged(const winrt::ElementAnimator& /* oldValue */, const winrt::ElementAnimator& newValue)
{
    m_animationManager.OnAnimatorChanged(newValue);
    if (!SharedHelpers::IsRS5OrHigher())
    {
        // Bug in framework's reference tracking causes crash during
        // UIAffinityQueue cleanup. To avoid that bug, take a strong ref
        m_animator = newValue;
    }
}

void ItemsRepeater::OnItemsSourceViewChanged(const winrt::IInspectable& sender, const winrt::NotifyCollectionChangedEventArgs& args)
{
    if (m_isLayoutInProgress)
    {
        // Bad things will follow if the data changes while we are in the middle of a layout pass.
        throw winrt::hresult_error(E_FAIL, L"Changes in data source are not allowed during layout.");
    }

    if (IsProcessingCollectionChange())
    {
        throw winrt::hresult_error(E_FAIL, L"Changes in the data source are not allowed during another change in the data source.");
    }

    m_processingItemsSourceChange.set(args);
    auto processingChange = gsl::finally([this]()
    {
        m_processingItemsSourceChange.set(nullptr);
    });

    m_animationManager.OnItemsSourceChanged(sender, args);
    m_viewManager.OnItemsSourceChanged(sender, args);

    if (auto layout = Layout())
    {
        if (auto virtualLayout = layout.try_as<winrt::VirtualizingLayout>())
        {
            virtualLayout.OnItemsChangedCore(GetLayoutContext(), sender, args);
        }
        else
        {
            // NonVirtualizingLayout
            InvalidateMeasure();
        }
    }
}

void ItemsRepeater::InvalidateMeasureForLayout(winrt::Layout const&, winrt::IInspectable const&)
{
    InvalidateMeasure();
}

void ItemsRepeater::InvalidateArrangeForLayout(winrt::Layout const&, winrt::IInspectable const&)
{
    InvalidateArrange();
}

winrt::VirtualizingLayoutContext ItemsRepeater::GetLayoutContext()
{
    if (!m_layoutContext)
    {
        m_layoutContext.set(winrt::make<RepeaterLayoutContext>(*this));
    }
    return m_layoutContext.get();
}

winrt::IIterable<winrt::DependencyObject> ItemsRepeater::CreateChildrenInTabFocusOrderIterable()
{
    auto children = Children();
    if (children.Size() > 0u)
    {
        return winrt::make<ChildrenInTabFocusOrderIterable>(*this);
    }
    return nullptr;
}
