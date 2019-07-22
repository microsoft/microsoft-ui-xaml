// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#pragma once

#include "UniqueIdElementPool.h"
#include "VirtualizationInfo.h"
#include "Phaser.h"

class ItemsRepeater;

// Manages elements on behalf of ItemsRepeater.
// ViewManager automatically pins focused elements.
class ViewManager final
{
public:
    ViewManager(ItemsRepeater* owner);

    winrt::UIElement GetElement(int index, bool forceCreate, bool suppressAutoRecycle);
    void ClearElement(const winrt::UIElement& element, bool isClearedDueToCollectionChange);
    void ClearElementToElementFactory(const winrt::UIElement& element);
    int GetElementIndex(const winrt::com_ptr<VirtualizationInfo>& virtInfo);

    void PrunePinnedElements();
    void UpdatePin(const winrt::UIElement& element, bool addPin);

    void OnItemsSourceChanged(const winrt::IInspectable& source, const winrt::NotifyCollectionChangedEventArgs& args);
    void OnLayoutChanging();
    void OnOwnerArranged();

private:
#pragma region GetElement providers

    winrt::UIElement GetElementIfAlreadyHeldByLayout(int index);
    winrt::UIElement GetElementFromUniqueIdResetPool(int index);
    winrt::UIElement GetElementFromPinnedElements(int index);
    winrt::UIElement GetElementFromElementFactory(int index);

#pragma endregion

#pragma region RecycleElement handlers

    bool ClearElementToUniqueIdResetPool(const winrt::UIElement& element, const winrt::com_ptr<VirtualizationInfo>& virtInfo);
    bool ClearElementToAnimator(const winrt::UIElement& element, const winrt::com_ptr<VirtualizationInfo>& virtInfo);
    bool ClearElementToPinnedPool(const winrt::UIElement& element, const winrt::com_ptr<VirtualizationInfo>& virtInfo, bool isClearedDueToCollectionChange);

#pragma endregion

    void UpdateFocusedElement();
    void OnFocusChanged(const winrt::IInspectable& sender, const winrt::RoutedEventArgs& args);
    void MoveFocusFromClearedIndex(int clearedIndex);
    winrt::Control FindFocusCandidate(int clearedIndex, winrt::UIElement& focusedChild);

    void EnsureEventSubscriptions();

    void UpdateElementIndex(const winrt::UIElement& element, const winrt::com_ptr<VirtualizationInfo>& virtInfo, int index);

    void InvalidateRealizedIndicesHeldByLayout();
    void EnsureFirstLastRealizedIndices();

    struct PinnedElementInfo
    {
        PinnedElementInfo(const ITrackerHandleManager* owner, const winrt::UIElement& element);

        [[nodiscard]] winrt::UIElement PinnedElement() const { return m_pinnedElement.get(); }
        [[nodiscard]] winrt::com_ptr<VirtualizationInfo> VirtualizationInfo() const { return m_virtInfo.get(); }

    private:
        tracker_ref<winrt::UIElement> m_pinnedElement;

        // We hold on VirtualizationInfo to make sure we can
        // quickly access its content rather than go through
        // ItemsRepeater.GetVirtualizationInfo(element) which is
        // slower (assuming it's implemented using attached
        // properties).
        tracker_com_ref<::VirtualizationInfo> m_virtInfo;
    };

    ItemsRepeater* m_owner{ nullptr };

    // Pinned elements that are currently owned by layout are *NOT* in this pool.
    std::vector<PinnedElementInfo> m_pinnedPool;
    UniqueIdElementPool m_resetPool;

    // _lastFocusedElement is listed in _pinnedPool.
    // It has to be an element we own (i.e. a direct child).
    tracker_ref<winrt::UIElement> m_lastFocusedElement;
    bool m_isDataSourceStableResetPending{};

    // Event tokens
    winrt::UIElement::GotFocus_revoker m_gotFocus{};
    winrt::UIElement::LostFocus_revoker m_lostFocus{};

    ::Phaser m_phaser;

    // Cached generate/clear contexts to avoid cost of creation every time.
#ifdef BUILD_WINDOWS
    tracker_ref<winrt::Windows::UI::Xaml::ElementFactoryGetArgs> m_ElementFactoryGetArgs;
    tracker_ref<winrt::Windows::UI::Xaml::ElementFactoryRecycleArgs> m_ElementFactoryRecycleArgs;
#else
    tracker_ref<winrt::Microsoft::UI::Xaml::Controls::ElementFactoryGetArgs> m_ElementFactoryGetArgs;
    tracker_ref<winrt::Microsoft::UI::Xaml::Controls::ElementFactoryRecycleArgs> m_ElementFactoryRecycleArgs;
#endif

    // These are first/last indices requested by layout and not cleared yet.
    // These are also not truly first / last because they are a lower / upper bound on the known realized range.
    // For example, if we didn't have the optimization in ElementManager.cpp, m_lastRealizedElementIndexHeldByLayout 
    // will not be accurate. Rather, it will be an upper bound on what we think is the last realized index.
    int m_firstRealizedElementIndexHeldByLayout{ FirstRealizedElementIndexDefault };
    int m_lastRealizedElementIndexHeldByLayout{ LastRealizedElementIndexDefault };
    static constexpr int FirstRealizedElementIndexDefault = std::numeric_limits<int>::max();
    static constexpr int LastRealizedElementIndexDefault = std::numeric_limits<int>::min();
};
